#ifndef GUARD_BATTLE_ACTION_MENU_H
#define GUARD_BATTLE_ACTION_MENU_H

#include "global.h"
#include "constants/battle.h"
#include "constants/pokemon.h"

// TRUE draws the styled action panel; FALSE keeps the classic prompt + menu windows.
#define ACTION_MENU_NEW TRUE

enum ActionMenuId
{
    ACTION_MENU_STANDARD,
    ACTION_MENU_SAFARI,
    ACTION_MENU_TUTORIAL,   // Standard slots for the scripted tutorial
    ACTION_MENU_COUNT,
};

enum ActionMenuDirection
{
    ACTION_DIR_UP,
    ACTION_DIR_DOWN,
    ACTION_DIR_LEFT,
    ACTION_DIR_RIGHT,
};

enum ActionSlotState
{
    ACTION_SLOT_ENABLED,
    ACTION_SLOT_UNAVAILABLE,    // Greyed with a lock badge; still selectable so the engine can explain
    ACTION_SLOT_EMPTY,
};

enum ActionMenuIcon
{
    ACTION_ICON_NONE,
    ACTION_ICON_BATTLE,
    ACTION_ICON_BAG,
    ACTION_ICON_POKEMON,
    ACTION_ICON_RUN,
    ACTION_ICON_SAFARI_BALL,
    ACTION_ICON_GO_NEAR,
    ACTION_ICON_LOCK,
};

#define ACTION_MENU_SLOT_COUNT 4

// Both windows sit on BG0 rows 34-39 (the action view, BG0_Y = DISPLAY_HEIGHT).
#define ACTION_PANEL_TOP_TILE     34
#define ACTION_PANEL_HEIGHT_TILES 6

#define ACTION_PROMPT_LEFT_TILE   0
#define ACTION_PROMPT_WIDTH_TILES 12
#define ACTION_PROMPT_BASE_BLOCK  0x10C
#define ACTION_PROMPT_TILE_COUNT  (ACTION_PROMPT_WIDTH_TILES * ACTION_PANEL_HEIGHT_TILES)

#define ACTION_GRID_LEFT_TILE     (ACTION_PROMPT_LEFT_TILE + ACTION_PROMPT_WIDTH_TILES)
#define ACTION_GRID_WIDTH_TILES   18
#define ACTION_GRID_BASE_BLOCK    (ACTION_PROMPT_BASE_BLOCK + ACTION_PROMPT_TILE_COUNT)
#define ACTION_GRID_TILE_COUNT    (ACTION_GRID_WIDTH_TILES * ACTION_PANEL_HEIGHT_TILES)

// Last BG0 tile (exclusive) the panel may use; 0x1C0-0x1FF stays free, 0x200+ aliases BG1/BG2.
#define ACTION_PANEL_TILE_LIMIT   0x1C0

#define ACTION_PALETTE_IDLE       12
#define ACTION_PALETTE_LIT        13

// Palette indices 0-6 are shared by idle and lit; 7 is the outline; 8-15 are the slot hues.
#define ACTION_PALETTE_BASE_COLORS 8

// Icon sheet (action_menu_icons.png): 16 px wide, 8x8 tiles two per row, in ActionMenuIcon order starting at
// ACTION_ICON_BATTLE, followed by the keypad glyphs.
#define ACTION_ICON_SIZE          8
#define ACTION_ICON_COUNT         7
#define ACTION_GLYPH_FIRST_TILE   ACTION_ICON_COUNT
#define ACTION_SHEET_HEIGHT       40
enum ActionMenuGlyph
{
    ACTION_GLYPH_R,
    ACTION_GLYPH_B,
    ACTION_GLYPH_START,
};

#define ACTION_CHIP_WIDTH         64
#define ACTION_CHIP_HEIGHT        19
#define ACTION_LABEL_WIDTH        40

struct ActionMenuSlot
{
    const u8 *label;        // NULL on an empty cell
    u8 icon;                // ACTION_ICON_*
    u8 action;              // B_ACTION_*
    bool8 navigable;
    u16 idleHue[2];         // base, dark
    u16 litHue[2];
};

struct ActionMenuRect
{
    u8 left;
    u8 top;
    u8 right;               // inclusive
    u8 bottom;              // inclusive
};

// Move view (BG0 rows 54-59 at BG0_Y = 320): a plate window with the selected move's details and a grid
// window with the four move cells. Tiles stay above the action panel's 0x10C-0x1BF and below the Move Info
// window at 0x350.
#define MOVE_PANEL_TOP_TILE       54
#define MOVE_PANEL_HEIGHT_TILES   ACTION_PANEL_HEIGHT_TILES

#define MOVE_PLATE_LEFT_TILE      0
#define MOVE_PLATE_WIDTH_TILES    ACTION_PROMPT_WIDTH_TILES
#define MOVE_PLATE_BASE_BLOCK     0x290
#define MOVE_PLATE_TILE_COUNT     (MOVE_PLATE_WIDTH_TILES * MOVE_PANEL_HEIGHT_TILES)

