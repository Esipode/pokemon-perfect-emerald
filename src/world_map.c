#include "global.h"
#include "bg.h"
#include "decompress.h"
#include "gpu_regs.h"
#include "main.h"
#include "malloc.h"
#include "overworld.h"
#include "palette.h"
#include "scanline_effect.h"
#include "sprite.h"
#include "task.h"
#include "util.h"
#include "world_map.h"
#include "constants/rgb.h"

#include "data/region_map/world_map_layout.h"
#include "data/region_map/region_map_layout_world.h"

/*
 *  Combined Hoenn / Kanto / Sevii / Johto map. Text-mode BG0 (64x64 tiles, 4bpp) built by
 *  tools/gs_convert/build_world_map.py; scrolled with BG offsets, no zoom.
 *  Each region owns its palette banks, so a locked region is dimmed by blending only those.
 */

#define WORLD_MAP_PX_W (56 * 8)
#define WORLD_MAP_PX_H (45 * 8)
#define SCROLL_MAX_X (WORLD_MAP_PX_W - DISPLAY_WIDTH)
#define SCROLL_MAX_Y (WORLD_MAP_PX_H - DISPLAY_HEIGHT)
#define SCROLL_SPEED 2
#define SCROLL_SPEED_FAST 6
#define NUM_PANELS 6
#define NUM_BANK_GROUPS 4
#define DIM_COEFF 8

struct WorldMap
{
    s16 scrollX;
    s16 scrollY;
    u8 panel;
    u8 dimmedGroups;
};

struct Panel
{
    u8 x;
    u8 y;
    u8 w;
    u8 h;
};

struct BankGroup
{
    u8 first;
    u8 count;
};

enum
{
    GROUP_JOHTO,
    GROUP_KANTO,
    GROUP_HOENN,
    GROUP_SEVII,
};

static EWRAM_DATA struct WorldMap *sWorldMap = NULL;

static const u16 sWorldMap_Pal[] = INCGFX_U16("graphics/world_map/map.pal", ".gbapal");
static const u32 sWorldMap_Gfx[] = INCGFX_U32("graphics/world_map/tiles.png", ".4bpp.smol");
static const u32 sWorldMap_Tilemap[] = INCGFX_U32("graphics/world_map/map.bin", ".smolTM");

static const struct BgTemplate sWorldMapBgTemplates[] =
{
    {
        .bg = 0,
        .charBaseIndex = 0,
        .mapBaseIndex = 28,
        .screenSize = 3,
        .paletteMode = 0,
        .priority = 0
    },
};

static const struct Panel sPanels[NUM_PANELS] =
{
    { WORLD_MAP_JOHTO_X,    WORLD_MAP_JOHTO_Y,    WORLD_MAP_JOHTO_W,    WORLD_MAP_JOHTO_H },
    { WORLD_MAP_KANTO_X,    WORLD_MAP_KANTO_Y,    WORLD_MAP_KANTO_W,    WORLD_MAP_KANTO_H },
    { WORLD_MAP_HOENN_X,    WORLD_MAP_HOENN_Y,    WORLD_MAP_HOENN_W,    WORLD_MAP_HOENN_H },
    { WORLD_MAP_SEVII123_X, WORLD_MAP_SEVII123_Y, WORLD_MAP_SEVII123_W, WORLD_MAP_SEVII123_H },
    { WORLD_MAP_SEVII45_X,  WORLD_MAP_SEVII45_Y,  WORLD_MAP_SEVII45_W,  WORLD_MAP_SEVII45_H },
    { WORLD_MAP_SEVII67_X,  WORLD_MAP_SEVII67_Y,  WORLD_MAP_SEVII67_W,  WORLD_MAP_SEVII67_H },
};

static const u8 sPanelGroup[NUM_PANELS] =
{
    GROUP_JOHTO, GROUP_KANTO, GROUP_HOENN, GROUP_SEVII, GROUP_SEVII, GROUP_SEVII,
};

static const struct BankGroup sBankGroups[NUM_BANK_GROUPS] =
{
    [GROUP_JOHTO] = { WORLD_MAP_BANK_JOHTO_FIRST, WORLD_MAP_BANK_JOHTO_COUNT },
    [GROUP_KANTO] = { WORLD_MAP_BANK_KANTO_FIRST, WORLD_MAP_BANK_KANTO_COUNT },
    [GROUP_HOENN] = { WORLD_MAP_BANK_HOENN_FIRST, WORLD_MAP_BANK_HOENN_COUNT },
    [GROUP_SEVII] = { WORLD_MAP_BANK_SEVII_FIRST, WORLD_MAP_BANK_SEVII_COUNT },
};

static void CB2_WorldMap(void);
static void CB2_ExitWorldMap(void);
static void VBlankCB_WorldMap(void);
static void ClampScroll(void);
static void CenterOnPanel(u32 panel);
static void ApplyGroupDimming(void);
static void ApplyScroll(void);

mapsec_u16_t GetWorldMapSecIdAt(u16 x, u16 y)
{
    if (x >= WORLD_MAP_CELLS_W || y >= WORLD_MAP_CELLS_H)
        return MAPSEC_NONE;
    return sWorldMapSections[y][x];
}

