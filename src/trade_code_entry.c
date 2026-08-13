#include "global.h"
#include "trade_code_entry.h"
#include "trade_code.h"
#include "trade_code_display.h"
#include "bg.h"
#include "decompress.h"
#include "gpu_regs.h"
#include "malloc.h"
#include "menu.h"
#include "menu_helpers.h"
#include "palette.h"
#include "scanline_effect.h"
#include "sound.h"
#include "task.h"
#include "text.h"
#include "window.h"
#include "constants/characters.h"
#include "constants/rgb.h"
#include "constants/songs.h"

// Stage 6 of "Trading Codes.md": the purpose-built code entry screen. See
// include/trade_code_entry.h for the public contract and the reasoning
// behind its shape (why a validator hook instead of protocol logic living
// here, why results come back through caller-owned out-params rather than
// a new callback-with-arguments type). Modelled on src/trade_code_display.c
// (Stage 5)'s CB2_/Task_-driven full-screen pattern, which is itself
// modelled on src/ui_stat_editor.c - same malloc'd-EWRAM-BG, gMain.state
// gfx setup, VBlank/main callback split. No sprites: the grid cursor and
// the typed-code caret are both drawn as plain text/color changes rather
// than sprite overlays, deliberately - it avoids pulling in any new
// graphics assets or OAM/palette-tag bookkeeping for what is, functionally,
// just "which cell is highlighted right now."

//==========DEFINES==========//
// Matches TRADE_CODE_MAX_CHARS' own derivation (include/config/trade_code.h)
// minus hyphens/EOS: 428 worst-case payload bits -> 86 Base32 symbols. The
// raw (unhyphenated) symbol buffer is sized to this rather than
// TRADE_CODE_MAX_CHARS itself, which already budgets for hyphens this
// buffer doesn't store (they're inserted only at display time). Declared
// ahead of struct TradeCodeEntryResources below, which sizes two of its
// own fields off these.
#define TRADE_CODE_ENTRY_MAX_SYMBOLS 86
// ceil(TRADE_CODE_ENTRY_MAX_SYMBOLS * 5 / 8) - the decode scratch buffer
// TradeCode_Decode writes into (see TradeCodeEntry_TrySubmit).
#define TRADE_CODE_ENTRY_SCRATCH_BYTES ((TRADE_CODE_ENTRY_MAX_SYMBOLS * 5 + 7) / 8)

struct TradeCodeEntryResources
{
    MainCallback callback;                 // where SetMainCallback2 goes once this screen is done
    struct TradeCodeBits *outBits;         // caller's buffer - only written on TRADE_CODE_ENTRY_OK
    enum TradeCodeEntryStatus *outStatus;  // caller's result slot - written exactly once, before callback
    TradeCodeEntryValidator validator;     // may be NULL - see trade_code_entry.h
    u32 expectedSymbols;                   // 0 = variable-length offer code, else a fixed confirm-code length
    u8 gfxLoadState;
    u8 taskId;
    bool8 isConfirmMode;                   // expectedSymbols != 0 - no hyphens, single ungrouped run
    s8 col;                                // 0..TRADE_CODE_ENTRY_GRID_COLS-1 = a symbol column,
                                            // TRADE_CODE_ENTRY_GRID_COLS = the BACK/OK button column
    s8 row;                                // 0..TRADE_CODE_ENTRY_GRID_ROWS-1, meaningful while col is a symbol column
    s8 buttonRow;                          // TRADE_CODE_ENTRY_BUTTON_BACK/_OK, meaningful while col is the button column
    u32 symbolCount;
    u8 rawSymbols[TRADE_CODE_ENTRY_MAX_SYMBOLS + 1];    // no hyphens, EOS-terminated, game-charmap
    u8 scratchBuffer[TRADE_CODE_ENTRY_SCRATCH_BYTES];   // TradeCode_Decode's target - see TradeCodeEntry_TrySubmit
    u8 caretBlinkTimer;
    bool8 caretVisible;
    s16 caretCol;                          // WINDOW_ENTRY cell coords of the live caret, -1 = none to draw
    s16 caretRow;                          // (off-screen past the visible rows, or the field is full)
    enum TradeCodeEntryStatus errorStatus; // TRADE_CODE_ENTRY_OK = no error banner shown
};

enum WindowIds
{
    WINDOW_HEADER,
    WINDOW_ENTRY,
    WINDOW_GRID,
    WINDOW_FOOTER,
    WINDOW_ERROR,
};

// All 32 Base32/Crockford symbols on one page, 8x4 - see trade_code_entry.h
// and the plan doc's own Stage 6 bullet list for why this isn't a paged
// naming_screen.c-style keyboard.
#define TRADE_CODE_ENTRY_GRID_COLS 8
#define TRADE_CODE_ENTRY_GRID_ROWS 4
#define TRADE_CODE_ENTRY_CELL_WIDTH  16
#define TRADE_CODE_ENTRY_CELL_HEIGHT 16
// The BACK/OK column sits one tile to the right of the symbol grid, wide
// enough for "BACK" in FONT_SMALL_NARROW (5px/glyph, comfortably under the
// column's 32px).
#define TRADE_CODE_ENTRY_BUTTON_GAP_TILES       1
#define TRADE_CODE_ENTRY_BUTTON_COL_WIDTH_TILES 4
#define TRADE_CODE_ENTRY_BUTTON_COL_X     (TRADE_CODE_ENTRY_GRID_COLS * TRADE_CODE_ENTRY_CELL_WIDTH + TRADE_CODE_ENTRY_BUTTON_GAP_TILES * 8)
#define TRADE_CODE_ENTRY_BUTTON_COL_WIDTH (TRADE_CODE_ENTRY_BUTTON_COL_WIDTH_TILES * 8)
#define TRADE_CODE_ENTRY_GRID_WIDTH_TILES (TRADE_CODE_ENTRY_GRID_COLS * (TRADE_CODE_ENTRY_CELL_WIDTH / 8) + TRADE_CODE_ENTRY_BUTTON_GAP_TILES + TRADE_CODE_ENTRY_BUTTON_COL_WIDTH_TILES)
#define TRADE_CODE_ENTRY_GRID_HEIGHT_TILES (TRADE_CODE_ENTRY_GRID_ROWS * (TRADE_CODE_ENTRY_CELL_HEIGHT / 8))

