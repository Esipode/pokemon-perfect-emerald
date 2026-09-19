#include "global.h"
#include "trade_code_display.h"
#include "trade_code.h"
#include "bg.h"
#include "decompress.h"
#include "gpu_regs.h"
#include "graphics.h"
#include "malloc.h"
#include "menu.h"
#include "menu_helpers.h"
#include "palette.h"
#include "pokemon.h"
#include "pokemon_icon.h"
#include "scanline_effect.h"
#include "sound.h"
#include "string_util.h"
#include "task.h"
#include "text.h"
#include "window.h"
#include "constants/characters.h"
#include "constants/rgb.h"
#include "constants/songs.h"
#include "constants/species.h"

// Read-only, full-screen trade/confirm code display, modelled on
// src/ui_stat_editor.c's CB2_/Task_-driven pattern. See
// include/trade_code_display.h for the public contract.

struct TradeCodeDisplayResources
{
    MainCallback savedCallback; // where SetMainCallback2 goes once the player presses A
    u8 gfxLoadState;
    u8 monIconSpriteId;
    u16 species;
    bool8 hasMon;         // FALSE for a confirm code - nothing to sanity-check against
    bool8 isConfirmCode;
    u8 codeStr[TRADE_CODE_MAX_CHARS + 1];
    u8 nickname[POKEMON_NAME_LENGTH + 1];
};

enum WindowIds
{
    WINDOW_HEADER,
    WINDOW_MON,
    WINDOW_CODE,
    WINDOW_FOOTER,
};

static EWRAM_DATA struct TradeCodeDisplayResources *sTradeCodeDisplayDataPtr = NULL;
static EWRAM_DATA u8 *sBg1TilemapBuffer = NULL;

static void TradeCodeDisplay_RunSetup(void);
static bool8 TradeCodeDisplay_DoGfxSetup(void);
static bool8 TradeCodeDisplay_InitBgs(void);
static void TradeCodeDisplay_FadeAndBail(void);
static bool8 TradeCodeDisplay_LoadGraphics(void);
static void TradeCodeDisplay_InitWindows(void);
static void TradeCodeDisplay_PrintHeader(void);
static void TradeCodeDisplay_PrintMon(void);
static void TradeCodeDisplay_PrintCode(void);
static void TradeCodeDisplay_PrintFooter(void);
static void Task_TradeCodeDisplayWaitFadeIn(u8 taskId);
static void Task_TradeCodeDisplayMain(u8 taskId);
static void Task_TradeCodeDisplayWaitFadeAndBail(u8 taskId);
static void TradeCodeDisplay_FreeResources(void);

static const struct BgTemplate sTradeCodeDisplayBgTemplates[] =
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

static const struct WindowTemplate sTradeCodeDisplayWindowTemplates[] =
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
    [WINDOW_MON] =
    {
        .bg = 0,
        .tilemapLeft = 6,
        .tilemapTop = 2,
        .width = 20,
        .height = 4,
        .paletteNum = 15,
        .baseBlock = 1 + (28 * 2),
    },
    [WINDOW_CODE] =
    {
        .bg = 0,
        .tilemapLeft = 1,
        .tilemapTop = 6,
        .width = TRADE_CODE_DISPLAY_ROW_CAPACITY,
        .height = TRADE_CODE_DISPLAY_MAX_ROWS * (CODE_CELL_HEIGHT / 8),
        .paletteNum = 15,
        .baseBlock = 1 + (28 * 2) + (20 * 4),
    },
    [WINDOW_FOOTER] =
    {
        .bg = 0,
        .tilemapLeft = 1,
        .tilemapTop = 14,
        .width = 28,
        .height = 4,
        .paletteNum = 15,
        .baseBlock = 1 + (28 * 2) + (20 * 4) + (TRADE_CODE_DISPLAY_ROW_CAPACITY * TRADE_CODE_DISPLAY_MAX_ROWS * (CODE_CELL_HEIGHT / 8)),
    },
    DUMMY_WIN_TEMPLATE
};

