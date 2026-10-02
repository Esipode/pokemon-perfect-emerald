#!/usr/bin/env python3
"""Stage 14: convert HnS Johto wild tables to Johto time-of-day tables in wild_encounters.json.

Per imported Johto map: HnS Day -> Morning + Day, HnS Night -> Evening + Night (a map with one
table, or only Day, copies it to every missing time). Levels become offsets below the progression
cap, like the Kanto tables: per map and field, lowest HnS level -> LOW_OFFSET, highest -> HIGH_OFFSET,
fixed level -> HIGH_OFFSET. Johto levels are read as offsets by the Stage 15 branch of
GetWildMonLevelRange; until then they read as absolute levels.

Species are re-authored for type/theme variety across all nine generations (as for Kanto):
each distinct HnS species in a map is replaced by one species with a shared type and a similar
base stat total, picking the least-used generation so far. No Ultra Beast, Paradox, legendary,
sub-legendary, mythical or totem replacements. Unown (and Magikarp/Gyarados at Lake of Rage) are kept.

Water/fishing tables with more slots than the field defines (HnS has 10-12) are cut to the slots
the field rates can reach (5 water, 10 fishing).

Run from the repository root:  python3 tools/gs_convert/convert_wild.py [--hns PATH] [--dry-run]
"""
import argparse
import glob
import json
import os
import random
import re
import sys
from collections import Counter, OrderedDict

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from common import ROOT, add_hns_arg, load_json  # noqa: E402

JSON_PATH = os.path.join(ROOT, "src/data/wild_encounters.json")
SPECIES_H = os.path.join(ROOT, "include/constants/species.h")
SPECIES_INFO_GLOB = os.path.join(ROOT, "src/data/pokemon/species_info/gen_*_families.h")
RENAME_MAP = os.path.join(os.path.dirname(os.path.abspath(__file__)), "out/rename_map.json")
LOW_OFFSET = 6
HIGH_OFFSET = 3
FIELDS = ("land_mons", "water_mons", "rock_smash_mons", "fishing_mons")
FIELD_SLOTS = {"land_mons": 12, "water_mons": 5, "rock_smash_mons": 5, "fishing_mons": 10}
TIMES = ("Morning", "Day", "Evening", "Night")
BANNED_FLAGS = ("isMythical", "isUltraBeast", "isParadox", "isRestrictedLegendary", "isSubLegendary", "isTotem")
GEN_END = (151, 251, 386, 493, 649, 721, 809, 905, 1025)
GEN9_FIRST_ID = 1289
MIN_GEN_SHARE = 7  # percent of land encounters per generation across all maps
LAND_RATES = (20, 20, 10, 10, 10, 10, 5, 5, 4, 4, 1, 1)
KEEP_ALL = {"SPECIES_UNOWN"}
KEEP_BY_MAP = {"MAP_LAKE_OF_RAGE": {"SPECIES_MAGIKARP", "SPECIES_GYARADOS"}}
# HnS tables that exist twice for one map, or never ship.
SKIP_LABELS = {"gMtSilver_SnowUnused_hns_Day", "gMtSilver_SnowUnused_hns_Night"}
NIGHT_LABELS = {"gMtSilver_SnowNight_hns_Day"}
LABEL_RE = re.compile(r"^g(?P<name>.+?)_hns(?:_(?P<time>Day|Night))?$")
SEED = 14


def gen_of(species_id):
    if species_id >= GEN9_FIRST_ID:
        return 9
    return next(i + 1 for i, end in enumerate(GEN_END) if species_id <= end)


def load_species_ids():
    raw = dict(re.findall(r"^\s*(SPECIES_\w+)\s*=\s*(\w+)\s*,", open(SPECIES_H).read(), re.M))

    def resolve(name, depth=0):
        v = raw[name]
        return int(v) if v.isdigit() else resolve(v, depth + 1)

    out = {}
    for n in raw:
        try:
            out[n] = resolve(n)
        except (KeyError, RecursionError):
            pass
    return out


