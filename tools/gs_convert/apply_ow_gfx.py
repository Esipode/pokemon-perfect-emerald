#!/usr/bin/env python3
"""Stage 9: write the Johto overworld sprites into the target tree.

Input: out/ow_gfx.json (from ow_gfx.py). Edits the object-event data headers, event_objects.h and the
palette table, and copies PNG/PAL assets. Refuses to run twice (looks for the Johto marker).
Pass --dry-run to print the report only. Standard library only.
"""

import argparse
import hashlib
import json
import os
import re
import shutil
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from common import OUT_DIR, ROOT, add_hns_arg, read  # noqa: E402

OBJ = "src/data/object_events"
MARK = "// Johto overworld sprites (Stage 9)"
PAL_TAG_START = 0x1170
SLOT_NAMES = ["PALSLOT_PLAYER", "PALSLOT_PLAYER_REFLECTION", "PALSLOT_NPC_1", "PALSLOT_NPC_2", "PALSLOT_NPC_3",
              "PALSLOT_NPC_4", "PALSLOT_NPC_1_REFLECTION", "PALSLOT_NPC_2_REFLECTION", "PALSLOT_NPC_3_REFLECTION",
              "PALSLOT_NPC_4_REFLECTION", "PALSLOT_NPC_SPECIAL", "PALSLOT_NPC_SPECIAL_REFLECTION"]
# HnS anim tables that already exist in the target under their plain name.
ANIM_REUSE = {"sAnimTable_Nurse_hns": "sAnimTable_Nurse", "sAnimTable_BreakableRock_hns": "sAnimTable_BreakableRock",
              "sAnimTable_CuttableTree_hns": "sAnimTable_CuttableTree"}
ANIM_NEW = {"sAnimTable_TowerBeam_hns": "sAnimTable_TowerBeam_Johto", "sAnimTable_Whirlpool_hns": "sAnimTable_Whirlpool_Johto"}
FIELD_ORDER = ["tileTag", "paletteTag", "reflectionPaletteTag", "size", "width", "height", "paletteSlot", "shadowSize",
               "inanimate", "compressed", "tracks", "oam", "subspriteTables", "anims", "images", "affineAnims"]


def md5(path):
    with open(path, "rb") as f:
        return hashlib.md5(f.read()).hexdigest()


def read_pal(path):
    t = read(path).split()
    n = int(t[2])
    return tuple(tuple(int(x) for x in t[3 + i * 3:6 + i * 3]) for i in range(n))


def johto_name(sym):
    return re.sub(r"_hns$", "_Johto", sym)


