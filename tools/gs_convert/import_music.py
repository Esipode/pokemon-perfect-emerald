#!/usr/bin/env python3
"""Stage 11: import the HnS songs the Johto set needs, plus their voicegroups, samples and key splits.

Songs = every MUS_HG_* token in the manifest + the encounter/VS songs PlayTrainerEncounterMusic
returns for the HG trainer-encounter ids. One-shot: refuses to run when mus_hg_ is already in
sound/song_table.inc. Writes midi, midi.cfg, song_table.inc, songs.h, voice_groups.inc,
voicegroups/, direct_sound_data.inc, direct_sound_samples/ and keysplit_tables.inc.
"""
import argparse
import json
import os
import re
import shutil
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import common
from common import ROOT, read

# Returned by PlayTrainerEncounterMusic / the HnS battle_setup.c switch, not named in Johto scripts.
EXTRA = [
    "MUS_HG_ENCOUNTER_GIRL_1", "MUS_HG_ENCOUNTER_BOY_1", "MUS_HG_ENCOUNTER_SUSPICIOUS_1",
    "MUS_HG_ENCOUNTER_SAGE", "MUS_HG_ENCOUNTER_KIMONO_GIRL", "MUS_HG_ENCOUNTER_ROCKET",
    "MUS_HG_ENCOUNTER_GIRL_2", "MUS_HG_ENCOUNTER_BOY_2", "MUS_HG_ENCOUNTER_SUSPICIOUS_2",
    "MUS_HG_VS_CHAMPION", "MUS_HG_VS_GYM_LEADER",
]


def w(path, text):
    with open(os.path.join(ROOT, path), "w", encoding="utf-8", newline="\n") as f:
        f.write(text)


