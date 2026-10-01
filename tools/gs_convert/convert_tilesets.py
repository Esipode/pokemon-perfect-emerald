#!/usr/bin/env python3
"""Convert HnS Johto tilesets to Emerald format without changing VRAM contents.

HnS primaries hold 640 tiles / 640 metatiles / palettes 0-6 with 2-byte
(Emerald-format) attributes. Emerald primaries hold 512 / 512 / 0-5.
For each (primary P, secondary S) pair used by a Johto layout:

  * primary  P' = P tiles 0-511, metatiles 0-511, attributes 0-511.
  * secondary S' = P tiles 512-639 + S tiles, P metatiles 512-639 + S
    metatiles (attributes likewise), S palettes with slot 6 taken from P.

A secondary paired with more than one primary gets one copy per extra
primary. Writes the converted files under --out (target-relative paths) and
a manifest consumed by verify_tilesets.py. No repo files are touched.

Usage: convert_tilesets.py --out <dir> [--manifest <path>] [--hns <clone>]
"""

import argparse
import collections
import json
import os
import re
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, "..", "frlg_convert"))
import importlib.util  # noqa: E402
import png4  # noqa: E402
from common import OUT_DIR, add_hns_arg, johto_set, load_json, read  # noqa: E402

# The Kanto converter shares this file name; load it by path under another module name.
_spec = importlib.util.spec_from_file_location(
    "frlg_convert_tilesets", os.path.join(HERE, "..", "frlg_convert", "convert_tilesets.py"))
_frlg = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_frlg)
ConvertError, u16s = _frlg.ConvertError, _frlg.u16s
EM_PRIMARY_TILES, EM_PRIMARY_PALS = _frlg.EM_PRIMARY_TILES, _frlg.EM_PRIMARY_PALS
EM_SECONDARY_CAPACITY = _frlg.EM_SECONDARY_CAPACITY
METATILE_BYTES, TILE_BYTES_4BPP = _frlg.METATILE_BYTES, _frlg.TILE_BYTES_4BPP

HNS_PRIMARY_TILES = 640
HNS_PRIMARY_PALS = 7
HNS_SECONDARY_CAPACITY = 1024 - HNS_PRIMARY_TILES  # 384
DUP_COUNT = HNS_PRIMARY_TILES - EM_PRIMARY_TILES  # 128

# HnS primary symbol -> (target symbol, short name used in variant names). Locked decision 8.
PRIMARIES = {
    "gTileset_Johto_General_Hns": ("gTileset_JohtoGeneral", "General"),
    "gTileset_Johto_Building_Hns": ("gTileset_JohtoBuilding", "Building"),
    "gTileset_Johto_NorthEast_Hns": ("gTileset_JohtoNorthEast", "NorthEast"),
    "gTileset_Johto_NorthWest_Hns": ("gTileset_JohtoNorthWest", "NorthWest"),
    "gTileset_Johto_South_Hns": ("gTileset_JohtoSouth", "South"),
    "gTileset_Kanto_General_Hns": ("gTileset_JohtoKantoGeneral", "KantoGeneral"),
}


def snake(name):
    return re.sub(r"(?<=[a-z0-9])(?=[A-Z])", "_", name).lower()


