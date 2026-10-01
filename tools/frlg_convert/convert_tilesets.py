#!/usr/bin/env python3
"""Convert FRLG-format tilesets to Emerald format without changing VRAM contents.

FRLG primaries hold 640 tiles / 640 metatiles / palettes 0-6 with 4-byte
attributes. Emerald primaries hold 512 / 512 / 0-5 with 2-byte attributes.
For each (primary P, secondary S) pair used by a kept FRLG layout:

  * primary  P' = P tiles 0-511, metatiles 0-511, attributes 0-511.
  * secondary S' = P tiles 512-639 + S tiles, P metatiles 512-639 + S
    metatiles (attributes likewise), S palettes with slot 6 taken from P.

A secondary paired with more than one primary gets one copy per extra
primary. Writes the converted files under --out (paths relative to the repo
root) and a manifest consumed by verify_tilesets.py.

Usage: convert_tilesets.py --out <dir> [--manifest <path>]
"""

import argparse
import collections
import json
import os
import re
import shutil
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import png4  # noqa: E402

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))

HEADERS_H = "src/data/tilesets/headers.h"
GRAPHICS_H = "src/data/tilesets/graphics.h"
METATILES_H = "src/data/tilesets/metatiles.h"
LAYOUTS_JSON = "data/layouts/layouts.json"
MAP_GROUPS_JSON = "data/maps/map_groups.json"

FRLG_PRIMARY_TILES = 640
FRLG_PRIMARY_PALS = 7
EM_PRIMARY_TILES = 512
EM_PRIMARY_PALS = 6
TOTAL_TILES = 1024
TOTAL_MAP_PALS = 13
DUP_COUNT = FRLG_PRIMARY_TILES - EM_PRIMARY_TILES  # 128
EM_SECONDARY_CAPACITY = TOTAL_TILES - EM_PRIMARY_TILES  # 512
FRLG_SECONDARY_CAPACITY = TOTAL_TILES - FRLG_PRIMARY_TILES  # 384
METATILE_BYTES = 16
TILE_BYTES_4BPP = 32

# New primary header names and directories.
PRIMARY_RENAMES = {
    "gTileset_General_Frlg": ("gTileset_GeneralKanto", "data/tilesets/primary/general_kanto", "GeneralKanto"),
    "gTileset_BuildingFrlg": ("gTileset_BuildingKanto", "data/tilesets/primary/building_kanto", "BuildingKanto"),
}
# Suffixes for extra copies of a secondary paired with a non-home primary.
PRIMARY_VARIANT_SUFFIX = {
    "gTileset_General_Frlg": ("General", "general"),
    "gTileset_BuildingFrlg": ("Building", "building"),
}

# Maps dropped from the merged game (plan O§8.4 / Stage 11).
DROP_MAP_GROUPS = {"gMapGroup_Link_Frlg"}
DROP_MAP_RE = re.compile(r"^(NavelRock|BirthIsland)_.*_Frlg$")


class ConvertError(Exception):
    pass


# --------------------------------------------------------------------------
# Data source: working tree or a git ref
# --------------------------------------------------------------------------

class Source:
    def __init__(self, ref=None, root=REPO_ROOT):
        self.ref = ref
        self.root = root
        self._cache = {}

    def read(self, path):
        if path in self._cache:
            return self._cache[path]
        if self.ref is None:
            with open(os.path.join(self.root, path), "rb") as f:
                data = f.read()
        else:
            data = subprocess.run(["git", "-C", self.root, "show", f"{self.ref}:{path}"],
                                  check=True, capture_output=True).stdout
        self._cache[path] = data
        return data

    def text(self, path):
        return self.read(path).decode("utf-8")

    def json(self, path):
        return json.loads(self.text(path))


# --------------------------------------------------------------------------
# Parsing
# --------------------------------------------------------------------------

def eval_pal_bits(expr):
    """Evaluate a palette bitmask: integers and SWAP_PAL(n) terms joined by '|'."""
    value = 0
    for term in expr.split("|"):
        term = term.strip()
        m = re.fullmatch(r"SWAP_PAL\((\d+)\)", term)
        if m:
            # headers.h: SWAP_PAL(x) = x < NUM_PALS_IN_PRIMARY ? 1 << x : 1 << (x - NUM_PALS_IN_PRIMARY)
            n = int(m.group(1))
            value |= 1 << (n if n < EM_PRIMARY_PALS else n - EM_PRIMARY_PALS)
        else:
            value |= int(term, 0)
    return value


