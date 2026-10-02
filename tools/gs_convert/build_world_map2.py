#!/usr/bin/env python3
"""Build the redesigned world map (GS Stage 28.10).

R1: placement. Reads the per-region 28x15 MAPSEC grids, moves each cluster in
world_map_layout.py to its world destination and writes, to tools/gs_convert/out/world_map/:
  world_grid.json    world MAPSEC grid (rows of MAPSEC names)
  region_grid.json   world region grid (0 = open sea, 1.. = REGIONS order)
  clusters.json      cluster table (source rect, destination, offset, lock unit, MAPSECs)
  grid_preview.png   one colour per region, MAPSEC cells outlined (review only)

Checks: cluster rects cover every source cell once; no two clusters overlap; clusters stay in
the world and in 64x64; every MAPSEC keeps the cell count and shape it has in the 28.3 world
grid; the cursor can reach every unlocked MAPSEC from every other one in every lock state.

Usage: build_world_map2.py [--print]
"""

import argparse
import itertools
import json
import os
import re
import struct
import sys
import zlib

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from common import ROOT, OUT_DIR  # noqa: E402
import world_map_layout as L  # noqa: E402

DATA = os.path.join(ROOT, "src", "data", "region_map")
STAGE = os.path.join(OUT_DIR, "world_map")
GRID_W, GRID_H = 28, 15
BG_MAX = 64                    # text BG size 3 (64x64 tiles)
NONE = "MAPSEC_NONE"
CELL_PX = 16                   # preview scale

LOCK_UNITS = ["JOHTO", "KANTO", "HOENN", "SEVII123", "SEVII4567"]

# Preview colours: (MAPSEC cell, owned sea) per lock unit.
COLORS = {
    "JOHTO": ((214, 170, 92), (58, 58, 70)),
    "KANTO": ((112, 186, 108), (40, 66, 70)),
    "HOENN": ((92, 168, 214), (36, 56, 88)),
    "SEVII123": ((222, 206, 140), (62, 60, 80)),
    "SEVII4567": ((196, 146, 196), (62, 60, 80)),
}
OPEN_SEA = (20, 32, 64)
OUTLINE = (16, 16, 24)
GRID_LINE = (28, 42, 76)


def parse_grid(path):
    rows = [re.findall(r"MAPSEC_\w+", ln) for ln in open(path)
            if ln.strip().startswith("{") and "MAPSEC_" in ln]
    return rows


def write_rgb_png(path, rows):
    h, w = len(rows), len(rows[0])
    raw = bytearray()
    for r in rows:
        raw.append(0)
        for c in r:
            raw += bytes(c)

    def chunk(t, b):
        return struct.pack(">I", len(b)) + t + b + struct.pack(">I", zlib.crc32(t + b) & 0xFFFFFFFF)

    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
                + chunk(b"IDAT", zlib.compress(bytes(raw), 9)) + chunk(b"IEND", b""))


def shape(cells):
    x0 = min(c[0] for c in cells)
    y0 = min(c[1] for c in cells)
    return frozenset((x - x0, y - y0) for x, y in cells)


def place(errors):
    """Move every cluster to the world. Returns (world grid, unit grid, cluster records)."""
    grids = {}
    for src, (fn, _) in L.SOURCES.items():
        g = parse_grid(os.path.join(DATA, fn))
        if len(g) != GRID_H or any(len(r) != GRID_W for r in g):
            sys.exit("%s is not %dx%d" % (fn, GRID_W, GRID_H))
        grids[src] = g

    if L.WORLD_W > BG_MAX or L.WORLD_H > BG_MAX:
        errors.append("world %dx%d exceeds %dx%d" % (L.WORLD_W, L.WORLD_H, BG_MAX, BG_MAX))

    # Every occupied source cell lies in exactly one cluster rect.
    covered = {}
    for name, src, (sx, sy, sw, sh), _, _ in L.CLUSTERS:
        if sx < 0 or sy < 0 or sx + sw > GRID_W or sy + sh > GRID_H:
            errors.append("%s: source rect outside the %s grid" % (name, src))
        for y in range(sy, sy + sh):
            for x in range(sx, sx + sw):
                if (src, x, y) in covered:
                    errors.append("%s: source rect overlaps %s at (%d,%d)" % (name, covered[(src, x, y)], x, y))
                covered[(src, x, y)] = name
    for src, g in grids.items():
        for y in range(GRID_H):
            for x in range(GRID_W):
                if g[y][x] != NONE and (src, x, y) not in covered:
                    errors.append("%s (%d,%d) %s is in no cluster" % (src, x, y, g[y][x]))

    world = [[NONE] * L.WORLD_W for _ in range(L.WORLD_H)]
    unit = [[None] * L.WORLD_W for _ in range(L.WORLD_H)]
    owner = {}
    records = []
    for name, src, (sx, sy, sw, sh), (dx, dy), lock in L.CLUSTERS:
        region = L.SOURCES[src][1]
        if dx < 0 or dy < 0 or dx + sw > L.WORLD_W or dy + sh > L.WORLD_H:
            errors.append("%s: destination rect (%d,%d %dx%d) outside the world" % (name, dx, dy, sw, sh))
            continue
        cells, secs = [], []
        for y in range(sh):
            for x in range(sw):
                tok = grids[src][sy + y][sx + x]
                if tok == NONE:
                    continue
                wx, wy = dx + x, dy + y
                if (wx, wy) in owner:
                    errors.append("%s overlaps %s at world (%d,%d)" % (name, owner[(wx, wy)], wx, wy))
                    continue
                owner[(wx, wy)] = name
                world[wy][wx] = tok
                unit[wy][wx] = lock
                cells.append((wx, wy))
                if tok not in secs:
                    secs.append(tok)
        records.append({
            "name": name, "source": src, "region": region, "lock": lock,
            "src_rect": [sx, sy, sw, sh], "dest": [dx, dy],
            "offset": [dx - sx, dy - sy], "cells": len(cells), "mapsecs": secs,
        })

    # Clusters keep a sea gap unless listed as touching.
    touch_ok = {frozenset(p) for p in L.TOUCHING}
    seen = set()
    for (x, y), a in owner.items():
        for ddx, ddy in itertools.product((-1, 0, 1), repeat=2):
            b = owner.get((x + ddx, y + ddy))
            if b and b != a and frozenset((a, b)) not in touch_ok and frozenset((a, b)) not in seen:
                seen.add(frozenset((a, b)))
                print("warning: %s touches %s at world (%d,%d)" % (a, b, x, y))
    return world, unit, records


