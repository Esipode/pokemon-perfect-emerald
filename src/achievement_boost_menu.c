#include "global.h"
#include "achievements.h"
#include "achievement_boost_menu.h"
#include "achievement_icons.h"
#include "bg.h"
#include "decompress.h"
#include "gpu_regs.h"
#include "international_string_util.h"
#include "line_break.h"
#include "list_menu.h"
#include "main.h"
#include "malloc.h"
#include "menu.h"
#include "money.h"
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

// Skeleton copied from src/achievements_menu.c (BG/window templates, staged CB2
// init, ListMenu + scroll arrows).
//
// bg1 is a dedicated art layer showing the "boosts" screen of
// graphics/achievements/ui/bg_main.png, loaded once at CB2 init. bg0 holds all
// three windows in front of it; windows fill with PIXEL_FILL(0), the
// see-through index, so the art shows wherever there is no glyph ink. See
// src/ui_stat_editor.c for the same bg1-art/bg0-window split.
//
// Single flat list, no tiers. [A] on a row purchases that boost's next level.
// Once owned, [A] on a binary boost flips it on/off, and L/R on a leveled
// boost dials its active level without spending or refunding anything (see
// AchievementBoost_GetActiveLevel/_TryChangeActiveLevel in src/achievements.c).
//
// Reached from the achievements menu's tier select via a "BOOSTS" row shown
// when Achievement_BoostsUnlocked() && Achievement_BoostsEnabled(). Also
// reachable from the debug menu regardless of the unlock gate.
//
// A synthetic "RESET BOOSTS" row (BOOST_MENU_ITEM_RESET) follows the real
// boosts. [A] on it swaps WIN_LIST for a throwaway Yes/No list (see
// EnterResetConfirmLevel), since AchievementBoost_Reset() refunds every
// purchased level at once and costs Poké money.

enum
{
    WIN_HEADER,
    WIN_LIST,
    WIN_DESCRIPTION,
};

// The teal box holds 5 rows at 16px, as in src/achievements_menu.c's tier select list.
#define BOOST_MENU_MAX_SHOWED 5

// One past the last real enum value. Never a real BoostId; never passed to AchievementBoost_GetInfo.
#define BOOST_MENU_ITEM_RESET BOOSTS_COUNT
#define BOOST_MENU_ITEM_COUNT (BOOSTS_COUNT) // (BOOSTS_COUNT - 1) real boosts (excludes BOOST_NONE) + the reset row

#define tListTaskId        data[0]
#define tScrollArrowTaskId data[1]

#define TAG_BOOST_MENU_SCROLL_ARROWS 6002

// Centers of the level/toggle and cost columns, window-relative (WIN_LIST
// tilemapLeft is 16px), pixel-sampled from bg_main.png's light-blue boxes at
// screen x=167/202/238. Names top out ~100px from item_X 8, so neither column
// collides with them.
#define BOOST_MENU_LEVEL_CENTER_X 168
#define BOOST_MENU_COST_CENTER_X  204

// FONT_NORMAL lines are 16px and the window is 40px tall, leaving 8px of margin
// split 6px above LINE1_Y and 2px below LINE2_Y, to center against the art.
#define BOOST_MENU_LINE1_Y 7
// Fixed y for the two places needing two independent lines: the RESET BOOSTS status and PrintResetConfirmText.
#define BOOST_MENU_LINE2_Y 23

#define BOOST_MENU_ARROW_X        200
// 4px outside WIN_LIST's top/bottom edge, as src/achievements_menu.c's ACHIEVEMENTS_ARROW_TOP_Y/_BOTTOM_Y.
#define BOOST_MENU_ARROW_TOP_Y    20
#define BOOST_MENU_ARROW_BOTTOM_Y 108

// WIN_DESCRIPTION is 208px wide with text at x=8. Text printers never wrap, so
// an unwrapped longer description bleeds into the next line's tile memory.
#define BOOST_MENU_DESC_MAX_WIDTH 190

