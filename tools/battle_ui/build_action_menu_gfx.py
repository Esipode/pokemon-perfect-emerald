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


# Move Info opener sprites (32x32 OBJ, shared ability pop-up palette). Rows keep the original art's
# palette indices; MOVE_INFO_REMAP recolours them to the panel. Indices 12 and 15 are unused by every
# sprite on that palette, so they carry the panel background and accent.
MOVE_INFO_OPENER_L = [
    "00bbbbbbbbbbbbbbbbbbbbbbbbbbbb00",
    "0baaeeeeeeeeeeeeeeeeeeeeeeeeaab0",
    "baaeddddd777777777777addddddeaab",
    "baeddddd77777a77777777addddddeab",
    "bedddddd77777a77777777adddddddeb",
    "bedddddd77777a77777777adddddddeb",
    "bedddddd77777a77777777adddddddeb",
    "bedddddd77777aaaa77777adddddddeb",
    "bedddddda777777777777aadddddddeb",
    "bedddddddaaaaaaaaaaaaaddddddddeb",
    "beddddddddddddddddddddddddddddeb",
    "beddddd5ddd5dd55dd5d5d555dddddeb",
    "beddddd55d55d5dd5d5d5d5dddddddeb",
    "beddddd5d5d5d5dd5d5d5d55ddddddeb",
    "beddddd5ddd5d5dd5d5d5d5dddddddeb",
    "beddddd5ddd5dd55ddd5dd555dddddeb",
    "beddddddddddddddddddddddddddddeb",
    "bedddddd5d5dd5d555dd55ddddddddeb",
    "bedddddd5d55d5d5ddd5dd5dddddddeb",
    "bedddddd5d5d55d55dd5dd5dddddddeb",
    "bedddddd5d5dd5d5ddd5dd5dddddddeb",
    "bedddddd5d5dd5d5dddd55ddddddddeb",
    "beddddddddddddddddddddddddddddeb",
    "baeddddddddddddddddddddddddddeab",
    "baaeddddddddddddddddddddddddeaab",
    "0baaeeeeeeeeeeeeeeeeeeeeeeeeaab0",
]

MOVE_INFO_OPENER_R = [
    "00bbbbbbbbbbbbbbbbbbbbbbbbbbbb00",
    "0baaeeeeeeeeeeeeeeeeeeeeeeeeaab0",
    "baaeddddd777777777777addddddeaab",
    "baeddddd77777aaa777777addddddeab",
    "bedddddd77777a77a77777adddddddeb",
    "bedddddd77777aaa777777adddddddeb",
    "bedddddd77777a77a77777adddddddeb",
    "bedddddd77777a77a77777adddddddeb",
    "bedddddda777777777777aadddddddeb",
    "bedddddddaaaaaaaaaaaaaddddddddeb",
    "beddddddddddddddddddddddddddddeb",
    "beddddd5ddd5dd55dd5d5d555dddddeb",
    "beddddd55d55d5dd5d5d5d5dddddddeb",
    "beddddd5d5d5d5dd5d5d5d55ddddddeb",
    "beddddd5ddd5d5dd5d5d5d5dddddddeb",
    "beddddd5ddd5dd55ddd5dd555dddddeb",
    "beddddddddddddddddddddddddddddeb",
    "bedddddd5d5dd5d555dd55ddddddddeb",
    "bedddddd5d55d5d5ddd5dd5dddddddeb",
    "bedddddd5d5d55d55dd5dd5dddddddeb",
    "bedddddd5d5dd5d5ddd5dd5dddddddeb",
    "bedddddd5d5dd5d5dddd55ddddddddeb",
    "beddddddddddddddddddddddddddddeb",
    "baeddddddddddddddddddddddddddeab",
    "baaeddddddddddddddddddddddddeaab",
    "0baaeeeeeeeeeeeeeeeeeeeeeeeeaab0",
]

OPENER_PAL = os.path.join(OUT_DIR, "ability_pop_up.pal")
OPENER_BG = (5, 6, 9)       # panel background
OPENER_ACCENT = (22, 24, 28)  # accent stripe
OPENER_BG_INDEX = 12
OPENER_ACCENT_INDEX = 15
OPENER_OUTLINE_INDEX = 11
OPENER_WHITE_INDEX = 7
OPENER_BORDER_INSET = 3     # pixels from the edge that belong to the frame


