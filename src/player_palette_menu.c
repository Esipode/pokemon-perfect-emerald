#include "global.h"
#include "player_palette_menu.h"
#include "bg.h"
#include "event_object_movement.h"
#include "field_player_avatar.h"
#include "gpu_regs.h"
#include "international_string_util.h"
#include "list_menu.h"
#include "main.h"
#include "menu_helpers.h"
#include "palette.h"
#include "player_customization.h"
#include "scanline_effect.h"
#include "sound.h"
#include "sprite.h"
#include "string_util.h"
#include "task.h"
#include "text.h"
#include "text_window.h"
#include "trainer_pokemon_sprites.h"
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

// Frame tile ids within the frame gfx LoadBgTiles loads at init (see
// CB2_InitPlayerPaletteMenu case 3/4). Matches the tileNum+0..8 layout
// DrawTextBorderOuter expects, so DrawStdFrameWithCustomTileAndPalette can
// reuse this already-loaded frame gfx for the Stage 11 style-change prompt
// instead of loading a second copy.
#define TILE_TOP_CORNER_L 0x1A2
#define TILE_TOP_EDGE     0x1A3
#define TILE_TOP_CORNER_R 0x1A4
#define TILE_LEFT_EDGE    0x1A5
#define TILE_RIGHT_EDGE   0x1A7
#define TILE_BOT_CORNER_L 0x1A8
#define TILE_BOT_EDGE     0x1A9
#define TILE_BOT_CORNER_R 0x1AA

// A row's kind decides how Task_PaletteMenuProcessInput handles it and what
// its `id` means: a slot id for ROW_KIND_SLOT, unused for the rest.
// ROW_KIND_STYLE (Part A Stage 11) sits directly above ROW_KIND_RESET.
// Groups (bulk hue/shade macros) were removed -- they read as a confusing
// second copy of a slot row with the same name.
enum
{
    ROW_KIND_SLOT,
    ROW_KIND_STYLE,
    ROW_KIND_RESET,
    ROW_KIND_CONFIRM,
};

// Worst case: every slot, STYLE, RESET and CONFIRM. Not every (style,
// gender) uses every slot, so the live row count is usually smaller.
#define PALETTE_MENU_MAX_ROWS (PLAYER_COLOR_SLOT_COUNT + 3)
#define PALETTE_MENU_VISIBLE_ROWS 6

#define tListTaskId data[0]

#define PALETTE_MENU_LABEL_X 8

// DPAD_LEFT/RIGHT hold-repeat on a SLOT row, at 60fps. Start delay is longer
// than the repeat interval so a single tap never double-fires.
#define PALETTE_MENU_DPAD_HOLD_START_DELAY 20
#define PALETTE_MENU_DPAD_HOLD_REPEAT_DELAY 12

// WIN_LIST spans tiles 2..15 (tilemapLeft 2, width 14); the frame's right
// edge is at tile 16. Tiles 14-15 put the swatch column flush against that
// edge, clear of the row label text.
#define PALETTE_MENU_LIST_WIDTH 14
#define SWATCH_TILEMAP_LEFT (2 + PALETTE_MENU_LIST_WIDTH - 2)
#define SWATCH_SIZE          8
#define SWATCH_X              4
#define SWATCH_Y_OFFSET        4

// 2x2 block, right-hand pane. WIN_LIST ends at tilemap column 20 (x = 168),
// so x >= 180 is clear of it.
enum
{
    PREVIEW_FACING_SOUTH,
    PREVIEW_FACING_NORTH,
    PREVIEW_FACING_WEST,
    PREVIEW_FACING_EAST,
    PREVIEW_SPRITE_COUNT,
};

static const s16 sPreviewSpriteX[PREVIEW_SPRITE_COUNT] = {180, 208, 180, 208};
static const s16 sPreviewSpriteY[PREVIEW_SPRITE_COUNT] = { 56,  56, 100, 100};
static const u8 sPreviewSpriteAnim[PREVIEW_SPRITE_COUNT] =
{
    [PREVIEW_FACING_SOUTH] = ANIM_STD_GO_SOUTH,
    [PREVIEW_FACING_NORTH] = ANIM_STD_GO_NORTH,
    [PREVIEW_FACING_WEST]  = ANIM_STD_GO_WEST,
    [PREVIEW_FACING_EAST]  = ANIM_STD_GO_EAST,
};

