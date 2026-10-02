#!/usr/bin/env python3
"""Colorize the Johto Trainer Card badges (graphics/trainer_card/gs/badges.png).

The source is a 4-shade grey sheet: 8 badges, 16x16 each, 17 px pitch, starting at x=1.
Output: badges_color_1..4.png, 32x16 4bpp, two badges per file sharing one 16-color palette
(same layout as graphics/trainer_card/frlg/badges_color_*.png).

Usage: color_trainer_badges.py
"""

import os
import struct
import sys
import zlib

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from common import ROOT  # noqa: E402

DIR = os.path.join(ROOT, "graphics/trainer_card/gs")
TRANSPARENT = (136, 96, 112)

# Source shade index (1 light .. 4 dark, 15 outline) -> RGB, in sheet order.
RAMPS = [
    {1: (200, 232, 184), 2: (88, 200, 152), 3: (56, 152, 120), 4: (24, 56, 48), 15: (16, 32, 28)},   # Zephyr
    {1: (248, 184, 168), 2: (216, 40, 40), 3: (168, 40, 48), 4: (136, 56, 72), 15: (12, 4, 4)},      # Hive
    {1: (248, 248, 216), 2: (200, 184, 168), 3: (150, 141, 123), 4: (248, 216, 72), 15: (8, 8, 8)},  # Plain
    {1: (184, 176, 224), 2: (136, 128, 184), 3: (168, 160, 208), 4: (88, 88, 136), 15: (8, 8, 8)},   # Fog
    {1: (248, 184, 104), 2: (232, 136, 56), 3: (195, 139, 78), 4: (138, 95, 58), 15: (8, 8, 8)},     # Storm
    {1: (216, 224, 240), 2: (176, 184, 208), 3: (88, 88, 136), 4: (152, 168, 200), 15: (8, 8, 8)},   # Mineral
    {1: (248, 248, 216), 2: (200, 232, 184), 3: (200, 184, 168), 4: (72, 168, 152), 15: (8, 8, 8)},  # Glacier
    {1: (88, 88, 136), 2: (56, 56, 88), 3: (168, 24, 40), 4: (40, 40, 64), 15: (8, 8, 16)},          # Rising
]
SLOT = {1: 1, 2: 2, 3: 3, 4: 4, 15: 5}  # shade -> palette index for the first badge; +5 for the second


def read_indexed(path):
    data = open(path, "rb").read()[8:]
    pos, idat = 0, b""
    while pos < len(data):
        n, = struct.unpack(">I", data[pos:pos + 4])
        tag, body = data[pos + 4:pos + 8], data[pos + 8:pos + 8 + n]
        pos += 12 + n
        if tag == b"IHDR":
            w, h, depth, ctype = struct.unpack(">IIBB", body[:10])
        elif tag == b"IDAT":
            idat += body
    assert depth == 8 and ctype == 3, "expected 8-bit indexed PNG"
    raw, stride, prev, off, rows = zlib.decompress(idat), w, bytearray(w), 0, []
    for _ in range(h):
        f, line = raw[off], bytearray(raw[off + 1:off + 1 + stride])
        off += 1 + stride
        for x in range(stride):
            a = line[x - 1] if x else 0
            c = prev[x - 1] if x else 0
            if f == 1:
                line[x] = (line[x] + a) & 255
            elif f == 2:
                line[x] = (line[x] + prev[x]) & 255
            elif f == 3:
                line[x] = (line[x] + ((a + prev[x]) >> 1)) & 255
            elif f == 4:
                pa, pb, pc = abs(prev[x] - c), abs(a - c), abs(a + prev[x] - 2 * c)
                line[x] = (line[x] + (a if pa <= pb and pa <= pc else prev[x] if pb <= pc else c)) & 255
        prev = line
        rows.append(list(line))
    return w, h, rows


def write_4bit(path, w, h, rows, palette):
    def chunk(tag, body):
        return struct.pack(">I", len(body)) + tag + body + struct.pack(">I", zlib.crc32(tag + body))

    raw = b""
    for r in rows:
        raw += b"\0" + bytes((r[x] << 4) | r[x + 1] for x in range(0, w, 2))
    plte = b"".join(bytes(c) for c in palette)
    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 4, 3, 0, 0, 0))
    png += chunk(b"PLTE", plte) + chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b"")
    open(path, "wb").write(png)


def main():
    _, _, src = read_indexed(os.path.join(DIR, "badges.png"))
    for pair in range(4):
        palette = [TRANSPARENT] + [(0, 0, 0)] * 15
        rows = [[0] * 32 for _ in range(16)]
        for half in range(2):
            badge = pair * 2 + half
            x0 = 1 + badge * 17
            for shade, rgb in RAMPS[badge].items():
                palette[SLOT[shade] + half * 5] = rgb
            for y in range(16):
                for x in range(16):
                    v = src[y][x0 + x]
                    if v:
                        rows[y][half * 16 + x] = SLOT[v] + half * 5
        write_4bit(os.path.join(DIR, "badges_color_%d.png" % (pair + 1)), 32, 16, rows, palette)
    print("wrote 4 files to", DIR)


if __name__ == "__main__":
    main()