#define TRADE_CODE_ENTRY_BUTTON_BACK 0
#define TRADE_CODE_ENTRY_BUTTON_OK   1

// The typed-code field mirrors Stage 5's grid (TRADE_CODE_DISPLAY_* /
// CODE_CELL_* from trade_code_display.h - same cell size, same grouping),
// but only ever shows the most recent TRADE_CODE_ENTRY_VISIBLE_ROWS rows,
// scrolling as the player types past that - unlike the read-only display
// screen, which reserves all TRADE_CODE_DISPLAY_MAX_ROWS up front because
// it never changes after Init. Reserving all 4 rows here as well would
// have pushed the on-screen keyboard below the fold for no benefit: the
// typical code is ~2 rows (see the plan doc's payload spec), and a player
// actively typing cares about the tail near their caret, not rows already
// visually double-checked against their partner's screen.
#define TRADE_CODE_ENTRY_VISIBLE_ROWS 2

enum TradeCodeEntryFontColor
{
    FONT_COLOR_NORMAL,
    FONT_COLOR_HIGHLIGHT,
};

//==========EWRAM==========//
static EWRAM_DATA struct TradeCodeEntryResources *sTradeCodeEntryDataPtr = NULL;
static EWRAM_DATA u8 *sBg1TilemapBuffer = NULL;

//==========STATIC=DEFINES==========//
static void TradeCodeEntry_RunSetup(void);
static bool8 TradeCodeEntry_DoGfxSetup(void);
static bool8 TradeCodeEntry_InitBgs(void);
static void TradeCodeEntry_FadeAndBail(void);
static bool8 TradeCodeEntry_LoadGraphics(void);
static void TradeCodeEntry_InitWindows(void);
static void TradeCodeEntry_PrintHeader(void);
static void TradeCodeEntry_PrintFooter(void);
static void TradeCodeEntry_PrintError(void);
static void TradeCodeEntry_PrintEntryField(void);
static void TradeCodeEntry_DrawCaret(void);
static void TradeCodeEntry_PrintGrid(void);
static void TradeCodeEntry_DrawGridCell(s8 col, s8 row, bool8 highlighted);
static void TradeCodeEntry_DrawButtonCell(s8 buttonRow, bool8 highlighted);
static void TradeCodeEntry_MoveCursor(s8 dCol, s8 dRow);
static void TradeCodeEntry_AppendSymbol(u8 symbolIndex);
static void TradeCodeEntry_DeleteSymbol(void);
static void TradeCodeEntry_HandleBack(void);
static void TradeCodeEntry_TrySubmit(void);
static void TradeCodeEntry_Finish(enum TradeCodeEntryStatus status);
static void Task_TradeCodeEntryWaitFadeIn(u8 taskId);
static void Task_TradeCodeEntryMain(u8 taskId);
static void Task_TradeCodeEntryWaitFadeAndBail(u8 taskId);
static void TradeCodeEntry_FreeResources(void);

//==========CONST=DATA==========//
// Identical to trade_code_display.c's own BG setup - same generic
// full-screen UI background shared by ui_stat_editor.c/achievements_menu.c/
// trade_code_display.c. Kept as its own local copy rather than exported,
// matching how each of those modules already keeps its own WindowTemplate/
// BgTemplate tables despite the visual similarity - a WindowTemplate array
// is inherently screen-specific (baseBlock allocation differs per screen)
// even when the underlying tileset is shared.
static const struct BgTemplate sTradeCodeEntryBgTemplates[] =
{
    {
        .bg = 0,    // windows
        .charBaseIndex = 0,
        .mapBaseIndex = 30,
        .priority = 1
    },
    {
        .bg = 1,    // UI tilemap
        .charBaseIndex = 3,
        .mapBaseIndex = 28,
        .priority = 2
    },
    {
        .bg = 2,    // UI tilemap
        .charBaseIndex = 0,
        .mapBaseIndex = 26,
        .priority = 0
    }
};

