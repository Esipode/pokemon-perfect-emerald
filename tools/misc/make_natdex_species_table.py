import glob
import re

# Builds src/data/pokemon/national_dex_num_to_species.h: a compile-time reverse
# of species -> natDexNum (src/data/pokemon/species_info/gen_*_families.h),
# indexed by national dex number. Replaces a runtime-built EWRAM cache with a
# const ROM table (Free Space Stage 2).
#
# natDexNum is assigned either directly in a species' designated initializer
# block, or via a macro whose body contains it (used for large cosmetic-variant
# families like Unown/Vivillon/Minior, where dozens of species share one
# natDexNum through one macro). Both forms are handled, including a macro whose
# body itself invokes an earlier macro that carries natDexNum.
#
# When several species share a dex number, the lowest species ID should win
# (matches the original runtime scan: ascending species order, first writer
# wins). Entries are emitted sorted by descending species ID so that, whichever
# subset survive this build's #if config, the lowest-ID survivor is always the
# last designated initializer for that index and therefore the one that sticks.

SPECIES_H = 'include/constants/species.h'
GEN_FILES = sorted(glob.glob('src/data/pokemon/species_info/gen_*_families.h'))
OUT_FILE = 'src/data/pokemon/national_dex_num_to_species.h'

re_species_line = re.compile(r'^\s*SPECIES_(\w+)\s*=\s*(.+?),\s*$')
re_if = re.compile(r'^\s*#\s*(if|ifdef|ifndef)\b')
re_endif = re.compile(r'^\s*#\s*endif\b')
re_else = re.compile(r'^\s*#\s*(else|elif)\b')
# Captures the macro name and whether it takes arguments (no space before the
# paren) -- that's also the C rule for function-like vs. object-like macros.
re_define = re.compile(r'^\s*#\s*define\s+(\w+)(\([^)]*\))?')
re_species_block = re.compile(r'^\s*\[SPECIES_(\w+)\]\s*=\s*(.*)$')
re_natdex = re.compile(r'\.natDexNum\s*=\s*(NATIONAL_DEX_\w+)\s*,')


def BuildSpeciesIdMap():
    raw = {}
    with open(SPECIES_H, encoding='utf-8') as f:
        for line in f:
            m = re_species_line.match(line)
            if m:
                raw[m.group(1)] = m.group(2).strip()

    id_map = {}

    def Resolve(name, stack):
        if name in id_map:
            return id_map[name]
        if name not in raw:
            raise Exception(f"SPECIES_{name} has no numeric value or SPECIES_ alias in {SPECIES_H}")
        if name in stack:
            raise Exception(f"alias cycle involving SPECIES_{name}")
        stack.add(name)
        val = raw[name]
        if val.isdigit():
            n = int(val)
        elif val.startswith('SPECIES_'):
            n = Resolve(val[len('SPECIES_'):], stack)
        else:
            raise Exception(f"SPECIES_{name} = {val} is not a plain number or SPECIES_ alias")
        id_map[name] = n
        return n

    # Resolved lazily (only for species actually referenced by the gen_*_families.h
    # scan below) -- a handful of enum members like SPECIES_CUSTOM_END have no "="
    # at all and would otherwise need special-casing here for no benefit.
    return raw, Resolve


def FindDexInLine(line, macros):
    m = re_natdex.search(line)
    if m:
        return m.group(1)
    for mname, (mdex, is_func) in macros.items():
        if not mdex:
            continue
        pattern = r'\b' + re.escape(mname) + (r'\s*\(' if is_func else r'\b')
        if re.search(pattern, line):
            return mdex
    return None


