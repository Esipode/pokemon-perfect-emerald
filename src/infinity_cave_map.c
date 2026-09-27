#include "global.h"
#include "fieldmap.h"
#include "infinity_cave.h"
#include "malloc.h"
#include "overworld.h"
#include "script.h"
#include "constants/event_object_movement.h"
#include "constants/event_objects.h"
#include "constants/infinity_cave.h"
#include "constants/layouts.h"
#include "constants/map_event_ids.h"
#include "constants/trainer_types.h"
#include "data/infinity_cave.h"

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
    { INFCAVE_ROLE_DECOR_ROCK_SMALL, INFCAVE_ROLE_DECOR_BONES + 1 },
    { INFCAVE_ROLE_PAD_ENTRANCE,     INFCAVE_ROLE_PAD_SHOP + 1 },
    { INFCAVE_ROLE_SAND_NW,          INFCAVE_ROLE_SAND_SE + 1 },
};

// Minimum wall run thickness. gTileset_Cave draws the face a single metatile
// tall, but the mask still needs a solid margin behind every face.
#define INFCAVE_WALL_THICKNESS 2

// Flags OR'd into a mask cell alongside its kind. COMPONENT and KEEP are
// transient, owned by the connectivity passes; NO_DECOR survives for the room's
// lifetime and marks the tiles the entrance and exit pads own.
#define INFCAVE_FLAG_BLOCKED   0x10 // a generated object stands here, or its sight line is under test
#define INFCAVE_FLAG_NO_DECOR  0x20 // the decoration pass must leave this tile bare
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

// The four per-tile grids every generation pass works on. One 40x40 grid is 1600
// bytes, so they live on the heap for the length of a generation rather than in
// EWRAM, which has no room for 6400 bytes of statics. Nothing outside generation
// reads them, so the block is freed before InfCave_GenerateRoom returns.
//
//   mask  - the cell kinds and flags every pass reads and writes.
//   decor - props on the mask's floor, as one plus an index into sInfCaveDecor
//           so zero means an empty tile. The decoration pass leaves the mask
//           itself untouched, so the autotiler still sees every prop tile as
//           floor and tiles the walls around it normally; the prop's own block
//           is written over the floor block.
//   patch - terrain patches, as one plus an index into sInfCavePatch. Like props
//           these sit beside the mask rather than in it, so the wall shapes
//           around a patch are unaffected; unlike props a patch cell's block
//           depends on where the cell sits in its rectangle, which the autotiler
//           works out from the neighbouring cells.
//   dist  - step distance in tiles from the entrance, INFCAVE_DIST_UNREACHED
//           where the fill never arrived. Only the entrance/exit pass reads it.
struct InfCaveGrids
{
    u8 mask[INFCAVE_MAP_HEIGHT][INFCAVE_MAP_WIDTH];
    u8 decor[INFCAVE_MAP_HEIGHT][INFCAVE_MAP_WIDTH];
    u8 patch[INFCAVE_MAP_HEIGHT][INFCAVE_MAP_WIDTH];
    u8 dist[INFCAVE_MAP_HEIGHT][INFCAVE_MAP_WIDTH];
};

static EWRAM_DATA struct InfCaveGrids *sGrids = NULL;

#define sMask  (sGrids->mask)
#define sDecor (sGrids->decor)
#define sPatch (sGrids->patch)
#define sDist  (sGrids->dist)

// Grabs the generation grids. Alloc is fatal on failure, so a non-NULL result is
// the only outcome the callers see; the guard only keeps a nested call from
// allocating a second block over the first.
static bool32 AllocGrids(void)
{
    if (sGrids == NULL)
        sGrids = AllocZeroed(sizeof(*sGrids));

    return sGrids != NULL;
}

static void FreeGrids(void)
{
    TRY_FREE_AND_SET_NULL(sGrids);
}

// The rectangles the rooms were carved from. Small enough to stay in EWRAM, and
// later passes (set pieces, entrance/exit, trainer placement) read them back.
static EWRAM_DATA struct InfCaveRoomRect sRooms[INFCAVE_MAX_ROOMS] = {0};
static EWRAM_DATA u8 sRoomCount = 0;

// The set piece this room hosts, if any. layoutId is INFCAVE_PIECE_NONE when
// the room type stamps nothing; w and h come from the layout, and x and y are
// filled in once the stamp pass has picked the room that hosts it. The mask
// generator reserves a room of w by h so the piece always has somewhere to land.
struct InfCaveStamp
{
    u16 layoutId;
    u8 x;
    u8 y;
    u8 w;
    u8 h;
};

static EWRAM_DATA struct InfCaveStamp sStamp = {0};

// The room the set piece was stamped into, or -1 when nothing was stamped. The
// entrance pass skips it: that room is wall-to-wall authored art.
static EWRAM_DATA s8 sStampHost = 0;

// Where the player arrives and where the descent ladder is drawn. Both are plain
// floor tiles the pads are drawn over; the mask itself still reads as floor
// there, so the walls around them tile normally.
static EWRAM_DATA u8 sEntranceX = 0;
static EWRAM_DATA u8 sEntranceY = 0;
static EWRAM_DATA u8 sExitX = 0;
static EWRAM_DATA u8 sExitY = 0;

// Distinct RNG streams within one room, so adding a consumer cannot shift the
// numbers an existing pass draws.
#define INFCAVE_SALT_MASK 0x4D41534Bu // 'MASK'
#define INFCAVE_SALT_TILE 0x54494C45u // 'TILE'
#define INFCAVE_SALT_EXIT 0x45584954u // 'EXIT'
#define INFCAVE_SALT_DECO 0x4445434Fu // 'DECO'
#define INFCAVE_SALT_PTCH 0x50544348u // 'PTCH'
#define INFCAVE_SALT_NPCS 0x4E504353u // 'NPCS'

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

static bool32 IsReserved(u32 x, u32 y)
{
    return CellKind(x, y) == INFCAVE_CELL_RESERVED;
}

// The stamped block a reserved cell draws. Only valid inside the stamp rect,
// which is the only place a reserved cell exists.
static u16 StampBlockAt(u32 x, u32 y)
{
    const struct MapLayout *layout = GetMapLayout(sStamp.layoutId);

    return layout->map[(y - sStamp.y) * layout->width + (x - sStamp.x)];
}

// Picks the set piece the current room type calls for. Runs before the mask so
// the generator can reserve a room the piece fits in. An oversized piece is
// dropped rather than stamped: reserving a room larger than the canvas allows
// would corrupt the placement arithmetic.
static void ResolveSetPiece(void)
{
    u32 roomType = InfCave_GetRoomType();
    // Only the asserts read the room layout, and only a debug build has those.
    const struct MapLayout *roomLayout UNUSED;
    const struct MapLayout *layout;

    memset(&sStamp, 0, sizeof(sStamp));
    sStampHost = -1;

    if (roomType >= INFCAVE_ROOM_COUNT || sInfCavePieceLayout[roomType] == INFCAVE_PIECE_NONE)
        return;

    layout = GetMapLayout(sInfCavePieceLayout[roomType]);
    roomLayout = GetMapLayout(LAYOUT_INFINITY_CAVE_ROOM);

    // Stamping copies raw block words, so a piece authored against other tilesets
    // would draw unrelated art.
    AGB_ASSERT(layout->primaryTileset == roomLayout->primaryTileset);
    AGB_ASSERT(layout->secondaryTileset == roomLayout->secondaryTileset);
    AGB_ASSERT(layout->width <= INFCAVE_PIECE_MAX_W);
    AGB_ASSERT(layout->height <= INFCAVE_PIECE_MAX_H);

    if (layout->width > INFCAVE_PIECE_MAX_W || layout->height > INFCAVE_PIECE_MAX_H)
        return;

    sStamp.layoutId = sInfCavePieceLayout[roomType];
    sStamp.w = layout->width;
    sStamp.h = layout->height;
}

// Half-open bounds of the area passes may write, i.e. the canvas minus the
// solid margin.
#define INFCAVE_AREA_MIN       INFCAVE_MARGIN
#define INFCAVE_AREA_MAX_X     (INFCAVE_MAP_WIDTH - INFCAVE_MARGIN)
#define INFCAVE_AREA_MAX_Y     (INFCAVE_MAP_HEIGHT - INFCAVE_MARGIN)

