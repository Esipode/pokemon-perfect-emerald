#include "global.h"
#include "new_game_settings_menu.h"
#include "bg.h"
#include "event_data.h"
#include "gpu_regs.h"
#include "international_string_util.h"
#include "list_menu.h"
#include "main.h"
#include "main_menu.h"
#include "menu.h"
#include "palette.h"
#include "scanline_effect.h"
#include "sound.h"
#include "sprite.h"
#include "task.h"
#include "text.h"
#include "text_window.h"
#include "window.h"
#include "constants/difficulty.h"
#include "constants/flags.h"
#include "constants/rgb.h"
#include "constants/songs.h"

// A one-time, scrollable ruleset menu shown right before the Professor Birch intro
// when a New Game is started. Values chosen here are held in EWRAM (not save data)
// until ApplyPendingNewGameSettings() commits them - see new_game.c for why.
struct NewGameSettings gPendingNewGameSettings;

enum
{
    SETTING_NUZLOCKE,
    SETTING_DIFFICULTY,
    SETTING_RANDOMIZE_SPECIES,
    SETTING_RANDOMIZE_TYPES,
    SETTING_RANDOMIZE_MOVES,
    SETTING_STAT_EDITOR,
    SETTING_DEBUG,
    SETTING_LEVEL_CAP,
    SETTING_COUNT,
};

enum
{
    WIN_HEADER,
    WIN_LIST,
    WIN_DESCRIPTION,
};

#define SETTINGS_MAX_SHOWED 4

#define tListTaskId       data[0]
#define tScrollArrowTaskId data[1]

#define TAG_SETTINGS_SCROLL_ARROWS 6000

#define SETTINGS_VALUE_RIGHT_X 190
#define SETTINGS_ARROW_X       200
#define SETTINGS_ARROW_TOP_Y   36
#define SETTINGS_ARROW_BOTTOM_Y 100

EWRAM_DATA static struct
{
    u16 scrollOffset;
    u16 selectedRow;
} sSettingsScroll = {0};

static void Task_SettingsMenuFadeIn(u8 taskId);
static void Task_SettingsMenuProcessInput(u8 taskId);
static void Task_SettingsMenuConfirm(u8 taskId);
static void Task_SettingsMenuCancel(u8 taskId);
static void SettingsMenu_MoveCursorCallback(s32 itemIndex, bool8 onInit, struct ListMenu *list);
static void SettingsMenu_ItemPrintCallback(u8 windowId, u32 settingId, u8 y);
static void HandleValueChange(u8 settingId, bool8 rightPressed);
static const u8 *GetSettingValueText(u8 settingId);
static void PrintSettingDescription(s32 settingId);
static void DrawHeaderText(void);
static void DrawBgWindowFrames(void);

static const u8 sText_GameSettingsTitle[] = _("GAME SETTINGS");
static const u8 sText_ControlHint[]       = _("{A_BUTTON}BEGIN {B_BUTTON}BACK {LEFT_ARROW}{RIGHT_ARROW}CHANGE");

static const u8 sText_Off[] = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}OFF");
static const u8 sText_On[]  = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}ON");

static const u8 *const sDifficultyTexts[] =
{
    [DIFFICULTY_EASY]   = COMPOUND_STRING("{COLOR GREEN}{SHADOW LIGHT_GREEN}EASY"),
    [DIFFICULTY_NORMAL] = COMPOUND_STRING("{COLOR GREEN}{SHADOW LIGHT_GREEN}NORMAL"),
    [DIFFICULTY_HARD]   = COMPOUND_STRING("{COLOR GREEN}{SHADOW LIGHT_GREEN}HARD"),
};

static const u8 *const sSettingDescriptions[SETTING_COUNT] =
{
    [SETTING_NUZLOCKE]          = COMPOUND_STRING(
                                       "Pokémon are lost when fainted.\n"
                                       "Can only catch one Pokémon per route."),
    [SETTING_DIFFICULTY]        = COMPOUND_STRING(
                                       "Changes encountered Pokémon levels\n"
                                       "and Trainer AI complexity."),
    [SETTING_RANDOMIZE_SPECIES] = COMPOUND_STRING("Pokémon species are randomized."),
    [SETTING_RANDOMIZE_TYPES]   = COMPOUND_STRING("Pokémon types are randomized."),
    [SETTING_RANDOMIZE_MOVES]   = COMPOUND_STRING("Pokémon movesets are randomized."),
    [SETTING_STAT_EDITOR]       = COMPOUND_STRING("Change IV/EV values of your Pokémon."),
    [SETTING_LEVEL_CAP]         = COMPOUND_STRING("Prevents over-levelling your Pokémon."),
    [SETTING_DEBUG]             = COMPOUND_STRING(
                                       "Allows opening the Debug Menu\n"
                                       "using {R_BUTTON}+{START_BUTTON}."),
};

