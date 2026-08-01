#include "global.h"
#include "achievements.h"
#include "achievements_menu.h"
#include "achievement_boost_menu.h"
#include "bg.h"
#include "gpu_regs.h"
#include "international_string_util.h"
#include "line_break.h"
#include "list_menu.h"
#include "main.h"
#include "menu.h"
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

// Stage 3.1 template (design doc §3.1): src/new_game_settings_menu.c's
// skeleton copied wholesale -- BG/window templates, staged CB2 init,
// ListMenu + scroll arrows, DrawBgWindowFrames chrome.
//
// Stage 3.2 (design doc §3.2): the three-level TIER SELECT / LIST / DETAIL
// flow. One CB2 boots the screen straight into TIER SELECT; the three levels
// then swap the task's func and rebuild the same WIN_HEADER/WIN_LIST/
// WIN_DESCRIPTION trio in place rather than re-running the CB2 state machine,
// so moving between levels never fades. Only leaving the menu entirely (B
// from TIER SELECT) fades out.
//
// Stage 3.3 (Start Menu entry point) is wired separately in
// src/start_menu.c (MENU_ACTION_ACHIEVEMENTS / StartMenuAchievementsCallback).
//
// The "Boosts" row on the TIER SELECT mockup (Stage 7): appended to the tier
// list as one extra row, id TIER_SELECT_ITEM_BOOSTS, only when
// Achievement_BoostsUnlocked() && Achievement_BoostsEnabled() (Stage 6: OFF
// hides the shop, not just the toggle). Selecting it fades out and jumps to
// src/achievement_boost_menu.c's CB2_InitAchievementBoostMenu, with
// gMain.savedCallback pointed back at CB2_InitAchievementsMenu so its own
// [B] Back re-enters here at a fresh TIER SELECT. Since that overwrites
// gMain.savedCallback -- the same slot this screen's own [B] Back reads on
// its way out -- sAchievementsMenuReturnCallback/sReturningFromBoostShop
// stash the real caller (Start Menu/debug menu) beforehand and restore it on
// the way back in, so leaving from TIER SELECT after a boost-shop visit
// still returns to the real caller instead of looping back into this menu.

enum
{
    WIN_HEADER,
    WIN_LIST,
    WIN_DESCRIPTION,
};

#define ACHIEVEMENT_TIER_COUNT (ACHIEVEMENT_TIER_DIAMOND + 1)

#define ACHIEVEMENTS_MENU_MAX_SHOWED 4
#define ACHIEVEMENTS_MENU_ITEM_COUNT (ACHIEVEMENTS_COUNT - 1) // excludes ACHIEVEMENT_NONE

#define tListTaskId        data[0]
#define tScrollArrowTaskId data[1]

#define TAG_ACHIEVEMENTS_SCROLL_ARROWS 6001

#define ACHIEVEMENTS_POINTS_RIGHT_X 190
#define ACHIEVEMENTS_ARROW_X        200
#define ACHIEVEMENTS_ARROW_TOP_Y    36
#define ACHIEVEMENTS_ARROW_BOTTOM_Y 100

// WIN_DESCRIPTION/WIN_LIST are 26 tiles (208px) wide, text starts at x=8 --
// AddTextPrinterParameterized never clips or wraps on its own (design doc/
// src/achievement_popup.c's own ACHIEVEMENT_POPUP_DESC_MAX_WIDTH precedent),
// so an unwrapped achievement description longer than this bleeds past the
// window's right edge into the tile memory of the row below it.
#define ACHIEVEMENTS_DESC_MAX_WIDTH 190

// Checkbox/tier-name prefix plus the item text itself; the longest real
// content (an achievement name) is already capped at ACHIEVEMENT_NAME_LENGTH
// (including its terminator) by ACHIEVEMENT_NAME(), so this leaves generous
// headroom rather than computing the exact minimum. Reused for both the
// achievement list rows and the (shorter) tier select rows.
#define ACHIEVEMENTS_LIST_NAME_BUFFER_SIZE (ACHIEVEMENT_NAME_LENGTH + 8)

