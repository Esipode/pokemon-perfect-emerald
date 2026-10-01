#!/usr/bin/env python3
"""Build the GS migration manifest from the HnS clone (plan Stage 1).

Collects the Johto set (§0.3), layouts/tilesets (§0.4), referenced tokens against the target
(§0.5), external references, unreachable maps, symbol collisions, wild tables, TM/HM sites
(§0.9), cross-region writes (§0.10) and a ROM estimate (§0.8). Writes
`tools/gs_convert/out/manifest.json` and prints a summary with the plan §0 numbers beside it.
Run from anywhere:

    python3 tools/gs_convert/hns_manifest.py [--hns <path>] [--no-rom]
"""

import argparse
import collections
import glob
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from common import (OUT_DIR, ROOT, add_hns_arg, defined_names, johto_set, load_json,  # noqa: E402
                    map_groups, map_scripts, read, strip_hns, tm_lists)
import tm_sites  # noqa: E402

# Plan §0 numbers this manifest must reproduce (Stage 1 "done when").
EXPECTED = {
    "maps": 257, "layouts": 257, "primaries": 6, "secondaries": 65,
    "tokens.FLAG_": 726, "tokens.VAR_": 108, "tokens.TRAINER_": 306,
    "tokens.OBJ_EVENT_GFX_": 116, "tokens.ITEM_": 239, "tokens.MUS_": 61,
    "missing.FLAG_": 662, "missing.VAR_": 77, "missing.OBJ_EVENT_GFX_": 102, "missing.ITEM_": 21,
    "missing.SPECIES_": 0, "missing.MUS_": 53, "missing.MULTI_": 15, "missing.MAPSEC_": 56,
    "missing.WEATHER_": 1, "missing.MOVE_": 1, "macros": 206, "missing.macros": 14,
    "specials": 109, "missing.specials": 38, "ext_labels": 127, "missing.ext_labels": 30,
    "trainer_pics": 61, "missing.trainer_pics": 36, "trainer_classes": 43, "wild_entries": 152,
    "wild_maps": 96, "tm.one_time": 48, "tm.shop_slots": 16, "label_collisions": 8,
    "localid_collisions": 3, "oversized_layouts": 4, "missing.ITEM_": 22, "unreachable_maps": 3,
}
# Plan §0 values this tool corrected (plan §12 Stage 1 notes).
PLAN_VALUES = {
    "specials": 122, "missing.specials": 39, "ext_labels": 37, "missing.ext_labels": 12,
    "label_collisions": 5, "missing.ITEM_": 21, "unreachable_maps": 2,
}

TOKEN_PREFIXES = ["FLAG_", "VAR_", "TRAINER_", "OBJ_EVENT_GFX_", "ITEM_", "SPECIES_", "MUS_", "SE_",
                  "MAP_", "MOVE_", "MULTI_", "LOCALID_", "MAPSEC_", "WEATHER_", "HEAL_LOCATION_"]
TARGET_DEFS = {
    "FLAG_": ["include/constants/flags*.h"],
    "VAR_": ["include/constants/vars*.h"],
    "TRAINER_": ["include/constants/opponents*.h", "include/constants/trainers.h"],
    "OBJ_EVENT_GFX_": ["include/constants/event_objects.h"],
    "ITEM_": ["include/constants/items.h"],
    "SPECIES_": ["include/constants/species.h"],
    "MUS_": ["include/constants/songs.h"],
    "SE_": ["include/constants/songs.h"],
    "MULTI_": ["include/constants/script_menu.h"],
    "WEATHER_": ["include/constants/weather.h"],
    "MOVE_": ["include/constants/moves.h"],
}
MAX_MAP_DATA_SIZE = 9400
LABEL_RE = re.compile(r"^(\w+)(::?)", re.M)
TERMINATORS = ("end", "return", "goto", "step_end", "releaseall_end")


def code_lines(text):
    """(line number, code without comment) for each non-empty script line."""
    for i, line in enumerate(text.splitlines(), 1):
        code = line.split("@", 1)[0].strip()
        if code:
            yield i, code


# --- Johto set, layouts, tilesets ---------------------------------------------------------

def inventory(hns, jset):
    groups = map_groups(hns)
    regions = collections.Counter()
    for group in groups["group_order"]:
        if group.endswith("_Hns"):
            for name in groups[group]:
                regions[load_json(os.path.join(hns, "data/maps", name, "map.json")).get("region", "none")] += 1
    by_group = collections.Counter(g for g, _, _ in jset)
    return dict(count=len(jset), by_group=dict(by_group), hns_regions=dict(regions),
                maps=[n for _, n, _ in jset])


def tileset_dirs(hns):
    """HnS tileset symbol -> (directory, is_secondary, anim callback)."""
    headers = read(os.path.join(hns, "src/data/tilesets/headers.h"))
    graphics = read(os.path.join(hns, "src/data/tilesets/graphics.h"))
    tile_paths = dict(re.findall(r"(gTilesetTiles_\w+)\[\]\s*=\s*INC\w+\(\"([^\"]+)\"", graphics))
    out = {}
    for sym, body in re.findall(r"const struct Tileset (gTileset_\w+)\s*=\s*\{(.*?)\};", headers, re.S):
        tiles = re.search(r"\.tiles\s*=\s*(\w+)", body).group(1)
        cb = re.search(r"\.callback\s*=\s*(\w+)", body)
        out[sym] = dict(dir=os.path.dirname(tile_paths.get(tiles, "")),
                        secondary="isSecondary = TRUE" in body,
                        callback=cb.group(1) if cb else None)
    return out


