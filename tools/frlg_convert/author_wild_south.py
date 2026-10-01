#!/usr/bin/env python3
"""Author species for the Kanto-south wild tables (Stage 25).

Replaces the species of the converted _Kanto_ tables for Pallet Town through Vermilion City,
Mt. Moon, Rock Tunnel, Diglett's Cave and the Cerulean / Saffron-side routes. Levels, encounter
rates and slot counts are kept; only species change.

Rules (shared with Stages 26-27, see the plan):
  * Kanto does not have to make every species obtainable (Hoenn already does), so the goal is type
    and theme variety per area, with species from any generation.
  * Spread species across all nine generations: every list must span a minimum number of distinct
    generations (MIN_GENS) and every land map at least MIN_MAP_GENS across its four times.
  * No Ultra Beasts, Paradox, legendary, sub-legendary, mythical or totem species.
  * Land: 6 species per time of day, spread over the 12 slots by LAND_PATTERN (the first two are the
    common slots, the last two the rare ones).
  * Water / rock smash / fishing: one list per field, identical for all four times (as in Hoenn).

Run from the repository root:  python3 tools/frlg_convert/author_wild_south.py [--dry-run]
"""
import argparse
import json
import re
import sys
from collections import OrderedDict

JSON_PATH = "src/data/wild_encounters.json"
SPECIES_H = "include/constants/species.h"
SPECIES_INFO_DIR = "src/data/pokemon/species_info/"
TIMES = ("Morning", "Day", "Evening", "Night")
# Land slot rates are 20 20 10 10 10 10 5 5 4 4 1 1; species 0-1 get 30% each, 2-3 15%, 4-5 5%.
LAND_PATTERN = (0, 1, 2, 3, 0, 1, 2, 3, 4, 5, 4, 5)
BANNED_FLAGS = ("isMythical", "isUltraBeast", "isParadox", "isRestrictedLegendary", "isSubLegendary", "isTotem")