static void ClearMask(void)
{
    memset(sMask, INFCAVE_CELL_VOID, sizeof(sMask));
    memset(sDecor, 0, sizeof(sDecor));
    memset(sPatch, 0, sizeof(sPatch));
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

// Places the room that hosts this room's set piece, at exactly the piece's size
// so the piece can be stamped over it whole. It goes down before any rolled
// room, on an empty canvas, so it cannot be crowded out.
static void PlaceStampRoom(rng_value_t *rng)
{
    u32 tries;

    for (tries = 0; tries < INFCAVE_ROOM_TRIES; tries++)
    {
        u32 x = InfCave_RandRange(rng, INFCAVE_AREA_MIN, INFCAVE_AREA_MAX_X - sStamp.w);
        u32 y = InfCave_RandRange(rng, INFCAVE_AREA_MIN, INFCAVE_AREA_MAX_Y - sStamp.h);

        if (!RoomFits(x, y, sStamp.w, sStamp.h))
            continue;

        sRooms[sRoomCount].x = x;
        sRooms[sRoomCount].y = y;
        sRooms[sRoomCount].w = sStamp.w;
        sRooms[sRoomCount].h = sStamp.h;
        sRoomCount++;
        CarveRect(x, y, sStamp.w, sStamp.h);
        return;
    }
}

static void PlaceRooms(rng_value_t *rng)
{
    u32 wanted = InfCave_RandRange(rng, INFCAVE_MIN_ROOMS, INFCAVE_MAX_ROOMS);
    u32 tries;

    if (sStamp.layoutId != INFCAVE_PIECE_NONE)
        PlaceStampRoom(rng);

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

// Why a mask was rejected. Reported by the debug harness so a bad seed names the
// rule it broke instead of only failing.
enum InfCaveMaskFault
{
    INFCAVE_FAULT_NONE,
    INFCAVE_FAULT_MARGIN,    // floor inside the solid canvas margin
    INFCAVE_FAULT_DIAGONAL,  // two floor regions joined only through a diagonal
    INFCAVE_FAULT_THIN_WALL, // wall run one tile thick between two floor tiles
    INFCAVE_FAULT_RIM_SEAM,  // wall rim shape drawn beside a wall face shape
    INFCAVE_FAULT_AREA,      // less walkable area than INFCAVE_MIN_FLOOR_TILES
    INFCAVE_FAULT_SPLIT,     // more than one floor component
};

static bool32 IsFloorSafe(s32 x, s32 y)
{
    if (x < 0 || x >= INFCAVE_MAP_WIDTH || y < 0 || y >= INFCAVE_MAP_HEIGHT)
        return FALSE;
    return IsFloor(x, y);
}

// Open space as the autotiler sees it: carved floor, or a set piece's cell,
// which draws its own authored art and must never read as rock to the wall
// shapes around it.
static bool32 IsOpenSafe(s32 x, s32 y)
{
    if (x < 0 || x >= INFCAVE_MAP_WIDTH || y < 0 || y >= INFCAVE_MAP_HEIGHT)
        return FALSE;
    return IsFloor(x, y) || IsReserved(x, y);
}

// Wall shape for a non-floor cell. The mask legality rules forbid a wall with
// floor on two opposite sides, so the only cardinal combinations left are none,
// one side, or two perpendicular sides; that is what collapses the 256
// eight-neighbour cases onto the authored shape set. A cell with no cardinal
// floor but floor on one diagonal is a concave corner.
static u32 WallRoleForCell(s32 x, s32 y)
{
    bool32 n = IsOpenSafe(x, y - 1);
    bool32 s = IsOpenSafe(x, y + 1);
    bool32 w = IsOpenSafe(x - 1, y);
    bool32 e = IsOpenSafe(x + 1, y);

    if (s && w)
        return INFCAVE_ROLE_WALL_SW;
    if (s && e)
        return INFCAVE_ROLE_WALL_SE;
    if (n && w)
        return INFCAVE_ROLE_WALL_NW;
    if (n && e)
        return INFCAVE_ROLE_WALL_NE;
    if (n)
        return INFCAVE_ROLE_WALL_N;
    if (s)
        return INFCAVE_ROLE_WALL_S;
    if (w)
        return INFCAVE_ROLE_WALL_W;
    if (e)
        return INFCAVE_ROLE_WALL_E;

    if (IsOpenSafe(x - 1, y - 1))
        return INFCAVE_ROLE_WALL_INNER_NW;
    if (IsOpenSafe(x + 1, y - 1))
        return INFCAVE_ROLE_WALL_INNER_NE;
    if (IsOpenSafe(x - 1, y + 1))
        return INFCAVE_ROLE_WALL_INNER_SW;
    if (IsOpenSafe(x + 1, y + 1))
        return INFCAVE_ROLE_WALL_INNER_SE;

    return INFCAVE_ROLE_WALL_FILL;
}

// Which metatile row a wall shape draws in. The cave tileset puts the north rim
// and the south face on different rows and has no art for the two meeting along
// a vertical seam, so the mask legality pass rejects that pairing.
static bool32 IsRimRole(u32 role)
{
    return role == INFCAVE_ROLE_WALL_NW || role == INFCAVE_ROLE_WALL_N
        || role == INFCAVE_ROLE_WALL_NE || role == INFCAVE_ROLE_WALL_INNER_NW
        || role == INFCAVE_ROLE_WALL_INNER_NE;
}

static bool32 IsFaceRole(u32 role)
{
    return role == INFCAVE_ROLE_WALL_SW || role == INFCAVE_ROLE_WALL_S
        || role == INFCAVE_ROLE_WALL_SE || role == INFCAVE_ROLE_WALL_INNER_SW
        || role == INFCAVE_ROLE_WALL_INNER_SE;
}

// TRUE when (x, y) and its eastern neighbour are both wall and one draws a rim
// while the other draws a face. A wall band that steps down by one row produces
// this, and the step needs a third wall row for the shapes to meet.
static bool32 HasRimFaceSeam(s32 x, s32 y)
{
    u32 a, b;

    if (IsOpenSafe(x, y) || IsOpenSafe(x + 1, y))
        return FALSE;

    a = WallRoleForCell(x, y);
    b = WallRoleForCell(x + 1, y);

    return (IsFaceRole(a) && IsRimRole(b)) || (IsRimRole(a) && IsFaceRole(b));
}

static u32 CountFloor(void)
{
    u32 x, y, count = 0;

    for (y = 0; y < INFCAVE_MAP_HEIGHT; y++)
    {
        for (x = 0; x < INFCAVE_MAP_WIDTH; x++)
        {
            if (IsFloor(x, y))
                count++;
        }
    }
    return count;
}

// TRUE when every floor tile belongs to one component. Only the carved area is
// walked, so a floor tile stranded in the margin reads as a split.
static bool32 IsFloorConnected(u32 floorCount)
{
    u32 x, y, reached;

    if (floorCount == 0)
        return FALSE;

    for (y = INFCAVE_AREA_MIN; y < INFCAVE_AREA_MAX_Y; y++)
    {
        for (x = INFCAVE_AREA_MIN; x < INFCAVE_AREA_MAX_X; x++)
        {
            if (!IsFloor(x, y))
                continue;

            reached = FloodFillComponent(x, y, INFCAVE_FLAG_KEEP);
            ClearMaskFlags(INFCAVE_FLAG_KEEP);
            return reached == floorCount;
        }
    }
    return FALSE;
}

// The canvas border must stay solid: the map is rendered at MAP_OFFSET inside a
// larger backup layout, and a floor tile on the edge would let the player walk
// into the connecting border.
static void ClearMarginFloor(void)
{
    u32 x, y;

    for (y = 0; y < INFCAVE_MAP_HEIGHT; y++)
    {
        for (x = 0; x < INFCAVE_MAP_WIDTH; x++)
        {
            if (x >= INFCAVE_AREA_MIN && x < INFCAVE_AREA_MAX_X
             && y >= INFCAVE_AREA_MIN && y < INFCAVE_AREA_MAX_Y)
                continue;
            if (IsFloor(x, y))
                SetCell(x, y, INFCAVE_CELL_VOID);
        }
    }
}

// Two floor tiles touching only at a corner are walkable in neither direction,
// so the pair reads as a dead end that looks like a passage. Filling either
// elbow turns it into a real corner; which one is a coin flip so the repairs do
// not all lean the same way.
static bool32 FixDiagonalLinks(rng_value_t *rng)
{
    u32 x, y;
    bool32 changed = FALSE;

    for (y = INFCAVE_AREA_MIN; y < INFCAVE_AREA_MAX_Y - 1; y++)
    {
        for (x = INFCAVE_AREA_MIN; x < INFCAVE_AREA_MAX_X - 1; x++)
        {
            bool32 nw = IsFloor(x, y);
            bool32 ne = IsFloor(x + 1, y);
            bool32 sw = IsFloor(x, y + 1);
            bool32 se = IsFloor(x + 1, y + 1);

            if (nw && se && !ne && !sw)
            {
                if (InfCave_Rand(rng) & 1)
                    SetCell(x + 1, y, INFCAVE_CELL_FLOOR);
                else
                    SetCell(x, y + 1, INFCAVE_CELL_FLOOR);
                changed = TRUE;
            }
            else if (ne && sw && !nw && !se)
            {
                if (InfCave_Rand(rng) & 1)
                    SetCell(x, y, INFCAVE_CELL_FLOOR);
                else
                    SetCell(x + 1, y + 1, INFCAVE_CELL_FLOOR);
                changed = TRUE;
            }
        }
    }
    return changed;
}

// A wall run one tile thick cannot be drawn: the same tile would have to carry
// the north edge, the fill and the south face at once. Opening it into floor
// also removes the single wall tile stranded inside a floor region, which is the
// same case with floor on all four sides.
static bool32 OpenThinWallRuns(void)
{
    u32 x, y;
    bool32 changed = FALSE;

    for (y = INFCAVE_AREA_MIN; y < INFCAVE_AREA_MAX_Y; y++)
    {
        for (x = INFCAVE_AREA_MIN; x < INFCAVE_AREA_MAX_X; x++)
        {
            if (IsFloor(x, y))
                continue;
            if ((IsFloorSafe(x, y - 1) && IsFloorSafe(x, y + 1))
             || (IsFloorSafe(x - 1, y) && IsFloorSafe(x + 1, y)))
            {
                SetCell(x, y, INFCAVE_CELL_FLOOR);
                changed = TRUE;
            }
        }
    }
    return changed;
}

// Opens one side of every rim-next-to-face seam, which is what a wall band that
// steps down by a single row leaves behind. Which side gives way is a coin flip
// so the repairs do not all shave the same edge.
static bool32 FixRimFaceSeams(rng_value_t *rng)
{
    u32 x, y;
    bool32 changed = FALSE;

    for (y = INFCAVE_AREA_MIN; y < INFCAVE_AREA_MAX_Y; y++)
    {
        for (x = INFCAVE_AREA_MIN; x < INFCAVE_AREA_MAX_X - 1; x++)
        {
            if (!HasRimFaceSeam(x, y))
                continue;

            if (InfCave_Rand(rng) & 1)
                SetCell(x, y, INFCAVE_CELL_FLOOR);
            else
                SetCell(x + 1, y, INFCAVE_CELL_FLOOR);
            changed = TRUE;
        }
    }
    return changed;
}

// Repair sweeps run to a fixed point: each sweep adds floor, which can expose a
// fresh diagonal, thin run or rim seam one tile further out. Every sweep only
// turns wall into floor, so this can be re-run over a finished mask without
// undoing anything a later pass carved.
static void EnforceLegality(rng_value_t *rng)
{
    u32 pass;

    for (pass = 0; pass < INFCAVE_LEGALITY_PASSES; pass++)
    {
        bool32 changed = FALSE;

        changed |= FixDiagonalLinks(rng);
        changed |= OpenThinWallRuns();
        changed |= FixRimFaceSeams(rng);

        if (!changed)
            break;
    }
}

// Judges the repaired mask. Returns INFCAVE_FAULT_NONE when every rule holds.
static u32 MaskLegalityFault(void)
{
    u32 x, y;
    u32 floorCount = CountFloor();

    for (y = 0; y < INFCAVE_MAP_HEIGHT; y++)
    {
        for (x = 0; x < INFCAVE_MAP_WIDTH; x++)
        {
            if (!IsFloor(x, y))
                continue;
            if (x < INFCAVE_AREA_MIN || x >= INFCAVE_AREA_MAX_X
             || y < INFCAVE_AREA_MIN || y >= INFCAVE_AREA_MAX_Y)
                return INFCAVE_FAULT_MARGIN;
        }
    }

    for (y = INFCAVE_AREA_MIN; y < INFCAVE_AREA_MAX_Y; y++)
    {
        for (x = INFCAVE_AREA_MIN; x < INFCAVE_AREA_MAX_X; x++)
        {
            if (IsFloor(x, y))
            {
                // Both diagonals: floor reachable only through a shared corner.
                if (IsFloorSafe(x + 1, y + 1) && !IsFloorSafe(x + 1, y) && !IsFloorSafe(x, y + 1))
                    return INFCAVE_FAULT_DIAGONAL;
                if (IsFloorSafe(x + 1, y - 1) && !IsFloorSafe(x + 1, y) && !IsFloorSafe(x, y - 1))
                    return INFCAVE_FAULT_DIAGONAL;
                continue;
            }

            if ((IsFloorSafe(x, y - 1) && IsFloorSafe(x, y + 1))
             || (IsFloorSafe(x - 1, y) && IsFloorSafe(x + 1, y)))
                return INFCAVE_FAULT_THIN_WALL;

            if (x < INFCAVE_AREA_MAX_X - 1 && HasRimFaceSeam(x, y))
                return INFCAVE_FAULT_RIM_SEAM;
        }
    }

    if (floorCount < INFCAVE_MIN_FLOOR_TILES)
        return INFCAVE_FAULT_AREA;
    if (!IsFloorConnected(floorCount))
        return INFCAVE_FAULT_SPLIT;

    return INFCAVE_FAULT_NONE;
}

// Last resort when no seed produced a legal mask: one centred rectangle, which
// satisfies every rule by construction.
static void BuildEmergencyMask(void)
{
    ClearMask();
    CarveRect((INFCAVE_MAP_WIDTH - INFCAVE_EMERGENCY_W) / 2,
              (INFCAVE_MAP_HEIGHT - INFCAVE_EMERGENCY_H) / 2,
              INFCAVE_EMERGENCY_W, INFCAVE_EMERGENCY_H);

    sRooms[0].x = (INFCAVE_MAP_WIDTH - INFCAVE_EMERGENCY_W) / 2;
    sRooms[0].y = (INFCAVE_MAP_HEIGHT - INFCAVE_EMERGENCY_H) / 2;
    sRooms[0].w = INFCAVE_EMERGENCY_W;
    sRooms[0].h = INFCAVE_EMERGENCY_H;
    sRoomCount = 1;
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

// One generation attempt. attempt shifts the RNG salt, so a rejected mask is
// rebuilt from a different stream while the room's stored seed is untouched.
// Returns the fault the finished mask still has, or INFCAVE_FAULT_NONE.
static u32 TryBuildMask(u32 attempt)
{
    rng_value_t rng = InfCave_SeedRoomRng(INFCAVE_SALT_MASK + attempt);

    ClearMask();
    PlaceRooms(&rng);
    CarveCorridors(&rng);
    SmoothMask();
    PruneToLargestComponent();
    ClearMarginFloor();
    EnforceLegality(&rng);

    return MaskLegalityFault();
}

// The mask pass. Every attempt reads and writes sMask only; nothing here knows
// about metatiles. The wall shell runs after the mask is judged, since it only
// relabels void the player cannot reach.
static void BuildMask(void)
{
    u32 attempt;

    for (attempt = 0; attempt < INFCAVE_MASK_ATTEMPTS; attempt++)
    {
        u32 fault = TryBuildMask(attempt);

        if (fault == INFCAVE_FAULT_NONE)
            break;

#if INFCAVE_TRACE == TRUE
        DebugPrintf("InfCave mask attempt %d rejected, fault %d", attempt, fault);
#endif
    }

    if (attempt == INFCAVE_MASK_ATTEMPTS)
        BuildEmergencyMask();

    BuildWallShell();

#if INFCAVE_TRACE == TRUE
    TraceMask();
#endif
}

// Debug harness for the legality rules: rebuilds count consecutive room seeds
// from baseSeed and returns how many failed. The first failure's seed and fault
// go to firstBadSeed / firstFault when those are non-NULL. Only the first
// attempt of each seed is judged, so the retry path cannot hide a bad rule.
// This leaves sMask holding the last generated room; the live room is rebuilt
// from its seed on the next map load, so nothing on screen depends on it.
u32 InfCave_DebugValidateMask(u32 baseSeed, u32 count, u32 *firstBadSeed, u32 *firstFault)
{
    u32 savedSeed = gSaveBlock1Ptr->infinityCaveRun.roomSeed;
    u32 i, failures = 0;

    if (!AllocGrids())
        return 0;

    for (i = 0; i < count; i++)
    {
        u32 fault;

        gSaveBlock1Ptr->infinityCaveRun.roomSeed = baseSeed + i;
        fault = TryBuildMask(0);
        if (fault == INFCAVE_FAULT_NONE)
            continue;

        if (failures == 0)
        {
            if (firstBadSeed != NULL)
                *firstBadSeed = baseSeed + i;
            if (firstFault != NULL)
                *firstFault = fault;
        }
        failures++;
    }

    gSaveBlock1Ptr->infinityCaveRun.roomSeed = savedSeed;
    FreeGrids();
    return failures;
}

static const u8 sMaskFaultName_None[] = _("none");
static const u8 sMaskFaultName_Margin[] = _("margin");
static const u8 sMaskFaultName_Diagonal[] = _("diagonal");
static const u8 sMaskFaultName_ThinWall[] = _("thin wall");
static const u8 sMaskFaultName_Area[] = _("area");
static const u8 sMaskFaultName_Split[] = _("split");

// Indexed by enum InfCaveMaskFault, for the debug harness's report line.
static const u8 *const sMaskFaultNames[] =
{
    [INFCAVE_FAULT_NONE]      = sMaskFaultName_None,
    [INFCAVE_FAULT_MARGIN]    = sMaskFaultName_Margin,
    [INFCAVE_FAULT_DIAGONAL]  = sMaskFaultName_Diagonal,
    [INFCAVE_FAULT_THIN_WALL] = sMaskFaultName_ThinWall,
    [INFCAVE_FAULT_AREA]      = sMaskFaultName_Area,
    [INFCAVE_FAULT_SPLIT]     = sMaskFaultName_Split,
};

const u8 *InfCave_GetMaskFaultName(u32 fault)
{
    if (fault >= ARRAY_COUNT(sMaskFaultNames))
        return sMaskFaultNames[INFCAVE_FAULT_NONE];
    return sMaskFaultNames[fault];
}

static bool32 IsSamePatch(s32 x, s32 y, u32 value)
{
    if (x < 0 || x >= INFCAVE_MAP_WIDTH || y < 0 || y >= INFCAVE_MAP_HEIGHT)
        return FALSE;
    return sPatch[y][x] == value;
}

// Which of the nine blocks a patch cell draws. Patches are rectangles, so a cell
// is named by the sides that leave the rectangle and the opposite-side cases
// cannot arise.
static u32 PatchShapeAt(s32 x, s32 y)
{
    u32 value = sPatch[y][x];
    bool32 n = IsSamePatch(x, y - 1, value);
    bool32 s = IsSamePatch(x, y + 1, value);
    bool32 w = IsSamePatch(x - 1, y, value);
    bool32 e = IsSamePatch(x + 1, y, value);

    if (!n && !w)
        return INFCAVE_PATCH_NW;
    if (!n && !e)
        return INFCAVE_PATCH_NE;
    if (!n)
        return INFCAVE_PATCH_N;
    if (!s && !w)
        return INFCAVE_PATCH_SW;
    if (!s && !e)
        return INFCAVE_PATCH_SE;
    if (!s)
        return INFCAVE_PATCH_S;
    if (!w)
        return INFCAVE_PATCH_W;
    if (!e)
        return INFCAVE_PATCH_E;
    return INFCAVE_PATCH_FILL;
}

static u32 PatchRoleAt(s32 x, s32 y)
{
    return sInfCavePatch[sPatch[y][x] - 1].baseRole + PatchShapeAt(x, y);
}

// A block a walking player cannot enter: either it collides, or it sits at an
// elevation the floor does not reach.
static bool32 IsBlockOnFoot(u16 block)
{
    if ((block & MAPGRID_COLLISION_MASK) != 0)
        return TRUE;
    return (block & MAPGRID_ELEVATION_MASK) != (sTileRole[INFCAVE_ROLE_FLOOR_0] & MAPGRID_ELEVATION_MASK);
}

// Whether a prop blocks movement comes from the collision bits of its authored
// block, not from the table, so the placer can never disagree with what the key
// layout actually draws.
static bool32 IsDecorSolid(u32 index)
{
    return IsBlockOnFoot(sTileRole[sInfCaveDecor[index].role]);
}

// A tile a walking player can stand on: carved floor with no solid prop on it.
static bool32 IsWalkable(s32 x, s32 y)
{
    if (x >= 0 && x < INFCAVE_MAP_WIDTH && y >= 0 && y < INFCAVE_MAP_HEIGHT)
    {
        // A placed object holds its tile against the player as firmly as a wall,
        // so every connectivity test after the placement pass has to see it.
        if (sMask[y][x] & INFCAVE_FLAG_BLOCKED)
            return FALSE;

        // A set piece's cells carry no prop or patch data: what the player can
        // cross there is whatever the authored block says.
        if (IsReserved(x, y))
            return !IsBlockOnFoot(StampBlockAt(x, y));
    }

    if (!IsFloorSafe(x, y))
        return FALSE;
    if (sDecor[y][x] == 0)
        return TRUE;
    return !IsDecorSolid(sDecor[y][x] - 1);
}

// Ring order around a tile, clockwise from north. Used by the chokepoint test,
// which needs the neighbours in adjacency order rather than as a raw 3x3 block.
static const s8 sRingOffsets[8][2] =
{
    { 0, -1 }, { 1, -1 }, { 1, 0 }, { 1, 1 }, { 0, 1 }, { -1, 1 }, { -1, 0 }, { -1, -1 },
};

// TRUE when the walkable tiles around (x, y) form more than one run, i.e.
// blocking this tile would cut the local paths apart. Solid props already down
// count as blocked. The legality pass removed diagonal-only links, so counting
// runs around the ring is enough; no flood fill is needed per candidate tile.
static bool32 IsChokepoint(s32 x, s32 y)
{
    bool32 prev = IsWalkable(x + sRingOffsets[7][0], y + sRingOffsets[7][1]);
    u32 i, runs = 0;

    for (i = 0; i < ARRAY_COUNT(sRingOffsets); i++)
    {
        bool32 cur = IsWalkable(x + sRingOffsets[i][0], y + sRingOffsets[i][1]);

        if (cur && !prev)
            runs++;
        prev = cur;
    }
    return runs > 1;
}

// Walkable tiles inside the carved area only, which is the same area the fill
// below visits.
static u32 CountWalkable(void)
{
    u32 x, y, count = 0;

    for (y = INFCAVE_AREA_MIN; y < INFCAVE_AREA_MAX_Y; y++)
    {
        for (x = INFCAVE_AREA_MIN; x < INFCAVE_AREA_MAX_X; x++)
        {
            if (IsWalkable(x, y))
                count++;
        }
    }
    return count;
}

// Flood fill over walkable tiles only, so solid props block it. Same alternating
// sweep as FloodFillComponent, which walks plain floor and cannot see props.
static u32 FloodFillWalkable(u32 sx, u32 sy)
{
    bool32 changed = TRUE;
    u32 x, y, count = 0;

    sMask[sy][sx] |= INFCAVE_FLAG_COMPONENT;

    while (changed)
    {
        changed = FALSE;

        for (y = INFCAVE_AREA_MIN; y < INFCAVE_AREA_MAX_Y; y++)
        {
            for (x = INFCAVE_AREA_MIN; x < INFCAVE_AREA_MAX_X; x++)
            {
                if (!IsWalkable(x, y) || (sMask[y][x] & INFCAVE_FLAG_COMPONENT))
                    continue;
                if ((sMask[y][x - 1] & INFCAVE_FLAG_COMPONENT) || (sMask[y - 1][x] & INFCAVE_FLAG_COMPONENT))
                {
                    sMask[y][x] |= INFCAVE_FLAG_COMPONENT;
                    changed = TRUE;
                }
            }
        }

        for (y = INFCAVE_AREA_MAX_Y; y-- > INFCAVE_AREA_MIN; )
        {
            for (x = INFCAVE_AREA_MAX_X; x-- > INFCAVE_AREA_MIN; )
            {
                if (!IsWalkable(x, y) || (sMask[y][x] & INFCAVE_FLAG_COMPONENT))
                    continue;
                if ((sMask[y][x + 1] & INFCAVE_FLAG_COMPONENT) || (sMask[y + 1][x] & INFCAVE_FLAG_COMPONENT))
                {
                    sMask[y][x] |= INFCAVE_FLAG_COMPONENT;
                    changed = TRUE;
                }
            }
        }
    }

    for (y = INFCAVE_AREA_MIN; y < INFCAVE_AREA_MAX_Y; y++)
    {
        for (x = INFCAVE_AREA_MIN; x < INFCAVE_AREA_MAX_X; x++)
        {
            if (sMask[y][x] & INFCAVE_FLAG_COMPONENT)
                count++;
        }
    }
    return count;
}

// TRUE when every walkable tile is still reachable from every other one with the
// solid props in place.
static bool32 IsRoomStillConnected(void)
{
    u32 x, y;
    u32 walkable = CountWalkable();

    if (walkable == 0)
        return FALSE;

    for (y = INFCAVE_AREA_MIN; y < INFCAVE_AREA_MAX_Y; y++)
    {
        for (x = INFCAVE_AREA_MIN; x < INFCAVE_AREA_MAX_X; x++)
        {
            u32 reached;

            if (!IsWalkable(x, y))
                continue;

            reached = FloodFillWalkable(x, y);
            ClearMaskFlags(INFCAVE_FLAG_COMPONENT);
            return reached == walkable;
        }
    }
    return FALSE;
}

static void RemoveSolidDecor(void)
{
    u32 x, y;

    for (y = 0; y < INFCAVE_MAP_HEIGHT; y++)
    {
        for (x = 0; x < INFCAVE_MAP_WIDTH; x++)
        {
            if (sDecor[y][x] != 0 && IsDecorSolid(sDecor[y][x] - 1))
                sDecor[y][x] = 0;
        }
    }
}

// A patch and the one-tile floor ring around it must all be plain floor: the
// ring keeps a patch off the walls, so the wall shapes stay correct and every
// patch edge has floor to sit against.
static bool32 PatchRectFree(u32 x, u32 y, u32 w, u32 h)
{
    s32 cx, cy;

    for (cy = (s32)y - 1; cy <= (s32)(y + h); cy++)
    {
        for (cx = (s32)x - 1; cx <= (s32)(x + w); cx++)
        {
            if (cx < INFCAVE_AREA_MIN || cx >= INFCAVE_AREA_MAX_X
             || cy < INFCAVE_AREA_MIN || cy >= INFCAVE_AREA_MAX_Y)
                return FALSE;
            if (!IsFloor(cx, cy) || sPatch[cy][cx] != 0)
                return FALSE;
            if (sMask[cy][cx] & INFCAVE_FLAG_NO_DECOR)
                return FALSE;
        }
    }
    return TRUE;
}

static void FillPatchRect(u32 x, u32 y, u32 w, u32 h, u32 value)
{
    u32 cx, cy;

    for (cy = y; cy < y + h; cy++)
    {
        for (cx = x; cx < x + w; cx++)
            sPatch[cy][cx] = value;
    }
}

// Weighted pick among the materials unlocked at this depth, or -1 when none is.
static s32 RollPatch(rng_value_t *rng, u32 depth)
{
    u32 total = 0, i;
    u32 roll;

    for (i = 0; i < ARRAY_COUNT(sInfCavePatch); i++)
    {
        if (depth >= sInfCavePatch[i].minDepth)
            total += sInfCavePatch[i].weight;
    }

    if (total == 0)
        return -1;

    roll = InfCave_RandRange(rng, 0, total - 1);
    for (i = 0; i < ARRAY_COUNT(sInfCavePatch); i++)
    {
        if (depth < sInfCavePatch[i].minDepth)
            continue;
        if (roll < sInfCavePatch[i].weight)
            return i;
        roll -= sInfCavePatch[i].weight;
    }
    return -1;
}

// Lays rectangles of a second ground material over the floor. Patches are
// walkable, so a rectangle only has to clear the placement rules; it can never
// cut the room in two.
static void PlacePatches(void)
{
    rng_value_t rng = InfCave_SeedRoomRng(INFCAVE_SALT_PTCH);
    u32 depth = InfCave_GetDepth();
    u32 tries = INFCAVE_PATCH_TRIES;
    u32 placed = 0;

    while (placed < INFCAVE_PATCH_MAX_COUNT && tries-- != 0)
    {
        u32 w = InfCave_RandRange(&rng, INFCAVE_PATCH_MIN_W, INFCAVE_PATCH_MAX_W);
        u32 h = InfCave_RandRange(&rng, INFCAVE_PATCH_MIN_H, INFCAVE_PATCH_MAX_H);
        u32 x = InfCave_RandRange(&rng, INFCAVE_AREA_MIN + 1, INFCAVE_AREA_MAX_X - w - 2);
        u32 y = InfCave_RandRange(&rng, INFCAVE_AREA_MIN + 1, INFCAVE_AREA_MAX_Y - h - 2);
        s32 index = RollPatch(&rng, depth);

        if (index < 0)
            return;
        if (!PatchRectFree(x, y, w, h))
            continue;

        FillPatchRect(x, y, w, h, index + 1);
        placed++;
    }

#if INFCAVE_TRACE == TRUE
    DebugPrintf("InfCave patches placed %d", placed);
#endif
}

// Spacing rule, applied from both sides: a candidate must clear its own
// minDistance and the minDistance of every prop already down.
static bool32 DecorSpacingOk(u32 x, u32 y, u32 minDistance)
{
    s32 dx, dy;

    for (dy = -INFCAVE_DECOR_MAX_DISTANCE; dy <= INFCAVE_DECOR_MAX_DISTANCE; dy++)
    {
        for (dx = -INFCAVE_DECOR_MAX_DISTANCE; dx <= INFCAVE_DECOR_MAX_DISTANCE; dx++)
        {
            s32 nx = (s32)x + dx, ny = (s32)y + dy;
            u32 distance = max(abs(dx), abs(dy));
            u32 other;

            if (nx < 0 || nx >= INFCAVE_MAP_WIDTH || ny < 0 || ny >= INFCAVE_MAP_HEIGHT)
                continue;
            if (sDecor[ny][nx] == 0)
                continue;

            other = sInfCaveDecor[sDecor[ny][nx] - 1].minDistance;
            if (distance < minDistance || distance < other)
                return FALSE;
        }
    }
    return TRUE;
}

static bool32 CanPlaceDecor(u32 index, u32 x, u32 y)
{
    const struct InfCaveDecor *decor = &sInfCaveDecor[index];

    if (CellKind(x, y) != INFCAVE_CELL_FLOOR || sDecor[y][x] != 0)
        return FALSE;

    // A prop's block draws its own cave floor under it, so it would cut a hole
    // in a patch's material.
    if (sPatch[y][x] != 0)
        return FALSE;

    // Pads and access corridors own their tiles: a prop there would hide the pad
    // art, and a solid one could seal the only way to the exit.
    if (sMask[y][x] & INFCAVE_FLAG_NO_DECOR)
        return FALSE;

    // A tall prop drawn against a wall hides the wall's face art, so it is kept
    // one tile clear of every wall on all four sides.
    if (decor->tall
     && (!IsOpenSafe(x, y - 1) || !IsOpenSafe(x, y + 1)
      || !IsOpenSafe(x - 1, y) || !IsOpenSafe(x + 1, y)))
        return FALSE;

    if (IsDecorSolid(index) && IsChokepoint(x, y))
        return FALSE;

    return DecorSpacingOk(x, y, decor->minDistance);
}

// Share of the room's floor that becomes props, rising with depth.
static u32 DecorTargetCount(u32 floorCount)
{
    u32 percent = INFCAVE_DECOR_PERCENT_BASE + InfCave_GetDepth() / INFCAVE_DECOR_DEPTH_PER_STEP;
    u32 count;

    if (percent > INFCAVE_DECOR_PERCENT_MAX)
        percent = INFCAVE_DECOR_PERCENT_MAX;

    count = floorCount * percent / 100;
    if (count > INFCAVE_DECOR_MAX_PROPS)
        count = INFCAVE_DECOR_MAX_PROPS;
    return count;
}

// Weighted pick among the props unlocked at this depth. Returns the table index,
// or -1 when nothing is available, which only happens at depth 0 if every entry
// gains a minDepth.
static s32 RollDecor(rng_value_t *rng, u32 depth)
{
    u32 total = 0, i;
    u32 roll;

    for (i = 0; i < ARRAY_COUNT(sInfCaveDecor); i++)
    {
        if (depth >= sInfCaveDecor[i].minDepth)
            total += sInfCaveDecor[i].weight;
    }

    if (total == 0)
        return -1;

    roll = InfCave_RandRange(rng, 0, total - 1);
    for (i = 0; i < ARRAY_COUNT(sInfCaveDecor); i++)
    {
        if (depth < sInfCaveDecor[i].minDepth)
            continue;
        if (roll < sInfCaveDecor[i].weight)
            return i;
        roll -= sInfCaveDecor[i].weight;
    }
    return -1;
}

// Scatters props over the finished mask. Candidates are rolled rather than
// swept, so the attempt budget bounds the cost; a prop that breaks a rule is
// dropped instead of relocated. Solid props are cleared wholesale if the room
// ends up split, which the per-tile chokepoint test makes rare.
static void DecorateRoom(void)
{
    rng_value_t rng = InfCave_SeedRoomRng(INFCAVE_SALT_DECO);
    u32 depth = InfCave_GetDepth();
    u32 target = DecorTargetCount(CountFloor());
    u32 tries = target * INFCAVE_DECOR_TRIES_PER_PROP;
    u32 placed = 0;
    bool32 anySolid = FALSE;

    while (placed < target && tries-- != 0)
    {
        u32 x = InfCave_RandRange(&rng, INFCAVE_AREA_MIN, INFCAVE_AREA_MAX_X - 1);
        u32 y = InfCave_RandRange(&rng, INFCAVE_AREA_MIN, INFCAVE_AREA_MAX_Y - 1);
        s32 index = RollDecor(&rng, depth);

        if (index < 0)
            return;
        if (!CanPlaceDecor(index, x, y))
            continue;

        sDecor[y][x] = index + 1;
        placed++;
        if (IsDecorSolid(index))
            anySolid = TRUE;
    }

    if (anySolid && !IsRoomStillConnected())
        RemoveSolidDecor();

#if INFCAVE_TRACE == TRUE
    DebugPrintf("InfCave decor placed %d of %d target", placed, target);
#endif
}

// Stamps the room type's authored chunk over the largest room that can hold it,
// centred, and marks its cells RESERVED so the patch, decoration and autotile
// passes leave them alone. Paths survive the stamp because a piece's outer ring
// is walkable, so every corridor that met the room still leads past the piece.
static void StampSetPiece(void)
{
    // Only the ring assert reads the piece's blocks here; the stamp itself is
    // drawn by the autotile pass straight from the layout.
    const struct MapLayout *layout UNUSED;
    u32 i, lx, ly;
    u32 bestArea = 0;
    s32 host = -1;

    if (sStamp.layoutId == INFCAVE_PIECE_NONE)
        return;

    for (i = 0; i < sRoomCount; i++)
    {
        u32 area = sRooms[i].w * sRooms[i].h;

        if (sRooms[i].w < sStamp.w || sRooms[i].h < sStamp.h)
            continue;
        if (host >= 0 && area <= bestArea)
            continue;

        host = i;
        bestArea = area;
    }

    // Nothing fits only if the reserved room was crowded out, which leaves the
    // room type without its centrepiece rather than with a broken one.
    if (host < 0)
    {
#if INFCAVE_TRACE == TRUE
        DebugPrintf("InfCave set piece %d has no host room", sStamp.layoutId);
#endif
        sStamp.layoutId = INFCAVE_PIECE_NONE;
        return;
    }

    layout = GetMapLayout(sStamp.layoutId);
    sStampHost = host;
    sStamp.x = sRooms[host].x + (sRooms[host].w - sStamp.w) / 2;
    sStamp.y = sRooms[host].y + (sRooms[host].h - sStamp.h) / 2;

    for (ly = 0; ly < sStamp.h; ly++)
    {
        for (lx = 0; lx < sStamp.w; lx++)
        {
            u32 x = sStamp.x + lx, y = sStamp.y + ly;

            // The ring carries every path around the piece, so a blocking block
            // there would strand whatever the interior seals off.
            if (lx == 0 || ly == 0 || lx == sStamp.w - 1u || ly == sStamp.h - 1u)
                AGB_ASSERT(!IsBlockOnFoot(layout->map[ly * layout->width + lx]));

            SetCell(x, y, INFCAVE_CELL_RESERVED);
            sDecor[y][x] = 0;
            sPatch[y][x] = 0;
        }
    }

#if INFCAVE_TRACE == TRUE
    DebugPrintf("InfCave set piece %d stamped at %d,%d in room %d",
                sStamp.layoutId, sStamp.x, sStamp.y, host);
#endif
}

// Four-way steps, in the order the distance fill relaxes them.
static const s8 sStepOffsets[4][2] = { { 0, -1 }, { 0, 1 }, { -1, 0 }, { 1, 0 } };

// Lowers (x, y) to one step more than its nearest neighbour. Returns TRUE when
// it changed something, which is what drives the sweeps to a fixed point.
static bool32 RelaxDistance(u32 x, u32 y)
{
    u32 best = INFCAVE_DIST_UNREACHED;
    u32 i;

    if (!IsWalkable(x, y))
        return FALSE;

    for (i = 0; i < ARRAY_COUNT(sStepOffsets); i++)
    {
        u32 dist = sDist[y + sStepOffsets[i][1]][x + sStepOffsets[i][0]];

        if (dist < best)
            best = dist;
    }

    if (best >= INFCAVE_DIST_MAX || sDist[y][x] <= best + 1)
        return FALSE;

    sDist[y][x] = best + 1;
    return TRUE;
}

// Step distance from (sx, sy) over every tile a walking player can cross, by the
// same alternating sweeps the flood fills use: each sweep carries a distance one
// row further, so this converges without a queue in EWRAM. Set-piece cells are
// crossed, since their ring is walkable authored art.
static void FillStepDistance(u32 sx, u32 sy)
{
    bool32 changed = TRUE;

    memset(sDist, INFCAVE_DIST_UNREACHED, sizeof(sDist));
    sDist[sy][sx] = 0;

    while (changed)
    {
        u32 x, y;

        changed = FALSE;

        for (y = INFCAVE_AREA_MIN; y < INFCAVE_AREA_MAX_Y; y++)
        {
            for (x = INFCAVE_AREA_MIN; x < INFCAVE_AREA_MAX_X; x++)
            {
                if (RelaxDistance(x, y))
                    changed = TRUE;
            }
        }

        for (y = INFCAVE_AREA_MAX_Y; y-- > INFCAVE_AREA_MIN; )
        {
            for (x = INFCAVE_AREA_MAX_X; x-- > INFCAVE_AREA_MIN; )
            {
                if (RelaxDistance(x, y))
                    changed = TRUE;
            }
        }
    }
}

// A tile a pad may be drawn on: plain floor the player can stand on that nothing
// else has claimed. Set-piece cells are excluded, since a pad there would cover
// authored art.
static bool32 IsPadTile(u32 x, u32 y)
{
    if (x < INFCAVE_AREA_MIN || x >= INFCAVE_AREA_MAX_X
     || y < INFCAVE_AREA_MIN || y >= INFCAVE_AREA_MAX_Y)
        return FALSE;
    if (CellKind(x, y) != INFCAVE_CELL_FLOOR)
        return FALSE;
    if (sDecor[y][x] != 0 || sPatch[y][x] != 0)
        return FALSE;
    if (sMask[y][x] & INFCAVE_FLAG_NO_DECOR)
        return FALSE;

    return IsWalkable(x, y);
}

// Nearest pad tile to (cx, cy), searched outward in square rings, so a room whose
// middle is covered by a set piece still yields a tile beside it.
static bool32 FindPadTileNear(u32 cx, u32 cy, u8 *ox, u8 *oy)
{
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
                if (x < 0 || y < 0 || !IsPadTile(x, y))
                    continue;

                *ox = x;
                *oy = y;
                return TRUE;
            }
        }
    }
    return FALSE;
}

