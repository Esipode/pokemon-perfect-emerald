#include "text.h"
#include "constants/battle.h"

// Bands stack top to bottom; a band is dropped when none of its elements is shown.
enum HealthboxBand
{
    HB_BAND_A, // nick, level
    HB_BAND_B, // HP bar, HP value (doubles: also the stat strip)
    HB_BAND_C, // status pill, caught icons (singles: also the stat strip)
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
        [HB_RECT_STATUS]   = { HB_BAND_C,  5, 0, 24, 7 },
        [HB_RECT_STRIP]    = { HB_BAND_C, 32, 1, 40, 6 },
        [HB_RECT_CAUGHT]   = { HB_BAND_C, 82, 0, 16, 8 },
    },
};

// The 24 px height is a hard limit (stacked boxes and the textbox at y 112).
static const struct HealthboxLayoutSpec sHealthboxDoublesSpec =
{
    .topPad = 2,
    .rightMargin = 4,
    .rightSpriteW = 32,
    .bandH = { 9, 6, 7, 0 },
    .elems =
    {
        [HB_RECT_NICK]     = { HB_BAND_A,  4, 0, 48, 8 },
        [HB_RECT_LEVEL]    = { HB_BAND_A, 59, 0, 32, 8 },
        [HB_RECT_HP_BAR]   = { HB_BAND_B,  4, 1, 48, 4 },
        [HB_RECT_HP_VALUE] = { HB_BAND_B,  4, 0, 48, 6 },
        [HB_RECT_STRIP]    = { HB_BAND_B, 52, 0, 40, 6 },
        [HB_RECT_STATUS]   = { HB_BAND_C,  4, 0, 24, 7 },
        [HB_RECT_CAUGHT]   = { HB_BAND_C, 76, 0, 16, 7 },
    },
};

struct HealthboxAnchor
{
    s16 x;
    s16 y;
    bool8 rightAnchored;
};

// Indexed by B_POSITION_*; singles use the first two entries (player, foe).
static const struct HealthboxAnchor sHealthboxSinglesAnchors[] =
{
    [B_POSITION_PLAYER_LEFT]   = { 231, 58, TRUE },
    [B_POSITION_OPPONENT_LEFT] = {  13, 16, FALSE },
};

static const struct HealthboxAnchor sHealthboxDoublesAnchors[] =
{
    [B_POSITION_PLAYER_LEFT]    = { 228, 62, TRUE },
    [B_POSITION_OPPONENT_LEFT]  = {  13,  5, FALSE },
    [B_POSITION_PLAYER_RIGHT]   = { 240, 87, TRUE },
    [B_POSITION_OPPONENT_RIGHT] = {   1, 30, FALSE },
};

// Indexed by HB_PAL_*. Slots 12-15 are status colours written at runtime.
static const u16 sHealthboxNewPal[16] =
{
    [0]              = RGB_BLACK,
    [HB_PAL_TEXT]    = RGB(31, 31, 31),
    [HB_PAL_FILL]    = RGB(5, 6, 9),
    [HB_PAL_SHADOW]  = RGB(2, 3, 5),
    [HB_PAL_RIM]     = RGB(14, 16, 20),
    [HB_PAL_EXP]     = RGB(6, 14, 28),
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

// Status pill label font; its ink is this many rows tall.
#define HB_PILL_FONT     FONT_SMALL_NARROW
#define HB_PILL_INK_H    6
