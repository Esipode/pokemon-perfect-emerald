#include "global.h"
#include "battle.h"
#include "battle_interface.h"
#include "healthbox.h"
#include "sprite.h"
#include "string_util.h"
#include "text.h"
#include "constants/characters.h"
#include "constants/healthbox.h"
#include "constants/rgb.h"
#include "data/healthbox.h"

#define HB_LEFT_W   64
#define HB_SPRITE_H 32

// Bar sprite tiles: 0-5 HP fill, 8-9 opponent icon slots (caught ball, can/cannot catch).
#define HB_BAR_TILE_ICON_CAUGHT 8
#define HB_BAR_TILE_ICON_CATCH  9

static const struct OamData sOamData_HealthboxBox =
{
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(64x32),
    .size = SPRITE_SIZE(64x32),
    .priority = 1,
};

static const struct SpriteTemplate sPlayerBoxTemplates[2] =
{
    { .tileTag = TAG_HEALTHBOX_PLAYER1_TILE, .paletteTag = TAG_HEALTHBOX_PAL, .oam = &sOamData_HealthboxBox },
    { .tileTag = TAG_HEALTHBOX_PLAYER2_TILE, .paletteTag = TAG_HEALTHBOX_PAL, .oam = &sOamData_HealthboxBox },
};

static const struct SpriteTemplate sOpponentBoxTemplates[2] =
{
    { .tileTag = TAG_HEALTHBOX_OPPONENT1_TILE, .paletteTag = TAG_HEALTHBOX_PAL, .oam = &sOamData_HealthboxBox },
    { .tileTag = TAG_HEALTHBOX_OPPONENT2_TILE, .paletteTag = TAG_HEALTHBOX_PAL, .oam = &sOamData_HealthboxBox },
};

// Settings-preview double buffering; 0 in battle.
static u32 sPreviewBoxTileOffset;
static u32 sPreviewBarTileOffset;

void HealthboxPreview_SetTileOffsets(u32 boxTiles, u32 barTiles)
{
    sPreviewBoxTileOffset = boxTiles;
    sPreviewBarTileOffset = barTiles;
}

u32 HealthboxPreview_GetBoxTileOffset(void)
{
    return sPreviewBoxTileOffset;
}

u32 HealthboxPreview_GetBarTileOffset(void)
{
    return sPreviewBarTileOffset;
}

void HealthboxRender_CreateBox(bool32 player, u32 tagIdx, u32 rightSpriteW, struct HealthboxSprites *out)
{
    const struct SpriteTemplate *template = (player ? sPlayerBoxTemplates : sOpponentBoxTemplates) + tagIdx;
    struct Sprite *right;

    out->left = CreateSprite(template, DISPLAY_WIDTH, DISPLAY_HEIGHT, 1);
    out->right = CreateSpriteAtEnd(template, DISPLAY_WIDTH, DISPLAY_HEIGHT, 1);
    out->bar = SPRITE_NONE;

    gSprites[out->left].oam.tileNum += sPreviewBoxTileOffset;
    right = &gSprites[out->right];
    right->oam.tileNum += HB_BOX_TILES_LEFT + sPreviewBoxTileOffset;
    if (rightSpriteW == 32)
    {
        right->oam.shape = SPRITE_SHAPE(32x32);
        right->oam.size = SPRITE_SIZE(32x32);
        // The corner vector was computed from the 64x32 template.
        CalcCenterToCornerVec(right, right->oam.shape, right->oam.size, right->oam.affineMode);
    }
    gSprites[out->left].oam.affineParam = out->right;
}

static void AddBarSubsprite(struct Subsprite *subsprites, struct SubspriteTable *table, const struct HealthboxRect *rect,
                            s32 dx, s32 dy, u32 shape, u32 size, u32 tile)
{
    struct Subsprite *sub = &subsprites[table->subspriteCount++];

    // Offsets are from the bar sprite origin, which sits on the box main sprite's centre.
    sub->x = rect->x + dx - HB_BOX_CENTER_X;
    sub->y = rect->y + dy - HB_BOX_CENTER_Y;
    sub->shape = shape;
    sub->size = size;
    sub->tileOffset = tile;
    sub->priority = 1;
}

void HealthboxRender_BuildBarSubsprites(const struct HealthboxLayout *layout, struct Subsprite *subsprites,
                                        struct SubspriteTable *table)
{
    const struct HealthboxRect *bar = &layout->rects[HB_RECT_HP_BAR];
    const struct HealthboxRect *caught = &layout->rects[HB_RECT_CAUGHT];
    const struct HealthboxRect *catchable = &layout->rects[HB_RECT_CATCHABLE];

