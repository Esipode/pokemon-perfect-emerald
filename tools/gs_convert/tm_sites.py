#!/usr/bin/env python3
"""List every TM/HM site in the HnS Johto set (plan §0.9).

A site is one place that hands out a TM/HM: an item ball, a hidden item, a gift (gym leader,
NPC or HM giver), a mart slot or a Game Corner prize. Other mentions of the item (checkitem,
removeitem, bufferitemname, ...) are listed as references. Numbered HnS ids (`ITEM_TM75`) are
resolved through HnS's own FOREACH_TM. Run from anywhere:

    python3 tools/gs_convert/tm_sites.py [--hns <path>] [--tsv <out.tsv>]
"""

import argparse
import json
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from common import add_hns_arg, johto_set, map_scripts, tm_lists  # noqa: E402

ITEM_RE = re.compile(r"\bITEM_((?:TM|HM)\d+|(?:TM|HM)_[A-Z0-9_]+)\b")
LABEL_RE = re.compile(r"^(\w+):{1,2}")
# Mart whose TMs exist only for HnS's finite-TM option (FLAG_FINITE_TMS); not imported.
DROPPED_MARTS = {"SafariZoneGate_hns"}
PRIZE_MAPS = {"GoldenrodCity_GameCorner_hns"}
# Gym-leader gift maps not named `*_Gym_*` (Chuck; Clair gives hers in Dragon's Den).
GYM_GIFT_MAPS = {"CianwoodGym_hns", "DragonsDen_Cavern_hns"}


def resolve(token, hns_tms, hns_hms):
    """HnS item token -> (kind, move), e.g. TM75 -> ("TM", "SWORDS_DANCE")."""
    m = re.match(r"(TM|HM)(\d+)$", token)
    if m:
        moves = hns_tms if m.group(1) == "TM" else hns_hms
        return m.group(1), moves[int(m.group(2)) - 1]
    return token[:2], token[3:]


def scan(hns):
    hns_tms, hns_hms = tm_lists(hns, hns_branch=True)
    sites, refs = [], []
    seen_gifts, seen_prizes = set(), set()
    for _, name, data in johto_set(hns):
        for bg in data.get("bg_events", []):
            m = ITEM_RE.search(json.dumps(bg))
            if m and bg.get("type") == "hidden_item":
                kind, move = resolve(m.group(1), hns_tms, hns_hms)
                sites.append(dict(map=name, site="hidden", item=m.group(1), kind=kind, move=move,
                                  line="map.json", label=bg.get("flag", "")))
        label = ""
        for lineno, line in enumerate(map_scripts(hns, name).splitlines(), 1):
            lm = LABEL_RE.match(line)
            if lm:
                label = lm.group(1)
            code = line.split("@", 1)[0].strip()
            m = ITEM_RE.search(code)
            if not m:
                continue
            token = m.group(1)
            kind, move = resolve(token, hns_tms, hns_hms)
            cmd = code.split()[0]
            row = dict(map=name, item=token, kind=kind, move=move, line=lineno, label=label, code=code)
            if cmd == "finditem":
                row["site"] = "ball"
            elif cmd == ".2byte":
                row["site"] = "dropped_mart" if name in DROPPED_MARTS else "mart"
            elif cmd == "additem" and name in PRIZE_MAPS:
                if (name, token) in seen_prizes:
                    refs.append(row)
                    continue
                seen_prizes.add((name, token))
                row["site"] = "prize"
            elif cmd == "giveitem":
                if (name, token) in seen_gifts:
                    refs.append(row)
                    continue
                seen_gifts.add((name, token))
                if "_Gym_" in name or name in GYM_GIFT_MAPS:
                    row["site"] = "gym_gift"
                elif kind == "HM":
                    row["site"] = "hm_gift"
                else:
                    row["site"] = "npc_gift"
            else:
                refs.append(row)
                continue
            sites.append(row)
    return sites, refs


ONE_TIME = ("ball", "hidden", "gym_gift", "npc_gift", "hm_gift")
SHOP = ("mart", "prize")


def summary(sites):
    counts = {}
    for s in sites:
        counts[s["site"]] = counts.get(s["site"], 0) + 1
    return dict(counts=counts,
                one_time=sum(counts.get(k, 0) for k in ONE_TIME),
                shop_slots=sum(counts.get(k, 0) for k in SHOP),
                dropped=counts.get("dropped_mart", 0))


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    add_hns_arg(parser)
    parser.add_argument("--tsv", help="write sites and references as TSV")
    args = parser.parse_args()
    sites, refs = scan(args.hns)
    for s in sites:
        print("%-12s %-40s %-24s %s" % (s["site"], s["map"], s["item"], s["move"]))
    print(json.dumps(summary(sites)))
    if args.tsv:
        with open(args.tsv, "w") as f:
            for r in sites + [dict(r, site="ref") for r in refs]:
                f.write("\t".join(str(r.get(k, "")) for k in ("site", "map", "line", "label", "item", "move")) + "\n")


if __name__ == "__main__":
    main()