// Background for both the offer-code and confirm-code views; isConfirmCode only
// changes the title text. graphics/trade_codes/view_tileset.{png,pal,bin} were
// generated from graphics/trade_codes/bg_view_trade_code.png with an
// exact-tile-match dedup (no flip detection): 37 unique tiles, 9 colors, within
// the 512-tile BG1 charblock and the 16-color palette the LoadPalette(..., 32)
// call below assumes.
static const u32 sTradeCodeDisplayBgTiles[] = INCBIN_U32("graphics/trade_codes/view_tileset.4bpp.smol");
static const u32 sTradeCodeDisplayBgTilemap[] = INCBIN_U32("graphics/trade_codes/view_tileset.bin.smolTM");
static const u16 sTradeCodeDisplayBgPalette[] = INCBIN_U16("graphics/trade_codes/view_tileset.gbapal");

static const u8 sTradeCodeDisplayFontColors[][3] =
{
    {TEXT_COLOR_TRANSPARENT, TEXT_COLOR_WHITE, TEXT_COLOR_DARK_GRAY},
};

static const u8 sText_TitleOffer[]   = _("YOUR TRADE CODE");
static const u8 sText_TitleConfirm[] = _("YOUR CONFIRM CODE");
static const u8 sText_Offering[]     = _("Offering:");
static const u8 sText_Footer[]       = _("Send this to your partner.\nPress A when they have it.");

void TradeCodeDisplay_Init(const u8 *codeStr, u16 species, const u8 *nickname, bool8 isConfirmCode, MainCallback callback)
{
    if ((sTradeCodeDisplayDataPtr = AllocZeroed(sizeof(struct TradeCodeDisplayResources))) == NULL)
    {
        SetMainCallback2(callback);
        return;
    }

    sTradeCodeDisplayDataPtr->gfxLoadState = 0;
    sTradeCodeDisplayDataPtr->savedCallback = callback;
    sTradeCodeDisplayDataPtr->monIconSpriteId = 0xFF;
    sTradeCodeDisplayDataPtr->species = species;
    sTradeCodeDisplayDataPtr->hasMon = (species != SPECIES_NONE);
    sTradeCodeDisplayDataPtr->isConfirmCode = isConfirmCode;

    // Plain StringCopy, not StringCopyN: StringCopyN copies exactly n bytes with
    // no EOS check, so a shorter source would be read past its end. The buffers
    // are sized to the documented maximum (TRADE_CODE_MAX_CHARS /
    // POKEMON_NAME_LENGTH), and that contract is what keeps the copy safe.
    StringCopy(sTradeCodeDisplayDataPtr->codeStr, codeStr);
    if (sTradeCodeDisplayDataPtr->hasMon)
    {
        if (nickname != NULL)
            StringCopy(sTradeCodeDisplayDataPtr->nickname, nickname);
        else
            StringCopy(sTradeCodeDisplayDataPtr->nickname, GetSpeciesName(species));
    }

    SetMainCallback2(TradeCodeDisplay_RunSetup);
}

static void TradeCodeDisplay_RunSetup(void)
{
    while (1)
    {
        if (TradeCodeDisplay_DoGfxSetup() == TRUE)
            break;
    }
}

static void TradeCodeDisplay_MainCB(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    DoScheduledBgTilemapCopiesToVram();
    UpdatePaletteFade();
}

static void TradeCodeDisplay_VBlankCB(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

static bool8 TradeCodeDisplay_DoGfxSetup(void)
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
        if (TradeCodeDisplay_InitBgs())
        {
            sTradeCodeDisplayDataPtr->gfxLoadState = 0;
            gMain.state++;
        }
        else
        {
            TradeCodeDisplay_FadeAndBail();
            return TRUE;
        }
        break;
    case 3:
        if (TradeCodeDisplay_LoadGraphics() == TRUE)
            gMain.state++;
        break;
    case 4:
        if (sTradeCodeDisplayDataPtr->hasMon)
        {
            // Single-mon screen: LoadMonIconPalette suffices (case 1 already ran
            // FreeAllSpritePalettes), unlike ui_stat_editor.c's load-all pair.
            // y=30, not the WINDOW_MON band's center (~40): a 32x32 icon there
            // would overlap WINDOW_CODE (y=48+) below WINDOW_MON (y=16..48).
            LoadMonIconPalette(sTradeCodeDisplayDataPtr->species);
            sTradeCodeDisplayDataPtr->monIconSpriteId = CreateMonIconNoPersonality(sTradeCodeDisplayDataPtr->species, SpriteCallbackDummy, 24, 30, 0);
        }
        gMain.state++;
        break;
    case 5:
        TradeCodeDisplay_InitWindows();
        TradeCodeDisplay_PrintHeader();
        TradeCodeDisplay_PrintMon();
        TradeCodeDisplay_PrintCode();
        TradeCodeDisplay_PrintFooter();
        gMain.state++;
        break;
    case 6:
        CreateTask(Task_TradeCodeDisplayWaitFadeIn, 0);
        BlendPalettes(0xFFFFFFFF, 16, RGB_BLACK);
        gMain.state++;
        break;
    case 7:
        BeginNormalPaletteFade(0xFFFFFFFF, 0, 16, 0, RGB_BLACK);
        gMain.state++;
        break;
    default:
        SetVBlankCallback(TradeCodeDisplay_VBlankCB);
        SetMainCallback2(TradeCodeDisplay_MainCB);
        return TRUE;
    }
    return FALSE;
}