static const struct WindowTemplate sTradeCodeEntryWindowTemplates[] =
{
    [WINDOW_HEADER] =
    {
        .bg = 0,
        .tilemapLeft = 1,
        .tilemapTop = 0,
        .width = 28,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 1,
    },
    [WINDOW_ENTRY] =
    {
        .bg = 0,
        .tilemapLeft = 1,
        .tilemapTop = 2,
        .width = TRADE_CODE_DISPLAY_ROW_CAPACITY,
        .height = TRADE_CODE_ENTRY_VISIBLE_ROWS * (CODE_CELL_HEIGHT / 8),
        .paletteNum = 15,
        .baseBlock = 1 + (28 * 2),
    },
    [WINDOW_GRID] =
    {
        .bg = 0,
        .tilemapLeft = 1,
        .tilemapTop = 6,
        .width = TRADE_CODE_ENTRY_GRID_WIDTH_TILES,
        .height = TRADE_CODE_ENTRY_GRID_HEIGHT_TILES,
        .paletteNum = 15,
        .baseBlock = 1 + (28 * 2) + (TRADE_CODE_DISPLAY_ROW_CAPACITY * TRADE_CODE_ENTRY_VISIBLE_ROWS * (CODE_CELL_HEIGHT / 8)),
    },
    [WINDOW_FOOTER] =
    {
        .bg = 0,
        .tilemapLeft = 1,
        .tilemapTop = 14,
        .width = 28,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 1 + (28 * 2) + (TRADE_CODE_DISPLAY_ROW_CAPACITY * TRADE_CODE_ENTRY_VISIBLE_ROWS * (CODE_CELL_HEIGHT / 8)) + (TRADE_CODE_ENTRY_GRID_WIDTH_TILES * TRADE_CODE_ENTRY_GRID_HEIGHT_TILES),
    },
    [WINDOW_ERROR] =
    {
        .bg = 0,
        .tilemapLeft = 1,
        .tilemapTop = 16,
        .width = 28,
        .height = 3,
        .paletteNum = 15,
        .baseBlock = 1 + (28 * 2) + (TRADE_CODE_DISPLAY_ROW_CAPACITY * TRADE_CODE_ENTRY_VISIBLE_ROWS * (CODE_CELL_HEIGHT / 8)) + (TRADE_CODE_ENTRY_GRID_WIDTH_TILES * TRADE_CODE_ENTRY_GRID_HEIGHT_TILES) + (28 * 2),
    },
    DUMMY_WIN_TEMPLATE
};

static const u32 sTradeCodeEntryBgTiles[] = INCBIN_U32("graphics/ui_menu/background_tileset.4bpp.smol");
static const u32 sTradeCodeEntryBgTilemap[] = INCBIN_U32("graphics/ui_menu/background_tileset.bin.smolTM");
static const u16 sTradeCodeEntryBgPalette[] = INCBIN_U16("graphics/ui_menu/background_pal.gbapal");

// FONT_COLOR_HIGHLIGHT reuses the same palette bank (paletteNum 15, loaded
// from the shared background_tileset palette) as FONT_COLOR_NORMAL - only
// the foreground TEXT_COLOR_* index differs, so highlighting a cell is a
// same-window recolor, not a separate palette load.
static const u8 sTradeCodeEntryFontColors[][3] =
{
    [FONT_COLOR_NORMAL]    = {TEXT_COLOR_TRANSPARENT, TEXT_COLOR_WHITE, TEXT_COLOR_DARK_GRAY},
    [FONT_COLOR_HIGHLIGHT] = {TEXT_COLOR_TRANSPARENT, TEXT_COLOR_GREEN, TEXT_COLOR_DARK_GRAY},
};

static const u8 sText_TitleOffer[]   = _("ENTER TRADE CODE");
static const u8 sText_TitleConfirm[] = _("ENTER CONFIRM CODE");
static const u8 sText_Footer[]       = _("A: Select  B: Delete  START: Submit");
static const u8 sText_Back[]         = _("BACK");
static const u8 sText_Ok[]           = _("OK");

// Canned messages - see the plan doc's Stage 6 bullet list, quoted where it
// gives exact wording. WRONG_LENGTH covers both TRADE_CODE_TOO_SHORT and
// TRADE_CODE_TOO_LONG (see TradeCodeEntry_TrySubmit) - the player-facing
// advice is the same either way. BAD_CHAR is unreachable through the
// on-screen keyboard alone (see trade_code_entry.h) but still needs text
// for parity with TradeCode_Decode's own status enum.
static const u8 sText_ErrorBadChar[]      = _("That code has an invalid\ncharacter.");
static const u8 sText_ErrorWrongLength[]  = _("That code is the wrong\nlength. Check for missing or\nextra characters.");
static const u8 sText_ErrorInvalid[]      = _("This code isn't valid - check\nfor typos, or make sure it\nwas made for you.");
static const u8 sText_ErrorAlreadyUsed[]  = _("This code was already used.");
static const u8 sText_ErrorWrongVersion[] = _("This code is from a\ndifferent game version.");

static const u8 *const sTradeCodeEntryErrorText[] =
{
    [TRADE_CODE_ENTRY_OK]            = NULL,
    [TRADE_CODE_ENTRY_BAD_CHAR]      = sText_ErrorBadChar,
    [TRADE_CODE_ENTRY_WRONG_LENGTH]  = sText_ErrorWrongLength,
    [TRADE_CODE_ENTRY_INVALID]       = sText_ErrorInvalid,
    [TRADE_CODE_ENTRY_ALREADY_USED]  = sText_ErrorAlreadyUsed,
    [TRADE_CODE_ENTRY_WRONG_VERSION] = sText_ErrorWrongVersion,
    [TRADE_CODE_ENTRY_CANCELLED]     = NULL,
};

