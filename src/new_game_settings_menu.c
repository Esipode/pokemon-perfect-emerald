#include "global.h"
#include "new_game_settings_menu.h"
#include "battle_main.h"
#include "bg.h"
#include "draft_mode.h"
#include "event_data.h"
#include "gpu_regs.h"
#include "international_string_util.h"
#include "limited_party.h"
#include "list_menu.h"
#include "main.h"
#include "main_menu.h"
#include "menu.h"
#include "mono_gen.h"
#include "mono_type.h"
#include "palette.h"
#include "scanline_effect.h"
#include "sound.h"
#include "sprite.h"
#include "string_util.h"
#include "task.h"
#include "text.h"
#include "text_window.h"
#include "window.h"
#include "constants/difficulty.h"
#include "constants/flags.h"
#include "constants/pokemon.h"
#include "constants/rgb.h"
#include "constants/songs.h"

// A one-time, scrollable ruleset menu shown right before the Professor Birch intro
// when a New Game is started. Values chosen here are held in EWRAM (not save data)
// until ApplyPendingNewGameSettings() commits them - see new_game.c for why.
struct NewGameSettings gPendingNewGameSettings;

enum
{
    SETTING_NUZLOCKE,
    SETTING_DRAFT,
    SETTING_MONO_TYPE,
    SETTING_MONO_GEN,
    SETTING_LIMITED_PARTY,
    SETTING_DIFFICULTY,
    SETTING_RANDOMIZE_SPECIES,
    SETTING_RANDOMIZE_TYPES,
    SETTING_RANDOMIZE_MOVES,
    SETTING_STAT_EDITOR,
    SETTING_LEVEL_CAP,
    SETTING_DEBUG,
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

// Nuzlocke and Draft (draft_mode.h) are mutually exclusive, so the settings
// list can't just be sSettingsListItems handed straight to ListMenuInit --
// whichever one is off has to disappear from the list entirely rather than
// merely being unselectable, since the two modes share the same per-area
// save bytes and both being "on" makes no sense. These hold the filtered
// copy actually shown, plus a row -> setting id map (row index no longer
// equals setting id once a row is hidden).
static EWRAM_DATA struct ListMenuItem sVisibleSettings[SETTING_COUNT] = {0};
static EWRAM_DATA u8 sVisibleSettingIds[SETTING_COUNT] = {0};
static EWRAM_DATA u8 sVisibleSettingCount = 0;

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
static void BuildVisibleSettings(void);
static void RebuildVisibleSettingsList(u8 taskId);

static const u8 sText_GameSettingsTitle[] = _("GAME SETTINGS");
static const u8 sText_ControlHint[]       = _("{A_BUTTON}BEGIN {B_BUTTON}BACK {LEFT_ARROW}{RIGHT_ARROW}CHANGE");

static const u8 sText_Off[] = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}OFF");
static const u8 sText_On[]  = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}ON");

// Prefix for values that have to be composed at runtime (i.e. the mono type
// name, which comes from gTypesInfo rather than being a fixed literal).
static const u8 sText_ValueColorPrefix[] = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}");

static const u8 *const sDifficultyTexts[] =
{
    [DIFFICULTY_EASY]   = COMPOUND_STRING("{COLOR GREEN}{SHADOW LIGHT_GREEN}EASY"),
    [DIFFICULTY_NORMAL] = COMPOUND_STRING("{COLOR GREEN}{SHADOW LIGHT_GREEN}NORMAL"),
    [DIFFICULTY_HARD]   = COMPOUND_STRING("{COLOR GREEN}{SHADOW LIGHT_GREEN}HARD"),
};