// Puts the entrance in a rolled room and the exit on the walkable tile farthest
// from it by step distance, so a descent always crosses the room. Both pads and
// the ring around the exit are closed to props and patches: the ladder is drawn on
// the exit pad, and a solid prop beside it could seal it off.
static void PlaceEntranceExit(void)
{
    rng_value_t rng = InfCave_SeedRoomRng(INFCAVE_SALT_EXIT);
    u32 cx = INFCAVE_MAP_WIDTH / 2, cy = INFCAVE_MAP_HEIGHT / 2;
    u32 x, y, i, best = 0;

    // The emergency mask carves no rooms at all, and the set piece's host room is
    // wall-to-wall authored art; both fall back to the canvas centre, from which
    // the ring search walks out to a tile that can host a pad.
    if (sRoomCount != 0)
    {
        u32 pick = InfCave_RandRange(&rng, 0, sRoomCount - 1);

        for (i = 0; i < sRoomCount; i++)
        {
            u32 room = (pick + i) % sRoomCount;

            if ((s32)room == sStampHost)
                continue;

            cx = sRooms[room].x + sRooms[room].w / 2;
            cy = sRooms[room].y + sRooms[room].h / 2;
            break;
        }
    }

    sEntranceX = sExitX = cx;
    sEntranceY = sExitY = cy;

    // No tile on the canvas can host a pad, which needs a mask with no walkable
    // floor at all. The pads stay at the centre rather than at stale coordinates.
    if (!FindPadTileNear(cx, cy, &sEntranceX, &sEntranceY))
        return;

    FillStepDistance(sEntranceX, sEntranceY);

    // The entrance is distance 0, so it can never win this scan and the two pads
    // only share a tile when the entrance is the room's one walkable tile.
    sExitX = sEntranceX;
    sExitY = sEntranceY;
    for (y = INFCAVE_AREA_MIN; y < INFCAVE_AREA_MAX_Y; y++)
    {
        for (x = INFCAVE_AREA_MIN; x < INFCAVE_AREA_MAX_X; x++)
        {
            if (sDist[y][x] == INFCAVE_DIST_UNREACHED || sDist[y][x] <= best)
                continue;
            if (!IsPadTile(x, y))
                continue;

            best = sDist[y][x];
            sExitX = x;
            sExitY = y;
        }
    }

    sMask[sEntranceY][sEntranceX] |= INFCAVE_FLAG_NO_DECOR;
    sMask[sExitY][sExitX] |= INFCAVE_FLAG_NO_DECOR;
    for (i = 0; i < ARRAY_COUNT(sRingOffsets); i++)
    {
        u32 nx = sExitX + sRingOffsets[i][0], ny = sExitY + sRingOffsets[i][1];

        if (IsFloor(nx, ny))
            sMask[ny][nx] |= INFCAVE_FLAG_NO_DECOR;
    }

#if INFCAVE_TRACE == TRUE
    DebugPrintf("InfCave entrance %d,%d exit %d,%d distance %d",
                sEntranceX, sEntranceY, sExitX, sExitY, best);
#endif
}

