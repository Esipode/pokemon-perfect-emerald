// Display order of the stat rows, as indices into enum Stat.
static const u8 sPartyDashStatOrder[PARTY_DASH_STAT_COUNT] =
{
    [PARTY_DASH_STAT_HP]    = STAT_HP,
    [PARTY_DASH_STAT_ATK]   = STAT_ATK,
    [PARTY_DASH_STAT_DEF]   = STAT_DEF,
    [PARTY_DASH_STAT_SPATK] = STAT_SPATK,
    [PARTY_DASH_STAT_SPDEF] = STAT_SPDEF,
    [PARTY_DASH_STAT_SPEED] = STAT_SPEED,
};

// BG1 chrome tiles are generated at runtime (PartyDashboard_LoadGfx): flat fills and slot frame edges.
enum PartyDashChromeTile
{
    CHROME_TILE_BLANK,
    CHROME_TILE_FILL,
    CHROME_TILE_TOP,
    CHROME_TILE_BOTTOM,
    CHROME_TILE_LEFT,
    CHROME_TILE_RIGHT,
    CHROME_TILE_TOP_LEFT,
    CHROME_TILE_TOP_RIGHT,
    CHROME_TILE_BOTTOM_LEFT,
    CHROME_TILE_BOTTOM_RIGHT,
    CHROME_TILE_TAB_FILL,
    CHROME_TILE_HINT_FILL,
    CHROME_TILE_COUNT,
};

#define CHROME_EDGE_TOP    (1 << 0)
#define CHROME_EDGE_BOTTOM (1 << 1)
#define CHROME_EDGE_LEFT   (1 << 2)
#define CHROME_EDGE_RIGHT  (1 << 3)
#define CHROME_EDGE_PX     2

static const u8 sChromeTileEdges[CHROME_TILE_COUNT] =
{
    [CHROME_TILE_TOP]          = CHROME_EDGE_TOP,
    [CHROME_TILE_BOTTOM]       = CHROME_EDGE_BOTTOM,
    [CHROME_TILE_LEFT]         = CHROME_EDGE_LEFT,
    [CHROME_TILE_RIGHT]        = CHROME_EDGE_RIGHT,
    [CHROME_TILE_TOP_LEFT]     = CHROME_EDGE_TOP | CHROME_EDGE_LEFT,
    [CHROME_TILE_TOP_RIGHT]    = CHROME_EDGE_TOP | CHROME_EDGE_RIGHT,
    [CHROME_TILE_BOTTOM_LEFT]  = CHROME_EDGE_BOTTOM | CHROME_EDGE_LEFT,
    [CHROME_TILE_BOTTOM_RIGHT] = CHROME_EDGE_BOTTOM | CHROME_EDGE_RIGHT,
};

// Palette indices used by the generated tiles.
#define CHROME_PIX_FILL 1
#define CHROME_PIX_EDGE 2 // Slot frame; tab strip fill when used with palette 0.
#define CHROME_PIX_HINT 3

// Rows of gPartyDashboard_Pal (16 colours each). Rows other than ROW_CHROME and ROW_INFO are
// loaded into the slot palettes (BG palettes 3-8) on demand.
enum PartyDashPalRow
{
    PAL_ROW_CHROME,
    PAL_ROW_SLOT_NORMAL,
    PAL_ROW_SLOT_SELECTED,
    PAL_ROW_SLOT_FAINTED,
    PAL_ROW_SLOT_FAINTED_SELECTED,
    PAL_ROW_SLOT_ACTION,
    PAL_ROW_SLOT_EMPTY,
    PAL_ROW_INFO,
};

// Slot palette entries (rows PAL_ROW_SLOT_*): 1 fill, 2 frame, then text and bar colours.
#define SLOT_PIX_FILL       1
#define SLOT_PIX_TEXT       3
#define SLOT_PIX_SHADOW     4
#define SLOT_PIX_HP_GREEN   5
#define SLOT_PIX_HP_YELLOW  6
#define SLOT_PIX_HP_RED     7
#define SLOT_PIX_HP_TRACK   8
#define SLOT_PIX_HATCH      9