// Longest real content is a boost name, capped at BOOST_NAME_LENGTH.
#define BOOST_MENU_NAME_BUFFER_SIZE (BOOST_NAME_LENGTH + 8)

EWRAM_DATA static u8 sBoostMenuNameBuffers[BOOST_MENU_ITEM_COUNT][BOOST_MENU_NAME_BUFFER_SIZE] = {0};
EWRAM_DATA static struct ListMenuItem sBoostMenuListItems[BOOST_MENU_ITEM_COUNT] = {0};

EWRAM_DATA static struct
{
    u16 scrollOffset;
    u16 selectedRow;
    u16 highlightedId;
} sBoostMenu = {0};

static void Task_BoostMenuFadeIn(u8 taskId);
static void Task_BoostMenuCancel(u8 taskId);
static void Task_BoostMenu_ProcessInput(u8 taskId);
static void EnterBoostMenuLevel(u8 taskId);
static void DestroyCurrentBoostList(u8 taskId);
static void TryPurchaseBoost(u8 taskId, u16 boostId);
static void TryPurchaseOrToggleBoost(u8 taskId, u16 boostId);
static void TryChangeHighlightedBoostActiveLevel(u8 taskId);
static void BoostMenu_MoveCursorCallback(s32 itemIndex, bool8 onInit, struct ListMenu *list);
static void BoostMenu_ItemPrintCallback(u8 windowId, u32 boostId, u8 y);
static void BuildBoostMenuListItems(void);
static void PrintBoostDescription(s32 boostId);
static s32 PrintBoostLineText(const u8 *text, s32 x, u8 y);
static s32 BlitBoostLinePointsIcon(s32 x, u8 y);
static void PrintBoostStatus(s32 boostId);
static void DrawHeaderText(void);

// Reset-confirmation sub-flow: reuses WIN_LIST/WIN_DESCRIPTION with a throwaway 2-item Yes/No list.
static void TryResetBoosts(u8 taskId);
static void EnterResetConfirmLevel(u8 taskId);
static void DestroyResetConfirmList(u8 taskId);
static void ReturnToBoostList(u8 taskId);
static void Task_BoostMenu_ConfirmResetInput(u8 taskId);
static void ResetConfirmMoveCursorCallback(s32 itemIndex, bool8 onInit, struct ListMenu *list);
static void PrintResetConfirmText(void);

static const u8 sText_BoostMenuTitle[]  = _("BOOSTS");
static const u8 sText_ControlHint[]     = _("{B_BUTTON} BACK");
// Matches src/achievements_menu.c's sText_PointsSummaryFormat.
static const u8 sText_PointsSummaryFormat[] = _("{STR_VAR_1}/{STR_VAR_2}");
static const u8 sText_BoostOn[]         = _("ON");
static const u8 sText_BoostOff[]        = _("OFF");
static const u8 sText_BoostMax[]        = _("MAX");
static const u8 sText_BoostLevelSep[]   = _("/");
static const u8 sText_BoostEffectSpace[] = _(" ");

// Only shown when the menu is reached before Achievement_BoostsUnlocked() (debug).
static const u8 sText_BoostSystemLockedStatus[] = _("Boosts are locked.");

