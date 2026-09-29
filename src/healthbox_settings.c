#include "global.h"
#include "healthbox.h"
#include "battle.h"
#include "battle_interface.h"
#include "bg.h"
#include "decompress.h"
#include "gpu_regs.h"
#include "graphics.h"
#include "international_string_util.h"
#include "main.h"
#include "menu.h"
#include "palette.h"
#include "scanline_effect.h"
#include "sound.h"
#include "sprite.h"
#include "string_util.h"
#include "strings.h"
#include "task.h"
#include "text.h"
#include "text_window.h"
#include "type_icons.h"
#include "window.h"
#include "constants/battle.h"
#include "constants/characters.h"
#include "constants/pokemon.h"
#include "constants/rgb.h"
#include "constants/songs.h"

enum
{
    WIN_HEADER,
    WIN_LIST,
    WIN_PREVIEW,
};

enum
{
    ROW_STYLE,
    ROW_BACKGROUND,
    ROW_PRESET, // Derived from that side's toggles; not stored.
    ROW_NAME,
    ROW_LEVEL,
    ROW_HP_BAR,
    ROW_HP_VALUE,
    ROW_EXP,        // Player only.
    ROW_STATUS,
    ROW_TYPES,
    ROW_CAUGHT,     // Foe only.
    ROW_CATCHABLE,  // Foe only.
    ROW_STAT_STAGES,
    ROW_COUNT,
};

#define VISIBLE_ROWS 6
#define ROW_PITCH    12

// Rows from ROW_NAME on have a YOU and a FOE value; the rest have one shared value.
#define FIRST_PER_SIDE_ROW ROW_NAME

// Window-local pixel columns. Both windows start at tile 2, so header captions line up with the cells.
#define LABEL_X        8
#define COLUMN_YOU_X   122
#define COLUMN_FOE_X   176
#define COLUMN_SHARED_X 149
#define SWITCH_HINT_X  190 // L/R glyphs after the FOE caption
#define LIST_TEXT_Y_SHIFT 3 // List text sits this many px above its row top

// Tile rows: the header frame takes 0-3, the preview 4-8, the list frame 9-19.
#define PREVIEW_TILE_TOP  4
#define PREVIEW_TILE_ROWS 5
#define PREVIEW_PIXEL_TOP (PREVIEW_TILE_TOP * 8)
#define PREVIEW_PIXEL_H   (PREVIEW_TILE_ROWS * 8)
#define LIST_TILE_TOP  10
#define LIST_PIXEL_TOP (LIST_TILE_TOP * 8)

// Preview boxes; each slot fits the largest box of its kind (104 and 96 px wide) plus a type icon on its outer side.
enum
{
    PREVIEW_SINGLES,
    PREVIEW_DOUBLES,
    PREVIEW_COUNT,
};

static const u8 sPreviewX[PREVIEW_COUNT] = { 12, 132 };

// Sample mon shown in the preview.
#define SAMPLE_LEVEL      50
#define SAMPLE_HP         120
#define SAMPLE_MAX_HP     200
#define SAMPLE_HP_COLOUR  0 // green
#define SAMPLE_TYPE       TYPE_ELECTRIC

// Bar sprite tiles: 8 for the HP bar plus the two icon slots.
#define HB_PREVIEW_BAR_TILES 10

// Frame tile ids within the frame gfx loaded at init.
#define TILE_TOP_CORNER_L 0x1A2
#define TILE_TOP_EDGE     0x1A3
#define TILE_TOP_CORNER_R 0x1A4
#define TILE_LEFT_EDGE    0x1A5
#define TILE_RIGHT_EDGE   0x1A7
#define TILE_BOT_CORNER_L 0x1A8
#define TILE_BOT_EDGE     0x1A9
#define TILE_BOT_CORNER_R 0x1AA

struct PreviewBox
{
    struct HealthboxSprites sprites;
    struct HealthboxLayout layout;
    struct Subsprite subsprites[HB_BAR_SUBSPRITES_MAX];
    struct SubspriteTable subspriteTable;
    u8 typeIcon; // SPRITE_NONE when absent
};

static EWRAM_DATA struct
{
    struct HealthboxOptions options; // Working copy; committed on exit.
    struct PreviewBox preview[PREVIEW_COUNT];
    u8 row;
    u8 top;  // First visible row.
    u8 side; // HB_SIDE_*; the column the cursor is in.
    u8 buffer; // Half of each sprite sheet the on-screen boxes use.
} sSettings = {0};

