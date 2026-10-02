"""Stage 26a: static state inventory for Johto scripts and map.json files.

Writes out/progression_inventory.json and prints a summary. Subcommands:
  inventory                 build the JSON
  sym NAME [NAME...]        writers/readers/hides/triggers for symbols
  entry                     map-entry (ON_TRANSITION/ON_LOAD/ON_FRAME/ON_WARP) persistent writes
  foreign                   Johto writes to non-Johto flags/vars/items
  walk MAP X Y              approximate flood fill from a tile (collision only)
"""

import collections
import glob
import json
import os
import re
import sys

from common import ROOT, OUT_DIR, load_json, read

MAPS = os.path.join(ROOT, "data/maps")
SHARED = ["data/scripts/johto_common.inc", "data/scripts/johto_field.inc"]

WRITE_OPS = {"setflag": "set", "clearflag": "clear", "setvar": "setvar", "setvar_min": "setvar_min",
             "addvar": "addvar", "subvar": "subvar", "copyvar": "copyvar", "setorcopyvar": "setvar"}
READ_RE = re.compile(r"^\s*(goto_if_\w+|call_if_\w+|compare|checkflag|goto_if_set|goto_if_unset|call_if_set|call_if_unset)\s+(.*)$")
FLAGVAR_RE = re.compile(r"\b((?:FLAG|VAR)_[A-Z0-9_]+)\b")
LABEL_RE = re.compile(r"^([A-Za-z0-9_]+)::?\s*(?:@.*)?$")
JUMP_RE = re.compile(r"^\s*(goto|call|goto_if_\w+|call_if_\w+|trainerbattle\w*|msgbox)\b(.*)$")


def johto_maps():
    out = []
    for p in sorted(glob.glob(os.path.join(MAPS, "*/map.json"))):
        d = load_json(p)
        if d.get("region") == "REGION_JOHTO":
            out.append((os.path.basename(os.path.dirname(p)), d))
    return out


def parse_blocks(text):
    """label -> list of (lineno, stripped line)."""
    blocks, cur = {}, None
    for i, raw in enumerate(text.splitlines(), 1):
        line = raw.split("@")[0].rstrip()
        m = LABEL_RE.match(line)
        if m:
            cur = m.group(1)
            blocks[cur] = []
            continue
        if cur and line.strip():
            blocks[cur].append((i, line.strip()))
    return blocks


def build():
    maps = johto_maps()
    inv = {"maps": {}, "labels": {}, "writes": collections.defaultdict(list),
           "reads": collections.defaultdict(list), "hides": collections.defaultdict(list),
           "triggers": collections.defaultdict(list), "items": collections.defaultdict(list),
           "mapscripts": {}}
    srcs = []
    for name, d in maps:
        path = os.path.join(MAPS, name, "scripts.inc")
        srcs.append((name, path, read(path) if os.path.exists(path) else ""))
    for s in SHARED:
        srcs.append(("<shared>", os.path.join(ROOT, s), read(os.path.join(ROOT, s))))
    for name, path, text in srcs:
        rel = os.path.relpath(path, ROOT)
        blocks = parse_blocks(text)
        for label, lines in blocks.items():
            inv["labels"][label] = {"map": name, "file": rel, "lines": lines}
            for ln, line in lines:
                op = line.split(None, 1)[0]
                if op in WRITE_OPS:
                    syms = FLAGVAR_RE.findall(line)
                    if syms:
                        inv["writes"][syms[0]].append([name, label, ln, line])
                elif op in ("additem", "removeitem"):
                    inv["items"][line.split()[1].rstrip(",")].append([name, label, ln, line])
                m = READ_RE.match(line)
                if m:
                    for s in FLAGVAR_RE.findall(m.group(2).split(",")[0]):
                        inv["reads"][s].append([name, label, ln, line])
        # map script tables
        for m in re.finditer(r"map_script (MAP_SCRIPT_\w+), (\w+)", text):
            inv["mapscripts"].setdefault(name, []).append([m.group(1), m.group(2)])
        for m in re.finditer(r"map_script_2 (VAR_\w+), (\w+), (\w+)", text):
            inv["mapscripts"].setdefault(name, []).append(["FRAME:" + m.group(1) + "==" + m.group(2), m.group(3)])
    for name, d in maps:
        inv["maps"][name] = {"id": d["id"], "layout": d["layout"], "connections": d.get("connections"),
                             "warps": [(w["x"], w["y"], w["dest_map"], w["dest_warp_id"]) for w in d["warp_events"]]}
        for o in d["object_events"]:
            f = o.get("flag", "0")
            if f not in ("0", 0):
                inv["hides"][f].append([name, o.get("script"), o["x"], o["y"], o["graphics_id"]])
        for c in d["coord_events"]:
            if c.get("type") == "trigger":
                inv["triggers"][c["var"]].append([name, c["script"], c["x"], c["y"], c["var_value"]])
            else:
                inv["triggers"][c.get("flag", "?")].append([name, c["script"], c["x"], c["y"], "weather"])
        for o in d["object_events"]:
            if o.get("flag") in ("0", 0, None):
                pass
    os.makedirs(OUT_DIR, exist_ok=True)
    with open(os.path.join(OUT_DIR, "progression_inventory.json"), "w") as f:
        json.dump(inv, f)
    print("maps %d labels %d writes-syms %d reads-syms %d" % (
        len(maps), len(inv["labels"]), len(inv["writes"]), len(inv["reads"])))


