#!/usr/bin/env python3
"""Classify every flag/var name that FRLG content uses into Shared, Kanto or Dropped.

Writes a Markdown review table (name, Emerald/FRLG values, bucket, final name, final id, uses).
Standard library only. Run from the repo root:

    python3 tools/frlg_convert/classify_flags_vars.py --out "<path>/FRLG - Flag Classification.md"
"""

import argparse
import glob
import json
import os
import re
import sys
from collections import defaultdict

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))

TOKEN_RE = re.compile(r"\b(?:FLAG|VAR)_[A-Za-z0-9_]+\b")
IDENT_RE = re.compile(r"\b[A-Za-z_][A-Za-z0-9_]*\b")
DEFINE_RE = re.compile(r"^\s*#\s*define\s+([A-Za-z_][A-Za-z0-9_]*)(\([^)]*\))?\s*(.*)$")
INCLUDE_RE = re.compile(r'^\s*#\s*include\s+"([^"]+)"')

# Fallbacks for constants the Stage 4 headers define.
DEFAULT_RANGES = {
    "KANTO_FLAGS_START": 0x1000,
    "KANTO_TRAINER_FLAGS_START": 0x1400,
    "KANTO_VARS_START": 0x4100,
    "KANTO_VARS_END": 0x41FF,
}
TEMP_FLAGS_END = 0x1F
TEMP_VARS_END = 0x400F
SPECIAL_FLAGS_START = 0x4000
SPECIAL_VARS_START = 0x8000

# Kanto flag sub-blocks, in id order. Each starts on a 0x10 boundary.
FLAG_BLOCKS = ["story", "hide", "gift", "badge", "champion", "world_map", "item_ball", "hidden_item"]

# Real Emerald names that mean Kanto state in FRLG content.
RENAMES = {
    **{f"FLAG_BADGE0{n}_GET": f"FLAG_BADGE0{n}_GET_FRLG" for n in range(1, 9)},
    "FLAG_IS_CHAMPION": "FLAG_KANTO_CHAMPION",
    "FLAG_SYS_GAME_CLEAR": "FLAG_KANTO_CHAMPION",
}

# FRLG names that move onto an existing Emerald flag (final name differs).
SHARED_MAP = {
    "FLAG_SYS_ON_CYCLING_ROAD": "FLAG_SYS_CYCLING_ROAD",
    # Key items: one gift across both regions.
    "FLAG_GOT_OLD_ROD": "FLAG_RECEIVED_OLD_ROD",
    "FLAG_GOT_GOOD_ROD": "FLAG_RECEIVED_GOOD_ROD",
    "FLAG_GOT_SUPER_ROD": "FLAG_RECEIVED_SUPER_ROD",
    "FLAG_GOT_BICYCLE": "FLAG_RECEIVED_BIKE",
    "FLAG_GOT_COIN_CASE": "FLAG_RECEIVED_COIN_CASE",
}

# Shared by rule regardless of value (plan Stage 5 step 2).
SHARED_NAMES = {
    "FLAG_SYS_B_DASH",
    "FLAG_SYS_POKEDEX_GET",
    "FLAG_SYS_POKEMON_GET",
    "FLAG_SYS_PC_LANETTE",
    # Legendary hide flags move with the legendary (Locked decision 13).
    "FLAG_HIDE_ARTICUNO",
    "FLAG_HIDE_ZAPDOS",
    "FLAG_HIDE_MOLTRES",
    "FLAG_HIDE_MEWTWO",
}

# Emerald event-island names; Emerald's own maps stay canonical (O§8.4).
EVENT_ISLAND_NAMES = {"FLAG_HIDE_DEOXYS", "FLAG_HIDE_LUGIA", "FLAG_HIDE_HO_OH",
                      "FLAG_HIDE_BIRTH_ISLAND_METEORITE", "FLAG_DEOXYS_ROCK_COMPLETE"}