//==========UI=SETUP==========// (mirrors trade_code_display.c / ui_stat_editor.c)
void TradeCodeEntry_Init(struct TradeCodeBits *outBits, u32 expectedSymbols,
                          TradeCodeEntryValidator validator, enum TradeCodeEntryStatus *outStatus,
                          MainCallback callback)
{
    if ((sTradeCodeEntryDataPtr = AllocZeroed(sizeof(struct TradeCodeEntryResources))) == NULL)
    {
        // Can't even open the screen - report it as the player backing out
        // rather than silently dropping the request on the floor.
        *outStatus = TRADE_CODE_ENTRY_CANCELLED;
        SetMainCallback2(callback);
        return;
    }

    sTradeCodeEntryDataPtr->gfxLoadState = 0;
    sTradeCodeEntryDataPtr->callback = callback;
    sTradeCodeEntryDataPtr->outBits = outBits;
    sTradeCodeEntryDataPtr->outStatus = outStatus;
    sTradeCodeEntryDataPtr->validator = validator;
    sTradeCodeEntryDataPtr->expectedSymbols = (expectedSymbols > TRADE_CODE_ENTRY_MAX_SYMBOLS) ? TRADE_CODE_ENTRY_MAX_SYMBOLS : expectedSymbols;
    sTradeCodeEntryDataPtr->isConfirmMode = (expectedSymbols != 0);
    sTradeCodeEntryDataPtr->rawSymbols[0] = EOS;
    sTradeCodeEntryDataPtr->caretVisible = TRUE;
    sTradeCodeEntryDataPtr->caretRow = -1;
    sTradeCodeEntryDataPtr->errorStatus = TRADE_CODE_ENTRY_OK;

    SetMainCallback2(TradeCodeEntry_RunSetup);
}

static void TradeCodeEntry_RunSetup(void)
{
    while (1)
    {
        if (TradeCodeEntry_DoGfxSetup() == TRUE)
            break;
    }
}

static void TradeCodeEntry_MainCB(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    DoScheduledBgTilemapCopiesToVram();
    UpdatePaletteFade();
}

static void TradeCodeEntry_VBlankCB(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

static bool8 TradeCodeEntry_DoGfxSetup(void)
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
        if (TradeCodeEntry_InitBgs())
        {
            sTradeCodeEntryDataPtr->gfxLoadState = 0;
            gMain.state++;
        }
        else
        {
            TradeCodeEntry_FadeAndBail();
            return TRUE;
        }
        break;
    case 3:
        if (TradeCodeEntry_LoadGraphics() == TRUE)
            gMain.state++;
        break;
    case 4:
        TradeCodeEntry_InitWindows();
        TradeCodeEntry_PrintHeader();
        TradeCodeEntry_PrintFooter();
        TradeCodeEntry_PrintError();
        TradeCodeEntry_PrintGrid();
        TradeCodeEntry_DrawGridCell(0, 0, TRUE); // highlight the starting cursor cell
        TradeCodeEntry_PrintEntryField();
        gMain.state++;
        break;
    case 5:
        sTradeCodeEntryDataPtr->taskId = CreateTask(Task_TradeCodeEntryWaitFadeIn, 0);
        BlendPalettes(0xFFFFFFFF, 16, RGB_BLACK);
        gMain.state++;
        break;
    case 6:
        BeginNormalPaletteFade(0xFFFFFFFF, 0, 16, 0, RGB_BLACK);
        gMain.state++;
        break;
    default:
        SetVBlankCallback(TradeCodeEntry_VBlankCB);
        SetMainCallback2(TradeCodeEntry_MainCB);
        return TRUE;
    }
    return FALSE;
}

#define try_free(ptr) ({        \
    void ** ptr__ = (void **)&(ptr);   \
    if (*ptr__ != NULL)                \
        Free(*ptr__);                  \
})

static void TradeCodeEntry_FreeResources(void)
{
    try_free(sTradeCodeEntryDataPtr);
    try_free(sBg1TilemapBuffer);
    FreeAllWindowBuffers();
}

static void Task_TradeCodeEntryWaitFadeAndBail(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        SetMainCallback2(sTradeCodeEntryDataPtr->callback);
        TradeCodeEntry_FreeResources();
        DestroyTask(taskId);
    }
}

static void TradeCodeEntry_FadeAndBail(void)
{
    BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, RGB_BLACK);
    CreateTask(Task_TradeCodeEntryWaitFadeAndBail, 0);
    SetVBlankCallback(TradeCodeEntry_VBlankCB);
    SetMainCallback2(TradeCodeEntry_MainCB);
}

static bool8 TradeCodeEntry_InitBgs(void)
{
    ResetAllBgsCoordinates();
    sBg1TilemapBuffer = Alloc(0x800);
    if (sBg1TilemapBuffer == NULL)
        return FALSE;

    memset(sBg1TilemapBuffer, 0, 0x800);
    ResetBgsAndClearDma3BusyFlags(0);
    InitBgsFromTemplates(0, sTradeCodeEntryBgTemplates, NELEMS(sTradeCodeEntryBgTemplates));
    SetBgTilemapBuffer(1, sBg1TilemapBuffer);
    ScheduleBgCopyTilemapToVram(1);
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
    ShowBg(0);
    ShowBg(1);
    ShowBg(2);
    return TRUE;
}

static bool8 TradeCodeEntry_LoadGraphics(void)
{
    switch (sTradeCodeEntryDataPtr->gfxLoadState)
    {
    case 0:
        ResetTempTileDataBuffers();
        DecompressAndCopyTileDataToVram(1, sTradeCodeEntryBgTiles, 0, 0, 0);
        sTradeCodeEntryDataPtr->gfxLoadState++;
        break;
    case 1:
        if (FreeTempTileDataBuffersIfPossible() != TRUE)
        {
            DecompressDataWithHeaderWram(sTradeCodeEntryBgTilemap, sBg1TilemapBuffer);
            sTradeCodeEntryDataPtr->gfxLoadState++;
        }
        break;
    case 2:
        LoadPalette(sTradeCodeEntryBgPalette, 0, 32);
        sTradeCodeEntryDataPtr->gfxLoadState++;
        break;
    default:
        sTradeCodeEntryDataPtr->gfxLoadState = 0;
        return TRUE;
    }
    return FALSE;
}

