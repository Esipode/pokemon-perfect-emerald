#!/usr/bin/env python3
"""Apply the reviewed flag/var classification (Stage 6).

Reads the reviewed `GS - Flag Classification.md`, assigns Johto ids with the Stage 5 rules,
writes `include/constants/flags_johto.h` / `vars_johto.h` and `out/rename_map.json` (the one
HnS -> target name table every later import stage uses). Run once; the headers are hand-maintained
afterwards. Standard library only. Run from the repo root:

    python3 tools/gs_convert/apply_classification.py --table "<path>/GS - Flag Classification.md"
    python3 tools/gs_convert/apply_classification.py --table "<path>" --write
"""

import argparse
import json
import os
import re
import sys
from collections import defaultdict

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import classify_flags_vars as cfv  # noqa: E402
from common import ROOT, OUT_DIR, add_hns_arg, johto_set, load_json, read, strip_hns  # noqa: E402

VALID_BUCKETS = {"Johto", "Johto (renamed)", "Shared", "Config", "Dropped"}

BLOCK_TITLES = {
    "hidden_item": "Hidden items",
    "item_ball": "Item balls",
    "story": "Story and scene state",
    "hide": "Object hide flags",
    "gift": "Gifts and one-time events",
    "badge": "Badges",
    "champion": "Champion",
    "fly": "Fly destinations (visited)",
}

# HnS MAPSEC -> target Johto MAPSEC (plan §4.5 fold table). Anything absent keeps its own name.
MAPSEC_FOLDS = {
    "MAPSEC_SPROUT_TOWER": "MAPSEC_VIOLET_CITY",
    "MAPSEC_SLOWPOKE_WELL": "MAPSEC_AZALEA_TOWN",
    "MAPSEC_BURNED_TOWER": "MAPSEC_ECRUTEAK_CITY",
    "MAPSEC_TIN_TOWER": "MAPSEC_ECRUTEAK_CITY",
    "MAPSEC_OLIVINE_LIGHTHOUSE": "MAPSEC_OLIVINE_CITY",
    "MAPSEC_SS_AQUA": "MAPSEC_OLIVINE_CITY",
    "MAPSEC_ROCKET_HIDEOUT_HNS": "MAPSEC_MAHOGANY_TOWN",
    "MAPSEC_DRAGONS_DEN": "MAPSEC_BLACKTHORN_CITY",
    "MAPSEC_NATIONAL_PARK": "MAPSEC_ROUTE_35",
    "MAPSEC_SAFARI_ZONE_GATE": "MAPSEC_ROUTE_48",
    "MAPSEC_EMBEDDED_TOWER": "MAPSEC_ROUTE_47",
    "MAPSEC_CLIFF_CAVE": "MAPSEC_ROUTE_47",
    "MAPSEC_TOHJO_FALLS": "MAPSEC_ROUTE_27",
    "MAPSEC_INDIGO_PLATEAU": "MAPSEC_JOHTO_LEAGUE",
    "MAPSEC_VICTORY_ROAD_HNS": "MAPSEC_VICTORY_ROAD_JOHTO",
}
MAPSEC_CUT = {"MAPSEC_TRAINER_HILL": "F20: Trainer Hill dropped"}

# Item tokens that the Johto set references and the target lacks (manifest missing.ITEM_).
ITEM_ACTIONS = {
    "ITEM_AZURE_FLUTE": ("delete line", "F31: Azure Flute dropped"),
    "ITEM_EXP_SHARE_SMALL": ("delete line", "F17: HnS Exp. Share option; target has its own rules"),
    "ITEM_HM_WHIRLPOOL": ("delete line", "F32: no HM; Whirlpool tiles check 7 Johto badges (D21)"),
    "ITEM_TM90": ("replace branch", "HnS numbered TM; Stage 12a per-site mapping"),
    "ITEM_TM_SWAGGER": ("replace branch", "Tutor move, cannot be a TM; Stage 12a per-site mapping"),
    "ITEM_TM_NATURAL_GIFT": ("replace branch", "Universal move, cannot be a TM; Stage 12a per-site mapping"),
}
KEY_ITEMS = {"ITEM_CLEAR_BELL", "ITEM_GS_BALL", "ITEM_MYSTERY_EGG", "ITEM_PASS", "ITEM_RAINBOW_WING",
             "ITEM_RED_SCALE", "ITEM_SECRET_POTION", "ITEM_SILVER_WING", "ITEM_SQUIRT_BOTTLE", "ITEM_TIDAL_BELL"}