// TIER SELECT's own row count once the Stage 7 "BOOSTS" row is visible --
// one past the last real tier ID, reused as that row's ListMenuItem.id too
// (see TIER_SELECT_ITEM_BOOSTS below).
#define TIER_SELECT_ROW_COUNT (ACHIEVEMENT_TIER_COUNT + 1)

// Shared by both lists this menu ever shows (tier select's up to
// TIER_SELECT_ROW_COUNT rows, or one tier's worth of achievement rows),
// sized to whichever is larger. Right now the placeholder catalog (Stage
// 2.3's 3 test achievements) is smaller than the tier count, so this must
// not just be ACHIEVEMENTS_MENU_ITEM_COUNT -- that undersized the array and
// corrupted memory past its end when tier select wrote all its rows.
#define ACHIEVEMENTS_MENU_LIST_CAPACITY \
    (ACHIEVEMENTS_MENU_ITEM_COUNT > TIER_SELECT_ROW_COUNT ? ACHIEVEMENTS_MENU_ITEM_COUNT : TIER_SELECT_ROW_COUNT)

EWRAM_DATA static u8 sAchievementsListNameBuffers[ACHIEVEMENTS_MENU_LIST_CAPACITY][ACHIEVEMENTS_LIST_NAME_BUFFER_SIZE] = {0};
EWRAM_DATA static struct ListMenuItem sAchievementsListItems[ACHIEVEMENTS_MENU_LIST_CAPACITY] = {0};

EWRAM_DATA static struct
{
    u8 selectedTier;
    u16 listItemCount; // filtered count for the tier currently shown at LIST level
    u16 tierScrollOffset;
    u16 tierSelectedRow;
    u16 listScrollOffset;
    u16 listSelectedRow;
} sAchievementsMenu = {0};

// Cached once per TIER SELECT build (see BuildTierSelectListItems) so the
// per-row itemPrintFunc doesn't re-scan every achievement on every redraw.
EWRAM_DATA static struct
{
    u16 completed;
    u16 total;
} sTierCounts[ACHIEVEMENT_TIER_COUNT] = {0};

// Stage 7: this menu's own CB2 doubles as the boost shop's return point
// (Task_TierSelect_OpenBoostMenu sets gMain.savedCallback =
// CB2_InitAchievementsMenu before jumping there), which would otherwise
// clobber the *real* caller (Start Menu/debug menu) recorded in
// gMain.savedCallback on entry. sAchievementsMenuReturnCallback is that real
// caller, stashed away before the overwrite and restored into
// gMain.savedCallback the moment this screen is re-entered from the boost
// shop -- sReturningFromBoostShop is what tells case 0 which of those two
// things is happening.
EWRAM_DATA static bool8 sReturningFromBoostShop = FALSE;
EWRAM_DATA static void (*sAchievementsMenuReturnCallback)(void) = NULL;

static void Task_AchievementsMenuFadeIn(u8 taskId);
static void Task_AchievementsMenuCancel(u8 taskId);
static void Task_TierSelect_ProcessInput(u8 taskId);
static void Task_TierSelect_OpenBoostMenu(u8 taskId);
static void Task_List_ProcessInput(u8 taskId);
static void Task_Detail_ProcessInput(u8 taskId);
static void EnterTierSelectLevel(u8 taskId);
static bool8 IsBoostShopRowVisible(void);
static void EnterListLevel(u8 taskId, u8 tier);
static void EnterDetailLevel(u8 taskId, u16 achievementId);
static void DestroyCurrentAchievementsList(u8 taskId);
static void TierSelect_MoveCursorCallback(s32 itemIndex, bool8 onInit, struct ListMenu *list);
static void TierSelect_ItemPrintCallback(u8 windowId, u32 tier, u8 y);
static void AchievementsMenu_MoveCursorCallback(s32 itemIndex, bool8 onInit, struct ListMenu *list);
static void AchievementsMenu_ItemPrintCallback(u8 windowId, u32 achievementId, u8 y);
static void BuildTierSelectListItems(void);
static void BuildAchievementListItems(u8 tier);
static void PrintPointsSummary(void);
static void PrintAchievementDescription(s32 achievementId);
static void DrawHeaderText(const u8 *title);
static void DrawBgWindowFrames(void);