// TRUE when (x, y) is this room's exit pad, in layout coordinates. The step
// trigger that descends reads this, since the pad is a metatile rather than an
// object or a coord event the map header could hold.
bool32 InfCave_IsExitTile(u32 x, u32 y)
{
    return x == sExitX && y == sExitY;
}

// --- Object placement -------------------------------------------------------

// Placed trainers, in local id order. Positions live here rather than in the
// save block's templates so the legality harness can run the pass without
// touching the live room's objects.
struct InfCaveNpc
{
    u8 x;
    u8 y;
    u8 facing; // index into sInfCaveFacings
};

static EWRAM_DATA struct InfCaveNpc sNpcs[INFCAVE_MAX_TRAINERS] = {0};
static EWRAM_DATA u8 sNpcCount = 0;

// Object event templates the generator wrote. The spawner reads this instead of
// the map header's object count, which is zero: the room declares no objects of
// its own.
static EWRAM_DATA u8 sObjectCount = 0;

// The template layout the placer and the room's scripts both assume: trainer slot
// n owns index n, and the whole set fits the room's object budget.
STATIC_ASSERT(INFCAVE_MAX_TRAINERS <= INFCAVE_MAX_OBJECTS, sInfCaveObjectBudget);
STATIC_ASSERT(INFCAVE_MAX_OBJECTS <= OBJECT_EVENT_TEMPLATES_COUNT, sInfCaveTemplateBudget);

