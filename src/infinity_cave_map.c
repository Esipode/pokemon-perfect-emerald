#include "global.h"
#include "fieldmap.h"
#include "infinity_cave.h"
#include "overworld.h"
#include "script.h"
#include "constants/infinity_cave.h"
#include "constants/layouts.h"

// Room generation. The room map's ROM layout is never used: every entry
// rewrites sBackupMapData in place, so one static map hosts every room of a
// run. Output is a pure function of the run's roomSeed.

// Raw block words (metatile id + collision + elevation) pulled from the
// Porymap-authored key layout, indexed by enum InfCaveTileRole.
static EWRAM_DATA u16 sTileRole[INFCAVE_ROLE_COUNT] = {0};

// Fallbacks used if the key layout's floor or fill cell is blank, so a bad edit
// cannot produce a room with no walkable tiles or no solid border.
#define INFCAVE_FALLBACK_FLOOR 0x3201 // gTileset_Cave floor, elevation 3
#define INFCAVE_FALLBACK_WALL  0x0611 // gTileset_Cave wall fill, collision 1

// Half-open cell ranges the key layout must fill. Roles outside these are spare.
static const u8 sRequiredRoles[][2] =
{
    { INFCAVE_ROLE_FLOOR_0,          INFCAVE_ROLE_FLOOR_7 + 1 },
    { INFCAVE_ROLE_WALL_NW,          INFCAVE_ROLE_WALL_INNER_SE + 1 },
    { INFCAVE_ROLE_FACE_L,           INFCAVE_ROLE_FACE_INNER_R + 1 },
    { INFCAVE_ROLE_DECOR_ROCK_SMALL, INFCAVE_ROLE_DECOR_PUDDLE + 1 },
    { INFCAVE_ROLE_PAD_ENTRANCE,     INFCAVE_ROLE_PAD_SHOP + 1 },
};

// Temporary way out until Stage 10 places the exit crystal. Must match the
// warp event in data/maps/InfinityCave_Room/map.json.
#define INFCAVE_TEMP_EXIT_X 20
#define INFCAVE_TEMP_EXIT_Y 37

// Minimum wall run thickness. gTileset_Cave draws the face a single metatile
// tall, but the mask still needs a solid margin behind every face.
#define INFCAVE_WALL_THICKNESS 2

// Copies the key layout's block words into sTileRole. The layout is ROM data,
// so this is a straight copy; reloading it every room keeps a Porymap edit
// visible without a new game.
void InfCave_LoadTileRoles(void)
{
    const struct MapLayout *keyLayout = GetMapLayout(LAYOUT_INFINITY_CAVE_TILEKEY);
    u32 i;

    AGB_ASSERT(keyLayout->width == INFCAVE_TILEKEY_WIDTH);
    AGB_ASSERT(keyLayout->height == INFCAVE_TILEKEY_HEIGHT);

    for (i = 0; i < INFCAVE_ROLE_COUNT; i++)
        sTileRole[i] = keyLayout->map[i];

    for (i = 0; i < ARRAY_COUNT(sRequiredRoles); i++)
    {
        u32 role;

        for (role = sRequiredRoles[i][0]; role < sRequiredRoles[i][1]; role++)
            AGB_ASSERT(sTileRole[role] != 0);
    }

    if (sTileRole[INFCAVE_ROLE_FLOOR_0] == 0)
        sTileRole[INFCAVE_ROLE_FLOOR_0] = INFCAVE_FALLBACK_FLOOR;
    if (sTileRole[INFCAVE_ROLE_WALL_FILL] == 0)
        sTileRole[INFCAVE_ROLE_WALL_FILL] = INFCAVE_FALLBACK_WALL;
}

u16 InfCave_GetRoleBlock(u32 role)
{
    return sTileRole[role];
}

void InfCave_GenerateRoom(u16 *backupMapData, bool8 setPlayerPosition)
{
    u32 x, y;
    u16 *map;

    InfCave_LoadTileRoles();

    gBackupMapLayout.map = backupMapData;
    gBackupMapLayout.width = INFCAVE_MAP_WIDTH + MAP_OFFSET_W;
    gBackupMapLayout.height = INFCAVE_MAP_HEIGHT + MAP_OFFSET_H;

    map = backupMapData + gBackupMapLayout.width * MAP_OFFSET + MAP_OFFSET;
    for (y = 0; y < INFCAVE_MAP_HEIGHT; y++)
    {
        for (x = 0; x < INFCAVE_MAP_WIDTH; x++)
        {
            bool32 isWall = x < INFCAVE_WALL_THICKNESS
                         || x >= INFCAVE_MAP_WIDTH - INFCAVE_WALL_THICKNESS
                         || y < INFCAVE_WALL_THICKNESS
                         || y >= INFCAVE_MAP_HEIGHT - INFCAVE_WALL_THICKNESS;

            map[x] = sTileRole[isWall ? INFCAVE_ROLE_WALL_FILL : INFCAVE_ROLE_FLOOR_0];
        }
        map += gBackupMapLayout.width;
    }

    backupMapData[gBackupMapLayout.width * (MAP_OFFSET + INFCAVE_TEMP_EXIT_Y)
                + MAP_OFFSET + INFCAVE_TEMP_EXIT_X] = sTileRole[INFCAVE_ROLE_PAD_EXIT];

    // setPlayerPosition mirrors the Battle Pyramid's inverted sense: TRUE means
    // the position is already restored from the save and must be kept.
    if (setPlayerPosition == FALSE)
    {
        gSaveBlock1Ptr->pos.x = INFCAVE_MAP_WIDTH / 2;
        gSaveBlock1Ptr->pos.y = INFCAVE_MAP_HEIGHT / 2;
    }

    RunOnLoadMapScript();
}
