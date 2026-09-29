#include "text.h"
#include "constants/battle.h"
#include "constants/pokemon.h"

// Bands stack top to bottom; a band is dropped when none of its elements is shown.
enum HealthboxBand
{
    HB_BAND_A, // nick, level
    HB_BAND_B, // HP bar, HP value
    HB_BAND_C, // status pill, stat strip, caught / can-catch icons
    HB_BAND_D, // EXP bar (singles player)
    HB_BAND_COUNT,
};

struct HealthboxElemSpec
{
    u8 band;
    u8 x;    // box-local
    u8 yOff; // from the top of the band
    u8 w;
    u8 h;
};

struct HealthboxLayoutSpec
{
    u8 topPad;
    u8 rightMargin;
    u8 rightSpriteW;
    u8 bandH[HB_BAND_COUNT];
    struct HealthboxElemSpec elems[HB_RECT_COUNT];
};

static const struct HealthboxLayoutSpec sHealthboxSinglesSpec =
{
    .topPad = 2,
    .rightMargin = 6,
    .rightSpriteW = 64,
    .bandH = { 10, 8, 8, 4 },
    .elems =
    {
        [HB_RECT_NICK]     = { HB_BAND_A,  5, 0, 56, 8 },
        [HB_RECT_LEVEL]    = { HB_BAND_A, 62, 0, 36, 8 },
        [HB_RECT_HP_BAR]   = { HB_BAND_B,  5, 2, 48, 4 },
        [HB_RECT_HP_VALUE] = { HB_BAND_B, 58, 0, 40, 8 },
        [HB_RECT_EXP]      = { HB_BAND_D, 34, 1, 64, 2 },
        [HB_RECT_STATUS]   = { HB_BAND_C,  5, 0, 24, 8 },
        [HB_RECT_STRIP]    = { HB_BAND_C, 32, 1, 40, 6 },
        [HB_RECT_CAUGHT]   = { HB_BAND_C, 82, 0, 8, 8 },
        [HB_RECT_CATCHABLE] = { HB_BAND_C, 90, 0, 8, 8 },
    },
};

// The 26 px height is a hard limit (stacked boxes and the textbox at y 112); the anchors below
// leave exactly one spare row between the two boxes of a side.
static const struct HealthboxLayoutSpec sHealthboxDoublesSpec =
{
    .topPad = 2,
    .rightMargin = 4,
    .rightSpriteW = 32,
    .bandH = { 10, 7, 7, 0 },
    .elems =
    {
        [HB_RECT_NICK]     = { HB_BAND_A,  4, 0, 48, 8 },
        [HB_RECT_LEVEL]    = { HB_BAND_A, 59, 0, 32, 8 },
        [HB_RECT_HP_BAR]   = { HB_BAND_B,  4, 1, 48, 4 },
        [HB_RECT_HP_VALUE] = { HB_BAND_B,  4, 0, 48, 6 },
        [HB_RECT_STATUS]   = { HB_BAND_C,  4, 0, 24, 7 },
        [HB_RECT_STRIP]    = { HB_BAND_C, 32, 0, 40, 6 },
        [HB_RECT_CAUGHT]   = { HB_BAND_C, 76, 0, 8, 7 },
        [HB_RECT_CATCHABLE] = { HB_BAND_C, 84, 0, 8, 7 },
    },
};

struct HealthboxAnchor
{
    s16 x;
    s16 y;              // top edge, or bottom edge when bottomAnchored
    bool8 rightAnchored;
    bool8 bottomAnchored;
};

// Indexed by B_POSITION_*; singles use the first two entries (player, foe).
static const struct HealthboxAnchor sHealthboxSinglesAnchors[] =
{
    [B_POSITION_PLAYER_LEFT]   = { 231, 106, TRUE,  TRUE },
    [B_POSITION_OPPONENT_LEFT] = {  13,   4, FALSE, FALSE },
};