    table->subspriteCount = 0;
    table->subsprites = subsprites;

    if (bar->w != 0)
    {
        AddBarSubsprite(subsprites, table, bar, 0, -HB_HP_BAR_ROW, SPRITE_SHAPE(32x8), SPRITE_SIZE(32x8), 0);
        AddBarSubsprite(subsprites, table, bar, 32, -HB_HP_BAR_ROW, SPRITE_SHAPE(16x8), SPRITE_SIZE(16x8), 4);
    }
    if (caught->w != 0)
        AddBarSubsprite(subsprites, table, caught, 0, 0, SPRITE_SHAPE(8x8), SPRITE_SIZE(8x8), HB_BAR_TILE_ICON_CAUGHT);
    if (catchable->w != 0)
        AddBarSubsprite(subsprites, table, catchable, 0, 0, SPRITE_SHAPE(8x8), SPRITE_SIZE(8x8), HB_BAR_TILE_ICON_CATCH);
}

u8 *HealthboxRender_FormatHpValue(u8 *dst, u32 mode, s32 currHp, s32 maxHp)
{
    if (mode == HB_HPVAL_PERCENT)
    {
        s32 percent = currHp == 0 || maxHp <= 0 ? 0 : max((currHp * 100) / maxHp, 1);

        dst = ConvertIntToDecimalStringN(dst, percent, STR_CONV_MODE_LEFT_ALIGN, 3);
        *dst++ = CHAR_PERCENT;
    }
    else
    {
        dst = ConvertIntToDecimalStringN(dst, currHp, STR_CONV_MODE_LEFT_ALIGN, 4);
        *dst++ = CHAR_SLASH;
        dst = ConvertIntToDecimalStringN(dst, maxHp, STR_CONV_MODE_LEFT_ALIGN, 4);
    }
    *dst = EOS;
    return dst;
}

void HealthboxRender_PutPixel(const struct HealthboxSprites *sprites, const struct HealthboxLayout *layout,
                              s32 x, s32 y, u8 palIndex)
{
    const struct Sprite *sprite;
    u32 spriteW, tile, shift;
    u16 *halfword;

    if (x < 0 || y < 0 || y >= HB_SPRITE_H)
        return;

    if (x < HB_LEFT_W)
    {
        sprite = &gSprites[sprites->left];
        spriteW = HB_LEFT_W;
    }
    else
    {
        x -= HB_LEFT_W;
        if (x >= layout->rightSpriteW)
            return;
        sprite = &gSprites[sprites->right];
        spriteW = layout->rightSpriteW;
    }

    // 1D tile mapping; OBJ VRAM ignores byte writes, so edit whole halfwords (4 pixels each).
    tile = (y / 8) * (spriteW / 8) + x / 8;
    halfword = (u16 *)(OBJ_VRAM0 + (sprite->oam.tileNum + tile) * TILE_SIZE_4BPP + (y % 8) * 4 + ((x % 8) / 4) * 2);
    shift = (x % 4) * 4;
    *halfword = (*halfword & ~(0xF << shift)) | ((palIndex & 0xF) << shift);
}

void HealthboxRender_FillRect(const struct HealthboxSprites *sprites, const struct HealthboxLayout *layout,
                              s32 x, s32 y, s32 w, s32 h, u8 palIndex)
{
    s32 i, j;

    for (j = 0; j < h; j++)
    {
        for (i = 0; i < w; i++)
            HealthboxRender_PutPixel(sprites, layout, x + i, y + j, palIndex);
    }
}

void HealthboxRender_Clear(const struct HealthboxSprites *sprites, const struct HealthboxLayout *layout)
{
    CpuFill32(0, (void *)(OBJ_VRAM0 + gSprites[sprites->left].oam.tileNum * TILE_SIZE_4BPP),
              (HB_LEFT_W / 8) * (HB_SPRITE_H / 8) * TILE_SIZE_4BPP);
    CpuFill32(0, (void *)(OBJ_VRAM0 + gSprites[sprites->right].oam.tileNum * TILE_SIZE_4BPP),
              (layout->rightSpriteW / 8) * (HB_SPRITE_H / 8) * TILE_SIZE_4BPP);
}

// Box-local x, sprite-local print y; handles text crossing the sprite seam.
static void PrintTextAt(const struct HealthboxSprites *sprites, u32 fontId, s32 x, s32 top, const u8 *str,
                        union TextColor color)
{
    struct Sprite *left = &gSprites[sprites->left];
    struct Sprite *right = &gSprites[sprites->right];
    s16 savedLeft1 = left->data[1], savedRight1 = right->data[1];