static const u8 sText_AchievementsTitle[]  = _("ACHIEVEMENTS");
static const u8 sText_ControlHint[]        = _("{B_BUTTON}BACK");
// '[' and ']' aren't in charmap.txt -- use the existing filled/hollow circle
// glyphs instead of literal brackets.
static const u8 sText_CompletedPrefix[]    = _("{CIRCLE_DOT} ");
static const u8 sText_IncompletePrefix[]   = _("{CIRCLE_HOLLOW} ");
// design doc §17: hidden achievements show as "???" -- name and description
// both -- until completed. Their point value and tier are not withheld
// (mirrors the design doc §3.2 mockup: "[ ] ???                50").
static const u8 sText_HiddenName[]         = _("???");
static const u8 sText_HiddenDescription[]  = _("???");

static const u8 sText_TierBronze[]  = _("BRONZE");
static const u8 sText_TierSilver[]  = _("SILVER");
static const u8 sText_TierGold[]    = _("GOLD");
static const u8 sText_TierDiamond[] = _("DIAMOND");

static const u8 *const sTierNames[ACHIEVEMENT_TIER_COUNT] =
{
    [ACHIEVEMENT_TIER_BRONZE]  = sText_TierBronze,
    [ACHIEVEMENT_TIER_SILVER]  = sText_TierSilver,
    [ACHIEVEMENT_TIER_GOLD]    = sText_TierGold,
    [ACHIEVEMENT_TIER_DIAMOND] = sText_TierDiamond,
};

static const u8 sText_TierCountSeparator[]  = _(" / ");
static const u8 sText_BoostsMenuRowLabel[]  = _("BOOSTS");

// The extra TIER SELECT row (Stage 7) sits one past the last real tier ID --
// safe as a ListMenuItem.id since tier IDs and this are otherwise disjoint,
// and TierSelect_ItemPrintCallback/Task_TierSelect_ProcessInput both check
// for it before treating an itemId as a tier.
#define TIER_SELECT_ITEM_BOOSTS ACHIEVEMENT_TIER_COUNT
static const u8 sText_PointsSummaryFormat[] = _("Points: {STR_VAR_1} ({STR_VAR_2} free)");
static const u8 sText_RewardFormat[]        = _("Reward: {STR_VAR_1} Points");
static const u8 sText_StatusCompleted[]     = _("Status: Completed");
static const u8 sText_StatusIncomplete[]    = _("Status: Not completed");

static const struct WindowTemplate sAchievementsMenuWinTemplates[] =
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

static const struct BgTemplate sAchievementsMenuBgTemplates[] =
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

static const u16 sAchievementsMenuBg_Pal[] = {RGB(17, 18, 31)};
static const u16 sAchievementsMenuText_Pal[] = INCGFX_U16("graphics/interface/option_menu_text.pal", ".gbapal");

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

