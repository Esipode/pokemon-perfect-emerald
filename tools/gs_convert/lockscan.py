"""Stage 26a: find script paths that run `lock`/`lockall` and reach `end` without `release`/`releaseall`.
Usage: lockscan.py            (all Johto maps; entries are object, bg and coord scripts)
"""
import json, os, re, sys, collections
from common import ROOT, load_json
import progression_audit as PA

inv = load_inv = PA.load_inv()
labels = inv["labels"]
order = collections.defaultdict(list)
for l, v in labels.items():
    order[v["file"]].append((v["lines"][0][0] if v["lines"] else 0, l))
nxt = {}
for f, lst in order.items():
    lst.sort()
    for i, (_, l) in enumerate(lst[:-1]):
        nxt[l] = lst[i + 1][1]

TERMINAL_OK = re.compile(r"^(trainerbattle_single|trainerbattle_double|trainerbattle_rematch\w*|trainerbattle_two_trainers|warp\w*|return|waitstate|special\s+(StartBattle|StartLegendaryBattle)|callnative\s+\w*Reset)\b")
BRANCH = re.compile(r"^(goto_if_\w+|call_if_\w+)\s+(.*)$")


def args_of(line):
    parts = line.split(None, 1)
    return parts[1] if len(parts) > 1 else ""


def scan(entry):
    out = []
    seen = set()
    stack = [(entry, 0, False, (entry,))]
    while stack:
        lab, idx, locked, trail = stack.pop()
        key = (lab, idx, locked)
        if key in seen or lab not in labels:
            continue
        seen.add(key)
        lines = labels[lab]["lines"]
        i = idx
        cont = True
        while i < len(lines):
            ln, line = lines[i]
            op = line.split(None, 1)[0]
            if op in ("lock", "lockall"):
                locked = True
            elif op in ("release", "releaseall"):
                locked = False
            elif op == "msgbox":
                a = args_of(line)
                if re.search(r"MSGBOX_(NPC|AUTOCLOSE|SIGN)\b", a):
                    locked = False
                if re.search(r"MSGBOX_SIGN\b", a):
                    pass
            elif op == "finditem":
                locked = False
            elif op == "end":
                if locked:
                    out.append((trail, labels[lab]["map"], lab, ln))
                cont = False
                break
            elif TERMINAL_OK.match(line):
                if op.startswith("trainerbattle_single") or op == "return" or op.startswith("warp"):
                    cont = False
                    break
            elif op in ("goto",):
                tgt = args_of(line).strip()
                stack.append((tgt, 0, locked, trail + (tgt,)))
                cont = False
                break
            elif op in ("call",):
                pass
            else:
                m = BRANCH.match(line)
                if m and m.group(1).startswith("goto_if"):
                    tgt = m.group(2).split(",")[-1].strip()
                    stack.append((tgt, 0, locked, trail + (tgt,)))
            i += 1
        if cont and i >= len(lines):
            n = nxt.get(lab)
            if n:
                stack.append((n, 0, locked, trail + (n,)))
    return out


def entries():
    ents = set()
    for name in inv["maps"]:
        d = load_json(os.path.join(ROOT, "data/maps", name, "map.json"))
        for o in d["object_events"]:
            s = o.get("script")
            if s and s != "NULL" and o.get("trainer_type") == "TRAINER_TYPE_NONE":
                ents.add((name, s))
        for b in d["bg_events"]:
            s = b.get("script")
            if s and s not in ("NULL", "None"):
                ents.add((name, s))
        for c in d["coord_events"]:
            s = c.get("script")
            if s and s not in ("NULL", "None"):
                ents.add((name, s))
    return sorted(ents)


if __name__ == "__main__":
    n = 0
    for m, s in entries():
        for trail, mp, lab, ln in scan(s):
            n += 1
            print("%s | entry %s | ends in %s (%s:%d) | path %s" % (m, s, lab, mp, ln, " > ".join(trail[-4:])))
            break
    print("total", n)
