#include "global.h"
#include "main.h"
#include "menu.h"
#include "bg.h"
#include "decompress.h"
#include "gpu_regs.h"
#include "malloc.h"
#include "palette.h"
#include "pokedex_area_region_map.h"
#include "world_map.h"

/*
 *  The Pokédex area screen draws the world map on BG3 (text mode, 64x64 tiles, 4bpp) and the
 *  species glow on BG2 with the same size, so both scroll with the same offsets.
 *  VRAM (the Pokédex keeps BG1 tiles at the start of charblock 0 and its tilemaps at 12-15):
 *    BG3 tiles  charblock 2 from 0x8000     BG3 map  screenblocks 28-31
 *    BG2 tiles  charblock 3, tile 128       BG2 map  screenblocks 8-11
 */

#define AREA_MAP_BG 3
#define AREA_GLOW_BG 2

#define AREA_MAP_CHAR_BASE 2
#define AREA_MAP_MAP_BASE 28
#define AREA_MAP_FIRST_BANK 6
#define AREA_GLOW_CHAR_BASE 3
#define AREA_GLOW_MAP_BASE 8

// Pokédex info screen values restored on exit.
#define DEX_BG3_CHAR_BASE 0
#define DEX_BG3_MAP_BASE 15
#define DEX_BG2_CHAR_BASE 2
#define DEX_BG2_MAP_BASE 14

#define SCROLL_MAX_X (WORLD_MAP_DEX_W * 8 - DISPLAY_WIDTH)
#define SCROLL_MAX_Y (WORLD_MAP_DEX_H * 8 - DISPLAY_HEIGHT)
#define SCROLL_EASE_DIV 3 // Camera covers 1/3 of the remaining distance per frame, at least 1 px

static s32 sScrollX;
static s32 sScrollY;
static s32 sTargetX;
static s32 sTargetY;

// BG setup; the exit path restores the Pokédex values, so a reload must redo it.
void SetUpPokedexAreaMapBgs(void)
{
    SetBgAttribute(AREA_MAP_BG, BG_ATTR_CHARBASEINDEX, AREA_MAP_CHAR_BASE);
    SetBgAttribute(AREA_MAP_BG, BG_ATTR_MAPBASEINDEX, AREA_MAP_MAP_BASE);
    SetBgAttribute(AREA_MAP_BG, BG_ATTR_SCREENSIZE, 3);
    SetBgAttribute(AREA_MAP_BG, BG_ATTR_PALETTEMODE, 0);
    SetBgAttribute(AREA_GLOW_BG, BG_ATTR_CHARBASEINDEX, AREA_GLOW_CHAR_BASE);
    SetBgAttribute(AREA_GLOW_BG, BG_ATTR_MAPBASEINDEX, AREA_GLOW_MAP_BASE);
    SetBgAttribute(AREA_GLOW_BG, BG_ATTR_SCREENSIZE, 3);
}

void LoadPokedexAreaMapGfx(void)
{
    SetUpPokedexAreaMapBgs();
    LoadWorldMapForDex(AREA_MAP_CHAR_BASE, AREA_MAP_MAP_BASE, AREA_MAP_FIRST_BANK);
}

bool32 TryShowPokedexAreaMap(void)
{
    if (!FreeTempTileDataBuffersIfPossible())
    {
        ShowBg(AREA_MAP_BG);
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

void ResetPokedexAreaMapBg(void)
{
    SetBgAttribute(AREA_MAP_BG, BG_ATTR_CHARBASEINDEX, DEX_BG3_CHAR_BASE);
    SetBgAttribute(AREA_MAP_BG, BG_ATTR_MAPBASEINDEX, DEX_BG3_MAP_BASE);
    SetBgAttribute(AREA_MAP_BG, BG_ATTR_SCREENSIZE, 0);
    SetBgAttribute(AREA_GLOW_BG, BG_ATTR_CHARBASEINDEX, DEX_BG2_CHAR_BASE);
    SetBgAttribute(AREA_GLOW_BG, BG_ATTR_MAPBASEINDEX, DEX_BG2_MAP_BASE);
    SetBgAttribute(AREA_GLOW_BG, BG_ATTR_SCREENSIZE, 0);
}

static void ClampTarget(void)
{
    if (sTargetX < 0)
        sTargetX = 0;
    if (sTargetX > SCROLL_MAX_X)
        sTargetX = SCROLL_MAX_X;
    if (sTargetY < 0)
        sTargetY = 0;
    if (sTargetY > SCROLL_MAX_Y)
        sTargetY = SCROLL_MAX_Y;
}

static void ApplyScroll(void)
{
    SetGpuReg(REG_OFFSET_BG2HOFS, sScrollX);
    SetGpuReg(REG_OFFSET_BG2VOFS, sScrollY);
    SetGpuReg(REG_OFFSET_BG3HOFS, sScrollX);
    SetGpuReg(REG_OFFSET_BG3VOFS, sScrollY);
}

// Sets the camera target to the pixel position; the camera eases there unless snap is set.
void PokedexAreaMapCenterOn(s32 x, s32 y, bool32 snap)
{
    sTargetX = x - DISPLAY_WIDTH / 2;
    sTargetY = y - DISPLAY_HEIGHT / 2;
    ClampTarget();
    if (snap)
    {
        sScrollX = sTargetX;
        sScrollY = sTargetY;
        ApplyScroll();
    }
}

void PokedexAreaMapPan(s32 dx, s32 dy)
{
    sTargetX += dx;
    sTargetY += dy;
    ClampTarget();
}

static s32 EaseAxis(s32 cur, s32 target)
{
    s32 d = target - cur;
    s32 step = d / SCROLL_EASE_DIV;

    if (step == 0)
        step = (d > 0) - (d < 0);
    return cur + step;
}

void PokedexAreaMapUpdateScroll(void)
{
    sScrollX = EaseAxis(sScrollX, sTargetX);
    sScrollY = EaseAxis(sScrollY, sTargetY);
    ApplyScroll();
}

void PokedexAreaMapGetScroll(s32 *x, s32 *y)
{
    *x = sScrollX;
    *y = sScrollY;
}
