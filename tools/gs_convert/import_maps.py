#!/usr/bin/env python3
"""Import the HnS Johto maps, layouts, scripts and heal locations into the target (Stage 13).

One-shot: refuses to run when a Johto map group is already in `data/maps/map_groups.json`.

    python3 tools/gs_convert/import_maps.py [--hns <path>] [--dry-run]

Writes: data/maps/<Johto maps>/{map.json,scripts.inc}, data/layouts/<Johto layouts>/, the appended
entries of data/layouts/layouts.json and data/maps/map_groups.json, the Johto include block of
data/event_scripts.s, src/data/heal_locations.json, and out/import_report.txt (every decision that
needs a human look). Does not touch Hoenn or Kanto map files.
"""

import argparse
import json
import os
import re
import shutil
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from common import OUT_DIR, ROOT, add_hns_arg, johto_set, load_json, map_scripts, read  # noqa: E402
from rename_tokens import ALWAYS_HIDDEN_FLAG, DELETED, SMALL_LIGHT, Renamer  # noqa: E402

# Maps cut from the Johto set (Stage 13 step 1).
CUT_PREFIXES = ("SSAqua_",)
CUT_MAPS = {
    "TrainerHill_Courtyard_hns", "Gate_Route40_TrainerHill_Courtyard_hns", "MtSilver_1F_MoltresRoom_hns",
}
# HnS Elite Four scenes -> the Kanto E4 scenes the target already has (F22).
BATTLE_SCENES = {
    "MAP_BATTLE_SCENE_WILL_HNS": "MAP_BATTLE_SCENE_LORELEI",
    "MAP_BATTLE_SCENE_KOGA_HNS": "MAP_BATTLE_SCENE_AGATHA",
    "MAP_BATTLE_SCENE_BRUNO_HNS": "MAP_BATTLE_SCENE_BRUNO",
    "MAP_BATTLE_SCENE_KAREN_HNS": "MAP_BATTLE_SCENE_LORELEI",
    "MAP_BATTLE_SCENE_LANCE_HNS": "MAP_BATTLE_SCENE_LANCE",
}
LIGHT_REPLACEMENT = {"graphics_id": "OBJ_EVENT_GFX_LIGHT_SPRITE", "elevation": 3,
                     "movement_type": "MOVEMENT_TYPE_NONE", "movement_range_x": 0, "movement_range_y": 0,
                     "trainer_type": "TRAINER_TYPE_NONE", "trainer_sight_or_berry_tree_id": "LIGHT_TYPE_BALL",
                     "script": "NULL", "flag": "0"}


# HnS Johto maps that reuse a Hoenn tree id (it would share that tree's growth state).
BERRY_TREE_REMAP = {
    ("Route26_hns", "BERRY_TREE_ROUTE_118_SITRUS_1"): "BERRY_TREE_ROUTE_26_SITRUS",
    ("Route30_hns", "BERRY_TREE_ROUTE_102_ORAN"): "BERRY_TREE_ROUTE_30_ORAN",
}
# D22: first-visit initialiser triggers -> (town map, trigger label, route map holding its coord events,
# town state var, fly flag). The trigger's flag lines move to the Johto reset script; the first-visit state
# change moves to the town's OnTransition so it no longer depends on the entrance used.
FIRST_VISIT_TRIGGERS = [
    ("AzaleaTown_hns", "AzaleaTown_EventScript_Trigger", "Route33_hns", "VAR_AZALEA_TOWN_STATE",
     "FLAG_VISITED_AZALEA_TOWN"),
    ("GoldenrodCity_hns", "GoldenrodCity_EventScript_Trigger", "Route34_hns", "VAR_GOLDENROD_CITY_STATE",
     "FLAG_VISITED_GOLDENROD_CITY"),
    ("EcruteakCity_hns", "EcruteakCity_EventScript_Trigger", "Route37_hns", "VAR_ECRUTEAK_CITY_STATE",
     "FLAG_VISITED_ECRUTEAK_CITY"),
]
# Objects whose script has no Johto equivalent: the slot stays (local ids are positional) but is hidden.
REMOVED_OBJECT_SCRIPTS = {"IlexForest_EventScript_HeadbuttMoveTutor"}  # Headbutt is TM161 (Stage 12a)


