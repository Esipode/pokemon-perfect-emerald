#include "global.h"
#include "bg.h"
#include "decompress.h"
#include "event_data.h"
#include "field_effect.h"
#include "gpu_regs.h"
#include "main.h"
#include "malloc.h"
#include "menu.h"
#include "overworld.h"
#include "party_menu.h"
#include "palette.h"
#include "region_map.h"
#include "regions.h"
#include "scanline_effect.h"
#include "sound.h"
#include "sprite.h"
#include "strings.h"
#include "task.h"
#include "text.h"
#include "window.h"
#include "util.h"
#include "world_map.h"
#include "constants/flags.h"
#include "constants/rgb.h"
#include "constants/songs.h"

#include "data/region_map/world_map_layout.h"
#include "data/region_map/region_map_layout_world.h"

/*
 *  Combined Hoenn / Kanto / Sevii / Johto map. Text-mode BG0 (64x64 tiles, 4bpp) built by
 *  tools/gs_convert/build_world_map.py; scrolled with BG offsets, no zoom.
 *  Each region owns its palette banks, so a locked region is dimmed by blending only those.
 *  The cursor moves one cell at a time; leaving a panel jumps to the neighbouring panel.
 *  Locked panels are not enterable. A bank group is dimmed when none of its panels is unlocked;
 *  Sevii panels share banks, so 4-5 and 6-7 are only blocked (not dimmed) while 1-2-3 is open.
 *  BG1 holds the text windows (palette bank 15).
 *  Fly mode (CB2_OpenFlyMap): every panel can be scrolled to, but only fly points in the player's
 *  current bank group are selectable; other groups are dimmed lightly so they stay readable.
 */

#define WORLD_MAP_PX_W (56 * 8)
#define WORLD_MAP_PX_H (45 * 8)
#define SCROLL_MAX_X (WORLD_MAP_PX_W - DISPLAY_WIDTH)
#define SCROLL_MAX_Y (WORLD_MAP_PX_H - DISPLAY_HEIGHT)
#define SCROLL_MARGIN (3 * 8)
#define CURSOR_REPEAT_DELAY 4
#define CURSOR_TAG 0
#define PLAYER_ICON_TAG 1
#define NO_PANEL 0xFF
#define NUM_PANELS 6
#define NUM_BANK_GROUPS 4
#define DIM_COEFF 8
#define FLY_DIM_COEFF 5
#define FLY_ICON_TAG 2
#define FLY_OUTLINE_ANIM 6
#define FLY_ICON_MARGIN 16

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
    u8 playerPanel; // NO_PANEL when the player has no icon
    u8 playerIconSpriteId;
    u8 playerCellX;
    u8 playerCellY;
    u8 shownPanel;
    mapsec_u16_t shownMapSec;
    u8 shownPos;
    bool8 flyMode;
    u8 flyGroup;
    bool8 choseFlyLocation;
    u8 cursorGfx[0x100];
    u8 flyIconGfx[0x1c0];
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
    PANEL_SEVII123,
    PANEL_SEVII45,
    PANEL_SEVII67,
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
    WIN_FLY_PROMPT,
    WIN_COUNT,
};

static EWRAM_DATA struct WorldMap *sWorldMap = NULL;
static bool8 sOpenInFlyMode;

static const u16 sWorldMap_Pal[] = INCGFX_U16("graphics/world_map/map.pal", ".gbapal");
static const u32 sWorldMap_Gfx[] = INCGFX_U32("graphics/world_map/tiles.png", ".4bpp.smol");
static const u32 sWorldMap_Tilemap[] = INCGFX_U32("graphics/world_map/map.bin", ".smolTM");