def parse_tileset_headers(src):
    headers = {}
    for name, body in re.findall(r"const struct Tileset (\w+)\s*=\s*\{(.*?)\};", src.text(HEADERS_H), re.S):
        fields = dict(re.findall(r"\.(\w+)\s*=\s*([^\n]+?),\s*\n", body))
        headers[name] = {
            "isCompressed": fields.get("isCompressed", "FALSE").strip() == "TRUE",
            "isSecondary": fields.get("isSecondary", "FALSE").strip() == "TRUE",
            "swapPalettes": eval_pal_bits(fields.get("swapPalettes", "0")),
            "lightPalettes": eval_pal_bits(fields.get("lightPalettes", "0")),
            "customLightColor": eval_pal_bits(fields.get("customLightColor", "0")),
            "tiles": fields["tiles"].strip(),
            "palettes": fields["palettes"].strip(),
            "metatiles": fields["metatiles"].strip(),
            "metatileAttributes": fields["metatileAttributes"].strip(),
            "callback": fields.get("callback", "NULL").strip(),
        }
    return headers


def parse_graphics(src):
    text = src.text(GRAPHICS_H)
    tiles = {}
    for sym, path, ext in re.findall(r'const u32 (?:ALIGNED\(\d+\) )?(\w+)\[\]\s*=\s*INCGFX_U32\("([^"]+)",\s*"([^"]+)"\)', text):
        tiles[sym] = {"path": path, "ext": ext}
    palettes = {}
    for sym, body in re.findall(r"const u16 (?:ALIGNED\(\d+\) )?(\w+)\[\]\[16\]\s*=\s*\{(.*?)\};", text, re.S):
        palettes[sym] = re.findall(r'INCGFX_U16\("([^"]+)"', body)
    return tiles, palettes


def parse_metatiles(src):
    return dict(re.findall(r'const u16 (\w+)\[\]\s*=\s*INCBIN_U16\("([^"]+)"\)', src.text(METATILES_H)))


def tileset_files(name, headers, gfx_tiles, gfx_pals, metas):
    h = headers[name]
    if h["tiles"] not in gfx_tiles or h["palettes"] not in gfx_pals:
        raise ConvertError(f"{name}: tiles/palettes symbol not found in {GRAPHICS_H}")
    pals = gfx_pals[h["palettes"]]
    if len(pals) != 16:
        raise ConvertError(f"{name}: expected 16 palettes, found {len(pals)}")
    return {
        "tiles": gfx_tiles[h["tiles"]]["path"],
        "tiles_ext": gfx_tiles[h["tiles"]]["ext"],
        "palettes": pals,
        "metatiles": metas[h["metatiles"]],
        "attributes": metas[h["metatileAttributes"]],
    }


def parse_pal(text):
    lines = text.split()
    if lines[0] != "JASC-PAL":
        raise ConvertError("palette is not JASC-PAL")
    count = int(lines[2])
    vals = list(map(int, lines[3:3 + count * 3]))
    return tuple(tuple(vals[i:i + 3]) for i in range(0, len(vals), 3))


def u16s(data):
    return [int.from_bytes(data[i:i + 2], "little") for i in range(0, len(data), 2)]


def u32s(data):
    return [int.from_bytes(data[i:i + 4], "little") for i in range(0, len(data), 4)]


# --------------------------------------------------------------------------
# Layout selection
# --------------------------------------------------------------------------

def text_references(names):
    """Return the subset of names referenced in tracked src/, include/, data/ files (excluding map/layout JSON)."""
    found = set()
    if not names:
        return found
    pattern = re.compile(r"\b(" + "|".join(map(re.escape, names)) + r")\b")
    tracked = subprocess.run(["git", "-C", REPO_ROOT, "ls-files", "src", "include", "data"],
                             check=True, capture_output=True, text=True).stdout.split("\n")
    for rel in tracked:
        if not rel.endswith((".c", ".h", ".inc", ".s", ".json", ".pory")):
            continue
        if rel == LAYOUTS_JSON or (rel.startswith("data/maps/") and rel.endswith("/map.json")):
            continue
        with open(os.path.join(REPO_ROOT, rel), encoding="utf-8", errors="replace") as f:
            found.update(pattern.findall(f.read()))
    return found