#define MOVE_GRID_LEFT_TILE       (MOVE_PLATE_LEFT_TILE + MOVE_PLATE_WIDTH_TILES)
#define MOVE_GRID_WIDTH_TILES     ACTION_GRID_WIDTH_TILES
#define MOVE_GRID_BASE_BLOCK      (MOVE_PLATE_BASE_BLOCK + MOVE_PLATE_TILE_COUNT)
#define MOVE_GRID_TILE_COUNT      (MOVE_GRID_WIDTH_TILES * MOVE_PANEL_HEIGHT_TILES)

// Unused move windows shrink to 1x1 stubs on tilemap row 62 (off screen at the move view).
#define MOVE_STUB_BASE_BLOCK      (MOVE_GRID_BASE_BLOCK + MOVE_GRID_TILE_COUNT)
#define MOVE_STUB_COUNT           6
#define MOVE_STUB_TOP_TILE        62

// First tile of the Move Info window (B_WIN_MOVE_DESCRIPTION); the move view must stay below it.
#define MOVE_PANEL_TILE_LIMIT     0x350

#define MOVE_MENU_SLOT_COUNT      4
#define MOVE_CELL_WIDTH           72
#define MOVE_CELL_HEIGHT          22
// Cell contents, left to right: pointer (x 0-2), type icon (x 3-10, its outer pixels are dark), name (x 11 to the cell edge).
#define MOVE_POINTER_X            0
#define MOVE_ICON_X               3
#define MOVE_NAME_X               11

// Move palette indices 9/11/13 are status colours; hues sit at 8/10/12/14.
#define MOVE_COLOR_AMBER          9
#define MOVE_COLOR_RED            11
#define MOVE_COLOR_GREEN          13
#define MOVE_NAME_WIDTH           (MOVE_CELL_WIDTH - MOVE_NAME_X)

enum MovePpTier
{
    MOVE_PP_NORMAL,
    MOVE_PP_AMBER,      // <= 50 %
    MOVE_PP_RED,        // <= 25 %
    MOVE_PP_EMPTY,
};

enum MoveEffBadge
{
    MOVE_EFF_NONE,      // Normal damage, status move or unknown matchup
    MOVE_EFF_SUPER,
    MOVE_EFF_EXTREME,
    MOVE_EFF_RESISTED,
    MOVE_EFF_MOSTLY_RESISTED,
    MOVE_EFF_IMMUNE,
};

enum MoveMenuDirection
{
    MOVE_DIR_UP,
    MOVE_DIR_DOWN,
    MOVE_DIR_LEFT,
    MOVE_DIR_RIGHT,
};

// Everything the move view draws; the controller fills it so names and types already reflect Z-Move and Dynamax.
struct MoveMenuView
{
    const u8 *names[MOVE_MENU_SLOT_COUNT];  // NULL or empty on an unused slot
    u8 types[MOVE_MENU_SLOT_COUNT];         // enum Type of each move as displayed
    u8 cursor;
    u8 currentPp;
    u8 maxPp;
    bool8 showPp;
    u8 effBadge;                            // enum MoveEffBadge
    bool8 switching;                        // move rearrange mode: plate shows a prompt
};

extern const u32 gActionMenuIconsGfx[];

const struct ActionMenuSlot *ActionMenu_GetSlot(enum ActionMenuId menuId, u32 slot);
u32 ActionMenu_GetNextSlot(enum ActionMenuId menuId, u32 slot, enum ActionMenuDirection direction);
struct ActionMenuRect ActionMenu_GetChipPixelRect(u32 slot);
struct ActionMenuRect ActionMenu_GetChipTileRect(u32 slot);
void ActionMenu_BuildPalette(enum ActionMenuId menuId, bool32 lit, u16 *dest);
bool32 ActionMenu_IsBagLocked(enum BattlerId battler, bool32 restrictionsApply);
bool32 ActionMenu_IsRunLocked(void);
enum ActionSlotState ActionMenu_GetSlotState(enum BattlerId battler, enum ActionMenuId menuId, u32 slot);

u32 MoveMenu_GetNextSlot(u32 moveCount, u32 slot, enum MoveMenuDirection direction);
enum MovePpTier MoveMenu_GetPpTier(u32 currentPp, u32 maxPp);
enum MoveEffBadge MoveMenu_GetEffectivenessBadge(u32 modifier, bool32 isStatus);
struct ActionMenuRect MoveMenu_GetCellPixelRect(u32 slot);
struct ActionMenuRect MoveMenu_GetCellTileRect(u32 slot);
void MoveMenu_BuildPalette(const enum Type types[MOVE_MENU_SLOT_COUNT], bool32 lit, u16 *dest);

void ActionMenu_Show(enum BattlerId battler, enum ActionMenuId menuId, const u8 *line2Template);
void ActionMenu_Redraw(enum BattlerId battler);
void ActionMenu_SetPromptText(const u8 *line1, const u8 *line2);
void ActionMenu_SetHighlight(u32 slot);
void ActionMenu_ClearHighlight(u32 slot);
void ActionMenu_Hide(void);
bool32 ActionMenu_RunEntrance(void);

void MoveMenu_Show(const struct MoveMenuView *view);
void MoveMenu_SetDetails(const struct MoveMenuView *view);
void MoveMenu_SetHighlight(u32 slot);
void MoveMenu_SetMarker(u32 slot);
void MoveMenu_FlushTilemap(void);
void MoveMenu_ClearHighlight(u32 slot);

#endif // GUARD_BATTLE_ACTION_MENU_H
