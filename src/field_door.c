#include "global.h"
#include "event_data.h"
#include "field_door.h"
#include "field_camera.h"
#include "fieldmap.h"
#include "metatile_behavior.h"
#include "task.h"
#include "tilesets.h"
#include "constants/songs.h"
#include "constants/metatile_labels.h"

#define DOOR_SOUND_NORMAL  0
#define DOOR_SOUND_SLIDING 1
#define DOOR_SOUND_ARENA   2

// Johto primaries leave tiles 434-441 free; the top of VRAM can hold Johto secondary tiles.
#define DOOR_TILE_START_JOHTO 434

enum DoorSize
{
    DOOR_SIZE_1x1,
    DOOR_SIZE_1x2,
    DOOR_SIZE_2x2_LEFT,
    DOOR_SIZE_2x2_RIGHT,
    DOOR_SIZE_COUNT,
};

struct DoorGraphics
{
    u16 metatileNum;
    const struct Tileset *tileset;
    u8 sound;
    enum DoorSize size:8;
    const void *tiles;
    const void *palettes;
    u16 tileStart; // 0 = default VRAM slot for the door size
};

struct DoorAnimFrame
{
    u8 time;
    u16 offset;
};

struct DoorSizeInfo
{
    const struct DoorAnimFrame *openAnimFrames;
    const struct DoorAnimFrame *closeAnimFrames;
};

static bool8 ShouldUseMultiCorridorDoor(void);

static const u8 sDoorAnimTiles_Littleroot[] = INCGFX_U8("graphics/door_anims/littleroot.png", ".4bpp");
static const u16 sDoorNullPalette1[16] = {};
static const u8 sDoorAnimTiles_BirchsLab[] = INCGFX_U8("graphics/door_anims/birchs_lab.png", ".4bpp");
static const u16 sDoorNullPalette2[16] = {};
static const u8 sDoorAnimTiles_FallarborLightRoof[] = INCGFX_U8("graphics/door_anims/fallarbor_light_roof.png", ".4bpp");
static const u16 sDoorNullPalette3[16] = {};
static const u8 sDoorAnimTiles_Lilycove[] = INCGFX_U8("graphics/door_anims/lilycove.png", ".4bpp");
static const u16 sDoorNullPalette4[16] = {};
static const u8 sDoorAnimTiles_LilycoveWooden[] = INCGFX_U8("graphics/door_anims/lilycove_wooden.png", ".4bpp");
static const u16 sDoorNullPalette5[16] = {};
static const u8 sDoorAnimTiles_General[] = INCGFX_U8("graphics/door_anims/general.png", ".4bpp");
static const u16 sDoorNullPalette6[16] = {};
static const u8 sDoorAnimTiles_PokeCenter[] = INCGFX_U8("graphics/door_anims/poke_center.png", ".4bpp");
static const u8 sDoorAnimTiles_Gym[] = INCGFX_U8("graphics/door_anims/gym.png", ".4bpp");
static const u8 sDoorAnimTiles_PokeMart[] = INCGFX_U8("graphics/door_anims/poke_mart.png", ".4bpp");
static const u8 sDoorAnimTiles_RustboroTan[] = INCGFX_U8("graphics/door_anims/rustboro_tan.png", ".4bpp");
static const u16 sDoorNullPalette7[16] = {};
static const u8 sDoorAnimTiles_RustboroGray[] = INCGFX_U8("graphics/door_anims/rustboro_gray.png", ".4bpp");
static const u16 sDoorNullPalette8[16] = {};
static const u8 sDoorAnimTiles_Oldale[] = INCGFX_U8("graphics/door_anims/oldale.png", ".4bpp");
static const u8 sFiller1[0x5900] = {};
static const u8 sDoorAnimTiles_UnusedTops[] = INCGFX_U8("graphics/door_anims/unused_top.png", ".4bpp");
static const u8 sFiller2[0x140] = {};
static const u8 sDoorAnimTiles_UnusedBottoms[] = INCGFX_U8("graphics/door_anims/unused_bottom.png", ".4bpp");
static const u16 sDoorNullPalette11[16] = {};
static const u8 sDoorAnimTiles_Mauville[] = INCGFX_U8("graphics/door_anims/mauville.png", ".4bpp");
static const u16 sDoorNullPalette12[16] = {};
static const u8 sDoorAnimTiles_Verdanturf[] = INCGFX_U8("graphics/door_anims/verdanturf.png", ".4bpp");
static const u16 sDoorNullPalette13[16] = {};
static const u8 sDoorAnimTiles_Slateport[] = INCGFX_U8("graphics/door_anims/slateport.png", ".4bpp");
static const u16 sDoorNullPalette14[16] = {};
static const u8 sDoorAnimTiles_Dewford[] = INCGFX_U8("graphics/door_anims/dewford.png", ".4bpp");
static const u16 sDoorNullPalette15[16] = {};
static const u8 sDoorAnimTiles_Contest[] = INCGFX_U8("graphics/door_anims/contest.png", ".4bpp");
static const u16 sDoorNullPalette16[16] = {};
static const u8 sDoorAnimTiles_Mossdeep[] = INCGFX_U8("graphics/door_anims/mossdeep.png", ".4bpp");
static const u16 sDoorNullPalette17[16] = {};
static const u8 sDoorAnimTiles_SootopolisPeakedRoof[] = INCGFX_U8("graphics/door_anims/sootopolis_peaked_roof.png", ".4bpp");
static const u16 sDoorNullPalette18[16] = {};
static const u8 sDoorAnimTiles_Sootopolis[] = INCGFX_U8("graphics/door_anims/sootopolis.png", ".4bpp");
static const u16 sDoorNullPalette19[16] = {};
static const u8 sDoorAnimTiles_PokemonLeague[] = INCGFX_U8("graphics/door_anims/pokemon_league.png", ".4bpp");
static const u16 sDoorNullPalette20[16] = {};
static const u8 sDoorAnimTiles_Pacifidlog[] = INCGFX_U8("graphics/door_anims/pacifidlog.png", ".4bpp");
static const u16 sDoorNullPalette21[16] = {};
static const u8 sDoorAnimTiles_PetalburgGym[] = INCGFX_U8("graphics/door_anims/petalburg_gym.png", ".4bpp");
static const u16 sDoorNullPalette22[16] = {};
static const u8 sDoorAnimTiles_CyclingRoad[] = INCGFX_U8("graphics/door_anims/cycling_road.png", ".4bpp");
static const u16 sDoorNullPalette23[16] = {};
static const u8 sDoorAnimTiles_LilycoveDeptStore[] = INCGFX_U8("graphics/door_anims/lilycove_dept_store.png", ".4bpp");
static const u16 sDoorNullPalette24[16] = {};
static const u8 sDoorAnimTiles_SafariZone[] = INCGFX_U8("graphics/door_anims/safari_zone.png", ".4bpp");
static const u16 sDoorNullPalette25[16] = {};
static const u8 sDoorAnimTiles_MossdeepSpaceCenter[] = INCGFX_U8("graphics/door_anims/mossdeep_space_center.png", ".4bpp");
static const u16 sDoorNullPalette26[16] = {};
static const u8 sDoorAnimTiles_CableClub[] = INCGFX_U8("graphics/door_anims/cable_club.png", ".4bpp");
static const u16 sDoorNullPalette27[16] = {};
static const u8 sDoorAnimTiles_AbandonedShip[] = INCGFX_U8("graphics/door_anims/abandoned_ship.png", ".4bpp");
static const u16 sDoorNullPalette28[16] = {};
static const u8 sDoorAnimTiles_FallarborDarkRoof[] = INCGFX_U8("graphics/door_anims/fallarbor_dark_roof.png", ".4bpp");
static const u16 sDoorNullPalette29[16] = {};
static const u8 sDoorAnimTiles_AbandonedShipRoom[] = INCGFX_U8("graphics/door_anims/abandoned_ship_room.png", ".4bpp");
static const u16 sDoorNullPalette30[16] = {};
static const u8 sDoorAnimTiles_LilycoveDeptStoreElevator[] = INCGFX_U8("graphics/door_anims/lilycove_dept_store_elevator.png", ".4bpp");
static const u16 sDoorNullPalette31[16] = {};
static const u8 sDoorAnimTiles_BattleTowerOld[] = INCGFX_U8("graphics/door_anims/battle_tower_old.png", ".4bpp");
static const u16 sDoorNullPalette32[16] = {};
static const u8 sDoorAnimTiles_BattleTowerElevator[] = INCGFX_U8("graphics/door_anims/battle_tower_elevator.png", ".4bpp");
static const u16 sDoorNullPalette33[16] = {};
static const u8 sDoorAnimTiles_UnusedBattleFrontier[] = INCGFX_U8("graphics/door_anims/unused_battle_frontier.png", ".4bpp");
static const u16 sDoorNullPalette34[16] = {};
static const u8 sDoorAnimTiles_BattleDome[] = INCGFX_U8("graphics/door_anims/battle_dome.png", ".4bpp");
static const u16 sDoorNullPalette35[16] = {};
static const u8 sDoorAnimTiles_BattleFactory[] = INCGFX_U8("graphics/door_anims/battle_factory.png", ".4bpp");
static const u16 sDoorNullPalette36[16] = {};
static const u8 sDoorAnimTiles_BattleTower[] = INCGFX_U8("graphics/door_anims/battle_tower.png", ".4bpp");
static const u16 sDoorNullPalette37[16] = {};
static const u8 sDoorAnimTiles_BattleArena[] = INCGFX_U8("graphics/door_anims/battle_arena.png", ".4bpp");
static const u16 sDoorNullPalette38[16] = {};
static const u8 sDoorAnimTiles_BattleArenaLobby[] = INCGFX_U8("graphics/door_anims/battle_arena_lobby.png", ".4bpp");
static const u16 sDoorNullPalette39[16] = {};
static const u8 sDoorAnimTiles_BattleDomeLobby[] = INCGFX_U8("graphics/door_anims/battle_dome_lobby.png", ".4bpp");
static const u16 sDoorNullPalette40[16] = {};
static const u8 sDoorAnimTiles_BattlePalaceLobby[] = INCGFX_U8("graphics/door_anims/battle_palace_lobby.png", ".4bpp");
static const u8 sDoorAnimTiles_BattleTent[] = INCGFX_U8("graphics/door_anims/battle_tent.png", ".4bpp");
static const u16 sDoorNullPalette41[16] = {};
static const u8 sDoorAnimTiles_BattleDomeCorridor[] = INCGFX_U8("graphics/door_anims/battle_dome_corridor.png", ".4bpp");
static const u16 sDoorNullPalette42[16] = {};
static const u8 sDoorAnimTiles_BattleTowerMultiCorridor[] = INCGFX_U8("graphics/door_anims/battle_tower_multi_corridor.png", ".4bpp", "-mwidth 2 -mheight 4");
static const u16 sDoorNullPalette43[16] = {};
static const u8 sDoorAnimTiles_BattleFrontier[] = INCGFX_U8("graphics/door_anims/battle_frontier.png", ".4bpp");
static const u16 sDoorNullPalette44[16] = {};
static const u8 sDoorAnimTiles_BattleFrontierSliding[] = INCGFX_U8("graphics/door_anims/battle_frontier_sliding.png", ".4bpp");
static const u16 sDoorNullPalette45[16] = {};
static const u8 sDoorAnimTiles_BattleDomePreBattleRoom[] = INCGFX_U8("graphics/door_anims/battle_dome_pre_battle_room.png", ".4bpp");
static const u16 sDoorNullPalette46[16] = {};
static const u8 sDoorAnimTiles_BattleTentInterior[] = INCGFX_U8("graphics/door_anims/battle_tent_interior.png", ".4bpp");
static const u16 sDoorNullPalette47[16] = {};
static const u8 sDoorAnimTiles_TrainerHillLobbyElevator[] = INCGFX_U8("graphics/door_anims/trainer_hill_lobby_elevator.png", ".4bpp");
static const u16 sDoorNullPalette48[16] = {};
static const u8 sDoorAnimTiles_TrainerHillRoofElevator[] = INCGFX_U8("graphics/door_anims/trainer_hill_roof_elevator.png", ".4bpp");
static const u16 sDoorNullPalette49[16] = {};