void CB2_InitAchievementsMenu(void)
{
    u8 taskId;
    switch (gMain.state)
    {
    default:
    case 0:
        SetVBlankCallback(NULL);
        if (sReturningFromBoostShop)
        {
            gMain.savedCallback = sAchievementsMenuReturnCallback;
            sReturningFromBoostShop = FALSE;
        }
        else
        {
            sAchievementsMenuReturnCallback = gMain.savedCallback;
        }
        memset(&sAchievementsMenu, 0, sizeof(sAchievementsMenu));
        gMain.state++;
        break;
    case 1:
        DmaClearLarge16(3, (void *)(VRAM), VRAM_SIZE, 0x1000);
        DmaClear32(3, OAM, OAM_SIZE);
        DmaClear16(3, PLTT, PLTT_SIZE);
        SetGpuReg(REG_OFFSET_DISPCNT, 0);
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(0, sAchievementsMenuBgTemplates, ARRAY_COUNT(sAchievementsMenuBgTemplates));
        ChangeBgX(0, 0, BG_COORD_SET);
        ChangeBgY(0, 0, BG_COORD_SET);
        ChangeBgX(1, 0, BG_COORD_SET);
        ChangeBgY(1, 0, BG_COORD_SET);
        InitWindows(sAchievementsMenuWinTemplates);
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
        LoadPalette(sAchievementsMenuBg_Pal, BG_PLTT_ID(0), sizeof(sAchievementsMenuBg_Pal));
        LoadPalette(GetWindowFrameTilesPal(gSaveBlock2Ptr->optionsWindowFrameType)->pal, BG_PLTT_ID(7), PLTT_SIZE_4BPP);
        gMain.state++;
        break;
    case 5:
        LoadPalette(sAchievementsMenuText_Pal, BG_PLTT_ID(1), sizeof(sAchievementsMenuText_Pal));
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
        taskId = CreateTask(Task_AchievementsMenuFadeIn, 0);
        EnterTierSelectLevel(taskId);
        gMain.state++;
        break;
    case 10:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
        SetVBlankCallback(VBlankCB);
        SetMainCallback2(MainCB2);
        return;
    }
}

static void Task_AchievementsMenuFadeIn(u8 taskId)
{
    if (!gPaletteFade.active)
        gTasks[taskId].func = Task_TierSelect_ProcessInput;
}

static void Task_AchievementsMenuCancel(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        DestroyCurrentAchievementsList(taskId);
        DestroyTask(taskId);
        FreeAllWindowBuffers();
        SetMainCallback2(gMain.savedCallback);
    }
}

// ---- TIER SELECT ---------------------------------------------------------

static void EnterTierSelectLevel(u8 taskId)
{
    struct ListMenuTemplate template = {0};
    u8 itemCount = ACHIEVEMENT_TIER_COUNT + (IsBoostShopRowVisible() ? 1 : 0);

    DrawHeaderText(sText_AchievementsTitle);
    PrintPointsSummary();
    BuildTierSelectListItems();

    template.items = sAchievementsListItems;
    template.moveCursorFunc = TierSelect_MoveCursorCallback;
    template.itemPrintFunc = TierSelect_ItemPrintCallback;
    template.totalItems = itemCount;
    template.maxShowed = ACHIEVEMENTS_MENU_MAX_SHOWED;
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

    gTasks[taskId].tListTaskId = ListMenuInit(&template, sAchievementsMenu.tierScrollOffset, sAchievementsMenu.tierSelectedRow);
    gTasks[taskId].tScrollArrowTaskId = AddScrollIndicatorArrowPairParameterized(
        SCROLL_ARROW_UP, ACHIEVEMENTS_ARROW_X, ACHIEVEMENTS_ARROW_TOP_Y, ACHIEVEMENTS_ARROW_BOTTOM_Y,
        itemCount - ACHIEVEMENTS_MENU_MAX_SHOWED, TAG_ACHIEVEMENTS_SCROLL_ARROWS, TAG_ACHIEVEMENTS_SCROLL_ARROWS,
        &sAchievementsMenu.tierScrollOffset);
}

