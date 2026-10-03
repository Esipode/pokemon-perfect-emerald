#include "global.h"
#include "battle.h"
#include "battle_action_menu.h"
#include "battle_controllers.h"
#include "battle_main.h"
#include "battle_message.h"
#include "battle_util.h"
#include "bg.h"
#include "event_data.h"
#include "fpmath.h"
#include "main.h"
#include "palette.h"
#include "safari_zone.h"
#include "string_util.h"
#include "task.h"
#include "text.h"
#include "trig.h"
#include "window.h"
#include "constants/flags.h"
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

// Mirrors the engine's Bag block in B_ACTION_USE_ITEM handling (battle_main.c) without side effects.
bool32 ActionMenu_IsBagLocked(enum BattlerId battler, bool32 restrictionsApply)
{
    if (restrictionsApply && !IsAllowedToUseBag())
        return TRUE;
    if (gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_FRONTIER_NO_PYRAMID | BATTLE_TYPE_EREADER_TRAINER | BATTLE_TYPE_RECORDED_LINK))
        return TRUE;
    return gBattleMons[battler].volatiles.semiInvulnerable == STATE_SKY_DROP_TARGET;
}

// Trapping is not checked: the engine's full check writes gPotentialItemEffectBattler.
bool32 ActionMenu_IsRunLocked(void)
{
    if (FlagGet(WE_FLAG_NO_RUNNING))
        return TRUE;
    return (gBattleTypeFlags & BATTLE_TYPE_TRAINER) && !B_RUN_TRAINER_BATTLE;
}

enum ActionSlotState ActionMenu_GetSlotState(enum BattlerId battler, enum ActionMenuId menuId, u32 slot)
{
    const struct ActionMenuSlot *info = &sActionMenus[menuId][slot];

    if (info->label == NULL)
        return ACTION_SLOT_EMPTY;
    if (menuId != ACTION_MENU_STANDARD)
        return ACTION_SLOT_ENABLED;
    if (info->action == B_ACTION_USE_ITEM && ActionMenu_IsBagLocked(battler, ShouldBattleRestrictionsApply(battler)))
        return ACTION_SLOT_UNAVAILABLE;
    if (info->action == B_ACTION_RUN && ActionMenu_IsRunLocked())
        return ACTION_SLOT_UNAVAILABLE;
    return ACTION_SLOT_ENABLED;
}

// Same bounded 2x2 rules as the classic move menu: a step onto a slot past moveCount is refused.
u32 MoveMenu_GetNextSlot(u32 moveCount, u32 slot, enum MoveMenuDirection direction)
{
    u32 next = slot;

    switch (direction)
    {
    case MOVE_DIR_UP:
        if (slot & 2)
            next = slot ^ 2;
        break;
    case MOVE_DIR_DOWN:
        if (!(slot & 2))
            next = slot ^ 2;
        break;
    case MOVE_DIR_LEFT:
        if (slot & 1)
            next = slot ^ 1;
        break;
    default:
        if (!(slot & 1))
            next = slot ^ 1;
        break;
    }

    return next < moveCount ? next : slot;
}

enum MovePpTier MoveMenu_GetPpTier(u32 currentPp, u32 maxPp)
{
    if (maxPp == 0)
        return MOVE_PP_NORMAL;
    if (currentPp == 0)
        return MOVE_PP_EMPTY;
    if (currentPp * 4 <= maxPp)
        return MOVE_PP_RED;
    if (currentPp * 2 <= maxPp)
        return MOVE_PP_AMBER;
    return MOVE_PP_NORMAL;
}