// Slot fill recolour for choose-half / eligibility states; index 1 = selected.
enum PartyDashSlotState
{
    SLOT_STATE_NONE,
    SLOT_STATE_DIM,
    SLOT_STATE_PICKED,
};
static const u16 sSlotDimFill[2] = {RGB(2, 3, 4), RGB(4, 5, 8)};
static const u16 sSlotPickedFill[2] = {RGB(6, 16, 10), RGB(11, 22, 14)};

// Slot window (4x1 tiles, bottom row of the slot): bar and label geometry in pixels.
#define SLOT_BAR_X          2
#define SLOT_BAR_Y          3
#define SLOT_BAR_W          28
#define SLOT_BAR_H          3
#define SLOT_BAR_RECRUIT_W  15 // Narrowed to leave room for the battles-left label.
#define SLOT_LABEL_X        19
#define SLOT_WIN_W          (SLOT_W_TILES * TILE_WIDTH)

// Info palette (BG palette 12) entries (0 transparent, 1 panel fill), shared by every info-area window.
#define INFO_PIX_TEXT        2
#define INFO_PIX_SHADOW      3
#define INFO_PIX_LABEL       4
#define INFO_PIX_NATURE_UP   5
#define INFO_PIX_NATURE_DOWN 6
#define INFO_PIX_HP_GREEN    7
#define INFO_PIX_HP_YELLOW   8
#define INFO_PIX_HP_RED      9
#define INFO_PIX_EXP         10
#define INFO_PIX_HATCH       11
#define INFO_PIX_TRACK       12
#define INFO_PIX_STAT_BAR    13
#define INFO_PIX_ACCENT      15

// Identity window (12x8 tiles) geometry in pixels.
#define IDENT_TEXT_X         2
#define IDENT_RIGHT_X        94
#define IDENT_NAME_Y         2
#define IDENT_NAME_MAX_W     76
#define IDENT_LEVEL_Y        16
#define IDENT_HP_Y           28
#define IDENT_HP_BAR_Y       42
#define IDENT_HP_BAR_H       4
#define IDENT_EXP_Y          44
#define IDENT_EXP_BAR_Y      58
#define IDENT_EXP_BAR_H      3
#define IDENT_BAR_X          4

// BG3 tile cells for the type icons (identity rows 16-31) and the status chip sprite centre.
#define IDENT_ICON_COL       5
#define IDENT_ICON_ROW       (IDENT_Y + 2)
#define IDENT_TERA_COL       7
#define IDENT_CHIP_X         78
#define IDENT_CHIP_Y         (IDENT_Y * TILE_HEIGHT + 24)

#define PARTY_DASH_PAL_CHROME     0
#define PARTY_DASH_PAL_SLOT_FIRST 3 // Slot 0 must keep BG palette 3 (first-battle fade masks).
#define PARTY_DASH_PAL_TYPE_1     9
#define PARTY_DASH_PAL_TYPE_2     10
#define PARTY_DASH_PAL_INFO       12

// BG3 type-icon tiles: tile 0 stays blank, then the two 8x16 icon sheets (20 tiles each).
#define BG3_TILE_BLANK   0
#define BG3_TILE_SHEET_1 1
#define BG3_TILE_SHEET_2 21
#define BG3_TILE_COUNT   41
#define BG3_SHEET_TILES  20

// Selected slot frame (palette entry 2) pulse.
#define SLOT_PIX_FRAME   2
#define PULSE_FRAMES     8
static const u16 sPulseColors[] = {RGB(31, 26, 8), RGB(31, 28, 14), RGB(31, 30, 20), RGB(31, 28, 14)};

// Layout in tiles. Slot = 4x6; grid = 3x2 slots.
#define SLOT_W_TILES 4
#define SLOT_H_TILES 6

#define IDENT_X 0
#define IDENT_Y 12
#define IDENT_W 12
#define IDENT_H 8
#define TABS_X  12
#define TABS_Y  0
#define TABS_W  18
#define TABS_H  2
#define BODY_X  12
#define BODY_Y  2
#define BODY_W  18
#define BODY_H  16
#define HINT_X  12
#define HINT_Y  18
#define HINT_W  18
#define HINT_H  2

// Window tiles start after both window-border sets (0x4F and the first-battle tutorial's 0x58).
// WIN_MSG keeps its legacy block (0x21F): the transient prompt windows at 0x24F/0x279 overlap it, so
// no permanent window may extend past 0x21F. The hint window is allocated dynamically above the
// item/mail submenu block.
#define HINT_BASE_BLOCK 0x3CE
#define HINT_TEXT_X     4
#define HINT_TEXT_Y     1