static const u8 sText_Header[]   = _("HEALTHBOX");
static const u8 sText_You[]      = _("YOU");
static const u8 sText_Foe[]      = _("FOE");
static const u8 sText_Scroll[]   = _("{UP_ARROW}{DOWN_ARROW}");
static const u8 sText_SwitchHint[] = _("{L_BUTTON}{R_BUTTON}");
static const u8 sText_Dash[]     = _("-");
static const u8 sText_SampleNick[] = _("Pikachu");
static const u8 sText_SamplePsn[]  = _("PSN");

// Strip order: Atk, Def, SpA, SpD, Spe.
static const u8 sSampleStages[5] =
{
    DEFAULT_STAT_STAGE + 2, DEFAULT_STAT_STAGE - 1, DEFAULT_STAT_STAGE, DEFAULT_STAT_STAGE + 1, DEFAULT_STAT_STAGE,
};

static const u8 *const sRowNames[ROW_COUNT] =
{
    [ROW_STYLE]       = COMPOUND_STRING("STYLE"),
    [ROW_BACKGROUND]  = COMPOUND_STRING("BACKGROUND"),
    [ROW_PRESET]      = COMPOUND_STRING("PRESET"),
    [ROW_NAME]        = COMPOUND_STRING("NAME"),
    [ROW_LEVEL]       = COMPOUND_STRING("LEVEL"),
    [ROW_HP_BAR]      = COMPOUND_STRING("HP BAR"),
    [ROW_HP_VALUE]    = COMPOUND_STRING("HP VALUE"),
    [ROW_EXP]         = COMPOUND_STRING("EXP"),
    [ROW_STATUS]      = COMPOUND_STRING("STATUS"),
    [ROW_TYPES]       = COMPOUND_STRING("TYPES"),
    [ROW_CAUGHT]      = COMPOUND_STRING("CAUGHT"),
    [ROW_CATCHABLE]   = COMPOUND_STRING("CAN CATCH"),
    [ROW_STAT_STAGES] = COMPOUND_STRING("STAT CHANGES"),
};

static const u8 *const sStyleNames[] =
{
    [HEALTHBOX_STYLE_CLASSIC] = COMPOUND_STRING("CLASSIC"),
    [HEALTHBOX_STYLE_NEW]     = COMPOUND_STRING("NEW"),
};

static const u8 *const sBackgroundNames[] =
{
    [HB_BG_SOLID] = COMPOUND_STRING("SOLID"),
    [HB_BG_NONE]  = COMPOUND_STRING("NONE"),
};

static const u8 *const sPresetNames[HB_PRESET_COUNT] =
{
    [HB_PRESET_MINIMAL]  = COMPOUND_STRING("MINIMAL"),
    [HB_PRESET_STANDARD] = COMPOUND_STRING("STANDARD"),
    [HB_PRESET_FULL]     = COMPOUND_STRING("FULL"),
    [HB_PRESET_CUSTOM]   = COMPOUND_STRING("CUSTOM"),
};

static const u8 *const sHpValueNames[] =
{
    [HB_HPVAL_NONE]    = COMPOUND_STRING("OFF"),
    [HB_HPVAL_NUMBERS] = COMPOUND_STRING("NUM"),
    [HB_HPVAL_PERCENT] = COMPOUND_STRING("%"),
};

static const u8 *const sOnOffNames[] =
{
    COMPOUND_STRING("OFF"),
    COMPOUND_STRING("ON"),
};

// Text colours as { background, foreground, shadow } from option_menu_text.pal.
static const u8 sColorLabel[]    = {TEXT_COLOR_WHITE, TEXT_COLOR_DARK_GRAY, TEXT_COLOR_LIGHT_GRAY};
static const u8 sColorValue[]    = {TEXT_COLOR_WHITE, TEXT_COLOR_GREEN, TEXT_COLOR_LIGHT_GREEN};
static const u8 sColorSelected[] = {TEXT_COLOR_WHITE, TEXT_COLOR_RED, TEXT_COLOR_LIGHT_RED};
static const u8 sColorDisabled[] = {TEXT_COLOR_WHITE, TEXT_COLOR_LIGHT_GREEN, TEXT_COLOR_LIGHT_GREEN};

static const struct WindowTemplate sWinTemplates[] =
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
        .tilemapTop = LIST_TILE_TOP,
        .width = 26,
        .height = 9,
        .paletteNum = 1,
        .baseBlock = 0x36
    },
    [WIN_PREVIEW] = {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = PREVIEW_TILE_TOP,
        .width = 26,
        .height = PREVIEW_TILE_ROWS,
        .paletteNum = 1,
        .baseBlock = 0x120
    },
    DUMMY_WIN_TEMPLATE
};

static const struct OamData sOamData_PreviewBar =
{
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(32x8),
    .size = SPRITE_SIZE(32x8),
    .priority = 1,
};