// Refund and fee are separate strings because the points icon is drawn between them.
static const u8 sText_ResetBoostsRowLabel[]       = _("RESET BOOSTS");
// One line: the reset row's status needs WIN_DESCRIPTION's second line.
static const u8 sText_ResetBoostsDescription[]    = _("Refunds points spent on boosts.");
static const u8 sText_ResetNothingStatus[]        = _("Nothing to reset.");
static const u8 sText_ResetCantAffordStatus[]     = _("Not enough money.");
static const u8 sText_ResetRefundFormat[]         = _("Refund: {STR_VAR_1}");
static const u8 sText_ResetFeeFormat[]            = _("Fee: ¥{STR_VAR_1}");
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
    // tilemapTop 0: the art's header band is y=0-15.
    [WIN_HEADER] = {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 0,
        .width = 26,
        .height = 2,
        .paletteNum = 1,
        .baseBlock = 2
    },
    // tilemapTop 3: the art's teal body box starts at pixel row ~23 on every
    // bg_main.png screen. Height 10: the box runs to about row 107. Width 28: the
    // price column reaches screen x=238; 26 tiles clipped the cost text around
    // BOOST_MENU_COST_CENTER_X.
    [WIN_LIST] = {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 3,
        .width = 28,
        .height = 10,
        .paletteNum = 1,
        .baseBlock = 0x36
    },
    // baseBlock follows WIN_LIST's (0x36 + 28*10 tiles = 0x14E).
    //
    // tilemapTop 14, height 5: the extra tile leaves a top margin above the two
    // lines while keeping the bottom edge fixed (14 + 5 == 15 + 4).
    [WIN_DESCRIPTION] = {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 14,
        .width = 26,
        .height = 5,
        .paletteNum = 1,
        .baseBlock = 0x14E
    },
    DUMMY_WIN_TEMPLATE
};

static const struct BgTemplate sBoostMenuBgTemplates[] =
{
    {
        // Art layer -- its own charBaseIndex, never touched by window text.
        .bg = 1,
        .charBaseIndex = 3,
        .mapBaseIndex = 30,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 1,
        .baseTile = 0
    },
    {
        // Windows only. Lower priority number than bg1 so text draws in
        // front of the art; PIXEL_FILL(0) everywhere there's no glyph ink
        // lets bg1 show through.
        .bg = 0,
        .charBaseIndex = 1,
        .mapBaseIndex = 31,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0
    }
};

static const u16 sBoostMenuText_Pal[] = INCGFX_U16("graphics/interface/option_menu_text.pal", ".gbapal");

// {background, foreground, shadow} palette indices: transparent, white, dark
// gray. TEXT_COLOR_* constants past WHITE don't match this palette
// (DARK_GRAY/LIGHT_GRAY are orange/amber, see option_menu_text.pal), so shadow
// index 7 (74 74 74) is written directly.
static const u8 sBoostMenuTextColors[3] = {
    TEXT_COLOR_TRANSPARENT, TEXT_COLOR_WHITE, 6
};

// Selected-row highlight: raw index 2 is orange in this palette. WIN_LIST uses
// the shared bank (paletteNum 1, no per-window remap), so the shadow index is
// the raw 6 every other window here uses.
static const u8 sBoostMenuListHighlightTextColors[3] = {
    TEXT_COLOR_TRANSPARENT, 2, 6
};

// Wraps the highlighted row's value in chevrons (as src/option_menu.c's
// DrawOptionMenuValue) to signal it can be dialed.
static const u8 sText_ChevronLeft[]  = _("{LEFT_ARROW}");
static const u8 sText_ChevronRight[] = _("{RIGHT_ARROW}");
#define BOOST_MENU_CHEVRON_GAP 2

// Deduped from graphics/achievements/ui/bg_main.png (the "boosts" screen).
static const u32 sBoostsScreenTiles[]   = INCBIN_U32("graphics/achievements/ui/boosts_tileset.4bpp.smol");
static const u32 sBoostsScreenTilemap[] = INCBIN_U32("graphics/achievements/ui/boosts_tileset.bin.smolTM");
static const u16 sBoostsScreenPal[]     = INCBIN_U16("graphics/achievements/ui/boosts_tileset.gbapal");

// bg1's WRAM tilemap buffer, allocated at CB2 init and freed in Task_BoostMenuCancel. Never reloaded.
EWRAM_DATA static u8 *sBoostMenuBg1Tilemap = NULL;