// Trainer count the harness pins the pass to, or 0 to use the room's own roll.
static EWRAM_DATA u8 sNpcCountOverride = 0;

// Connectivity tests left in this room's budget. Each is one flood fill, so this
// is what bounds the pass's cost.
static EWRAM_DATA u8 sConnectChecks = 0;

// Facings a placed trainer may take, with the step its sight line walks. A fixed
// facing is mandatory: a wandering NPC would drift into a corridor, where the
// player has no way past it.
static const struct
{
    u8 movementType;
    s8 dx;
    s8 dy;
} sInfCaveFacings[] =
{
    { MOVEMENT_TYPE_FACE_DOWN,   0,  1 },
    { MOVEMENT_TYPE_FACE_UP,     0, -1 },
    { MOVEMENT_TYPE_FACE_LEFT,  -1,  0 },
    { MOVEMENT_TYPE_FACE_RIGHT,  1,  0 },
};

// Elevation every generated object stands at, taken from the key layout's floor
// block rather than a literal, so retheming the cave onto a tileset with another
// ground elevation needs no code change.
static u8 FloorElevation(void)
{
    return UNPACK_ELEVATION(sTileRole[INFCAVE_ROLE_FLOOR_0]);
}

// TRUE when the entrance pad can still reach the exit pad with every blocked tile
// in place. One flood fill per call, spent from the room's budget by the caller.
static bool32 EntranceReachesExit(void)
{
    bool32 linked;

    if (!IsWalkable(sEntranceX, sEntranceY) || !IsWalkable(sExitX, sExitY))
        return FALSE;

    FloodFillWalkable(sEntranceX, sEntranceY);
    linked = (sMask[sExitY][sExitX] & INFCAVE_FLAG_COMPONENT) != 0;
    ClearMaskFlags(INFCAVE_FLAG_COMPONENT);
    return linked;
}