static const u8 sDoorAnimTiles_GeneralFrlg[] = INCGFX_U8("graphics/door_anims/general_frlg.png", ".4bpp");
static const u8 sDoorAnimTiles_SlidingSingle[] = INCGFX_U8("graphics/door_anims/sliding_single.png", ".4bpp");
static const u8 sDoorAnimTiles_SlidingDouble[] = INCGFX_U8("graphics/door_anims/sliding_double.png", ".4bpp");
static const u8 sDoorAnimTiles_Pallet[] = INCGFX_U8("graphics/door_anims/pallet.png", ".4bpp");
static const u8 sDoorAnimTiles_OaksLab[] = INCGFX_U8("graphics/door_anims/oaks_lab.png", ".4bpp");
static const u8 sDoorAnimTiles_Viridian[] = INCGFX_U8("graphics/door_anims/viridian.png", ".4bpp");
static const u8 sDoorAnimTiles_Pewter[] = INCGFX_U8("graphics/door_anims/pewter.png", ".4bpp");
static const u8 sDoorAnimTiles_Saffron[] = INCGFX_U8("graphics/door_anims/saffron.png", ".4bpp");
static const u8 sDoorAnimTiles_SilphCo[] = INCGFX_U8("graphics/door_anims/silph_co.png", ".4bpp");
static const u8 sDoorAnimTiles_Cerulean[] = INCGFX_U8("graphics/door_anims/cerulean.png", ".4bpp");
static const u8 sDoorAnimTiles_Lavender[] = INCGFX_U8("graphics/door_anims/lavender.png", ".4bpp");
static const u8 sDoorAnimTiles_Vermilion[] = INCGFX_U8("graphics/door_anims/vermilion.png", ".4bpp");
static const u8 sDoorAnimTiles_PokemonFanClub[] = INCGFX_U8("graphics/door_anims/pokemon_fan_club.png", ".4bpp");
static const u8 sDoorAnimTiles_DeptStore[] = INCGFX_U8("graphics/door_anims/dept_store.png", ".4bpp");
static const u8 sDoorAnimTiles_Fuchsia[] = INCGFX_U8("graphics/door_anims/fuchsia.png", ".4bpp");
static const u8 sDoorAnimTiles_SafariZoneFrlg[] = INCGFX_U8("graphics/door_anims/safari_zone_frlg.png", ".4bpp");
static const u8 sDoorAnimTiles_CinnabarLab[] = INCGFX_U8("graphics/door_anims/cinnabar_lab.png", ".4bpp");
static const u8 sDoorAnimTiles_DeptStoreElevator[] = INCGFX_U8("graphics/door_anims/dept_store_elevator.png", ".4bpp");
static const u8 sDoorAnimTiles_CableClubFrlg[] = INCGFX_U8("graphics/door_anims/cable_club_frlg.png", ".4bpp");
static const u8 sDoorAnimTiles_HideoutElevator[] = INCGFX_U8("graphics/door_anims/hideout_elevator.png", ".4bpp");
static const u8 sDoorAnimTiles_SSAnne[] = INCGFX_U8("graphics/door_anims/ss_anne.png", ".4bpp");
static const u8 sDoorAnimTiles_SilphCoElevator[] = INCGFX_U8("graphics/door_anims/silph_co_elevator.png", ".4bpp");
static const u8 sDoorAnimTiles_Sevii123[] = INCGFX_U8("graphics/door_anims/sevii_123.png", ".4bpp");
static const u8 sDoorAnimTiles_JoyfulGameCorner[] = INCGFX_U8("graphics/door_anims/joyful_game_corner.png", ".4bpp");
static const u8 sDoorAnimTiles_OneIslandPokeCenter[] = INCGFX_U8("graphics/door_anims/one_island_poke_center.png", ".4bpp");
static const u8 sDoorAnimTiles_Sevii45[] = INCGFX_U8("graphics/door_anims/sevii_45.png", ".4bpp");
static const u8 sDoorAnimTiles_FourIslandDayCare[] = INCGFX_U8("graphics/door_anims/four_island_day_care.png", ".4bpp");
static const u8 sDoorAnimTiles_RocketWarehouse[] = INCGFX_U8("graphics/door_anims/rocket_warehouse.png", ".4bpp");
static const u8 sDoorAnimTiles_Sevii67[] = INCGFX_U8("graphics/door_anims/sevii_67.png", ".4bpp");
static const u8 sDoorAnimTiles_Teleporter[] = INCGFX_U8("graphics/door_anims/teleporter.png", ".4bpp");

// Johto doors (GS Stage 8)
static const u8 sDoorAnimTiles_JohtoGeneral[] = INCGFX_U8("graphics/door_anims/johtogeneral.png", ".4bpp");
static const u8 sDoorAnimTiles_JohtoGym[] = INCGFX_U8("graphics/door_anims/johtogym.png", ".4bpp");
static const u8 sDoorAnimTiles_JohtoPokeCenter[] = INCGFX_U8("graphics/door_anims/johtopokecenter.png", ".4bpp");
static const u8 sDoorAnimTiles_Johto_BlackthornCity[] = INCGFX_U8("graphics/door_anims/blackthorn_city_johto.png", ".4bpp");
static const u8 sDoorAnimTiles_Johto_Cianwood[] = INCGFX_U8("graphics/door_anims/cianwood_johto.png", ".4bpp");
static const u8 sDoorAnimTiles_Johto_DojoDoor[] = INCGFX_U8("graphics/door_anims/dojo_door_johto.png", ".4bpp");
static const u8 sDoorAnimTiles_Johto_DojoDoorEcruteak[] = INCGFX_U8("graphics/door_anims/dojo_door_ecruteak_johto.png", ".4bpp");
static const u8 sDoorAnimTiles_Johto_Goldenrod[] = INCGFX_U8("graphics/door_anims/goldenrod_johto.png", ".4bpp");
static const u8 sDoorAnimTiles_Johto_HnsCerulean[] = INCGFX_U8("graphics/door_anims/cerulean_johto.png", ".4bpp");
static const u8 sDoorAnimTiles_Johto_HnsDeptStoreElevator[] = INCGFX_U8("graphics/door_anims/dept_store_elevator_johto.png", ".4bpp");
static const u8 sDoorAnimTiles_Johto_HnsSafariZone[] = INCGFX_U8("graphics/door_anims/safari_zone_johto.png", ".4bpp");
static const u8 sDoorAnimTiles_Johto_HnsVermilion[] = INCGFX_U8("graphics/door_anims/vermilion_johto.png", ".4bpp");
static const u8 sDoorAnimTiles_Johto_HnsViridian[] = INCGFX_U8("graphics/door_anims/viridian_johto.png", ".4bpp");
static const u8 sDoorAnimTiles_Johto_NewBarkTownRed[] = INCGFX_U8("graphics/door_anims/NewBarkTown_Door_Red_johto.png", ".4bpp");
static const u8 sDoorAnimTiles_Johto_NewBarkTownYellow[] = INCGFX_U8("graphics/door_anims/NewBarkTown_Door_Yellow_johto.png", ".4bpp");
static const u8 sDoorAnimTiles_Johto_Olivine[] = INCGFX_U8("graphics/door_anims/olivine_johto.png", ".4bpp");
static const u8 sDoorAnimTiles_Johto_RocketElevator[] = INCGFX_U8("graphics/door_anims/rocket_elevator_johto.png", ".4bpp");
static const u8 sDoorAnimTiles_Johto_SSAqua[] = INCGFX_U8("graphics/door_anims/ssaqua_johto.png", ".4bpp");

static const struct DoorAnimFrame sDoorAnimFrames_Open1x1[] = {
    {4, -1},
    {4, 0 * TILE_SIZE_4BPP},
    {4, 4 * TILE_SIZE_4BPP},
    {4, 8 * TILE_SIZE_4BPP},
    {}
};

static const struct DoorAnimFrame sDoorAnimFrames_Close1x1[] = {
    {4, 8 * TILE_SIZE_4BPP},
    {4, 4 * TILE_SIZE_4BPP},
    {4, 0 * TILE_SIZE_4BPP},
    {4, -1},
    {}
};

static const struct DoorAnimFrame sDoorAnimFrames_Open1x2[] = {
    {4, -1},
    {4, 0 * TILE_SIZE_4BPP},
    {4, 8 * TILE_SIZE_4BPP},
    {4, 16 * TILE_SIZE_4BPP},
    {}
};

static const struct DoorAnimFrame sDoorAnimFrames_Close1x2[] = {
    {4, 16 * TILE_SIZE_4BPP},
    {4, 8 * TILE_SIZE_4BPP},
    {4, 0 * TILE_SIZE_4BPP},
    {4, -1},
    {}
};

static const struct DoorAnimFrame sDoorAnimFrames_Open2x2[] =
{
    {4, -1},
    {4, 0 * TILE_SIZE_4BPP},
    {4, 16 * TILE_SIZE_4BPP},
    {4, 32 * TILE_SIZE_4BPP},
    {0, 0},
};

static const struct DoorAnimFrame sDoorAnimFrames_Close2x2[] =
{
    {4, 32 * TILE_SIZE_4BPP},
    {4, 16 * TILE_SIZE_4BPP},
    {4, 0 * TILE_SIZE_4BPP},
    {4, -1},
    {}
};

static const struct DoorSizeInfo sDoorSizeInfo[DOOR_SIZE_COUNT] = 
{
    [DOOR_SIZE_1x1] =
    {
        .openAnimFrames = sDoorAnimFrames_Open1x1,
        .closeAnimFrames = sDoorAnimFrames_Close1x1,
    },
    [DOOR_SIZE_1x2] =
    {
        .openAnimFrames = sDoorAnimFrames_Open1x2,
        .closeAnimFrames = sDoorAnimFrames_Close1x2,
    },
    [DOOR_SIZE_2x2_LEFT] =
    {
        .openAnimFrames = sDoorAnimFrames_Open2x2,
        .closeAnimFrames = sDoorAnimFrames_Close2x2,
    },
    [DOOR_SIZE_2x2_RIGHT] =
    {
        .openAnimFrames = sDoorAnimFrames_Open2x2,
        .closeAnimFrames = sDoorAnimFrames_Close2x2,
    },
};

