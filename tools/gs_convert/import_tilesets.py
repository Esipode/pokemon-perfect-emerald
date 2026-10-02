#!/usr/bin/env python3
"""Stage 8: import converted Johto tilesets, door animations and metatile labels.

Reads out/tilesets (convert_tilesets.py), out/rename_map.json (apply_classification.py)
and the HnS clone. Copies tileset data into data/tilesets, appends the tileset
headers/graphics/metatile tables and externs, copies the door PNGs, patches the door
table in src/field_door.c and adds metatile labels used by Johto doors/scripts.
Idempotent: refuses to run twice (markers).

Usage: import_tilesets.py [--hns <clone>]
"""

import argparse
import os
import re
import shutil
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from common import OUT_DIR, ROOT, add_hns_arg, johto_set, load_json, read  # noqa: E402

MARK = "// Johto tilesets (GS Stage 8)"
DOOR_MARK = "// Johto doors (GS Stage 8)"


def snip(path, text, after=None, before=None):
    s = read(path)
    if after:
        i = s.index(after) + len(after)
    elif before:
        i = s.index(before)
    else:
        i = len(s.rstrip("\n")) + 1
    with open(path, "w", encoding="utf-8") as f:
        f.write(s[:i] + text + s[i:])


def main():
    ap = argparse.ArgumentParser()
    add_hns_arg(ap)
    args = ap.parse_args()
    hns = args.hns
    man = load_json(os.path.join(OUT_DIR, "tilesets", "manifest.json"))
    ren = load_json(os.path.join(OUT_DIR, "rename_map.json"))
    conv = {k[5:]: v for k, v in man["tilesets"].items() if k.startswith("conv:")}

    headers = os.path.join(ROOT, "src/data/tilesets/headers.h")
    if MARK in read(headers):
        sys.exit("already imported")

    # 1. data
    src = os.path.join(OUT_DIR, "tilesets", "data", "tilesets")
    for kind in ("primary", "secondary"):
        for d in sorted(os.listdir(os.path.join(src, kind))):
            dst = os.path.join(ROOT, "data/tilesets", kind, d)
            shutil.copytree(os.path.join(src, kind, d), dst)

    # 2. tables
    hdr, gfx, meta, ext = [MARK + "\n"], [MARK + "\n"], [MARK + "\n"], []
    for name, t in sorted(conv.items(), key=lambda kv: kv[1]["symbol"] if "symbol" in kv[1] else kv[0]):
        sym = t.get("symbol") or name
        sec = t["isSecondary"]
        sy = t["symbols"]
        base = os.path.dirname(t["tiles"])
        cb = t["callback"]
        hdr.append(
            "const struct Tileset %s =\n{\n    .isCompressed = TRUE,\n    .isSecondary = %s,\n"
            "    .tiles = %s,\n    .palettes = %s,\n    .metatiles = %s,\n    .metatileAttributes = %s,\n"
            "    .callback = %s,\n};\n" % (sym, "TRUE" if sec else "FALSE", sy["tiles"], sy["palettes"],
                                          sy["metatiles"], sy["attributes"], cb))
        pals = "".join('    INCGFX_U16("%s", ".gbapal"),\n' % p for p in t["palettes"])
        gfx.append('const u32 %s[] = INCGFX_U32("%s", ".4bpp.%s");\n\nconst u16 %s[][16] =\n{\n%s};\n'
                   % (sy["tiles"], t["tiles"], "fastSmol" if sec else "smol", sy["palettes"], pals))
        meta.append('const u16 %s[] = INCBIN_U16("%s");\nconst u16 %s[] = INCBIN_U16("%s");\n'
                    % (sy["metatiles"], t["metatiles"], sy["attributes"], t["attributes"]))
        ext.append("extern const struct Tileset %s;\n" % sym)
    for path, parts in (("src/data/tilesets/headers.h", hdr), ("src/data/tilesets/graphics.h", gfx),
                        ("src/data/tilesets/metatiles.h", meta)):
        p = os.path.join(ROOT, path)
        sep = "\n" if path.endswith("headers.h") else "\n"
        with open(p, "a", encoding="utf-8") as f:
            f.write("\n" + sep.join(parts))
    snip(os.path.join(ROOT, "include/tilesets.h"), "\n" + MARK + "\n" + "".join(ext) + "\n",
         before="#endif //GUARD_tilesets_H")

    # 3. door rows
    fd = read(os.path.join(hns, "src/field_door.c"))
    blk = fd[fd.index("#elif IS_HNS\n    {METATILE_General_Door_PokeCenter"):]
    blk = blk[:blk.index("#endif")]
    rows = re.findall(r"\{(METATILE_\w+),\s*&(\w+),\s*(\w+),\s*(\d),\s*(\w+),\s*(\w+)\}", blk)
    by_orig = {}
    for k, v in conv.items():
        by_orig.setdefault(v["replaces"], []).append(v.get("symbol") or k)

    labels = {}
    for line in read(os.path.join(hns, "include/constants/metatile_labels.h")).splitlines():
        m = re.match(r"#define (METATILE_\w+)\s+(0x[0-9A-Fa-f]+)", line)
        if m:
            labels[m[1]] = m[2]
    existing = set(re.findall(r"#define (METATILE_\w+)", read(os.path.join(ROOT, "include/constants/metatile_labels.h"))))
    lab_map = {}
    tiles_decl, pal_decl, out_rows = {}, {}, []

    def new_label(n):
        if n in lab_map:
            return lab_map[n]
        t = n[:-4] if n.endswith("_Hns") else n
        if t in existing or t in lab_map.values():
            t = t + "_Johto"
        assert t not in existing, t
        lab_map[n] = t
        return t

    pal_src = {m[1]: m[2] for m in re.finditer(r"static const u8 (sDoorAnimPalettes_\w+)\[\] = (\{[^}]*\});", fd)}
    tile_src = {m[1]: m[2] for m in re.finditer(r'static const u8 (sDoorAnimTiles_\w+)\[\] = INCBIN_U8\("graphics/door_anims/(\w+)_hns\.4bpp"\)', fd)}

    def pre(prefix, n):
        tail = n[len(prefix):]
        return prefix + (tail if tail.startswith("Johto") else "Johto_" + tail)

    for lab, tset, snd, size, tiles, pals in rows:
        if tset not in by_orig or tset not in ren["tilesets"]:
            continue
        assert size == "1", (lab, size)
        assert tiles in tile_src, tiles
        png = tile_src[tiles]
        tn, pn = pre("sDoorAnimTiles_", tiles), pre("sDoorAnimPalettes_", pals)
        tiles_decl[tn] = png
        pal_decl[pn] = pal_src[pals]
        lname = new_label(lab)
        for sym in by_orig[tset]:
            out_rows.append((lname, sym, snd, tn, pn))

    def pngname(b):
        return b if b.startswith("johto") else b + "_johto"

    fdp = os.path.join(ROOT, "src/field_door.c")
    for b in tiles_decl.values():
        shutil.copy(os.path.join(hns, "graphics/door_anims", b + "_hns.png"),
                    os.path.join(ROOT, "graphics/door_anims", pngname(b) + ".png"))
    s = read(fdp)
    assert DOOR_MARK not in s
    tdecl = "\n" + DOOR_MARK + "\n" + "".join(
        'static const u8 %s[] = INCGFX_U8("graphics/door_anims/%s.png", ".4bpp");\n' % (n, pngname(b))
        for n, b in sorted(tiles_decl.items()))
    pdecl = "\n" + "".join("static const u8 %s[] = %s;\n" % (n, v) for n, v in sorted(pal_decl.items()))
    rowtxt = "".join(
        "    {\n        .metatileNum = %s,\n        .tileset = &%s,\n        .sound = %s,\n"
        "        .size = DOOR_SIZE_1x1,\n        .tileStart = DOOR_TILE_START_JOHTO,\n"
        "        .tiles = %s,\n        .palettes = %s\n    },\n" % r for r in out_rows)
    a = 'static const u8 sDoorAnimTiles_Teleporter[] = INCGFX_U8("graphics/door_anims/teleporter.png", ".4bpp");\n'
    b2 = "static const u8 sDoorAnimPalettes_Teleporter[] = {8, 8, 8, 8, 8, 8, 8, 8};\n"
    s = s.replace(a, a + tdecl, 1).replace(b2, b2 + pdecl, 1)
    i = s.index("static const struct DoorGraphics sDoorAnimGraphicsTable[]")
    j = s.index("    {},\n};", i)
    s = s[:j] + rowtxt + s[j:]
    with open(fdp, "w", encoding="utf-8") as f:
        f.write(s)

    # 4. metatile labels used by Johto scripts
    for g, folder, _ in johto_set(hns):
        p = os.path.join(hns, "data/maps", folder, "scripts.inc")
        if os.path.exists(p):
            for m in re.findall(r"\bMETATILE_\w+", read(p)):
                if m in labels:
                    new_label(m)
    lines = [MARK.replace("tilesets", "metatile labels")]
    for n in sorted(lab_map, key=lambda k: lab_map[k]):
        lines.append("#define %-60s %s" % (lab_map[n], labels[n]))
    snip(os.path.join(ROOT, "include/constants/metatile_labels.h"), "\n" + "\n".join(lines) + "\n\n",
         before="#endif")
    ren["metatile_labels"] = {k: {"to": v, "action": "rename"} for k, v in lab_map.items()}
    import json
    with open(os.path.join(OUT_DIR, "rename_map.json"), "w") as f:
        json.dump(ren, f, indent=1, sort_keys=True)
    print("tilesets", len(conv), "door rows", len(out_rows), "labels", len(lab_map))


main()