def parse_headers(hns):
    """HnS tileset symbol -> header fields, plus the tile/palette/metatile paths."""
    headers = read(os.path.join(hns, "src/data/tilesets/headers.h"))
    graphics = read(os.path.join(hns, "src/data/tilesets/graphics.h"))
    meta_src = read(os.path.join(hns, "src/data/tilesets/metatiles.h"))
    tile_paths = dict(re.findall(r'(gTilesetTiles_\w+)\[\]\s*=\s*INC\w+\("([^"]+)"', graphics))
    pal_paths = {sym: re.findall(r'INC\w+\("([^"]+)"', body)
                 for sym, body in re.findall(r"(gTilesetPalettes_\w+)\[\]\[16\]\s*=\s*\{(.*?)\};", graphics, re.S)}
    meta_paths = dict(re.findall(r'(gMetatile\w+)\[\]\s*=\s*INCBIN_U16\("([^"]+)"\)', meta_src))
    out = {}
    for sym, body in re.findall(r"const struct Tileset (gTileset_\w+)\s*=\s*\{(.*?)\};", headers, re.S):
        fields = dict(re.findall(r"\.(\w+)\s*=\s*([^\n]+?),\s*\n", body))
        for k in ("swapPalettes", "lightPalettes", "customLightColor"):
            if k in fields:
                raise ConvertError(f"{sym}: header sets {k}; the converter assumes none")
        if fields["tiles"].strip() not in tile_paths or "_Hns" not in sym:
            continue  # Emerald/FRLG tilesets HnS still carries
        tdir = os.path.dirname(tile_paths[fields["tiles"].strip()])
        pals = [p.replace(".gbapal", ".pal") for p in pal_paths[fields["palettes"].strip()]]
        if len(pals) not in (13, 16):
            raise ConvertError(f"{sym}: expected 13 or 16 palettes, found {len(pals)}")
        out[sym] = {
            "secondary": fields.get("isSecondary", "FALSE").strip() == "TRUE",
            "callback": fields.get("callback", "NULL").strip(),
            "dir": tdir,
            "tiles": tdir + "/tiles.png",
            "palettes": pals,
            "metatiles": meta_paths[fields["metatiles"].strip()],
            "attributes": meta_paths[fields["metatileAttributes"].strip()],
        }
    return out


def johto_layouts(hns):
    """Layout entries (layouts.json) used by the Johto map set, with the maps that use them."""
    table = {l["id"]: l for l in load_json(os.path.join(hns, "data/layouts/layouts.json"))["layouts"] if "id" in l}
    layouts = collections.OrderedDict()
    for _, name, data in johto_set(hns):
        lid = data["layout"]
        layouts.setdefault(lid, dict(table[lid], maps=[]))["maps"].append(name)
    for l in layouts.values():
        if l.get("layout_version") != "hns":
            raise ConvertError(f"{l['id']}: layout_version {l.get('layout_version')!r}, expected 'hns'")
    return list(layouts.values())


def load_tileset(hns, files):
    tiles, plte = png4.read_tiles(open(os.path.join(hns, files["tiles"]), "rb").read())
    meta = open(os.path.join(hns, files["metatiles"]), "rb").read()
    attrs = u16s(open(os.path.join(hns, files["attributes"]), "rb").read())
    return {
        "tiles": tiles, "plte": plte,
        "metatiles": [meta[i:i + METATILE_BYTES] for i in range(0, len(meta), METATILE_BYTES)],
        "attrs": attrs,
        "pal_text": [open(os.path.join(hns, p), "rb").read() for p in files["palettes"]],
    }


def pack_u16(vals):
    return b"".join(v.to_bytes(2, "little") for v in vals)


def new_secondary_name(sym):
    """gTileset_Blackthorn_Hns -> Blackthorn."""
    m = re.fullmatch(r"gTileset_(\w+?)_Hns", sym)
    if not m:
        raise ConvertError(f"unexpected HnS secondary symbol {sym}")
    return m.group(1)


def extras(hns, src_dir, files):
    """Files in a tileset directory other than the converted components (anim frames)."""
    skip = {files["tiles"], files["metatiles"], files["attributes"], *files["palettes"]}
    out = []
    root = os.path.join(hns, src_dir)
    for dirpath, _, names in os.walk(root):
        for n in names:
            rel = os.path.relpath(os.path.join(dirpath, n), root)
            if os.path.join(src_dir, rel) not in skip:
                out.append(rel)
    return sorted(out)


