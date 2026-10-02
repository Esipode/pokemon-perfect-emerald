#!/usr/bin/env python3
"""List every TM/HM site in the HnS Johto set (plan §0.9).

A site is one place that hands out a TM/HM: an item ball, a hidden item, a gift (gym leader,
NPC or HM giver), a mart slot or a Game Corner prize. Other mentions of the item (checkitem,
removeitem, bufferitemname, ...) are listed as references. Numbered HnS ids (`ITEM_TM75`) are
resolved through HnS's own FOREACH_TM. Run from anywhere:

    python3 tools/gs_convert/tm_sites.py [--hns <path>] [--tsv <out.tsv>] [--map [--write]]

`--map` resolves each site to its Johto `ITEM_TM_<MOVE>` alias (§4.7) and checks that the 64 new
TMs are each used once; `--write` stores the result in out/rename_map.json under `tm_sites`.
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


# §4.7: new move -> HnS item replaced, as "<TM|HM>_<MOVE>". The Game Corner prizes ("prize") reuse
# some HnS moves also given elsewhere, so a prize site is looked up under its own key.
JOHTO_TMS = """
HEADBUTT TM_REST|PSYCHIC_FANGS TM_ROAR|MAGICAL_LEAF TM_BULLET_SEED|HYPNOSIS HM_FLASH|ROOST TM_ROOST
ROCK_POLISH TM_ROCK_TOMB|AQUA_TAIL TM_RAIN_DANCE|LUNGE TM_U_TURN|LEAF_BLADE HM_CUT|FALSE_SWIPE TM_FALSE_SWIPE
EMBARGO TM_EMBARGO|FAKE_OUT TM_ATTRACT|WISH TM_RETURN|PAYBACK TM_PAYBACK|SIGNAL_BEAM TM_DIG
ROCK_CLIMB HM_ROCK_SMASH|LAVA_PLUME TM_TAUNT|AQUA_JET HM_SURF|SHADOW_SNEAK TM_SHADOW_BALL
MACH_PUNCH TM_DRAIN_PUNCH|HEAL_BELL TM_SNATCH|SPARK TM_SWAGGER|DISCHARGE TM_CHARGE_BEAM
BULLET_PUNCH HM_STRENGTH|PLUCK TM_PLUCK|CROSS_CHOP TM_FOCUS_PUNCH|DUAL_WINGBEAT HM_FLY
SMART_STRIKE TM_IRON_TAIL|PSYCHO_CUT TM_SHADOW_CLAW|RAPID_SPIN TM_AERIAL_ACE|BOUNCE TM_SECRET_POWER
WEATHER_BALL TM_HIDDEN_POWER|POISON_FANG TM_SLUDGE_BOMB|SUCKER_PUNCH TM_THIEF|SKY_ATTACK HM_WHIRLPOOL
ICICLE_CRASH TM_HAIL|MORNING_SUN TM_SUNNY_DAY|EXTRASENSORY TM_CALM_MIND|ICE_SHARD HM_WATERFALL
AVALANCHE TM_AVALANCHE|DRAGON_RUSH TM_DRAGON_PULSE|DRILL_RUN TM_DRAGON_CLAW|SAND_TOMB TM_SANDSTORM
PURSUIT TM_DARK_PULSE|HEAT_CRASH TM_EARTHQUAKE|BLAZE_KICK TM_FLAMETHROWER|MIRROR_COAT TM_STEEL_WING
EXTREME_SPEED TM_STEALTH_ROCK|BREAKING_SWIPE TM_NATURAL_GIFT|ANCIENT_POWER TM_LIGHT_SCREEN
MEGAHORN TM_PROTECT|MOONBLAST TM_REFLECT|FREEZE_DRY TM_BLIZZARD|ZAP_CANNON TM_HYPER_BEAM
VACUUM_WAVE TM_SOLAR_BEAM|SYNTHESIS TM_THUNDER|STUN_SPORE TM_FIRE_BLAST|TRI_ATTACK TM_FOCUS_BLAST
BELLY_DRUM prize:TM_ICE_BEAM|QUIVER_DANCE prize:TM_FLAMETHROWER|COIL prize:TM_THUNDERBOLT
LOVELY_KISS prize:TM_REST|PERISH_SONG prize:TM_SWORDS_DANCE|DESTINY_BOND prize:TM_SUBSTITUTE
"""


def johto_tm_map():
    """(site class, "<TM|HM>_<MOVE>") -> Johto move; entries are in TM161.. order."""
    out = {}
    for entry in JOHTO_TMS.replace("\n", "|").split("|"):
        if not entry.strip():
            continue
        new, old = entry.split()
        cls, _, old = old.rpartition(":") if ":" in old else ("other", "", old)
        assert (cls, old) not in out, (cls, old)
        out[(cls, old)] = new
    assert len(out) == 64, len(out)
    return out


def alias_map(sites, tm_map):
    """Per-site alias table; asserts every §4.7 row is used by at least one site."""
    rows, used = [], set()
    for s in sites:
        if s["site"] == "dropped_mart":
            continue
        cls = "prize" if s["site"] == "prize" else "other"
        key = (cls, "%s_%s" % (s["kind"], s["move"]))
        # HnS marts and prizes list moves as TM_<MOVE>; numbered ids resolve to the same form.
        new = tm_map.get(key)
        if new is None:
            raise SystemExit("no §4.7 row for site %s %s %s" % (s["site"], s["map"], s["item"]))
        used.add(key)
        rows.append(dict(map=s["map"], site=s["site"], line=s["line"], label=s["label"],
                         hns_item=s["item"], alias="ITEM_TM_" + new))
    missing = sorted(set(tm_map) - used)
    if missing:
        raise SystemExit("§4.7 rows with no site: %s" % missing)
    return rows


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
    parser.add_argument("--map", action="store_true", help="resolve each site to its Johto TM alias")
    parser.add_argument("--write", action="store_true", help="with --map, write out/rename_map.json")
    args = parser.parse_args()
    sites, refs = scan(args.hns)
    if args.map:
        rows = alias_map(sites, johto_tm_map())
        print("%d sites mapped, 64 §4.7 rows used" % len(rows))
        if args.write:
            path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "out", "rename_map.json")
            with open(path, encoding="utf-8") as f:
                rm = json.load(f)
            rm["tm_sites"] = {"status": "mapped Stage 12a", "count": len(rows), "sites": rows}
            with open(path, "w", encoding="utf-8", newline="\n") as f:
                json.dump(rm, f, indent=1, sort_keys=True, ensure_ascii=False)
                f.write("\n")
        return
    for s in sites:
        print("%-12s %-40s %-24s %s" % (s["site"], s["map"], s["item"], s["move"]))
    print(json.dumps(summary(sites)))
    if args.tsv:
        with open(args.tsv, "w") as f:
            for r in sites + [dict(r, site="ref") for r in refs]:
                f.write("\t".join(str(r.get(k, "")) for k in ("site", "map", "line", "label", "item", "move")) + "\n")


if __name__ == "__main__":
    main()