def check_mapsecs(world, errors):
    """Every MAPSEC keeps its 28.3 cell count and shape."""
    old = parse_grid(os.path.join(DATA, "region_map_layout_world.h"))

    def cellsets(g):
        out = {}
        for y, row in enumerate(g):
            for x, tok in enumerate(row):
                if tok != NONE:
                    out.setdefault(tok, []).append((x, y))
        return out

    a, b = cellsets(old), cellsets(world)
    for tok in sorted(set(a) | set(b)):
        if tok not in b:
            errors.append("%s lost (was %d cells)" % (tok, len(a[tok])))
        elif tok not in a:
            errors.append("%s is new in the world grid" % tok)
        elif len(a[tok]) != len(b[tok]):
            errors.append("%s cell count %d -> %d" % (tok, len(a[tok]), len(b[tok])))
        elif shape(a[tok]) != shape(b[tok]):
            errors.append("%s changed shape" % tok)
    return len(b)


def territory(unit):
    """Lock unit per cell: MAPSEC cells, sea within TERRITORY_MARGIN, and enclosed sea."""
    W, H = L.WORLD_W, L.WORLD_H
    occ = [(x, y, unit[y][x]) for y in range(H) for x in range(W) if unit[y][x]]
    order = {u: i for i, u in enumerate(LOCK_UNITS)}

    def nearest(x, y):
        return min(((max(abs(x - ox), abs(y - oy)), order[u], u) for ox, oy, u in occ))

    terr = [[None] * W for _ in range(H)]
    for y in range(H):
        for x in range(W):
            d, _, u = nearest(x, y)
            if d <= L.TERRITORY_MARGIN:
                terr[y][x] = u

    # Open-sea pockets that do not reach the world edge belong to the nearest unit.
    seen = set()
    for y in range(H):
        for x in range(W):
            if terr[y][x] or (x, y) in seen:
                continue
            comp, stack, edge = [], [(x, y)], False
            seen.add((x, y))
            while stack:
                cx, cy = stack.pop()
                comp.append((cx, cy))
                edge |= cx in (0, W - 1) or cy in (0, H - 1)
                for nx, ny in ((cx + 1, cy), (cx - 1, cy), (cx, cy + 1), (cx, cy - 1)):
                    if 0 <= nx < W and 0 <= ny < H and not terr[ny][nx] and (nx, ny) not in seen:
                        seen.add((nx, ny))
                        stack.append((nx, ny))
            if not edge:
                for cx, cy in comp:
                    terr[cy][cx] = nearest(cx, cy)[2]
    return terr