static void TradeCodeEntry_InitWindows(void)
{
    InitWindows(sTradeCodeEntryWindowTemplates);
    DeactivateAllTextPrinters();
    ScheduleBgCopyTilemapToVram(0);
}

static void Task_TradeCodeEntryWaitFadeIn(u8 taskId)
{
    if (!gPaletteFade.active)
        gTasks[taskId].func = Task_TradeCodeEntryMain;
}

//
//       Trade Code Entry specific code
//
static void TradeCodeEntry_PrintHeader(void)
{
    const u8 *title = sTradeCodeEntryDataPtr->isConfirmMode ? sText_TitleConfirm : sText_TitleOffer;

    FillWindowPixelBuffer(WINDOW_HEADER, PIXEL_FILL(TEXT_COLOR_TRANSPARENT));
    AddTextPrinterParameterized4(WINDOW_HEADER, FONT_NORMAL, 2, 2, 0, 0, sTradeCodeEntryFontColors[FONT_COLOR_NORMAL], TEXT_SKIP_DRAW, title);
    PutWindowTilemap(WINDOW_HEADER);
    CopyWindowToVram(WINDOW_HEADER, COPYWIN_FULL);
}

static void TradeCodeEntry_PrintFooter(void)
{
    FillWindowPixelBuffer(WINDOW_FOOTER, PIXEL_FILL(TEXT_COLOR_TRANSPARENT));
    AddTextPrinterParameterized4(WINDOW_FOOTER, FONT_SMALL_NARROW, 2, 2, 0, 0, sTradeCodeEntryFontColors[FONT_COLOR_NORMAL], TEXT_SKIP_DRAW, sText_Footer);
    PutWindowTilemap(WINDOW_FOOTER);
    CopyWindowToVram(WINDOW_FOOTER, COPYWIN_FULL);
}

static void TradeCodeEntry_PrintError(void)
{
    const u8 *text = sTradeCodeEntryErrorText[sTradeCodeEntryDataPtr->errorStatus];

    FillWindowPixelBuffer(WINDOW_ERROR, PIXEL_FILL(TEXT_COLOR_TRANSPARENT));
    if (text != NULL)
        AddTextPrinterParameterized4(WINDOW_ERROR, FONT_SMALL_NARROW, 2, 2, 0, 0, sTradeCodeEntryFontColors[FONT_COLOR_NORMAL], TEXT_SKIP_DRAW, text);
    PutWindowTilemap(WINDOW_ERROR);
    CopyWindowToVram(WINDOW_ERROR, COPYWIN_FULL);
}

// The on-screen keyboard is static content - drawn once, never re-laid-out
// (only individual cells get re-colored as the cursor moves, see
// TradeCodeEntry_DrawGridCell/_DrawButtonCell).
static void TradeCodeEntry_PrintGrid(void)
{
    s8 col, row;

    FillWindowPixelBuffer(WINDOW_GRID, PIXEL_FILL(TEXT_COLOR_TRANSPARENT));
    PutWindowTilemap(WINDOW_GRID);

    for (row = 0; row < TRADE_CODE_ENTRY_GRID_ROWS; row++)
    {
        for (col = 0; col < TRADE_CODE_ENTRY_GRID_COLS; col++)
            TradeCodeEntry_DrawGridCell(col, row, FALSE);
    }
    TradeCodeEntry_DrawButtonCell(TRADE_CODE_ENTRY_BUTTON_BACK, FALSE);
    TradeCodeEntry_DrawButtonCell(TRADE_CODE_ENTRY_BUTTON_OK, FALSE);
}

// Single-glyph cell, centered in a TRADE_CODE_ENTRY_CELL_WIDTH x _HEIGHT
// box - reads straight out of TradeCode_AlphabetSymbol (trade_code.h) so
// this grid can never draw a different alphabet than TradeCode_Encode/
// Decode actually use.
static void TradeCodeEntry_DrawGridCell(s8 col, s8 row, bool8 highlighted)
{
    u8 ch[2];
    u32 x = col * TRADE_CODE_ENTRY_CELL_WIDTH + (TRADE_CODE_ENTRY_CELL_WIDTH - 5) / 2;
    u32 y = row * TRADE_CODE_ENTRY_CELL_HEIGHT + (TRADE_CODE_ENTRY_CELL_HEIGHT - 8) / 2;

    ch[0] = TradeCode_AlphabetSymbol(row * TRADE_CODE_ENTRY_GRID_COLS + col);
    ch[1] = EOS;

    FillWindowPixelRect(WINDOW_GRID, PIXEL_FILL(TEXT_COLOR_TRANSPARENT), col * TRADE_CODE_ENTRY_CELL_WIDTH, row * TRADE_CODE_ENTRY_CELL_HEIGHT, TRADE_CODE_ENTRY_CELL_WIDTH, TRADE_CODE_ENTRY_CELL_HEIGHT);
    AddTextPrinterParameterized4(WINDOW_GRID, FONT_SMALL_NARROW, x, y, 0, 0, sTradeCodeEntryFontColors[highlighted ? FONT_COLOR_HIGHLIGHT : FONT_COLOR_NORMAL], TEXT_SKIP_DRAW, ch);
    CopyWindowToVram(WINDOW_GRID, COPYWIN_GFX);
}