static const struct SpriteTemplate sPreviewBarTemplates[PREVIEW_COUNT] =
{
    { .tileTag = TAG_HEALTHBAR_PLAYER1_TILE, .paletteTag = TAG_HEALTHBAR_PAL, .oam = &sOamData_PreviewBar },
    { .tileTag = TAG_HEALTHBAR_PLAYER2_TILE, .paletteTag = TAG_HEALTHBAR_PAL, .oam = &sOamData_PreviewBar },
};

// Blank sheets sized for the largest box of either style: Classic singles 2x 64x64, Classic doubles
// 2x 64x32 (New needs less); bars 8 tiles + 2 icon slots. Each sheet holds two of those, so a refresh
// draws into the half that is not on screen.
static const u16 sBoxBufferTiles[PREVIEW_COUNT] = { 128, 64 };

static const struct CompressedSpriteSheet sPreviewSheets[] =
{
    { gBlankGfxCompressed, 2 * 128 * TILE_SIZE_4BPP, TAG_HEALTHBOX_PLAYER1_TILE },
    { gBlankGfxCompressed, 2 * 64 * TILE_SIZE_4BPP, TAG_HEALTHBOX_PLAYER2_TILE },
    { gBlankGfxCompressed, 2 * HB_PREVIEW_BAR_TILES * TILE_SIZE_4BPP, TAG_HEALTHBAR_PLAYER1_TILE },
    { gBlankGfxCompressed, 2 * HB_PREVIEW_BAR_TILES * TILE_SIZE_4BPP, TAG_HEALTHBAR_PLAYER2_TILE },
};

static const struct SpritePalette sPreviewBarPalette = { gBattleInterface_BallDisplayPal, TAG_HEALTHBAR_PAL };

static const struct BgTemplate sBgTemplates[] =
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

static const u16 sBg_Pal[] = {RGB(17, 18, 31)};
static const u16 sText_Pal[] = INCGFX_U16("graphics/interface/option_menu_text.pal", ".gbapal");

static void Task_FadeIn(u8 taskId);
static void Task_ProcessInput(u8 taskId);
static void Task_FadeOut(u8 taskId);

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

// The preset row holds a separate value per side, so it joins the per-side rows.
static bool32 IsPerSideRow(u32 row)
{
    return row >= FIRST_PER_SIDE_ROW || row == ROW_PRESET;
}

// Classic only honours HP BAR / HP VALUE; the rest are greyed out.
static bool32 IsRowActive(u32 row)
{
    if (row == ROW_STYLE || row == ROW_HP_BAR || row == ROW_HP_VALUE)
        return TRUE;
    return sSettings.options.style == HEALTHBOX_STYLE_NEW;
}

static bool32 IsCellApplicable(u32 row, u32 side)
{
    if (row == ROW_EXP)
        return side == HB_SIDE_PLAYER;
    if (row == ROW_CAUGHT || row == ROW_CATCHABLE)
        return side == HB_SIDE_FOE;
    return TRUE;
}

static u32 GetValueCount(u32 row)
{
    switch (row)
    {
    case ROW_HP_VALUE:
    case ROW_PRESET:
        return 3; // CUSTOM is display only.
    default:
        return 2;
    }
}

static u32 GetValue(u32 row, u32 side)
{
    const struct HealthboxOptions *o = &sSettings.options;
    bool32 player = side == HB_SIDE_PLAYER;

    switch (row)
    {
    case ROW_STYLE:       return o->style;
    case ROW_BACKGROUND:  return o->background;
    case ROW_PRESET:      return HealthboxOptions_GetPreset(o, side);
    case ROW_NAME:        return player ? o->playerNick : o->foeNick;
    case ROW_LEVEL:       return player ? o->playerLevel : o->foeLevel;
    case ROW_HP_BAR:      return player ? o->playerHpBar : o->foeHpBar;
    case ROW_HP_VALUE:    return player ? o->playerHpValue : o->foeHpValue;
    case ROW_EXP:         return o->playerExp;
    case ROW_STATUS:      return player ? o->playerStatus : o->foeStatus;
    case ROW_TYPES:       return player ? o->playerTypes : o->foeTypes;
    case ROW_CAUGHT:      return o->foeCaught;
    case ROW_CATCHABLE:   return o->foeCatchable;
    case ROW_STAT_STAGES: return player ? o->playerStatStages : o->foeStatStages;
    default:              return 0;
    }
}

