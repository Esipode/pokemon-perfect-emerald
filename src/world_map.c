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
 *  BG1 holds the text windows (palette bank 15) on transparent backgrounds. Hardware windows
 *  WIN0 (top bar) and WIN1 (bottom bar) darken BG0 and OBJ behind the text; BG1 is not a blend
 *  target, so the text stays crisp.
 *  Fly mode (CB2_OpenFlyMap): every region can be scrolled to, but only fly points in the player's
 *  current region are selectable; other regions are blended lightly toward the sea colour.
 */

#define WORLD_MAP_PX_W (WORLD_MAP_CELLS_W * 8)
#define WORLD_MAP_PX_H (WORLD_MAP_CELLS_H * 8)
#define SCROLL_MAX_X (WORLD_MAP_PX_W - DISPLAY_WIDTH)
#define SCROLL_MAX_Y (WORLD_MAP_PX_H - DISPLAY_HEIGHT)
// The art is drawn this many px left of the cell grid; sprites and scroll limits use the grid.
#define MAP_ART_SHIFT_X 0
#define SCROLL_MARGIN (3 * 8)
#define CURSOR_REPEAT_DELAY 4
#define SCROLL_EASE_DIV 3 // Camera covers 1/3 of the remaining distance per frame, at least 1 px
#define UI_TAG 0 // Tile and palette tag of ui_sprites.png (brackets, rings, halo)
#define PLAYER_ICON_TAG 1
#define NO_REGION 0xFF
#define NUM_BANK_GROUPS 4
#define SEA_COLOR RGB(2, 4, 8) // WORLD_MAP_BANK_SEA ocean colour
#define FLY_DIM_COEFF 5
#define SPRITE_MARGIN 32
#define HALO_OFFSET_Y 2
#define CURSOR_PULSE_SHIFT 4
#define BAR_HEIGHT 16
#define BAR_DARKEN 8
#define BAR_TEXT_PAD 2
#define PIP_PAL_FIRST 3 // Text palette index of the open-sea pip; regions follow in id order
#define HINT_NONE 0xFF

// Tile offsets of the frames in graphics/world_map/ui_sprites.png (tools/gs_convert/build_world_map_ui.py).
#define UI_TILE_BRACKET     0
#define UI_TILE_RING        1  // 16x16 bright, then faint
#define UI_TILE_RING_FAINT  5
#define UI_TILE_WIDE        9  // 32x16 bright, then faint
#define UI_TILE_WIDE_FAINT  17
#define UI_TILE_TALL        25 // 16x32 bright, then faint
#define UI_TILE_TALL_FAINT  33
#define UI_TILE_FRONTIER    41 // 16x16 Battle Frontier outline
#define UI_TILE_HALO_SMALL  45
#define UI_TILE_HALO_LARGE  49
#define UI_TILE_COUNT       53