def layouts(hns, jset):
    table = {l["id"]: l for l in load_json(os.path.join(hns, "data/layouts/layouts.json"))["layouts"] if "id" in l}
    used = [table[d["layout"]] for _, _, d in jset]
    prim = collections.Counter(l["primary_tileset"] for l in used)
    sec_pairs = collections.defaultdict(set)
    for l in used:
        sec_pairs[l["secondary_tileset"]].add(l["primary_tileset"])
    oversized = {l["id"]: (l["width"] + 15) * (l["height"] + 14) for l in used
                 if (l["width"] + 15) * (l["height"] + 14) > MAX_MAP_DATA_SIZE}
    blockdata = sum(os.path.getsize(os.path.join(hns, l[k])) for l in used
                    for k in ("blockdata_filepath", "border_filepath"))
    return dict(
        count=len(set(l["id"] for l in used)),
        versions=dict(collections.Counter("%s/%s" % (l.get("game_version"), l.get("layout_version")) for l in used)),
        primaries=dict(prim),
        secondaries=sorted(sec_pairs),
        multi_primary_secondaries={s: sorted(p) for s, p in sec_pairs.items() if len(p) > 1},
        pairs=sorted([s, p] for s, ps in sec_pairs.items() for p in ps),
        oversized=oversized,
        blockdata_bytes=blockdata,
        layouts={l["id"]: dict(name=l["name"], width=l["width"], height=l["height"],
                               primary=l["primary_tileset"], secondary=l["secondary_tileset"]) for l in used},
    )


# --- token tables -------------------------------------------------------------------------

def target_items(hns):
    """Resolve HnS ITEM_ tokens: (exists-in-target predicate inputs)."""
    hns_tms, hns_hms = tm_lists(hns, hns_branch=True)
    tgt_tms, tgt_hms = tm_lists(ROOT, hns_branch=False)
    items = defined_names(ROOT, TARGET_DEFS["ITEM_"], "ITEM_")

    def exists(token):
        m = re.match(r"ITEM_(TM|HM)(\d+)$", token)
        if m:
            kind = m.group(1)
            move = (hns_tms if kind == "TM" else hns_hms)[int(m.group(2)) - 1]
        else:
            m = re.match(r"ITEM_(TM|HM)_(\w+)$", token)
            if not m:
                return token in items
            kind, move = m.groups()
        return move in (tgt_tms if kind == "TM" else tgt_hms)
    return exists


def target_mapsecs():
    data = load_json(os.path.join(ROOT, "src/data/region_map/region_map_sections.json"))
    return {s["id"] for s in data["map_sections"]}


def target_macros():
    names = set()
    for path in glob.glob(os.path.join(ROOT, "asm/macros/*.inc")) + glob.glob(os.path.join(ROOT, "asm/macros/**/*.inc")):
        text = read(path)
        names.update(re.findall(r"\.macro\s+(\w+)", text))
        names.update(re.findall(r"create_movement_action\s+(\w+)", text))
    return names


def target_labels():
    """Script label -> defining file (repo-relative) for every label in the target data/."""
    labels = {}
    paths = (glob.glob(os.path.join(ROOT, "data/**/*.inc"), recursive=True)
             + glob.glob(os.path.join(ROOT, "data/*.s")))
    for path in paths:
        for m in LABEL_RE.finditer(read(path)):
            labels.setdefault(m.group(1), os.path.relpath(path, ROOT))
    return labels


def tokens(hns, jset):
    texts = {n: map_scripts(hns, n) for _, n, _ in jset}
    johto_labels = set()
    for t in texts.values():
        johto_labels.update(m.group(1) for m in LABEL_RE.finditer(t))
    tok = collections.defaultdict(collections.Counter)
    macros, specials, ext = collections.Counter(), collections.Counter(), collections.Counter()
    for _, name, data in jset:
        full = texts[name] + "\n" + json.dumps(data)
        for p in TOKEN_PREFIXES:
            tok[p].update(re.findall(r"\b(" + p + r"\w+)", full))
        refs = [e.get("script") for k in ("object_events", "coord_events", "bg_events")
                for e in data.get(k, []) if e.get("script")]
        for _, code in code_lines(texts[name]):
            if LABEL_RE.match(code) or code.startswith("."):
                if code.startswith((".2byte", ".4byte")):
                    refs += re.findall(r"\b\w+\b", code)[1:]
                continue
            word = code.split()[0].rstrip(",")
            if re.match(r"^[a-z_][a-z0-9_]*$", word):
                macros[word] += 1
            sm = re.match(r"special(?:var)?\s+(?:\w+\s*,\s*)?(\w+)", code)
            if sm and word in ("special", "specialvar"):
                specials[sm.group(1)] += 1
                continue
            refs += re.findall(r"\b\w+\b", code)[1:]
        for r in refs:
            # Script labels start with a CamelCase segment (`Common_`, `Route30_`); constants do not.
            if r and r not in johto_labels and re.match(r"[A-Z][A-Za-z0-9]*[a-z][A-Za-z0-9]*_\w+$", r):
                ext[r] += 1
    tok["TRAINER_"] = collections.Counter({k: v for k, v in tok["TRAINER_"].items()
                                           if not k.startswith("TRAINER_TYPE_")})
    return texts, johto_labels, tok, macros, specials, ext


