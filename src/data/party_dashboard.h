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

enum
{
    WIN_DASH_IDENT = PARTY_SIZE + 1,
    WIN_DASH_TABS,
    WIN_DASH_BODY,
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
        (col) * 32 + 16, (row) * 48 + 14,                                     \
        (col) * 32 + 28, (row) * 48 + 4,                                      \
        (col) * 32 + 16, (row) * 48 + 38,                                     \
        (col) * 32 + 16, (row) * 48 + 16,                                     \
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
static const u8 sDashTabColors[3] = {TEXT_COLOR_TRANSPARENT, 2, 3}; // Palette 12: text, shadow.
