#include "global.h"
#include "bg.h"
#include "decompress.h"
#include "event_data.h"
#include "gpu_regs.h"
#include "graphics.h"
#include "international_string_util.h"
#include "main.h"
#include "malloc.h"
#include "menu.h"
#include "overworld.h"
#include "palette.h"
#include "pokedex.h"
#include "pokedex_area_screen.h"
#include "regions.h"
#include "region_map.h"
#include "roamer.h"
#include "rtc.h"
#include "sound.h"
#include "string_util.h"
#include "text.h"
#include "text_window.h"
#include "trig.h"
#include "pokedex_area_region_map.h"
#include "wild_encounter.h"
#include "window.h"
#include "world_map.h"
#include "constants/region_map_sections.h"
#include "constants/rgb.h"
#include "constants/songs.h"
#include "config/pokedex_plus_hgss.h"

// There are two types of indicators for the area screen to show where a Pokémon can occur:
// - Area glows, which highlight any of the maps in MAP_GROUP_TOWNS_AND_ROUTES that have the species.
//   These are a tilemap with colored rectangular areas that blends in and out. The positions of the
//   rectangles is determined by the positions of the matching MAPSEC values on the world map layout.
// - Area markers, which highlight any of the maps in MAP_GROUP_DUNGEONS or MAP_GROUP_SPECIAL_AREA that
//   have the species. These are circular sprites that flash twice. The positions of the sprites is
//   the centre of the corresponding MAPSEC's cells on the world map.
// The map is the scrolling world map (Hoenn, Kanto, Johto, Sevii); regions the player has not
// reached are drawn grey and their encounters are not shown.

// Only maps in the following map groups have their encounters considered for the area screen
#define MAP_GROUP_TOWNS_AND_ROUTES MAP_GROUP(MAP_PETALBURG_CITY)
#define MAP_GROUP_TOWNS_AND_ROUTES_FRLG MAP_GROUP(MAP_PALLET_TOWN)
#define MAP_GROUP_DUNGEONS MAP_GROUP(MAP_METEOR_FALLS_1F_1R)
#define MAP_GROUP_DUNGEONS_FRLG MAP_GROUP(MAP_VIRIDIAN_FOREST)
#define MAP_GROUP_SPECIAL_AREA MAP_GROUP(MAP_SAFARI_ZONE_NORTHWEST)
#define MAP_GROUP_SPECIAL_AREA_FRLG MAP_GROUP(MAP_FIVE_ISLAND_LOST_CAVE_ENTRANCE)
#define MAP_GROUP_TOWNS_AND_ROUTES_JOHTO MAP_GROUP(MAP_NEW_BARK_TOWN)
#define MAP_GROUP_DUNGEONS_JOHTO MAP_GROUP(MAP_DARK_CAVE_SOUTH_SIDE)
#define MAP_GROUP_SPECIAL_AREA_JOHTO MAP_GROUP(MAP_SAFARI_ZONE_TOP_LEFT)

#define AREA_SCREEN_WIDTH WORLD_MAP_DEX_W
#define AREA_SCREEN_HEIGHT WORLD_MAP_DEX_H
#define GLOW_MAP_ENTRIES (64 * 64) // 64x64 tile BG, stored in screenblock order
#define GLOW_BASE_TILE 128 // First tile of area_glow.png in BG2's charblock; the world art uses the start

#define GLOW_FULL      0xFFFF
#define GLOW_EDGE_R    (1 << 0)
#define GLOW_EDGE_L    (1 << 1)
#define GLOW_EDGE_B    (1 << 2)
#define GLOW_EDGE_T    (1 << 3)
#define GLOW_CORNER_TL (1 << 4)
#define GLOW_CORNER_BL (1 << 5)
#define GLOW_CORNER_TR (1 << 6)
#define GLOW_CORNER_BR (1 << 7)

#define GLOW_PALETTE AREA_MAP_GLOW_PALETTE
#define PAN_SPEED 4
#define NO_GROUP 0xFF

#define TAG_AREA_MARKER 2
#define TAG_AREA_UNKNOWN 3
#define TAG_PLAYER_ICON 1

#define SPRITE_MARGIN 16

#define sFlashHidden data[0] // Marker sprites: hidden by the flash cycle

#define MAX_AREA_HIGHLIGHTS 192 // Maximum number of rectangular route highlights
#define MAX_AREA_MARKERS 64 // Maximum number of circular spot highlights

#define LABEL_WINDOW_BG 1
#define NUM_LABEL_WINDOWS 3

enum PokedexAreaLabels
{
    DEX_AREA_LABEL_TIME_OF_DAY,
    DEX_AREA_LABEL_AREA_UNKNOWN,
    DEX_AREA_LABEL_MAP_HINT
};

struct OverworldArea
{
    u8 mapGroup;
    u8 mapNum;
    mapsec_u16_t regionMapSectionId;
};

struct
{
    void (*callback)(void); // unused
    MainCallback prev; // unused
    MainCallback next; // unused
    u16 state; // unused
    enum Species species;
    struct OverworldArea overworldAreasWithMons[MAX_AREA_HIGHLIGHTS];
    u16 numOverworldAreas;
    u16 numSpecialAreas;
    u16 drawAreaGlowState;
    u16 areaGlowTilemap[GLOW_MAP_ENTRIES];
    u16 markerTimer;
    u16 glowTimer;
    u16 areaShadeBldArgLo;
    u16 areaShadeBldArgHi;
    bool8 showingMarkers;
    u8 markerFlashCounter;
    mapsec_u16_t specialAreaRegionMapSectionIds[MAX_AREA_MARKERS];
    struct Sprite *areaMarkerSprites[MAX_AREA_MARKERS];
    s16 areaMarkerX[MAX_AREA_MARKERS]; // World pixel position, same order as areaMarkerSprites
    s16 areaMarkerY[MAX_AREA_MARKERS];
    u16 numAreaMarkerSprites;
    u16 alteringCaveCounter;
    u16 alteringCaveId;
    u8 *screenSwitchState;
    struct Sprite *playerIconSprite;
    s16 playerIconX; // World pixel position
    s16 playerIconY;
    u8 charBuffer[64];
    struct Sprite *areaUnknownSprites[3];
    u8 areaUnknownGraphicsBuffer[0x600];
    u8 areaScreenLabelIds[NUM_LABEL_WINDOWS];
    u8 areaState;
} static EWRAM_DATA *sPokedexAreaScreen = NULL;

EWRAM_DATA u8 gAreaTimeOfDay = 0;