# map -> four land lists (Morning, Day, Evening, Night), each 6 species, common first.
LAND = {
    "ROUTE1": (
        "HOPPIP BIDOOF PIDGEY LILLIPUP ROOKIDEE PAWMI",
        "ZIGZAGOON BUNNELBY SKWOVET LECHONK RATTATA SENTRET",
        "YUNGOOS PATRAT NYMBLE WURMPLE FLETCHLING HOOTHOOT",
        "ODDISH NICKIT STUNKY TAROUNTULA SPINARAK PURRLOIN",
    ),
    "ROUTE2": (
        "GRUBBIN SCATTERBUG SUNKERN BURMY CATERPIE SEWADDLE",
        "SMOLIV NIDORAN_M PETILIL LEDYBA CHERUBI FOMANTIS",
        "WURMPLE GOSSIFLEUR PACHIRISU SKIDDO COTTONEE BELLSPROUT",
        "MORELULL SPINARAK SEEDOT VENIPEDE BRAMBLIN VENONAT",
    ),
    "VIRIDIAN_FOREST": (
        "KRICKETOT BLIPBUG NYMBLE SEWADDLE CATERPIE PIKACHU",
        "SCATTERBUG SHROOMISH GRUBBIN WEEDLE COMBEE PIKACHU",
        "VENIPEDE CUTIEFLY SIZZLIPEDE SURSKIT PARAS PIKACHU",
        "TAROUNTULA SPINARAK KRICKETOT VENONAT JOLTIK PIKACHU",
    ),
    "ROUTE22": (
        "LITLEO YAMPER STARLY MASCHIFF MUDBRAY MANKEY",
        "ZIGZAGOON BLITZLE FLETCHLING ROCKRUFF WOOLOO RATTATA",
        "MAREEP SHINX FIDOUGH SKIDDO SPEAROW DEERLING",
        "IMPIDIMP POOCHYENA HOUNDOUR MEOWTH GLAMEOW ZORUA",
    ),
    "ROUTE3": (
        "STUFFUL PANCHAM STARLY SPEAROW ROLYCOLY JIGGLYPUFF",
        "NACLI TIMBURR CRABRAWLER MAKUHITA CRANIDOS SANDSHREW",
        "CUFANT PANCHAM NOSEPASS DWEBBLE JIGGLYPUFF GEODUDE",
        "GREAVARD SKORUPI ZUBAT WOOBAT NOIBAT CLEFAIRY",
    ),
    "ROUTE4": (
        "SENTRET PIKIPEK SQUAWKABILLY PIDOVE SPEAROW SANDSHREW",
        "SILICOBRA SKORUPI HELIOPTILE RATTATA SANDILE EKANS",
        "TOGEDEMARU ORTHWORM BRONZOR KLINK MEOWTH MAGNEMITE",
        "SALANDIT TOEDSCOOL CROAGUNK SCRAGGY ZUBAT EKANS",
    ),
    "MT_MOON_1F": (
        "CARBINK BONSLY ROGGENROLA ZUBAT GEODUDE CLEFAIRY",
        "ROLYCOLY NACLI WOOBAT GEODUDE CRANIDOS ZUBAT",
        "NOSEPASS MINIOR SHIELDON GLIMMET CLEFAIRY ZUBAT",
        "MEDITITE HATENNA ELGYEM CHINGLING ZUBAT CLEFAIRY",
    ),
    "MT_MOON_B1F": (
        "CARBINK ROGGENROLA GLIMMET ZUBAT GEODUDE ONIX",
        "MAWILE MINIOR BRONZOR DWEBBLE GEODUDE PARAS",
        "ROLYCOLY CRANIDOS FERROSEED ZUBAT CLEFAIRY MAGNEMITE",
        "SABLEYE SINISTEA WOOBAT DRIFLOON ZUBAT CLEFAIRY",
    ),
    "MT_MOON_B2F": (
        "NACLI ROGGENROLA SHIELDON ZUBAT GEODUDE CLEFAIRY",
        "ROLYCOLY BONSLY DRILBUR GEODUDE ZUBAT CLEFAIRY",
        "CARBINK NOSEPASS KLINK ZUBAT CLEFAIRY MAGNEMITE",
        "SABLEYE GREAVARD HATENNA ELGYEM ZUBAT CLEFAIRY",
    ),
    "ROUTE24": (
        "SUNKERN FLABEBE BOUNSWEET BUDEW PETILIL PIDGEY",
        "ESPURR GOSSIFLEUR COMBEE COTTONEE BELLSPROUT ABRA",
        "MAREEP DEDENNE SMOLIV PACHIRISU ODDISH ABRA",
        "SPINARAK PHANTUMP DRIFLOON GOTHITA VENONAT ABRA",
    ),
    "ROUTE25": (
        "FOMANTIS SHROOMISH BURMY SEWADDLE PIDGEY ODDISH",
        "SNUBBULL GRUBBIN SCATTERBUG KRICKETOT WEEDLE ABRA",
        "WURMPLE BLIPBUG PETILIL CHERUBI ODDISH ABRA",
        "HOOTHOOT TAROUNTULA JOLTIK STUNKY VENONAT ABRA",
    ),
    "ROUTE5": (
        "SNUBBULL LITLEO PATRAT GLAMEOW PIDGEY MEOWTH",
        "YUNGOOS WOOLOO DEERLING BUNEARY MANKEY ODDISH",
        "HOUNDOUR YAMPER DARUMAKA GROWLITHE SHINX MEOWTH",
        "MURKROW IMPIDIMP ZORUA STUNKY ODDISH MEOWTH",
    ),
    "ROUTE6": (
        "ELEKID LECHONK FLETCHLING BIDOOF PIDGEY MEOWTH",
        "TOGEDEMARU MAREEP BLITZLE SHINX MANKEY MEOWTH",
        "NUMEL LITLEO SIZZLIPEDE DARUMAKA ODDISH GROWLITHE",
        "HOOTHOOT ESPURR NICKIT CROAGUNK VENONAT DROWZEE",
    ),
    "ROUTE7": (
        "SKIDDO ROOKIDEE BUNEARY PIDGEY GROWLITHE VULPIX",
        "ROCKRUFF TORKOAL LITLEO YAMPER MEOWTH PONYTA",
        "FIDOUGH HOUNDOUR PANCHAM BUNEARY VULPIX ODDISH",
        "HOUNDOUR PUMPKABOO LITWICK STUNKY VENONAT MEOWTH",
    ),
    "ROUTE8": (
        "CRABRAWLER GLIGAR PANCHAM RIOLU PIDGEY ABRA",
        "GIRAFARIG WOOLOO MINCCINO GLAMEOW MEOWTH VULPIX",
        "MASCHIFF HOUNDOUR PATRAT SHINX GROWLITHE ABRA",
        "DUSKULL PHANTUMP MISDREAVUS GOTHITA MEOWTH GASTLY",
    ),
    "ROUTE9": (
        "SPOINK MAREEP DEERLING FLETCHLING SPEAROW SANDSHREW",
        "MUDBRAY CUFANT BLITZLE SKORUPI RATTATA NIDORAN_F",
        "PAWMI DEDENNE PHANPY PACHIRISU SPEAROW MEOWTH",
        "SALANDIT POOCHYENA SCRAGGY STUNKY EKANS ZUBAT",
    ),
    "ROUTE10": (
        "YAMPER BLITZLE SHINX VOLTORB SPEAROW MAGNEMITE",
        "TOGEDEMARU MAREEP HELIOPTILE JOLTIK PACHIRISU VOLTORB",
        "TADBULB ELECTRIKE DEDENNE KLINK BRONZOR VOLTORB",
        "TADBULB MINUN TYNAMO ROTOM MAGNEMITE VOLTORB",
    ),
    "ROCK_TUNNEL_1F": (
        "NACLI ROLYCOLY ROGGENROLA ZUBAT GEODUDE MACHOP",
        "CRABRAWLER MAKUHITA CRANIDOS TIMBURR GEODUDE MACHOP",
        "KLAWF NOSEPASS HAWLUCHA DRILBUR ZUBAT CUBONE",
        "GREAVARD SABLEYE RIOLU PAWNIARD ZUBAT MACHOP",
    ),
    "ROCK_TUNNEL_B1F": (
        "CARBINK BONSLY ROGGENROLA ZUBAT GEODUDE MACHOP",
        "SANDYGAST HIPPOPOTAS TIMBURR GEODUDE MACHOP CUBONE",
        "ORTHWORM DRILBUR BRONZOR ZUBAT MACHOP MAGNEMITE",
        "SABLEYE FALINKS CROAGUNK YAMASK ZUBAT MACHOP",
    ),
    "ROUTE11": (
        "PIKIPEK BIDOOF DEERLING SPEAROW SANDSHREW DROWZEE",
        "NACLI TRAPINCH SILICOBRA SANDILE SANDSHREW EKANS",
        "MAREEP SKWOVET LITLEO BUNEARY DROWZEE MEOWTH",
        "HOUNDOUR INKAY SOLOSIS STUNKY DROWZEE EKANS",
    ),
    "DIGLETTS_CAVE_B1F": (
        "WIGLETT TRAPINCH DRILBUR DIGLETT SANDSHREW DUGTRIO",
        "MUDBRAY PHANPY HIPPOPOTAS DRILBUR DIGLETT DUGTRIO",
        "SILICOBRA SANDILE GIBLE DIGLETT SANDSHREW DUGTRIO",
        "SANDYGAST SKORUPI DRILBUR DIGLETT ZUBAT DUGTRIO",
    ),
}

