"""Shared helpers for the GS (HnS Johto) conversion tools. Standard library only."""

import glob
import json
import os
import re

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
OUT_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "out")
DEFAULT_HNS = os.path.normpath(os.path.join(ROOT, "..", "pokehns-expansion"))
HNS_COMMIT = "167aa6d537"

# JOHTO-tagged HnS maps that are test or placeholder maps, not content.
JOHTO_EXCLUDE = {"TestMap1_hns", "TestMap2_hns", "Trees_hns", "Saffron_Temp_hns"}


def add_hns_arg(parser):
    parser.add_argument("--hns", default=DEFAULT_HNS,
                        help="HnS clone checked out at %s (default: %%(default)s)" % HNS_COMMIT)


def read(path):
    with open(path, encoding="utf-8", errors="ignore") as f:
        return f.read()


def load_json(path):
    with open(path, encoding="utf-8") as f:
        return json.load(f)


def map_groups(hns):
    return load_json(os.path.join(hns, "data/maps/map_groups.json"))


def johto_set(hns):
    """(group, map folder, map.json) for every JOHTO map in the HnS `*_Hns` groups, minus test maps."""
    groups = map_groups(hns)
    out = []
    for group in groups["group_order"]:
        if not group.endswith("_Hns"):
            continue
        for name in groups[group]:
            data = load_json(os.path.join(hns, "data/maps", name, "map.json"))
            if data.get("region") == "REGION_JOHTO" and name not in JOHTO_EXCLUDE:
                out.append((group, name, data))
    return out


def map_scripts(hns, name):
    path = os.path.join(hns, "data/maps", name, "scripts.inc")
    return read(path) if os.path.exists(path) else ""


def defined_names(root, patterns, prefix):
    """Names starting with `prefix` defined by #define or as enum members in the given files."""
    names = set()
    define_re = re.compile(r"#define\s+(" + prefix + r"\w+)")
    enum_re = re.compile(r"^\s*(" + prefix + r"\w+)\s*[,=]", re.M)
    for pattern in patterns:
        for path in glob.glob(os.path.join(root, pattern)):
            text = read(path)
            names.update(define_re.findall(text))
            names.update(enum_re.findall(text))
    return names


def foreach_list(text, macro):
    """Moves listed in one `#define FOREACH_xx(F)` block of `text`."""
    m = re.search(r"#define\s+" + macro + r"\(F\)\s*\\\n(.*?)(?:\n\s*\n|\n#)", text, re.S)
    return re.findall(r"F\((\w+)\)", m.group(1)) if m else []


def tm_lists(root, hns_branch):
    """(TM moves, HM moves) from include/constants/tms_hms.h. HnS: the `#if IS_HNS` branch."""
    text = read(os.path.join(root, "include/constants/tms_hms.h"))
    if hns_branch:
        text = text.split("#if IS_HNS", 1)[1].split("#else", 1)[0]
    return foreach_list(text, "FOREACH_TM"), foreach_list(text, "FOREACH_HM")


def strip_hns(name):
    """Drop the HnS suffix (`_HNS`, `_hns`, `_Hns`) from an identifier."""
    return re.sub(r"_(HNS|hns|Hns)(?=$|_)", "", name)
