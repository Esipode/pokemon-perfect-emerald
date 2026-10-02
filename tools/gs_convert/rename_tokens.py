#!/usr/bin/env python3
"""Rewrite HnS Johto script text and map.json strings with the Stage 6 rename table (Stage 13).

Library use (import_maps.py): `Renamer(hns).script(text, map_name)` and `.token(tok)`.
CLI: `python3 tools/gs_convert/rename_tokens.py <scripts.inc> [--map <HnS map folder>]` prints the result.

Rules, applied per instruction line:
  * rename: every token with a `rename` entry is replaced (flags, vars, items, maps, trainers, ...).
  * delete line: a line that uses a Dropped token is removed. A conditional on a Dropped flag or var
    is resolved with the value "unset / 0": `*_if_set` lines go, `*_if_unset` lines become plain
    goto/call, var compares are evaluated against 0. Config flags use the same rule, except the
    `Constant TRUE` ones.
  * Locked 14: a write to a Shared flag/var (not scratch, not `write_ok`) is cut and counted; a
    FLAG_/VAR_ token that is in no table is an error.
  * TM/HM items: tokens at a §0.9 site block become the site's `ITEM_TM_<MOVE>` alias; any other
    `ITEM_TM*`/`ITEM_HM*` token is reported.
Every non-trivial decision is recorded in `Renamer.report` for review.
"""

import argparse
import json
import operator
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from check_refs import MACROS_REMOVED, NATIVES_REMOVED, SPECIALS_HANDLED  # noqa: E402
from common import OUT_DIR, ROOT, add_hns_arg, read  # noqa: E402

COND_FLAG = {
    "goto_if_set": ("goto", True), "goto_if_unset": ("goto", False),
    "call_if_set": ("call", True), "call_if_unset": ("call", False),
}
CMP_OPS = {"eq": operator.eq, "ne": operator.ne, "lt": operator.lt, "le": operator.le,
           "gt": operator.gt, "ge": operator.ge}
FLAG_WRITES = {"setflag", "clearflag"}
VAR_WRITES = {"setvar", "addvar", "subvar", "copyvar", "copybyte", "random"}
TOKEN_RE = re.compile(r"\b[A-Za-z_][A-Za-z0-9_]*\b")
LABEL_RE = re.compile(r"^(\w+):{1,2}")
INSTR_RE = re.compile(r"^(\s+)(\w+)\b\s*(.*)$")
# Tokens that look like map ids but are script/map-header constants.
NOT_MAP_IDS = ("MAP_SCRIPT_", "MAP_TYPE_", "MAP_BATTLE_SCENE_", "MAP_DYNAMIC")
SCENE_FALLBACK = "MAP_BATTLE_SCENE_NORMAL"
SMALL_LIGHT = "OBJ_EVENT_GFX_SMALL_LIGHT_HNS"
EXTRA_RENAMES = {
    "Common_EventScript_SetGymTrainers_hns": "Common_EventScript_SetGymTrainers_Johto",
    "PARTNER_LANCE_HNS": "PARTNER_LANCE_JOHTO",
    "EventScript_FakePC": "EventScript_PC",
    "Route7_EventScript_Door": "Johto_EventScript_LockedDoor",
    "Route121_SafariZoneEntrance_Text_ThatWillBe500Please": "SafariZoneGate_Text_ThatWillBe500Please",
    "Debug_Text_ArianaDefeat": "RocketHideout_B2F_Text_ArianaMultiDefeat",
    "Debug_Text_Grunt23Defeat": "RocketHideout_B2F_Text_GruntMultiDefeat",
    # The three Bug-Catching Contest result jingles are GBS songs (F4); fanfares stand in.
    "MUS_HG_BUG_CONTEST_1ST_PLACE": "MUS_OBTAIN_BADGE",
    "MUS_HG_BUG_CONTEST_2ND_PLACE": "MUS_OBTAIN_ITEM",
    "MUS_HG_BUG_CONTEST_3RD_PLACE": "MUS_LEVEL_UP",
    "INGAME_TRADE_MACHOP": "INGAME_TRADE_RANDOM14",
    "INGAME_TRADE_VOLTORB": "INGAME_TRADE_RANDOM15",
    "INGAME_TRADE_MR_MIME": "INGAME_TRADE_RANDOM16",
    "INGAME_TRADE_ONIX": "INGAME_TRADE_RANDOM17",
    "INGAME_TRADE_STEELIX": "INGAME_TRADE_RANDOM18",
    "PC_LOCATION_PLAYER_HOUSE_HNS": "PC_LOCATION_PLAYER_HOUSE_FRLG",
}
# Dropped flags that stay set: the HnS Safari expansion engineers are not part of the Johto Safari Zone (D12).
FORCED_SET = {"FLAG_SAFARI_ZONE_WEST_EXPANSION"}
ALWAYS_HIDDEN_FLAG = "FLAG_JOHTO_ALWAYS_HIDDEN"
# Tokens whose whole instruction line is removed: HnS-only systems with no Johto equivalent (D4, F1, F26).
LINE_DROP_TOKENS = {
    "SetTimeEncounters", "EventScript_RockSmashHeartscale",
    "PlayersHouse_1F_Text_YouShouldRestABit_PokecenterChallenge",
}
LINE_DROP_PREFIXES = ("SecretBase_EventScript_",)
DELETED = object()


