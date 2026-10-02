#!/usr/bin/env python3
"""Stage 12a: emit Johto TM161-TM224 into items.h (constants + data) and tms_hms.h.

Idempotent: each edit is skipped when its marker is already present.
"""
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

# (move, three description lines) in §4.7 order, TM161 first.
TMS = [
    ("HEADBUTT", "Rams the foe with", "the head. May", "cause flinching."),
    ("PSYCHIC_FANGS", "Bites with psychic", "power. Breaks", "barriers."),
    ("MAGICAL_LEAF", "Hurls magical", "leaves that", "never miss."),
    ("HYPNOSIS", "A hypnotic urge", "puts the foe", "to sleep."),
    ("ROOST", "The user lands", "and rests to", "restore HP."),
    ("ROCK_POLISH", "Polishes the body", "to sharply", "raise Speed."),
    ("AQUA_TAIL", "Swings a tail like", "a wave to", "strike the foe."),
    ("LUNGE", "Lunges at the foe.", "Lowers its", "Attack."),
    ("LEAF_BLADE", "Slashes with a", "sharp leaf. High", "critical rate."),
    ("FALSE_SWIPE", "Leaves the foe", "with at least", "1 HP."),
    ("EMBARGO", "Seals the foe's", "items so they", "cannot be used."),
    ("FAKE_OUT", "A first-turn", "strike that", "causes flinching."),
    ("WISH", "Heals the user or", "its ally on", "the next turn."),
    ("PAYBACK", "Power doubles if", "the user moves", "after the foe."),
    ("SIGNAL_BEAM", "A strange beam.", "May confuse", "the foe."),
    ("ROCK_CLIMB", "A charge that", "may confuse", "the foe."),
    ("LAVA_PLUME", "Scarlet flames", "hit all around.", "May burn."),
    ("AQUA_JET", "A watery strike", "that always", "goes first."),
    ("SHADOW_SNEAK", "Extends a shadow", "to strike the", "foe. Goes first."),
    ("MACH_PUNCH", "A punch thrown at", "blinding speed.", "Goes first."),
    ("HEAL_BELL", "A soothing bell", "heals all status", "in the party."),
    ("SPARK", "A jolt of", "electricity.", "May paralyze."),
    ("DISCHARGE", "A flare of", "electricity hits", "all. May paralyze."),
    ("BULLET_PUNCH", "A punch as hard", "as steel.", "Goes first."),
    ("PLUCK", "Pecks the foe.", "Steals and eats", "its Berry."),
    ("CROSS_CHOP", "Chops with both", "fists. High", "critical rate."),
    ("DUAL_WINGBEAT", "Hits the foe", "twice with", "both wings."),
    ("SMART_STRIKE", "A sharp horn", "strike that", "never misses."),
    ("PSYCHO_CUT", "Tears with blades", "of psychic power.", "High crit rate."),
    ("RAPID_SPIN", "A spin attack that", "clears traps", "and binding."),
    ("BOUNCE", "Bounces up, then", "drops on the foe.", "May paralyze."),
    ("WEATHER_BALL", "Type and power", "change with", "the weather."),
    ("POISON_FANG", "Bites with toxic", "fangs. May badly", "poison."),
    ("SUCKER_PUNCH", "Strikes first if", "the foe is about", "to attack."),
    ("SKY_ATTACK", "Charges, then", "strikes. High", "critical rate."),
    ("ICICLE_CRASH", "Hurls large icicles", "at the foe.", "May flinch."),
    ("MORNING_SUN", "Restores HP. The", "amount depends", "on the weather."),
    ("EXTRASENSORY", "Unseen force hits", "the foe.", "May flinch."),
    ("ICE_SHARD", "Hurls a shard of", "ice. Always", "goes first."),
    ("AVALANCHE", "Power doubles if", "the user was", "hit this turn."),
    ("DRAGON_RUSH", "Charges with", "menace.", "May flinch."),
    ("DRILL_RUN", "Spins like a", "drill. High", "critical rate."),
    ("SAND_TOMB", "Traps the foe", "in a whirling", "sandstorm."),
    ("PURSUIT", "Hits hard if the", "foe is about", "to switch out."),
    ("HEAT_CRASH", "Slams the foe.", "Heavier users", "hit harder."),
    ("BLAZE_KICK", "A fiery kick.", "High crit rate.", "May burn."),
    ("MIRROR_COAT", "Returns double", "the damage of a", "special hit."),
    ("EXTREME_SPEED", "A blindingly fast", "charge that", "always goes first."),
    ("BREAKING_SWIPE", "Swings a tough", "tail at all foes.", "Lowers Attack."),
    ("ANCIENT_POWER", "An ancient power.", "May raise all", "of the stats."),
    ("MEGAHORN", "A tremendous", "horn attack with", "great power."),
    ("MOONBLAST", "Borrows the moon's", "power. May lower", "Sp. Atk."),
    ("FREEZE_DRY", "Freezes the foe.", "Super effective", "on Water types."),
    ("ZAP_CANNON", "A huge blast that", "always paralyzes", "if it hits."),
    ("VACUUM_WAVE", "A wave of vacuum", "that always", "goes first."),
    ("SYNTHESIS", "Restores HP. The", "amount depends", "on the weather."),
    ("STUN_SPORE", "Scatters powder", "that paralyzes", "the foe."),
    ("TRI_ATTACK", "Fires three", "beams. May burn,", "freeze or paralyze."),
    ("BELLY_DRUM", "Cuts HP to max", "out the user's", "Attack."),
    ("QUIVER_DANCE", "A mystical dance", "that raises", "Sp. Atk, Sp. Def."),
    ("COIL", "Coils up to raise", "Attack, Defense", "and accuracy."),
    ("LOVELY_KISS", "A scary face and", "kiss put the", "foe to sleep."),
    ("PERISH_SONG", "Any Pokémon that", "hears this faints", "in 3 turns."),
    ("DESTINY_BOND", "If the user faints,", "the foe that", "hit it faints."),
]
assert len(TMS) == 64