def missing_tokens(hns, tok, macros, specials, ext):
    item_exists = target_items(hns)
    mapsecs = target_mapsecs()
    missing = {}
    for p, files in TARGET_DEFS.items():
        if p == "ITEM_":
            missing[p] = sorted(t for t in tok[p] if not item_exists(t))
            continue
        defs = defined_names(ROOT, files, p)
        missing[p] = sorted(t for t in tok[p] if t not in defs)
    missing["MAPSEC_"] = sorted(t for t in tok["MAPSEC_"] if t not in mapsecs)
    missing["macros"] = sorted(set(macros) - target_macros())
    special_defs = set(re.findall(r"def_special\s+(\w+)", read(os.path.join(ROOT, "data/specials.inc"))))
    missing["specials"] = sorted(set(specials) - special_defs)
    labels = target_labels()
    missing["ext_labels"] = sorted(set(ext) - set(labels))
    return missing, {l: labels[l] for l in sorted(ext) if l in labels}


# --- trainers ---------------------------------------------------------------------------

def rematch_tiers(hns, used_ids):
    """HnS gRematchTable (IS_HNS branch): Johto phone trainers -> their rematch tier ids."""
    text = read(os.path.join(hns, "src/battle_setup.c")).split("#if IS_HNS", 1)[1]
    table = text.split("gRematchTable", 1)[1].split("};", 1)[0]
    out = {}
    for row in re.findall(r"REMATCH\(([^)]*)\)", table):
        ids = [x.strip() for x in row.split(",")[:-1]]
        if ids[0] in used_ids:
            out[ids[0]] = [x for x in ids[1:] if x != "TRAINER_NONE"]
    return out


def trainers(hns, used_ids, texts):
    text = read(os.path.join(hns, "src/data/trainers_hns.party"))
    blocks = re.split(r"^=== (TRAINER_\w+) ===\s*$", text, flags=re.M)
    parties = {blocks[i]: blocks[i + 1] for i in range(1, len(blocks), 2)}
    tgt_pics = defined_names(ROOT, ["include/constants/trainers.h"], "TRAINER_PIC_")
    pics, classes, music = collections.Counter(), collections.Counter(), collections.Counter()
    mons, missing_party = {}, []
    for tid in sorted(used_ids):
        body = parties.get(tid)
        if body is None:
            missing_party.append(tid)
            continue
        field = dict(re.findall(r"^(Pic|Class|Music):\s*(.+?)\s*$", body, re.M))
        pics[field.get("Pic")] += 1
        classes[field.get("Class")] += 1
        music[field.get("Music")] += 1
        head, _, rest = body.partition("\n\n")
        mons[tid] = len([b for b in rest.strip().split("\n\n") if b.strip()])

    def pic_const(name):
        return "TRAINER_PIC_" + re.sub(r"\W+", "_", name.upper()).strip("_")
    missing_pics = sorted(p for p in pics if p and pic_const(p) not in tgt_pics)
    phone = set()
    for t in texts.values():
        phone.update(re.findall(r"^\s*(?:register_matchcall|trainerbattle_rematch\w*)\s+(TRAINER_\w+)", t, re.M))
    return dict(ids=len(used_ids), no_party=missing_party,
                numbered_variants=sorted(t for t in used_ids if re.search(r"_\d+_HNS$", t)),
                phone_trainers=sorted(phone), rematch_table_tiers=rematch_tiers(hns, used_ids),
                pics=dict(pics), missing_pics=missing_pics,
                missing_pics_with_target_name=sorted(p for p in missing_pics if strip_hns(pic_const(p)) in tgt_pics),
                classes=dict(classes), music=dict(music), mons=mons)


# --- references, reachability, collisions ------------------------------------------------

def references(hns, jset, texts):
    ids = {d["id"]: n for _, n, d in jset}
    inbound = collections.defaultdict(set)
    script_inbound = collections.defaultdict(set)
    external = collections.defaultdict(set)
    for _, name, data in jset:
        for w in data.get("warp_events", []):
            dest = w["dest_map"]
            if dest in ids:
                inbound[ids[dest]].add(name)
            elif dest not in ("MAP_DYNAMIC", "MAP_NONE"):
                external[dest].add(name + " (warp)")
        for c in data.get("connections") or []:
            if c["map"] in ids:
                inbound[ids[c["map"]]].add(name)
            else:
                external[c["map"]].add(name + " (connection %s)" % c["direction"])
        for mid in set(re.findall(r"\b(MAP_\w+)", texts[name])):
            if mid in ids:
                script_inbound[ids[mid]].add(name)
            elif not mid.startswith(("MAP_SCRIPT_", "MAP_TYPE_", "MAP_BATTLE_SCENE_", "MAP_OFFSET")):
                external[mid].add(name + " (script)")
    unreachable = sorted(n for _, n, _ in jset if not inbound[n])
    return dict(external={k: sorted(v) for k, v in sorted(external.items())},
                unreachable={n: sorted(script_inbound[n]) for n in unreachable})


