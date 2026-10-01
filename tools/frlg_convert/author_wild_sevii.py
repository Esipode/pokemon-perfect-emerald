#!/usr/bin/env python3
"""Author species for the Sevii Islands wild tables (Stage 27).

Covers One Island - Seven Island (Kindle Road, Treasure Beach, Mt. Ember, Cape Brink, Berry Forest,
Bond Bridge, Icefall Cave, Lost Cave, Pattern Bush, Resort / Water Labyrinth / Meadow / Memorial
Pillar, Outcast Island, Green / Water Path, Ruin Valley, Sevault Canyon, Tanoby Ruins) and the One / Four / Five Island waters. Levels, rates and slot counts are kept; only species
change.

Rules and helpers are shared with author_wild_south.py (Stage 25): type / theme variety per area,
all nine generations, no Ultra Beast / Paradox / legendary / sub-legendary / mythical / totem.
Land: 6 species per time (common first), spread over 12 slots by LAND_PATTERN. Water, rock smash and
fishing: one list per field, identical for all four times.

Left untouched: Tanoby chambers (Unown only; forms picked per chamber), Three Island Port (single
Dunsparce slot) and the Altering Cave, whose tables are copies of Hoenn's (Ultra Beast / Paradox
daily pools, absolute level 65; see GetAlteringCaveDailySpecies).

Run from the repository root:  python3 tools/frlg_convert/author_wild_sevii.py [--dry-run]
"""
import argparse
import json
from collections import OrderedDict

from author_wild_south import (JSON_PATH, LAND_PATTERN, LAND_WEIGHTS, MIN_GEN_SHARE, MIN_MAP_GENS,
                               TIMES, gen_of, load_banned, load_species, parse, set_species)

