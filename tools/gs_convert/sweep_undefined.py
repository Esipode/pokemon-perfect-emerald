#!/usr/bin/env python3
"""Find FLAG_/VAR_ names in Johto scripts that no header defines and the rename map does not cover.

Default scans the HnS Johto set (before import): every name must be in `out/rename_map.json`.
`--maps DIR...` scans imported target map folders (after Stage 13): every name must be defined by
the target headers, and no Dropped/Config name may survive. Exit code 1 on any finding. Standard
library only. Run from the repo root:

    python3 tools/gs_convert/sweep_undefined.py
    python3 tools/gs_convert/sweep_undefined.py --maps data/maps/NewBarkTown data/maps/Route29
"""

import argparse
import json
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from common import ROOT, OUT_DIR, add_hns_arg, johto_set, read  # noqa: E402

TOKEN_RE = re.compile(r"\b(?:FLAG|VAR)_[A-Za-z0-9_]+\b")
HEADERS = ("flags.h", "vars.h", "flags_kanto.h", "vars_kanto.h", "flags_johto.h", "vars_johto.h")


def target_defined():
    names = set()
    for h in HEADERS:
        names |= set(re.findall(r"^#define\s+((?:FLAG|VAR)_\w+)", read(os.path.join(ROOT, "include/constants", h)), re.M))
    return names


def scan_hns(hns, rename):
    known = set(rename["flags"]) | set(rename["vars"])
    missing = {}
    for _, folder, data in johto_set(hns):
        base = os.path.join(hns, "data/maps", folder)
        blob = read(os.path.join(base, "scripts.inc")) + json.dumps(data)
        for tok in set(TOKEN_RE.findall(blob)):
            if tok not in known and tok not in target_defined_cache:
                missing.setdefault(tok, []).append(folder)
    return missing


def scan_imported(dirs, rename):
    defined = target_defined_cache
    gone = {n for sec in ("flags", "vars") for n, e in rename[sec].items() if e["action"] in ("delete line", "replace branch")}
    missing, surviving = {}, {}
    for d in dirs:
        for fn in ("scripts.inc", "map.json"):
            path = os.path.join(d, fn)
            if not os.path.exists(path):
                continue
            for tok in set(TOKEN_RE.findall(read(path))):
                if tok in gone:
                    surviving.setdefault(tok, []).append(path)
                elif tok not in defined:
                    missing.setdefault(tok, []).append(path)
    return missing, surviving


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    add_hns_arg(ap)
    ap.add_argument("--rename-map", default=os.path.join(OUT_DIR, "rename_map.json"))
    ap.add_argument("--maps", nargs="*", help="Imported map folders to sweep instead of the HnS set")
    args = ap.parse_args()
    with open(args.rename_map, encoding="utf-8") as f:
        rename = json.load(f)
    global target_defined_cache
    target_defined_cache = target_defined()

    if args.maps:
        missing, surviving = scan_imported(args.maps, rename)
    else:
        missing, surviving = scan_hns(args.hns, rename), {}
    for title, found in (("undefined", missing), ("Dropped/Config name still used", surviving)):
        for tok in sorted(found):
            print(f"{title}: {tok} ({', '.join(sorted(set(found[tok]))[:3])})")
    print(f"{len(missing)} undefined, {len(surviving)} surviving Dropped/Config")
    return 1 if missing or surviving else 0


target_defined_cache = set()

if __name__ == "__main__":
    sys.exit(main())