static void Task_TierSelect_ProcessInput(u8 taskId)
{
    s32 itemId = ListMenu_ProcessInput(gTasks[taskId].tListTaskId);
    ListMenuGetScrollAndRow(gTasks[taskId].tListTaskId, &sAchievementsMenu.tierScrollOffset, &sAchievementsMenu.tierSelectedRow);

    switch (itemId)
    {
    case LIST_NOTHING_CHOSEN:
        break;
    case LIST_CANCEL:
        PlaySE(SE_SELECT);
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        gTasks[taskId].func = Task_AchievementsMenuCancel;
        break;
    case TIER_SELECT_ITEM_BOOSTS:
        PlaySE(SE_SELECT);
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        gTasks[taskId].func = Task_TierSelect_OpenBoostMenu;
        break;
    default:
        PlaySE(SE_SELECT);
        sAchievementsMenu.listScrollOffset = 0;
        sAchievementsMenu.listSelectedRow = 0;
        DestroyCurrentAchievementsList(taskId);
        EnterListLevel(taskId, itemId);
        gTasks[taskId].func = Task_List_ProcessInput;
        break;
    }
}

// Fades out, tears down this screen exactly like Task_AchievementsMenuCancel
// does, then jumps straight into the boost shop (src/achievement_boost_menu.c)
// instead of gMain.savedCallback -- with gMain.savedCallback repointed at
// CB2_InitAchievementsMenu first, so the shop's own [B] Back re-enters here
// at a fresh TIER SELECT rather than returning to the field/debug menu.
static void Task_TierSelect_OpenBoostMenu(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        DestroyCurrentAchievementsList(taskId);
        DestroyTask(taskId);
        FreeAllWindowBuffers();
        sReturningFromBoostShop = TRUE;
        gMain.savedCallback = CB2_InitAchievementsMenu;
        SetMainCallback2(CB2_InitAchievementBoostMenu);
    }
}

static void TierSelect_MoveCursorCallback(s32 itemIndex, bool8 onInit, struct ListMenu *list)
{
    if (!onInit)
        PlaySE(SE_SELECT);
}

static void TierSelect_ItemPrintCallback(u8 windowId, u32 tier, u8 y)
{
    u8 *ptr;
    s32 width;

    // The Stage 7 "BOOSTS" row (id TIER_SELECT_ITEM_BOOSTS) isn't a tier --
    // sTierCounts[] has no entry for it, and it doesn't need a count column.
    if (tier >= ACHIEVEMENT_TIER_COUNT)
        return;

    ptr = ConvertIntToDecimalStringN(gStringVar4, sTierCounts[tier].completed, STR_CONV_MODE_LEFT_ALIGN, 3);
    ptr = StringCopy(ptr, sText_TierCountSeparator);
    ConvertIntToDecimalStringN(ptr, sTierCounts[tier].total, STR_CONV_MODE_LEFT_ALIGN, 3);

    width = GetStringWidth(FONT_NORMAL, gStringVar4, 0);
    AddTextPrinterParameterized(windowId, FONT_NORMAL, gStringVar4, ACHIEVEMENTS_POINTS_RIGHT_X - width, y, TEXT_SKIP_DRAW, NULL);
}

static void BuildTierSelectListItems(void)
{
    u32 tier, id;

    for (tier = 0; tier < ACHIEVEMENT_TIER_COUNT; tier++)
    {
        sTierCounts[tier].completed = 0;
        sTierCounts[tier].total = 0;
    }

    for (id = ACHIEVEMENT_NONE + 1; id < ACHIEVEMENTS_COUNT; id++)
    {
        tier = Achievement_GetInfo(id)->tier;
        sTierCounts[tier].total++;
        if (Achievement_IsCompleted(id))
            sTierCounts[tier].completed++;
    }

    for (tier = 0; tier < ACHIEVEMENT_TIER_COUNT; tier++)
    {
        u8 *buffer = sAchievementsListNameBuffers[tier];

        StringCopy(buffer, sTierNames[tier]);
        sAchievementsListItems[tier].name = buffer;
        sAchievementsListItems[tier].id = tier;
    }

    if (IsBoostShopRowVisible())
    {
        u8 *buffer = sAchievementsListNameBuffers[ACHIEVEMENT_TIER_COUNT];

        StringCopy(buffer, sText_BoostsMenuRowLabel);
        sAchievementsListItems[ACHIEVEMENT_TIER_COUNT].name = buffer;
        sAchievementsListItems[ACHIEVEMENT_TIER_COUNT].id = TIER_SELECT_ITEM_BOOSTS;
    }
}