# Real Emerald names with these prefixes are per-location state; in FRLG content they mean a
# Kanto location, so they get the `_FRLG` suffix (O§5.2 naming).
COLLISION_PREFIXES = ("FLAG_HIDDEN_ITEM_", "FLAG_ITEM_", "FLAG_HIDE_", "FLAG_DEFEATED_",
                      "FLAG_WORLD_MAP_", "FLAG_VISITED_", "VAR_MAP_SCENE_")

# Name fragments of removed systems (O§8.1, O§8.4).
DROP_FRAGMENTS = ["FAME_CHECKER", "TEACHY_TV", "VS_SEEKER", "NAVEL_ROCK", "BIRTH_ISLAND"]

# Set only for legendaries of the dropped Navel Rock / Birth Island copies.
DROP_NAMES = {"FLAG_LUGIA_FLEW_AWAY", "FLAG_HO_OH_FLEW_AWAY", "FLAG_DEOXYS_FLEW_AWAY"}

# Data files that belong only to removed systems or dropped maps.
DROPPED_DATA_FILES = {
    "data/scripts/fame_checker_frlg.inc",
    "data/text/fame_checker_frlg.inc",
    "data/scripts/cable_club_frlg.inc",
    "data/text/new_game_intro_frlg.inc",
}
DROPPED_MAP_GROUPS = {"gMapGroup_Link_Frlg"}
DROPPED_MAP_PREFIXES = ("NavelRock_", "BirthIsland_")

# FRLG-only C files of removed systems (O§8.1).
DROPPED_C_FILES = {
    "src/battle_controller_oak_old_man.c",
    "src/credits_frlg.c",
    "src/fame_checker.c",
    "src/hall_of_fame_frlg.c",
    "src/intro_frlg.c",
    "src/oak_speech.c",
    "src/title_screen_frlg.c",
    "src/vs_seeker.c",
}

# Review hints for single names.
NAME_NOTES = {
    "FLAG_PENDING_DAYCARE_EGG": "Engine day-care egg flag; a kept Four Island Day Care (D9) needs its own",
    "FLAG_SHOWN_AURORA_TICKET": "Vermilion pier ship to Birth Island (Stage 15)",
    "FLAG_SHOWN_MYSTIC_TICKET": "Vermilion pier ship to Navel Rock (Stage 15)",
    "FLAG_ENABLE_SHIP_BIRTH_ISLAND": "Vermilion pier ship to Birth Island (Stage 15)",
    "FLAG_ENABLE_SHIP_NAVEL_ROCK": "Vermilion pier ship to Navel Rock (Stage 15)",
    "FLAG_SYS_ON_CYCLING_ROAD": "Same engine meaning as the Emerald flag",
    "FLAG_OPENED_START_MENU": "Oak intro tutorial; set by Emerald C today (Stage 6 audit)",
    "FLAG_SYS_GOT_BERRY_POUCH": "No Berry Pouch in Emerald; drop candidate",
    "FLAG_SYS_SET_TRAINER_CARD_PROFILE": "mystery_event_club.inc (Stage 7)",
    "FLAG_MET_STICKER_MAN": "trainer_card_frlg.inc (Stage 7)",
    "FLAG_SYS_PC_STORAGE_DISABLED": "Sevii detour; no C reader",
    "FLAG_GOT_POWDER_JAR": "Berry Powder (Stage 7)",
    "VAR_STARTER_MON": "D2: Blue starter branches removed; FRLG reads go away",
    "VAR_MAP_SCENE_POKEMON_CENTER_TEALA": "Link-room attendant (cable_club_frlg.inc)",
    "VAR_ELEVATOR_FLOOR": "O§8.3: Silph / Rocket / Celadon / Trainer Tower elevators",
    "FLAG_TUTOR_FRENZY_PLANT": "Cape Brink starter tutor",
    "FLAG_TUTOR_BLAST_BURN": "Cape Brink starter tutor",
    "FLAG_TUTOR_HYDRO_CANNON": "Cape Brink starter tutor",
    "FLAG_LEARNED_ALL_MOVES_AT_CAPE_BRINK": "Cape Brink starter tutor",
    "FLAG_FOUGHT_ARTICUNO": "Stage 22: FRLG plain battle replaced; drop candidate",
    "FLAG_FOUGHT_ZAPDOS": "Stage 22: FRLG plain battle replaced; drop candidate",
    "FLAG_FOUGHT_MOLTRES": "Stage 22: FRLG plain battle replaced; drop candidate",
    "FLAG_FOUGHT_MEWTWO": "Stage 22: FRLG plain battle replaced; drop candidate",
    "FLAG_GOT_SS_TICKET_DUP": "Duplicate S.S. Ticket guard (D4)",
}