// Indexed 1-9 (index 0 unused; "OFF" is sText_Off) - unlike the mono type
// name, the generation label is a fixed literal, so no runtime composition
// or scratch buffer is needed.
static const u8 *const sMonoGenTexts[MONO_GEN_COUNT + 1] =
{
    [1] = COMPOUND_STRING("{COLOR GREEN}{SHADOW LIGHT_GREEN}GEN 1"),
    [2] = COMPOUND_STRING("{COLOR GREEN}{SHADOW LIGHT_GREEN}GEN 2"),
    [3] = COMPOUND_STRING("{COLOR GREEN}{SHADOW LIGHT_GREEN}GEN 3"),
    [4] = COMPOUND_STRING("{COLOR GREEN}{SHADOW LIGHT_GREEN}GEN 4"),
    [5] = COMPOUND_STRING("{COLOR GREEN}{SHADOW LIGHT_GREEN}GEN 5"),
    [6] = COMPOUND_STRING("{COLOR GREEN}{SHADOW LIGHT_GREEN}GEN 6"),
    [7] = COMPOUND_STRING("{COLOR GREEN}{SHADOW LIGHT_GREEN}GEN 7"),
    [8] = COMPOUND_STRING("{COLOR GREEN}{SHADOW LIGHT_GREEN}GEN 8"),
    [9] = COMPOUND_STRING("{COLOR GREEN}{SHADOW LIGHT_GREEN}GEN 9"),
};

static const u8 *const sSettingDescriptions[SETTING_COUNT] =
{
    [SETTING_NUZLOCKE]          = COMPOUND_STRING(
                                       "Pokémon are lost when fainted.\n"
                                       "Can only catch one Pokémon per route."),
    [SETTING_DRAFT]             = COMPOUND_STRING(
                                       "Draft one Pokémon per area. No catching,\n"
                                       "no PC. Choices are permanent."),
    [SETTING_MONO_TYPE]         = COMPOUND_STRING(
                                       "Choose a starter of this type and only\n"
                                       "obtain Pokémon of this type."),
    [SETTING_MONO_GEN]          = COMPOUND_STRING(
                                       "Choose a starter of this Gen and only\n"
                                       "obtain Pokémon from this Gen."),
    [SETTING_LIMITED_PARTY]     = COMPOUND_STRING(
                                       "Party starts at 3 Pokémon. Extra\n"
                                       "slots are earned from Gym Badges."),
    [SETTING_DIFFICULTY]        = COMPOUND_STRING(
                                       "Changes encountered Pokémon levels\n"
                                       "and Trainer AI complexity."),
    [SETTING_RANDOMIZE_SPECIES] = COMPOUND_STRING("Pokémon species are randomized."),
    [SETTING_RANDOMIZE_TYPES]   = COMPOUND_STRING("Pokémon types are randomized."),
    [SETTING_RANDOMIZE_MOVES]   = COMPOUND_STRING("Pokémon movesets are randomized."),
    [SETTING_STAT_EDITOR]       = COMPOUND_STRING(
                                       "{COLOR RED}{SHADOW LIGHT_RED}(ON DISABLES ACHIEVEMENTS){COLOR DARK_GRAY}{SHADOW LIGHT_GRAY}\n"
                                       "Change IV/EV values of your Pokémon."),
    [SETTING_LEVEL_CAP]         = COMPOUND_STRING(
                                       "{COLOR RED}{SHADOW LIGHT_RED}(OFF DISABLES ACHIEVEMENTS){COLOR DARK_GRAY}{SHADOW LIGHT_GRAY}\n"
                                       "Prevents over-levelling your Pokémon."),
    // Body condensed to a single line (from the original two) to make room for
    // the achievements-disabled line above it -- WIN_DESCRIPTION only fits 2
    // lines of FONT_NORMAL text (see sSettingsMenuWinTemplates[WIN_DESCRIPTION]).
    [SETTING_DEBUG]             = COMPOUND_STRING(
                                       "{COLOR RED}{SHADOW LIGHT_RED}(DISABLES ACHIEVEMENTS){COLOR DARK_GRAY}{SHADOW LIGHT_GRAY}\n"
                                       "Debug Menu: {R_BUTTON}+{START_BUTTON}"),
};