def load_inv():
    return load_json(os.path.join(OUT_DIR, "progression_inventory.json"))


def sym(names):
    inv = load_inv()
    for n in names:
        print("==", n)
        for k in ("writes", "reads"):
            for e in inv[k].get(n, []):
                print("  %-6s %s:%s:%d  %s" % (k[:-1], e[0], e[1], e[2], e[3]))
        for e in inv["hides"].get(n, []):
            print("  hides  ", e)
        for e in inv["triggers"].get(n, []):
            print("  trigger", e)


def reach(inv, label, seen=None):
    """Transitive set of labels reached from `label` via goto/call."""
    seen = seen if seen is not None else set()
    if label in seen or label not in inv["labels"]:
        return seen
    seen.add(label)
    for _, line in inv["labels"][label]["lines"]:
        m = JUMP_RE.match(line)
        if not m:
            continue
        for tok in re.findall(r"\b([A-Za-z][A-Za-z0-9_]*_[A-Za-z0-9_]+)\b", m.group(2)):
            if tok in inv["labels"]:
                reach(inv, tok, seen)
    return seen


def entry():
    inv = load_inv()
    for name, tabs in sorted(inv["mapscripts"].items()):
        for kind, label in tabs:
            if kind in ("MAP_SCRIPT_ON_RESUME", "MAP_SCRIPT_ON_DIVE_WARP", "MAP_SCRIPT_ON_WARP_INTO_MAP_TABLE"):
                continue
            ws = []
            for l in sorted(reach(inv, label)):
                for _, line in inv["labels"][l]["lines"]:
                    if line.split(None, 1)[0] in WRITE_OPS:
                        ws.append("%s: %s" % (l, line))
            if ws:
                print("%s  %s -> %s" % (name, kind, label))
                for w in ws:
                    print("    ", w)


def foreign():
    inv = load_inv()
    owned = re.compile(r"JOHTO|_HNS|ELM|BUG_CONTEST|ECRUTEAK|GOLDENROD|AZALEA|VIOLET|CHERRYGROVE|OLIVINE|CIANWOOD|MAHOGANY|BLACKTHORN|NEW_BARK|ROCKET")
    for k in ("writes",):
        for sym_, es in sorted(inv[k].items()):
            if sym_.startswith(("FLAG_", "VAR_")):
                for e in es:
                    print("%s  %s:%s:%d  %s" % (sym_, e[0], e[1], e[2], e[3]))


def main():
    cmd = sys.argv[1] if len(sys.argv) > 1 else "inventory"
    if cmd == "inventory":
        build()
    elif cmd == "sym":
        sym(sys.argv[2:])
    elif cmd == "entry":
        entry()
    elif cmd == "scenes":
        scenes()
    elif cmd == "foreign":
        foreign()



def scenes():
    """Per story var: exact-match readers (scenes) and writers, to find scenes a higher write can skip."""
    inv = load_inv()
    eq = collections.defaultdict(list)
    for name, path, text in [(n, os.path.join(MAPS, n, "scripts.inc"), None) for n in inv["maps"]] + [("<shared>", None, None)]:
        pass
    pat = re.compile(r"^\s*(goto_if_eq|call_if_eq|map_script_2)\s+(VAR_\w+),\s*(\w+),\s*(\w+)")
    for label, v in inv["labels"].items():
        for ln, line in v["lines"]:
            m = pat.match(line)
            if m and m.group(3).isdigit():
                eq[m.group(2)].append((int(m.group(3)), v["map"], label, m.group(4)))
    for mp_, tabs in inv["mapscripts"].items():
        for kind, label in tabs:
            m = re.match(r"FRAME:(VAR_\w+)==(\w+)", kind)
            if m and m.group(2).isdigit():
                eq[m.group(1)].append((int(m.group(2)), mp_, "FRAME", label))
    for var, trig in inv["triggers"].items():
        if var.startswith("VAR_"):
            for t in trig:
                if t[4].isdigit():
                    eq[var].append((int(t[4]), t[0], "COORD", t[1]))
    for var in sorted(eq):
        if var.startswith(("VAR_TEMP", "VAR_0x", "VAR_RESULT", "VAR_FACING", "VAR_LAST", "VAR_ELEVATOR", "VAR_TRAIN", "VAR_CABLE")):
            continue
        vals = collections.defaultdict(set)
        for n, m, l, t in eq[var]:
            vals[n].add("%s:%s" % (m, t))
        writes = collections.OrderedDict()
        for m, l, ln, line in inv["writes"].get(var, []):
            parts = line.replace(",", " ").split()
            if parts[0] in ("setvar", "setvar_min") and parts[-1].isdigit():
                writes.setdefault(int(parts[-1]), set()).add("%s%s" % ("min " if parts[0] == "setvar_min" else "", m))
        print("==", var)
        print("  scenes:", {k: sorted(v)[:3] for k, v in sorted(vals.items())})
        print("  writes:", {k: sorted(v)[:3] for k, v in sorted(writes.items())})

if __name__ == "__main__":
    main()
