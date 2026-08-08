#include "global.h"
#include "keep_storage_prompt.h"
#include "bg.h"
#include "gpu_regs.h"
#include "main.h"
#include "main_menu.h"
#include "menu.h"
#include "new_game_settings_menu.h"
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
// something in the PC or party worth keeping. See Storage_Retention_Plan.md Part 2.
EWRAM_DATA bool8 gKeepStorageOnNewGame = FALSE;

enum
{
    WIN_HEADER,
    WIN_TEXT,
};

// Same look every other in-game prompt uses: the fixed dialogue-box graphic (gMessageBox_Gfx,
// loaded via LoadMessageBoxGfx) for the main message box, and the player's chosen menu-frame
// skin (GetWindowFrameTilesPal(optionsWindowFrameType)) for both the header and the Yes/No box
// -- the header is single-line, and the dialogue frame can't do that (see the comment on
// WIN_HEADER's template below). DLG_WINDOW_PALETTE_NUM / STD_WINDOW_PALETTE_NUM come from menu.h.
//
// Tile budget below is laid out low-to-high on purpose: the window pixel buffers (baseBlock)
// start at 1, NOT 0. Tile index 0 is the "blank" tile every unwritten cell of the screen's
// backdrop implicitly points at (see the DmaClearLarge16 fill in case 1). A window buffer
// placed at baseBlock 0 overwrites that shared tile as its own text renders, which reads as
// the whole screen's background flickering in sync with the letters being drawn.
#define WIN_HEADER_BASE_BLOCK 0x1               // header window pixel buffer, 27*2 = 0x36 tiles
#define WIN_TEXT_BASE_BLOCK   0x6D              // text window pixel buffer, 27*4 = 0x6C tiles (plenty of headroom above WIN_HEADER_BASE_BLOCK's 0x36)
#define YESNO_BASE_BLOCK      (WIN_TEXT_BASE_BLOCK + 0x6C) // 0xD9; yes/no pixel buffer, 5*4 = 0x14 tiles
#define DLG_BASE_TILE         0xF0              // gMessageBox_Gfx, size 0x1C0 -- see LoadMessageBoxGfx
#define STD_FRAME_BASE_TILE   (DLG_BASE_TILE + 0x1C0) // 0x2B0; window-frame skin gfx, size 0x120 -- see LoadWindowGfx

#define YESNO_X          20
#define YESNO_Y          8

static void Task_KeepStoragePromptFadeIn(u8 taskId);
static void Task_KeepStoragePromptWaitPage(u8 taskId);
static void Task_KeepStoragePromptProcessYesNo(u8 taskId);
static void Task_KeepStoragePromptConfirm(u8 taskId);
static void Task_KeepStoragePromptCancel(u8 taskId);

static const u8 sText_KeepStoragePromptTitle[] = _("{COLOR RED}{SHADOW LIGHT_RED}KEEP POKéMON?");

// Printed one page at a time by the task below rather than as a single \p-separated
// string -- see the comment in Task_KeepStoragePromptWaitPage for why.
static const u8 *const sKeepStoragePromptPages[] =
{
    COMPOUND_STRING(
        "You have POKéMON stored in\n"
        "your PC."),
    COMPOUND_STRING(
        "They can't join your party\n"
        "until you become the CHAMPION."),
    COMPOUND_STRING(
        "Keep them for your new\n"
        "adventure?"),
};

#define tPageNum data[0]

static const struct WindowTemplate sKeepStoragePromptWinTemplates[] =
{
    // Single-line title, so its border uses the STD frame style (like the Yes/No box)
    // rather than the DLG dialogue frame: WindowFunc_DrawDialogueFrame hardcodes a
    // 5-row-tall body regardless of the window's own height, so it only looks right at
    // height 4 -- WindowFunc_DrawStandardFrame sizes itself to whatever height it's
    // given. .paletteNum stays on the DLG bank though, same as the Yes/No box's own
    // template: DrawStdFrameWithCustomTileAndPalette's paletteNum argument (passed at
    // the call site) only colors the *border* tiles. This .paletteNum field is the
    // separate thing that colors the window's own text, and the STD skin bank is
    // whatever border-decoration palette the player picked in Options -- it has no
    // guaranteed white-paper/dark-text/red-warning layout the way gMessageBox_Pal does,
    // which is what left the header dark-gray-on-dark-gray instead of red-on-white.
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
        // Nothing to keep -- skip straight to the settings menu, same destination
        // as the HAS_NO_SAVED_GAME shortcut in ui_main_menu.c.
        if (gSaveFileStatus != SAVE_STATUS_OK
         || (CountAllStorageMons() == 0 && CalculatePlayerPartyCount() == 0))
        {
            gKeepStorageOnNewGame = FALSE;
            SetMainCallback2(CB2_InitNewGameSettingsMenu);
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
        // The header and the Yes/No box use the player's chosen menu-frame skin (the
        // same graphic Options/GAME SETTINGS use); the main message box uses the fixed
        // dialogue-box graphic every NPC/system message in the game uses.
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

static void KeepStoragePrompt_PrintPage(u8 taskId)
{
    FillWindowPixelBuffer(WIN_TEXT, PIXEL_FILL(1));
    AddTextPrinterParameterized(WIN_TEXT, FONT_NORMAL, sKeepStoragePromptPages[gTasks[taskId].tPageNum], 0, 1, GetPlayerTextSpeedDelay(), NULL);
}

static void Task_KeepStoragePromptFadeIn(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        // gTextFlags is a shared global. autoScroll no longer matters for page
        // advancement here (see Task_KeepStoragePromptWaitPage), but forceMidTextSpeed
        // still overrides the per-character reveal rate if left set by whatever screen
        // ran before this one, so it's worth clearing along with it.
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

    // Deliberately not using \p page breaks for this: \p's wait is skipped by
    // gTextFlags.autoScroll OR the save-file flag FLAG_AUTO_SCROLL_TEXT (see
    // TextPrinterWaitWithDownArrow in text.c), so a player with that accessibility
    // option on -- or a leftover autoScroll left set by whatever screen ran before
    // this one -- would still blow through every page on a timer. Gating the
    // advance on our own JOY_NEW check here can't be shortcut by either of those.
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
    case 1: // NO
        gKeepStorageOnNewGame = FALSE;
        PlaySE(SE_SELECT);
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        gTasks[taskId].func = Task_KeepStoragePromptConfirm;
        break;
    case MENU_B_PRESSED:
        PlaySE(SE_SELECT);
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        gTasks[taskId].func = Task_KeepStoragePromptCancel;
        break;
    }
}

static void Task_KeepStoragePromptConfirm(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        DestroyTask(taskId);
        FreeAllWindowBuffers();
        SetMainCallback2(CB2_InitNewGameSettingsMenu);
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