static const struct HealthboxAnchor sHealthboxDoublesAnchors[] =
{
    [B_POSITION_PLAYER_LEFT]    = { 228,  85, TRUE,  TRUE },
    [B_POSITION_OPPONENT_LEFT]  = {  13,   5, FALSE, FALSE },
    [B_POSITION_PLAYER_RIGHT]   = { 240, 112, TRUE,  TRUE },
    [B_POSITION_OPPONENT_RIGHT] = {   1,  32, FALSE, FALSE },
};

// Indexed by HB_PAL_*. Slots 12-15 are status colours written at runtime.
static const u16 sHealthboxNewPal[16] =
{
    [0]              = RGB_BLACK,
    [HB_PAL_TEXT]    = RGB(31, 31, 31),
    [HB_PAL_FILL]    = RGB(5, 6, 9),
    [HB_PAL_SHADOW]  = RGB(2, 3, 5),
    [HB_PAL_RIM]     = RGB(14, 16, 20),
    [HB_PAL_EXP]     = RGB(11, 20, 31),
    [HB_PAL_TROUGH]  = RGB(10, 13, 12),
    [HB_PAL_ATK]     = RGB(29, 12, 6),
    [HB_PAL_DEF]     = RGB(29, 25, 5),
    [HB_PAL_SPDEF]   = RGB(9, 26, 10),
    [HB_PAL_SPE]     = RGB(31, 19, 18),
    [HB_PAL_SPATK]   = RGB(8, 25, 31),
};

// Horizontal inset of the box's top and bottom rows; the corner radius is the table length.
static const u8 sHealthboxCornerInset[] = { 3, 1, 1 };

static const union TextColor sHealthboxTextColor =
{
    .background = 0,
    .foreground = HB_PAL_TEXT,
    .shadow = HB_PAL_SHADOW,
    .accent = 0,
};

// FONT_SMALL glyph ink starts this many rows below the print y.
#define HB_TEXT_INK_OFFSET 3

struct HealthboxStatusSpec
{
    u32 mask;
    u8 palId; // PAL_STATUS_*
    const u8 *label;
};

// Checked in order; the first match is shown (same priority as Classic).
static const struct HealthboxStatusSpec sHealthboxStatusSpecs[] =
{
    { STATUS1_SLEEP,      PAL_STATUS_SLP, COMPOUND_STRING("SLP") },
    { STATUS1_PSN_ANY,    PAL_STATUS_PSN, COMPOUND_STRING("PSN") },
    { STATUS1_BURN,       PAL_STATUS_BRN, COMPOUND_STRING("BRN") },
    { STATUS1_FREEZE,     PAL_STATUS_FRZ, COMPOUND_STRING("FRZ") },
    { STATUS1_FROSTBITE,  PAL_STATUS_FRZ, COMPOUND_STRING("FRB") },
    { STATUS1_PARALYSIS,  PAL_STATUS_PAR, COMPOUND_STRING("PAR") },
};

// Status pill labels use their own glyphs; the text fonts' 8-row capitals do not fit the pill.
#define HB_PILL_GLYPH_W   5
#define HB_PILL_GLYPH_H   5
#define HB_PILL_GLYPH_GAP 1

// Indexed by letter, CHAR_A-relative. One byte per row; bit 4 is the leftmost pixel.
// Only the letters used by sHealthboxStatusSpecs and the gimmick badge labels are drawn; the rest
// render blank.
static const u8 sPillGlyphs[26][HB_PILL_GLYPH_H] =
{
    [0]  = { 0x0E, 0x11, 0x1F, 0x11, 0x11 }, // A
    [1]  = { 0x1E, 0x11, 0x1E, 0x11, 0x1E }, // B
    [3]  = { 0x1E, 0x11, 0x11, 0x11, 0x1E }, // D
    [4]  = { 0x1F, 0x10, 0x1E, 0x10, 0x1F }, // E
    [5]  = { 0x1F, 0x10, 0x1E, 0x10, 0x10 }, // F
    [6]  = { 0x0F, 0x10, 0x13, 0x11, 0x0F }, // G
    [11] = { 0x10, 0x10, 0x10, 0x10, 0x1F }, // L
    [12] = { 0x11, 0x1B, 0x15, 0x11, 0x11 }, // M
    [13] = { 0x11, 0x19, 0x15, 0x13, 0x11 }, // N
    [14] = { 0x0E, 0x11, 0x11, 0x11, 0x0E }, // O
    [15] = { 0x1E, 0x11, 0x1E, 0x10, 0x10 }, // P
    [17] = { 0x1E, 0x11, 0x1E, 0x12, 0x11 }, // R
    [18] = { 0x0F, 0x10, 0x0E, 0x01, 0x1E }, // S
    [19] = { 0x1F, 0x04, 0x04, 0x04, 0x04 }, // T
    [21] = { 0x11, 0x11, 0x11, 0x0A, 0x04 }, // V
    [23] = { 0x11, 0x0A, 0x04, 0x0A, 0x11 }, // X
    [25] = { 0x1F, 0x02, 0x04, 0x08, 0x1F }, // Z
};

