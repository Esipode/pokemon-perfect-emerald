"""Approximate Johto reachability walker (Stage 26a). Single-tile BFS across maps.

Assumptions: every Hoenn field move works (Surf, Waterfall, Cut, Rock Smash, Strength);
objects are not walls except entries in BLOCKERS; ledges are one-way jumps; elevation ignored.
Usage: walker.py START_MAP X Y [--block MAP,X,Y ...] [--target MAP ...] [--reach]
"""
import json, os, re, sys, collections
from common import ROOT, load_json

L = {l["id"]: l for l in load_json(os.path.join(ROOT, "data/layouts/layouts.json"))["layouts"]}
attr_cache, blk_cache, map_cache = {}, {}, {}


def tileset_dir(sym):
    hdr = open(os.path.join(ROOT, "src/data/tilesets/headers.h"), encoding="utf-8").read()
    m = re.search(r"const struct Tileset %s\s*=\s*\{(.*?)\n\};" % sym, hdr, re.S)
    a = re.search(r"\.metatileAttributes\s*=\s*(\w+)", m.group(1)).group(1)
    mt = open(os.path.join(ROOT, "src/data/tilesets/metatiles.h"), encoding="utf-8").read()
    p = re.search(a + r"\[\]\s*=\s*INCBIN_U16\(\"([^\"]+)\"", mt).group(1)
    return os.path.join(ROOT, p)


def attrs(sym):
    if sym not in attr_cache:
        b = open(tileset_dir(sym), "rb").read()
        attr_cache[sym] = [int.from_bytes(b[i:i + 2], "little") for i in range(0, len(b), 2)]
    return attr_cache[sym]


def mp(name):
    if name not in map_cache:
        p = os.path.join(ROOT, "data/maps", name, "map.json")
        d = load_json(p)
        lay = L[d["layout"]]
        raw = open(os.path.join(ROOT, lay["blockdata_filepath"]), "rb").read()
        w, h = lay["width"], lay["height"]
        blocks = [int.from_bytes(raw[i:i + 2], "little") for i in range(0, len(raw), 2)]
        pa, sa = attrs(lay["primary_tileset"]), attrs(lay["secondary_tileset"])
        d["_w"], d["_h"], d["_blocks"], d["_pa"], d["_sa"] = w, h, blocks, pa, sa
        map_cache[name] = d
    return map_cache[name]


def idname(i):
    return i[4:] if i.startswith("MAP_") else i


ID2DIR = None


def id2dir(mid):
    global ID2DIR
    if ID2DIR is None:
        ID2DIR = {}
        for d in os.listdir(os.path.join(ROOT, "data/maps")):
            p = os.path.join(ROOT, "data/maps", d, "map.json")
            if os.path.exists(p):
                ID2DIR[load_json(p)["id"]] = d
    return ID2DIR.get(mid)


def tile(d, x, y):
    b = d["_blocks"][y * d["_w"] + x]
    mt, col = b & 0x3FF, (b >> 10) & 3
    a = d["_pa"][mt] if mt < 512 else (d["_sa"][mt - 512] if mt - 512 < len(d["_sa"]) else 0)
    return col, a & 0xFF


# behavior ids
def mbids():
    names = re.findall(r"^\s*(MB_\w+)", open(os.path.join(ROOT, "include/constants/metatile_behaviors.h")).read(), re.M)
    return {n: i for i, n in enumerate(names)}


MB = mbids()
JUMP = {MB["MB_JUMP_EAST"]: (1, 0), MB["MB_JUMP_WEST"]: (-1, 0), MB["MB_JUMP_NORTH"]: (0, -1), MB["MB_JUMP_SOUTH"]: (0, 1)}
DIRS = {"up": (0, -1), "down": (0, 1), "left": (-1, 0), "right": (1, 0)}


_wt = {}


def warptiles(name):
    if name not in _wt:
        _wt[name] = {(w["x"], w["y"]) for w in mp(name)["warp_events"]}
    return _wt[name]


def neighbors(name, x, y, blocked):
    d = mp(name)
    out = []
    for dx, dy in DIRS.values():
        nx, ny = x + dx, y + dy
        if 0 <= nx < d["_w"] and 0 <= ny < d["_h"]:
            col, beh = tile(d, nx, ny)
            if col and (nx, ny) not in warptiles(name):
                continue
            if (name, nx, ny) in blocked:
                continue
            _, cb = tile(d, x, y)
            # ledge: only enter a jump tile in its direction, land 2 tiles on
            if beh in JUMP:
                if JUMP[beh] != (dx, dy):
                    continue
                lx, ly = nx + dx, ny + dy
                if 0 <= lx < d["_w"] and 0 <= ly < d["_h"] and not tile(d, lx, ly)[0]:
                    out.append((name, lx, ly))
                continue
            out.append((name, nx, ny))
        else:
            # map connection
            for c in d.get("connections") or []:
                if DIRS[c["direction"]] != (dx, dy):
                    continue
                tn = id2dir(c["map"])
                if not tn:
                    continue
                t = mp(tn)
                if dx == 0:
                    tx = x - c["offset"]
                    ty = t["_h"] - 1 if dy < 0 else 0
                else:
                    ty = y - c["offset"]
                    tx = t["_w"] - 1 if dx < 0 else 0
                if 0 <= tx < t["_w"] and 0 <= ty < t["_h"] and not tile(t, tx, ty)[0] and (tn, tx, ty) not in blocked:
                    out.append((tn, tx, ty))
    # warps
    for w in d["warp_events"]:
        if w["x"] == x and w["y"] == y:
            tn = id2dir(w["dest_map"])
            if not tn:
                continue
            t = mp(tn)
            try:
                wi = int(w["dest_warp_id"])
                dw = t["warp_events"][wi]
                out.append((tn, dw["x"], dw["y"]))
            except Exception:
                pass
    return out


def bfs(start, blocked=frozenset(), cross=None):
    seen = {start}
    q = collections.deque([start])
    while q:
        cur = q.popleft()
        for n in neighbors(*cur, blocked):
            if n not in seen:
                seen.add(n)
                q.append(n)
    return seen


def maps_of(seen):
    return sorted({m for m, _, _ in seen})


if __name__ == "__main__":
    start = (sys.argv[1], int(sys.argv[2]), int(sys.argv[3]))
    blocked = set()
    args = sys.argv[4:]
    i = 0
    while i < len(args):
        if args[i] == "--block":
            m, x, y = args[i + 1].split(",")
            blocked.add((m, int(x), int(y)))
            i += 2
        else:
            i += 1
    s = bfs(start, frozenset(blocked))
    ms = maps_of(s)
    print(len(s), "tiles", len(ms), "maps")
    for m in ms:
        print(m)
