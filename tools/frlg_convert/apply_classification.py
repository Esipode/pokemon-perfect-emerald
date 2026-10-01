#!/usr/bin/env python3
"""Apply the reviewed flag/var classification (Stage 6).

Reads the reviewed `FRLG - Flag Classification.md`, assigns Kanto ids with the Stage 5 rules,
writes `include/constants/flags_kanto.h` / `vars_kanto.h` and renames FRLG data uses to their
final names. One-shot: needs `include/constants/flags_frlg.h` for the item-ball section, so run it
before that header is deleted. Standard library only. Run from the repo root:

    python3 tools/frlg_convert/apply_classification.py --table "<path>/FRLG - Flag Classification.md" --headers
    python3 tools/frlg_convert/apply_classification.py --table "<path>" --rename [--write]
"""

import argparse
import glob
import os
import re
import sys
from collections import defaultdict

import classify_flags_vars as cfv

ROOT = cfv.ROOT
VALID_BUCKETS = {"Kanto", "Kanto (renamed)", "Shared", "Dropped"}

BLOCK_TITLES = {
    "story": "Story and scene state",
    "hide": "Object hide flags",
    "gift": "Gifts and one-time events",
    "badge": "Badges",
    "champion": "Champion",
    "world_map": "World map: fly destinations and map previews",
    "item_ball": "Item balls",
    "hidden_item": "Hidden items",
}


def parse_int(text):
    text = text.strip()
    return None if text in ("", "—") else int(text, 16)


def read_table(path):
    rows = []
    with open(path, encoding="utf-8") as f:
        for line in f:
            if not re.match(r"^\| (FLAG|VAR)_", line):
                continue
            cells = [c.strip() for c in line.strip().strip("|").split("|")]
            name, _emerald, frlg, _status, bucket, final, preview = cells[:7]
            if bucket not in VALID_BUCKETS:
                sys.exit(f"error: {name}: unknown bucket '{bucket}'")
            r = cfv.Row(name)
            r.frlg = parse_int(frlg)
            r.bucket = bucket
            r.final_name = final
            r.preview = parse_int(preview)
            rows.append(r)
    return rows


def kanto_finals(rows):
    """Final name -> (kind, block, id) for every Kanto final name."""
    out = {}
    for r in rows:
        if r.bucket.startswith("Kanto"):
            prev = out.get(r.final_name)
            if prev and prev[2] != r.final_id:
                sys.exit(f"error: {r.final_name} got two ids")
            out[r.final_name] = (r.kind, r.block, r.final_id)
    return out


# ---------------------------------------------------------------------------
# Headers