TM_MOVE_ITEMS = {"ITEM_TM_AVALANCHE", "ITEM_TM_EMBARGO", "ITEM_TM_FALSE_SWIPE", "ITEM_TM_PAYBACK",
                 "ITEM_TM_PLUCK", "ITEM_TM_ROOST"}

# Cut multichoice lists (F-decisions); everything else is kept and renamed.
MULTI_CUT = {"MULTI_BATTLE_MODE_HNS": "F17: HnS battle-mode option", "MULTI_LINK_SERVICES_HNS": "F5: link menu is the target's",
             "MULTI_MOM_MENU": "F12: Mom's Savings dropped"}

# External map references (manifest references.external): action and why.
EXTERNAL_MAPS = {
    "MAP_SAFFRON_CITY_TRAIN_STATION_HNS": ("rename", "MAP_SAFFRON_CITY_TRAIN_STATION", "D1(d): Kanto-side station map (KANTO TOUCH approved)"),
    "MAP_ROUTE22_HNS": ("delete line", None, "D1: Route 22 link removed; the Magnet Train is the link"),
    "MAP_TEST_MAP1_HNS": ("delete line", None, "Test map warp"),
}

WRITE_RE = cfv.WRITE_RE


def parse_int(text):
    text = text.strip()
    return None if text in ("", "—") else int(text, 16)


def read_table(path):
    rows = []
    with open(path, encoding="utf-8") as f:
        for line in f:
            if not re.match(r"^\| (FLAG|VAR)_", line):
                continue
            cells = [c.strip() for c in line.strip().strip("|").split("|")]
            if len(cells) < 10:
                sys.exit(f"error: short table row: {line.strip()[:80]}")
            name, hns, tgt, status, bucket, final, preview, _uses, _crefs, note = cells[:10]
            if bucket not in VALID_BUCKETS:
                sys.exit(f"error: {name}: unknown bucket '{bucket}'")
            r = cfv.Row(name)
            r.hns = parse_int(hns)
            r.target = parse_int(tgt)
            r.status = status
            r.bucket = bucket
            r.final_name = final
            r.preview = parse_int(preview)
            r.note = note
            r.c_uses = {c.strip() for c in _crefs.split(",") if c.strip()}
            rows.append(r)
    return rows


# ---------------------------------------------------------------------------
# Headers


