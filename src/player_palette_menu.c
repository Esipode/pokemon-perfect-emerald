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
#include "task.h"
#include "text.h"
#include "text_window.h"
#include "window.h"
#include "constants/event_object_movement.h"
#include "constants/rgb.h"
#include "constants/songs.h"

enum
{
    WIN_HEADER,
    WIN_LIST,
    WIN_SWATCH,
};

// A row's kind decides how Task_PaletteMenuProcessInput handles it and what
// its `id` means: a group id (enum PlayerColorRegion) for ROW_KIND_GROUP, a
// slot id for ROW_KIND_SLOT, unused for the rest. ROW_KIND_STYLE is reserved
// for Part A Stage 11, which adds the STYLE row once Part B is done.
enum
{
    ROW_KIND_GROUP,
    ROW_KIND_SLOT,
    ROW_KIND_STYLE,
    ROW_KIND_RESET,
    ROW_KIND_CONFIRM,
};

// Worst case: every group, every slot, RESET and CONFIRM. Not every
// (style, gender) uses every slot, so the live row count is usually smaller.
#define PALETTE_MENU_MAX_ROWS (PLAYER_COLOR_REGION_COUNT + PLAYER_COLOR_SLOT_COUNT + 2)
#define PALETTE_MENU_VISIBLE_ROWS 6

#define tListTaskId data[0]

#define PALETTE_MENU_LABEL_X 8
#define PALETTE_MENU_VALUE_X 84

#define SWATCH_TILEMAP_LEFT 9
#define SWATCH_SIZE          8
#define SWATCH_X              4
#define SWATCH_Y_OFFSET        4

#define PREVIEW_SPRITE_X 208
#define PREVIEW_SPRITE_Y  72

struct PaletteMenuRow
{
    u8 kind;
    u8 id;
};

static void Task_PaletteMenuFadeIn(u8 taskId);
static void Task_PaletteMenuProcessInput(u8 taskId);
static void Task_PaletteMenuFadeOut(u8 taskId);
static void PaletteMenu_MoveCursorCallback(s32 itemIndex, bool8 onInit, struct ListMenu *list);
static void BuildRowMap(void);
static void RedrawSwatches(void);
static void RefreshPreviewPalette(void);
static void DrawHeaderText(void);
static void DrawBgWindowFrames(void);

// Working copy of gSaveBlock2Ptr->playerColorSlots, committed only on
// CONFIRM. rows/items are rebuilt by BuildRowMap() whenever style or gender
// changes (only on init for now; Stage 11 rebuilds them on a style change).
static EWRAM_DATA struct
{
    u16 choices[PLAYER_COLOR_SLOT_COUNT];
    u8 gender;
    u8 style;
    u8 previewSpriteId;
    u8 listTaskId;
    u8 rowCount;
    struct PaletteMenuRow rows[PALETTE_MENU_MAX_ROWS];
    struct ListMenuItem items[PALETTE_MENU_MAX_ROWS];
} sPaletteMenu = {0};