static void FindMapsWithMon(enum Species);
static void BuildAreaGlowTilemap(void);
static void SetAreaHasMon(u16, u16);
static void SetSpecialMapHasMon(u16, u16);
static mapsec_u16_t GetRegionMapSectionId(u8, u8);
static bool8 MapHasSpecies(const struct WildEncounterTypes *, u32, enum Species);
static bool8 MonListHasSpecies(const struct WildPokemonInfo *, enum Species, u16);
static void DoAreaGlow(void);
static void Task_ShowPokedexAreaScreen(u8 taskId);
static void Task_UpdatePokedexAreaScreen(u8 taskId);
static void CreateAreaMarkerSprites(void);
static void LoadAreaUnknownGraphics(void);
static void CreateAreaUnknownSprites(void);
static void Task_HandlePokedexAreaScreenInput(u8);
static void CreateAreaPlayerIcon(void);
static void UpdateAreaSprites(void);
static void FrameAreasOnMap(void);
static void ShowMapHintLabel(void);
static void DestroyAreaScreenSprites(void);
static void AddTimeOfDayLabels(void);
static void ShowEncounterInfoLabel(void);
static void ShowAreaUnknownLabel(void);
static void PrintAreaLabelText(const u8 *text, enum PokedexAreaLabels labelId, int textXPos);
static void ClearAreaWindowLabel(enum PokedexAreaLabels labelId);

bool32 ShouldShowAreaUnknownLabel(void);

static const u32 sAreaGlow_Pal[] = INCGFX_U32("graphics/pokedex/area_glow.png", ".gbapal");
static const u32 sAreaGlow_Gfx[] = INCGFX_U32("graphics/pokedex/area_glow.png", ".4bpp.smol");

static const u32 sPokedexPlusHGSS_ScreenSelectBarSubmenu_Tilemap[] = INCGFX_U32("graphics/pokedex/hgss/SelectBar.bin", ".smolTM");
static void LoadHGSSScreenSelectBarSubmenu(void);

static const enum Species sSpeciesHiddenFromAreaScreen[] = { SPECIES_WYNAUT };

static const mapsec_u16_t sMovingRegionMapSections[3] =
{
    MAPSEC_MARINE_CAVE,
    MAPSEC_UNDERWATER_MARINE_CAVE,
    MAPSEC_TERRA_CAVE
};

static const u16 sFeebasData[][3] =
{
    {SPECIES_FEEBAS, MAP_GROUP(MAP_ROUTE119), MAP_NUM(MAP_ROUTE119)},
    {NUM_SPECIES}
};

static const mapsec_u16_t sLandmarkData[][2] =
{
    {MAPSEC_SKY_PILLAR,       FLAG_LANDMARK_SKY_PILLAR},
    {MAPSEC_SEAFLOOR_CAVERN,  FLAG_LANDMARK_SEAFLOOR_CAVERN},
    {MAPSEC_ALTERING_CAVE,    FLAG_LANDMARK_ALTERING_CAVE},
    {MAPSEC_MIRAGE_TOWER,     FLAG_LANDMARK_MIRAGE_TOWER},
    {MAPSEC_DESERT_UNDERPASS, FLAG_LANDMARK_DESERT_UNDERPASS},
    {MAPSEC_ARTISAN_CAVE,     FLAG_LANDMARK_ARTISAN_CAVE},
    {MAPSEC_NONE}
};

#include "data/pokedex_area_glow.h"

static const u8 sAreaMarkerTiles[];
static const struct SpriteSheet sAreaMarkerSpriteSheet =
{
    .data = sAreaMarkerTiles, .size = 0x80, .tag = TAG_AREA_MARKER
};

static const u16 sAreaMarkerPalette[];
static const struct SpritePalette sAreaMarkerSpritePalette =
{
    .data = sAreaMarkerPalette, .tag = TAG_AREA_MARKER
};

static const struct OamData sAreaMarkerOamData =
{
    .shape = SPRITE_SHAPE(16x16),
    .size = SPRITE_SIZE(16x16),
    .priority = 1
};

static const struct SpriteTemplate sAreaMarkerSpriteTemplate =
{
    .tileTag = TAG_AREA_MARKER,
    .paletteTag = TAG_AREA_MARKER,
    .oam = &sAreaMarkerOamData,
};

static const u16 sAreaMarkerPalette[] = INCGFX_U16("graphics/pokedex/area_marker.png", ".gbapal");
static const u8 sAreaMarkerTiles[] = INCGFX_U8("graphics/pokedex/area_marker.png", ".4bpp");

static const struct SpritePalette sAreaUnknownSpritePalette =
{
    .data = gPokedexAreaScreenAreaUnknown_Pal, .tag = TAG_AREA_UNKNOWN
};

static const struct OamData sAreaUnknownOamData =
{
    .shape = SPRITE_SHAPE(32x32),
    .size = SPRITE_SIZE(32x32),
    .priority = 1
};

static const struct SpriteTemplate sAreaUnknownSpriteTemplate =
{
    .tileTag = TAG_AREA_UNKNOWN,
    .paletteTag = TAG_AREA_UNKNOWN,
    .oam = &sAreaUnknownOamData,
};

static const u8 sFontColor_AreaInfo[3] = {TEXT_COLOR_TRANSPARENT, TEXT_COLOR_WHITE, 5};
static const struct WindowTemplate sTimeOfDayWindowLabelTemplates[] =
{
    [DEX_AREA_LABEL_TIME_OF_DAY] =
    {
        .bg = LABEL_WINDOW_BG,
        .tilemapLeft = 22,
        .tilemapTop = 18,
        .width = 8,
        .height = 2,
        .paletteNum = 0,
        .baseBlock = 0x16C
    },

    [DEX_AREA_LABEL_AREA_UNKNOWN] =
    {
        .bg = LABEL_WINDOW_BG,
        .tilemapLeft = 12,
        .tilemapTop = 18,
        .width = 10,
        .height = 2,
        .paletteNum = 0,
        .baseBlock = 0x17C
    },
    [DEX_AREA_LABEL_MAP_HINT] =
    {
        .bg = LABEL_WINDOW_BG,
        .tilemapLeft = 0,
        .tilemapTop = 18,
        .width = 11,
        .height = 2,
        .paletteNum = 0,
        .baseBlock = 0x192
    }
};

static void ResetDrawAreaGlowState(void)
{
    sPokedexAreaScreen->drawAreaGlowState = 0;
}