// BACK spans grid rows 0-1, OK spans rows 2-3 - a two-way split of the same
// four rows the symbol grid uses, so the button column's cells line up
// with the grid vertically without needing a separate row count.
static void TradeCodeEntry_DrawButtonCell(s8 buttonRow, bool8 highlighted)
{
    const u8 *text = (buttonRow == TRADE_CODE_ENTRY_BUTTON_BACK) ? sText_Back : sText_Ok;
    u32 cellHeight = TRADE_CODE_ENTRY_CELL_HEIGHT * (TRADE_CODE_ENTRY_GRID_ROWS / 2);
    u32 y = buttonRow * cellHeight + (cellHeight - 8) / 2;

    FillWindowPixelRect(WINDOW_GRID, PIXEL_FILL(TEXT_COLOR_TRANSPARENT), TRADE_CODE_ENTRY_BUTTON_COL_X, buttonRow * cellHeight, TRADE_CODE_ENTRY_BUTTON_COL_WIDTH, cellHeight);
    AddTextPrinterParameterized4(WINDOW_GRID, FONT_SMALL_NARROW, TRADE_CODE_ENTRY_BUTTON_COL_X + 2, y, 0, 0, sTradeCodeEntryFontColors[highlighted ? FONT_COLOR_HIGHLIGHT : FONT_COLOR_NORMAL], TEXT_SKIP_DRAW, text);
    CopyWindowToVram(WINDOW_GRID, COPYWIN_GFX);
}

// Redraws the typed-code field: rawSymbols laid out on the same monospace
// grid TradeCodeDisplay_PrintCode (trade_code_display.c) uses, grouped/
// hyphenated every TRADE_CODE_GROUP_SIZE symbols unless isConfirmMode
// (which prints one ungrouped run - see the payload spec's "Confirm code...
// formatted as one group of 6"). Only shows the last TRADE_CODE_ENTRY_
// VISIBLE_ROWS rows, scrolling forward as symbols are typed past that -
// see TRADE_CODE_ENTRY_VISIBLE_ROWS's own comment for why. Also recomputes
// where the live caret belongs (sTradeCodeEntryDataPtr->caretCol/Row) and
// draws it.
static void TradeCodeEntry_PrintEntryField(void)
{
    struct TradeCodeEntryResources *res = sTradeCodeEntryDataPtr;
    u32 symbolsPerRow = res->isConfirmMode ? TRADE_CODE_CONFIRM_CHARS : TRADE_CODE_DISPLAY_SYMBOLS_PER_ROW;
    u32 maxSymbols = (res->expectedSymbols != 0) ? res->expectedSymbols : TRADE_CODE_ENTRY_MAX_SYMBOLS;
    bool8 hasCaretCell = (res->symbolCount < maxSymbols);
    // The scrolled-into-view window always reaches at least the caret's own
    // row (index symbolCount, one past the last typed symbol), not just the
    // rows containing real symbols - a symbolCount that lands exactly on a
    // row boundary (e.g. 50 with 25 symbols/row) still needs a 3rd row
    // shown for the caret even though only 2 rows have actual symbols in
    // them. Harmless to always include this row even when the field is
    // completely full and there's no real caret cell to draw there (see
    // hasCaretCell below) - it just means the last visible row is blank.
    u32 rowCount = res->symbolCount / symbolsPerRow + 1;
    u32 startRow = (rowCount > TRADE_CODE_ENTRY_VISIBLE_ROWS) ? (rowCount - TRADE_CODE_ENTRY_VISIBLE_ROWS) : 0;
    u32 i;

    FillWindowPixelBuffer(WINDOW_ENTRY, PIXEL_FILL(TEXT_COLOR_TRANSPARENT));

    for (i = 0; i <= res->symbolCount; i++)
    {
        u32 row = i / symbolsPerRow;
        u32 col = i % symbolsPerRow;
        u32 printCol = col;
        u8 ch[2];

        if (!res->isConfirmMode)
            printCol += col / TRADE_CODE_GROUP_SIZE; // leaves room for a hyphen before every group after the first

        if (row < startRow || row >= startRow + TRADE_CODE_ENTRY_VISIBLE_ROWS)
            continue;

        if (i == res->symbolCount)
        {
            // Past the last typed symbol - this is the caret cell, not a
            // character to print. Only a real cell if there's still room
            // to type here.
            if (hasCaretCell)
            {
                res->caretRow = row - startRow;
                res->caretCol = printCol;
            }
            else
            {
                res->caretRow = -1;
            }
            break;
        }

        if (!res->isConfirmMode && col != 0 && col % TRADE_CODE_GROUP_SIZE == 0)
        {
            ch[0] = CHAR_HYPHEN;
            ch[1] = EOS;
            AddTextPrinterParameterized4(WINDOW_ENTRY, FONT_SHORT_NARROW, (printCol - 1) * CODE_CELL_WIDTH, (row - startRow) * CODE_CELL_HEIGHT, 0, 0, sTradeCodeEntryFontColors[FONT_COLOR_NORMAL], TEXT_SKIP_DRAW, ch);
        }

        ch[0] = res->rawSymbols[i];
        ch[1] = EOS;
        AddTextPrinterParameterized4(WINDOW_ENTRY, FONT_SHORT_NARROW, printCol * CODE_CELL_WIDTH, (row - startRow) * CODE_CELL_HEIGHT, 0, 0, sTradeCodeEntryFontColors[FONT_COLOR_NORMAL], TEXT_SKIP_DRAW, ch);
    }

    PutWindowTilemap(WINDOW_ENTRY);
    CopyWindowToVram(WINDOW_ENTRY, COPYWIN_GFX);

    // Restart the blink cycle visible after any edit, so the caret doesn't
    // look like it silently vanished mid-blink.
    res->caretVisible = TRUE;
    res->caretBlinkTimer = 0;
    TradeCodeEntry_DrawCaret();
}