static const u8 sDoorAnimPalettes_General[] = {1, 1, 1, 1, 1, 1, 1, 1};
static const u8 sDoorAnimPalettes_PokeCenter[] = {1, 1, 1, 1, 1, 1, 1, 1};
static const u8 sDoorAnimPalettes_Gym[] = {5, 5, 5, 5, 5, 5, 5, 5};
static const u8 sDoorAnimPalettes_PokeMart[] = {0, 0, 1, 1, 1, 1, 1, 1};
static const u8 sDoorAnimPalettes_Littleroot[] = {10, 10, 6, 6, 6, 6, 6, 6};
static const u8 sDoorAnimPalettes_BirchsLab[] = {8, 8, 8, 8, 8, 8, 8, 8};
static const u8 sDoorAnimPalettes_RustboroTan[] = {11, 11, 11, 11, 11, 11, 11, 11};
static const u8 sDoorAnimPalettes_RustboroGray[] = {10, 10, 10, 10, 10, 10, 10, 10};
static const u8 sDoorAnimPalettes_FallarborLightRoof[] = {7, 7, 7, 7, 7, 7, 7, 7};
static const u8 sDoorAnimPalettes_Lilycove[] = {8, 8, 8, 8, 8, 8, 8, 8};
static const u8 sDoorAnimPalettes_Oldale[] = {10, 10, 9, 9, 9, 9, 9, 9};
static const u8 sDoorAnimPalettes_Mossdeep[] = {9, 9, 1, 1, 1, 1, 1, 1};
static const u8 sDoorAnimPalettes_PokemonLeague[] = {8, 8, 8, 8, 8, 8, 8, 8};
static const u8 sDoorAnimPalettes_Pacifidlog[] = {9, 9, 9, 9, 9, 9, 9, 9};
static const u8 sDoorAnimPalettes_SootopolisPeakedRoof[] = {6, 6, 6, 6, 6, 6, 6, 6};
static const u8 sDoorAnimPalettes_Sootopolis[] = {6, 6, 6, 6, 6, 6, 6, 6};
static const u8 sDoorAnimPalettes_Dewford[] = {0, 0, 5, 5, 5, 5, 5, 5};
static const u8 sDoorAnimPalettes_Slateport[] = {6, 6, 1, 1, 1, 1, 1, 1};
static const u8 sDoorAnimPalettes_Mauville[] = {7, 7, 7, 7, 7, 7, 7, 7};
static const u8 sDoorAnimPalettes_Verdanturf[] = {6, 6, 5, 5, 5, 5, 5, 5};
static const u8 sDoorAnimPalettes_LilycoveWooden[] = {5, 5, 5, 5, 5, 5, 5, 5};
static const u8 sDoorAnimPalettes_Contest[] = {1, 1, 1, 1, 1, 1, 1, 1};
static const u8 sDoorAnimPalettes_PetalburgGym[] = {6, 6, 6, 6, 6, 6, 6, 6};
static const u8 sDoorAnimPalettes_CyclingRoad[] = {7, 7, 7, 7, 7, 7, 7, 7};
static const u8 sDoorAnimPalettes_LilycoveDeptStore[] = {5, 5, 5, 5, 5, 5, 5, 5};
static const u8 sDoorAnimPalettes_SafariZone[] = {9, 9, 9, 9, 9, 9, 9, 9};
static const u8 sDoorAnimPalettes_MossdeepSpaceCenter[] = {8, 8, 8, 8, 8, 8, 8, 8};
static const u8 sDoorAnimPalettes_CableClub[] = {6, 6, 6, 6, 6, 6, 6, 6};
static const u8 sDoorAnimPalettes_AbandonedShip[] = {7, 7, 7, 7, 7, 7, 7, 7};
static const u8 sDoorAnimPalettes_FallarborDarkRoof[] = {11, 11, 7, 7, 7, 7, 7, 7};
static const u8 sDoorAnimPalettes_AbandonedShipRoom[] = {7, 7, 7, 7, 7, 7, 7, 7};
static const u8 sDoorAnimPalettes_LilycoveDeptStoreElevator[] = {6, 6, 7, 7, 7, 7, 7, 7};
static const u8 sDoorAnimPalettes_BattleTowerOld[] = {9, 9, 9, 9, 9, 9, 9, 9};
static const u8 sDoorAnimPalettes_BattleTowerElevator[] = {7, 7, 7, 7, 7, 7, 7, 7};
static const u8 sDoorAnimPalettes_UnusedBattleFrontier[] = {9, 9, 9, 9, 9, 9, 9, 9};
static const u8 sDoorAnimPalettes_BattleDome[] = {1, 1, 1, 1, 1, 1, 1, 1};
static const u8 sDoorAnimPalettes_BattleFactory[] = {9, 9, 9, 9, 9, 9, 9, 9};
static const u8 sDoorAnimPalettes_BattleTower[] = {0, 0, 0, 0, 0, 0, 0, 0};
static const u8 sDoorAnimPalettes_BattleArena[] = {5, 5, 5, 5, 5, 5, 5, 5};
static const u8 sDoorAnimPalettes_BattleArenaLobby[] = {7, 7, 7, 7, 7, 7, 7, 7};
static const u8 sDoorAnimPalettes_BattleDomeLobby[] = {7, 7, 7, 7, 7, 7, 7, 7};
static const u8 sDoorAnimPalettes_BattlePalaceLobby[] = {7, 7, 7, 7, 7, 7, 7, 7};
static const u8 sDoorAnimPalettes_BattleTent[] = {1, 1, 1, 1, 1, 1, 1, 1};
static const u8 sDoorAnimPalettes_BattleDomeCorridor[] = {7, 7, 7, 7, 7, 7, 7, 7};
static const u8 sDoorAnimPalettes_BattleTowerMultiCorridor[] = {7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7};
static const u8 sDoorAnimPalettes_Unused[] = {1, 1, 1, 1, 1, 1, 1, 1};
static const u8 sDoorAnimPalettes_BattleFrontier[] = {1, 1, 1, 1, 1, 1, 1, 1};
static const u8 sDoorAnimPalettes_BattleDomePreBattleRoom[] = {9, 9, 7, 7, 7, 7, 7, 7};
static const u8 sDoorAnimPalettes_BattleTentInterior[] = {9, 9, 9, 9, 9, 9, 9, 9};
static const u8 sDoorAnimPalettes_TrainerHillLobbyElevator[] = {7, 7, 7, 7, 7, 7, 7, 7};
static const u8 sDoorAnimPalettes_TrainerHillRoofElevator[] = {9, 9, 7, 7, 7, 7, 7, 7};

static const u8 sDoorAnimPalettes_GeneralFrlg[] = {2, 2, 2, 2, 2, 2, 2, 2};
static const u8 sDoorAnimPalettes_SlidingSingle[] = {3, 3, 3, 3, 3, 3, 3, 3};
static const u8 sDoorAnimPalettes_SlidingDouble[] = {3, 3, 3, 3, 3, 3, 3, 3};
static const u8 sDoorAnimPalettes_Pallet[] = {8, 8, 8, 8, 8, 8, 8, 8};
static const u8 sDoorAnimPalettes_OaksLab[] = {10, 10, 10, 10, 10, 10, 10, 10};
static const u8 sDoorAnimPalettes_Viridian[] = {8, 8, 8, 8, 8, 8, 8, 8};
static const u8 sDoorAnimPalettes_Pewter[] = {8, 8, 8, 8, 8, 8, 8, 8};
static const u8 sDoorAnimPalettes_Saffron[] = {8, 8, 8, 8, 8, 8, 8, 8};
static const u8 sDoorAnimPalettes_SilphCo[] = {3, 3, 3, 3, 3, 3, 3, 3};
static const u8 sDoorAnimPalettes_Cerulean[] = {12, 12, 12, 12, 12, 12, 12, 12};
static const u8 sDoorAnimPalettes_Lavender[] = {9, 9, 9, 9, 9, 9, 9, 9};
static const u8 sDoorAnimPalettes_Vermilion[] = {9, 9, 9, 9, 9, 9, 9, 9};
static const u8 sDoorAnimPalettes_PokemonFanClub[] = {9, 9, 9, 9, 9, 9, 9, 9};
static const u8 sDoorAnimPalettes_DeptStore[] = {3, 3, 3, 3, 3, 3, 3, 3};
static const u8 sDoorAnimPalettes_Fuchsia[] = {8, 8, 8, 8, 8, 8, 8, 8};
static const u8 sDoorAnimPalettes_SafariZoneFrlg[] = {9, 9, 9, 9, 9, 9, 9, 9};
static const u8 sDoorAnimPalettes_CinnabarLab[] = {3, 3, 3, 3, 3, 3, 3, 3};
static const u8 sDoorAnimPalettes_DeptStoreElevator[] = {8, 8, 8, 8, 8, 8, 8, 8};
static const u8 sDoorAnimPalettes_CableClubFrlg[] = {8, 8, 8, 8, 8, 8, 8, 8};
static const u8 sDoorAnimPalettes_HideoutElevator[] = {12, 12, 2, 2, 2, 2, 2, 2};
static const u8 sDoorAnimPalettes_SSAnne[] = {7, 7, 7, 7, 7, 7, 7, 7};
static const u8 sDoorAnimPalettes_SilphCoElevator[] = {8, 8, 2, 2, 2, 2, 2, 2};
static const u8 sDoorAnimPalettes_Sevii123[] = {5, 5, 5, 5, 5, 5, 5, 5};
static const u8 sDoorAnimPalettes_JoyfulGameCorner[] = {3, 3, 3, 3, 3, 3, 3, 3};
static const u8 sDoorAnimPalettes_OneIslandPokeCenter[] = {3, 3, 3, 3, 3, 3, 3, 3};
static const u8 sDoorAnimPalettes_Sevii45[] = {5, 5, 5, 5, 5, 5, 5, 5};
static const u8 sDoorAnimPalettes_FourIslandDayCare[] = {3, 3, 3, 3, 3, 3, 3, 3};
static const u8 sDoorAnimPalettes_RocketWarehouse[] = {10, 10, 10, 10, 10, 10, 10, 10};
static const u8 sDoorAnimPalettes_Sevii67[] = {5, 5, 5, 5, 5, 5, 5, 5};
static const u8 sDoorAnimPalettes_Teleporter[] = {8, 8, 8, 8, 8, 8, 8, 8};

