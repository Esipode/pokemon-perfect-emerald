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
    // One shared single-line window, not two - see TRADE_CODE_ENTRY_
    // VISIBLE_ROWS's comment for why there's no tile budget left for a
    // separate footer hint and error banner. Shows the control hint by
    // default; a failed submit swaps it to that failure's canned message
    // until the player edits the field again (TradeCodeEntry_PrintMessage).
    WINDOW_MESSAGE,
};

// All 32 Base32/Crockford symbols on one page, 8x4 - see trade_code_entry.h
// and the plan doc's own Stage 6 bullet list for why this isn't a paged
// naming_screen.c-style keyboard.
#define TRADE_CODE_ENTRY_GRID_COLS 8
#define TRADE_CODE_ENTRY_GRID_ROWS 4
#define TRADE_CODE_ENTRY_CELL_WIDTH  16
// 16, matching CODE_CELL_HEIGHT (trade_code_display.h) - *every* font in
// this engine renders at a real height of 16px (two stacked 8px tiles),
// regardless of what its .maxLetterHeight metric in src/text.c's font table
// claims (FONT_SMALL_NARROW says 8; DecompressGlyph_SmallNarrow still
// writes both gCurGlyph.gfxBufferTop *and* gfxBufferBottom for every glyph -
// checked before trusting the metric). An earlier draft of this file set
// this to 8 to save vertical space and it silently clipped the bottom half
// of every grid glyph - the maxLetterHeight field is not glyph pixel
// height, whatever else it's used for.
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
// Centers the grid+BACK/OK window within the same 28-tile content band
// (columns 1-28) every other window on this screen uses, rather than
// left-aligning it to column 1 like they do - the grid's own content is
// narrower than the full band, so left-aligning it left a lopsided gap on
// the right.
#define TRADE_CODE_ENTRY_GRID_LEFT (1 + (28 - TRADE_CODE_ENTRY_GRID_WIDTH_TILES) / 2)

#define TRADE_CODE_ENTRY_BUTTON_BACK 0
#define TRADE_CODE_ENTRY_BUTTON_OK   1

// The typed-code field mirrors Stage 5's grid (TRADE_CODE_DISPLAY_* /
// CODE_CELL_* from trade_code_display.h - same cell size, same grouping),
// and shows every row Stage 5's own display screen could ever need
// (TRADE_CODE_DISPLAY_MAX_ROWS) rather than a scrolled-down subset - so a
// player typing the worst-case ~86-symbol code never has an earlier group
// scrolled out of view behind the caret. What pays for this: at 16px/line
// (see TRADE_CODE_ENTRY_CELL_HEIGHT), a full 4-row field plus the full 8x4
// keyboard plus a header already spend the entire 512-tile budget BG0's
// windows have to themselves (charBaseIndex 0 - confirmed unshared with
// the UI background art, which loads into BG1's own charBaseIndex 3
// instead; see TradeCodeEntry_InitBgs/_LoadGraphics), leaving no separate
// room for a footer hint AND an error banner - see WINDOW_MESSAGE, which
// folds those two into one shared single-line window instead of trimming
// this field back down. The scroll-window math in TradeCodeEntry_
// PrintEntryField is still written generally (startRow can still be > 0)
// rather than assuming this equality forever - harmless dead weight today,
// a safety net if TRADE_CODE_ENTRY_MAX_SYMBOLS ever grows past what
// TRADE_CODE_DISPLAY_MAX_ROWS rows hold.
#define TRADE_CODE_ENTRY_VISIBLE_ROWS TRADE_CODE_DISPLAY_MAX_ROWS
// The live caret is a solid bar along a cell's bottom edge (see
// TradeCodeEntry_DrawCaret), not a text glyph - this is its thickness.
#define TRADE_CODE_ENTRY_CARET_BAR_HEIGHT 2
// Nudges the typed-code field's rows down within WINDOW_ENTRY, purely
// cosmetic (requested after the header got the same treatment - see
// TradeCodeEntry_PrintHeader). WINDOW_ENTRY's own height has zero spare
// (TRADE_CODE_ENTRY_VISIBLE_ROWS rows at 16px/row exactly fill it, no
// margin - see that macro's comment), so this borrows the bottom-most
// pixels of the last visible row's own cell instead of growing the window.
// Confirmed on hardware: 6px was one step too far - it cut the bottom 2px
// off the last visible row (the glyphs' own top-aligned blank space in
// their cell absorbed the first 4px of overflow harmlessly, then real ink
// started getting clipped). Back down to 4px, the value that overflow data
// point implies is exactly the safe ceiling here (4px eaten by blank space,
// 0px of real ink cut) - do not raise this again without also growing
// WINDOW_ENTRY's height, which has no spare tile-row budget to give (see
// this file's own status notes on the 512-tile charblock).
#define TRADE_CODE_ENTRY_FIELD_TOP_PADDING 4
// Nudges the typed-code field 2px left, purely cosmetic. Unlike the top
// padding above, this can't be a flat per-column offset: column 0 already
// sits flush against WINDOW_ENTRY's own left edge (x=0), and every
// AddTextPrinterParameterized4/FillWindowPixelRect x argument here is a u16
// - passing a literal -2 for column 0 wouldn't clip harmlessly the way the
// vertical overflow did, it'd wrap to a huge unsigned value and try to draw
// far outside the window's buffer. TRADE_CODE_ENTRY_FIELD_X saturates at 0
// instead: every column from 1 up shifts the full 2px (and only gets safer
// against right-edge overflow by moving left, so no width/tile-budget
// consequences there), while column 0 stays put since there's nowhere left
// for it to go - a real, hard architectural floor, not an oversight.
#define TRADE_CODE_ENTRY_FIELD_LEFT_SHIFT 2
#define TRADE_CODE_ENTRY_FIELD_X(col) \
    (((col) * CODE_CELL_WIDTH > TRADE_CODE_ENTRY_FIELD_LEFT_SHIFT) \
        ? ((col) * CODE_CELL_WIDTH - TRADE_CODE_ENTRY_FIELD_LEFT_SHIFT) \
        : 0)