def collisions(hns, jset, johto_labels, layout_info):
    tgt_maps, tgt_folders, tgt_localids = set(), set(), {}
    for path in glob.glob(os.path.join(ROOT, "data/maps/*/map.json")):
        d = load_json(path)
        tgt_maps.add(d["id"])
        tgt_folders.add(d["name"])
        for o in d.get("object_events", []):
            if o.get("local_id"):
                tgt_localids[o["local_id"]] = d["name"]
    tgt_layouts = {l["id"] for l in load_json(os.path.join(ROOT, "data/layouts/layouts.json"))["layouts"] if "id" in l}
    tgt_tilesets = set(re.findall(r"const struct Tileset (gTileset_\w+)", read(os.path.join(ROOT, "src/data/tilesets/headers.h"))))
    j_tilesets = set()
    for l in layout_info["layouts"].values():
        j_tilesets.update((l["primary"], l["secondary"]))
    j_localids = {}
    for _, name, d in jset:
        for o in d.get("object_events", []):
            if o.get("local_id"):
                j_localids.setdefault(o["local_id"], set()).add(name)
    labels = target_labels()

    def both(names, target):
        return dict(as_is=sorted(n for n in names if n in target),
                    stripped=sorted(n for n in names if strip_hns(n) in target))
    return dict(
        maps=both({d["id"] for _, _, d in jset}, tgt_maps),
        map_folders=both({n for _, n, _ in jset}, tgt_folders),
        layouts=both(set(layout_info["layouts"]), tgt_layouts),
        tilesets=both(j_tilesets, tgt_tilesets),
        script_labels=sorted(johto_labels & set(labels)),
        script_labels_stripped=sorted(l for l in johto_labels if l not in labels and strip_hns(l) in labels),
        localids={k: dict(johto=sorted(v), target=tgt_localids[k]) for k, v in sorted(j_localids.items())
                  if k in tgt_localids},
    )


# --- wild encounters ----------------------------------------------------------------------

def wild(hns, jset):
    ids = {d["id"] for _, _, d in jset}
    data = load_json(os.path.join(hns, "src/data/wild_encounters.json"))
    entries = [e for g in data["wild_encounter_groups"] if g["label"] == "gWildMonHeaders"
               for e in g["encounters"] if e.get("map") in ids]
    fields = collections.Counter(k for e in entries for k in e if k.endswith("_mons"))
    slots = sum(len(e[k]["mons"]) for e in entries for k in e if k.endswith("_mons"))
    levels = [m[k] for e in entries for f in e if f.endswith("_mons") for m in e[f]["mons"]
              for k in ("min_level", "max_level")]
    suffixes = collections.Counter(e["base_label"].rsplit("_", 1)[-1] for e in entries)
    return dict(entries=len(entries), maps=len({e["map"] for e in entries}), fields=dict(fields),
                time_suffixes=dict(suffixes), slots=slots, level_range=[min(levels), max(levels)])


# --- cross-region writes (§0.10) -----------------------------------------------------------

WRITE_RE = re.compile(r"^(setflag|clearflag|setvar|addvar|subvar|copyvar|setorcopyvar|removeitem|additem|giveitem)\s+(\w+)")
SCRATCH_RE = re.compile(r"^(FLAG_TEMP_|FLAG_HIDDEN_ITEMS_START|VAR_TEMP_|VAR_0x|VAR_RESULT|VAR_FACING|VAR_LAST_|VAR_SPECIAL_|VAR_UNUSED|VAR_ITEM_ID$|FLAG_SPECIAL_|FLAG_SYS_CTRL_OBJ_DELETE$)")


def key_items():
    """Target items that the player keeps once (Key Items pocket, TM/HM pocket HMs)."""
    text = read(os.path.join(ROOT, "src/data/items.h"))
    keys = set()
    for name, body in re.findall(r"\[(ITEM_\w+)\]\s*=\s*\{(.*?)\n    \},", text, re.S):
        if "POCKET_KEY_ITEMS" in body or name.startswith("ITEM_HM"):
            keys.add(name)
    return keys