def main():
    ap = argparse.ArgumentParser()
    common.add_hns_arg(ap)
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()
    H = args.hns

    table = read(os.path.join(ROOT, "sound/song_table.inc"))
    if "mus_hg_" in table:
        sys.exit("mus_hg_ already imported")

    manifest = json.load(open(os.path.join(common.OUT_DIR, "manifest.json")))
    used = [m for m in manifest["tokens"]["MUS_"] if m.startswith("MUS_HG_")]
    wanted = set(used) | set(EXTRA)

    # HnS song order + constant comments.
    hns_h = read(os.path.join(H, "include/constants/songs.h"))
    defs = re.findall(r"#define (MUS_HG_\w+)\s+\(HG_MUSIC_START \+ (0x[0-9A-Fa-f]+)\)\s*(//.*)?", hns_h)
    order = [(n, int(o, 16), c or "") for n, o, c in defs if n in wanted]
    missing = wanted - {n for n, _, _ in order}
    if missing:
        sys.exit("not in HnS songs.h: %s" % sorted(missing))
    print("songs: %d (%d used + %d extra)" % (len(order), len(used), len(wanted - set(used))))

    hns_table = {m.group(1): m.group(0) for m in
                 re.finditer(r"^\tsong (mus_hg_\w+), .*$", read(os.path.join(H, "sound/song_table.inc")), re.M)}
    cfg = dict(re.findall(r"^(mus_hg_\w+)\.mid:\s*(.*)$", read(os.path.join(H, "sound/songs/midi/midi.cfg")), re.M))

    names = [n.lower() for n, _, _ in order]
    roots = set()
    for n in names:
        roots.update(re.findall(r"-G(\w+)", cfg[n]))
    roots = {"voicegroup" + r if r.isdigit() else r for r in roots}

    # Voicegroup closure.
    label_file = {}
    for path in sorted(os.listdir(os.path.join(H, "sound/voicegroups"))):
        if not path.endswith(".inc"):
            continue
        for lab in re.findall(r"^(\w+)::", read(os.path.join(H, "sound/voicegroups", path)), re.M):
            label_file[lab] = path
    tgt_voice = read(os.path.join(ROOT, "sound/voice_groups.inc"))
    tgt_labels = set()
    for dp, _, fs in os.walk(os.path.join(ROOT, "sound/voicegroups")):
        for f in fs:
            tgt_labels.update(re.findall(r"^(\w+)::", read(os.path.join(dp, f)), re.M))
    seen, stack, files, samples, splits = set(), list(roots), [], set(), set()
    while stack:
        g = stack.pop()
        if g in seen:
            continue
        seen.add(g)
        if g not in label_file:
            sys.exit("voicegroup not found in HnS: " + g)
        if g in tgt_labels:
            sys.exit("voicegroup label already in target: " + g)
        files.append(label_file[g])
        text = read(os.path.join(H, "sound/voicegroups", label_file[g]))
        stack += re.findall(r"\b(voicegroup\d+)\b", text)
        samples.update(re.findall(r"\b(DirectSoundWaveData_\w+)", text))
        splits.update(re.findall(r"\b(KeySplitTable\d+)", text))
        if re.search(r"ProgrammableWaveData_|voice_programmable", text):
            sys.exit("programmable wave voice in " + g)
    files = sorted(set(files))
    print("voicegroups: %d (%d files), samples: %d, key splits: %d" % (len(seen), len(files), len(samples), len(splits)))

    # Samples.
    def sample_map(root):
        return dict(re.findall(r"(DirectSoundWaveData_\w+)::\s*\n\s*\.incbin\s+\"([^\"]+)\"",
                               read(os.path.join(root, "sound/direct_sound_data.inc"))))
    hs, ts = sample_map(H), sample_map(ROOT)
    new, shared = [], 0
    for s in sorted(samples):
        if s in ts:
            a = open(os.path.join(H, os.path.splitext(hs[s])[0] + ".wav"), "rb").read()
            b = open(os.path.join(ROOT, os.path.splitext(ts[s])[0] + ".wav"), "rb").read()
            if a != b:
                sys.exit("sample name conflict: " + s)
            shared += 1
        else:
            new.append(s)
    print("samples new %d, shared %d" % (len(new), shared))

    # Key split blocks.
    ks = read(os.path.join(H, "sound/keysplit_tables.inc")).split("\n")
    starts = [i for i, l in enumerate(ks) if re.match(r"\.set KeySplitTable\d+,", l)]
    blocks = {}
    for k, i in enumerate(starts):
        end = starts[k + 1] if k + 1 < len(starts) else len(ks)
        blocks[re.match(r"\.set (KeySplitTable\d+),", ks[i]).group(1)] = "\n".join(ks[i:end]).rstrip("\n")
    for s in splits:
        if s not in blocks:
            sys.exit("key split not found: " + s)

    if args.dry_run:
        return

    for n in names:
        shutil.copy(os.path.join(H, "sound/songs/midi", n + ".mid"), os.path.join(ROOT, "sound/songs/midi", n + ".mid"))
    for f in files:
        shutil.copy(os.path.join(H, "sound/voicegroups", f), os.path.join(ROOT, "sound/voicegroups", f))
    for s in new:
        wav = os.path.splitext(hs[s])[0] + ".wav"
        shutil.copy(os.path.join(H, wav), os.path.join(ROOT, wav))

    with open(os.path.join(ROOT, "sound/songs/midi/midi.cfg"), "a", newline="\n") as f:
        for n in names:
            f.write("%s.mid: %s\n" % (n, cfg[n]))
    with open(os.path.join(ROOT, "sound/voice_groups.inc"), "a", newline="\n") as f:
        f.write("\n@ HGSS\n")
        for fn in files:
            f.write('.include "sound/voicegroups/%s"\n' % fn)
    with open(os.path.join(ROOT, "sound/direct_sound_data.inc"), "a", newline="\n") as f:
        for s in new:
            f.write('\n\t.align 2\n%s::\n\t.incbin "%s"\n' % (s, re.sub(r"\.wav$", ".bin", hs[s])))
    with open(os.path.join(ROOT, "sound/keysplit_tables.inc"), "a", newline="\n") as f:
        for s in sorted(splits, key=lambda x: int(x[len("KeySplitTable"):])):
            f.write("\n" + blocks[s] + "\n")

    # song_table.inc: after ph_nurse_solo.
    lines = "\n".join(hns_table[n] for n in names)
    anchor = "\tsong ph_nurse_solo, MUSIC_PLAYER_SE2, 2\n"
    assert anchor in table
    w("sound/song_table.inc", table.replace(anchor, anchor + "\n\t@ HGSS Music\n" + lines + "\n"))

    # songs.h: sequential ids after PH_NURSE_SOLO.
    songs_h = read(os.path.join(ROOT, "include/constants/songs.h"))
    out = ["", "// HGSS music (Johto)", "#define HG_MUSIC_START             (PH_NURSE_SOLO + 1)", ""]
    for i, (n, _, c) in enumerate(order):
        out.append("#define %-28s (HG_MUSIC_START + 0x%02X)%s" % (n, i, "  " + c if c else ""))
    anchor = "#define PH_NURSE_SOLO               609\n"
    assert anchor in songs_h
    w("include/constants/songs.h", songs_h.replace(anchor, anchor + "\n".join(out) + "\n"))
    print("done")


if __name__ == "__main__":
    main()
