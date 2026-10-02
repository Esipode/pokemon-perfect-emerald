#!/usr/bin/env python3
"""Rebuild the Hoenn Trainer Card front tilemaps for a third badge row (Johto).

Reads the committed (HEAD) front.bin / front_link.bin, so re-running is safe.
Changes, both tilemaps:
  * header plate (rows 2-3, cols 2-14) becomes stripes; the ID text is printed there
  * trainer-pic disc moves up 2 tile rows (rows 4-12 -> 2-10)
  * the time bullet pair (rows 11-12) is removed; DEX and time share the row-9/10 line
Front only: badge strip becomes label row 11 + three slot pairs (rows 12-17).

Usage: shift_trainer_card_front.py [--out DIR]   (default: graphics/trainer_card)
"""

import argparse
import os
import struct
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from common import ROOT  # noqa: E402

COLS, ROWS = 30, 20
STRIPE, WHITE = 4, 1
DISC_COLS = range(19, 27)


def load(name):
    raw = subprocess.check_output(["git", "show", "HEAD:graphics/trainer_card/%s" % name], cwd=ROOT)
    return list(struct.unpack("<%dH" % (len(raw) // 2), raw))


def row(m, r):
    return m[r * COLS:(r + 1) * COLS]


def put(m, r, cells):
    m[r * COLS:(r + 1) * COLS] = cells


def shift(m, strip):
    new = m[:]
    for r in (2, 3):
        for c in range(2, 15):
            new[r * COLS + c] = STRIPE
    # Clear the old disc and the time bullets, then redraw the disc two rows up.
    for r in range(4, 13):
        for c in DISC_COLS:
            new[r * COLS + c] = WHITE
        new[r * COLS + 2] = WHITE
    for r in range(4, 13):
        for c in DISC_COLS:
            t = m[r * COLS + c]
            if (t & 0x3ff) != WHITE:
                new[(r - 2) * COLS + c] = t
    # Name and money/DEX bullets keep rows 4-5, 7-8, 9-10.
    for r in (4, 7, 9):
        new[r * COLS + 2] = m[r * COLS + 2]
    for r in (5, 8, 10):
        new[r * COLS + 2] = m[r * COLS + 2]
    new[6 * COLS + 2] = m[6 * COLS + 2]
    if strip:
        label, top, bot = row(m, 13), row(m, 14), row(m, 15)
        put(new, 11, label)
        for r in (12, 14, 16):
            put(new, r, top)
            put(new, r + 1, bot)
    return new


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default=os.path.join(ROOT, "graphics/trainer_card"))
    args = ap.parse_args()
    for name, strip in (("front.bin", True), ("front_link.bin", False)):
        out = shift(load(name), strip)
        open(os.path.join(args.out, name), "wb").write(struct.pack("<%dH" % len(out), *out))
        print("wrote", name)


if __name__ == "__main__":
    main()
