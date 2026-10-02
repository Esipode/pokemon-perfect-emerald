#include "global.h"
#include "bg.h"
#include "decompress.h"
#include "gpu_regs.h"
#include "main.h"
#include "malloc.h"
#include "menu.h"
#include "overworld.h"
#include "palette.h"
#include "region_map.h"
#include "scanline_effect.h"
#include "sprite.h"
#include "strings.h"
#include "task.h"
#include "text.h"
#include "window.h"
#include "util.h"
#include "world_map.h"
#include "constants/rgb.h"

#include "data/region_map/world_map_layout.h"
#include "data/region_map/region_map_layout_world.h"

/*
 *  Combined Hoenn / Kanto / Sevii / Johto map. Text-mode BG0 (64x64 tiles, 4bpp) built by
 *  tools/gs_convert/build_world_map.py; scrolled with BG offsets, no zoom.
 *  Each region owns its palette banks, so a locked region is dimmed by blending only those.
 *  The cursor moves one cell at a time; leaving a panel jumps to the neighbouring panel.
 *  BG1 holds the text windows (palette bank 15).
 */

#define WORLD_MAP_PX_W (56 * 8)
#define WORLD_MAP_PX_H (45 * 8)
#define SCROLL_MAX_X (WORLD_MAP_PX_W - DISPLAY_WIDTH)
#define SCROLL_MAX_Y (WORLD_MAP_PX_H - DISPLAY_HEIGHT)
#define SCROLL_MARGIN (3 * 8)
#define CURSOR_REPEAT_DELAY 4
#define CURSOR_TAG 0
#define NUM_PANELS 6
#define NUM_BANK_GROUPS 4
#define DIM_COEFF 8

struct WorldMap
{
    s16 scrollX;
    s16 scrollY;
    u8 cursorX;
    u8 cursorY;
    u8 panel;
    u8 dimmedGroups;
    u8 moveDelay;
    u8 cursorSpriteId;
    u8 shownPanel;
    mapsec_u16_t shownMapSec;
    u8 cursorGfx[0x100];
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
    PANEL_JOHTO,
    PANEL_KANTO,
    PANEL_HOENN,
};

enum
{
    GROUP_JOHTO,
    GROUP_KANTO,
    GROUP_HOENN,
    GROUP_SEVII,
};

enum
{
    DIR_UP,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT,
};

enum
{
    WIN_MAPSEC_NAME,
    WIN_REGION_NAME,
    WIN_COUNT,
};

static EWRAM_DATA struct WorldMap *sWorldMap = NULL;

static const u16 sWorldMap_Pal[] = INCGFX_U16("graphics/world_map/map.pal", ".gbapal");
static const u32 sWorldMap_Gfx[] = INCGFX_U32("graphics/world_map/tiles.png", ".4bpp.smol");
static const u32 sWorldMap_Tilemap[] = INCGFX_U32("graphics/world_map/map.bin", ".smolTM");

static const u16 sCursor_Pal[] = INCGFX_U16("graphics/region_map/cursor.pal", ".gbapal");
static const u32 sCursor_Gfx[] = INCGFX_U32("graphics/region_map/cursor_small.png", ".4bpp.smol");

static const u8 sText_Johto[] = _("JOHTO");
static const u8 sText_Sevii[] = _("SEVII ISLANDS");
static const u8 sTextColors[] = {1, 2, 3};

static const struct BgTemplate sWorldMapBgTemplates[] =
{
    {
        .bg = 0,
        .charBaseIndex = 0,
        .mapBaseIndex = 28,
        .screenSize = 3,
        .paletteMode = 0,
        .priority = 1
    },
    {
        .bg = 1,
        .charBaseIndex = 2,
        .mapBaseIndex = 26,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0
    },
};

static const struct WindowTemplate sWorldMapWindowTemplates[WIN_COUNT + 1] =
{
    [WIN_MAPSEC_NAME] = {
        .bg = 1,
        .tilemapLeft = 0,
        .tilemapTop = 18,
        .width = 15,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0x01
    },
    [WIN_REGION_NAME] = {
        .bg = 1,
        .tilemapLeft = 19,
        .tilemapTop = 0,
        .width = 11,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0x21
    },
    DUMMY_WIN_TEMPLATE
};