    // The text printer spills into the sprite named by data[1].
    left->data[1] = sprites->right;
    right->data[1] = SPRITE_NONE;

    if (x >= HB_LEFT_W)
        AddSpriteTextPrinterParameterized6(sprites->right, fontId, x - HB_LEFT_W, top, 0, 0, color, 0, str);
    else
        AddSpriteTextPrinterParameterized6(sprites->left, fontId, x, top, 0, 0, color, 0, str);

    left->data[1] = savedLeft1;
    right->data[1] = savedRight1;
}

void HealthboxRender_PrintText(const struct HealthboxSprites *sprites, const struct HealthboxLayout *layout,
                               const struct HealthboxRect *rect, const u8 *str, bool32 rightAlign, u32 background)
{
    u32 fontId = GetFontIdToFit(str, FONT_SMALL, 0, rect->w);
    s32 x = rect->x;
    s32 top = rect->y - HB_TEXT_INK_OFFSET;
    s32 clearH = rect->h;

    if (rightAlign)
        x += rect->w - GetStringWidth(fontId, str, 0);
    if (x < 0)
        x = 0;
    if (top < 0)
    {
        // The print y is unsigned; the ink sits lower, so clear the extra rows too.
        clearH -= top;
        top = 0;
    }

    HealthboxRender_FillRect(sprites, layout, rect->x, rect->y, rect->w, clearH, background == HB_BG_NONE ? 0 : HB_PAL_FILL);

    PrintTextAt(sprites, fontId, x, top, str, sHealthboxTextColor);
}

static u32 HpValueGlyphIndex(u8 c)
{
    if (c >= CHAR_0 && c <= CHAR_9)
        return c - CHAR_0;
    if (c == CHAR_SLASH)
        return HB_NUM_SLASH;
    if (c == CHAR_PERCENT)
        return HB_NUM_PERCENT;
    return HB_NUM_COUNT;
}

void HealthboxRender_DrawHpValue(const struct HealthboxSprites *sprites, const struct HealthboxLayout *layout,
                                 const struct HealthboxRect *rect, const u8 *str, u32 background)
{
    s32 x, top, row, col;

    HealthboxRender_FillRect(sprites, layout, rect->x, rect->y, rect->w, rect->h,
                             background == HB_BG_NONE ? 0 : HB_PAL_FILL);

    x = rect->x;
    top = rect->y + (rect->h - HB_NUM_GLYPH_H) / 2;
    for (; *str != EOS; str++)
    {
        u32 glyph = HpValueGlyphIndex(*str);

        if (glyph != HB_NUM_COUNT)
        {
            for (row = 0; row < HB_NUM_GLYPH_H; row++)
            {
                for (col = 0; col < HB_NUM_GLYPH_W; col++)
                {
                    if (sHpValueGlyphs[glyph][row] & (1 << (HB_NUM_GLYPH_W - 1 - col)))
                        HealthboxRender_PutPixel(sprites, layout, x + col, top + row, HB_PAL_TEXT);
                }
            }
        }
        x += HB_NUM_GLYPH_W + HB_NUM_GLYPH_GAP;
    }
}

void HealthboxRender_DrawExpBar(const struct HealthboxSprites *sprites, const struct HealthboxLayout *layout,
                                const struct HealthboxRect *rect, u32 fillPx)
{
    if (fillPx > rect->w)
        fillPx = rect->w;

    HealthboxRender_FillRect(sprites, layout, rect->x, rect->y, fillPx, rect->h, HB_PAL_EXP);
    HealthboxRender_FillRect(sprites, layout, rect->x + fillPx, rect->y, rect->w - fillPx, rect->h, HB_PAL_TROUGH);
}

static const u8 *PillGlyph(u8 c)
{
    if (c < CHAR_A || c > CHAR_Z)
        return NULL;
    return sPillGlyphs[c - CHAR_A];
}

static s32 PillLabelWidth(const u8 *label)
{
    s32 len = 0;

    while (label[len] != EOS)
        len++;
    return len == 0 ? 0 : len * (HB_PILL_GLYPH_W + HB_PILL_GLYPH_GAP) - HB_PILL_GLYPH_GAP;
}