// TRUE when the room rectangle is wide enough in both axes to hold a trainer and
// is not the one the set piece owns, whose cells are authored art.
static bool32 IsNpcRoom(u32 room)
{
    if ((s32)room == sStampHost)
        return FALSE;

    return sRooms[room].w >= INFCAVE_NPC_ROOM_MIN_SIDE
        && sRooms[room].h >= INFCAVE_NPC_ROOM_MIN_SIDE;
}

// TRUE when (x, y) lies inside a room region rather than a corridor. Corridors
// are carved outside every room rectangle, so the rectangles are the whole test.
static bool32 IsInNpcRoom(u32 x, u32 y)
{
    u32 room;

    for (room = 0; room < sRoomCount; room++)
    {
        if (!IsNpcRoom(room))
            continue;
        if (x >= sRooms[room].x && x < sRooms[room].x + sRooms[room].w
         && y >= sRooms[room].y && y < sRooms[room].y + sRooms[room].h)
            return TRUE;
    }
    return FALSE;
}

static u32 ChebyshevDistance(s32 ax, s32 ay, s32 bx, s32 by)
{
    return max(abs(ax - bx), abs(ay - by));
}

// Trainers stand well apart from each other and well back from the arrival pad,
// so the player is never dropped into a sight line on entering the room.
static bool32 NpcSpacingOk(u32 x, u32 y)
{
    u32 i;

    if (ChebyshevDistance(x, y, sEntranceX, sEntranceY) < INFCAVE_NPC_FROM_ENTRANCE)
        return FALSE;

    for (i = 0; i < sNpcCount; i++)
    {
        if (ChebyshevDistance(x, y, sNpcs[i].x, sNpcs[i].y) < INFCAVE_NPC_MIN_APART)
            return FALSE;
    }
    return TRUE;
}

// Everything a candidate tile must satisfy before a connectivity test is spent on
// it. IsChokepoint is the cheap stand-in for the articulation test: a tile whose
// walkable neighbours form one run cannot be the only link between two halves of
// the room, so the flood fill below only has to judge what survives this.
static bool32 IsNpcCandidate(u32 x, u32 y)
{
    if (CellKind(x, y) != INFCAVE_CELL_FLOOR)
        return FALSE;
    // Pads, the ring around the exit and the tiles already taken by an object.
    if (sMask[y][x] & (INFCAVE_FLAG_NO_DECOR | INFCAVE_FLAG_BLOCKED))
        return FALSE;
    if (sDecor[y][x] != 0)
        return FALSE;
    if (!IsInNpcRoom(x, y))
        return FALSE;
    if (!IsWalkable(x, y))
        return FALSE;
    if (IsChokepoint(x, y))
        return FALSE;

    return NpcSpacingOk(x, y);
}

// The tiles a trainer at (x, y) would see: the straight run ahead, stopping at
// the first tile the player cannot cross. Emerald trainers only notice the player
// dead ahead, so the sight cone is this line. Built into the caller's buffer
// before anything is blocked, so the same tiles can be marked and unmarked.
static u32 BuildSightLine(u32 x, u32 y, u32 facing, u8 *lineX, u8 *lineY)
{
    u32 i, count = 0;

    for (i = 1; i <= INFCAVE_NPC_SIGHT; i++)
    {
        s32 nx = (s32)x + sInfCaveFacings[facing].dx * (s32)i;
        s32 ny = (s32)y + sInfCaveFacings[facing].dy * (s32)i;

        if (!IsWalkable(nx, ny))
            break;

        lineX[count] = nx;
        lineY[count] = ny;
        count++;
    }
    return count;
}

static void SetLineBlocked(const u8 *lineX, const u8 *lineY, u32 count, bool32 blocked)
{
    u32 i;

    for (i = 0; i < count; i++)
    {
        if (blocked)
            sMask[lineY[i]][lineX[i]] |= INFCAVE_FLAG_BLOCKED;
        else
            sMask[lineY[i]][lineX[i]] &= ~INFCAVE_FLAG_BLOCKED;
    }
}

// Tries to stand a trainer on (x, y). Facings are walked from a rolled start, and
// one is accepted only when the room still links the entrance to the exit both
// with the trainer's own tile blocked and with its sight line blocked on top: a
// sight line lying across the only route would force the battle rather than offer
// it. The trainer's tile stays blocked on success, which is what makes the test
// cumulative as later trainers are added.
static bool32 TryPlaceNpc(u32 x, u32 y, rng_value_t *rng)
{
    u32 start = InfCave_RandRange(rng, 0, ARRAY_COUNT(sInfCaveFacings) - 1);
    u32 i;

    if (sConnectChecks == 0)
        return FALSE;

    sMask[y][x] |= INFCAVE_FLAG_BLOCKED;
    sConnectChecks--;
    if (!EntranceReachesExit())
    {
        sMask[y][x] &= ~INFCAVE_FLAG_BLOCKED;
        return FALSE;
    }

    for (i = 0; i < ARRAY_COUNT(sInfCaveFacings); i++)
    {
        u32 facing = (start + i) % ARRAY_COUNT(sInfCaveFacings);
        u8 lineX[INFCAVE_NPC_SIGHT], lineY[INFCAVE_NPC_SIGHT];
        u32 count = BuildSightLine(x, y, facing, lineX, lineY);
        bool32 linked;

        // A facing into a wall sees nothing, so the trainer could never challenge
        // the player from it.
        if (count == 0)
            continue;
        if (sConnectChecks == 0)
            break;

        SetLineBlocked(lineX, lineY, count, TRUE);
        linked = EntranceReachesExit();
        SetLineBlocked(lineX, lineY, count, FALSE);
        sConnectChecks--;

        if (!linked)
            continue;

        sNpcs[sNpcCount].x = x;
        sNpcs[sNpcCount].y = y;
        sNpcs[sNpcCount].facing = facing;
        sNpcCount++;
        return TRUE;
    }

    sMask[y][x] &= ~INFCAVE_FLAG_BLOCKED;
    return FALSE;
}

// Index into sInfCaveFacings for the step that points from (fromX, fromY) toward
// (toX, toY) along whichever axis separates them further.
static u32 FacingTowards(s32 fromX, s32 fromY, s32 toX, s32 toY)
{
    s32 dx = toX - fromX, dy = toY - fromY;
    s8 stepX = 0, stepY = 0;
    u32 i;

    if (abs(dx) > abs(dy))
        stepX = dx > 0 ? 1 : -1;
    else
        stepY = dy > 0 ? 1 : -1;

    for (i = 0; i < ARRAY_COUNT(sInfCaveFacings); i++)
    {
        if (sInfCaveFacings[i].dx == stepX && sInfCaveFacings[i].dy == stepY)
            return i;
    }
    return 0;
}