static void SetValue(u32 row, u32 side, u32 value)
{
    struct HealthboxOptions *o = &sSettings.options;
    bool32 player = side == HB_SIDE_PLAYER;

    switch (row)
    {
    case ROW_STYLE:       o->style = value; break;
    case ROW_BACKGROUND:  o->background = value; break;
    case ROW_PRESET:      HealthboxOptions_ApplyPreset(o, side, value); break;
    case ROW_NAME:        if (player) o->playerNick = value; else o->foeNick = value; break;
    case ROW_LEVEL:       if (player) o->playerLevel = value; else o->foeLevel = value; break;
    case ROW_HP_BAR:      if (player) o->playerHpBar = value; else o->foeHpBar = value; break;
    case ROW_HP_VALUE:    if (player) o->playerHpValue = value; else o->foeHpValue = value; break;
    case ROW_EXP:         o->playerExp = value; break;
    case ROW_STATUS:      if (player) o->playerStatus = value; else o->foeStatus = value; break;
    case ROW_TYPES:       if (player) o->playerTypes = value; else o->foeTypes = value; break;
    case ROW_CAUGHT:      o->foeCaught = value; break;
    case ROW_CATCHABLE:   o->foeCatchable = value; break;
    case ROW_STAT_STAGES: if (player) o->playerStatStages = value; else o->foeStatStages = value; break;
    }
}

static const u8 *GetValueText(u32 row, u32 value)
{
    switch (row)
    {
    case ROW_STYLE:      return sStyleNames[value];
    case ROW_BACKGROUND: return sBackgroundNames[value];
    case ROW_HP_VALUE:   return sHpValueNames[value];
    case ROW_PRESET:     return sPresetNames[value];
    default:             return sOnOffNames[value];
    }
}

static void DrawCentred(u32 windowId, const u8 *color, s32 centreX, s32 y, const u8 *text)
{
    s32 width = GetStringWidth(FONT_SMALL_NARROW, text, 0);

    AddTextPrinterParameterized3(windowId, FONT_SMALL_NARROW, centreX - width / 2, y, color, TEXT_SKIP_DRAW, text);
}

static const u8 *GetCellColor(u32 row, u32 side)
{
    if (!IsRowActive(row) || !IsCellApplicable(row, side))
        return sColorDisabled;
    if (row == sSettings.row && (!IsPerSideRow(row) || side == sSettings.side))
        return sColorSelected;
    return sColorValue;
}

static void DrawCell(u32 row, u32 side, s32 centreX, s32 y)
{
    const u8 *text = IsCellApplicable(row, side) ? GetValueText(row, GetValue(row, side)) : sText_Dash;

    DrawCentred(WIN_LIST, GetCellColor(row, side), centreX, y, text);
}

static void DrawHeader(void)
{
    const u8 *youColor = sSettings.side == HB_SIDE_PLAYER ? sColorSelected : sColorValue;
    const u8 *foeColor = sSettings.side == HB_SIDE_FOE ? sColorSelected : sColorValue;

    FillWindowPixelBuffer(WIN_HEADER, PIXEL_FILL(1));
    AddTextPrinterParameterized(WIN_HEADER, FONT_NORMAL, sText_Header, 8, 1, TEXT_SKIP_DRAW, NULL);
    if (sSettings.top > 0 || sSettings.top + VISIBLE_ROWS < ROW_COUNT)
        AddTextPrinterParameterized3(WIN_HEADER, FONT_NORMAL, 84, 1, sColorValue, TEXT_SKIP_DRAW, sText_Scroll);
    if (!IsPerSideRow(sSettings.row))
        youColor = foeColor = sColorDisabled;
    DrawCentred(WIN_HEADER, youColor, COLUMN_YOU_X, 0, sText_You);
    DrawCentred(WIN_HEADER, foeColor, COLUMN_FOE_X, 0, sText_Foe);
    AddTextPrinterParameterized3(WIN_HEADER, FONT_SMALL_NARROW, SWITCH_HINT_X, 0, sColorLabel, TEXT_SKIP_DRAW, sText_SwitchHint);
    CopyWindowToVram(WIN_HEADER, COPYWIN_GFX);
}

static void HighlightSelectedRow(void)
{
    u32 y = LIST_PIXEL_TOP + (sSettings.row - sSettings.top) * ROW_PITCH;

    SetGpuReg(REG_OFFSET_WIN0H, WIN_RANGE(16, DISPLAY_WIDTH - 16));
    SetGpuReg(REG_OFFSET_WIN0V, WIN_RANGE(y, y + ROW_PITCH));
}