// design doc Stage 6: OFF hides the shop (not just the toggle), so this
// checks both -- unlocked but disabled must not show the row.
static bool8 IsBoostShopRowVisible(void)
{
    return Achievement_BoostsUnlocked() && Achievement_BoostsEnabled();
}

static void PrintPointsSummary(void)
{
    ConvertIntToDecimalStringN(gStringVar1, Achievement_GetTotalPoints(), STR_CONV_MODE_LEFT_ALIGN, 6);
    ConvertIntToDecimalStringN(gStringVar2, Achievement_GetAvailablePoints(), STR_CONV_MODE_LEFT_ALIGN, 6);
    StringExpandPlaceholders(gStringVar4, sText_PointsSummaryFormat);

    FillWindowPixelBuffer(WIN_DESCRIPTION, PIXEL_FILL(1));
    AddTextPrinterParameterized(WIN_DESCRIPTION, FONT_NORMAL, gStringVar4, 8, 1, TEXT_SKIP_DRAW, NULL);
    CopyWindowToVram(WIN_DESCRIPTION, COPYWIN_GFX);
}

// ---- ACHIEVEMENT LIST -----------------------------------------------------

static void EnterListLevel(u8 taskId, u8 tier)
{
    struct ListMenuTemplate template = {0};

    sAchievementsMenu.selectedTier = tier;

    DrawHeaderText(sTierNames[tier]);
    BuildAchievementListItems(tier);

    template.items = sAchievementsListItems;
    template.moveCursorFunc = AchievementsMenu_MoveCursorCallback;
    template.itemPrintFunc = AchievementsMenu_ItemPrintCallback;
    template.totalItems = sAchievementsMenu.listItemCount;
    template.maxShowed = ACHIEVEMENTS_MENU_MAX_SHOWED;
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

    gTasks[taskId].tListTaskId = ListMenuInit(&template, sAchievementsMenu.listScrollOffset, sAchievementsMenu.listSelectedRow);
    gTasks[taskId].tScrollArrowTaskId = AddScrollIndicatorArrowPairParameterized(
        SCROLL_ARROW_UP, ACHIEVEMENTS_ARROW_X, ACHIEVEMENTS_ARROW_TOP_Y, ACHIEVEMENTS_ARROW_BOTTOM_Y,
        // Clamped, not a bare subtraction: every tier but BRONZE currently
        // has fewer than ACHIEVEMENTS_MENU_MAX_SHOWED achievements (Stage
        // 2.3's 3 test achievements are all BRONZE), so this can go
        // negative. A negative threshold truncates into a huge u16 when
        // stored (struct ScrollIndicatorPair.fullyDownThreshold,
        // src/list_menu.c:29) that the real scroll offset can never match,
        // leaving the down arrow stuck visible with nothing left to scroll
        // to (same issue fixed for the Stage 7 boost list in
        // src/achievement_boost_menu.c).
        (sAchievementsMenu.listItemCount > ACHIEVEMENTS_MENU_MAX_SHOWED) ? (sAchievementsMenu.listItemCount - ACHIEVEMENTS_MENU_MAX_SHOWED) : 0,
        TAG_ACHIEVEMENTS_SCROLL_ARROWS, TAG_ACHIEVEMENTS_SCROLL_ARROWS,
        &sAchievementsMenu.listScrollOffset);
}

