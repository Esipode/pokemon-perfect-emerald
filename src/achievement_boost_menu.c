#include "global.h"
#include "achievements.h"
#include "achievement_boost_menu.h"
#include "achievement_icons.h"
#include "bg.h"
#include "gpu_regs.h"
#include "international_string_util.h"
#include "line_break.h"
#include "list_menu.h"
#include "main.h"
#include "menu.h"
#include "money.h"               // IsEnoughMoney, for the Stage 11 reset row's status text
#include "palette.h"
#include "scanline_effect.h"
#include "sound.h"
#include "sprite.h"
#include "string_util.h"
#include "task.h"
#include "text.h"
#include "text_window.h"
#include "window.h"
#include "constants/rgb.h"
#include "constants/songs.h"

// Stage 7 (design doc Stage 7 / plan §7): skeleton copied wholesale from
// src/achievements_menu.c (same BG/window templates, staged CB2 init,
// ListMenu + scroll arrows, DrawBgWindowFrames chrome) -- same precedent
// achievements_menu.c itself followed from src/new_game_settings_menu.c.
// Unlike achievements_menu.c there's no tier grouping here: BOOSTS_COUNT is
// small and boosts aren't tiered, so this is a single flat list rather than
// a tier -> list -> detail flow. [A] on a row attempts to purchase that
// boost's next level directly; there's no separate confirmation screen. Once
// a boost is owned, [A] on a binary one instead flips it on/off, and L/R on
// a leveled one dials its *active* level up/down without spending or
// refunding anything (see AchievementBoost_GetActiveLevel/
// _TryChangeActiveLevel in src/achievements.c and
// TryPurchaseOrToggleBoost/TryChangeHighlightedBoostActiveLevel below).
//
// Reached from src/achievements_menu.c's TIER SELECT level via a "BOOSTS"
// row appended when Achievement_BoostsUnlocked() && Achievement_BoostsEnabled()
// (design doc Stage 6: OFF hides the shop, not just the toggle). Also
// reachable unconditionally from the debug menu for testing before Stage 5's
// unlock gate has actually been cleared on a given save.
//
// Stage 11 (design doc §13/Stage 11): a synthetic "RESET BOOSTS" row is
// appended after the real boosts (BOOST_MENU_ITEM_RESET, same trick as
// achievements_menu.c's own BOOSTS row). Unlike a purchase, [A] on it does
// show a confirmation screen -- it swaps WIN_LIST's ListMenu for a throwaway
// Yes/No list (see EnterResetConfirmLevel) rather than committing directly,
// since AchievementBoost_Reset() is destructive to every purchased level at
// once and costs real Poké money.

enum
{
    WIN_HEADER,
    WIN_LIST,
    WIN_DESCRIPTION,
};

// Three rows rather than four (bug, reported after initial delivery): the
// description window used to be 4 tiles (32px), which is two lines of
// FONT_NORMAL, and PrintBoostStatus appended the status line after the
// description. Any boost whose description wrapped to two lines pushed that
// status onto a third line the window had no room for, so its cost simply
// wasn't drawn -- which is most of the catalog. Giving the description window
// a third line (see sBoostMenuWinTemplates) means the status always has a line
// of its own, and the tile row for it comes from the list.
#define BOOST_MENU_MAX_SHOWED 3

// Stage 11: a synthetic row appended after the real boosts, same "one past
// the last real enum value" trick src/achievements_menu.c uses for its own
// TIER_SELECT_ITEM_BOOSTS row -- never a real BoostId, never passed to
// AchievementBoost_GetInfo.
#define BOOST_MENU_ITEM_RESET BOOSTS_COUNT
#define BOOST_MENU_ITEM_COUNT (BOOSTS_COUNT) // (BOOSTS_COUNT - 1) real boosts (excludes BOOST_NONE) + the reset row

#define tListTaskId        data[0]
#define tScrollArrowTaskId data[1]

#define TAG_BOOST_MENU_SCROLL_ARROWS 6002

// Two right-aligned columns on each list row: the boost's current level (a
// lock icon or ON/OFF for a binary boost -- see BoostMenu_ItemPrintCallback),
// then what its next level costs. Names top out around 100px from item_X 8
// ("Legendary Encounter", the longest in the catalog), so neither column
// collides with them.
#define BOOST_MENU_LEVEL_RIGHT_X  152
#define BOOST_MENU_COST_RIGHT_X   190

// Third text line of the description window, at a fixed y rather than
// wherever the description above it happened to stop. See PrintBoostStatus.
#define BOOST_MENU_STATUS_Y 33

#define BOOST_MENU_ARROW_X        200
#define BOOST_MENU_ARROW_TOP_Y    36
// Follows the shortened list window (tile rows 5-10, so pixel rows 40-88)
// rather than the 4-row one it used to sit under.
#define BOOST_MENU_ARROW_BOTTOM_Y 92

// WIN_DESCRIPTION is 26 tiles (208px) wide, text starts at x=8 --
// AddTextPrinterParameterized never clips or wraps on its own (same
// precedent as src/achievement_popup.c's ACHIEVEMENT_POPUP_DESC_MAX_WIDTH),
// so an unwrapped boost description longer than this bleeds past the
// window's right edge into the tile memory of the status line below it.
#define BOOST_MENU_DESC_MAX_WIDTH 190

// Checkbox-free version of achievements_menu.c's equivalent buffer sizing
// comment: the longest real content is a boost name, already capped at
// BOOST_NAME_LENGTH (including its terminator) by BOOST_NAME().
#define BOOST_MENU_NAME_BUFFER_SIZE (BOOST_NAME_LENGTH + 8)

EWRAM_DATA static u8 sBoostMenuNameBuffers[BOOST_MENU_ITEM_COUNT][BOOST_MENU_NAME_BUFFER_SIZE] = {0};
EWRAM_DATA static struct ListMenuItem sBoostMenuListItems[BOOST_MENU_ITEM_COUNT] = {0};