# list key -> four land lists (Morning, Day, Evening, Night), each 6 species, common first.
LAND_LISTS = {
    "EMBER_EXT": (
        "SLUGMA GROWLITHE CHARCADET TORKOAL ROLYCOLY NUMEL",
        "LITLEO ROCKRUFF SIZZLIPEDE HOUNDOUR GEODUDE PONYTA",
        "CAPSAKID VULPIX SALANDIT SANDYGAST MAGBY HEATMOR",
        "LITWICK DARUMAKA CHARCADET MURKROW CARKOL HOUNDOUR",
    ),
    "EMBER_SUMMIT": (
        "ROCKRUFF TORKOAL CAPSAKID NACLI MACHOP SLUGMA",
        "HEATMOR SIZZLIPEDE ROLYCOLY GROWLITHE ROCKRUFF MAGMAR",
        "SALANDIT CHARCADET NUMEL SKARMORY TYRUNT HOUNDOUR",
        "DARUMAKA LITWICK HOUNDOUR CHARCADET MAGBY CARKOL",
    ),
    "EMBER_RUBY": (
        "GEODUDE ZUBAT SLUGMA DRILBUR NACLI CARKOL",
        "ROGGENROLA SIZZLIPEDE ONIX WOOBAT TORKOAL RHYHORN",
        "SANDYGAST MAGBY NOSEPASS TIMBURR GOLBAT SALANDIT",
        "LITWICK ZUBAT SABLEYE CHARCADET SKORUPI HOUNDOUR",
    ),
    "EMBER_RUBY_DEEP": (
        "ROLYCOLY GRAVELER HEATMOR GOLBAT CAPSAKID CARBINK",
        "BOLDORE SIZZLIPEDE MACHOKE ONIX NUMEL WOOBAT",
        "CARKOL SALANDIT TORKOAL SKARMORY GOLBAT LARVESTA",
        "DUSKULL MAGMAR SABLEYE CHARCADET SKORUPI ZUBAT",
    ),
    "KINDLE": (
        "CRABRAWLER ROCKRUFF WINGULL MACHOP NUMEL SLUGMA",
        "TOGEDEMARU SKIDDO SANDSHREW MAKUHITA HOUNDOUR PIDOVE",
        "SALANDIT GROWLITHE TIMBURR TORKOAL SANDYGAST MANKEY",
        "MURKROW CHARCADET SKORUPI SABLEYE LITWICK ZUBAT",
    ),
    "TREASURE": (
        "PIKIPEK STARLY CRABRAWLER SANDSHREW BOUNSWEET WINGULL",
        "SILICOBRA SANDYGAST PIDOVE DIGLETT CUFANT PINSIR",
        "SANDILE CUBONE TOGEDEMARU HERACROSS LECHONK SKWOVET",
        "HOUNDOUR NICKIT SKORUPI GASTLY MORELULL SPINARAK",
    ),
    "CAPE_BRINK": (
        "FLETCHLING SPEAROW WINGULL ROOKIDEE SKWOVET NATU",
        "PIDOVE STARAVIA SWABLU HOPPIP FARFETCHD SENTRET",
        "ROWLET PACHIRISU DEDENNE KRICKETOT BUNEARY PIDGEY",
        "HOOTHOOT NATU VULLABY GASTLY ROWLET MURKROW",
    ),
    "BERRY_FOREST": (
        "SCATTERBUG GRUBBIN SEWADDLE SKWOVET PETILIL KRICKETOT",
        "BUNNELBY PIKIPEK LILLIPUP SMOLIV COMBEE ODDISH",
        "NYMBLE FOMANTIS SCATTERBUG BELLSPROUT SHROOMISH TROPIUS",
        "SPINARAK HOOTHOOT MORELULL TAROUNTULA VENONAT NICKIT",
    ),
    "BOND_BRIDGE": (
        "PIKIPEK STARLY SKIDDO PIDOVE WINGULL MANKEY",
        "SHINX ROOKIDEE MAREEP BLITZLE LECHONK PIDGEY",
        "PACHIRISU TOGEDEMARU YUNGOOS BUNNELBY CHERUBI RATTATA",
        "STUNKY VULLABY MURKROW HOOTHOOT GREAVARD GASTLY",
    ),
    "ICEFALL": (
        "SNOM CUBCHOO SWINUB SNORUNT SEEL FRIGIBAX",
        "BERGMITE STUNFISK EISCUE SNEASEL SPHEAL ZUBAT",
        "VANILLITE SNORUNT CRABRAWLER SHELLDER JYNX ARROKUDA",
        "SNEASEL FRILLISH VANILLITE GOLBAT GLALIE BERGMITE",
    ),
    "ICEFALL_DEEP": (
        "BERGMITE CUBCHOO DELIBIRD SWINUB SNOM GLALIE",
        "STUNFISK EISCUE SEEL SPHEAL SNEASEL CRYOGONAL",
        "SNORUNT VANILLISH SHELLDER FRIGIBAX JYNX ZUBAT",
        "SNEASEL FRILLISH GLALIE GOLBAT VANILLITE ABOMASNOW",
    ),
    "PATTERN_BUSH": (
        "GRUBBIN KRICKETOT SCATTERBUG SEWADDLE WEEDLE COMBEE",
        "VENIPEDE LEDYBA BLIPBUG NYMBLE PINSIR HERACROSS",
        "SIZZLIPEDE CUTIEFLY SURSKIT PARAS BURMY SHUCKLE",
        "SPINARAK JOLTIK TAROUNTULA VENONAT WURMPLE SCYTHER",
    ),
    "LOST_CAVE": (
        "PHANTUMP DUSKULL GASTLY ZUBAT MURKROW GREAVARD",
        "SHUPPET LITWICK GASTLY WOOBAT SINISTEA HONEDGE",
        "MISDREAVUS YAMASK HAUNTER ZUBAT PUMPKABOO SKORUPI",
        "DRIFLOON GREAVARD MURKROW DUSKULL SABLEYE GASTLY",
    ),
    "LOST_CAVE_DEEP": (
        "PHANTUMP DUSCLOPS HAUNTER GOLBAT SPIRITOMB MIMIKYU",
        "MISMAGIUS LITWICK HAUNTER GOLBAT SINISTEA HONEDGE",
        "PUMPKABOO FRILLISH GASTLY HAUNTER COFAGRIGUS GREAVARD",
        "GREAVARD BANETTE HAUNTER GOLBAT SABLEYE MURKROW",
    ),
    "MEADOW": (
        "SKWOVET ROWLET PIDGEY HOPPIP SMOLIV PETILIL",
        "SENTRET LILLIPUP LECHONK COTTONEE TOGEDEMARU PIKIPEK",
        "PACHIRISU YUNGOOS MEOWTH FOMANTIS DEERLING BELLSPROUT",
        "HOOTHOOT NICKIT STUNKY ODDISH VULLABY SPINARAK",
    ),
    "MEMORIAL": (
        "WINGULL ROOKIDEE GOTHITA PHANTUMP BUNEARY PIDGEOTTO",
        "ESPURR WOOLOO NATU GIRAFARIG DEERLING PACHIRISU",
        "SLOWPOKE HATENNA CHINGLING MUNNA ABRA PAWMI",
        "MISDREAVUS DUSKULL GASTLY MORELULL GREAVARD MURKROW",
    ),
    "WATER_PATH": (
        "WINGULL PSYDUCK SKRELP ALOMOMOLA CRABRAWLER SPOINK",
        "FLETCHLING BIBAREL MAREANIE LOTAD PIKIPEK DEWPIDER",
        "TYMPOLE PIPLUP ODDISH BUIZEL SLOWPOKE CHEWTLE",
        "STUNKY MURKROW IMPIDIMP SKORUPI MORELULL CROAGUNK",
    ),
    "RUIN_VALLEY": (
        "ARON SKIDDO NACLI RHYHORN NOSEPASS ROLYCOLY",
        "ROGGENROLA TIMBURR LUCARIO SANDSHREW DWEBBLE GIRAFARIG",
        "BRONZOR SOLOSIS BELDUM MAKUHITA SKARMORY ONIX",
        "SABLEYE GREAVARD DUSKULL IMPIDIMP MAWILE ZUBAT",
    ),
    "SEVAULT": (
        "SANDSHREW ROCKRUFF ROLYCOLY SILICOBRA TRAPINCH GEODUDE",
        "DIGLETT DRILBUR NACLI HIPPOPOTAS CUBONE ORTHWORM",
        "SANDILE SANDYGAST MACHOP NOSEPASS BRONZOR TRAPINCH",
        "MAWILE SABLEYE GREAVARD PAWNIARD ZUBAT GRAVELER",
    ),
}
LAND_MAPS = {
    "MT_EMBER_EXTERIOR": "EMBER_EXT",
    "MT_EMBER_SUMMIT_PATH_1F": "EMBER_SUMMIT",
    "MT_EMBER_SUMMIT_PATH_2F": "EMBER_SUMMIT",
    "MT_EMBER_SUMMIT_PATH_3F": "EMBER_SUMMIT",
    "MT_EMBER_RUBY_PATH_1F": "EMBER_RUBY",
    "MT_EMBER_RUBY_PATH_B1F": "EMBER_RUBY",
    "MT_EMBER_RUBY_PATH_B1F_STAIRS": "EMBER_RUBY",
    "MT_EMBER_RUBY_PATH_B2F": "EMBER_RUBY_DEEP",
    "MT_EMBER_RUBY_PATH_B2F_STAIRS": "EMBER_RUBY_DEEP",
    "MT_EMBER_RUBY_PATH_B3F": "EMBER_RUBY_DEEP",
    "ONE_ISLAND_KINDLE_ROAD": "KINDLE",
    "ONE_ISLAND_TREASURE_BEACH": "TREASURE",
    "TWO_ISLAND_CAPE_BRINK": "CAPE_BRINK",
    "THREE_ISLAND_BERRY_FOREST": "BERRY_FOREST",
    "THREE_ISLAND_BOND_BRIDGE": "BOND_BRIDGE",
    "FOUR_ISLAND_ICEFALL_CAVE_ENTRANCE": "ICEFALL",
    "FOUR_ISLAND_ICEFALL_CAVE_1F": "ICEFALL",
    "FOUR_ISLAND_ICEFALL_CAVE_B1F": "ICEFALL_DEEP",
    "FOUR_ISLAND_ICEFALL_CAVE_BACK": "ICEFALL_DEEP",
    "SIX_ISLAND_PATTERN_BUSH": "PATTERN_BUSH",
    "FIVE_ISLAND_MEADOW": "MEADOW",
    "FIVE_ISLAND_MEMORIAL_PILLAR": "MEMORIAL",
    "SIX_ISLAND_WATER_PATH": "WATER_PATH",
    "SIX_ISLAND_RUIN_VALLEY": "RUIN_VALLEY",
    "SEVEN_ISLAND_SEVAULT_CANYON_ENTRANCE": "SEVAULT",
    "SEVEN_ISLAND_SEVAULT_CANYON": "SEVAULT",
}
for _n in range(1, 11):
    LAND_MAPS[f"FIVE_ISLAND_LOST_CAVE_ROOM{_n}"] = "LOST_CAVE"