def reachable_labels(jset, texts):
    """Labels reached from map events and map_script tables, following jumps and fallthrough."""
    body, order = {}, []
    for _, name, _ in jset:
        current = None
        for _, code in code_lines(texts[name]):
            m = LABEL_RE.match(code)
            if m:
                if current is not None and (not body[current] or body[current][-1].split()[0] not in TERMINATORS):
                    body[current].append("__fallthrough__ " + m.group(1))
                current = m.group(1)
                body[current] = []
                order.append(current)
            elif current is not None:
                body[current].append(code)
    roots = set()
    for _, name, d in jset:
        roots.update(e.get("script") for k in ("object_events", "coord_events", "bg_events")
                     for e in d.get(k, []) if e.get("script"))
        for code in (c for _, c in code_lines(texts[name])):
            if code.startswith("map_script"):
                roots.update(re.findall(r"\b\w+\b", code)[1:])
        roots.add(name.replace("_hns", "") + "_MapScripts")
        roots.add(name + "_MapScripts")
    seen, stack = set(), [r for r in roots if r in body]
    while stack:
        lab = stack.pop()
        if lab in seen:
            continue
        seen.add(lab)
        for code in body[lab]:
            for ref in re.findall(r"\b\w+\b", code):
                if ref in body and ref not in seen:
                    stack.append(ref)
    return seen


def cross_region_writes(jset, texts, reachable):
    names = set()
    for p in ("FLAG_", "VAR_"):
        names |= defined_names(ROOT, TARGET_DEFS[p], p)
    keys = key_items()
    rows = []
    for _, name, _ in jset:
        label = None
        for lineno, code in code_lines(texts[name]):
            m = LABEL_RE.match(code)
            if m:
                label = m.group(1)
                continue
            w = WRITE_RE.match(code)
            if not w:
                continue
            cmd, target = w.groups()
            if cmd in ("removeitem", "additem", "giveitem"):
                if cmd != "removeitem" and target not in keys:
                    continue
            elif target not in names or SCRATCH_RE.match(target):
                continue
            rows.append(dict(map=name, line=lineno, label=label, cmd=cmd, name=target, code=code,
                             reachable=label in reachable))
    by_name = collections.defaultdict(list)
    for r in rows:
        by_name[r["name"]].append("%s:%d %s" % (r["map"], r["line"], r["cmd"]))
    return dict(rows=rows, names=sorted(by_name), count=len(rows),
                unreachable=[r for r in rows if not r["reachable"]])


# --- ROM estimate --------------------------------------------------------------------------

def run(cmd, **kw):
    subprocess.run(cmd, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, **kw)


def compressed_size(raw, tmp, fast):
    src = os.path.join(tmp, "c.bin")
    dst = src + ".smol"
    with open(src, "wb") as f:
        f.write(raw)
    if fast:
        run([os.path.join(ROOT, "tools/compresSmol/compresSmol"), "-w", src, dst, "false", "false", "false"])
    else:
        run([os.path.join(ROOT, "tools/compresSmol/compresSmol"), "-w", src, dst])
    return os.path.getsize(dst)


def png_to_4bpp(png, tmp):
    out = os.path.join(tmp, "g.4bpp")
    run([os.path.join(ROOT, "tools/gbagfx/gbagfx"), png, out])
    with open(out, "rb") as f:
        return f.read()


def png_dims(png):
    with open(png, "rb") as f:
        head = f.read(24)
    return int.from_bytes(head[16:20], "big"), int.from_bytes(head[20:24], "big")


def rom_used():
    out = subprocess.run(["arm-none-eabi-objdump", "-h", os.path.join(ROOT, "pokeemerald.elf")],
                         check=True, capture_output=True, text=True).stdout
    end = 0
    for m in re.finditer(r"^\s*\d+\s+\S+\s+([0-9a-f]+)\s+[0-9a-f]+\s+([0-9a-f]+)\s+[0-9a-f]+\s+\S+\n\s+(.*)$", out, re.M):
        size, lma, flags = int(m.group(1), 16), int(m.group(2), 16), m.group(3)
        if "LOAD" in flags and 0x08000000 <= lma < 0x0A000000:
            end = max(end, lma + size)
    return end - 0x08000000


def est_tilesets(hns, lay, tmp):
    dirs = tileset_dirs(hns)
    out = dict(primaries={}, pairs={}, total=0)
    tile = 32
    for sym in lay["primaries"]:
        d = os.path.join(hns, dirs[sym]["dir"])
        raw = png_to_4bpp(os.path.join(d, "tiles.png"), tmp)
        lo, hi = raw[:512 * tile], raw[512 * tile:640 * tile]
        meta = os.path.getsize(os.path.join(d, "metatiles.bin"))
        attr = os.path.getsize(os.path.join(d, "metatile_attributes.bin"))
        out["primaries"][sym] = dict(
            tiles_lo=compressed_size(lo, tmp, fast=False),
            tiles_hi=compressed_size(hi, tmp, fast=True) if hi else 0,
            metatiles_lo=min(meta, 512 * 16) + min(attr, 512 * 2),
            metatiles_hi=max(0, meta - 512 * 16) + max(0, attr - 512 * 2),
            palettes=13 * 32)
    for sec, prim in lay["pairs"]:
        d = os.path.join(hns, dirs[sec]["dir"])
        raw = png_to_4bpp(os.path.join(d, "tiles.png"), tmp)
        p = out["primaries"][prim]
        size = (compressed_size(raw, tmp, fast=True) + p["tiles_hi"]
                + os.path.getsize(os.path.join(d, "metatiles.bin"))
                + os.path.getsize(os.path.join(d, "metatile_attributes.bin"))
                + p["metatiles_hi"] + 13 * 32 + 24)
        out["pairs"]["%s+%s" % (sec, prim)] = size
    out["total"] = (sum(p["tiles_lo"] + p["metatiles_lo"] + p["palettes"] + 24 for p in out["primaries"].values())
                    + sum(out["pairs"].values()))
    return out


