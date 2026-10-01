#!/usr/bin/env python3
"""Prove converted Kanto tilesets render every kept FRLG layout identically.

For each layout in the manifest written by convert_tilesets.py, every metatile
id used in map.bin and border.bin is resolved twice:

  * FRLG rules on the original data: primary split at 640, palettes 0-6 from
    the primary, 4-byte attributes.
  * Emerald rules on the converted data: split at 512, palettes 0-5 from the
    primary, 2-byte attributes.

Each of the 8 tile entries is compared as flip bits plus 64 RGB pixels (tile
indices mapped through the palette slot). Behavior, layer and night-swap
palettes are compared too.

Usage: verify_tilesets.py <converted root> [--manifest <path>] [--orig-ref <git ref>]
  <converted root>  directory holding the converted files at repo-relative paths
  --orig-ref        read the original FRLG data from a git ref instead of the working tree
"""

import argparse
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import png4  # noqa: E402
from convert_tilesets import (  # noqa: E402
    REPO_ROOT, Source, parse_pal, u16s, u32s,
    FRLG_PRIMARY_TILES, FRLG_PRIMARY_PALS, EM_PRIMARY_TILES, EM_PRIMARY_PALS,
    TOTAL_TILES, TOTAL_MAP_PALS, METATILE_BYTES,
)
import json  # noqa: E402

MAX_LISTED_PER_LAYOUT = 10


class Tileset:
    def __init__(self, src, entry):
        self.format = entry["format"]
        self.swap = entry.get("swapPalettes", 0)
        self.tiles, _ = png4.read_tiles(src.read(entry["tiles"]))
        meta = u16s(src.read(entry["metatiles"]))
        self.metatiles = [meta[i:i + 8] for i in range(0, len(meta), 8)]
        raw = src.read(entry["attributes"])
        self.attrs = u32s(raw) if self.format == "frlg" else u16s(raw)
        self.pals = [parse_pal(src.text(p)) for p in entry["palettes"]]
        if len(raw) // (4 if self.format == "frlg" else 2) != len(self.metatiles):
            raise SystemExit(f"error: {entry['metatiles']}: attribute count does not match metatile count")


class Model:
    """VRAM/palette view of a (primary, secondary) pair under FRLG or Emerald rules."""

    def __init__(self, primary, secondary, frlg):
        n_tiles = FRLG_PRIMARY_TILES if frlg else EM_PRIMARY_TILES
        n_pals = FRLG_PRIMARY_PALS if frlg else EM_PRIMARY_PALS
        self.frlg = frlg
        self.n_meta = n_tiles
        self.primary, self.secondary = primary, secondary
        self.vram = [primary.tiles[i] if i < len(primary.tiles) else None for i in range(n_tiles)]
        self.vram += [secondary.tiles[i] if i < len(secondary.tiles) else None
                      for i in range(TOTAL_TILES - n_tiles)]
        self.slots = [primary.pals[i] for i in range(n_pals)]
        self.slots += [secondary.pals[i] for i in range(n_pals, TOTAL_MAP_PALS)]
        self.slots += [None] * (16 - TOTAL_MAP_PALS)
        # Night alternate for slot i is palette (i + 9) % 16 of the owning tileset.
        self.swap = []
        for i in range(TOTAL_MAP_PALS):
            owner, bit = (primary, i) if i < n_pals else (secondary, i - n_pals)
            enabled = bool((owner.swap >> bit) & 1) and i != 0
            self.swap.append(owner.pals[(i + 9) % 16] if enabled else None)
        self._cache = {}

    def _attr(self, a):
        if self.frlg:
            return a & 0x1FF, (a >> 29) & 3
        return a & 0xFF, (a >> 12) & 0xF

    def _render(self, entry):
        tile = entry & 0x3FF
        hflip = (entry >> 10) & 1
        vflip = (entry >> 11) & 1
        pal = entry >> 12
        key = entry
        if key in self._cache:
            return self._cache[key]
        pixels = self.vram[tile] if tile < TOTAL_TILES else None
        colors = self.slots[pal]
        if pixels is None:
            r = (hflip, vflip, "undefined tile")
        elif colors is None:
            r = (hflip, vflip, "non-map palette", pal, pixels)
        else:
            r = (hflip, vflip, tuple(colors[p] if p < len(colors) else None for p in pixels))
        self._cache[key] = r
        return r

    def metatile(self, mid):
        if mid < self.n_meta:
            ts, idx = self.primary, mid
        else:
            ts, idx = self.secondary, mid - self.n_meta
        if idx >= len(ts.metatiles):
            return None
        behavior, layer = self._attr(ts.attrs[idx])
        return {
            "tiles": [self._render(e) for e in ts.metatiles[idx]],
            "behavior": behavior,
            "layer": layer,
        }