def convert(hns, out_root, manifest_path):
    headers = parse_headers(hns)
    layouts = johto_layouts(hns)
    pairs = collections.defaultdict(list)
    for l in layouts:
        pairs[(l["primary_tileset"], l["secondary_tileset"])].append(l)
    for p, s in pairs:
        if p not in PRIMARIES:
            raise ConvertError(f"unexpected primary {p}")
        if not headers[s]["secondary"]:
            raise ConvertError(f"{s} is not a secondary tileset")

    manifest = {"tilesets": {}, "layouts": {}}
    report = {"primaries": [], "secondaries": [], "capacity_violations": [], "truncated": [], "extra_files": {}}
    files_out = {}  # target path -> bytes
    copies = []     # (source abs path, target path)

    def orig_entry(sym):
        h = headers[sym]
        manifest["tilesets"].setdefault("orig:" + sym, {
            "tiles": h["tiles"], "palettes": h["palettes"], "metatiles": h["metatiles"],
            "attributes": h["attributes"], "format": "hns", "isSecondary": h["secondary"], "swapPalettes": 0})
        return h

    def put(path, data):
        if path in files_out and files_out[path] != data:
            raise ConvertError(f"conflicting output for {path}")
        files_out[path] = data

    # Primaries
    prim_data = {}
    for p, (new_sym, short) in PRIMARIES.items():
        if not any(pp == p for pp, _ in pairs):
            continue
        h = orig_entry(p)
        data = load_tileset(hns, h)
        if len(data["tiles"]) < HNS_PRIMARY_TILES or len(data["metatiles"]) < HNS_PRIMARY_TILES:
            raise ConvertError(f"{p}: fewer than {HNS_PRIMARY_TILES} tiles/metatiles")
        if len(data["attrs"]) != len(data["metatiles"]):
            raise ConvertError(f"{p}: attribute count differs from metatile count")
        prim_data[p] = data
        base = new_sym[len("gTileset_"):]
        new_dir = f"data/tilesets/primary/{snake(base)}"
        out = {"tiles": f"{new_dir}/tiles.png", "metatiles": f"{new_dir}/metatiles.bin",
               "attributes": f"{new_dir}/metatile_attributes.bin",
               "palettes": [f"{new_dir}/palettes/{i:02d}.pal" for i in range(len(h["palettes"]))]}
        put(out["tiles"], png4.write_tiles_png(data["tiles"][:EM_PRIMARY_TILES], data["plte"]))
        put(out["metatiles"], b"".join(data["metatiles"][:EM_PRIMARY_TILES]))
        put(out["attributes"], pack_u16(data["attrs"][:EM_PRIMARY_TILES]))
        for i in range(len(h["palettes"])):
            put(out["palettes"][i], data["pal_text"][i])
        ex = extras(hns, h["dir"], h)
        for rel in ex:
            copies.append((os.path.join(hns, h["dir"], rel), f"{new_dir}/{rel}"))
        report["extra_files"][new_sym] = ex
        manifest["tilesets"]["conv:" + new_sym] = dict(
            out, format="emerald", isSecondary=False, swapPalettes=0, callback=h["callback"],
            symbols={"tiles": f"gTilesetTiles_{base}", "palettes": f"gTilesetPalettes_{base}",
                     "metatiles": f"gMetatiles_{base}", "attributes": f"gMetatileAttributes_{base}"},
            replaces=p)
        report["primaries"].append({
            "name": new_sym, "from": p, "layouts": sum(len(ls) for (pp, _), ls in pairs.items() if pp == p),
            "orig_bytes": len(data["tiles"]) * TILE_BYTES_4BPP + len(data["metatiles"]) * (METATILE_BYTES + 2),
            "new_bytes": EM_PRIMARY_TILES * (TILE_BYTES_4BPP + METATILE_BYTES + 2)})

    # Secondaries: home primary = the one most layouts pair with.
    sec_primaries = collections.defaultdict(collections.Counter)
    for (p, s), ls in pairs.items():
        sec_primaries[s][p] += len(ls)

    for (p, s), ls in sorted(pairs.items(), key=lambda kv: (kv[0][1], kv[0][0])):
        home = sec_primaries[s].most_common(1)[0][0]
        h = orig_entry(s)
        data = load_tileset(hns, h)
        pdata = prim_data[p]
        n_s_tiles, n_s_meta = len(data["tiles"]), len(data["metatiles"])
        if len(data["attrs"]) != n_s_meta:
            raise ConvertError(f"{s}: attribute count differs from metatile count")
        if n_s_meta > HNS_SECONDARY_CAPACITY:
            report["capacity_violations"].append(f"({p}, {s}): {n_s_meta} metatiles "
                                                 f"exceed HnS capacity {HNS_SECONDARY_CAPACITY}")
        if n_s_tiles > HNS_SECONDARY_CAPACITY:
            # Tiles past VRAM tile 1023 are unreachable. Drop them only if no metatile points at them.
            used = {e & 0x3FF for mt in data["metatiles"] for e in u16s(mt)}
            if max(used) > 1023:
                raise ConvertError(f"{s}: metatiles reference tile past VRAM end")
            report["truncated"].append(f"{s}: {n_s_tiles} tiles in PNG, {HNS_SECONDARY_CAPACITY} reachable "
                                       f"(highest referenced VRAM tile {max(used)}); extra "
                                       f"{n_s_tiles - HNS_SECONDARY_CAPACITY} dropped")
            data["tiles"] = data["tiles"][:HNS_SECONDARY_CAPACITY]
            n_s_tiles = HNS_SECONDARY_CAPACITY

        base = new_secondary_name(s)
        short = PRIMARIES[p][1]
        if p == home:
            new_sym = f"gTileset_{base}_Johto"
            res = f"{base}_Johto"
            new_dir = f"data/tilesets/secondary/{snake(base)}_johto"
        else:
            new_sym = f"gTileset_{base}_{short}_Johto"
            res = f"{base}_{short}_Johto"
            new_dir = f"data/tilesets/secondary/{snake(base)}_{snake(short)}_johto"
        out = {"tiles": f"{new_dir}/tiles.png", "metatiles": f"{new_dir}/metatiles.bin",
               "attributes": f"{new_dir}/metatile_attributes.bin",
               "palettes": [f"{new_dir}/palettes/{i:02d}.pal" for i in range(len(h["palettes"]))]}

        tiles = pdata["tiles"][EM_PRIMARY_TILES:HNS_PRIMARY_TILES] + data["tiles"]
        metatiles = pdata["metatiles"][EM_PRIMARY_TILES:HNS_PRIMARY_TILES] + data["metatiles"]
        attrs = pdata["attrs"][EM_PRIMARY_TILES:HNS_PRIMARY_TILES] + data["attrs"]
        if len(tiles) > EM_SECONDARY_CAPACITY or len(metatiles) > EM_SECONDARY_CAPACITY:
            report["capacity_violations"].append(f"{new_sym}: {len(tiles)} tiles / {len(metatiles)} metatiles "
                                                 f"exceed {EM_SECONDARY_CAPACITY}")
        pal_text = list(data["pal_text"])
        pal_text[EM_PRIMARY_PALS] = pdata["pal_text"][EM_PRIMARY_PALS]

        put(out["tiles"], png4.write_tiles_png(tiles, data["plte"]))
        put(out["metatiles"], b"".join(metatiles))
        put(out["attributes"], pack_u16(attrs))
        for i in range(len(h["palettes"])):
            put(out["palettes"][i], pal_text[i])
        if p == home:
            ex = extras(hns, h["dir"], h)
            for rel in ex:
                copies.append((os.path.join(hns, h["dir"], rel), f"{new_dir}/{rel}"))
            report["extra_files"][new_sym] = ex

        manifest["tilesets"]["conv:" + res] = dict(
            out, format="emerald", isSecondary=True, swapPalettes=0, callback=h["callback"],
            symbols={"tiles": f"gTilesetTiles_{res}", "palettes": f"gTilesetPalettes_{res}",
                     "metatiles": f"gMetatiles_{res}", "attributes": f"gMetatileAttributes_{res}"},
            replaces=s, primary=PRIMARIES[p][0], symbol=new_sym)
        for l in ls:
            manifest["layouts"][l["name"]] = {
                "id": l["id"], "maps": l["maps"], "width": l["width"], "height": l["height"],
                "blockdata": l["blockdata_filepath"], "border": l["border_filepath"],
                "orig": {"primary": "orig:" + p, "secondary": "orig:" + s},
                "conv": {"primary": "conv:" + PRIMARIES[p][0], "secondary": "conv:" + res},
            }
        report["secondaries"].append({
            "name": new_sym, "from": s, "primary": p, "home": p == home, "layouts": len(ls),
            "tiles": len(tiles), "metatiles": len(metatiles),
            "orig_bytes": n_s_tiles * TILE_BYTES_4BPP + n_s_meta * (METATILE_BYTES + 2),
            "new_bytes": len(tiles) * TILE_BYTES_4BPP + len(metatiles) * (METATILE_BYTES + 2)})

    for path, data in files_out.items():
        target = os.path.join(out_root, path)
        os.makedirs(os.path.dirname(target), exist_ok=True)
        with open(target, "wb") as f:
            f.write(data)
    for srcp, path in copies:
        target = os.path.join(out_root, path)
        os.makedirs(os.path.dirname(target), exist_ok=True)
        shutil.copyfile(srcp, target)

    os.makedirs(os.path.dirname(os.path.abspath(manifest_path)), exist_ok=True)
    with open(manifest_path, "w") as f:
        json.dump(manifest, f, indent=1, sort_keys=True)
    return report


