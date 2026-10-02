#!/usr/bin/env python3
"""Delete the explicit `waitstate` that follows an implicit one, in the imported Johto scripts only.

Same idea as migration_scripts/1.15/delete_implicit_waitstates.py, restricted to the Johto map
folders (from data/maps/map_groups.json) and data/scripts/bug_contest.inc so no Hoenn or Kanto file
changes. Assembles data/event_scripts.s once and edits the files the warnings point at.

    python3 tools/gs_convert/fix_waitstates.py
"""
import collections
import json
import os
import re
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from common import ROOT  # noqa: E402


def main():
    groups = json.load(open(os.path.join(ROOT, "data/maps/map_groups.json")))
    allowed = {"data/scripts/bug_contest.inc", "data/scripts/johto_field.inc"}
    for g in groups["group_order"]:
        if g.endswith("_Johto"):
            allowed.update("data/maps/%s/scripts.inc" % m for m in groups[g])
    p = subprocess.run(["make", "-j12", "build/emerald/data/event_scripts.o", "-W", "data/event_scripts.s"],
                       cwd=ROOT, capture_output=True, text=True)
    lines = p.stderr.split("\n") + p.stdout.split("\n")
    todo = collections.defaultdict(set)
    for i, line in enumerate(lines):
        if "explicit waitstate follows implicit waitstate" in line:
            m = re.match(r"([^:]+):(\d+):\s+Info: macro invoked from here", lines[i + 1])
            if m and m.group(1) in allowed:
                todo[m.group(1)].add(int(m.group(2)))
    for path, numbers in todo.items():
        full = os.path.join(ROOT, path)
        text = open(full).read().split("\n")
        for n in sorted(numbers, reverse=True):
            assert text[n - 1].strip() == "waitstate", (path, n, text[n - 1])
            del text[n - 1]
        open(full, "w").write("\n".join(text))
    print("removed %d waitstates in %d files" % (sum(len(v) for v in todo.values()), len(todo)))


if __name__ == "__main__":
    main()
