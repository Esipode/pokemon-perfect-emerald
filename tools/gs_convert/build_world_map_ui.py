#!/usr/bin/env python3
"""Build graphics/world_map/ui_sprites.png (GS Stage 28.10, R7).

World map UI sprite sheet: cursor corner bracket, Fly rings, Battle Frontier outline, player
halo. The PNG is one 8 px wide column: tile N of the sprite sheet is row N, so every frame is a
run of consecutive tiles in sprite order. Keep FRAMES in step with the UI_TILE_* constants in
src/world_map.c.

Usage: build_world_map_ui.py
"""

import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "frlg_convert"))
import png4  # noqa: E402

ROOT = os.path.join(os.path.dirname(__file__), "..", "..")
OUT = os.path.join(ROOT, "graphics", "world_map", "ui_sprites.png")

PALETTE = [
    (255, 0, 255),    # 0 transparent
    (248, 248, 248),  # 1 ring, bright
    (255, 216, 64),   # 2 cursor
    (24, 28, 48),     # 3 outline
    (136, 144, 168),  # 4 ring, faint
    (232, 64, 56),    # 5 Battle Frontier outline
    (176, 232, 255),  # 6 halo
    (232, 248, 255),  # 7 halo rim
]
T, WHITE, GOLD, DARK, GREY, RED, HALO, RIM = range(8)


def blank(w, h):
    return [[T] * w for _ in range(h)]


def bracket():
    img = blank(8, 8)
    for y in range(8):
        for x in range(8):
            if (x < 5 and y < 2) or (x < 2 and y < 5):
                img[y][x] = GOLD
    for y in range(8):
        for x in range(8):
            if img[y][x] == T and any(
                0 <= x + dx < 8 and 0 <= y + dy < 8 and img[y + dy][x + dx] == GOLD
                for dx, dy in ((1, 0), (0, 1))
            ):
                img[y][x] = DARK
    return img


def inside(px, py, w, h, inset, radius):
    x0, y0, x1, y1 = inset, inset, w - inset, h - inset
    r = min(radius, (x1 - x0) / 2, (y1 - y0) / 2)
    cx = min(max(px + 0.5, x0 + r), x1 - r)
    cy = min(max(py + 0.5, y0 + r), y1 - r)
    return (px + 0.5 - cx) ** 2 + (py + 0.5 - cy) ** 2 <= r * r


def ring(w, h, colour, radius=7):
    img = blank(w, h)
    for y in range(h):
        for x in range(w):
            if inside(x, y, w, h, 1, radius) and not inside(x, y, w, h, 3, radius - 2):
                img[y][x] = colour
    out = [row[:] for row in img]
    for y in range(h):
        for x in range(w):
            if img[y][x] == T and any(
                0 <= x + dx < w and 0 <= y + dy < h and img[y + dy][x + dx] == colour
                for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))
            ):
                out[y][x] = DARK
    return out


def halo(radius):
    img = blank(16, 16)
    for y in range(16):
        for x in range(16):
            d = ((x - 7.5) ** 2 + (y - 7.5) ** 2) ** 0.5
            if d <= radius - 1:
                if (x + y) % 2 == 0:
                    img[y][x] = HALO
            elif d <= radius:
                img[y][x] = RIM
    return img


# Frame order and tile offsets: see UI_TILE_* in src/world_map.c.
FRAMES = [
    bracket(),                      # 0
    ring(16, 16, WHITE),            # 1
    ring(16, 16, GREY),             # 5
    ring(32, 16, WHITE),            # 9
    ring(32, 16, GREY),             # 17
    ring(16, 32, WHITE),            # 25
    ring(16, 32, GREY),             # 33
    ring(16, 16, RED, 3),           # 41
    halo(5),                        # 45
    halo(7.5),                      # 49
]


def frame_tiles(img):
    h, w = len(img), len(img[0])
    tiles = []
    for ty in range(h // 8):
        for tx in range(w // 8):
            tiles.append([img[ty * 8 + y][tx * 8 + x] for y in range(8) for x in range(8)])
    return tiles


def main():
    tiles = []
    for img in FRAMES:
        tiles += frame_tiles(img)
    pixels = bytearray()
    for tile in tiles:
        for y in range(8):
            pixels += bytes(tile[y * 8:(y + 1) * 8])
    plte = bytearray()
    for r, g, b in PALETTE:
        plte += bytes((r, g, b))
    plte += bytes(3 * (16 - len(PALETTE)))
    with open(OUT, "wb") as f:
        f.write(png4.write_indexed_png(8, len(tiles) * 8, pixels, plte))
    print(f"{OUT}: {len(tiles)} tiles")


if __name__ == "__main__":
    main()
