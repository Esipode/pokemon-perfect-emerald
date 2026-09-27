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

// Transient flags OR'd into a mask cell by the connectivity passes.
#define INFCAVE_FLAG_COMPONENT 0x40 // member of the component being measured
#define INFCAVE_FLAG_KEEP      0x80 // member of the component that survives
#define INFCAVE_CELL_KIND_MASK 0x0F

// Room placement attempts before settling for however many rooms landed.
#define INFCAVE_ROOM_TRIES 80

struct InfCaveRoomRect
{
    u8 x;
    u8 y;
    u8 w;
    u8 h;
};

// The per-tile mask every generation pass works on, and the rectangles the
// rooms were carved from. Both are EWRAM: 1600 bytes is far too much for the
// stack, and later passes (set pieces, entrance/exit, trainer placement) read
// the rooms back.
static EWRAM_DATA u8 sMask[INFCAVE_MAP_HEIGHT][INFCAVE_MAP_WIDTH] = {0};
static EWRAM_DATA struct InfCaveRoomRect sRooms[INFCAVE_MAX_ROOMS] = {0};
static EWRAM_DATA u8 sRoomCount = 0;

// Distinct RNG streams within one room, so adding a consumer cannot shift the
// numbers an existing pass draws.
#define INFCAVE_SALT_MASK 0x4D41534Bu // 'MASK'

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

static u32 CellKind(u32 x, u32 y)
{
    return sMask[y][x] & INFCAVE_CELL_KIND_MASK;
}

static void SetCell(u32 x, u32 y, u32 kind)
{
    sMask[y][x] = kind;
}

static bool32 IsFloor(u32 x, u32 y)
{
    return CellKind(x, y) == INFCAVE_CELL_FLOOR;
}

// Half-open bounds of the area passes may write, i.e. the canvas minus the
// solid margin.
#define INFCAVE_AREA_MIN       INFCAVE_MARGIN
#define INFCAVE_AREA_MAX_X     (INFCAVE_MAP_WIDTH - INFCAVE_MARGIN)
#define INFCAVE_AREA_MAX_Y     (INFCAVE_MAP_HEIGHT - INFCAVE_MARGIN)

static void ClearMask(void)
{
    memset(sMask, INFCAVE_CELL_VOID, sizeof(sMask));
    memset(sRooms, 0, sizeof(sRooms));
    sRoomCount = 0;
}

static void CarveRect(u32 x, u32 y, u32 w, u32 h)
{
    u32 cx, cy;

    for (cy = y; cy < y + h; cy++)
    {
        for (cx = x; cx < x + w; cx++)
        {
            if (cx < INFCAVE_AREA_MIN || cx >= INFCAVE_AREA_MAX_X
             || cy < INFCAVE_AREA_MIN || cy >= INFCAVE_AREA_MAX_Y)
                continue;
            SetCell(cx, cy, INFCAVE_CELL_FLOOR);
        }
    }
}

// Rooms keep INFCAVE_ROOM_GAP free tiles between them so the wall shell between
// two rooms is always thick enough to tile.
static bool32 RoomFits(u32 x, u32 y, u32 w, u32 h)
{
    u32 i;

    for (i = 0; i < sRoomCount; i++)
    {
        const struct InfCaveRoomRect *other = &sRooms[i];

        if (x < other->x + other->w + INFCAVE_ROOM_GAP
         && other->x < x + w + INFCAVE_ROOM_GAP
         && y < other->y + other->h + INFCAVE_ROOM_GAP
         && other->y < y + h + INFCAVE_ROOM_GAP)
            return FALSE;
    }
    return TRUE;
}

