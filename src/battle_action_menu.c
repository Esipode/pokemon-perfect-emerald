#include "global.h"
#include "battle.h"
#include "battle_action_menu.h"
#include "battle_message.h"
#include "bg.h"
#include "palette.h"
#include "string_util.h"
#include "text.h"
#include "window.h"
#include "constants/rgb.h"

// Icon sheet: 16 px wide, see tools/battle_ui/build_action_menu_gfx.py for the layout.
const u32 gActionMenuIconsGfx[] = INCGFX_U32("graphics/battle_interface/action_menu_icons.png", ".4bpp");

// Panel base colours, indices 0-7 (indices 8-15 are filled from the menu hues).
static const u16 sActionMenuBasePalette[] = INCGFX_U16("graphics/battle_interface/action_menu.pal", ".gbapal");

#include "data/battle_action_menu.h"

const struct ActionMenuSlot *ActionMenu_GetSlot(enum ActionMenuId menuId, u32 slot)
{
    return &sActionMenus[menuId][slot];
}

// Same 2x2 rules as the classic menu; a move onto a non-navigable slot is refused.
u32 ActionMenu_GetNextSlot(enum ActionMenuId menuId, u32 slot, enum ActionMenuDirection direction)
{
    u32 next;

    switch (direction)
    {
    case ACTION_DIR_UP:
        next = (slot & 2) ? slot ^ 2 : slot;
        break;
    case ACTION_DIR_DOWN:
        next = !(slot & 2) ? slot ^ 2 : slot;
        break;
    case ACTION_DIR_LEFT:
        next = (slot & 1) ? slot ^ 1 : slot;
        break;
    default:
        next = !(slot & 1) ? slot ^ 1 : slot;
        break;
    }

    return sActionMenus[menuId][next].navigable ? next : slot;
}

struct ActionMenuRect ActionMenu_GetChipPixelRect(u32 slot)
{
    return sActionChipRects[slot];
}

struct ActionMenuRect ActionMenu_GetChipTileRect(u32 slot)
{
    struct ActionMenuRect px = sActionChipRects[slot];
    struct ActionMenuRect tiles = { px.left / 8, px.top / 8, px.right / 8, px.bottom / 8 };

    return tiles;
}

// Indices 0-6 are shared by both palettes; 7-15 carry the outline and the four slot hues.
void ActionMenu_BuildPalette(enum ActionMenuId menuId, bool32 lit, u16 *dest)
{
    u32 i;

    for (i = 0; i < ACTION_PALETTE_BASE_COLORS; i++)
        dest[i] = sActionMenuBasePalette[i];
    if (lit)
        dest[7] = sActionMenuLitOutline;

    for (i = 0; i < ACTION_MENU_SLOT_COUNT; i++)
    {
        const struct ActionMenuSlot *slot = &sActionMenus[menuId][i];
        const u16 *hue = lit ? slot->litHue : slot->idleHue;

        dest[8 + i * 2] = hue[0];
        dest[9 + i * 2] = hue[1];
    }
}

// Prompt plate geometry in plate window pixels (see §1.2).
#define PLATE_WIDTH_PX      (ACTION_PROMPT_WIDTH_TILES * 8)
#define PLATE_HEIGHT_PX     (ACTION_PANEL_HEIGHT_TILES * 8)
#define PLATE_TEXT_X        6
#define PLATE_TEXT_WIDTH    84
#define PLATE_LINE1_Y       5
#define PLATE_LINE2_Y       16
#define PLATE_DIVIDER_X     92
#define PLATE_DIVIDER_SLOPE 8   // pixels the divider leans over the plate height
#define PLATE_STRIPE_HEIGHT 2

#define PIX_BACKGROUND      1
#define PIX_ACCENT          3
#define PIX_TEXT            4
#define PIX_SHADOW          5
#define PIX_MUTED           6
#define PIX_OUTLINE         7

#define CHIP_ICON_X         5
#define CHIP_ICON_Y         5
#define CHIP_LABEL_X        16
#define CHIP_LABEL_Y        1

static const u8 sText_ActionPromptLine1[] = _("What will");
static const u8 sText_ActionPromptLine2[] = _("{B_BUFF1} do?");

static enum ActionMenuId sMenuId;

static void PrintActionText(u32 windowId, u32 fontId, const u8 *text, u32 x, u32 y, u32 fgColor)
{
    struct TextPrinterTemplate printer;

    printer.currentChar = text;
    printer.type = WINDOW_TEXT_PRINTER;
    printer.windowId = windowId;
    printer.fontId = fontId;
    printer.x = x;
    printer.y = y;
    printer.currentX = x;
    printer.currentY = y;
    printer.letterSpacing = 0;
    printer.lineSpacing = 0;
    printer.color.background = TEXT_COLOR_TRANSPARENT;
    printer.color.foreground = fgColor;
    printer.color.accent = TEXT_COLOR_TRANSPARENT;
    printer.color.shadow = PIX_SHADOW;
    AddTextPrinter(&printer, 0, NULL);
}

