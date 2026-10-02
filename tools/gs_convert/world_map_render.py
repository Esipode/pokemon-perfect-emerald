"""World map renderer and tile builder (GS Stage 28.10, R3). Host only; used by build_world_map2.py.

Draws the flat-style map into a 448x288 canvas of palette roles (world_map_layout.C_*), then
builds 4bpp tiles (H/V flip dedupe), a 16-bank palette, a 64x64 tilemap and truecolour previews.
"""

import itertools
import os
import sys
import zlib

import world_map_layout as L

NONE = "MAPSEC_NONE"

GLYPHS = {
    "A": ".###.|#...#|#...#|#####|#...#|#...#|#...#",
    "B": "####.|#...#|#...#|####.|#...#|#...#|####.",
    "C": ".####|#....|#....|#....|#....|#....|.####",
    "D": "####.|#...#|#...#|#...#|#...#|#...#|####.",
    "E": "#####|#....|#....|####.|#....|#....|#####",
    "F": "#####|#....|#....|####.|#....|#....|#....",
    "G": ".####|#....|#....|#.###|#...#|#...#|.####",
    "H": "#...#|#...#|#...#|#####|#...#|#...#|#...#",
    "I": "#####|..#..|..#..|..#..|..#..|..#..|#####",
    "J": "..###|...#.|...#.|...#.|...#.|#..#.|.##..",
    "K": "#...#|#..#.|#.#..|##...|#.#..|#..#.|#...#",
    "L": "#....|#....|#....|#....|#....|#....|#####",
    "M": "#...#|##.##|#.#.#|#.#.#|#...#|#...#|#...#",
    "N": "#...#|##..#|#.#.#|#..##|#...#|#...#|#...#",
    "O": ".###.|#...#|#...#|#...#|#...#|#...#|.###.",
    "P": "####.|#...#|#...#|####.|#....|#....|#....",
    "Q": ".###.|#...#|#...#|#...#|#.#.#|#..#.|.##.#",
    "R": "####.|#...#|#...#|####.|#.#..|#..#.|#...#",
    "S": ".####|#....|#....|.###.|....#|....#|####.",
    "T": "#####|..#..|..#..|..#..|..#..|..#..|..#..",
    "U": "#...#|#...#|#...#|#...#|#...#|#...#|.###.",
    "V": "#...#|#...#|#...#|#...#|#...#|.#.#.|..#..",
    "W": "#...#|#...#|#...#|#.#.#|#.#.#|##.##|#...#",
    "X": "#...#|#...#|.#.#.|..#..|.#.#.|#...#|#...#",
    "Y": "#...#|#...#|.#.#.|..#..|..#..|..#..|..#..",
    "Z": "#####|....#|...#.|..#..|.#...|#....|#####",
    "?": ".###.|#...#|....#|...#.|..#..|.....|..#..",
}
GLYPH_W, GLYPH_H, LABEL_SPACING, WORD_GAP = 5, 7, 2, 5


def adjacent(x, y):
    return ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1))


def bank_of(unit, terr, cx, cy):
    return L.BANKS[unit[cy][cx] or terr[cy][cx]]


def sec_cells(world):
    cells = {}
    for y, row in enumerate(world):
        for x, tok in enumerate(row):
            if tok != NONE:
                cells.setdefault(tok[7:], []).append((x, y))
    return cells


def node_kind(name):
    if name.startswith("ROUTE_") and name not in L.SMALL_NODES or name in L.ROUTE_LIKE:
        return "route"
    if name.endswith("_CITY"):
        return "big"
    if name.endswith("_TOWN") or name in L.SMALL_NODES:
        return "small"
    return "landmark"


class Canvas:
    def __init__(self, w, h, fill=0):
        self.w, self.h = w, h
        self.px = bytearray([fill]) * (w * h)

    def get(self, x, y):
        return self.px[y * self.w + x] if 0 <= x < self.w and 0 <= y < self.h else None

    def put(self, x, y, v):
        if 0 <= x < self.w and 0 <= y < self.h:
            self.px[y * self.w + x] = v


def text_width(text):
    w = 0
    for i, ch in enumerate(text):
        w += (WORD_GAP if ch == " " else GLYPH_W) + (LABEL_SPACING if i < len(text) - 1 else 0)
    return w