for _n in range(11, 15):
    LAND_MAPS[f"FIVE_ISLAND_LOST_CAVE_ROOM{_n}"] = "LOST_CAVE_DEEP"

# list key -> (water list, fishing list). Water has 5 slots, fishing 10 (old rod 2, good rod 3, super rod 5).
WATER_LISTS = {
    "SEA_ONE": ("TENTACOOL WINGULL KRABBY PYUKUMUKU BINACLE",
                "MAGIKARP TENTACOOL SHELLDER HORSEA STARYU CLAMPERL LUVDISC ARROKUDA WIMPOD CETODDLE"),
    "SEA_TWO": ("TENTACOOL WINGULL CHEWTLE MANTYKE FINIZEN",
                "MAGIKARP KRABBY HORSEA CORSOLA STARYU CORPHISH SKRELP FRILLISH BRUXISH CETODDLE"),
    "SEA_FOUR": ("TENTACOOL SHELLDER SEEL CLAUNCHER WIMPOD",
                 "MAGIKARP TENTACOOL SPHEAL HORSEA STARYU CLAMPERL REMORAID FINIZEN ARROKUDA FRILLISH"),
    "SEA_FIVE": ("TENTACOOL WINGULL SKRELP MAREANIE PYUKUMUKU",
                 "MAGIKARP SHELLDER KRABBY STARYU MANTYKE LUVDISC CORSOLA DEWPIDER BARRASKEWDA WISHIWASHI_SOLO"),
    "SEA_SIX": ("TENTACOOL WINGULL BINACLE CLAUNCHER CHEWTLE",
                "MAGIKARP KRABBY HORSEA STARYU CORPHISH FINNEON LAPRAS WIMPOD ARROKUDA CETODDLE"),
    "LAGOON": ("PSYDUCK MARILL WOOPER TYMPOLE FINNEON",
               "MAGIKARP GOLDEEN POLIWAG BARBOACH DEWPIDER BUIZEL WOOPER BASCULIN CHINCHOU CLAUNCHER"),
    "ICE_SEA": ("SEEL SHELLDER SPHEAL CLAMPERL FINIZEN",
                "MAGIKARP SHELLDER SEEL STARYU CORSOLA REMORAID WIMPOD FRILLISH ARROKUDA CETODDLE"),
    "DEEP_SEA": ("TENTACRUEL WAILMER LANTURN MANTINE BASCULIN",
                 "MAGIKARP CHINCHOU STARYU LAPRAS WAILMER MANTYKE FINIZEN FRILLISH BRUXISH CETODDLE"),
}
WATER_MAPS = {
    "ONE_ISLAND": "SEA_ONE",
    "ONE_ISLAND_KINDLE_ROAD": "SEA_ONE",
    "ONE_ISLAND_TREASURE_BEACH": "SEA_ONE",
    "TWO_ISLAND_CAPE_BRINK": "SEA_TWO",
    "THREE_ISLAND_BERRY_FOREST": "LAGOON",
    "THREE_ISLAND_BOND_BRIDGE": "SEA_TWO",
    "FOUR_ISLAND": "SEA_FOUR",
    "FOUR_ISLAND_ICEFALL_CAVE_ENTRANCE": "ICE_SEA",
    "FOUR_ISLAND_ICEFALL_CAVE_BACK": "ICE_SEA",
    "FIVE_ISLAND": "SEA_FIVE",
    "FIVE_ISLAND_RESORT_GORGEOUS": "SEA_FIVE",
    "FIVE_ISLAND_WATER_LABYRINTH": "DEEP_SEA",
    "FIVE_ISLAND_MEADOW": "LAGOON",
    "FIVE_ISLAND_MEMORIAL_PILLAR": "SEA_FIVE",
    "SIX_ISLAND_OUTCAST_ISLAND": "SEA_SIX",
    "SIX_ISLAND_GREEN_PATH": "SEA_SIX",
    "SIX_ISLAND_WATER_PATH": "SEA_SIX",
    "SIX_ISLAND_RUIN_VALLEY": "LAGOON",
    "SEVEN_ISLAND_TANOBY_RUINS": "DEEP_SEA",
}