static void DrawListRow(u32 i, s32 y)
{
    u32 row = sSettings.top + i;

    AddTextPrinterParameterized3(WIN_LIST, FONT_SMALL_NARROW, LABEL_X, y,
                                 IsRowActive(row) ? sColorLabel : sColorDisabled, TEXT_SKIP_DRAW, sRowNames[row]);
    if (IsPerSideRow(row))
    {
        DrawCell(row, HB_SIDE_PLAYER, COLUMN_YOU_X, y);
        DrawCell(row, HB_SIDE_FOE, COLUMN_FOE_X, y);
    }
    else
    {
        DrawCell(row, HB_SIDE_PLAYER, COLUMN_SHARED_X, y);
    }
}

static void DrawList(void)
{
    u32 i;

    FillWindowPixelBuffer(WIN_LIST, PIXEL_FILL(1));
    for (i = 0; i < VISIBLE_ROWS - 1; i++)
        DrawListRow(i, i * ROW_PITCH + 1);
    ScrollWindow(WIN_LIST, 0, LIST_TEXT_Y_SHIFT, PIXEL_FILL(1));
    // The last row is drawn after the shift; at its unshifted y the window edge clips its glyphs.
    DrawListRow(VISIBLE_ROWS - 1, (VISIBLE_ROWS - 1) * ROW_PITCH + 1 - LIST_TEXT_Y_SHIFT);
    CopyWindowToVram(WIN_LIST, COPYWIN_GFX);
    HighlightSelectedRow();
}

static void Redraw(void)
{
    DrawHeader();
    DrawList();
}

static void ScrollToRow(void)
{
    if (sSettings.row < sSettings.top)
        sSettings.top = sSettings.row;
    else if (sSettings.row >= sSettings.top + VISIBLE_ROWS)
        sSettings.top = sSettings.row - VISIBLE_ROWS + 1;
}

// Moves the cursor to the next active row in dir (+1 or -1), wrapping.
static void MoveRow(s32 dir)
{
    u32 row = sSettings.row;
    u32 i;

    for (i = 0; i < ROW_COUNT; i++)
    {
        row = (row + ROW_COUNT + dir) % ROW_COUNT;
        if (IsRowActive(row))
        {
            sSettings.row = row;
            break;
        }
    }
    ScrollToRow();
}

static void ChangeValue(s32 dir)
{
    u32 row = sSettings.row;
    u32 count = GetValueCount(row);
    u32 side = sSettings.side;

    if (!IsRowActive(row) || !IsCellApplicable(row, side))
        return;
    if (row == ROW_PRESET && GetValue(row, side) == HB_PRESET_CUSTOM)
        SetValue(row, side, dir > 0 ? HB_PRESET_MINIMAL : HB_PRESET_FULL);
    else
        SetValue(row, side, (GetValue(row, side) + count + dir) % count);
}

static void DrawPreviewBox(const struct PreviewBox *box, const struct HealthboxResolvedOpts *opts, u32 slot)
{
    const struct HealthboxSprites *sprites = &box->sprites;
    const struct HealthboxLayout *layout = &box->layout;
    const struct HealthboxRect *rects = layout->rects;
    u8 text[POKEMON_NAME_LENGTH + 12];

    HealthboxRender_Clear(sprites, layout);
    HealthboxRender_DrawFrame(sprites, layout, opts->background);

    if (rects[HB_RECT_NICK].w != 0)
    {
        StringAppend(StringCopy(text, sText_SampleNick), gText_HealthboxGender_Male);
        HealthboxRender_PrintText(sprites, layout, &rects[HB_RECT_NICK], text, FALSE, opts->background);
    }
    if (rects[HB_RECT_LEVEL].w != 0)
    {
        text[0] = CHAR_EXTRA_SYMBOL;
        text[1] = CHAR_LV_2;
        ConvertIntToDecimalStringN(text + 2, SAMPLE_LEVEL, STR_CONV_MODE_LEFT_ALIGN, 3);
        HealthboxRender_PrintText(sprites, layout, &rects[HB_RECT_LEVEL], text, TRUE, opts->background);
    }
    if (rects[HB_RECT_HP_VALUE].w != 0)
    {
        HealthboxRender_FormatHpValue(text, opts->hpValue, SAMPLE_HP, SAMPLE_MAX_HP);
        if (layout->compactHpValue)
            HealthboxRender_DrawHpValue(sprites, layout, &rects[HB_RECT_HP_VALUE], text, opts->background);
        else
            HealthboxRender_PrintText(sprites, layout, &rects[HB_RECT_HP_VALUE], text, TRUE, opts->background);
    }
    if (rects[HB_RECT_HP_BAR].w != 0)
        HealthboxRender_DrawHpBar(sprites->bar, HB_HP_BAR_W * SAMPLE_HP / SAMPLE_MAX_HP, 0, SAMPLE_HP_COLOUR);
    if (rects[HB_RECT_EXP].w != 0)
        HealthboxRender_DrawExpBar(sprites, layout, &rects[HB_RECT_EXP], rects[HB_RECT_EXP].w / 2);
    if (rects[HB_RECT_STATUS].w != 0)
    {
        LoadHealthboxStatusColor(sprites->left, slot, PAL_STATUS_PSN);
        HealthboxRender_DrawStatusPill(sprites, layout, &rects[HB_RECT_STATUS], HB_PAL_STATUS_FIRST + slot,
                                       sText_SamplePsn, opts->background);
    }
    if (rects[HB_RECT_STRIP].w != 0)
        HealthboxRender_DrawStatStrip(sprites, layout, &rects[HB_RECT_STRIP], sSampleStages, opts->background);
    if (rects[HB_RECT_CAUGHT].w != 0)
        HealthboxPreview_LoadCaughtIcon(sprites->bar);
    if (rects[HB_RECT_CATCHABLE].w != 0)
        HealthboxPreview_LoadCatchIcon(sprites->bar);
}