void HealthboxRender_DrawStatusPill(const struct HealthboxSprites *sprites, const struct HealthboxLayout *layout,
                                    const struct HealthboxRect *rect, u32 palIndex, const u8 *label, u32 background)
{
    u8 backdrop = background == HB_BG_NONE ? 0 : HB_PAL_FILL;
    s32 x, top, row, col;

    HealthboxRender_FillRect(sprites, layout, rect->x, rect->y, rect->w, rect->h, backdrop);
    if (label == NULL)
        return;

    HealthboxRender_FillRect(sprites, layout, rect->x, rect->y, rect->w, rect->h, palIndex);
    // Round the corners.
    HealthboxRender_PutPixel(sprites, layout, rect->x, rect->y, backdrop);
    HealthboxRender_PutPixel(sprites, layout, rect->x + rect->w - 1, rect->y, backdrop);
    HealthboxRender_PutPixel(sprites, layout, rect->x, rect->y + rect->h - 1, backdrop);
    HealthboxRender_PutPixel(sprites, layout, rect->x + rect->w - 1, rect->y + rect->h - 1, backdrop);

    x = rect->x + (rect->w - PillLabelWidth(label)) / 2;
    top = rect->y + (rect->h - HB_PILL_GLYPH_H) / 2;
    for (; *label != EOS; label++)
    {
        const u8 *glyph = PillGlyph(*label);

        if (glyph != NULL)
        {
            for (row = 0; row < HB_PILL_GLYPH_H; row++)
            {
                for (col = 0; col < HB_PILL_GLYPH_W; col++)
                {
                    if (glyph[row] & (1 << (HB_PILL_GLYPH_W - 1 - col)))
                        HealthboxRender_PutPixel(sprites, layout, x + col, top + row, HB_PAL_TEXT);
                }
            }
        }
        x += HB_PILL_GLYPH_W + HB_PILL_GLYPH_GAP;
    }
}

// Gimmick badge: a 32x16 sprite holding a rounded pill with a short label, sized to the label and
// centred in the sprite. It is not part of a healthbox sprite pair, so it plots straight to OBJ VRAM.
#define HB_BADGE_MAX_LABEL 4
STATIC_ASSERT(HB_BADGE_MAX_LABEL * (HB_PILL_GLYPH_W + HB_PILL_GLYPH_GAP) - HB_PILL_GLYPH_GAP
              + 2 * HB_BADGE_PILL_PAD <= HB_BADGE_W, GimmickBadgeLabelFitsSprite);

static void BadgePutPixel(u32 tileNum, s32 x, s32 y, u8 palIndex)
{
    u32 tile, shift;
    u16 *halfword;

    if (x < 0 || y < 0 || x >= HB_BADGE_W || y >= HB_BADGE_H)
        return;

    tile = (y / 8) * (HB_BADGE_W / 8) + x / 8;
    halfword = (u16 *)(OBJ_VRAM0 + (tileNum + tile) * TILE_SIZE_4BPP + (y % 8) * 4 + ((x % 8) / 4) * 2);
    shift = (x % 4) * 4;
    *halfword = (*halfword & ~(0xF << shift)) | ((palIndex & 0xF) << shift);
}

void HealthboxRender_DrawGimmickBadge(u32 tileNum, const u8 *label, bool32 selected)
{
    u8 body = selected ? HB_PAL_RIM : HB_PAL_FILL;
    u8 rim = selected ? HB_PAL_TEXT : HB_PAL_RIM;
    s32 labelW = PillLabelWidth(label);
    s32 w = labelW + 2 * HB_BADGE_PILL_PAD;
    s32 x0 = (HB_BADGE_W - w) / 2;
    s32 y0 = (HB_BADGE_H - HB_BADGE_PILL_H) / 2;
    s32 x, y, row, col;

    CpuFill32(0, (void *)(OBJ_VRAM0 + tileNum * TILE_SIZE_4BPP), HB_BADGE_TILES * TILE_SIZE_4BPP);

    for (y = 0; y < HB_BADGE_PILL_H; y++)
    {
        for (x = 0; x < w; x++)
        {
            bool32 edge = x == 0 || x == w - 1 || y == 0 || y == HB_BADGE_PILL_H - 1;

            BadgePutPixel(tileNum, x0 + x, y0 + y, edge ? rim : body);
        }
    }
    // Round the corners.
    BadgePutPixel(tileNum, x0, y0, 0);
    BadgePutPixel(tileNum, x0 + w - 1, y0, 0);
    BadgePutPixel(tileNum, x0, y0 + HB_BADGE_PILL_H - 1, 0);
    BadgePutPixel(tileNum, x0 + w - 1, y0 + HB_BADGE_PILL_H - 1, 0);

    x = x0 + HB_BADGE_PILL_PAD;
    y = y0 + (HB_BADGE_PILL_H - HB_PILL_GLYPH_H) / 2;
    for (; *label != EOS; label++)
    {
        const u8 *glyph = PillGlyph(*label);

        if (glyph != NULL)
        {
            for (row = 0; row < HB_PILL_GLYPH_H; row++)
            {
                for (col = 0; col < HB_PILL_GLYPH_W; col++)
                {
                    if (glyph[row] & (1 << (HB_PILL_GLYPH_W - 1 - col)))
                        BadgePutPixel(tileNum, x + col, y + row, HB_PAL_TEXT);
                }
            }
        }
        x += HB_PILL_GLYPH_W + HB_PILL_GLYPH_GAP;
    }
}