# Review hints by name fragment. First match wins.
NOTE_RULES = [
    ("BULBASAUR_BALL", "O§7.2: no starter choice in Kanto"),
    ("SQUIRTLE_BALL", "O§7.2: no starter choice in Kanto"),
    ("CHARMANDER_BALL", "O§7.2: no starter choice in Kanto"),
    ("OAKS_AIDE", "Oak's aide dex-count gift"),
    ("OAK", "O§7.2: Oak story beat (parcel / Pokédex / intro)"),
    ("PARCEL", "O§7.2: Oak's Parcel removed"),
    ("POKEDEX", "O§7.2: Pokédex gate removed"),
    ("OLD_MAN", "O§7.2: Old Man catching demo removed"),
    ("GOT_HM", "O§7.2 / D7: HM gift"),
    ("RIVAL", "D2: Blue fight state; check order safety"),
    ("TRAINER_TOWER", "O§8.3: Trainer Tower (Stage 7)"),
    ("FAN_CLUB", "O§8.3: Trainer Fan Club (Stage 7)"),
    ("DAY_CARE", "O§8.3: Four Island Day Care (Stage 7)"),
    ("DAYCARE", "O§8.3: Four Island Day Care (Stage 7)"),
    ("GAME_CORNER", "O§8.3: Game Corner (Stage 7)"),
    ("GAMBLER", "O§8.3: Game Corner (Stage 7)"),
    ("TANOBY", "O§8.3: Tanoby Ruins (Stage 7)"),
    ("LOST_CAVE", "O§8.3: Lost Cave (Stage 7)"),
    ("BERRY_FOREST", "O§8.3: Berry Forest (Stage 7)"),
    ("LOSTELLE", "O§8.3: Berry Forest (Stage 7)"),
    ("DOTTED_HOLE", "O§8.3: Dotted Hole (Stage 7)"),
    ("SS_ANNE", "D4: S.S. Anne stays docked"),
    ("S_S_ANNE", "D4: S.S. Anne stays docked"),
    ("RUBY", "O§8.3: Celio / Ruby / Sapphire quest (Stage 7)"),
    ("SAPPHIRE", "O§8.3: Celio / Ruby / Sapphire quest (Stage 7)"),
    ("CELIO", "O§8.3: Celio / Ruby / Sapphire quest (Stage 7)"),
    ("TRAINER_CARD", "FRLG Trainer Card (Stage 7 / D10)"),
    ("BRAG", "Link / Trainer Card brag (trainer_card_frlg.inc)"),
    ("SIZE_RECORD", "pokemon_size_record.c (Stage 7)"),
    ("RESORT_GOR", "Resort Gorgeous (Stage 7)"),
    ("MASSAGE", "Daisy's massage (field_specials.c)"),
    ("TUTOR_", "O§8.2: FRLG move tutors"),
    ("MYSTERY", "Mystery Gift / event content"),
    ("WONDER", "Mystery Gift / event content"),
]


# ---------------------------------------------------------------------------
# Header parsing


class Define:
    def __init__(self, name, expr, path, line):
        self.name = name
        self.expr = expr
        self.path = path
        self.line = line


