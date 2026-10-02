#!/usr/bin/env python3
"""Replace starter Pokemon (all nine generations, every stage) in wild_encounters.json.

Each starter species in a map becomes one non-starter species with the identical type set and a
similar base stat total, picking the least-used generation first (weighted by slot rate). Same
exclusions as convert_wild.py (no UB/Paradox/legendary/sub-legendary/mythical/totem). The Hoenn
Safari Zone tables (MAP_SAFARI_ZONE_<direction>, no _Johto_ suffix) are left alone.

Run from the repository root:  python3 tools/gs_convert/replace_starters.py [--dry-run]
"""
import argparse
import json
import os
import random
import sys
from collections import Counter, OrderedDict

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import convert_wild as cw  # noqa: E402

STARTERS = set("SPECIES_" + s for s in """BULBASAUR IVYSAUR VENUSAUR CHARMANDER CHARMELEON CHARIZARD SQUIRTLE WARTORTLE BLASTOISE
CHIKORITA BAYLEEF MEGANIUM CYNDAQUIL QUILAVA TYPHLOSION TOTODILE CROCONAW FERALIGATR TREECKO GROVYLE SCEPTILE
TORCHIC COMBUSKEN BLAZIKEN MUDKIP MARSHTOMP SWAMPERT TURTWIG GROTLE TORTERRA CHIMCHAR MONFERNO INFERNAPE
PIPLUP PRINPLUP EMPOLEON SNIVY SERVINE SERPERIOR TEPIG PIGNITE EMBOAR OSHAWOTT DEWOTT SAMUROTT CHESPIN QUILLADIN
CHESNAUGHT FENNEKIN BRAIXEN DELPHOX FROAKIE FROGADIER GRENINJA ROWLET DARTRIX DECIDUEYE LITTEN TORRACAT INCINEROAR
POPPLIO BRIONNE PRIMARINA GROOKEY THWACKEY RILLABOOM SCORBUNNY RABOOT CINDERACE SOBBLE DRIZZILE INTELEON
SPRIGATITO FLORAGATO MEOWSCARADA FUECOCO CROCALOR SKELEDIRGE QUAXLY QUAXWELL QUAQUAVAL""".split())
HOENN_SAFARI = {"MAP_SAFARI_ZONE_SOUTH", "MAP_SAFARI_ZONE_SOUTHWEST", "MAP_SAFARI_ZONE_NORTH",
                "MAP_SAFARI_ZONE_NORTHWEST", "MAP_SAFARI_ZONE_SOUTHEAST", "MAP_SAFARI_ZONE_NORTHEAST"}


def is_hoenn_safari(e):
    return e.get("map") in HOENN_SAFARI and "_Johto_" not in e["base_label"] and "_Kanto_" not in e["base_label"]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()

    data = cw.load_json(cw.JSON_PATH)
    ids = cw.load_species_ids()
    info = cw.load_species_info()
    rng = random.Random(15)
    pool = [n for n, d in info.items() if d["canonical"] and not d["banned"] and d["types"] and n in ids
            and (ids[n] <= 905 or ids[n] >= cw.GEN9_FIRST_ID) and n not in STARTERS]
    gen_use, sp_use = Counter(), Counter()

    # Group tables per map so one starter maps to one replacement across all its times.
    tables = OrderedDict()
    for g in data["wild_encounter_groups"]:
        for e in g["encounters"]:
            if is_hoenn_safari(e):
                continue
            tables.setdefault((g["label"], e.get("map", e["base_label"])), []).append(e)

    replaced = Counter()
    for (_, _), es in tables.items():
        weight, present = Counter(), set()
        for e in es:
            for f in cw.FIELDS:
                for i, m in enumerate(e.get(f, {"mons": []})["mons"]):
                    present.add(m["species"])
                    if m["species"] in STARTERS:
                        weight[m["species"]] += cw.LAND_RATES[i] if f == "land_mons" and i < 12 else 4
        mapping = {}
        for s in sorted(weight, key=lambda x: -weight[x]):
            o = info[s]
            for tol in (30, 50, 80, 120, 200, 400):
                cand = [n for n in pool if n not in present and info[n]["types"] == o["types"]
                        and abs(info[n]["bst"] - o["bst"]) <= tol]
                if len(cand) >= 3:
                    break
            else:
                cand = [n for n in pool if n not in present and info[n]["types"] & o["types"]
                        and abs(info[n]["bst"] - o["bst"]) <= 120]
            best = min(cand, key=lambda n: (gen_use[cw.gen_of(ids[n])], sp_use[n], rng.random()))
            gen_use[cw.gen_of(ids[best])] += weight[s]
            sp_use[best] += 1
            mapping[s] = best
            present.add(best)
        for e in es:
            for f in cw.FIELDS:
                for m in e.get(f, {"mons": []})["mons"]:
                    if m["species"] in mapping:
                        m["species"] = mapping[m["species"]]
                        replaced[e["base_label"]] += 1

    print(f"tables touched: {len(replaced)}  slots replaced: {sum(replaced.values())}")
    print("replacement gen use:", dict(sorted(gen_use.items())))
    left = [e["base_label"] for g in data["wild_encounter_groups"] for e in g["encounters"]
            if not is_hoenn_safari(e) for f in cw.FIELDS for m in e.get(f, {"mons": []})["mons"]
            if m["species"] in STARTERS]
    assert not left, left[:5]
    if not args.dry_run:
        with open(cw.JSON_PATH, "w") as f:
            json.dump(data, f, indent=2)
            f.write("\n")


if __name__ == "__main__":
    main()