static bool8 DrawAreaGlow(void)
{
    switch (sPokedexAreaScreen->drawAreaGlowState)
    {
    case 0:
        FindMapsWithMon(sPokedexAreaScreen->species);
        break;
    case 1:
        BuildAreaGlowTilemap();
        break;
    case 2:
        DecompressAndCopyTileDataToVram(2, sAreaGlow_Gfx, 0, GLOW_BASE_TILE, 0);
        LoadBgTilemap(2, sPokedexAreaScreen->areaGlowTilemap, sizeof(sPokedexAreaScreen->areaGlowTilemap), 0);
        break;
    case 3:
        if (!FreeTempTileDataBuffersIfPossible())
        {
            CpuCopy32(sAreaGlow_Pal, &gPlttBufferUnfaded[BG_PLTT_ID(GLOW_PALETTE)], sizeof(sAreaGlow_Pal));
            sPokedexAreaScreen->drawAreaGlowState++;
        }
        return TRUE;
    default:
        return FALSE;
    }

    sPokedexAreaScreen->drawAreaGlowState++;
    return TRUE;
}

static void FindMapsWithMon(enum Species species)
{
    u16 i;
    struct Roamer *roamer;

    sPokedexAreaScreen->alteringCaveCounter = 0;
    sPokedexAreaScreen->alteringCaveId = VarGet(VAR_ALTERING_CAVE_WILD_SET);
    if (sPokedexAreaScreen->alteringCaveId >= NUM_ALTERING_CAVE_TABLES)
        sPokedexAreaScreen->alteringCaveId = 0;

    sPokedexAreaScreen->numOverworldAreas = 0;
    sPokedexAreaScreen->numSpecialAreas = 0;

    // Check if this species should be hidden from the area map.
    // This only applies to Wynaut, to hide the encounters on Mirage Island.
    for (i = 0; i < ARRAY_COUNT(sSpeciesHiddenFromAreaScreen); i++)
    {
        if (sSpeciesHiddenFromAreaScreen[i] == species)
            return;
    }

    // Add Pokémon with special encounter circumstances (i.e. not listed
    // in the regular wild encounter table) to the area map.
    // This only applies to Feebas on Route 119, but it was clearly set
    // up to allow handling others.
    for (i = 0; sFeebasData[i][0] != NUM_SPECIES; i++)
    {
        if (species == sFeebasData[i][0])
        {
            switch (sFeebasData[i][1])
            {
            case MAP_GROUP_TOWNS_AND_ROUTES:
            case MAP_GROUP_TOWNS_AND_ROUTES_FRLG:
                SetAreaHasMon(sFeebasData[i][1], sFeebasData[i][2]);
                break;
            case MAP_GROUP_DUNGEONS:
            case MAP_GROUP_DUNGEONS_FRLG:
            case MAP_GROUP_SPECIAL_AREA:
            case MAP_GROUP_SPECIAL_AREA_FRLG:
                SetSpecialMapHasMon(sFeebasData[i][1], sFeebasData[i][2]);
                break;
            }
        }
    }

    // Add regular species to the area map
    for (i = 0; gWildMonHeaders[i].mapGroup != MAP_GROUP(MAP_UNDEFINED); i++)
    {
        u32 headerSectionId = Overworld_GetMapHeaderByGroupAndId(gWildMonHeaders[i].mapGroup, gWildMonHeaders[i].mapNum)->regionMapSectionId;

        if (!IsWorldMapSecUnlocked(headerSectionId))
            continue;

        if (MapHasSpecies(&gWildMonHeaders[i].encounterTypes[gAreaTimeOfDay], headerSectionId, species))
        {
            switch (gWildMonHeaders[i].mapGroup)
            {
            case MAP_GROUP_TOWNS_AND_ROUTES:
            case MAP_GROUP_TOWNS_AND_ROUTES_FRLG:
            case MAP_GROUP_TOWNS_AND_ROUTES_JOHTO:
                SetAreaHasMon(gWildMonHeaders[i].mapGroup, gWildMonHeaders[i].mapNum);
                break;
            case MAP_GROUP_DUNGEONS:
            case MAP_GROUP_DUNGEONS_FRLG:
            case MAP_GROUP_DUNGEONS_JOHTO:
            case MAP_GROUP_SPECIAL_AREA:
            case MAP_GROUP_SPECIAL_AREA_FRLG:
            case MAP_GROUP_SPECIAL_AREA_JOHTO:
                SetSpecialMapHasMon(gWildMonHeaders[i].mapGroup, gWildMonHeaders[i].mapNum);
                break;
            }
        }
    }

    // Add roamers to the area map
    for (i = 0; i < ROAMER_COUNT; i++)
    {
        roamer = &gSaveBlock1Ptr->roamer[i];
        if (species == roamer->species && roamer->active)
        {
            // This is a roamer's species, show where this roamer is currently
            struct OverworldArea *roamerLocation = &sPokedexAreaScreen->overworldAreasWithMons[sPokedexAreaScreen->numOverworldAreas];
            GetRoamerLocation(i, &roamerLocation->mapGroup, &roamerLocation->mapNum);
            roamerLocation->regionMapSectionId = Overworld_GetMapHeaderByGroupAndId(roamerLocation->mapGroup, roamerLocation->mapNum)->regionMapSectionId;
            if (!IsWorldMapSecUnlocked(roamerLocation->regionMapSectionId))
                continue;
            sPokedexAreaScreen->numOverworldAreas++;
        }
    }
}

static void SetAreaHasMon(u16 mapGroup, u16 mapNum)
{
    if (sPokedexAreaScreen->numOverworldAreas < MAX_AREA_HIGHLIGHTS)
    {
        sPokedexAreaScreen->overworldAreasWithMons[sPokedexAreaScreen->numOverworldAreas].mapGroup = mapGroup;
        sPokedexAreaScreen->overworldAreasWithMons[sPokedexAreaScreen->numOverworldAreas].mapNum = mapNum;
        sPokedexAreaScreen->overworldAreasWithMons[sPokedexAreaScreen->numOverworldAreas].regionMapSectionId = CorrectSpecialMapSecId(Overworld_GetMapHeaderByGroupAndId(mapGroup, mapNum)->regionMapSectionId);
        sPokedexAreaScreen->numOverworldAreas++;
    }
}

static void SetSpecialMapHasMon(u16 mapGroup, u16 mapNum)
{
    int i;

    if (sPokedexAreaScreen->numSpecialAreas < MAX_AREA_MARKERS)
    {
        mapsec_u16_t regionMapSectionId = GetRegionMapSectionId(mapGroup, mapNum);
        if (regionMapSectionId < MAPSEC_NONE)
        {
            // Don't highlight the area if it's a moving area (Marine/Terra Cave)
            for (i = 0; i < ARRAY_COUNT(sMovingRegionMapSections); i++)
            {
                if (regionMapSectionId == sMovingRegionMapSections[i])
                    return;
            }

            // Don't highlight the area if it's an undiscovered landmark (e.g. Sky Pillar)
            for (i = 0; sLandmarkData[i][0] != MAPSEC_NONE; i++)
            {
                if (regionMapSectionId == sLandmarkData[i][0] && !FlagGet(sLandmarkData[i][1]))
                    return;
            }

            // Check if this special area is already being tracked
            for (i = 0; i < sPokedexAreaScreen->numSpecialAreas; i++)
            {
                if (sPokedexAreaScreen->specialAreaRegionMapSectionIds[i] == regionMapSectionId)
                    break;
            }

            if (i == sPokedexAreaScreen->numSpecialAreas)
            {
                // New special area
                sPokedexAreaScreen->specialAreaRegionMapSectionIds[i] = regionMapSectionId;
                sPokedexAreaScreen->numSpecialAreas++;
            }
        }
    }
}

