import glob
import re

# Builds src/data/pokemon/legendary_families.h: the evolution families whose
# base form is a designated legendary (restricted legendary / sub-legendary /
# mythical, with Ultra Beasts and Paradox species excluded), read from
# src/data/pokemon/species_info/gen_*_families.h.
#
# Replaces a runtime walk of the whole species table -- one that called
# GetSpeciesPreEvolution (itself a full-table scan) per species -- with a flat
# ROM table of ~100 entries, so the Legendary Collection achievements can be
# counted without a visible freeze.
#
# Output shape: each family is its members in evolution order followed by a
# SPECIES_NONE terminator. Alternate forms are not members -- only the base
# form of each species is emitted, matching the "one family, one entry" rule
# the count uses. Every entry keeps the #if guards its species block sits
# under, so a family switched off by config never reaches the table.
#
# A species block either writes its fields directly or expands a macro that
# writes them (Arceus, Silvally and the other large form families), so both
# the block's own text and the text of every macro it invokes are scanned.

SPECIES_INFO_FILES = sorted(glob.glob('src/data/pokemon/species_info/gen_*_families.h'))
FORM_TABLE_FILE = 'src/data/pokemon/form_species_tables.h'
OUT_FILE = 'src/data/pokemon/legendary_families.h'

LEGENDARY_FLAGS = frozenset(('isRestrictedLegendary', 'isSubLegendary', 'isMythical'))
EXCLUDED_FLAGS = frozenset(('isUltraBeast', 'isParadox'))

re_if = re.compile(r'^\s*#\s*(if|ifdef|ifndef)\b')
re_endif = re.compile(r'^\s*#\s*endif\b')
re_else = re.compile(r'^\s*#\s*(else|elif)\b')
re_define = re.compile(r'^\s*#\s*define\s+(\w+)')
re_species_block = re.compile(r'^\s*\[SPECIES_(\w+)\]\s*=\s*(.*)$')
re_flag = re.compile(r'\.(' + '|'.join(sorted(LEGENDARY_FLAGS | EXCLUDED_FLAGS)) + r')\s*=\s*TRUE\s*,')
re_form_table = re.compile(r'\.formSpeciesIdTable\s*=\s*(\w+)\s*,')
re_evolutions = re.compile(r'\.evolutions\s*=\s*EVOLUTION\s*\(')
re_form_table_decl = re.compile(r'^\s*static const u16 (\w+)\[\]\s*=')
re_species_token = re.compile(r'SPECIES_(\w+)')


def ParseFormTables():
    """Maps each form-species table name to the species at index 0 -- the base
    form every other species in that table is a form of."""
    tables = {}
    name = None
    with open(FORM_TABLE_FILE, encoding='utf-8') as f:
        for line in f:
            decl = re_form_table_decl.match(line)
            if decl:
                name = decl.group(1)
                continue
            if name is None:
                continue
            m = re_species_token.search(line)
            if m:
                tables[name] = m.group(1)
                name = None
            elif '}' in line:
                name = None
    return tables


def SplitTopLevel(text, opener, closer):
    """Yields the top-level `opener`...`closer` groups in `text`, contents only.
    Nesting is kept intact, so an evolution's CONDITIONS(...) list stays inside
    the entry it belongs to instead of being read as an entry of its own."""
    depth = 0
    start = 0
    for i, c in enumerate(text):
        if c == opener:
            depth += 1
            if depth == 1:
                start = i + 1
        elif c == closer:
            depth -= 1
            if depth == 0:
                yield text[start:i]


def SplitArguments(text):
    """`text` split on its top-level commas."""
    args = []
    depth = 0
    current = []
    for c in text:
        if c in '({[':
            depth += 1
        elif c in ')}]':
            depth -= 1
        if c == ',' and depth == 0:
            args.append(''.join(current).strip())
            current = []
        else:
            current.append(c)
    args.append(''.join(current).strip())
    return args


def EvolutionTargets(text):
    """Species named by a .evolutions = EVOLUTION(...) in `text`: the third
    field ({method, argument, target}) of each entry."""
    m = re_evolutions.search(text)
    if not m:
        return []

    body = next(SplitTopLevel(text[m.end() - 1:], '(', ')'), '')

    targets = []
    for entry in SplitTopLevel(body, '{', '}'):
        fields = SplitArguments(entry)
        if len(fields) >= 3:
            target = re_species_token.fullmatch(fields[2].strip())
            if target:
                targets.append(target.group(1))
    return targets


def ScanText(text, macros):
    """(flags, formSpeciesIdTable, evolutionTargets) written by `text`, either
    directly or by the macros it expands."""
    flags = set(re_flag.findall(text))
    table = re_form_table.search(text)
    form_table = table.group(1) if table else None
    evolutions = EvolutionTargets(text)

    for name, (macro_flags, macro_table, macro_evolutions) in macros.items():
        if not re.search(r'\b' + re.escape(name) + r'\b', text):
            continue
        flags |= macro_flags
        form_table = form_table or macro_table
        evolutions = evolutions or macro_evolutions

    return flags, form_table, evolutions


