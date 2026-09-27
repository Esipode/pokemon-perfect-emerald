#include "global.h"
#include "fieldmap.h"
#include "infinity_cave.h"
#include "overworld.h"
#include "script.h"
#include "constants/infinity_cave.h"
#include "constants/layouts.h"
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
    { INFCAVE_ROLE_WATER_NW,         INFCAVE_ROLE_WATER_SE + 1 },
};

// Temporary way out until Stage 10 places the exit crystal. Must match the
// warp event in data/maps/InfinityCave_Room/map.json.
#define INFCAVE_TEMP_EXIT_X 20
#define INFCAVE_TEMP_EXIT_Y 37

// Minimum wall run thickness. gTileset_Cave draws the face a single metatile
// tall, but the mask still needs a solid margin behind every face.
#define INFCAVE_WALL_THICKNESS 2

// Flags OR'd into a mask cell alongside its kind. COMPONENT and KEEP are
// transient, owned by the connectivity passes; NO_DECOR survives for the room's
// lifetime and marks tiles a pad or an access corridor owns.
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

// The per-tile mask every generation pass works on, and the rectangles the
// rooms were carved from. Both are EWRAM: 1600 bytes is far too much for the
// stack, and later passes (set pieces, entrance/exit, trainer placement) read
// the rooms back.
static EWRAM_DATA u8 sMask[INFCAVE_MAP_HEIGHT][INFCAVE_MAP_WIDTH] = {0};
static EWRAM_DATA struct InfCaveRoomRect sRooms[INFCAVE_MAX_ROOMS] = {0};
static EWRAM_DATA u8 sRoomCount = 0;

// Props placed on the mask's floor, as one plus an index into sInfCaveDecor so
// zero means an empty tile. The decoration pass leaves the mask itself
// untouched, so the autotiler still sees every prop tile as floor and tiles the
// walls around it normally; the prop's own block is written over the floor block.
static EWRAM_DATA u8 sDecor[INFCAVE_MAP_HEIGHT][INFCAVE_MAP_WIDTH] = {0};

// Terrain patches, as one plus an index into sInfCavePatch. Like props these sit
// beside the mask rather than in it, so the wall shapes around a patch are
// unaffected; unlike props a patch cell's block depends on where the cell sits
// in its rectangle, which the autotiler works out from the neighbouring cells.
static EWRAM_DATA u8 sPatch[INFCAVE_MAP_HEIGHT][INFCAVE_MAP_WIDTH] = {0};

// Distinct RNG streams within one room, so adding a consumer cannot shift the
// numbers an existing pass draws.
#define INFCAVE_SALT_MASK 0x4D41534Bu // 'MASK'
#define INFCAVE_SALT_TILE 0x54494C45u // 'TILE'
#define INFCAVE_SALT_EXIT 0x45584954u // 'EXIT'
#define INFCAVE_SALT_DECO 0x4445434Fu // 'DECO'
#define INFCAVE_SALT_PTCH 0x50544348u // 'PTCH'

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

// Why a mask was rejected. Reported by the debug harness so a bad seed names the
// rule it broke instead of only failing.
enum InfCaveMaskFault
{
    INFCAVE_FAULT_NONE,
    INFCAVE_FAULT_MARGIN,    // floor inside the solid canvas margin
    INFCAVE_FAULT_DIAGONAL,  // two floor regions joined only through a diagonal
    INFCAVE_FAULT_THIN_WALL, // wall run one tile thick between two floor tiles
    INFCAVE_FAULT_AREA,      // less walkable area than INFCAVE_MIN_FLOOR_TILES
    INFCAVE_FAULT_SPLIT,     // more than one floor component
};