static const u8 sDoorAnimPalettes_JohtoGeneral[] = {2, 2, 2, 2, 2, 2, 2, 2};
static const u8 sDoorAnimPalettes_JohtoGym[] = {3, 3, 3, 3, 3, 3, 3, 3};
static const u8 sDoorAnimPalettes_JohtoPokeCenter[] = {3, 3, 3, 3, 3, 3, 3, 3};
static const u8 sDoorAnimPalettes_JohtoSafariZone[] = {9, 9, 9, 9, 9, 9, 9, 9};
static const u8 sDoorAnimPalettes_Johto_Azalea[] = {5, 5, 5, 5, 5, 5, 5, 5};
static const u8 sDoorAnimPalettes_Johto_Blackthorn[] = {7, 7, 7, 7, 7, 7, 7, 7};
static const u8 sDoorAnimPalettes_Johto_CherrygroveRed[] = {8, 8, 8, 8, 8, 8, 8, 8};
static const u8 sDoorAnimPalettes_Johto_Cianwood[] = {11, 11, 11, 11, 11, 11, 11, 11};
static const u8 sDoorAnimPalettes_Johto_DeptStoreHns[] = {8, 8, 8, 8, 8, 8, 8, 8};
static const u8 sDoorAnimPalettes_Johto_DragonsDenShrine[] = {12, 12, 12, 12, 12, 12, 12, 12};
static const u8 sDoorAnimPalettes_Johto_EcruteakDojo[] = {10, 10, 10, 10, 10, 10, 10, 10};
static const u8 sDoorAnimPalettes_Johto_Goldenrod[] = {10, 10, 10, 10, 10, 10, 10, 10};
static const u8 sDoorAnimPalettes_Johto_NewBarkTownBlue[] = {7, 7, 7, 7, 7, 7, 7, 7};
static const u8 sDoorAnimPalettes_Johto_NewBarkTownRed[] = {8, 8, 8, 8, 8, 8, 8, 8};
static const u8 sDoorAnimPalettes_Johto_NewBarkTownYellow[] = {5, 5, 5, 5, 5, 5, 5, 5};
static const u8 sDoorAnimPalettes_Johto_Olivine[] = {11, 11, 11, 11, 11, 11, 11, 11};
static const u8 sDoorAnimPalettes_Johto_RocketElevator[] = {2, 2, 2, 2, 2, 2, 2, 2};
static const u8 sDoorAnimPalettes_Johto_SSAqua[] = {7, 7, 7, 7, 7, 7, 7, 7};
static const u8 sDoorAnimPalettes_Johto_VermilionHns[] = {9, 9, 9, 9, 9, 9, 9, 9};
static const u8 sDoorAnimPalettes_Johto_VioletDojo[] = {12, 12, 12, 12, 12, 12, 12, 12};
static const u8 sDoorAnimPalettes_Johto_ViridianHns[] = {8, 8, 8, 8, 8, 8, 8, 8};