// Types pop out beside the outer edge: left of the singles box, right of the doubles box.
static void RefreshTypeIcon(struct PreviewBox *box, u32 slot, bool32 show)
{
    if (!show)
        return;

    box->typeIcon = TypeIcons_CreatePreviewIcon(SAMPLE_TYPE, sPreviewX[slot], sPreviewX[slot] + box->layout.boxW,
                                                gSprites[box->sprites.left].y - HB_BOX_CENTER_Y,
                                                gSprites[box->sprites.left].y - HB_BOX_CENTER_Y + box->layout.boxH,
                                                slot == PREVIEW_DOUBLES, 0);
}

static void PlacePreviewBox(const struct PreviewBox *box, u32 slot)
{
    struct Sprite *left = &gSprites[box->sprites.left];
    struct Sprite *right = &gSprites[box->sprites.right];
    struct Sprite *bar = &gSprites[box->sprites.bar];
    s32 top = PREVIEW_PIXEL_TOP + (PREVIEW_PIXEL_H - box->layout.boxH) / 2;

    left->x = sPreviewX[slot] + HB_BOX_CENTER_X;
    left->y = top + HB_BOX_CENTER_Y;
    // The narrow right half of a doubles box starts 32 px in, as in battle.
    right->x = left->x + 64 - (box->layout.rightSpriteW == 32 ? 16 : 0);
    right->y = left->y;
    bar->x = left->x;
    bar->y = left->y;
}

static void DestroyPreviewSprites(struct HealthboxSprites *sprites)
{
    u8 *ids[] = { &sprites->left, &sprites->right, &sprites->bar };
    u32 i;

    for (i = 0; i < ARRAY_COUNT(ids); i++)
    {
        if (*ids[i] != SPRITE_NONE)
        {
            DestroySprite(&gSprites[*ids[i]]);
            *ids[i] = SPRITE_NONE;
        }
    }
}

static void CreateNewPreviewBox(struct PreviewBox *box, u32 slot)
{
    struct Sprite *bar;

    HealthboxRender_CreateBox(TRUE, slot, box->layout.rightSpriteW, &box->sprites);
    box->sprites.bar = CreateSpriteAtEnd(&sPreviewBarTemplates[slot], 0, 0, 0);
    bar = &gSprites[box->sprites.bar];
    bar->oam.tileNum += HealthboxPreview_GetBarTileOffset();
    bar->subspriteMode = SUBSPRITES_IGNORE_PRIORITY;
    bar->oam.priority = 1;
    // Blank any Classic bar or icon art left in the shared bar tiles.
    CpuFill32(0, (void *)(OBJ_VRAM0 + bar->oam.tileNum * TILE_SIZE_4BPP), HB_PREVIEW_BAR_TILES * TILE_SIZE_4BPP);
}

