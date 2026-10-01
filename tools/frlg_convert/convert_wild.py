#!/usr/bin/env python3
"""Convert FireRed/LeafGreen wild tables in wild_encounters.json to Kanto time-of-day tables.

Each map's FireRed and LeafGreen entries are merged: FireRed fills Morning/Day, LeafGreen fills
Evening/Night. Levels become offsets below the Kanto progression cap (see GetWildMonLevelRange):
per map and per encounter field, the lowest FRLG level maps to LOW_OFFSET, the highest to
HIGH_OFFSET, and levels in between are interpolated. Fixed-level fields use HIGH_OFFSET.

Run from the repository root:  python3 tools/frlg_convert/convert_wild.py [--dry-run]
"""
import argparse
import json
import re
import sys
from collections import OrderedDict

JSON_PATH = "src/data/wild_encounters.json"
LOW_OFFSET = 6
HIGH_OFFSET = 3
FIELDS = ("land_mons", "water_mons", "rock_smash_mons", "fishing_mons")
TIMES = (("Morning", "FireRed"), ("Day", "FireRed"), ("Evening", "LeafGreen"), ("Night", "LeafGreen"))
LABEL_RE = re.compile(r"^s(?P<name>.+?)(?:_(?P<set>\d+))?_(?P<ver>FireRed|LeafGreen)$")


def field_bounds(entries):
    bounds = {}
    for f in FIELDS:
        levels = [m[k] for e in entries if f in e for m in e[f]["mons"] for k in ("min_level", "max_level")]
        if levels:
            bounds[f] = (min(levels), max(levels))
    return bounds


def to_offset(level, lo, hi):
    if hi == lo:
        return HIGH_OFFSET
    return round(LOW_OFFSET - (level - lo) * (LOW_OFFSET - HIGH_OFFSET) / (hi - lo))


def convert_field(field, bounds):
    lo, hi = bounds
    out = OrderedDict(encounter_rate=field["encounter_rate"], mons=[])
    for m in field["mons"]:
        a = to_offset(m["max_level"], lo, hi)
        b = to_offset(m["min_level"], lo, hi)
        out["mons"].append(OrderedDict(min_level=min(a, b), max_level=max(a, b), species=m["species"]))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()

    with open(JSON_PATH) as f:
        data = json.load(f, object_pairs_hook=OrderedDict)
    group = data["wild_encounter_groups"][0]
    assert group["label"] == "gWildMonHeaders"
    encounters = group["encounters"]

    versions = OrderedDict()  # map -> {ver: entry}, set 1 only for Altering Cave
    skipped_sets = []
    first_index = {}
    for i, e in enumerate(encounters):
        m = LABEL_RE.match(e["base_label"])
        if not m:
            continue
        if m.group("set") and m.group("set") != "1":
            skipped_sets.append(e["base_label"])
            continue
        entry = versions.setdefault(e["map"], {"name": m.group("name"), "entries": {}})
        assert m.group("ver") not in entry["entries"], e["base_label"]
        entry["entries"][m.group("ver")] = e
        first_index.setdefault(e["map"], i)

    if not versions:
        sys.exit("No FireRed/LeafGreen entries left; already converted.")

    new_entries = OrderedDict()
    differing = []
    for map_name, info in versions.items():
        entries = list(info["entries"].values())
        bounds = field_bounds(entries)
        by_ver = info["entries"]
        if len(by_ver) == 2 and any(
            json.dumps(by_ver["FireRed"].get(f)) != json.dumps(by_ver["LeafGreen"].get(f)) for f in FIELDS
        ):
            differing.append(map_name)
        out = []
        for time, ver in TIMES:
            src = by_ver.get(ver) or by_ver["FireRed" if ver == "LeafGreen" else "LeafGreen"]
            entry = OrderedDict(map=map_name, base_label=f"g{info['name']}_Kanto_{time}")
            for f in FIELDS:
                if f in src:
                    entry[f] = convert_field(src[f], bounds[f])
            out.append(entry)
        new_entries[map_name] = out

    labels = [e["base_label"] for out in new_entries.values() for e in out]
    assert len(labels) == len(set(labels)), "duplicate label"
    existing = {e["base_label"] for e in encounters}
    assert not existing.intersection(labels), "label collision with existing table"

    result = []
    placed = set()
    for i, e in enumerate(encounters):
        if LABEL_RE.match(e["base_label"]):
            if e["map"] in new_entries and e["map"] not in placed and first_index[e["map"]] == i:
                result.extend(new_entries[e["map"]])
                placed.add(e["map"])
            continue
        result.append(e)
    assert placed == set(new_entries)
    group["encounters"] = result

    print(f"maps converted: {len(new_entries)}  new tables: {len(labels)}")
    print(f"maps where FireRed and LeafGreen differ (time slots split): {len(differing)}")
    for m in differing:
        print("  ", m)
    print(f"dropped Altering Cave sets 2-9: {len(skipped_sets)} tables")

    if not args.dry_run:
        with open(JSON_PATH, "w") as f:
            json.dump(data, f, indent=2)
            f.write("\n")


if __name__ == "__main__":
    main()