static const u8 sText_Title[] = _("PLAYER COLOURS");
static const u8 sText_ControlHint[] = _("{SELECT_BUTTON}MODE {A_BUTTON}OK {B_BUTTON}BACK");
static const u8 sText_ResetToDefault[] = _("RESET TO DEFAULT");
static const u8 sText_Confirm[] = _("CONFIRM");

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
        .width = 18,
        .height = PALETTE_MENU_VISIBLE_ROWS * 2,
        .paletteNum = 1,
        .baseBlock = 0x36
    },
    // Overlaps a column inside WIN_LIST's rect so the swatches use palette
    // bank 2 instead of the text palette (1). Re-put after every list redraw
    // (RedrawSwatches) since a scrolling ListMenu repaints WIN_LIST's tiles.
    [WIN_SWATCH] = {
        .bg = 0,
        .tilemapLeft = SWATCH_TILEMAP_LEFT,
        .tilemapTop = 5,
        .width = 2,
        .height = PALETTE_MENU_VISIBLE_ROWS * 2,
        .paletteNum = 2,
        .baseBlock = 0x36 + 18 * (PALETTE_MENU_VISIBLE_ROWS * 2)
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

// Builds sPaletteMenu.rows/items/rowCount for the active (style, gender):
// group rows, then slot rows, then RESET, then CONFIRM. Slots and groups
// with no name for this (style, gender) are skipped, so a protagonist with
// fewer colours shows fewer rows instead of blank ones.
static void BuildRowMap(void)
{
    u8 style = sPaletteMenu.style;
    u8 gender = sPaletteMenu.gender;
    u8 count = 0;
    u32 i;

    for (i = 0; i < PLAYER_COLOR_REGION_COUNT; i++)
    {
        const struct PlayerColorGroupInfo *group = PlayerCustomization_GetGroupInfo(style, gender, i);

        if (group->name == NULL)
            continue;
        sPaletteMenu.rows[count].kind = ROW_KIND_GROUP;
        sPaletteMenu.rows[count].id = i;
        sPaletteMenu.items[count].name = group->name;
        sPaletteMenu.items[count].id = count;
        count++;
    }

    for (i = 0; i < PLAYER_COLOR_SLOT_COUNT; i++)
    {
        const struct PlayerColorSlotInfo *slot = PlayerCustomization_GetSlotInfo(style, gender, i);

        if (slot->name == NULL)
            continue;
        sPaletteMenu.rows[count].kind = ROW_KIND_SLOT;
        sPaletteMenu.rows[count].id = i;
        sPaletteMenu.items[count].name = slot->name;
        sPaletteMenu.items[count].id = count;
        count++;
    }

    sPaletteMenu.rows[count].kind = ROW_KIND_RESET;
    sPaletteMenu.items[count].name = sText_ResetToDefault;
    sPaletteMenu.items[count].id = count;
    count++;

    sPaletteMenu.rows[count].kind = ROW_KIND_CONFIRM;
    sPaletteMenu.items[count].name = sText_Confirm;
    sPaletteMenu.items[count].id = count;
    count++;

    sPaletteMenu.rowCount = count;
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
        sPaletteMenu.style = Player_GetSpriteStyle();
        memcpy(sPaletteMenu.choices, gSaveBlock2Ptr->playerColorSlots, sizeof(sPaletteMenu.choices));
        BuildRowMap();
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

        template.items = sPaletteMenu.items;
        template.moveCursorFunc = PaletteMenu_MoveCursorCallback;
        template.itemPrintFunc = NULL; // Stage P6 adds the value column
        template.totalItems = sPaletteMenu.rowCount;
        template.maxShowed = PALETTE_MENU_VISIBLE_ROWS;
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
        sPaletteMenu.listTaskId = gTasks[taskId].tListTaskId;
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

static void ConfirmAndExit(u8 taskId)
{
    PlaySE(SE_SELECT);
    memcpy(gSaveBlock2Ptr->playerColorSlots, sPaletteMenu.choices, sizeof(sPaletteMenu.choices));
    BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
    gTasks[taskId].func = Task_PaletteMenuFadeOut;
}

static void Task_PaletteMenuProcessInput(u8 taskId)
{
    s32 itemId = ListMenu_ProcessInput(gTasks[taskId].tListTaskId);

    switch (itemId)
    {
    case LIST_NOTHING_CHOSEN:
        if (JOY_NEW(START_BUTTON))
            ConfirmAndExit(taskId);
        // Left/Right/SELECT editing on GROUP and SLOT rows lands in Stage P6.
        break;
    case LIST_CANCEL:
        PlaySE(SE_SELECT);
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        gTasks[taskId].func = Task_PaletteMenuFadeOut;
        break;
    default:
        switch (sPaletteMenu.rows[itemId].kind)
        {
        case ROW_KIND_RESET:
            PlaySE(SE_SELECT);
            memset(sPaletteMenu.choices, 0, sizeof(sPaletteMenu.choices));
            RedrawListMenu(gTasks[taskId].tListTaskId);
            CopyWindowToVram(WIN_LIST, COPYWIN_GFX);
            RefreshPreviewPalette();
            break;
        case ROW_KIND_CONFIRM:
            ConfirmAndExit(taskId);
            break;
        default:
            break;
        }
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
    {
        PlaySE(SE_SELECT);
        // Scrolling repaints WIN_LIST without calling RefreshPreviewPalette,
        // so keep the swatch column's row alignment in sync here too.
        RedrawSwatches();
    }
}

// Swatches read from the palette bank RefreshPreviewPalette() loaded
// (BG_PLTT_ID(2)), so they match the live working colours, not the saved
// ones. Only paints one swatch per *visible* row, at the scroll offset
// ListMenuGetScrollAndRow() reports, so it stays aligned while scrolling.
// GROUP rows have no single colour of their own yet (Stage P6 gives them a
// representative one), so they show no swatch, same as RESET/CONFIRM.
static void RedrawSwatches(void)
{
    u16 scrollOffset;
    u32 i;

    ListMenuGetScrollAndRow(sPaletteMenu.listTaskId, &scrollOffset, NULL);
    FillWindowPixelBuffer(WIN_SWATCH, PIXEL_FILL(1));

    for (i = 0; i < PALETTE_MENU_VISIBLE_ROWS && scrollOffset + i < sPaletteMenu.rowCount; i++)
    {
        const struct PaletteMenuRow *row = &sPaletteMenu.rows[scrollOffset + i];

        if (row->kind == ROW_KIND_SLOT)
        {
            u8 index = PlayerCustomization_GetSlotSwatchIndex(sPaletteMenu.style, sPaletteMenu.gender, row->id);
            FillWindowPixelRect(WIN_SWATCH, PIXEL_FILL(index), SWATCH_X, i * 16 + SWATCH_Y_OFFSET, SWATCH_SIZE, SWATCH_SIZE);
        }
    }

    PutWindowTilemap(WIN_SWATCH);
    CopyWindowToVram(WIN_SWATCH, COPYWIN_GFX);
}

static void RefreshPreviewPalette(void)
{
    u16 buf[16];

    PlayerCustomization_BuildPreviewPalette(sPaletteMenu.style, sPaletteMenu.gender, sPaletteMenu.choices, buf);
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
    FillBgTilemapBufferRect(1, TILE_TOP_EDGE,      2,  4, 19,  1,  7);
    FillBgTilemapBufferRect(1, TILE_TOP_CORNER_R, 20,  4,  1,  1,  7);
    FillBgTilemapBufferRect(1, TILE_LEFT_EDGE,     1,  5,  1, PALETTE_MENU_VISIBLE_ROWS * 2,  7);
    FillBgTilemapBufferRect(1, TILE_RIGHT_EDGE,   20,  5,  1, PALETTE_MENU_VISIBLE_ROWS * 2,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_CORNER_L,  1,  5 + PALETTE_MENU_VISIBLE_ROWS * 2,  1,  1,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_EDGE,      2,  5 + PALETTE_MENU_VISIBLE_ROWS * 2, 19,  1,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_CORNER_R, 20,  5 + PALETTE_MENU_VISIBLE_ROWS * 2,  1,  1,  7);

    CopyBgTilemapBufferToVram(1);
}