def load_species_info():
    """{SPECIES_X: dict(bst, types, banned, canonical)} from the species_info family files."""
    info = {}
    for path in glob.glob(SPECIES_INFO_GLOB):
        cur = None
        for line in open(path):
            m = re.match(r"\s*\[(SPECIES_\w+)\]\s*=", line)
            if m:
                cur = info.setdefault(m.group(1), {"stats": {}, "types": None, "banned": False, "natdex": None})
                continue
            if cur is None:
                continue
            m = re.match(r"\s*\.base(HP|Attack|Defense|Speed|SpAttack|SpDefense)\s*=\s*(\d+)", line)
            if m:
                cur["stats"].setdefault(m.group(1), int(m.group(2)))
            if ".types" in line and "MON_TYPES(" in line and cur["types"] is None:
                found = re.findall(r"TYPE_\w+", line)
                cur["types"] = set(found) if found else None
            m = re.match(r"\s*\.natDexNum\s*=\s*(\w+)", line)
            if m and cur["natdex"] is None:
                cur["natdex"] = m.group(1)
            if any(f"{f} = TRUE" in line for f in BANNED_FLAGS):
                cur["banned"] = True
    for name, d in info.items():
        d["bst"] = sum(d["stats"].values())
        d["canonical"] = d["natdex"] == "NATIONAL_DEX_" + name[len("SPECIES_"):]
    return info


def level_bounds(entries, field):
    levels = [m[k] for e in entries if field in e for m in e[field]["mons"] for k in ("min_level", "max_level")]
    return (min(levels), max(levels)) if levels else None


def to_offset(level, lo, hi):
    if hi == lo:
        return HIGH_OFFSET
    return round(LOW_OFFSET - (level - lo) * (LOW_OFFSET - HIGH_OFFSET) / (hi - lo))


def johto_map_ids():
    """Target map ids of imported Johto maps, keyed by HnS map id."""
    renames = load_json(RENAME_MAP)["maps"]
    region = {}
    for path in glob.glob(os.path.join(ROOT, "data/maps/*/map.json")):
        d = load_json(path)
        region[d["id"]] = d.get("region")
    return {hns: v["to"] for hns, v in renames.items()
            if v.get("action") == "rename" and region.get(v["to"]) == "REGION_JOHTO"}


class Picker:
    def __init__(self, info, ids, rng):
        self.rng = rng
        self.info = info
        self.pool = [n for n, d in info.items()
                     if d["canonical"] and not d["banned"] and d["types"] and n in ids
                     and (ids[n] <= 905 or ids[n] >= GEN9_FIRST_ID)]
        self.gen_use = Counter()
        self.species_use = Counter()
        self.ids = ids

    def pick(self, orig, used, weight):
        o = self.info[orig]
        for tol in (30, 50, 80, 120, 200, 400):
            for need_type in (True, False):
                cand = [n for n in self.pool
                        if n not in used and abs(self.info[n]["bst"] - o["bst"]) <= tol
                        and (not need_type or self.info[n]["types"] & o["types"])]
                if len(cand) >= 6:
                    best = min(cand, key=lambda n: (self.gen_use[gen_of(self.ids[n])], self.species_use[n], self.rng.random()))
                    self.gen_use[gen_of(self.ids[best])] += weight
                    self.species_use[best] += 1
                    return best
        raise RuntimeError(f"no replacement for {orig}")


