#include "global.h"
#include "trade_code_prompt.h"
#include "bg.h"
#include "gpu_regs.h"
#include "malloc.h"
#include "menu.h"
#include "palette.h"
#include "scanline_effect.h"
#include "sound.h"
#include "string_util.h"
#include "task.h"
#include "text.h"
#include "window.h"
#include "constants/rgb.h"
#include "constants/songs.h"
#include "menu_helpers.h"

// A small, self-contained full-screen message/yes-no prompt for
// trade_code_session.c. See include/trade_code_prompt.h for the public
// contract and why this is its own screen.
//
// Built on the game's standard message-box and yes/no-box primitives
// (LoadMessageBoxAndBorderGfx/DrawDialogueFrame/CreateYesNoMenu) on windows
// this screen owns via its own InitWindows call, never the overworld's window
// 0. No custom tile/tilemap/palette assets.
//
// Still a full CB2_/Task_-driven takeover (like trade_code_display.c): owning
// all of its own state, with nothing borrowed from the previous screen, is what
// avoids the hardware hang.

// Sized over the longest real message (sText_ConfirmCommit in
// trade_code_session.c, ~130 characters including \n breaks).
#define TRADE_CODE_PROMPT_MESSAGE_MAX_CHARS 200

enum TradeCodePromptPhase
{
    PROMPT_PHASE_PRINTING, // message is still being typed out
    PROMPT_PHASE_ACK,      // ACK-only mode: waiting for A
    PROMPT_PHASE_YESNO,    // waiting on CreateYesNoMenu's own cursor/input
};

struct TradeCodePromptResources
{
    MainCallback savedCallback;
    enum TradeCodePromptResult *outResult;
    u8 phase;   // enum TradeCodePromptPhase
    bool8 hasYesNo;
    u8 yesNoInitialCursorPos; // 0 = YES, 1 = NO - only read once, when CreateYesNoMenu is called
    u8 message[TRADE_CODE_PROMPT_MESSAGE_MAX_CHARS + 1];
};

// Window 0 - must stay index 0: AddTextPrinterForMessage,
// RunTextPrintersAndIsPrinter0Active and DrawDialogueFrame implicitly target
// window 0 (src/menu.c). The yes/no box is not in this array; CreateYesNoMenu
// creates and owns its own window.
enum WindowIds
{
    WINDOW_MESSAGE,
};

static EWRAM_DATA struct TradeCodePromptResources *sTradeCodePromptDataPtr = NULL;

static void TradeCodePrompt_RunSetup(void);
static bool8 TradeCodePrompt_DoGfxSetup(void);
static void TradeCodePrompt_FreeResources(void);
static void Task_TradeCodePromptWaitFadeIn(u8 taskId);
static void Task_TradeCodePromptMain(u8 taskId);
static void TradeCodePrompt_Finish(enum TradeCodePromptResult result);

// Matches the field message box's position (sStandardTextBox_WindowTemplates,
// src/menu.c). baseBlock is this screen's own (tiles 1-107 are free here), not
// menu.c's overworld-specific 0x194.
static const struct WindowTemplate sTradeCodePromptWindowTemplates[] =
{
    [WINDOW_MESSAGE] =
    {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 15,
        .width = 27,
        .height = 4,
        .paletteNum = DLG_WINDOW_PALETTE_NUM,
        .baseBlock = 1,
    },
    DUMMY_WIN_TEMPLATE
};

// Matches the field yes/no box's position (sYesNo_WindowTemplates, src/menu.c),
// upper-right, clear of the message box. baseBlock is this screen's own (after
// WINDOW_MESSAGE's 27*4=108 tiles), not menu.c's 0x125.
//
// paletteNum must be DLG (15), not STD (14). The template's paletteNum colours
// the window's contents (interior fill and YES/NO text), while the frame tiles
// use the paletteNum passed to CreateYesNoMenu. Only bank 15 has white at
// colour index 1; bank 14 (the user's window-frame palette) has near black
// there, which renders the interior black.
static const struct WindowTemplate sTradeCodePromptYesNoTemplate =
{
    .bg = 0,
    .tilemapLeft = 21,
    .tilemapTop = 9,
    .width = 5,
    .height = 4,
    .paletteNum = DLG_WINDOW_PALETTE_NUM,
    .baseBlock = 1 + (27 * 4),
};

// The blank tile VRAM is DMA-filled with (case 0 below) references BG palette
// bank 1 (tilemap entry 0x1000's top nibble). Without an explicit load, that
// bank keeps the previous screen's colours once the fade-in completes,
// garbling the backdrop. Zeroed so the backdrop is black; this screen loads no
// background palette of its own.
static const u16 sBlankPalette[32] = {0};

void TradeCodePrompt_Init(const u8 *message, bool8 hasYesNo, bool8 yesNoDefaultNo, enum TradeCodePromptResult *outResult, MainCallback callback)
{
    if ((sTradeCodePromptDataPtr = AllocZeroed(sizeof(struct TradeCodePromptResources))) == NULL)
    {
        *outResult = TRADE_CODE_PROMPT_ACK;
        SetMainCallback2(callback);
        return;
    }

    sTradeCodePromptDataPtr->savedCallback = callback;
    sTradeCodePromptDataPtr->outResult = outResult;
    sTradeCodePromptDataPtr->hasYesNo = hasYesNo;
    sTradeCodePromptDataPtr->yesNoInitialCursorPos = yesNoDefaultNo ? 1 : 0;

    // Plain StringCopy, not StringCopyN (see TradeCodeDisplay_Init): the buffer
    // is sized over every real message (TRADE_CODE_PROMPT_MESSAGE_MAX_CHARS).
    // Callers expand {STR_VAR_n} tokens themselves; this screen only copies
    // and prints.
    StringCopy(sTradeCodePromptDataPtr->message, message);

    SetMainCallback2(TradeCodePrompt_RunSetup);
}