def describe(a, b):
    if a is None or b is None:
        return f"defined FRLG={a is not None} Emerald={b is not None}"
    out = []
    for k in ("behavior", "layer"):
        if a[k] != b[k]:
            out.append(f"{k} {a[k]:#x} != {b[k]:#x}")
    for i, (ta, tb) in enumerate(zip(a["tiles"], b["tiles"])):
        if ta != tb:
            out.append(f"tile entry {i}")
    return ", ".join(out)


def used_metatiles(src, layout):
    ids = set()
    for path in (layout["blockdata"], layout["border"]):
        ids.update(v & 0x3FF for v in u16s(src.read(path)))
    return sorted(ids)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("converted_root")
    ap.add_argument("--manifest", help="manifest path (default <converted_root>/manifest.json)")
    ap.add_argument("--orig-ref", help="git ref holding the original FRLG data (default: working tree)")
    args = ap.parse_args()

    manifest_path = args.manifest or os.path.join(args.converted_root, "manifest.json")
    with open(manifest_path) as f:
        manifest = json.load(f)
    orig_src = Source(ref=args.orig_ref)
    conv_src = Source(root=os.path.abspath(args.converted_root))

    tilesets = {}

    def get(key):
        if key not in tilesets:
            entry = manifest["tilesets"][key]
            tilesets[key] = Tileset(orig_src if entry["format"] == "frlg" else conv_src, entry)
        return tilesets[key]

    models = {}
    total = 0
    total_ids = 0
    failed_layouts = 0
    for name, layout in sorted(manifest["layouts"].items()):
        okey = (layout["orig"]["primary"], layout["orig"]["secondary"])
        ckey = (layout["conv"]["primary"], layout["conv"]["secondary"])
        if okey not in models:
            models[okey] = Model(get(okey[0]), get(okey[1]), frlg=True)
        if ckey not in models:
            models[ckey] = Model(get(ckey[0]), get(ckey[1]), frlg=False)
        fm, em = models[okey], models[ckey]

        mismatches = []
        swap_diff = [i for i in range(TOTAL_MAP_PALS) if fm.swap[i] != em.swap[i]]
        if swap_diff:
            mismatches.append(f"night-swap palettes differ in slots {swap_diff}")
        ids = used_metatiles(orig_src, layout)
        total_ids += len(ids)
        for mid in ids:
            a, b = fm.metatile(mid), em.metatile(mid)
            if a != b:
                mismatches.append(f"metatile {mid:#05x}: {describe(a, b)}")
        if mismatches:
            failed_layouts += 1
            total += len(mismatches)
            print(f"{name}: {len(mismatches)} mismatch(es)")
            for m in mismatches[:MAX_LISTED_PER_LAYOUT]:
                print("    " + m)
            if len(mismatches) > MAX_LISTED_PER_LAYOUT:
                print(f"    ... {len(mismatches) - MAX_LISTED_PER_LAYOUT} more")

    print(f"\nlayouts checked: {len(manifest['layouts'])}, metatile ids checked: {total_ids}, "
          f"layouts with mismatches: {failed_layouts}, total mismatches: {total}")
    return 1 if total else 0


if __name__ == "__main__":
    sys.exit(main())