static bool32 IsFloorSafe(s32 x, s32 y)
{
    if (x < 0 || x >= INFCAVE_MAP_WIDTH || y < 0 || y >= INFCAVE_MAP_HEIGHT)
        return FALSE;
    return IsFloor(x, y);
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

// Repair sweeps run to a fixed point: each sweep adds floor, which can expose a
// fresh diagonal or thin run one tile further out. Both sweeps only ever turn
// wall into floor, so this can be re-run over a finished mask without undoing
// anything a later pass carved.
static void EnforceLegality(rng_value_t *rng)
{
    u32 pass;

    for (pass = 0; pass < INFCAVE_LEGALITY_PASSES; pass++)
    {
        bool32 changed = FALSE;

        changed |= FixDiagonalLinks(rng);
        changed |= OpenThinWallRuns();

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
            sMask[y][x] |= INFCAVE_FLAG_NO_DECOR;
        }

        if (reached)
            return;
    }

    // The column found no floor at all: cut one row straight across instead.
    for (y = INFCAVE_AREA_MIN; y < INFCAVE_AREA_MIN + INFCAVE_CORRIDOR_WIDTH; y++)
    {
        u32 x;

        CarveRect(INFCAVE_AREA_MIN, y, INFCAVE_AREA_MAX_X - INFCAVE_AREA_MIN, 1);
        for (x = INFCAVE_AREA_MIN; x < INFCAVE_AREA_MAX_X; x++)
            sMask[y][x] |= INFCAVE_FLAG_NO_DECOR;
    }
}

static bool32 IsWalkable(s32 x, s32 y);

// Nearest tile to the canvas centre the player can stand on, searched outward in
// square rings. Props are already placed, so a solid prop's tile is skipped.
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
                if (!IsWalkable(x, y) || sDecor[y][x] != 0)
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

// The ordered generation pipeline. Every pass reads and writes sMask only;
// nothing here knows about metatiles. The temporary exit access and the wall
// shell run after the mask is judged: the access corridor deliberately breaks
// the margin rule, and the shell only relabels void the player cannot reach.
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

    CarveTempExitAccess();

    // The corridor cuts through walls the legality pass already signed off, so
    // it can leave a one-tile wall run beside itself. Repairing again fixes
    // those without ClearMarginFloor, which would delete the corridor.
    {
        rng_value_t rng = InfCave_SeedRoomRng(INFCAVE_SALT_EXIT);

        EnforceLegality(&rng);
    }

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

// A block a walking player cannot enter: either it collides, or it sits at
// another elevation, which is what surfable water is. Every pass here plans for
// a player on foot, so water counts as blocked even though Surf crosses it.
static bool32 IsBlockOnFoot(u16 block)
{
    if ((block & MAPGRID_COLLISION_MASK) != 0)
        return TRUE;
    return (block & MAPGRID_ELEVATION_MASK) != (sTileRole[INFCAVE_ROLE_FLOOR_0] & MAPGRID_ELEVATION_MASK);
}

// Blocking is per cell, not per material: a pool's shore row is ground art the
// player walks on even though the water beside it is not.
static bool32 IsPatchCellBlocked(s32 x, s32 y)
{
    return IsBlockOnFoot(sTileRole[PatchRoleAt(x, y)]);
}

// Whether a material blocks a walking player at all, read from its fill block.
// Only a blocking material can split the room, so only that one costs a walk.
static bool32 IsPatchBlocking(u32 index)
{
    return IsBlockOnFoot(sTileRole[sInfCavePatch[index].baseRole + INFCAVE_PATCH_FILL]);
}

// Whether a prop blocks movement comes from the collision bits of its authored
// block, not from the table, so the placer can never disagree with what the key
// layout actually draws.
static bool32 IsDecorSolid(u32 index)
{
    return IsBlockOnFoot(sTileRole[sInfCaveDecor[index].role]);
}

// A tile a walking player can stand on: carved floor with no water and no solid
// prop on it.
static bool32 IsWalkable(s32 x, s32 y)
{
    if (!IsFloorSafe(x, y))
        return FALSE;
    if (sPatch[y][x] != 0 && IsPatchCellBlocked(x, y))
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

// Walkable tiles inside the carved area only. The temporary exit corridor pokes
// floor into the canvas margin, which the fill below never visits, so counting
// the whole canvas would read as a split room.
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

// Lays rectangles of a second ground material over the floor. A blocking
// material is taken back again if it cuts the room in two, which is why each
// rectangle goes down and is judged on its own rather than all at once.
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
        if (IsPatchBlocking(index) && !IsRoomStillConnected())
        {
            FillPatchRect(x, y, w, h, 0);
            continue;
        }
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
     && (!IsFloorSafe(x, y - 1) || !IsFloorSafe(x, y + 1)
      || !IsFloorSafe(x - 1, y) || !IsFloorSafe(x + 1, y)))
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