static void TradeCodePrompt_RunSetup(void)
{
    while (1)
    {
        if (TradeCodePrompt_DoGfxSetup() == TRUE)
            break;
    }
}

static void TradeCodePrompt_MainCB(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    DoScheduledBgTilemapCopiesToVram();
    UpdatePaletteFade();
}

static void TradeCodePrompt_VBlankCB(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

static const struct BgTemplate sTradeCodePromptBgTemplates[] =
{
    {
        .bg = 0,    // windows only - no decorative background layer at all
        .charBaseIndex = 0,
        .mapBaseIndex = 30,
        .priority = 0
    },
};

static bool8 TradeCodePrompt_DoGfxSetup(void)
{
    switch (gMain.state)
    {
    case 0:
        DmaClearLarge16(3, (void *)VRAM, VRAM_SIZE, 0x1000);
        SetVBlankHBlankCallbacksToNull();
        ResetVramOamAndBgCntRegs();
        ClearScheduledBgCopiesToVram();
        gMain.state++;
        break;
    case 1:
        ScanlineEffect_Stop();
        FreeAllSpritePalettes();
        ResetPaletteFade();
        ResetSpriteData();
        ResetTasks();
        gMain.state++;
        break;
    case 2:
        ResetAllBgsCoordinates();
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(0, sTradeCodePromptBgTemplates, NELEMS(sTradeCodePromptBgTemplates));
        SetGpuReg(REG_OFFSET_DISPCNT, 0);
        ShowBg(0);
        // See sBlankPalette; must happen before the fade-in becomes visible.
        // LoadMessageBoxAndBorderGfx below overwrites banks 14/15, not 0-1.
        LoadPalette(sBlankPalette, 0, sizeof(sBlankPalette));
        gMain.state++;
        break;
    case 3:
        InitWindows(sTradeCodePromptWindowTemplates);
        DeactivateAllTextPrinters();
        // LoadMessageBoxGfx/LoadUserWindowBorderGfx resolve their BG from
        // window 0's .bg attribute (src/text_window.c), so this must run after
        // InitWindows.
        LoadMessageBoxAndBorderGfx();
        DrawDialogueFrame(WINDOW_MESSAGE, TRUE);
        StringCopy(gStringVar4, sTradeCodePromptDataPtr->message);
        AddTextPrinterForMessage(TRUE);
        gMain.state++;
        break;
    case 4:
        CreateTask(Task_TradeCodePromptWaitFadeIn, 0);
        BlendPalettes(0xFFFFFFFF, 16, RGB_BLACK);
        gMain.state++;
        break;
    case 5:
        BeginNormalPaletteFade(0xFFFFFFFF, 0, 16, 0, RGB_BLACK);
        gMain.state++;
        break;
    default:
        SetVBlankCallback(TradeCodePrompt_VBlankCB);
        SetMainCallback2(TradeCodePrompt_MainCB);
        return TRUE;
    }
    return FALSE;
}

#define try_free(ptr) ({        \
    void ** ptr__ = (void **)&(ptr);   \
    if (*ptr__ != NULL)                \
        Free(*ptr__);                  \
})

static void TradeCodePrompt_FreeResources(void)
{
    try_free(sTradeCodePromptDataPtr);
    FreeAllWindowBuffers();
}

static void Task_TradeCodePromptWaitFadeIn(u8 taskId)
{
    if (!gPaletteFade.active)
        gTasks[taskId].func = Task_TradeCodePromptMain;
}

static void Task_TradeCodePromptTurnOff(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        SetMainCallback2(sTradeCodePromptDataPtr->savedCallback);
        TradeCodePrompt_FreeResources();
        DestroyTask(taskId);
    }
}

static void TradeCodePrompt_Finish(enum TradeCodePromptResult result)
{
    *sTradeCodePromptDataPtr->outResult = result;
    BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, RGB_BLACK);
}

static void Task_TradeCodePromptMain(u8 taskId)
{
    struct TradeCodePromptResources *res = sTradeCodePromptDataPtr;
    s8 input;

    switch (res->phase)
    {
    case PROMPT_PHASE_PRINTING:
        if (!RunTextPrintersAndIsPrinter0Active())
        {
            if (res->hasYesNo)
            {
                CreateYesNoMenu(&sTradeCodePromptYesNoTemplate, STD_WINDOW_BASE_TILE_NUM, STD_WINDOW_PALETTE_NUM, res->yesNoInitialCursorPos);
                res->phase = PROMPT_PHASE_YESNO;
            }
            else
            {
                res->phase = PROMPT_PHASE_ACK;
            }
        }
        break;
    case PROMPT_PHASE_ACK:
        if (JOY_NEW(A_BUTTON))
        {
            PlaySE(SE_SELECT);
            TradeCodePrompt_Finish(TRADE_CODE_PROMPT_ACK);
            gTasks[taskId].func = Task_TradeCodePromptTurnOff;
        }
        break;
    case PROMPT_PHASE_YESNO:
        // Menu_ProcessInputNoWrapClearOnChoose (src/menu.c) already calls
        // EraseYesNoWindow once a choice is made; calling it again would
        // double-remove the same window ID.
        input = Menu_ProcessInputNoWrapClearOnChoose();
        if (input != MENU_NOTHING_CHOSEN)
        {
            PlaySE(SE_SELECT);
            TradeCodePrompt_Finish((input == 0) ? TRADE_CODE_PROMPT_YES : TRADE_CODE_PROMPT_NO);
            gTasks[taskId].func = Task_TradeCodePromptTurnOff;
        }
        break;
    }
}
