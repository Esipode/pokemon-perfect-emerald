#!/usr/bin/env python3
"""Classify every flag/var name that Johto content uses into Johto, Shared, Config or Dropped.

Writes the Stage 5 review table (name, HnS/target values, bucket, final name, final id, uses,
C refs). Reads the HnS clone (`flags_hns.h`, `vars_hns.h`, Johto maps, shared scripts, feature C)
and the target headers. Standard library only. Run from the repo root:

    python3 tools/gs_convert/classify_flags_vars.py --out "<path>/GS - Flag Classification.md"
"""

import argparse
import glob
import json
import os
import re
import sys
from collections import defaultdict

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from common import ROOT, OUT_DIR, add_hns_arg, johto_set, map_scripts, read, strip_hns  # noqa: E402
from hns_manifest import target_labels, LABEL_RE  # noqa: E402

TOKEN_RE = re.compile(r"\b(?:FLAG|VAR)_[A-Za-z0-9_]+\b")
DEFINE_RE = re.compile(r"^\s*#\s*define\s+([A-Za-z_][A-Za-z0-9_]*)(\([^)]*\))?\s*(.*)$")
INCLUDE_RE = re.compile(r'^\s*#\s*include\s+"([^"]+)"')
WRITE_RE = re.compile(r"^\s*(setflag|clearflag|setvar|addvar|subvar|copyvar|setorcopyvar)\s+(\w+)")

JOHTO_FLAGS_START = 0x2000
JOHTO_FLAGS_STORY_START = 0x2200
JOHTO_TRAINER_FLAGS_START = 0x2400
JOHTO_HIDDEN_ITEMS_END = 0x21F3
JOHTO_VARS_START = 0x4200
JOHTO_VARS_END = 0x42FF
TEMP_FLAGS_END = 0x1F
TEMP_VARS_END = 0x400F
SPECIAL_FLAGS_START = 0x4000
SPECIAL_VARS_START = 0x8000

# Johto flag sub-blocks in id order (plan §4.1). Each starts on a 0x10 boundary.
FLAG_BLOCKS = ["hidden_item", "item_ball", "story", "hide", "gift", "badge", "champion", "fly"]

BUCKET_ORDER = ["Johto (renamed)", "Johto", "Shared", "Config", "Dropped", "Unresolved"]

# ---------------------------------------------------------------------------
# Rules

# Target names that mean something else in HnS Johto content (plan §0.5 hazards).
RENAMES = {
    **{f"FLAG_BADGE0{n}_GET": (f"FLAG_JOHTO_BADGE0{n}_GET", "Johto badge; the Hoenn name must not be written") for n in range(1, 9)},
    "FLAG_IS_CHAMPION": ("FLAG_JOHTO_CHAMPION", "D23: the Johto HoF sets the Johto flag, never Hoenn's"),
    "VAR_STARTER_MON": ("VAR_JOHTO_STARTER_MON", "D4: Elm-chain starter; unset when the chain is skipped (Stage 18 fallback)"),
    "FLAG_ADVENTURE_STARTED": ("FLAG_JOHTO_ADVENTURE_STARTED", "D4: only written in an unreachable label; drop candidate"),
}

# Kanto reads (D13). HnS badge 09-16 order is Pewter, Cerulean, Vermilion, Celadon, Saffron,
# Fuchsia, Seafoam/Cinnabar, Viridian; the target Kanto order swaps Saffron and Fuchsia.
KANTO_READS = {
    "FLAG_BADGE09_GET": ("FLAG_BADGE01_GET_FRLG", "D13: Pewter"),
    "FLAG_BADGE10_GET": ("FLAG_BADGE02_GET_FRLG", "D13: Cerulean"),
    "FLAG_BADGE11_GET": ("FLAG_BADGE03_GET_FRLG", "D13: Vermilion"),
    "FLAG_BADGE12_GET": ("FLAG_BADGE04_GET_FRLG", "D13: Celadon"),
    "FLAG_BADGE13_GET": ("FLAG_BADGE06_GET_FRLG", "D13: Saffron (HnS 13, target 06)"),
    "FLAG_BADGE14_GET": ("FLAG_BADGE05_GET_FRLG", "D13: Fuchsia (HnS 14, target 05)"),
    "FLAG_BADGE15_GET": ("FLAG_BADGE07_GET_FRLG", "D13: Cinnabar (HnS gym is Seafoam)"),
    "FLAG_BADGE16_GET": ("FLAG_BADGE08_GET_FRLG", "D13: Viridian"),
    "FLAG_IS_KANTO_CHAMPION": ("FLAG_KANTO_CHAMPION", "D13: read only"),
    "FLAG_DEFEATED_VIRIDIAN_GYM": ("FLAG_BADGE08_GET_FRLG", "D13: Route 47 object condition = Viridian badge; read only"),
}

# One-gift key items and system flags shared across regions (FRLG-Flags rule: read only).
SHARED_READS = {
    "FLAG_SYS_POKEDEX_GET": "Read only; already set. Mr. Pokémon writes are cut (§0.10)",
    "FLAG_SYS_NATIONAL_DEX": "Read only; already set. Mr. Pokémon writes are cut (§0.10)",
    "FLAG_SYS_POKEMON_GET": "Read only; already set. Elm's lab write is cut",
    "FLAG_SYS_B_DASH": "Read only; already set",
    "FLAG_RECEIVED_BIKE": "One bike across regions; Bike Shop write is cut, giver says already have it",
    "FLAG_RECEIVED_OLD_ROD": "One rod across regions; read only",
    "FLAG_RECEIVED_GOOD_ROD": "One rod across regions; read only",
    "FLAG_RECEIVED_SUPER_ROD": "One rod across regions; read only",
    "FLAG_RECEIVED_COIN_CASE": "One coin case across regions; read only",
    "FLAG_DONT_TRANSITION_MUSIC": "Engine flag; Johto may write it",
    "FLAG_SYS_CTRL_OBJ_DELETE": "Engine flag; Johto may write it",
    "FLAG_SYS_DEXNAV_SEARCH": "Engine flag",
    "FLAG_SYS_CYCLING_ROAD": "Engine flag",
    "FLAG_PENDING_DAYCARE_EGG": "D11: Route 34 Day-Care shares the Hoenn engine flag; clearing it after a pickup is engine behaviour",
    "FLAG_SYS_GAME_CLEAR": "Read only (PC / Lanette text); Johto Champion does not set it (D15)",
    "FLAG_SYS_PC_LANETTE": "Read only (PC text)",
    "VAR_CABLE_CLUB_STATE": "Engine var (FRLG-Flags precedent); the Goldenrod Pokémon Center sets it as the Hoenn one does",
    "VAR_SAFARI_ZONE_STATE": "D12: Hoenn Safari rules; the Johto gate runs the same engine state (Stage 13 review)",
    "FLAG_GOOD_LUCK_SAFARI_ZONE": "D12: Hoenn Safari rules; the Johto gate runs the same engine state (Stage 13 review)",
}