static mapsec_u16_t GetRegionMapSectionId(u8 mapGroup, u8 mapNum)
{
    return Overworld_GetMapHeaderByGroupAndId(mapGroup, mapNum)->regionMapSectionId;
}

static bool8 MapHasSpecies(const struct WildEncounterTypes *info, u32 headerSectionId, enum Species species)
{
    // If this is a header for Altering Cave, skip it if it's not the current Altering Cave encounter set
    if (headerSectionId == MAPSEC_ALTERING_CAVE)
    {
        sPokedexAreaScreen->alteringCaveCounter++;
        if (sPokedexAreaScreen->alteringCaveCounter != sPokedexAreaScreen->alteringCaveId + 1)
            return FALSE;
    }

    if (MonListHasSpecies(info->landMonsInfo, species, NUM_LAND_MONS_ENCOUNTER_SLOTS))
        return TRUE;
    if (MonListHasSpecies(info->waterMonsInfo, species, NUM_WATER_MONS_ENCOUNTER_SLOTS))
        return TRUE;
// When searching the fishing encounters, this incorrectly uses the size of the land encounters.
// As a result it's reading out of bounds of the fishing encounters tables.
#ifdef BUGFIX
    if (MonListHasSpecies(info->fishingMonsInfo, species, NUM_FISHING_MONS_ENCOUNTER_SLOTS))
#else
    if (MonListHasSpecies(info->fishingMonsInfo, species, NUM_LAND_MONS_ENCOUNTER_SLOTS))
#endif
        return TRUE;
    if (MonListHasSpecies(info->rockSmashMonsInfo, species, NUM_ROCK_SMASH_MONS_ENCOUNTER_SLOTS))
        return TRUE;
    return FALSE;
}

static bool8 MonListHasSpecies(const struct WildPokemonInfo *info, enum Species species, u16 size)
{
    u16 i;
    if (info != NULL)
    {
        for (i = 0; i < size; i++)
        {
            if (info->wildPokemon[i].species == species)
                return TRUE;
        }
    }
    return FALSE;
}

// Index of a cell in the 64x64 BG tilemap, which is stored as four 32x32 screenblocks.
static u32 GlowIndex(u32 x, u32 y)
{
    return ((y >> 5) << 11) | ((x >> 5) << 10) | ((y & 31) << 5) | (x & 31);
}

static void BuildAreaGlowTilemap(void)
{
    u16 *tilemap = sPokedexAreaScreen->areaGlowTilemap;
    u32 areaBits[256 / 32] = {0};
    u32 i, y, x;

    // Reset tilemap
    for (i = 0; i < GLOW_MAP_ENTRIES; i++)
        tilemap[i] = 0;

    for (i = 0; i < sPokedexAreaScreen->numOverworldAreas; i++)
    {
        mapsec_u16_t mapSec = sPokedexAreaScreen->overworldAreasWithMons[i].regionMapSectionId;

        if (mapSec < 256)
            areaBits[mapSec / 32] |= 1u << (mapSec % 32);
    }

    // For each world cell whose MAPSEC has this species, add a "full glow" indicator.
    for (y = 0; y < AREA_SCREEN_HEIGHT; y++)
    {
        for (x = 0; x < AREA_SCREEN_WIDTH; x++)
        {
            mapsec_u16_t mapSec = GetWorldMapSecIdAt(x, y);

            if (mapSec < 256 && (areaBits[mapSec / 32] & (1u << (mapSec % 32))))
                tilemap[GlowIndex(x, y)] = GLOW_FULL;
        }
    }

    // Scan the tilemap. For every "full glow" indicator added above, fill in its edges and corners.
    for (y = 0; y < AREA_SCREEN_HEIGHT; y++)
    {
        for (x = 0; x < AREA_SCREEN_WIDTH; x++)
        {
            if (tilemap[GlowIndex(x, y)] != GLOW_FULL)
                continue;
            // Edges
            if (x != 0 && tilemap[GlowIndex(x - 1, y)] != GLOW_FULL)
                tilemap[GlowIndex(x - 1, y)] |= GLOW_EDGE_L;
            if (x != AREA_SCREEN_WIDTH - 1 && tilemap[GlowIndex(x + 1, y)] != GLOW_FULL)
                tilemap[GlowIndex(x + 1, y)] |= GLOW_EDGE_R;
            if (y != 0 && tilemap[GlowIndex(x, y - 1)] != GLOW_FULL)
                tilemap[GlowIndex(x, y - 1)] |= GLOW_EDGE_T;
            if (y != AREA_SCREEN_HEIGHT - 1 && tilemap[GlowIndex(x, y + 1)] != GLOW_FULL)
                tilemap[GlowIndex(x, y + 1)] |= GLOW_EDGE_B;
            // Corners
            if (x != 0 && y != 0 && tilemap[GlowIndex(x - 1, y - 1)] != GLOW_FULL)
                tilemap[GlowIndex(x - 1, y - 1)] |= GLOW_CORNER_TL;
            if (x != AREA_SCREEN_WIDTH - 1 && y != 0 && tilemap[GlowIndex(x + 1, y - 1)] != GLOW_FULL)
                tilemap[GlowIndex(x + 1, y - 1)] |= GLOW_CORNER_TR;
            if (x != 0 && y != AREA_SCREEN_HEIGHT - 1 && tilemap[GlowIndex(x - 1, y + 1)] != GLOW_FULL)
                tilemap[GlowIndex(x - 1, y + 1)] |= GLOW_CORNER_BL;
            if (x != AREA_SCREEN_WIDTH - 1 && y != AREA_SCREEN_HEIGHT - 1 && tilemap[GlowIndex(x + 1, y + 1)] != GLOW_FULL)
                tilemap[GlowIndex(x + 1, y + 1)] |= GLOW_CORNER_BR;
        }
    }

    // Scan the tilemap again. Replace the flags with the actual tile id (offset to where
    // area_glow.png is loaded), and remove corner flags when they're overlapped by an edge.
    // Cells without a glow point at the empty tile, as tile 0 of this charblock is not blank.
    for (i = 0; i < GLOW_MAP_ENTRIES; i++)
    {
        if (tilemap[i] == GLOW_FULL)
        {
            tilemap[i] = GLOW_BASE_TILE + GLOW_TILE_FULL;
            tilemap[i] |= (GLOW_PALETTE << 12);
        }
        else if (tilemap[i])
        {
            // Get rid of overlapping flags.
            // This is pointless, as sAreaGlowTilemapMapping can handle overlaps.
            if (tilemap[i] & GLOW_EDGE_L)
                tilemap[i] &= ~(GLOW_CORNER_TL | GLOW_CORNER_BL);
            if (tilemap[i] & GLOW_EDGE_R)
                tilemap[i] &= ~(GLOW_CORNER_TR | GLOW_CORNER_BR);
            if (tilemap[i] & GLOW_EDGE_T)
                tilemap[i] &= ~(GLOW_CORNER_TR | GLOW_CORNER_TL);
            if (tilemap[i] & GLOW_EDGE_B)
                tilemap[i] &= ~(GLOW_CORNER_BR | GLOW_CORNER_BL);
            // Assign tile id
            tilemap[i] = GLOW_BASE_TILE + sAreaGlowTilemapMapping[tilemap[i]];
            tilemap[i] |= (GLOW_PALETTE << 12);
        }
        else
        {
            tilemap[i] = GLOW_BASE_TILE + GLOW_TILE_EMPTY;
        }
    }
}

