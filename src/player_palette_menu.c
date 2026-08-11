#include "global.h"
#include "player_palette_menu.h"
#include "bg.h"
#include "event_object_movement.h"
#include "field_player_avatar.h"
#include "gpu_regs.h"
#include "international_string_util.h"
#include "list_menu.h"
#include "main.h"
#include "palette.h"
#include "player_customization.h"
#include "scanline_effect.h"
#include "sound.h"
#include "sprite.h"
#include "string_util.h"
#include "task.h"
#include "text.h"
#include "text_window.h"
#include "window.h"
#include "constants/characters.h"
#include "constants/event_object_movement.h"
#include "constants/rgb.h"
#include "constants/songs.h"

// See Customization.md, Stage 3. Structure mirrors src/new_game_settings_menu.c
// (gMain.state setup machine -> FadeIn -> ProcessInput -> FadeOut, ListMenu
// for row/cursor handling) and reuses src/option_menu.c's bracket-value idiom
// for the HUE/SHADE column, but printed as one literal "< 03 >" string rather
// than separately flanking chevrons -- simpler, and there's no ROM-build
// loop in this environment to tune pixel-perfect chevron centering against,
// so this favours the lower-risk option. Geometry below is a first pass;
// nudge it once it's actually on screen.

enum
{
    WIN_HEADER,
    WIN_LIST,
    WIN_SWATCH,
};

// Rows 0..PLAYER_COLOR_REGION_COUNT-1 line up 1:1 with enum PlayerColorRegion
// (HAIR, HAT, OUTFIT, ACCENT in that order) so a chosen/selected row index
// can be used directly as a region index without a translation table.
enum
{
    ROW_RESET = PLAYER_COLOR_REGION_COUNT,
    ROW_CONFIRM,
    ROW_COUNT,
};

#define tListTaskId data[0]

#define PALETTE_MENU_LABEL_X 8
#define PALETTE_MENU_VALUE_X 84

#define SWATCH_TILEMAP_LEFT 9
#define SWATCH_SIZE          8
#define SWATCH_X              4
#define SWATCH_Y_OFFSET        4

#define PREVIEW_SPRITE_X 208
#define PREVIEW_SPRITE_Y  72

static void Task_PaletteMenuFadeIn(u8 taskId);
static void Task_PaletteMenuProcessInput(u8 taskId);
static void Task_PaletteMenuFadeOut(u8 taskId);
static void PaletteMenu_MoveCursorCallback(s32 itemIndex, bool8 onInit, struct ListMenu *list);
static void PaletteMenu_ItemPrintCallback(u8 windowId, u32 itemId, u8 y);
static void AdjustRegionValue(enum PlayerColorRegion region, bool8 increase);
static void RedrawSwatches(void);
static void RefreshPreviewPalette(void);
static void DrawHeaderText(void);
static void DrawBgWindowFrames(void);

// Working copy of gSaveBlock2Ptr->playerColors -- only committed back on
// CONFIRM, so RESET/B can freely discard it. axisIsShade[] remembers which
// axis (HUE/SHADE) each of the four colour rows is currently showing,
// per-row, so e.g. HAIR can sit on HUE while HAT sits on SHADE at once.
static EWRAM_DATA struct
{
    u8 choices[PLAYER_COLOR_REGION_COUNT];
    bool8 axisIsShade[PLAYER_COLOR_REGION_COUNT];
    u8 gender;
    u8 previewSpriteId;
} sPaletteMenu = {0};

static const u8 sText_Title[] = _("PLAYER COLOURS");
static const u8 sText_ControlHint[] = _("{SELECT_BUTTON}AXIS {A_BUTTON}OK {B_BUTTON}BACK");

static const u8 sText_Hue[]   = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}HUE   < ");
static const u8 sText_Shade[] = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}SHADE < ");
static const u8 sText_ChevronClose[] = _(" >");

static const struct ListMenuItem sPaletteMenuItems[ROW_COUNT] =
{
    [PLAYER_COLOR_REGION_HAIR]   = {COMPOUND_STRING("HAIR"),   PLAYER_COLOR_REGION_HAIR},
    [PLAYER_COLOR_REGION_HAT]    = {COMPOUND_STRING("HAT"),    PLAYER_COLOR_REGION_HAT},
    [PLAYER_COLOR_REGION_OUTFIT] = {COMPOUND_STRING("OUTFIT"), PLAYER_COLOR_REGION_OUTFIT},
    [PLAYER_COLOR_REGION_ACCENT] = {COMPOUND_STRING("ACCENT"), PLAYER_COLOR_REGION_ACCENT},
    [ROW_RESET]                  = {COMPOUND_STRING("RESET TO DEFAULT"), ROW_RESET},
    [ROW_CONFIRM]                = {COMPOUND_STRING("CONFIRM"), ROW_CONFIRM},
};