static const u16 sFlyIcons_Pal[] = INCGFX_U16("graphics/region_map/fly_target_icons.png", ".gbapal");
static const u32 sFlyIcons_Gfx[] = INCGFX_U32("graphics/region_map/fly_target_icons.png", ".4bpp.smol");

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
    [WIN_FLY_PROMPT] = {
        .bg = 1,
        .tilemapLeft = 0,
        .tilemapTop = 0,
        .width = 14,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0x37
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

struct FlyLocation
{
    u16 mapsec;
    u16 flag;
};

// Route 4 / Route 10 Pokemon Centers are separate MAPSECs with their own flags.
static const struct FlyLocation sFlyLocations[] =
{
    { MAPSEC_LITTLEROOT_TOWN, FLAG_VISITED_LITTLEROOT_TOWN },
    { MAPSEC_OLDALE_TOWN, FLAG_VISITED_OLDALE_TOWN },
    { MAPSEC_DEWFORD_TOWN, FLAG_VISITED_DEWFORD_TOWN },
    { MAPSEC_LAVARIDGE_TOWN, FLAG_VISITED_LAVARIDGE_TOWN },
    { MAPSEC_FALLARBOR_TOWN, FLAG_VISITED_FALLARBOR_TOWN },
    { MAPSEC_VERDANTURF_TOWN, FLAG_VISITED_VERDANTURF_TOWN },
    { MAPSEC_PACIFIDLOG_TOWN, FLAG_VISITED_PACIFIDLOG_TOWN },
    { MAPSEC_PETALBURG_CITY, FLAG_VISITED_PETALBURG_CITY },
    { MAPSEC_SLATEPORT_CITY, FLAG_VISITED_SLATEPORT_CITY },
    { MAPSEC_MAUVILLE_CITY, FLAG_VISITED_MAUVILLE_CITY },
    { MAPSEC_RUSTBORO_CITY, FLAG_VISITED_RUSTBORO_CITY },
    { MAPSEC_FORTREE_CITY, FLAG_VISITED_FORTREE_CITY },
    { MAPSEC_LILYCOVE_CITY, FLAG_VISITED_LILYCOVE_CITY },
    { MAPSEC_MOSSDEEP_CITY, FLAG_VISITED_MOSSDEEP_CITY },
    { MAPSEC_SOOTOPOLIS_CITY, FLAG_VISITED_SOOTOPOLIS_CITY },
    { MAPSEC_EVER_GRANDE_CITY, FLAG_VISITED_EVER_GRANDE_CITY },
    { MAPSEC_BATTLE_FRONTIER, FLAG_LANDMARK_BATTLE_FRONTIER },
    { MAPSEC_PALLET_TOWN, FLAG_WORLD_MAP_PALLET_TOWN },
    { MAPSEC_VIRIDIAN_CITY, FLAG_WORLD_MAP_VIRIDIAN_CITY },
    { MAPSEC_PEWTER_CITY, FLAG_WORLD_MAP_PEWTER_CITY },
    { MAPSEC_CERULEAN_CITY, FLAG_WORLD_MAP_CERULEAN_CITY },
    { MAPSEC_LAVENDER_TOWN, FLAG_WORLD_MAP_LAVENDER_TOWN },
    { MAPSEC_VERMILION_CITY, FLAG_WORLD_MAP_VERMILION_CITY },
    { MAPSEC_CELADON_CITY, FLAG_WORLD_MAP_CELADON_CITY },
    { MAPSEC_FUCHSIA_CITY, FLAG_WORLD_MAP_FUCHSIA_CITY },
    { MAPSEC_CINNABAR_ISLAND, FLAG_WORLD_MAP_CINNABAR_ISLAND },
    { MAPSEC_INDIGO_PLATEAU, FLAG_WORLD_MAP_INDIGO_PLATEAU_EXTERIOR },
    { MAPSEC_SAFFRON_CITY, FLAG_WORLD_MAP_SAFFRON_CITY },
    { MAPSEC_ONE_ISLAND, FLAG_WORLD_MAP_ONE_ISLAND },
    { MAPSEC_TWO_ISLAND, FLAG_WORLD_MAP_TWO_ISLAND },
    { MAPSEC_THREE_ISLAND, FLAG_WORLD_MAP_THREE_ISLAND },
    { MAPSEC_FOUR_ISLAND, FLAG_WORLD_MAP_FOUR_ISLAND },
    { MAPSEC_FIVE_ISLAND, FLAG_WORLD_MAP_FIVE_ISLAND },
    { MAPSEC_SEVEN_ISLAND, FLAG_WORLD_MAP_SEVEN_ISLAND },
    { MAPSEC_SIX_ISLAND, FLAG_WORLD_MAP_SIX_ISLAND },
    { MAPSEC_ROUTE_4_POKECENTER, FLAG_WORLD_MAP_ROUTE4_POKEMON_CENTER_1F },
    { MAPSEC_ROUTE_10_POKECENTER, FLAG_WORLD_MAP_ROUTE10_POKEMON_CENTER_1F },
    { MAPSEC_NEW_BARK_TOWN, FLAG_VISITED_NEWBARK_TOWN },
    { MAPSEC_CHERRYGROVE_CITY, FLAG_VISITED_CHERRYGROVE_CITY },
    { MAPSEC_VIOLET_CITY, FLAG_VISITED_VIOLET_CITY },
    { MAPSEC_AZALEA_TOWN, FLAG_VISITED_AZALEA_TOWN },
    { MAPSEC_GOLDENROD_CITY, FLAG_VISITED_GOLDENROD_CITY },
    { MAPSEC_ECRUTEAK_CITY, FLAG_VISITED_ECRUTEAK_CITY },
    { MAPSEC_OLIVINE_CITY, FLAG_VISITED_OLIVINE_CITY },
    { MAPSEC_CIANWOOD_CITY, FLAG_VISITED_CIANWOOD_CITY },
    { MAPSEC_MAHOGANY_TOWN, FLAG_VISITED_MAHOGANY_TOWN },
    { MAPSEC_BLACKTHORN_CITY, FLAG_VISITED_BLACKTHORN_CITY },
    { MAPSEC_MT_SILVER, FLAG_VISITED_MT_SILVER },
    { MAPSEC_JOHTO_LEAGUE, FLAG_VISITED_INDIGO_PLATEAU },
    { MAPSEC_ROUTE_48, FLAG_VISITED_SAFARI_ZONE_GATE },
};

static const struct OamData sFlyIconOam =
{
    .shape = SPRITE_SHAPE(8x8),
    .size = SPRITE_SIZE(8x8),
    .priority = 1
};

// Frames: 0-4 selectable (8x8, 16x8, -, 8x16), 5-9 not yet visited, 10 red outline.
static const union AnimCmd sFlyIconAnim_8x8Can[] = { ANIMCMD_FRAME(0, 5), ANIMCMD_END };
static const union AnimCmd sFlyIconAnim_16x8Can[] = { ANIMCMD_FRAME(1, 5), ANIMCMD_END };
static const union AnimCmd sFlyIconAnim_8x16Can[] = { ANIMCMD_FRAME(3, 5), ANIMCMD_END };
static const union AnimCmd sFlyIconAnim_8x8Cant[] = { ANIMCMD_FRAME(5, 5), ANIMCMD_END };
static const union AnimCmd sFlyIconAnim_16x8Cant[] = { ANIMCMD_FRAME(6, 5), ANIMCMD_END };
static const union AnimCmd sFlyIconAnim_8x16Cant[] = { ANIMCMD_FRAME(8, 5), ANIMCMD_END };
static const union AnimCmd sFlyIconAnim_RedOutline[] = { ANIMCMD_FRAME(10, 5), ANIMCMD_END };

// Indexed by SPRITE_SHAPE, +3 for not selectable.
static const union AnimCmd *const sFlyIconAnims[] =
{
    [SPRITE_SHAPE(8x8)]       = sFlyIconAnim_8x8Can,
    [SPRITE_SHAPE(16x8)]      = sFlyIconAnim_16x8Can,
    [SPRITE_SHAPE(8x16)]      = sFlyIconAnim_8x16Can,
    [SPRITE_SHAPE(8x8) + 3]   = sFlyIconAnim_8x8Cant,
    [SPRITE_SHAPE(16x8) + 3]  = sFlyIconAnim_16x8Cant,
    [SPRITE_SHAPE(8x16) + 3]  = sFlyIconAnim_8x16Cant,
    [FLY_OUTLINE_ANIM]        = sFlyIconAnim_RedOutline
};

static const struct SpritePalette sFlyIconSpritePalette =
{
    .data = sFlyIcons_Pal,
    .tag = FLY_ICON_TAG
};

static const struct SpriteTemplate sFlyIconSpriteTemplate =
{
    .tileTag = FLY_ICON_TAG,
    .paletteTag = FLY_ICON_TAG,
    .oam = &sFlyIconOam,
    .anims = sFlyIconAnims,
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

// Panel crop offset inside the 28x15 source grid.
static const struct { u8 x; u8 y; } sPanelSrc[NUM_PANELS] =
{
    { WORLD_MAP_JOHTO_SRC_X,    WORLD_MAP_JOHTO_SRC_Y },
    { WORLD_MAP_KANTO_SRC_X,    WORLD_MAP_KANTO_SRC_Y },
    { WORLD_MAP_HOENN_SRC_X,    WORLD_MAP_HOENN_SRC_Y },
    { WORLD_MAP_SEVII123_SRC_X, WORLD_MAP_SEVII123_SRC_Y },
    { WORLD_MAP_SEVII45_SRC_X,  WORLD_MAP_SEVII45_SRC_Y },
    { WORLD_MAP_SEVII67_SRC_X,  WORLD_MAP_SEVII67_SRC_Y },
};

// First panel of each bank group; the Sevii group starts at 1-2-3.
static const u8 sFlyGroupPanel[NUM_BANK_GROUPS] =
{
    [GROUP_JOHTO] = PANEL_JOHTO,
    [GROUP_KANTO] = PANEL_KANTO,
    [GROUP_HOENN] = PANEL_HOENN,
    [GROUP_SEVII] = PANEL_SEVII123,
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
static void DimLockedGroups(void);
static bool32 IsPanelUnlocked(u32 panel);
static void ApplyScroll(void);
static void CreateCursor(void);
static void UpdateCursorSprite(void);
static void DestroyCursor(void);
static void CreatePlayerIcon(void);
static void LocatePlayer(void);
static void UpdateWindows(void);
static void CreateFlyIcons(void);
static void DestroyFlyIcons(void);
static u32 GetMapSecGroup(mapsec_u16_t mapSec);
static bool32 CanFlyFromCell(void);
static u32 GetCursorPosWithinMapSec(void);
static bool32 TryMoveCursor(s32 dx, s32 dy);
static void SetCursorToPanel(u32 panel);
static u32 NextUnlockedPanel(u32 panel, s32 step);

static bool32 IsPanelUnlocked(u32 panel)
{
    u32 flag;

    if (sWorldMap->flyMode || panel == sWorldMap->playerPanel)
        return TRUE;
    switch (panel)
    {
    case PANEL_JOHTO:
        return FlagGet(FLAG_VISITED_JOHTO);
    case PANEL_KANTO:
        if (FlagGet(FLAG_KANTO_HOENN_LINKED))
            return TRUE;
        // Any Kanto town visited (Pallet Town to Saffron City flags are contiguous).
        for (flag = FLAG_WORLD_MAP_PALLET_TOWN; flag <= FLAG_WORLD_MAP_SAFFRON_CITY; flag++)
        {
            if (FlagGet(flag))
                return TRUE;
        }
        return FALSE;
    case PANEL_SEVII123:
        return FlagGet(FLAG_SYS_SEVII_MAP_123);
    case PANEL_SEVII45:
    case PANEL_SEVII67:
        return FlagGet(FLAG_SYS_SEVII_MAP_4567);
    default:
        return TRUE;
    }
}

static u32 NextUnlockedPanel(u32 panel, s32 step)
{
    u32 i;

    for (i = 0; i < NUM_PANELS; i++)
    {
        panel = (panel + NUM_PANELS + step) % NUM_PANELS;
        if (IsPanelUnlocked(panel))
            return panel;
    }
    return panel;
}

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
        if (panel != sWorldMap->panel && !IsPanelUnlocked(panel))
            return FALSE;
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

        if (next < 0 || !IsPanelUnlocked(next))
            return FALSE;
        x = x < sPanels[next].x ? sPanels[next].x : min(x, sPanels[next].x + sPanels[next].w - 1);
        y = y < sPanels[next].y ? sPanels[next].y : min(y, sPanels[next].y + sPanels[next].h - 1);
        SnapCursorToPanelMapSec(next, x, y);
        FollowCursor(TRUE);
        return TRUE;
    }
}

static void OpenWorldMap(void)
{
    switch (gMain.state)
    {
    case 0:
        SetVBlankCallback(NULL);
        SetGpuReg(REG_OFFSET_DISPCNT, 0);
        sWorldMap = AllocZeroed(sizeof(*sWorldMap));
        sWorldMap->flyMode = sOpenInFlyMode;
        sWorldMap->flyGroup = GetMapSecGroup(gMapHeader.regionMapSectionId);
        ResetPaletteFade();
        ResetSpriteData();
        FreeSpriteTileRanges();
        FreeAllSpritePalettes();
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
        LocatePlayer();
        if (sWorldMap->playerPanel != NO_PANEL)
        {
            sWorldMap->cursorX = sWorldMap->playerCellX;
            sWorldMap->cursorY = sWorldMap->playerCellY;
            sWorldMap->panel = sWorldMap->playerPanel;
        }
        else if (sWorldMap->flyMode)
        {
            SetCursorToPanel(sFlyGroupPanel[sWorldMap->flyGroup]);
        }
        else
        {
            SetCursorToPanel(PANEL_HOENN);
        }
        FollowCursor(TRUE);
        ApplyScroll();
        DimLockedGroups();
        DecompressDataWithHeaderWram(sCursor_Gfx, sWorldMap->cursorGfx);
        CreatePlayerIcon();
        if (sWorldMap->flyMode)
            CreateFlyIcons();
        CreateCursor();
        sWorldMap->shownPanel = 0xFF;
        sWorldMap->shownMapSec = 0xFFFF;
        PutWindowTilemap(WIN_MAPSEC_NAME);
        PutWindowTilemap(WIN_REGION_NAME);
        if (sWorldMap->flyMode)
        {
            PutWindowTilemap(WIN_FLY_PROMPT);
            FillWindowPixelBuffer(WIN_FLY_PROMPT, PIXEL_FILL(1));
            AddTextPrinterParameterized3(WIN_FLY_PROMPT, FONT_NORMAL, 2, 1, sTextColors, 0, gText_FlyToWhere);
            CopyWindowToVram(WIN_FLY_PROMPT, COPYWIN_FULL);
        }
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

void CB2_OpenWorldMap(void)
{
    sOpenInFlyMode = FALSE;
    OpenWorldMap();
}

void CB2_OpenFlyMap(void)
{
    sOpenInFlyMode = TRUE;
    OpenWorldMap();
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

// Finds the player's world cell from their panel-local region map position.
static void LocatePlayer(void)
{
    struct PlayerRegionMapPos pos;
    u32 panel;
    s32 x, y;

    sWorldMap->playerPanel = NO_PANEL;
    if (IsEventIslandMapSecId(gMapHeader.regionMapSectionId))
        return;

    GetPlayerPositionOnRegionMap(&pos);
    switch (GetRegionForSectionId(pos.mapSecId))
    {
    case REGION_JOHTO:
        panel = PANEL_JOHTO;
        break;
    case REGION_KANTO:
        switch (GetKantoSubregion(pos.mapSecId))
        {
        case KANTO_SUBREGION_SEVII123:
            panel = PANEL_SEVII123;
            break;
        case KANTO_SUBREGION_SEVII45:
            panel = PANEL_SEVII45;
            break;
        case KANTO_SUBREGION_SEVII67:
            panel = PANEL_SEVII67;
            break;
        default:
            panel = PANEL_KANTO;
            break;
        }
        break;
    default:
        panel = PANEL_HOENN;
        break;
    }

    x = pos.cursorPosX - MAPCURSOR_X_MIN - sPanelSrc[panel].x + sPanels[panel].x;
    y = pos.cursorPosY - MAPCURSOR_Y_MIN - sPanelSrc[panel].y + sPanels[panel].y;
    if (PanelAt(x, y) != (s32)panel)
        return;
    sWorldMap->playerPanel = panel;
    sWorldMap->playerCellX = x;
    sWorldMap->playerCellY = y;
}

static void CreatePlayerIcon(void)
{
    struct Sprite *sprite;

    sWorldMap->playerIconSpriteId = SPRITE_NONE;
    if (sWorldMap->playerPanel == NO_PANEL)
        return;
    sprite = CreatePlayerIconSprite(PLAYER_ICON_TAG, PLAYER_ICON_TAG);
    // Sprites share priority with BG0; keep the icon above the map, below the cursor.
    sprite->oam.priority = 1;
    sprite->x = sWorldMap->playerCellX * 8 + 4;
    sprite->y = sWorldMap->playerCellY * 8 + 4;
    sWorldMap->playerIconSpriteId = sprite - gSprites;
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

    if (sWorldMap->playerIconSpriteId != SPRITE_NONE)
    {
        struct Sprite *icon = &gSprites[sWorldMap->playerIconSpriteId];

        icon->x = sWorldMap->playerCellX * 8 + 4 - sWorldMap->scrollX;
        icon->y = sWorldMap->playerCellY * 8 + 4 - sWorldMap->scrollY;
    }

    sprite->x = sWorldMap->cursorX * 8 + 4 - sWorldMap->scrollX;
    sprite->y = sWorldMap->cursorY * 8 + 4 - sWorldMap->scrollY;
}

static void DestroyCursor(void)
{
    if (sWorldMap->playerIconSpriteId != SPRITE_NONE)
    {
        DestroySprite(&gSprites[sWorldMap->playerIconSpriteId]);
        FreeSpriteTilesByTag(PLAYER_ICON_TAG);
        FreeSpritePaletteByTag(PLAYER_ICON_TAG);
    }
    DestroySprite(&gSprites[sWorldMap->cursorSpriteId]);
    FreeSpriteTilesByTag(CURSOR_TAG);
    FreeSpritePaletteByTag(CURSOR_TAG);
}

// Redraws only the windows whose content changed.
static void UpdateWindows(void)
{
    mapsec_u16_t mapSec = GetWorldMapSecIdAt(sWorldMap->cursorX, sWorldMap->cursorY);

    u32 pos = mapSec == MAPSEC_EVER_GRANDE_CITY ? GetCursorPosWithinMapSec() : 0;

    if (mapSec != sWorldMap->shownMapSec || pos != sWorldMap->shownPos)
    {
        sWorldMap->shownMapSec = mapSec;
        sWorldMap->shownPos = pos;
        FillWindowPixelBuffer(WIN_MAPSEC_NAME, PIXEL_FILL(1));
        if (mapSec < MAPSEC_NONE)
        {
            const u8 *name = gRegionMapEntries[mapSec].name;

            // Ever Grande's second cell is the Pokemon Center once the League is unlocked.
            if (sWorldMap->flyMode && mapSec == MAPSEC_EVER_GRANDE_CITY && FlagGet(FLAG_LANDMARK_POKEMON_LEAGUE))
                name = pos == 0 ? gText_PokemonLeague : gText_PokemonCenter;
            AddTextPrinterParameterized3(WIN_MAPSEC_NAME, FONT_NORMAL, 2, 1, sTextColors, 0, name);
        }
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

// Bakes the dimming into the unfaded buffer so palette fades keep it.
static void DimLockedGroups(void)
{
    u32 panel, group;

    u32 coeff = DIM_COEFF;

    sWorldMap->dimmedGroups = 0;
    for (group = 0; group < NUM_BANK_GROUPS; group++)
        sWorldMap->dimmedGroups |= 1 << group;
    if (sWorldMap->flyMode)
    {
        sWorldMap->dimmedGroups &= ~(1 << sWorldMap->flyGroup);
        coeff = FLY_DIM_COEFF;
    }
    else
    {
        for (panel = 0; panel < NUM_PANELS; panel++)
        {
            if (IsPanelUnlocked(panel))
                sWorldMap->dimmedGroups &= ~(1 << sPanelGroup[panel]);
        }
    }

    for (group = 0; group < NUM_BANK_GROUPS; group++)
    {
        u32 offset = BG_PLTT_ID(sBankGroups[group].first);
        u32 count = sBankGroups[group].count * 16;

        if (sWorldMap->dimmedGroups & (1 << group))
        {
            BlendPalette(offset, count, coeff, RGB_BLACK);
            CpuCopy16(&gPlttBufferFaded[offset], &gPlttBufferUnfaded[offset], count * sizeof(u16));
        }
    }
}

static void CB2_WorldMap(void)
{
    if (!gPaletteFade.active)
    {
        s32 dx = 0, dy = 0;

        if (JOY_NEW(B_BUTTON) || (!sWorldMap->flyMode && JOY_NEW(START_BUTTON)))
        {
            if (sWorldMap->flyMode)
                PlaySE(SE_SELECT);
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
            SetMainCallback2(CB2_ExitWorldMap);
            return;
        }
        if (sWorldMap->flyMode && JOY_NEW(A_BUTTON) && CanFlyFromCell())
        {
            PlaySE(SE_SELECT);
            sWorldMap->choseFlyLocation = TRUE;
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
            SetMainCallback2(CB2_ExitWorldMap);
            return;
        }
        if (JOY_NEW(R_BUTTON))
        {
            SetCursorToPanel(NextUnlockedPanel(sWorldMap->panel, 1));
            FollowCursor(TRUE);
        }
        else if (JOY_NEW(L_BUTTON))
        {
            SetCursorToPanel(NextUnlockedPanel(sWorldMap->panel, -1));
            FollowCursor(TRUE);
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
        bool32 flyMode = sWorldMap->flyMode;
        bool32 chose = sWorldMap->choseFlyLocation;
        mapsec_u16_t mapSec = GetWorldMapSecIdAt(sWorldMap->cursorX, sWorldMap->cursorY);
        u32 pos = GetCursorPosWithinMapSec();

        if (flyMode)
            DestroyFlyIcons();
        DestroyCursor();
        FreeAllWindowBuffers();
        TRY_FREE_AND_SET_NULL(sWorldMap);
        if (!flyMode)
        {
            SetMainCallback2(CB2_ReturnToField);
        }
        else if (chose)
        {
            SetFlyDestinationToMapSec(mapSec, pos);
            ReturnToFieldFromFlyMapSelect();
        }
        else
        {
            SetMainCallback2(CB2_ReturnToPartyMenuFromFlyMap);
        }
    }
}

static u32 GetMapSecGroup(mapsec_u16_t mapSec)
{
    switch (GetRegionForSectionId(mapSec))
    {
    case REGION_JOHTO:
        return GROUP_JOHTO;
    case REGION_KANTO:
        return GetKantoSubregion(mapSec) == KANTO_SUBREGION_KANTO ? GROUP_KANTO : GROUP_SEVII;
    default:
        return GROUP_HOENN;
    }
}

// Cells of the same MAPSEC before the cursor in row-major order.
static u32 GetCursorPosWithinMapSec(void)
{
    mapsec_u16_t mapSec = GetWorldMapSecIdAt(sWorldMap->cursorX, sWorldMap->cursorY);
    u32 pos = 0;
    s32 x, y;

    for (y = sPanels[sWorldMap->panel].y; y <= sWorldMap->cursorY; y++)
    {
        for (x = sPanels[sWorldMap->panel].x; x < sPanels[sWorldMap->panel].x + sPanels[sWorldMap->panel].w; x++)
        {
            if (y == sWorldMap->cursorY && x >= sWorldMap->cursorX)
                break;
            if (sWorldMapSections[y][x] == mapSec)
                pos++;
        }
    }
    return pos;
}

static bool32 CanFlyFromCell(void)
{
    mapsec_u16_t mapSec = GetWorldMapSecIdAt(sWorldMap->cursorX, sWorldMap->cursorY);
    u32 i;

    if (GetMapSecGroup(mapSec) != sWorldMap->flyGroup)
        return FALSE;
    for (i = 0; i < ARRAY_COUNT(sFlyLocations); i++)
    {
        if (sFlyLocations[i].mapsec == mapSec)
            return FlagGet(sFlyLocations[i].flag);
    }
    return FALSE;
}

// Sprite data for SpriteCB_FlyIcon
#define sCellX      data[0]
#define sCellY      data[1]
#define sIconMapSec data[2]
#define sFlicker    data[3]
#define sOutline    data[4]

static void SpriteCB_FlyIcon(struct Sprite *sprite)
{
    s32 x = sprite->sCellX * 8 + 4 - sWorldMap->scrollX;
    s32 y = sprite->sCellY * 8 + 4 - sWorldMap->scrollY;

    if (sprite->sOutline)
    {
        x -= 4;
        y -= 4;
    }
    sprite->x = x;
    sprite->y = y;

    // OAM coordinates wrap, so sprites outside the screen are hidden explicitly.
    if (x < -FLY_ICON_MARGIN || x > DISPLAY_WIDTH + FLY_ICON_MARGIN || y < -FLY_ICON_MARGIN || y > DISPLAY_HEIGHT + FLY_ICON_MARGIN)
    {
        sprite->invisible = TRUE;
        return;
    }

    if (GetWorldMapSecIdAt(sWorldMap->cursorX, sWorldMap->cursorY) == sprite->sIconMapSec)
    {
        if (++sprite->sFlicker > 16)
        {
            sprite->sFlicker = 0;
            sprite->invisible = !sprite->invisible;
        }
    }
    else
    {
        sprite->sFlicker = 16;
        sprite->invisible = FALSE;
    }
}

static void CreateFlyIcons(void)
{
    struct SpriteSheet sheet = { .data = sWorldMap->flyIconGfx, .size = sizeof(sWorldMap->flyIconGfx), .tag = FLY_ICON_TAG };
    u32 i;

    DecompressDataWithHeaderWram(sFlyIcons_Gfx, sWorldMap->flyIconGfx);
    LoadSpriteSheet(&sheet);
    LoadSpritePalette(&sFlyIconSpritePalette);

    for (i = 0; i < ARRAY_COUNT(sFlyLocations); i++)
    {
        mapsec_u16_t mapSec = sFlyLocations[i].mapsec;
        bool32 visited = FlagGet(sFlyLocations[i].flag);
        u32 width = gRegionMapEntries[mapSec].width;
        u32 height = gRegionMapEntries[mapSec].height;
        u32 shape, spriteId;
        s32 x, y;
        bool32 found = FALSE;

        // First cell in row-major order is the MAPSEC's top-left.
        for (y = 0; y < WORLD_MAP_CELLS_H && !found; y++)
        {
            for (x = 0; x < WORLD_MAP_CELLS_W; x++)
            {
                if (sWorldMapSections[y][x] == mapSec)
                {
                    found = TRUE;
                    break;
                }
            }
        }
        if (!found)
            continue;
        y--;

        if (width == 2)
            shape = SPRITE_SHAPE(16x8);
        else if (height == 2)
            shape = SPRITE_SHAPE(8x16);
        else
            shape = SPRITE_SHAPE(8x8);

        spriteId = CreateSpriteUnchecked(&sFlyIconSpriteTemplate, 0, 0, 10);
        if (spriteId == MAX_SPRITES)
            continue;

        gSprites[spriteId].sCellX = x;
        gSprites[spriteId].sCellY = y;
        gSprites[spriteId].sIconMapSec = mapSec;
        gSprites[spriteId].sFlicker = 16;
        gSprites[spriteId].callback = SpriteCB_FlyIcon;
        if (mapSec == MAPSEC_BATTLE_FRONTIER)
        {
            // Battle Frontier has no icon of its own, only a red outline once discovered.
            if (!visited)
            {
                DestroySprite(&gSprites[spriteId]);
                continue;
            }
            gSprites[spriteId].oam.size = SPRITE_SIZE(16x16);
            gSprites[spriteId].sOutline = TRUE;
            StartSpriteAnim(&gSprites[spriteId], FLY_OUTLINE_ANIM);
            continue;
        }
        gSprites[spriteId].oam.shape = shape;
        StartSpriteAnim(&gSprites[spriteId], visited ? shape : shape + 3);
    }
}

static void DestroyFlyIcons(void)
{
    u32 i;

    for (i = 0; i < MAX_SPRITES; i++)
    {
        if (gSprites[i].inUse && gSprites[i].callback == SpriteCB_FlyIcon)
            DestroySprite(&gSprites[i]);
    }
    FreeSpriteTilesByTag(FLY_ICON_TAG);
    FreeSpritePaletteByTag(FLY_ICON_TAG);
}

#undef sCellX
#undef sCellY
#undef sIconMapSec
#undef sFlicker
#undef sOutline