// Nearest tile to the stamped piece's centre that a boss can stand on, searched
// outward in square rings and confined to the stamp rect, so the boss always
// ends up on the arena rather than beside it. The arena's cells are RESERVED
// authored art, which IsWalkable already judges from the stamped block.
static bool32 FindArenaTileNear(u32 cx, u32 cy, u8 *ox, u8 *oy)
{
    u32 radius;

    for (radius = 0; radius < INFCAVE_PIECE_MAX_W + INFCAVE_PIECE_MAX_H; radius++)
    {
        s32 dx, dy;

        for (dy = -(s32)radius; dy <= (s32)radius; dy++)
        {
            for (dx = -(s32)radius; dx <= (s32)radius; dx++)
            {
                s32 x = (s32)cx + dx, y = (s32)cy + dy;

                if (abs(dx) != (s32)radius && abs(dy) != (s32)radius)
                    continue;
                if (x < (s32)sStamp.x || x >= (s32)(sStamp.x + sStamp.w))
                    continue;
                if (y < (s32)sStamp.y || y >= (s32)(sStamp.y + sStamp.h))
                    continue;
                if (sMask[y][x] & (INFCAVE_FLAG_NO_DECOR | INFCAVE_FLAG_BLOCKED))
                    continue;
                if (sDecor[y][x] != 0 || !IsWalkable(x, y))
                    continue;

                *ox = x;
                *oy = y;
                return TRUE;
            }
        }
    }
    return FALSE;
}

// Stands a boss room's single trainer on its arena's anchor tile, facing the
// entrance so it looks at the player crossing the room. The boss never notices
// the player on its own; the fight starts when the player walks up and talks, and
// the room's sealed ladder is what stops the arena being skipped. Returns FALSE
// when the room stamped no arena, the arena has no tile to stand on, or the boss
// would cut the room in two; the caller then falls back to the ordinary placer.
static bool32 PlaceBoss(void)
{
    u8 bx, by;

    if (sStamp.layoutId == INFCAVE_PIECE_NONE)
        return FALSE;
    if (sConnectChecks == 0)
        return FALSE;
    if (!FindArenaTileNear(sStamp.x + sStamp.w / 2, sStamp.y + sStamp.h / 2, &bx, &by))
        return FALSE;

    sMask[by][bx] |= INFCAVE_FLAG_BLOCKED;
    sConnectChecks--;
    if (!EntranceReachesExit())
    {
        sMask[by][bx] &= ~INFCAVE_FLAG_BLOCKED;
        return FALSE;
    }

    sNpcs[0].x = bx;
    sNpcs[0].y = by;
    sNpcs[0].facing = FacingTowards(bx, by, sEntranceX, sEntranceY);
    sNpcCount = 1;

#if INFCAVE_TRACE == TRUE
    DebugPrintf("InfCave boss at %d,%d facing %d", bx, by, sNpcs[0].facing);
#endif
    return TRUE;
}

// Rolls a tile inside one of the rooms that may host a trainer. Rolling per room
// rather than over the whole canvas keeps the attempt budget meaningful in a room
// whose floor is mostly corridor.
static bool32 RollNpcTile(rng_value_t *rng, u32 *ox, u32 *oy)
{
    u32 pick;
    u32 i;

    if (sRoomCount == 0)
        return FALSE;

    pick = InfCave_RandRange(rng, 0, sRoomCount - 1);
    for (i = 0; i < sRoomCount; i++)
    {
        u32 room = (pick + i) % sRoomCount;

        if (!IsNpcRoom(room))
            continue;

        *ox = InfCave_RandRange(rng, sRooms[room].x, sRooms[room].x + sRooms[room].w - 1);
        *oy = InfCave_RandRange(rng, sRooms[room].y, sRooms[room].y + sRooms[room].h - 1);
        return TRUE;
    }
    return FALSE;
}

// Stands the room's rolled trainers on its floor. Slots are filled in order and
// the pass stops at the first slot it cannot place, so a slot's local id is
// always its index: a gap would leave the room's scripts addressing the wrong
// NPC. A room that places fewer trainers than it rolled is still playable.
static void PlaceTrainers(void)
{
    rng_value_t rng = InfCave_SeedRoomRng(INFCAVE_SALT_NPCS);
    u32 wanted = sNpcCountOverride != 0 ? sNpcCountOverride : InfCave_RollTrainerCount();
    u32 slot;

    sNpcCount = 0;
    sConnectChecks = INFCAVE_NPC_CONNECT_CHECKS;

    // The harness pins a count to exercise the rolled placer, so it keeps taking
    // the path below even in a boss room.
    if (sNpcCountOverride == 0 && InfCave_GetRoomType() == INFCAVE_ROOM_BOSS && PlaceBoss())
        return;

    if (wanted > INFCAVE_MAX_TRAINERS)
        wanted = INFCAVE_MAX_TRAINERS;
    if (wanted > INFCAVE_MAX_OBJECTS)
        wanted = INFCAVE_MAX_OBJECTS;

    for (slot = 0; slot < wanted; slot++)
    {
        u32 tries;
        bool32 placed = FALSE;

        for (tries = 0; tries < INFCAVE_NPC_TRIES_PER_SLOT && !placed; tries++)
        {
            u32 x, y;

            if (!RollNpcTile(&rng, &x, &y))
                return;
            if (!IsNpcCandidate(x, y))
                continue;

            placed = TryPlaceNpc(x, y, &rng);
        }

        if (!placed)
            break;
    }

#if INFCAVE_TRACE == TRUE
    DebugPrintf("InfCave placed %d of %d trainers, %d checks left",
                sNpcCount, wanted, sConnectChecks);
#endif
}

// Scripts the generated objects run. A trainer's is per slot, because
// trainerbattle takes a literal stub id and the sight-approach code reads that id
// straight out of the script.
extern const u8 InfinityCave_EventScript_Boss[];
extern const u8 InfinityCave_EventScript_Trainer0[];
extern const u8 InfinityCave_EventScript_Trainer1[];
extern const u8 InfinityCave_EventScript_Trainer2[];
extern const u8 InfinityCave_EventScript_Trainer3[];
extern const u8 InfinityCave_EventScript_Trainer4[];
extern const u8 InfinityCave_EventScript_Trainer5[];
extern const u8 InfinityCave_EventScript_Trainer6[];
extern const u8 InfinityCave_EventScript_Trainer7[];

static const u8 *const sInfCaveTrainerScripts[INFCAVE_MAX_TRAINERS] =
{
    InfinityCave_EventScript_Trainer0,
    InfinityCave_EventScript_Trainer1,
    InfinityCave_EventScript_Trainer2,
    InfinityCave_EventScript_Trainer3,
    InfinityCave_EventScript_Trainer4,
    InfinityCave_EventScript_Trainer5,
    InfinityCave_EventScript_Trainer6,
    InfinityCave_EventScript_Trainer7,
};

static const u8 *ScriptForLocalId(u32 localId)
{
    u32 slot = localId - INFCAVE_LOCALID_TRAINER_0;

    if (localId < INFCAVE_LOCALID_TRAINER_0 || slot >= INFCAVE_MAX_TRAINERS)
        return NULL;
    // A boss room stands one trainer, in slot 0, and it runs the talk-to script
    // rather than the plain sight battle.
    if (slot == 0 && InfCave_GetRoomType() == INFCAVE_ROOM_BOSS)
        return InfinityCave_EventScript_Boss;

    return sInfCaveTrainerScripts[slot];
}

// TRUE for the slot a boss room's boss stands in. A boss does not notice the
// player: it holds no sight line, so the fight starts only when the player walks
// up to it and talks.
static bool32 IsBossSlot(u32 slot)
{
    return slot == 0 && InfCave_GetRoomType() == INFCAVE_ROOM_BOSS;
}

// Writes the placed trainers into the save block's templates. Slot n owns index n,
// and every field is written here so a template never carries anything from the
// room before. Templates past the room's own objects are blanked: the spawner is bounded by sObjectCount, but a
// stale template left addressable by local id would answer for an NPC that is no
// longer there.
static void WriteTrainerTemplates(void)
{
    u32 i;

    for (i = 0; i < sNpcCount; i++)
    {
        struct ObjectEventTemplate *template = &gSaveBlock1Ptr->objectEventTemplates[i];

        memset(template, 0, sizeof(*template));
        template->localId = INFCAVE_LOCALID_TRAINER_0 + i;
        template->graphicsId = InfCave_BuildTrainer(i);
        template->kind = OBJ_KIND_NORMAL;
        template->x = sNpcs[i].x;
        template->y = sNpcs[i].y;
        template->elevation = FloorElevation();
        template->movementType = sInfCaveFacings[sNpcs[i].facing].movementType;
        template->trainerType = IsBossSlot(i) ? TRAINER_TYPE_NONE : TRAINER_TYPE_NORMAL;
        template->trainerRange_berryTreeId = IsBossSlot(i) ? 0 : INFCAVE_NPC_SIGHT;
        template->script = ScriptForLocalId(template->localId);
    }

    sObjectCount = sNpcCount;

    for (i = sObjectCount; i < OBJECT_EVENT_TEMPLATES_COUNT; i++)
        memset(&gSaveBlock1Ptr->objectEventTemplates[i], 0, sizeof(struct ObjectEventTemplate));

#if INFCAVE_TRACE == TRUE
    DebugPrintf("InfCave wrote %d templates", sObjectCount);
    for (i = 0; i < sObjectCount; i++)
    {
        struct ObjectEventTemplate *template = &gSaveBlock1Ptr->objectEventTemplates[i];

        DebugPrintf("InfCave template %d: localId %d gfx %d at %d,%d",
                    i, template->localId, template->graphicsId, template->x, template->y);
    }
#endif
}

// Trace probe for the duplicate-NPC hunt: logs every active object event with its
// local id, graphics id and tile, so a second object standing on a generated
// trainer's post can be told apart from one object drawn twice. Called from the
// boss script; a no-op unless INFCAVE_TRACE is on.
void InfCave_DebugDumpObjects(void)
{
#if INFCAVE_TRACE == TRUE
    u32 i;

    DebugPrintf("InfCave objects: count %d", sObjectCount);
    for (i = 0; i < OBJECT_EVENTS_COUNT; i++)
    {
        struct ObjectEvent *object = &gObjectEvents[i];

        if (!object->active)
            continue;

        DebugPrintf("InfCave object %d: localId %d gfx %d at %d,%d",
                    i, object->localId, object->graphicsId,
                    object->currentCoords.x - MAP_OFFSET, object->currentCoords.y - MAP_OFFSET);
    }
#endif
}

u32 InfCave_GetObjectCount(void)
{
    return sObjectCount;
}

u32 InfCave_GetRoomTrainerCount(void)
{
    return sNpcCount;
}

bool32 InfCave_InGeneratedRoom(void)
{
    return gMapHeader.mapLayoutId == LAYOUT_INFINITY_CAVE_ROOM;
}