def ParseGenFile(path):
    """Yields (species_name, dex_token, guard_lines) for every species in
    this file that has a resolvable natDexNum."""
    with open(path, encoding='utf-8') as f:
        lines = [l.rstrip('\n') for l in f]

    guard_stack = []
    macros = {}
    in_macro = False
    macro_name = None
    macro_is_func = False
    macro_body = []

    cur_species = None
    cur_guard = None
    cur_resolved = False

    def Resolved(species, dex, guard):
        return (species, dex, guard)

    i = 0
    n = len(lines)
    while i < n:
        line = lines[i]

        if in_macro:
            macro_body.append(line)
            if line.rstrip().endswith('\\'):
                i += 1
                continue
            macros[macro_name] = (FindDexInLine('\n'.join(macro_body), macros), macro_is_func)
            in_macro = False
            i += 1
            continue

        mdef = re_define.match(line)
        if mdef:
            macro_name = mdef.group(1)
            macro_is_func = mdef.group(2) is not None
            macro_body = [line]
            if line.rstrip().endswith('\\'):
                in_macro = True
            else:
                macros[macro_name] = (FindDexInLine(line, macros), macro_is_func)
            i += 1
            continue

        if re_if.match(line):
            guard_stack.append(line.strip())
            i += 1
            continue
        if re_endif.match(line):
            if guard_stack:
                guard_stack.pop()
            i += 1
            continue
        if re_else.match(line):
            i += 1
            continue

        msp = re_species_block.match(line)
        if msp:
            if cur_species is not None and not cur_resolved:
                raise Exception(f"{path}: SPECIES_{cur_species} has no resolvable natDexNum")
            cur_species = msp.group(1)
            cur_guard = tuple(guard_stack)
            cur_resolved = False
            dex = FindDexInLine(line, macros)
            if dex:
                yield Resolved(cur_species, dex, cur_guard)
                cur_resolved = True
            i += 1
            continue

        if cur_species is not None and not cur_resolved:
            dex = FindDexInLine(line, macros)
            if dex:
                yield Resolved(cur_species, dex, cur_guard)
                cur_resolved = True
        i += 1

    if cur_species is not None and not cur_resolved:
        raise Exception(f"{path}: SPECIES_{cur_species} has no resolvable natDexNum")


def main():
    raw, Resolve = BuildSpeciesIdMap()

    entries = []  # (species_id, species_name, dex_token, guard_lines)
    for path in GEN_FILES:
        for species_name, dex_token, guard in ParseGenFile(path):
            if species_name not in raw:
                raise Exception(f"{path}: SPECIES_{species_name} not found in {SPECIES_H}")
            entries.append((Resolve(species_name, set()), species_name, dex_token, guard))

    entries.sort(key=lambda e: -e[0])

    with open(OUT_FILE, 'w', encoding='utf-8', newline='\n') as f:
        f.write(
            "// Generated by tools/misc/make_natdex_species_table.py -- do not edit by hand.\n"
            "// Reverse of species -> natDexNum, indexed by national dex number. Entries are\n"
            "// emitted in descending species-ID order so that, when multiple species share a\n"
            "// dex number (regional forms, cosmetic variants), the lowest-ID species' designated\n"
            "// initializer is the last one applied and therefore wins -- matching the original\n"
            "// runtime scan this table replaces (ascending species order, first writer wins).\n"
            "//\n"
            "// Species sharing a dex number deliberately overwrite each other in initializer\n"
            "// order (see the comment above) -- exactly what -Woverride-init exists to flag,\n"
            "// so it's suppressed locally rather than dropped project-wide.\n"
            "#pragma GCC diagnostic push\n"
            "#pragma GCC diagnostic ignored \"-Woverride-init\"\n"
            "static const u16 sNationalDexNumToSpecies[NATIONAL_DEX_PECHARUNT + 1] =\n"
            "{\n"
        )
        for _, species_name, dex_token, guard in entries:
            for g in guard:
                f.write(g + '\n')
            f.write(f"    [{dex_token}] = SPECIES_{species_name},\n")
            for _ in guard:
                f.write("#endif\n")
        f.write("};\n#pragma GCC diagnostic pop\n")


if __name__ == '__main__':
    main()
