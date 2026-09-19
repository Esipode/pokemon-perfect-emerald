#include "global.h"
#include "keep_storage_prompt.h"
#include "bg.h"
#include "gpu_regs.h"
#include "main.h"
#include "main_menu.h"
#include "menu.h"
#include "new_game_settings_menu.h"
#include "option_menu.h"
#include "palette.h"
#include "pokemon.h"
#include "pokemon_storage_system.h"
#include "save.h"
#include "scanline_effect.h"
#include "sound.h"
#include "sprite.h"
#include "task.h"
#include "text.h"
#include "text_window.h"
#include "window.h"
#include "constants/rgb.h"
#include "constants/songs.h"

// Shown right after NEW GAME, before GAME SETTINGS, whenever the current save has
// something in the PC or party worth keeping.
EWRAM_DATA bool8 gKeepStorageOnNewGame = FALSE;

enum
{
    WIN_HEADER,
    WIN_TEXT,
};

// Same look as other in-game prompts: gMessageBox_Gfx (LoadMessageBoxGfx) for the
// main message box, and the player's menu-frame skin
// (GetWindowFrameTilesPal(optionsWindowFrameType)) for the header and Yes/No box.
// The header is single-line, which the dialogue frame can't do (see WIN_HEADER's
// template). DLG_WINDOW_PALETTE_NUM / STD_WINDOW_PALETTE_NUM come from menu.h.
//
// Window pixel buffers (baseBlock) start at tile 1, NOT 0: tile 0 is the blank
// tile every unwritten backdrop cell points at (see the DmaClearLarge16 fill in
// case 1), and a buffer there makes the background flicker as text renders.
#define WIN_HEADER_BASE_BLOCK 0x1               // header window pixel buffer, 27*2 = 0x36 tiles
#define WIN_TEXT_BASE_BLOCK   0x6D              // text window pixel buffer, 27*4 = 0x6C tiles (plenty of headroom above WIN_HEADER_BASE_BLOCK's 0x36)
#define YESNO_BASE_BLOCK      (WIN_TEXT_BASE_BLOCK + 0x6C) // 0xD9; yes/no pixel buffer, 5*4 = 0x14 tiles
#define DLG_BASE_TILE         0xF0              // gMessageBox_Gfx, size 0x1C0 -- see LoadMessageBoxGfx
#define STD_FRAME_BASE_TILE   (DLG_BASE_TILE + 0x1C0) // 0x2B0; window-frame skin gfx, size 0x120 -- see LoadWindowGfx

#define YESNO_X          20
#define YESNO_Y          8

static void Task_KeepStoragePromptFadeIn(u8 taskId);
static void Task_KeepStoragePromptWaitPage(u8 taskId);
static void Task_KeepStoragePromptWaitTextThenYesNo(u8 taskId);
static void Task_KeepStoragePromptProcessYesNo(u8 taskId);
static void Task_KeepStoragePromptProcessDeleteYesNo(u8 taskId);
static void Task_KeepStoragePromptConfirm(u8 taskId);
static void Task_KeepStoragePromptCancel(u8 taskId);

static const u8 sText_KeepStoragePromptTitle[] = _("{COLOR RED}{SHADOW LIGHT_RED}KEEP POKéMON?");

// Printed a page at a time by the task below; see Task_KeepStoragePromptWaitPage.
static const u8 *const sKeepStoragePromptPages[] =
{
    COMPOUND_STRING(
        "You have (non-randomized)\n"
        "POKéMON stored in your PC."),
    COMPOUND_STRING(
        "They can't join your party\n"
        "until you become the CHAMPION."),
    COMPOUND_STRING(
        "Keep them for your new\n"
        "adventure?"),
};

static const u8 sText_KeepStoragePromptConfirmDelete[] = _(
    "This will permanently delete\n"
    "your POKéMON. Are you sure?");

#define tPageNum data[0]
#define tConfirmDelete data[1] // TRUE while asking "are you sure?" after NO

static const struct WindowTemplate sKeepStoragePromptWinTemplates[] =
{
    // Single-line title, so its border uses the STD frame: WindowFunc_DrawDialogueFrame
    // hardcodes a 5-row body and only fits at height 4, while
    // WindowFunc_DrawStandardFrame sizes to the window. .paletteNum stays on the DLG
    // bank: it colors the window's text (the call site's paletteNum argument only
    // colors the border tiles), and the player-chosen STD bank has no guaranteed
    // white-paper/dark-text/red-warning layout, unlike gMessageBox_Pal.
    [WIN_HEADER] = {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 1,
        .width = 27,
        .height = 2,
        .paletteNum = DLG_WINDOW_PALETTE_NUM,
        .baseBlock = WIN_HEADER_BASE_BLOCK
    },
    // Same proportions as every standard bottom-of-screen dialogue box in the game
    // (compare sNewGameBirchSpeechTextWindows[0] in main_menu.c).
    [WIN_TEXT] = {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 15,
        .width = 27,
        .height = 4,
        .paletteNum = DLG_WINDOW_PALETTE_NUM,
        .baseBlock = WIN_TEXT_BASE_BLOCK
    },
    DUMMY_WIN_TEMPLATE
};