def edit(path, marker, fn):
    p = ROOT / path
    s = p.read_text()
    if marker in s:
        print(f"skip {path}")
        return
    p.write_text(fn(s))
    print(f"edit {path}")


def consts(s):
    lines = "    // Johto TMs\n" + "".join(
        f"    ITEM_TM{161 + i} = {934 + i},\n" for i in range(64))
    return s.replace("    ITEM_TM160 = 933,\n", "    ITEM_TM160 = 933,\n\n" + lines, 1)


def data(s):
    blocks = ""
    for i, (move, *d) in enumerate(TMS):
        desc = "\n".join(f'            "{l}\\n"' for l in d[:-1]) + f'\n            "{d[-1]}"'
        if not d[-1]:
            raise SystemExit(f"{move}: empty last line")
        blocks += f"""    [ITEM_TM{161 + i}] =
    {{
        .name = ITEM_NAME("TM{161 + i}"),
        .price = 3000,
        .description = COMPOUND_STRING(
{desc}),
        .importance = I_REUSABLE_TMS,
        .pocket = POCKET_TM_HM,
        .type = ITEM_USE_PARTY_MENU,
        .fieldUseFunc = ItemUseOutOfBattle_TMHM,
        .secondaryId = MOVE_{move},
    }},

"""
    anchor = "        .secondaryId = MOVE_POISON_JAB,\n    },\n\n"
    i = s.index("[ITEM_TM160]")
    j = s.index(anchor, i) + len(anchor)
    return s[:j] + blocks + s[j:]


def tms(s):
    body = "".join(f" \\\n    F({m})" for m, *_ in TMS)
    return s.replace("    F(POISON_JAB)\n", "    F(POISON_JAB)" + body + "\n", 1)


edit("include/constants/items.h", "ITEM_TM224 = 997", consts)
edit("src/data/items.h", "[ITEM_TM224]", data)
edit("include/constants/tms_hms.h", "F(DESTINY_BOND)", tms)
