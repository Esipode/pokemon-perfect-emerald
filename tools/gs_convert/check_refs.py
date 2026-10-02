#!/usr/bin/env python3
"""Engine-level reference check for the not-yet-imported HnS Johto scripts (Stage 12).

Scans the scripts of the Johto set (plus the shared contest script) and reports every script
macro, special, multichoice, item and weather constant that the target does not provide. A
symbol is "handled" when the rename tool removes or rewrites it; those lists are explicit below
so a new unresolved symbol cannot hide. Exit status is 1 when anything is unresolved.

    python3 tools/gs_convert/check_refs.py [--hns <path>]
"""

import argparse
import glob
import json
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from common import OUT_DIR, ROOT, add_hns_arg, defined_names, johto_set, map_scripts, read, tm_lists  # noqa: E402

# Script macros the rename tool deletes (the whole instruction line) and why.
MACROS_REMOVED = {
    "helpwindow": "F16 help window dropped",
    "trainerbattle_rematch": "F6 phone rematches dropped",
    "register_matchcall": "F6 phone rematches dropped",
    "baobacheckmon": "D12 Baoba quest cut",
    "givebp": "F20 Battle Frontier / BP shops dropped",
}

# HnS special -> action. alias = target special with the same behaviour, removed = the rename tool
# deletes the call, replaced = the rename tool rewrites the call, deferred = a later stage owns it.
SPECIALS_HANDLED = {
    "GetDaycareCost": ("alias", "GetDaycareCostAndPrepareString"),
    "HasMovesToRelearn": ("alias", "Special_HasMoveToRelearn"),
    "CreateKurtBallShop": ("replaced", "pokemart Johto_Mart_KurtBalls (F13)"),
    "CreateBPDecorShop1": ("removed", "F20"),
    "CreateBPDecorShop2": ("removed", "F20"),
    "CreateBPHoldItemShop": ("removed", "F20"),
    "CreateBPHoldItemShop2": ("removed", "F20"),
    "CreateBPPowerShop": ("removed", "F20"),
    "CreateBPVitaminShop": ("removed", "F20"),
    "InitRoamer": ("removed", "F8"),
    "InitKantoRoamers": ("removed", "F8"),
    "NameRival": ("removed", "F18 / D5"),
    "ShouldTryRematchBattle": ("removed", "F6"),
    "Special_BuenaCheckAnswer": ("removed", "F7"),
    "Special_BuenaPasswordShowMultichoice": ("removed", "F7"),
    "Special_BuenaRollReward": ("removed", "F7"),
    "Special_MomDisableSaving": ("removed", "F12"),
    "Special_MomEnableSaving": ("removed", "F12"),
    "Special_MomEnsureInitialized": ("removed", "F12"),
    "Special_MomGetBalance": ("removed", "F12"),
    "Special_MomIsSavingEnabled": ("removed", "F12"),
    "Special_MomOpenDepositInput": ("removed", "F12"),
    "Special_MomOpenWithdrawInput": ("removed", "F12"),
    "Special_ViewVoltorbFlip": ("removed", "F11"),
}

# callnative targets the rename tool deletes.
NATIVES_REMOVED = {
    "SetTimeBasedEncounters": "F1: the target picks the wild time slot from the clock",
}

# Constants the rename tool rewrites, so the target does not define them under the HnS name.
WEATHER_HANDLED = {"WEATHER_LEAVES": "F23 -> WEATHER_NONE"}
# HnS macros that stay HnS-only text in the scan but are movement/command macros the target has.
IGNORED_MNEMONICS = {"step_end"}

SCRIPT_FILES = ["data/scripts/bug_contest.inc"]


def script_texts(hns):
    for _group, name, _data in johto_set(hns):
        yield name, map_scripts(hns, name)
    for rel in SCRIPT_FILES:
        path = os.path.join(hns, rel)
        if os.path.exists(path):
            yield rel, read(path)