static const struct DoorGraphics sDoorAnimGraphicsTable[] =
{
    {
        .metatileNum = METATILE_General_Door,
        .tileset = &gTileset_General,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x2,
        .tiles = sDoorAnimTiles_General,
        .palettes = sDoorAnimPalettes_General
    },
    {
        .metatileNum = METATILE_General_Door_PokeCenter,
        .tileset = &gTileset_General,
        .sound = DOOR_SOUND_SLIDING,
        .size = DOOR_SIZE_1x2,
        .tiles = sDoorAnimTiles_PokeCenter,
        .palettes = sDoorAnimPalettes_PokeCenter
    },
    {
        .metatileNum = METATILE_General_Door_Gym,
        .tileset = &gTileset_General,
        .sound = DOOR_SOUND_SLIDING,
        .size = DOOR_SIZE_1x2,
        .tiles = sDoorAnimTiles_Gym,
        .palettes = sDoorAnimPalettes_Gym
    },
    {
        .metatileNum = METATILE_General_Door_PokeMart,
        .tileset = &gTileset_General,
        .sound = DOOR_SOUND_SLIDING,
        .size = DOOR_SIZE_1x2,
        .tiles = sDoorAnimTiles_PokeMart,
        .palettes = sDoorAnimPalettes_PokeMart
    },
    {
        .metatileNum = METATILE_Petalburg_Door_Littleroot,
        .tileset = &gTileset_Petalburg,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x2,
        .tiles = sDoorAnimTiles_Littleroot,
        .palettes = sDoorAnimPalettes_Littleroot
    },
    {
        .metatileNum = METATILE_Petalburg_Door_BirchsLab,
        .tileset = &gTileset_Petalburg,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x2,
        .tiles = sDoorAnimTiles_BirchsLab,
        .palettes = sDoorAnimPalettes_BirchsLab
    },
    {
        .metatileNum = METATILE_Rustboro_Door_Tan,
        .tileset = &gTileset_Rustboro,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x2,
        .tiles = sDoorAnimTiles_RustboroTan,
        .palettes = sDoorAnimPalettes_RustboroTan
    },
    {
        .metatileNum = METATILE_Rustboro_Door_Gray,
        .tileset = &gTileset_Rustboro,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x2,
        .tiles = sDoorAnimTiles_RustboroGray,
        .palettes = sDoorAnimPalettes_RustboroGray
    },
    {
        .metatileNum = METATILE_Fallarbor_Door_LightRoof,
        .tileset = &gTileset_Fallarbor,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x2,
        .tiles = sDoorAnimTiles_FallarborLightRoof,
        .palettes = sDoorAnimPalettes_FallarborLightRoof
    },
    {
        .metatileNum = METATILE_Petalburg_Door_Oldale,
        .tileset = &gTileset_Petalburg,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x2,
        .tiles = sDoorAnimTiles_Oldale,
        .palettes = sDoorAnimPalettes_Oldale
    },
    {
        .metatileNum = METATILE_Mauville_Door,
        .tileset = &gTileset_Mauville,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x2,
        .tiles = sDoorAnimTiles_Mauville,
        .palettes = sDoorAnimPalettes_Mauville
    },
    {
        .metatileNum = METATILE_Mauville_Door_Verdanturf,
        .tileset = &gTileset_Mauville,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x2,
        .tiles = sDoorAnimTiles_Verdanturf,
        .palettes = sDoorAnimPalettes_Verdanturf
    },
    {
        .metatileNum = METATILE_Slateport_Door,
        .tileset = &gTileset_Slateport,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x2,
        .tiles = sDoorAnimTiles_Slateport,
        .palettes = sDoorAnimPalettes_Slateport
    },
    {
        .metatileNum = METATILE_Dewford_Door,
        .tileset = &gTileset_Dewford,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x2,
        .tiles = sDoorAnimTiles_Dewford,
        .palettes = sDoorAnimPalettes_Dewford
    },
    {
        .metatileNum = METATILE_General_Door_Contest,
        .tileset = &gTileset_General,
        .sound = DOOR_SOUND_SLIDING,
        .size = DOOR_SIZE_1x2,
        .tiles = sDoorAnimTiles_Contest,
        .palettes = sDoorAnimPalettes_Contest
    },
    {
        .metatileNum = METATILE_Lilycove_Door,
        .tileset = &gTileset_Lilycove,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x2,
        .tiles = sDoorAnimTiles_Lilycove,
        .palettes = sDoorAnimPalettes_Lilycove
    },
    {
        .metatileNum = METATILE_Lilycove_Door_Wooden,
        .tileset = &gTileset_Lilycove,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x2,
        .tiles = sDoorAnimTiles_LilycoveWooden,
        .palettes = sDoorAnimPalettes_LilycoveWooden
    },
    {
        .metatileNum = METATILE_Mossdeep_Door,
        .tileset = &gTileset_Mossdeep,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x2,
        .tiles = sDoorAnimTiles_Mossdeep,
        .palettes = sDoorAnimPalettes_Mossdeep
    },
    {
        .metatileNum = METATILE_Sootopolis_Door_PeakedRoof,
        .tileset = &gTileset_Sootopolis,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x2,
        .tiles = sDoorAnimTiles_SootopolisPeakedRoof,
        .palettes = sDoorAnimPalettes_SootopolisPeakedRoof
    },
    {
        .metatileNum = METATILE_Sootopolis_Door,
        .tileset = &gTileset_Sootopolis,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x2,
        .tiles = sDoorAnimTiles_Sootopolis,
        .palettes = sDoorAnimPalettes_Sootopolis
    },
    {
        .metatileNum = METATILE_EverGrande_Door_PokemonLeague,
        .tileset = &gTileset_EverGrande,
        .sound = DOOR_SOUND_SLIDING,
        .size = DOOR_SIZE_1x2,
        .tiles = sDoorAnimTiles_PokemonLeague,
        .palettes = sDoorAnimPalettes_PokemonLeague
    },
    {
        .metatileNum = METATILE_Pacifidlog_Door,
        .tileset = &gTileset_Pacifidlog,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x2,
        .tiles = sDoorAnimTiles_Pacifidlog,
        .palettes = sDoorAnimPalettes_Pacifidlog
    },
    {
        .metatileNum = METATILE_PetalburgGym_Door,
        .tileset = &gTileset_PetalburgGym,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x2,
        .tiles = sDoorAnimTiles_PetalburgGym,
        .palettes = sDoorAnimPalettes_PetalburgGym
    },
    {
        .metatileNum = METATILE_Mauville_Door_CyclingRoad,
        .tileset = &gTileset_Mauville,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x2,
        .tiles = sDoorAnimTiles_CyclingRoad,
        .palettes = sDoorAnimPalettes_CyclingRoad
    },
    {
        .metatileNum = METATILE_Lilycove_Door_DeptStore,
        .tileset = &gTileset_Lilycove,
        .sound = DOOR_SOUND_SLIDING,
        .size = DOOR_SIZE_1x2,
        .tiles = sDoorAnimTiles_LilycoveDeptStore,
        .palettes = sDoorAnimPalettes_LilycoveDeptStore
    },
    {
        .metatileNum = METATILE_Lilycove_Door_SafariZone,
        .tileset = &gTileset_Lilycove,
        .sound = DOOR_SOUND_SLIDING,
        .size = DOOR_SIZE_1x2,
        .tiles = sDoorAnimTiles_SafariZone,
        .palettes = sDoorAnimPalettes_SafariZone
    },
    {
        .metatileNum = METATILE_Mossdeep_Door_SpaceCenter,
        .tileset = &gTileset_Mossdeep,
        .sound = DOOR_SOUND_SLIDING,
        .size = DOOR_SIZE_1x2,
        .tiles = sDoorAnimTiles_MossdeepSpaceCenter,
        .palettes = sDoorAnimPalettes_MossdeepSpaceCenter
    },
    {
        .metatileNum = METATILE_PokemonCenter_Door_CableClub,
        .tileset = &gTileset_PokemonCenter,
        .sound = DOOR_SOUND_SLIDING,
        .size = DOOR_SIZE_1x2,
        .tiles = sDoorAnimTiles_CableClub,
        .palettes = sDoorAnimPalettes_CableClub
    },
    {
        .metatileNum = METATILE_InsideShip_IntactDoor_Bottom_Unlocked,
        .tileset = &gTileset_InsideShip,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x2, sDoorAnimTiles_AbandonedShip,
        .palettes = sDoorAnimPalettes_AbandonedShip
    },
    {
        .metatileNum = METATILE_Fallarbor_Door_DarkRoof,
        .tileset = &gTileset_Fallarbor,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x2,
        .tiles = sDoorAnimTiles_FallarborDarkRoof,
        .palettes = sDoorAnimPalettes_FallarborDarkRoof
    },
    {
        .metatileNum = METATILE_InsideShip_IntactDoor_Bottom_Interior,
        .tileset = &gTileset_InsideShip,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x2,
        .tiles = sDoorAnimTiles_AbandonedShipRoom,
        .palettes = sDoorAnimPalettes_AbandonedShipRoom
    },
    {
        .metatileNum = METATILE_Shop_Door_Elevator,
        .tileset = &gTileset_Shop,
        .sound = DOOR_SOUND_SLIDING,
        .size = DOOR_SIZE_1x2,
        .tiles = sDoorAnimTiles_LilycoveDeptStoreElevator,
        .palettes = sDoorAnimPalettes_LilycoveDeptStoreElevator
    },
    {
        .metatileNum = METATILE_Dewford_Door_BattleTower,
        .tileset = &gTileset_Dewford,
        .sound = DOOR_SOUND_SLIDING,
        .size = DOOR_SIZE_1x2,
        .tiles = sDoorAnimTiles_BattleTowerOld,
        .palettes = sDoorAnimPalettes_BattleTowerOld
    },
    {
        .metatileNum = METATILE_BattleFrontier_Door_Elevator,
        .tileset = &gTileset_BattleFrontier,
        .sound = DOOR_SOUND_SLIDING,
        .size = DOOR_SIZE_1x2,
        .tiles = sDoorAnimTiles_BattleTowerElevator,
        .palettes = sDoorAnimPalettes_BattleTowerElevator
    },
    {
        .metatileNum = METATILE_BattleFrontierOutsideWest_Door_BattleDome,
        .tileset = &gTileset_BattleFrontierOutsideWest,
        .sound = DOOR_SOUND_SLIDING,
        .size = DOOR_SIZE_1x2,
        .tiles = sDoorAnimTiles_BattleDome,
        .palettes = sDoorAnimPalettes_BattleDome
    },
    {
        .metatileNum = METATILE_BattleFrontierOutsideWest_Door_BattleFactory,
        .tileset = &gTileset_BattleFrontierOutsideWest,
        .sound = DOOR_SOUND_SLIDING,
        .size = DOOR_SIZE_1x2,
        .tiles = sDoorAnimTiles_BattleFactory,
        .palettes = sDoorAnimPalettes_BattleFactory
    },
    {
        .metatileNum = METATILE_BattleFrontierOutsideEast_Door_BattleTower,
        .tileset = &gTileset_BattleFrontierOutsideEast,
        .sound = DOOR_SOUND_SLIDING,
        .size = DOOR_SIZE_1x2,
        .tiles = sDoorAnimTiles_BattleTower,
        .palettes = sDoorAnimPalettes_BattleTower
    },
    {
        .metatileNum = METATILE_BattleFrontierOutsideEast_Door_BattleArena,
        .tileset = &gTileset_BattleFrontierOutsideEast,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x2,
        .tiles = sDoorAnimTiles_BattleArena,
        .palettes = sDoorAnimPalettes_BattleArena
    },
    {
        .metatileNum = METATILE_BattleArena_Door,
        .tileset = &gTileset_BattleArena,
        .sound = DOOR_SOUND_ARENA,
        .size = DOOR_SIZE_1x2,
        .tiles = sDoorAnimTiles_BattleArenaLobby,
        .palettes = sDoorAnimPalettes_BattleArenaLobby
    },
    {
        .metatileNum = METATILE_BattleDome_Door_Lobby,
        .tileset = &gTileset_BattleDome,
        .sound = DOOR_SOUND_SLIDING,
        .size = DOOR_SIZE_1x2,
        .tiles = sDoorAnimTiles_BattleDomeLobby,
        .palettes = sDoorAnimPalettes_BattleDomeLobby
    },
    {
        .metatileNum = METATILE_BattlePalace_Door,
        .tileset = &gTileset_BattlePalace,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x2,
        .tiles = sDoorAnimTiles_BattlePalaceLobby,
        .palettes = sDoorAnimPalettes_BattlePalaceLobby
    },
    {
        .metatileNum = METATILE_Slateport_Door_BattleTent,
        .tileset = &gTileset_Slateport,
        .sound = DOOR_SOUND_SLIDING,
        .size = DOOR_SIZE_1x2,
        .tiles = sDoorAnimTiles_BattleTent,
        .palettes = sDoorAnimPalettes_BattleTent
    },
    {
        .metatileNum = METATILE_Mauville_Door_BattleTent,
        .tileset = &gTileset_Mauville,
        .sound = DOOR_SOUND_SLIDING,
        .size = DOOR_SIZE_1x2,
        .tiles = sDoorAnimTiles_BattleTent,
        .palettes = sDoorAnimPalettes_BattleTent
    },
    {
        .metatileNum = METATILE_Fallarbor_Door_BattleTent,
        .tileset = &gTileset_Fallarbor,
        .sound = DOOR_SOUND_SLIDING,
        .size = DOOR_SIZE_1x2,
        .tiles = sDoorAnimTiles_BattleTent,
        .palettes = sDoorAnimPalettes_BattleTent
    },
    {
        .metatileNum = METATILE_BattleDome_Door_Corridor,
        .tileset = &gTileset_BattleDome,
        .sound = DOOR_SOUND_SLIDING,
        .size = DOOR_SIZE_1x2,
        .tiles = sDoorAnimTiles_BattleDomeCorridor,
        .palettes = sDoorAnimPalettes_BattleDomeCorridor
    },
    {
        .metatileNum = METATILE_BattleFrontier_Door_MultiCorridor_Left,
        .tileset = &gTileset_BattleFrontier,
        .sound = DOOR_SOUND_SLIDING,
        .size = DOOR_SIZE_2x2_LEFT,
        .tiles = sDoorAnimTiles_BattleTowerMultiCorridor,
        .palettes = sDoorAnimPalettes_BattleTowerMultiCorridor
    },
    {
        .metatileNum = METATILE_BattleFrontier_Door_MultiCorridor_Right,
        .tileset = &gTileset_BattleFrontier,
        .sound = DOOR_SOUND_SLIDING,
        .size = DOOR_SIZE_2x2_RIGHT,
        .tiles = sDoorAnimTiles_BattleTowerMultiCorridor,
        .palettes = sDoorAnimPalettes_BattleTowerMultiCorridor
    },
    {
        .metatileNum = METATILE_BattleFrontierOutsideWest_Door,
        .tileset = &gTileset_BattleFrontierOutsideWest,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x2,
        .tiles = sDoorAnimTiles_BattleFrontier,
        .palettes = sDoorAnimPalettes_BattleFrontier
    },
    {
        .metatileNum = METATILE_BattleFrontierOutsideWest_Door_Sliding,
        .tileset = &gTileset_BattleFrontierOutsideWest,
        .sound = DOOR_SOUND_SLIDING,
        .size = DOOR_SIZE_1x2,
        .tiles = sDoorAnimTiles_BattleFrontierSliding,
        .palettes = sDoorAnimPalettes_BattleFrontier
    },
    {
        .metatileNum = METATILE_BattleDome_Door_PreBattleRoom,
        .tileset = &gTileset_BattleDome,
        .sound = DOOR_SOUND_SLIDING,
        .size = DOOR_SIZE_1x2,
        .tiles = sDoorAnimTiles_BattleDomePreBattleRoom,
        .palettes = sDoorAnimPalettes_BattleDomePreBattleRoom
    },
    {
        .metatileNum = METATILE_BattleTent_Door,
        .tileset = &gTileset_BattleTent,
        .sound = DOOR_SOUND_SLIDING,
        .size = DOOR_SIZE_1x2,
        .tiles = sDoorAnimTiles_BattleTentInterior,
        .palettes = sDoorAnimPalettes_BattleTentInterior
    },
    {
        .metatileNum = METATILE_TrainerHill_Door_Elevator_Lobby,
        .tileset = &gTileset_TrainerHill,
        .sound = DOOR_SOUND_SLIDING,
        .size = DOOR_SIZE_1x2,
        .tiles = sDoorAnimTiles_TrainerHillLobbyElevator,
        .palettes = sDoorAnimPalettes_TrainerHillLobbyElevator
    },
    {
        .metatileNum = METATILE_TrainerHill_Door_Elevator_Roof,
        .tileset = &gTileset_TrainerHill,
        .sound = DOOR_SOUND_SLIDING,
        .size = DOOR_SIZE_1x2,
        .tiles = sDoorAnimTiles_TrainerHillRoofElevator,
        .palettes = sDoorAnimPalettes_TrainerHillRoofElevator
    },
    {
        .metatileNum = METATILE_GeneralFrlg_Door,
        .tileset = &gTileset_GeneralKanto,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_GeneralFrlg,
        .palettes = sDoorAnimPalettes_GeneralFrlg
    },
    {
        .metatileNum = METATILE_GeneralFrlg_SlidingSingleDoor,
        .tileset = &gTileset_GeneralKanto,
        .sound = DOOR_SOUND_SLIDING,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_SlidingSingle,
        .palettes = sDoorAnimPalettes_SlidingSingle
    },
    {
        .metatileNum = METATILE_GeneralFrlg_SlidingDoubleDoor,
        .tileset = &gTileset_GeneralKanto,
        .sound = DOOR_SOUND_SLIDING,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_SlidingDouble,
        .palettes = sDoorAnimPalettes_SlidingDouble
    },
    {
        .metatileNum = METATILE_PalletTown_Door,
        .tileset = &gTileset_PalletTown,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_Pallet,
        .palettes = sDoorAnimPalettes_Pallet
    },
    {
        .metatileNum = METATILE_PalletTown_OaksLabDoor,
        .tileset = &gTileset_PalletTown,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_OaksLab,
        .palettes = sDoorAnimPalettes_OaksLab
    },
    {
        .metatileNum = METATILE_ViridianCity_Door,
        .tileset = &gTileset_ViridianCity,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_Viridian,
        .palettes = sDoorAnimPalettes_Viridian
    },
    {
        .metatileNum = METATILE_PewterCity_Door,
        .tileset = &gTileset_PewterCity,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_Pewter,
        .palettes = sDoorAnimPalettes_Pewter
    },
    {
        .metatileNum = METATILE_SaffronCity_Door,
        .tileset = &gTileset_SaffronCity,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_Saffron,
        .palettes = sDoorAnimPalettes_Saffron
    },
    {
        .metatileNum = METATILE_SaffronCity_SilphCoDoor,
        .tileset = &gTileset_SaffronCity,
        .sound = DOOR_SOUND_SLIDING,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_SilphCo,
        .palettes = sDoorAnimPalettes_SilphCo
    },
    {
        .metatileNum = METATILE_CeruleanCity_Door,
        .tileset = &gTileset_CeruleanCity,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_Cerulean,
        .palettes = sDoorAnimPalettes_Cerulean
    },
    {
        .metatileNum = METATILE_LavenderTown_Door,
        .tileset = &gTileset_LavenderTown,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_Lavender,
        .palettes = sDoorAnimPalettes_Lavender
    },
    {
        .metatileNum = METATILE_VermilionCity_Door,
        .tileset = &gTileset_VermilionCity,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_Vermilion,
        .palettes = sDoorAnimPalettes_Vermilion
    },
    {
        .metatileNum = METATILE_CeladonCity_DeptStoreDoor,
        .tileset = &gTileset_CeladonCity,
        .sound = DOOR_SOUND_SLIDING,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_DeptStore,
        .palettes = sDoorAnimPalettes_DeptStore
    },
    {
        .metatileNum = METATILE_FuchsiaCity_Door,
        .tileset = &gTileset_FuchsiaCity,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_Fuchsia,
        .palettes = sDoorAnimPalettes_Fuchsia
    },
    {
        .metatileNum = METATILE_FuchsiaCity_SafariZoneDoor,
        .tileset = &gTileset_FuchsiaCity,
        .sound = DOOR_SOUND_SLIDING,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_SafariZoneFrlg,
        .palettes = sDoorAnimPalettes_SafariZoneFrlg
    },
    {
        .metatileNum = METATILE_CinnabarIsland_LabDoor,
        .tileset = &gTileset_CinnabarIsland,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_CinnabarLab,
        .palettes = sDoorAnimPalettes_CinnabarLab
    },
    {
        .metatileNum = METATILE_SeviiIslands123_Door,
        .tileset = &gTileset_SeviiIslands123,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_Sevii123,
        .palettes = sDoorAnimPalettes_Sevii123
    },
    {
        .metatileNum = METATILE_SeviiIslands123_GameCornerDoor,
        .tileset = &gTileset_SeviiIslands123,
        .sound = DOOR_SOUND_SLIDING,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_JoyfulGameCorner,
        .palettes = sDoorAnimPalettes_JoyfulGameCorner
    },
    {
        .metatileNum = METATILE_SeviiIslands123_PokeCenterDoor,
        .tileset = &gTileset_SeviiIslands123,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_OneIslandPokeCenter,
        .palettes = sDoorAnimPalettes_OneIslandPokeCenter
    },
    {
        .metatileNum = METATILE_SeviiIslands45_Door,
        .tileset = &gTileset_SeviiIslands45,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_Sevii45,
        .palettes = sDoorAnimPalettes_Sevii45
    },
    {
        .metatileNum = METATILE_SeviiIslands45_DayCareDoor,
        .tileset = &gTileset_SeviiIslands45,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_FourIslandDayCare,
        .palettes = sDoorAnimPalettes_FourIslandDayCare
    },
    {
        .metatileNum = METATILE_SeviiIslands45_RocketWarehouseDoor_Unlocked,
        .tileset = &gTileset_SeviiIslands45,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_RocketWarehouse,
        .palettes = sDoorAnimPalettes_RocketWarehouse
    },
    {
        .metatileNum = METATILE_SeviiIslands67_Door,
        .tileset = &gTileset_SeviiIslands67,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_Sevii67,
        .palettes = sDoorAnimPalettes_Sevii67
    },
    {
        .metatileNum = METATILE_DepartmentStore_ElevatorDoor,
        .tileset = &gTileset_DepartmentStore,
        .sound = DOOR_SOUND_SLIDING,
        .size = DOOR_SIZE_1x2,
        .tiles = sDoorAnimTiles_DeptStoreElevator,
        .palettes = sDoorAnimPalettes_DeptStoreElevator
    },
    {
        .metatileNum = METATILE_PokemonCenterFrlg_CableClubDoor,
        .tileset = &gTileset_PokemonCenterFrlg,
        .sound = DOOR_SOUND_SLIDING,
        .size = DOOR_SIZE_1x2,
        .tiles = sDoorAnimTiles_CableClubFrlg,
        .palettes = sDoorAnimPalettes_CableClubFrlg
    },
    {
        .metatileNum = METATILE_SilphCo_HideoutElevatorDoor,
        .tileset = &gTileset_SilphCo,
        .sound = DOOR_SOUND_SLIDING,
        .size = DOOR_SIZE_1x2,
        .tiles = sDoorAnimTiles_HideoutElevator,
        .palettes = sDoorAnimPalettes_HideoutElevator
    },
    {
        .metatileNum = METATILE_SSAnne_Door,
        .tileset = &gTileset_SSAnne,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x2,
        .tiles = sDoorAnimTiles_SSAnne,
        .palettes = sDoorAnimPalettes_SSAnne
    },
    {
        .metatileNum = METATILE_SilphCo_ElevatorDoor,
        .tileset = &gTileset_SilphCo,
        .sound = DOOR_SOUND_SLIDING,
        .size = DOOR_SIZE_1x2,
        .tiles = sDoorAnimTiles_SilphCoElevator,
        .palettes = sDoorAnimPalettes_SilphCoElevator
    },
    {
        .metatileNum = METATILE_SeaCottage_Teleporter_Door,
        .tileset = &gTileset_SeaCottage,
        .sound = DOOR_SOUND_SLIDING,
        .size = DOOR_SIZE_1x2,
        .tiles = sDoorAnimTiles_Teleporter,
        .palettes = sDoorAnimPalettes_Teleporter
    },
    {
        .metatileNum = METATILE_Johto_General_Door,
        .tileset = &gTileset_JohtoGeneral,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_JohtoGeneral,
        .palettes = sDoorAnimPalettes_JohtoGeneral,
        .tileStart = DOOR_TILE_START_JOHTO
    },
    {
        .metatileNum = METATILE_Johto_General_Door_Sliding,
        .tileset = &gTileset_JohtoGeneral,
        .sound = DOOR_SOUND_SLIDING,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_JohtoPokeCenter,
        .palettes = sDoorAnimPalettes_JohtoPokeCenter,
        .tileStart = DOOR_TILE_START_JOHTO
    },
    {
        .metatileNum = METATILE_Johto_General_Door_Gym,
        .tileset = &gTileset_JohtoGeneral,
        .sound = DOOR_SOUND_SLIDING,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_JohtoGym,
        .palettes = sDoorAnimPalettes_JohtoGym,
        .tileStart = DOOR_TILE_START_JOHTO
    },
    {
        .metatileNum = METATILE_Johto_General_Door,
        .tileset = &gTileset_JohtoSouth,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_JohtoGeneral,
        .palettes = sDoorAnimPalettes_JohtoGeneral,
        .tileStart = DOOR_TILE_START_JOHTO
    },
    {
        .metatileNum = METATILE_Johto_General_Door_Sliding,
        .tileset = &gTileset_JohtoSouth,
        .sound = DOOR_SOUND_SLIDING,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_JohtoPokeCenter,
        .palettes = sDoorAnimPalettes_JohtoPokeCenter,
        .tileStart = DOOR_TILE_START_JOHTO
    },
    {
        .metatileNum = METATILE_Johto_General_Door_Gym,
        .tileset = &gTileset_JohtoSouth,
        .sound = DOOR_SOUND_SLIDING,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_JohtoGym,
        .palettes = sDoorAnimPalettes_JohtoGym,
        .tileStart = DOOR_TILE_START_JOHTO
    },
    {
        .metatileNum = METATILE_Johto_General_Door,
        .tileset = &gTileset_JohtoNorthEast,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_JohtoGeneral,
        .palettes = sDoorAnimPalettes_JohtoGeneral,
        .tileStart = DOOR_TILE_START_JOHTO
    },
    {
        .metatileNum = METATILE_Johto_General_Door_Sliding,
        .tileset = &gTileset_JohtoNorthEast,
        .sound = DOOR_SOUND_SLIDING,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_JohtoPokeCenter,
        .palettes = sDoorAnimPalettes_JohtoPokeCenter,
        .tileStart = DOOR_TILE_START_JOHTO
    },
    {
        .metatileNum = METATILE_Johto_General_Door_Gym,
        .tileset = &gTileset_JohtoNorthEast,
        .sound = DOOR_SOUND_SLIDING,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_JohtoGym,
        .palettes = sDoorAnimPalettes_JohtoGym,
        .tileStart = DOOR_TILE_START_JOHTO
    },
    {
        .metatileNum = METATILE_Johto_General_Door,
        .tileset = &gTileset_JohtoNorthWest,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_JohtoGeneral,
        .palettes = sDoorAnimPalettes_JohtoGeneral,
        .tileStart = DOOR_TILE_START_JOHTO
    },
    {
        .metatileNum = METATILE_Johto_General_Door_Sliding,
        .tileset = &gTileset_JohtoNorthWest,
        .sound = DOOR_SOUND_SLIDING,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_JohtoPokeCenter,
        .palettes = sDoorAnimPalettes_JohtoPokeCenter,
        .tileStart = DOOR_TILE_START_JOHTO
    },
    {
        .metatileNum = METATILE_Johto_General_Door_Gym,
        .tileset = &gTileset_JohtoNorthWest,
        .sound = DOOR_SOUND_SLIDING,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_JohtoGym,
        .palettes = sDoorAnimPalettes_JohtoGym,
        .tileStart = DOOR_TILE_START_JOHTO
    },
    {
        .metatileNum = METATILE_Johto_General_Door,
        .tileset = &gTileset_JohtoBuilding,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_JohtoGeneral,
        .palettes = sDoorAnimPalettes_JohtoGeneral,
        .tileStart = DOOR_TILE_START_JOHTO
    },
    {
        .metatileNum = METATILE_Johto_General_Door_Sliding,
        .tileset = &gTileset_JohtoBuilding,
        .sound = DOOR_SOUND_SLIDING,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_JohtoPokeCenter,
        .palettes = sDoorAnimPalettes_JohtoPokeCenter,
        .tileStart = DOOR_TILE_START_JOHTO
    },
    {
        .metatileNum = METATILE_Johto_General_Door_Gym,
        .tileset = &gTileset_JohtoBuilding,
        .sound = DOOR_SOUND_SLIDING,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_JohtoGym,
        .palettes = sDoorAnimPalettes_JohtoGym,
        .tileStart = DOOR_TILE_START_JOHTO
    },
    {
        .metatileNum = METATILE_Johto_General_Door,
        .tileset = &gTileset_JohtoKantoGeneral,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_JohtoGeneral,
        .palettes = sDoorAnimPalettes_JohtoGeneral,
        .tileStart = DOOR_TILE_START_JOHTO
    },
    {
        .metatileNum = METATILE_Johto_General_Door_Sliding,
        .tileset = &gTileset_JohtoKantoGeneral,
        .sound = DOOR_SOUND_SLIDING,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_JohtoPokeCenter,
        .palettes = sDoorAnimPalettes_JohtoPokeCenter,
        .tileStart = DOOR_TILE_START_JOHTO
    },
    {
        .metatileNum = METATILE_Johto_General_Door_Gym,
        .tileset = &gTileset_JohtoKantoGeneral,
        .sound = DOOR_SOUND_SLIDING,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_JohtoGym,
        .palettes = sDoorAnimPalettes_JohtoGym,
        .tileStart = DOOR_TILE_START_JOHTO
    },
    {
        .metatileNum = METATILE_Goldenrod_Door,
        .tileset = &gTileset_Goldenrod_Johto,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_Johto_Goldenrod,
        .palettes = sDoorAnimPalettes_Johto_Goldenrod,
        .tileStart = DOOR_TILE_START_JOHTO
    },
    {
        .metatileNum = METATILE_NewBarkTown_Door_Yellow,
        .tileset = &gTileset_NewBarkTown_Johto,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_Johto_NewBarkTownYellow,
        .palettes = sDoorAnimPalettes_Johto_NewBarkTownYellow,
        .tileStart = DOOR_TILE_START_JOHTO
    },
    {
        .metatileNum = METATILE_NewBarkTown_Door_Red,
        .tileset = &gTileset_NewBarkTown_Johto,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_Johto_NewBarkTownRed,
        .palettes = sDoorAnimPalettes_Johto_NewBarkTownRed,
        .tileStart = DOOR_TILE_START_JOHTO
    },
    {
        .metatileNum = METATILE_NewBarkTown_Door_Blue,
        .tileset = &gTileset_NewBarkTown_Johto,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_Johto_HnsCerulean,
        .palettes = sDoorAnimPalettes_Johto_NewBarkTownBlue,
        .tileStart = DOOR_TILE_START_JOHTO
    },
    {
        .metatileNum = METATILE_AzaleaTown_Door,
        .tileset = &gTileset_AzaleaTown_Johto,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_Johto_NewBarkTownYellow,
        .palettes = sDoorAnimPalettes_Johto_Azalea,
        .tileStart = DOOR_TILE_START_JOHTO
    },
    {
        .metatileNum = METATILE_MahoganyTown_Door,
        .tileset = &gTileset_MahoganyTown_Johto,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_Johto_NewBarkTownYellow,
        .palettes = sDoorAnimPalettes_Johto_Azalea,
        .tileStart = DOOR_TILE_START_JOHTO
    },
    {
        .metatileNum = METATILE_CianwoodCity_Door,
        .tileset = &gTileset_CianwoodCity_Johto,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_Johto_Cianwood,
        .palettes = sDoorAnimPalettes_Johto_Cianwood,
        .tileStart = DOOR_TILE_START_JOHTO
    },
    {
        .metatileNum = METATILE_ViridianCity_Door_Johto,
        .tileset = &gTileset_ViridianCity_Johto,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_Johto_HnsViridian,
        .palettes = sDoorAnimPalettes_Johto_ViridianHns,
        .tileStart = DOOR_TILE_START_JOHTO
    },
    {
        .metatileNum = METATILE_ViridianCity_Door_Johto,
        .tileset = &gTileset_ViridianCity_NorthEast_Johto,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_Johto_HnsViridian,
        .palettes = sDoorAnimPalettes_Johto_ViridianHns,
        .tileStart = DOOR_TILE_START_JOHTO
    },
    {
        .metatileNum = METATILE_OlivineCity_Door_Vermilion,
        .tileset = &gTileset_OlivineCity_Johto,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_Johto_HnsVermilion,
        .palettes = sDoorAnimPalettes_Johto_VermilionHns,
        .tileStart = DOOR_TILE_START_JOHTO
    },
    {
        .metatileNum = METATILE_SafariZoneJohto_Door,
        .tileset = &gTileset_SafariZoneJohto_General_Johto,
        .sound = DOOR_SOUND_SLIDING,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_Johto_HnsSafariZone,
        .palettes = sDoorAnimPalettes_JohtoSafariZone,
        .tileStart = DOOR_TILE_START_JOHTO
    },
    {
        .metatileNum = METATILE_SafariZoneJohto_Door,
        .tileset = &gTileset_SafariZoneJohto_Johto,
        .sound = DOOR_SOUND_SLIDING,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_Johto_HnsSafariZone,
        .palettes = sDoorAnimPalettes_JohtoSafariZone,
        .tileStart = DOOR_TILE_START_JOHTO
    },
    {
        .metatileNum = METATILE_CherrygroveCity_Door_Red,
        .tileset = &gTileset_CherrygroveCity_Johto,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_Johto_NewBarkTownRed,
        .palettes = sDoorAnimPalettes_Johto_CherrygroveRed,
        .tileStart = DOOR_TILE_START_JOHTO
    },
    {
        .metatileNum = METATILE_CherrygroveCity_Door_Red,
        .tileset = &gTileset_CherrygroveCity_NorthEast_Johto,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_Johto_NewBarkTownRed,
        .palettes = sDoorAnimPalettes_Johto_CherrygroveRed,
        .tileStart = DOOR_TILE_START_JOHTO
    },
    {
        .metatileNum = METATILE_VioletCity_Dojo_Door,
        .tileset = &gTileset_VioletCity_Johto,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_Johto_DojoDoor,
        .palettes = sDoorAnimPalettes_Johto_VioletDojo,
        .tileStart = DOOR_TILE_START_JOHTO
    },
    {
        .metatileNum = METATILE_EcruteakCity_Door,
        .tileset = &gTileset_Ecruteak_City_General_Johto,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_Johto_DojoDoorEcruteak,
        .palettes = sDoorAnimPalettes_Johto_EcruteakDojo,
        .tileStart = DOOR_TILE_START_JOHTO
    },
    {
        .metatileNum = METATILE_EcruteakCity_Door,
        .tileset = &gTileset_Ecruteak_City_Johto,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_Johto_DojoDoorEcruteak,
        .palettes = sDoorAnimPalettes_Johto_EcruteakDojo,
        .tileStart = DOOR_TILE_START_JOHTO
    },
    {
        .metatileNum = METATILE_BellchimeTrail_Door,
        .tileset = &gTileset_BellchimeTrail_Johto,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_Johto_DojoDoorEcruteak,
        .palettes = sDoorAnimPalettes_Johto_EcruteakDojo,
        .tileStart = DOOR_TILE_START_JOHTO
    },
    {
        .metatileNum = METATILE_OlivineCity_Door,
        .tileset = &gTileset_OlivineCity_Johto,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_Johto_Olivine,
        .palettes = sDoorAnimPalettes_Johto_Olivine,
        .tileStart = DOOR_TILE_START_JOHTO
    },
    {
        .metatileNum = METATILE_Blackthorn_Door,
        .tileset = &gTileset_Blackthorn_General_Johto,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_Johto_BlackthornCity,
        .palettes = sDoorAnimPalettes_Johto_Blackthorn,
        .tileStart = DOOR_TILE_START_JOHTO
    },
    {
        .metatileNum = METATILE_Blackthorn_Door,
        .tileset = &gTileset_Blackthorn_Johto,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_Johto_BlackthornCity,
        .palettes = sDoorAnimPalettes_Johto_Blackthorn,
        .tileStart = DOOR_TILE_START_JOHTO
    },
    {
        .metatileNum = METATILE_DepartmentStore_Door,
        .tileset = &gTileset_DepartmentStore_Johto,
        .sound = DOOR_SOUND_SLIDING,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_Johto_HnsDeptStoreElevator,
        .palettes = sDoorAnimPalettes_Johto_DeptStoreHns,
        .tileStart = DOOR_TILE_START_JOHTO
    },
    {
        .metatileNum = METATILE_GoldenrodUnderground_Elevator,
        .tileset = &gTileset_GoldenrodUndergroundTunnel_Johto,
        .sound = DOOR_SOUND_SLIDING,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_Johto_RocketElevator,
        .palettes = sDoorAnimPalettes_Johto_RocketElevator,
        .tileStart = DOOR_TILE_START_JOHTO
    },
    {
        .metatileNum = METATILE_GoldenrodUnderground_Elevator,
        .tileset = &gTileset_Goldenrod_Underground_Storage_Johto,
        .sound = DOOR_SOUND_SLIDING,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_Johto_RocketElevator,
        .palettes = sDoorAnimPalettes_Johto_RocketElevator,
        .tileStart = DOOR_TILE_START_JOHTO
    },
    {
        .metatileNum = METATILE_SSAqua_Door,
        .tileset = &gTileset_ssaqua_General_Johto,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_Johto_SSAqua,
        .palettes = sDoorAnimPalettes_Johto_SSAqua,
        .tileStart = DOOR_TILE_START_JOHTO
    },
    {
        .metatileNum = METATILE_SSAqua_Door,
        .tileset = &gTileset_ssaqua_Johto,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_Johto_SSAqua,
        .palettes = sDoorAnimPalettes_Johto_SSAqua,
        .tileStart = DOOR_TILE_START_JOHTO
    },
    {
        .metatileNum = METATILE_DragonDen_Shrine_Door,
        .tileset = &gTileset_Cave_DragonsDen_Johto,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_Johto_DojoDoor,
        .palettes = sDoorAnimPalettes_Johto_DragonsDenShrine,
        .tileStart = DOOR_TILE_START_JOHTO
    },
    {
        .metatileNum = METATILE_DragonDen_Shrine_Door,
        .tileset = &gTileset_Cave_DragonsDen_NorthEast_Johto,
        .sound = DOOR_SOUND_NORMAL,
        .size = DOOR_SIZE_1x1,
        .tiles = sDoorAnimTiles_Johto_DojoDoor,
        .palettes = sDoorAnimPalettes_Johto_DragonsDenShrine,
        .tileStart = DOOR_TILE_START_JOHTO
    },
    {},
};