def select_layouts(src, headers):
    layouts = [l for l in src.json(LAYOUTS_JSON)["layouts"] if l.get("layout_version") == "frlg"]
    groups = src.json(MAP_GROUPS_JSON)
    users = collections.defaultdict(list)
    for grp in groups["group_order"]:
        for m in groups[grp]:
            mj = src.json(f"data/maps/{m}/map.json")
            dropped = grp in DROP_MAP_GROUPS or bool(DROP_MAP_RE.match(m))
            users[mj["layout"]].append((m, dropped))

    map_dropped, unreferenced = set(), set()
    no_users = [l["id"] for l in layouts if l["id"] not in users]
    refs = text_references(no_users)
    for l in layouts:
        u = users.get(l["id"])
        if u and all(d for _, d in u):
            map_dropped.add(l["name"])
        elif not u and l["id"] not in refs:
            unreferenced.add(l["name"])

    pairs = collections.defaultdict(list)
    for l in layouts:
        pairs[(l["primary_tileset"], l["secondary_tileset"])].append(l)

    kept_pairs, skipped_pairs = {}, {}
    for pair, ls in pairs.items():
        if all(l["name"] in map_dropped or l["name"] in unreferenced for l in ls):
            skipped_pairs[pair] = ls
        else:
            kept_pairs[pair] = ls
    for p, s in kept_pairs:
        if p not in PRIMARY_RENAMES:
            raise ConvertError(f"unexpected FRLG primary {p}")
        if not headers[s]["isSecondary"]:
            raise ConvertError(f"{s} is not a secondary tileset")
    return kept_pairs, skipped_pairs, map_dropped, unreferenced


# --------------------------------------------------------------------------
# Conversion
# --------------------------------------------------------------------------

def convert_attr(a, where, stats):
    behavior = a & 0x1FF
    layer = (a >> 29) & 3
    if behavior > 0xFF:
        raise ConvertError(f"{where}: behavior 0x{behavior:X} does not fit 8 bits")
    if a & ~(0x1FF | (3 << 29)):
        stats["attr_bits_dropped"].add(where)
    return (layer << 12) | behavior


def pack_u16(vals):
    return b"".join(v.to_bytes(2, "little") for v in vals)


def variant_dir(path_dir, primary):
    _, short = PRIMARY_VARIANT_SUFFIX[primary]
    base = os.path.basename(path_dir)
    stem = base[:-len("_frlg")] if base.endswith("_frlg") else base
    return os.path.join(os.path.dirname(path_dir), f"{stem}_{short}_frlg")


def variant_symbol(sym, primary):
    cap, _ = PRIMARY_VARIANT_SUFFIX[primary]
    return sym + cap


class Writer:
    """Collects output files; refuses to write one path with two contents."""

    def __init__(self):
        self.files = {}
        self.copies = {}

    def put(self, path, data):
        if path in self.files and self.files[path] != data:
            raise ConvertError(f"conflicting output for {path}")
        self.files[path] = data

    def copy_dir_extras(self, src_dir, dst_dir, skip):
        self.copies[(src_dir, dst_dir)] = set(skip)

    def flush(self, out_root):
        for (src_dir, dst_dir), skip in self.copies.items():
            src_abs = os.path.join(REPO_ROOT, src_dir)
            dst_abs = os.path.join(out_root, dst_dir)
            if os.path.abspath(src_abs) == os.path.abspath(dst_abs):
                continue
            for dirpath, _, files in os.walk(src_abs):
                for fn in files:
                    rel = os.path.relpath(os.path.join(dirpath, fn), src_abs)
                    if os.path.join(src_dir, rel) in skip:
                        continue
                    target = os.path.join(dst_abs, rel)
                    os.makedirs(os.path.dirname(target), exist_ok=True)
                    shutil.copyfile(os.path.join(dirpath, fn), target)
        for path, data in self.files.items():
            target = os.path.join(out_root, path)
            os.makedirs(os.path.dirname(target), exist_ok=True)
            with open(target, "wb") as f:
                f.write(data)


