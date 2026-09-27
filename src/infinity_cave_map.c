#include "global.h"
#include "fieldmap.h"
#include "infinity_cave.h"
#include "script.h"
#include "constants/infinity_cave.h"

// Room generation. The room map's ROM layout is never used: every entry
// rewrites sBackupMapData in place, so one static map hosts every room of a
// run. Output is a pure function of the run's roomSeed.

// Raw block words (metatile id + collision + elevation) from gTileset_Cave.
// Stage 4 replaces these with the Porymap-authored tile-role key map.
#define INFCAVE_BLOCK_FLOOR 0x3201
#define INFCAVE_BLOCK_WALL  0x06F1
#define INFCAVE_BLOCK_EXIT  0x3207

// Temporary way out until Stage 10 places the exit crystal. Must match the
// warp event in data/maps/InfinityCave_Room/map.json.
#define INFCAVE_TEMP_EXIT_X 20
#define INFCAVE_TEMP_EXIT_Y 37

// Cave wall art is two metatiles tall on its south face, so no wall run may be
// thinner than this vertically.
#define INFCAVE_WALL_THICKNESS 2

void InfCave_GenerateRoom(u16 *backupMapData, bool8 setPlayerPosition)
{
    u32 x, y;
    u16 *map;

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

            map[x] = isWall ? INFCAVE_BLOCK_WALL : INFCAVE_BLOCK_FLOOR;
        }
        map += gBackupMapLayout.width;
    }

    backupMapData[gBackupMapLayout.width * (MAP_OFFSET + INFCAVE_TEMP_EXIT_Y)
                + MAP_OFFSET + INFCAVE_TEMP_EXIT_X] = INFCAVE_BLOCK_EXIT;

    // setPlayerPosition mirrors the Battle Pyramid's inverted sense: TRUE means
    // the position is already restored from the save and must be kept.
    if (setPlayerPosition == FALSE)
    {
        gSaveBlock1Ptr->pos.x = INFCAVE_MAP_WIDTH / 2;
        gSaveBlock1Ptr->pos.y = INFCAVE_MAP_HEIGHT / 2;
    }

    RunOnLoadMapScript();
}