// NOTE: The tiles of a door's animation must be copied to VRAM because they are not already part of any given tileset.
//       This means that if there are any pre-existing tiles in this copied region that are visible when the door
//       animation is played they will be overwritten.
#define DOOR_TILE_START_SIZE1 (NUM_TILES_TOTAL - 8)
#define DOOR_TILE_START_SIZE2 (NUM_TILES_TOTAL - 16)
static u16 GetDoorTileStart(const struct DoorGraphics *gfx)
{
    if (gfx->tileStart != 0)
        return gfx->tileStart;
    if (gfx->size == DOOR_SIZE_2x2_LEFT || gfx->size == DOOR_SIZE_2x2_RIGHT)
        return DOOR_TILE_START_SIZE2;
    return DOOR_TILE_START_SIZE1;
}

static void CopyDoorTilesToVram(const struct DoorGraphics *gfx, const struct DoorAnimFrame *frame)
{
    u32 tileCount = (gfx->size == DOOR_SIZE_2x2_LEFT || gfx->size == DOOR_SIZE_2x2_RIGHT) ? 16 : 8;
    CpuFastCopy(gfx->tiles + frame->offset, (void *)(VRAM + TILE_OFFSET_4BPP(GetDoorTileStart(gfx))), tileCount * TILE_SIZE_4BPP);
}