# Shared names Johto may write (the Note says why).
WRITE_OK = {"FLAG_PENDING_DAYCARE_EGG", "VAR_CABLE_CLUB_STATE", "VAR_SAFARI_ZONE_STATE",
            "FLAG_GOOD_LUCK_SAFARI_ZONE", "FLAG_DONT_TRANSITION_MUSIC", "FLAG_SYS_CTRL_OBJ_DELETE"}

# Hoenn state a Johto feature needs its own copy of (the Stage 1 cross-region list).
OWN_COPY = {
    "VAR_FOSSIL_RESURRECTION_STATE": ("VAR_FOSSIL_RESURRECTION_STATE_JOHTO", "Ruins of Alph lab; Hoenn Devon fossil state must not be written"),
    "VAR_WHICH_FOSSIL_REVIVED": ("VAR_WHICH_FOSSIL_REVIVED_JOHTO", "Ruins of Alph lab; Hoenn Devon fossil state must not be written"),
    "FLAG_DAILY_PICKED_LOTO_TICKET": ("FLAG_DAILY_PICKED_LOTO_TICKET_JOHTO", "Radio Tower 1F lottery; Hoenn Lilycove lottery state must not be written (Stage 26a: lottery number is shared)"),
    "VAR_POKELOT_PRIZE_ITEM": ("VAR_POKELOT_PRIZE_ITEM_JOHTO", "Radio Tower 1F lottery; Hoenn Lilycove lottery state must not be written"),
    "VAR_POKELOT_PRIZE_PLACE": ("VAR_POKELOT_PRIZE_PLACE_JOHTO", "Radio Tower 1F lottery; Hoenn Lilycove lottery state must not be written"),
    "FLAG_SYS_RIBBON_GET": ("FLAG_JOHTO_RIBBON_GET", "F34 ribbon NPC; Hoenn ribbon flag must not be written"),
    "FLAG_CAUGHT_CELEBI": ("FLAG_CAUGHT_CELEBI", "F29 / D6: GS Ball chain flag; Stage 25 may map it to the moved Celebi script's flag"),
}

# Legendaries whose encounter moves into Johto (D6): Johto reuses the existing target flags.
D6_SHARED = {
    "FLAG_HIDE_HO_OH", "FLAG_HIDE_LUGIA", "FLAG_HIDE_CELEBI",
    "FLAG_CAUGHT_HO_OH", "FLAG_CAUGHT_LUGIA",
    "FLAG_DEFEATED_HO_OH", "FLAG_DEFEATED_LUGIA",
}