def write_header(path, guard, intro, groups, base_name, base_value):
    width = max(len(name) for _, members in groups for name, _ in members) + 1
    lines = [f"#ifndef {guard}", f"#define {guard}", ""]
    lines += [f"// {t}" if t else "//" for t in intro]
    for title, members in groups:
        first, last = members[0][1], members[-1][1]
        lines += ["", f"// {title} (0x{first:X}-0x{last:X})"]
        for name, value in members:
            lines.append(f"#define {name.ljust(width)}({base_name} + 0x{value - base_value:03X})")
    lines += ["", f"#endif // {guard}", ""]
    with open(os.path.join(ROOT, path), "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(lines))


def write_headers(finals, ranges):
    by_block = defaultdict(list)
    for name, (kind, block, value) in finals.items():
        by_block[block].append((name, value))
    for members in by_block.values():
        members.sort(key=lambda m: m[1])

    flag_groups = [(BLOCK_TITLES[b], by_block[b]) for b in cfv.FLAG_BLOCKS if by_block[b]]
    write_header(
        "include/constants/flags_kanto.h", "GUARD_CONSTANTS_FLAGS_KANTO_H",
        ["Kanto flags, stored in SaveBlock2 kantoFlags. Story slots end at KANTO_TRAINER_FLAGS_START.",
         "Each block starts on a 0x10 boundary. Ids inside a block keep the FireRed order, so",
         "FireRed range loops (badges, hidden items) stay valid."],
        flag_groups, "KANTO_FLAGS_START", ranges["KANTO_FLAGS_START"])
    write_header(
        "include/constants/vars_kanto.h", "GUARD_CONSTANTS_VARS_KANTO_H",
        ["Kanto vars, stored in SaveBlock2 kantoVars. Ids keep the FireRed order."],
        [("Vars", by_block["var"])], "KANTO_VARS_START", ranges["KANTO_VARS_START"])


# ---------------------------------------------------------------------------
# Renames


def frlg_line_scopes():
    """(path, set of 0-based line numbers or None for the whole file) for every FRLG data file."""
    scopes = [(inc, None) for inc in cfv.frlg_script_includes()]
    scopes.append(("data/scripts/hall_of_fame_frlg.inc", None))
    scopes += [(cfv.rel(p), None) for p in sorted(glob.glob(os.path.join(ROOT, "data/maps/*_Frlg/map.json")))]

    # EventScript_ResetAllMapFlagsFrlg only.
    path = "data/scripts/new_game.inc"
    with open(os.path.join(ROOT, path), encoding="utf-8") as f:
        lines = f.read().split("\n")
    inside, picked = False, set()
    for i, line in enumerate(lines):
        if re.match(r"^\w+::", line):
            inside = line.startswith("EventScript_ResetAllMapFlagsFrlg::")
        if inside:
            picked.add(i)
    scopes.append((path, picked))

    # IS_FRLG branches of shared data files.
    frlg_set = {p for p, _ in scopes}
    shared = set()
    for pattern in ("data/**/*.inc", "data/**/*.s"):
        shared |= {cfv.rel(p) for p in glob.glob(os.path.join(ROOT, pattern), recursive=True)}
    for path in sorted(shared - frlg_set):
        if "/maps/" in path:
            continue
        with open(os.path.join(ROOT, path), encoding="utf-8") as f:
            lines = f.read().split("\n")
        depth, picked = 0, set()
        for i, line in enumerate(lines):
            s = line.strip()
            if re.match(r"^[#.]if\s+IS_FRLG\b", s):
                depth = 1
                continue
            if depth and re.match(r"^[#.](else|endif)\b", s):
                depth = 0
                continue
            if depth:
                picked.add(i)
        if picked:
            scopes.append((path, picked))
    return scopes


def rename(rows, write):
    mapping = {r.name: r.final_name for r in rows
               if r.final_name and r.final_name != r.name and r.bucket != "Dropped"}
    pattern = re.compile(r"\b(" + "|".join(sorted(map(re.escape, mapping), key=len, reverse=True)) + r")\b")
    per_name = defaultdict(int)
    per_file = defaultdict(int)
    for path, scope in frlg_line_scopes():
        full = os.path.join(ROOT, path)
        with open(full, encoding="utf-8", newline="") as f:
            text = f.read()
        lines = text.split("\n")
        changed = False
        for i, line in enumerate(lines):
            if scope is not None and i not in scope:
                continue
            new = pattern.sub(lambda m: mapping[m.group(1)], line)
            if new != line:
                for m in pattern.finditer(line):
                    per_name[m.group(1)] += 1
                    per_file[path] += 1
                lines[i] = new
                changed = True
        if changed and write:
            with open(full, "w", encoding="utf-8", newline="") as f:
                f.write("\n".join(lines))

    print(f"{'written' if write else 'dry run'}: {sum(per_name.values())} replacements in {len(per_file)} files")
    for name in sorted(mapping):
        print(f"  {name} -> {mapping[name]}: {per_name.get(name, 0)}")


# ---------------------------------------------------------------------------


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--table", required=True, help="Reviewed classification Markdown")
    ap.add_argument("--headers", action="store_true", help="Write flags_kanto.h and vars_kanto.h")
    ap.add_argument("--rename", action="store_true", help="Rename FRLG data uses (dry run without --write)")
    ap.add_argument("--write", action="store_true", help="Write the renames")
    args = ap.parse_args()

    emerald = cfv.World(False)
    emerald.load("constants/flags.h")
    emerald.load("constants/vars.h")
    ranges = {k: emerald.value(k) if emerald.value(k) is not None else v for k, v in cfv.DEFAULT_RANGES.items()}

    rows = read_table(args.table)
    cfv.assign_ids(rows, ranges, cfv.frlg_flag_sections())
    finals = kanto_finals(rows)

    moved = [r.name for r in rows if r.bucket.startswith("Kanto") and r.final_id != r.preview]
    print(f"{len(rows)} rows, {len(finals)} Kanto final names, {len(moved)} ids differ from the preview")
    flag_end = max((v for k, _, v in finals.values() if k == "flag"), default=0)
    var_end = max((v for k, _, v in finals.values() if k == "var"), default=0)
    if flag_end >= ranges["KANTO_TRAINER_FLAGS_START"] or var_end > ranges["KANTO_VARS_END"]:
        sys.exit("error: Kanto ids exceed their range")

    if args.headers:
        write_headers(finals, ranges)
        print("wrote include/constants/flags_kanto.h, include/constants/vars_kanto.h")
    if args.rename:
        rename(rows, args.write)
    return 0


if __name__ == "__main__":
    sys.exit(main())