// Rebuilds both boxes for the selected column. Classic and New use different sprite shapes and art, so
// the sprites are recreated on every refresh.
//
// A rebuild writes box tiles pixel by pixel and runs over several frames, so it is kept off screen:
// the new sprites draw into the sheet half the displayed ones do not use, the palette transfer is held
// until the art is complete, and the old sprites are destroyed only at the end. OAM is rebuilt once per
// frame from the main loop, so the swap lands in a single frame.
static void RefreshPreview(void)
{
    struct HealthboxSprites oldSprites[PREVIEW_COUNT];
    u8 oldTypeIcon[PREVIEW_COUNT];
    u32 slot;
    u32 side = sSettings.side;
    bool32 classic = sSettings.options.style == HEALTHBOX_STYLE_CLASSIC;

    gPaletteFade.bufferTransferDisabled = TRUE;
    sSettings.buffer ^= 1;

    FillWindowPixelBuffer(WIN_PREVIEW, PIXEL_FILL(0));
    CopyWindowToVram(WIN_PREVIEW, COPYWIN_GFX);

    // Both boxes share one palette slot, so it is switched once per refresh.
    if (classic)
        HealthboxPreview_ApplyClassicPalette();
    else
        HealthboxBattle_ApplyPalette();

    for (slot = 0; slot < PREVIEW_COUNT; slot++)
    {
        struct PreviewBox *box = &sSettings.preview[slot];
        struct HealthboxResolvedOpts opts;

        oldSprites[slot] = box->sprites;
        oldTypeIcon[slot] = box->typeIcon;
        box->sprites.left = box->sprites.right = box->sprites.bar = SPRITE_NONE;
        box->typeIcon = SPRITE_NONE;

        HealthboxPreview_SetTileOffsets(sSettings.buffer ? sBoxBufferTiles[slot] : 0,
                                        sSettings.buffer ? HB_PREVIEW_BAR_TILES : 0);
        Healthbox_ResolveOptsFrom(&sSettings.options, side, &opts);

        if (classic)
        {
            HealthboxPreview_CreateClassic(&box->sprites, side, slot == PREVIEW_DOUBLES, slot,
                                           opts.hpBar, opts.hpValue, sPreviewX[slot],
                                           PREVIEW_PIXEL_TOP, PREVIEW_PIXEL_H);
            continue;
        }

        Healthbox_ComputeLayout(&opts, side, slot == PREVIEW_DOUBLES,
                                side == HB_SIDE_PLAYER ? B_POSITION_PLAYER_LEFT : B_POSITION_OPPONENT_LEFT,
                                &box->layout);
        CreateNewPreviewBox(box, slot);
        HealthboxRender_BuildBarSubsprites(&box->layout, box->subsprites, &box->subspriteTable);
        SetSubspriteTables(&gSprites[box->sprites.bar], &box->subspriteTable);
        gSprites[box->sprites.bar].invisible = box->subspriteTable.subspriteCount == 0;

        PlacePreviewBox(box, slot);
        DrawPreviewBox(box, &opts, slot);
        RefreshTypeIcon(box, slot, opts.types);
    }

    HealthboxPreview_SetTileOffsets(0, 0);

    for (slot = 0; slot < PREVIEW_COUNT; slot++)
    {
        DestroyPreviewSprites(&oldSprites[slot]);
        if (oldTypeIcon[slot] != SPRITE_NONE)
            DestroySprite(&gSprites[oldTypeIcon[slot]]);
    }

    gPaletteFade.bufferTransferDisabled = FALSE;
}

static void CreatePreview(void)
{
    u32 slot;

    for (slot = 0; slot < ARRAY_COUNT(sPreviewSheets); slot++)
        LoadCompressedSpriteSheet(&sPreviewSheets[slot]);
    HealthboxBattle_LoadPalette();
    LoadSpritePalette(&sPreviewBarPalette);

    for (slot = 0; slot < PREVIEW_COUNT; slot++)
    {
        struct PreviewBox *box = &sSettings.preview[slot];

        box->sprites.left = box->sprites.right = box->sprites.bar = SPRITE_NONE;
        box->typeIcon = SPRITE_NONE;
    }
    RefreshPreview();
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

    FillBgTilemapBufferRect(1, TILE_TOP_CORNER_L,  1, LIST_TILE_TOP - 1,  1,  1,  7);
    FillBgTilemapBufferRect(1, TILE_TOP_EDGE,      2, LIST_TILE_TOP - 1, 26,  1,  7);
    FillBgTilemapBufferRect(1, TILE_TOP_CORNER_R, 28, LIST_TILE_TOP - 1,  1,  1,  7);
    FillBgTilemapBufferRect(1, TILE_LEFT_EDGE,     1, LIST_TILE_TOP,  1,  9,  7);
    FillBgTilemapBufferRect(1, TILE_RIGHT_EDGE,   28, LIST_TILE_TOP,  1,  9,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_CORNER_L,  1, LIST_TILE_TOP + 9,  1,  1,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_EDGE,      2, LIST_TILE_TOP + 9, 26,  1,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_CORNER_R, 28, LIST_TILE_TOP + 9,  1,  1,  7);

    CopyBgTilemapBufferToVram(1);
}