static void TradeCodeEntry_DrawCaret(void)
{
    struct TradeCodeEntryResources *res = sTradeCodeEntryDataPtr;
    u8 ch[2];

    if (res->caretRow < 0)
        return;

    FillWindowPixelRect(WINDOW_ENTRY, PIXEL_FILL(TEXT_COLOR_TRANSPARENT), res->caretCol * CODE_CELL_WIDTH, res->caretRow * CODE_CELL_HEIGHT, CODE_CELL_WIDTH, CODE_CELL_HEIGHT);
    if (res->caretVisible)
    {
        ch[0] = CHAR_UNDERSCORE;
        ch[1] = EOS;
        AddTextPrinterParameterized4(WINDOW_ENTRY, FONT_SHORT_NARROW, res->caretCol * CODE_CELL_WIDTH, res->caretRow * CODE_CELL_HEIGHT, 0, 0, sTradeCodeEntryFontColors[FONT_COLOR_NORMAL], TEXT_SKIP_DRAW, ch);
    }
    CopyWindowToVram(WINDOW_ENTRY, COPYWIN_GFX);
}

// dCol/dRow are always exactly one of {-1, 0, 1}, and never both nonzero in
// the same call - see the D-pad handling in Task_TradeCodeEntryMain.
static void TradeCodeEntry_MoveCursor(s8 dCol, s8 dRow)
{
    struct TradeCodeEntryResources *res = sTradeCodeEntryDataPtr;
    s8 prevCol = res->col;
    s8 prevRow = res->row;
    s8 prevButtonRow = res->buttonRow;

    if (dCol != 0)
    {
        if (res->col < TRADE_CODE_ENTRY_GRID_COLS)
        {
            s8 newCol = res->col + dCol;

            if (newCol < 0)
                return; // already at the left edge of the symbol grid
            else if (newCol >= TRADE_CODE_ENTRY_GRID_COLS)
            {
                // Enter the button column. row is left untouched - it's
                // restored automatically if the player moves back left,
                // since it's simply not used while col == GRID_COLS.
                res->col = TRADE_CODE_ENTRY_GRID_COLS;
                res->buttonRow = (res->row < TRADE_CODE_ENTRY_GRID_ROWS / 2) ? TRADE_CODE_ENTRY_BUTTON_BACK : TRADE_CODE_ENTRY_BUTTON_OK;
            }
            else
            {
                res->col = newCol;
            }
        }
        else if (dCol < 0)
        {
            res->col = TRADE_CODE_ENTRY_GRID_COLS - 1; // leave the button column, back into the grid
        }
        else
        {
            return; // already at the rightmost column (the button column)
        }
    }

    if (dRow != 0)
    {
        if (res->col < TRADE_CODE_ENTRY_GRID_COLS)
        {
            s8 newRow = res->row + dRow;

            if (newRow < 0 || newRow >= TRADE_CODE_ENTRY_GRID_ROWS)
                return;
            res->row = newRow;
        }
        else
        {
            s8 newButtonRow = res->buttonRow + dRow;

            if (newButtonRow < TRADE_CODE_ENTRY_BUTTON_BACK || newButtonRow > TRADE_CODE_ENTRY_BUTTON_OK)
                return;
            res->buttonRow = newButtonRow;
        }
    }

    if (res->col == prevCol && res->row == prevRow && res->buttonRow == prevButtonRow)
        return; // hit an edge above - nothing actually moved

    PlaySE(SE_SELECT);

    if (prevCol < TRADE_CODE_ENTRY_GRID_COLS)
        TradeCodeEntry_DrawGridCell(prevCol, prevRow, FALSE);
    else
        TradeCodeEntry_DrawButtonCell(prevButtonRow, FALSE);

    if (res->col < TRADE_CODE_ENTRY_GRID_COLS)
        TradeCodeEntry_DrawGridCell(res->col, res->row, TRUE);
    else
        TradeCodeEntry_DrawButtonCell(res->buttonRow, TRUE);
}

static void TradeCodeEntry_AppendSymbol(u8 symbolIndex)
{
    struct TradeCodeEntryResources *res = sTradeCodeEntryDataPtr;
    u32 maxSymbols = (res->expectedSymbols != 0) ? res->expectedSymbols : TRADE_CODE_ENTRY_MAX_SYMBOLS;

    if (res->symbolCount >= maxSymbols)
    {
        PlaySE(SE_FAILURE);
        return;
    }

    res->rawSymbols[res->symbolCount++] = TradeCode_AlphabetSymbol(symbolIndex);
    res->rawSymbols[res->symbolCount] = EOS;
    PlaySE(SE_SELECT);
    TradeCodeEntry_PrintEntryField();

    if (res->expectedSymbols != 0 && res->symbolCount == res->expectedSymbols)
        TradeCodeEntry_TrySubmit(); // confirm-mode auto-submit - see trade_code_entry.h
}

static void TradeCodeEntry_DeleteSymbol(void)
{
    struct TradeCodeEntryResources *res = sTradeCodeEntryDataPtr;

    res->symbolCount--;
    res->rawSymbols[res->symbolCount] = EOS;
    PlaySE(SE_SELECT);
    TradeCodeEntry_PrintEntryField();
}