def is_cut(name):
    return name in CUT_MAPS or name.startswith(CUT_PREFIXES)


def group_name(hns_group):
    return re.sub(r"_Hns$", "", hns_group) + "_Johto"


def battle_scene_names():
    text = read(os.path.join(ROOT, "include/constants/map_types.h")) if os.path.exists(
        os.path.join(ROOT, "include/constants/map_types.h")) else ""
    return set(re.findall(r"(MAP_BATTLE_SCENE_\w+)", text))


class Importer:
    def __init__(self, hns, dry_run):
        self.hns = hns
        self.dry = dry_run
        self.r = Renamer(hns)
        self.log = []
        self.removed_warps = []
        self.removed_connections = []
        self.scenes = battle_scene_names()
        self.kept = [(g, n, d) for g, n, d in johto_set(hns) if not is_cut(n)]
        self.kept_ids = {d["id"] for _, _, d in self.kept}
        # Cut maps and the Saffron station (Stage 16) are not in the build: lines that name them go.
        self.r.drop_ids = ({d["id"] for _, n, d in johto_set(hns) if is_cut(n)}
                           | {"MAP_SAFFRON_CITY_TRAIN_STATION_HNS", "MAP_SAFFRON_CITY_TRAIN_STATION"})
        self.scripts = {n: map_scripts(hns, n) for _, n, _ in self.kept}
        self.r.prepare_labels(self.scripts)
        self.folder = {n: self.r.folder_map[n] for _, n, _ in self.kept}
        self.map_by_id = {d["id"]: (n, d) for _, n, d in self.kept}

    def note(self, msg):
        self.log.append(msg)

    # ---- map.json ---------------------------------------------------------------------------------
    def tok(self, value, map_name):
        """Rename the tokens inside one JSON string; DELETED when the whole value is a Dropped token."""
        if not isinstance(value, str):
            return value
        if value in BATTLE_SCENES:
            return BATTLE_SCENES[value]
        whole = self.r.token(value, map_name)
        if whole is DELETED:
            return DELETED
        return re.sub(r"\b[A-Za-z_]\w*\b", lambda m: (lambda t: m.group(0) if t is DELETED else t)(
            self.r.token(m.group(0), map_name)), value)

    def convert_map(self, group, name, data):
        out = {}
        new_folder = self.folder[name]
        for key, val in data.items():
            if key in ("game_version", "region", "connections", "object_events", "warp_events",
                       "coord_events", "bg_events"):
                continue
            if key == "name":
                out[key] = new_folder
            elif key == "layout":
                out[key] = self.r.table["layouts"][val]["to"]
            elif key == "id":
                out[key] = self.r.table["maps"][val]["to"]
            else:
                new = self.tok(val, name)
                if new is DELETED:
                    new = {"battle_scene": "MAP_BATTLE_SCENE_NORMAL"}.get(key)
                    self.note("%s: %s=%r has no replacement" % (name, key, val))
                out[key] = new
        out["region"] = "REGION_JOHTO"
        scene = data.get("battle_scene")
        if scene and scene.endswith("_HNS") and scene not in BATTLE_SCENES:
            self.note("%s: battle scene %s -> NORMAL" % (name, scene))
        if out.get("battle_scene", "").endswith("_HNS"):
            out["battle_scene"] = "MAP_BATTLE_SCENE_NORMAL"
        out["connections"] = self.connections(name, data)
        out["object_events"] = self.objects(name, data)
        warps, wmap = self.warps(name, data)
        out["warp_events"] = warps
        self.warp_index[name] = wmap
        out["coord_events"] = self.coords(name, data)
        out["bg_events"] = self.bgs(name, data)
        return out

    def connections(self, name, data):
        conns = []
        for c in data.get("connections") or []:
            if c["map"] not in self.kept_ids:
                self.removed_connections.append((name, c))
                self.note("%s: connection %s %s removed" % (name, c["direction"], c["map"]))
                continue
            conns.append({"map": self.r.table["maps"][c["map"]]["to"], "offset": c["offset"],
                          "direction": c["direction"]})
        return conns

    def objects(self, name, data):
        objs = []
        for o in data.get("object_events") or []:
            new = {}
            for k, v in o.items():
                if k == "graphics_id":
                    m = re.match(r"OBJ_EVENT_GFX_MON_BASE\+(SPECIES_\w+)$", v)
                    new[k] = "OBJ_EVENT_GFX_SPECIES(%s)" % m.group(1)[len("SPECIES_"):] if m else v
                    if not m:
                        new[k] = self.tok(v, name)
                elif k == "flag":
                    t = self.tok(v, name)
                    if t is DELETED:
                        t = ALWAYS_HIDDEN_FLAG if self.r.flag_truth(v) else "0"
                        self.note("%s: object flag %s -> %s" % (name, v, t))
                    new[k] = t
                else:
                    t = self.tok(v, name)
                    if k == "trainer_sight_or_berry_tree_id":
                        t = BERRY_TREE_REMAP.get((name, v), t)
                    new[k] = "0" if t is DELETED and k != "script" else ("NULL" if t is DELETED else t)
            if o["script"] in REMOVED_OBJECT_SCRIPTS:
                new.update({"flag": ALWAYS_HIDDEN_FLAG, "script": "NULL"})
                self.note("%s: object script %s removed" % (name, o["script"]))
            if o["graphics_id"] == SMALL_LIGHT:
                new.update(LIGHT_REPLACEMENT)
                new.pop("local_id", None)
                self.r.report["light"] += 1
            objs.append(new)
        return objs

    def warps(self, name, data):
        warps, index_map = [], {}
        for i, w in enumerate(data.get("warp_events") or []):
            if w["dest_map"] == "MAP_DYNAMIC":
                index_map[i] = len(warps)
                warps.append({"x": w["x"], "y": w["y"], "elevation": w["elevation"],
                              "dest_map": "MAP_DYNAMIC", "dest_warp_id": w["dest_warp_id"], "_old_dest": None})
                continue
            if w["dest_map"] not in self.kept_ids:
                self.removed_warps.append({"map": name, "index": i, "x": w["x"], "y": w["y"], "dest": w["dest_map"]})
                self.note("%s: warp %d (%d,%d) -> %s removed" % (name, i, w["x"], w["y"], w["dest_map"]))
                continue
            index_map[i] = len(warps)
            warps.append({"x": w["x"], "y": w["y"], "elevation": w["elevation"],
                          "dest_map": self.r.table["maps"][w["dest_map"]]["to"],
                          "dest_warp_id": w["dest_warp_id"], "_old_dest": w["dest_map"]})
        return warps, index_map

    def coords(self, name, data):
        events = []
        for e in data.get("coord_events") or []:
            new = {}
            drop = False
            for k, v in e.items():
                t = self.tok(v, name)
                if t is DELETED:
                    drop = True
                    self.note("%s: coord event (%s,%s) dropped (%s=%s)" % (name, e["x"], e["y"], k, v))
                new[k] = t
            if not drop:
                events.append(new)
        return events

    def bgs(self, name, data):
        events = []
        aliases = self.r.tm_aliases(name)
        for e in data.get("bg_events") or []:
            new = {}
            drop = False
            for k, v in e.items():
                if k == "item" and v in aliases:
                    new[k] = aliases[v]
                    continue
                t = self.tok(v, name)
                if t is DELETED:
                    drop = True
                    self.note("%s: bg event (%s,%s) dropped (%s=%s)" % (name, e["x"], e["y"], k, v))
                new[k] = t
            if not drop:
                events.append(new)
        return events

    def fix_warp_ids(self, converted):
        """Re-index dest_warp_id after warps to cut maps were removed."""
        for name, data in converted.items():
            for w in data["warp_events"]:
                old_dest = w.pop("_old_dest")
                if old_dest is None:
                    continue
                dest_name, _ = self.map_by_id[old_dest]
                old = int(w["dest_warp_id"]) if str(w["dest_warp_id"]).isdigit() else None
                if old is None:
                    continue
                mapped = self.warp_index[dest_name].get(old)
                if mapped is None:
                    self.note("%s: warp (%d,%d) targets removed warp %d of %s" % (name, w["x"], w["y"], old, dest_name))
                    mapped = 0
                w["dest_warp_id"] = str(mapped)

    @staticmethod
    def tileset_symbol(key):
        """`conv:AzaleaTown_Johto` -> gTileset_AzaleaTown_Johto; primaries already carry the prefix."""
        name = key.split(":", 1)[1]
        return name if name.startswith("gTileset_") else "gTileset_" + name

    # ---- layouts ----------------------------------------------------------------------------------------
    def layouts(self):
        full = load_json(os.path.join(OUT_DIR, "tilesets/manifest.json"))
        manifest, tilesets = full["layouts"], full["tilesets"]
        by_id = {v["id"]: v for v in manifest.values()}
        hns_layouts = {x["id"]: x for x in load_json(os.path.join(self.hns, "data/layouts/layouts.json"))["layouts"]}
        entries, copies = [], []
        seen = set()
        for _, name, data in self.kept:
            lid = data["layout"]
            if lid in seen:
                continue
            seen.add(lid)
            src, conv = hns_layouts[lid], by_id[lid]
            old_dir = os.path.basename(os.path.dirname(src["blockdata_filepath"]))
            new_dir = self.r.folder_map.get(old_dir, re.sub(r"_hns$", "", old_dir))
            if os.path.exists(os.path.join(ROOT, "data/layouts", new_dir)):
                raise SystemExit("layout dir %s already exists" % new_dir)
            entries.append({
                "id": self.r.table["layouts"][lid]["to"], "name": new_dir + "_Layout",
                "width": src["width"], "height": src["height"], "border_width": 2, "border_height": 2,
                "primary_tileset": self.tileset_symbol(conv["conv"]["primary"]),
                "secondary_tileset": self.tileset_symbol(conv["conv"]["secondary"]),
                "border_filepath": "data/layouts/%s/border.bin" % new_dir,
                "blockdata_filepath": "data/layouts/%s/map.bin" % new_dir,
            })
            copies.append((os.path.join(self.hns, os.path.dirname(src["blockdata_filepath"])),
                           os.path.join(ROOT, "data/layouts", new_dir)))
        return entries, copies

    # ---- everything -----------------------------------------------------------------------------------------
    def run(self):
        groups_path = os.path.join(ROOT, "data/maps/map_groups.json")
        groups = load_json(groups_path)
        if any(g.endswith("_Johto") for g in groups["group_order"]):
            raise SystemExit("Johto map groups already imported")
        self.warp_index = {}
        converted = {}
        for group, name, data in self.kept:
            converted[name] = self.convert_map(group, name, data)
        self.fix_warp_ids(converted)

        layout_entries, layout_copies = self.layouts()

        new_groups = {}
        for group, name, _ in self.kept:
            new_groups.setdefault(group_name(group), []).append(self.folder[name])

        scripts_out = {}
        for _, name, _ in self.kept:
            text = self.r.script(self.scripts[name], name)
            self.r.leftovers(text, name)
            scripts_out[name] = text

        self.reset_extra = self.first_visit(scripts_out, converted)
        heal = self.heal_locations()
        if self.dry:
            self.write_report(layout_entries, new_groups, heal)
            return
        for name, data in converted.items():
            d = os.path.join(ROOT, "data/maps", self.folder[name])
            os.makedirs(d, exist_ok=True)
            with open(os.path.join(d, "map.json"), "w") as f:
                json.dump(data, f, indent=2)
                f.write("\n")
            with open(os.path.join(d, "scripts.inc"), "w") as f:
                f.write(scripts_out[name])
        for src, dst in layout_copies:
            shutil.copytree(src, dst)
            for junk in os.listdir(dst):
                if junk not in ("map.bin", "border.bin"):
                    os.remove(os.path.join(dst, junk))
        lp = os.path.join(ROOT, "data/layouts/layouts.json")
        layouts = load_json(lp)
        layouts["layouts"].extend(layout_entries)
        with open(lp, "w") as f:
            json.dump(layouts, f, indent=2)
            f.write("\n")
        for g, maps in new_groups.items():
            groups["group_order"].append(g)
            groups[g] = maps
        with open(groups_path, "w") as f:
            json.dump(groups, f, indent=2)
            f.write("\n")
        self.event_scripts(new_groups)
        self.reset_script()
        self.extras()
        self.write_heal(heal)
        self.write_report(layout_entries, new_groups, heal)

    SHARED_FILES = ["data/layouts/layouts.json", "data/maps/map_groups.json", "data/event_scripts.s",
                    "src/data/heal_locations.json", "data/scripts/new_game.inc", "src/new_game.c",
                    "data/scripts/johto_common.inc", "include/constants/berry.h"]

    def undo(self):
        subprocess.check_call(["git", "checkout", "--"] + self.SHARED_FILES, cwd=ROOT)
        for _, name, data in self.kept:
            shutil.rmtree(os.path.join(ROOT, "data/maps", self.folder[name]), ignore_errors=True)
        for path in self.layout_dirs():
            shutil.rmtree(path, ignore_errors=True)

    def layout_dirs(self):
        hns_layouts = {x["id"]: x for x in load_json(os.path.join(self.hns, "data/layouts/layouts.json"))["layouts"]}
        for _, name, data in self.kept:
            old_dir = os.path.basename(os.path.dirname(hns_layouts[data["layout"]]["blockdata_filepath"]))
            yield os.path.join(ROOT, "data/layouts", self.r.folder_map.get(old_dir, re.sub(r"_hns$", "", old_dir)))

    def extras(self):
        """Shared HnS contest script: replaces the Stage 12 end stubs."""
        out = self.r.script(read(os.path.join(self.hns, "data/scripts/bug_contest.inc")), None)
        self.r.leftovers(out, "bug_contest")
        with open(os.path.join(ROOT, "data/scripts/bug_contest.inc"), "w") as f:
            f.write(out)
        path = os.path.join(ROOT, "data/scripts/johto_common.inc")
        text = read(path)
        marker = "@ Bug-Catching Contest end scripts."
        if marker in text:
            with open(path, "w") as f:
                f.write(text[:text.index(marker)].rstrip("\n") + "\n")
        path = os.path.join(ROOT, "data/event_scripts.s")
        text = read(path)
        anchor = '\t.include "data/scripts/johto_common.inc"\n'
        for inc in ("bug_contest", "johto_field"):
            line = '\t.include "data/scripts/%s.inc"\n' % inc
            if line not in text:
                text = text.replace(anchor, anchor + line, 1)
        with open(path, "w") as f:
            f.write(text)
        self.berry_trees()

    def berry_trees(self):
        """Append the Johto berry tree ids (HnS names, in HnS id order) after the last used target id."""
        used = {}  # new name -> HnS name (for the HnS id order)
        for _, name, data in self.kept:
            for o in data.get("object_events") or []:
                tok = o["trainer_sight_or_berry_tree_id"]
                if tok.startswith("BERRY_TREE_"):
                    used[BERRY_TREE_REMAP.get((name, tok), tok)] = tok
        hns_ids = {n: int(v) for n, v in re.findall(r"#define (BERRY_TREE_\w+)\s+(\d+)",
                                                   read(os.path.join(self.hns, "include/constants/berry.h")))}
        path = os.path.join(ROOT, "include/constants/berry.h")
        text = read(path)
        last = max(int(v) for v in re.findall(r"#define BERRY_TREE_\w+\s+(\d+)", text))
        lines = []
        for i, tok in enumerate(sorted(used, key=lambda t: hns_ids[used[t]])):
            lines.append("#define %-29s %d" % (tok, last + 1 + i))
        if last + len(lines) >= 128:
            raise SystemExit("berry tree ids overflow BERRY_TREES_COUNT")
        marker = "#define BERRY_TREES_COUNT 128"
        text = text.replace(marker, "// Johto\n" + "\n".join(lines) + "\n\n" + marker, 1)
        with open(path, "w") as f:
            f.write(text)

    def event_scripts(self, new_groups):
        path = os.path.join(ROOT, "data/event_scripts.s")
        text = read(path)
        anchor = '\t.include "data/maps/SevenIsland_SevaultCanyon_House_Frlg/scripts.inc"\n'
        assert anchor in text
        block = "\n" + "".join('\t.include "data/maps/%s/scripts.inc"\n' % m
                               for maps in new_groups.values() for m in maps)
        with open(path, "w") as f:
            f.write(text.replace(anchor, anchor + block, 1))

    def first_visit(self, scripts_out, converted):
        """D22. Returns the flag/var lines to append to the Johto reset script."""
        moved = []
        for town, label, route, state, visited in FIRST_VISIT_TRIGGERS:
            text = scripts_out[town].split("\n")
            start = next(i for i, l in enumerate(text) if l.startswith(label + "::"))
            end = next((i for i in range(start + 1, len(text)) if re.match(r"^\w+:{1,2}", text[i])), len(text))
            block = text[start:end]
            for line in block[1:]:
                m = re.match(r"\s+(setflag|clearflag|setvar)\s+(\w+)", line)
                if m and m.group(2) not in (visited, state) and not line.strip().startswith("@"):
                    moved.append(line.split("@")[0].rstrip())
            body = ["%s_EventScript_FirstVisit::" % town.replace("_hns", ""),
                    "\tsetflag %s" % visited, "\tsetvar %s, 1" % state, "\treturn", ""]
            text[start:end] = body
            joined = "\n".join(text)
            prefix = town.replace("_hns", "")
            call = "\tcall_if_eq %s, 0, %s_EventScript_FirstVisit\n" % (state, prefix)
            tm = re.search(r"^(\w+_OnTransition)::\n", joined, re.M)
            if tm:
                joined = joined.replace(tm.group(0), tm.group(0) + call, 1)
            else:
                joined = joined.replace("\t.byte 0\n", "\tmap_script MAP_SCRIPT_ON_TRANSITION, %s_OnTransition\n\t.byte 0\n\n"
                                        "%s_OnTransition::\n%s\tend\n" % (prefix, prefix, call), 1)
            scripts_out[town] = joined
            before = len(converted[route]["coord_events"])
            converted[route]["coord_events"] = [e for e in converted[route]["coord_events"] if e["script"] != label]
            self.note("D22: %s trigger removed (%d coord events on %s), %d reset lines"
                      % (label, before - len(converted[route]["coord_events"]), route, len(moved)))
        return moved

    def reset_script(self):
        """EventScript_ResetAllMapFlagsJohto from the HnS reset script (Johto names only)."""
        text = read(os.path.join(self.hns, "data/scripts/new_game.inc"))
        body = text.split("EventScript_ResetAllMapFlagsHnS::\n", 1)[1]
        lines = []
        for line in body.split("\n"):
            op = line.split()[0] if line.split() else ""
            if op == "end":
                break
            if op in ("additem", "setrespawn", "call"):
                self.note("reset script: `%s` dropped (Hoenn/shared state)" % line.strip())
                continue
            lines.append(line)
        lines += self.reset_extra
        out = self.r.script("\n".join(lines), None)
        out = "\n".join(l.rstrip() for l in out.split("\n"))
        out = re.sub(r"\n{3,}", "\n\n", out).strip("\n")
        out = "\tsetflag FLAG_JOHTO_ALWAYS_HIDDEN\n" + out
        path = os.path.join(ROOT, "data/scripts/new_game.inc")
        with open(path, "a") as f:
            f.write("\nEventScript_ResetAllMapFlagsJohto::\n" + out + "\n\tend\n")
        path = os.path.join(ROOT, "src/new_game.c")
        c = read(path)
        c = c.replace("extern const u8 EventScript_ResetAllMapFlagsFrlg[];\n",
                      "extern const u8 EventScript_ResetAllMapFlagsFrlg[];\nextern const u8 EventScript_ResetAllMapFlagsJohto[];\n", 1)
        c = c.replace("    RunScriptImmediately(EventScript_ResetAllMapFlagsFrlg);\n",
                      "    RunScriptImmediately(EventScript_ResetAllMapFlagsFrlg);\n    RunScriptImmediately(EventScript_ResetAllMapFlagsJohto);\n", 1)
        with open(path, "w") as f:
            f.write(c)

    def heal_locations(self):
        src = load_json(os.path.join(self.hns, "src/data/heal_locations.json"))["heal_locations"]
        wanted = self.r.table["heal_locations"]
        out = []
        for h in src:
            entry = wanted.get(h["id"])
            if not entry:
                continue
            if h["map"] not in self.kept_ids:
                self.note("heal location %s skipped (map %s not imported)" % (h["id"], h["map"]))
                continue
            new = {"id": entry["to"], "map": self.r.table["maps"][h["map"]]["to"], "x": h["x"], "y": h["y"]}
            if h.get("respawn_map"):
                new["respawn_map"] = self.r.table["maps"][h["respawn_map"]]["to"]
                npc = h.get("respawn_npc")
                if npc and npc != "0":
                    new["respawn_npc"] = npc
            for k in ("respawn_x", "respawn_y"):
                if k in h:
                    new[k] = h[k]
            out.append(new)
        return out

    def write_heal(self, heal):
        path = os.path.join(ROOT, "src/data/heal_locations.json")
        data = load_json(path)
        ids = {h["id"] for h in data["heal_locations"]}
        for h in heal:
            if h["id"] in ids:
                raise SystemExit("heal location %s already exists" % h["id"])
        data["heal_locations"].extend(heal)
        with open(path, "w") as f:
            json.dump(data, f, indent=2)
            f.write("\n")

    def write_report(self, layout_entries, new_groups, heal):
        rep = self.r.report
        lines = ["maps %d, layouts %d, groups %d, heal locations %d, light placeholders %d" % (
            len(self.kept), len(layout_entries), len(new_groups), len(heal), rep["light"]), ""]
        lines.append("== import log (%d)" % len(self.log))
        lines += self.log
        for key in ("review", "convert", "cut_write", "tm_other", "leftover", "shared_item", "drop"):
            lines += ["", "== %s (%d)" % (key, len(rep[key]))]
            lines += sorted(rep[key])
        lines += ["", "== unclassified (%d)" % len(rep["unclassified"])] + sorted(rep["unclassified"])
        os.makedirs(OUT_DIR, exist_ok=True)
        with open(os.path.join(OUT_DIR, "import_report.txt"), "w") as f:
            f.write("\n".join(lines) + "\n")
        with open(os.path.join(OUT_DIR, "removed_warps.json"), "w") as f:
            json.dump({"warps": self.removed_warps,
                       "connections": [{"map": m, **c} for m, c in self.removed_connections]}, f, indent=2)
        print(lines[0])
        print("unclassified:", len(rep["unclassified"]), "leftover:", len(rep["leftover"]))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    add_hns_arg(parser)
    parser.add_argument("--dry-run", action="store_true")
    parser.add_argument("--redo", action="store_true",
                        help="undo a previous import (git checkout of the shared files, delete the Johto dirs) first")
    args = parser.parse_args()
    imp = Importer(args.hns, args.dry_run)
    if args.redo and not args.dry_run:
        imp.undo()
    imp.run()
    return 1 if imp.r.report["unclassified"] else 0


if __name__ == "__main__":
    sys.exit(main())