def print_report(report, layout_count):
    print("== Size report (uncompressed bytes: 4bpp tiles + metatiles + attributes) ==")
    tot_orig = tot_new = 0
    for pr in report["primaries"]:
        print(f"  {pr['name']:44s} {pr['orig_bytes']:7d} -> {pr['new_bytes']:7d} ({pr['new_bytes'] - pr['orig_bytes']:+d})"
              f"  [{pr['layouts']} layouts]")
        tot_orig += pr["orig_bytes"]
        tot_new += pr["new_bytes"]
    seen = set()
    for se in report["secondaries"]:
        orig = se["orig_bytes"] if se["from"] not in seen else 0
        seen.add(se["from"])
        print(f"  {se['name']:44s} {orig:7d} -> {se['new_bytes']:7d} ({se['new_bytes'] - orig:+d})"
              f"  tiles {se['tiles']:3d} metatiles {se['metatiles']:3d}  [{se['layouts']} layouts]")
        tot_orig += orig
        tot_new += se["new_bytes"]
    n_sec = len(report["secondaries"])
    extra = sum(1 for se in report["secondaries"] if not se["home"])
    print(f"\n  primaries written: {len(report['primaries'])}")
    print(f"  secondaries written: {n_sec} ({n_sec - extra} home + {extra} extra primary copies)")
    print(f"  duplicated primary data: {n_sec} x {DUP_COUNT} = {n_sec * DUP_COUNT * (TILE_BYTES_4BPP + METATILE_BYTES + 2)} bytes")
    print(f"  total: {tot_orig} -> {tot_new} ({tot_new - tot_orig:+d})")
    print(f"  layouts covered: {layout_count}")
    print(f"  max secondary': {max(se['tiles'] for se in report['secondaries'])} tiles / "
          f"{max(se['metatiles'] for se in report['secondaries'])} metatiles (capacity {EM_SECONDARY_CAPACITY})")
    print("\n== Capacity violations ==")
    for v in report["capacity_violations"] or ["  none"]:
        print("  " + v if not v.startswith("  ") else v)
    print("\n== Truncated secondaries (unreachable tiles) ==")
    for v in report["truncated"] or ["none"]:
        print("  " + v)
    print("\n== Extra files copied (anim frames) ==")
    for sym, ex in report["extra_files"].items():
        if ex:
            print(f"  {sym}: {len(ex)} files")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    add_hns_arg(ap)
    ap.add_argument("--out", default=os.path.join(OUT_DIR, "tilesets"),
                    help="output root; files are written at target-relative paths under it")
    ap.add_argument("--manifest", help="manifest path (default <out>/manifest.json)")
    args = ap.parse_args()
    manifest = args.manifest or os.path.join(args.out, "manifest.json")
    try:
        report = convert(args.hns, os.path.abspath(args.out), manifest)
    except (ConvertError, png4.PngError) as e:
        print(f"error: {e}", file=sys.stderr)
        return 1
    with open(manifest) as f:
        n_layouts = len(json.load(f)["layouts"])
    print_report(report, n_layouts)
    print(f"\nmanifest: {manifest}")
    return 1 if report["capacity_violations"] else 0


if __name__ == "__main__":
    sys.exit(main())