static const struct OamData sCursorOam =
{
    .shape = SPRITE_SHAPE(16x16),
    .size = SPRITE_SIZE(16x16),
    .priority = 0
};

static const union AnimCmd sCursorAnim[] =
{
    ANIMCMD_FRAME(0, 20),
    ANIMCMD_FRAME(4, 20),
    ANIMCMD_JUMP(0)
};

static const union AnimCmd *const sCursorAnimTable[] =
{
    sCursorAnim
};

static const struct SpritePalette sCursorSpritePalette =
{
    .data = sCursor_Pal,
    .tag = CURSOR_TAG
};

static const struct SpriteTemplate sCursorSpriteTemplate =
{
    .tileTag = CURSOR_TAG,
    .paletteTag = CURSOR_TAG,
    .oam = &sCursorOam,
    .anims = sCursorAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy
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

static const u8 *const sPanelNames[NUM_PANELS] =
{
    sText_Johto, gText_Kanto, gText_Hoenn, sText_Sevii, sText_Sevii, sText_Sevii,
};

// Neighbouring panel per direction (up, down, left, right); -1 = none.
static const s8 sPanelNeighbours[NUM_PANELS][4] =
{
    { -1,  2, -1,  1 },
    { -1,  3,  0, -1 },
    {  0,  4, -1,  3 },
    {  1,  5,  2, -1 },
    {  2, -1, -1,  5 },
    {  3, -1,  4, -1 },
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
static void FollowCursor(bool32 center);
static void ApplyGroupDimming(void);
static void ApplyScroll(void);
static void CreateCursor(void);
static void UpdateCursorSprite(void);
static void DestroyCursor(void);
static void UpdateWindows(void);
static bool32 TryMoveCursor(s32 dx, s32 dy);
static void SetCursorToPanel(u32 panel);

mapsec_u16_t GetWorldMapSecIdAt(u16 x, u16 y)
{
    if (x >= WORLD_MAP_CELLS_W || y >= WORLD_MAP_CELLS_H)
        return MAPSEC_NONE;
    return sWorldMapSections[y][x];
}

static s32 PanelAt(s32 x, s32 y)
{
    u32 i;

    for (i = 0; i < NUM_PANELS; i++)
    {
        if (x >= sPanels[i].x && x < sPanels[i].x + sPanels[i].w
         && y >= sPanels[i].y && y < sPanels[i].y + sPanels[i].h)
            return i;
    }
    return -1;
}

// Moves the cursor to the MAPSEC cell of the panel nearest to (targetX, targetY).
static void SnapCursorToPanelMapSec(u32 panel, s32 targetX, s32 targetY)
{
    s32 x, y;
    s32 bestDist = 0x7FFF;
    s32 bestX = targetX, bestY = targetY;

    for (y = sPanels[panel].y; y < sPanels[panel].y + sPanels[panel].h; y++)
    {
        for (x = sPanels[panel].x; x < sPanels[panel].x + sPanels[panel].w; x++)
        {
            s32 dist = abs(x - targetX) + abs(y - targetY);

            if (sWorldMapSections[y][x] != MAPSEC_NONE && dist < bestDist)
            {
                bestDist = dist;
                bestX = x;
                bestY = y;
            }
        }
    }
    sWorldMap->cursorX = bestX;
    sWorldMap->cursorY = bestY;
    sWorldMap->panel = panel;
}

static void SetCursorToPanel(u32 panel)
{
    SnapCursorToPanelMapSec(panel, sPanels[panel].x + sPanels[panel].w / 2, sPanels[panel].y + sPanels[panel].h / 2);
}

static bool32 TryMoveCursor(s32 dx, s32 dy)
{
    s32 x = sWorldMap->cursorX + dx;
    s32 y = sWorldMap->cursorY + dy;
    s32 panel = PanelAt(x, y);

    if (panel >= 0)
    {
        sWorldMap->cursorX = x;
        sWorldMap->cursorY = y;
        sWorldMap->panel = panel;
        FollowCursor(FALSE);
        return TRUE;
    }
    else
    {
        u32 dir = dy < 0 ? DIR_UP : dy > 0 ? DIR_DOWN : dx < 0 ? DIR_LEFT : DIR_RIGHT;
        s32 next = sPanelNeighbours[sWorldMap->panel][dir];

        if (next < 0)
            return FALSE;
        x = x < sPanels[next].x ? sPanels[next].x : min(x, sPanels[next].x + sPanels[next].w - 1);
        y = y < sPanels[next].y ? sPanels[next].y : min(y, sPanels[next].y + sPanels[next].h - 1);
        SnapCursorToPanelMapSec(next, x, y);
        FollowCursor(TRUE);
        return TRUE;
    }
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
        InitWindows(sWorldMapWindowTemplates);
        DeactivateAllTextPrinters();
        gMain.state++;
        break;
    case 2:
        DecompressDataWithHeaderVram(sWorldMap_Gfx, (u16 *)BG_CHAR_ADDR(0));
        DecompressDataWithHeaderVram(sWorldMap_Tilemap, (u16 *)BG_SCREEN_ADDR(28));
        LoadPalette(sWorldMap_Pal, BG_PLTT_ID(0), 15 * PLTT_SIZE_4BPP);
        LoadPalette(gStandardMenuPalette, BG_PLTT_ID(15), PLTT_SIZE_4BPP);
        SetCursorToPanel(PANEL_HOENN);
        FollowCursor(TRUE);
        ApplyScroll();
        DecompressDataWithHeaderWram(sCursor_Gfx, sWorldMap->cursorGfx);
        CreateCursor();
        sWorldMap->shownPanel = 0xFF;
        sWorldMap->shownMapSec = 0xFFFF;
        PutWindowTilemap(WIN_MAPSEC_NAME);
        PutWindowTilemap(WIN_REGION_NAME);
        UpdateWindows();
        gMain.state++;
        break;
    case 3:
        BlendPalettes(PALETTES_ALL, 16, RGB_BLACK);
        SetVBlankCallback(VBlankCB_WorldMap);
        gMain.state++;
        break;
    case 4:
        SetGpuRegBits(REG_OFFSET_DISPCNT, DISPCNT_OBJ_1D_MAP | DISPCNT_OBJ_ON);
        ShowBg(0);
        ShowBg(1);
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
    DoScheduledBgTilemapCopiesToVram();
    TransferPlttBuffer();
}

static void ApplyScroll(void)
{
    // BG1 holds the fixed text windows; clear any offset left by the previous screen.
    SetGpuReg(REG_OFFSET_BG1HOFS, 0);
    SetGpuReg(REG_OFFSET_BG1VOFS, 0);
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

static void FollowCursor(bool32 center)
{
    s32 x = sWorldMap->cursorX * 8;
    s32 y = sWorldMap->cursorY * 8;

    if (center)
    {
        sWorldMap->scrollX = x + 4 - DISPLAY_WIDTH / 2;
        sWorldMap->scrollY = y + 4 - DISPLAY_HEIGHT / 2;
    }
    else
    {
        if (x - sWorldMap->scrollX < SCROLL_MARGIN)
            sWorldMap->scrollX = x - SCROLL_MARGIN;
        if (x + 8 - sWorldMap->scrollX > DISPLAY_WIDTH - SCROLL_MARGIN)
            sWorldMap->scrollX = x + 8 - DISPLAY_WIDTH + SCROLL_MARGIN;
        if (y - sWorldMap->scrollY < SCROLL_MARGIN)
            sWorldMap->scrollY = y - SCROLL_MARGIN;
        if (y + 8 - sWorldMap->scrollY > DISPLAY_HEIGHT - SCROLL_MARGIN)
            sWorldMap->scrollY = y + 8 - DISPLAY_HEIGHT + SCROLL_MARGIN;
    }
    ClampScroll();
}

static void CreateCursor(void)
{
    struct SpriteSheet sheet = { .data = sWorldMap->cursorGfx, .size = sizeof(sWorldMap->cursorGfx), .tag = CURSOR_TAG };

    LoadSpriteSheet(&sheet);
    LoadSpritePalette(&sCursorSpritePalette);
    sWorldMap->cursorSpriteId = CreateSprite(&sCursorSpriteTemplate, 0, 0, 0);
    UpdateCursorSprite();
}

static void UpdateCursorSprite(void)
{
    struct Sprite *sprite = &gSprites[sWorldMap->cursorSpriteId];

    sprite->x = sWorldMap->cursorX * 8 + 4 - sWorldMap->scrollX;
    sprite->y = sWorldMap->cursorY * 8 + 4 - sWorldMap->scrollY;
}

static void DestroyCursor(void)
{
    DestroySprite(&gSprites[sWorldMap->cursorSpriteId]);
    FreeSpriteTilesByTag(CURSOR_TAG);
    FreeSpritePaletteByTag(CURSOR_TAG);
}

// Redraws only the windows whose content changed.
static void UpdateWindows(void)
{
    mapsec_u16_t mapSec = GetWorldMapSecIdAt(sWorldMap->cursorX, sWorldMap->cursorY);

    if (mapSec != sWorldMap->shownMapSec)
    {
        sWorldMap->shownMapSec = mapSec;
        FillWindowPixelBuffer(WIN_MAPSEC_NAME, PIXEL_FILL(1));
        if (mapSec < MAPSEC_NONE)
            AddTextPrinterParameterized3(WIN_MAPSEC_NAME, FONT_NORMAL, 2, 1, sTextColors, 0, gRegionMapEntries[mapSec].name);
        CopyWindowToVram(WIN_MAPSEC_NAME, COPYWIN_FULL);
    }
    if (sWorldMap->panel != sWorldMap->shownPanel)
    {
        sWorldMap->shownPanel = sWorldMap->panel;
        FillWindowPixelBuffer(WIN_REGION_NAME, PIXEL_FILL(1));
        AddTextPrinterParameterized3(WIN_REGION_NAME, FONT_NORMAL, 2, 1, sTextColors, 0, sPanelNames[sWorldMap->panel]);
        CopyWindowToVram(WIN_REGION_NAME, COPYWIN_FULL);
    }
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
    if (!gPaletteFade.active)
    {
        s32 dx = 0, dy = 0;

        if (JOY_NEW(B_BUTTON) || JOY_NEW(START_BUTTON))
        {
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
            SetMainCallback2(CB2_ExitWorldMap);
            return;
        }
        if (JOY_NEW(R_BUTTON))
        {
            SetCursorToPanel((sWorldMap->panel + 1) % NUM_PANELS);
            FollowCursor(TRUE);
        }
        else if (JOY_NEW(L_BUTTON))
        {
            SetCursorToPanel((sWorldMap->panel + NUM_PANELS - 1) % NUM_PANELS);
            FollowCursor(TRUE);
        }
        if (JOY_NEW(SELECT_BUTTON))
        {
            sWorldMap->dimmedGroups ^= 1 << sPanelGroup[sWorldMap->panel];
            ApplyGroupDimming();
        }

        if (sWorldMap->moveDelay != 0)
            sWorldMap->moveDelay--;
        if (JOY_HELD(DPAD_LEFT))
            dx = -1;
        else if (JOY_HELD(DPAD_RIGHT))
            dx = 1;
        else if (JOY_HELD(DPAD_UP))
            dy = -1;
        else if (JOY_HELD(DPAD_DOWN))
            dy = 1;
        if ((dx != 0 || dy != 0) && sWorldMap->moveDelay == 0 && TryMoveCursor(dx, dy))
            sWorldMap->moveDelay = CURSOR_REPEAT_DELAY;
        else if (dx == 0 && dy == 0)
            sWorldMap->moveDelay = 0;

        ApplyScroll();
        UpdateCursorSprite();
        UpdateWindows();
    }
    RunTextPrinters();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

static void CB2_ExitWorldMap(void)
{
    if (!UpdatePaletteFade())
    {
        DestroyCursor();
        FreeAllWindowBuffers();
        TRY_FREE_AND_SET_NULL(sWorldMap);
        SetMainCallback2(CB2_ReturnToField);
    }
}
