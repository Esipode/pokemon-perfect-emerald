#!/usr/bin/env python3
"""Prove converted Johto tilesets render every Johto layout identically.

For each layout in the manifest written by convert_tilesets.py, every metatile
id used in map.bin and border.bin is resolved twice:

  * HnS rules on the original data: primary split at 640, palettes 0-6 from
    the primary, 2-byte attributes.
  * Emerald rules on the converted data: split at 512, palettes 0-5 from the
    primary, 2-byte attributes.

Each of the 8 tile entries is compared as flip bits plus 64 RGB pixels (tile
indices mapped through the palette slot). Behavior, layer and night-swap
palettes are compared too. Door metatiles and tileset animation destinations
are checked against the converted layouts (--hns clone; skipped with --no-code).

Usage: verify_tilesets.py [converted root] [--hns <clone>] [--no-code]
"""

import argparse
import importlib.util
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, "..", "frlg_convert"))
import png4  # noqa: E402
from common import OUT_DIR, add_hns_arg, read  # noqa: E402

_spec = importlib.util.spec_from_file_location(
    "frlg_verify_tilesets", os.path.join(HERE, "..", "frlg_convert", "verify_tilesets.py"))
_v = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_v)
parse_pal, u16s = _v.parse_pal, _v.u16s
HNS_PRIMARY_TILES, HNS_PRIMARY_PALS = 640, 7
EM_PRIMARY_TILES = _v.EM_PRIMARY_TILES
MAX_LISTED_PER_LAYOUT = 10


class FileSource:
    def __init__(self, root):
        self.root = root

    def read(self, path):
        with open(os.path.join(self.root, path), "rb") as f:
            return f.read()

    def text(self, path):
        return self.read(path).decode("utf-8")


class Tileset:
    def __init__(self, src, entry):
        self.swap = entry.get("swapPalettes", 0)
        self.tiles, _ = png4.read_tiles(src.read(entry["tiles"]))
        meta = u16s(src.read(entry["metatiles"]))
        self.metatiles = [meta[i:i + 8] for i in range(0, len(meta), 8)]
        self.attrs = u16s(src.read(entry["attributes"]))
        self.pals = [parse_pal(src.text(p)) for p in entry["palettes"]]
        if len(self.attrs) != len(self.metatiles):
            raise SystemExit(f"error: {entry['metatiles']}: attribute count does not match metatile count")


class HnsModel(_v.Model):
    """HnS view: FRLG geometry (640 / 7 palettes) with Emerald 2-byte attributes."""

    def __init__(self, primary, secondary):
        super().__init__(primary, secondary, frlg=True)

    def _attr(self, a):
        return a & 0xFF, (a >> 12) & 0xF


def describe(a, b):
    return _v.describe(a, b)


def used_metatiles(src, layout):
    ids = set()
    for path in (layout["blockdata"], layout["border"]):
        ids.update(v & 0x3FF for v in u16s(src.read(path)))
    return sorted(ids)


# --------------------------------------------------------------------------
# Door and animation checks (read the HnS C source)
# --------------------------------------------------------------------------

def metatile_labels(hns):
    return {k: int(v, 0) for k, v in re.findall(r"#define\s+(METATILE_\w+)\s+(0x[0-9A-Fa-f]+|\d+)",
                                                read(os.path.join(hns, "include/constants/metatile_labels.h")))}


def check_doors(hns, manifest, used_by_tileset):
    """Door table rows keyed on a Johto tileset: which converted tileset(s) now hold the metatile."""
    labels = metatile_labels(hns)
    text = read(os.path.join(hns, "src/field_door.c"))
    body = text[text.index("sDoorAnimGraphicsTable"):]
    rows = re.findall(r"\{\s*(METATILE_\w+),\s*&(gTileset_\w+),", body)
    orig_to_conv = {}  # HnS tileset -> converted tilesets that replace/follow it
    for key, ts in manifest["tilesets"].items():
        if key.startswith("conv:"):
            orig_to_conv.setdefault(ts["replaces"], []).append((key[5:], ts))
    problems, relocated, checked = [], [], 0
    for label, tileset in rows:
        if tileset not in orig_to_conv:
            continue  # tileset not part of the Johto set
        if label not in labels:
            problems.append(f"{label}: label not found")
            continue
        mid = labels[label]
        checked += 1
        if mid >= HNS_PRIMARY_TILES + 384:
            problems.append(f"{label}: metatile {mid:#x} outside the 1024 range")
            continue
        for name, ts in orig_to_conv[tileset]:
            if mid < EM_PRIMARY_TILES:
                continue
            if not ts["isSecondary"] and mid < HNS_PRIMARY_TILES:
                # Door in primary 512-639 now lives in every secondary paired with this primary.
                holders = sorted(n for n, t in ((k[5:], v) for k, v in manifest["tilesets"].items() if k.startswith("conv:"))
                                 if t["isSecondary"] and t["primary"] == name)
                relocated.append((label, tileset, mid, holders))
    return checked, problems, relocated