def voicegroups(hns):
    groups, current = {}, None
    for path in glob.glob(os.path.join(hns, "sound/voicegroups/*.inc")):
        for _, code in code_lines(read(path)):
            m = re.match(r"(\w+)::", code)
            if m:
                current = m.group(1)
                groups[current] = []
            elif current and code.startswith("voice_"):
                groups[current].append(code)
    return groups


def sample_paths(root):
    text = read(os.path.join(root, "sound/direct_sound_data.inc"))
    return dict(re.findall(r"(DirectSoundWaveData_\w+)::\s*\n\s*\.incbin\s+\"([^\"]+)\"", text))


def file_hash(path):
    with open(path, "rb") as f:
        return hashlib.sha1(f.read()).hexdigest()


def est_music(hns, songs, tmp):
    cfg = dict(re.findall(r"^(\S+)\.mid:\s*(.*)$", read(os.path.join(hns, "sound/songs/midi/midi.cfg")), re.M))
    groups = voicegroups(hns)
    hns_samples, tgt_samples = sample_paths(hns), sample_paths(ROOT)
    song_sizes, roots, problems = {}, set(), []
    for const in songs:
        name = const.lower()
        mid = os.path.join(hns, "sound/songs/midi", name + ".mid")
        if not os.path.exists(mid) or name not in cfg:
            problems.append(const)
            continue
        s, o = os.path.join(tmp, name + ".s"), os.path.join(tmp, name + ".o")
        run([os.path.join(ROOT, "tools/mid2agb/mid2agb"), mid, s] + cfg[name].split())
        run(["arm-none-eabi-as", "-mcpu=arm7tdmi", "-I", os.path.join(hns, "sound"), "-o", o, s])
        sizes = subprocess.run(["arm-none-eabi-size", "-A", o], check=True, capture_output=True, text=True).stdout
        song_sizes[const] = sum(int(x) for x in re.findall(r"^\.(?:data|rodata|text)\S*\s+(\d+)", sizes, re.M))
        roots.update(re.findall(r"\.equ\s+\w+_grp,\s*(voicegroup\d+)", read(s)))
    seen, stack, samples, keysplits = set(), list(roots), set(), set()
    while stack:
        g = stack.pop()
        if g in seen or g not in groups:
            continue
        seen.add(g)
        for v in groups[g]:
            stack += re.findall(r"\b(voicegroup\d+)\b", v)
            samples.update(re.findall(r"\b(DirectSoundWaveData_\w+)", v))
            keysplits.update(re.findall(r"\b(KeySplitTable\d+)", v))
    voice_bytes = sum(12 * len(groups[g]) for g in seen)
    new_samples, shared, conflicts = {}, [], []
    for smp in sorted(samples):
        rel = hns_samples.get(smp)
        if not rel:
            problems.append(smp)
            continue
        wav = os.path.join(hns, os.path.splitext(rel)[0] + ".wav")
        if smp in tgt_samples:
            twav = os.path.join(ROOT, os.path.splitext(tgt_samples[smp])[0] + ".wav")
            if os.path.exists(twav) and file_hash(twav) == file_hash(wav):
                shared.append(smp)
                continue
            conflicts.append(smp)
        out = os.path.join(tmp, "s.bin")
        run([os.path.join(ROOT, "tools/wav2agb/wav2agb"), "-b", wav, out])
        new_samples[smp] = os.path.getsize(out)
    return dict(songs=song_sizes, voicegroups=len(seen), voice_bytes=voice_bytes,
                keysplit_tables=len(keysplits), samples_shared=len(shared),
                samples_name_conflicts=conflicts, samples_new=new_samples,
                total=sum(song_sizes.values()) + voice_bytes + 128 * len(keysplits) + sum(new_samples.values()),
                problems=problems)