EWRAM_DATA static struct
{
    u16 scrollOffset;
    u16 selectedRow;
} sBoostMenu = {0};

static void Task_BoostMenuFadeIn(u8 taskId);
static void Task_BoostMenuCancel(u8 taskId);
static void Task_BoostMenu_ProcessInput(u8 taskId);
static void EnterBoostMenuLevel(u8 taskId);
static void DestroyCurrentBoostList(u8 taskId);
static void TryPurchaseBoost(u8 taskId, u16 boostId);
static void TryPurchaseOrToggleBoost(u8 taskId, u16 boostId);
static void TryChangeHighlightedBoostActiveLevel(void);
static void BoostMenu_MoveCursorCallback(s32 itemIndex, bool8 onInit, struct ListMenu *list);
static void BoostMenu_ItemPrintCallback(u8 windowId, u32 boostId, u8 y);
static void BuildBoostMenuListItems(void);
static void PrintBoostDescription(const u8 *description);
static s32 PrintBoostLineText(const u8 *text, s32 x, u8 y);
static s32 BlitBoostLinePointsIcon(s32 x, u8 y);
static void PrintBoostStatus(s32 boostId);
static void DrawHeaderText(void);
static void DrawBgWindowFrames(void);

// Stage 11: the reset-confirmation sub-flow. Reuses WIN_LIST/WIN_DESCRIPTION
// and the same ListMenu plumbing as the main boost list -- "tear down and
// re-enter with a different item set" is the same trick TryPurchaseBoost
// already uses to refresh the list after a purchase, just pointed at a
// throwaway 2-item Yes/No list instead of the real boost catalog.
static void TryResetBoosts(u8 taskId);
static void EnterResetConfirmLevel(u8 taskId);
static void DestroyResetConfirmList(u8 taskId);
static void ReturnToBoostList(u8 taskId);
static void Task_BoostMenu_ConfirmResetInput(u8 taskId);
static void ResetConfirmMoveCursorCallback(s32 itemIndex, bool8 onInit, struct ListMenu *list);
static void PrintResetConfirmText(void);

static const u8 sText_BoostMenuTitle[]  = _("ACHIEVEMENT BOOSTS");
static const u8 sText_ControlHint[]     = _("{B_BUTTON}BACK");
static const u8 sText_BoostOn[]         = _("ON");
static const u8 sText_BoostOff[]        = _("OFF");
static const u8 sText_BoostMax[]        = _("MAX");
static const u8 sText_BoostLevelSep[]   = _("/");

// No "(have N)" any more -- the balance it used to tail every cost with is now
// in the header, where it stays visible whatever row is highlighted (see
// DrawHeaderText). The points icon takes the place of naming the unit.
static const u8 sText_BoostCostFormat[]      = _("Cost: {STR_VAR_1}");

// A boost's *active* level (AchievementBoost_GetActiveLevel) can sit below
// what's actually been purchased -- the boost menu's dpad L/R dials it there
// (see AchievementBoost_TryChangeActiveLevel) -- so once anything is owned,
// the status line always leads with where the active level stands relative
// to the purchased one, whether or not there's still a next level to buy.
static const u8 sText_BoostLevelCostFormat[] = _("Status: Lv {STR_VAR_1}/{STR_VAR_2} Cost: {STR_VAR_3}");
static const u8 sText_BoostLevelMaxFormat[]  = _("Status: Lv {STR_VAR_1}/{STR_VAR_2} (MAX)");

// A binary boost (maxLevel 1) has no level to dial -- just the one on/off
// switch, [A] on an owned row (see Task_BoostMenu_ProcessInput).
static const u8 sText_BoostToggleOnStatus[]  = _("Status: ON ({A_BUTTON} to turn off)");
static const u8 sText_BoostToggleOffStatus[] = _("Status: OFF ({A_BUTTON} to turn on)");
static const u8 sText_BoostSystemLockedStatus[] = _("Status: Boosts are locked");

// Stage 11: the synthetic "RESET BOOSTS" row and its confirmation sub-flow.
// The two figures that used to be suffixed "pts" are drawn with the points
// icon instead, which is why the refund and the fee are separate strings now
// rather than one format -- the icon goes between them.
static const u8 sText_ResetBoostsRowLabel[]       = _("RESET BOOSTS");
static const u8 sText_ResetBoostsDescription[]    = _("Refunds all points spent on boosts, for a fee.");
static const u8 sText_ResetNothingStatus[]        = _("Status: Nothing to reset");
static const u8 sText_ResetCantAffordStatus[]     = _("Status: Not enough money");
static const u8 sText_ResetRefundFormat[]         = _("Refund: {STR_VAR_1}");
static const u8 sText_ResetFeeFormat[]            = _("Fee: ¥{STR_VAR_1}");
static const u8 sText_ResetNewTotalFormat[]       = _("New total: {STR_VAR_1}");
static const u8 sText_ResetConfirmQuestion[]      = _("Reset all boosts?");
static const u8 sText_ResetConfirmYes[]           = _("YES");
static const u8 sText_ResetConfirmNo[]            = _("NO");

enum
{
    RESET_CONFIRM_YES,
    RESET_CONFIRM_NO,
};

static const struct ListMenuItem sResetConfirmListItems[] =
{
    [RESET_CONFIRM_YES] = { .name = sText_ResetConfirmYes, .id = RESET_CONFIRM_YES },
    [RESET_CONFIRM_NO]  = { .name = sText_ResetConfirmNo,  .id = RESET_CONFIRM_NO },
};