// modifier is the type multiplier in UQ_4_12; status moves never show a badge.
enum MoveEffBadge MoveMenu_GetEffectivenessBadge(u32 modifier, bool32 isStatus)
{
    if (isStatus)
        return MOVE_EFF_NONE;
    if (modifier == UQ_4_12(0.0))
        return MOVE_EFF_IMMUNE;
    if (modifier <= UQ_4_12(0.25))
        return MOVE_EFF_MOSTLY_RESISTED;
    if (modifier <= UQ_4_12(0.5))
        return MOVE_EFF_RESISTED;
    if (modifier >= UQ_4_12(4.0))
        return MOVE_EFF_EXTREME;
    if (modifier >= UQ_4_12(2.0))
        return MOVE_EFF_SUPER;
    return MOVE_EFF_NONE;
}

struct ActionMenuRect MoveMenu_GetCellPixelRect(u32 slot)
{
    return sMoveCellRects[slot];
}

struct ActionMenuRect MoveMenu_GetCellTileRect(u32 slot)
{
    struct ActionMenuRect px = sMoveCellRects[slot];
    struct ActionMenuRect tiles = { px.left / 8, px.top / 8, px.right / 8, px.bottom / 8 };

    return tiles;
}

static u16 ScaleMoveColor(const u8 *rgb, u32 numerator, u32 denominator)
{
    return RGB(rgb[0] * numerator / denominator, rgb[1] * numerator / denominator, rgb[2] * numerator / denominator);
}

