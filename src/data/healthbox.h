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