static const u8 sText_DashIdleHint[] = _("{A_BUTTON}Actions {B_BUTTON}Back {L_BUTTON}{R_BUTTON}Tab");

enum
{
    WIN_DASH_IDENT = PARTY_SIZE + 1,
    WIN_DASH_TABS,
    WIN_DASH_BODY,
    WIN_DASH_LABEL_FIRST, // One per slot; ids up to WIN_DASH_LABEL_FIRST + PARTY_SIZE - 1.
};

#define SLOT_WINDOW(slot, base)                                       \
    {                                                                 \
        .bg = 0,                                                      \
        .tilemapLeft = ((slot) % PARTY_DASH_COLUMNS) * SLOT_W_TILES,  \
        .tilemapTop = ((slot) / PARTY_DASH_COLUMNS) * SLOT_H_TILES + SLOT_H_TILES - 1, \
        .width = SLOT_W_TILES,                                        \
        .height = 1,                                                  \
        .paletteNum = PARTY_DASH_PAL_SLOT_FIRST + (slot),             \
        .baseBlock = (base),                                          \
    }

// Label window per slot (LOCKED text, Recruits battles-left): 4x3 tiles on BG3 (own tiles after the icon sheets), ending on the slot's bar row.
#define LABEL_WINDOW(slot)                                                                 \
    [WIN_DASH_LABEL_FIRST + (slot)] = {                                                    \
        .bg = 3,                                                                            \
        .tilemapLeft = ((slot) % PARTY_DASH_COLUMNS) * SLOT_W_TILES,                        \
        .tilemapTop = ((slot) / PARTY_DASH_COLUMNS) * SLOT_H_TILES + SLOT_H_TILES - 3,      \
        .width = SLOT_W_TILES,                                                              \
        .height = 3,                                                                        \
        .paletteNum = PARTY_DASH_PAL_SLOT_FIRST + (slot),                                   \
        .baseBlock = BG3_TILE_COUNT + (slot) * SLOT_W_TILES * 3,                            \
    }
#define LOCKED_TEXT_Y 4  // 12 px above the bar row.
#define RECRUIT_TEXT_Y 10 // 6 px above the bar row.

static const struct WindowTemplate sPartyDashboardWindowTemplate[] =
{
    SLOT_WINDOW(0, 0x61),
    SLOT_WINDOW(1, 0x65),
    SLOT_WINDOW(2, 0x69),
    SLOT_WINDOW(3, 0x6D),
    SLOT_WINDOW(4, 0x71),
    SLOT_WINDOW(5, 0x75),
    { // WIN_MSG
        .bg = 2,
        .tilemapLeft = 1,
        .tilemapTop = 15,
        .width = 28,
        .height = 4,
        .paletteNum = 14,
        .baseBlock = 0x21F,
    },
    [WIN_DASH_IDENT] = {
        .bg = 0,
        .tilemapLeft = IDENT_X,
        .tilemapTop = IDENT_Y,
        .width = IDENT_W,
        .height = IDENT_H,
        .paletteNum = PARTY_DASH_PAL_INFO,
        .baseBlock = 0x79,
    },
    [WIN_DASH_TABS] = {
        .bg = 0,
        .tilemapLeft = TABS_X,
        .tilemapTop = TABS_Y,
        .width = TABS_W,
        .height = TABS_H,
        .paletteNum = PARTY_DASH_PAL_INFO,
        .baseBlock = 0xD9,
    },
    [WIN_DASH_BODY] = {
        .bg = 0,
        .tilemapLeft = BODY_X,
        .tilemapTop = BODY_Y,
        .width = BODY_W,
        .height = BODY_H,
        .paletteNum = PARTY_DASH_PAL_INFO,
        .baseBlock = 0xFD,
    },
    LABEL_WINDOW(0),
    LABEL_WINDOW(1),
    LABEL_WINDOW(2),
    LABEL_WINDOW(3),
    LABEL_WINDOW(4),
    LABEL_WINDOW(5),
    DUMMY_WIN_TEMPLATE
};

