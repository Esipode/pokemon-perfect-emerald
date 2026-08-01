#include "global.h"
#include "achievements.h"
#include "achievement_boost_menu.h"
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
// boost's next level directly; there's no separate confirmation screen.
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

#define BOOST_MENU_MAX_SHOWED 4

// Stage 11: a synthetic row appended after the real boosts, same "one past
// the last real enum value" trick src/achievements_menu.c uses for its own
// TIER_SELECT_ITEM_BOOSTS row -- never a real BoostId, never passed to
// AchievementBoost_GetInfo.
#define BOOST_MENU_ITEM_RESET BOOSTS_COUNT
#define BOOST_MENU_ITEM_COUNT (BOOSTS_COUNT) // (BOOSTS_COUNT - 1) real boosts (excludes BOOST_NONE) + the reset row

#define tListTaskId        data[0]
#define tScrollArrowTaskId data[1]

#define TAG_BOOST_MENU_SCROLL_ARROWS 6002

#define BOOST_MENU_POINTS_RIGHT_X 190
#define BOOST_MENU_ARROW_X        200
#define BOOST_MENU_ARROW_TOP_Y    36
#define BOOST_MENU_ARROW_BOTTOM_Y 100

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
static void BoostMenu_MoveCursorCallback(s32 itemIndex, bool8 onInit, struct ListMenu *list);
static void BoostMenu_ItemPrintCallback(u8 windowId, u32 boostId, u8 y);
static void BuildBoostMenuListItems(void);
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
static const u8 sText_BoostLocked[]     = _("LOCKED");
static const u8 sText_BoostOwned[]      = _("OWNED");
static const u8 sText_BoostMax[]        = _("MAX");
static const u8 sText_BoostLevelSep[]   = _("/");

static const u8 sText_BoostCostFormat[]      = _("Cost: {STR_VAR_1} (have {STR_VAR_2})");
static const u8 sText_BoostMaxLevelStatus[]  = _("Status: Max level reached");
static const u8 sText_BoostOwnedStatus[]     = _("Status: Already unlocked");
static const u8 sText_BoostSystemLockedStatus[] = _("Status: Boosts are locked");
// The status line's Y position always follows wherever the (possibly
// wrapped, possibly multi-line) description ends, rather than a hardcoded
// y=17 -- combining them into one printer call with an embedded "\n" lets
// the text engine's own CHAR_NEWLINE handling (src/text.c) place it
// correctly instead of this file guessing at line-height math.
static const u8 sText_BoostDescriptionAndStatusFormat[] = _("{STR_VAR_1}\n{STR_VAR_2}");