def target_macros():
    names = set()
    for path in glob.glob(os.path.join(ROOT, "asm/macros/**/*.inc"), recursive=True):
        text = read(path)
        names.update(re.findall(r"^\s*\.macro\s+(\w+)", text, re.M))
        names.update(re.findall(r"^\s*create_movement_action\s+(\w+)", text, re.M))
    return names


def target_specials():
    return set(re.findall(r"def_special\s+(\w+)", read(os.path.join(ROOT, "data/specials.inc"))))


def target_native_functions():
    names = set()
    for path in glob.glob(os.path.join(ROOT, "src/*.c")):
        names.update(re.findall(r"^\w[\w\s\*]*?\b(\w+)\(", read(path), re.M))
    return names


def check_maps():
    """Stage 13 map-level checks on the imported Johto folders: warps, connections, layouts and assets."""
    groups = json.load(open(os.path.join(ROOT, "data/maps/map_groups.json")))
    all_maps = {}
    for g in groups["group_order"]:
        for folder in groups[g]:
            path = os.path.join(ROOT, "data/maps", folder, "map.json")
            if os.path.exists(path):
                all_maps[json.load(open(path))["id"]] = (g, folder, json.load(open(path)))
    layouts = {l["id"]: l for l in json.load(open(os.path.join(ROOT, "data/layouts/layouts.json")))["layouts"]}
    problems = []
    johto = [(i, v) for i, v in all_maps.items() if v[0].endswith("_Johto")]
    for mid, (_, folder, data) in johto:
        for n, w in enumerate(data.get("warp_events") or []):
            dest = w["dest_map"]
            if dest == "MAP_DYNAMIC":
                continue
            if dest not in all_maps:
                problems.append("%s warp %d -> missing map %s" % (folder, n, dest))
            elif int(w["dest_warp_id"]) >= len(all_maps[dest][2].get("warp_events") or []):
                problems.append("%s warp %d -> %s warp %s out of range" % (folder, n, dest, w["dest_warp_id"]))
            elif not all_maps[dest][0].endswith("_Johto"):
                problems.append("%s warp %d leaves Johto for %s" % (folder, n, dest))
        for c in data.get("connections") or []:
            if c["map"] not in all_maps:
                problems.append("%s connection -> missing map %s" % (folder, c["map"]))
            elif not all_maps[c["map"]][0].endswith("_Johto"):
                problems.append("%s connection leaves Johto for %s" % (folder, c["map"]))
        lay = layouts.get(data["layout"])
        if not lay:
            problems.append("%s layout %s missing" % (folder, data["layout"]))
            continue
        for key in ("border_filepath", "blockdata_filepath"):
            if not os.path.exists(os.path.join(ROOT, lay[key])):
                problems.append("%s: %s missing" % (folder, lay[key]))
        for key in ("primary_tileset", "secondary_tileset"):
            if not re.search(r"\b%s\b" % lay[key], read(os.path.join(ROOT, "src/data/tilesets/headers.h"))):
                problems.append("%s: tileset %s has no header" % (folder, lay[key]))
    # Warps from outside Johto into a Johto map.
    for mid, (g, folder, data) in all_maps.items():
        if g.endswith("_Johto"):
            continue
        for n, w in enumerate(data.get("warp_events") or []):
            if w["dest_map"] in all_maps and all_maps[w["dest_map"]][0].endswith("_Johto"):
                problems.append("%s warp %d enters Johto (%s)" % (folder, n, w["dest_map"]))
    for p in problems:
        print(p)
    print("maps checked: %d Johto, %d total; problems: %d" % (len(johto), len(all_maps), len(problems)))
    return 1 if problems else 0


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    add_hns_arg(parser)
    parser.add_argument("--maps", action="store_true", help="check the imported Johto maps instead of the HnS scripts")
    args = parser.parse_args()
    if args.maps:
        return check_maps()

    rename = json.load(open(os.path.join(OUT_DIR, "rename_map.json")))
    macros = target_macros()
    specials = target_specials()
    multichoices = defined_names(ROOT, ["include/constants/script_menu.h"], "MULTI_")
    items = defined_names(ROOT, ["include/constants/items.h"], "ITEM_")
    weather = defined_names(ROOT, ["include/constants/weather.h"], "WEATHER_")
    natives = target_native_functions()
    tms, hms = tm_lists(ROOT, hns_branch=False)
    machine_moves = set(tms) | set(hms)

    unresolved = {"macro": {}, "special": {}, "multichoice": {}, "item": {}, "weather": {}, "callnative": {}}
    deferred = {}
    handled = {"macro": set(), "special": set()}

    mnemonic_re = re.compile(r"^\s+([a-z_][a-z0-9_]*)\b")
    special_re = re.compile(r"^\s+(?:specialvar\s+\w+,|special)\s*(\w+)")
    native_re = re.compile(r"^\s+callnative\s+(\w+)")
    multi_re = re.compile(r"^\s+multichoice(?:grid)?\s+.*?\b(MULTI_\w+)")
    token_re = {"item": re.compile(r"\b(ITEM_[A-Z0-9_]+)\b"), "weather": re.compile(r"\b(WEATHER_[A-Z0-9_]+)\b")}

    for name, text in script_texts(args.hns):
        in_movement = False
        for line in text.split("\n"):
            stripped = line.split("@", 1)[0]
            if re.match(r"^\w+::?", stripped):
                in_movement = False
            if not stripped.strip() or stripped.lstrip().startswith((".", "#")):
                continue
            m = mnemonic_re.match(stripped)
            if not m:
                continue
            op = m.group(1)
            if op in IGNORED_MNEMONICS:
                continue
            if op in MACROS_REMOVED:
                handled["macro"].add(op)
            elif op not in macros:
                unresolved["macro"].setdefault(op, []).append(name)
            m = special_re.match(stripped)
            if m:
                sp = m.group(1)
                if sp in specials:
                    pass
                elif sp in SPECIALS_HANDLED:
                    if SPECIALS_HANDLED[sp][0] == "deferred":
                        deferred.setdefault(sp, []).append(name)
                    else:
                        handled["special"].add(sp)
                else:
                    unresolved["special"].setdefault(sp, []).append(name)
            m = native_re.match(stripped)
            if m and m.group(1) not in natives and m.group(1) not in NATIVES_REMOVED:
                unresolved["callnative"].setdefault(m.group(1), []).append(name)
            m = multi_re.match(stripped)
            if m:
                const = m.group(1)
                entry = rename["multichoices"].get(const)
                to = entry["to"] if entry else const
                if const != "MULTI_B_PRESSED" and (entry or {}).get("action") != "delete line" and to not in multichoices:
                    unresolved["multichoice"].setdefault(const, []).append(name)
            for kind in ("item", "weather"):
                for tok in token_re[kind].findall(stripped):
                    if kind == "item":
                        machine = re.match(r"ITEM_(?:TM|HM)_(\w+)$", tok)
                        if machine and machine.group(1) in machine_moves:
                            continue  # TM/HM alias; Stage 12a/13a map each site
                        entry = rename["items"].get(tok)
                        if entry:
                            if entry["action"] != "keep" or entry["to"] in items:
                                continue
                        elif tok in items:
                            continue
                    else:
                        if tok in WEATHER_HANDLED or tok in weather:
                            continue
                    unresolved[kind].setdefault(tok, []).append(name)

    total = 0
    for kind, found in unresolved.items():
        print("%s: %d unresolved" % (kind, len(found)))
        for sym in sorted(found):
            print("  %s  (%s%s)" % (sym, found[sym][0], ", ..." if len(found[sym]) > 1 else ""))
        total += len(found)
    print("handled by the rename tool: %d macros, %d specials" % (len(handled["macro"]), len(handled["special"])))
    for sp, maps in sorted(deferred.items()):
        print("deferred: %s -> %s (%d uses)" % (sp, SPECIALS_HANDLED[sp][1], len(maps)))
    print("TOTAL UNRESOLVED:", total)
    return 1 if total else 0


if __name__ == "__main__":
    sys.exit(main())