static const struct WindowTemplate sBoostMenuWinTemplates[] =
{
    [WIN_HEADER] = {
        .bg = 1,
        .tilemapLeft = 2,
        .tilemapTop = 1,
        .width = 26,
        .height = 2,
        .paletteNum = 1,
        .baseBlock = 2
    },
    // One tile row of the list was given to the description window (see
    // BOOST_MENU_MAX_SHOWED): 6 tiles each, three lines apiece. Both windows'
    // baseBlocks moved with that -- bg 0 and bg 1 share charBaseIndex 1, so
    // these are allocated end to end from the header's, and the description
    // window's old 0x106 plus its new 26x6 tiles would have run into the
    // window frame tiles at 0x1A2 (see DrawBgWindowFrames).
    [WIN_LIST] = {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 5,
        .width = 26,
        .height = 6,
        .paletteNum = 1,
        .baseBlock = 0x36
    },
    [WIN_DESCRIPTION] = {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 13,
        .width = 26,
        .height = 6,
        .paletteNum = 1,
        .baseBlock = 0xD2
    },
    DUMMY_WIN_TEMPLATE
};

static const struct BgTemplate sBoostMenuBgTemplates[] =
{
    {
        .bg = 1,
        .charBaseIndex = 1,
        .mapBaseIndex = 30,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0
    },
    {
        .bg = 0,
        .charBaseIndex = 1,
        .mapBaseIndex = 31,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 1,
        .baseTile = 0
    }
};

static const u16 sBoostMenuBg_Pal[] = {RGB(17, 18, 31)};
static const u16 sBoostMenuText_Pal[] = INCGFX_U16("graphics/interface/option_menu_text.pal", ".gbapal");