# Name fragments of content that is cut or replaced (F-decisions, Locked 9, D4, D12, D13).
# (fragment, reason shown in the Note); first match wins.
DROP_FRAGMENTS = [
    ("ALOLA", "F30: Alola extension cut"), ("SINJOH", "F30: Sinjoh extension cut"),
    ("HISUI", "F30: Hisui extension cut"), ("SNOWSWEPT", "F30: Snowswept extension cut"),
    ("NOBLE", "F30: Hisui noble Pokémon cut"), ("AZURE_FLUTE", "F31: Azure Flute dropped"),
    ("SS_AQUA", "F28: S.S. Aqua dropped"), ("SSAQUA", "F28: S.S. Aqua dropped"),
    ("MATCH_CALL", "F6: phone rematches dropped"), ("MATCHCALL", "F6: phone rematches dropped"),
    ("REMATCH", "F6: phone rematches dropped"), ("HAS_ITEM_HNS", "F6: phone call gift state dropped"),
    ("_ITEM_HNS", "F6: phone call gift state dropped"),
    ("MOMS_SAVINGS", "F12: Mom's Savings dropped"), ("MOM_SAVINGS", "F12: Mom's Savings dropped"),
    ("TRAINER_HILL", "F20: Route 40 battle hub dropped"), ("BATTLEFRONTIER", "F20: Frontier ferry dropped"),
    ("BATTLE_FRONTIER", "F20: Frontier ferry dropped"), ("VOLTORB", "F11: Voltorb Flip dropped"),
    ("HELP_WINDOW", "F16: help window dropped"), ("NAME_RIVAL", "F18 / D5: literal SILVER"),
    ("ROAMER", "F8: HnS roamers dropped"), ("BAOBA", "D12: Baoba quest cut"),
    ("BUENA", "F7: radio / Buena's Password dropped"),
    ("POWER_PLANT", "D13: HnS Kanto cut"), ("COPYCAT", "D13: HnS Kanto cut"),
    ("NUZLOCKE", "F17: HnS challenge hooks"), ("RANDOMIZER", "F17: HnS challenge hooks"),
    ("VS_SEEKER", "F6: VS Seeker off"), ("FLAG_HIDE_DOJO_", "HnS Kanto Saffron dojo leader rematches (Locked 9)"),
    ("VIRIDIAN", "HnS Kanto (Locked 9, D13)"), ("VERMILION", "HnS Kanto (Locked 9, D13)"),
    ("PEWTER", "HnS Kanto (Locked 9, D13)"), ("CERULEAN", "HnS Kanto (Locked 9, D13)"),
    ("CELADON", "HnS Kanto (Locked 9, D13)"), ("FUCHSIA", "HnS Kanto (Locked 9, D13)"),
    ("LAVENDER", "HnS Kanto (Locked 9, D13)"), ("CINNABAR", "HnS Kanto (Locked 9, D13)"),
    ("SEAFOAM", "HnS Kanto (Locked 9, D13)"), ("PALLET", "HnS Kanto (Locked 9, D13)"),
    ("MTMOON", "HnS Kanto (Locked 9, D13)"), ("MT_MOON", "HnS Kanto (Locked 9, D13)"),
]
# Cut by name (regexes), each with the reason shown in the Note.
DROP_PATTERNS = [
    (r"^(FLAG|VAR)_(KANTO|IN_KANTO)_|_KANTO_(RADIO|ROCKET|SAFARI)|^FLAG_VISITED_KANTO$|^FLAG_IS_KANTO",
     "HnS Kanto (Locked 9, D13)"),
    (r"RADIO(?!TOWER)(?!_TOWER)", "F7: radio dropped"),
]
# Cut by exact name (reason in the Note).
DROP_NAMES = {
    "FLAG_RECEIVED_POKENAV": "D4: bedroom/Mom intro cut; Pokégear calls stay without PokéNav (F5)",
    "FLAG_SYS_POKENAV_GET": "D4: bedroom/Mom intro cut",
    "FLAG_REGISTER_RIVAL_POKENAV": "D4: bedroom/Mom intro cut",
    "FLAG_RECEIVED_RUNNING_SHOES": "D4: bedroom beat cut; Hoenn name, the write is cut (§0.10)",
    "FLAG_SET_WALL_CLOCK": "D4: bedroom beat cut; Hoenn name, the write is cut (§0.10)",
    "VAR_SECRET_BASE_INITIALIZED": "D4: New Bark house secret-base init cut (F36)",
    "FLAG_SAFARI_ZONE_EAST_EXPANSION": "D12: HnS Safari area customisation cut",
    "FLAG_SAFARI_ZONE_WEST_EXPANSION": "D12: HnS Safari area customisation cut",
    "VAR_PLATECOUNTER": "F30: Sinjoh/Regi extension from Mt. Silver cut",
    "VAR_REGIS_SCANNED": "F30: Sinjoh/Regi extension from Mt. Silver cut",
    "FLAG_ENABLE_BAOBA_MATCH_CALL": "F6/D12: phone and Baoba cut",
    "FLAG_HIDE_LATIAS": "D23: Hoenn legendary re-spawn cut",
    "FLAG_HIDE_LATIOS": "D23: Hoenn legendary re-spawn cut",
    "FLAG_LATIOS_OR_LATIAS_ROAMING": "D23: Hoenn roamer state; the write is cut",
    "FLAG_BATTLED_DEOXYS": "D23: Hoenn legendary reset cut",
    "FLAG_DEFEATED_DEOXYS": "D23: Hoenn legendary reset cut",
    "FLAG_DEFEATED_MEW": "D23: Kanto legendary reset cut",
    "FLAG_CAUGHT_MEW": "D23: Kanto legendary reset cut",
    "FLAG_HIDE_ARTICUNO": "D23: Kanto legendary re-spawn cut",
    "FLAG_HIDE_ZAPDOS": "D23: Kanto legendary re-spawn cut",
    "FLAG_HIDE_MOLTRES": "D6: Mt. Silver Moltres room cut; HoF re-spawn cut (D23)",
    "FLAG_HIDE_MEWTWO": "D23: Kanto legendary re-spawn cut",
    "FLAG_HIDE_GROUDON": "D23: Hoenn legendary re-spawn cut",
    "FLAG_HIDE_KYOGRE": "D23: Hoenn legendary re-spawn cut",
    "FLAG_HIDE_DEOXYS": "D23: Hoenn legendary reset cut",
    "FLAG_HIDE_TAPU_BULU": "D23: legendary re-spawn cut",
    "FLAG_HIDE_TAPU_LELE": "D23: legendary re-spawn cut",
    "FLAG_HIDE_TAPU_FINI": "D23: legendary re-spawn cut",
    "FLAG_HIDE_TAPU_KOKO": "D23: legendary re-spawn cut",
}

# HnS option flags with no value (plan §0.5); F17 makes each a constant branch.
CONFIG_NAMES = {
    "FLAG_DISABLE_EXP_GAIN": "Constant FALSE: exp gain stays on",
    "FLAG_START_NUZLOCKE": "Constant FALSE",
    "FLAG_END_NUZLOCKE": "Constant FALSE",
    "FLAG_NO_WILD_CATCHING": "Constant FALSE: catching stays on",
    "FLAG_MINTS_DISABLED": "Constant FALSE",
    "FLAG_EXP_SHARE_ENABLED": "Constant FALSE (target has its own Exp. Share rules)",
    "FLAG_RECEIVED_FIRST_BALLS": "Constant TRUE: balls are not an HnS starter gift here",
    "FLAG_SHINY_STARTER_1": "Starter shiny roll option; starter scripts are cut",
    "FLAG_SHINY_STARTER_2": "Starter shiny roll option; starter scripts are cut",
    "FLAG_SHINY_STARTER_3": "Starter shiny roll option; starter scripts are cut",
    "FLAG_FINITE_TMS": "F17 / D20: take the infinite-TM branch",
}

# Per-location names that collide with a target name in another region get `_JOHTO`.
COLLISION_PREFIXES = ("FLAG_HIDDEN_ITEM_", "FLAG_ITEM_", "FLAG_HIDE_", "FLAG_DEFEATED_",
                      "FLAG_VISITED_", "FLAG_GOT_", "FLAG_RECEIVED_", "FLAG_CAUGHT_", "FLAG_BEAT_")

SCRATCH_RE = re.compile(r"^(FLAG_TEMP_|VAR_TEMP_|VAR_0x|VAR_RESULT$|VAR_FACING$|VAR_LAST_|VAR_ITEM_ID$|VAR_SPECIAL_)")

# Hand-made notes for single names.
NAME_NOTES = {
    "FLAG_HIDE_RAYQUAZA": "Embedded Tower Rayquaza is Johto-owned (Stage 25); the Hoenn name and the HoF write stay untouched",
    "FLAG_INDIGOJUNCTION_HIDE_KANTO_GUARD": "Reception Gate guard (D21 8-badge gate); name keeps HnS wording",
    "FLAG_INDIGOJUNCTION_HIDE_SILVER_GUARD": "Reception Gate guard (D21 8-badge gate)",
    "VAR_NUM_BADGES": "Badge-count proxy (§0.10): Cianwood/Olivine/Mahogany gates read it; Stage 26a",
    "FLAG_RECEIVED_RUNNING_SHOES": "D4: bedroom beat cut; drop candidate",
    "FLAG_SET_WALL_CLOCK": "D4: bedroom beat cut; drop candidate",
    "FLAG_GOT_EEVEE": "Goldenrod Eevee gift; the target name is Kanto's Celadon Eevee",
}