ANIM_CALLBACKS = ("InitTilesetAnim_JohtoGeneral", "InitTilesetAnim_EcruteakTheater", "InitTilesetAnim_AzaleaTownGym")


def check_anims(hns, manifest):
    """Destination tile ranges of the three Johto callbacks, mapped to converted indices."""
    text = read(os.path.join(hns, "src/tileset_anims.c"))
    out = []
    for cb in ANIM_CALLBACKS:
        stem = cb.replace("InitTilesetAnim_", "")
        # Queue functions of this tileset: QueueAnimTiles_<stem>_*
        for fn, body in re.findall(r"static void (QueueAnimTiles_%s_\w+)\(u16 timer\)\s*\{(.*?)\n\}" % stem, text, re.S):
            for kind, off, size in re.findall(
                    r"TILE_OFFSET_4BPP\((sSecondaryTilesetBaseTile \+ )?(\d+)\)\),\s*([^)]*?)\);", body):
                out.append((cb, fn, bool(kind), int(off), size.strip()))
    return out


def eval_size(expr):
    expr = expr.replace("TILE_SIZE_4BPP", "32")
    return eval(expr, {"__builtins__": {}}) // 32 if re.fullmatch(r"[\d\s*x+A-Fa-f]+", expr) else None


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("converted_root", nargs="?", default=os.path.join(OUT_DIR, "tilesets"))
    add_hns_arg(ap)
    ap.add_argument("--manifest", help="manifest path (default <converted_root>/manifest.json)")
    ap.add_argument("--no-code", action="store_true", help="skip the door/animation checks")
    args = ap.parse_args()

    manifest_path = args.manifest or os.path.join(args.converted_root, "manifest.json")
    with open(manifest_path) as f:
        manifest = json.load(f)
    orig_src = FileSource(args.hns)
    conv_src = FileSource(os.path.abspath(args.converted_root))

    tilesets = {}

    def get(key):
        if key not in tilesets:
            entry = manifest["tilesets"][key]
            tilesets[key] = Tileset(orig_src if entry["format"] == "hns" else conv_src, entry)
        return tilesets[key]

    models = {}
    total = total_ids = failed_layouts = 0
    for name, layout in sorted(manifest["layouts"].items()):
        okey = (layout["orig"]["primary"], layout["orig"]["secondary"])
        ckey = (layout["conv"]["primary"], layout["conv"]["secondary"])
        if okey not in models:
            models[okey] = HnsModel(get(okey[0]), get(okey[1]))
        if ckey not in models:
            models[ckey] = _v.Model(get(ckey[0]), get(ckey[1]), frlg=False)
        hm, em = models[okey], models[ckey]

        mismatches = []
        if any(hm.swap) or any(em.swap):
            mismatches.append("night-swap palettes present; HnS headers define none")
        ids = used_metatiles(orig_src, layout)
        total_ids += len(ids)
        for mid in ids:
            a, b = hm.metatile(mid), em.metatile(mid)
            if a is None:
                mismatches.append(f"metatile {mid:#05x}: undefined in HnS (map uses it)")
            elif a != b:
                mismatches.append(f"metatile {mid:#05x}: {describe(a, b)}")
        if mismatches:
            failed_layouts += 1
            total += len(mismatches)
            print(f"{name}: {len(mismatches)} mismatch(es)")
            for m in mismatches[:MAX_LISTED_PER_LAYOUT]:
                print("    " + m)
            if len(mismatches) > MAX_LISTED_PER_LAYOUT:
                print(f"    ... {len(mismatches) - MAX_LISTED_PER_LAYOUT} more")

    print(f"layouts checked: {len(manifest['layouts'])}, metatile ids checked: {total_ids}, "
          f"layouts with mismatches: {failed_layouts}, total mismatches: {total}")

    if not args.no_code:
        checked, problems, relocated = check_doors(args.hns, manifest, None)
        print(f"\ndoor rows on Johto tilesets: {checked}, problems: {len(problems)}")
        for p in problems:
            print("  " + p)
        print(f"door rows whose metatile moved into secondaries (primary id 512-639): {len(relocated)}")
        for label, ts, mid, holders in relocated:
            print(f"  {label} ({mid:#x}) on {ts}: key on {len(holders)} secondaries")
        anims = check_anims(args.hns, manifest)
        print(f"\nanimation destinations: {len(anims)}")
        for cb, fn, secondary, off, size in anims:
            if secondary:
                print(f"  {fn}: base+{off} -> base+{off + 128} (HnS secondary index {off} = converted index {off + 128})")
            else:
                where = "primary'" if off < EM_PRIMARY_TILES else "secondary' (duplicated 512-639)"
                print(f"  {fn}: absolute tile {off} stays in {where}")
    return 1 if total else 0


if __name__ == "__main__":
    sys.exit(main())