static const struct ListMenuItem sSettingsListItems[SETTING_COUNT] =
{
    [SETTING_NUZLOCKE]          = {COMPOUND_STRING("NUZLOCKE MODE"),    SETTING_NUZLOCKE},
    [SETTING_DRAFT]             = {COMPOUND_STRING("DRAFT MODE"),       SETTING_DRAFT},
    [SETTING_MONO_TYPE]         = {COMPOUND_STRING("MONO TYPE"),        SETTING_MONO_TYPE},
    [SETTING_MONO_GEN]          = {COMPOUND_STRING("MONO GEN"),         SETTING_MONO_GEN},
    [SETTING_LIMITED_PARTY]     = {COMPOUND_STRING("LIMITED PARTY"),    SETTING_LIMITED_PARTY},
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
        gPendingNewGameSettings.draftMode = FALSE;
        gPendingNewGameSettings.monoType = TYPE_NONE;
        gPendingNewGameSettings.monoGen = 0;
        gPendingNewGameSettings.limitedParty = FALSE;
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

        BuildVisibleSettings();

        template.items = sVisibleSettings;
        template.moveCursorFunc = SettingsMenu_MoveCursorCallback;
        template.itemPrintFunc = SettingsMenu_ItemPrintCallback;
        template.totalItems = sVisibleSettingCount;
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
            sVisibleSettingCount - SETTINGS_MAX_SHOWED, TAG_SETTINGS_SCROLL_ARROWS, TAG_SETTINGS_SCROLL_ARROWS,
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
            u8 settingId = sVisibleSettingIds[sSettingsScroll.scrollOffset + sSettingsScroll.selectedRow];

            PlaySE(SE_SELECT);
            HandleValueChange(settingId, JOY_NEW(DPAD_RIGHT));

            // Nuzlocke and Draft are mutually exclusive (draft_mode.h) and
            // hide each other's row -- toggling either one can change which
            // rows are visible, so the list has to be torn down and rebuilt
            // rather than just redrawn. Every other setting only ever
            // changes its own value text, which RedrawListMenu handles.
            if (settingId == SETTING_NUZLOCKE || settingId == SETTING_DRAFT)
            {
                RebuildVisibleSettingsList(taskId);
            }
            else
            {
                RedrawListMenu(gTasks[taskId].tListTaskId);
                CopyWindowToVram(WIN_LIST, COPYWIN_GFX);
            }
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
    case SETTING_DRAFT:
        gPendingNewGameSettings.draftMode ^= 1;
        break;
    case SETTING_MONO_TYPE:
        gPendingNewGameSettings.monoType = MonoType_CycleType(gPendingNewGameSettings.monoType, rightPressed);
        break;
    case SETTING_MONO_GEN:
        gPendingNewGameSettings.monoGen = MonoGen_CycleGen(gPendingNewGameSettings.monoGen, rightPressed);
        break;
    case SETTING_LIMITED_PARTY:
        gPendingNewGameSettings.limitedParty ^= 1;
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
    static u8 sMonoTypeValueText[32];

    switch (settingId)
    {
    case SETTING_NUZLOCKE:          return gPendingNewGameSettings.nuzlockeEnabled ? sText_On : sText_Off;
    case SETTING_DRAFT:             return gPendingNewGameSettings.draftMode ? sText_On : sText_Off;
    case SETTING_MONO_TYPE:
        if (gPendingNewGameSettings.monoType == TYPE_NONE)
            return sText_Off;
        StringCopy(sMonoTypeValueText, sText_ValueColorPrefix);
        StringAppend(sMonoTypeValueText, gTypesInfo[gPendingNewGameSettings.monoType].name);
        return sMonoTypeValueText;
    case SETTING_MONO_GEN:
        return gPendingNewGameSettings.monoGen == 0 ? sText_Off : sMonoGenTexts[gPendingNewGameSettings.monoGen];
    case SETTING_LIMITED_PARTY:     return gPendingNewGameSettings.limitedParty ? sText_On : sText_Off;
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

// Filters sSettingsListItems down into sVisibleSettings, skipping
// SETTING_DRAFT while Nuzlocke is enabled and SETTING_NUZLOCKE while Draft
// mode is enabled. Each item's .id field is copied as-is (it's already the
// setting's real enum value - see sSettingsListItems), so
// SettingsMenu_ItemPrintCallback/SettingsMenu_MoveCursorCallback, which are
// handed that id rather than the row, keep working untouched. What does
// change is the row -> id mapping used to interpret D-pad input outside the
// list menu (Task_SettingsMenuProcessInput), which this rebuilds into
// sVisibleSettingIds.
static void BuildVisibleSettings(void)
{
    u8 i;

    sVisibleSettingCount = 0;
    for (i = 0; i < SETTING_COUNT; i++)
    {
        if (i == SETTING_DRAFT && gPendingNewGameSettings.nuzlockeEnabled)
            continue;
        if (i == SETTING_NUZLOCKE && gPendingNewGameSettings.draftMode)
            continue;

        sVisibleSettings[sVisibleSettingCount] = sSettingsListItems[i];
        sVisibleSettingIds[sVisibleSettingCount] = i;
        sVisibleSettingCount++;
    }
}

// Tears down and recreates the list menu after a toggle that changed which
// rows are visible (SETTING_NUZLOCKE / SETTING_DRAFT only - see
// BuildVisibleSettings). The setting the cursor was sitting on is never the
// one that just vanished (toggling one of the pair only ever hides the
// *other* one), so it's still present in the rebuilt list; this finds its
// new row and lands the cursor there instead of snapping back to the top.
static void RebuildVisibleSettingsList(u8 taskId)
{
    struct ListMenuTemplate template = {0};
    u8 settingId = sVisibleSettingIds[sSettingsScroll.scrollOffset + sSettingsScroll.selectedRow];
    u8 i, newRow = 0;

    DestroyListMenuTask(gTasks[taskId].tListTaskId, NULL, NULL);
    RemoveScrollIndicatorArrowPair(gTasks[taskId].tScrollArrowTaskId);

    BuildVisibleSettings();

    for (i = 0; i < sVisibleSettingCount; i++)
    {
        if (sVisibleSettingIds[i] == settingId)
        {
            newRow = i;
            break;
        }
    }

    if (newRow < SETTINGS_MAX_SHOWED)
    {
        sSettingsScroll.scrollOffset = 0;
        sSettingsScroll.selectedRow = newRow;
    }
    else
    {
        sSettingsScroll.scrollOffset = newRow - (SETTINGS_MAX_SHOWED - 1);
        sSettingsScroll.selectedRow = SETTINGS_MAX_SHOWED - 1;
    }

    template.items = sVisibleSettings;
    template.moveCursorFunc = SettingsMenu_MoveCursorCallback;
    template.itemPrintFunc = SettingsMenu_ItemPrintCallback;
    template.totalItems = sVisibleSettingCount;
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

    gTasks[taskId].tListTaskId = ListMenuInit(&template, sSettingsScroll.scrollOffset, sSettingsScroll.selectedRow);
    gTasks[taskId].tScrollArrowTaskId = AddScrollIndicatorArrowPairParameterized(
        SCROLL_ARROW_UP, SETTINGS_ARROW_X, SETTINGS_ARROW_TOP_Y, SETTINGS_ARROW_BOTTOM_Y,
        sVisibleSettingCount > SETTINGS_MAX_SHOWED ? sVisibleSettingCount - SETTINGS_MAX_SHOWED : 0,
        TAG_SETTINGS_SCROLL_ARROWS, TAG_SETTINGS_SCROLL_ARROWS, &sSettingsScroll.scrollOffset);
}

void ApplyPendingNewGameSettings(void)
{
    gSaveBlock1Ptr->difficulty = gPendingNewGameSettings.difficulty;
    gSaveBlock1Ptr->nuzlockeModeEnabled = gPendingNewGameSettings.nuzlockeEnabled;
    gSaveBlock1Ptr->draftModeEnabled = gPendingNewGameSettings.draftMode;
    gSaveBlock2Ptr->monoTypeSetting = gPendingNewGameSettings.monoType;
    gSaveBlock2Ptr->monoGenSetting = gPendingNewGameSettings.monoGen;
    gSaveBlock2Ptr->limitedPartySetting = gPendingNewGameSettings.limitedParty;
    gPendingNewGameSettings.randomizeSpecies ? FlagSet(FLAG_RANDOMIZE_MON)     : FlagClear(FLAG_RANDOMIZE_MON);
    gPendingNewGameSettings.randomizeTypes   ? FlagSet(FLAG_RANDOMIZE_TYPE)    : FlagClear(FLAG_RANDOMIZE_TYPE);
    gPendingNewGameSettings.randomizeMoves   ? FlagSet(FLAG_RANDOMIZE_MOVES)   : FlagClear(FLAG_RANDOMIZE_MOVES);
    gPendingNewGameSettings.levelCapOff      ? FlagSet(FLAG_LEVEL_CAP_OFF)     : FlagClear(FLAG_LEVEL_CAP_OFF);
    gPendingNewGameSettings.allowStatEditor  ? FlagSet(FLAG_ALLOW_STAT_EDITOR) : FlagClear(FLAG_ALLOW_STAT_EDITOR);
    gPendingNewGameSettings.debugMode        ? FlagSet(FLAG_DEBUG)             : FlagClear(FLAG_DEBUG);
    // Debug Mode, Stat Editor, and Level Cap Off all give the player tools
    // that can trivially manufacture achievement-worthy state (arbitrary
    // IV/EV values, uncapped levels, the Debug Menu itself), so all three
    // permanently disqualify the run the same way -- see the matching
    // [DISABLES ACHIEVEMENTS] callouts on their descriptions above.
    if (gPendingNewGameSettings.debugMode
     || gPendingNewGameSettings.allowStatEditor
     || gPendingNewGameSettings.levelCapOff)
        gSaveBlock1Ptr->achievementsBlocked = TRUE;
}

// Mirror image of ApplyPendingNewGameSettings: reads the settings back out of
// the *current* save instead of writing them into a new one. NewGameInitData
// calls ApplyPendingNewGameSettings unconditionally for any non-New-Game-Plus
// start, so gPendingNewGameSettings has to hold the right values by the time
// CB2_NewGame runs -- normally guaranteed because this screen is the only
// thing that ever sets it, right before handing off to CB2_NewGame itself.
// The Nuzlocke-restart "YES" path (field_screen_effect.c) breaks that
// guarantee: it calls CB2_NewGame directly, skipping this screen entirely.
// gPendingNewGameSettings then still holds whatever was last chosen here
// *this power-on session* -- stale, or still at its all-FALSE default, for a
// save that was simply continued from a previous one -- so without this,
// restarting would silently reset nuzlocke mode, difficulty, and every other
// toggle back to their defaults instead of carrying the failed run's own
// settings forward.
void CaptureCurrentSaveIntoPendingNewGameSettings(void)
{
    gPendingNewGameSettings.difficulty = gSaveBlock1Ptr->difficulty;
    gPendingNewGameSettings.nuzlockeEnabled = gSaveBlock1Ptr->nuzlockeModeEnabled;
    gPendingNewGameSettings.draftMode = gSaveBlock1Ptr->draftModeEnabled;
    gPendingNewGameSettings.monoType = gSaveBlock2Ptr->monoTypeSetting;
    gPendingNewGameSettings.monoGen = gSaveBlock2Ptr->monoGenSetting;
    gPendingNewGameSettings.limitedParty = gSaveBlock2Ptr->limitedPartySetting;
    gPendingNewGameSettings.randomizeSpecies = FlagGet(FLAG_RANDOMIZE_MON);
    gPendingNewGameSettings.randomizeTypes = FlagGet(FLAG_RANDOMIZE_TYPE);
    gPendingNewGameSettings.randomizeMoves = FlagGet(FLAG_RANDOMIZE_MOVES);
    gPendingNewGameSettings.levelCapOff = FlagGet(FLAG_LEVEL_CAP_OFF);
    gPendingNewGameSettings.allowStatEditor = FlagGet(FLAG_ALLOW_STAT_EDITOR);
    gPendingNewGameSettings.debugMode = FlagGet(FLAG_DEBUG);
}