enum TradeCodeEntryFontColor
{
    FONT_COLOR_NORMAL,
    FONT_COLOR_HIGHLIGHT,
    FONT_COLOR_CODE, // the typed-code field's own text - see its own declaration comment
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
static void TradeCodeEntry_PrintMessage(void);
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
        .tilemapLeft = TRADE_CODE_ENTRY_GRID_LEFT,
        .tilemapTop = 10,
        .width = TRADE_CODE_ENTRY_GRID_WIDTH_TILES,
        .height = TRADE_CODE_ENTRY_GRID_HEIGHT_TILES,
        .paletteNum = 15,
        .baseBlock = 1 + (28 * 2) + (TRADE_CODE_DISPLAY_ROW_CAPACITY * TRADE_CODE_ENTRY_VISIBLE_ROWS * (CODE_CELL_HEIGHT / 8)),
    },
    [WINDOW_MESSAGE] =
    {
        .bg = 0,
        .tilemapLeft = 1,
        .tilemapTop = 18,
        // 27, not 28 like the other full-width windows - header (56) +
        // entry (232) + grid (168) + a 28-wide message window (56) would
        // sum to exactly 512 tiles, and starting from baseBlock 1 that
        // reaches tile index 512 - one past the last valid index (0-511)
        // in a 512-tile 4bpp charblock. Trimmed by 1 tile of width (2
        // tiles of budget, since height is 2) for real margin instead of
        // landing exactly on the boundary; the longest message here is
        // still comfortably under 27 tiles' worth of characters.
        .width = 27,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 1 + (28 * 2) + (TRADE_CODE_DISPLAY_ROW_CAPACITY * TRADE_CODE_ENTRY_VISIBLE_ROWS * (CODE_CELL_HEIGHT / 8)) + (TRADE_CODE_ENTRY_GRID_WIDTH_TILES * TRADE_CODE_ENTRY_GRID_HEIGHT_TILES),
    },
    DUMMY_WIN_TEMPLATE
};

// Purpose-built background for this screen (used for both the offer-code
// and confirm-code entry modes - see TradeCodeEntry_Init's expectedSymbols
// param, which only changes field layout/behavior, not the art). See the
// matching comment on sTradeCodeDisplayBgTiles (src/trade_code_display.c)
// for how graphics/trade_codes/enter_tileset.{png,pal,bin} were produced
// from the source mockup graphics/trade_codes/bg_enter_trade_code.png - the
// same dedup process, round-trip-verified the same way. 23 unique tiles, 9
// colors, well inside the BG1 charblock and the LoadPalette(..., 32) call
// below.
static const u32 sTradeCodeEntryBgTiles[] = INCBIN_U32("graphics/trade_codes/enter_tileset.4bpp.smol");
static const u32 sTradeCodeEntryBgTilemap[] = INCBIN_U32("graphics/trade_codes/enter_tileset.bin.smolTM");
static const u16 sTradeCodeEntryBgPalette[] = INCBIN_U16("graphics/trade_codes/enter_tileset.gbapal");