static void MainCB2(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    // Flushes bg1's art tilemap, which CB2_InitAchievementBoostMenu only schedules.
    // Windows copy to VRAM on their own.
    DoScheduledBgTilemapCopiesToVram();
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
        sBoostMenuBg1Tilemap = Alloc(0x800);
        memset(sBoostMenuBg1Tilemap, 0, 0x800);
        SetBgTilemapBuffer(1, sBoostMenuBg1Tilemap);
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
        DecompressAndCopyTileDataToVram(1, sBoostsScreenTiles, 0, 0, 0);
        gMain.state++;
        break;
    case 4:
        FreeTempTileDataBuffersIfPossible();
        DecompressDataWithHeaderWram(sBoostsScreenTilemap, sBoostMenuBg1Tilemap);
        ScheduleBgCopyTilemapToVram(1);
        LoadPalette(sBoostsScreenPal, BG_PLTT_ID(0), PLTT_SIZE_4BPP);
        gMain.state++;
        break;
    case 5:
        LoadPalette(sBoostMenuText_Pal, BG_PLTT_ID(1), sizeof(sBoostMenuText_Pal));
        // Must follow the LoadPalette above; appends the points icon's colours to that palette.
        AchievementIcons_Load(1);
        gMain.state++;
        break;
    case 6:
        PutWindowTilemap(WIN_HEADER);
        gMain.state++;
        break;
    case 7:
        PutWindowTilemap(WIN_LIST);
        PutWindowTilemap(WIN_DESCRIPTION);
        CopyBgTilemapBufferToVram(0);
        gMain.state++;
        break;
    case 8:
        taskId = CreateTask(Task_BoostMenuFadeIn, 0);
        EnterBoostMenuLevel(taskId);
        gMain.state++;
        break;
    case 9:
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
        Free(sBoostMenuBg1Tilemap);
        sBoostMenuBg1Tilemap = NULL;
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
    template.cursorPal = 1;
    template.fillValue = 0;
    template.cursorShadowPal = 6;
    template.lettersSpacing = 0;
    template.itemVerticalPadding = 0;
    template.scrollMultiple = LIST_NO_MULTIPLE_SCROLL;
    template.fontId = FONT_NORMAL;
    // No arrow cursor; ListMenuOverrideSetColors' orange marks the selected row.
    template.cursorKind = CURSOR_INVISIBLE;

    gTasks[taskId].tListTaskId = ListMenuInit(&template, sBoostMenu.scrollOffset, sBoostMenu.selectedRow);
    gTasks[taskId].tScrollArrowTaskId = AddScrollIndicatorArrowPairParameterized(
        SCROLL_ARROW_UP, BOOST_MENU_ARROW_X, BOOST_MENU_ARROW_TOP_Y, BOOST_MENU_ARROW_BOTTOM_Y,
        // Clamped: a negative difference truncates in the u16 fullyDownThreshold
        // (src/list_menu.c:29), leaving the down arrow permanently visible.
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
        // Neither a selection nor a cursor move; L/R dial the highlighted boost's active level.
        TryChangeHighlightedBoostActiveLevel(taskId);
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

// [A] on an owned binary boost flips its active level between 0 and 1.
// Everything else goes through the purchase path.
static void TryPurchaseOrToggleBoost(u8 taskId, u16 boostId)
{
    const struct AchievementBoost *info = AchievementBoost_GetInfo(boostId);

    if (info->type == BOOST_TYPE_BINARY && AchievementBoost_GetLevel(boostId) > 0)
    {
        s8 delta = (AchievementBoost_GetActiveLevel(boostId) != 0) ? -1 : 1;

// Full rebuild like a purchase; [A] does not repeat.
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

// L/R dials the highlighted leveled boost's active level, or flips a binary's
// ON/OFF, without changing what is purchased. No-op on the reset row.
// RedrawListMenu rather than a full rebuild since L/R can repeat rapidly and the
// item set is unchanged.
static void TryChangeHighlightedBoostActiveLevel(u8 taskId)
{
    u16 boostId = sBoostMenuListItems[sBoostMenu.scrollOffset + sBoostMenu.selectedRow].id;
    s8 delta;

    if (boostId == BOOST_MENU_ITEM_RESET)
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
        RedrawListMenu(gTasks[taskId].tListTaskId);
        PrintBoostStatus(boostId);
    }
    else
    {
        PlaySE(SE_FAILURE);
    }
}

// A purchase changes the row's level/cost and the points balance, so tear down
// and re-enter with the cached scroll/selected row.
static void TryPurchaseBoost(u8 taskId, u16 boostId)
{
    // A purchase buys the next level past what is owned. If the active level was
    // dialed back below that, buying would jump the owned level past the active
    // one with no cost shown (the cost column follows this same condition).
    // Refuse until L/R dials back up.
    if (AchievementBoost_GetActiveLevel(boostId) != AchievementBoost_GetLevel(boostId))
    {
        PlaySE(SE_FAILURE);
        return;
    }

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

// Commits nothing itself: AchievementBoost_CanReset() gates entry, and
// Task_BoostMenu_ConfirmResetInput calls AchievementBoost_Reset().
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
    // Read by BoostMenu_ItemPrintCallback to pick the highlighted row. itemIndex is
    // the item id, not the row position.
    sBoostMenu.highlightedId = itemIndex;
    // A same-page cursor move never reprints row text (CURSOR_INVISIBLE), so
    // without this the highlight would stick to the initially selected row.
    ListMenuRepaintItems(list);
    PrintBoostStatus(itemIndex);
}

// Centers text in the level/toggle column, chevron-wrapped on the highlighted
// row. Everything reaching here is dialable, so chevrons and the orange
// highlight always agree.
static void DrawBoostMenuLevelValue(u8 windowId, u8 y, const u8 *text, const u8 *colors, bool8 selected)
{
    s32 width = GetStringWidth(FONT_NORMAL, text, 0);
    s32 x = BOOST_MENU_LEVEL_CENTER_X - width / 2;

    AddTextPrinterParameterized3(windowId, FONT_NORMAL, x, y, colors, TEXT_SKIP_DRAW, text);

    if (selected)
    {
        s32 leftWidth = GetStringWidth(FONT_NORMAL, sText_ChevronLeft, 0);
        AddTextPrinterParameterized3(windowId, FONT_NORMAL, x - leftWidth - BOOST_MENU_CHEVRON_GAP, y, colors, TEXT_SKIP_DRAW, sText_ChevronLeft);
        AddTextPrinterParameterized3(windowId, FONT_NORMAL, x + width + BOOST_MENU_CHEVRON_GAP, y, colors, TEXT_SKIP_DRAW, sText_ChevronRight);
    }
}

static void BoostMenu_ItemPrintCallback(u8 windowId, u32 boostId, u8 y)
{
    const struct AchievementBoost *info;
    u8 owned, active;
    s32 width;
    bool8 selected = (boostId == sBoostMenu.highlightedId);
    const u8 *colors = selected ? sBoostMenuListHighlightTextColors : sBoostMenuTextColors;

// Recolours the row name ListMenuPrint draws after this returns, including
    // the RESET BOOSTS row before its early return.
    if (selected)
        ListMenuOverrideSetColors(colors[1], colors[0], colors[2]);

// The reset row has no level/cost; AchievementBoost_GetInfo would return the
    // BOOST_NONE dummy (maxLevel 0) and misprint "MAX".
    if (boostId == BOOST_MENU_ITEM_RESET)
        return;

    info = AchievementBoost_GetInfo(boostId);
    owned = AchievementBoost_GetLevel(boostId);
    active = AchievementBoost_GetActiveLevel(boostId);

// First column: locked (nothing purchased) shows the lock icon. Owned shows
    // ON/OFF, MAX, or the active/owned fraction, chevron-wrapped when highlighted.
    if (owned == 0)
    {
        AchievementIcons_Blit(ACHIEVEMENT_ICON_LOCK, windowId, BOOST_MENU_LEVEL_CENTER_X - ACHIEVEMENT_ICON_SIZE / 2, ACHIEVEMENT_ICON_Y(y));
    }
    else if (info->type == BOOST_TYPE_BINARY)
    {
        StringCopy(gStringVar4, active != 0 ? sText_BoostOn : sText_BoostOff);
        DrawBoostMenuLevelValue(windowId, y, gStringVar4, colors, selected);
    }
    else if (owned >= info->maxLevel && active == owned)
    {
        DrawBoostMenuLevelValue(windowId, y, sText_BoostMax, colors, selected);
    }
    else
    {
        u8 *ptr = ConvertIntToDecimalStringN(gStringVar4, active, STR_CONV_MODE_LEFT_ALIGN, 2);
        ptr = StringCopy(ptr, sText_BoostLevelSep);
        ConvertIntToDecimalStringN(ptr, owned, STR_CONV_MODE_LEFT_ALIGN, 2);
        DrawBoostMenuLevelValue(windowId, y, gStringVar4, colors, selected);
    }

// Second column: next level cost, only when [A] would purchase (not maxed,
    // and active level not dialed below owned).
    if (owned >= info->maxLevel || active != owned)
        return;

    ConvertIntToDecimalStringN(gStringVar4, info->costs[owned], STR_CONV_MODE_LEFT_ALIGN, 6);
    width = GetStringWidth(FONT_NORMAL, gStringVar4, 0);
    AddTextPrinterParameterized3(windowId, FONT_NORMAL, BOOST_MENU_COST_CENTER_X - width / 2, y, colors, TEXT_SKIP_DRAW, gStringVar4);
}

// Completion cannot change while the menu is open outside a purchase, which rebuilds the level.
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

    StringCopy(sBoostMenuNameBuffers[index], sText_ResetBoostsRowLabel);
    sBoostMenuListItems[index].name = sBoostMenuNameBuffers[index];
    sBoostMenuListItems[index].id = BOOST_MENU_ITEM_RESET;
}

// Wraps the description into the window's two lines (StripLineBreaks +
// BreakStringAutomatic) so it does not bleed past the right edge. A leveled
// boost appends its active level's effect; nothing at level 0.
static void PrintBoostDescription(s32 boostId)
{
    const struct AchievementBoost *info = AchievementBoost_GetInfo(boostId);
    u8 active = AchievementBoost_GetActiveLevel(boostId);

    StringCopy(gStringVar1, info->description);
    StripLineBreaks(gStringVar1);

    if (info->effectFormat != NULL && info->effects != NULL && active != 0)
    {
        ConvertIntToDecimalStringN(gStringVar2, info->effects[active], STR_CONV_MODE_LEFT_ALIGN, 3);
        StringExpandPlaceholders(gStringVar3, info->effectFormat);
        StringAppend(gStringVar1, sText_BoostEffectSpace);
        StringAppend(gStringVar1, gStringVar3);
    }
    BreakStringAutomatic(gStringVar1, BOOST_MENU_DESC_MAX_WIDTH, 2, FONT_NORMAL, HIDE_SCROLL_PROMPT);
    AddTextPrinterParameterized3(WIN_DESCRIPTION, FONT_NORMAL, 8, BOOST_MENU_LINE1_Y, sBoostMenuTextColors, TEXT_SKIP_DRAW, gStringVar1);
}

// Prints text at x and returns where the next element starts, so alternating
// strings and points icons lay out left to right.
static s32 PrintBoostLineText(const u8 *text, s32 x, u8 y)
{
    AddTextPrinterParameterized3(WIN_DESCRIPTION, FONT_NORMAL, x, y, sBoostMenuTextColors, TEXT_SKIP_DRAW, text);
    return x + GetStringWidth(FONT_NORMAL, text, 0) + 2;
}

static s32 BlitBoostLinePointsIcon(s32 x, u8 y)
{
    AchievementIcons_Blit(ACHIEVEMENT_ICON_POINTS, WIN_DESCRIPTION, x, ACHIEVEMENT_ICON_Y(y));
    return x + ACHIEVEMENT_ICON_SIZE + 4;
}

// WIN_DESCRIPTION shows the plain description, or the RESET BOOSTS row's status
// (refund/fee, or why it is unavailable).
static void PrintBoostStatus(s32 boostId)
{
    u8 statusBuf[40];

    FillWindowPixelBuffer(WIN_DESCRIPTION, PIXEL_FILL(0));

// Debug-only: the menu is reachable before Achievement_BoostsUnlocked().
    // Skip the description; the window has only two lines.
    if (!Achievement_BoostsUnlocked())
    {
        PrintBoostLineText(sText_BoostSystemLockedStatus, 8, BOOST_MENU_LINE1_Y);
    }
    else if (boostId == BOOST_MENU_ITEM_RESET)
    {
        PrintBoostLineText(sText_ResetBoostsDescription, 8, BOOST_MENU_LINE1_Y);

// Same priority order as AchievementBoost_CanReset (src/achievements.c),
        // reimplemented to explain which check failed.
        if (Achievement_GetAvailablePoints() == Achievement_GetTotalPoints())
        {
            // available == total iff pointsInvested == 0 -- nothing purchased
            // to refund. Avoids needing a public pointsInvested accessor.
            PrintBoostLineText(sText_ResetNothingStatus, 8, BOOST_MENU_LINE2_Y);
        }
        else if (!IsEnoughMoney(&gSaveBlock1Ptr->money, ACHIEVEMENT_BOOST_RESET_FEE))
        {
            PrintBoostLineText(sText_ResetCantAffordStatus, 8, BOOST_MENU_LINE2_Y);
        }
        else
        {
            s32 x;

            ConvertIntToDecimalStringN(gStringVar1, Achievement_GetTotalPoints() - Achievement_GetAvailablePoints(), STR_CONV_MODE_LEFT_ALIGN, 6);
            StringExpandPlaceholders(statusBuf, sText_ResetRefundFormat);
            x = PrintBoostLineText(statusBuf, 8, BOOST_MENU_LINE2_Y);
            x = BlitBoostLinePointsIcon(x, BOOST_MENU_LINE2_Y);

            ConvertIntToDecimalStringN(gStringVar1, ACHIEVEMENT_BOOST_RESET_FEE, STR_CONV_MODE_LEFT_ALIGN, 6);
            StringExpandPlaceholders(statusBuf, sText_ResetFeeFormat);
            PrintBoostLineText(statusBuf, x, BOOST_MENU_LINE2_Y);
        }
    }
    else if (boostId >= BOOST_NONE + 1 && boostId < BOOSTS_COUNT)
    {
        PrintBoostDescription(boostId);
    }

    CopyWindowToVram(WIN_DESCRIPTION, COPYWIN_GFX);
}

static void DestroyCurrentBoostList(u8 taskId)
{
    DestroyListMenuTask(gTasks[taskId].tListTaskId, NULL, NULL);
    RemoveScrollIndicatorArrowPair(gTasks[taskId].tScrollArrowTaskId);
}

// Only 2 rows against BOOST_MENU_MAX_SHOWED 5, so no scroll arrows.
// DestroyResetConfirmList leaves tScrollArrowTaskId alone; it still holds the
// main list's arrows until DestroyCurrentBoostList.
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
    template.fillValue = 0;
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
        // Re-verified inside AchievementBoost_Reset; can fail if money was spent or
        // boosts were re-locked since the prompt opened.
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

// Entered only when AchievementBoost_CanReset() returned TRUE, so unlike
// PrintBoostStatus's reset branch there is always a real refund and fee.
// Two lines: the question, then refund and fee. The header already shows the
// available/total fraction.
static void PrintResetConfirmText(void)
{
    u8 lineBuf[40];
    s32 x;

    FillWindowPixelBuffer(WIN_DESCRIPTION, PIXEL_FILL(0));

    PrintBoostLineText(sText_ResetConfirmQuestion, 8, BOOST_MENU_LINE1_Y);

    ConvertIntToDecimalStringN(gStringVar1, Achievement_GetTotalPoints() - Achievement_GetAvailablePoints(), STR_CONV_MODE_LEFT_ALIGN, 6);
    StringExpandPlaceholders(lineBuf, sText_ResetRefundFormat);
    x = PrintBoostLineText(lineBuf, 8, BOOST_MENU_LINE2_Y);
    x = BlitBoostLinePointsIcon(x, BOOST_MENU_LINE2_Y);

    ConvertIntToDecimalStringN(gStringVar1, ACHIEVEMENT_BOOST_RESET_FEE, STR_CONV_MODE_LEFT_ALIGN, 6);
    StringExpandPlaceholders(lineBuf, sText_ResetFeeFormat);
    PrintBoostLineText(lineBuf, x, BOOST_MENU_LINE2_Y);

    CopyWindowToVram(WIN_DESCRIPTION, COPYWIN_GFX);
}

// Matches src/achievements_menu.c's DrawTierSelectHeaderText layout on
// WIN_HEADER's one line: title, points summary ({available}/{total earned}),
// then the [B] BACK hint. Redrawn by EnterBoostMenuLevel, which every purchase
// re-runs.
static void DrawHeaderText(void)
{
    // FONT_NARROW leaves the points summary more room before it falls back
    // through GetFontIdToFit.
    s32 titleX = 2;
    s32 hintX = GetStringRightAlignXOffset(FONT_NARROW, sText_ControlHint, 198);
    s32 pointsIconX = titleX + GetStringWidth(FONT_NARROW, sText_BoostMenuTitle, 0) + 4;
    s32 pointsTextX = pointsIconX + ACHIEVEMENT_ICON_SIZE + 2;
    s32 availWidth = (hintX - 8) - pointsTextX;
    u32 fontId;

    if (availWidth < 0)
        availWidth = 0;

    FillWindowPixelBuffer(WIN_HEADER, PIXEL_FILL(0));
    AddTextPrinterParameterized3(WIN_HEADER, FONT_NARROW, titleX, 0, sBoostMenuTextColors, TEXT_SKIP_DRAW, sText_BoostMenuTitle);
    AddTextPrinterParameterized3(WIN_HEADER, FONT_NARROW, hintX, 0, sBoostMenuTextColors, TEXT_SKIP_DRAW, sText_ControlHint);

    // {available points}/{total points earned}.
    ConvertIntToDecimalStringN(gStringVar1, Achievement_GetAvailablePoints(), STR_CONV_MODE_LEFT_ALIGN, 6);
    ConvertIntToDecimalStringN(gStringVar2, Achievement_GetTotalPoints(), STR_CONV_MODE_LEFT_ALIGN, 6);
    StringExpandPlaceholders(gStringVar4, sText_PointsSummaryFormat);
    fontId = GetFontIdToFit(gStringVar4, FONT_NORMAL, 0, availWidth);

// Not ACHIEVEMENT_ICON_Y(0): its -1 would underflow a u16 at the header's
    // y=0 (same as achievements_menu.c's header).
    AchievementIcons_Blit(ACHIEVEMENT_ICON_POINTS, WIN_HEADER, pointsIconX, 0);
    AddTextPrinterParameterized3(WIN_HEADER, fontId, pointsTextX, 0, sBoostMenuTextColors, TEXT_SKIP_DRAW, gStringVar4);

    CopyWindowToVram(WIN_HEADER, COPYWIN_FULL);
}