static const struct WindowTemplate sPaletteMenuWinTemplates[] =
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
        .width = 17,
        .height = ROW_COUNT * 2,
        .paletteNum = 1,
        .baseBlock = 0x36
    },
    // Overlaps a column inside WIN_LIST's own rect (rows 0-3 only) so the
    // swatches can use their own palette bank (2) instead of the text
    // palette (1) -- see RedrawSwatches(). Its PutWindowTilemap() call runs
    // after WIN_LIST's at init, so it owns that column's screen tiles for
    // good; WIN_LIST is never re-PutWindowTilemap'd afterwards, only
    // COPYWIN_GFX-refreshed, so the two never fight over it again.
    [WIN_SWATCH] = {
        .bg = 0,
        .tilemapLeft = SWATCH_TILEMAP_LEFT,
        .tilemapTop = 5,
        .width = 2,
        .height = PLAYER_COLOR_REGION_COUNT * 2,
        .paletteNum = 2,
        .baseBlock = 0x36 + 17 * (ROW_COUNT * 2)
    },
    DUMMY_WIN_TEMPLATE
};

static const struct BgTemplate sPaletteMenuBgTemplates[] =
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

static const u16 sPaletteMenuBg_Pal[] = {RGB(17, 18, 31)};
static const u16 sPaletteMenuText_Pal[] = INCGFX_U16("graphics/interface/option_menu_text.pal", ".gbapal");

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

