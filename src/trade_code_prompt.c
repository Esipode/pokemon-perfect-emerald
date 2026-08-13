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

// A small, self-contained full-screen message/yes-no prompt for src/
// trade_code_session.c. See include/trade_code_prompt.h for the public
// contract and why this exists as its own screen instead of reusing the
// overworld's own dialogue-box system.
//
// Deliberately built on the game's own *standard* message-box and yes/no-
// box primitives (LoadMessageBoxAndBorderGfx/DrawDialogueFrame/CreateYesNo
// Menu - the exact same ones an ordinary NPC conversation or a vanilla
// "Would you like to save?" prompt uses), on this screen's own freshly
// created window(s), rather than a bespoke full-screen background - the
// same "just a regular message box and yes/no box" request the plan doc's
// own status block records. No custom tile/tilemap/palette assets at all,
// unlike Stage 5/6's own screens - just the game's built-in dialogue
// assets, loaded onto a window this screen owns outright (via its own
// InitWindows call), never onto the overworld's own window 0 the way the
// version of this file that hung on hardware tried to.
//
// Still a full CB2_/Task_-driven takeover underneath (mirrors src/trade_
// code_display.c / src/ui_stat_editor.c's own gMain.state gfx setup, VBlank/
// main callback split) - that part of the shape is what actually fixed the
// hang (a screen that owns 100% of its own state, with nothing borrowed
// from whatever was on screen before), and is kept even though the visual
// footprint is now much smaller.

//==========DEFINES==========//
// Sized comfortably over the longest real message this screen shows
// (trade_code_session.c's own sText_ConfirmCommit, ~130 characters
// including its \n line breaks).
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

// Window 0 - must stay index 0: AddTextPrinterForMessage, RunTextPrinters
// AndIsPrinter0Active and DrawDialogueFrame's own conventional callers all
// implicitly target window 0 (see their own definitions in src/menu.c).
// The yes/no box isn't in this array at all - CreateYesNoMenu (src/menu.c)
// creates and owns its own window via AddWindow internally.
enum WindowIds
{
    WINDOW_MESSAGE,
};

//==========EWRAM==========//
static EWRAM_DATA struct TradeCodePromptResources *sTradeCodePromptDataPtr = NULL;

//==========STATIC=DEFINES==========//
static void TradeCodePrompt_RunSetup(void);
static bool8 TradeCodePrompt_DoGfxSetup(void);
static void TradeCodePrompt_FreeResources(void);
static void Task_TradeCodePromptWaitFadeIn(u8 taskId);
static void Task_TradeCodePromptMain(u8 taskId);
static void TradeCodePrompt_Finish(enum TradeCodePromptResult result);

//==========CONST=DATA==========//
// Matches the real field message box's own position exactly (src/menu.c's
// sStandardTextBox_WindowTemplates) - the classic bottom-of-screen
// dialogue box every player already recognises. baseBlock is this
// screen's own (nothing else here uses tiles 1-107), not menu.c's 0x194 -
// that value is specific to the overworld's own window layout.
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

// Matches the real field yes/no box's own position exactly (src/menu.c's
// sYesNo_WindowTemplates, what DisplayYesNoMenuDefaultYes itself uses) -
// upper-right, clear of the message box above. baseBlock is this screen's
// own (right after WINDOW_MESSAGE's own 27*4=108 tiles), not menu.c's
// 0x125 - same reasoning as WINDOW_MESSAGE's own baseBlock.
//
// paletteNum is DLG (15), not STD (14), exactly as sYesNo_WindowTemplates
// itself has it - and the two are NOT interchangeable here. A window
// template's own paletteNum colours the window's *contents* (the interior
// FillWindowPixelBuffer(PIXEL_FILL(1)) that DrawStdFrameWithCustomTileAnd
// Palette does, plus the YES/NO text), while the frame tiles drawn around
// it are coloured by the paletteNum passed to CreateYesNoMenu below. Only
// bank 15 (gMessageBox_Pal) has white at colour index 1; bank 14 (the
// user's own window-frame palette, graphics/text_window/N.png) has a near
// black there, which is what made this box's interior render black.
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

// The blank tile VRAM is DMA-filled with in this screen's own gfx setup
// (case 0 below, matching src/trade_code_display.c's own DmaClearLarge16
// call) references BG palette bank 1 (tilemap entry 0x1000's own top
// nibble). Without an explicit load here, that bank keeps whatever was
// left over from the previous screen once the fade-in reaches full
// brightness - not a problem while still fully faded to black, but a real
// "garbled backdrop behind a clean message box" risk once faded in. Zeroed
// out explicitly so the backdrop is genuinely black, not stale leftover
// colour data - Stage 5/6's own screens never needed this because they
// always loaded a full custom background palette that overwrote
// everything relevant; this screen deliberately doesn't load one at all.
static const u16 sBlankPalette[32] = {0};

//==========UI=SETUP==========// (mirrors trade_code_display.c)
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

    // Plain StringCopy, not StringCopyN - see TradeCodeDisplay_Init's own
    // comment on why (src/trade_code_display.c): StringCopyN has no EOS
    // check of its own and would walk off the end of a shorter source.
    // The buffer above is sized to comfortably exceed every real message
    // this screen is ever handed (see TRADE_CODE_PROMPT_MESSAGE_MAX_CHARS),
    // which is what actually keeps this safe. Callers are expected to have
    // already run this through StringExpandPlaceholders themselves if it
    // has {STR_VAR_n} tokens (see trade_code_session.c's own call sites) -
    // this screen only copies and prints, it doesn't expand anything.
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
        // See sBlankPalette's own comment - this has to happen before the
        // fade-in reaches visible brightness, and there's no harm doing it
        // this early (LoadMessageBoxAndBorderGfx below overwrites banks
        // 14/15 specifically, not 0-1).
        LoadPalette(sBlankPalette, 0, sizeof(sBlankPalette));
        gMain.state++;
        break;
    case 3:
        InitWindows(sTradeCodePromptWindowTemplates);
        DeactivateAllTextPrinters();
        // LoadMessageBoxGfx/LoadUserWindowBorderGfx (both called by this)
        // resolve which BG to load into via window 0's own .bg attribute
        // (GetWindowAttribute, src/text_window.c) - so this must run after
        // InitWindows, not before, or it would resolve against whatever
        // window 0 meant on the *previous* screen.
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

//
//       Trade Code Prompt specific code
//
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
        // Menu_ProcessInputNoWrapClearOnChoose (src/menu.c) owns the D-pad/
        // A/B handling and the cursor's own redraw entirely, and - true to
        // its own "ClearOnChoose" name - already calls EraseYesNoWindow
        // itself the moment a choice is made (confirmed by reading its
        // body before relying on it - it's not just a naming convention).
        // Calling EraseYesNoWindow again here would double-remove the same
        // window ID.
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