static void PlaceRooms(rng_value_t *rng)
{
    u32 wanted = InfCave_RandRange(rng, INFCAVE_MIN_ROOMS, INFCAVE_MAX_ROOMS);
    u32 tries;

    for (tries = 0; tries < INFCAVE_ROOM_TRIES && sRoomCount < wanted; tries++)
    {
        u32 w = InfCave_RandRange(rng, INFCAVE_ROOM_MIN_W, INFCAVE_ROOM_MAX_W);
        u32 h = InfCave_RandRange(rng, INFCAVE_ROOM_MIN_H, INFCAVE_ROOM_MAX_H);
        u32 x, y;

        if (w >= INFCAVE_AREA_MAX_X - INFCAVE_AREA_MIN || h >= INFCAVE_AREA_MAX_Y - INFCAVE_AREA_MIN)
            continue;

        x = InfCave_RandRange(rng, INFCAVE_AREA_MIN, INFCAVE_AREA_MAX_X - w);
        y = InfCave_RandRange(rng, INFCAVE_AREA_MIN, INFCAVE_AREA_MAX_Y - h);

        if (!RoomFits(x, y, w, h))
            continue;

        sRooms[sRoomCount].x = x;
        sRooms[sRoomCount].y = y;
        sRooms[sRoomCount].w = w;
        sRooms[sRoomCount].h = h;
        sRoomCount++;
        CarveRect(x, y, w, h);
    }
}

static void CarveCorridorH(u32 xa, u32 xb, u32 y)
{
    u32 x = min(xa, xb);
    u32 span = max(xa, xb) - x + 1;

    CarveRect(x, y, span, INFCAVE_CORRIDOR_WIDTH);
}

static void CarveCorridorV(u32 ya, u32 yb, u32 x)
{
    u32 y = min(ya, yb);
    u32 span = max(ya, yb) - y + 1;

    CarveRect(x, y, INFCAVE_CORRIDOR_WIDTH, span);
}

// Chains the rooms in placement order with L-shaped corridors. The elbow goes
// either way round at random, which is what keeps the layouts from all reading
// as one spine.
static void CarveCorridors(rng_value_t *rng)
{
    u32 i;

    for (i = 1; i < sRoomCount; i++)
    {
        u32 x0 = sRooms[i - 1].x + sRooms[i - 1].w / 2;
        u32 y0 = sRooms[i - 1].y + sRooms[i - 1].h / 2;
        u32 x1 = sRooms[i].x + sRooms[i].w / 2;
        u32 y1 = sRooms[i].y + sRooms[i].h / 2;

        if (InfCave_Rand(rng) & 1)
        {
            CarveCorridorH(x0, x1, y0);
            CarveCorridorV(y0, y1, x1);
        }
        else
        {
            CarveCorridorV(y0, y1, x0);
            CarveCorridorH(x0, x1, y1);
        }
    }
}

static u32 CountFloorNeighbours(u32 x, u32 y, const u8 *prevRow, const u8 *curRow)
{
    s32 dx, dy;
    u32 count = 0;

    for (dy = -1; dy <= 1; dy++)
    {
        s32 ny = (s32)y + dy;

        if (ny < 0 || ny >= INFCAVE_MAP_HEIGHT)
            continue;

        for (dx = -1; dx <= 1; dx++)
        {
            s32 nx = (s32)x + dx;
            u32 kind;

            if ((dx == 0 && dy == 0) || nx < 0 || nx >= INFCAVE_MAP_WIDTH)
                continue;

            // Rows above the one being written were already modified this pass,
            // so read them from the saved copies instead of the mask.
            if (dy < 0)
                kind = prevRow[nx];
            else if (dy == 0)
                kind = curRow[nx];
            else
                kind = CellKind(nx, ny);

            if (kind == INFCAVE_CELL_FLOOR)
                count++;
        }
    }
    return count;
}