# map -> (water list, fishing list). Water has 5 slots, fishing 10 (old rod 2, good rod 3, super rod 5).
WATER = {
    "PALLET_TOWN": ("TENTACOOL WINGULL FINNEON CHEWTLE WIGLETT",
                    "MAGIKARP KRABBY TENTACOOL SHELLDER MAREANIE STARYU FINIZEN ARROKUDA SKRELP TYMPOLE"),
    "VIRIDIAN_CITY": ("PSYDUCK POLIWAG TYMPOLE BUIZEL WOOPER",
                      "MAGIKARP POLIWAG GOLDEEN PSYDUCK DEWPIDER BUIZEL BARBOACH TYMPOLE CHEWTLE WOOPER"),
    "CERULEAN_CITY": ("PSYDUCK STARYU CHINCHOU BASCULIN CLAUNCHER",
                      "MAGIKARP GOLDEEN HORSEA STARYU CHINCHOU CLAUNCHER FINNEON ARROKUDA ALOMOMOLA VELUZA"),
    "VERMILION_CITY": ("TENTACOOL SHELLDER WINGULL BINACLE PYUKUMUKU",
                       "MAGIKARP KRABBY TENTACOOL HORSEA CLAMPERL MANTYKE FRILLISH SKRELP BRUXISH CETODDLE"),
    "SSANNE_EXTERIOR": ("TENTACOOL SHELLDER WINGULL WIMPOD FINIZEN",
                        "MAGIKARP TENTACOOL KRABBY SHELLDER MANTYKE CORPHISH INKAY ARROKUDA DEWPIDER WIGLETT"),
    "ROUTE4": ("PSYDUCK SLOWPOKE MARILL TYMPOLE DEWPIDER",
               "MAGIKARP GOLDEEN PSYDUCK POLIWAG MAREANIE SLOWPOKE MARILL BARBOACH BUIZEL CHEWTLE"),
    "ROUTE6": ("PSYDUCK POLIWAG KRABBY FINNEON ALOMOMOLA",
               "MAGIKARP KRABBY GOLDEEN PSYDUCK STARYU SHELLDER CORPHISH SKRELP ARROKUDA PYUKUMUKU"),
    "ROUTE10": ("PSYDUCK SLOWPOKE CHINCHOU BUIZEL TYNAMO",
                "MAGIKARP CHINCHOU POLIWAG PSYDUCK WOOPER TYNAMO BUIZEL LANTURN PINCURCHIN DEWPIDER"),
    "ROUTE11": ("TENTACOOL KRABBY WINGULL BINACLE ARROKUDA",
                "MAGIKARP TENTACOOL KRABBY SHELLDER HORSEA STARYU CLAMPERL MANTYKE SKRELP FINIZEN"),
    "ROUTE22": ("POLIWAG PSYDUCK MARILL TYMPOLE CHEWTLE",
                "MAGIKARP POLIWAG GOLDEEN PSYDUCK TYMPOLE MARILL BARBOACH BUIZEL DEWPIDER CHEWTLE"),
    "ROUTE24": ("PSYDUCK POLIWAG SLOWPOKE FINNEON BASCULIN",
                "MAGIKARP GOLDEEN POLIWAG PSYDUCK STARYU HORSEA REMORAID FINNEON CLAUNCHER FINIZEN"),
    "ROUTE25": ("PSYDUCK KRABBY TENTACOOL WINGULL SKRELP",
                "MAGIKARP KRABBY GOLDEEN TENTACOOL STARYU CORSOLA LUVDISC MANTYKE ARROKUDA WIMPOD"),
}
ROCK_SMASH = {
    "ROCK_TUNNEL_B1F": "GEODUDE ROGGENROLA ONIX ROLYCOLY SHUCKLE",
}