static void StartAreaGlow(void)
{
    if (sPokedexAreaScreen->numSpecialAreas && sPokedexAreaScreen->numOverworldAreas == 0)
        sPokedexAreaScreen->showingMarkers = TRUE;
    else
        sPokedexAreaScreen->showingMarkers = FALSE;

    sPokedexAreaScreen->markerTimer = 0;
    sPokedexAreaScreen->glowTimer = 0;
    sPokedexAreaScreen->areaShadeBldArgLo = 0;
    sPokedexAreaScreen->areaShadeBldArgHi = 64;
    sPokedexAreaScreen->markerFlashCounter = 1;
    SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_BG2 | BLDCNT_EFFECT_BLEND | BLDCNT_TGT2_ALL);
    SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(0, 16));
    DoAreaGlow();
}

static void DoAreaGlow(void)
{
    u16 x, y;
    u16 i;

    if (!sPokedexAreaScreen->showingMarkers)
    {
        // Showing area glow
        if (sPokedexAreaScreen->markerTimer == 0)
        {
            sPokedexAreaScreen->glowTimer++;
            if (sPokedexAreaScreen->glowTimer & 1)
                sPokedexAreaScreen->areaShadeBldArgLo = (sPokedexAreaScreen->areaShadeBldArgLo + 4) & 0x7f;
            else
                sPokedexAreaScreen->areaShadeBldArgHi = (sPokedexAreaScreen->areaShadeBldArgHi + 4) & 0x7f;

            x = gSineTable[sPokedexAreaScreen->areaShadeBldArgLo] >> 4;
            y = gSineTable[sPokedexAreaScreen->areaShadeBldArgHi] >> 4;
            SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(x, y));
            sPokedexAreaScreen->markerTimer = 0;
            if (sPokedexAreaScreen->glowTimer == 64)
            {
                // Done glowing, reset and try to switch to the special area markers
                sPokedexAreaScreen->glowTimer = 0;
                if (sPokedexAreaScreen->numSpecialAreas != 0)
                    sPokedexAreaScreen->showingMarkers = TRUE;
            }
        }
        else
            sPokedexAreaScreen->markerTimer--;
    }
    else
    {
        // Showing special area markers
        sPokedexAreaScreen->markerTimer++;
        if (sPokedexAreaScreen->markerTimer > 12)
        {
            sPokedexAreaScreen->markerTimer = 0;

            // Flash the marker
            // With a max of 4, the marker will disappear twice
            sPokedexAreaScreen->markerFlashCounter++;
            for (i = 0; i < sPokedexAreaScreen->numAreaMarkerSprites; i++)
                sPokedexAreaScreen->areaMarkerSprites[i]->sFlashHidden = sPokedexAreaScreen->markerFlashCounter & 1;

            if (sPokedexAreaScreen->markerFlashCounter > 4)
            {
                // Done flashing, reset and try to switch to the area glow
                sPokedexAreaScreen->markerFlashCounter = 1;
                if (sPokedexAreaScreen->numOverworldAreas != 0)
                    sPokedexAreaScreen->showingMarkers = FALSE;
            }
        }
    }
}

static const u8 *GetTimeOfDayTextWithButton(enum TimeOfDay timeOfDay)
{
    static const u8 gText_Morning[] = _("{DPAD_UPDOWN} MORNING");
    static const u8 gText_Day[] = _("{DPAD_UPDOWN} DAY");
    static const u8 gText_Evening[] = _("{DPAD_UPDOWN} EVENING");
    static const u8 gText_Night[] = _("{DPAD_UPDOWN} NIGHT");

    switch (gAreaTimeOfDay)
    {
    case TIME_MORNING:
        return gText_Morning;
    case TIME_EVENING:
        return gText_Evening;
    case TIME_NIGHT:
        return gText_Night;
    case TIME_DAY:
    default:
        return gText_Day;
    }
}

static void AddTimeOfDayLabels(void)
{
    u32 i;

    // clear the background before adding any more windows
    RemoveAllWindowsOnBg(LABEL_WINDOW_BG);

    for (i = 0; i < NUM_LABEL_WINDOWS; i ++)
    {
        sPokedexAreaScreen->areaScreenLabelIds[i] = AddWindow(&sTimeOfDayWindowLabelTemplates[i]);
        FillWindowPixelBuffer(sPokedexAreaScreen->areaScreenLabelIds[i], PIXEL_FILL(0));
    }
}

static void ShowEncounterInfoLabel(void)
{
    const u8 *gText_TimeOfDay = GetTimeOfDayTextWithButton(gAreaTimeOfDay);
    int stringXPos = GetStringCenterAlignXOffset(FONT_NORMAL, gText_TimeOfDay, 64);

    PrintAreaLabelText(gText_TimeOfDay, DEX_AREA_LABEL_TIME_OF_DAY, stringXPos);
}

static void ShowAreaUnknownLabel(void)
{
    static const u8 gText_AreaUnknown[] = _("AREA UNKNOWN");
    int stringXPos = GetStringCenterAlignXOffset(FONT_NORMAL, gText_AreaUnknown, 80);

    PrintAreaLabelText(gText_AreaUnknown, DEX_AREA_LABEL_AREA_UNKNOWN, stringXPos);
}