// Cellular automaton passes over the floor set: thin floor erodes, pockets
// surrounded by floor fill in. This is what breaks up the rectangles the room
// and corridor passes leave behind.
static void SmoothMask(void)
{
    u8 prevRow[INFCAVE_MAP_WIDTH];
    u8 curRow[INFCAVE_MAP_WIDTH];
    u32 pass, x, y;

    for (pass = 0; pass < INFCAVE_SMOOTH_PASSES; pass++)
    {
        for (x = 0; x < INFCAVE_MAP_WIDTH; x++)
            prevRow[x] = CellKind(x, INFCAVE_AREA_MIN - 1);

        for (y = INFCAVE_AREA_MIN; y < INFCAVE_AREA_MAX_Y; y++)
        {
            for (x = 0; x < INFCAVE_MAP_WIDTH; x++)
                curRow[x] = CellKind(x, y);

            for (x = INFCAVE_AREA_MIN; x < INFCAVE_AREA_MAX_X; x++)
            {
                u32 neighbours = CountFloorNeighbours(x, y, prevRow, curRow);

                if (curRow[x] == INFCAVE_CELL_FLOOR)
                {
                    if (neighbours < 4)
                        SetCell(x, y, INFCAVE_CELL_VOID);
                }
                else if (curRow[x] == INFCAVE_CELL_VOID && neighbours >= 5)
                {
                    SetCell(x, y, INFCAVE_CELL_FLOOR);
                }
            }

            memcpy(prevRow, curRow, sizeof(prevRow));
        }
    }
}

static void ClearMaskFlags(u32 flag)
{
    u32 x, y;

    for (y = 0; y < INFCAVE_MAP_HEIGHT; y++)
    {
        for (x = 0; x < INFCAVE_MAP_WIDTH; x++)
            sMask[y][x] &= ~flag;
    }
}

// Flood fills the floor component containing (sx, sy) by sweeping the mask
// until nothing new is marked. Alternating sweep directions converges in a
// handful of passes and needs no queue in EWRAM.
static u32 FloodFillComponent(u32 sx, u32 sy, u32 flag)
{
    bool32 changed = TRUE;
    u32 x, y, count = 0;

    sMask[sy][sx] |= flag;

    while (changed)
    {
        changed = FALSE;

        for (y = INFCAVE_AREA_MIN; y < INFCAVE_AREA_MAX_Y; y++)
        {
            for (x = INFCAVE_AREA_MIN; x < INFCAVE_AREA_MAX_X; x++)
            {
                if (!IsFloor(x, y) || (sMask[y][x] & flag))
                    continue;
                if ((sMask[y][x - 1] & flag) || (sMask[y - 1][x] & flag))
                {
                    sMask[y][x] |= flag;
                    changed = TRUE;
                }
            }
        }

        for (y = INFCAVE_AREA_MAX_Y; y-- > INFCAVE_AREA_MIN; )
        {
            for (x = INFCAVE_AREA_MAX_X; x-- > INFCAVE_AREA_MIN; )
            {
                if (!IsFloor(x, y) || (sMask[y][x] & flag))
                    continue;
                if ((sMask[y][x + 1] & flag) || (sMask[y + 1][x] & flag))
                {
                    sMask[y][x] |= flag;
                    changed = TRUE;
                }
            }
        }
    }

    for (y = INFCAVE_AREA_MIN; y < INFCAVE_AREA_MAX_Y; y++)
    {
        for (x = INFCAVE_AREA_MIN; x < INFCAVE_AREA_MAX_X; x++)
        {
            if (sMask[y][x] & flag)
                count++;
        }
    }
    return count;
}