static void Task_List_ProcessInput(u8 taskId)
{
    s32 itemId = ListMenu_ProcessInput(gTasks[taskId].tListTaskId);
    ListMenuGetScrollAndRow(gTasks[taskId].tListTaskId, &sAchievementsMenu.listScrollOffset, &sAchievementsMenu.listSelectedRow);

    switch (itemId)
    {
    case LIST_NOTHING_CHOSEN:
        break;
    case LIST_CANCEL:
        PlaySE(SE_SELECT);
        DestroyCurrentAchievementsList(taskId);
        EnterTierSelectLevel(taskId);
        gTasks[taskId].func = Task_TierSelect_ProcessInput;
        break;
    default:
        PlaySE(SE_SELECT);
        DestroyCurrentAchievementsList(taskId);
        EnterDetailLevel(taskId, itemId);
        gTasks[taskId].func = Task_Detail_ProcessInput;
        break;
    }
}

static void AchievementsMenu_MoveCursorCallback(s32 itemIndex, bool8 onInit, struct ListMenu *list)
{
    if (!onInit)
        PlaySE(SE_SELECT);
    PrintAchievementDescription(itemIndex);
}

static void AchievementsMenu_ItemPrintCallback(u8 windowId, u32 achievementId, u8 y)
{
    s32 width;

    ConvertIntToDecimalStringN(gStringVar1, Achievement_GetInfo(achievementId)->points, STR_CONV_MODE_LEFT_ALIGN, 5);
    width = GetStringWidth(FONT_NORMAL, gStringVar1, 0);

    AddTextPrinterParameterized(windowId, FONT_NORMAL, gStringVar1, ACHIEVEMENTS_POINTS_RIGHT_X - width, y, TEXT_SKIP_DRAW, NULL);
}

// Builds the tier-filtered item list (skips ACHIEVEMENT_NONE and any ID
// outside this tier) and bakes the completion checkbox into each row's label
// text. Hidden achievements (design doc §17) render their name as "???"
// until completed. Achievement completion can't change while this menu is
// open, so this only needs to run once, at entry to the tier, rather than
// being recomputed per redraw.
static void BuildAchievementListItems(u8 tier)
{
    u32 id, index = 0;

    for (id = ACHIEVEMENT_NONE + 1; id < ACHIEVEMENTS_COUNT; id++)
    {
        const struct Achievement *info = Achievement_GetInfo(id);
        u8 *buffer;
        bool8 completed;

        if (info->tier != tier)
            continue;

        completed = Achievement_IsCompleted(id);
        buffer = sAchievementsListNameBuffers[index];

        StringCopy(buffer, completed ? sText_CompletedPrefix : sText_IncompletePrefix);
        StringAppend(buffer, (info->hidden && !completed) ? sText_HiddenName : info->name);

        sAchievementsListItems[index].name = buffer;
        sAchievementsListItems[index].id = id;
        index++;
    }

    sAchievementsMenu.listItemCount = index;
}

// Bug (reported after initial delivery): descriptions were printed raw, with
// no width limit. AddTextPrinterParameterized doesn't clip or wrap on its
// own, so anything wider than the window kept drawing past its right edge --
// which (given how the window's tile buffer is laid out) bled into the tile
// memory of the row below, showing up as leftover/overlapping text the next
// time that row was drawn. StripLineBreaks + BreakStringAutomatic is the
// same fix src/achievement_popup.c already uses for achievement descriptions
// (see its ACHIEVEMENT_POPUP_DESC_MAX_WIDTH) -- strip any pre-existing manual
// breaks so BreakStringAutomatic computes clean wrapping from scratch, then
// let it insert real line breaks so long descriptions correctly use this
// window's second line instead of spilling off the first one.
static void PrintAchievementDescription(s32 achievementId)
{
    FillWindowPixelBuffer(WIN_DESCRIPTION, PIXEL_FILL(1));
    if (achievementId >= ACHIEVEMENT_NONE + 1 && achievementId < ACHIEVEMENTS_COUNT)
    {
        const struct Achievement *info = Achievement_GetInfo(achievementId);
        bool8 masked = info->hidden && !Achievement_IsCompleted(achievementId);

        StringCopy(gStringVar1, masked ? sText_HiddenDescription : info->description);
        StripLineBreaks(gStringVar1);
        BreakStringAutomatic(gStringVar1, ACHIEVEMENTS_DESC_MAX_WIDTH, 2, FONT_NORMAL, HIDE_SCROLL_PROMPT);
        AddTextPrinterParameterized(WIN_DESCRIPTION, FONT_NORMAL, gStringVar1, 8, 1, TEXT_SKIP_DRAW, NULL);
    }
    CopyWindowToVram(WIN_DESCRIPTION, COPYWIN_GFX);
}