static void ShowMapHintLabel(void)
{
    static const u8 gText_PanMap[] = _("{A_BUTTON}+{DPAD_NONE} MAP");
    int stringXPos = GetStringCenterAlignXOffset(FONT_NORMAL, gText_PanMap, 88);

    PrintAreaLabelText(gText_PanMap, DEX_AREA_LABEL_MAP_HINT, stringXPos);
}

static void ClearAreaWindowLabel(enum PokedexAreaLabels labelId)
{
    FillWindowPixelBuffer(sPokedexAreaScreen->areaScreenLabelIds[labelId], PIXEL_FILL(0));
    ClearWindowTilemap(sPokedexAreaScreen->areaScreenLabelIds[labelId]);
    ScheduleBgCopyTilemapToVram(0);
}

static void PrintAreaLabelText(const u8 *text, enum PokedexAreaLabels labelId, int textXPos)
{
    ClearAreaWindowLabel(labelId);

    PutWindowTilemap(sPokedexAreaScreen->areaScreenLabelIds[labelId]);
    FillWindowPixelBuffer(sPokedexAreaScreen->areaScreenLabelIds[labelId], PIXEL_FILL(7));

    AddTextPrinterParameterized4(sPokedexAreaScreen->areaScreenLabelIds[labelId], FONT_NORMAL, textXPos, 0, 0, 0, sFontColor_AreaInfo, TEXT_SKIP_DRAW, text);
    CopyWindowToVram(sPokedexAreaScreen->areaScreenLabelIds[labelId], COPYWIN_FULL);
}

bool32 ShouldShowAreaUnknownLabel(void)
{
    return !sPokedexAreaScreen->numOverworldAreas && !sPokedexAreaScreen->numSpecialAreas;
}

#define tState data[0]

void DisplayPokedexAreaScreen(enum Species species, u8 *screenSwitchState, enum TimeOfDay timeOfDay, enum PokedexAreaScreenState areaState)
{
    u8 taskId;

    sPokedexAreaScreen = AllocZeroed(sizeof(*sPokedexAreaScreen));
    sPokedexAreaScreen->species = species;
    sPokedexAreaScreen->screenSwitchState = screenSwitchState;
    sPokedexAreaScreen->areaState = areaState;
    gAreaTimeOfDay = timeOfDay;
    screenSwitchState[0] = 0;

    if (sPokedexAreaScreen->areaState == DEX_UPDATE_AREA_SCREEN)
        taskId = CreateTask(Task_UpdatePokedexAreaScreen, 0);
    else
        taskId = CreateTask(Task_ShowPokedexAreaScreen, 0);

    gTasks[taskId].tState = 0;
}

static void Task_ShowPokedexAreaScreen(u8 taskId)
{
    switch (gTasks[taskId].tState)
    {
    case 0:
        ResetSpriteData();
        FreeAllSpritePalettes();
        HideBg(3);
        HideBg(2);
        HideBg(0);
        break;
    case 1:
        LoadPokedexAreaMapGfx();
        StringFill(sPokedexAreaScreen->charBuffer, CHAR_SPACE, 16);
        break;
    case 2:
        if (TryShowPokedexAreaMap() == TRUE)
            return;
        break;
    case 3:
        ResetDrawAreaGlowState();
        break;
    case 4:
        if (DrawAreaGlow())
            return;
        break;
    case 5:
        CreateAreaPlayerIcon();
        FrameAreasOnMap();
        break;
    case 6:
        CreateAreaMarkerSprites();
        UpdateAreaSprites();
        break;
    case 7:
        if (!OW_TIME_OF_DAY_ENCOUNTERS)
            LoadAreaUnknownGraphics();
        break;
    case 8:
        if (!OW_TIME_OF_DAY_ENCOUNTERS)
            CreateAreaUnknownSprites();
        break;
    case 9:
        BeginNormalPaletteFade(PALETTES_ALL & ~(0x14), 0, 16, 0, RGB_BLACK);
        break;
    case 10:
        SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_BG0 | BLDCNT_EFFECT_BLEND | BLDCNT_TGT2_BG0 | BLDCNT_TGT2_ALL);
        StartAreaGlow();
        if (OW_TIME_OF_DAY_ENCOUNTERS)
        {
            AddTimeOfDayLabels();
            ShowEncounterInfoLabel();
            ShowMapHintLabel();
            if (ShouldShowAreaUnknownLabel())
                ShowAreaUnknownLabel();
            DoScheduledBgTilemapCopiesToVram();
        }
        if (POKEDEX_PLUS_HGSS)
            LoadHGSSScreenSelectBarSubmenu();
        ShowBg(2);
        ShowBg(3); // TryShowPokedexAreaMap will have done this already
        SetGpuRegBits(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON);
        break;
    case 11:
        gTasks[taskId].func = Task_HandlePokedexAreaScreenInput;
        gTasks[taskId].tState = 0;
        return;
    }

    gTasks[taskId].tState++;
}

static void Task_UpdatePokedexAreaScreen(u8 taskId)
{
    switch (gTasks[taskId].tState)
    {
    case 0:
        ClearAreaWindowLabel(DEX_AREA_LABEL_TIME_OF_DAY);
        ClearAreaWindowLabel(DEX_AREA_LABEL_AREA_UNKNOWN);
        ResetSpriteData();
        FreeAllSpritePalettes();
        ResetDrawAreaGlowState();
        HideBg(2);
        HideBg(0);
        break;
    case 1:
        SetUpPokedexAreaMapBgs();
        StringFill(sPokedexAreaScreen->charBuffer, CHAR_SPACE, 16);
        break;
    case 2:
        if (TryShowPokedexAreaMap() == TRUE)
            return;
        break;
    case 3:
        if (DrawAreaGlow())
            return;
        break;
    case 4:
        CreateAreaPlayerIcon();
        CreateAreaMarkerSprites();
        UpdateAreaSprites();
        break;
    case 5:
        SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_BG0 | BLDCNT_EFFECT_BLEND | BLDCNT_TGT2_BG0 | BLDCNT_TGT2_ALL);
        StartAreaGlow();
        AddTimeOfDayLabels();
        ShowEncounterInfoLabel();
        ShowMapHintLabel();
        if (ShouldShowAreaUnknownLabel())
            ShowAreaUnknownLabel();
        ShowBg(2);
        SetGpuRegBits(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON);
        break;
    case 6:
        gTasks[taskId].func = Task_HandlePokedexAreaScreenInput;
        gTasks[taskId].tState = 0;
        return;
    }

    gTasks[taskId].tState++;
}

