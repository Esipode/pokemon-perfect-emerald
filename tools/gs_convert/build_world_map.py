#!/usr/bin/env python3
"""Build the combined world map asset set (GS Stage 28, §9.1).

Composites the Hoenn, Kanto, Sevii 1-7 and Johto (HnS) region maps into one text-mode BG:
4bpp tiles, up to 15 palette banks, 64x64 tilemap. Source files are never modified.

Region maps are affine 8bpp tile sheets (png) + 64x64 byte tilemaps (bin); the playable 28x15
cell grid starts at tile (1, 2) (MAPCURSOR_X_MIN / MAPCURSOR_Y_MIN in src/region_map.c).
Colours are matched by RGB, and every region owns its own palette banks so a locked region can
be dimmed at run time by blending only its banks.

Outputs (graphics/world_map/):
  tiles.png    4bpp tile sheet, 16 tiles per row (bank-local indices 0-15)
  map.pal      JASC palette, 16 banks x 16 colours (bank 15 is left for text)
  map.bin      64x64 u16 tilemap in screenblock order (tile | bank << 12)
  preview.png  truecolour render of the composite (review only, not built)
and src/data/region_map/world_map_layout.h (panel origins and bank ranges).

Usage: build_world_map.py [--hns PATH]
"""

import argparse
import collections
import os
import struct
import sys
import zlib

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from common import ROOT, add_hns_arg  # noqa: E402

sys.path.insert(0, os.path.join(ROOT, "tools", "frlg_convert"))
import png4  # noqa: E402

GRID_X, GRID_Y = 1, 2          # first playable tile of a region map
GRID_W, GRID_H = 28, 15
WORLD_W, WORLD_H = 64, 64      # text BG size 3 (64x64 tiles)
MAX_BANKS = 15                 # bank 15 stays free for text windows
COLORS_PER_BANK = 15           # index 0 is transparent / backdrop

# name, bank group, png/bin stem relative to ROOT (or HnS), is_hns, crop (None = full grid), origin
HNS_DIR = "graphics/pokenav/region_map/"
PANELS = [
    ("johto",  "johto",  HNS_DIR + "map_johto",             True,  None,   (0, 0)),
    ("kanto",  "kanto",  "graphics/region_map/map_kanto",   False, None,   (28, 0)),
    ("hoenn",  "hoenn",  "graphics/region_map/map",         False, None,   (0, 15)),
    ("sevii123", "sevii", "graphics/region_map/map_sevii_123", False, "auto", (28, 15)),
    ("sevii45",  "sevii", "graphics/region_map/map_sevii_45",  False, "auto", (0, 30)),
    ("sevii67",  "sevii", "graphics/region_map/map_sevii_67",  False, "auto", (28, 30)),
]


def load_panel(hns, stem, is_hns, crop):
    base = os.path.join(hns if is_hns else ROOT, stem)
    img = png4.read_png(open(base + ".png", "rb").read())
    tiles = png4.image_to_tiles(img)
    tmap = open(base + ".bin", "rb").read()
    plte = img.plte

    def rgb(i):
        return tuple(plte[3 * i:3 * i + 3])

    rows = [[tmap[(GRID_Y + y) * 64 + GRID_X + x] for x in range(GRID_W)] for y in range(GRID_H)]
    bg = collections.Counter(t for r in rows for t in r).most_common(1)[0][0]
    x0, y0, x1, y1 = 0, 0, GRID_W - 1, GRID_H - 1
    if crop == "auto":
        pts = [(x, y) for y in range(GRID_H) for x in range(GRID_W) if rows[y][x] != bg]
        x0, x1 = min(p[0] for p in pts), max(p[0] for p in pts)
        y0, y1 = min(p[1] for p in pts), max(p[1] for p in pts)
    cells = []
    for y in range(y0, y1 + 1):
        cells.append([[rgb(p) for p in tiles[rows[y][x]]] for x in range(x0, x1 + 1)])
    return cells, (x0, y0, x1 - x0 + 1, y1 - y0 + 1)


