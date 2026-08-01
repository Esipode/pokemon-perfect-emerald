#include "global.h"
#include "achievements.h"
#include "achievements_menu.h"
#include "bg.h"
#include "gpu_regs.h"
#include "international_string_util.h"
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
// ListMenu + scroll arrows, DrawBgWindowFrames chrome. Deliberately just a
// single flat, ID-ordered list of every real achievement for now:
//   - no tier grouping into the three-level TIER SELECT / LIST / DETAIL flow
//     (design doc §3.2, Stage 3.2)
//   - no hidden-achievement "???" masking (design doc §17, also Stage 3.2)
//   - no Start Menu entry point yet (Stage 3.3) -- nothing calls
//     CB2_InitAchievementsMenu() until then.

enum
{
    WIN_HEADER,
    WIN_LIST,
    WIN_DESCRIPTION,
};

#define ACHIEVEMENTS_MENU_MAX_SHOWED 4
#define ACHIEVEMENTS_MENU_ITEM_COUNT (ACHIEVEMENTS_COUNT - 1) // excludes ACHIEVEMENT_NONE

#define tListTaskId        data[0]
#define tScrollArrowTaskId data[1]

#define TAG_ACHIEVEMENTS_SCROLL_ARROWS 6001

#define ACHIEVEMENTS_POINTS_RIGHT_X 190
#define ACHIEVEMENTS_ARROW_X        200
#define ACHIEVEMENTS_ARROW_TOP_Y    36
#define ACHIEVEMENTS_ARROW_BOTTOM_Y 100

// "[x] " / "[ ] " prefix plus the name itself; the name's own encoded size is
// already capped at ACHIEVEMENT_NAME_LENGTH (including its terminator) by
// ACHIEVEMENT_NAME(), so this leaves generous headroom rather than computing
// the exact minimum.
#define ACHIEVEMENTS_LIST_NAME_BUFFER_SIZE (ACHIEVEMENT_NAME_LENGTH + 8)

EWRAM_DATA static u8 sAchievementsListNameBuffers[ACHIEVEMENTS_MENU_ITEM_COUNT][ACHIEVEMENTS_LIST_NAME_BUFFER_SIZE] = {0};
EWRAM_DATA static struct ListMenuItem sAchievementsListItems[ACHIEVEMENTS_MENU_ITEM_COUNT] = {0};

EWRAM_DATA static struct
{
    u16 scrollOffset;
    u16 selectedRow;
} sAchievementsScroll = {0};

static void Task_AchievementsMenuFadeIn(u8 taskId);
static void Task_AchievementsMenuProcessInput(u8 taskId);
static void Task_AchievementsMenuCancel(u8 taskId);
static void AchievementsMenu_MoveCursorCallback(s32 itemIndex, bool8 onInit, struct ListMenu *list);
static void AchievementsMenu_ItemPrintCallback(u8 windowId, u32 achievementId, u8 y);
static void BuildAchievementsListItems(void);
static void PrintAchievementDescription(s32 achievementId);
static void DrawHeaderText(void);
static void DrawBgWindowFrames(void);