def main():
    ap = argparse.ArgumentParser()
    add_hns_arg(ap)
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()

    hns = load_json(os.path.join(args.hns, "src/data/wild_encounters.json"))
    hns_entries = hns["wild_encounter_groups"][0]["encounters"]
    target = load_json(JSON_PATH)
    group = target["wild_encounter_groups"][0]
    assert group["label"] == "gWildMonHeaders"
    if any("_Johto_" in e["base_label"] for e in group["encounters"]):
        sys.exit("Johto tables already present.")
    existing = {e["base_label"] for e in group["encounters"]}

    map_ids = johto_map_ids()
    per_map = OrderedDict()  # target map -> {"name", "Day": entry, "Night": entry}
    for e in hns_entries:
        m = LABEL_RE.match(e["base_label"])
        if not m or e["map"] not in map_ids or e["base_label"] in SKIP_LABELS:
            continue
        time = "Night" if (m.group("time") == "Night" or e["base_label"] in NIGHT_LABELS) else "Day"
        info = per_map.setdefault(map_ids[e["map"]], {})
        if e["base_label"] not in NIGHT_LABELS:
            info.setdefault("name", m.group("name"))
        assert time not in info, e["base_label"]
        info[time] = e

    species_ids = load_species_ids()
    sp_info = load_species_info()
    picker = Picker(sp_info, species_ids, random.Random(SEED))

    new_entries = []
    trimmed = []
    land_gen_weight = Counter()
    kept_banned = set()
    for map_id, info in per_map.items():
        tables = {t: info[t] for t in ("Day", "Night") if t in info}
        entries = list(tables.values())
        mapping = {}
        used = set()
        keep = KEEP_ALL | KEEP_BY_MAP.get(map_id, set())
        weight = Counter()
        for e in entries:
            for f in FIELDS:
                for i, mon in enumerate(e.get(f, {"mons": []})["mons"][:FIELD_SLOTS[f]]):
                    weight[mon["species"]] += LAND_RATES[i] if f == "land_mons" else 4
        for e in entries:
            for f in FIELDS:
                for mon in e.get(f, {"mons": []})["mons"][:FIELD_SLOTS[f]]:
                    s = mon["species"]
                    if s in mapping:
                        continue
                    if s in keep or s not in sp_info or not sp_info[s]["types"] or not sp_info[s]["bst"]:
                        mapping[s] = s
                    elif sp_info[s]["banned"]:
                        mapping[s] = s
                        kept_banned.add((map_id, s))
                    else:
                        mapping[s] = picker.pick(s, used, weight[s])
                    used.add(mapping[s])
        bounds = {f: level_bounds(entries, f) for f in FIELDS}
        for time in TIMES:
            wanted = "Night" if time in ("Evening", "Night") else "Day"
            src = tables.get(wanted) or next(iter(tables.values()))
            out = OrderedDict(map=map_id, base_label=f"g{info['name']}_Johto_{time}")
            for f in FIELDS:
                if f not in src:
                    continue
                lo, hi = bounds[f]
                mons = src[f]["mons"]
                if len(mons) > FIELD_SLOTS[f]:
                    trimmed.append((out["base_label"], f, len(mons)))
                    mons = mons[:FIELD_SLOTS[f]]
                field = OrderedDict(encounter_rate=src[f]["encounter_rate"], mons=[])
                for i, m in enumerate(mons):
                    a = to_offset(m["max_level"], lo, hi)
                    b = to_offset(m["min_level"], lo, hi)
                    sp = mapping[m["species"]]
                    field["mons"].append(OrderedDict(min_level=min(a, b), max_level=max(a, b), species=sp))
                    if f == "land_mons":
                        land_gen_weight[gen_of(species_ids[sp])] += LAND_RATES[i]
                out[f] = field
            new_entries.append(out)

    labels = [e["base_label"] for e in new_entries]
    assert len(labels) == len(set(labels)), "duplicate label"
    assert not existing.intersection(labels), "label collision"
    assert not any("FireRed" in l or "LeafGreen" in l for l in labels)

    total = sum(land_gen_weight.values())
    share = {g: round(100 * land_gen_weight[g] / total, 1) for g in range(1, 10)}
    print(f"maps: {len(per_map)}  tables: {len(new_entries)}")
    print("land encounter share per generation (%):", share)
    print(f"trimmed over-long fields: {len(trimmed)}")
    for t in sorted(set(trimmed))[:3]:
        print("  ", t)
    print("banned HnS species kept:", sorted(kept_banned))
    assert min(share.values()) >= MIN_GEN_SHARE, "a generation is under the share floor"

    group["encounters"].extend(new_entries)
    if not args.dry_run:
        with open(JSON_PATH, "w") as f:
            json.dump(target, f, indent=2)
            f.write("\n")


if __name__ == "__main__":
    main()