// Continue-from-save counterpart to LoadSaveblockObjEventScripts, which cannot be
// used here: it copies one script per template slot out of the map header, and the
// room's header declares no objects at all. Script pointers are not saved, so
// every generated object needs its own reassigned before field control returns.
void LoadInfinityCaveObjectEventScripts(void)
{
    u32 i;

    for (i = 0; i < OBJECT_EVENT_TEMPLATES_COUNT; i++)
    {
        struct ObjectEventTemplate *template = &gSaveBlock1Ptr->objectEventTemplates[i];

        template->script = ScriptForLocalId(template->localId);
    }
}

static u32 RollFloorRole(rng_value_t *rng)
{
    if (InfCave_RandRange(rng, 0, 99) < INFCAVE_FLOOR_PLAIN_PERCENT)
        return INFCAVE_ROLE_FLOOR_0;

    return INFCAVE_ROLE_FLOOR_1 + InfCave_RandRange(rng, 0, INFCAVE_FLOOR_VARIANT_COUNT - 2);
}

// Renders the finished mask into the backup layout at the MAP_OFFSET origin, the
// same offset arithmetic GenerateBattlePyramidFloorLayout uses. One pass: the
// mask legality rules leave only shapes the authored key can answer, so every
// cell's block comes from its own eight-neighbourhood.
static void AutotileRoom(u16 *origin, u32 stride)
{
    rng_value_t rng = InfCave_SeedRoomRng(INFCAVE_SALT_TILE);
    u32 x, y;

    for (y = 0; y < INFCAVE_MAP_HEIGHT; y++)
    {
        for (x = 0; x < INFCAVE_MAP_WIDTH; x++)
        {
            u32 role;

            if (IsReserved(x, y))
            {
                origin[y * stride + x] = StampBlockAt(x, y);
                continue;
            }

            if (x == sEntranceX && y == sEntranceY)
                role = INFCAVE_ROLE_PAD_ENTRANCE;
            else if (x == sExitX && y == sExitY)
                role = INFCAVE_ROLE_PAD_EXIT;
            else if (!IsFloor(x, y))
                role = WallRoleForCell(x, y);
            else if (sPatch[y][x] != 0)
                role = PatchRoleAt(x, y);
            else if (sDecor[y][x] != 0)
                role = sInfCaveDecor[sDecor[y][x] - 1].role;
            else
                role = RollFloorRole(&rng);

            origin[y * stride + x] = sTileRole[role];
        }
    }
}

// The room's generation passes, in order. Every pass works on the generator's own
// buffers; only the autotile pass that follows them writes blocks, so a pass may
// be added or reordered here without touching tile art.
struct InfCavePass
{
    void (*run)(void);
    const char *name;
};

static const struct InfCavePass sInfCavePasses[] =
{
    { ResolveSetPiece,   "set piece pick" },
    { BuildMask,         "mask" },
    { StampSetPiece,     "set piece stamp" },
    { PlaceEntranceExit, "entrance and exit" },
    { PlacePatches,      "patch" },
    { DecorateRoom,      "decor" },
    { PlaceTrainers,     "trainers" },
};

enum InfCavePlacementFault
{
    INFCAVE_PLACE_FAULT_NONE,
    INFCAVE_PLACE_FAULT_BUDGET,   // more objects than a room may hold
    INFCAVE_PLACE_FAULT_CORRIDOR, // a trainer outside every room region
    INFCAVE_PLACE_FAULT_TILE,     // a trainer on a pad, a prop or something other than floor
    INFCAVE_PLACE_FAULT_SPACING,  // two trainers too close, or one too close to the arrival pad
    INFCAVE_PLACE_FAULT_SPLIT,    // the trainers jointly cut the exit off
    INFCAVE_PLACE_FAULT_SIGHT,    // a sight line lies across the only route to the exit
};

// Re-judges the placement the pass just made, from the finished grids rather than
// from the pass's own bookkeeping, so a rule the pass applies wrongly still shows
// up here. The trainers' tiles are already blocked in the mask, which is what
// makes the split test cumulative.
static u32 PlacementFault(void)
{
    u32 i, j;

    if (sNpcCount > INFCAVE_MAX_OBJECTS)
        return INFCAVE_PLACE_FAULT_BUDGET;

    for (i = 0; i < sNpcCount; i++)
    {
        u32 x = sNpcs[i].x, y = sNpcs[i].y;

        if (!IsInNpcRoom(x, y))
            return INFCAVE_PLACE_FAULT_CORRIDOR;
        if (CellKind(x, y) != INFCAVE_CELL_FLOOR || sDecor[y][x] != 0
         || (sMask[y][x] & INFCAVE_FLAG_NO_DECOR))
            return INFCAVE_PLACE_FAULT_TILE;
        if (ChebyshevDistance(x, y, sEntranceX, sEntranceY) < INFCAVE_NPC_FROM_ENTRANCE)
            return INFCAVE_PLACE_FAULT_SPACING;

        for (j = 0; j < i; j++)
        {
            if (ChebyshevDistance(x, y, sNpcs[j].x, sNpcs[j].y) < INFCAVE_NPC_MIN_APART)
                return INFCAVE_PLACE_FAULT_SPACING;
        }
    }

    if (!EntranceReachesExit())
        return INFCAVE_PLACE_FAULT_SPLIT;

    for (i = 0; i < sNpcCount; i++)
    {
        u8 lineX[INFCAVE_NPC_SIGHT], lineY[INFCAVE_NPC_SIGHT];
        u32 count = BuildSightLine(sNpcs[i].x, sNpcs[i].y, sNpcs[i].facing, lineX, lineY);
        bool32 linked;

        SetLineBlocked(lineX, lineY, count, TRUE);
        linked = EntranceReachesExit();
        SetLineBlocked(lineX, lineY, count, FALSE);

        if (!linked)
            return INFCAVE_PLACE_FAULT_SIGHT;
    }

    return INFCAVE_PLACE_FAULT_NONE;
}

// Debug harness for the placement rules: builds count consecutive room seeds from
// baseSeed with the trainer count pinned, and returns how many placements broke a
// rule. Placing fewer trainers than asked is not a failure — a crowded room is
// allowed to come up short — so only rule violations are counted. This leaves the
// grids holding the last room; the live room is rebuilt from its seed on the next
// map load, so nothing on screen depends on it.
u32 InfCave_DebugValidatePlacement(u32 baseSeed, u32 count, u32 trainers, u32 *firstBadSeed, u32 *firstFault)
{
    u32 savedSeed = gSaveBlock1Ptr->infinityCaveRun.roomSeed;
    u32 i, failures = 0;

    if (!AllocGrids())
        return 0;

    sNpcCountOverride = trainers;
    for (i = 0; i < count; i++)
    {
        u32 fault, pass;

        gSaveBlock1Ptr->infinityCaveRun.roomSeed = baseSeed + i;
        for (pass = 0; pass < ARRAY_COUNT(sInfCavePasses); pass++)
            sInfCavePasses[pass].run();

        fault = PlacementFault();
        if (fault == INFCAVE_PLACE_FAULT_NONE)
            continue;

        if (failures == 0)
        {
            if (firstBadSeed != NULL)
                *firstBadSeed = baseSeed + i;
            if (firstFault != NULL)
                *firstFault = fault;
        }
        failures++;
    }
    sNpcCountOverride = 0;

    gSaveBlock1Ptr->infinityCaveRun.roomSeed = savedSeed;
    FreeGrids();
    return failures;
}

static const u8 sPlaceFaultName_None[] = _("none");
static const u8 sPlaceFaultName_Budget[] = _("budget");
static const u8 sPlaceFaultName_Corridor[] = _("corridor");
static const u8 sPlaceFaultName_Tile[] = _("tile");
static const u8 sPlaceFaultName_Spacing[] = _("spacing");
static const u8 sPlaceFaultName_Split[] = _("split");
static const u8 sPlaceFaultName_Sight[] = _("sight");

// Indexed by enum InfCavePlacementFault, for the debug harness's report line.
static const u8 *const sPlaceFaultNames[] =
{
    [INFCAVE_PLACE_FAULT_NONE]     = sPlaceFaultName_None,
    [INFCAVE_PLACE_FAULT_BUDGET]   = sPlaceFaultName_Budget,
    [INFCAVE_PLACE_FAULT_CORRIDOR] = sPlaceFaultName_Corridor,
    [INFCAVE_PLACE_FAULT_TILE]     = sPlaceFaultName_Tile,
    [INFCAVE_PLACE_FAULT_SPACING]  = sPlaceFaultName_Spacing,
    [INFCAVE_PLACE_FAULT_SPLIT]    = sPlaceFaultName_Split,
    [INFCAVE_PLACE_FAULT_SIGHT]    = sPlaceFaultName_Sight,
};

const u8 *InfCave_GetPlacementFaultName(u32 fault)
{
    if (fault >= ARRAY_COUNT(sPlaceFaultNames))
        return sPlaceFaultNames[INFCAVE_PLACE_FAULT_NONE];
    return sPlaceFaultNames[fault];
}

void InfCave_GenerateRoom(u16 *backupMapData, bool8 setPlayerPosition)
{
    u16 *origin;
    u32 i;

    if (!AllocGrids())
        return;

    InfCave_LoadTileRoles();

    for (i = 0; i < ARRAY_COUNT(sInfCavePasses); i++)
    {
#if INFCAVE_TRACE == TRUE
        DebugPrintf("InfCave pass %s", sInfCavePasses[i].name);
#endif
        sInfCavePasses[i].run();
    }

    gBackupMapLayout.map = backupMapData;
    gBackupMapLayout.width = INFCAVE_MAP_WIDTH + MAP_OFFSET_W;
    gBackupMapLayout.height = INFCAVE_MAP_HEIGHT + MAP_OFFSET_H;

    origin = backupMapData + gBackupMapLayout.width * MAP_OFFSET + MAP_OFFSET;
    AutotileRoom(origin, gBackupMapLayout.width);

    WriteTrainerTemplates();
    FreeGrids();

    // Both the warp and the reload path reach here, so the cave's loss handling is
    // armed for every room the player can stand in.
    InfCave_ArmNoWhiteout();

    // setPlayerPosition mirrors the Battle Pyramid's inverted sense: TRUE means
    // the position is already restored from the save and must be kept.
    if (setPlayerPosition == FALSE)
    {
        gSaveBlock1Ptr->pos.x = sEntranceX;
        gSaveBlock1Ptr->pos.y = sEntranceY;
    }

    RunOnLoadMapScript();
}