def assign_banks(tile_colors):
    """Greedy lossless packing of tile colour sets into banks of 15 colours."""
    banks = []  # list of [color list]
    where = {}
    for key, cols in sorted(tile_colors.items(), key=lambda kv: -len(kv[1])):
        best, best_add = None, None
        for bi, bank in enumerate(banks):
            add = len(cols - set(bank))
            if len(bank) + add <= COLORS_PER_BANK and (best is None or add < best_add):
                best, best_add = bi, add
        if best is None:
            banks.append([])
            best = len(banks) - 1
        banks[best] += sorted(cols - set(banks[best]))
        where[key] = best
    return banks, where


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
        f.write(png4.PNG_SIGNATURE + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
                + chunk(b"IDAT", zlib.compress(bytes(raw), 9)) + chunk(b"IEND", b""))


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    add_hns_arg(ap)
    args = ap.parse_args()

    panels = []
    for name, group, stem, is_hns, crop, origin in PANELS:
        cells, box = load_panel(args.hns, stem, is_hns, crop)
        panels.append((name, group, cells, box, origin))
        print("%-9s crop x%d y%d  %dx%d  at world (%d,%d)" % (name, box[0], box[1], box[2], box[3], *origin))

    # Per bank group: unique 8x8 RGB tiles and their colour sets.
    groups = collections.OrderedDict()
    for name, group, cells, box, origin in panels:
        g = groups.setdefault(group, {})
        for row in cells:
            for tile in row:
                g[tuple(tile)] = frozenset(tile)

    bank_base, group_banks, group_where = {}, {}, {}
    total = 0
    for group, tcol in groups.items():
        banks, where = assign_banks(tcol)
        bank_base[group] = total
        group_banks[group] = banks
        group_where[group] = where
        total += len(banks)
        print("group %-6s tiles %4d  colours %2d  banks %d" % (
            group, len(tcol), len({c for s in tcol.values() for c in s}), len(banks)))
    if total > MAX_BANKS:
        sys.exit("need %d palette banks, only %d available" % (total, MAX_BANKS))

    # Void tile: index 0 everywhere, shows the backdrop.
    char_tiles = [bytes(64)]
    seen = {bytes(64): 0}
    world = [[0] * WORLD_W for _ in range(WORLD_H)]
    preview = [[(0, 0, 0)] * (WORLD_W * 8) for _ in range(WORLD_H * 8)]

    for name, group, cells, box, origin in panels:
        banks = group_banks[group]
        for ty, row in enumerate(cells):
            for tx, tile in enumerate(row):
                bi = group_where[group][tuple(tile)]
                lut = {c: i + 1 for i, c in enumerate(banks[bi])}
                pix = bytes(lut[c] for c in tile)
                if pix not in seen:
                    seen[pix] = len(char_tiles)
                    char_tiles.append(pix)
                gx, gy = origin[0] + tx, origin[1] + ty
                world[gy][gx] = seen[pix] | ((bank_base[group] + bi) << 12)
                for py in range(8):
                    for px in range(8):
                        preview[gy * 8 + py][gx * 8 + px] = tile[py * 8 + px]
    print("unique tiles %d (incl. void) of 1024 text-BG limit" % len(char_tiles))
    if len(char_tiles) > 1024:
        sys.exit("too many tiles")

    palette = [(0, 0, 0)] * 256
    for group, banks in group_banks.items():
        for bi, cols in enumerate(banks):
            for ci, c in enumerate(cols):
                palette[(bank_base[group] + bi) * 16 + ci + 1] = c

    out = os.path.join(ROOT, "graphics", "world_map")
    os.makedirs(out, exist_ok=True)
    gray = bytes(v for i in range(16) for v in (i * 17,) * 3)
    open(os.path.join(out, "tiles.png"), "wb").write(png4.write_tiles_png(char_tiles, gray))
    with open(os.path.join(out, "map.pal"), "w") as f:
        f.write("JASC-PAL\n0100\n256\n")
        for c in palette:
            f.write("%d %d %d\n" % c)
    with open(os.path.join(out, "map.bin"), "wb") as f:
        # 64x64 text BG layout: four 32x32 screenblocks (TL, TR, BL, BR).
        for by in (0, 32):
            for bx in (0, 32):
                for y in range(by, by + 32):
                    for x in range(bx, bx + 32):
                        f.write(struct.pack("<H", world[y][x]))
    write_rgb_png(os.path.join(out, "preview.png"), preview)

    hdr = os.path.join(ROOT, "src", "data", "region_map", "world_map_layout.h")
    with open(hdr, "w") as f:
        f.write("// Generated by tools/gs_convert/build_world_map.py. Do not edit.\n\n")
        for name, group, cells, box, origin in panels:
            n = name.upper()
            f.write("#define WORLD_MAP_%s_X %d\n#define WORLD_MAP_%s_Y %d\n" % (n, origin[0], n, origin[1]))
            f.write("#define WORLD_MAP_%s_W %d\n#define WORLD_MAP_%s_H %d\n" % (n, box[2], n, box[3]))
            f.write("#define WORLD_MAP_%s_SRC_X %d\n#define WORLD_MAP_%s_SRC_Y %d\n" % (n, box[0], n, box[1]))
        f.write("\n")
        for group, banks in group_banks.items():
            f.write("#define WORLD_MAP_BANK_%s_FIRST %d\n#define WORLD_MAP_BANK_%s_COUNT %d\n" % (
                group.upper(), bank_base[group], group.upper(), len(banks)))
    print("wrote", out, "and", hdr)


if __name__ == "__main__":
    main()