static void BuildDoorTiles(u16 *tiles, u16 tileNum, const u8 *paletteNums)
{
    int i;
    u16 tile;

    // Only the first 4 tiles of each metatile (bottom layer) actually use the door tiles
    for (i = 0; i < 4; i++)
    {
        tile = *(paletteNums++) << 12;
        tiles[i] = tile | (tileNum + i);
    }

    // The remaining layers are left as tile 0 (with the same palette)
    for (; i < 8; i++)
    {
        tile = *(paletteNums++) << 12;
        tiles[i] = tile;
    }
}

static void DrawCurrentDoorAnimFrame(const struct DoorGraphics *gfx, u32 x, u32 y, const u8 *paletteNums)
{
    u16 tiles[24];
    u16 tileStart = GetDoorTileStart(gfx);

    switch (gfx->size)
    {
    case DOOR_SIZE_2x2_LEFT:
        // Top left metatile
        BuildDoorTiles(&tiles[8], tileStart + 0, &paletteNums[0]);
        DrawDoorMetatileAt(x, y - 1, &tiles[8]);

        // Bottom left metatile
        BuildDoorTiles(&tiles[8], tileStart + 4, &paletteNums[4]);
        DrawDoorMetatileAt(x, y, &tiles[8]);

        // Top right metatile
        BuildDoorTiles(&tiles[8], tileStart + 8, &paletteNums[8]);
        DrawDoorMetatileAt(x + 1, y - 1, &tiles[8]);

        // Bottom right metatile
        BuildDoorTiles(&tiles[8], tileStart + 12, &paletteNums[12]);
        DrawDoorMetatileAt(x + 1, y, &tiles[8]);
        break;
    case DOOR_SIZE_2x2_RIGHT:
        // Top left metatile
        BuildDoorTiles(&tiles[8], tileStart + 0, &paletteNums[0]);
        DrawDoorMetatileAt(x - 1, y - 1, &tiles[8]);

        // Bottom left metatile
        BuildDoorTiles(&tiles[8], tileStart + 4, &paletteNums[4]);
        DrawDoorMetatileAt(x - 1, y, &tiles[8]);

        // Top right metatile
        BuildDoorTiles(&tiles[8], tileStart + 8, &paletteNums[8]);
        DrawDoorMetatileAt(x, y - 1, &tiles[8]);

        // Bottom right metatile
        BuildDoorTiles(&tiles[8], tileStart + 12, &paletteNums[12]);
        DrawDoorMetatileAt(x, y, &tiles[8]);
        break;
    case DOOR_SIZE_1x2:
        // Top metatile
        BuildDoorTiles(&tiles[0], tileStart + 0, &paletteNums[0]);
        DrawDoorMetatileAt(x, y - 1, &tiles[0]);

        // Bottom metatile
        BuildDoorTiles(&tiles[0], tileStart + 4, &paletteNums[4]);
        DrawDoorMetatileAt(x, y, &tiles[0]);
        break;
    default:
    case DOOR_SIZE_1x1:
        BuildDoorTiles(&tiles[0], tileStart + 0, &paletteNums[0]);
        DrawDoorMetatileAt(x, y, &tiles[0]);
        break;
    }
}