def opener_pixels(rows):
    pixels = []
    for y, row in enumerate(rows):
        for x, c in enumerate(row):
            v = int(c, 16)
            frame = (x < OPENER_BORDER_INSET or x >= 32 - OPENER_BORDER_INSET
                     or y < OPENER_BORDER_INSET or y >= 26 - OPENER_BORDER_INSET)
            if v == 0:
                out = 0
            elif frame and v in (0xA, 0xB):
                out = OPENER_OUTLINE_INDEX
            elif v == 0xE:
                out = OPENER_ACCENT_INDEX
            elif v == 7 or v == 5:
                out = OPENER_WHITE_INDEX   # key cap and "MOVE INFO" text
            else:
                out = OPENER_BG_INDEX      # panel fill and the key letter
            pixels.append(out)
    return pixels + [0] * (32 * 6)


# Last-used-ball window sprites. Idx 13 is the fill (the cycle arrows fade to it at runtime);
# idx 11 / 10 are the shown arrow body / outline, idx 11 also the frame outline.
BALL_SHEETS = ("last_used_ball_l", "last_used_ball_l_cycle", "last_used_ball_r", "last_used_ball_r_cycle")
BALL_FILL_INDEX = 13
BALL_ARROW_OUTLINE = (8, 9, 11)
BALL_OUTLINE = (17, 19, 22)


def ball_pixels(width, height, src):
    """Remap a ball sheet to panel indices; also accepts an already remapped sheet."""
    def at(x, y):
        return src[y * width + x] if 0 <= x < width and 0 <= y < height else 0

    out = bytearray()
    for y in range(height):
        for x in range(width):
            v = at(x, y)
            if v in (10, 11):
                frame = v == 11 or any(at(x + dx, y + dy) in (0, 11, 14)
                                       for dx in (-1, 0, 1) for dy in (-1, 0, 1))
                v = OPENER_OUTLINE_INDEX if frame else BALL_FILL_INDEX
            elif v == 14:
                v = OPENER_ACCENT_INDEX
            out.append(v)
    return bytes(out)


def update_opener_palette():
    with open(OPENER_PAL, newline="") as f:
        lines = f.read().split("\r\n")
    for index, rgb in ((OPENER_BG_INDEX, OPENER_BG), (OPENER_ACCENT_INDEX, OPENER_ACCENT),
                       (BALL_FILL_INDEX, OPENER_BG), (OPENER_OUTLINE_INDEX, BALL_OUTLINE),
                       (10, BALL_ARROW_OUTLINE)):
        lines[3 + index] = " ".join(str(c * 8) for c in rgb)
    with open(OPENER_PAL, "w", newline="") as f:
        f.write("\r\n".join(lines))
    plte = bytearray()
    for line in lines[3:19]:
        plte += bytes(int(v) for v in line.split())
    return bytes(plte)


def write_openers():
    plte = update_opener_palette()
    for suffix, rows in (("l", MOVE_INFO_OPENER_L), ("r", MOVE_INFO_OPENER_R)):
        with open(os.path.join(OUT_DIR, f"move_info_window_{suffix}.png"), "wb") as f:
            f.write(png4.write_indexed_png(32, 32, bytes(opener_pixels(rows)), plte))
    for name in BALL_SHEETS:
        path = os.path.join(OUT_DIR, f"{name}.png")
        with open(path, "rb") as f:
            img = png4.read_png(f.read())
        with open(path, "wb") as f:
            f.write(png4.write_indexed_png(img.width, img.height, ball_pixels(img.width, img.height, img.pixels), plte))


def main():
    width, height, pixels = build_sheet()
    plte = palette_bytes()
    with open(os.path.join(OUT_DIR, "action_menu_icons.png"), "wb") as f:
        f.write(png4.write_tiles_png(
            png4.image_to_tiles(png4.IndexedImage(width, height, bytes(pixels), plte)), plte, tiles_per_row=2))
    write_pal(os.path.join(OUT_DIR, "action_menu.pal"), plte)
    write_openers()
    print(f"icons: {width}x{height}, {len(ICON_ORDER)} icons, {len(GLYPH_ORDER)} glyphs")


if __name__ == "__main__":
    main()