# Review hints by name fragment. First match wins.
NOTE_RULES = [
    ("TM_", "D18: TM site (Stage 12a)"),
    ("RECEIVED_HM", "D18: HM gift site becomes a TM site"),
    ("GYM", "Gym state"),
    ("RIVAL", "Silver battle state; check order safety (Stage 18, 26a)"),
    ("STARTER", "D4: Elm chain optional"),
    ("SUDOWOODO", "Squirt Bottle gate (D21)"),
    ("BUG_CONTEST", "F9 Bug-Catching Contest"),
    ("SLIDING", "F10 Ruins of Alph puzzles"),
    ("PUZZLE", "F10 Ruins of Alph puzzles"),
    ("KURT", "F13 Kurt's ball shop"),
    ("GS_BALL", "F29 / D6: GS Ball chain"),
    ("CELEBI", "F29 / D6: Celebi moves to the Ilex shrine"),
    ("WHIRLPOOL", "F32: 7 Johto badges"),
    ("PASS", "D1/D2: Magnet Train Pass"),
    ("MAGNET", "F27: Magnet Train"),
    ("ROCKET", "Team Rocket takeover; order dependency (Stage 26a)"),
    ("RADIO_TOWER", "Radio Tower takeover; order dependency (Stage 26a)"),
]

# Target C that reads shared flag/var state Johto content will set (plan Stage 5 source 5).
C_READER_NOTES = {
    "FLAG_JOHTO_BADGE": "Badge counters, level cap, Trainer Card: Stage 6 audit, Stage 27",
    "FLAG_JOHTO_CHAMPION": "Level cap, Hall of Fame, Mt. Silver gate: Stage 6 audit",
}


# ---------------------------------------------------------------------------
# Header parsing (same macro evaluator as the Kanto tool, parametrised on root)