// Stage P8: R toggles the 2x2 overworld block for the player's trainer front
// pic, for slots that only exist there (e.g. hair hidden under a cap/hat in
// every overworld sheet). Centred in the same right-hand pane the OW block
// uses; the OW sprites are freed while this is shown (only one preview asset
// is ever loaded at a time).
enum
{
    PREVIEW_MODE_OW,
    PREVIEW_MODE_TRAINER,
};

#define TRAINER_PREVIEW_X 204
#define TRAINER_PREVIEW_Y 86
#define TRAINER_PREVIEW_PAL_SLOT 6

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
static u16 GetSelectedRowIndex(void);
static void AdjustSlot(u8 slot, u8 axis, s8 dir);
static void CreateOwPreviewSprites(void);
static void DestroyOwPreviewSprites(void);
static void CreateTrainerPreviewSprite(void);
static void DestroyTrainerPreviewSprite(void);
static void SetPreviewSpritesInvisible(bool8 invisible);
static u8 CreatePaletteListMenu(void);
static void StartStyleChangeConfirm(u8 taskId, u8 newStyle);
static void Task_StyleChangeYes(u8 taskId);
static void Task_StyleChangeNo(u8 taskId);
static void ApplyStyleChange(u8 taskId);

// SLOT rows cycle H -> S -> V. SELECT wraps mod 3, shared by every row --
// there is one active axis for the whole menu, shown once in the header
// (see DrawHeaderText) instead of repeated per row.
enum
{
    AXIS_HUE,
    AXIS_SAT,
    AXIS_VAL,
};

// Working copy of gSaveBlock2Ptr->playerColorSlots, committed only on
// CONFIRM. rows/items are rebuilt by BuildRowMap() whenever style or gender
// changes (only on init for now; Stage 11 rebuilds them on a style change).
static EWRAM_DATA struct
{
    u16 choices[PLAYER_COLOR_SLOT_COUNT];
    u8 gender;
    u8 style;
    u8 pendingStyle; // valid only while the Stage 11 style-change prompt is up
    u8 previewSpriteIds[PREVIEW_SPRITE_COUNT];
    u8 trainerPreviewSpriteId;
    u8 previewMode;
    u8 listTaskId;
    u8 rowCount;
    u8 axis;
    // Hold-repeat for DPAD_LEFT/RIGHT on a SLOT row. dpadHoldDir is 0 (none),
    // 1 (left) or 2 (right); dpadHoldTimer counts down to the next repeat.
    u8 dpadHoldDir;
    u8 dpadHoldTimer;
    // Working HSV per slot, valid iff choices[slot] != 0. AdjustSlot edits
    // this instead of re-deriving H/S/V from the stored RGB15 each press --
    // RGB15(5-bit)->HSV(8-bit)->RGB15 is lossy, so re-deriving every press
    // compounded quantization error and dragged colours toward black.
    u8 slotHsv[PLAYER_COLOR_SLOT_COUNT][3];
    u8 styleRowText[16]; // "STYLE" + current value; rebuilt by BuildRowMap
    struct PaletteMenuRow rows[PALETTE_MENU_MAX_ROWS];
    struct ListMenuItem items[PALETTE_MENU_MAX_ROWS];
} sPaletteMenu = {0};