static const u8 sText_AchievementsTitle[] = _("ACHIEVEMENTS");
static const u8 sText_ControlHint[]       = _("{B_BUTTON}BACK");
// '[' and ']' aren't in charmap.txt -- use the existing filled/hollow circle
// glyphs instead of literal brackets.
static const u8 sText_CompletedPrefix[]   = _("{CIRCLE_DOT} ");
static const u8 sText_IncompletePrefix[]  = _("{CIRCLE_HOLLOW} ");

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
        sAchievementsScroll.scrollOffset = 0;
        sAchievementsScroll.selectedRow = 0;
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
        DrawHeaderText();
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
    {
        struct ListMenuTemplate template = {0};

        BuildAchievementsListItems();

        template.items = sAchievementsListItems;
        template.moveCursorFunc = AchievementsMenu_MoveCursorCallback;
        template.itemPrintFunc = AchievementsMenu_ItemPrintCallback;
        template.totalItems = ACHIEVEMENTS_MENU_ITEM_COUNT;
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

        taskId = CreateTask(Task_AchievementsMenuFadeIn, 0);
        gTasks[taskId].tListTaskId = ListMenuInit(&template, sAchievementsScroll.scrollOffset, sAchievementsScroll.selectedRow);
        gTasks[taskId].tScrollArrowTaskId = AddScrollIndicatorArrowPairParameterized(
            SCROLL_ARROW_UP, ACHIEVEMENTS_ARROW_X, ACHIEVEMENTS_ARROW_TOP_Y, ACHIEVEMENTS_ARROW_BOTTOM_Y,
            ACHIEVEMENTS_MENU_ITEM_COUNT - ACHIEVEMENTS_MENU_MAX_SHOWED, TAG_ACHIEVEMENTS_SCROLL_ARROWS, TAG_ACHIEVEMENTS_SCROLL_ARROWS,
            &sAchievementsScroll.scrollOffset);
        gMain.state++;
        break;
    }
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
        gTasks[taskId].func = Task_AchievementsMenuProcessInput;
}

static void Task_AchievementsMenuProcessInput(u8 taskId)
{
    s32 itemId = ListMenu_ProcessInput(gTasks[taskId].tListTaskId);
    ListMenuGetScrollAndRow(gTasks[taskId].tListTaskId, &sAchievementsScroll.scrollOffset, &sAchievementsScroll.selectedRow);

    switch (itemId)
    {
    case LIST_NOTHING_CHOSEN:
        break;
    case LIST_CANCEL:
        PlaySE(SE_SELECT);
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        gTasks[taskId].func = Task_AchievementsMenuCancel;
        break;
    default:
        // Stage 3.2 hook: selecting an achievement will swap to the DETAIL
        // screen here. No detail screen exists yet, so this is a no-op.
        PlaySE(SE_SELECT);
        break;
    }
}

static void Task_AchievementsMenuCancel(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        DestroyListMenuTask(gTasks[taskId].tListTaskId, NULL, NULL);
        RemoveScrollIndicatorArrowPair(gTasks[taskId].tScrollArrowTaskId);
        DestroyTask(taskId);
        FreeAllWindowBuffers();
        SetMainCallback2(gMain.savedCallback);
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

// Builds the flat, ID-ordered item list (skips ACHIEVEMENT_NONE) and bakes
// the completion checkbox into each row's label text. Achievement completion
// can't change while this menu is open, so this only needs to run once, at
// entry, rather than being recomputed per redraw.
static void BuildAchievementsListItems(void)
{
    u32 id, index = 0;

    for (id = ACHIEVEMENT_NONE + 1; id < ACHIEVEMENTS_COUNT; id++, index++)
    {
        u8 *buffer = sAchievementsListNameBuffers[index];

        StringCopy(buffer, Achievement_IsCompleted(id) ? sText_CompletedPrefix : sText_IncompletePrefix);
        StringAppend(buffer, Achievement_GetInfo(id)->name);

        sAchievementsListItems[index].name = buffer;
        sAchievementsListItems[index].id = id;
    }
}

static void PrintAchievementDescription(s32 achievementId)
{
    FillWindowPixelBuffer(WIN_DESCRIPTION, PIXEL_FILL(1));
    if (achievementId >= ACHIEVEMENT_NONE + 1 && achievementId < ACHIEVEMENTS_COUNT)
        AddTextPrinterParameterized(WIN_DESCRIPTION, FONT_NORMAL, Achievement_GetInfo(achievementId)->description, 8, 1, TEXT_SKIP_DRAW, NULL);
    CopyWindowToVram(WIN_DESCRIPTION, COPYWIN_GFX);
}

static void DrawHeaderText(void)
{
    s32 hintX = GetStringRightAlignXOffset(FONT_NARROW, sText_ControlHint, 198);

    FillWindowPixelBuffer(WIN_HEADER, PIXEL_FILL(1));
    AddTextPrinterParameterized(WIN_HEADER, FONT_NORMAL, sText_AchievementsTitle, 8, 1, TEXT_SKIP_DRAW, NULL);
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