def strip_c_comments(text):
    text = re.sub(r"/\*.*?\*/", lambda m: "\n" * m.group(0).count("\n"), text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


class World:
    """Macro table for one build configuration (IS_FRLG 0 or 1)."""

    def __init__(self, is_frlg):
        self.defs = {"IS_FRLG": Define("IS_FRLG", str(int(is_frlg)), "<cmdline>", 0), "TESTING": Define("TESTING", "0", "<cmdline>", 0)}
        self.cache = {}
        self.seen = set()

    def load(self, rel):
        path = os.path.join(ROOT, "include", rel)
        if rel in self.seen or not os.path.exists(path):
            return
        self.seen.add(rel)
        with open(path, encoding="utf-8") as f:
            lines = strip_c_comments(f.read()).split("\n")
        stack = []  # (active, taken)
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


def rel(path):
    return os.path.relpath(path, ROOT).replace(os.sep, "/")


def frlg_script_includes():
    """Includes inside the `.if IS_FRLG` block of data/event_scripts.s."""
    out = []
    inside = False
    with open(os.path.join(ROOT, "data/event_scripts.s"), encoding="utf-8") as f:
        for line in f:
            s = line.strip()
            if s.startswith(".if IS_FRLG"):
                inside = True
            elif inside and s.startswith(".endif"):
                break
            elif inside:
                m = re.match(r'\.include\s+"([^"]+)"', s)
                if m:
                    out.append(m.group(1))
    return out


def label_body(path, label):
    """Lines from `label::` up to the next global label."""
    out = []
    inside = False
    with open(os.path.join(ROOT, path), encoding="utf-8") as f:
        for line in f:
            if re.match(r"^\w+::", line):
                if inside:
                    break
                inside = line.startswith(label + "::")
            if inside:
                out.append(line)
    return "".join(out)


def frlg_branches(path):
    """Text inside `#if IS_FRLG` / `.if IS_FRLG` branches (up to #else / #endif) of a shared file."""
    out = []
    depth = 0
    with open(os.path.join(ROOT, path), encoding="utf-8") as f:
        for line in f:
            s = line.strip()
            if re.match(r"^[#.]if\s+IS_FRLG\b", s):
                depth = 1
                continue
            if depth and re.match(r"^[#.](else|endif)\b", s):
                depth = 0
                continue
            if depth:
                out.append(line)
    return "".join(out)


def strip_asm_comments(text):
    out = []
    for line in text.split("\n"):
        if line.lstrip().startswith(".string"):
            continue
        out.append(re.sub(r"(@|//).*$", "", line))
    return "\n".join(out)


def load_map_groups():
    with open(os.path.join(ROOT, "data/maps/map_groups.json"), encoding="utf-8") as f:
        groups = json.load(f)
    map_to_group = {}
    for g in groups["group_order"]:
        for m in groups[g]:
            map_to_group[m] = g
    return map_to_group


def is_dropped_map(name, map_to_group):
    return map_to_group.get(name) in DROPPED_MAP_GROUPS or name.startswith(DROPPED_MAP_PREFIXES)


def scan_data(map_to_group):
    """Returns (uses: name -> set(source), sources: list of (source, dropped))."""
    uses = defaultdict(set)
    sources = []

    def add(source, text, dropped):
        sources.append((source, dropped))
        for tok in set(TOKEN_RE.findall(text)):
            uses[tok].add(source)

    for inc in frlg_script_includes():
        path = os.path.join(ROOT, inc)
        with open(path, encoding="utf-8") as f:
            text = strip_asm_comments(f.read())
        parts = inc.split("/")
        if parts[1] == "maps":
            dropped = is_dropped_map(parts[2], map_to_group)
        else:
            dropped = inc in DROPPED_DATA_FILES
        add(inc, text, dropped)

    with open(os.path.join(ROOT, "data/scripts/hall_of_fame_frlg.inc"), encoding="utf-8") as f:
        add("data/scripts/hall_of_fame_frlg.inc", strip_asm_comments(f.read()), False)

    add("data/scripts/new_game.inc#EventScript_ResetAllMapFlagsFrlg",
        strip_asm_comments(label_body("data/scripts/new_game.inc", "EventScript_ResetAllMapFlagsFrlg")), False)

    for path in sorted(glob.glob(os.path.join(ROOT, "data/maps/*_Frlg/map.json"))):
        name = os.path.basename(os.path.dirname(path))
        with open(path, encoding="utf-8") as f:
            add(rel(path), f.read(), is_dropped_map(name, map_to_group))

    # FRLG branches inside shared data files.
    shared = []
    for pattern in ("data/**/*.inc", "data/**/*.s"):
        shared += glob.glob(os.path.join(ROOT, pattern), recursive=True)
    frlg_set = {s for s, _ in sources}
    for path in sorted(set(shared)):
        r = rel(path)
        if r in frlg_set or "/maps/" in r:
            continue
        with open(path, encoding="utf-8") as f:
            if "IS_FRLG" not in f.read():
                continue
        text = frlg_branches(r)
        if text:
            add(r + "#IS_FRLG", strip_asm_comments(text), False)
    return uses, sources


def scan_c(names):
    """C/header files under src/ and include/ that name any of `names`."""
    skip = {"include/constants/flags.h", "include/constants/flags_frlg.h",
            "include/constants/vars.h", "include/constants/vars_frlg.h"}
    uses = defaultdict(set)
    paths = glob.glob(os.path.join(ROOT, "src/**/*.[ch]"), recursive=True)
    paths += glob.glob(os.path.join(ROOT, "include/**/*.h"), recursive=True)
    for path in sorted(paths):
        r = rel(path)
        if r in skip:
            continue
        with open(path, encoding="utf-8", errors="replace") as f:
            text = strip_c_comments(f.read())
        for tok in set(TOKEN_RE.findall(text)):
            if tok in names:
                uses[tok].add(r)
    return uses


# ---------------------------------------------------------------------------
# Classification


class Row:
    def __init__(self, name):
        self.name = name
        self.kind = "flag" if name.startswith("FLAG_") else "var"
        self.emerald = None      # value in the Emerald build, None if undefined
        self.frlg = None         # value in the FRLG build, None if undefined
        self.status = ""         # real / stub / frlg-only / emerald-only / undefined
        self.data_uses = set()
        self.c_uses = set()
        self.bucket = ""
        self.final_name = ""
        self.final_id = None
        self.block = ""
        self.note = ""


def emerald_stub_line(path):
    """First line of the Emerald build's FRLG stub block in flags.h."""
    with open(os.path.join(ROOT, path), encoding="utf-8") as f:
        for i, line in enumerate(f, 1):
            if line.strip() == "// FRLG flags":
                return i
    return 10 ** 9


def classify(row, emerald_world, frlg_world, stub_start, dropped_sources):
    name = row.name
    edef = emerald_world.defs.get(name)
    row.emerald = emerald_world.value(name) if edef else None
    row.frlg = frlg_world.value(name) if name in frlg_world.defs else None

    if row.kind == "flag":
        if edef and edef.path == "include/constants/flags.h" and edef.line >= stub_start and row.emerald == 0:
            row.status = "stub"
        elif edef:
            row.status = "real" if row.frlg is not None else "emerald-only"
        elif row.frlg is not None:
            row.status = "frlg-only"
        else:
            row.status = "undefined"
    else:
        in_emerald_header = edef and edef.path != "include/constants/vars_frlg.h"
        if in_emerald_header:
            row.status = "real" if row.frlg is not None else "emerald-only"
        elif row.frlg is not None:
            row.status = "frlg-only"
        else:
            row.status = "undefined"

    row.note = NAME_NOTES.get(name, "")
    if not row.note:
        for frag, note in NOTE_RULES:
            if frag in name:
                row.note = note
                break

    if row.status == "undefined":
        row.bucket = "Unresolved"
        return

    data_live = {s for s in row.data_uses if s not in dropped_sources}
    c_live = {s for s in row.c_uses if s not in DROPPED_C_FILES}
    live = bool(data_live or c_live)
    emerald_name = row.status in ("real", "emerald-only")
    value = row.emerald if emerald_name else row.frlg

    if name in RENAMES:
        row.bucket = "Kanto (renamed)"
        row.final_name = RENAMES[name]
    elif name in SHARED_MAP:
        row.bucket = "Shared"
        row.final_name = SHARED_MAP[name]
        row.final_id = emerald_world.value(SHARED_MAP[name])
    elif name in SHARED_NAMES or is_shared_range(row.kind, value):
        row.bucket = "Shared"
    elif name in EVENT_ISLAND_NAMES:
        row.bucket = "Shared"
        add_note(row, "Emerald event island; drop the FRLG uses")
    elif emerald_name and live and name.startswith(COLLISION_PREFIXES):
        row.bucket = "Kanto (renamed)"
        row.final_name = name + "_FRLG"
        add_note(row, "Emerald name for a Hoenn location")
    elif emerald_name:
        row.bucket = "Shared"
        if not live:
            add_note(row, "used only by dropped FRLG maps")
    elif name in DROP_NAMES or any(frag in name for frag in DROP_FRAGMENTS) or not live:
        row.bucket = "Dropped"
    else:
        row.bucket = "Kanto"

    if row.bucket == "Shared" and not row.final_name:
        row.final_name = name
        row.final_id = row.emerald
    elif row.bucket == "Kanto":
        row.final_name = name


def add_note(row, text):
    row.note = row.note + "; " + text if row.note else text


def is_shared_range(kind, value):
    if value is None:
        return False
    if kind == "flag":
        return 0 < value <= TEMP_FLAGS_END or value >= SPECIAL_FLAGS_START
    return value <= TEMP_VARS_END or value >= SPECIAL_VARS_START


def frlg_flag_sections():
    """FRLG flag name -> section comment it is defined under in flags_frlg.h."""
    sections = {}
    current = ""
    with open(os.path.join(ROOT, "include/constants/flags_frlg.h"), encoding="utf-8") as f:
        for line in f:
            if line.startswith("// "):
                current = line[3:].strip()
            m = DEFINE_RE.match(line)
            if m and m.group(1) not in sections:
                sections[m.group(1)] = current
    return sections


def flag_block(row, sections):
    final = row.final_name
    if final.startswith("FLAG_BADGE"):
        return "badge"
    if final == "FLAG_KANTO_CHAMPION":
        return "champion"
    if final.startswith("FLAG_WORLD_MAP_"):
        return "world_map"
    if final.startswith("FLAG_HIDDEN_ITEM_"):
        return "hidden_item"
    if final.startswith("FLAG_ITEM_") or sections.get(row.name) == "Item ball hide/show":
        return "item_ball"
    if final.startswith("FLAG_HIDE_"):
        return "hide"
    if final.startswith(("FLAG_GOT_", "FLAG_RECEIVED_")):
        return "gift"
    return "story"


def assign_ids(rows, ranges, sections):
    """Kanto flags get sub-blocks from KANTO_FLAGS_START; Kanto vars pack from KANTO_VARS_START.
    Order inside a block follows the FRLG value, so FRLG range arithmetic stays valid."""
    kanto = [r for r in rows if r.bucket.startswith("Kanto")]
    by_final = {}
    for r in kanto:
        by_final.setdefault(r.final_name, []).append(r)

    finals = []
    for final, group in by_final.items():
        frlg_vals = [r.frlg for r in group if r.frlg is not None]
        block = flag_block(group[0], sections) if group[0].kind == "flag" else "var"
        finals.append((final, block, min(frlg_vals) if frlg_vals else 1 << 20))

    layout = []
    next_id = ranges["KANTO_FLAGS_START"]
    for block in FLAG_BLOCKS:
        members = sorted((f for f in finals if f[1] == block), key=lambda f: (f[2], f[0]))
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

    next_id = ranges["KANTO_VARS_START"]
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
        return f"{m.group(1)}:{'inc' if m.group(2) == 'scripts.inc' else 'json'}"
    return s.replace("data/", "", 1) if s.startswith("data/") else s


def fmt_uses(sources, limit=6):
    items = sorted(short_source(s) for s in sources)
    if len(items) > limit:
        return ", ".join(items[:limit]) + f", +{len(items) - limit} more"
    return ", ".join(items)


BUCKET_ORDER = ["Kanto (renamed)", "Kanto", "Shared", "Dropped", "Unresolved"]
BLOCK_ORDER = {b: i for i, b in enumerate(FLAG_BLOCKS + ["var"])}


def write_report(out_path, rows, layout, flag_end, var_end, ranges, sources, c_count):
    kanto_story_slots = ranges["KANTO_TRAINER_FLAGS_START"] - ranges["KANTO_FLAGS_START"]
    var_slots = ranges["KANTO_VARS_END"] + 1 - ranges["KANTO_VARS_START"]
    lines = []
    w = lines.append
    w("# FRLG Kanto Merge — Flag and Var Classification (Stage 5)")
    w("")
    w("Generated by `tools/frlg_convert/classify_flags_vars.py`. Review gate for Stage 6.")
    w("")
    w("Edit only the **Bucket** and **Final name** columns. Stage 6 re-assigns every Kanto id from the")
    w("edited buckets, so **Final id** here is a preview. Valid buckets: `Kanto`, `Kanto (renamed)`,")
    w("`Shared`, `Dropped`. Two names with the same final name share one id.")
    w("")
    w("## Sources")
    w("")
    n_inc = sum(1 for s, _ in sources if s.endswith(".inc") and "#" not in s)
    n_json = sum(1 for s, _ in sources if s.endswith("map.json"))
    n_branch = sum(1 for s, _ in sources if s.endswith("#IS_FRLG"))
    n_drop = sum(1 for _, d in sources if d)
    w(f"* {n_inc} script/text includes (the `.if IS_FRLG` block of `data/event_scripts.s` plus "
      "`data/scripts/hall_of_fame_frlg.inc`).")
    w("* `EventScript_ResetAllMapFlagsFrlg` in `data/scripts/new_game.inc`.")
    w(f"* {n_json} `data/maps/*_Frlg/map.json` files.")
    w(f"* {n_branch} shared data files with `IS_FRLG` branches (FRLG branch only).")
    w(f"* {c_count} C files under `src/` / `include/` that name a stub flag, an FRLG-only flag or an "
      "FRLG-only var.")
    w(f"* {n_drop} of the data sources belong to dropped maps or removed systems; names used only "
      "there are suggested **Dropped**.")
    w("")
    w("Column meaning: **Emerald** = value in today's Emerald build (`0x0` with status `stub` is the")
    w("`flags.h` stub block); **FRLG** = value in the FRLG build; **Status** = `real` (Emerald name),")
    w("`stub`, `frlg-only`, `emerald-only`. Use lists: `Map:inc` / `Map:json` for map files, `#IS_FRLG`")
    w("for an FRLG branch in a shared file.")
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
    used_flags = flag_end - ranges["KANTO_FLAGS_START"]
    used_vars = var_end - ranges["KANTO_VARS_START"]
    distinct_flags = len({r.final_name for r in rows if r.bucket.startswith("Kanto") and r.kind == "flag"})
    distinct_vars = len({r.final_name for r in rows if r.bucket.startswith("Kanto") and r.kind == "var"})
    w(f"Kanto flags: {distinct_flags} distinct final names, ids "
      f"0x{ranges['KANTO_FLAGS_START']:X}-0x{flag_end - 1:X} ({used_flags} of {kanto_story_slots} story slots, "
      "including 0x10 alignment padding between blocks).")
    w(f"Kanto vars: {distinct_vars} distinct final names, ids 0x{ranges['KANTO_VARS_START']:X}-0x{var_end - 1:X} "
      f"({used_vars} of {var_slots}).")
    w("")
    if used_flags > kanto_story_slots or used_vars > var_slots:
        w("**OVER BUDGET.**")
        w("")

    w("## Kanto id layout (preview)")
    w("")
    w("| Kind | Block | First id | Count |")
    w("|---|---|---|---|")
    for kind, block, start, count in layout:
        w(f"| {kind} | {block} | 0x{start:X} | {count} |")
    w("")

    def key(r):
        return (r.kind, BLOCK_ORDER.get(r.block, 99), r.final_id if r.final_id is not None else 1 << 20,
                r.frlg if r.frlg is not None else 1 << 20, r.name)

    for b in BUCKET_ORDER:
        group = sorted((r for r in rows if r.bucket == b), key=key)
        if not group:
            continue
        w(f"## {b} ({len(group)})")
        w("")
        w("| Name | Emerald | FRLG | Status | Bucket | Final name | Final id | Uses | Note |")
        w("|---|---|---|---|---|---|---|---|---|")
        for r in group:
            uses = fmt_uses(r.data_uses | r.c_uses)
            w(f"| {r.name} | {fmt_val(r.emerald)} | {fmt_val(r.frlg)} | {r.status} | {r.bucket} | "
              f"{r.final_name} | {fmt_val(r.final_id)} | {uses} | {r.note} |")
        w("")

    with open(out_path, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(lines))


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--out", required=True, help="Markdown output path")
    args = ap.parse_args()

    emerald = World(False)
    emerald.load("constants/flags.h")
    emerald.load("constants/vars.h")
    frlg = World(True)
    frlg.load("constants/flags.h")
    frlg.load("constants/vars.h")

    ranges = {k: emerald.value(k) if emerald.value(k) is not None else v for k, v in DEFAULT_RANGES.items()}

    map_to_group = load_map_groups()
    data_uses, sources = scan_data(map_to_group)
    dropped_sources = {s for s, d in sources if d}

    stub_start = emerald_stub_line("include/constants/flags.h")
    stub_flags = {n for n, d in emerald.defs.items()
                  if n.startswith("FLAG_") and d.path == "include/constants/flags.h" and d.line >= stub_start}
    frlg_only_flags = {n for n in frlg.defs if n.startswith("FLAG_") and n not in emerald.defs}
    frlg_only_vars = {n for n, d in emerald.defs.items()
                      if n.startswith("VAR_") and d.path == "include/constants/vars_frlg.h"}
    c_names = stub_flags | frlg_only_flags | frlg_only_vars
    c_uses = scan_c(c_names)

    names = set(data_uses) | set(c_uses)
    rows = []
    for name in sorted(names):
        r = Row(name)
        r.data_uses = data_uses.get(name, set())
        r.c_uses = c_uses.get(name, set())
        classify(r, emerald, frlg, stub_start, dropped_sources)
        rows.append(r)

    layout, flag_end, var_end = assign_ids(rows, ranges, frlg_flag_sections())
    c_files = set().union(*(r.c_uses for r in rows)) if rows else set()
    write_report(args.out, rows, layout, flag_end, var_end, ranges, sources, len(c_files))

    counts = defaultdict(int)
    for r in rows:
        counts[(r.bucket, r.kind)] += 1
    for (b, k), n in sorted(counts.items()):
        print(f"{b:16} {k:4} {n}")
    print(f"Kanto flag ids end 0x{flag_end:X}, var ids end 0x{var_end:X}")
    if flag_end > ranges["KANTO_TRAINER_FLAGS_START"] or var_end > ranges["KANTO_VARS_END"] + 1:
        print("error: Kanto ids exceed their range", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