# Minimum distinct generations (1-9) per list, and per map across its four land lists.
MIN_GENS = {"land": 4, "water": 3, "fishing": 5, "rock smash": 3}
MIN_MAP_GENS = 6
MIN_GEN_SHARE = 7  # percent of land encounters, weighted by slot rate, per generation across all maps
LAND_WEIGHTS = (30, 30, 15, 15, 5, 5)
GEN_END = (151, 251, 386, 493, 649, 721, 809, 905, 1025)


GEN9_FIRST_ID = 1289  # SPECIES_SPRIGATITO; Gen 9 species sit after the form block in species.h


def gen_of(species_id):
    if species_id >= GEN9_FIRST_ID:
        return 9
    return next(i + 1 for i, end in enumerate(GEN_END) if species_id <= end)


def load_species():
    """Return {name: species id}, resolving aliases such as SPECIES_BURMY = SPECIES_BURMY_PLANT."""
    raw = dict(re.findall(r"^\s*SPECIES_(\w+)\s*=\s*(\w+)\s*,", open(SPECIES_H).read(), re.M))

    def resolve(name):
        v = raw[name]
        return int(v) if v.isdigit() else resolve(v[len("SPECIES_"):])

    out = {}
    for n in raw:
        try:
            out[n] = resolve(n)
        except KeyError:
            pass
    return out