static const struct BgTemplate sKeepStoragePromptBgTemplates[] =
{
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

static const u16 sKeepStoragePromptBg_Pal[] = {RGB(17, 18, 31)};

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

void CB2_InitKeepStoragePrompt(void)
{
    switch (gMain.state)
    {
    default:
    case 0:
        // Nothing to keep: go to the options menu (so defaults are set before
        // achievement-relevant options lock in), then the settings menu, as the
        // HAS_NO_SAVED_GAME shortcut in ui_main_menu.c does.
        if (gSaveFileStatus != SAVE_STATUS_OK
         || (CountAllStorageMons() == 0 && CalculatePlayerPartyCount() == 0))
        {
            gKeepStorageOnNewGame = FALSE;
            gMain.savedCallback = CB2_InitNewGameSettingsMenu;
            SetMainCallback2(CB2_InitOptionMenu);
            return;
        }
        SetVBlankCallback(NULL);
        gMain.state++;
        break;
    case 1:
        DmaClearLarge16(3, (void *)(VRAM), VRAM_SIZE, 0x1000);
        DmaClear32(3, OAM, OAM_SIZE);
        DmaClear16(3, PLTT, PLTT_SIZE);
        SetGpuReg(REG_OFFSET_DISPCNT, 0);
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(0, sKeepStoragePromptBgTemplates, ARRAY_COUNT(sKeepStoragePromptBgTemplates));
        ChangeBgX(0, 0, BG_COORD_SET);
        ChangeBgY(0, 0, BG_COORD_SET);
        InitWindows(sKeepStoragePromptWinTemplates);
        DeactivateAllTextPrinters();
        SetGpuReg(REG_OFFSET_WIN0H, 0);
        SetGpuReg(REG_OFFSET_WIN0V, 0);
        SetGpuReg(REG_OFFSET_WININ, 0);
        SetGpuReg(REG_OFFSET_WINOUT, 0);
        SetGpuReg(REG_OFFSET_BLDCNT, 0);
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
        ShowBg(0);
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
        LoadBgTiles(0, GetWindowFrameTilesPal(gSaveBlock2Ptr->optionsWindowFrameType)->tiles, 0x120, STD_FRAME_BASE_TILE);
        LoadMessageBoxGfx(WIN_TEXT, DLG_BASE_TILE, BG_PLTT_ID(DLG_WINDOW_PALETTE_NUM));
        gMain.state++;
        break;
    case 4:
        LoadPalette(sKeepStoragePromptBg_Pal, BG_PLTT_ID(0), sizeof(sKeepStoragePromptBg_Pal));
        LoadPalette(GetWindowFrameTilesPal(gSaveBlock2Ptr->optionsWindowFrameType)->pal, BG_PLTT_ID(STD_WINDOW_PALETTE_NUM), PLTT_SIZE_4BPP);
        gMain.state++;
        break;
    case 5:
        DrawStdFrameWithCustomTileAndPalette(WIN_HEADER, TRUE, STD_FRAME_BASE_TILE, STD_WINDOW_PALETTE_NUM);
        AddTextPrinterParameterized(WIN_HEADER, FONT_NORMAL, sText_KeepStoragePromptTitle, 8, 1, TEXT_SKIP_DRAW, NULL);
        CopyWindowToVram(WIN_HEADER, COPYWIN_FULL);
        gMain.state++;
        break;
    case 6:
        // Frame only -- the first page is printed once the fade-in finishes.
        DrawDialogFrameWithCustomTileAndPalette(WIN_TEXT, TRUE, DLG_BASE_TILE, DLG_WINDOW_PALETTE_NUM);
        gMain.state++;
        break;
    case 7:
        CreateTask(Task_KeepStoragePromptFadeIn, 0);
        gMain.state++;
        break;
    case 8:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
        SetVBlankCallback(VBlankCB);
        SetMainCallback2(MainCB2);
        return;
    }
}

static void KeepStoragePrompt_PrintText(const u8 *text)
{
    FillWindowPixelBuffer(WIN_TEXT, PIXEL_FILL(1));
    AddTextPrinterParameterized(WIN_TEXT, FONT_NORMAL, text, 0, 1, GetPlayerTextSpeedDelay(), NULL);
}

static void KeepStoragePrompt_PrintPage(u8 taskId)
{
    KeepStoragePrompt_PrintText(sKeepStoragePromptPages[gTasks[taskId].tPageNum]);
}

// Prints text, then shows the Yes/No box once it finishes. confirmDelete picks which
// question the Yes/No box answers.
static void KeepStoragePrompt_PrintThenYesNo(u8 taskId, const u8 *text, bool32 confirmDelete)
{
    KeepStoragePrompt_PrintText(text);
    gTasks[taskId].tConfirmDelete = confirmDelete;
    gTasks[taskId].func = Task_KeepStoragePromptWaitTextThenYesNo;
}

static void Task_KeepStoragePromptFadeIn(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        // gTextFlags is shared; clear forceMidTextSpeed in case a prior screen
        // left it set, which would override the per-character reveal rate.
        gTextFlags.canABSpeedUpPrint = TRUE;
        gTextFlags.autoScroll = FALSE;
        gTextFlags.forceMidTextSpeed = FALSE;
        gTextFlags.useAlternateDownArrow = FALSE;
        gTasks[taskId].tPageNum = 0;
        KeepStoragePrompt_PrintPage(taskId);
        gTasks[taskId].func = Task_KeepStoragePromptWaitPage;
    }
}