// Shared by the B button and the on-screen BACK cell - "delete", or "back
// out of the screen entirely" if there's nothing left to delete. Nothing in
// the plan doc's own Stage 6 bullet list asks for the latter explicitly,
// but without it there'd be no way to leave this screen at all once
// opened, which would make even the debug-menu round-trip test unable to
// recover from a misentered code. TRADE_CODE_ENTRY_CANCELLED exists for
// exactly this - see trade_code_entry.h.
static void TradeCodeEntry_HandleBack(void)
{
    if (sTradeCodeEntryDataPtr->symbolCount == 0)
        TradeCodeEntry_Finish(TRADE_CODE_ENTRY_CANCELLED);
    else
        TradeCodeEntry_DeleteSymbol();
}

// Decodes the field, optionally runs it past the caller's validator, and
// either finishes the screen (TRADE_CODE_ENTRY_OK) or shows an error and
// clears the field so the player can try again without leaving the screen.
static void TradeCodeEntry_TrySubmit(void)
{
    struct TradeCodeEntryResources *res = sTradeCodeEntryDataPtr;
    struct TradeCodeBits scratch;
    enum TradeCodeStatus codecStatus;
    enum TradeCodeEntryStatus finalStatus;
    u32 outCapacityBits;

    if (res->expectedSymbols != 0 && res->symbolCount != res->expectedSymbols)
    {
        // Fixed-length (confirm) mode: OK/START is a no-op until auto-
        // submit fires on its own - see TradeCodeEntry_AppendSymbol.
        PlaySE(SE_FAILURE);
        return;
    }

    scratch.data = res->scratchBuffer;
    scratch.capacity = TRADE_CODE_ENTRY_SCRATCH_BYTES * 8;
    scratch.bitPos = 0;
    scratch.error = FALSE;

    codecStatus = TradeCode_Decode(res->rawSymbols, &scratch);
    switch (codecStatus)
    {
    case TRADE_CODE_OK:
        finalStatus = TRADE_CODE_ENTRY_OK;
        break;
    case TRADE_CODE_BAD_CHAR:
        finalStatus = TRADE_CODE_ENTRY_BAD_CHAR;
        break;
    default: // TRADE_CODE_TOO_LONG / TRADE_CODE_TOO_SHORT
        finalStatus = TRADE_CODE_ENTRY_WRONG_LENGTH;
        break;
    }

    // Guard the caller's buffer before touching it - TradeCode_Decode's own
    // TRADE_CODE_TOO_LONG check is against *this* screen's scratch buffer,
    // not the caller's outBits, so a caller that (mis)sized outBits smaller
    // than what actually decoded needs its own check here rather than
    // risking memcpy-ing past the end of it below.
    outCapacityBits = res->outBits->capacity;
    if (finalStatus == TRADE_CODE_ENTRY_OK && scratch.capacity > outCapacityBits)
        finalStatus = TRADE_CODE_ENTRY_WRONG_LENGTH;

    if (finalStatus == TRADE_CODE_ENTRY_OK && res->validator != NULL)
        finalStatus = res->validator(&scratch);

    if (finalStatus == TRADE_CODE_ENTRY_OK)
    {
        memcpy(res->outBits->data, scratch.data, (scratch.capacity + 7) / 8);
        res->outBits->capacity = scratch.capacity;
        res->outBits->bitPos = 0;
        res->outBits->error = FALSE;
        TradeCodeEntry_Finish(TRADE_CODE_ENTRY_OK);
    }
    else
    {
        PlaySE(SE_FAILURE);
        res->errorStatus = finalStatus;
        res->symbolCount = 0;
        res->rawSymbols[0] = EOS;
        TradeCodeEntry_PrintEntryField();
        TradeCodeEntry_PrintError();
    }
}

static void TradeCodeEntry_Finish(enum TradeCodeEntryStatus status)
{
    struct TradeCodeEntryResources *res = sTradeCodeEntryDataPtr;

    *res->outStatus = status;
    BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, RGB_BLACK);
    gTasks[res->taskId].func = Task_TradeCodeEntryWaitFadeAndBail;
}

static void Task_TradeCodeEntryMain(u8 taskId)
{
    struct TradeCodeEntryResources *res = sTradeCodeEntryDataPtr;

    if (++res->caretBlinkTimer >= 16)
    {
        res->caretBlinkTimer = 0;
        res->caretVisible = !res->caretVisible;
        TradeCodeEntry_DrawCaret();
    }

    if (JOY_REPEAT(DPAD_LEFT))
        TradeCodeEntry_MoveCursor(-1, 0);
    else if (JOY_REPEAT(DPAD_RIGHT))
        TradeCodeEntry_MoveCursor(1, 0);
    else if (JOY_REPEAT(DPAD_UP))
        TradeCodeEntry_MoveCursor(0, -1);
    else if (JOY_REPEAT(DPAD_DOWN))
        TradeCodeEntry_MoveCursor(0, 1);
    else if (JOY_NEW(A_BUTTON))
    {
        if (res->col < TRADE_CODE_ENTRY_GRID_COLS)
            TradeCodeEntry_AppendSymbol(res->row * TRADE_CODE_ENTRY_GRID_COLS + res->col);
        else if (res->buttonRow == TRADE_CODE_ENTRY_BUTTON_BACK)
            TradeCodeEntry_HandleBack();
        else
            TradeCodeEntry_TrySubmit();
    }
    else if (JOY_NEW(B_BUTTON))
    {
        TradeCodeEntry_HandleBack();
    }
    else if (JOY_NEW(START_BUTTON))
    {
        TradeCodeEntry_TrySubmit();
    }
    // SELECT is deliberately dead - see trade_code_entry.h / the plan
    // doc's Stage 6 bullet list ("to avoid a stray page-swap reflex from
    // the naming screen").
}