// Stage 11: the synthetic "RESET BOOSTS" row and its confirmation sub-flow.
static const u8 sText_ResetBoostsRowLabel[]       = _("RESET BOOSTS");
static const u8 sText_ResetBoostsDescription[]    = _("Refunds all points spent on boosts, for a fee.");
static const u8 sText_ResetNothingStatus[]        = _("Status: Nothing to reset");
static const u8 sText_ResetCantAffordStatus[]     = _("Status: Not enough money");
static const u8 sText_ResetCostFormat[]           = _("Refund: {STR_VAR_1} pts (Fee: ¥{STR_VAR_2})");
static const u8 sText_ResetConfirmFormat[]        = _("Refund: {STR_VAR_1} pts\nFee: ¥{STR_VAR_2}\nNew total: {STR_VAR_3} pts\nReset all boosts?");
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
    [WIN_LIST] = {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 5,
        .width = 26,
        .height = 8,
        .paletteNum = 1,
        .baseBlock = 0x36
    },
    [WIN_DESCRIPTION] = {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 15,
        .width = 26,
        .height = 4,
        .paletteNum = 1,
        .baseBlock = 0x106
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
        // Clamped rather than a bare subtraction: with only 2 test boosts
        // (BOOST_MENU_ITEM_COUNT) against 4 visible rows
        // (BOOST_MENU_MAX_SHOWED), the raw difference is negative. That gets
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
        TryPurchaseBoost(taskId, itemId);
        break;
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

    if (info->type == BOOST_TYPE_BINARY)
    {
        StringCopy(gStringVar4, level > 0 ? sText_BoostOwned : sText_BoostLocked);
    }
    else if (level >= info->maxLevel)
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
    AddTextPrinterParameterized(windowId, FONT_NORMAL, gStringVar4, BOOST_MENU_POINTS_RIGHT_X - width, y, TEXT_SKIP_DRAW, NULL);
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

// Bug (reported after initial delivery): the description and status line
// were two separate AddTextPrinterParameterized calls at fixed y=1/y=17.
// AddTextPrinterParameterized doesn't clip or wrap on its own, so a
// description wider than the window (the real test descriptions are) kept
// drawing text past its right edge -- which, given how the window's tile
// buffer is laid out, bled into the status line's tile memory and showed up
// as the two overlapping. Fixed the same way src/achievement_popup.c already
// handles achievement descriptions: StripLineBreaks + BreakStringAutomatic
// to get clean, real line breaks, then the status line is appended after an
// explicit "\n" and both are drawn in a single printer call, so the text
// engine's own newline handling (src/text.c) places the status line whatever
// line the wrapped description actually ended on, instead of a hardcoded y.
static void PrintBoostStatus(s32 boostId)
{
    FillWindowPixelBuffer(WIN_DESCRIPTION, PIXEL_FILL(1));

    if (boostId == BOOST_MENU_ITEM_RESET)
    {
        u8 statusBuf[40];

        // Same priority order as AchievementBoost_CanReset (src/achievements.c),
        // reimplemented here for messaging -- same precedent as the real-boost
        // branch below, which reimplements AchievementBoost_CanPurchase's order
        // rather than calling it, so it can explain *which* check failed.
        if (!Achievement_BoostsUnlocked())
        {
            StringCopy(statusBuf, sText_BoostSystemLockedStatus);
        }
        else if (Achievement_GetAvailablePoints() == Achievement_GetTotalPoints())
        {
            // available == total iff pointsInvested == 0 -- nothing purchased
            // to refund. Avoids needing a public pointsInvested accessor.
            StringCopy(statusBuf, sText_ResetNothingStatus);
        }
        else if (!IsEnoughMoney(&gSaveBlock1Ptr->money, ACHIEVEMENT_BOOST_RESET_FEE))
        {
            StringCopy(statusBuf, sText_ResetCantAffordStatus);
        }
        else
        {
            u32 invested = Achievement_GetTotalPoints() - Achievement_GetAvailablePoints();
            ConvertIntToDecimalStringN(gStringVar1, invested, STR_CONV_MODE_LEFT_ALIGN, 6);
            ConvertIntToDecimalStringN(gStringVar2, ACHIEVEMENT_BOOST_RESET_FEE, STR_CONV_MODE_LEFT_ALIGN, 6);
            StringExpandPlaceholders(statusBuf, sText_ResetCostFormat);
        }

        StringCopy(gStringVar1, sText_ResetBoostsDescription);
        StripLineBreaks(gStringVar1);
        BreakStringAutomatic(gStringVar1, BOOST_MENU_DESC_MAX_WIDTH, 2, FONT_NORMAL, HIDE_SCROLL_PROMPT);
        StringCopy(gStringVar2, statusBuf);
        StringExpandPlaceholders(gStringVar4, sText_BoostDescriptionAndStatusFormat);

        AddTextPrinterParameterized(WIN_DESCRIPTION, FONT_NORMAL, gStringVar4, 8, 1, TEXT_SKIP_DRAW, NULL);
    }
    else if (boostId >= BOOST_NONE + 1 && boostId < BOOSTS_COUNT)
    {
        const struct AchievementBoost *info = AchievementBoost_GetInfo(boostId);
        u8 level = AchievementBoost_GetLevel(boostId);
        u8 statusBuf[40];

        // Same priority order as AchievementBoost_CanPurchase itself
        // (src/achievements.c): unlocked, then per-boost owned/maxed, then
        // cost. Deliberately not gated on debug mode here -- see
        // AchievementBoost_CanPurchase's own comment for why spending
        // already-earned points stays available even on a run that's
        // disqualified from earning new ones.
        if (!Achievement_BoostsUnlocked())
        {
            StringCopy(statusBuf, sText_BoostSystemLockedStatus);
        }
        else if (info->type == BOOST_TYPE_BINARY && level > 0)
        {
            StringCopy(statusBuf, sText_BoostOwnedStatus);
        }
        else if (level >= info->maxLevel)
        {
            StringCopy(statusBuf, sText_BoostMaxLevelStatus);
        }
        else
        {
            ConvertIntToDecimalStringN(gStringVar1, info->costs[level], STR_CONV_MODE_LEFT_ALIGN, 6);
            ConvertIntToDecimalStringN(gStringVar2, Achievement_GetAvailablePoints(), STR_CONV_MODE_LEFT_ALIGN, 6);
            StringExpandPlaceholders(statusBuf, sText_BoostCostFormat);
        }

        StringCopy(gStringVar1, info->description);
        StripLineBreaks(gStringVar1);
        BreakStringAutomatic(gStringVar1, BOOST_MENU_DESC_MAX_WIDTH, 2, FONT_NORMAL, HIDE_SCROLL_PROMPT);
        StringCopy(gStringVar2, statusBuf);
        StringExpandPlaceholders(gStringVar4, sText_BoostDescriptionAndStatusFormat);

        AddTextPrinterParameterized(WIN_DESCRIPTION, FONT_NORMAL, gStringVar4, 8, 1, TEXT_SKIP_DRAW, NULL);
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
static void PrintResetConfirmText(void)
{
    u32 invested = Achievement_GetTotalPoints() - Achievement_GetAvailablePoints();

    FillWindowPixelBuffer(WIN_DESCRIPTION, PIXEL_FILL(1));

    ConvertIntToDecimalStringN(gStringVar1, invested, STR_CONV_MODE_LEFT_ALIGN, 6);
    ConvertIntToDecimalStringN(gStringVar2, ACHIEVEMENT_BOOST_RESET_FEE, STR_CONV_MODE_LEFT_ALIGN, 6);
    // After a full refund pointsInvested becomes 0, so the new available
    // total is exactly totalPointsEarned.
    ConvertIntToDecimalStringN(gStringVar3, Achievement_GetTotalPoints(), STR_CONV_MODE_LEFT_ALIGN, 6);
    StringExpandPlaceholders(gStringVar4, sText_ResetConfirmFormat);

    AddTextPrinterParameterized(WIN_DESCRIPTION, FONT_NORMAL, gStringVar4, 8, 1, TEXT_SKIP_DRAW, NULL);
    CopyWindowToVram(WIN_DESCRIPTION, COPYWIN_GFX);
}

static void DrawHeaderText(void)
{
    s32 hintX = GetStringRightAlignXOffset(FONT_NARROW, sText_ControlHint, 198);

    FillWindowPixelBuffer(WIN_HEADER, PIXEL_FILL(1));
    AddTextPrinterParameterized(WIN_HEADER, FONT_NORMAL, sText_BoostMenuTitle, 8, 1, TEXT_SKIP_DRAW, NULL);
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

    // Boost list frame
    FillBgTilemapBufferRect(1, TILE_TOP_CORNER_L,  1,  4,  1,  1,  7);
    FillBgTilemapBufferRect(1, TILE_TOP_EDGE,      2,  4, 26,  1,  7);
    FillBgTilemapBufferRect(1, TILE_TOP_CORNER_R, 28,  4,  1,  1,  7);
    FillBgTilemapBufferRect(1, TILE_LEFT_EDGE,     1,  5,  1,  8,  7);
    FillBgTilemapBufferRect(1, TILE_RIGHT_EDGE,   28,  5,  1,  8,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_CORNER_L,  1, 13,  1,  1,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_EDGE,      2, 13, 26,  1,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_CORNER_R, 28, 13,  1,  1,  7);

    // Description frame
    FillBgTilemapBufferRect(1, TILE_TOP_CORNER_L,  1, 14,  1,  1,  7);
    FillBgTilemapBufferRect(1, TILE_TOP_EDGE,      2, 14, 26,  1,  7);
    FillBgTilemapBufferRect(1, TILE_TOP_CORNER_R, 28, 14,  1,  1,  7);
    FillBgTilemapBufferRect(1, TILE_LEFT_EDGE,     1, 15,  1,  4,  7);
    FillBgTilemapBufferRect(1, TILE_RIGHT_EDGE,   28, 15,  1,  4,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_CORNER_L,  1, 19,  1,  1,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_EDGE,      2, 19, 26,  1,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_CORNER_R, 28, 19,  1,  1,  7);

    CopyBgTilemapBufferToVram(1);
}
