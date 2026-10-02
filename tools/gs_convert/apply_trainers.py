#!/usr/bin/env python3
"""Stage 10: add Johto trainer pics, classes and encounter-music constants. One-shot.

Pics: copies 36 HnS pngs/palettes, appends TRAINER_PIC_*_JOHTO, the graphics symbols and
gTrainerPicInfo rows. Classes: appends 16 TRAINER_CLASS_*_JOHTO after TRAINER_CLASS_CHALLENGER.
Encounter music: appends the HnS TRAINER_ENCOUNTER_MUSIC_* values used by Johto parties.
"""
import os
import shutil
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import common

R = common.ROOT
H = common.DEFAULT_HNS

CLASSES = [  # (enum suffix, display name, money, ball)
    ("BURGLAR", "BURGLAR", None, None),
    ("CHAMPION", "CHAMPION", 50, "BALL_ULTRA"),
    ("ELITE_FOUR", "ELITE FOUR", 25, "BALL_ULTRA"),
    ("FIREBREATHER", "FIREBREATHER", None, None),
    ("JUGGLER", "JUGGLER", None, None),
    ("KIMONO_GIRL", "KIMONO GIRL", None, None),
    ("LEADER", "LEADER", 25, None),
    ("MYSTERY_MAN", "MYSTERY MAN", None, None),
    ("OFFICER", "OFFICER", None, None),
    ("PKMN_TRAINER_1", "{PKMN} TRAINER", None, None),
    ("RIVAL", "{PKMN} TRAINER", 15, None),
    ("ROCKET_ADMIN", "ROCKET ADMIN", None, None),
    ("SAGE", "SAGE", None, None),
    ("SKIER", "SKIER", 10, None),
    ("SUPER_NERD", "SUPER NERD", 8, None),
    ("TEAM_ROCKET", "ROCKET", None, None),
]

MUSIC = [
    ("HG_CHAMPION", 14), ("HG_BOY_1", 15), ("HG_BOY_2", 16), ("HG_GIRL_1", 17), ("HG_GIRL_2", 18),
    ("HG_SUSPICIOUS_1", 19), ("HG_SUSPICIOUS_2", 20), ("HG_SAGE", 21), ("ROCKET", 22),
    ("SILVER", 23), ("HG_KIMONO_GIRL", 24), ("HG_ELITE_FOUR", 25),
]


def edit(path, old, new):
    p = os.path.join(R, path)
    t = open(p).read()
    assert t.count(old) == 1, (path, old)
    open(p, "w").write(t.replace(old, new))


def camel(snake):
    return "".join(w.capitalize() for w in snake.split("_"))


def main():
    pics = [(k, v["to"]) for k, v in common.load_json(os.path.join(common.OUT_DIR, "rename_map.json"))["trainer_pics"].items()
            if v["action"] == "rename"]
    assert len(pics) == 36
    os.makedirs(os.path.join(R, "graphics/trainers/front_pics/johto"), exist_ok=True)
    os.makedirs(os.path.join(R, "graphics/trainers/palettes/johto"), exist_ok=True)

    enum, syms, rows = [], [], []
    for name, const in pics:
        base = name.lower().replace(" ", "_")          # e.g. archer_hns
        new = base[:-4] + "_johto"
        shutil.copy(os.path.join(H, "graphics/trainers/front_pics", base + ".png"),
                    os.path.join(R, "graphics/trainers/front_pics/johto", new + ".png"))
        shutil.copy(os.path.join(H, "graphics/trainers/palettes", base + ".pal"),
                    os.path.join(R, "graphics/trainers/palettes/johto", new + ".pal"))
        sym = camel(new)
        enum.append("    %s," % const)
        syms.append('const u32 gTrainerFrontPic_%s[] = INCGFX_U32("graphics/trainers/front_pics/johto/%s.png", ".4bpp.smol");\n'
                    'const u16 gTrainerPalette_%s[] = INCGFX_U16("graphics/trainers/palettes/johto/%s.pal", ".gbapal");\n'
                    % (sym, new, sym, new))
        rows.append("    [%s] =\n    {\n        .frontPic = TRAINER_FRONT_PIC(gTrainerFrontPic_%s, gTrainerPalette_%s),\n    },\n"
                    % (const, sym, sym))

    edit("include/constants/trainers.h", "    TRAINER_PIC_PAINTER_FRLG,\n",
         "    TRAINER_PIC_PAINTER_FRLG,\n" + "\n".join(enum) + "\n")
    edit("src/data/graphics/trainers.h",
         'const u16 gTrainerPalette_PainterFrlg[] = INCGFX_U16("graphics/trainers/palettes/painter_frlg.pal", ".gbapal");\n',
         'const u16 gTrainerPalette_PainterFrlg[] = INCGFX_U16("graphics/trainers/palettes/painter_frlg.pal", ".gbapal");\n\n'
         + "\n".join(syms))
    edit("src/data/graphics/trainers.h",
         "        .frontPic = TRAINER_FRONT_PIC(gTrainerFrontPic_PainterFrlg, gTrainerPalette_PainterFrlg),\n    },\n",
         "        .frontPic = TRAINER_FRONT_PIC(gTrainerFrontPic_PainterFrlg, gTrainerPalette_PainterFrlg),\n    },\n"
         + "".join(rows))

    edit("include/constants/trainers.h", "    TRAINER_CLASS_CHALLENGER,\n",
         "    TRAINER_CLASS_CHALLENGER,\n\n" + "".join("    TRAINER_CLASS_%s_JOHTO,\n" % c[0] for c in CLASSES))
    lines = []
    for suffix, disp, money, ball in CLASSES:
        body = '_("%s")' % disp
        if money is not None:
            body += ", %d" % money
            if ball:
                body += ", " + ball
        lines.append("    [TRAINER_CLASS_%s_JOHTO] = { %s },\n" % (suffix, body))
    edit("src/battle_main.c", '    [TRAINER_CLASS_CHALLENGER] =           { _("CHALLENGER"), 25, BALL_ULTRA },\n',
         '    [TRAINER_CLASS_CHALLENGER] =           { _("CHALLENGER"), 25, BALL_ULTRA },\n' + "".join(lines))

    edit("include/constants/trainers.h",
         "#define TRAINER_ENCOUNTER_MUSIC_RICH        13 // Used for Rich Boys and Gentlemen\n",
         "#define TRAINER_ENCOUNTER_MUSIC_RICH        13 // Used for Rich Boys and Gentlemen\n\n"
         + "".join("#define TRAINER_ENCOUNTER_MUSIC_%-16s %d\n" % m for m in MUSIC))


if __name__ == "__main__":
    main()