void CB2_InitPlayerPaletteMenu(void)
{
    u8 taskId;
    switch (gMain.state)
    {
    default:
    case 0:
        SetVBlankCallback(NULL);
        sPaletteMenu.gender = gSaveBlock2Ptr->playerGender;
        memcpy(sPaletteMenu.choices, gSaveBlock2Ptr->playerColors, sizeof(sPaletteMenu.choices));
        memset(sPaletteMenu.axisIsShade, 0, sizeof(sPaletteMenu.axisIsShade));
        gMain.state++;
        break;
    case 1:
        DmaClearLarge16(3, (void *)(VRAM), VRAM_SIZE, 0x1000);
        DmaClear32(3, OAM, OAM_SIZE);
        DmaClear16(3, PLTT, PLTT_SIZE);
        SetGpuReg(REG_OFFSET_DISPCNT, 0);
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(0, sPaletteMenuBgTemplates, ARRAY_COUNT(sPaletteMenuBgTemplates));
        ChangeBgX(0, 0, BG_COORD_SET);
        ChangeBgY(0, 0, BG_COORD_SET);
        ChangeBgX(1, 0, BG_COORD_SET);
        ChangeBgY(1, 0, BG_COORD_SET);
        InitWindows(sPaletteMenuWinTemplates);
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
        LoadPalette(sPaletteMenuBg_Pal, BG_PLTT_ID(0), sizeof(sPaletteMenuBg_Pal));
        LoadPalette(GetWindowFrameTilesPal(gSaveBlock2Ptr->optionsWindowFrameType)->pal, BG_PLTT_ID(7), PLTT_SIZE_4BPP);
        gMain.state++;
        break;
    case 5:
        LoadPalette(sPaletteMenuText_Pal, BG_PLTT_ID(1), sizeof(sPaletteMenuText_Pal));
        gMain.state++;
        break;
    case 6:
        PutWindowTilemap(WIN_HEADER);
        DrawHeaderText();
        gMain.state++;
        break;
    case 7:
        PutWindowTilemap(WIN_LIST);
        PutWindowTilemap(WIN_SWATCH);
        CopyBgTilemapBufferToVram(0);
        gMain.state++;
        break;
    case 8:
        DrawBgWindowFrames();
        gMain.state++;
        break;
    case 9:
    {
        struct ListMenuTemplate template = {0};

        template.items = sPaletteMenuItems;
        template.moveCursorFunc = PaletteMenu_MoveCursorCallback;
        template.itemPrintFunc = PaletteMenu_ItemPrintCallback;
        template.totalItems = ROW_COUNT;
        template.maxShowed = ROW_COUNT;
        template.windowId = WIN_LIST;
        template.header_X = 0;
        template.item_X = PALETTE_MENU_LABEL_X;
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

        sPaletteMenu.previewSpriteId = CreateObjectGraphicsSprite(
            GetPlayerAvatarGraphicsIdByStateIdAndGender(PLAYER_AVATAR_STATE_NORMAL, sPaletteMenu.gender),
            SpriteCallbackDummy, PREVIEW_SPRITE_X, PREVIEW_SPRITE_Y, 0);
        StartSpriteAnim(&gSprites[sPaletteMenu.previewSpriteId], ANIM_STD_GO_SOUTH);

        taskId = CreateTask(Task_PaletteMenuFadeIn, 0);
        gTasks[taskId].tListTaskId = ListMenuInit(&template, 0, 0);
        RefreshPreviewPalette();
        CopyWindowToVram(WIN_LIST, COPYWIN_GFX);
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

static void Task_PaletteMenuFadeIn(u8 taskId)
{
    if (!gPaletteFade.active)
        gTasks[taskId].func = Task_PaletteMenuProcessInput;
}

static void Task_PaletteMenuProcessInput(u8 taskId)
{
    s32 itemId = ListMenu_ProcessInput(gTasks[taskId].tListTaskId);
    u16 selectedRow;
    ListMenuGetScrollAndRow(gTasks[taskId].tListTaskId, NULL, &selectedRow);

    switch (itemId)
    {
    case LIST_NOTHING_CHOSEN:
        if (selectedRow < PLAYER_COLOR_REGION_COUNT)
        {
            enum PlayerColorRegion region = (enum PlayerColorRegion)selectedRow;

            if (JOY_NEW(SELECT_BUTTON))
            {
                PlaySE(SE_SELECT);
                sPaletteMenu.axisIsShade[region] ^= 1;
                RedrawListMenu(gTasks[taskId].tListTaskId);
                CopyWindowToVram(WIN_LIST, COPYWIN_GFX);
            }
            else if (JOY_NEW(DPAD_LEFT | DPAD_RIGHT))
            {
                PlaySE(SE_SELECT);
                AdjustRegionValue(region, JOY_NEW(DPAD_RIGHT));
                RedrawListMenu(gTasks[taskId].tListTaskId);
                CopyWindowToVram(WIN_LIST, COPYWIN_GFX);
                RefreshPreviewPalette();
            }
        }
        break;
    case LIST_CANCEL: // B -- discard the working copy and exit
        PlaySE(SE_SELECT);
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        gTasks[taskId].func = Task_PaletteMenuFadeOut;
        break;
    case ROW_RESET:
        PlaySE(SE_SELECT);
        memset(sPaletteMenu.choices, 0, sizeof(sPaletteMenu.choices));
        RedrawListMenu(gTasks[taskId].tListTaskId);
        CopyWindowToVram(WIN_LIST, COPYWIN_GFX);
        RefreshPreviewPalette();
        break;
    case ROW_CONFIRM:
        PlaySE(SE_SELECT);
        memcpy(gSaveBlock2Ptr->playerColors, sPaletteMenu.choices, sizeof(sPaletteMenu.choices));
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        gTasks[taskId].func = Task_PaletteMenuFadeOut;
        break;
    default:
        // A on one of the four colour rows -- nothing to do, those are
        // edited with Left/Right (value) and SELECT (axis), not A.
        break;
    }
}

static void Task_PaletteMenuFadeOut(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        DestroyListMenuTask(gTasks[taskId].tListTaskId, NULL, NULL);
        DestroySprite(&gSprites[sPaletteMenu.previewSpriteId]);
        DestroyTask(taskId);
        FreeAllWindowBuffers();
        SetMainCallback2(gMain.savedCallback);
    }
}

static void PaletteMenu_MoveCursorCallback(s32 itemIndex, bool8 onInit, struct ListMenu *list)
{
    if (!onInit)
        PlaySE(SE_SELECT);
}

// hue: low nibble, 0-15. shade: high nibble, signed PLAYER_COLOR_SHADE_MIN..
// PLAYER_COLOR_SHADE_MAX. Same packing as src/player_customization.c's
// (private) UnpackColorByte/PackColorByte -- duplicated here rather than
// exposed, since only the encoding (documented on gSaveBlock2Ptr->
// playerColors in include/player_customization.h) needs to be shared, not
// the helper functions.
static void UnpackChoice(enum PlayerColorRegion region, u8 *hue, s8 *shade)
{
    u8 raw = sPaletteMenu.choices[region];
    s8 s = (raw >> 4) & 0xF;

    if (s > 7)
        s -= 16;
    *hue = raw & 0xF;
    *shade = s;
}

static void AdjustRegionValue(enum PlayerColorRegion region, bool8 increase)
{
    u8 hue;
    s8 shade;

    UnpackChoice(region, &hue, &shade);

    if (sPaletteMenu.axisIsShade[region])
    {
        if (increase)
            shade = (shade < PLAYER_COLOR_SHADE_MAX) ? shade + 1 : PLAYER_COLOR_SHADE_MIN;
        else
            shade = (shade > PLAYER_COLOR_SHADE_MIN) ? shade - 1 : PLAYER_COLOR_SHADE_MAX;
    }
    else
    {
        if (increase)
            hue = (hue + 1 < PLAYER_COLOR_HUE_COUNT) ? hue + 1 : 0;
        else
            hue = (hue > 0) ? hue - 1 : PLAYER_COLOR_HUE_COUNT - 1;
    }

    sPaletteMenu.choices[region] = (hue & 0xF) | ((shade & 0xF) << 4);
}

static void PaletteMenu_ItemPrintCallback(u8 windowId, u32 itemId, u8 y)
{
    u8 hue;
    s8 shade;
    u8 text[24];
    u8 *ptr;

    if (itemId >= PLAYER_COLOR_REGION_COUNT)
        return; // RESET/CONFIRM rows have no HUE/SHADE column

    UnpackChoice((enum PlayerColorRegion)itemId, &hue, &shade);

    ptr = StringCopy(text, sPaletteMenu.axisIsShade[itemId] ? sText_Shade : sText_Hue);
    if (sPaletteMenu.axisIsShade[itemId])
    {
        *ptr++ = (shade < 0) ? CHAR_HYPHEN : CHAR_PLUS;
        ptr = ConvertIntToDecimalStringN(ptr, (shade < 0) ? -shade : shade, STR_CONV_MODE_LEFT_ALIGN, 1);
    }
    else
    {
        ptr = ConvertIntToDecimalStringN(ptr, hue, STR_CONV_MODE_LEADING_ZEROS, 2);
    }
    StringCopy(ptr, sText_ChevronClose);

    AddTextPrinterParameterized(windowId, FONT_NORMAL, text, PALETTE_MENU_VALUE_X, y, TEXT_SKIP_DRAW, NULL);
}

// One solid-colour square per colour row (rows 0..PLAYER_COLOR_REGION_COUNT-1),
// using that region's first palette index into the buffer RefreshPreviewPalette()
// just loaded into WIN_SWATCH's own palette bank (BG_PLTT_ID(2)) -- so the
// swatch always matches the live working colours, not the saved ones.
static void RedrawSwatches(void)
{
    u32 region;

    for (region = 0; region < PLAYER_COLOR_REGION_COUNT; region++)
    {
        u8 index = PlayerCustomization_GetRegionSwatchIndex(sPaletteMenu.gender, region);
        FillWindowPixelRect(WIN_SWATCH, PIXEL_FILL(index), SWATCH_X, region * 16 + SWATCH_Y_OFFSET, SWATCH_SIZE, SWATCH_SIZE);
    }
    CopyWindowToVram(WIN_SWATCH, COPYWIN_GFX);
}

static void RefreshPreviewPalette(void)
{
    u16 buf[16];

    PlayerCustomization_BuildPreviewPalette(sPaletteMenu.gender, sPaletteMenu.choices, buf);
    LoadPalette(buf, OBJ_PLTT_ID(gSprites[sPaletteMenu.previewSpriteId].oam.paletteNum), PLTT_SIZE_4BPP);
    LoadPalette(buf, BG_PLTT_ID(2), PLTT_SIZE_4BPP);
    RedrawSwatches();
}

static void DrawHeaderText(void)
{
    s32 hintX = GetStringRightAlignXOffset(FONT_NARROW, sText_ControlHint, 198);

    FillWindowPixelBuffer(WIN_HEADER, PIXEL_FILL(1));
    AddTextPrinterParameterized(WIN_HEADER, FONT_NORMAL, sText_Title, 8, 1, TEXT_SKIP_DRAW, NULL);
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

    // List frame -- also frames WIN_SWATCH, which sits inside it
    FillBgTilemapBufferRect(1, TILE_TOP_CORNER_L,  1,  4,  1,  1,  7);
    FillBgTilemapBufferRect(1, TILE_TOP_EDGE,      2,  4, 18,  1,  7);
    FillBgTilemapBufferRect(1, TILE_TOP_CORNER_R, 19,  4,  1,  1,  7);
    FillBgTilemapBufferRect(1, TILE_LEFT_EDGE,     1,  5,  1, ROW_COUNT * 2,  7);
    FillBgTilemapBufferRect(1, TILE_RIGHT_EDGE,   19,  5,  1, ROW_COUNT * 2,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_CORNER_L,  1,  5 + ROW_COUNT * 2,  1,  1,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_EDGE,      2,  5 + ROW_COUNT * 2, 18,  1,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_CORNER_R, 19,  5 + ROW_COUNT * 2,  1,  1,  7);

    CopyBgTilemapBufferToVram(1);
}
