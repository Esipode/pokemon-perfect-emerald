#include "global.h"
#include "healthbox.h"
#include "sprite.h"
#include "text.h"
#include "constants/healthbox.h"
#include "constants/rgb.h"
#include "data/healthbox.h"

#define HB_LEFT_W   64
#define HB_SPRITE_H 32

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
static void PrintTextAt(const struct HealthboxSprites *sprites, u32 fontId, s32 x, s32 top, const u8 *str)
{
    struct Sprite *left = &gSprites[sprites->left];
    struct Sprite *right = &gSprites[sprites->right];
    s16 savedLeft1 = left->data[1], savedRight1 = right->data[1];

    // The text printer spills into the sprite named by data[1].
    left->data[1] = sprites->right;
    right->data[1] = SPRITE_NONE;

    if (x >= HB_LEFT_W)
        AddSpriteTextPrinterParameterized6(sprites->right, fontId, x - HB_LEFT_W, top, 0, 0, sHealthboxTextColor, 0, str);
    else
        AddSpriteTextPrinterParameterized6(sprites->left, fontId, x, top, 0, 0, sHealthboxTextColor, 0, str);

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

    PrintTextAt(sprites, fontId, x, top, str);
}

void HealthboxRender_DrawExpBar(const struct HealthboxSprites *sprites, const struct HealthboxLayout *layout,
                                const struct HealthboxRect *rect, u32 fillPx)
{
    if (fillPx > rect->w)
        fillPx = rect->w;

    HealthboxRender_FillRect(sprites, layout, rect->x, rect->y, fillPx, rect->h, HB_PAL_EXP);
    HealthboxRender_FillRect(sprites, layout, rect->x + fillPx, rect->y, rect->w - fillPx, rect->h, HB_PAL_TROUGH);
}

void HealthboxRender_DrawStatusPill(const struct HealthboxSprites *sprites, const struct HealthboxLayout *layout,
                                    const struct HealthboxRect *rect, u32 palIndex, const u8 *label, u32 background)
{
    u8 backdrop = background == HB_BG_NONE ? 0 : HB_PAL_FILL;
    s32 x, top;

    HealthboxRender_FillRect(sprites, layout, rect->x, rect->y, rect->w, rect->h, backdrop);
    if (label == NULL)
        return;

    HealthboxRender_FillRect(sprites, layout, rect->x, rect->y, rect->w, rect->h, palIndex);
    // Round the corners.
    HealthboxRender_PutPixel(sprites, layout, rect->x, rect->y, backdrop);
    HealthboxRender_PutPixel(sprites, layout, rect->x + rect->w - 1, rect->y, backdrop);
    HealthboxRender_PutPixel(sprites, layout, rect->x, rect->y + rect->h - 1, backdrop);
    HealthboxRender_PutPixel(sprites, layout, rect->x + rect->w - 1, rect->y + rect->h - 1, backdrop);

    x = rect->x + (rect->w - GetStringWidth(HB_PILL_FONT, label, 0)) / 2;
    top = rect->y + (rect->h - HB_PILL_INK_H) / 2 - HB_TEXT_INK_OFFSET;
    if (top < 0)
        top = 0;
    PrintTextAt(sprites, HB_PILL_FONT, x, top, label);
}

void HealthboxRender_DrawHpBar(u8 barSpriteId, u32 fillPx, u32 colourLevel)
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