// The caller sets gMain.savedCallback to the return screen before switching to this one.
void CB2_InitHealthboxSettings(void)
{
    switch (gMain.state)
    {
    default:
    case 0:
        SetVBlankCallback(NULL);
        HealthboxOptions_Get(&sSettings.options);
        sSettings.row = ROW_STYLE;
        sSettings.top = 0;
        sSettings.side = HB_SIDE_PLAYER;
        gMain.state++;
        break;
    case 1:
        DmaClearLarge16(3, (void *)(VRAM), VRAM_SIZE, 0x1000);
        DmaClear32(3, OAM, OAM_SIZE);
        DmaClear16(3, PLTT, PLTT_SIZE);
        SetGpuReg(REG_OFFSET_DISPCNT, 0);
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(0, sBgTemplates, ARRAY_COUNT(sBgTemplates));
        ChangeBgX(0, 0, BG_COORD_SET);
        ChangeBgY(0, 0, BG_COORD_SET);
        ChangeBgX(1, 0, BG_COORD_SET);
        ChangeBgY(1, 0, BG_COORD_SET);
        InitWindows(sWinTemplates);
        DeactivateAllTextPrinters();
        // WIN0 marks the selected row; BG0 outside it is darkened. OBJs stay undimmed.
        SetGpuReg(REG_OFFSET_WIN0H, 0);
        SetGpuReg(REG_OFFSET_WIN0V, 0);
        SetGpuReg(REG_OFFSET_WININ, WININ_WIN0_BG0 | WININ_WIN0_BG1 | WININ_WIN0_OBJ);
        SetGpuReg(REG_OFFSET_WINOUT, WINOUT_WIN01_BG0 | WINOUT_WIN01_BG1 | WINOUT_WIN01_OBJ | WINOUT_WIN01_CLR);
        SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_BG0 | BLDCNT_EFFECT_DARKEN);
        SetGpuReg(REG_OFFSET_BLDALPHA, 0);
        SetGpuReg(REG_OFFSET_BLDY, 4);
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_WIN0_ON | DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
        ShowBg(0);
        ShowBg(1);
        gMain.state++;
        break;
    case 2:
        ResetPaletteFade();
        ScanlineEffect_Stop();
        ResetTasks();
        ResetSpriteData();
        FreeAllSpritePalettes();
        FreeSpriteTileRanges();
        gMain.state++;
        break;
    case 3:
        LoadBgTiles(1, GetWindowFrameTilesPal(gSaveBlock2Ptr->optionsWindowFrameType)->tiles, 0x120, 0x1A2);
        gMain.state++;
        break;
    case 4:
        LoadPalette(sBg_Pal, BG_PLTT_ID(0), sizeof(sBg_Pal));
        LoadPalette(GetWindowFrameTilesPal(gSaveBlock2Ptr->optionsWindowFrameType)->pal, BG_PLTT_ID(7), PLTT_SIZE_4BPP);
        gMain.state++;
        break;
    case 5:
        LoadPalette(sText_Pal, BG_PLTT_ID(1), sizeof(sText_Pal));
        gMain.state++;
        break;
    case 6:
        PutWindowTilemap(WIN_HEADER);
        PutWindowTilemap(WIN_LIST);
        PutWindowTilemap(WIN_PREVIEW);
        DrawBgWindowFrames();
        CopyBgTilemapBufferToVram(0);
        Redraw();
        CreatePreview();
        gMain.state++;
        break;
    case 7:
        CreateTask(Task_FadeIn, 0);
        BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
        SetVBlankCallback(VBlankCB);
        SetMainCallback2(MainCB2);
        return;
    }
}

static void Task_FadeIn(u8 taskId)
{
    if (!gPaletteFade.active)
        gTasks[taskId].func = Task_ProcessInput;
}

static void Task_ProcessInput(u8 taskId)
{
    if (JOY_NEW(A_BUTTON | B_BUTTON | START_BUTTON))
    {
        HealthboxOptions_Commit(&sSettings.options);
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        gTasks[taskId].func = Task_FadeOut;
    }
    else if (JOY_NEW(L_BUTTON | R_BUTTON))
    {
        sSettings.side ^= 1;
        PlaySE(SE_SELECT);
        Redraw();
        RefreshPreview();
    }
    else if (JOY_REPEAT(DPAD_UP))
    {
        MoveRow(-1);
        Redraw();
    }
    else if (JOY_REPEAT(DPAD_DOWN))
    {
        MoveRow(1);
        Redraw();
    }
    else if (JOY_NEW(DPAD_LEFT | DPAD_RIGHT))
    {
        ChangeValue(JOY_NEW(DPAD_RIGHT) ? 1 : -1);
        PlaySE(SE_SELECT);
        Redraw();
        RefreshPreview();
    }
}

static void Task_FadeOut(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        DestroyTask(taskId);
        FreeAllWindowBuffers();
        SetMainCallback2(gMain.savedCallback);
    }
}