#define try_free(ptr) ({        \
    void ** ptr__ = (void **)&(ptr);   \
    if (*ptr__ != NULL)                \
        Free(*ptr__);                  \
})

static void TradeCodeDisplay_FreeResources(void)
{
    if (sTradeCodeDisplayDataPtr->monIconSpriteId != 0xFF)
    {
        FreeAndDestroyMonIconSprite(&gSprites[sTradeCodeDisplayDataPtr->monIconSpriteId]);
        FreeMonIconPalette(sTradeCodeDisplayDataPtr->species);
    }
    try_free(sTradeCodeDisplayDataPtr);
    try_free(sBg1TilemapBuffer);
    FreeAllWindowBuffers();
}

static void Task_TradeCodeDisplayWaitFadeAndBail(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        SetMainCallback2(sTradeCodeDisplayDataPtr->savedCallback);
        TradeCodeDisplay_FreeResources();
        DestroyTask(taskId);
    }
}

static void TradeCodeDisplay_FadeAndBail(void)
{
    BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, RGB_BLACK);
    CreateTask(Task_TradeCodeDisplayWaitFadeAndBail, 0);
    SetVBlankCallback(TradeCodeDisplay_VBlankCB);
    SetMainCallback2(TradeCodeDisplay_MainCB);
}

static bool8 TradeCodeDisplay_InitBgs(void)
{
    ResetAllBgsCoordinates();
    sBg1TilemapBuffer = Alloc(0x800);
    if (sBg1TilemapBuffer == NULL)
        return FALSE;

    memset(sBg1TilemapBuffer, 0, 0x800);
    ResetBgsAndClearDma3BusyFlags(0);
    InitBgsFromTemplates(0, sTradeCodeDisplayBgTemplates, NELEMS(sTradeCodeDisplayBgTemplates));
    SetBgTilemapBuffer(1, sBg1TilemapBuffer);
    ScheduleBgCopyTilemapToVram(1);
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
    ShowBg(0);
    ShowBg(1);
    ShowBg(2);
    return TRUE;
}

static bool8 TradeCodeDisplay_LoadGraphics(void)
{
    switch (sTradeCodeDisplayDataPtr->gfxLoadState)
    {
    case 0:
        ResetTempTileDataBuffers();
        DecompressAndCopyTileDataToVram(1, sTradeCodeDisplayBgTiles, 0, 0, 0);
        sTradeCodeDisplayDataPtr->gfxLoadState++;
        break;
    case 1:
        if (FreeTempTileDataBuffersIfPossible() != TRUE)
        {
            DecompressDataWithHeaderWram(sTradeCodeDisplayBgTilemap, sBg1TilemapBuffer);
            sTradeCodeDisplayDataPtr->gfxLoadState++;
        }
        break;
    case 2:
        LoadPalette(sTradeCodeDisplayBgPalette, 0, 32);
        sTradeCodeDisplayDataPtr->gfxLoadState++;
        break;
    default:
        sTradeCodeDisplayDataPtr->gfxLoadState = 0;
        return TRUE;
    }
    return FALSE;
}

static void TradeCodeDisplay_InitWindows(void)
{
    InitWindows(sTradeCodeDisplayWindowTemplates);
    DeactivateAllTextPrinters();
    ScheduleBgCopyTilemapToVram(0);
}

static void Task_TradeCodeDisplayWaitFadeIn(u8 taskId)
{
    if (!gPaletteFade.active)
        gTasks[taskId].func = Task_TradeCodeDisplayMain;
}

