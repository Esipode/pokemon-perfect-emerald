#!/usr/bin/env python3
"""Build the battle action menu art (Battle UI Revamp, Stage 2).

Writes:
  graphics/battle_interface/action_menu_icons.png  16 px wide sheet of 8x8 tiles, two per row: seven
                                                   icons, then the keypad glyphs for the hint row
  graphics/battle_interface/action_menu.pal        panel base colours, indices 0-7

Icon pixels use index 4 (light) and 5 (dark) only; the chip fill behind them is the slot hue.
Keep ICON_* / GLYPH_* in step with the layout constants in include/battle_action_menu.h.

Usage: build_action_menu_gfx.py
"""

import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "frlg_convert"))
import png4  # noqa: E402

ROOT = os.path.join(os.path.dirname(__file__), "..", "..")
OUT_DIR = os.path.join(ROOT, "graphics", "battle_interface")

# Panel base colours as 5-bit RGB; mirrors the old inline table (indices 0-7, see §1.3).
BASE_PALETTE = [
    (0, 0, 0),      # 0 transparent
    (5, 6, 9),      # 1 panel background (HP box fill)
    (8, 9, 11),     # 2 disabled chip fill, empty cell dots
    (22, 24, 28),   # 3 accent stripe
    (31, 31, 31),   # 4 white text, icon light
    (2, 3, 5),      # 5 text shadow (HP box shadow), icon dark
    (17, 19, 22),   # 6 muted text
    (14, 16, 20),   # 7 chip outline, divider (HP box rim)
]

GLYPH_BG = 4
GLYPH_FG = 5

# '.' transparent, 'W' index 4, 'D' index 5. Icons are 8x8 (one tile each).
ICONS = {
    "battle": [
        "......WW",
        ".....WW.",
        "....WW..",
        "...WW...",
        "W.WW....",
        ".WW.....",
        ".WWW....",
        "W..W....",
    ],
    "bag": [
        "..WWWW..",
        "..W..W..",
        ".WWWWWW.",
        "WWWDDWWW",
        "WWWWWWWW",
        "WWWWWWWW",
        "WWWWWWWW",
        ".WWWWWW.",
    ],
    "pokemon": [
        "..WWWW..",
        ".WWWWWW.",
        "WWWWWWWW",
        "DDDWWDDD",
        "DDDWWDDD",
        "WWWWWWWW",
        ".WWWWWW.",
        "..WWWW..",
    ],
    "run": [
        "....W...",
        "....WW..",
        "WWWWWWW.",
        "WWWWWWWW",
        "WWWWWWW.",
        "....WW..",
        "....W...",
        "........",
    ],
    "safari_ball": [
        "..WWWW..",
        ".WWDDWW.",
        "WWDWWDWW",
        "WDWWWWDW",
        "WDWWWWDW",
        "WWDWWDWW",
        ".WWDDWW.",
        "..WWWW..",
    ],
    "go_near": [
        ".WW.....",
        "WWWW....",
        "WWWW....",
        ".WW.....",
        "....WW..",
        "...WWWW.",
        "...WWWW.",
        "....WW..",
    ],
    "lock": [
        "..WWWW..",
        ".W....W.",
        ".W....W.",
        "WWWWWWWW",
        "WWWDDWWW",
        "WWWDDWWW",
        "WWWWWWWW",
        "........",
    ],
}
ICON_ORDER = ["battle", "bag", "pokemon", "run", "safari_ball", "go_near", "lock"]

# 5x7 letters for the hint-row badges. '#' = glyph foreground.
LETTERS = {
    "R": ["####.", "#...#", "#...#", "####.", "#.#..", "#..#.", "#...#"],
    "B": ["####.", "#...#", "#...#", "####.", "#...#", "#...#", "####."],
    "S": [".####", "#....", "#....", ".###.", "....#", "....#", "####."],
}
GLYPH_ORDER = ["R", "B", "S"]  # R, B on the first row; S (Start) on the second


def icon_pixels(rows):
    assert len(rows) == 8 and all(len(r) == 8 for r in rows), "icon must be 8x8"
    table = {".": 0, "W": 4, "D": 5}
    return [table[c] for r in rows for c in r]


def glyph_pixels(letter):
    """8x8 round badge: light disc, dark 5x7 letter shifted to fit."""
    disc = [
        "..XXXX..",
        ".XXXXXX.",
        "XXXXXXXX",
        "XXXXXXXX",
        "XXXXXXXX",
        "XXXXXXXX",
        ".XXXXXX.",
        "..XXXX..",
    ]
    px = [GLYPH_BG if c == "X" else 0 for r in disc for c in r]
    # Letter drawn 4x6 (drop the right column and the bottom row of the 5x7 font).
    for y in range(6):
        for x in range(4):
            if LETTERS[letter][y][x] == "#":
                px[(y + 1) * 8 + x + 2] = GLYPH_FG
    return px


def build_sheet():
    """16 px wide sheet of 8x8 tiles, two per row: icons in ICON_ORDER, then the keypad glyphs."""
    width = 16
    tiles = [icon_pixels(ICONS[name]) for name in ICON_ORDER] + [glyph_pixels(c) for c in GLYPH_ORDER]
    if len(tiles) % 2:
        tiles.append([0] * 64)
    pixels = []
    for i in range(0, len(tiles), 2):
        for y in range(8):
            pixels += tiles[i][y * 8:(y + 1) * 8] + tiles[i + 1][y * 8:(y + 1) * 8]
    return width, len(pixels) // width, pixels


def palette_bytes():
    plte = bytearray()
    for i in range(16):
        r, g, b = BASE_PALETTE[i] if i < len(BASE_PALETTE) else (0, 0, 0)
        plte += bytes((r * 8, g * 8, b * 8))
    return bytes(plte)


def write_pal(path, plte):
    lines = ["JASC-PAL", "0100", "16"]
    for i in range(16):
        lines.append(f"{plte[3 * i]} {plte[3 * i + 1]} {plte[3 * i + 2]}")
    with open(path, "w", newline="\n") as f:
        f.write("\r\n".join(lines) + "\r\n")


def main():
    width, height, pixels = build_sheet()
    plte = palette_bytes()
    with open(os.path.join(OUT_DIR, "action_menu_icons.png"), "wb") as f:
        f.write(png4.write_tiles_png(
            png4.image_to_tiles(png4.IndexedImage(width, height, bytes(pixels), plte)), plte, tiles_per_row=2))
    write_pal(os.path.join(OUT_DIR, "action_menu.pal"), plte)
    print(f"icons: {width}x{height}, {len(ICON_ORDER)} icons, {len(GLYPH_ORDER)} glyphs")


if __name__ == "__main__":
    main()