struct WorldMap
{
    s16 scrollX;
    s16 scrollY;
    s16 targetX; // Scroll position the camera eases toward
    s16 targetY;
    u8 cursorX;
    u8 cursorY;
    u8 region;
    u8 moveDelay;
    u8 cursorSpriteIds[4]; // Corner brackets: top-left, top-right, bottom-left, bottom-right
    u8 cursorRectX; // MAPSEC cell rect under the cursor
    u8 cursorRectY;
    u8 cursorRectW;
    u8 cursorRectH;
    u8 rectForX; // Cursor cell the rect was computed for
    u8 rectForY;
    u8 haloSpriteId;
    u8 playerRegion; // NO_REGION when the player has no icon
    bool8 playerInSevii4567;
    u8 playerIconSpriteId;
    u8 playerCellX;
    u8 playerCellY;
    u8 shownRegion;
    mapsec_u16_t shownMapSec;
    u8 shownPos;
    u8 shownHint;
    bool8 flyMode;
    bool8 canSwitchRegions;
    u8 flyGroup;
    bool8 choseFlyLocation;
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

static const u16 sUiSprites_Pal[] = INCGFX_U16("graphics/world_map/ui_sprites.png", ".gbapal");
static const u32 sUiSprites_Gfx[] = INCGFX_U32("graphics/world_map/ui_sprites.png", ".4bpp");

static const u8 sText_Johto[] = _("JOHTO");
static const u8 sText_Sevii[] = _("SEVII ISLANDS");
static const u8 sText_HintBack[] = _("{B_BUTTON}BACK");
static const u8 sText_HintRegionBack[] = _("{L_BUTTON}{R_BUTTON}REGION {B_BUTTON}BACK");
static const u8 sText_HintFlyBack[] = _("{A_BUTTON}FLY {B_BUTTON}BACK");
static const u8 sText_HintFlyRegionBack[] = _("{A_BUTTON}FLY {L_BUTTON}{R_BUTTON}REGION {B_BUTTON}BACK");

// Indexed by (A usable ? 2 : 0) + (L/R usable ? 1 : 0).
static const u8 *const sHintTexts[] =
{
    sText_HintBack,
    sText_HintRegionBack,
    sText_HintFlyBack,
    sText_HintFlyRegionBack,
};
static const u8 sTextColors[] = {0, 1, 2}; // transparent, text, shadow

// Bar text palette: white text on the darkened map, then one pip colour per region (land tints).
static const u16 sBarText_Pal[16] =
{
    [0] = RGB_BLACK,
    [1] = RGB_WHITE,
    [2] = RGB(5, 5, 7),
    [PIP_PAL_FIRST + WORLD_MAP_REGION_SEA] = RGB(14, 14, 16),
    [PIP_PAL_FIRST + WORLD_MAP_REGION_JOHTO] = RGB(22, 22, 13),
    [PIP_PAL_FIRST + WORLD_MAP_REGION_KANTO] = RGB(11, 22, 17),
    [PIP_PAL_FIRST + WORLD_MAP_REGION_HOENN] = RGB(13, 24, 10),
    [PIP_PAL_FIRST + WORLD_MAP_REGION_SEVII] = RGB(27, 25, 18),
};

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

// Top bar: region (left) and MAPSEC name (right). Bottom bar: Fly prompt (left) and hints (right).
static const struct WindowTemplate sWorldMapWindowTemplates[WIN_COUNT + 1] =
{
    [WIN_MAPSEC_NAME] = {
        .bg = 1,
        .tilemapLeft = 14,
        .tilemapTop = 0,
        .width = 16,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0x1D
    },
    [WIN_REGION_NAME] = {
        .bg = 1,
        .tilemapLeft = 0,
        .tilemapTop = 0,
        .width = 14,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0x01
    },
    [WIN_FLY_PROMPT] = {
        .bg = 1,
        .tilemapLeft = 0,
        .tilemapTop = 18,
        .width = 11,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0x3D
    },
    [WIN_HINT] = {
        .bg = 1,
        .tilemapLeft = 11,
        .tilemapTop = 18,
        .width = 19,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0x53
    },
    DUMMY_WIN_TEMPLATE
};

static const struct SpritePalette sUiSpritePalette =
{
    .data = sUiSprites_Pal,
    .tag = UI_TAG
};

static const struct SpriteSheet sUiSpriteSheet =
{
    .data = sUiSprites_Gfx,
    .size = UI_TILE_COUNT * TILE_SIZE_4BPP,
    .tag = UI_TAG
};

static const struct OamData sBracketOam =
{
    .shape = SPRITE_SHAPE(8x8),
    .size = SPRITE_SIZE(8x8),
    .priority = 0
};

// One anim per corner, flipping the top-left bracket.
static const union AnimCmd sBracketAnim_TopLeft[] = { ANIMCMD_FRAME(UI_TILE_BRACKET, 1, FALSE, FALSE), ANIMCMD_END };
static const union AnimCmd sBracketAnim_TopRight[] = { ANIMCMD_FRAME(UI_TILE_BRACKET, 1, TRUE, FALSE), ANIMCMD_END };
static const union AnimCmd sBracketAnim_BottomLeft[] = { ANIMCMD_FRAME(UI_TILE_BRACKET, 1, FALSE, TRUE), ANIMCMD_END };
static const union AnimCmd sBracketAnim_BottomRight[] = { ANIMCMD_FRAME(UI_TILE_BRACKET, 1, TRUE, TRUE), ANIMCMD_END };

static const union AnimCmd *const sBracketAnims[] =
{
    sBracketAnim_TopLeft,
    sBracketAnim_TopRight,
    sBracketAnim_BottomLeft,
    sBracketAnim_BottomRight
};

static void SpriteCB_Bracket(struct Sprite *sprite);

static const struct SpriteTemplate sBracketSpriteTemplate =
{
    .tileTag = UI_TAG,
    .paletteTag = UI_TAG,
    .oam = &sBracketOam,
    .anims = sBracketAnims,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_Bracket
};

static const struct OamData sHaloOam =
{
    .shape = SPRITE_SHAPE(16x16),
    .size = SPRITE_SIZE(16x16),
    .priority = 1
};

static const union AnimCmd sHaloAnim[] =
{
    ANIMCMD_FRAME(UI_TILE_HALO_SMALL, 20),
    ANIMCMD_FRAME(UI_TILE_HALO_LARGE, 20),
    ANIMCMD_JUMP(0)
};

static const union AnimCmd *const sHaloAnims[] =
{
    sHaloAnim
};

static void SpriteCB_Halo(struct Sprite *sprite);

static const struct SpriteTemplate sHaloSpriteTemplate =
{
    .tileTag = UI_TAG,
    .paletteTag = UI_TAG,
    .oam = &sHaloOam,
    .anims = sHaloAnims,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_Halo
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

enum
{
    RING_ANIM_BRIGHT,
    RING_ANIM_FAINT,
    RING_ANIM_FRONTIER,
};

static const struct OamData sRingOam =
{
    .shape = SPRITE_SHAPE(16x16),
    .size = SPRITE_SIZE(16x16),
    .priority = 1
};

static const struct OamData sRingWideOam =
{
    .shape = SPRITE_SHAPE(32x16),
    .size = SPRITE_SIZE(32x16),
    .priority = 1
};

static const struct OamData sRingTallOam =
{
    .shape = SPRITE_SHAPE(16x32),
    .size = SPRITE_SIZE(16x32),
    .priority = 1
};

static const union AnimCmd sRingAnim_Bright[] = { ANIMCMD_FRAME(UI_TILE_RING, 5), ANIMCMD_END };
static const union AnimCmd sRingAnim_Faint[] = { ANIMCMD_FRAME(UI_TILE_RING_FAINT, 5), ANIMCMD_END };
static const union AnimCmd sRingAnim_Frontier[] = { ANIMCMD_FRAME(UI_TILE_FRONTIER, 5), ANIMCMD_END };
static const union AnimCmd sRingWideAnim_Bright[] = { ANIMCMD_FRAME(UI_TILE_WIDE, 5), ANIMCMD_END };
static const union AnimCmd sRingWideAnim_Faint[] = { ANIMCMD_FRAME(UI_TILE_WIDE_FAINT, 5), ANIMCMD_END };
static const union AnimCmd sRingTallAnim_Bright[] = { ANIMCMD_FRAME(UI_TILE_TALL, 5), ANIMCMD_END };
static const union AnimCmd sRingTallAnim_Faint[] = { ANIMCMD_FRAME(UI_TILE_TALL_FAINT, 5), ANIMCMD_END };

static const union AnimCmd *const sRingAnims[] =
{
    [RING_ANIM_BRIGHT]   = sRingAnim_Bright,
    [RING_ANIM_FAINT]    = sRingAnim_Faint,
    [RING_ANIM_FRONTIER] = sRingAnim_Frontier
};

static const union AnimCmd *const sRingWideAnims[] =
{
    [RING_ANIM_BRIGHT] = sRingWideAnim_Bright,
    [RING_ANIM_FAINT]  = sRingWideAnim_Faint
};

static const union AnimCmd *const sRingTallAnims[] =
{
    [RING_ANIM_BRIGHT] = sRingTallAnim_Bright,
    [RING_ANIM_FAINT]  = sRingTallAnim_Faint
};

static void SpriteCB_FlyRing(struct Sprite *sprite);

static const struct SpriteTemplate sRingSpriteTemplate =
{
    .tileTag = UI_TAG,
    .paletteTag = UI_TAG,
    .oam = &sRingOam,
    .anims = sRingAnims,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_FlyRing
};

static const struct SpriteTemplate sRingWideSpriteTemplate =
{
    .tileTag = UI_TAG,
    .paletteTag = UI_TAG,
    .oam = &sRingWideOam,
    .anims = sRingWideAnims,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_FlyRing
};

static const struct SpriteTemplate sRingTallSpriteTemplate =
{
    .tileTag = UI_TAG,
    .paletteTag = UI_TAG,
    .oam = &sRingTallOam,
    .anims = sRingTallAnims,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_FlyRing
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
static void EaseScroll(void);
static void FollowCursor(bool32 center, bool32 snap);
static void DimLockedGroups(void);
static bool32 IsRegionUnlocked(u32 region);
static void ApplyScroll(void);
static void LoadUiSprites(void);
static void CreateCursor(void);
static void UpdateCursorSprite(void);
static void DestroyCursor(void);
static void CreatePlayerIcon(void);
static void LocatePlayer(void);
static void UpdateWindows(void);
static void SetBarRegs(void);
static void ClearBarRegs(void);
static void PrintRightAligned(u32 windowId, const u8 *str);
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
    FollowCursor(FALSE, FALSE);
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
        LoadPalette(sBarText_Pal, BG_PLTT_ID(15), PLTT_SIZE_4BPP);
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
        FollowCursor(TRUE, TRUE);
        ApplyScroll();
        DimLockedGroups();
        LoadUiSprites();
        CreatePlayerIcon();
        if (sWorldMap->flyMode)
            CreateFlyIcons();
        CreateCursor();
        sWorldMap->shownRegion = 0xFF;
        sWorldMap->shownMapSec = 0xFFFF;
        sWorldMap->shownHint = HINT_NONE;
        PutWindowTilemap(WIN_MAPSEC_NAME);
        PutWindowTilemap(WIN_REGION_NAME);
        PutWindowTilemap(WIN_HINT);
        if (sWorldMap->flyMode)
        {
            PutWindowTilemap(WIN_FLY_PROMPT);
            FillWindowPixelBuffer(WIN_FLY_PROMPT, PIXEL_FILL(0));
            AddTextPrinterParameterized3(WIN_FLY_PROMPT, FONT_NORMAL, BAR_TEXT_PAD, 1, sTextColors, 0, gText_FlyToWhere);
            CopyWindowToVram(WIN_FLY_PROMPT, COPYWIN_FULL);
        }
        sWorldMap->canSwitchRegions = CanSwitchRegions();
        UpdateWindows();
        gMain.state++;
        break;
    case 3:
        BlendPalettes(PALETTES_ALL, 16, RGB_BLACK);
        SetVBlankCallback(VBlankCB_WorldMap);
        gMain.state++;
        break;
    case 4:
        SetBarRegs();
        SetGpuRegBits(REG_OFFSET_DISPCNT, DISPCNT_OBJ_1D_MAP | DISPCNT_OBJ_ON | DISPCNT_WIN0_ON | DISPCNT_WIN1_ON);
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
    if (sWorldMap->targetX < 0)
        sWorldMap->targetX = 0;
    if (sWorldMap->targetX > SCROLL_MAX_X)
        sWorldMap->targetX = SCROLL_MAX_X;
    if (sWorldMap->targetY < 0)
        sWorldMap->targetY = 0;
    if (sWorldMap->targetY > SCROLL_MAX_Y)
        sWorldMap->targetY = SCROLL_MAX_Y;
}

static s16 EaseAxis(s16 cur, s16 target)
{
    s32 d = target - cur;
    s32 step = d / SCROLL_EASE_DIV;

    if (step == 0)
        step = (d > 0) - (d < 0);
    return cur + step;
}

// Sprites and BG share scrollX/Y, so they always move together.
static void EaseScroll(void)
{
    sWorldMap->scrollX = EaseAxis(sWorldMap->scrollX, sWorldMap->targetX);
    sWorldMap->scrollY = EaseAxis(sWorldMap->scrollY, sWorldMap->targetY);
}

// Sets the scroll target; the camera eases there unless snap is set.
static void FollowCursor(bool32 center, bool32 snap)
{
    s32 x = sWorldMap->cursorX * 8;
    s32 y = sWorldMap->cursorY * 8;

    if (center)
    {
        sWorldMap->targetX = x + 4 - DISPLAY_WIDTH / 2;
        sWorldMap->targetY = y + 4 - DISPLAY_HEIGHT / 2;
    }
    else
    {
        if (x - sWorldMap->targetX < SCROLL_MARGIN)
            sWorldMap->targetX = x - SCROLL_MARGIN;
        if (x + 8 - sWorldMap->targetX > DISPLAY_WIDTH - SCROLL_MARGIN)
            sWorldMap->targetX = x + 8 - DISPLAY_WIDTH + SCROLL_MARGIN;
        if (y - sWorldMap->targetY < SCROLL_MARGIN)
            sWorldMap->targetY = y - SCROLL_MARGIN;
        if (y + 8 - sWorldMap->targetY > DISPLAY_HEIGHT - SCROLL_MARGIN)
            sWorldMap->targetY = y + 8 - DISPLAY_HEIGHT + SCROLL_MARGIN;
    }
    ClampScroll();
    if (snap)
    {
        sWorldMap->scrollX = sWorldMap->targetX;
        sWorldMap->scrollY = sWorldMap->targetY;
    }
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
    sWorldMap->haloSpriteId = SPRITE_NONE;
    if (sWorldMap->playerRegion == NO_REGION)
        return;
    sprite = CreatePlayerIconSprite(PLAYER_ICON_TAG, PLAYER_ICON_TAG);
    // Sprites share priority with BG0; keep the icon above the map, below the cursor.
    sprite->oam.priority = 1;
    sprite->x = sWorldMap->playerCellX * 8 + 4;
    sprite->y = sWorldMap->playerCellY * 8 + 4;
    sWorldMap->playerIconSpriteId = sprite - gSprites;

    // Subpriority 2 draws the halo behind the icon (subpriority 1).
    sWorldMap->haloSpriteId = CreateSprite(&sHaloSpriteTemplate, 0, 0, 2);
}

static void LoadUiSprites(void)
{
    LoadSpriteSheet(&sUiSpriteSheet);
    LoadSpritePalette(&sUiSpritePalette);
}

// Cells of the MAPSEC under the cursor, clipped to the actual cells along the cursor's row and column.
static void UpdateCursorRect(void)
{
    s32 cx = sWorldMap->cursorX;
    s32 cy = sWorldMap->cursorY;
    mapsec_u16_t mapSec = GetWorldMapSecIdAt(cx, cy);
    s32 maxW = 1, maxH = 1, x0 = cx, y0 = cy, w = 1, h = 1;

    if (mapSec < MAPSEC_NONE)
    {
        maxW = gRegionMapEntries[mapSec].width;
        maxH = gRegionMapEntries[mapSec].height;
    }
    if (gSaveBlock2Ptr->optionsClassicMapCursor)
        maxW = maxH = 1;
    while (cx - x0 < maxW - 1 && GetWorldMapSecIdAt(x0 - 1, cy) == mapSec)
        x0--;
    while (cy - y0 < maxH - 1 && GetWorldMapSecIdAt(cx, y0 - 1) == mapSec)
        y0--;
    while (w < maxW && GetWorldMapSecIdAt(x0 + w, cy) == mapSec)
        w++;
    while (h < maxH && GetWorldMapSecIdAt(cx, y0 + h) == mapSec)
        h++;
    sWorldMap->cursorRectX = x0;
    sWorldMap->cursorRectY = y0;
    sWorldMap->cursorRectW = w;
    sWorldMap->cursorRectH = h;
    sWorldMap->rectForX = cx;
    sWorldMap->rectForY = cy;
}

static bool32 IsOffScreen(s32 x, s32 y)
{
    return x < -SPRITE_MARGIN || x > DISPLAY_WIDTH + SPRITE_MARGIN || y < -SPRITE_MARGIN || y > DISPLAY_HEIGHT + SPRITE_MARGIN;
}

// Sprite data for SpriteCB_Bracket
#define sCorner data[0] // Bit 0: right, bit 1: bottom
#define sPulse  data[1]

static void SpriteCB_Bracket(struct Sprite *sprite)
{
    s32 pulse = (++sprite->sPulse >> CURSOR_PULSE_SHIFT) & 1;
    s32 left = sWorldMap->cursorRectX * 8;
    s32 top = sWorldMap->cursorRectY * 8;
    s32 right = left + sWorldMap->cursorRectW * 8;
    s32 bottom = top + sWorldMap->cursorRectH * 8;
    s32 x = (sprite->sCorner & 1) ? right - 4 + pulse : left + 4 - pulse;
    s32 y = (sprite->sCorner & 2) ? bottom - 4 + pulse : top + 4 - pulse;

    sprite->x = x - sWorldMap->scrollX;
    sprite->y = y - sWorldMap->scrollY;
    // OAM coordinates wrap, so sprites outside the screen are hidden explicitly.
    sprite->invisible = IsOffScreen(sprite->x, sprite->y);
}

static void SpriteCB_Halo(struct Sprite *sprite)
{
    sprite->x = sWorldMap->playerCellX * 8 + 4 - sWorldMap->scrollX;
    sprite->y = sWorldMap->playerCellY * 8 + 4 - sWorldMap->scrollY + HALO_OFFSET_Y;
    sprite->invisible = IsOffScreen(sprite->x, sprite->y);
}

static void CreateCursor(void)
{
    u32 i;

    UpdateCursorRect();
    for (i = 0; i < ARRAY_COUNT(sWorldMap->cursorSpriteIds); i++)
    {
        u32 spriteId = CreateSprite(&sBracketSpriteTemplate, 0, 0, 0);

        gSprites[spriteId].sCorner = i;
        StartSpriteAnim(&gSprites[spriteId], i);
        sWorldMap->cursorSpriteIds[i] = spriteId;
    }
    UpdateCursorSprite();
}

#undef sCorner
#undef sPulse

static void UpdateCursorSprite(void)
{
    if (sWorldMap->playerIconSpriteId != SPRITE_NONE)
    {
        struct Sprite *icon = &gSprites[sWorldMap->playerIconSpriteId];

        icon->x = sWorldMap->playerCellX * 8 + 4 - sWorldMap->scrollX;
        icon->y = sWorldMap->playerCellY * 8 + 4 - sWorldMap->scrollY;
    }

    if (sWorldMap->cursorX != sWorldMap->rectForX || sWorldMap->cursorY != sWorldMap->rectForY)
        UpdateCursorRect();
}

static void DestroyCursor(void)
{
    u32 i;

    if (sWorldMap->playerIconSpriteId != SPRITE_NONE)
    {
        DestroySprite(&gSprites[sWorldMap->playerIconSpriteId]);
        DestroySprite(&gSprites[sWorldMap->haloSpriteId]);
        FreeSpriteTilesByTag(PLAYER_ICON_TAG);
        FreeSpritePaletteByTag(PLAYER_ICON_TAG);
    }
    for (i = 0; i < ARRAY_COUNT(sWorldMap->cursorSpriteIds); i++)
        DestroySprite(&gSprites[sWorldMap->cursorSpriteIds[i]]);
    FreeSpriteTilesByTag(UI_TAG);
    FreeSpritePaletteByTag(UI_TAG);
}

static void PrintRightAligned(u32 windowId, const u8 *str)
{
    s32 x = GetWindowAttribute(windowId, WINDOW_WIDTH) * 8 - GetStringWidth(FONT_NORMAL, str, 0) - BAR_TEXT_PAD;

    AddTextPrinterParameterized3(windowId, FONT_NORMAL, x, 1, sTextColors, 0, str);
}

// Redraws only the windows whose content changed.
static void UpdateWindows(void)
{
    mapsec_u16_t mapSec = GetWorldMapSecIdAt(sWorldMap->cursorX, sWorldMap->cursorY);
    u32 pos = mapSec == MAPSEC_EVER_GRANDE_CITY ? GetCursorPosWithinMapSec() : 0;
    u32 hint = (sWorldMap->flyMode && CanFlyFromCell() ? 2 : 0) + (sWorldMap->canSwitchRegions ? 1 : 0);

    if (mapSec != sWorldMap->shownMapSec || pos != sWorldMap->shownPos)
    {
        sWorldMap->shownMapSec = mapSec;
        sWorldMap->shownPos = pos;
        FillWindowPixelBuffer(WIN_MAPSEC_NAME, PIXEL_FILL(0));
        if (mapSec < MAPSEC_NONE)
        {
            const u8 *name = gRegionMapEntries[mapSec].name;

            // Ever Grande's second cell is the Pokemon Center once the League is unlocked.
            if (sWorldMap->flyMode && mapSec == MAPSEC_EVER_GRANDE_CITY && FlagGet(FLAG_LANDMARK_POKEMON_LEAGUE))
                name = pos == 0 ? gText_PokemonLeague : gText_PokemonCenter;
            PrintRightAligned(WIN_MAPSEC_NAME, name);
        }
        CopyWindowToVram(WIN_MAPSEC_NAME, COPYWIN_FULL);
    }
    if (sWorldMap->region != sWorldMap->shownRegion)
    {
        sWorldMap->shownRegion = sWorldMap->region;
        FillWindowPixelBuffer(WIN_REGION_NAME, PIXEL_FILL(0));
        // White rim, then the region's land tint.
        FillWindowPixelRect(WIN_REGION_NAME, 1, BAR_TEXT_PAD, 4, 8, 8);
        FillWindowPixelRect(WIN_REGION_NAME, PIP_PAL_FIRST + sWorldMap->region, BAR_TEXT_PAD + 1, 5, 6, 6);
        AddTextPrinterParameterized3(WIN_REGION_NAME, FONT_NORMAL, BAR_TEXT_PAD + 12, 1, sTextColors, 0, sRegionNames[sWorldMap->region]);
        CopyWindowToVram(WIN_REGION_NAME, COPYWIN_FULL);
    }
    if (hint != sWorldMap->shownHint)
    {
        sWorldMap->shownHint = hint;
        FillWindowPixelBuffer(WIN_HINT, PIXEL_FILL(0));
        PrintRightAligned(WIN_HINT, sHintTexts[hint]);
        CopyWindowToVram(WIN_HINT, COPYWIN_FULL);
    }
}

// WIN0 = top bar, WIN1 = bottom bar; BG0 and OBJ are darkened inside them, BG1 text is not.
static void SetBarRegs(void)
{
    SetGpuReg(REG_OFFSET_WIN0H, WIN_RANGE(0, DISPLAY_WIDTH));
    SetGpuReg(REG_OFFSET_WIN0V, WIN_RANGE(0, BAR_HEIGHT));
    SetGpuReg(REG_OFFSET_WIN1H, WIN_RANGE(0, DISPLAY_WIDTH));
    SetGpuReg(REG_OFFSET_WIN1V, WIN_RANGE(DISPLAY_HEIGHT - BAR_HEIGHT, DISPLAY_HEIGHT));
    SetGpuReg(REG_OFFSET_WININ, WININ_WIN0_ALL | WININ_WIN1_ALL);
    SetGpuReg(REG_OFFSET_WINOUT, WINOUT_WIN01_BG_ALL | WINOUT_WIN01_OBJ);
    SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_BG0 | BLDCNT_TGT1_OBJ | BLDCNT_EFFECT_DARKEN);
    SetGpuReg(REG_OFFSET_BLDALPHA, 0);
    SetGpuReg(REG_OFFSET_BLDY, BAR_DARKEN);
}

static void ClearBarRegs(void)
{
    ClearGpuRegBits(REG_OFFSET_DISPCNT, DISPCNT_WIN0_ON | DISPCNT_WIN1_ON);
    SetGpuReg(REG_OFFSET_WIN0H, 0);
    SetGpuReg(REG_OFFSET_WIN0V, 0);
    SetGpuReg(REG_OFFSET_WIN1H, 0);
    SetGpuReg(REG_OFFSET_WIN1V, 0);
    SetGpuReg(REG_OFFSET_WININ, 0);
    SetGpuReg(REG_OFFSET_WINOUT, 0);
    SetGpuReg(REG_OFFSET_BLDCNT, 0);
    SetGpuReg(REG_OFFSET_BLDALPHA, 0);
    SetGpuReg(REG_OFFSET_BLDY, 0);
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
                FollowCursor(TRUE, FALSE);
            }
            else if (JOY_NEW(L_BUTTON))
            {
                SetCursorToRegion(NextUnlockedRegion(sWorldMap->region, -1));
                FollowCursor(TRUE, FALSE);
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

        EaseScroll();
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
        ClearBarRegs();
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

// Sprite data for SpriteCB_FlyRing
#define sCenterX    data[0]
#define sCenterY    data[1]
#define sIconMapSec data[2]
#define sFlicker    data[3]

static void SpriteCB_FlyRing(struct Sprite *sprite)
{
    sprite->x = sprite->sCenterX - sWorldMap->scrollX;
    sprite->y = sprite->sCenterY - sWorldMap->scrollY;

    // OAM coordinates wrap, so sprites outside the screen are hidden explicitly.
    if (IsOffScreen(sprite->x, sprite->y))
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

// Visited Fly points get a ring: bright when selectable, faint when in another region.
static void CreateFlyIcons(void)
{
    u32 i;

    for (i = 0; i < ARRAY_COUNT(sFlyLocations); i++)
    {
        mapsec_u16_t mapSec = sFlyLocations[i].mapsec;
        u32 width = gRegionMapEntries[mapSec].width;
        u32 height = gRegionMapEntries[mapSec].height;
        u32 spriteId, anim, region = GetMapSecGroup(mapSec) + 1;
        const struct SpriteTemplate *template;
        s32 x, y;
        bool32 found = FALSE;

        if (!FlagGet(sFlyLocations[i].flag))
            continue;

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

        if (mapSec == MAPSEC_BATTLE_FRONTIER)
            template = &sRingSpriteTemplate;
        else if (width >= 2)
            template = &sRingWideSpriteTemplate;
        else if (height >= 2)
            template = &sRingTallSpriteTemplate;
        else
            template = &sRingSpriteTemplate;

        spriteId = CreateSpriteUnchecked(template, 0, 0, 10);
        if (spriteId == MAX_SPRITES)
            continue;

        gSprites[spriteId].sCenterX = x * 8 + width * 4;
        gSprites[spriteId].sCenterY = y * 8 + height * 4;
        gSprites[spriteId].sIconMapSec = mapSec;
        gSprites[spriteId].sFlicker = 16;
        if (mapSec == MAPSEC_BATTLE_FRONTIER)
            anim = RING_ANIM_FRONTIER;
        else
            anim = GetMapSecGroup(mapSec) == sWorldMap->flyGroup ? RING_ANIM_BRIGHT : RING_ANIM_FAINT;
        StartSpriteAnim(&gSprites[spriteId], anim);
    }
}

static void DestroyFlyIcons(void)
{
    u32 i;

    for (i = 0; i < MAX_SPRITES; i++)
    {
        if (gSprites[i].inUse && gSprites[i].callback == SpriteCB_FlyRing)
            DestroySprite(&gSprites[i]);
    }
}

#undef sCenterX
#undef sCenterY
#undef sIconMapSec
#undef sFlicker