static void Task_KeepStoragePromptWaitPage(u8 taskId)
{
    RunTextPrinters();
    if (IsTextPrinterActiveOnWindow(WIN_TEXT))
        return;

    // Not \p page breaks: their wait is skipped by gTextFlags.autoScroll or
    // FLAG_AUTO_SCROLL_TEXT (see TextPrinterWaitWithDownArrow in text.c), so
    // pages would advance on a timer. Gating on our own JOY_NEW can't be bypassed.
    if (JOY_NEW(A_BUTTON | B_BUTTON))
    {
        PlaySE(SE_SELECT);
        gTasks[taskId].tPageNum++;
        if (gTasks[taskId].tPageNum < ARRAY_COUNT(sKeepStoragePromptPages))
        {
            KeepStoragePrompt_PrintPage(taskId);
        }
        else
        {
            // Cursor defaults to YES (position 0) -- it's the non-destructive answer.
            CreateYesNoMenuParameterized(YESNO_X, YESNO_Y, STD_FRAME_BASE_TILE, YESNO_BASE_BLOCK, STD_WINDOW_PALETTE_NUM, DLG_WINDOW_PALETTE_NUM);
            gTasks[taskId].func = Task_KeepStoragePromptProcessYesNo;
        }
    }
}

static void Task_KeepStoragePromptWaitTextThenYesNo(u8 taskId)
{
    RunTextPrinters();
    if (IsTextPrinterActiveOnWindow(WIN_TEXT))
        return;

    CreateYesNoMenuParameterized(YESNO_X, YESNO_Y, STD_FRAME_BASE_TILE, YESNO_BASE_BLOCK, STD_WINDOW_PALETTE_NUM, DLG_WINDOW_PALETTE_NUM);
    if (gTasks[taskId].tConfirmDelete)
    {
        // Default to NO -- YES here is the destructive answer.
        Menu_MoveCursorNoWrapAround(1);
        gTasks[taskId].func = Task_KeepStoragePromptProcessDeleteYesNo;
    }
    else
    {
        gTasks[taskId].func = Task_KeepStoragePromptProcessYesNo;
    }
}

static void Task_KeepStoragePromptProcessYesNo(u8 taskId)
{
    switch (Menu_ProcessInputNoWrapClearOnChoose())
    {
    case 0: // YES
        gKeepStorageOnNewGame = TRUE;
        PlaySE(SE_SELECT);
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        gTasks[taskId].func = Task_KeepStoragePromptConfirm;
        break;
    case 1: // NO -- ask again, since declining permanently deletes the storage
        PlaySE(SE_SELECT);
        KeepStoragePrompt_PrintThenYesNo(taskId, sText_KeepStoragePromptConfirmDelete, TRUE);
        break;
    case MENU_B_PRESSED:
        PlaySE(SE_SELECT);
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        gTasks[taskId].func = Task_KeepStoragePromptCancel;
        break;
    }
}

static void Task_KeepStoragePromptProcessDeleteYesNo(u8 taskId)
{
    switch (Menu_ProcessInputNoWrapClearOnChoose())
    {
    case 0: // YES -- delete
        gKeepStorageOnNewGame = FALSE;
        PlaySE(SE_SELECT);
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        gTasks[taskId].func = Task_KeepStoragePromptConfirm;
        break;
    case 1: // NO
    case MENU_B_PRESSED:
        // Back to the keep question (the last page).
        PlaySE(SE_SELECT);
        gTasks[taskId].tPageNum = ARRAY_COUNT(sKeepStoragePromptPages) - 1;
        KeepStoragePrompt_PrintThenYesNo(taskId, sKeepStoragePromptPages[gTasks[taskId].tPageNum], FALSE);
        break;
    }
}

static void Task_KeepStoragePromptConfirm(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        DestroyTask(taskId);
        FreeAllWindowBuffers();
        // Options menu first, then the settings menu -- see the case-0 shortcut above.
        gMain.savedCallback = CB2_InitNewGameSettingsMenu;
        SetMainCallback2(CB2_InitOptionMenu);
    }
}

static void Task_KeepStoragePromptCancel(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        DestroyTask(taskId);
        FreeAllWindowBuffers();
        SetMainCallback2(gMain.savedCallback);
    }
}