static void Task_TradeCodeDisplayTurnOff(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        SetMainCallback2(sTradeCodeDisplayDataPtr->savedCallback);
        TradeCodeDisplay_FreeResources();
        DestroyTask(taskId);
    }
}

static void TradeCodeDisplay_PrintHeader(void)
{
    const u8 *title = sTradeCodeDisplayDataPtr->isConfirmCode ? sText_TitleConfirm : sText_TitleOffer;

    FillWindowPixelBuffer(WINDOW_HEADER, PIXEL_FILL(TEXT_COLOR_TRANSPARENT));
    AddTextPrinterParameterized4(WINDOW_HEADER, FONT_NORMAL, 2, 2, 0, 0, sTradeCodeDisplayFontColors[0], TEXT_SKIP_DRAW, title);
    PutWindowTilemap(WINDOW_HEADER);
    CopyWindowToVram(WINDOW_HEADER, 3);
}

static void TradeCodeDisplay_PrintMon(void)
{
    if (!sTradeCodeDisplayDataPtr->hasMon)
        return;

    FillWindowPixelBuffer(WINDOW_MON, PIXEL_FILL(TEXT_COLOR_TRANSPARENT));
    AddTextPrinterParameterized4(WINDOW_MON, FONT_SMALL_NARROW, 2, 2, 0, 0, sTradeCodeDisplayFontColors[0], TEXT_SKIP_DRAW, sText_Offering);
    AddTextPrinterParameterized4(WINDOW_MON, FONT_NORMAL, 2, 16, 0, 0, sTradeCodeDisplayFontColors[0], TEXT_SKIP_DRAW, sTradeCodeDisplayDataPtr->nickname);
    PutWindowTilemap(WINDOW_MON);
    CopyWindowToVram(WINDOW_MON, 3);
}

// Prints the hyphenated codeStr one character at a time onto a strict
// CODE_CELL_WIDTH x CODE_CELL_HEIGHT grid, wrapping every
// TRADE_CODE_DISPLAY_GROUPS_PER_ROW groups. The hyphen at a row boundary
// triggers the wrap but is not printed; hyphens inside a row are.
static void TradeCodeDisplay_PrintCode(void)
{
    const u8 *src = sTradeCodeDisplayDataPtr->codeStr;
    u32 row = 0;
    u32 col = 0;
    u32 symbolsInRow = 0;
    u8 ch[2];

    ch[1] = EOS;

    FillWindowPixelBuffer(WINDOW_CODE, PIXEL_FILL(TEXT_COLOR_TRANSPARENT));

    while (*src != EOS && row < TRADE_CODE_DISPLAY_MAX_ROWS)
    {
        u8 c = *src;

        if (c == CHAR_HYPHEN && symbolsInRow >= TRADE_CODE_DISPLAY_SYMBOLS_PER_ROW)
        {
            row++;
            col = 0;
            symbolsInRow = 0;
            src++;
            continue;
        }

        ch[0] = c;
        AddTextPrinterParameterized4(WINDOW_CODE, FONT_SHORT_NARROW, col * CODE_CELL_WIDTH, row * CODE_CELL_HEIGHT, 0, 0, sTradeCodeDisplayFontColors[0], TEXT_SKIP_DRAW, ch);
        col++;
        if (c != CHAR_HYPHEN)
            symbolsInRow++;
        src++;
    }

    PutWindowTilemap(WINDOW_CODE);
    CopyWindowToVram(WINDOW_CODE, 3);
}

static void TradeCodeDisplay_PrintFooter(void)
{
    FillWindowPixelBuffer(WINDOW_FOOTER, PIXEL_FILL(TEXT_COLOR_TRANSPARENT));
    AddTextPrinterParameterized4(WINDOW_FOOTER, FONT_NORMAL, 2, 2, 0, 0, sTradeCodeDisplayFontColors[0], TEXT_SKIP_DRAW, sText_Footer);
    PutWindowTilemap(WINDOW_FOOTER);
    CopyWindowToVram(WINDOW_FOOTER, 3);
}

static void Task_TradeCodeDisplayMain(u8 taskId)
{
    if (JOY_NEW(A_BUTTON))
    {
        PlaySE(SE_SELECT);
        BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, RGB_BLACK);
        gTasks[taskId].func = Task_TradeCodeDisplayTurnOff;
    }
}