// Keeps the largest floor component and deletes the rest, so the room can never
// hand the player an unreachable pocket. KEEP doubles as "already measured"
// during the survey, then is rebuilt for the winning component alone.
static void PruneToLargestComponent(void)
{
    u32 x, y;
    u32 bestSize = 0, bestX = 0, bestY = 0;

    for (y = INFCAVE_AREA_MIN; y < INFCAVE_AREA_MAX_Y; y++)
    {
        for (x = INFCAVE_AREA_MIN; x < INFCAVE_AREA_MAX_X; x++)
        {
            u32 size, mx, my;

            if (!IsFloor(x, y) || (sMask[y][x] & INFCAVE_FLAG_KEEP))
                continue;

            size = FloodFillComponent(x, y, INFCAVE_FLAG_COMPONENT);
            if (size > bestSize)
            {
                bestSize = size;
                bestX = x;
                bestY = y;
            }

            for (my = INFCAVE_AREA_MIN; my < INFCAVE_AREA_MAX_Y; my++)
            {
                for (mx = INFCAVE_AREA_MIN; mx < INFCAVE_AREA_MAX_X; mx++)
                {
                    if (sMask[my][mx] & INFCAVE_FLAG_COMPONENT)
                        sMask[my][mx] |= INFCAVE_FLAG_KEEP;
                }
            }
            ClearMaskFlags(INFCAVE_FLAG_COMPONENT);
        }
    }

    ClearMaskFlags(INFCAVE_FLAG_KEEP);

    if (bestSize == 0)
        return;

    FloodFillComponent(bestX, bestY, INFCAVE_FLAG_KEEP);

    for (y = 0; y < INFCAVE_MAP_HEIGHT; y++)
    {
        for (x = 0; x < INFCAVE_MAP_WIDTH; x++)
        {
            if (CellKind(x, y) == INFCAVE_CELL_FLOOR && !(sMask[y][x] & INFCAVE_FLAG_KEEP))
                SetCell(x, y, INFCAVE_CELL_VOID);
        }
    }

    ClearMaskFlags(INFCAVE_FLAG_KEEP);
}

// Wraps every floor region in INFCAVE_WALL_THICKNESS wall tiles. Anything
// further out stays void; the autotile pass renders both as solid rock.
static void BuildWallShell(void)
{
    s32 x, y, dx, dy;

    for (y = 0; y < INFCAVE_MAP_HEIGHT; y++)
    {
        for (x = 0; x < INFCAVE_MAP_WIDTH; x++)
        {
            bool32 nearFloor = FALSE;

            if (CellKind(x, y) != INFCAVE_CELL_VOID)
                continue;

            for (dy = -INFCAVE_WALL_THICKNESS; dy <= INFCAVE_WALL_THICKNESS && !nearFloor; dy++)
            {
                for (dx = -INFCAVE_WALL_THICKNESS; dx <= INFCAVE_WALL_THICKNESS; dx++)
                {
                    s32 nx = x + dx, ny = y + dy;

                    if (nx < 0 || nx >= INFCAVE_MAP_WIDTH || ny < 0 || ny >= INFCAVE_MAP_HEIGHT)
                        continue;
                    if (IsFloor(nx, ny))
                    {
                        nearFloor = TRUE;
                        break;
                    }
                }
            }

            if (nearFloor)
                SetCell(x, y, INFCAVE_CELL_WALL);
        }
    }
}

// Temporary until Stage 10 places the exit crystal: punches a corridor from the
// fixed warp tile up into the cave. It deliberately runs after the legality
// passes, since a floor tile inside the canvas margin is exactly what those
// passes forbid.
static void CarveTempExitAccess(void)
{
    u32 y;

    for (y = INFCAVE_TEMP_EXIT_Y; y >= INFCAVE_AREA_MIN; y--)
    {
        u32 x;
        bool32 reached = FALSE;

        for (x = INFCAVE_TEMP_EXIT_X; x < INFCAVE_TEMP_EXIT_X + INFCAVE_CORRIDOR_WIDTH; x++)
        {
            if (IsFloor(x, y))
                reached = TRUE;
            SetCell(x, y, INFCAVE_CELL_FLOOR);
        }

        if (reached)
            return;
    }

    // The column found no floor at all: cut one row straight across instead.
    for (y = INFCAVE_AREA_MIN; y < INFCAVE_AREA_MIN + INFCAVE_CORRIDOR_WIDTH; y++)
        CarveRect(INFCAVE_AREA_MIN, y, INFCAVE_AREA_MAX_X - INFCAVE_AREA_MIN, 1);
}