void CB2_OpenWorldMap(void)
{
    switch (gMain.state)
    {
    case 0:
        SetVBlankCallback(NULL);
        SetGpuReg(REG_OFFSET_DISPCNT, 0);
        sWorldMap = AllocZeroed(sizeof(*sWorldMap));
        ResetPaletteFade();
        ResetSpriteData();
        ResetTasks();
        ScanlineEffect_Stop();
        gMain.state++;
        break;
    case 1:
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(0, sWorldMapBgTemplates, ARRAY_COUNT(sWorldMapBgTemplates));
        gMain.state++;
        break;
    case 2:
        DecompressDataWithHeaderVram(sWorldMap_Gfx, (u16 *)BG_CHAR_ADDR(0));
        DecompressDataWithHeaderVram(sWorldMap_Tilemap, (u16 *)BG_SCREEN_ADDR(28));
        LoadPalette(sWorldMap_Pal, BG_PLTT_ID(0), 15 * PLTT_SIZE_4BPP);
        sWorldMap->panel = GROUP_HOENN;
        CenterOnPanel(sWorldMap->panel);
        ApplyScroll();
        gMain.state++;
        break;
    case 3:
        BlendPalettes(PALETTES_ALL, 16, RGB_BLACK);
        SetVBlankCallback(VBlankCB_WorldMap);
        gMain.state++;
        break;
    case 4:
        ShowBg(0);
        BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
        SetMainCallback2(CB2_WorldMap);
        gMain.state = 0;
        break;
    }
}

static void VBlankCB_WorldMap(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

static void ApplyScroll(void)
{
    SetGpuReg(REG_OFFSET_BG0HOFS, sWorldMap->scrollX);
    SetGpuReg(REG_OFFSET_BG0VOFS, sWorldMap->scrollY);
}

static void ClampScroll(void)
{
    if (sWorldMap->scrollX < 0)
        sWorldMap->scrollX = 0;
    if (sWorldMap->scrollX > SCROLL_MAX_X)
        sWorldMap->scrollX = SCROLL_MAX_X;
    if (sWorldMap->scrollY < 0)
        sWorldMap->scrollY = 0;
    if (sWorldMap->scrollY > SCROLL_MAX_Y)
        sWorldMap->scrollY = SCROLL_MAX_Y;
}

static void CenterOnPanel(u32 panel)
{
    sWorldMap->scrollX = (sPanels[panel].x * 8 + sPanels[panel].w * 4) - DISPLAY_WIDTH / 2;
    sWorldMap->scrollY = (sPanels[panel].y * 8 + sPanels[panel].h * 4) - DISPLAY_HEIGHT / 2;
    ClampScroll();
}

static void ApplyGroupDimming(void)
{
    u32 group;

    for (group = 0; group < NUM_BANK_GROUPS; group++)
    {
        u32 offset = BG_PLTT_ID(sBankGroups[group].first);
        u32 count = sBankGroups[group].count * 16;

        if (sWorldMap->dimmedGroups & (1 << group))
            BlendPalette(offset, count, DIM_COEFF, RGB_BLACK);
        else
            CpuCopy16(&gPlttBufferUnfaded[offset], &gPlttBufferFaded[offset], count * sizeof(u16));
    }
}

static void CB2_WorldMap(void)
{
    u32 speed = JOY_HELD(A_BUTTON) ? SCROLL_SPEED_FAST : SCROLL_SPEED;

    if (!gPaletteFade.active)
    {
        if (JOY_NEW(B_BUTTON) || JOY_NEW(START_BUTTON))
        {
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
            SetMainCallback2(CB2_ExitWorldMap);
            return;
        }
        if (JOY_NEW(R_BUTTON))
        {
            sWorldMap->panel = (sWorldMap->panel + 1) % NUM_PANELS;
            CenterOnPanel(sWorldMap->panel);
        }
        else if (JOY_NEW(L_BUTTON))
        {
            sWorldMap->panel = (sWorldMap->panel + NUM_PANELS - 1) % NUM_PANELS;
            CenterOnPanel(sWorldMap->panel);
        }
        if (JOY_NEW(SELECT_BUTTON))
        {
            sWorldMap->dimmedGroups ^= 1 << sPanelGroup[sWorldMap->panel];
            ApplyGroupDimming();
        }
        if (JOY_HELD(DPAD_LEFT))
            sWorldMap->scrollX -= speed;
        if (JOY_HELD(DPAD_RIGHT))
            sWorldMap->scrollX += speed;
        if (JOY_HELD(DPAD_UP))
            sWorldMap->scrollY -= speed;
        if (JOY_HELD(DPAD_DOWN))
            sWorldMap->scrollY += speed;
        ClampScroll();
        ApplyScroll();
    }
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

static void CB2_ExitWorldMap(void)
{
    if (!UpdatePaletteFade())
    {
        TRY_FREE_AND_SET_NULL(sWorldMap);
        SetMainCallback2(CB2_ReturnToField);
    }
}