// Indices 0-7 match the action palettes; each slot's pair is that move's type colour (idle is 3/4 strength).
// Types[] entries of TYPE_NONE (unused slots) take the neutral grey.
void MoveMenu_BuildPalette(const enum Type types[MOVE_MENU_SLOT_COUNT], bool32 lit, u16 *dest)
{
    u32 i;

    for (i = 0; i < ACTION_PALETTE_BASE_COLORS; i++)
        dest[i] = sActionMenuBasePalette[i];
    if (lit)
        dest[7] = sActionMenuLitOutline;

    for (i = 0; i < MOVE_MENU_SLOT_COUNT; i++)
    {
        const u8 *rgb = sMoveTypeColors[types[i] < NUMBER_OF_MON_TYPES ? types[i] : TYPE_NONE];
        u32 strength = lit ? 4 : 3;

        dest[8 + i * 2] = ScaleMoveColor(rgb, strength, 4);
        dest[9 + i * 2] = ScaleMoveColor(rgb, strength * 5, 4 * 9);
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
#define PIX_DISABLED        2
#define PIX_ACCENT          3
#define PIX_TEXT            4
#define PIX_SHADOW          5
#define PIX_MUTED           6
#define PIX_OUTLINE         7

#define CHIP_ICON_X         5
#define CHIP_ICON_Y         5
#define CHIP_LABEL_X        16
#define CHIP_LABEL_Y        1
#define CHIP_LOCK_INSET     12

static const u8 sText_ActionPromptLine1[] = _("What will");
static const u8 sText_ActionPromptLine2[] = _("{B_BUFF1} do?");

#define ENTRANCE_FRAMES     6
#define ENTRANCE_DISTANCE   48  // px the panel falls into place
#define PULSE_PERIOD        48  // frames per sine cycle
#define PULSE_MAX_BLEND     80  // out of 256: 5/16 toward white

#define tFrame data[0]

static enum ActionMenuId sMenuId;
static const u8 *sPromptLine2;
static u8 sEntranceStep;

static void StartPulse(void);

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

static void BlitSheetTile(u32 windowId, u32 tile, u32 x, u32 y)
{
    BlitBitmapRectToWindowWithColorKey(windowId, (const u8 *)gActionMenuIconsGfx,
                                       (tile & 1) * ACTION_ICON_SIZE, (tile / 2) * ACTION_ICON_SIZE,
                                       16, ACTION_SHEET_HEIGHT, x, y, ACTION_ICON_SIZE, ACTION_ICON_SIZE, 0);
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

// Dotted index-2 outline marking a slot with no action.
static void DrawEmptyCell(u32 slot)
{
    struct ActionMenuRect r = ActionMenu_GetChipPixelRect(slot);
    u32 x, y;

    for (x = r.left + 2; x <= r.right - 2; x += 2)
    {
        FillWindowPixelRect(B_WIN_ACTION_MENU, PIXEL_FILL(PIX_DISABLED), x, r.top, 1, 1);
        FillWindowPixelRect(B_WIN_ACTION_MENU, PIXEL_FILL(PIX_DISABLED), x, r.bottom, 1, 1);
    }
    for (y = r.top + 2; y <= r.bottom - 2; y += 2)
    {
        FillWindowPixelRect(B_WIN_ACTION_MENU, PIXEL_FILL(PIX_DISABLED), r.left, y, 1, 1);
        FillWindowPixelRect(B_WIN_ACTION_MENU, PIXEL_FILL(PIX_DISABLED), r.right, y, 1, 1);
    }
}

// Rounded chip: outline, fill, darker bottom row. Corner pixels keep the panel background.
static void DrawActionChip(enum BattlerId battler, enum ActionMenuId menuId, u32 slot)
{
    const struct ActionMenuSlot *info = ActionMenu_GetSlot(menuId, slot);
    enum ActionSlotState state = ActionMenu_GetSlotState(battler, menuId, slot);
    struct ActionMenuRect r = ActionMenu_GetChipPixelRect(slot);
    u32 w = r.right - r.left + 1;
    u32 h = r.bottom - r.top + 1;
    u32 fill = 8 + slot * 2;
    u32 iconTile = info->icon - ACTION_ICON_BATTLE;
    u32 labelColor = PIX_TEXT;
    const u8 *label = info->label;
    u8 ballLabel[16];

    if (state == ACTION_SLOT_EMPTY)
    {
        DrawEmptyCell(slot);
        return;
    }

    if (info->action == B_ACTION_SAFARI_BALL)
    {
        u8 *end = StringCopy(ballLabel, sText_ActionBallPrefix);

        ConvertIntToDecimalStringN(end, gNumSafariBalls, STR_CONV_MODE_LEFT_ALIGN, 3);
        label = ballLabel;
    }

    FillWindowPixelRect(B_WIN_ACTION_MENU, PIXEL_FILL(PIX_OUTLINE), r.left + 1, r.top, w - 2, h);
    FillWindowPixelRect(B_WIN_ACTION_MENU, PIXEL_FILL(PIX_OUTLINE), r.left, r.top + 1, w, h - 2);
    if (state == ACTION_SLOT_UNAVAILABLE)
    {
        FillWindowPixelRect(B_WIN_ACTION_MENU, PIXEL_FILL(PIX_DISABLED), r.left + 1, r.top + 1, w - 2, h - 2);
        labelColor = PIX_MUTED;
    }
    else
    {
        FillWindowPixelRect(B_WIN_ACTION_MENU, PIXEL_FILL(fill), r.left + 1, r.top + 1, w - 2, h - 2);
        FillWindowPixelRect(B_WIN_ACTION_MENU, PIXEL_FILL(fill + 1), r.left + 2, r.bottom - 1, w - 4, 1);
    }

    BlitSheetTile(B_WIN_ACTION_MENU, iconTile, r.left + CHIP_ICON_X, r.top + CHIP_ICON_Y);
    PrintActionText(B_WIN_ACTION_MENU, FONT_NARROW, label, r.left + CHIP_LABEL_X, r.top + CHIP_LABEL_Y, labelColor);
    if (state == ACTION_SLOT_UNAVAILABLE)
        BlitSheetTile(B_WIN_ACTION_MENU, ACTION_ICON_LOCK - ACTION_ICON_BATTLE, r.right - CHIP_LOCK_INSET, r.top + CHIP_ICON_Y);
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

static void DrawActionPanel(enum BattlerId battler, enum ActionMenuId menuId)
{
    u16 palette[16];
    u32 slot;

    ActionMenu_BuildPalette(menuId, FALSE, palette);
    LoadPalette(palette, BG_PLTT_ID(ACTION_PALETTE_IDLE), PLTT_SIZE_4BPP);
    ActionMenu_BuildPalette(menuId, TRUE, palette);
    LoadPalette(palette, BG_PLTT_ID(ACTION_PALETTE_LIT), PLTT_SIZE_4BPP);

    PREPARE_MON_NICK_BUFFER(gBattleTextBuff1, battler, gBattlerPartyIndexes[battler]);
    BattleStringExpandPlaceholdersToDisplayedString(sPromptLine2);
    DrawActionPlate(sText_ActionPromptLine1, gDisplayedStringBattle);

    FillWindowPixelBuffer(B_WIN_ACTION_MENU, PIXEL_FILL(PIX_BACKGROUND));
    FillWindowPixelRect(B_WIN_ACTION_MENU, PIXEL_FILL(PIX_ACCENT), 0, 0, ACTION_GRID_WIDTH_TILES * 8, PLATE_STRIPE_HEIGHT);
    for (slot = 0; slot < ACTION_MENU_SLOT_COUNT; slot++)
        DrawActionChip(battler, menuId, slot);

    PutWindowTilemap(B_WIN_ACTION_PROMPT);
    PutWindowTilemap(B_WIN_ACTION_MENU);
    ActionMenu_SetHighlight(gActionSelectionCursor[battler]);
    CopyWindowToVram(B_WIN_ACTION_PROMPT, COPYWIN_FULL);
    CopyWindowToVram(B_WIN_ACTION_MENU, COPYWIN_FULL);
}

// line2Template: prompt second line with placeholders; NULL uses the battler nickname prompt.
void ActionMenu_Show(enum BattlerId battler, enum ActionMenuId menuId, const u8 *line2Template)
{
    sMenuId = menuId;
    sPromptLine2 = line2Template != NULL ? line2Template : sText_ActionPromptLine2;
    // Already on screen (partner cancel): no entrance.
    sEntranceStep = gBattle_BG0_Y == DISPLAY_HEIGHT ? ENTRANCE_FRAMES : 0;
    DrawActionPanel(battler, menuId);
}

// Redraws the last shown panel; the Bag/party screen overwrites its VRAM and palettes.
void ActionMenu_Redraw(enum BattlerId battler)
{
    if (sPromptLine2 == NULL)
        sPromptLine2 = sText_ActionPromptLine2;
    DrawActionPanel(battler, sMenuId);
    StartPulse();
}

// Blends the lit slot hues toward white. Writes only the faded buffer, so any palette fade or reload overrides it.
static void Task_ActionMenuPulse(u8 taskId)
{
    u16 lit[16];
    u32 i, blend;

    if (gBattle_BG0_Y != DISPLAY_HEIGHT)
    {
        ActionMenu_Hide();
        return;
    }
    if (gPaletteFade.active || gMain.callback2 != BattleMainCB2)
        return;

    blend = Sin(gTasks[taskId].tFrame * 256 / PULSE_PERIOD, PULSE_MAX_BLEND / 2) + PULSE_MAX_BLEND / 2;
    gTasks[taskId].tFrame = (gTasks[taskId].tFrame + 1) % PULSE_PERIOD;

    ActionMenu_BuildPalette(sMenuId, TRUE, lit);
    for (i = ACTION_PALETTE_BASE_COLORS; i < 16; i++)
    {
        u32 r = GET_R(lit[i]);
        u32 g = GET_G(lit[i]);
        u32 b = GET_B(lit[i]);

        r += (31 - r) * blend / 256;
        g += (31 - g) * blend / 256;
        b += (31 - b) * blend / 256;
        gPlttBufferFaded[BG_PLTT_ID(ACTION_PALETTE_LIT) + i] = RGB(r, g, b);
    }
}

static void StartPulse(void)
{
    if (!FuncIsActiveTask(Task_ActionMenuPulse))
        CreateTask(Task_ActionMenuPulse, 80);
}

// Stops the pulse and restores the lit palette.
void ActionMenu_Hide(void)
{
    u8 taskId = FindTaskIdByFunc(Task_ActionMenuPulse);

    if (taskId == TASK_NONE)
        return;
    DestroyTask(taskId);
    if (!gPaletteFade.active)
    {
        CpuCopy16(&gPlttBufferUnfaded[BG_PLTT_ID(ACTION_PALETTE_LIT)], &gPlttBufferFaded[BG_PLTT_ID(ACTION_PALETTE_LIT)], PLTT_SIZE_4BPP);
    }
}

// Scrolls the panel into view, called each frame once its graphics are in VRAM. TRUE when input may start.
bool32 ActionMenu_RunEntrance(void)
{
    u32 remaining;

    if (sEntranceStep >= ENTRANCE_FRAMES)
    {
        gBattle_BG0_Y = DISPLAY_HEIGHT;
        StartPulse();
        return TRUE;
    }

    remaining = ENTRANCE_FRAMES - sEntranceStep++;
    gBattle_BG0_Y = DISPLAY_HEIGHT + ENTRANCE_DISTANCE * remaining * remaining / (ENTRANCE_FRAMES * ENTRANCE_FRAMES);
    return FALSE;
}

// Move view geometry. The plate shares the action plate's stripe and divider.
#define MOVE_TEXT_Y          3
#define MOVE_PIP_Y           4
#define MOVE_PIP_WIDTH       4
#define MOVE_PIP_HEIGHT      8
#define MOVE_UNDERLINE_Y     18
#define MOVE_POINTER_HEIGHT  7
#define MOVE_POINTER_Y       7

#define MOVE_PLATE_NAME_Y    4
#define MOVE_PLATE_TYPE_Y    17
#define MOVE_PLATE_PP_Y      27
#define MOVE_PLATE_HINT_Y    37
#define MOVE_PLATE_TYPE_X    13
#define MOVE_PLATE_BADGE_X   66

static const u8 sText_MovePpAmber[] = _("!");
static const u8 sText_MovePpRed[] = _("!!");
static const u8 sText_MoveHint[] = _("A: Use  B: Back");

static const u8 sText_MoveBadgeSuper[] = _("{UP_ARROW}");
static const u8 sText_MoveBadgeExtreme[] = _("{STAR}");
static const u8 sText_MoveBadgeResisted[] = _("{DOWN_ARROW}");
static const u8 sText_MoveBadgeMostlyResisted[] = _("{TRIANGLE_UPSIDE_DOWN}");
static const u8 sText_MoveBadgeImmune[] = _("{BIG_MULT_X}");

static enum Type sMoveTypes[MOVE_MENU_SLOT_COUNT];

static void DrawMovePointer(u32 slot, u32 color)
{
    struct ActionMenuRect r = MoveMenu_GetCellPixelRect(slot);
    static const u8 widths[MOVE_POINTER_HEIGHT] = { 1, 2, 3, 4, 3, 2, 1 };
    u32 i;

    for (i = 0; i < MOVE_POINTER_HEIGHT; i++)
        FillWindowPixelRect(B_WIN_MOVE_NAME_2, PIXEL_FILL(color), r.left + MOVE_POINTER_X, r.top + MOVE_POINTER_Y + i, widths[i], 1);
}

static void DrawMoveCell(const struct MoveMenuView *view, u32 slot)
{
    struct ActionMenuRect r = MoveMenu_GetCellPixelRect(slot);
    const u8 *name = view->names[slot];
    u32 hue = 8 + slot * 2;

    if (name == NULL || name[0] == EOS)
        return;

    FillWindowPixelRect(B_WIN_MOVE_NAME_2, PIXEL_FILL(hue), r.left + MOVE_PIP_X, r.top + MOVE_PIP_Y, MOVE_PIP_WIDTH, MOVE_PIP_HEIGHT);
    PrintActionText(B_WIN_MOVE_NAME_2, GetFontIdToFit(name, FONT_NARROWER, 0, MOVE_NAME_WIDTH), name,
                    r.left + MOVE_NAME_X, r.top + MOVE_TEXT_Y, PIX_OUTLINE);
    FillWindowPixelRect(B_WIN_MOVE_NAME_2, PIXEL_FILL(hue + 1), r.left + MOVE_NAME_X, r.top + MOVE_UNDERLINE_Y, MOVE_NAME_WIDTH - 2, 2);
}

static const u8 *GetMoveBadgeText(enum MoveEffBadge badge)
{
    switch (badge)
    {
    case MOVE_EFF_SUPER:
        return sText_MoveBadgeSuper;
    case MOVE_EFF_EXTREME:
        return sText_MoveBadgeExtreme;
    case MOVE_EFF_RESISTED:
        return sText_MoveBadgeResisted;
    case MOVE_EFF_MOSTLY_RESISTED:
        return sText_MoveBadgeMostlyResisted;
    case MOVE_EFF_IMMUNE:
        return sText_MoveBadgeImmune;
    default:
        return NULL;
    }
}

static void DrawMovePlate(const struct MoveMenuView *view)
{
    u32 y, x;
    const u8 *name = view->names[view->cursor];
    const u8 *badge = GetMoveBadgeText(view->effBadge);

    FillWindowPixelBuffer(B_WIN_MOVE_NAME_1, PIXEL_FILL(PIX_BACKGROUND));
    FillWindowPixelRect(B_WIN_MOVE_NAME_1, PIXEL_FILL(PIX_ACCENT), 0, 0, PLATE_WIDTH_PX, PLATE_STRIPE_HEIGHT);

    for (y = PLATE_STRIPE_HEIGHT; y < PLATE_HEIGHT_PX; y++)
    {
        x = PLATE_DIVIDER_X - ((y - PLATE_STRIPE_HEIGHT) * PLATE_DIVIDER_SLOPE) / (PLATE_HEIGHT_PX - PLATE_STRIPE_HEIGHT);
        FillWindowPixelRect(B_WIN_MOVE_NAME_1, PIXEL_FILL(PIX_OUTLINE), x, y, 2, 1);
    }

    if (name != NULL && name[0] != EOS)
    {
        PrintActionText(B_WIN_MOVE_NAME_1, GetFontIdToFit(name, FONT_NORMAL, 0, PLATE_TEXT_WIDTH), name,
                        PLATE_TEXT_X, MOVE_PLATE_NAME_Y, PIX_TEXT);
        FillWindowPixelRect(B_WIN_MOVE_NAME_1, PIXEL_FILL(8 + view->cursor * 2), PLATE_TEXT_X, MOVE_PLATE_TYPE_Y + 1, MOVE_PIP_WIDTH, MOVE_PIP_HEIGHT);
        PrintActionText(B_WIN_MOVE_NAME_1, FONT_SMALL, gTypesInfo[view->types[view->cursor]].name,
                        MOVE_PLATE_TYPE_X, MOVE_PLATE_TYPE_Y, PIX_MUTED);
    }

    if (view->showPp)
    {
        u8 text[24];
        u8 *end;
        enum MovePpTier tier = MoveMenu_GetPpTier(view->currentPp, view->maxPp);

        end = StringCopy(text, gText_MoveInterfacePP);
        end = ConvertIntToDecimalStringN(end, view->currentPp, STR_CONV_MODE_LEFT_ALIGN, 3);
        *end++ = CHAR_SLASH;
        end = ConvertIntToDecimalStringN(end, view->maxPp, STR_CONV_MODE_LEFT_ALIGN, 3);
        if (tier == MOVE_PP_AMBER)
            StringCopy(end, sText_MovePpAmber);
        else if (tier == MOVE_PP_RED)
            StringCopy(end, sText_MovePpRed);
        PrintActionText(B_WIN_MOVE_NAME_1, FONT_SMALL, text, PLATE_TEXT_X, MOVE_PLATE_PP_Y,
                        tier == MOVE_PP_EMPTY ? PIX_MUTED : PIX_TEXT);
    }

    if (badge != NULL)
        PrintActionText(B_WIN_MOVE_NAME_1, FONT_SMALL, badge, MOVE_PLATE_BADGE_X, MOVE_PLATE_PP_Y, PIX_TEXT);

    PrintActionText(B_WIN_MOVE_NAME_1, FONT_SMALL, sText_MoveHint, PLATE_TEXT_X, MOVE_PLATE_HINT_Y, PIX_MUTED);
}

void MoveMenu_SetDetails(const struct MoveMenuView *view)
{
    DrawMovePlate(view);
    CopyWindowToVram(B_WIN_MOVE_NAME_1, COPYWIN_GFX);
}

static void SetMoveCellPalette(u32 slot, u32 palette)
{
    struct ActionMenuRect t = MoveMenu_GetCellTileRect(slot);

    PutWindowRectTilemapOverridePalette(B_WIN_MOVE_NAME_2, t.left, t.top, t.right - t.left + 1, t.bottom - t.top + 1, palette);
    CopyBgTilemapBufferToVram(0);
}

// The pointer is pixels in the cell and the lit palette brightens the name, so a move redraws the grid.
void MoveMenu_SetHighlight(u32 slot)
{
    if (slot >= MOVE_MENU_SLOT_COUNT)
        return;
    DrawMovePointer(slot, PIX_OUTLINE);
    CopyWindowToVram(B_WIN_MOVE_NAME_2, COPYWIN_GFX);
    SetMoveCellPalette(slot, ACTION_PALETTE_LIT);
}

void MoveMenu_ClearHighlight(u32 slot)
{
    if (slot >= MOVE_MENU_SLOT_COUNT)
        return;
    DrawMovePointer(slot, PIX_BACKGROUND);
    CopyWindowToVram(B_WIN_MOVE_NAME_2, COPYWIN_GFX);
    SetMoveCellPalette(slot, ACTION_PALETTE_IDLE);
}

void MoveMenu_Show(const struct MoveMenuView *view)
{
    u16 palette[16];
    u32 slot;

    for (slot = 0; slot < MOVE_MENU_SLOT_COUNT; slot++)
        sMoveTypes[slot] = view->types[slot];
    MoveMenu_BuildPalette(sMoveTypes, FALSE, palette);
    LoadPalette(palette, BG_PLTT_ID(ACTION_PALETTE_IDLE), PLTT_SIZE_4BPP);
    MoveMenu_BuildPalette(sMoveTypes, TRUE, palette);
    LoadPalette(palette, BG_PLTT_ID(ACTION_PALETTE_LIT), PLTT_SIZE_4BPP);

    DrawMovePlate(view);

    FillWindowPixelBuffer(B_WIN_MOVE_NAME_2, PIXEL_FILL(PIX_BACKGROUND));
    FillWindowPixelRect(B_WIN_MOVE_NAME_2, PIXEL_FILL(PIX_ACCENT), 0, 0, MOVE_GRID_WIDTH_TILES * 8, PLATE_STRIPE_HEIGHT);
    for (slot = 0; slot < MOVE_MENU_SLOT_COUNT; slot++)
        DrawMoveCell(view, slot);

    PutWindowTilemap(B_WIN_MOVE_NAME_1);
    PutWindowTilemap(B_WIN_MOVE_NAME_2);
    MoveMenu_SetHighlight(view->cursor);
    CopyWindowToVram(B_WIN_MOVE_NAME_1, COPYWIN_FULL);
    CopyWindowToVram(B_WIN_MOVE_NAME_2, COPYWIN_FULL);
}