static void Task_HandlePokedexAreaScreenInput(u8 taskId)
{
    DoAreaGlow();
    PokedexAreaMapUpdateScroll();
    UpdateAreaSprites();
    switch (gTasks[taskId].tState)
    {
    default:
        gTasks[taskId].tState = 0;
        // fall through
    case 0:
        if (gPaletteFade.active)
            return;
        break;
    case 1:
        if (JOY_HELD(A_BUTTON))
        {
            // Holding A turns the D-pad into map panning
            s32 dx = 0, dy = 0;

            if (JOY_HELD(DPAD_LEFT))
                dx -= PAN_SPEED;
            if (JOY_HELD(DPAD_RIGHT))
                dx += PAN_SPEED;
            if (JOY_HELD(DPAD_UP))
                dy -= PAN_SPEED;
            if (JOY_HELD(DPAD_DOWN))
                dy += PAN_SPEED;
            PokedexAreaMapPan(dx, dy);
            sPokedexAreaScreen->areaState = DEX_SHOW_AREA_SCREEN;
            return;
        }
        else if (JOY_NEW(B_BUTTON))
        {
            gTasks[taskId].data[1] = 1;
            PlaySE(SE_DEX_PAGE);
        }
        else if (JOY_NEW(DPAD_LEFT) || (JOY_NEW(L_BUTTON) && gSaveBlock2Ptr->optionsButtonMode == OPTIONS_BUTTON_MODE_LR))
        {
            gTasks[taskId].data[1] = 1;
            PlaySE(SE_DEX_PAGE);
        }
        else if (JOY_NEW(DPAD_RIGHT) || (JOY_NEW(R_BUTTON) && gSaveBlock2Ptr->optionsButtonMode == OPTIONS_BUTTON_MODE_LR))
        {
            if (!GetSetPokedexFlagBySpecies(sPokedexAreaScreen->species, FLAG_GET_CAUGHT))
            {
                PlaySE(SE_FAILURE);
                return;
            }
            gTasks[taskId].data[1] = 2;
            PlaySE(SE_DEX_PAGE);
        }
        else if (JOY_NEW(DPAD_UP) && OW_TIME_OF_DAY_ENCOUNTERS == TRUE)
        {
            gTasks[taskId].data[1] = 3;
            gAreaTimeOfDay = TryDecrementTimeOfDay(gAreaTimeOfDay);
            sPokedexAreaScreen->areaState = DEX_UPDATE_AREA_SCREEN;
            PlaySE(SE_DEX_PAGE);
        }
        else if (JOY_NEW(DPAD_DOWN) && OW_TIME_OF_DAY_ENCOUNTERS == TRUE)
        {
            gTasks[taskId].data[1] = 3;
            gAreaTimeOfDay = TryIncrementTimeOfDay(gAreaTimeOfDay);
            sPokedexAreaScreen->areaState = DEX_UPDATE_AREA_SCREEN;
            PlaySE(SE_DEX_PAGE);
        }
        else
        {
            // screen needs to fade if its doing anything except updating the area screen
            sPokedexAreaScreen->areaState = DEX_SHOW_AREA_SCREEN;
            return;
        }
        break;
    case 2:
        if (sPokedexAreaScreen->areaState != DEX_UPDATE_AREA_SCREEN)
            BeginNormalPaletteFade(PALETTES_ALL & ~(0x14), 0, 0, 16, RGB_BLACK);
        break;
    case 3:
        if (gPaletteFade.active)
            return;
        DestroyAreaScreenSprites();
        if (OW_TIME_OF_DAY_ENCOUNTERS)
        {
            ClearAreaWindowLabel(DEX_AREA_LABEL_TIME_OF_DAY);
            ClearAreaWindowLabel(DEX_AREA_LABEL_AREA_UNKNOWN);
            ClearAreaWindowLabel(DEX_AREA_LABEL_MAP_HINT);
            RemoveAllWindowsOnBg(LABEL_WINDOW_BG);
        }

        sPokedexAreaScreen->screenSwitchState[0] = gTasks[taskId].data[1];
        ResetPokedexAreaMapBg();
        DestroyTask(taskId);
        FREE_AND_SET_NULL(sPokedexAreaScreen);
        return;
    }

    gTasks[taskId].tState++;
}

// Creates the circular sprites to highlight special areas (like caves) where a Pokémon can be found.
// They stay hidden until the marker flash starts; UpdateAreaSprites positions them on the scrolling map.
static void CreateAreaMarkerSprites(void)
{
    u8 spriteId;
    s32 x;
    s32 y;
    s16 i;
    mapsec_u16_t mapSecId;
    s16 numSprites;

    LoadSpriteSheet(&sAreaMarkerSpriteSheet);
    LoadSpritePalette(&sAreaMarkerSpritePalette);

    numSprites = 0;
    for (i = 0; i < sPokedexAreaScreen->numSpecialAreas; i++)
    {
        mapSecId = sPokedexAreaScreen->specialAreaRegionMapSectionIds[i];
        if (!GetWorldMapSecCenter(mapSecId, &x, &y))
            continue;
        spriteId = CreateSpriteUnchecked(&sAreaMarkerSpriteTemplate, x, y, 0);
        if (spriteId != MAX_SPRITES)
        {
            gSprites[spriteId].invisible = TRUE;
            gSprites[spriteId].sFlashHidden = TRUE;
            sPokedexAreaScreen->areaMarkerX[numSprites] = x;
            sPokedexAreaScreen->areaMarkerY[numSprites] = y;
            sPokedexAreaScreen->areaMarkerSprites[numSprites++] = &gSprites[spriteId];
        }
    }
    sPokedexAreaScreen->numAreaMarkerSprites = numSprites;
}

static void CreateAreaPlayerIcon(void)
{
    s32 x, y;

    sPokedexAreaScreen->playerIconSprite = NULL;
    if (!GetWorldMapPlayerPos(&x, &y))
        return;
    sPokedexAreaScreen->playerIconSprite = CreatePlayerIconSprite(TAG_PLAYER_ICON, TAG_PLAYER_ICON);
    // Keep the icon above the glow
    sPokedexAreaScreen->playerIconSprite->oam.priority = 1;
    sPokedexAreaScreen->playerIconX = x;
    sPokedexAreaScreen->playerIconY = y;
}

// Sprites live in world pixels; hide them when they leave the screen, since OAM coordinates wrap.
static bool32 SetSpriteScreenPos(struct Sprite *sprite, s32 worldX, s32 worldY, s32 scrollX, s32 scrollY)
{
    s32 x = worldX - scrollX;
    s32 y = worldY - scrollY;

    sprite->x = x;
    sprite->y = y;
    return x < -SPRITE_MARGIN || x > DISPLAY_WIDTH + SPRITE_MARGIN || y < -SPRITE_MARGIN || y > DISPLAY_HEIGHT + SPRITE_MARGIN;
}