// FONT_COLOR_HIGHLIGHT and FONT_COLOR_CODE both reuse the same palette bank
// (paletteNum 15, loaded from the shared background_tileset palette) as
// FONT_COLOR_NORMAL - only the foreground/shadow TEXT_COLOR_* indices
// differ, so these are same-window recolors, not separate palette loads.
// FONT_COLOR_CODE is requested black text for the typed-code field
// specifically (WINDOW_ENTRY) - there's no TEXT_COLOR_BLACK in this
// engine's palette (constants/characters.h only goes up to LIGHT_BLUE),
// so TEXT_COLOR_DARK_GRAY is the closest available foreground, paired with
// a light shadow (the reverse of every other window's white-on-dark
// pairing) so the glyphs still read against whatever's behind them.
static const u8 sTradeCodeEntryFontColors[][3] =
{
    [FONT_COLOR_NORMAL]    = {TEXT_COLOR_TRANSPARENT, TEXT_COLOR_WHITE, TEXT_COLOR_DARK_GRAY},
    [FONT_COLOR_HIGHLIGHT] = {TEXT_COLOR_TRANSPARENT, TEXT_COLOR_GREEN, TEXT_COLOR_DARK_GRAY},
    [FONT_COLOR_CODE]      = {TEXT_COLOR_TRANSPARENT, TEXT_COLOR_DARK_GRAY, TEXT_COLOR_LIGHT_GRAY},
};

static const u8 sText_TitleOffer[]   = _("ENTER TRADE CODE");
static const u8 sText_TitleConfirm[] = _("ENTER CONFIRM CODE");
static const u8 sText_Hint[]         = _("A: Select  B: Delete  START: Submit");
static const u8 sText_Back[]         = _("BACK");
static const u8 sText_Ok[]           = _("OK");

// Canned messages for WINDOW_MESSAGE (see its own declaration comment) -
// single line only, no \n. Cut down from the plan doc's own Stage 6 quoted
// wording to fit: at this font's real 16px line height (see TRADE_CODE_
// ENTRY_CELL_HEIGHT's comment) this window has room for exactly one line,
// not the two or three a first draft assumed. WRONG_LENGTH covers both
// TRADE_CODE_TOO_SHORT and TRADE_CODE_TOO_LONG (see TradeCodeEntry_
// TrySubmit) - the player-facing advice is the same either way. BAD_CHAR is
// unreachable through the on-screen keyboard alone (see trade_code_entry.h)
// but still needs text for parity with TradeCode_Decode's own status enum.
static const u8 sText_ErrorBadChar[]      = _("That code has an invalid character.");
static const u8 sText_ErrorWrongLength[]  = _("Wrong length - check for typos.");
static const u8 sText_ErrorInvalid[]      = _("This code isn't valid, or isn't yours.");
static const u8 sText_ErrorAlreadyUsed[]  = _("This code was already used.");
static const u8 sText_ErrorWrongVersion[] = _("This code is from a different version.");

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
        TradeCodeEntry_PrintMessage();
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
    AddTextPrinterParameterized4(WINDOW_HEADER, FONT_NORMAL, 2, 0, 0, 0, sTradeCodeEntryFontColors[FONT_COLOR_NORMAL], TEXT_SKIP_DRAW, title);
    PutWindowTilemap(WINDOW_HEADER);
    CopyWindowToVram(WINDOW_HEADER, COPYWIN_FULL);
}

