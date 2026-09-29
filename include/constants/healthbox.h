#ifndef GUARD_CONSTANTS_HEALTHBOX_H
#define GUARD_CONSTANTS_HEALTHBOX_H

#define HEALTHBOX_STYLE_CLASSIC 0
#define HEALTHBOX_STYLE_NEW     1

// Style used by saves that predate the healthbox options.
#define HEALTHBOX_STYLE_DEFAULT HEALTHBOX_STYLE_NEW

#define HB_HPVAL_NONE    0
#define HB_HPVAL_NUMBERS 1
#define HB_HPVAL_PERCENT 2

#define HB_BG_SOLID 0
#define HB_BG_NONE  1

// Per-side detail presets; CUSTOM means that side's toggles match none of the others.
enum
{
    HB_PRESET_MINIMAL,
    HB_PRESET_STANDARD,
    HB_PRESET_FULL,
    HB_PRESET_CUSTOM,
    HB_PRESET_COUNT,
};

#define HB_SIDE_PLAYER 0
#define HB_SIDE_FOE    1

// New-style healthbox palette slots (see sHealthboxNewPal).
#define HB_PAL_TEXT   1
#define HB_PAL_FILL   2
#define HB_PAL_SHADOW 3
#define HB_PAL_RIM    4
#define HB_PAL_EXP    5
#define HB_PAL_TROUGH 6 // Shared with the enemy shadow sprite; must not change.
#define HB_PAL_ATK    7
#define HB_PAL_DEF    8
#define HB_PAL_SPDEF  9
#define HB_PAL_SPE    10 // Also the female gender colour.
#define HB_PAL_SPATK  11 // Also the male gender colour.
#define HB_PAL_STATUS_FIRST 12

// New-style HP bar sprite (uses the shared bar palette, not sHealthboxNewPal).
#define HB_HP_BAR_W     48
#define HB_HP_BAR_H     4
#define HB_HP_BAR_TILES (HB_HP_BAR_W / 8)
#define HB_HP_BAR_ROW   2 // First bar row inside each 8x8 tile.
#define HB_PAL_BAR_TROUGH 1
#define HB_PAL_BAR_TRAIL  2 // Damage trail.
#define HB_BAR_DATA6_NEW  3 // Bar sprite follows the box main sprite's centre.

// Status colours; sStatusIconColors in battle_interface.c is indexed by these.
enum
{
    PAL_STATUS_PSN,
    PAL_STATUS_PAR,
    PAL_STATUS_SLP,
    PAL_STATUS_FRZ,
    PAL_STATUS_BRN
};

// Stat strip glyphs; HB_GLYPH_NONE draws nothing (stage 6).
enum HealthboxGlyph
{
    HB_GLYPH_UP1,
    HB_GLYPH_UP2,
    HB_GLYPH_UP3,
    HB_GLYPH_DOWN1,
    HB_GLYPH_DOWN2,
    HB_GLYPH_DOWN3,
    HB_GLYPH_COUNT,
    HB_GLYPH_NONE = HB_GLYPH_COUNT,
};

enum HealthboxElement
{
    HB_ELEM_NICK,
    HB_ELEM_LEVEL,
    HB_ELEM_HP_BAR,
    HB_ELEM_EXP,         // player side only
    HB_ELEM_STATUS,
    HB_ELEM_TYPES,
    HB_ELEM_CAUGHT,      // foe side only
    HB_ELEM_STAT_STAGES,
    HB_ELEM_CATCHABLE,   // foe side only
    HB_ELEM_COUNT,
};

// Layout rects; types has no rect because the type icons are separate sprites.
enum HealthboxRectId
{
    HB_RECT_NICK,
    HB_RECT_LEVEL,
    HB_RECT_HP_BAR,
    HB_RECT_HP_VALUE,
    HB_RECT_EXP,
    HB_RECT_STATUS,
    HB_RECT_STRIP,
    HB_RECT_CAUGHT,
    HB_RECT_CATCHABLE,
    HB_RECT_COUNT,
};

#endif // GUARD_CONSTANTS_HEALTHBOX_H