def load_tileset(src, files):
    tiles, plte = png4.read_tiles(src.read(files["tiles"]))
    meta = src.read(files["metatiles"])
    attrs = u32s(src.read(files["attributes"]))
    return {
        "tiles": tiles,
        "plte": plte,
        "metatiles": [meta[i:i + METATILE_BYTES] for i in range(0, len(meta), METATILE_BYTES)],
        "attrs": attrs,
        "pal_text": [src.read(p) for p in files["palettes"]],
    }


def component_skip(files):
    return [files["tiles"], files["metatiles"], files["attributes"]] + files["palettes"]


def convert(src, out_root, manifest_path):
    headers = parse_tileset_headers(src)
    gfx_tiles, gfx_pals = parse_graphics(src)
    metas = parse_metatiles(src)
    kept_pairs, skipped_pairs, map_dropped, unreferenced = select_layouts(src, headers)

    writer = Writer()
    stats = collections.Counter(attr_bits_dropped=set())
    manifest = {"tilesets": {}, "layouts": {}}
    report = {"primaries": [], "secondaries": [], "skipped": [], "map_dropped": sorted(map_dropped),
              "unreferenced": sorted(unreferenced)}

    def orig_entry(name):
        files = tileset_files(name, headers, gfx_tiles, gfx_pals, metas)
        h = headers[name]
        manifest["tilesets"].setdefault("orig:" + name, dict(files, swapPalettes=h["swapPalettes"],
                                                             format="frlg", isSecondary=h["isSecondary"]))
        return files

    # Primaries
    prim_data = {}
    for p, (new_name, new_dir, sym) in PRIMARY_RENAMES.items():
        if not any(pp == p for pp, _ in kept_pairs):
            continue
        files = orig_entry(p)
        data = load_tileset(src, files)
        if len(data["tiles"]) < FRLG_PRIMARY_TILES or len(data["metatiles"]) < FRLG_PRIMARY_TILES:
            raise ConvertError(f"{p}: primary has fewer than {FRLG_PRIMARY_TILES} tiles/metatiles")
        prim_data[p] = data
        old_dir = os.path.dirname(files["tiles"])
        out = {
            "tiles": f"{new_dir}/tiles.png",
            "tiles_ext": files["tiles_ext"],
            "palettes": [f"{new_dir}/palettes/{i:02d}.pal" for i in range(16)],
            "metatiles": f"{new_dir}/metatiles.bin",
            "attributes": f"{new_dir}/metatile_attributes.bin",
        }
        writer.put(out["tiles"], png4.write_tiles_png(data["tiles"][:EM_PRIMARY_TILES], data["plte"]))
        writer.put(out["metatiles"], b"".join(data["metatiles"][:EM_PRIMARY_TILES]))
        writer.put(out["attributes"], pack_u16(convert_attr(a, f"{p}[{i}]", stats)
                                                for i, a in enumerate(data["attrs"][:EM_PRIMARY_TILES])))
        for i in range(16):
            writer.put(out["palettes"][i], data["pal_text"][i])
        writer.copy_dir_extras(old_dir, new_dir, component_skip(files))
        h = headers[p]
        manifest["tilesets"]["conv:" + new_name] = dict(
            out, format="emerald", isSecondary=False, isCompressed=h["isCompressed"],
            swapPalettes=h["swapPalettes"] & 0x3F, lightPalettes=h["lightPalettes"] & 0x3F,
            customLightColor=h["customLightColor"] & 0x3F, callback=h["callback"],
            symbols={"tiles": f"gTilesetTiles_{sym}", "palettes": f"gTilesetPalettes_{sym}",
                     "metatiles": f"gMetatiles_{sym}", "attributes": f"gMetatileAttributes_{sym}"},
            replaces=p)
        report["primaries"].append({
            "name": new_name, "from": p,
            "orig_bytes": len(data["tiles"]) * TILE_BYTES_4BPP + len(data["metatiles"]) * METATILE_BYTES + len(data["attrs"]) * 4,
            "new_bytes": EM_PRIMARY_TILES * (TILE_BYTES_4BPP + METATILE_BYTES + 2),
        })

    # Secondaries: home primary = the primary most layouts pair it with.
    sec_primaries = collections.defaultdict(collections.Counter)
    for (p, s), ls in kept_pairs.items():
        sec_primaries[s][p] += len(ls)

    for (p, s), ls in sorted(kept_pairs.items(), key=lambda kv: (kv[0][1], kv[0][0])):
        home = sec_primaries[s].most_common(1)[0][0]
        files = orig_entry(s)
        data = load_tileset(src, files)
        pdata = prim_data[p]
        h, ph = headers[s], headers[p]

        n_s_tiles = len(data["tiles"])
        n_s_meta = len(data["metatiles"])
        if n_s_tiles > FRLG_SECONDARY_CAPACITY or n_s_meta > FRLG_SECONDARY_CAPACITY:
            raise ConvertError(f"({p}, {s}): secondary has {n_s_tiles} tiles / {n_s_meta} metatiles; "
                               f"converted size exceeds {EM_SECONDARY_CAPACITY}")
        if len(data["attrs"]) != n_s_meta:
            raise ConvertError(f"{s}: {len(data['attrs'])} attributes for {n_s_meta} metatiles")

        if p == home:
            new_name = s
            out = {k: files[k] for k in ("tiles", "tiles_ext", "metatiles", "attributes", "palettes")}
            symbols = {"tiles": h["tiles"], "palettes": h["palettes"],
                       "metatiles": h["metatiles"], "attributes": h["metatileAttributes"]}
        else:
            new_name = variant_symbol(s, p)

            def vpath(path):
                return os.path.join(variant_dir(os.path.dirname(path), p), os.path.basename(path)) \
                    if "/palettes/" not in path else \
                    os.path.join(variant_dir(os.path.dirname(os.path.dirname(path)), p), "palettes",
                                 os.path.basename(path))
            out = {"tiles": vpath(files["tiles"]), "tiles_ext": files["tiles_ext"],
                   "metatiles": vpath(files["metatiles"]), "attributes": vpath(files["attributes"]),
                   "palettes": [vpath(x) for x in files["palettes"]]}
            symbols = {"tiles": variant_symbol(h["tiles"], p), "palettes": variant_symbol(h["palettes"], p),
                       "metatiles": variant_symbol(h["metatiles"], p),
                       "attributes": variant_symbol(h["metatileAttributes"], p)}

        tiles = pdata["tiles"][EM_PRIMARY_TILES:FRLG_PRIMARY_TILES] + data["tiles"]
        metatiles = pdata["metatiles"][EM_PRIMARY_TILES:FRLG_PRIMARY_TILES] + data["metatiles"]
        attrs = [convert_attr(a, f"{p}[{EM_PRIMARY_TILES + i}]", stats)
                 for i, a in enumerate(pdata["attrs"][EM_PRIMARY_TILES:FRLG_PRIMARY_TILES])]
        attrs += [convert_attr(a, f"{s}[{i}]", stats) for i, a in enumerate(data["attrs"])]

        pal_text = list(data["pal_text"])
        pal_text[EM_PRIMARY_PALS] = pdata["pal_text"][EM_PRIMARY_PALS]
        # Night alternate of slot i is palette (i + 9) % 16 of the same tileset.
        if (ph["swapPalettes"] >> EM_PRIMARY_PALS) & 1:
            alt = (EM_PRIMARY_PALS + 9) % 16
            pal_text[alt] = pdata["pal_text"][alt]

        writer.put(out["tiles"], png4.write_tiles_png(tiles, data["plte"]))
        writer.put(out["metatiles"], b"".join(metatiles))
        writer.put(out["attributes"], pack_u16(attrs))
        for i in range(16):
            writer.put(out["palettes"][i], pal_text[i])
        for k in ("tiles", "metatiles", "attributes"):
            writer.copy_dir_extras(os.path.dirname(files[k]), os.path.dirname(out[k]), component_skip(files))

        def shift(bits, prim_bits, width):
            return ((bits << 1) | ((prim_bits >> EM_PRIMARY_PALS) & 1)) & ((1 << width) - 1)

        swap = shift(h["swapPalettes"], ph["swapPalettes"], 7)
        light = shift(h["lightPalettes"], ph["lightPalettes"], 8)
        custom = shift(h["customLightColor"], ph["customLightColor"], 8)
        if h["swapPalettes"] or h["lightPalettes"] or h["customLightColor"] or ph["swapPalettes"] \
                or ph["lightPalettes"] or ph["customLightColor"]:
            stats["headers_with_palette_bits"] += 1
        manifest["tilesets"]["conv:" + new_name] = dict(
            out, format="emerald", isSecondary=True, isCompressed=h["isCompressed"],
            swapPalettes=swap, lightPalettes=light, customLightColor=custom, callback=h["callback"],
            symbols=symbols, replaces=s, primary=PRIMARY_RENAMES[p][0])

        for l in ls:
            manifest["layouts"][l["name"]] = {
                "id": l["id"],
                "width": l["width"], "height": l["height"],
                "blockdata": l["blockdata_filepath"], "border": l["border_filepath"],
                "orig": {"primary": "orig:" + p, "secondary": "orig:" + s},
                "conv": {"primary": "conv:" + PRIMARY_RENAMES[p][0], "secondary": "conv:" + new_name},
            }

        report["secondaries"].append({
            "name": new_name, "from": s, "primary": p, "layouts": len(ls),
            "tiles": len(tiles), "metatiles": len(metatiles),
            "orig_bytes": n_s_tiles * TILE_BYTES_4BPP + n_s_meta * (METATILE_BYTES + 4),
            "new_bytes": len(tiles) * TILE_BYTES_4BPP + len(metatiles) * (METATILE_BYTES + 2),
        })

    for (p, s), ls in sorted(skipped_pairs.items()):
        report["skipped"].append({"primary": p, "secondary": s, "layouts": sorted(l["name"] for l in ls)})

    writer.flush(out_root)
    os.makedirs(os.path.dirname(os.path.abspath(manifest_path)), exist_ok=True)
    with open(manifest_path, "w") as f:
        json.dump(manifest, f, indent=1, sort_keys=True)
    return report, stats


