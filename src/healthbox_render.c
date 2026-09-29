#include "global.h"
#include "healthbox.h"
#include "sprite.h"
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
