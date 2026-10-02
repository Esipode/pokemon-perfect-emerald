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

// A rect of a panel-local region map grid that moves as one piece to the world grid.
struct WorldMapCluster
{
    u8 src;
    u8 x;
    u8 y;
    u8 w;
    u8 h;
    s8 offsetX;
    s8 offsetY;
};

#include "data/region_map/world_map_layout.h"
#include "data/region_map/region_map_layout_world.h"

/*
 *  Combined Johto / Kanto / Hoenn / Sevii map. Text-mode BG0 (64x64 tiles, 4bpp) built by
 *  tools/gs_convert/build_world_map2.py; scrolled with BG offsets, no zoom.
 *  The world is one continuous grid of MAPSEC cells (sWorldMapSections) plus a region grid
 *  (sWorldMapRegions) that says which region owns each cell, including the sea around it.
 *  Open sea (region 0) is always crossable; the cursor stops only at locked regions and the edge.
 *  Each lock unit owns a palette bank, so a locked one gets the grey ramp (bank 6) copied over it.
 *  The Sevii region has two lock units: islands 1-3 and islands 4-7.
 *  BG1 holds the text windows (palette bank 15).
 *  Fly mode (CB2_OpenFlyMap): every region can be scrolled to, but only fly points in the player's
 *  current region are selectable; other regions are blended lightly toward the sea colour.
 */

#define WORLD_MAP_PX_W (WORLD_MAP_CELLS_W * 8)
#define WORLD_MAP_PX_H (WORLD_MAP_CELLS_H * 8)
#define SCROLL_MAX_X (WORLD_MAP_PX_W - DISPLAY_WIDTH)
#define SCROLL_MAX_Y (WORLD_MAP_PX_H - DISPLAY_HEIGHT)
// The art is drawn this many px left of the cell grid; sprites and scroll limits use the grid.
#define MAP_ART_SHIFT_X 1
#define SCROLL_MARGIN (3 * 8)
#define CURSOR_REPEAT_DELAY 4
#define CURSOR_TAG 0
#define PLAYER_ICON_TAG 1
#define NO_REGION 0xFF
#define NUM_BANK_GROUPS 4
#define SEA_COLOR RGB(2, 4, 8) // WORLD_MAP_BANK_SEA ocean colour
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
    u8 region;
    u8 moveDelay;
    u8 cursorSpriteId;
    u8 playerRegion; // NO_REGION when the player has no icon
    bool8 playerInSevii4567;
    u8 playerIconSpriteId;
    u8 playerCellX;
    u8 playerCellY;
    u8 shownRegion;
    mapsec_u16_t shownMapSec;
    u8 shownPos;
    bool8 flyMode;
    bool8 canSwitchRegions;
    u8 flyGroup;
    bool8 choseFlyLocation;
    u8 cursorGfx[0x100];
    u8 flyIconGfx[0x1c0];
};

struct BankGroup
{
    u8 first;
    u8 count;
};

// Bank groups are the regions, in WORLD_MAP_REGION_* order minus one.
enum
{
    GROUP_JOHTO,
    GROUP_KANTO,
    GROUP_HOENN,
    GROUP_SEVII,
};

enum
{
    WIN_MAPSEC_NAME,
    WIN_REGION_NAME,
    WIN_FLY_PROMPT,
    WIN_HINT,
    WIN_COUNT,
};

static EWRAM_DATA struct WorldMap *sWorldMap = NULL;
static bool8 sOpenInFlyMode;
static MainCallback sReturnCallback;

static const u16 sWorldMap_Pal[] = INCGFX_U16("graphics/world_map/map.pal", ".gbapal");
static const u32 sWorldMap_Gfx[] = INCGFX_U32("graphics/world_map/tiles.png", ".4bpp.smol");
static const u32 sWorldMap_Tilemap[] = INCGFX_U32("graphics/world_map/map.bin", ".smolTM");