static void DrawActionPlate(const u8 *line1, const u8 *line2)
{
    u32 y, x, font;

    FillWindowPixelBuffer(B_WIN_ACTION_PROMPT, PIXEL_FILL(PIX_BACKGROUND));
    FillWindowPixelRect(B_WIN_ACTION_PROMPT, PIXEL_FILL(PIX_ACCENT), 0, 0, PLATE_WIDTH_PX, PLATE_STRIPE_HEIGHT);

    for (y = PLATE_STRIPE_HEIGHT; y < PLATE_HEIGHT_PX; y++)
    {
        x = PLATE_DIVIDER_X - ((y - PLATE_STRIPE_HEIGHT) * PLATE_DIVIDER_SLOPE) / (PLATE_HEIGHT_PX - PLATE_STRIPE_HEIGHT);
        FillWindowPixelRect(B_WIN_ACTION_PROMPT, PIXEL_FILL(PIX_OUTLINE), x, y, 2, 1);
    }

    if (line1 != NULL)
        PrintActionText(B_WIN_ACTION_PROMPT, FONT_SMALL, line1, PLATE_TEXT_X, PLATE_LINE1_Y, PIX_MUTED);
    if (line2 != NULL)
    {
        font = GetStringWidth(FONT_NORMAL, line2, 0) > PLATE_TEXT_WIDTH ? FONT_NARROW : FONT_NORMAL;
        PrintActionText(B_WIN_ACTION_PROMPT, font, line2, PLATE_TEXT_X, PLATE_LINE2_Y, PIX_TEXT);
    }
}

// Rounded chip: outline, hue fill, darker bottom row. Corner pixels keep the panel background.
static void DrawActionChip(enum ActionMenuId menuId, u32 slot)
{
    const struct ActionMenuSlot *info = ActionMenu_GetSlot(menuId, slot);
    struct ActionMenuRect r = ActionMenu_GetChipPixelRect(slot);
    u32 w = r.right - r.left + 1;
    u32 h = r.bottom - r.top + 1;
    u32 fill = 8 + slot * 2;

    if (info->label == NULL)
        return;

    FillWindowPixelRect(B_WIN_ACTION_MENU, PIXEL_FILL(PIX_OUTLINE), r.left + 1, r.top, w - 2, h);
    FillWindowPixelRect(B_WIN_ACTION_MENU, PIXEL_FILL(PIX_OUTLINE), r.left, r.top + 1, w, h - 2);
    FillWindowPixelRect(B_WIN_ACTION_MENU, PIXEL_FILL(fill), r.left + 1, r.top + 1, w - 2, h - 2);
    FillWindowPixelRect(B_WIN_ACTION_MENU, PIXEL_FILL(fill + 1), r.left + 2, r.bottom - 1, w - 4, 1);

    u32 iconTile = info->icon - ACTION_ICON_BATTLE;

    BlitBitmapRectToWindowWithColorKey(B_WIN_ACTION_MENU, (const u8 *)gActionMenuIconsGfx,
                                       (iconTile & 1) * ACTION_ICON_SIZE, (iconTile / 2) * ACTION_ICON_SIZE,
                                       16, ACTION_SHEET_HEIGHT,
                                       r.left + CHIP_ICON_X, r.top + CHIP_ICON_Y,
                                       ACTION_ICON_SIZE, ACTION_ICON_SIZE, 0);
    PrintActionText(B_WIN_ACTION_MENU, FONT_NARROW, info->label, r.left + CHIP_LABEL_X, r.top + CHIP_LABEL_Y, PIX_TEXT);
}

static void SetChipPalette(u32 slot, u32 palette)
{
    struct ActionMenuRect t = ActionMenu_GetChipTileRect(slot);

    PutWindowRectTilemapOverridePalette(B_WIN_ACTION_MENU, t.left, t.top, t.right - t.left + 1, t.bottom - t.top + 1, palette);
    CopyBgTilemapBufferToVram(0);
}

void ActionMenu_SetHighlight(u32 slot)
{
    SetChipPalette(slot, ACTION_PALETTE_LIT);
}

void ActionMenu_ClearHighlight(u32 slot)
{
    SetChipPalette(slot, ACTION_PALETTE_IDLE);
}

void ActionMenu_SetPromptText(const u8 *line1, const u8 *line2)
{
    DrawActionPlate(line1, line2);
    CopyWindowToVram(B_WIN_ACTION_PROMPT, COPYWIN_GFX);
}

void ActionMenu_Show(enum BattlerId battler, enum ActionMenuId menuId)
{
    u16 palette[16];
    u32 slot;

    sMenuId = menuId;

    ActionMenu_BuildPalette(menuId, FALSE, palette);
    LoadPalette(palette, BG_PLTT_ID(ACTION_PALETTE_IDLE), PLTT_SIZE_4BPP);
    ActionMenu_BuildPalette(menuId, TRUE, palette);
    LoadPalette(palette, BG_PLTT_ID(ACTION_PALETTE_LIT), PLTT_SIZE_4BPP);

    PREPARE_MON_NICK_BUFFER(gBattleTextBuff1, battler, gBattlerPartyIndexes[battler]);
    BattleStringExpandPlaceholdersToDisplayedString(sText_ActionPromptLine2);
    DrawActionPlate(sText_ActionPromptLine1, gDisplayedStringBattle);

    FillWindowPixelBuffer(B_WIN_ACTION_MENU, PIXEL_FILL(PIX_BACKGROUND));
    FillWindowPixelRect(B_WIN_ACTION_MENU, PIXEL_FILL(PIX_ACCENT), 0, 0, ACTION_GRID_WIDTH_TILES * 8, PLATE_STRIPE_HEIGHT);
    for (slot = 0; slot < ACTION_MENU_SLOT_COUNT; slot++)
        DrawActionChip(menuId, slot);

    PutWindowTilemap(B_WIN_ACTION_PROMPT);
    PutWindowTilemap(B_WIN_ACTION_MENU);
    ActionMenu_SetHighlight(gActionSelectionCursor[battler]);
    CopyWindowToVram(B_WIN_ACTION_PROMPT, COPYWIN_FULL);
    CopyWindowToVram(B_WIN_ACTION_MENU, COPYWIN_FULL);
}