// Nearest floor tile to the canvas centre, searched outward in square rings.
static void PlacePlayerOnFloor(void)
{
    u32 cx = INFCAVE_MAP_WIDTH / 2;
    u32 cy = INFCAVE_MAP_HEIGHT / 2;
    u32 radius;

    for (radius = 0; radius < INFCAVE_MAP_WIDTH; radius++)
    {
        s32 dx, dy;

        for (dy = -(s32)radius; dy <= (s32)radius; dy++)
        {
            for (dx = -(s32)radius; dx <= (s32)radius; dx++)
            {
                s32 x = (s32)cx + dx, y = (s32)cy + dy;

                if (abs(dx) != (s32)radius && abs(dy) != (s32)radius)
                    continue;
                if (x < 0 || x >= INFCAVE_MAP_WIDTH || y < 0 || y >= INFCAVE_MAP_HEIGHT)
                    continue;
                if (!IsFloor(x, y))
                    continue;

                gSaveBlock1Ptr->pos.x = x;
                gSaveBlock1Ptr->pos.y = y;
                return;
            }
        }
    }

    gSaveBlock1Ptr->pos.x = cx;
    gSaveBlock1Ptr->pos.y = cy;
}

#if INFCAVE_TRACE == TRUE
static void TraceMask(void)
{
    static const char sCellChar[] = { ' ', '#', '.', '*' };
    u32 x, y;

    DebugPrintf("InfCave mask depth %d seed %08X rooms %d",
                InfCave_GetDepth(), gSaveBlock1Ptr->infinityCaveRun.roomSeed, sRoomCount);

    for (y = 0; y < INFCAVE_MAP_HEIGHT; y++)
    {
        char line[INFCAVE_MAP_WIDTH + 1];

        for (x = 0; x < INFCAVE_MAP_WIDTH; x++)
            line[x] = sCellChar[CellKind(x, y)];
        line[INFCAVE_MAP_WIDTH] = '\0';
        DebugPrintf("%s", line);
    }
}
#endif

// The ordered generation pipeline. Every pass reads and writes sMask only;
// nothing here knows about metatiles.
static void BuildMask(void)
{
    rng_value_t rng = InfCave_SeedRoomRng(INFCAVE_SALT_MASK);

    ClearMask();
    PlaceRooms(&rng);
    CarveCorridors(&rng);
    SmoothMask();
    PruneToLargestComponent();
    CarveTempExitAccess();
    BuildWallShell();

#if INFCAVE_TRACE == TRUE
    TraceMask();
#endif
}

void InfCave_GenerateRoom(u16 *backupMapData, bool8 setPlayerPosition)
{
    u32 x, y;
    u16 *map;

    InfCave_LoadTileRoles();
    BuildMask();

    gBackupMapLayout.map = backupMapData;
    gBackupMapLayout.width = INFCAVE_MAP_WIDTH + MAP_OFFSET_W;
    gBackupMapLayout.height = INFCAVE_MAP_HEIGHT + MAP_OFFSET_H;

    map = backupMapData + gBackupMapLayout.width * MAP_OFFSET + MAP_OFFSET;
    for (y = 0; y < INFCAVE_MAP_HEIGHT; y++)
    {
        for (x = 0; x < INFCAVE_MAP_WIDTH; x++)
        {
            // Stage 7 replaces this with the autotiler; until then void and
            // wall both render as fill and floor takes the plain variant.
            bool32 isFloor = CellKind(x, y) == INFCAVE_CELL_FLOOR;

            map[x] = sTileRole[isFloor ? INFCAVE_ROLE_FLOOR_0 : INFCAVE_ROLE_WALL_FILL];
        }
        map += gBackupMapLayout.width;
    }

    backupMapData[gBackupMapLayout.width * (MAP_OFFSET + INFCAVE_TEMP_EXIT_Y)
                + MAP_OFFSET + INFCAVE_TEMP_EXIT_X] = sTileRole[INFCAVE_ROLE_PAD_EXIT];

    // setPlayerPosition mirrors the Battle Pyramid's inverted sense: TRUE means
    // the position is already restored from the save and must be kept.
    if (setPlayerPosition == FALSE)
        PlacePlayerOnFloor();

    RunOnLoadMapScript();
}