def ParseSpeciesInfo(path, macros):
    """Yields (species, guard, flags, formSpeciesIdTable, evolutionTargets) for
    every species block in one gen_*_families.h. `macros` carries the macro
    definitions seen so far and is extended as this file's are parsed."""
    with open(path, encoding='utf-8') as f:
        lines = [l.rstrip('\n') for l in f]

    guard_stack = []
    macro_name = None
    macro_body = []
    species = None
    guard = ()
    body = []

    def Flush():
        if species is None:
            return None
        flags, form_table, evolutions = ScanText('\n'.join(body), macros)
        return (species, guard, flags, form_table, evolutions)

    for line in lines:
        if macro_name is not None:
            macro_body.append(line)
            if not line.rstrip().endswith('\\'):
                macros[macro_name] = ScanText('\n'.join(macro_body), macros)
                macro_name = None
            continue

        define = re_define.match(line)
        if define:
            macro_name = define.group(1)
            macro_body = [line]
            if not line.rstrip().endswith('\\'):
                macros[macro_name] = ScanText(line, macros)
                macro_name = None
            continue

        if re_if.match(line):
            guard_stack.append(line.strip())
            continue
        if re_endif.match(line):
            if guard_stack:
                guard_stack.pop()
            continue
        if re_else.match(line):
            continue

        block = re_species_block.match(line)
        if block:
            entry = Flush()
            if entry:
                yield entry
            species = block.group(1)
            guard = tuple(guard_stack)
            body = [block.group(2)]
            continue

        if species is not None:
            body.append(line)

    entry = Flush()
    if entry:
        yield entry


def main():
    form_tables = ParseFormTables()
    macros = {}

    guards = {}
    counted = set()      # base-form species the Legendary Collection counts
    base_forms = set()
    evolves_to = {}      # species -> the species it evolves into
    evolves_from = {}    # species -> the species it evolves from

    for path in SPECIES_INFO_FILES:
        for species, guard, flags, form_table, evolutions in ParseSpeciesInfo(path, macros):
            guards[species] = guard

            # A species with a form table is a base form only if it is that
            # table's first entry; the rest are alternate forms of it and never
            # count as a family of their own.
            is_base = form_table is None or form_tables.get(form_table) == species
            if is_base:
                base_forms.add(species)

            if evolutions:
                evolves_to[species] = evolutions
                for target in evolutions:
                    evolves_from[target] = species

            if flags & LEGENDARY_FLAGS and not flags & EXCLUDED_FLAGS and is_base:
                counted.add(species)

    # A family is recorded once, under its root. A counted species that evolves
    # from another counted species is a member of that root's family, not a
    # family of its own.
    def Root(species):
        seen = {species}
        while species in evolves_from:
            species = evolves_from[species]
            if species in seen:
                raise Exception(f"evolution cycle involving SPECIES_{species}")
            seen.add(species)
        return species

    roots = []
    for species in sorted(counted):
        root = Root(species)
        if root == species:
            roots.append(species)
        elif root not in counted:
            # The count keys off the root's flags, so a legendary evolving from
            # a species that isn't itself designated would be dropped silently.
            raise Exception(f"SPECIES_{species} is counted but its root SPECIES_{root} is not")

    def Members(root):
        """The root and every species it evolves into, breadth-first, base
        forms only."""
        members = [root]
        head = 0
        while head < len(members):
            for target in evolves_to.get(members[head], []):
                if target not in members and target in base_forms:
                    members.append(target)
            head += 1
        return members

    with open(OUT_FILE, 'w', encoding='utf-8', newline='\n') as f:
        f.write(
            "// Generated by tools/misc/make_legendary_family_table.py -- do not edit by hand.\n"
            "// Every evolution family whose base form is a designated legendary (restricted\n"
            "// legendary, sub-legendary or mythical; Ultra Beasts and Paradox species excluded),\n"
            "// read out of src/data/pokemon/species_info/gen_*_families.h at build time.\n"
            "//\n"
            "// One family per run of entries, in evolution order, terminated by SPECIES_NONE.\n"
            "// Alternate forms are not listed -- a family counts once, under its base form.\n"
            "// Entries carry the #if guards of the species block they came from, so a family\n"
            "// disabled by config is absent rather than present-but-empty.\n"
            "static const u16 sLegendaryFamilies[] =\n"
            "{\n"
        )
        for root in roots:
            members = Members(root)
            outer = guards[root]

            for g in outer:
                f.write(g + '\n')

            if all(guards[m] == outer for m in members):
                # The whole family shares the root's guard: one line for it.
                f.write("    " + ''.join(f"SPECIES_{m}, " for m in members) + "SPECIES_NONE,\n")
            else:
                for member in members:
                    # Members guarded more narrowly than the family (a form or
                    # generation switch of their own) keep that inner guard.
                    inner = guards[member]
                    inner = inner[len(outer):] if inner[:len(outer)] == outer else inner
                    for g in inner:
                        f.write(g + '\n')
                    f.write(f"    SPECIES_{member},\n")
                    for _ in inner:
                        f.write("#endif\n")
                f.write("    SPECIES_NONE,\n")

            for _ in outer:
                f.write("#endif\n")
        f.write("};\n")


if __name__ == '__main__':
    main()