// ---- DETAIL ----------------------------------------------------------------

static void EnterDetailLevel(u8 taskId, u16 achievementId)
{
    const struct Achievement *info = Achievement_GetInfo(achievementId);
    bool8 completed = Achievement_IsCompleted(achievementId);
    bool8 masked = info->hidden && !completed;

    DrawHeaderText(sTierNames[info->tier]);

    FillWindowPixelBuffer(WIN_LIST, PIXEL_FILL(1));
    AddTextPrinterParameterized(WIN_LIST, FONT_NORMAL, masked ? sText_HiddenName : info->name, 8, 1, TEXT_SKIP_DRAW, NULL);
    StringCopy(gStringVar1, masked ? sText_HiddenDescription : info->description);
    StripLineBreaks(gStringVar1);
    BreakStringAutomatic(gStringVar1, ACHIEVEMENTS_DESC_MAX_WIDTH, 3, FONT_NORMAL, HIDE_SCROLL_PROMPT);
    AddTextPrinterParameterized(WIN_LIST, FONT_NORMAL, gStringVar1, 8, 17, TEXT_SKIP_DRAW, NULL);
    CopyWindowToVram(WIN_LIST, COPYWIN_GFX);

    ConvertIntToDecimalStringN(gStringVar1, info->points, STR_CONV_MODE_LEFT_ALIGN, 5);
    StringExpandPlaceholders(gStringVar4, sText_RewardFormat);

    FillWindowPixelBuffer(WIN_DESCRIPTION, PIXEL_FILL(1));
    AddTextPrinterParameterized(WIN_DESCRIPTION, FONT_NORMAL, gStringVar4, 8, 1, TEXT_SKIP_DRAW, NULL);
    AddTextPrinterParameterized(WIN_DESCRIPTION, FONT_NORMAL, completed ? sText_StatusCompleted : sText_StatusIncomplete, 8, 17, TEXT_SKIP_DRAW, NULL);
    CopyWindowToVram(WIN_DESCRIPTION, COPYWIN_GFX);
}

static void Task_Detail_ProcessInput(u8 taskId)
{
    if (JOY_NEW(A_BUTTON | B_BUTTON))
    {
        PlaySE(SE_SELECT);
        EnterListLevel(taskId, sAchievementsMenu.selectedTier);
        gTasks[taskId].func = Task_List_ProcessInput;
    }
}

// ---- Shared ----------------------------------------------------------------

static void DestroyCurrentAchievementsList(u8 taskId)
{
    DestroyListMenuTask(gTasks[taskId].tListTaskId, NULL, NULL);
    RemoveScrollIndicatorArrowPair(gTasks[taskId].tScrollArrowTaskId);
}

static void DrawHeaderText(const u8 *title)
{
    s32 hintX = GetStringRightAlignXOffset(FONT_NARROW, sText_ControlHint, 198);

    FillWindowPixelBuffer(WIN_HEADER, PIXEL_FILL(1));
    AddTextPrinterParameterized(WIN_HEADER, FONT_NORMAL, title, 8, 1, TEXT_SKIP_DRAW, NULL);
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

    // Achievement list frame
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