static const u8 sText_Title[] = _("COLOURS");
static const u8 sText_ControlHintPrefix[] = _("{SELECT_BUTTON}");
static const u8 sText_ControlHintSuffix[] = _(" {A_BUTTON}OK {B_BUTTON}BACK {R_BUTTON}PIC");
static const u8 sText_ResetToDefault[] = _("RESET");
static const u8 sText_Confirm[] = _("CONFIRM");
static const u8 sText_AxisHue[] = _("HUE");
static const u8 sText_AxisSat[] = _("SAT");
static const u8 sText_AxisVal[] = _("VAL");
static const u8 *const sSlotAxisNames[3] = {sText_AxisHue, sText_AxisSat, sText_AxisVal};
static const u8 sText_StyleLabel[] = _("STYLE:  ");
static const u8 sText_StyleEmerald[] = _("HOENN");
static const u8 sText_StyleKanto[] = _("KANTO");
static const u8 sText_ConfirmStyleChange[] = _("CHANGE STYLE? COLOURS RESET.");

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
        .width = PALETTE_MENU_LIST_WIDTH,
        .height = PALETTE_MENU_VISIBLE_ROWS * 2,
        .paletteNum = 1,
        .baseBlock = 0x36
    },
    // Overlaps the rightmost 2 tiles of WIN_LIST's rect (see
    // SWATCH_TILEMAP_LEFT) so the swatches use palette bank 2 instead of the
    // text palette (1). Re-put after every list redraw (RedrawSwatches) since
    // a scrolling ListMenu repaints WIN_LIST's tiles.
    [WIN_SWATCH] = {
        .bg = 0,
        .tilemapLeft = SWATCH_TILEMAP_LEFT,
        .tilemapTop = 5,
        .width = 2,
        .height = PALETTE_MENU_VISIBLE_ROWS * 2,
        .paletteNum = 2,
        .baseBlock = 0x36 + PALETTE_MENU_LIST_WIDTH * (PALETTE_MENU_VISIBLE_ROWS * 2)
    },
    DUMMY_WIN_TEMPLATE
};