static const struct ListMenuItem sSettingsListItems[SETTING_COUNT] =
{
    [SETTING_NUZLOCKE]          = {COMPOUND_STRING("NUZLOCKE MODE"),    SETTING_NUZLOCKE},
    [SETTING_DIFFICULTY]        = {COMPOUND_STRING("DIFFICULTY"),       SETTING_DIFFICULTY},
    [SETTING_RANDOMIZE_SPECIES] = {COMPOUND_STRING("RANDOMIZE SPECIES"),SETTING_RANDOMIZE_SPECIES},
    [SETTING_RANDOMIZE_TYPES]   = {COMPOUND_STRING("RANDOMIZE TYPES"),  SETTING_RANDOMIZE_TYPES},
    [SETTING_RANDOMIZE_MOVES]   = {COMPOUND_STRING("RANDOMIZE MOVES"),  SETTING_RANDOMIZE_MOVES},
    [SETTING_STAT_EDITOR]       = {COMPOUND_STRING("STAT EDITOR"),      SETTING_STAT_EDITOR},
    [SETTING_LEVEL_CAP]         = {COMPOUND_STRING("LEVEL CAP"),        SETTING_LEVEL_CAP},
    [SETTING_DEBUG]             = {COMPOUND_STRING("DEBUG MODE"),       SETTING_DEBUG},
};

static const struct WindowTemplate sSettingsMenuWinTemplates[] =
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

static const struct BgTemplate sSettingsMenuBgTemplates[] =
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

static const u16 sSettingsMenuBg_Pal[] = {RGB(17, 18, 31)};
static const u16 sSettingsMenuText_Pal[] = INCGFX_U16("graphics/interface/option_menu_text.pal", ".gbapal");

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

