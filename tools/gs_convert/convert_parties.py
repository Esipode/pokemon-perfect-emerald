#!/usr/bin/env python3
"""Stage 10: build the Johto trainer party file and id header from the HnS party file.

Keeps only the trainers in rename_map.json["trainers"] (HnS party order), renames ids,
classes and pics, and rewrites each `Level:` to a spread above the party's weakest member
(Kanto rule, see tools/frlg_convert/convert_parties.py).

Writes src/data/trainers_johto.party, include/constants/opponents_johto.h and
out/trainer_table.json. Refuses to overwrite an existing party file.

Usage: convert_parties.py [--hns PATH] [--dry-run]
"""
import argparse
import json
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import common

# HnS class (as written in the party file) -> existing target class name (party-file spelling).
REUSE_CLASS = {
    "Beauty Hns": "Beauty", "Bird Keeper Hns": "Bird Keeper", "Black Belt Hns": "Black Belt",
    "Bug Catcher Hns": "Bug Catcher", "Camper Hns": "Camper", "Collector Hns": "Collector",
    "Cooltrainer Hns": "Cooltrainer", "Dragon Tamer Hns": "Dragon Tamer", "Expert Hns": "Expert",
    "Fisherman Hns": "Fisherman", "Gentleman Hns": "Gentleman", "Guitarist Hns": "Guitarist",
    "Hex Maniac Hns": "Hex Maniac", "Hiker Hns": "Hiker", "Lass Hns": "Lass",
    "Parasol Lady Hns": "Parasol Lady", "Picnicker Hns": "Picnicker", "Pokefan Hns": "Pokefan",
    "Pokemaniac Hns": "Pokemaniac", "Psychic M Hns": "Psychic", "Sailor Hns": "Sailor",
    "School Kid Hns": "School Kid", "Swimmer F Hns": "Swimmer F", "Swimmer M Hns": "Swimmer M",
    "Twins Hns": "Twins", "Young Couple Hns": "Young Couple", "Youngster Hns": "Youngster",
}

LEVEL_RE = re.compile(r"^Level: (\d+)$", re.M)


def johto_name(text):
    return re.sub(r" Hns$", " Johto", text)


def spread(body):
    levels = [int(v) for v in LEVEL_RE.findall(body)]
    if not levels:
        return body, [], []
    low = min(levels)
    spreads = iter(v - low for v in levels)
    return LEVEL_RE.sub(lambda _: "Level: %d" % next(spreads), body), levels, [v - low for v in levels]


def main():
    ap = argparse.ArgumentParser()
    common.add_hns_arg(ap)
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()

    rename = common.load_json(os.path.join(common.OUT_DIR, "rename_map.json"))["trainers"]
    src = common.read(os.path.join(args.hns, "src/data/trainers_hns.party"))
    parts = re.split(r"^=== (\S+) ===$", src, flags=re.M)

    out = ["/* ==================== Johto Trainers ==================== */\n"]
    ids, table = [], {}
    for i in range(1, len(parts), 2):
        hns_id, body = parts[i], parts[i + 1]
        if hns_id not in rename:
            continue
        new_id = rename[hns_id]["to"]
        cls = re.search(r"^Class: (.+)$", body, re.M).group(1)
        pic = re.search(r"^Pic: (.+)$", body, re.M).group(1)
        new_cls = REUSE_CLASS.get(cls) or johto_name(cls)
        new_pic = johto_name(pic)
        body = re.sub(r"^Class: .+$", "Class: " + new_cls, body, flags=re.M)
        body = re.sub(r"^Pic: .+$", "Pic: " + new_pic, body, flags=re.M)
        body, old, spreads = spread(body)
        out.append("\n=== %s ===%s" % (new_id, body.rstrip("\n") + "\n"))
        ids.append(new_id)
        table[new_id] = {"hns": hns_id, "class": new_cls, "pic": new_pic, "levels": old, "spreads": spreads}

    missing = set(t["to"] for t in rename.values()) - set(ids)
    if missing:
        sys.exit("trainers without a party: %s" % sorted(missing))

    header = ["#ifndef GUARD_CONSTANTS_OPPONENTS_JOHTO_H",
              "#define GUARD_CONSTANTS_OPPONENTS_JOHTO_H", "",
              "// Johto trainer ids start after the Kanto block; JOHTO_TRAINERS_START is defined in opponents.h.",
              "// Offset 0 is unused.", ""]
    for n, tid in enumerate(ids, 1):
        header.append("#define %-44s (JOHTO_TRAINERS_START + %d)" % (tid, n))
    header += ["",
               "// Each Johto trainer's defeat flag is JOHTO_TRAINER_FLAGS_START + (id - JOHTO_TRAINERS_START),",
               "// so MAX_JOHTO_TRAINERS_COUNT is bounded by the Johto trainer flag range (constants/flags.h).", "",
               "#define TRAINERS_COUNT_JOHTO                     %d" % (len(ids) + 1),
               "#define MAX_JOHTO_TRAINERS_COUNT                 512", "",
               "#endif  // GUARD_CONSTANTS_OPPONENTS_JOHTO_H", ""]

    print("%d trainers, %d classes new" % (len(ids), len({t["class"] for t in table.values() if t["class"].endswith(" Johto")})))
    if args.dry_run:
        return
    party_path = os.path.join(common.ROOT, "src/data/trainers_johto.party")
    if os.path.exists(party_path):
        sys.exit("%s exists" % party_path)
    with open(party_path, "w") as f:
        f.write("".join(out))
    with open(os.path.join(common.ROOT, "include/constants/opponents_johto.h"), "w") as f:
        f.write("\n".join(header))
    with open(os.path.join(common.OUT_DIR, "trainer_table.json"), "w") as f:
        json.dump(table, f, indent=1)


if __name__ == "__main__":
    main()