def load_banned():
    import glob
    banned = set()
    for path in glob.glob(SPECIES_INFO_DIR + "gen_*_families.h"):
        cur = None
        for line in open(path):
            m = re.match(r"\s*\[SPECIES_(\w+)\]\s*=", line)
            if m:
                cur = m.group(1)
            elif cur and any(f"{f} = TRUE" in line for f in BANNED_FLAGS):
                banned.add(cur)
    return banned


def parse(spec, count, ctx, valid, banned, kind):
    names = spec.split()
    assert len(names) == count, f"{ctx}: expected {count} species, got {len(names)}"
    assert len(set(names)) == len(names) or count > 6, f"{ctx}: duplicate species in {spec}"
    for n in names:
        assert n in valid, f"{ctx}: unknown species {n}"
        assert n not in banned, f"{ctx}: banned species {n}"
    gens = {gen_of(valid[n]) for n in names}
    assert len(gens) >= MIN_GENS[kind], f"{ctx}: only generations {sorted(gens)}"
    return ["SPECIES_" + n for n in names]


def set_species(field, species_per_slot, ctx):
    assert len(field["mons"]) == len(species_per_slot), f"{ctx}: slot count mismatch"
    for mon, sp in zip(field["mons"], species_per_slot):
        mon["species"] = sp


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()

    valid = load_species()
    banned = load_banned()
    with open(JSON_PATH) as f:
        data = json.load(f, object_pairs_hook=OrderedDict)
    encounters = data["wild_encounter_groups"][0]["encounters"]
    by_label = {e["base_label"]: e for e in encounters}

    def table(map_name, time):
        hits = [e for e in encounters if e["map"] == "MAP_" + map_name and e["base_label"].endswith("_Kanto_" + time)]
        assert len(hits) == 1, f"{map_name} {time}: {len(hits)} tables"
        return hits[0]

    for map_name, lists in LAND.items():
        assert len(lists) == 4
        for time, spec in zip(TIMES, lists):
            ctx = f"{map_name} {time} land"
            sp = parse(spec, 6, ctx, valid, banned, "land")
            set_species(table(map_name, time)["land_mons"], [sp[i] for i in LAND_PATTERN], ctx)

    gen_weight = {g: 0 for g in range(1, 10)}
    for map_name, lists in LAND.items():
        names = {n for spec in lists for n in spec.split()}
        gens = {gen_of(valid[n]) for n in names}
        assert len(gens) >= MIN_MAP_GENS, f"{map_name}: only generations {sorted(gens)} across times"
        for spec in lists:
            for weight, n in zip(LAND_WEIGHTS, spec.split()):
                gen_weight[gen_of(valid[n])] += weight
    total = sum(gen_weight.values())
    share = {g: round(100 * w / total, 1) for g, w in gen_weight.items()}
    print("land encounter share per generation (%):", share)
    assert min(share.values()) >= MIN_GEN_SHARE, f"a generation is under {MIN_GEN_SHARE}% of land encounters"

    for map_name, (water, fishing) in WATER.items():
        for time in TIMES:
            e = table(map_name, time)
            ctx = f"{map_name} {time}"
            set_species(e["water_mons"], parse(water, 5, ctx + " water", valid, banned, "water"), ctx + " water")
            set_species(e["fishing_mons"], parse(fishing, 10, ctx + " fishing", valid, banned, "fishing"), ctx + " fishing")

    for map_name, spec in ROCK_SMASH.items():
        for time in TIMES:
            ctx = f"{map_name} {time} rock smash"
            set_species(table(map_name, time)["rock_smash_mons"], parse(spec, 5, ctx, valid, banned, "rock smash"), ctx)

    maps = set(LAND) | set(WATER) | set(ROCK_SMASH)
    print(f"maps authored: {len(maps)}  tables touched: {len(maps) * 4}")

    if not args.dry_run:
        with open(JSON_PATH, "w") as f:
            json.dump(data, f, indent=2)
            f.write("\n")


if __name__ == "__main__":
    main()