def strip_c_comments(text):
    text = re.sub(r"/\*.*?\*/", lambda m: "\n" * m.group(0).count("\n"), text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


class Define:
    def __init__(self, name, expr, path, line):
        self.name = name
        self.expr = expr
        self.path = path
        self.line = line


class World:
    def __init__(self, root, defs):
        self.root = root
        self.defs = {k: Define(k, str(v), "<cmdline>", 0) for k, v in defs.items()}
        self.cache = {}
        self.seen = set()

    def load(self, rel):
        path = os.path.join(self.root, "include", rel)
        if rel in self.seen or not os.path.exists(path):
            return
        self.seen.add(rel)
        with open(path, encoding="utf-8", errors="replace") as f:
            lines = strip_c_comments(f.read()).split("\n")
        stack = []
        i = 0
        while i < len(lines):
            line = lines[i]
            lineno = i + 1
            while line.endswith("\\") and i + 1 < len(lines):
                i += 1
                line = line[:-1] + " " + lines[i]
            i += 1
            s = line.strip()
            if not s.startswith("#"):
                continue
            d = s[1:].strip()
            active = all(a for a, _ in stack)
            if d.startswith("ifndef") or d.startswith("ifdef"):
                name = d.split()[1]
                cond = (name in self.defs) == d.startswith("ifdef")
                if d.startswith("ifndef") and name.startswith("GUARD_"):
                    cond = True
                stack.append([cond, cond])
            elif d.startswith("if"):
                cond = bool(self.eval_cond(d[2:])) if active else False
                stack.append([cond, cond])
            elif d.startswith("elif"):
                top = stack[-1]
                cond = (not top[1]) and bool(self.eval_cond(d[4:]))
                top[0] = cond
                top[1] = top[1] or cond
            elif d.startswith("else"):
                top = stack[-1]
                top[0] = not top[1]
                top[1] = True
            elif d.startswith("endif"):
                stack.pop()
            elif not active:
                continue
            elif d.startswith("include"):
                m = INCLUDE_RE.match(s)
                if m and m.group(1).startswith("constants/"):
                    self.load(m.group(1))
            elif d.startswith("define"):
                m = DEFINE_RE.match(s)
                if m and not m.group(2):
                    self.defs[m.group(1)] = Define(m.group(1), m.group(3).strip(), os.path.join("include", rel), lineno)
            elif d.startswith("undef"):
                self.defs.pop(d.split()[1], None)

    def eval_cond(self, expr):
        expr = re.sub(r"defined\s*\(\s*(\w+)\s*\)", lambda m: "1" if m.group(1) in self.defs else "0", expr)
        expr = re.sub(r"defined\s+(\w+)", lambda m: "1" if m.group(1) in self.defs else "0", expr)
        try:
            return self.eval_expr(expr, set())
        except Exception:
            return 0

    def value(self, name):
        if name in self.cache:
            return self.cache[name]
        if name not in self.defs:
            return None
        try:
            v = self.eval_expr(self.defs[name].expr, {name})
        except Exception:
            v = None
        self.cache[name] = v
        return v

    def eval_expr(self, expr, stack):
        def ident(m):
            tok = m.group(0)
            if tok in stack:
                raise ValueError("recursive macro " + tok)
            if tok not in self.defs:
                return "0"
            v = self.eval_expr(self.defs[tok].expr, stack | {tok})
            return "(" + str(v) + ")"

        expr = re.sub(r"\b(0[xX][0-9a-fA-F]+|\d+)[uUlL]*\b", r"\1", expr)
        expr = re.sub(r"\b0[xX][0-9a-fA-F]+\b", lambda m: str(int(m.group(0), 16)), expr)
        expr = re.sub(r"(?<![0-9])\b[A-Za-z_][A-Za-z0-9_]*\b", ident, expr)
        expr = expr.replace("&&", " and ").replace("||", " or ").replace("/", "//")
        expr = re.sub(r"!(?!=)", " not ", expr)
        if not expr.strip():
            return 0
        return int(eval(expr, {"__builtins__": {}}, {}))


# ---------------------------------------------------------------------------
# Content scanning


def code_text(text):
    """Script text without `@` comments and `.string` dialogue."""
    out = []
    for line in text.split("\n"):
        if line.lstrip().startswith((".string", ".braille")):
            continue
        out.append(re.sub(r"@.*$", "", line))
    return "\n".join(out)


def is_cut_map(name):
    return name.startswith("SSAqua") or "TrainerHill" in name


def label_bodies(text):
    """Label -> (body text) for one scripts file."""
    bodies, order = {}, []
    cur = None
    for line in text.split("\n"):
        m = LABEL_RE.match(line)
        if m:
            cur = m.group(1)
            bodies[cur] = []
            order.append(cur)
        if cur:
            bodies[cur].append(line)
    return {k: "\n".join(v) for k, v in bodies.items()}


def hns_shared_scripts(hns, manifest, tlabels):
    """(source -> text) for HnS shared script labels Johto reaches that the target lacks."""
    files = {}
    for path in glob.glob(os.path.join(hns, "data/scripts/*.inc")):
        files[os.path.relpath(path, hns)] = label_bodies(code_text(read(path)))
    by_label = {}
    for f, bodies in files.items():
        for lab in bodies:
            by_label.setdefault(lab, f)
    roots = [l for l in manifest["missing"]["ext_labels"] if l in by_label]
    seen, stack = set(), list(roots)
    while stack:
        lab = stack.pop()
        if lab in seen or lab not in by_label:
            continue
        seen.add(lab)
        for ref in re.findall(r"\b\w+\b", files[by_label[lab]][lab]):
            if ref in by_label and ref not in seen and ref not in tlabels:
                stack.append(ref)
    out = defaultdict(list)
    for lab in sorted(seen):
        out[by_label[lab]].append(files[by_label[lab]][lab])
    return {f"{f}#labels": "\n".join(v) for f, v in out.items()}


def c_function_body(text, name):
    m = re.search(r"^[A-Za-z_][\w\s\*]*\b" + re.escape(name) + r"\s*\([^;{]*\)\s*\n?\{", text, re.M)
    if not m:
        return ""
    i = text.index("{", m.start())
    depth = 0
    for j in range(i, len(text)):
        if text[j] == "{":
            depth += 1
        elif text[j] == "}":
            depth -= 1
            if depth == 0:
                return text[m.start():j + 1]
    return ""


def hns_feature_c(hns, manifest):
    """(source -> text): whole feature files plus the bodies of the missing specials."""
    out = {}
    for f in ("src/bug_contest.c", "src/sliding_puzzle.c"):
        path = os.path.join(hns, f)
        if os.path.exists(path):
            out[f] = strip_c_comments(read(path))
    wanted = {s.replace("Special_", "") for s in manifest["missing"]["specials"]}
    wanted |= set(manifest["missing"]["specials"])
    srcs = {os.path.relpath(p, hns): strip_c_comments(read(p))
            for p in glob.glob(os.path.join(hns, "src/*.c"))}
    skip = ("voltorb_flip", "mom_savings", "pokenav", "roamer", "surfable", "help_window")
    for name in sorted(wanted):
        for f, text in srcs.items():
            if any(s in f for s in skip) or f in out:
                continue
            body = c_function_body(text, name)
            if body:
                out.setdefault(f"{f}#{name}", body)
                break
    return out


def scan_sources(hns, manifest, jset):
    """Returns (uses: name -> set(source), sources: list of (source, kind, cut))."""
    uses = defaultdict(set)
    sources = []

    def add(source, text, kind, cut=False):
        sources.append((source, kind, cut))
        for tok in set(TOKEN_RE.findall(text)):
            uses[tok].add(source)

    for _, name, data in jset:
        cut = is_cut_map(name)
        # The Johto HoF re-spawn block is cut (D23); its uses never make a name live.
        kind = "init" if name == "PokemonLeague_HallOfFame_hns" else "inc"
        add(f"data/maps/{name}/scripts.inc", code_text(map_scripts(hns, name)), kind, cut)
        add(f"data/maps/{name}/map.json", json.dumps(data), "json", cut)
    # Saffron station map: imported for the Magnet Train (F27) although HnS tags it Kanto.
    st = "SaffronCity_TrainStation_hns"
    sp = os.path.join(hns, "data/maps", st, "scripts.inc")
    if os.path.exists(sp):
        add(f"data/maps/{st}/scripts.inc", code_text(read(sp)), "inc")
        add(f"data/maps/{st}/map.json", read(os.path.join(hns, "data/maps", st, "map.json")), "json")

    tlabels = target_labels()
    for src, text in hns_shared_scripts(hns, manifest, tlabels).items():
        add(src, text, "shared")
    for f in ("data/scripts/bug_contest.inc", "data/scripts/set_gym_trainers.inc"):
        add(f, code_text(read(os.path.join(hns, f))), "shared")

    # EventScript_ResetAllMapFlagsHnS: init only; never makes a name live on its own.
    ng = read(os.path.join(hns, "data/scripts/new_game.inc"))
    m = re.search(r"^EventScript_ResetAllMapFlagsHnS::.*?(?=^EventScript_\w+::)", ng, re.M | re.S)
    add("data/scripts/new_game.inc#ResetAllMapFlagsHnS", code_text(m.group(0)) if m else "", "init")

    for src, text in hns_feature_c(hns, manifest).items():
        add(src, text, "c")
    return uses, sources


def target_c_uses(names):
    """Target C/header files naming any of `names` (excluding the constants headers)."""
    skip = re.compile(r"include/constants/(flags|vars)(_\w+)?\.h$")
    uses = defaultdict(set)
    paths = glob.glob(os.path.join(ROOT, "src/**/*.[ch]"), recursive=True)
    paths += glob.glob(os.path.join(ROOT, "include/**/*.h"), recursive=True)
    for path in sorted(paths):
        r = os.path.relpath(path, ROOT).replace(os.sep, "/")
        if skip.search(r):
            continue
        with open(path, encoding="utf-8", errors="replace") as f:
            text = strip_c_comments(f.read())
        for tok in set(TOKEN_RE.findall(text)):
            if tok in names:
                uses[tok].add(r)
    return uses


def write_sites(jset, hns):
    """name -> list of 'Map:line cmd' for flag/var writes in live Johto scripts."""
    out = defaultdict(list)
    for _, name, _ in jset:
        if is_cut_map(name):
            continue
        for n, line in enumerate(map_scripts(hns, name).split("\n"), 1):
            m = WRITE_RE.match(line)
            if m:
                out[m.group(2)].append(f"{name.replace('_hns', '')}:{n} {m.group(1)}")
    return out


# ---------------------------------------------------------------------------
# Classification


class Row:
    def __init__(self, name):
        self.name = name
        self.kind = "flag" if name.startswith("FLAG_") else "var"
        self.hns = None
        self.target = None
        self.status = ""
        self.uses = set()
        self.c_uses = set()
        self.bucket = ""
        self.final_name = ""
        self.final_id = None
        self.block = ""
        self.note = ""
        self.writes = []


def add_note(row, text):
    if text in row.note:
        return
    row.note = row.note + "; " + text if row.note else text


def is_special_range(kind, value):
    if value is None:
        return False
    if kind == "flag":
        return 0 < value <= TEMP_FLAGS_END or value >= SPECIAL_FLAGS_START
    return value <= TEMP_VARS_END or value >= SPECIAL_VARS_START


def fragment_reason(name):
    return next((why for frag, why in DROP_FRAGMENTS if frag in name), None)


def classify(row, hns_world, tgt_world, kinds):
    name = row.name
    hdef = hns_world.defs.get(name)
    tdef = tgt_world.defs.get(name)
    row.hns = hns_world.value(name) if hdef else None
    row.target = tgt_world.value(name) if tdef else None
    if name in CONFIG_NAMES:
        row.status = "config"
    elif tdef:
        row.status = "target"
    elif hdef:
        row.status = "hns-only"
    else:
        row.status = "undefined"

    live_kinds = {kinds[s] for s in row.uses}
    live = bool(live_kinds - {"init"})
    value = row.target if row.status == "target" else row.hns
    scratch = bool(SCRATCH_RE.match(name))
    drop_pattern = next((why for pat, why in DROP_PATTERNS if re.search(pat, name)), None)

    if name in CONFIG_NAMES:
        row.bucket = "Config"
        add_note(row, CONFIG_NAMES[name])
    elif name in DROP_NAMES:
        row.bucket = "Dropped"
        add_note(row, DROP_NAMES[name])
    elif name in RENAMES:
        row.bucket = "Johto (renamed)"
        row.final_name = RENAMES[name][0]
        add_note(row, RENAMES[name][1])
    elif name in OWN_COPY:
        row.bucket = "Johto (renamed)" if OWN_COPY[name][0] != name else "Johto"
        row.final_name = OWN_COPY[name][0]
        add_note(row, OWN_COPY[name][1])
    elif name.startswith("FLAG_RECEIVED_HM_") and row.status == "target":
        row.bucket = "Johto (renamed)"
        row.final_name = "FLAG_JOHTO_RECEIVED_" + name[len("FLAG_RECEIVED_HM_"):]
        add_note(row, "D18: HM gift site becomes a TM site")
    elif name in KANTO_READS:
        row.bucket = "Shared"
        row.final_name = KANTO_READS[name][0]
        row.final_id = tgt_world.value(row.final_name)
        add_note(row, KANTO_READS[name][1] if "read only" in KANTO_READS[name][1] else KANTO_READS[name][1] + "; read only")
    elif name in SHARED_READS:
        row.bucket = "Shared"
        row.final_name = name
        row.final_id = row.target
        add_note(row, SHARED_READS[name])
    elif name in D6_SHARED:
        row.bucket = "Shared"
        row.final_name = name
        row.final_id = row.target
        add_note(row, "D6: encounter moved into Johto; the moved script owns this flag")
    elif is_special_range(row.kind, value) and row.status == "target":
        row.bucket = "Shared"
        row.final_name = name
        row.final_id = row.target
        add_note(row, "Scratch: Johto may write" if scratch else "Engine/special range")
    elif row.status == "hns-only" and row.kind == "var" and row.hns is not None and 0x4000 <= row.hns <= TEMP_VARS_END:
        row.bucket = "Shared"
        row.final_name = f"VAR_TEMP_{row.hns - 0x4000:X}"
        row.final_id = row.hns
        add_note(row, "HnS alias of a temp var; scratch, Johto may write")
    elif row.status == "hns-only" and row.kind == "flag" and row.hns is not None and 0 < row.hns <= TEMP_FLAGS_END:
        row.bucket = "Shared"
        row.final_name = f"FLAG_TEMP_{row.hns:X}"
        row.final_id = row.hns
        add_note(row, "HnS alias of a temp flag; scratch, Johto may write")
    elif drop_pattern or fragment_reason(name) or not live:
        row.bucket = "Dropped"
        if drop_pattern:
            add_note(row, drop_pattern)
        elif fragment_reason(name):
            add_note(row, fragment_reason(name))
        elif not live:
            add_note(row, "used only by cut maps, the HoF re-spawn block (D23) or the New Game reset"
                     if row.uses else "no live use")
    elif row.status == "target" and name.startswith(COLLISION_PREFIXES):
        row.bucket = "Johto (renamed)"
        row.final_name = name + "_JOHTO"
        add_note(row, "Target name for another region's location")
    elif row.status == "target":
        row.bucket = "Unresolved"
        add_note(row, "target name, no rule: Shared or renamed?")
    elif row.status == "undefined":
        row.bucket = "Unresolved"
        add_note(row, "defined in neither tree")
    else:
        row.bucket = "Johto"
        row.final_name = strip_hns(name)
        if row.final_name != name:
            add_note(row, "Locked 8: _HNS suffix dropped")
        if name.startswith("FLAG_DAILY_"):
            add_note(row, "Daily flag: target ClearDailyFlags clears only the SaveBlock1 range; Stage 6 needs a Johto reset")

    hint = NAME_NOTES.get(name)
    if not hint and row.bucket.startswith("Johto"):
        hint = next((n for frag, n in NOTE_RULES if frag in name), None)
    if hint:
        add_note(row, hint)


def flag_block(row):
    final = row.final_name
    if final.startswith("FLAG_JOHTO_BADGE"):
        return "badge"
    if final == "FLAG_JOHTO_CHAMPION":
        return "champion"
    if final.startswith("FLAG_VISITED_"):
        return "fly"
    if final.startswith("FLAG_HIDDEN_ITEM_"):
        return "hidden_item"
    if final.startswith("FLAG_ITEM_"):
        return "item_ball"
    if final.startswith("FLAG_HIDE_"):
        return "hide"
    if final.startswith(("FLAG_GOT_", "FLAG_RECEIVED_", "FLAG_JOHTO_RECEIVED_")):
        return "gift"
    return "story"


def assign_ids(rows):
    """Johto flags get sub-blocks (hidden items at 0x2000, the rest from 0x2200); vars pack from
    0x4200. Order inside a block follows the HnS value, so HnS range arithmetic stays valid."""
    johto = [r for r in rows if r.bucket.startswith("Johto")]
    by_final = {}
    for r in johto:
        by_final.setdefault(r.final_name, []).append(r)
    finals = []
    for final, group in by_final.items():
        vals = [r.hns for r in group if r.hns is not None]
        block = flag_block(group[0]) if group[0].kind == "flag" else "var"
        finals.append((final, block, min(vals) if vals else 1 << 20))

    layout = []
    next_id = JOHTO_FLAGS_START
    for block in FLAG_BLOCKS:
        members = sorted((f for f in finals if f[1] == block), key=lambda f: (f[2], f[0]))
        if block == "story":
            next_id = max(next_id, JOHTO_FLAGS_STORY_START)
        if not members:
            continue
        next_id = (next_id + 0xF) & ~0xF
        start = next_id
        for final, _, _ in members:
            for r in by_final[final]:
                r.final_id = next_id
                r.block = block
            next_id += 1
        layout.append(("flag", block, start, next_id - start))
    flag_end = next_id

    next_id = JOHTO_VARS_START
    members = sorted((f for f in finals if f[1] == "var"), key=lambda f: (f[2], f[0]))
    start = next_id
    for final, _, _ in members:
        for r in by_final[final]:
            r.final_id = next_id
            r.block = "var"
        next_id += 1
    layout.append(("var", "var", start, next_id - start))
    return layout, flag_end, next_id


# ---------------------------------------------------------------------------
# Output


def fmt_val(v):
    return "—" if v is None else f"0x{v:X}"


def short_source(s):
    m = re.match(r"data/maps/([^/]+)/(scripts\.inc|map\.json)$", s)
    if m:
        return f"{m.group(1).replace('_hns', '')}:{'inc' if m.group(2) == 'scripts.inc' else 'json'}"
    return s.replace("data/", "", 1) if s.startswith("data/") else s


def fmt_uses(sources, limit=6):
    items = sorted(short_source(s) for s in sources)
    if len(items) > limit:
        return ", ".join(items[:limit]) + f", +{len(items) - limit} more"
    return ", ".join(items)


def write_report(out_path, rows, layout, flag_end, var_end, sources, hns_head):
    story_slots = JOHTO_TRAINER_FLAGS_START - JOHTO_FLAGS_STORY_START
    var_slots = JOHTO_VARS_END + 1 - JOHTO_VARS_START
    lines = []
    w = lines.append
    w("# GS Johto Migration — Flag and Var Classification (Stage 5)")
    w("")
    w("Generated by `tools/gs_convert/classify_flags_vars.py`. Review gate for Stage 6.")
    w("")
    w("Edit only the **Bucket** and **Final name** columns. Stage 6 re-assigns every Johto id from the")
    w("edited buckets, so **Final id** here is a preview. Valid buckets: `Johto`, `Johto (renamed)`,")
    w("`Shared`, `Config`, `Dropped`. Two names with the same final name share one id.")
    w("")
    w("## Sources")
    w("")
    n_inc = sum(1 for s, k, _ in sources if k in ("inc", "init") and s.endswith("scripts.inc"))
    n_json = sum(1 for _, k, _ in sources if k == "json")
    n_shared = sum(1 for _, k, _ in sources if k == "shared")
    n_c = sum(1 for _, k, _ in sources if k == "c")
    n_cut = sum(1 for _, _, c in sources if c)
    w(f"HnS `{hns_head}`.")
    w("")
    w(f"* {n_inc} `scripts.inc` and {n_json} `map.json` files of the Johto set (257 maps) plus the Saffron station map.")
    w(f"* {n_shared} HnS shared script files (`bug_contest.inc`, `set_gym_trainers.inc`, and every `data/scripts` label")
    w("  a Johto script reaches that the target lacks, following `call`/`goto`).")
    w("* `EventScript_ResetAllMapFlagsHnS` (`data/scripts/new_game.inc`): counts as a use, but never makes a name live alone.")
    w(f"* {n_c} HnS C sources of approved features (F9, F10 files; bodies of the missing specials for F13-F15, F33, F34).")
    w("* Target C that reads a name: the **C refs** column (Stage 6 step 5 audit).")
    w(f"* {n_cut} of the sources belong to cut maps (S.S. Aqua, Trainer Hill hub); names used only there, or only by the")
    w("  New Game reset, are suggested **Dropped**.")
    w("")
    w("Column meaning: **HnS** = value in `flags_hns.h` / `vars_hns.h`; **Target** = value in the target (blank if the")
    w("name does not exist); **Status** = `target` (same name exists), `hns-only`, `config` (HnS option, no value),")
    w("`undefined`. Uses: `Map:inc` / `Map:json` for map files, `scripts/x.inc#labels` for shared scripts, `src/x.c#Fn` for")
    w("HnS C. **C refs** = target C/headers that name the HnS name or the final name.")
    w("")

    w("## Review items found while classifying")
    w("")
    w("* **Daily flags.** Johto has `FLAG_DAILY_*` flags (Silver rematch, Bug Contest, Haircut brothers, Flower Shop).")
    w("  `ClearDailyFlags` clears only the SaveBlock1 daily range, so Stage 6 must add a Johto daily reset.")
    w("* **Renamed Hoenn names** (`Johto (renamed)`): every Johto write goes to the Johto copy; the Hoenn name is never written.")
    w("* **Shared writes** are listed in the Note of each Shared row. Rows without \"allowed\" are read-only and Stage 13 deletes the lines.")
    w("* **D4 cuts** (bedroom, Pokégear intro, running shoes, clock) are Dropped; their Johto lines are deleted in Stage 13.")
    w("* **HoF re-spawn block** (D23): names only the Johto Hall of Fame touches are Dropped; `FLAG_IS_CHAMPION` there becomes `FLAG_JOHTO_CHAMPION`.")
    w("* **Reception Gate guards** (`FLAG_INDIGOJUNCTION_HIDE_*`) are kept as Johto flags for the D21 8-badge gate.")
    w("* **Radio Tower takeover** flags are kept (F7 drops only the radio itself).")
    w("")
    w("## Counts")
    w("")
    w("| Bucket | Flags | Vars | Total |")
    w("|---|---|---|---|")
    for b in BUCKET_ORDER:
        f = sum(1 for r in rows if r.bucket == b and r.kind == "flag")
        v = sum(1 for r in rows if r.bucket == b and r.kind == "var")
        if f or v:
            w(f"| {b} | {f} | {v} | {f + v} |")
    w(f"| **All** | {sum(1 for r in rows if r.kind == 'flag')} | {sum(1 for r in rows if r.kind == 'var')} | {len(rows)} |")
    w("")
    story_used = flag_end - JOHTO_FLAGS_STORY_START
    distinct_flags = len({r.final_name for r in rows if r.bucket.startswith("Johto") and r.kind == "flag"})
    distinct_vars = len({r.final_name for r in rows if r.bucket.startswith("Johto") and r.kind == "var"})
    hidden = [x for x in layout if x[1] == "hidden_item"]
    hidden_end = hidden[0][2] + hidden[0][3] - 1 if hidden else JOHTO_FLAGS_START
    w(f"Johto flags: {distinct_flags} distinct final names, ids 0x{JOHTO_FLAGS_START:X}-0x{flag_end - 1:X}. "
      f"Hidden items end at 0x{hidden_end:X} (ceiling 0x{JOHTO_HIDDEN_ITEMS_END:X}). Ids from 0x{JOHTO_FLAGS_STORY_START:X}: "
      f"{max(story_used, 0)} of {story_slots} story slots, including 0x10 alignment padding between blocks.")
    w(f"Johto vars: {distinct_vars} distinct final names, ids 0x{JOHTO_VARS_START:X}-0x{var_end - 1:X} "
      f"({var_end - JOHTO_VARS_START} of {var_slots}).")
    w("")
    if flag_end > JOHTO_TRAINER_FLAGS_START or var_end > JOHTO_VARS_END + 1 or hidden_end > JOHTO_HIDDEN_ITEMS_END:
        w("**OVER BUDGET.**")
        w("")

    w("## Johto id layout (preview)")
    w("")
    w("| Kind | Block | First id | Count |")
    w("|---|---|---|---|")
    for kind, block, start, count in layout:
        w(f"| {kind} | {block} | 0x{start:X} | {count} |")
    w("")

    block_order = {b: i for i, b in enumerate(FLAG_BLOCKS + ["var"])}

    def key(r):
        return (r.kind, block_order.get(r.block, 99), r.final_id if r.final_id is not None else 1 << 20,
                r.hns if r.hns is not None else 1 << 20, r.name)

    for b in BUCKET_ORDER:
        group = sorted((r for r in rows if r.bucket == b), key=key)
        if not group:
            continue
        w(f"## {b} ({len(group)})")
        w("")
        w("| Name | HnS | Target | Status | Bucket | Final name | Final id | Uses | C refs | Note |")
        w("|---|---|---|---|---|---|---|---|---|---|")
        for r in group:
            w(f"| {r.name} | {fmt_val(r.hns)} | {fmt_val(r.target)} | {r.status} | {r.bucket} | "
              f"{r.final_name} | {fmt_val(r.final_id)} | {fmt_uses(r.uses)} | {fmt_uses(r.c_uses)} | {r.note} |")
        w("")

    with open(out_path, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(lines))


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    add_hns_arg(ap)
    ap.add_argument("--out", required=True, help="Markdown output path")
    ap.add_argument("--manifest", default=os.path.join(OUT_DIR, "manifest.json"))
    args = ap.parse_args()
    hns = args.hns

    with open(args.manifest, encoding="utf-8") as f:
        manifest = json.load(f)
    head = manifest["hns_head"]

    hns_world = World(hns, {"IS_HNS": 1, "IS_FRLG": 0, "TESTING": 0})
    hns_world.load("constants/flags.h")
    hns_world.load("constants/vars.h")
    tgt_world = World(ROOT, {"IS_FRLG": 0, "TESTING": 0})
    tgt_world.load("constants/flags.h")
    tgt_world.load("constants/vars.h")

    jset = johto_set(hns)
    uses, sources = scan_sources(hns, manifest, jset)
    kinds = {s: k for s, k, _ in sources}
    cut = {s for s, _, c in sources if c}
    for name in uses:
        uses[name] = {s for s in uses[name]}

    rows = []
    for name in sorted(uses):
        r = Row(name)
        r.uses = {s for s in uses[name] if s not in cut} or set(uses[name])
        classify(r, hns_world, tgt_world, kinds)
        if all(s in cut for s in uses[name]) and r.bucket not in ("Config",):
            r.bucket, r.final_name, r.final_id = "Dropped", "", None
            add_note(r, "used only by cut maps")
        rows.append(r)

    # Writes and C refs
    sites = write_sites(jset, hns)
    lookup = {}
    for r in rows:
        for n in (r.name, r.final_name):
            if n:
                lookup.setdefault(n, []).append(r)
    cnames = set(lookup)
    cuses = target_c_uses(cnames)
    for n, files in cuses.items():
        for r in lookup[n]:
            r.c_uses |= files
    for r in rows:
        r.writes = sites.get(r.name, [])
        if r.bucket == "Shared" and r.name in WRITE_OK and r.writes:
            add_note(r, f"{len(r.writes)} Johto write(s) allowed")
        elif r.bucket == "Shared" and r.writes and "write" not in r.note.lower() and "D6" not in r.note:
            add_note(r, f"{len(r.writes)} Johto write(s): {', '.join(r.writes[:3])}; cut in Stage 13 unless approved")
        elif r.bucket == "Dropped" and r.writes:
            add_note(r, f"{len(r.writes)} write(s) to delete in Stage 13")
        if r.bucket == "Dropped" and any(u.endswith("map.json") for u in r.uses):
            add_note(r, "map.json event uses it: Stage 13 clears the field or removes the object")
        if r.bucket == "Johto (renamed)" and r.final_name.startswith(tuple(C_READER_NOTES)):
            for k, v in C_READER_NOTES.items():
                if r.final_name.startswith(k):
                    add_note(r, v)

    layout, flag_end, var_end = assign_ids(rows)
    os.makedirs(os.path.dirname(os.path.abspath(args.out)), exist_ok=True)
    write_report(args.out, rows, layout, flag_end, var_end, sources, head)

    counts = defaultdict(int)
    for r in rows:
        counts[(r.bucket, r.kind)] += 1
    for (b, k), n in sorted(counts.items()):
        print(f"{b:16} {k:4} {n}")
    print(f"Johto flag ids end 0x{flag_end:X}, var ids end 0x{var_end:X}")
    hidden = [x for x in layout if x[1] == "hidden_item"]
    if flag_end > JOHTO_TRAINER_FLAGS_START or var_end > JOHTO_VARS_END + 1 or \
            (hidden and hidden[0][2] + hidden[0][3] - 1 > JOHTO_HIDDEN_ITEMS_END):
        print("error: Johto ids exceed their range", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