def est_object_events(hns, missing_gfx):
    ptrs = dict(re.findall(r"\[(OBJ_EVENT_GFX_\w+)\]\s*=\s*&(\w+)",
                           read(os.path.join(hns, "src/data/object_events/object_event_graphics_info_pointers.h"))))
    info = read(os.path.join(hns, "src/data/object_events/object_event_graphics_info.h"))
    tables = read(os.path.join(hns, "src/data/object_events/object_event_pic_tables.h"))
    gfx = read(os.path.join(hns, "src/data/object_events/object_event_graphics.h"))
    pic_paths = dict(re.findall(r"(gObjectEventPic_\w+)\[\]\s*=\s*INC\w+\(\"([^\"]+)\"", gfx))
    pics, palettes, unresolved = set(), set(), []
    for g in missing_gfx:
        sym = ptrs.get(g)
        m = sym and re.search(r"\b" + sym + r"\s*=\s*\{(.*?)\};", info, re.S)
        if not m:
            unresolved.append(g)
            continue
        # Designated (`.images = `) or positional initializer.
        tab = re.search(r"\.images\s*=\s*(\w+)", m.group(1)) or re.search(r"\b(sPicTable_\w+)", m.group(1))
        pal = (re.search(r"\.paletteTag\s*=\s*(\w+)", m.group(1))
               or re.search(r"\b(OBJ_EVENT_PAL_TAG_(?!NONE\b)\w+)", m.group(1)))
        if pal:
            palettes.add(pal.group(1))
        t = tab and re.search(r"\b" + tab.group(1) + r"\[\]\s*=\s*\{(.*?)\};", tables, re.S)
        if t:
            pics.update(re.findall(r"(gObjectEventPic_\w+)", t.group(1)))
    size = 0
    for p in pics:
        path = pic_paths.get(p)
        png = path and os.path.join(hns, re.sub(r"\.4bpp.*$", ".png", path))
        if png and os.path.exists(png):
            w, h = png_dims(png)
            size += w * h // 2
        else:
            unresolved.append(p)
    return dict(gfx=len(missing_gfx), pics=len(pics), palettes=len(palettes),
                total=size + 32 * len(palettes) + 64 * len(missing_gfx), unresolved=unresolved)


def est_trainer_pics(hns, missing_pics, tmp):
    gfx = read(os.path.join(hns, "src/data/graphics/trainers.h"))
    paths = dict(re.findall(r"(gTrainerFrontPic_\w+)\[\]\s*=\s*INC\w+\(\"([^\"]+)\"", gfx))
    sprites = dict(re.findall(r"TRAINER_SPRITE\((TRAINER_PIC_\w+),\s*(gTrainerFrontPic_\w+)", gfx))
    size, unresolved = 0, []
    for name in missing_pics:
        const = "TRAINER_PIC_FRONT_" + re.sub(r"\W+", "_", name.upper()).strip("_")
        path = paths.get(sprites.get(const, ""))
        png = path and os.path.join(hns, re.sub(r"\.4bpp.*$", ".png", path))
        if not png or not os.path.exists(png):
            unresolved.append(name)
            continue
        size += compressed_size(png_to_4bpp(png, tmp), tmp, fast=False) + 32
    return dict(total=size, unresolved=unresolved)


def est_scripts(jset, texts):
    move_macros = set(re.findall(r"create_movement_action\s+(\w+)", read(os.path.join(ROOT, "asm/macros/movement.inc"))))
    text_bytes = cmd_bytes = 0
    for _, name, d in jset:
        for _, code in code_lines(texts[name]):
            if code.startswith(".string"):
                s = re.sub(r"\{[^}]*\}", "xx", code[len(".string"):].strip().strip('"'))
                text_bytes += len(s.replace("\\n", "n").replace("\\p", "p").replace("\\l", "l"))
            elif code.startswith(".2byte"):
                cmd_bytes += 2 * len(code.split(","))
            elif code.startswith(".4byte"):
                cmd_bytes += 4 * len(code.split(","))
            elif code.startswith(".byte"):
                cmd_bytes += len(code.split(","))
            elif LABEL_RE.match(code) or code.startswith("."):
                continue
            else:
                cmd_bytes += 1 if code.split()[0] in move_macros else 5
        cmd_bytes += 28 + 20 + 24 * len(d.get("object_events", [])) + 8 * len(d.get("warp_events", []))
        cmd_bytes += 16 * len(d.get("coord_events", [])) + 12 * len(d.get("bg_events", []))
        cmd_bytes += 12 * len(d.get("connections") or []) + 8
    return dict(text=text_bytes, commands_and_events=cmd_bytes, total=text_bytes + cmd_bytes)


def rom_estimate(hns, lay, tr, missing, jset, texts, wild_info):
    tmp = tempfile.mkdtemp(prefix="gs_manifest_")
    try:
        used = rom_used()
        parts = {}
        parts["tilesets"] = est_tilesets(hns, lay, tmp)
        parts["blockdata"] = dict(total=lay["blockdata_bytes"] + 24 * lay["count"])
        parts["music"] = est_music(hns, missing["MUS_"], tmp)
        parts["object_events"] = est_object_events(hns, [g for g in missing["OBJ_EVENT_GFX_"]
                                                         if g != "OBJ_EVENT_GFX_MON_BASE"])
        parts["trainer_pics"] = est_trainer_pics(hns, tr["missing_pics"], tmp)
        kept = list(tr["mons"])
        trainers_count = 864 + 624
        gap = 1632 - trainers_count
        parts["trainers"] = dict(kept=len(kept), mons=sum(tr["mons"][t] for t in kept),
                                 total=52 * 3 * (len(kept) + gap) + 40 * sum(tr["mons"][t] for t in kept),
                                 note="52 B x 3 difficulties per gTrainers id, incl. %d unused ids between "
                                      "Kanto TRAINERS_COUNT and JOHTO_TRAINERS_START; 40 B per TrainerMon" % gap)
        parts["scripts_text"] = est_scripts(jset, texts)
        parts["wild"] = dict(total=wild_info["slots"] * 4 * 2 + wild_info["entries"] * 2 * 48,
                             note="Day->Morning+Day, Night->Evening+Night doubles slot data")
        parts["tm_teachables"] = dict(total=22 * 1024, note="plan §0.9 estimate for 64 TMs")
        total = sum(p["total"] for p in parts.values())
        no_music = total - parts["music"]["total"]
        free = 0x2000000 - used
        return dict(rom_used=used, rom_free=free, parts=parts, total=total, total_without_music=no_music,
                    free_after=free - total, free_after_without_music=free - no_music)
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


