#!/usr/bin/env python3
"""Stage 9: extract the HnS overworld sprite definitions the Johto set uses.

Reads the HnS clone and rename_map.json, resolves each OBJ_EVENT_GFX_*_HNS id to its info struct, pic
table, source PNG and palette, decides reuse vs import, and writes out/ow_gfx.json plus a palette report.
Standard library only.
"""

import argparse
import glob
import json
import os
import re
import sys
from collections import Counter, defaultdict

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from common import OUT_DIR, ROOT, add_hns_arg, load_json, read  # noqa: E402

OBJ_DIR = "src/data/object_events"


def read_pal(path):
    lines = read(path).split()
    n = int(lines[2])
    return tuple(tuple(int(x) for x in lines[3 + i * 3:6 + i * 3]) for i in range(n))


def parse_defines(text):
    return {m.group(1): m.group(2) for m in re.finditer(r"#define\s+(\w+)\s+(\S+)", text)}


def parse_info(text):
    """symbol -> positional field list (HnS uses single-line positional initialisers)."""
    out = {}
    for m in re.finditer(r"const struct ObjectEventGraphicsInfo (\w+) = \{(.*?)\};", text, re.S):
        body = m.group(2)
        if "." in body.split(",")[0]:
            fields = dict(re.findall(r"\.(\w+)\s*=\s*([^,\n]+),", body))
            out[m.group(1)] = {"named": fields}
        else:
            out[m.group(1)] = {"pos": [f.strip() for f in body.split(",")]}
    return out


def parse_pic_tables(text):
    out = {}
    for m in re.finditer(r"static const struct SpriteFrameImage (\w+)\[\] = \{(.*?)\n\};", text, re.S):
        out[m.group(1)] = m.group(2).strip()
    return out


def main():
    ap = argparse.ArgumentParser()
    add_hns_arg(ap)
    args = ap.parse_args()
    hns = args.hns

    rename = load_json(os.path.join(OUT_DIR, "rename_map.json"))["obj_event_gfx"]
    hns_consts = parse_defines(read(os.path.join(hns, "include/constants/event_objects.h")))
    pointers = dict(re.findall(r"\[(OBJ_EVENT_GFX_\w+)\]\s*=\s*&(\w+)", read(os.path.join(hns, OBJ_DIR, "object_event_graphics_info_pointers.h"))))
    info = parse_info(read(os.path.join(hns, OBJ_DIR, "object_event_graphics_info.h")))
    pics = parse_pic_tables(read(os.path.join(hns, OBJ_DIR, "object_event_pic_tables.h")))
    gfx_h = read(os.path.join(hns, OBJ_DIR, "object_event_graphics.h"))
    pic_src = {m.group(1): m.group(2) for m in re.finditer(r'const u(?:16|32) (\w+)\[\] = INCBIN_U(?:16|32)\("([^"]+)\.4bpp"\)', gfx_h)}
    pal_src = {m.group(1): m.group(2) for m in re.finditer(r'const u16 (\w+)\[\] = INCBIN_U16\("([^"]+)\.gbapal"\)', gfx_h)}
    pal_table = {m.group(2): m.group(1) for m in re.finditer(r"\{(gObjectEventPal_\w+),\s*(OBJ_EVENT_PAL_TAG_\w+)\}", read(os.path.join(hns, "src/event_object_movement.c")))}

    t_pal_table = {m.group(2): m.group(1) for m in re.finditer(r"\{(gObjectEventPal_\w+),\s*(OBJ_EVENT_PAL_TAG_\w+)\}", read(os.path.join(ROOT, "src/event_object_movement.c")))}
    t_gfx_h = read(os.path.join(ROOT, OBJ_DIR, "object_event_graphics.h"))
    t_pal_src = {m.group(1): m.group(2) for m in re.finditer(r'const u16 (\w+)\[\] = INCBIN_U16\("([^"]+)\.gbapal"\)', t_gfx_h)}
    t_pal_colors = {}
    for tag, sym in t_pal_table.items():
        p = t_pal_src.get(sym)
        if p and os.path.exists(os.path.join(ROOT, p + ".pal")):
            t_pal_colors[tag] = read_pal(os.path.join(ROOT, p + ".pal"))

    # per-map usage from map.json
    usage = defaultdict(set)
    for mj in glob.glob(os.path.join(hns, "data/maps/*/map.json")):
        d = load_json(mj)
        if d.get("region") not in (None, "REGION_JOHTO") and "_hns" not in d.get("name", "") and not d.get("name", "").endswith("_hns"):
            pass
        for o in d.get("object_events", []):
            usage[o["graphics_id"]].add(d["name"])
    # script-time graphics changes
    for sc in glob.glob(os.path.join(hns, "data/maps/*/scripts.inc")):
        t = read(sc)
        for g in re.findall(r"OBJ_EVENT_GFX_\w+", t):
            usage[g].add(os.path.basename(os.path.dirname(sc)))

    rows = []
    for old, ent in sorted(rename.items()):
        if ent["action"] != "rename":
            continue
        row = {"old": old, "new": ent["to"], "maps": sorted(usage.get(old, []))}
        sym = pointers.get(old)
        row["info_symbol"] = sym
        if sym is None:
            row["error"] = "no pointer entry"
            rows.append(row)
            continue
        f = info[sym]
        if "pos" in f:
            p = f["pos"]
            row["info"] = dict(zip(["tileTag", "paletteTag", "reflectionPaletteTag", "size", "width", "height", "paletteSlot",
                                    "shadowSize", "inanimate", "compressed", "tracks", "oam", "subspriteTables", "anims",
                                    "images", "affineAnims"], p))
        else:
            row["info"] = f["named"]
        im = row["info"]["images"]
        row["pic_table"] = pics.get(im)
        row["pic_symbols"] = sorted(set(re.findall(r"\b(gObjectEventPic_\w+)", row["pic_table"] or "")))
        row["pic_png"] = {s: pic_src.get(s) for s in row["pic_symbols"]}
        tag = row["info"]["paletteTag"]
        row["pal_tag"] = tag
        psym = pal_table.get(tag)
        ppath = pal_src.get(psym) if psym else None
        row["pal_sym"] = psym
        row["pal_path"] = ppath
        if ppath and os.path.exists(os.path.join(hns, ppath + ".pal")):
            cols = read_pal(os.path.join(hns, ppath + ".pal"))
            row["pal_same_as"] = [t for t, c in t_pal_colors.items() if c == cols]
        elif tag in t_pal_table:
            row["pal_same_as"] = [tag]
        rows.append(row)

    os.makedirs(OUT_DIR, exist_ok=True)
    with open(os.path.join(OUT_DIR, "ow_gfx.json"), "w") as fh:
        json.dump(rows, fh, indent=1)
    errs = [r for r in rows if "error" in r]
    print("sprites:", len(rows), "errors:", len(errs))
    for r in errs:
        print("  ", r["old"], r["error"])
    unused = [r["old"] for r in rows if not r["maps"]]
    print("unused by any map:", len(unused), unused)
    tags = Counter(r["pal_tag"] for r in rows if "pal_tag" in r)
    print("distinct palette tags:", len(tags))
    new_tags = sorted(t for t in tags if t not in t_pal_table and not r_same(rows, t))
    print("tags with no identical target palette:", len(new_tags))
    for t in new_tags:
        print("  ", t, tags[t])


def r_same(rows, tag):
    for r in rows:
        if r.get("pal_tag") == tag:
            return bool(r.get("pal_same_as"))
    return False


if __name__ == "__main__":
    main()