static const struct WindowTemplate sPartyDashboardHintWindowTemplate =
{
    .bg = 0,
    .tilemapLeft = HINT_X,
    .tilemapTop = HINT_Y,
    .width = HINT_W,
    .height = HINT_H,
    .paletteNum = PARTY_DASH_PAL_INFO,
    .baseBlock = HINT_BASE_BLOCK,
};

// Sprite coordinates per slot: icon, held-item marker, status chip, ball (x, y centres).
#define SLOT_SPRITE_COORDS(col, row)                                          \
    {                                                                         \
        (col) * 32 + 16, (row) * 48 + 18,                                     \
        (col) * 32 + 28, (row) * 48 + 6,                                      \
        (col) * 32 + 16, (row) * 48 + 38,                                     \
        (col) * 32 + 20, (row) * 48 + 18,                                     \
    }

static const u8 sPartyDashSpriteCoords[PARTY_DASH_MAX_SLOTS][8] =
{
    SLOT_SPRITE_COORDS(0, 0),
    SLOT_SPRITE_COORDS(1, 0),
    SLOT_SPRITE_COORDS(2, 0),
    SLOT_SPRITE_COORDS(0, 1),
    SLOT_SPRITE_COORDS(1, 1),
    SLOT_SPRITE_COORDS(2, 1),
};

static const u8 sText_DashTabInfo[] = _("INFO");
static const u8 sText_DashTabStats[] = _("STATS");
static const u8 sText_DashLocked[] = _("LOCKED");
static const u8 sSlotTextColors[3] = {TEXT_COLOR_TRANSPARENT, SLOT_PIX_TEXT, SLOT_PIX_SHADOW};
static const u8 sSlotRecruitColors[3] = {TEXT_COLOR_TRANSPARENT, SLOT_PIX_HP_RED, SLOT_PIX_SHADOW};
static const u8 sSlotHpBarPix[] =
{
    [HP_BAR_EMPTY]  = SLOT_PIX_HP_RED,
    [HP_BAR_RED]    = SLOT_PIX_HP_RED,
    [HP_BAR_YELLOW] = SLOT_PIX_HP_YELLOW,
    [HP_BAR_GREEN]  = SLOT_PIX_HP_GREEN,
    [HP_BAR_FULL]   = SLOT_PIX_HP_GREEN,
};

static const u8 sText_DashActive[] = _("ACTIVE");
static const u8 sText_DashEgg[] = _("EGG");
static const u8 sText_DashHp[] = _("HP");
static const u8 sText_DashExp[] = _("EXP");
static const u8 sText_DashHatch[] = _("HATCH");
static const u8 sText_DashMax[] = _("MAX");
static const u8 sText_DashToNext[] = _(" to next");
static const u8 sText_DashSlash[] = _("/");
static const u8 sText_DashPercent[] = _("%");
static const u8 sIdentTextColors[3] = {TEXT_COLOR_TRANSPARENT, INFO_PIX_TEXT, INFO_PIX_SHADOW};
static const u8 sIdentLabelColors[3] = {TEXT_COLOR_TRANSPARENT, INFO_PIX_LABEL, INFO_PIX_SHADOW};
static const u8 sIdentFaintedColors[3] = {TEXT_COLOR_TRANSPARENT, INFO_PIX_HP_RED, INFO_PIX_SHADOW};
static const u8 sIdentMaleColors[3] = {TEXT_COLOR_TRANSPARENT, INFO_PIX_NATURE_DOWN, INFO_PIX_SHADOW};
static const u8 sIdentFemaleColors[3] = {TEXT_COLOR_TRANSPARENT, INFO_PIX_NATURE_UP, INFO_PIX_SHADOW};
static const u8 sIdentHpBarPix[] =
{
    [HP_BAR_EMPTY]  = INFO_PIX_HP_RED,
    [HP_BAR_RED]    = INFO_PIX_HP_RED,
    [HP_BAR_YELLOW] = INFO_PIX_HP_YELLOW,
    [HP_BAR_GREEN]  = INFO_PIX_HP_GREEN,
    [HP_BAR_FULL]   = INFO_PIX_HP_GREEN,
};

static const u8 *const sDashTabLabels[PARTY_DASH_TAB_COUNT] =
{
    [PARTY_DASH_TAB_INFO]  = sText_DashTabInfo,
    [PARTY_DASH_TAB_STATS] = sText_DashTabStats,
};
static const u8 sDashTabX[PARTY_DASH_TAB_COUNT] = {8, 48};