void CB2_InitNewGameSettingsMenu(void)
{
    u8 taskId;
    switch (gMain.state)
    {
    default:
    case 0:
        SetVBlankCallback(NULL);
        gPendingNewGameSettings.difficulty = DIFFICULTY_NORMAL;
        gPendingNewGameSettings.nuzlockeEnabled = FALSE;
        gPendingNewGameSettings.randomizeSpecies = FALSE;
        gPendingNewGameSettings.randomizeTypes = FALSE;
        gPendingNewGameSettings.randomizeMoves = FALSE;
        gPendingNewGameSettings.allowStatEditor = FALSE;
        gPendingNewGameSettings.levelCapOff = FALSE;
        gPendingNewGameSettings.debugMode = FALSE;
        sSettingsScroll.scrollOffset = 0;
        sSettingsScroll.selectedRow = 0;
        gMain.state++;
        break;
    case 1:
        DmaClearLarge16(3, (void *)(VRAM), VRAM_SIZE, 0x1000);
        DmaClear32(3, OAM, OAM_SIZE);
        DmaClear16(3, PLTT, PLTT_SIZE);
        SetGpuReg(REG_OFFSET_DISPCNT, 0);
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(0, sSettingsMenuBgTemplates, ARRAY_COUNT(sSettingsMenuBgTemplates));
        ChangeBgX(0, 0, BG_COORD_SET);
        ChangeBgY(0, 0, BG_COORD_SET);
        ChangeBgX(1, 0, BG_COORD_SET);
        ChangeBgY(1, 0, BG_COORD_SET);
        InitWindows(sSettingsMenuWinTemplates);
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
        LoadPalette(sSettingsMenuBg_Pal, BG_PLTT_ID(0), sizeof(sSettingsMenuBg_Pal));
        LoadPalette(GetWindowFrameTilesPal(gSaveBlock2Ptr->optionsWindowFrameType)->pal, BG_PLTT_ID(7), PLTT_SIZE_4BPP);
        gMain.state++;
        break;
    case 5:
        LoadPalette(sSettingsMenuText_Pal, BG_PLTT_ID(1), sizeof(sSettingsMenuText_Pal));
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

        template.items = sSettingsListItems;
        template.moveCursorFunc = SettingsMenu_MoveCursorCallback;
        template.itemPrintFunc = SettingsMenu_ItemPrintCallback;
        template.totalItems = SETTING_COUNT;
        template.maxShowed = SETTINGS_MAX_SHOWED;
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

        taskId = CreateTask(Task_SettingsMenuFadeIn, 0);
        gTasks[taskId].tListTaskId = ListMenuInit(&template, sSettingsScroll.scrollOffset, sSettingsScroll.selectedRow);
        gTasks[taskId].tScrollArrowTaskId = AddScrollIndicatorArrowPairParameterized(
            SCROLL_ARROW_UP, SETTINGS_ARROW_X, SETTINGS_ARROW_TOP_Y, SETTINGS_ARROW_BOTTOM_Y,
            SETTING_COUNT - SETTINGS_MAX_SHOWED, TAG_SETTINGS_SCROLL_ARROWS, TAG_SETTINGS_SCROLL_ARROWS,
            &sSettingsScroll.scrollOffset);
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

static void Task_SettingsMenuFadeIn(u8 taskId)
{
    if (!gPaletteFade.active)
        gTasks[taskId].func = Task_SettingsMenuProcessInput;
}

static void Task_SettingsMenuProcessInput(u8 taskId)
{
    s32 itemId = ListMenu_ProcessInput(gTasks[taskId].tListTaskId);
    ListMenuGetScrollAndRow(gTasks[taskId].tListTaskId, &sSettingsScroll.scrollOffset, &sSettingsScroll.selectedRow);

    switch (itemId)
    {
    case LIST_NOTHING_CHOSEN:
        if (JOY_NEW(DPAD_LEFT | DPAD_RIGHT))
        {
            PlaySE(SE_SELECT);
            HandleValueChange(sSettingsScroll.scrollOffset + sSettingsScroll.selectedRow, JOY_NEW(DPAD_RIGHT));
            RedrawListMenu(gTasks[taskId].tListTaskId);
            CopyWindowToVram(WIN_LIST, COPYWIN_GFX);
        }
        break;
    case LIST_CANCEL:
        PlaySE(SE_SELECT);
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        gTasks[taskId].func = Task_SettingsMenuCancel;
        break;
    default:
        PlaySE(SE_SELECT);
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        gTasks[taskId].func = Task_SettingsMenuConfirm;
        break;
    }
}

static void Task_SettingsMenuConfirm(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        DestroyListMenuTask(gTasks[taskId].tListTaskId, NULL, NULL);
        RemoveScrollIndicatorArrowPair(gTasks[taskId].tScrollArrowTaskId);
        DestroyTask(taskId);
        FreeAllWindowBuffers();
        SetMainCallback2(CB2_NewGameBirchSpeech_FromNewMainMenu);
    }
}

static void Task_SettingsMenuCancel(u8 taskId)
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

static void SettingsMenu_MoveCursorCallback(s32 itemIndex, bool8 onInit, struct ListMenu *list)
{
    if (!onInit)
        PlaySE(SE_SELECT);
    PrintSettingDescription(itemIndex);
}

static void SettingsMenu_ItemPrintCallback(u8 windowId, u32 settingId, u8 y)
{
    const u8 *text = GetSettingValueText(settingId);
    s32 width = GetStringWidth(FONT_NORMAL, text, 0);

    AddTextPrinterParameterized(windowId, FONT_NORMAL, text, SETTINGS_VALUE_RIGHT_X - width, y, TEXT_SKIP_DRAW, NULL);
}

static void HandleValueChange(u8 settingId, bool8 rightPressed)
{
    switch (settingId)
    {
    case SETTING_NUZLOCKE:
        gPendingNewGameSettings.nuzlockeEnabled ^= 1;
        break;
    case SETTING_RANDOMIZE_SPECIES:
        gPendingNewGameSettings.randomizeSpecies ^= 1;
        break;
    case SETTING_RANDOMIZE_TYPES:
        gPendingNewGameSettings.randomizeTypes ^= 1;
        break;
    case SETTING_RANDOMIZE_MOVES:
        gPendingNewGameSettings.randomizeMoves ^= 1;
        break;
    case SETTING_STAT_EDITOR:
        gPendingNewGameSettings.allowStatEditor ^= 1;
        break;
    case SETTING_DEBUG:
        gPendingNewGameSettings.debugMode ^= 1;
        break;
    case SETTING_LEVEL_CAP:
        gPendingNewGameSettings.levelCapOff ^= 1;
        break;
    case SETTING_DIFFICULTY:
        if (rightPressed)
        {
            if (gPendingNewGameSettings.difficulty < DIFFICULTY_HARD)
                gPendingNewGameSettings.difficulty++;
            else
                gPendingNewGameSettings.difficulty = DIFFICULTY_EASY;
        }
        else
        {
            if (gPendingNewGameSettings.difficulty > DIFFICULTY_EASY)
                gPendingNewGameSettings.difficulty--;
            else
                gPendingNewGameSettings.difficulty = DIFFICULTY_HARD;
        }
        break;
    }
}

static const u8 *GetSettingValueText(u8 settingId)
{
    switch (settingId)
    {
    case SETTING_NUZLOCKE:          return gPendingNewGameSettings.nuzlockeEnabled ? sText_On : sText_Off;
    case SETTING_DIFFICULTY:        return sDifficultyTexts[gPendingNewGameSettings.difficulty];
    case SETTING_RANDOMIZE_SPECIES: return gPendingNewGameSettings.randomizeSpecies ? sText_On : sText_Off;
    case SETTING_RANDOMIZE_TYPES:   return gPendingNewGameSettings.randomizeTypes ? sText_On : sText_Off;
    case SETTING_RANDOMIZE_MOVES:   return gPendingNewGameSettings.randomizeMoves ? sText_On : sText_Off;
    case SETTING_STAT_EDITOR:       return gPendingNewGameSettings.allowStatEditor ? sText_On : sText_Off;
    case SETTING_LEVEL_CAP:         return gPendingNewGameSettings.levelCapOff ? sText_Off : sText_On;
    case SETTING_DEBUG:             return gPendingNewGameSettings.debugMode ? sText_On : sText_Off;
    default:                        return sText_Off;
    }
}

static void PrintSettingDescription(s32 settingId)
{
    FillWindowPixelBuffer(WIN_DESCRIPTION, PIXEL_FILL(1));
    if (settingId >= 0 && settingId < SETTING_COUNT)
        AddTextPrinterParameterized(WIN_DESCRIPTION, FONT_NORMAL, sSettingDescriptions[settingId], 8, 1, TEXT_SKIP_DRAW, NULL);
    CopyWindowToVram(WIN_DESCRIPTION, COPYWIN_GFX);
}

static void DrawHeaderText(void)
{
    s32 hintX = GetStringRightAlignXOffset(FONT_NARROW, sText_ControlHint, 198);

    FillWindowPixelBuffer(WIN_HEADER, PIXEL_FILL(1));
    AddTextPrinterParameterized(WIN_HEADER, FONT_NORMAL, sText_GameSettingsTitle, 8, 1, TEXT_SKIP_DRAW, NULL);
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

    // Settings list frame
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

void ApplyPendingNewGameSettings(void)
{
    gSaveBlock1Ptr->difficulty = gPendingNewGameSettings.difficulty;
    gSaveBlock1Ptr->nuzlockeModeEnabled = gPendingNewGameSettings.nuzlockeEnabled;
    gPendingNewGameSettings.randomizeSpecies ? FlagSet(FLAG_RANDOMIZE_MON)     : FlagClear(FLAG_RANDOMIZE_MON);
    gPendingNewGameSettings.randomizeTypes   ? FlagSet(FLAG_RANDOMIZE_TYPE)    : FlagClear(FLAG_RANDOMIZE_TYPE);
    gPendingNewGameSettings.randomizeMoves   ? FlagSet(FLAG_RANDOMIZE_MOVES)   : FlagClear(FLAG_RANDOMIZE_MOVES);
    gPendingNewGameSettings.levelCapOff      ? FlagSet(FLAG_LEVEL_CAP_OFF)     : FlagClear(FLAG_LEVEL_CAP_OFF);
    gPendingNewGameSettings.allowStatEditor  ? FlagSet(FLAG_ALLOW_STAT_EDITOR) : FlagClear(FLAG_ALLOW_STAT_EDITOR);
    gPendingNewGameSettings.debugMode        ? FlagSet(FLAG_DEBUG)             : FlagClear(FLAG_DEBUG);
    if (gPendingNewGameSettings.debugMode)
        gSaveBlock1Ptr->achievementsBlocked = TRUE;
}