def main():
    ap = argparse.ArgumentParser()
    add_hns_arg(ap)
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()
    hns = args.hns
    rows = json.load(open(os.path.join(OUT_DIR, "ow_gfx.json")))
    # SMALL_LIGHT is a light-system sprite (field-effect pic table), not a pic-table sprite; Stage 12 shim.
    skipped = [r["old"] for r in rows if not r["pic_table"]]
    rows = [r for r in rows if r["pic_table"]]
    print("skipped (light sprites):", skipped)

    def tpath(p):
        return os.path.join(ROOT, p)

    t_gfx = read(tpath(f"{OBJ}/object_event_graphics.h"))
    t_pic_by_md5 = {}
    for m in re.finditer(r'const u(?:16|32) (\w+)\[\] = INCGFX_U(?:16|32)\("([^"]+\.png)"', t_gfx):
        if os.path.exists(tpath(m.group(2))):
            t_pic_by_md5.setdefault(md5(tpath(m.group(2))), m.group(1))
    t_pal_syms = {m.group(2): m.group(1) for m in re.finditer(r"\{(gObjectEventPal_\w+),\s*(OBJ_EVENT_PAL_TAG_\w+)\}", read(tpath("src/event_object_movement.c")))}
    t_pal_files = {m.group(1): m.group(2) for m in re.finditer(r'const u16 (\w+)\[\] = INCGFX_U16\("([^"]+)\.pal", "\.gbapal"\)', t_gfx)}
    t_tags = set(re.findall(r"#define (OBJ_EVENT_PAL_TAG_\w+)", read(tpath("include/constants/event_objects.h"))))
    t_pal_colors = {}
    for tag, sym in t_pal_syms.items():
        if tag.startswith("OBJ_EVENT_PAL_TAG_NPC_") and sym in t_pal_files and os.path.exists(tpath(t_pal_files[sym] + ".pal")):
            t_pal_colors.setdefault(read_pal(tpath(t_pal_files[sym] + ".pal")), tag)

    # HnS pic table symbol dims (mwidth/mheight in tiles) per pic symbol.
    pic_dims = {}
    for r in rows:
        for s, w, h in re.findall(r"overworld_\w+\((\w+),\s*(\d+),\s*(\d+)", r["pic_table"] or ""):
            pic_dims.setdefault(s, (int(w), int(h)))
    for r in rows:  # obj_frame_tiles() tables carry no dimensions; use the info width/height
        for s in r["pic_symbols"]:
            pic_dims.setdefault(s, (int(r["info"]["width"]) // 8, int(r["info"]["height"]) // 8))

    new_pics = {}      # symbol -> (hns png path, w, h)
    pic_remap = {}     # hns symbol -> target symbol
    for r in rows:
        for s, p in r["pic_png"].items():
            if s in pic_remap:
                continue
            src = os.path.join(hns, p + ".png")
            hit = t_pic_by_md5.get(md5(src))
            if hit:
                pic_remap[s] = hit
            else:
                new = johto_name(s) if s.endswith("_hns") else s + "_Johto"
                pic_remap[s] = new
                new_pics[new] = (p, *pic_dims[s])

    # palettes
    tag_remap = {}
    new_pals = {}      # new tag -> (symbol, hns path)
    next_tag = PAL_TAG_START
    for r in rows:
        tag = r["pal_tag"]
        if tag in tag_remap:
            continue
        if tag in t_tags:
            tag_remap[tag] = tag
            continue
        cols = read_pal(os.path.join(hns, r["pal_path"] + ".pal"))
        if cols in t_pal_colors:
            tag_remap[tag] = t_pal_colors[cols]
            continue
        new = re.sub(r"_HNS$", "_JOHTO", tag)
        assert new not in t_tags, new
        tag_remap[tag] = new
        base = os.path.basename(r["pal_path"]).replace("_hns", "")
        new_pals[new] = (f"gObjectEventPal_{''.join(w.capitalize() for w in base.split('_'))}_Johto", r["pal_path"], base, next_tag)
        next_tag += 1

    report = [f"sprites {len(rows)}", f"pics reused from target {sum(1 for s,v in pic_remap.items() if s not in {a for a in []} and v not in new_pics)}",
              f"new pics {len(new_pics)}", f"palette tags remapped to existing {sum(1 for k,v in tag_remap.items() if k != v)}",
              f"new palette tags {len(new_pals)}"]
    print("\n".join(report))
    if args.dry_run:
        return

    consts = read(tpath("include/constants/event_objects.h"))
    if MARK in consts:
        sys.exit("already applied")

    # assets
    for new, (p, w, h) in new_pics.items():
        dst = tpath("graphics/object_events/pics/people/johto/" + os.path.basename(p).replace("_hns", "") + ".png")
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        shutil.copyfile(os.path.join(hns, p + ".png"), dst)
    for new, (sym, p, base, val) in new_pals.items():
        dst = tpath("graphics/object_events/palettes/johto/" + base + ".pal")
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        shutil.copyfile(os.path.join(hns, p + ".pal"), dst)

    # constants: gfx ids + palette tags
    ids = "".join(f"    {r['new']},\n" for r in rows)
    consts = consts.replace("    NUM_OBJ_EVENT_GFX,\n", f"    {MARK}\n{ids}    NUM_OBJ_EVENT_GFX,\n", 1)
    tags = "".join(f"#define {t:<42}0x{v[3]:04X}\n" for t, v in new_pals.items())
    anchor = "#define OBJ_EVENT_PAL_TAG_BALL_MASTER"
    consts = consts.replace(anchor, f"// Johto\n{tags}\n{anchor}", 1)
    open(tpath("include/constants/event_objects.h"), "w").write(consts)

    # graphics INCGFX / INCBIN
    g = t_gfx.rstrip("\n") + f"\n\n{MARK}\n"
    for sym, (p, w, h) in new_pics.items():
        rel = "graphics/object_events/pics/people/johto/" + os.path.basename(p).replace("_hns", "") + ".png"
        g += f'const u16 {sym}[] = INCGFX_U16("{rel}", ".4bpp", "-mwidth {w} -mheight {h}");\n'
    for tag, (sym, p, base, val) in new_pals.items():
        g += f'const u16 {sym}[] = INCGFX_U16("graphics/object_events/palettes/johto/{base}.pal", ".gbapal");\n'
    open(tpath(f"{OBJ}/object_event_graphics.h"), "w").write(g)

    # palette table
    mv = read(tpath("src/event_object_movement.c"))
    entries = "".join(f"    {{{sym + ',':<39}{tag}}},\n" for tag, (sym, p, base, val) in new_pals.items())
    anchor = "    {gObjectEventPal_May,                   OBJ_EVENT_PAL_TAG_SURF_BLOB_MAY},\n"
    assert anchor in mv
    mv = mv.replace(anchor, anchor + entries, 1)
    open(tpath("src/event_object_movement.c"), "w").write(mv)

    # pic tables + info structs + pointers
    pt, info_out, ext, ptr = f"\n{MARK}\n", f"\n{MARK}\n", "", ""
    for r in rows:
        name = johto_name(r["info"]["images"])
        body = r["pic_table"]
        for s, n in pic_remap.items():
            body = re.sub(rf"\b{s}\b", n, body)
        pt += f"static const struct SpriteFrameImage {name}[] = {{\n    {body}\n}};\n\n"
        i = dict(r["info"])
        i["paletteTag"] = tag_remap[i["paletteTag"]]
        slot = i["paletteSlot"]
        i["paletteSlot"] = SLOT_NAMES[int(slot)] if slot.isdigit() else slot
        i["images"] = name
        i["anims"] = ANIM_REUSE.get(i["anims"], ANIM_NEW.get(i["anims"], i["anims"]))
        sym = johto_name(r["info_symbol"])
        info_out += f"const struct ObjectEventGraphicsInfo {sym} = {{\n"
        info_out += "".join(f"    .{k} = {i[k]},\n" for k in FIELD_ORDER)
        info_out += "};\n\n"
        ext += f"extern const struct ObjectEventGraphicsInfo {sym};\n"
        ptr += f"    [{r['new']}] = &{sym},\n"

    def append(path, text):
        with open(tpath(path), "a") as f:
            f.write(text)

    append(f"{OBJ}/object_event_pic_tables.h", pt.rstrip("\n") + "\n")
    append(f"{OBJ}/object_event_graphics_info.h", info_out.rstrip("\n") + "\n")

    anims = (f"\n{MARK}\n"
             + re.sub(r"sAnimTable_TowerBeam_hns", "sAnimTable_TowerBeam_Johto", _anim_block(hns, "sAnimTable_TowerBeam_hns"))
             + "\n"
             + re.sub(r"sAnimTable_Whirlpool_hns", "sAnimTable_Whirlpool_Johto", _anim_block(hns, "sAnimTable_Whirlpool_hns")))
    append(f"{OBJ}/object_event_anims.h", anims)

    pp = read(tpath(f"{OBJ}/object_event_graphics_info_pointers.h"))
    a = "extern const struct ObjectEventGraphicsInfo gObjectEventGraphicsInfo_ApricornTree;\n"
    assert a in pp
    pp = pp.replace(a, a + ext, 1)
    head, rest = pp.split("const struct ObjectEventGraphicsInfo *const gObjectEventGraphicsInfoPointers[NUM_OBJ_EVENT_GFX] = {", 1)
    arr, tail = rest.split("\n};\n", 1)
    pp = head + "const struct ObjectEventGraphicsInfo *const gObjectEventGraphicsInfoPointers[NUM_OBJ_EVENT_GFX] = {" + arr + "\n" + ptr.rstrip("\n") + "\n};\n" + tail
    open(tpath(f"{OBJ}/object_event_graphics_info_pointers.h"), "w").write(pp)
    print("applied")


def _anim_block(hns, name):
    t = read(os.path.join(hns, OBJ, "object_event_anims.h"))
    m = re.search(rf"static const union AnimCmd \*const {name}\[\] = \{{.*?\n\}};\n", t, re.S)
    return m.group(0)


if __name__ == "__main__":
    main()