static void MainCB2(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

static void VBlankCB(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

void CB2_InitAchievementBoostMenu(void)
{
    u8 taskId;
    switch (gMain.state)
    {
    default:
    case 0:
        SetVBlankCallback(NULL);
        memset(&sBoostMenu, 0, sizeof(sBoostMenu));
        gMain.state++;
        break;
    case 1:
        DmaClearLarge16(3, (void *)(VRAM), VRAM_SIZE, 0x1000);
        DmaClear32(3, OAM, OAM_SIZE);
        DmaClear16(3, PLTT, PLTT_SIZE);
        SetGpuReg(REG_OFFSET_DISPCNT, 0);
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(0, sBoostMenuBgTemplates, ARRAY_COUNT(sBoostMenuBgTemplates));
        ChangeBgX(0, 0, BG_COORD_SET);
        ChangeBgY(0, 0, BG_COORD_SET);
        ChangeBgX(1, 0, BG_COORD_SET);
        ChangeBgY(1, 0, BG_COORD_SET);
        InitWindows(sBoostMenuWinTemplates);
        DeactivateAllTextPrinters();
        SetGpuReg(REG_OFFSET_WIN0H, 0);
        SetGpuReg(REG_OFFSET_WIN0V, 0);
        SetGpuReg(REG_OFFSET_WININ, 0);
        SetGpuReg(REG_OFFSET_WINOUT, 0);
        SetGpuReg(REG_OFFSET_BLDCNT, 0);
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
        ShowBg(0);
        ShowBg(1);
        gMain.state++;
        break;
    case 2:
        ResetPaletteFade();
        ScanlineEffect_Stop();
        ResetTasks();
        ResetSpriteData();
        gMain.state++;
        break;
    case 3:
        LoadBgTiles(1, GetWindowFrameTilesPal(gSaveBlock2Ptr->optionsWindowFrameType)->tiles, 0x120, 0x1A2);
        gMain.state++;
        break;
    case 4:
        LoadPalette(sBoostMenuBg_Pal, BG_PLTT_ID(0), sizeof(sBoostMenuBg_Pal));
        LoadPalette(GetWindowFrameTilesPal(gSaveBlock2Ptr->optionsWindowFrameType)->pal, BG_PLTT_ID(7), PLTT_SIZE_4BPP);
        gMain.state++;
        break;
    case 5:
        LoadPalette(sBoostMenuText_Pal, BG_PLTT_ID(1), sizeof(sBoostMenuText_Pal));
        // Must follow the LoadPalette above, not precede it -- this appends
        // the points icon's colours to that same palette's unused high
        // entries (src/achievement_icons.c).
        AchievementIcons_Load(1);
        gMain.state++;
        break;
    case 6:
        PutWindowTilemap(WIN_HEADER);
        gMain.state++;
        break;
    case 7:
        DrawBgWindowFrames();
        gMain.state++;
        break;
    case 8:
        PutWindowTilemap(WIN_LIST);
        PutWindowTilemap(WIN_DESCRIPTION);
        CopyBgTilemapBufferToVram(0);
        gMain.state++;
        break;
    case 9:
        taskId = CreateTask(Task_BoostMenuFadeIn, 0);
        EnterBoostMenuLevel(taskId);
        gMain.state++;
        break;
    case 10:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
        SetVBlankCallback(VBlankCB);
        SetMainCallback2(MainCB2);
        return;
    }
}

static void Task_BoostMenuFadeIn(u8 taskId)
{
    if (!gPaletteFade.active)
        gTasks[taskId].func = Task_BoostMenu_ProcessInput;
}

static void Task_BoostMenuCancel(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        DestroyCurrentBoostList(taskId);
        DestroyTask(taskId);
        FreeAllWindowBuffers();
        SetMainCallback2(gMain.savedCallback);
    }
}

static void EnterBoostMenuLevel(u8 taskId)
{
    struct ListMenuTemplate template = {0};

    DrawHeaderText();
    BuildBoostMenuListItems();

    template.items = sBoostMenuListItems;
    template.moveCursorFunc = BoostMenu_MoveCursorCallback;
    template.itemPrintFunc = BoostMenu_ItemPrintCallback;
    template.totalItems = BOOST_MENU_ITEM_COUNT;
    template.maxShowed = BOOST_MENU_MAX_SHOWED;
    template.windowId = WIN_LIST;
    template.header_X = 0;
    template.item_X = 8;
    template.cursor_X = 0;
    template.upText_Y = 1;
    template.cursorPal = 2;
    template.fillValue = 1;
    template.cursorShadowPal = 3;
    template.lettersSpacing = 0;
    template.itemVerticalPadding = 0;
    template.scrollMultiple = LIST_NO_MULTIPLE_SCROLL;
    template.fontId = FONT_NORMAL;
    template.cursorKind = CURSOR_BLACK_ARROW;

    gTasks[taskId].tListTaskId = ListMenuInit(&template, sBoostMenu.scrollOffset, sBoostMenu.selectedRow);
    gTasks[taskId].tScrollArrowTaskId = AddScrollIndicatorArrowPairParameterized(
        SCROLL_ARROW_UP, BOOST_MENU_ARROW_X, BOOST_MENU_ARROW_TOP_Y, BOOST_MENU_ARROW_BOTTOM_Y,
        // Clamped rather than a bare subtraction: a catalog smaller than
        // BOOST_MENU_MAX_SHOWED (as it was back when this file was written
        // against 2 test boosts) makes the raw difference negative. That gets
        // stored into struct ScrollIndicatorPair's fullyDownThreshold, which
        // is a u16 (src/list_menu.c:29) -- a negative s32 truncates into a
        // huge unsigned threshold the real (always-0) scroll offset can never
        // match, leaving the down arrow permanently visible with nothing left
        // to scroll to.
        (BOOST_MENU_ITEM_COUNT > BOOST_MENU_MAX_SHOWED) ? (BOOST_MENU_ITEM_COUNT - BOOST_MENU_MAX_SHOWED) : 0,
        TAG_BOOST_MENU_SCROLL_ARROWS, TAG_BOOST_MENU_SCROLL_ARROWS,
        &sBoostMenu.scrollOffset);
}

static void Task_BoostMenu_ProcessInput(u8 taskId)
{
    s32 itemId = ListMenu_ProcessInput(gTasks[taskId].tListTaskId);
    ListMenuGetScrollAndRow(gTasks[taskId].tListTaskId, &sBoostMenu.scrollOffset, &sBoostMenu.selectedRow);

    switch (itemId)
    {
    case LIST_NOTHING_CHOSEN:
        // Neither an item selection nor a cursor move -- the only other
        // input this screen reacts to is L/R dialing the highlighted
        // leveled boost's active level.
        TryChangeHighlightedBoostActiveLevel();
        break;
    case LIST_CANCEL:
        PlaySE(SE_SELECT);
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        gTasks[taskId].func = Task_BoostMenuCancel;
        break;
    case BOOST_MENU_ITEM_RESET:
        TryResetBoosts(taskId);
        break;
    default:
        TryPurchaseOrToggleBoost(taskId, itemId);
        break;
    }
}

// [A] on a real boost row. A binary boost (maxLevel 1) has no next level to
// buy once it's owned -- there's only the one copy of its effect -- so [A]
// there instead flips AchievementBoost_GetActiveLevel between 0 and 1 (see
// PrintBoostStatus/BoostMenu_ItemPrintCallback's ON/OFF). Everything else,
// including a not-yet-owned binary boost, still goes through the purchase
// path below unchanged.
static void TryPurchaseOrToggleBoost(u8 taskId, u16 boostId)
{
    const struct AchievementBoost *info = AchievementBoost_GetInfo(boostId);

    if (info->type == BOOST_TYPE_BINARY && AchievementBoost_GetLevel(boostId) > 0)
    {
        s8 delta = (AchievementBoost_GetActiveLevel(boostId) != 0) ? -1 : 1;

        // The ON/OFF column lives in the list row itself (unlike a leveled
        // boost's active level, which only ever shows in the status line),
        // so this needs the same full list rebuild a purchase does, not just
        // a PrintBoostStatus refresh.
        if (AchievementBoost_TryChangeActiveLevel(boostId, delta))
        {
            PlaySE(SE_SELECT);
            DestroyCurrentBoostList(taskId);
            EnterBoostMenuLevel(taskId);
        }
        else
        {
            PlaySE(SE_FAILURE);
        }
        return;
    }

    TryPurchaseBoost(taskId, boostId);
}

// L/R dpad: dials the highlighted leveled boost's active level up or down
// (AchievementBoost_TryChangeActiveLevel) without touching what's been
// purchased. A no-op on the reset row and on binary boosts, which use [A]
// instead (see TryPurchaseOrToggleBoost above). Unlike a purchase or a
// binary toggle, dialing a leveled boost's active level never changes what a
// list row shows -- only PrintBoostStatus's status line does -- so this is a
// plain status-line refresh rather than a list rebuild.
static void TryChangeHighlightedBoostActiveLevel(void)
{
    u16 boostId = sBoostMenuListItems[sBoostMenu.scrollOffset + sBoostMenu.selectedRow].id;
    s8 delta;

    if (boostId == BOOST_MENU_ITEM_RESET)
        return;
    if (AchievementBoost_GetInfo(boostId)->type == BOOST_TYPE_BINARY)
        return;

    if (JOY_NEW(DPAD_LEFT))
        delta = -1;
    else if (JOY_NEW(DPAD_RIGHT))
        delta = 1;
    else
        return;

    if (AchievementBoost_TryChangeActiveLevel(boostId, delta))
    {
        PlaySE(SE_SELECT);
        PrintBoostStatus(boostId);
    }
    else
    {
        PlaySE(SE_FAILURE);
    }
}

// A successful purchase changes the highlighted row's level/cost and this
// boost's available-points balance, so the simplest correct refresh is the
// same "tear down and re-enter with the cached scroll/selected row" trick
// achievements_menu.c uses between its own levels, rather than trying to
// patch just the affected row/text in place.
static void TryPurchaseBoost(u8 taskId, u16 boostId)
{
    if (AchievementBoost_Purchase(boostId))
    {
        PlaySE(SE_SHOP);
        DestroyCurrentBoostList(taskId);
        EnterBoostMenuLevel(taskId);
    }
    else
    {
        PlaySE(SE_FAILURE);
    }
}

// Stage 11: unlike TryPurchaseBoost, this doesn't commit anything by itself
// -- AchievementBoost_CanReset() gates entry into the confirmation sub-flow
// (Task_BoostMenu_ConfirmResetInput), which is the only place that actually
// calls AchievementBoost_Reset().
static void TryResetBoosts(u8 taskId)
{
    if (!AchievementBoost_CanReset())
    {
        PlaySE(SE_FAILURE);
        return;
    }

    PlaySE(SE_SELECT);
    DestroyCurrentBoostList(taskId);
    EnterResetConfirmLevel(taskId);
    gTasks[taskId].func = Task_BoostMenu_ConfirmResetInput;
}

static void BoostMenu_MoveCursorCallback(s32 itemIndex, bool8 onInit, struct ListMenu *list)
{
    if (!onInit)
        PlaySE(SE_SELECT);
    PrintBoostStatus(itemIndex);
}

static void BoostMenu_ItemPrintCallback(u8 windowId, u32 boostId, u8 y)
{
    const struct AchievementBoost *info;
    u8 level;
    u8 *ptr;
    s32 width;

    // Stage 11: the reset row has no level/cost to show on its right side --
    // AchievementBoost_GetInfo(BOOST_MENU_ITEM_RESET) would otherwise fall
    // back to the BOOST_NONE dummy entry (maxLevel 0) and misprint "MAX".
    if (boostId == BOOST_MENU_ITEM_RESET)
        return;

    info = AchievementBoost_GetInfo(boostId);
    level = AchievementBoost_GetLevel(boostId);

    // Binary boosts (maxLevel 1) have no level/cost columns of their own --
    // a lock icon in place of the word "LOCKED" while unpurchased, then
    // ON/OFF once owned, reflecting the active toggle (not just ownership) so
    // the row matches whatever PrintBoostStatus's [A]-toggle hint says.
    if (info->type == BOOST_TYPE_BINARY)
    {
        if (level == 0)
        {
            AchievementIcons_Blit(ACHIEVEMENT_ICON_LOCK, windowId, BOOST_MENU_LEVEL_RIGHT_X - ACHIEVEMENT_ICON_SIZE, ACHIEVEMENT_ICON_Y(y));
        }
        else
        {
            StringCopy(gStringVar4, AchievementBoost_GetActiveLevel(boostId) != 0 ? sText_BoostOn : sText_BoostOff);
            width = GetStringWidth(FONT_NORMAL, gStringVar4, 0);
            AddTextPrinterParameterized(windowId, FONT_NORMAL, gStringVar4, BOOST_MENU_LEVEL_RIGHT_X - width, y, TEXT_SKIP_DRAW, NULL);
        }
        return;
    }

    if (level >= info->maxLevel)
    {
        StringCopy(gStringVar4, sText_BoostMax);
    }
    else
    {
        ptr = ConvertIntToDecimalStringN(gStringVar4, level, STR_CONV_MODE_LEFT_ALIGN, 2);
        ptr = StringCopy(ptr, sText_BoostLevelSep);
        ConvertIntToDecimalStringN(ptr, info->maxLevel, STR_CONV_MODE_LEFT_ALIGN, 2);
    }

    width = GetStringWidth(FONT_NORMAL, gStringVar4, 0);
    AddTextPrinterParameterized(windowId, FONT_NORMAL, gStringVar4, BOOST_MENU_LEVEL_RIGHT_X - width, y, TEXT_SKIP_DRAW, NULL);

    // Second column: what this boost's next level costs. Previously a cost
    // was only ever shown for the highlighted row, down in the description
    // window, so the catalog couldn't be compared by price without stopping
    // on every row -- and for most boosts it wasn't shown even then (see
    // BOOST_MENU_MAX_SHOWED). Anything already maxed or owned has no next
    // level to price, and its level column already says so.
    if (level >= info->maxLevel)
        return;

    ConvertIntToDecimalStringN(gStringVar4, info->costs[level], STR_CONV_MODE_LEFT_ALIGN, 6);
    width = GetStringWidth(FONT_NORMAL, gStringVar4, 0);
    AddTextPrinterParameterized(windowId, FONT_NORMAL, gStringVar4, BOOST_MENU_COST_RIGHT_X - width, y, TEXT_SKIP_DRAW, NULL);
}

// Achievement completion (and therefore available points) can't change
// while this menu is open outside of a purchase, which already rebuilds the
// whole level -- same one-time-per-entry cost/benefit as
// achievements_menu.c's BuildAchievementListItems.
static void BuildBoostMenuListItems(void)
{
    u32 id, index = 0;

    for (id = BOOST_NONE + 1; id < BOOSTS_COUNT; id++)
    {
        const struct AchievementBoost *info = AchievementBoost_GetInfo(id);
        u8 *buffer = sBoostMenuNameBuffers[index];

        StringCopy(buffer, info->name);
        sBoostMenuListItems[index].name = buffer;
        sBoostMenuListItems[index].id = id;
        index++;
    }

    // Stage 11: the synthetic "RESET BOOSTS" row, always last.
    StringCopy(sBoostMenuNameBuffers[index], sText_ResetBoostsRowLabel);
    sBoostMenuListItems[index].name = sBoostMenuNameBuffers[index];
    sBoostMenuListItems[index].id = BOOST_MENU_ITEM_RESET;
}

// Wraps the description into the window's first two lines and leaves the
// third for the status. The wrapping (StripLineBreaks + BreakStringAutomatic,
// the same fix src/achievement_popup.c uses) is what stops a long description
// from drawing past the window's right edge and bleeding into the tile memory
// of the line below it; reserving a fixed line for the status is what stops a
// two-line description from pushing that status off the bottom of the window,
// which is why most boosts never showed a cost here.
static void PrintBoostDescription(const u8 *description)
{
    StringCopy(gStringVar1, description);
    StripLineBreaks(gStringVar1);
    BreakStringAutomatic(gStringVar1, BOOST_MENU_DESC_MAX_WIDTH, 2, FONT_NORMAL, HIDE_SCROLL_PROMPT);
    AddTextPrinterParameterized(WIN_DESCRIPTION, FONT_NORMAL, gStringVar1, 8, 1, TEXT_SKIP_DRAW, NULL);
}

// Prints text at x on the given line and returns where the next thing after it
// should start, so a run of alternating strings and points icons can be laid
// out left to right without any of them needing to know the widths of the
// others. Returns an x rather than drawing everything itself because what
// follows a figure differs per line: sometimes an icon, sometimes nothing.
static s32 PrintBoostLineText(const u8 *text, s32 x, u8 y)
{
    AddTextPrinterParameterized(WIN_DESCRIPTION, FONT_NORMAL, text, x, y, TEXT_SKIP_DRAW, NULL);
    return x + GetStringWidth(FONT_NORMAL, text, 0) + 2;
}

static s32 BlitBoostLinePointsIcon(s32 x, u8 y)
{
    AchievementIcons_Blit(ACHIEVEMENT_ICON_POINTS, WIN_DESCRIPTION, x, ACHIEVEMENT_ICON_Y(y));
    return x + ACHIEVEMENT_ICON_SIZE + 4;
}

static void PrintBoostStatus(s32 boostId)
{
    u8 statusBuf[40];

    FillWindowPixelBuffer(WIN_DESCRIPTION, PIXEL_FILL(1));

    if (boostId == BOOST_MENU_ITEM_RESET)
    {
        PrintBoostDescription(sText_ResetBoostsDescription);

        // Same priority order as AchievementBoost_CanReset (src/achievements.c),
        // reimplemented here for messaging -- same precedent as the real-boost
        // branch below, which reimplements AchievementBoost_CanPurchase's order
        // rather than calling it, so it can explain *which* check failed.
        if (!Achievement_BoostsUnlocked())
        {
            PrintBoostLineText(sText_BoostSystemLockedStatus, 8, BOOST_MENU_STATUS_Y);
        }
        else if (Achievement_GetAvailablePoints() == Achievement_GetTotalPoints())
        {
            // available == total iff pointsInvested == 0 -- nothing purchased
            // to refund. Avoids needing a public pointsInvested accessor.
            PrintBoostLineText(sText_ResetNothingStatus, 8, BOOST_MENU_STATUS_Y);
        }
        else if (!IsEnoughMoney(&gSaveBlock1Ptr->money, ACHIEVEMENT_BOOST_RESET_FEE))
        {
            PrintBoostLineText(sText_ResetCantAffordStatus, 8, BOOST_MENU_STATUS_Y);
        }
        else
        {
            s32 x;

            ConvertIntToDecimalStringN(gStringVar1, Achievement_GetTotalPoints() - Achievement_GetAvailablePoints(), STR_CONV_MODE_LEFT_ALIGN, 6);
            StringExpandPlaceholders(statusBuf, sText_ResetRefundFormat);
            x = PrintBoostLineText(statusBuf, 8, BOOST_MENU_STATUS_Y);
            x = BlitBoostLinePointsIcon(x, BOOST_MENU_STATUS_Y);

            ConvertIntToDecimalStringN(gStringVar1, ACHIEVEMENT_BOOST_RESET_FEE, STR_CONV_MODE_LEFT_ALIGN, 6);
            StringExpandPlaceholders(statusBuf, sText_ResetFeeFormat);
            PrintBoostLineText(statusBuf, x, BOOST_MENU_STATUS_Y);
        }
    }
    else if (boostId >= BOOST_NONE + 1 && boostId < BOOSTS_COUNT)
    {
        const struct AchievementBoost *info = AchievementBoost_GetInfo(boostId);
        u8 level = AchievementBoost_GetLevel(boostId);

        PrintBoostDescription(info->description);

        // Same priority order as AchievementBoost_CanPurchase itself
        // (src/achievements.c): unlocked, then per-boost owned/maxed, then
        // cost. Deliberately not gated on debug mode here -- see
        // AchievementBoost_CanPurchase's own comment for why spending
        // already-earned points stays available even on a run that's
        // disqualified from earning new ones.
        if (!Achievement_BoostsUnlocked())
        {
            PrintBoostLineText(sText_BoostSystemLockedStatus, 8, BOOST_MENU_STATUS_Y);
        }
        else if (info->type == BOOST_TYPE_BINARY && level > 0)
        {
            // Owned: [A] no longer purchases (there's no level 2 of a binary
            // boost), it flips AchievementBoost_GetActiveLevel between 0 and
            // 1 -- see Task_BoostMenu_ProcessInput.
            PrintBoostLineText(AchievementBoost_GetActiveLevel(boostId) != 0 ? sText_BoostToggleOnStatus : sText_BoostToggleOffStatus, 8, BOOST_MENU_STATUS_Y);
        }
        else if (level > 0 && level >= info->maxLevel)
        {
            // Leveled and maxed: still worth showing where the active level
            // stands -- AchievementBoost_TryChangeActiveLevel can dial a
            // maxed boost back just as freely as a partially-purchased one.
            ConvertIntToDecimalStringN(gStringVar1, AchievementBoost_GetActiveLevel(boostId), STR_CONV_MODE_LEFT_ALIGN, 2);
            ConvertIntToDecimalStringN(gStringVar2, level, STR_CONV_MODE_LEFT_ALIGN, 2);
            StringExpandPlaceholders(statusBuf, sText_BoostLevelMaxFormat);
            PrintBoostLineText(statusBuf, 8, BOOST_MENU_STATUS_Y);
        }
        else if (level > 0)
        {
            // Leveled, owned but not maxed: the active level shares the line
            // with what the next purchase would cost.
            ConvertIntToDecimalStringN(gStringVar1, AchievementBoost_GetActiveLevel(boostId), STR_CONV_MODE_LEFT_ALIGN, 2);
            ConvertIntToDecimalStringN(gStringVar2, level, STR_CONV_MODE_LEFT_ALIGN, 2);
            ConvertIntToDecimalStringN(gStringVar3, info->costs[level], STR_CONV_MODE_LEFT_ALIGN, 6);
            StringExpandPlaceholders(statusBuf, sText_BoostLevelCostFormat);
            BlitBoostLinePointsIcon(PrintBoostLineText(statusBuf, 8, BOOST_MENU_STATUS_Y), BOOST_MENU_STATUS_Y);
        }
        else
        {
            // Nothing purchased yet (true of every binary boost still
            // locked, and a leveled boost at level 0) -- nothing to dial,
            // just the entry cost.
            ConvertIntToDecimalStringN(gStringVar1, info->costs[level], STR_CONV_MODE_LEFT_ALIGN, 6);
            StringExpandPlaceholders(statusBuf, sText_BoostCostFormat);
            BlitBoostLinePointsIcon(PrintBoostLineText(statusBuf, 8, BOOST_MENU_STATUS_Y), BOOST_MENU_STATUS_Y);
        }
    }
    CopyWindowToVram(WIN_DESCRIPTION, COPYWIN_GFX);
}

static void DestroyCurrentBoostList(u8 taskId)
{
    DestroyListMenuTask(gTasks[taskId].tListTaskId, NULL, NULL);
    RemoveScrollIndicatorArrowPair(gTasks[taskId].tScrollArrowTaskId);
}

// Stage 11: the two-item Yes/No confirmation list. Only 2 rows against
// BOOST_MENU_MAX_SHOWED 4, so unlike EnterBoostMenuLevel this never needs
// scroll arrows -- DestroyResetConfirmList (below) mirrors that by never
// touching tScrollArrowTaskId, which still holds the main list's arrow pair
// until DestroyCurrentBoostList tears it down on the way back in.
static void EnterResetConfirmLevel(u8 taskId)
{
    struct ListMenuTemplate template = {0};

    PrintResetConfirmText();

    template.items = sResetConfirmListItems;
    template.moveCursorFunc = ResetConfirmMoveCursorCallback;
    template.itemPrintFunc = NULL;
    template.totalItems = ARRAY_COUNT(sResetConfirmListItems);
    template.maxShowed = ARRAY_COUNT(sResetConfirmListItems);
    template.windowId = WIN_LIST;
    template.header_X = 0;
    template.item_X = 8;
    template.cursor_X = 0;
    template.upText_Y = 1;
    template.cursorPal = 2;
    template.fillValue = 1;
    template.cursorShadowPal = 3;
    template.lettersSpacing = 0;
    template.itemVerticalPadding = 0;
    template.scrollMultiple = LIST_NO_MULTIPLE_SCROLL;
    template.fontId = FONT_NORMAL;
    template.cursorKind = CURSOR_BLACK_ARROW;

    gTasks[taskId].tListTaskId = ListMenuInit(&template, 0, 0);
}

static void DestroyResetConfirmList(u8 taskId)
{
    DestroyListMenuTask(gTasks[taskId].tListTaskId, NULL, NULL);
}

static void ReturnToBoostList(u8 taskId)
{
    DestroyResetConfirmList(taskId);
    EnterBoostMenuLevel(taskId);
    gTasks[taskId].func = Task_BoostMenu_ProcessInput;
}

static void Task_BoostMenu_ConfirmResetInput(u8 taskId)
{
    s32 itemId = ListMenu_ProcessInput(gTasks[taskId].tListTaskId);

    switch (itemId)
    {
    case LIST_NOTHING_CHOSEN:
        break;
    case LIST_CANCEL:
    case RESET_CONFIRM_NO:
        PlaySE(SE_SELECT);
        ReturnToBoostList(taskId);
        break;
    case RESET_CONFIRM_YES:
        // Re-verified inside AchievementBoost_Reset itself (same "never trust
        // a stale Can* result" precedent as AchievementBoost_Purchase) -- this
        // can still fail if something spent the fee's worth of money or the
        // profile's boosts got re-locked between opening this prompt and
        // choosing YES.
        if (AchievementBoost_Reset())
            PlaySE(SE_SHOP);
        else
            PlaySE(SE_FAILURE);
        ReturnToBoostList(taskId);
        break;
    }
}

static void ResetConfirmMoveCursorCallback(s32 itemIndex, bool8 onInit, struct ListMenu *list)
{
    if (!onInit)
        PlaySE(SE_SELECT);
}

// design doc §13/Stage 11 "the confirmation prompt shows the refund, the fee
// and the resulting totals before committing": this view is only ever
// entered when AchievementBoost_CanReset() already returned TRUE, so unlike
// PrintBoostStatus's reset branch, this doesn't need to handle the
// locked/nothing-to-reset/can't-afford cases -- there is always a real
// refund and fee to show.
// Three lines, one per text row of the description window. This used to be a
// single four-line format string in a window only two lines tall, so its last
// two lines -- the new total and the question itself -- were drawn past the
// bottom edge and never seen; the question leads now, and the figures follow
// on the lines the window actually has.
static void PrintResetConfirmText(void)
{
    u8 lineBuf[40];
    s32 x;

    FillWindowPixelBuffer(WIN_DESCRIPTION, PIXEL_FILL(1));

    PrintBoostLineText(sText_ResetConfirmQuestion, 8, 1);

    ConvertIntToDecimalStringN(gStringVar1, Achievement_GetTotalPoints() - Achievement_GetAvailablePoints(), STR_CONV_MODE_LEFT_ALIGN, 6);
    StringExpandPlaceholders(lineBuf, sText_ResetRefundFormat);
    x = PrintBoostLineText(lineBuf, 8, 17);
    x = BlitBoostLinePointsIcon(x, 17);

    ConvertIntToDecimalStringN(gStringVar1, ACHIEVEMENT_BOOST_RESET_FEE, STR_CONV_MODE_LEFT_ALIGN, 6);
    StringExpandPlaceholders(lineBuf, sText_ResetFeeFormat);
    PrintBoostLineText(lineBuf, x, 17);

    // After a full refund pointsInvested becomes 0, so the new available
    // total is exactly totalPointsEarned.
    ConvertIntToDecimalStringN(gStringVar1, Achievement_GetTotalPoints(), STR_CONV_MODE_LEFT_ALIGN, 6);
    StringExpandPlaceholders(lineBuf, sText_ResetNewTotalFormat);
    BlitBoostLinePointsIcon(PrintBoostLineText(lineBuf, 8, BOOST_MENU_STATUS_Y), BOOST_MENU_STATUS_Y);

    CopyWindowToVram(WIN_DESCRIPTION, COPYWIN_GFX);
}

// The header carries the player's spendable balance, between the title and the
// [B] BACK hint. It lives here rather than in the description window (where it
// used to tail every cost as "(have N)") so it's on screen whatever row is
// highlighted, and whatever sub-screen the menu is on -- shopping without
// knowing what you can afford was the other half of the reported problem.
// Redrawn by EnterBoostMenuLevel, which every purchase re-runs, so the figure
// tracks what was just spent.
static void DrawHeaderText(void)
{
    s32 hintX = GetStringRightAlignXOffset(FONT_NARROW, sText_ControlHint, 198);
    s32 pointsX;

    ConvertIntToDecimalStringN(gStringVar1, Achievement_GetAvailablePoints(), STR_CONV_MODE_LEFT_ALIGN, 6);
    // Right-aligned against the hint rather than laid out left to right, so
    // the balance holds its place as digits come and go instead of shuffling
    // sideways after every purchase.
    pointsX = hintX - 8 - GetStringWidth(FONT_NORMAL, gStringVar1, 0);

    FillWindowPixelBuffer(WIN_HEADER, PIXEL_FILL(1));
    AddTextPrinterParameterized(WIN_HEADER, FONT_NORMAL, sText_BoostMenuTitle, 8, 1, TEXT_SKIP_DRAW, NULL);
    AchievementIcons_Blit(ACHIEVEMENT_ICON_POINTS, WIN_HEADER, pointsX - ACHIEVEMENT_ICON_SIZE, ACHIEVEMENT_ICON_Y(1));
    AddTextPrinterParameterized(WIN_HEADER, FONT_NORMAL, gStringVar1, pointsX, 1, TEXT_SKIP_DRAW, NULL);
    AddTextPrinterParameterized(WIN_HEADER, FONT_NARROW, sText_ControlHint, hintX, 1, TEXT_SKIP_DRAW, NULL);
    CopyWindowToVram(WIN_HEADER, COPYWIN_FULL);
}

#define TILE_TOP_CORNER_L 0x1A2
#define TILE_TOP_EDGE     0x1A3
#define TILE_TOP_CORNER_R 0x1A4
#define TILE_LEFT_EDGE    0x1A5
#define TILE_RIGHT_EDGE   0x1A7
#define TILE_BOT_CORNER_L 0x1A8
#define TILE_BOT_EDGE     0x1A9
#define TILE_BOT_CORNER_R 0x1AA

static void DrawBgWindowFrames(void)
{
    //                     bg, tile,              x, y, width, height, palNum
    // Header frame
    FillBgTilemapBufferRect(1, TILE_TOP_CORNER_L,  1,  0,  1,  1,  7);
    FillBgTilemapBufferRect(1, TILE_TOP_EDGE,      2,  0, 27,  1,  7);
    FillBgTilemapBufferRect(1, TILE_TOP_CORNER_R, 28,  0,  1,  1,  7);
    FillBgTilemapBufferRect(1, TILE_LEFT_EDGE,     1,  1,  1,  2,  7);
    FillBgTilemapBufferRect(1, TILE_RIGHT_EDGE,   28,  1,  1,  2,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_CORNER_L,  1,  3,  1,  1,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_EDGE,      2,  3, 27,  1,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_CORNER_R, 28,  3,  1,  1,  7);

    // Boost list frame (one tile row shorter than it was, so the description
    // frame below can be one taller -- see sBoostMenuWinTemplates)
    FillBgTilemapBufferRect(1, TILE_TOP_CORNER_L,  1,  4,  1,  1,  7);
    FillBgTilemapBufferRect(1, TILE_TOP_EDGE,      2,  4, 26,  1,  7);
    FillBgTilemapBufferRect(1, TILE_TOP_CORNER_R, 28,  4,  1,  1,  7);
    FillBgTilemapBufferRect(1, TILE_LEFT_EDGE,     1,  5,  1,  6,  7);
    FillBgTilemapBufferRect(1, TILE_RIGHT_EDGE,   28,  5,  1,  6,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_CORNER_L,  1, 11,  1,  1,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_EDGE,      2, 11, 26,  1,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_CORNER_R, 28, 11,  1,  1,  7);

    // Description frame
    FillBgTilemapBufferRect(1, TILE_TOP_CORNER_L,  1, 12,  1,  1,  7);
    FillBgTilemapBufferRect(1, TILE_TOP_EDGE,      2, 12, 26,  1,  7);
    FillBgTilemapBufferRect(1, TILE_TOP_CORNER_R, 28, 12,  1,  1,  7);
    FillBgTilemapBufferRect(1, TILE_LEFT_EDGE,     1, 13,  1,  6,  7);
    FillBgTilemapBufferRect(1, TILE_RIGHT_EDGE,   28, 13,  1,  6,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_CORNER_L,  1, 19,  1,  1,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_EDGE,      2, 19, 26,  1,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_CORNER_R, 28, 19,  1,  1,  7);

    CopyBgTilemapBufferToVram(1);
}