// Wall shape for a non-floor cell. The mask legality rules forbid a wall with
// floor on two opposite sides, so the only cardinal combinations left are none,
// one side, or two perpendicular sides; that is what collapses the 256
// eight-neighbour cases onto the authored shape set. A cell with no cardinal
// floor but floor on one diagonal is a concave corner.
static u32 WallRoleForCell(s32 x, s32 y)
{
    bool32 n = IsFloorSafe(x, y - 1);
    bool32 s = IsFloorSafe(x, y + 1);
    bool32 w = IsFloorSafe(x - 1, y);
    bool32 e = IsFloorSafe(x + 1, y);

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

    if (IsFloorSafe(x - 1, y - 1))
        return INFCAVE_ROLE_WALL_INNER_NW;
    if (IsFloorSafe(x + 1, y - 1))
        return INFCAVE_ROLE_WALL_INNER_NE;
    if (IsFloorSafe(x - 1, y + 1))
        return INFCAVE_ROLE_WALL_INNER_SW;
    if (IsFloorSafe(x + 1, y + 1))
        return INFCAVE_ROLE_WALL_INNER_SE;

    return INFCAVE_ROLE_WALL_FILL;
}

static u32 RollFloorRole(rng_value_t *rng)
{
    if (InfCave_RandRange(rng, 0, 99) < INFCAVE_FLOOR_PLAIN_PERCENT)
        return INFCAVE_ROLE_FLOOR_0;

    return INFCAVE_ROLE_FLOOR_1 + InfCave_RandRange(rng, 0, INFCAVE_FLOOR_VARIANT_COUNT - 2);
}

// Renders the finished mask into the backup layout at the MAP_OFFSET origin, the
// same offset arithmetic GenerateBattlePyramidFloorLayout uses. Two passes: the
// first gives every cell its shape, the second makes the south face two tiles
// tall, which is how the cave tileset draws it. The upper row is only written
// where the cell's own shape would have been plain fill, so the wall shapes that
// already answer a neighbouring floor tile survive.
static void AutotileRoom(u16 *origin, u32 stride)
{
    rng_value_t rng = InfCave_SeedRoomRng(INFCAVE_SALT_TILE);
    u32 x, y;

    for (y = 0; y < INFCAVE_MAP_HEIGHT; y++)
    {
        for (x = 0; x < INFCAVE_MAP_WIDTH; x++)
        {
            u32 role;

            if (!IsFloor(x, y))
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

void InfCave_GenerateRoom(u16 *backupMapData, bool8 setPlayerPosition)
{
    u16 *origin;

    InfCave_LoadTileRoles();
    BuildMask();
    PlacePatches();
    DecorateRoom();

    gBackupMapLayout.map = backupMapData;
    gBackupMapLayout.width = INFCAVE_MAP_WIDTH + MAP_OFFSET_W;
    gBackupMapLayout.height = INFCAVE_MAP_HEIGHT + MAP_OFFSET_H;

    origin = backupMapData + gBackupMapLayout.width * MAP_OFFSET + MAP_OFFSET;
    AutotileRoom(origin, gBackupMapLayout.width);

    origin[gBackupMapLayout.width * INFCAVE_TEMP_EXIT_Y + INFCAVE_TEMP_EXIT_X]
        = sTileRole[INFCAVE_ROLE_PAD_EXIT];

    // setPlayerPosition mirrors the Battle Pyramid's inverted sense: TRUE means
    // the position is already restored from the save and must be kept.
    if (setPlayerPosition == FALSE)
        PlacePlayerOnFloor();

    RunOnLoadMapScript();
}