def write_header(path, guard, intro, groups, base_name, base_value):
    width = max(len(name) for _, members in groups for name, _ in members) + 1
    lines = [f"#ifndef {guard}", f"#define {guard}", ""]
    lines += [f"// {t}" for t in intro]
    for title, members in groups:
        first, last = members[0][1], members[-1][1]
        lines += ["", f"// {title} (0x{first:X}-0x{last:X})"]
        for name, value in members:
            lines.append(f"#define {name.ljust(width)}({base_name} + 0x{value - base_value:03X})")
    lines += ["", f"#endif // {guard}", ""]
    with open(os.path.join(ROOT, path), "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(lines))


def johto_finals(rows):
    """Final name -> (block, id) for every Johto final name."""
    out = {}
    for r in rows:
        if r.bucket.startswith("Johto"):
            prev = out.get(r.final_name)
            if prev and prev[1] != r.final_id:
                sys.exit(f"error: {r.final_name} got two ids")
            out[r.final_name] = (r.block, r.final_id)
    return out


def existing_names():
    """Flag/var names already defined by the Hoenn and Kanto headers (Johto headers excluded)."""
    names = set()
    for path in ("flags.h", "vars.h", "flags_kanto.h", "vars_kanto.h"):
        names |= set(re.findall(r"^#define\s+((?:FLAG|VAR)_\w+)", read(os.path.join(ROOT, "include/constants", path)), re.M))
    return names


def write_headers(finals):
    by_block = defaultdict(list)
    for name, (block, value) in finals.items():
        by_block[block].append((name, value))
    for members in by_block.values():
        members.sort(key=lambda m: m[1])
    groups = [(BLOCK_TITLES[b], by_block[b]) for b in cfv.FLAG_BLOCKS if by_block[b]]
    write_header(
        "include/constants/flags_johto.h", "GUARD_CONSTANTS_FLAGS_JOHTO_H",
        ["Johto flags, stored in SaveBlock2 johtoFlags. Hidden items start at JOHTO_FLAGS_START and end at",
         "JOHTO_HIDDEN_ITEMS_END; the other blocks start at 0x2200 and end before JOHTO_TRAINER_FLAGS_START.",
         "Each block starts on a 0x10 boundary. Ids inside a block keep the HnS order, so HnS range",
         "arithmetic (badge loops, hidden-item offsets) stays valid."],
        groups, "JOHTO_FLAGS_START", cfv.JOHTO_FLAGS_START)
    write_header(
        "include/constants/vars_johto.h", "GUARD_CONSTANTS_VARS_JOHTO_H",
        ["Johto vars, stored in SaveBlock2 johtoVars. Ids keep the HnS order."],
        [("Vars", by_block["var"])], "JOHTO_VARS_START", cfv.JOHTO_VARS_START)
    return by_block


def daily_range(finals):
    ids = sorted(v for n, (b, v) in finals.items() if n.startswith("FLAG_DAILY_") or "_DAILY_" in n)
    if not ids:
        return None
    lo, hi = ids[0], ids[-1]
    stray = [n for n, (b, v) in finals.items() if lo <= v <= hi and "DAILY" not in n and b != "var"]
    return lo, hi, stray


# ---------------------------------------------------------------------------
# Rename map


def entry(to, action="rename", note=""):
    e = {"to": to, "action": action}
    if note:
        e["note"] = note
    return e


def flag_var_section(rows, kind):
    out = {}
    for r in sorted((r for r in rows if r.kind == kind), key=lambda r: r.name):
        if r.bucket in ("Johto", "Johto (renamed)"):
            e = entry(r.final_name, "rename" if r.final_name != r.name else "keep", r.note)
            e["id"] = r.final_id
        elif r.bucket == "Shared":
            e = entry(r.final_name or r.name, "keep", r.note)
            e["write_ok"] = r.name in cfv.WRITE_OK or "allowed" in r.note.lower()
            if e["to"] != r.name:
                e["action"] = "rename"
        elif r.bucket == "Config":
            e = entry(None, "replace branch", r.note)
        else:
            e = entry(None, "delete line", r.note)
        out[r.name] = e
    return out


def target_heal_ids():
    data = load_json(os.path.join(ROOT, "src/data/heal_locations.json"))
    return {e["id"] for e in data.get("heal_locations", [])}


def title_symbol(name):
    """`johto_north_east` -> `JohtoNorthEast`."""
    return "".join(p.capitalize() for p in name.split("_"))


def build_rename_map(rows, manifest, hns):
    jset = johto_set(hns)
    coll = manifest["collisions"]
    map_coll = set(coll["maps"]["stripped"])
    layout_coll = set(coll["layouts"]["stripped"])
    rm = {"hns_head": manifest["hns_head"], "flags": flag_var_section(rows, "flag"),
          "vars": flag_var_section(rows, "var")}

    # Maps, folders, layouts.
    maps, folders, layouts = {}, {}, {}
    for _, folder, data in jset:
        mid = data["id"]
        to = strip_hns(mid) + ("_JOHTO" if mid in map_coll else "")
        maps[mid] = entry(to, "rename" if to != mid else "keep")
        new_folder = strip_hns(folder) + ("_Johto" if mid in map_coll else "")
        folders[folder] = entry(new_folder)
        lid = data["layout"]
        lto = strip_hns(lid) + ("_JOHTO" if lid in layout_coll else "")
        layouts[lid] = entry(lto)
    for token in manifest["tokens"]["MAP_"]:
        if token in maps:
            continue
        if token in EXTERNAL_MAPS:
            act, to, note = EXTERNAL_MAPS[token]
            maps[token] = entry(to, act, note)
        else:
            maps[token] = entry(None, "delete line", "Warp/script target outside the kept Johto set (HnS Kanto, Alola, Sinjoh, S.S. Aqua, Trainer Hill or Hoenn copy)")
    rm["maps"], rm["map_folders"], rm["layouts"] = maps, folders, layouts
    rm["script_labels"] = {
        lab: entry(None, "rename", "folder prefix replaced per map_folders (colliding Johto League / Rocket Hideout label)")
        for lab in coll["script_labels"]}
    rm["label_prefixes"] = {f: {"to": v["to"]} for f, v in folders.items() if v["to"] != strip_hns(f)}

    # Tilesets: primaries lose the suffix; secondaries take `_Johto`.
    tilesets = {}
    for sym, info in manifest["tilesets"].items():
        stem = os.path.basename(info["dir"])
        base = re.sub(r"_hns$", "", stem)
        if info["secondary"]:
            to_sym = strip_hns(sym) + "_Johto"
            to_dir = f"data/tilesets/secondary/{base}_johto"
        else:
            sym_core = strip_hns(sym).replace("gTileset_", "")
            to_sym = "gTileset_" + sym_core.replace("_", "")
            to_dir = f"data/tilesets/primary/{base if base.startswith('johto_') else 'johto_' + base}"
        tilesets[sym] = {"to": to_sym, "action": "rename", "dir": to_dir, "from_dir": info["dir"]}
    rm["tilesets"] = tilesets

    # Trainers, overworld graphics, pics, classes.
    rm["trainers"] = {t: entry(re.sub(r"_HNS$", "_JOHTO", t)) for t in sorted(manifest["tokens"]["TRAINER_"])}
    gfx = {}
    for t in sorted(manifest["tokens"]["OBJ_EVENT_GFX_"]):
        gfx[t] = entry(re.sub(r"_HNS$", "_JOHTO", t), "rename" if t.endswith("_HNS") else "keep")
    rm["obj_event_gfx"] = gfx
    pics = {}
    for name in sorted(manifest["trainers"]["pics"]):
        const = "TRAINER_PIC_" + re.sub(r"\s+", "_", name).upper()
        if const.endswith("_HNS"):
            pics[name] = entry(const[:-4] + "_JOHTO", "rename", "Stage 10 may map to an existing pic")
        else:
            pics[name] = entry(const, "keep")
    rm["trainer_pics"] = pics
    classes = {}
    for name in sorted(manifest["trainers"]["classes"]):
        const = "TRAINER_CLASS_" + re.sub(r"\s+", "_", name).upper()
        classes[name] = entry(re.sub(r"_HNS$", "_JOHTO", const), "rename", "Stage 10 may map to an existing class")
    rm["trainer_classes"] = classes

    # Songs, multichoices, items.
    missing_mus = set(manifest["missing"]["MUS_"])
    rm["songs"] = {s: entry(s, "keep", "F3: added by Stage 11" if s in missing_mus else "")
                   for s in sorted(manifest["tokens"]["MUS_"])}
    missing_multi = set(manifest["missing"]["MULTI_"])
    multi = {}
    for m in sorted(manifest["tokens"]["MULTI_"]):
        if m in MULTI_CUT:
            multi[m] = entry(None, "delete line", MULTI_CUT[m])
        elif m in missing_multi:
            multi[m] = entry(re.sub(r"_HNS$", "_JOHTO", m), "rename" if m.endswith("_HNS") else "keep")
        else:
            multi[m] = entry(m, "keep")
    rm["multichoices"] = multi
    items = {}
    for it in sorted(manifest["missing"]["ITEM_"]):
        if it in ITEM_ACTIONS:
            act, note = ITEM_ACTIONS[it]
            items[it] = entry(None, act, note)
        elif it in KEY_ITEMS:
            items[it] = entry(it, "keep", "F31 key item (Stage 12)")
        elif it in TM_MOVE_ITEMS:
            items[it] = entry(None, "replace branch", "HnS-only TM move; Stage 12a writes the per-site ITEM_TM<n>")
    rm["items"] = items
    rm["tm_sites"] = {"status": "pending Stage 12a", "count": len(manifest["tm_sites"]["sites"])}

    # LOCALIDs, MAPSECs, heal locations.
    rm["localids"] = {k: {"to": f"{k}_JOHTO", "action": "rename", "maps": v["johto"]}
                      for k, v in coll["localids"].items()}
    mapsec = {}
    for s in sorted(manifest["tokens"]["MAPSEC_"]):
        if s in MAPSEC_CUT:
            mapsec[s] = entry(None, "delete line", MAPSEC_CUT[s])
        else:
            to = MAPSEC_FOLDS.get(s, s)
            mapsec[s] = entry(to, "rename" if to != s else "keep", "folded (§4.5)" if s in MAPSEC_FOLDS else "")
    rm["mapsec"] = mapsec
    tgt_heal = target_heal_ids()
    heal = {}
    for h in sorted(manifest["tokens"]["HEAL_LOCATION_"]):
        stripped = strip_hns(h)
        heal[h] = entry(stripped + ("_JOHTO" if stripped in tgt_heal else ""))
    rm["heal_locations"] = heal
    return rm


# ---------------------------------------------------------------------------
# Johto-only audits


def hns_nonvar_defines(hns):
    """Non-VAR defines in HnS vars.h that Johto scripts use (plan Stage 6 step 4)."""
    text = read(os.path.join(hns, "include/constants/vars.h"))
    names = {m for m in re.findall(r"#define\s+([A-Z][A-Z0-9_]+)\s", text) if not m.startswith("VAR_")}
    names -= {"VARS_START", "VARS_END", "VARS_COUNT"}
    used = defaultdict(int)
    for _, folder, data in johto_set(hns):
        blob = read(os.path.join(hns, "data/maps", folder, "scripts.inc")) + json.dumps(data)
        for n in names:
            if re.search(r"\b" + n + r"\b", blob):
                used[n] += 1
    return dict(used)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    add_hns_arg(ap)
    ap.add_argument("--table", required=True, help="Reviewed classification Markdown")
    ap.add_argument("--write", action="store_true", help="Write the headers and rename_map.json")
    ap.add_argument("--manifest", default=os.path.join(OUT_DIR, "manifest.json"))
    args = ap.parse_args()

    rows = read_table(args.table)
    layout, flag_end, var_end = cfv.assign_ids(rows)
    finals = johto_finals(rows)
    clash = sorted(set(finals) & existing_names())
    if clash:
        sys.exit(f"error: Johto final names already defined by Hoenn/Kanto: {clash}")

    moved = sorted({r.name for r in rows if r.bucket.startswith("Johto") and r.final_id != r.preview})
    counts = defaultdict(int)
    for r in rows:
        counts[r.bucket] += 1
    print(f"{len(rows)} rows; " + ", ".join(f"{b} {n}" for b, n in sorted(counts.items())))
    print(f"{len(finals)} Johto final names; flag ids end 0x{flag_end:X}, var ids end 0x{var_end:X}")
    print(f"{len(moved)} ids differ from the Stage 5 preview" + (f": {moved[:10]}" if moved else ""))
    for kind, block, start, n in layout:
        print(f"  {kind:4} {block:12} 0x{start:X} x{n}")

    hidden = [x for x in layout if x[1] == "hidden_item"]
    if flag_end > cfv.JOHTO_TRAINER_FLAGS_START or var_end > cfv.JOHTO_VARS_END + 1 or \
            (hidden and hidden[0][2] + hidden[0][3] - 1 > cfv.JOHTO_HIDDEN_ITEMS_END):
        sys.exit("error: Johto ids exceed their range")

    daily = daily_range(finals)
    if daily:
        lo, hi, stray = daily
        print(f"daily flags 0x{lo:X}-0x{hi:X}" + (f"; non-daily flags inside: {stray}" if stray else ""))

    nonvar = hns_nonvar_defines(args.hns)
    print(f"non-VAR defines in HnS vars.h used by Johto: {nonvar or 'none'}")

    with open(args.manifest, encoding="utf-8") as f:
        manifest = json.load(f)
    rm = build_rename_map(rows, manifest, args.hns)
    summary = {k: len(v) for k, v in rm.items() if isinstance(v, dict)}
    print("rename_map sections: " + ", ".join(f"{k} {n}" for k, n in summary.items()))

    if args.write:
        write_headers(finals)
        os.makedirs(OUT_DIR, exist_ok=True)
        with open(os.path.join(OUT_DIR, "rename_map.json"), "w", encoding="utf-8", newline="\n") as f:
            json.dump(rm, f, indent=1, sort_keys=True, ensure_ascii=False)
            f.write("\n")
        print("wrote include/constants/flags_johto.h, vars_johto.h, tools/gs_convert/out/rename_map.json")
    else:
        print("dry run (no files written)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