// Shows the control hint by default, or - when sTradeCodeEntryDataPtr->
// errorStatus is set - that status's canned message instead (see
// sTradeCodeEntryErrorText). Single line only; see WINDOW_MESSAGE's own
// declaration comment for why this replaced two separate windows. Called
// from Init (hint), TradeCodeEntry_TrySubmit's failure branch (sets
// errorStatus first, then this), and TradeCodeEntry_AppendSymbol/
// _DeleteSymbol (clear errorStatus back to OK first, so editing the field
// after a failure silently reverts the message to the hint rather than
// leaving a stale error up).
static void TradeCodeEntry_PrintMessage(void)
{
    struct TradeCodeEntryResources *res = sTradeCodeEntryDataPtr;
    const u8 *text = (res->errorStatus == TRADE_CODE_ENTRY_OK) ? sText_Hint : sTradeCodeEntryErrorText[res->errorStatus];

    FillWindowPixelBuffer(WINDOW_MESSAGE, PIXEL_FILL(TEXT_COLOR_TRANSPARENT));
    AddTextPrinterParameterized4(WINDOW_MESSAGE, FONT_SMALL_NARROW, 2, 2, 0, 0, sTradeCodeEntryFontColors[FONT_COLOR_NORMAL], TEXT_SKIP_DRAW, text);
    PutWindowTilemap(WINDOW_MESSAGE);
    CopyWindowToVram(WINDOW_MESSAGE, COPYWIN_FULL);
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
            AddTextPrinterParameterized4(WINDOW_ENTRY, FONT_SHORT_NARROW, TRADE_CODE_ENTRY_FIELD_X(printCol - 1), (row - startRow) * CODE_CELL_HEIGHT + TRADE_CODE_ENTRY_FIELD_TOP_PADDING, 0, 0, sTradeCodeEntryFontColors[FONT_COLOR_CODE], TEXT_SKIP_DRAW, ch);
        }

        ch[0] = res->rawSymbols[i];
        ch[1] = EOS;
        AddTextPrinterParameterized4(WINDOW_ENTRY, FONT_SHORT_NARROW, TRADE_CODE_ENTRY_FIELD_X(printCol), (row - startRow) * CODE_CELL_HEIGHT + TRADE_CODE_ENTRY_FIELD_TOP_PADDING, 0, 0, sTradeCodeEntryFontColors[FONT_COLOR_CODE], TEXT_SKIP_DRAW, ch);
    }

    PutWindowTilemap(WINDOW_ENTRY);
    CopyWindowToVram(WINDOW_ENTRY, COPYWIN_GFX);

    // Restart the blink cycle visible after any edit, so the caret doesn't
    // look like it silently vanished mid-blink.
    res->caretVisible = TRUE;
    res->caretBlinkTimer = 0;
    TradeCodeEntry_DrawCaret();
}

// Drawn as a solid bar along the cell's bottom edge, not a text glyph -
// CHAR_UNDERSCORE (constants/characters.h) turned out to only mean
// "underscore" inside the separate CHAR_EXTRA_SYMBOL glyph table; under the
// normal font table that this window actually prints with, that same byte
// value is CHAR_I_GRAVE ("i" with a grave accent), which is what was
// showing up instead. A plain pixel-filled rectangle sidesteps the whole
// charmap/glyph-table question - there's no character to misinterpret.
static void TradeCodeEntry_DrawCaret(void)
{
    struct TradeCodeEntryResources *res = sTradeCodeEntryDataPtr;

    if (res->caretRow < 0)
        return;

    FillWindowPixelRect(WINDOW_ENTRY, PIXEL_FILL(TEXT_COLOR_TRANSPARENT), TRADE_CODE_ENTRY_FIELD_X(res->caretCol), res->caretRow * CODE_CELL_HEIGHT + TRADE_CODE_ENTRY_FIELD_TOP_PADDING, CODE_CELL_WIDTH, CODE_CELL_HEIGHT);
    if (res->caretVisible)
    {
        FillWindowPixelRect(WINDOW_ENTRY, PIXEL_FILL(TEXT_COLOR_WHITE),
                             TRADE_CODE_ENTRY_FIELD_X(res->caretCol),
                             res->caretRow * CODE_CELL_HEIGHT + TRADE_CODE_ENTRY_FIELD_TOP_PADDING + (CODE_CELL_HEIGHT - TRADE_CODE_ENTRY_CARET_BAR_HEIGHT),
                             CODE_CELL_WIDTH, TRADE_CODE_ENTRY_CARET_BAR_HEIGHT);
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
    // Editing the field silently clears any error banner WINDOW_MESSAGE was
    // showing back to the control hint, rather than leaving a stale
    // failure message up while the player retypes - see TradeCodeEntry_
    // PrintMessage's own comment.
    if (res->errorStatus != TRADE_CODE_ENTRY_OK)
    {
        res->errorStatus = TRADE_CODE_ENTRY_OK;
        TradeCodeEntry_PrintMessage();
    }

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
    if (res->errorStatus != TRADE_CODE_ENTRY_OK)
    {
        res->errorStatus = TRADE_CODE_ENTRY_OK;
        TradeCodeEntry_PrintMessage();
    }
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
        TradeCodeEntry_PrintMessage();
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