// Stage 11 style-change Yes/No prompt. AddWindow'd on demand by
// CreateYesNoMenuWithCallbacks, clear of WIN_LIST (col 2-16, row 5-17) on
// screen. baseBlock must also clear WIN_LIST/WIN_SWATCH's tile ranges
// (0x36..0x186 and 0x186..0x1B6) since this window stays up alongside them
// -- 0x1D0 is the next free block. Reuses the frame gfx and text palette
// already loaded for the other windows, so no extra gfx/palette load is
// needed.
static const struct WindowTemplate sStyleConfirmYesNoWinTemplate =
{
    .bg = 0,
    .tilemapLeft = 21,
    .tilemapTop = 9,
    .width = 5,
    .height = 4,
    .paletteNum = 1,
    .baseBlock = 0x1D0,
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
// slot rows, then STYLE, then RESET, then CONFIRM. Slots with no name for
// this (style, gender) are skipped, so a protagonist with fewer colours
// shows fewer rows instead of blank ones.
static void BuildRowMap(void)
{
    u8 style = sPaletteMenu.style;
    u8 gender = sPaletteMenu.gender;
    u8 count = 0;
    u32 i;

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

    StringCopy(sPaletteMenu.styleRowText, sText_StyleLabel);
    StringAppend(sPaletteMenu.styleRowText, style == PLAYER_SPRITE_STYLE_FRLG ? sText_StyleKanto : sText_StyleEmerald);
    sPaletteMenu.rows[count].kind = ROW_KIND_STYLE;
    sPaletteMenu.items[count].name = sPaletteMenu.styleRowText;
    sPaletteMenu.items[count].id = count;
    count++;

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

// Shared by init (case 9) and ApplyStyleChange, which rebuilds the list
// menu from scratch after a style change resizes the row map.
static u8 CreatePaletteListMenu(void)
{
    struct ListMenuTemplate template = {0};

    template.items = sPaletteMenu.items;
    template.moveCursorFunc = PaletteMenu_MoveCursorCallback;
    template.itemPrintFunc = NULL;
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

    return ListMenuInit(&template, 0, 0);
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
        sPaletteMenu.previewMode = PREVIEW_MODE_OW;
        CreateOwPreviewSprites();

        taskId = CreateTask(Task_PaletteMenuFadeIn, 0);
        gTasks[taskId].tListTaskId = CreatePaletteListMenu();
        sPaletteMenu.listTaskId = gTasks[taskId].tListTaskId;
        RefreshPreviewPalette();
        CopyWindowToVram(WIN_LIST, COPYWIN_GFX);
        gMain.state++;
        break;
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
    Player_SetSpriteStyle(sPaletteMenu.style);
    memcpy(gSaveBlock2Ptr->playerColorSlots, sPaletteMenu.choices, sizeof(sPaletteMenu.choices));
    BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
    gTasks[taskId].func = Task_PaletteMenuFadeOut;
}

static u16 GetSelectedRowIndex(void)
{
    u16 scrollOffset, selectedRow;

    ListMenuGetScrollAndRow(sPaletteMenu.listTaskId, &scrollOffset, &selectedRow);
    return scrollOffset + selectedRow;
}

// Nudges one HSV axis of `slot`'s working colour. An unset slot (choices == 0)
// is seeded from its ROM colour first, per Stage P6 item 3, so the first press
// steps off the vanilla colour instead of off black.
static void AdjustSlot(u8 slot, u8 axis, s8 dir)
{
    u8 h, s, v;
    s16 n;

    if (sPaletteMenu.choices[slot] == 0)
    {
        u16 rgb = PlayerCustomization_GetSlotRomColor(sPaletteMenu.style, sPaletteMenu.gender, slot);
        PlayerCustomization_RgbToHsv(rgb, &h, &s, &v);
        sPaletteMenu.slotHsv[slot][0] = h;
        sPaletteMenu.slotHsv[slot][1] = s;
        sPaletteMenu.slotHsv[slot][2] = v;
    }
    h = sPaletteMenu.slotHsv[slot][0];
    s = sPaletteMenu.slotHsv[slot][1];
    v = sPaletteMenu.slotHsv[slot][2];

    switch (axis % 3)
    {
    case AXIS_HUE:
        h += dir * 8; // u8 wraps mod 256
        break;
    case AXIS_SAT:
        n = (s16)s + dir * 16;
        s = (u8)(n < 0 ? 0 : (n > 255 ? 255 : n));
        break;
    case AXIS_VAL:
        n = (s16)v + dir * 8;
        v = (u8)(n < 0 ? 0 : (n > 255 ? 255 : n));
        break;
    }
    sPaletteMenu.slotHsv[slot][0] = h;
    sPaletteMenu.slotHsv[slot][1] = s;
    sPaletteMenu.slotHsv[slot][2] = v;
    sPaletteMenu.choices[slot] = PLAYER_COLOR_SET | PlayerCustomization_HsvToRgb(h, s, v);
}

// Style ids mean different garments on different protagonists (Stage P9),
// so a style change clears every slot rather than reinterpreting them. Ask
// first, since RESET-on-select would be surprising to hit by accident.
static void StartStyleChangeConfirm(u8 taskId, u8 newStyle)
{
    static const struct YesNoFuncTable sStyleChangeYesNo = {Task_StyleChangeYes, Task_StyleChangeNo};

    sPaletteMenu.pendingStyle = newStyle;
    SetPreviewSpritesInvisible(TRUE);
    FillWindowPixelBuffer(WIN_HEADER, PIXEL_FILL(1));
    AddTextPrinterParameterized(WIN_HEADER, FONT_NORMAL, sText_ConfirmStyleChange, 8, 1, TEXT_SKIP_DRAW, NULL);
    CopyWindowToVram(WIN_HEADER, COPYWIN_FULL);
    CreateYesNoMenuWithCallbacks(taskId, &sStyleConfirmYesNoWinTemplate, 0, 0, 0, TILE_TOP_CORNER_L, 7, &sStyleChangeYesNo);
}

static void Task_StyleChangeYes(u8 taskId)
{
    // ApplyStyleChange destroys and re-creates the preview sprite(s), which
    // start visible, so no explicit unhide is needed on this path.
    ApplyStyleChange(taskId);
    DrawHeaderText();
    gTasks[taskId].func = Task_PaletteMenuProcessInput;
}

static void Task_StyleChangeNo(u8 taskId)
{
    SetPreviewSpritesInvisible(FALSE);
    DrawHeaderText();
    gTasks[taskId].func = Task_PaletteMenuProcessInput;
}

// Changing style invalidates every slot choice (Stage P9), changes which
// slots exist (rebuild the row map/list menu, Stage P5), and changes the
// gfx id / trainer pic id the preview sprites were created with (re-create
// them, Stage P7).
static void ApplyStyleChange(u8 taskId)
{
    sPaletteMenu.style = sPaletteMenu.pendingStyle;
    memset(sPaletteMenu.choices, 0, sizeof(sPaletteMenu.choices));
    memset(sPaletteMenu.slotHsv, 0, sizeof(sPaletteMenu.slotHsv));
    BuildRowMap();

    DestroyListMenuTask(sPaletteMenu.listTaskId, NULL, NULL);
    gTasks[taskId].tListTaskId = CreatePaletteListMenu();
    sPaletteMenu.listTaskId = gTasks[taskId].tListTaskId;

    if (sPaletteMenu.previewMode == PREVIEW_MODE_TRAINER)
    {
        DestroyTrainerPreviewSprite();
        CreateTrainerPreviewSprite();
    }
    else
    {
        DestroyOwPreviewSprites();
        CreateOwPreviewSprites();
    }

    RefreshPreviewPalette();
    CopyWindowToVram(WIN_LIST, COPYWIN_GFX);
    RedrawSwatches();
    // CopyWindowToVram(..., COPYWIN_GFX) above only reloads tile *graphics*,
    // not screen entries. CreatePaletteListMenu's ListMenuInit() re-puts
    // WIN_LIST's tilemap (list_menu.c:363), which re-claims the two tile
    // columns WIN_SWATCH overlaps -- but that write only lands in the CPU-side
    // bg tilemap buffer. Nothing has pushed bg 0's tilemap to VRAM since
    // CB2_InitPlayerPaletteMenu's case 7, so without this call the hardware
    // screen entries never pick up WIN_SWATCH's last-write-wins ordering.
    CopyBgTilemapBufferToVram(0);
}

static void Task_PaletteMenuProcessInput(u8 taskId)
{
    s32 itemId = ListMenu_ProcessInput(gTasks[taskId].tListTaskId);

    switch (itemId)
    {
    case LIST_NOTHING_CHOSEN:
    {
        u16 selected = GetSelectedRowIndex();
        const struct PaletteMenuRow *row = &sPaletteMenu.rows[selected];

        if (JOY_NEW(START_BUTTON))
        {
            ConfirmAndExit(taskId);
            break;
        }

        if (JOY_NEW(R_BUTTON))
        {
            // Only one preview asset is loaded at a time -- free the old
            // one before creating the other (see CreateTrainerPreviewSprite).
            PlaySE(SE_SELECT);
            if (sPaletteMenu.previewMode == PREVIEW_MODE_OW)
            {
                DestroyOwPreviewSprites();
                sPaletteMenu.previewMode = PREVIEW_MODE_TRAINER;
                CreateTrainerPreviewSprite();
            }
            else
            {
                DestroyTrainerPreviewSprite();
                sPaletteMenu.previewMode = PREVIEW_MODE_OW;
                CreateOwPreviewSprites();
            }
            RefreshPreviewPalette();
        }
        else if (JOY_NEW(SELECT_BUTTON))
        {
            // Axis is global, so SELECT works from any row -- it's shown once
            // in the header, not per row.
            PlaySE(SE_SELECT);
            sPaletteMenu.axis = (sPaletteMenu.axis + 1) % 3;
            DrawHeaderText();
            RedrawListMenu(gTasks[taskId].tListTaskId);
            CopyWindowToVram(WIN_LIST, COPYWIN_GFX);
        }
        else if (row->kind == ROW_KIND_STYLE && (JOY_NEW(DPAD_LEFT) || JOY_NEW(DPAD_RIGHT)))
        {
            s8 dir = JOY_NEW(DPAD_RIGHT) ? 1 : -1;
            u8 newStyle = (sPaletteMenu.style + dir + PLAYER_SPRITE_STYLE_COUNT) % PLAYER_SPRITE_STYLE_COUNT;

            PlaySE(SE_SELECT);
            StartStyleChangeConfirm(taskId, newStyle);
        }
        else if (row->kind == ROW_KIND_SLOT && (JOY_HELD(DPAD_LEFT) ^ JOY_HELD(DPAD_RIGHT)))
        {
            // Hold-repeat: a fresh press fires immediately and (re)arms the
            // start delay; holding through it fires every REPEAT_DELAY
            // frames after. Releasing, or holding the other direction,
            // resets via the trailing else below.
            u8 dirId = JOY_HELD(DPAD_RIGHT) ? 2 : 1;
            bool8 fire = JOY_NEW(DPAD_LEFT) || JOY_NEW(DPAD_RIGHT);

            if (fire)
            {
                sPaletteMenu.dpadHoldTimer = PALETTE_MENU_DPAD_HOLD_START_DELAY;
            }
            else if (sPaletteMenu.dpadHoldDir == dirId && --sPaletteMenu.dpadHoldTimer == 0)
            {
                fire = TRUE;
                sPaletteMenu.dpadHoldTimer = PALETTE_MENU_DPAD_HOLD_REPEAT_DELAY;
            }
            sPaletteMenu.dpadHoldDir = dirId;

            if (fire)
            {
                s8 dir = dirId == 2 ? 1 : -1;

                AdjustSlot(row->id, sPaletteMenu.axis, dir);
                PlaySE(SE_SELECT);
                RedrawListMenu(gTasks[taskId].tListTaskId);
                CopyWindowToVram(WIN_LIST, COPYWIN_GFX);
                RefreshPreviewPalette();
            }
        }
        else
        {
            sPaletteMenu.dpadHoldDir = 0;
        }
        break;
    }
    case LIST_CANCEL:
        PlaySE(SE_SELECT);
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        gTasks[taskId].func = Task_PaletteMenuFadeOut;
        break;
    default:
        switch (sPaletteMenu.rows[itemId].kind)
        {
        case ROW_KIND_SLOT:
        {
            u8 slot = sPaletteMenu.rows[itemId].id;

            PlaySE(SE_SELECT);
            if (sPaletteMenu.choices[slot] != 0)
            {
                sPaletteMenu.choices[slot] = 0;
            }
            else
            {
                u16 rgb = PlayerCustomization_GetSlotRomColor(sPaletteMenu.style, sPaletteMenu.gender, slot);
                sPaletteMenu.choices[slot] = PLAYER_COLOR_SET | rgb;
                PlayerCustomization_RgbToHsv(rgb, &sPaletteMenu.slotHsv[slot][0], &sPaletteMenu.slotHsv[slot][1], &sPaletteMenu.slotHsv[slot][2]);
            }
            RedrawListMenu(gTasks[taskId].tListTaskId);
            CopyWindowToVram(WIN_LIST, COPYWIN_GFX);
            RefreshPreviewPalette();
            break;
        }
        case ROW_KIND_RESET:
            PlaySE(SE_SELECT);
            memset(sPaletteMenu.choices, 0, sizeof(sPaletteMenu.choices));
            memset(sPaletteMenu.slotHsv, 0, sizeof(sPaletteMenu.slotHsv));
            RedrawListMenu(gTasks[taskId].tListTaskId);
            CopyWindowToVram(WIN_LIST, COPYWIN_GFX);
            RefreshPreviewPalette();
            break;
        case ROW_KIND_STYLE:
            // A cycles forward, same as DPAD_RIGHT.
            PlaySE(SE_SELECT);
            StartStyleChangeConfirm(taskId, (sPaletteMenu.style + 1) % PLAYER_SPRITE_STYLE_COUNT);
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
        if (sPaletteMenu.previewMode == PREVIEW_MODE_TRAINER)
            DestroyTrainerPreviewSprite();
        else
            DestroyOwPreviewSprites();
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
// RESET/CONFIRM rows show no swatch.
//
// Background fill uses index 15: every slot's swatch colour comes from a
// customizable index (e.g. HAIR is index 1), so filling with any of those
// indices can make a swatch blend into its own background once the player
// picks a matching colour. Index 15 is never assigned to a slot, and
// RefreshPreviewPalette overwrites it (in this window's copy only) with
// white to match the rest of the box, instead of a border, since GBA
// background tiles always render pixel value 0 as the screen backdrop, not
// a controllable colour -- 15 is the only index this window can paint with
// a fixed, chosen colour.
static void RedrawSwatches(void)
{
    u16 scrollOffset;
    u32 i;

    ListMenuGetScrollAndRow(sPaletteMenu.listTaskId, &scrollOffset, NULL);
    FillWindowPixelBuffer(WIN_SWATCH, PIXEL_FILL(15));

    for (i = 0; i < PALETTE_MENU_VISIBLE_ROWS && scrollOffset + i < sPaletteMenu.rowCount; i++)
    {
        const struct PaletteMenuRow *row = &sPaletteMenu.rows[scrollOffset + i];

        if (row->kind == ROW_KIND_SLOT)
        {
            u8 index = (sPaletteMenu.previewMode == PREVIEW_MODE_TRAINER)
                ? PlayerCustomization_GetTrainerSlotSwatchIndex(sPaletteMenu.style, sPaletteMenu.gender, row->id)
                : PlayerCustomization_GetSlotSwatchIndex(sPaletteMenu.style, sPaletteMenu.gender, row->id);
            u8 y = i * 16 + SWATCH_Y_OFFSET;

            FillWindowPixelRect(WIN_SWATCH, PIXEL_FILL(index), SWATCH_X, y, SWATCH_SIZE, SWATCH_SIZE);
        }
    }

    PutWindowTilemap(WIN_SWATCH);
    CopyWindowToVram(WIN_SWATCH, COPYWIN_GFX);
}

// Same graphicsId for all four -- under OW_GFX_COMPRESS,
// CreateObjectGraphicsSpriteWithTag (event_object_movement.c) shares one tile
// sheet across sprites with the same tileTag instead of allocating four, so
// this is not a 4x VRAM cost.
static void CreateOwPreviewSprites(void)
{
    // Not GetPlayerAvatarGraphicsIdByStateIdAndGender -- that reads the
    // committed save's style, so it would keep showing the old gfx while
    // sPaletteMenu.style is only a pending, uncommitted choice.
    u16 graphicsId = GetPlayerAvatarGraphicsIdByStateGenderAndStyle(PLAYER_AVATAR_STATE_NORMAL, sPaletteMenu.gender, sPaletteMenu.style);
    u32 i;

    for (i = 0; i < PREVIEW_SPRITE_COUNT; i++)
    {
        sPaletteMenu.previewSpriteIds[i] = CreateObjectGraphicsSprite(
            graphicsId, SpriteCallbackDummy, sPreviewSpriteX[i], sPreviewSpriteY[i], 0);
        StartSpriteAnim(&gSprites[sPaletteMenu.previewSpriteIds[i]], sPreviewSpriteAnim[i]);
    }
}

static void DestroyOwPreviewSprites(void)
{
    u32 i;

    for (i = 0; i < PREVIEW_SPRITE_COUNT; i++)
        DestroySprite(&gSprites[sPaletteMenu.previewSpriteIds[i]]);
}

// CreateTrainerPicSprite loads the *committed* save's trainer palette as a
// side effect (via GetTrainerFrontPicPalette -> PlayerCustomization_Get
// TrainerPaletteOverride, which reads gSaveBlock2Ptr directly); RefreshPreview
// Palette immediately overwrites that with the working `choices` copy, same
// as the OW sprites.
static void CreateTrainerPreviewSprite(void)
{
    u32 picId = PlayerCustomization_GetTrainerPicId(sPaletteMenu.style, sPaletteMenu.gender);

    sPaletteMenu.trainerPreviewSpriteId = CreateTrainerPicSprite(
        picId, TRUE, TRAINER_PREVIEW_X, TRAINER_PREVIEW_Y, TRAINER_PREVIEW_PAL_SLOT, TAG_NONE);
}

static void DestroyTrainerPreviewSprite(void)
{
    FreeAndDestroyTrainerPicSprite(sPaletteMenu.trainerPreviewSpriteId);
}

// Hides whichever preview asset is currently shown, so a style-change
// confirm box (an OBJ-layer sprite otherwise renders above it) doesn't get
// drawn over. Only touches the active previewMode's sprite(s).
static void SetPreviewSpritesInvisible(bool8 invisible)
{
    if (sPaletteMenu.previewMode == PREVIEW_MODE_TRAINER)
    {
        gSprites[sPaletteMenu.trainerPreviewSpriteId].invisible = invisible;
    }
    else
    {
        u32 i;

        for (i = 0; i < PREVIEW_SPRITE_COUNT; i++)
            gSprites[sPaletteMenu.previewSpriteIds[i]].invisible = invisible;
    }
}

static void RefreshPreviewPalette(void)
{
    u16 buf[16];
    u16 swatchBuf[16];
    u8 paletteNum;

    if (sPaletteMenu.previewMode == PREVIEW_MODE_TRAINER)
    {
        PlayerCustomization_BuildTrainerPreviewPalette(sPaletteMenu.style, sPaletteMenu.gender, sPaletteMenu.choices, buf);
        paletteNum = gSprites[sPaletteMenu.trainerPreviewSpriteId].oam.paletteNum;
        LoadPalette(buf, OBJ_PLTT_ID(paletteNum), PLTT_SIZE_4BPP);
    }
    else
    {
        PlayerCustomization_BuildPreviewPalette(sPaletteMenu.style, sPaletteMenu.gender, sPaletteMenu.choices, buf);
        paletteNum = gSprites[sPaletteMenu.previewSpriteIds[0]].oam.paletteNum;
        // All four sprites share one palette tag, so one LoadPalette recolours
        // all four. AGB_ASSERT catches the tag setup breaking (e.g. from a
        // future per-facing gfx change) instead of silently recolouring only
        // one sprite.
        AGB_ASSERT(gSprites[sPaletteMenu.previewSpriteIds[1]].oam.paletteNum == paletteNum);
        AGB_ASSERT(gSprites[sPaletteMenu.previewSpriteIds[2]].oam.paletteNum == paletteNum);
        AGB_ASSERT(gSprites[sPaletteMenu.previewSpriteIds[3]].oam.paletteNum == paletteNum);
        LoadPalette(buf, OBJ_PLTT_ID(paletteNum), PLTT_SIZE_4BPP);
    }

    // Separate copy for WIN_SWATCH's own BG palette bank: index 15 becomes
    // white chrome here without touching the preview sprite's black outline,
    // which is loaded from the untouched `buf` above into a different
    // hardware palette (OBJ, not BG).
    memcpy(swatchBuf, buf, sizeof(swatchBuf));
    swatchBuf[15] = RGB_WHITE;
    LoadPalette(swatchBuf, BG_PLTT_ID(2), PLTT_SIZE_4BPP);
    RedrawSwatches();
}

static void DrawHeaderText(void)
{
    u8 hint[32];
    s32 hintX;

    StringCopy(hint, sText_ControlHintPrefix);
    StringAppend(hint, sSlotAxisNames[sPaletteMenu.axis % 3]);
    StringAppend(hint, sText_ControlHintSuffix);
    hintX = GetStringRightAlignXOffset(FONT_NARROW, hint, 198);

    FillWindowPixelBuffer(WIN_HEADER, PIXEL_FILL(1));
    AddTextPrinterParameterized(WIN_HEADER, FONT_NORMAL, sText_Title, 8, 1, TEXT_SKIP_DRAW, NULL);
    AddTextPrinterParameterized(WIN_HEADER, FONT_NARROW, hint, hintX, 1, TEXT_SKIP_DRAW, NULL);
    CopyWindowToVram(WIN_HEADER, COPYWIN_FULL);
}

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
    FillBgTilemapBufferRect(1, TILE_TOP_EDGE,      2,  4, PALETTE_MENU_LIST_WIDTH,  1,  7);
    FillBgTilemapBufferRect(1, TILE_TOP_CORNER_R, 2 + PALETTE_MENU_LIST_WIDTH,  4,  1,  1,  7);
    FillBgTilemapBufferRect(1, TILE_LEFT_EDGE,     1,  5,  1, PALETTE_MENU_VISIBLE_ROWS * 2,  7);
    FillBgTilemapBufferRect(1, TILE_RIGHT_EDGE,   2 + PALETTE_MENU_LIST_WIDTH,  5,  1, PALETTE_MENU_VISIBLE_ROWS * 2,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_CORNER_L,  1,  5 + PALETTE_MENU_VISIBLE_ROWS * 2,  1,  1,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_EDGE,      2,  5 + PALETTE_MENU_VISIBLE_ROWS * 2, PALETTE_MENU_LIST_WIDTH,  1,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_CORNER_R, 2 + PALETTE_MENU_LIST_WIDTH,  5 + PALETTE_MENU_VISIBLE_ROWS * 2,  1,  1,  7);

    CopyBgTilemapBufferToVram(1);
}