def print_report(report, stats):
    print("== Dropped layouts (only used by dropped maps) ==")
    for n in report["map_dropped"]:
        print("  " + n)
    print("== Unreferenced layouts (no map, no text reference) ==")
    for n in report["unreferenced"]:
        print("  " + n)
    print("== Skipped pairs (every layout dropped or unreferenced) ==")
    for sk in report["skipped"]:
        print(f"  ({sk['primary']}, {sk['secondary']}): {', '.join(sk['layouts'])}")

    print("\n== Size report (uncompressed bytes: 4bpp tiles + metatiles + attributes) ==")
    tot_orig = tot_new = 0
    for pr in report["primaries"]:
        print(f"  {pr['name']:42s} {pr['orig_bytes']:7d} -> {pr['new_bytes']:7d} ({pr['new_bytes'] - pr['orig_bytes']:+d})")
        tot_orig += pr["orig_bytes"]
        tot_new += pr["new_bytes"]
    seen_from = set()
    for se in report["secondaries"]:
        orig = se["orig_bytes"] if se["from"] not in seen_from else 0
        seen_from.add(se["from"])
        print(f"  {se['name']:42s} {orig:7d} -> {se['new_bytes']:7d} ({se['new_bytes'] - orig:+d})"
              f"  tiles {se['tiles']:3d} metatiles {se['metatiles']:3d}  [{se['primary']}, {se['layouts']} layouts]")
        tot_orig += orig
        tot_new += se["new_bytes"]
    n_sec = len(report["secondaries"])
    dup = n_sec * DUP_COUNT * (TILE_BYTES_4BPP + METATILE_BYTES + 2)
    print(f"\n  secondaries written: {n_sec}")
    print(f"  duplicated primary data: {n_sec} x {DUP_COUNT} tiles/metatiles = {dup} bytes "
          f"({n_sec * DUP_COUNT * TILE_BYTES_4BPP} tiles, {n_sec * DUP_COUNT * METATILE_BYTES} metatiles, "
          f"{n_sec * DUP_COUNT * 2} attributes)")
    print(f"  total: {tot_orig} -> {tot_new} ({tot_new - tot_orig:+d})")
    print(f"\n  attributes with dropped terrain/encounter bits: {len(stats['attr_bits_dropped'])} (unique source entries)")
    print(f"  pairs with palette swap/light bits: {stats['headers_with_palette_bits']}")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--out", required=True, help="output root; files are written at repo-relative paths under it")
    ap.add_argument("--manifest", help="manifest path (default <out>/manifest.json)")
    args = ap.parse_args()
    manifest = args.manifest or os.path.join(args.out, "manifest.json")
    try:
        report, stats = convert(Source(), os.path.abspath(args.out), manifest)
    except (ConvertError, png4.PngError) as e:
        print(f"error: {e}", file=sys.stderr)
        return 1
    print_report(report, stats)
    print(f"\nmanifest: {manifest}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
