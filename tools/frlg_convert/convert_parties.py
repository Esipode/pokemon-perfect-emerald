#!/usr/bin/env python3
"""Rewrite each Kanto party's `Level:` lines to a non-negative spread.

Spread = monLevel - lowestLevelInParty, so the weakest member is 0. The trainer's base
level and role offset are applied at battle time (see CreateNPCTrainerPartyFromTrainer).
Re-running is a no-op: an already converted party has a lowest level of 0.

Usage: convert_parties.py [--party PATH] [--table PATH] [--dry-run]
"""
import argparse
import re
import sys

LEVEL_RE = re.compile(r"^Level: (\d+)$", re.M)


def convert_block(block):
    old = [int(m) for m in LEVEL_RE.findall(block)]
    if not old:
        return block, old, old
    low = min(old)
    spreads = [lvl - low for lvl in old]
    it = iter(spreads)
    return LEVEL_RE.sub(lambda _: "Level: %d" % next(it), block), old, spreads


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--party", default="src/data/trainers_frlg.party")
    ap.add_argument("--table", help="write the per-trainer table here instead of stdout")
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()

    with open(args.party) as f:
        text = f.read()

    parts = re.split(r"^(=== \S+ ===)$", text, flags=re.M)
    out = [parts[0]]
    rows = []
    for i in range(1, len(parts), 2):
        header, body = parts[i], parts[i + 1]
        trainer = header.split()[1]
        cls = re.search(r"^Class: (.+)$", body, re.M)
        body, old, spreads = convert_block(body)
        rows.append((trainer, cls.group(1) if cls else "?", old, spreads))
        out.append(header)
        out.append(body)

    table = ["%-40s %-22s %-28s %s" % ("id", "class", "old levels", "spreads")]
    for trainer, cls, old, spreads in rows:
        table.append("%-40s %-22s %-28s %s" % (
            trainer, cls, ",".join(map(str, old)), ",".join(map(str, spreads))))
    table_text = "\n".join(table) + "\n"

    if args.table:
        with open(args.table, "w") as f:
            f.write(table_text)
    else:
        sys.stdout.write(table_text)

    if not args.dry_run:
        with open(args.party, "w") as f:
            f.write("".join(out))


if __name__ == "__main__":
    main()