static void DrawClosedDoorTiles(const struct DoorGraphics *gfx, u32 x, u32 y)
{
    switch (gfx->size)
    {
    case DOOR_SIZE_2x2_LEFT:
        CurrentMapDrawMetatileAt(x + 1, y - 1);
        CurrentMapDrawMetatileAt(x + 1, y);
        CurrentMapDrawMetatileAt(x, y - 1);
        CurrentMapDrawMetatileAt(x, y);
        break;
    case DOOR_SIZE_2x2_RIGHT:
        CurrentMapDrawMetatileAt(x - 1, y - 1);
        CurrentMapDrawMetatileAt(x - 1, y);
        CurrentMapDrawMetatileAt(x, y - 1);
        CurrentMapDrawMetatileAt(x, y);
        break;
    case DOOR_SIZE_1x2:
        CurrentMapDrawMetatileAt(x, y - 1);
        CurrentMapDrawMetatileAt(x, y);
        break;
    default:
    case DOOR_SIZE_1x1:
        CurrentMapDrawMetatileAt(x, y);
        break;
    }
}

static void DrawDoor(const struct DoorGraphics *gfx, const struct DoorAnimFrame *frame, u32 x, u32 y)
{
    if (frame->offset == 0xFFFF)
    {
        DrawClosedDoorTiles(gfx, x, y);
        if (ShouldUseMultiCorridorDoor())
            DrawClosedDoorTiles(gfx, gSpecialVar_0x8004 + MAP_OFFSET, gSpecialVar_0x8005 + MAP_OFFSET);
    }
    else
    {
        CopyDoorTilesToVram(gfx, frame);
        DrawCurrentDoorAnimFrame(gfx, x, y, gfx->palettes);
        if (ShouldUseMultiCorridorDoor())
            DrawCurrentDoorAnimFrame(gfx, gSpecialVar_0x8004 + MAP_OFFSET, gSpecialVar_0x8005 + MAP_OFFSET, gfx->palettes);
    }
}

#define tFramesHi data[0]
#define tFramesLo data[1]
#define tGfxHi data[2]
#define tGfxLo data[3]
#define tFrameId data[4]
#define tCounter data[5]
#define tX data[6]
#define tY data[7]

// Draws a single frame of the door animation, or skips drawing to wait between frames.
// Returns FALSE when the final frame has been reached
static bool32 AnimateDoorFrame(struct DoorGraphics *gfx, struct DoorAnimFrame *frames, s16 *data)
{
    if (tCounter == 0)
        DrawDoor(gfx, &frames[tFrameId], tX, tY);

    if (tCounter == frames[tFrameId].time)
    {
        tCounter = 0;
        tFrameId++;
        if (frames[tFrameId].time == 0)
            return FALSE;
        else
            return TRUE;
    }
    tCounter++;
    return TRUE;
}

static void Task_AnimateDoor(u8 taskId)
{
    u16 *data = (u16*) gTasks[taskId].data;
    struct DoorAnimFrame *frames = (struct DoorAnimFrame *)(tFramesHi << 16 | tFramesLo);
    struct DoorGraphics *gfx = (struct DoorGraphics *)(tGfxHi << 16 | tGfxLo);

    if (AnimateDoorFrame(gfx, frames, gTasks[taskId].data) == FALSE)
        DestroyTask(taskId);
}

static const struct DoorAnimFrame *GetLastDoorFrame(const struct DoorAnimFrame *frame, const void *unused)
{
    while (frame->time != 0)
        frame++;
    return frame - 1;
}

static const struct DoorGraphics *GetDoorGraphics(const struct DoorGraphics *gfx, u16 metatileNum)
{
    while (gfx->tiles != NULL)
    {
        if (gfx->metatileNum == metatileNum && (gfx->tileset == gMapHeader.mapLayout->primaryTileset || gfx->tileset == gMapHeader.mapLayout->secondaryTileset))
            return gfx;
        gfx++;
    }
    return NULL;
}

static s8 StartDoorAnimationTask(const struct DoorGraphics *gfx, const struct DoorAnimFrame *frames, u32 x, u32 y)
{
    if (FuncIsActiveTask(Task_AnimateDoor) == TRUE)
    {
        return -1;
    }
    else
    {
        u8 taskId = CreateTask(Task_AnimateDoor, 0x50);
        s16 *data = gTasks[taskId].data;

        tX = x;
        tY = y;

        tFramesLo = (u32)frames;
        tFramesHi = (u32)frames >> 16;

        tGfxLo = (u32)gfx;
        tGfxHi = (u32)gfx >> 16;

        return taskId;
    }
}

static void DrawClosedDoor(const struct DoorGraphics *gfx, u32 x, u32 y)
{
    DrawClosedDoorTiles(gfx, x, y);
}

static void DrawOpenedDoor(const struct DoorGraphics *gfx, u32 x, u32 y)
{
    gfx = GetDoorGraphics(gfx, MapGridGetMetatileIdAt(x, y));
    if (gfx != NULL)
    {
        const struct DoorAnimFrame *doorAnimFrames = sDoorSizeInfo[gfx->size].openAnimFrames;
        DrawDoor(gfx, GetLastDoorFrame(doorAnimFrames, doorAnimFrames), x, y);
    }
}

static s8 StartDoorOpenAnimation(const struct DoorGraphics *gfx, u32 x, u32 y)
{
    gfx = GetDoorGraphics(gfx, MapGridGetMetatileIdAt(x, y));
    if (gfx == NULL)
    {
        return -1;
    }
    else
    {
        const struct DoorAnimFrame *doorAnimFrames = sDoorSizeInfo[gfx->size].openAnimFrames;
        return StartDoorAnimationTask(gfx, doorAnimFrames, x, y);
    }
}

static s8 StartDoorCloseAnimation(const struct DoorGraphics *gfx, u32 x, u32 y)
{
    gfx = GetDoorGraphics(gfx, MapGridGetMetatileIdAt(x, y));
    if (gfx == NULL)
    {
        return -1;
    }
    else
    {
        const struct DoorAnimFrame *doorAnimFrames = sDoorSizeInfo[gfx->size].closeAnimFrames;
        return StartDoorAnimationTask(gfx, doorAnimFrames, x, y);
    }
}

static s8 GetDoorSoundType(const struct DoorGraphics *gfx, u32 x, u32 y)
{
    gfx = GetDoorGraphics(gfx, MapGridGetMetatileIdAt(x, y));
    if (gfx == NULL)
        return -1;
    else
        return gfx->sound;
}

void FieldSetDoorOpened(u32 x, u32 y)
{
    if (MetatileBehavior_IsDoor(MapGridGetMetatileBehaviorAt(x, y)))
        DrawOpenedDoor(sDoorAnimGraphicsTable, x, y);
}

void FieldSetDoorClosed(u32 x, u32 y)
{
    if (MetatileBehavior_IsDoor(MapGridGetMetatileBehaviorAt(x, y)))
        DrawClosedDoor(sDoorAnimGraphicsTable, x, y);
}

s8 FieldAnimateDoorClose(u32 x, u32 y)
{
    if (!MetatileBehavior_IsDoor(MapGridGetMetatileBehaviorAt(x, y)))
        return -1;
    else
        return StartDoorCloseAnimation(sDoorAnimGraphicsTable, x, y);
}

s8 FieldAnimateDoorOpen(u32 x, u32 y)
{
    if (!MetatileBehavior_IsDoor(MapGridGetMetatileBehaviorAt(x, y)))
        return -1;
    else
        return StartDoorOpenAnimation(sDoorAnimGraphicsTable, x, y);
}

bool8 FieldIsDoorAnimationRunning(void)
{
    return FuncIsActiveTask(Task_AnimateDoor);
}

u32 GetDoorSoundEffect(u32 x, u32 y)
{
    int sound = GetDoorSoundType(sDoorAnimGraphicsTable, x, y);

    if (sound == DOOR_SOUND_NORMAL)
        return SE_DOOR;
    else if (sound == DOOR_SOUND_SLIDING)
        return SE_SLIDING_DOOR;
    else if (sound == DOOR_SOUND_ARENA)
        return SE_REPEL;
    else
        return SE_DOOR;
}

// Opens the Battle Tower multi partner's door in sync with the player's door
static bool8 ShouldUseMultiCorridorDoor(void)
{
    if (FlagGet(FLAG_ENABLE_MULTI_CORRIDOR_DOOR))
    {
        if (gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(MAP_BATTLE_FRONTIER_BATTLE_TOWER_MULTI_CORRIDOR)
            && gSaveBlock1Ptr->location.mapNum == MAP_NUM(MAP_BATTLE_FRONTIER_BATTLE_TOWER_MULTI_CORRIDOR))
        {
            return TRUE;
        }
    }
    return FALSE;
}