// Tab strip window (144x16 px): label row and the underline of the active tab.
#define TAB_LABEL_Y      1
#define TAB_UNDERLINE_Y  12
#define TAB_UNDERLINE_H  2
#define TAB_ACTIVE_RIGHT_X 138 // Right edge of the doubles ACTIVE tag.

// Item icon sprite (INFO tab): one tile tag and one palette tag, the spare OBJ palette.
#define TAG_DASH_ITEM_ICON 55130

// Body window (144x128 px) geometry in pixels. BODY_X/BODY_Y are in tiles, so pixel offsets are added to them.
#define INFO_LABEL_X       2
#define INFO_VALUE_X       50
#define INFO_RIGHT_X       142
#define INFO_NATURE_Y      0
#define INFO_ABILITY_Y     13
#define INFO_ITEM_Y        26
// Small-font glyphs sit one row higher in their cell than normal-font glyphs, so labels drop 1 px to share a baseline.
#define INFO_LABEL_DY      1
#define INFO_DIVIDER_Y     58
#define INFO_MOVES_Y       64
#define INFO_MOVE_STEP     16
#define INFO_MOVE_NAME_X   12
#define INFO_MOVE_NAME_W   90
#define INFO_VALUE_W       90
#define INFO_ITEM_NAME_W   64
#define INFO_ITEM_ICON_X   (BODY_X * TILE_WIDTH + 128)
#define INFO_ITEM_ICON_Y   (BODY_Y * TILE_HEIGHT + 36)
#define INFO_EGG_TEXT_Y    44

static const u8 sText_DashNature[] = _("NATURE");
static const u8 sText_DashAbility[] = _("ABILITY");
static const u8 sText_DashItem[] = _("ITEM");
static const u8 sText_DashNatureUp[] = _("+");
static const u8 sText_DashNatureDown[] = _("-");
static const u8 sText_DashStatHp[] = _("HP");
static const u8 sText_DashStatAtk[] = _("ATK");
static const u8 sText_DashStatDef[] = _("DEF");
static const u8 sText_DashStatSpe[] = _("SPE");
static const u8 sText_DashStatSpa[] = _("SPA");
static const u8 sText_DashStatSpd[] = _("SPD");

// Indexed by enum Stat.
static const u8 *const sDashStatLabels[NUM_STATS] =
{
    [STAT_HP]    = sText_DashStatHp,
    [STAT_ATK]   = sText_DashStatAtk,
    [STAT_DEF]   = sText_DashStatDef,
    [STAT_SPEED] = sText_DashStatSpe,
    [STAT_SPATK] = sText_DashStatSpa,
    [STAT_SPDEF] = sText_DashStatSpd,
};

static const u8 sInfoUpColors[3] = {TEXT_COLOR_TRANSPARENT, INFO_PIX_NATURE_UP, INFO_PIX_SHADOW};
static const u8 sInfoDownColors[3] = {TEXT_COLOR_TRANSPARENT, INFO_PIX_NATURE_DOWN, INFO_PIX_SHADOW};
static const u8 sInfoPpLowColors[3] = {TEXT_COLOR_TRANSPARENT, INFO_PIX_HP_YELLOW, INFO_PIX_SHADOW};

// STATS tab geometry in pixels (body window). Columns are right-aligned to the given x.
#define STATS_HEADER_Y      0
#define STATS_ROWS_Y        14
#define STATS_ROW_STEP      16
#define STATS_BAR_X         28
#define STATS_BAR_W         56
#define STATS_BAR_W_WIDE    80 // No IV/EV columns.
#define STATS_BAR_H         4
#define STATS_BAR_DY        5
#define STATS_VALUE_RIGHT_X 104
#define STATS_VALUE_WIDE_X  140
#define STATS_IV_RIGHT_X    122
#define STATS_EV_RIGHT_X    142
#define STATS_EV_TOTAL_Y    112

static const u8 sText_DashStatHeader[] = _("STAT");
static const u8 sText_DashValueHeader[] = _("VALUE");
static const u8 sText_DashIvHeader[] = _("IV");
static const u8 sText_DashEvHeader[] = _("EV");
static const u8 sText_DashEvTotal[] = _("EV TOTAL");