ROCK_LISTS = {
    "EMBER": "GEODUDE ROGGENROLA NOSEPASS CARKOL ROLYCOLY",
    "KINDLE": "SHUCKLE ROGGENROLA ONIX NACLI DWEBBLE",
    "SEVAULT": "GEODUDE DWEBBLE SHUCKLE CARBINK ROLYCOLY",
}
ROCK_MAPS = {
    "MT_EMBER_EXTERIOR": "EMBER",
    "MT_EMBER_SUMMIT_PATH_2F": "EMBER",
    "MT_EMBER_RUBY_PATH_1F": "EMBER",
    "MT_EMBER_RUBY_PATH_B1F": "EMBER",
    "MT_EMBER_RUBY_PATH_B2F": "EMBER",
    "MT_EMBER_RUBY_PATH_B3F": "EMBER",
    "MT_EMBER_RUBY_PATH_B1F_STAIRS": "EMBER",
    "MT_EMBER_RUBY_PATH_B2F_STAIRS": "EMBER",
    "ONE_ISLAND_KINDLE_ROAD": "KINDLE",
    "SEVEN_ISLAND_SEVAULT_CANYON": "SEVAULT",
}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()

    valid = load_species()
    banned = load_banned()
    with open(JSON_PATH) as f:
        data = json.load(f, object_pairs_hook=OrderedDict)
    encounters = data["wild_encounter_groups"][0]["encounters"]

    def table(map_name, time):
        hits = [e for e in encounters if e["map"] == "MAP_" + map_name and e["base_label"].endswith("_Kanto_" + time)]
        assert len(hits) == 1, f"{map_name} {time}: {len(hits)} tables"
        return hits[0]

    # Validate each shared list once, then apply it to every map that uses it.
    parsed_land = {}
    for key, lists in LAND_LISTS.items():
        assert len(lists) == 4, key
        parsed_land[key] = [parse(spec, 6, f"{key} {t} land", valid, banned, "land") for t, spec in zip(TIMES, lists)]
        names = {n for spec in lists for n in spec.split()}
        gens = {gen_of(valid[n]) for n in names}
        assert len(gens) >= MIN_MAP_GENS, f"{key}: only generations {sorted(gens)} across times"

    for map_name, key in LAND_MAPS.items():
        for time, sp in zip(TIMES, parsed_land[key]):
            set_species(table(map_name, time)["land_mons"], [sp[i] for i in LAND_PATTERN], f"{map_name} {time} land")

    gen_weight = {g: 0 for g in range(1, 10)}
    for map_name, key in LAND_MAPS.items():
        for spec in LAND_LISTS[key]:
            for weight, n in zip(LAND_WEIGHTS, spec.split()):
                gen_weight[gen_of(valid[n])] += weight
    total = sum(gen_weight.values())
    share = {g: round(100 * w / total, 1) for g, w in gen_weight.items()}
    print("land encounter share per generation (%):", share)
    assert min(share.values()) >= MIN_GEN_SHARE, f"a generation is under {MIN_GEN_SHARE}% of land encounters"

    for map_name, key in WATER_MAPS.items():
        water, fishing = WATER_LISTS[key]
        for time in TIMES:
            e = table(map_name, time)
            ctx = f"{map_name} {time}"
            set_species(e["water_mons"], parse(water, 5, ctx + " water", valid, banned, "water"), ctx + " water")
            set_species(e["fishing_mons"], parse(fishing, 10, ctx + " fishing", valid, banned, "fishing"), ctx + " fishing")

    for map_name, key in ROCK_MAPS.items():
        for time in TIMES:
            ctx = f"{map_name} {time} rock smash"
            set_species(table(map_name, time)["rock_smash_mons"],
                        parse(ROCK_LISTS[key], 5, ctx, valid, banned, "rock smash"), ctx)

    maps = set(LAND_MAPS) | set(WATER_MAPS) | set(ROCK_MAPS)
    print(f"maps authored: {len(maps)}  tables touched: {len(maps) * 4}")

    if not args.dry_run:
        with open(JSON_PATH, "w") as f:
            json.dump(data, f, indent=2)
            f.write("\n")


if __name__ == "__main__":
    main()