void HealthboxRender_DrawStatStrip(const struct HealthboxSprites *sprites, const struct HealthboxLayout *layout,
                                   const struct HealthboxRect *rect, const u8 stages[HB_STRIP_SLOTS], u32 background)
{
    u8 backdrop = background == HB_BG_NONE ? 0 : HB_PAL_FILL;
    u32 slot;
    s32 row, col;

    HealthboxRender_FillRect(sprites, layout, rect->x, rect->y, rect->w, rect->h, backdrop);

    for (slot = 0; slot < HB_STRIP_SLOTS; slot++)
    {
        u32 glyph = Healthbox_StageToGlyph(stages[slot]);
        s32 x0 = rect->x + slot * HB_STRIP_SLOT_W + (HB_STRIP_SLOT_W - HB_GLYPH_W) / 2;

        if (glyph == HB_GLYPH_NONE)
            continue;

        for (row = 0; row < HB_GLYPH_H; row++)
        {
            for (col = 0; col < HB_GLYPH_W; col++)
            {
                if (sStatGlyphs[glyph][row] & (1 << (HB_GLYPH_W - 1 - col)))
                    HealthboxRender_PutPixel(sprites, layout, x0 + col, rect->y + row, sStatStripHue[slot]);
            }
        }
    }
}

void HealthboxRender_DrawHpBar(u8 barSpriteId, u32 fillPx, u32 trailPx, u32 colourLevel)
{
    static const u8 sFillPal[][2] = { {10, 11}, {12, 13}, {14, 15} };
    u32 tile, row, px;
    u32 tileNum = gSprites[barSpriteId].oam.tileNum;

    if (colourLevel >= ARRAY_COUNT(sFillPal))
        colourLevel = ARRAY_COUNT(sFillPal) - 1;

    // The bar occupies rows 2-5 of each 8x8 tile; the rest stays transparent.
    for (tile = 0; tile < HB_HP_BAR_TILES; tile++)
    {
        u32 data[8] = {0};

        for (row = 0; row < HB_HP_BAR_H; row++)
        {
            for (px = 0; px < 8; px++)
            {
                u32 x = tile * 8 + px;
                u32 idx;

                if ((x == 0 || x == HB_HP_BAR_W - 1) && (row == 0 || row == HB_HP_BAR_H - 1))
                    continue; // rounded ends
                if (x < fillPx)
                    idx = sFillPal[colourLevel][row < HB_HP_BAR_H / 2 ? 0 : 1];
                else if (x < trailPx)
                    idx = HB_PAL_BAR_TRAIL;
                else
                    idx = HB_PAL_BAR_TROUGH;
                data[row + HB_HP_BAR_ROW] |= idx << (px * 4);
            }
        }
        CpuCopy32(data, (void *)(OBJ_VRAM0 + (tileNum + tile) * TILE_SIZE_4BPP), TILE_SIZE_4BPP);
    }
}

static bool32 IsInsideFrame(s32 x, s32 y, s32 w, s32 h)
{
    s32 dy, inset;

    if (x < 0 || y < 0 || x >= w || y >= h)
        return FALSE;

    dy = y < h - 1 - y ? y : h - 1 - y;
    if (dy < (s32)ARRAY_COUNT(sHealthboxCornerInset))
    {
        inset = sHealthboxCornerInset[dy];
        return x >= inset && x < w - inset;
    }
    return TRUE;
}

void HealthboxRender_DrawFrame(const struct HealthboxSprites *sprites, const struct HealthboxLayout *layout,
                               u32 background)
{
    s32 x, y, w = layout->boxW, h = layout->boxH;

    if (background == HB_BG_NONE)
        return;

    for (y = 0; y < h; y++)
    {
        for (x = 0; x < w; x++)
        {
            bool32 rim;

            if (!IsInsideFrame(x, y, w, h))
                continue;
            rim = !IsInsideFrame(x - 1, y, w, h) || !IsInsideFrame(x + 1, y, w, h)
               || !IsInsideFrame(x, y - 1, w, h) || !IsInsideFrame(x, y + 1, w, h);
            HealthboxRender_PutPixel(sprites, layout, x, y, rim ? HB_PAL_RIM : HB_PAL_FILL);
        }
    }
}