def glyph_pixels(text, x0, y0):
    out, x = [], x0
    for ch in text:
        if ch == " ":
            x += WORD_GAP + LABEL_SPACING
            continue
        for gy, row in enumerate(GLYPHS[ch].split("|")):
            for gx, c in enumerate(row):
                if c == "#":
                    out.append((x + gx, y0 + gy))
        x += GLYPH_W + LABEL_SPACING
    return out


def render(mask, world, unit, terr):
    """Returns (canvas, report dict). mask is the land mask (bytearray, 1 = land)."""
    W, H = L.WORLD_W * 8, L.WORLD_H * 8
    rep = {"warnings": []}
    cells = sec_cells(world)
    cv = Canvas(W, H, L.C_SEA)

    # Land exists only inside region territory; stubs in open-sea cells are dropped.
    land = bytearray(W * H)
    stubs = 0
    for y in range(H):
        for x in range(W):
            if mask[y * W + x]:
                if bank_of(unit, terr, x // 8, y // 8):
                    land[y * W + x] = 1
                else:
                    stubs += 1
    rep["dropped_stub_px"] = stubs

    def is_land(x, y):
        return 0 <= x < W and 0 <= y < H and land[y * W + x]

    for y in range(H):
        for x in range(W):
            if land[y * W + x]:
                coast = any(not is_land(nx, ny) for nx, ny in adjacent(x, y))
                cv.put(x, y, L.C_COAST if coast else L.C_LAND)
            elif any(is_land(x + dx, y + dy) for dx, dy in itertools.product((-1, 0, 1), repeat=2)):
                cv.put(x, y, L.C_SHALLOW)

    # Johto/Kanto land join: dotted line on the seam.
    for y in range(H):
        for x in range(W - 1):
            if is_land(x, y) and is_land(x + 1, y) and y % 2 == 0:
                a, b = bank_of(unit, terr, x // 8, y // 8), bank_of(unit, terr, (x + 1) // 8, y // 8)
                if a != b and a and b:
                    cv.put(x, y, L.C_BORDER)

    # Mountain relief: two peaks in the relief colour, land pixels only.
    peak = ["...#...", "..###..", ".#####.", "#######"]
    for name in L.RELIEF:
        if name not in cells:
            continue
        cs = cells[name]
        mx = round(sum(c[0] for c in cs) / len(cs) * 8 + 4)
        my = round(sum(c[1] for c in cs) / len(cs) * 8 + 4)
        for ox, oy in ((-9, -3), (2, -1)):
            for j, row in enumerate(peak):
                for i, ch in enumerate(row):
                    x, y = mx + ox + i, my + oy + j
                    if ch == "#" and cv.get(x, y) == L.C_LAND:
                        cv.put(x, y, L.C_RELIEF)

    # Sea MAPSECs (under 25% land) draw dotted routes.
    sea = set()
    for name, cs in cells.items():
        n = sum(land[(cy * 8 + py) * W + cx * 8 + px] for cx, cy in cs for py in range(8) for px in range(8))
        if n < len(cs) * 16:
            sea.add(name)
    rep["sea_mapsecs"] = sorted(sea)

    def center(c):
        return c[0] * 8 + 3, c[1] * 8 + 3        # top-left of the 2x2 centre

    def line(a, b, dotted):
        (ax, ay), (bx, by) = center(a), center(b)
        for x in range(min(ax, bx), max(ax, bx) + 2):
            for y in range(min(ay, by), max(ay, by) + 2):
                if not dotted or ((x + y) // 2) % 2 == 0:
                    cv.put(x, y, L.C_ROUTE)

    kinds = {n: node_kind(n) for n in cells}
    cell_sec = {c: n for n, cs in cells.items() for c in cs}
    node_cell = {}
    for name, cs in cells.items():
        bx = sum(c[0] for c in cs) / len(cs)
        by = sum(c[1] for c in cs) / len(cs)
        node_cell[name] = min(cs, key=lambda c: (c[0] - bx) ** 2 + (c[1] - by) ** 2)

    # Joins between MAPSECs.
    joins = {frozenset(p) for p in L.ROUTE_JOINS}
    breaks = {frozenset(p) for p in L.ROUTE_BREAKS}
    edges = {}
    attach = {n: [] for n in cells}
    contacts = {}
    for c, a in cell_sec.items():
        for nb in adjacent(*c):
            b = cell_sec.get(nb)
            if not b or b == a or c > nb:
                continue
            pair = frozenset((a, b))
            if pair in breaks:
                continue
            if unit[c[1]][c[0]] != unit[nb[1]][nb[0]] and pair not in joins:
                continue
            contacts.setdefault(pair, []).append((c, nb, a, b))
    # One line per joined pair, at the middle of the shared border.
    for pair, cand in sorted(contacts.items(), key=lambda kv: sorted(kv[0])):
        c, nb, a, b = sorted(cand)[len(cand) // 2]
        edges[(c, nb)] = (a, b)
        attach[a].append(c)
        attach[b].append(nb)

    # Inside each MAPSEC: shortest paths joining its attachment cells, its node cell and, for
    # routes, the far end, so a route shows its length without filling a block with lines.
    for name, cs in cells.items():
        inside = set(cs)
        terms = sorted(set(attach[name]))
        if kinds[name] != "route":
            terms.append(node_cell[name])
        root = terms[0] if terms else cs[0]
        parent, order = {root: None}, [root]
        for cur in order:
            for nb in adjacent(*cur):
                if nb in inside and nb not in parent:
                    parent[nb] = cur
                    order.append(nb)
        if kinds[name] == "route":
            terms.append(order[-1])
        for t in terms:
            while parent[t] is not None:
                edges[(parent[t], t)] = (name, name)
                t = parent[t]
    for (c, nb), (a, b) in edges.items():
        dotted = any(n in sea and kinds[n] == "route" for n in (a, b))
        line(c, nb, dotted)
    rep["edges"] = len(edges)

    # Nodes.
    def rim_around(fill):
        for x, y in fill:
            cv.put(x, y, L.C_NODE)
        for x, y in fill:
            for nb in adjacent(x, y):
                if nb not in fill:
                    cv.put(nb[0], nb[1], L.C_RIM)
        for x, y in fill:
            cv.put(x, y, L.C_NODE)

    for name, cs in cells.items():
        kind = kinds[name]
        if kind == "route":
            continue
        nc = node_cell[name]
        if kind == "big":
            fill = set()
            for cx, cy in cs:
                fill |= {(cx * 8 + 2 + i, cy * 8 + 2 + j) for i in range(4) for j in range(4)}
                if (cx + 1, cy) in cs:
                    fill |= {(cx * 8 + 2 + i, cy * 8 + 2 + j) for i in range(12) for j in range(4)}
                if (cx, cy + 1) in cs:
                    fill |= {(cx * 8 + 2 + i, cy * 8 + 2 + j) for i in range(4) for j in range(12)}
            rim_around(fill)
        elif kind == "small":
            rim_around({(nc[0] * 8 + 3 + i, nc[1] * 8 + 3 + j) for i in range(2) for j in range(2)})
        else:
            cx, cy = nc[0] * 8 + 4, nc[1] * 8 + 4
            for dx, dy in itertools.product(range(-2, 3), repeat=2):
                d = abs(dx) + abs(dy)
                if d <= 1:
                    cv.put(cx + dx, cy + dy, L.C_RIM)
                elif d == 2:
                    cv.put(cx + dx, cy + dy, L.C_NODE)

    # Region labels: deep-sea spot nearest the anchor. The label cells take the region's bank so
    # a locked region hides its label even when the sea there is open sea.
    unit_bank = {"JOHTO": 1, "KANTO": 2, "HOENN": 3, "SEVII": 4}
    rep["bank_override"] = {}
    for text, (ax, ay), region in L.LABELS:
        tw = text_width(text)
        best = None
        for y in range(2, H - GLYPH_H - 2, 2):
            for x in range(2, W - tw - 2, 2):
                cost = abs(x - ax) + abs(y - ay)
                if best and cost >= best[0]:
                    continue
                if all(cv.get(px, py) == L.C_SEA for px in range(x - 2, x + tw + 2)
                       for py in range(y - 2, y + GLYPH_H + 2)):
                    best = (cost, x, y)
        if not best:
            rep["warnings"].append("label %s: no deep-sea spot" % text)
            continue
        for px, py in glyph_pixels(text, best[1], best[2]):
            cv.put(px, py, L.C_LABEL)
        for cy in range(best[2] // 8, (best[2] + GLYPH_H - 1) // 8 + 1):
            for cx in range(best[1] // 8, (best[1] + tw - 1) // 8 + 1):
                rep["bank_override"][(cx, cy)] = unit_bank[region]
        rep["warnings"].append("label %s at px (%d,%d)" % (text, best[1], best[2]))
    return cv, rep


def flip_h(t):
    return bytes(t[y * 8 + 7 - x] for y in range(8) for x in range(8))


def flip_v(t):
    return bytes(t[(7 - y) * 8 + x] for y in range(8) for x in range(8))


def build_tiles(cv, unit, terr, override):
    """Dedupe cell tiles with H/V flips. Returns (tiles, map entries 64x64, bank usage)."""
    tiles = [bytes(64)]
    seen = {}

    def register(t, idx):
        for v, f in ((t, 0), (flip_h(t), 1), (flip_v(t), 2), (flip_h(flip_v(t)), 3)):
            seen.setdefault(v, (idx, f))

    register(tiles[0], 0)
    entries = [0] * (64 * 64)
    banks = {}
    for cy in range(L.WORLD_H):
        for cx in range(L.WORLD_W):
            t = bytes(cv.px[(cy * 8 + y) * cv.w + cx * 8 + x] for y in range(8) for x in range(8))
            if t not in seen:
                tiles.append(t)
                register(t, len(tiles) - 1)
            idx, f = seen[t]
            bank = override.get((cx, cy), bank_of(unit, terr, cx, cy))
            banks[bank] = banks.get(bank, 0) + 1
            entries[cy * 64 + cx] = idx | (f & 1) << 10 | (f >> 1) << 11 | bank << 12
    return tiles, entries, banks


def q5(v):
    v5 = v >> 3
    return (v5 << 3) | (v5 >> 2)


def ramp(bank_rgb_land, grey=False):
    """16-colour ramp (RGB tuples) for one bank."""
    land = bank_rgb_land
    coast = tuple(q5(min(255, c + (255 - c) * 55 // 100)) for c in land)
    relief = tuple(q5(c * 72 // 100) for c in land)
    cols = [L.SEA_RGB, L.SHALLOW_RGB, land, coast, relief, L.ROUTE_RGB, L.NODE_RGB, L.RIM_RGB,
            L.LABEL_RGB, L.BORDER_RGB]
    if grey:
        # Locked look: route, node, rim and border read as land; the label reads as sea.
        cols = [L.SEA_RGB, L.SHALLOW_RGB, land, coast, land, land, land, land, L.SEA_RGB, land]
    cols = [tuple(q5(c) for c in col) for col in cols]
    return cols + [(0, 0, 0)] * (16 - len(cols))


def palette():
    banks = [[(0, 0, 0)] * 16 for _ in range(16)]
    for b, land in L.LAND_RGB.items():
        banks[b] = ramp(land)
    banks[L.LOCK_BANK] = ramp(L.GREY_LAND_RGB, grey=True)
    return banks


def write_jasc(banks, path):
    with open(path, "w") as f:
        f.write("JASC-PAL\r\n0100\r\n256\r\n")
        for b in banks:
            for r, g, bl in b:
                f.write("%d %d %d\r\n" % (r, g, bl))


def write_tilemap(entries, path):
    with open(path, "wb") as f:
        for e in entries:
            f.write(bytes((e & 0xFF, e >> 8)))


def tiles_4bpp(tiles):
    out = bytearray()
    for t in tiles:
        for i in range(0, 64, 2):
            out.append(t[i] | t[i + 1] << 4)
    return bytes(out)


def truecolour(cv, entries_bank, banks_pal):
    """Rows of RGB for the canvas, coloured per cell bank."""
    rows = []
    for y in range(cv.h):
        row = []
        for x in range(cv.w):
            row.append(banks_pal[entries_bank[(y // 8) * 64 + x // 8]][cv.px[y * cv.w + x]])
        rows.append(row)
    return rows


def scaled(rows, s):
    out = []
    for r in rows:
        wide = [c for c in r for _ in range(s)]
        out.extend([wide] * s)
    return out