static const u16 sFlyIcons_Pal[] = INCGFX_U16("graphics/region_map/fly_target_icons.png", ".gbapal");
static const u32 sFlyIcons_Gfx[] = INCGFX_U32("graphics/region_map/fly_target_icons.png", ".4bpp.smol");

static const u16 sCursor_Pal[] = INCGFX_U16("graphics/region_map/cursor.pal", ".gbapal");
static const u32 sCursor_Gfx[] = INCGFX_U32("graphics/region_map/cursor_small.png", ".4bpp.smol");

static const u8 sText_Johto[] = _("JOHTO");
static const u8 sText_Sevii[] = _("SEVII ISLANDS");
static const u8 sText_RegionHint[] = _("L/R: REGION");
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
    [WIN_HINT] = {
        .bg = 1,
        .tilemapLeft = 19,
        .tilemapTop = 18,
        .width = 11,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0x53
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

static const u8 *const sRegionNames[WORLD_MAP_NUM_REGIONS + 1] =
{
    [WORLD_MAP_REGION_JOHTO] = sText_Johto,
    [WORLD_MAP_REGION_KANTO] = gText_Kanto,
    [WORLD_MAP_REGION_HOENN] = gText_Hoenn,
    [WORLD_MAP_REGION_SEVII] = sText_Sevii,
};

static const struct BankGroup sBankGroups[NUM_BANK_GROUPS] =
{
    [GROUP_JOHTO] = { WORLD_MAP_BANK_JOHTO, 1 },
    [GROUP_KANTO] = { WORLD_MAP_BANK_KANTO, 1 },
    [GROUP_HOENN] = { WORLD_MAP_BANK_HOENN, 1 },
    [GROUP_SEVII] = { WORLD_MAP_BANK_SEVII123, 2 },
};

static void CB2_WorldMap(void);
static void CB2_ExitWorldMap(void);
static void VBlankCB_WorldMap(void);
static void ClampScroll(void);
static void FollowCursor(bool32 center);
static void DimLockedGroups(void);
static bool32 IsRegionUnlocked(u32 region);
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
static void SetCursorToRegion(u32 region);
static u32 NextUnlockedRegion(u32 region, s32 step);
static bool32 CanSwitchRegions(void);

static bool32 IsSeviiUnitUnlocked(bool32 is4567)
{
    if (sWorldMap->flyMode)
        return TRUE;
    if (sWorldMap->playerRegion == WORLD_MAP_REGION_SEVII && sWorldMap->playerInSevii4567 == is4567)
        return TRUE;
    return FlagGet(is4567 ? FLAG_SYS_SEVII_MAP_4567 : FLAG_SYS_SEVII_MAP_123);
}

static bool32 IsRegionUnlocked(u32 region)
{
    u32 flag;

    if (sWorldMap->flyMode || region == sWorldMap->playerRegion)
        return TRUE;
    switch (region)
    {
    case WORLD_MAP_REGION_JOHTO:
        return FlagGet(FLAG_VISITED_JOHTO);
    case WORLD_MAP_REGION_KANTO:
        if (FlagGet(FLAG_KANTO_HOENN_LINKED))
            return TRUE;
        // Any Kanto town visited (Pallet Town to Saffron City flags are contiguous).
        for (flag = FLAG_WORLD_MAP_PALLET_TOWN; flag <= FLAG_WORLD_MAP_SAFFRON_CITY; flag++)
        {
            if (FlagGet(flag))
                return TRUE;
        }
        return FALSE;
    case WORLD_MAP_REGION_SEVII:
        return IsSeviiUnitUnlocked(FALSE) || IsSeviiUnitUnlocked(TRUE);
    default:
        return TRUE;
    }
}

// L/R needs at least two reachable regions; Fly mode can always scroll to every region.
static bool32 CanSwitchRegions(void)
{
    u32 region, count = 0;

    if (sWorldMap->flyMode)
        return TRUE;
    for (region = 1; region <= WORLD_MAP_NUM_REGIONS; region++)
    {
        if (IsRegionUnlocked(region))
            count++;
    }
    return count > 1;
}

static u32 NextUnlockedRegion(u32 region, s32 step)
{
    u32 i;

    for (i = 0; i < WORLD_MAP_NUM_REGIONS; i++)
    {
        region = (region - 1 + WORLD_MAP_NUM_REGIONS + step) % WORLD_MAP_NUM_REGIONS + 1;
        if (IsRegionUnlocked(region))
            return region;
    }
    return region;
}

mapsec_u16_t GetWorldMapSecIdAt(u16 x, u16 y)
{
    if (x >= WORLD_MAP_CELLS_W || y >= WORLD_MAP_CELLS_H)
        return MAPSEC_NONE;
    return sWorldMapSections[y][x];
}

// Open sea is always enterable; a region's cells need the region unlocked, and Sevii 4-7 cells
// need their own unit.
static bool32 IsCellEnterable(s32 x, s32 y)
{
    u32 region = sWorldMapRegions[y][x];
    mapsec_u16_t mapSec;

    if (region == WORLD_MAP_REGION_SEA)
        return TRUE;
    if (!IsRegionUnlocked(region))
        return FALSE;
    mapSec = sWorldMapSections[y][x];
    if (region != WORLD_MAP_REGION_SEVII || mapSec == MAPSEC_NONE)
        return TRUE;
    return IsSeviiUnitUnlocked(GetKantoSubregion(mapSec) != KANTO_SUBREGION_SEVII123);
}

// Moves the cursor to the enterable MAPSEC cell of the region nearest to (targetX, targetY).
static void SnapCursorToRegionMapSec(u32 region, s32 targetX, s32 targetY)
{
    s32 x, y;
    s32 bestDist = 0x7FFF;
    s32 bestX = sWorldMap->cursorX, bestY = sWorldMap->cursorY;

    for (y = 0; y < WORLD_MAP_CELLS_H; y++)
    {
        for (x = 0; x < WORLD_MAP_CELLS_W; x++)
        {
            s32 dist = abs(x - targetX) + abs(y - targetY);

            if (sWorldMapRegions[y][x] == region && sWorldMapSections[y][x] != MAPSEC_NONE
             && dist < bestDist && IsCellEnterable(x, y))
            {
                bestDist = dist;
                bestX = x;
                bestY = y;
            }
        }
    }
    sWorldMap->cursorX = bestX;
    sWorldMap->cursorY = bestY;
    sWorldMap->region = region;
}

static void SetCursorToRegion(u32 region)
{
    SnapCursorToRegionMapSec(region, sWorldMapRegionAnchors[region].x, sWorldMapRegionAnchors[region].y);
}

static bool32 TryMoveCursor(s32 dx, s32 dy)
{
    s32 x = sWorldMap->cursorX + dx;
    s32 y = sWorldMap->cursorY + dy;

    if (x < 0 || y < 0 || x >= WORLD_MAP_CELLS_W || y >= WORLD_MAP_CELLS_H || !IsCellEnterable(x, y))
        return FALSE;
    sWorldMap->cursorX = x;
    sWorldMap->cursorY = y;
    // Open sea keeps the region the cursor came from.
    if (sWorldMapRegions[y][x] != WORLD_MAP_REGION_SEA)
        sWorldMap->region = sWorldMapRegions[y][x];
    FollowCursor(FALSE);
    return TRUE;
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
        if (sWorldMap->playerRegion != NO_REGION)
        {
            sWorldMap->cursorX = sWorldMap->playerCellX;
            sWorldMap->cursorY = sWorldMap->playerCellY;
            sWorldMap->region = sWorldMap->playerRegion;
        }
        else if (sWorldMap->flyMode)
        {
            SetCursorToRegion(sWorldMap->flyGroup + 1);
        }
        else
        {
            SetCursorToRegion(WORLD_MAP_REGION_HOENN);
        }
        FollowCursor(TRUE);
        ApplyScroll();
        DimLockedGroups();
        DecompressDataWithHeaderWram(sCursor_Gfx, sWorldMap->cursorGfx);
        CreatePlayerIcon();
        if (sWorldMap->flyMode)
            CreateFlyIcons();
        CreateCursor();
        sWorldMap->shownRegion = 0xFF;
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
        sWorldMap->canSwitchRegions = CanSwitchRegions();
        if (sWorldMap->canSwitchRegions)
        {
            PutWindowTilemap(WIN_HINT);
            FillWindowPixelBuffer(WIN_HINT, PIXEL_FILL(1));
            AddTextPrinterParameterized3(WIN_HINT, FONT_NORMAL, 2, 1, sTextColors, 0, sText_RegionHint);
            CopyWindowToVram(WIN_HINT, COPYWIN_FULL);
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

void FieldInitWorldMap(MainCallback callback)
{
    SetVBlankCallback(NULL);
    sReturnCallback = callback;
    SetMainCallback2(CB2_OpenWorldMap);
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
    SetGpuReg(REG_OFFSET_BG0HOFS, sWorldMap->scrollX + MAP_ART_SHIFT_X);
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
    u32 src, i, region;
    s32 x, y;
    bool32 found = FALSE;

    sWorldMap->playerRegion = NO_REGION;
    if (IsEventIslandMapSecId(gMapHeader.regionMapSectionId))
        return;

    GetPlayerPositionOnRegionMap(&pos);
    switch (GetRegionForSectionId(pos.mapSecId))
    {
    case REGION_JOHTO:
        src = WORLD_MAP_SRC_JOHTO;
        region = WORLD_MAP_REGION_JOHTO;
        break;
    case REGION_KANTO:
        region = WORLD_MAP_REGION_KANTO;
        switch (GetKantoSubregion(pos.mapSecId))
        {
        case KANTO_SUBREGION_SEVII123:
            src = WORLD_MAP_SRC_SEVII123;
            region = WORLD_MAP_REGION_SEVII;
            break;
        case KANTO_SUBREGION_SEVII45:
            src = WORLD_MAP_SRC_SEVII45;
            region = WORLD_MAP_REGION_SEVII;
            break;
        case KANTO_SUBREGION_SEVII67:
            src = WORLD_MAP_SRC_SEVII67;
            region = WORLD_MAP_REGION_SEVII;
            break;
        default:
            src = WORLD_MAP_SRC_KANTO;
            break;
        }
        break;
    default:
        src = WORLD_MAP_SRC_HOENN;
        region = WORLD_MAP_REGION_HOENN;
        break;
    }

    x = pos.cursorPosX - MAPCURSOR_X_MIN;
    y = pos.cursorPosY - MAPCURSOR_Y_MIN;
    for (i = 0; i < ARRAY_COUNT(sWorldMapClusters); i++)
    {
        const struct WorldMapCluster *cluster = &sWorldMapClusters[i];

        if (cluster->src == src && x >= cluster->x && x < cluster->x + cluster->w
         && y >= cluster->y && y < cluster->y + cluster->h)
        {
            x += cluster->offsetX;
            y += cluster->offsetY;
            found = TRUE;
            break;
        }
    }
    if (!found)
    {
        // Position lies outside every cluster: use the first cell of the player's MAPSEC.
        for (y = 0; y < WORLD_MAP_CELLS_H && !found; y++)
        {
            for (x = 0; x < WORLD_MAP_CELLS_W; x++)
            {
                if (sWorldMapSections[y][x] == pos.mapSecId)
                {
                    found = TRUE;
                    break;
                }
            }
        }
        if (!found)
            return;
        y--;
    }
    sWorldMap->playerRegion = region;
    sWorldMap->playerInSevii4567 = src == WORLD_MAP_SRC_SEVII45 || src == WORLD_MAP_SRC_SEVII67;
    sWorldMap->playerCellX = x;
    sWorldMap->playerCellY = y;
}

static void CreatePlayerIcon(void)
{
    struct Sprite *sprite;

    sWorldMap->playerIconSpriteId = SPRITE_NONE;
    if (sWorldMap->playerRegion == NO_REGION)
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
    if (sWorldMap->region != sWorldMap->shownRegion)
    {
        sWorldMap->shownRegion = sWorldMap->region;
        FillWindowPixelBuffer(WIN_REGION_NAME, PIXEL_FILL(1));
        AddTextPrinterParameterized3(WIN_REGION_NAME, FONT_NORMAL, 2, 1, sTextColors, 0, sRegionNames[sWorldMap->region]);
        CopyWindowToVram(WIN_REGION_NAME, COPYWIN_FULL);
    }
}

// Blends toward the sea colour so a dimmed region stays readable against the ocean.
static void DimBanks(u32 first, u32 count, u32 coeff)
{
    u32 offset = BG_PLTT_ID(first);

    BlendPalette(offset, count * 16, coeff, SEA_COLOR);
    CpuCopy16(&gPlttBufferFaded[offset], &gPlttBufferUnfaded[offset], count * 16 * sizeof(u16));
}

// Swaps in the grey ramp: details and the region label take land/sea colours, `???` shows.
static void LockBank(u32 bank)
{
    u32 offset = BG_PLTT_ID(bank);

    CpuCopy16(&gPlttBufferUnfaded[BG_PLTT_ID(WORLD_MAP_BANK_LOCKED)], &gPlttBufferUnfaded[offset], PLTT_SIZE_4BPP);
    CpuCopy16(&gPlttBufferUnfaded[offset], &gPlttBufferFaded[offset], PLTT_SIZE_4BPP);
}

// Bakes the lock/dim looks into the unfaded buffer so palette fades keep them.
static void DimLockedGroups(void)
{
    u32 group, i;

    for (group = 0; group < NUM_BANK_GROUPS; group++)
    {
        if (sWorldMap->flyMode)
        {
            if (group != sWorldMap->flyGroup)
                DimBanks(sBankGroups[group].first, sBankGroups[group].count, FLY_DIM_COEFF);
        }
        else if (group == GROUP_SEVII)
        {
            if (!IsSeviiUnitUnlocked(FALSE))
                LockBank(WORLD_MAP_BANK_SEVII123);
            if (!IsSeviiUnitUnlocked(TRUE))
                LockBank(WORLD_MAP_BANK_SEVII4567);
        }
        else if (!IsRegionUnlocked(group + 1))
        {
            for (i = 0; i < sBankGroups[group].count; i++)
                LockBank(sBankGroups[group].first + i);
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
        if (sWorldMap->canSwitchRegions)
        {
            if (JOY_NEW(R_BUTTON))
            {
                SetCursorToRegion(NextUnlockedRegion(sWorldMap->region, 1));
                FollowCursor(TRUE);
            }
            else if (JOY_NEW(L_BUTTON))
            {
                SetCursorToRegion(NextUnlockedRegion(sWorldMap->region, -1));
                FollowCursor(TRUE);
            }
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
            MainCallback callback = sReturnCallback != NULL ? sReturnCallback : CB2_ReturnToField;

            sReturnCallback = NULL;
            SetMainCallback2(callback);
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

    for (y = 0; y <= sWorldMap->cursorY; y++)
    {
        for (x = 0; x < WORLD_MAP_CELLS_W; x++)
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
        u32 shape, spriteId, region = GetMapSecGroup(mapSec) + 1;
        s32 x, y;
        bool32 found = FALSE;

        // First cell in row-major order is the MAPSEC's top-left.
        for (y = 0; y < WORLD_MAP_CELLS_H && !found; y++)
        {
            for (x = 0; x < WORLD_MAP_CELLS_W; x++)
            {
                if (sWorldMapSections[y][x] == mapSec && sWorldMapRegions[y][x] == region)
                {
                    found = TRUE;
                    break;
                }
            }
        }
        y--;
        if (!found)
            continue;

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