def check_reach(world, unit, terr, errors):
    """In every lock state, each unlocked MAPSEC cell reaches every other (4-way moves).

    Blocked: sea owned by a locked region, and MAPSEC cells of a locked unit. A Sevii unit only
    blocks its own MAPSEC cells; Sevii sea is blocked only when both Sevii units are locked.
    """
    W, H = L.WORLD_W, L.WORLD_H
    region_of = {"JOHTO": "JOHTO", "KANTO": "KANTO", "HOENN": "HOENN",
                 "SEVII123": "SEVII", "SEVII4567": "SEVII"}
    bad = 0
    for n in range(1, len(LOCK_UNITS) + 1):
        for open_units in itertools.combinations(LOCK_UNITS, n):
            open_regions = {region_of[u] for u in open_units}

            def passable(x, y):
                if unit[y][x]:
                    return unit[y][x] in open_units
                t = terr[y][x]
                return t is None or region_of[t] in open_regions

            targets = [(x, y) for y in range(H) for x in range(W) if unit[y][x] in open_units]
            start = targets[0]
            seen, stack = {start}, [start]
            while stack:
                cx, cy = stack.pop()
                for nx, ny in ((cx + 1, cy), (cx - 1, cy), (cx, cy + 1), (cx, cy - 1)):
                    if 0 <= nx < W and 0 <= ny < H and (nx, ny) not in seen and passable(nx, ny):
                        seen.add((nx, ny))
                        stack.append((nx, ny))
            missed = sorted({world[y][x] for x, y in targets if (x, y) not in seen})
            if missed:
                bad += 1
                errors.append("unlocked %s: unreachable %s" % ("+".join(open_units), ", ".join(missed)))
    return 2 ** len(LOCK_UNITS) - 1 - bad


def write_preview(world, unit, terr, path):
    W, H, S = L.WORLD_W, L.WORLD_H, CELL_PX
    img = [[OPEN_SEA] * (W * S) for _ in range(H * S)]
    for y in range(H):
        for x in range(W):
            if unit[y][x]:
                col = COLORS[unit[y][x]][0]
            elif terr[y][x]:
                col = COLORS[terr[y][x]][1]
            else:
                col = OPEN_SEA
            for py in range(S):
                row = img[y * S + py]
                for px in range(S):
                    row[x * S + px] = col
            if not unit[y][x]:
                for i in range(S):
                    img[y * S][x * S + i] = GRID_LINE
                    img[y * S + i][x * S] = GRID_LINE
                continue
            tok = world[y][x]
            # Outline edges where the neighbour is a different MAPSEC.
            for nx, ny, edge in ((x - 1, y, "l"), (x + 1, y, "r"), (x, y - 1, "t"), (x, y + 1, "b")):
                same = 0 <= nx < W and 0 <= ny < H and world[ny][nx] == tok
                if same:
                    continue
                for i in range(S):
                    if edge == "l":
                        img[y * S + i][x * S] = OUTLINE
                    elif edge == "r":
                        img[y * S + i][x * S + S - 1] = OUTLINE
                    elif edge == "t":
                        img[y * S][x * S + i] = OUTLINE
                    else:
                        img[y * S + S - 1][x * S + i] = OUTLINE
    write_rgb_png(path, img)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--print", action="store_true", help="print the world and region grids")
    args = ap.parse_args()

    errors = []
    world, unit, records = place(errors)
    n_secs = check_mapsecs(world, errors)
    terr = territory(unit)
    states = check_reach(world, unit, terr, errors)

    region_id = {r: i + 1 for i, r in enumerate(L.REGIONS)}
    unit_region = {"SEVII123": "SEVII", "SEVII4567": "SEVII"}
    region_grid = [[region_id[unit_region.get(t, t)] if t else 0 for t in row] for row in terr]

    os.makedirs(STAGE, exist_ok=True)
    with open(os.path.join(STAGE, "world_grid.json"), "w") as f:
        json.dump({"width": L.WORLD_W, "height": L.WORLD_H, "rows": world}, f, indent=1)
    with open(os.path.join(STAGE, "region_grid.json"), "w") as f:
        json.dump({"width": L.WORLD_W, "height": L.WORLD_H, "regions": ["OPEN_SEA"] + L.REGIONS,
                   "rows": ["".join(str(v) for v in row) for row in region_grid]}, f, indent=1)
    with open(os.path.join(STAGE, "clusters.json"), "w") as f:
        json.dump(records, f, indent=1)
    write_preview(world, unit, terr, os.path.join(STAGE, "grid_preview.png"))

    print("world %dx%d cells (%dx%d px), %d clusters, %d MAPSECs" % (
        L.WORLD_W, L.WORLD_H, L.WORLD_W * 8, L.WORLD_H * 8, len(records), n_secs))
    for r in records:
        print("  %-13s %-9s src %-15s -> (%2d,%2d)  %3d cells  %s" % (
            r["name"], r["source"], "(%d,%d %dx%d)" % tuple(r["src_rect"]), r["dest"][0], r["dest"][1],
            r["cells"], r["lock"]))
    open_sea = sum(row.count(0) for row in region_grid)
    print("open sea cells %d, lock states reachable %d / %d" % (
        open_sea, states, 2 ** len(LOCK_UNITS) - 1))
    if args.print:
        for y in range(L.WORLD_H):
            print("".join(("#" if unit[y][x] else str(region_grid[y][x]) if region_grid[y][x] else ".")
                          for x in range(L.WORLD_W)))
    print("wrote", STAGE)
    if errors:
        for e in errors:
            print("error:", e)
        sys.exit(1)


if __name__ == "__main__":
    main()