def johto_header_names():
    text = ""
    for rel in ("include/constants/flags_johto.h", "include/constants/vars_johto.h"):
        text += read(os.path.join(ROOT, rel))
    return set(re.findall(r"#define\s+((?:FLAG|VAR)_\w+)", text))


class Renamer:
    def __init__(self, hns):
        self.hns = hns
        self.table = json.load(open(os.path.join(OUT_DIR, "rename_map.json")))
        self.johto_names = johto_header_names()
        items = read(os.path.join(ROOT, "src/data/items.h"))
        self.key_items = {m.group(1) for m in re.finditer(r"\[(ITEM_\w+)\]\s*=\s*\{(.*?)\n    \},", items, re.S)
                          if "POCKET_KEY_ITEMS" in m.group(2)}
        self.sections = {
            "FLAG_": self.table["flags"], "VAR_": self.table["vars"], "ITEM_": self.table["items"],
            "TRAINER_": self.table["trainers"], "OBJ_EVENT_GFX_": self.table["obj_event_gfx"],
            "MAPSEC_": self.table["mapsec"], "HEAL_LOCATION_": self.table["heal_locations"],
            "MULTI_": self.table["multichoices"], "METATILE_": self.table["metatile_labels"],
            "MAP_": self.table["maps"], "LAYOUT_": self.table["layouts"],
        }
        self.localids = self.table["localids"]
        self.folder_map = {k: v["to"] for k, v in self.table["map_folders"].items()}
        self.label_renames = {}
        self.kurt_pending = False
        self.drop_ids = set()  # map ids that are cut or held back: lines naming them are removed
        self.report = {"drop": [], "convert": [], "cut_write": [], "review": [], "unclassified": set(),
                       "leftover": [], "tm_other": [], "shared_item": [], "light": 0}
        self.tm_blocks = {}
        for site in self.table["tm_sites"]["sites"]:
            self.tm_blocks.setdefault(site["map"], []).append(site)

    # ---- token tables -------------------------------------------------------------------------
    def entry(self, tok):
        if tok.startswith(NOT_MAP_IDS):
            return None
        for prefix, section in self.sections.items():
            if tok.startswith(prefix):
                return section.get(tok)
        return None

    def token(self, tok, map_name=None):
        """New text for `tok`, `DELETED` for a Dropped token, or `tok` when it is not in a table."""
        if tok.startswith("TMALIAS__"):
            return tok[len("TMALIAS__"):]
        if tok in EXTRA_RENAMES:
            return EXTRA_RENAMES[tok]
        if tok in self.label_renames:
            return self.label_renames[tok]
        if tok in self.drop_ids:
            return DELETED
        if tok.startswith("LOCALID_") and tok in self.localids:
            e = self.localids[tok]
            return e["to"] if map_name in e["maps"] else tok
        if tok == "WEATHER_LEAVES":
            return "WEATHER_NONE"
        e = self.entry(tok)
        if e is None:
            return tok
        if e["action"] in ("delete line", "replace branch"):
            return DELETED
        return e["to"] or tok

    def is_shared(self, tok):
        e = self.entry(tok)
        if e is None or "id" in e:
            return False
        final = e["to"] or tok
        return final not in self.johto_names

    def write_allowed(self, tok):
        e = self.entry(tok)
        return bool(e and (e.get("write_ok") or e.get("note", "").startswith("Scratch")))

    def flag_truth(self, tok):
        """Value a Dropped or Config flag takes: HIDE flags of cut content are set (hidden); the rest are unset."""
        e = self.entry(tok) or {}
        return (e.get("note", "").startswith("Constant TRUE") or tok.startswith("FLAG_HIDE_")
                or tok in FORCED_SET)

    # ---- label prefixes (colliding Johto League / Rocket Hideout / Route 23 labels) -----------------
    def prepare_labels(self, scripts_by_map):
        """Collect `label -> new label`: the MapScripts label of every map, and the label prefix of maps
        whose folder gets a `_Johto` suffix."""
        for old_folder, new_folder in self.folder_map.items():
            stripped = re.sub(r"_hns$", "", old_folder)
            for line in scripts_by_map.get(old_folder, "").split("\n"):
                m = LABEL_RE.match(line)
                if not m:
                    continue
                label = m.group(1)
                if label.startswith(old_folder + "_"):
                    self.label_renames[label] = new_folder + label[len(old_folder):]
                elif stripped != new_folder and label.startswith(stripped + "_"):
                    self.label_renames[label] = new_folder + label[len(stripped):]

    # ---- script text -------------------------------------------------------------------------------
    def tm_aliases(self, map_name):
        """{old item token: alias} for the TM/HM sites of one map (an item token is unique per map)."""
        mapping = {}
        for site in self.tm_blocks.get(map_name, []):
            old = "ITEM_" + site["hns_item"]
            if mapping.get(old, site["alias"]) != site["alias"]:
                raise SystemExit("%s: %s has two TM aliases" % (map_name, old))
            mapping[old] = site["alias"]
        return mapping

    def script(self, text, map_name=None):
        lines = text.split("\n")
        aliases = self.tm_aliases(map_name) if map_name else {}
        out = []
        current_label = ""
        for idx, raw in enumerate(lines):
            line = raw.replace("{RIVAL}", "SILVER")
            code, sep, comment = line.partition("@")
            m = LABEL_RE.match(code)
            if m:
                current_label = m.group(1)
            stripped = code.strip()
            if not stripped or stripped.startswith(".string") or stripped.startswith("@"):
                out.append(self.rename_plain(line, map_name))
                continue
            for old, new in aliases.items():
                code = re.sub(r"\b%s\b" % re.escape(old), "TMALIAS__" + new, code)
            res = self.instruction(code, map_name, current_label)
            if res is None:
                continue
            out.append(res + (sep + comment if sep else ""))
        return "\n".join(out)

    def rename_plain(self, line, map_name):
        def sub(m):
            new = self.token(m.group(0), map_name)
            return m.group(0) if new is DELETED else new
        return TOKEN_RE.sub(sub, line)

    def instruction(self, code, map_name, label):
        m = INSTR_RE.match(code)
        tokens = TOKEN_RE.findall(code)
        deleted = [t for t in tokens if self.token(t, map_name) is DELETED]
        # Unknown flag/var tokens block the import (Locked 14 needs every name classified).
        for t in tokens:
            if t.startswith(("FLAG_", "VAR_")) and self.entry(t) is None and t not in self.johto_names:
                self.report["unclassified"].add(t)
        if not m:
            return self.rename_plain(code, map_name)
        indent, op, rest = m.groups()
        where = "%s:%s" % (map_name, label)
        args = [a.strip() for a in rest.split(",")] if rest else []

        code = self.format_fixes(code, indent, op, args)
        m = INSTR_RE.match(code)
        indent, op, rest = m.groups()
        args = [a.strip() for a in rest.split(",")] if rest else []
        if self.kurt_pending and op == "goto_if_eq" and args[:2] == ["VAR_RESULT", "FALSE"]:
            # A mart sets no result: always leave through the cancel path.
            self.kurt_pending = False
            return "%sgoto %s" % (indent, self.rename_plain(args[2], map_name))
        if any(t in LINE_DROP_TOKENS or t.startswith(LINE_DROP_PREFIXES) for t in tokens):
            self.report["drop"].append("%s: %s" % (where, code.strip()))
            return None
        if op in MACROS_REMOVED:
            self.report["drop"].append("%s: %s" % (where, code.strip()))
            return None
        if op == "callnative" and args and args[0] in NATIVES_REMOVED:
            return None
        if op in ("special", "specialvar") and args:
            sp = args[-1]
            action = SPECIALS_HANDLED.get(sp)
            if action:
                kind, to = action
                if kind == "alias":
                    return self.rename_plain(code.replace(sp, to), map_name)
                if sp == "CreateKurtBallShop":
                    self.report["review"].append("%s: Kurt shop -> pokemart; the berry-to-ball tail is now dead code" % where)
                    self.kurt_pending = True
                    return indent + "pokemart Johto_Mart_KurtBalls"
                if kind in ("removed",):
                    if op == "specialvar":
                        self.report["review"].append("%s: %s result forced to FALSE" % (where, sp))
                        return "%ssetvar %s, FALSE" % (indent, args[0])
                    return None
        if op == "trainerbattle_rematch":
            return None

        # TM/HM tokens that survived the site-block replacement.
        for t in tokens:
            if re.match(r"ITEM_(TM|HM)", t) and self.token(t, map_name) is not DELETED:
                if not self.entry(t):
                    self.report["tm_other"].append("%s: %s" % (where, code.strip()))
                    break

        if not deleted:
            return self.finish(code, op, args, map_name, where)

        # ---- a Dropped / Config token is on this line ---------------------------------------------
        first = deleted[0]
        flag_like = first.startswith("FLAG_")
        if op in COND_FLAG and flag_like and len(args) == 2:
            kind, when_set = COND_FLAG[op]
            truth = self.flag_truth(first)
            if truth == when_set:
                self.report["convert"].append("%s: %s -> %s" % (where, code.strip(), kind))
                return "%s%s %s" % (indent, kind, self.rename_plain(args[1], map_name))
            self.report["drop"].append("%s: %s" % (where, code.strip()))
            return None
        mm = re.match(r"(goto|call)_if_(eq|ne|lt|le|gt|ge)$", op)
        if mm and first.startswith("VAR_") and len(args) == 3:
            try:
                const = {"TRUE": 1, "FALSE": 0}.get(args[1]) if args[1] in ("TRUE", "FALSE") else int(args[1], 0)
            except ValueError:
                const = None
            if const is not None:
                if CMP_OPS[mm.group(2)](0, const):
                    self.report["convert"].append("%s: %s -> %s" % (where, code.strip(), mm.group(1)))
                    return "%s%s %s" % (indent, mm.group(1), self.rename_plain(args[2], map_name))
                self.report["drop"].append("%s: %s" % (where, code.strip()))
                return None
            self.report["review"].append("%s: cannot evaluate %s" % (where, code.strip()))
            return None
        if op in ("checkitem", "checkitemspace", "checkflag", "multichoice", "multichoicegrid", "bufferitemname") \
                or first.startswith(("ITEM_", "MULTI_", "MAP_")):
            self.report["review"].append("%s: dropped %s on `%s`" % (where, first, code.strip()))
        else:
            self.report["drop"].append("%s: %s" % (where, code.strip()))
        return None

    @staticmethod
    def format_fixes(code, indent, op, args):
        """1.17 money/coin box syntax, and HnS `trainerbattle_*` lines missing the comma after the trainer."""
        if op in ("updatecoinsbox", "hidecoinsbox") and args:
            return indent + op
        if op == "showcoinsbox" and len(args) == 2:
            return "%sshowcoinsbox %d, %d" % (indent, int(args[0]) - 1, int(args[1]) - 1)
        if op == "checkmoney" and len(args) == 2:
            return "%scheckmoney %s" % (indent, args[0])
        if op == "updatemoneybox" and args:
            return indent + op
        if op.startswith("trainerbattle") and args and " " in args[0]:
            first, _, tail = args[0].partition(" ")
            return "%s%s %s, %s" % (indent, op, first, ", ".join([tail.strip()] + args[1:]))
        return code

    def finish(self, code, op, args, map_name, where):
        """No Dropped token: apply renames and the Locked 14 write rule."""
        if args and (op in FLAG_WRITES or op in VAR_WRITES):
            dest = args[0]
            if dest.startswith(("FLAG_", "VAR_")) and self.is_shared(dest) and not self.write_allowed(dest):
                self.report["cut_write"].append("%s: %s" % (where, code.strip()))
                return None
        if op in ("giveitem", "removeitem", "additem", "finditem", "checkitem"):
            for t in TOKEN_RE.findall(code):
                if not t.startswith("ITEM_") or t.startswith("TMALIAS__"):
                    continue
                entry = self.entry(t)
                johto_item = entry is not None
                if op == "removeitem" and not johto_item or op != "removeitem" and op != "checkitem" \
                        and not johto_item and t in self.key_items:
                    self.report["shared_item"].append("%s: %s" % (where, code.strip()))
        return self.rename_plain(code, map_name)

    def leftovers(self, text, where):
        """Report identifiers that still carry an HnS suffix."""
        for t in sorted(set(re.findall(r"\b\w*_(?:HNS|hns|Hns)\w*\b", text))):
            self.report["leftover"].append("%s: %s" % (where, t))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    add_hns_arg(parser)
    parser.add_argument("file")
    parser.add_argument("--map", default=None)
    args = parser.parse_args()
    r = Renamer(args.hns)
    print(r.script(read(args.file), args.map))
    for k, v in r.report.items():
        print("#", k, len(v) if hasattr(v, "__len__") else v, file=sys.stderr)


if __name__ == "__main__":
    main()
