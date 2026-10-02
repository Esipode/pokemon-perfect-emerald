#!/usr/bin/env python3
"""Stage 30.1: trace palette index usage of the HnS Gold/Kris player assets.

For each asset (OW sheets, front pic, back pic, map icon) writes a legend (index, colour, pixel count,
frames using it, bounding box) and an ASCII index grid per frame (hex digit per index, '.' = transparent
index 0) to out/player_trace/{gold,kris}/<asset>.txt, plus a cross-asset summary.txt. Standard library only.
"""

import argparse
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "frlg_convert"))
from common import OUT_DIR, add_hns_arg  # noqa: E402
import png4  # noqa: E402

# (name, relative path template, frame width, frame height)
ASSETS = [
    ("ow_walking", "object_events/pics/people/{c}/walking_hns.png", 16, 32),
    ("ow_running", "object_events/pics/people/{c}/running_hns.png", 16, 32),
    ("ow_field_move", "object_events/pics/people/{c}/field_move_hns.png", 32, 32),
    ("ow_surfing", "object_events/pics/people/{c}/surfing_hns.png", 32, 32),
    ("ow_mach_bike", "object_events/pics/people/{c}/mach_bike_hns.png", 32, 32),
    ("ow_acro_bike", "object_events/pics/people/{c}/acro_bike_hns.png", 32, 32),
    ("ow_fishing", "object_events/pics/people/{c}/fishing_hns.png", 32, 32),
    ("front", "trainers/front_pics/{c}_hns.png", 64, 64),
    ("back", "trainers/back_pics/{c}_hns.png", 64, 64),
    ("icon", "pokenav/region_map/{c}_icon.png", 16, 16),
]
OW_PAL = "object_events/palettes/{c}_hns.pal"
FRONT_PAL = "trainers/palettes/{c}_hns.pal"
BACK_PAL = "trainers/back_pics/{c}_hns.pal"


def read_pal(path):
    lines = open(path).read().split()
    n = int(lines[2])
    return [tuple(int(x) for x in lines[3 + i * 3:6 + i * 3]) for i in range(n)]


def trace(img, fw, fh):
    frames = []
    for fy in range(0, img.height, fh):
        for fx in range(0, img.width, fw):
            frames.append([img.pixels[(fy + y) * img.width + fx:(fy + y) * img.width + fx + fw] for y in range(fh)])
    stats = {}
    for fi, rows in enumerate(frames):
        for y, row in enumerate(rows):
            for x, v in enumerate(row):
                s = stats.setdefault(v, {"n": 0, "frames": set(), "box": [fw, fh, -1, -1]})
                s["n"] += 1
                s["frames"].add(fi)
                b = s["box"]
                b[0], b[1], b[2], b[3] = min(b[0], x), min(b[1], y), max(b[2], x), max(b[3], y)
    return frames, stats


def main():
    ap = argparse.ArgumentParser()
    add_hns_arg(ap)
    args = ap.parse_args()
    gfx = os.path.join(args.hns, "graphics")
    summary = []
    for char in ("gold", "kris"):
        outdir = os.path.join(OUT_DIR, "player_trace", char)
        os.makedirs(outdir, exist_ok=True)
        summary.append(f"== {char} ==")
        for name, tmpl, fw, fh in ASSETS:
            path = os.path.join(gfx, tmpl.format(c=char))
            img = png4.read_png(open(path, "rb").read())
            plte = [tuple(img.plte[i:i + 3]) for i in range(0, len(img.plte), 3)]
            frames, stats = trace(img, fw, fh)
            lines = [f"{name}: {img.width}x{img.height}, {len(frames)} frames of {fw}x{fh}", "",
                     "idx  rgb              px  frames  bbox(x0,y0-x1,y1)"]
            for idx in sorted(stats):
                s = stats[idx]
                rgb = plte[idx] if idx < len(plte) else ("?",)
                lines.append(f"{idx:>3}  {str(rgb):<15}{s['n']:>6}  {len(s['frames']):>6}  {tuple(s['box'])}")
            lines.append("")
            for fi, rows in enumerate(frames):
                lines.append(f"-- frame {fi}")
                lines += ["".join("." if v == 0 else "0123456789abcdef"[v] for v in r) for r in rows]
            with open(os.path.join(outdir, name + ".txt"), "w") as f:
                f.write("\n".join(lines) + "\n")
            used = ",".join(str(i) for i in sorted(stats) if i)
            summary.append(f"{name:<14} idx used: {used}")
        # palette layouts
        for label, tmpl in (("OW pal", OW_PAL), ("front pal", FRONT_PAL), ("back pal", BACK_PAL)):
            summary.append(f"{label}: " + " ".join(f"{i}={c}" for i, c in enumerate(read_pal(os.path.join(gfx, tmpl.format(c=char))))))
        summary.append("")
    with open(os.path.join(OUT_DIR, "player_trace", "summary.txt"), "w") as f:
        f.write("\n".join(summary) + "\n")
    print("\n".join(summary))


if __name__ == "__main__":
    main()