// Compact HP-value glyphs. FONT_SMALL ink is 8 rows tall; the doubles HP value band is 6, so the
// font bleeds onto the status pill below it. Doubles draw the value with these instead.
#define HB_NUM_GLYPH_W   4
#define HB_NUM_GLYPH_H   5
#define HB_NUM_GLYPH_GAP 1

enum
{
    HB_NUM_SLASH = 10,
    HB_NUM_PERCENT,
    HB_NUM_COUNT,
};

// One byte per row; bit 3 is the leftmost pixel.
static const u8 sHpValueGlyphs[HB_NUM_COUNT][HB_NUM_GLYPH_H] =
{
    [0]              = { 0x6, 0x9, 0x9, 0x9, 0x6 },
    [1]              = { 0x2, 0x6, 0x2, 0x2, 0x7 },
    [2]              = { 0x6, 0x9, 0x2, 0x4, 0xF },
    [3]              = { 0xE, 0x1, 0x6, 0x1, 0xE },
    [4]              = { 0x9, 0x9, 0xF, 0x1, 0x1 },
    [5]              = { 0xF, 0x8, 0xE, 0x1, 0xE },
    [6]              = { 0x6, 0x8, 0xE, 0x9, 0x6 },
    [7]              = { 0xF, 0x1, 0x2, 0x4, 0x4 },
    [8]              = { 0x6, 0x9, 0x6, 0x9, 0x6 },
    [9]              = { 0x6, 0x9, 0x7, 0x1, 0x6 },
    [HB_NUM_SLASH]   = { 0x1, 0x2, 0x2, 0x4, 0x8 },
    [HB_NUM_PERCENT] = { 0x9, 0x1, 0x2, 0x4, 0x9 },
};

#define HB_STRIP_SLOTS   5
#define HB_STRIP_SLOT_W  8
#define HB_GLYPH_W       5
#define HB_GLYPH_H       6

// Strip order is not the STAT_* enum order.
static const u8 sStatStripStats[HB_STRIP_SLOTS] = { STAT_ATK, STAT_DEF, STAT_SPATK, STAT_SPDEF, STAT_SPEED };

static const u8 sStatStripHue[HB_STRIP_SLOTS] = { HB_PAL_ATK, HB_PAL_DEF, HB_PAL_SPATK, HB_PAL_SPDEF, HB_PAL_SPE };

// Indexed by HB_GLYPH_*. One byte per row; bit 4 is the leftmost pixel.
static const u8 sStatGlyphs[HB_GLYPH_COUNT][HB_GLYPH_H] =
{
    [HB_GLYPH_UP1]   = { 0x00, 0x00, 0x04, 0x0A, 0x00, 0x00 },
    [HB_GLYPH_UP2]   = { 0x00, 0x04, 0x0A, 0x04, 0x0A, 0x00 },
    [HB_GLYPH_UP3]   = { 0x04, 0x0A, 0x04, 0x0A, 0x04, 0x0A },
    [HB_GLYPH_DOWN1] = { 0x00, 0x00, 0x0A, 0x04, 0x00, 0x00 },
    [HB_GLYPH_DOWN2] = { 0x00, 0x0A, 0x04, 0x0A, 0x04, 0x00 },
    [HB_GLYPH_DOWN3] = { 0x0A, 0x04, 0x0A, 0x04, 0x0A, 0x04 },
};