# --- main ----------------------------------------------------------------------------------

def check(manifest):
    got = {
        "maps": manifest["johto_set"]["count"], "layouts": manifest["layouts"]["count"],
        "primaries": len(manifest["layouts"]["primaries"]), "secondaries": len(manifest["layouts"]["secondaries"]),
        "macros": len(manifest["macros"]), "specials": len(manifest["specials"]),
        "ext_labels": len(manifest["ext_labels"]),
        "trainer_pics": len(manifest["trainers"]["pics"]),
        "missing.trainer_pics": len(manifest["trainers"]["missing_pics"]),
        "trainer_classes": len(manifest["trainers"]["classes"]),
        "wild_entries": manifest["wild"]["entries"], "wild_maps": manifest["wild"]["maps"],
        "tm.one_time": manifest["tm_sites"]["summary"]["one_time"],
        "tm.shop_slots": manifest["tm_sites"]["summary"]["shop_slots"],
        "label_collisions": len(manifest["collisions"]["script_labels"]),
        "localid_collisions": len(manifest["collisions"]["localids"]),
        "oversized_layouts": len(manifest["layouts"]["oversized"]),
        "unreachable_maps": len(manifest["references"]["unreachable"]),
    }
    for p, v in manifest["tokens"].items():
        got["tokens." + p] = len(v)
    for p, v in manifest["missing"].items():
        got["missing." + p] = len(v)
    return {k: dict(expected=v, got=got.get(k), ok=got.get(k) == v, plan=PLAN_VALUES.get(k, v))
            for k, v in EXPECTED.items()}


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    add_hns_arg(parser)
    parser.add_argument("--no-rom", action="store_true", help="skip the ROM estimate (needs built tools)")
    parser.add_argument("--out", default=os.path.join(OUT_DIR, "manifest.json"))
    args = parser.parse_args()
    hns = args.hns
    head = subprocess.run(["git", "-C", hns, "rev-parse", "--short=10", "HEAD"], capture_output=True, text=True).stdout.strip()

    jset = johto_set(hns)
    lay = layouts(hns, jset)
    texts, johto_labels, tok, macros, specials, ext = tokens(hns, jset)
    missing, ext_files = missing_tokens(hns, tok, macros, specials, ext)
    tr = trainers(hns, set(tok["TRAINER_"]), texts)
    sites, site_refs = tm_sites.scan(hns)
    wild_info = wild(hns, jset)
    reach = reachable_labels(jset, texts)
    manifest = dict(
        hns_head=head,
        johto_set=inventory(hns, jset),
        layouts=lay,
        tilesets={k: v for k, v in tileset_dirs(hns).items()
                  if k in lay["primaries"] or k in lay["secondaries"]},
        tokens={p: dict(sorted(c.items())) for p, c in tok.items()},
        macros=dict(sorted(macros.items())),
        specials=dict(sorted(specials.items())),
        ext_labels=dict(sorted(ext.items())),
        ext_label_target_files=ext_files,
        missing=missing,
        trainers=tr,
        references=references(hns, jset, texts),
        collisions=collisions(hns, jset, johto_labels, lay),
        wild=wild_info,
        tm_sites=dict(sites=sites, references=site_refs, summary=tm_sites.summary(sites)),
        cross_region_writes=cross_region_writes(jset, texts, reach),
    )
    if not args.no_rom:
        manifest["rom"] = rom_estimate(hns, lay, tr, missing, jset, texts, wild_info)
    manifest["check"] = check(manifest)

    os.makedirs(os.path.dirname(args.out), exist_ok=True)
    with open(args.out, "w") as f:
        json.dump(manifest, f, indent=1, default=sorted)

    print("HnS HEAD %s -> %s" % (head, os.path.relpath(args.out, ROOT)))
    for k, v in manifest["check"].items():
        note = "" if v["plan"] == v["expected"] else "  (plan said %s)" % v["plan"]
        print("  %-26s expected %-5s got %-5s %s%s" % (k, v["expected"], v["got"], "ok" if v["ok"] else "DIFF", note))
    if "rom" in manifest:
        r = manifest["rom"]
        print("ROM used %d, free %d" % (r["rom_used"], r["rom_free"]))
        for k, p in r["parts"].items():
            print("  %-16s %10d" % (k, p["total"]))
        print("  %-16s %10d  (free after: %d)" % ("total", r["total"], r["free_after"]))
        print("  %-16s %10d  (free after: %d)" % ("without music", r["total_without_music"], r["free_after_without_music"]))


if __name__ == "__main__":
    main()
