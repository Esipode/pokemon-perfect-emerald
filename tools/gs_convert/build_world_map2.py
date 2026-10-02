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

R2: land/water mask. graphics/world_map/src/land.png (world size in pixels, 2 colours) is the
hand-editable land mask. --seed-mask (re)creates it from the source art: every source pixel is
classified water (blue dominant) or land, per cluster rect, then pasted at the cluster's world
destination. Without --seed-mask the existing file is read, never written. Writes
mask_preview.png (mask + MAPSEC cell outlines) to the staging dir and lists suspect MAPSECs.

Usage: build_world_map2.py [--print] [--seed-mask] [--hns PATH]
"""

import argparse
import itertools
import json
import math
import os
import re
import struct
import sys
import zlib

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from common import ROOT, OUT_DIR, add_hns_arg  # noqa: E402
import world_map_layout as L  # noqa: E402

sys.path.insert(0, os.path.join(ROOT, "tools", "frlg_convert"))
import png4  # noqa: E402

DATA = os.path.join(ROOT, "src", "data", "region_map")
MASK_PATH = os.path.join(ROOT, "graphics", "world_map", "src", "land.png")
MASK_PLTE = bytes((24, 40, 80, 190, 210, 150))   # index 0 water, index 1 land
PREVIEW_SCALE = 2
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


def is_water(rgb):
    r, g, b = rgb
    return b > r + 20 and b >= g


def seed_mask(world, hns):
    """Classify source pixels per cluster rect and paste them at the world destination."""
    W, H = L.WORLD_W * 8, L.WORLD_H * 8
    mask = bytearray(W * H)
    art = {}
    for src, (stem, is_hns) in L.SOURCE_ART.items():
        base = os.path.join(hns if is_hns else ROOT, stem)
        img = png4.read_png(open(base + ".png", "rb").read())
        land = [0 if is_water(tuple(img.plte[3 * i:3 * i + 3])) else 1 for i in range(256)]
        tiles = png4.image_to_tiles(img)
        art[src] = (tiles, open(base + ".bin", "rb").read(), land)

    def source_land(src, x, y):
        """Land pixels (8x8 list) of source grid cell (x, y)."""
        tiles, tmap, land = art[src]
        tile = tiles[tmap[(L.ART_GRID_Y + y) * 64 + L.ART_GRID_X + x]]
        return [land[p] for p in tile]

    used = set()
    rects = {n: (dx, dy, sw, sh) for n, _, (_, _, sw, sh), (dx, dy), _ in L.CLUSTERS}

    def in_other_rect(name, wx, wy):
        return any(o != name and ox <= wx < ox + ow and oy <= wy < oy + oh
                   for o, (ox, oy, ow, oh) in rects.items())

    for name, src, (sx, sy, sw, sh), (dx, dy), _ in L.CLUSTERS:
        m = L.MASK_MARGIN
        for cy in range(-m, sh + m):
            for cx in range(-m, sw + m):
                gx, gy, wx, wy = sx + cx, sy + cy, dx + cx, dy + cy
                inside = 0 <= cx < sw and 0 <= cy < sh
                if not (0 <= gx < GRID_W and 0 <= gy < GRID_H and 0 <= wx < L.WORLD_W and 0 <= wy < L.WORLD_H):
                    continue
                if not inside and in_other_rect(name, wx, wy):
                    continue
                used.add((src, gx, gy))
                px = source_land(src, gx, gy)
                for py in range(8):
                    row = (wy * 8 + py) * W + wx * 8
                    for k in range(8):
                        if inside:
                            mask[row + k] = px[py * 8 + k]
                        elif px[py * 8 + k]:
                            mask[row + k] = 1
    for src in art:
        dropped = sum(sum(source_land(src, x, y)) for y in range(GRID_H) for x in range(GRID_W)
                      if (src, x, y) not in used)
        if dropped:
            print("  %-9s %d land pixels lie outside every cluster rect (dropped)" % (src, dropped))
    print("  taper removed %d px, specks removed %d px" % (taper_edges(mask, world), remove_specks(mask)))
    return mask


def taper_edges(mask, world):
    """Round off land cut by a cluster rect edge, in cells that hold no MAPSEC.

    Each outer edge (not shared with a touching cluster) loses a wavy 0-8 px band, so panel
    crops do not leave ruler-straight coasts.
    """
    W = L.WORLD_W * 8
    touching = {frozenset(p) for p in L.TOUCHING}
    rects = {n: (dx, dy, sw, sh) for n, _, (_, _, sw, sh), (dx, dy), _ in L.CLUSTERS}

    def shared(name, side):
        dx, dy, sw, sh = rects[name]
        probe = {"l": (dx - 1, dy), "r": (dx + sw, dy), "t": (dx, dy - 1), "b": (dx, dy + sh)}[side]
        for other, (ox, oy, ow, oh) in rects.items():
            if other == name or frozenset((name, other)) not in touching:
                continue
            if side in "lr" and oy < dy + sh and dy < oy + oh and ox <= probe[0] < ox + ow:
                return True
            if side in "tb" and ox < dx + sw and dx < ox + ow and oy <= probe[1] < oy + oh:
                return True
        return False

    def depth(t, phase):
        return max(0.0, min(8.0, 3.5 + 3 * math.sin(t * 0.21 + phase) + 2 * math.sin(t * 0.53 + 2 * phase)))

    removed = 0
    for name, (dx, dy, sw, sh) in rects.items():
        outer = [s for s in "lrtb" if not shared(name, s)]
        m = L.MASK_MARGIN
        x0 = max(0, dx - (m if "l" in outer else 0)) * 8
        y0 = max(0, dy - (m if "t" in outer else 0)) * 8
        x1 = min(L.WORLD_W, dx + sw + (m if "r" in outer else 0)) * 8
        y1 = min(L.WORLD_H, dy + sh + (m if "b" in outer else 0)) * 8
        phase = sum(map(ord, name)) * 0.37
        for y in range(y0, y1):
            for x in range(x0, x1):
                if not mask[y * W + x] or world[y // 8][x // 8] != NONE:
                    continue
                cut = ((("l" in outer) and x - x0 < depth(y, phase))
                       or (("r" in outer) and x1 - 1 - x < depth(y, phase + 1))
                       or (("t" in outer) and y - y0 < depth(x, phase + 2))
                       or (("b" in outer) and y1 - 1 - y < depth(x, phase + 3)))
                if cut:
                    mask[y * W + x] = 0
                    removed += 1
    return removed


def remove_specks(mask, limit=6):
    """Delete 4-connected land components of at most `limit` pixels."""
    W, H = L.WORLD_W * 8, L.WORLD_H * 8
    seen = bytearray(W * H)
    removed = 0
    for start in range(W * H):
        if not mask[start] or seen[start]:
            continue
        comp, stack = [], [start]
        seen[start] = 1
        while stack:
            i = stack.pop()
            comp.append(i)
            x, y = i % W, i // W
            for ni, ok in ((i - 1, x > 0), (i + 1, x < W - 1), (i - W, y > 0), (i + W, y < H - 1)):
                if ok and mask[ni] and not seen[ni]:
                    seen[ni] = 1
                    stack.append(ni)
        if len(comp) <= limit:
            for i in comp:
                mask[i] = 0
            removed += len(comp)
    return removed


def write_mask(mask):
    os.makedirs(os.path.dirname(MASK_PATH), exist_ok=True)
    with open(MASK_PATH, "wb") as f:
        f.write(png4.write_indexed_png(L.WORLD_W * 8, L.WORLD_H * 8, bytes(mask), MASK_PLTE))


def read_mask():
    """Load land.png from any PNG type: indexed index 1+ or brightness >= 128 is land."""
    with open(MASK_PATH, "rb") as f:
        data = f.read()
    w, h, px = png4.read_rgba(data)
    if (w, h) != (L.WORLD_W * 8, L.WORLD_H * 8):
        sys.exit("%s is %dx%d, expected %dx%d" % (MASK_PATH, w, h, L.WORLD_W * 8, L.WORLD_H * 8))
    try:
        idx = png4.read_png(data).pixels
        return bytearray(1 if i else 0 for i in idx)
    except png4.PngError:
        return bytearray(1 if (r + g + b) // 3 >= 128 and a >= 128 else 0 for r, g, b, a in px)


def check_mask(mask, world, errors):
    """Per-MAPSEC land coverage. Returns (water-like MAPSECs, MAPSECs with under 25% land)."""
    W = L.WORLD_W * 8
    cover = {}
    for y in range(L.WORLD_H):
        for x in range(L.WORLD_W):
            tok = world[y][x]
            if tok == NONE:
                continue
            n = sum(sum(mask[(y * 8 + py) * W + x * 8:(y * 8 + py) * W + x * 8 + 8]) for py in range(8))
            c = cover.setdefault(tok, [0, 0])
            c[0] += n
            c[1] += 64
    water = sorted(t for t, (n, _) in cover.items() if n == 0)
    sparse = sorted((t, n / total) for t, (n, total) in cover.items() if 0 < n < total // 4)
    return water, sparse


def write_mask_preview(mask, world, unit, terr, water, path):
    W, H, S = L.WORLD_W * 8, L.WORLD_H * 8, PREVIEW_SCALE
    img = [[OPEN_SEA] * (W * S) for _ in range(H * S)]
    for y in range(H):
        for x in range(W):
            if not mask[y * W + x]:
                continue
            u = unit[y // 8][x // 8] or terr[y // 8][x // 8]
            col = COLORS[u][0] if u else (170, 170, 170)
            for py in range(S):
                row = img[y * S + py]
                for px in range(S):
                    row[x * S + px] = col
    cs = 8 * S
    for y in range(L.WORLD_H):
        for x in range(L.WORLD_W):
            tok = world[y][x]
            if tok == NONE:
                continue
            line = (230, 40, 40) if tok in water else OUTLINE
            for nx, ny, edge in ((x - 1, y, "l"), (x + 1, y, "r"), (x, y - 1, "t"), (x, y + 1, "b")):
                if 0 <= nx < L.WORLD_W and 0 <= ny < L.WORLD_H and world[ny][nx] == tok:
                    continue
                for i in range(cs):
                    if edge == "l":
                        img[y * cs + i][x * cs] = line
                    elif edge == "r":
                        img[y * cs + i][x * cs + cs - 1] = line
                    elif edge == "t":
                        img[y * cs][x * cs + i] = line
                    else:
                        img[y * cs + cs - 1][x * cs + i] = line
    write_rgb_png(path, img)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--print", action="store_true", help="print the world and region grids")
    ap.add_argument("--seed-mask", action="store_true", help="(re)create land.png from the source art")
    add_hns_arg(ap)
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
    if args.seed_mask:
        print("seeding", MASK_PATH)
        write_mask(seed_mask(world, args.hns))
    if os.path.exists(MASK_PATH):
        mask = read_mask()
        water, sparse = check_mask(mask, world, errors)
        write_mask_preview(mask, world, unit, terr, water, os.path.join(STAGE, "mask_preview.png"))
        print("land pixels %d of %d" % (sum(mask), len(mask)))
        print("MAPSECs with no land (sea routes expected): %s" % ", ".join(t[7:] for t in water))
        for t, f in sparse:
            print("warning: %s has only %d%% land under its cells" % (t, f * 100))
    else:
        print("no land.png yet; run with --seed-mask")
    print("wrote", STAGE)
    if errors:
        for e in errors:
            print("error:", e)
        sys.exit(1)


if __name__ == "__main__":
    main()