static void UpdateAreaSprites(void)
{
    s32 scrollX, scrollY;
    u32 i;

    PokedexAreaMapGetScroll(&scrollX, &scrollY);
    if (sPokedexAreaScreen->playerIconSprite != NULL)
    {
        sPokedexAreaScreen->playerIconSprite->invisible = SetSpriteScreenPos(sPokedexAreaScreen->playerIconSprite,
            sPokedexAreaScreen->playerIconX, sPokedexAreaScreen->playerIconY, scrollX, scrollY);
    }
    for (i = 0; i < sPokedexAreaScreen->numAreaMarkerSprites; i++)
    {
        struct Sprite *marker = sPokedexAreaScreen->areaMarkerSprites[i];
        bool32 offScreen = SetSpriteScreenPos(marker, sPokedexAreaScreen->areaMarkerX[i], sPokedexAreaScreen->areaMarkerY[i], scrollX, scrollY);

        marker->invisible = offScreen || marker->sFlashHidden;
    }
}

static mapsec_u16_t GetAreaMapSec(u32 i)
{
    if (i < sPokedexAreaScreen->numOverworldAreas)
        return sPokedexAreaScreen->overworldAreasWithMons[i].regionMapSectionId;
    return sPokedexAreaScreen->specialAreaRegionMapSectionIds[i - sPokedexAreaScreen->numOverworldAreas];
}

// Centres the camera on the species' areas in one region: the player's region if the species
// occurs there, else the region of the first area. With no areas it centres on the player.
static void FrameAreasOnMap(void)
{
    u32 total = sPokedexAreaScreen->numOverworldAreas + sPokedexAreaScreen->numSpecialAreas;
    u32 playerGroup = GetWorldMapSecGroup(gMapHeader.regionMapSectionId);
    u32 group = NO_GROUP;
    s32 x, y, minX = 0x7FFF, minY = 0x7FFF, maxX = -1, maxY = -1;
    u32 i;

    for (i = 0; i < total; i++)
    {
        u32 areaGroup = GetWorldMapSecGroup(GetAreaMapSec(i));

        if (areaGroup == playerGroup)
        {
            group = areaGroup;
            break;
        }
        if (group == NO_GROUP)
            group = areaGroup;
    }

    for (i = 0; i < total && group != NO_GROUP; i++)
    {
        mapsec_u16_t mapSec = GetAreaMapSec(i);

        if (GetWorldMapSecGroup(mapSec) != group || !GetWorldMapSecCenter(mapSec, &x, &y))
            continue;
        if (x < minX)
            minX = x;
        if (x > maxX)
            maxX = x;
        if (y < minY)
            minY = y;
        if (y > maxY)
            maxY = y;
    }

    if (maxX >= 0)
        PokedexAreaMapCenterOn((minX + maxX) / 2, (minY + maxY) / 2, TRUE);
    else if (GetWorldMapPlayerPos(&x, &y))
        PokedexAreaMapCenterOn(x, y, TRUE);
    else
    {
        GetWorldMapGroupAnchor(playerGroup, &x, &y);
        PokedexAreaMapCenterOn(x, y, TRUE);
    }
}

static void DestroyAreaScreenSprites(void)
{
    u16 i;

    // Destroy area marker sprites
    FreeSpriteTilesByTag(TAG_AREA_MARKER);
    FreeSpritePaletteByTag(TAG_AREA_MARKER);
    for (i = 0; i < sPokedexAreaScreen->numAreaMarkerSprites; i++)
        DestroySprite(sPokedexAreaScreen->areaMarkerSprites[i]);

    // Destroy player icon
    if (sPokedexAreaScreen->playerIconSprite != NULL)
    {
        FreeSpriteTilesByTag(TAG_PLAYER_ICON);
        FreeSpritePaletteByTag(TAG_PLAYER_ICON);
        DestroySprite(sPokedexAreaScreen->playerIconSprite);
        sPokedexAreaScreen->playerIconSprite = NULL;
    }

    if (!OW_TIME_OF_DAY_ENCOUNTERS)
    {
        // Destroy "Area Unknown" sprites
        FreeSpriteTilesByTag(TAG_AREA_UNKNOWN);
        FreeSpritePaletteByTag(TAG_AREA_UNKNOWN);
        for (i = 0; i < ARRAY_COUNT(sPokedexAreaScreen->areaUnknownSprites); i++)
        {
            if (sPokedexAreaScreen->areaUnknownSprites[i])
                DestroySprite(sPokedexAreaScreen->areaUnknownSprites[i]);
        }
    }
}

static void LoadAreaUnknownGraphics(void)
{
    struct SpriteSheet spriteSheet = {
        .data = sPokedexAreaScreen->areaUnknownGraphicsBuffer,
        .size = sizeof(sPokedexAreaScreen->areaUnknownGraphicsBuffer),
        .tag = TAG_AREA_UNKNOWN,
    };
    DecompressDataWithHeaderWram(gPokedexAreaScreenAreaUnknown_Gfx, sPokedexAreaScreen->areaUnknownGraphicsBuffer);
    LoadSpriteSheet(&spriteSheet);
    LoadSpritePalette(&sAreaUnknownSpritePalette);
}

static void CreateAreaUnknownSprites(void)
{
    u16 i;

    if (sPokedexAreaScreen->numOverworldAreas || sPokedexAreaScreen->numSpecialAreas)
    {
        // The current species is present on the map, don't create any "Area Unknown" sprites
        for (i = 0; i < ARRAY_COUNT(sPokedexAreaScreen->areaUnknownSprites); i++)
            sPokedexAreaScreen->areaUnknownSprites[i] = NULL;
    }
    else
    {
        // The current species is absent on the map, try to create "Area Unknown" sprites
        for (i = 0; i < ARRAY_COUNT(sPokedexAreaScreen->areaUnknownSprites); i++)
        {
            u8 spriteId = CreateSpriteUnchecked(&sAreaUnknownSpriteTemplate, i * 32 + 160, 140, 0);
            if (spriteId != MAX_SPRITES)
            {
                gSprites[spriteId].oam.tileNum += i * 16;
                sPokedexAreaScreen->areaUnknownSprites[i] = &gSprites[spriteId];
            }
            else
            {
                // Failed to create sprite
                sPokedexAreaScreen->areaUnknownSprites[i] = NULL;
            }
        }
    }
}

static void LoadHGSSScreenSelectBarSubmenu(void)
{
    CopyToBgTilemapBuffer(1, sPokedexPlusHGSS_ScreenSelectBarSubmenu_Tilemap, 0, 0);
    CopyBgTilemapBufferToVram(1);
}
