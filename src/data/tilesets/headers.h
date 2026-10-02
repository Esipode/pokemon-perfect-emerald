#include "fieldmap.h"

// Whether a palette has a night version, located at ((x + 9) % 16).pal
#define SWAP_PAL(x) ((x) < NUM_PALS_IN_PRIMARY ? 1 << (x) : 1 << ((x) - NUM_PALS_IN_PRIMARY))

const struct Tileset gTileset_SecretBase =
{
    .isCompressed = FALSE,
    .isSecondary = FALSE,
    .tiles = gTilesetTiles_SecretBase,
    .palettes = gTilesetPalettes_SecretBase,
    .metatiles = gMetatiles_SecretBasePrimary,
    .metatileAttributes = gMetatileAttributes_SecretBasePrimary,
    .callback = NULL,
};

const struct Tileset gTileset_SecretBaseRedCave =
{
    .isCompressed = FALSE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_SecretBaseRedCave,
    .palettes = gTilesetPalettes_SecretBaseRedCave,
    .metatiles = gMetatiles_SecretBaseSecondary,
    .metatileAttributes = gMetatileAttributes_SecretBaseSecondary,
    .callback = NULL,
};

const struct Tileset *const gTilesetPointer_SecretBase = &gTileset_SecretBase;
const struct Tileset *const gTilesetPointer_SecretBaseRedCave = &gTileset_SecretBaseRedCave;


const struct Tileset gTileset_General =
{
    .isCompressed = TRUE,
    .swapPalettes = SWAP_PAL(4),
    .isSecondary = FALSE,
    .tiles = gTilesetTiles_General,
    .palettes = gTilesetPalettes_General,
    .metatiles = gMetatiles_General,
    .metatileAttributes = gMetatileAttributes_General,
    .callback = InitTilesetAnim_General,
};

const struct Tileset gTileset_Petalburg =
{
    .isCompressed = TRUE,
    .swapPalettes = SWAP_PAL(9),
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_Petalburg,
    .palettes = gTilesetPalettes_Petalburg,
    .metatiles = gMetatiles_Petalburg,
    .metatileAttributes = gMetatileAttributes_Petalburg,
    .callback = InitTilesetAnim_Petalburg,
};

const struct Tileset gTileset_Rustboro =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_Rustboro,
    .palettes = gTilesetPalettes_Rustboro,
    .metatiles = gMetatiles_Rustboro,
    .metatileAttributes = gMetatileAttributes_Rustboro,
    .callback = InitTilesetAnim_Rustboro,
};

const struct Tileset gTileset_Dewford =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_Dewford,
    .palettes = gTilesetPalettes_Dewford,
    .metatiles = gMetatiles_Dewford,
    .metatileAttributes = gMetatileAttributes_Dewford,
    .callback = InitTilesetAnim_Dewford,
};

const struct Tileset gTileset_Slateport =
{
    .isCompressed = TRUE,
    .swapPalettes = SWAP_PAL(6),
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_Slateport,
    .palettes = gTilesetPalettes_Slateport,
    .metatiles = gMetatiles_Slateport,
    .metatileAttributes = gMetatileAttributes_Slateport,
    .callback = InitTilesetAnim_Slateport,
};

const struct Tileset gTileset_Mauville =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_Mauville,
    .palettes = gTilesetPalettes_Mauville,
    .metatiles = gMetatiles_Mauville,
    .metatileAttributes = gMetatileAttributes_Mauville,
    .callback = InitTilesetAnim_Mauville,
};

const struct Tileset gTileset_Lavaridge =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_Lavaridge,
    .palettes = gTilesetPalettes_Lavaridge,
    .metatiles = gMetatiles_Lavaridge,
    .metatileAttributes = gMetatileAttributes_Lavaridge,
    .callback = InitTilesetAnim_Lavaridge,
};

const struct Tileset gTileset_Fallarbor =
{
    .isCompressed = TRUE,
    .swapPalettes = SWAP_PAL(7),
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_Fallarbor,
    .palettes = gTilesetPalettes_Fallarbor,
    .metatiles = gMetatiles_Fallarbor,
    .metatileAttributes = gMetatileAttributes_Fallarbor,
    .callback = InitTilesetAnim_Fallarbor,
};

const struct Tileset gTileset_Fortree =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_Fortree,
    .palettes = gTilesetPalettes_Fortree,
    .metatiles = gMetatiles_Fortree,
    .metatileAttributes = gMetatileAttributes_Fortree,
    .callback = InitTilesetAnim_Fortree,
};

const struct Tileset gTileset_Lilycove =
{
    .isCompressed = TRUE,
    .swapPalettes = SWAP_PAL(6),
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_Lilycove,
    .palettes = gTilesetPalettes_Lilycove,
    .metatiles = gMetatiles_Lilycove,
    .metatileAttributes = gMetatileAttributes_Lilycove,
    .callback = InitTilesetAnim_Lilycove,
};

const struct Tileset gTileset_Mossdeep =
{
    .isCompressed = TRUE,
    .swapPalettes = SWAP_PAL(8),
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_Mossdeep,
    .palettes = gTilesetPalettes_Mossdeep,
    .metatiles = gMetatiles_Mossdeep,
    .metatileAttributes = gMetatileAttributes_Mossdeep,
    .callback = InitTilesetAnim_Mossdeep,
};

const struct Tileset gTileset_EverGrande =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_EverGrande,
    .palettes = gTilesetPalettes_EverGrande,
    .metatiles = gMetatiles_EverGrande,
    .metatileAttributes = gMetatileAttributes_EverGrande,
    .callback = InitTilesetAnim_EverGrande,
    .swapPalettes = SWAP_PAL(8),
};

const struct Tileset gTileset_Pacifidlog =
{
    .isCompressed = TRUE,
    .swapPalettes = SWAP_PAL(9),
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_Pacifidlog,
    .palettes = gTilesetPalettes_Pacifidlog,
    .metatiles = gMetatiles_Pacifidlog,
    .metatileAttributes = gMetatileAttributes_Pacifidlog,
    .callback = InitTilesetAnim_Pacifidlog,
};

const struct Tileset gTileset_Sootopolis =
{
    .isCompressed = TRUE,
    .swapPalettes = SWAP_PAL(6),
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_Sootopolis,
    .palettes = gTilesetPalettes_Sootopolis,
    .metatiles = gMetatiles_Sootopolis,
    .metatileAttributes = gMetatileAttributes_Sootopolis,
    .callback = InitTilesetAnim_Sootopolis,
};

const struct Tileset gTileset_BattleFrontierOutsideWest =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_BattleFrontierOutsideWest,
    .palettes = gTilesetPalettes_BattleFrontierOutsideWest,
    .metatiles = gMetatiles_BattleFrontierOutsideWest,
    .metatileAttributes = gMetatileAttributes_BattleFrontierOutsideWest,
    .callback = InitTilesetAnim_BattleFrontierOutsideWest,
};

const struct Tileset gTileset_BattleFrontierOutsideEast =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_BattleFrontierOutsideEast,
    .palettes = gTilesetPalettes_BattleFrontierOutsideEast,
    .metatiles = gMetatiles_BattleFrontierOutsideEast,
    .metatileAttributes = gMetatileAttributes_BattleFrontierOutsideEast,
    .callback = InitTilesetAnim_BattleFrontierOutsideEast,
};

const struct Tileset gTileset_Building =
{
    .isCompressed = TRUE,
    .isSecondary = FALSE,
    .tiles = gTilesetTiles_InsideBuilding,
    .palettes = gTilesetPalettes_InsideBuilding,
    .metatiles = gMetatiles_InsideBuilding,
    .metatileAttributes = gMetatileAttributes_InsideBuilding,
    .callback = InitTilesetAnim_Building,
};

const struct Tileset gTileset_Shop =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_Shop,
    .palettes = gTilesetPalettes_Shop,
    .metatiles = gMetatiles_Shop,
    .metatileAttributes = gMetatileAttributes_Shop,
    .callback = NULL,
};

const struct Tileset gTileset_PokemonCenter =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_PokemonCenter,
    .palettes = gTilesetPalettes_PokemonCenter,
    .metatiles = gMetatiles_PokemonCenter,
    .metatileAttributes = gMetatileAttributes_PokemonCenter,
    .callback = NULL,
};

const struct Tileset gTileset_Cave =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_Cave,
    .palettes = gTilesetPalettes_Cave,
    .metatiles = gMetatiles_Cave,
    .metatileAttributes = gMetatileAttributes_Cave,
    .callback = InitTilesetAnim_Cave,
};

const struct Tileset gTileset_PokemonSchool =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_PokemonSchool,
    .palettes = gTilesetPalettes_PokemonSchool,
    .metatiles = gMetatiles_PokemonSchool,
    .metatileAttributes = gMetatileAttributes_PokemonSchool,
    .callback = NULL,
};

const struct Tileset gTileset_PokemonFanClub =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_PokemonFanClub,
    .palettes = gTilesetPalettes_PokemonFanClub,
    .metatiles = gMetatiles_PokemonFanClub,
    .metatileAttributes = gMetatileAttributes_PokemonFanClub,
    .callback = NULL,
};

const struct Tileset gTileset_MeteorFalls =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_MeteorFalls,
    .palettes = gTilesetPalettes_MeteorFalls,
    .metatiles = gMetatiles_MeteorFalls,
    .metatileAttributes = gMetatileAttributes_MeteorFalls,
    .callback = NULL,
};

const struct Tileset gTileset_OceanicMuseum =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_OceanicMuseum,
    .palettes = gTilesetPalettes_OceanicMuseum,
    .metatiles = gMetatiles_OceanicMuseum,
    .metatileAttributes = gMetatileAttributes_OceanicMuseum,
    .callback = NULL,
};

const struct Tileset gTileset_CableClub =
{
    .isCompressed = FALSE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_CableClub,
    .palettes = gTilesetPalettes_CableClub,
    .metatiles = gMetatiles_CableClub,
    .metatileAttributes = gMetatileAttributes_CableClub,
    .callback = NULL,
};

const struct Tileset gTileset_SeashoreHouse =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_SeashoreHouse,
    .palettes = gTilesetPalettes_SeashoreHouse,
    .metatiles = gMetatiles_SeashoreHouse,
    .metatileAttributes = gMetatileAttributes_SeashoreHouse,
    .callback = NULL,
};

const struct Tileset gTileset_PrettyPetalFlowerShop =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_PrettyPetalFlowerShop,
    .palettes = gTilesetPalettes_PrettyPetalFlowerShop,
    .metatiles = gMetatiles_PrettyPetalFlowerShop,
    .metatileAttributes = gMetatileAttributes_PrettyPetalFlowerShop,
    .callback = NULL,
};

const struct Tileset gTileset_PokemonDayCare =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_PokemonDayCare,
    .palettes = gTilesetPalettes_PokemonDayCare,
    .metatiles = gMetatiles_PokemonDayCare,
    .metatileAttributes = gMetatileAttributes_PokemonDayCare,
    .callback = NULL,
};

const struct Tileset gTileset_Facility =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_Facility,
    .palettes = gTilesetPalettes_Facility,
    .metatiles = gMetatiles_Facility,
    .metatileAttributes = gMetatileAttributes_Facility,
    .callback = NULL,
};

const struct Tileset gTileset_BikeShop =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_BikeShop,
    .palettes = gTilesetPalettes_BikeShop,
    .metatiles = gMetatiles_BikeShop,
    .metatileAttributes = gMetatileAttributes_BikeShop,
    .callback = InitTilesetAnim_BikeShop,
};

const struct Tileset gTileset_RusturfTunnel =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_RusturfTunnel,
    .palettes = gTilesetPalettes_RusturfTunnel,
    .metatiles = gMetatiles_RusturfTunnel,
    .metatileAttributes = gMetatileAttributes_RusturfTunnel,
    .callback = NULL,
};

const struct Tileset gTileset_InsideOfTruck =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_InsideOfTruck,
    .palettes = gTilesetPalettes_InsideOfTruck,
    .metatiles = gMetatiles_InsideOfTruck,
    .metatileAttributes = gMetatileAttributes_InsideOfTruck,
    .callback = NULL,
};

const struct Tileset gTileset_Contest =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_Contest,
    .palettes = gTilesetPalettes_Contest,
    .metatiles = gMetatiles_Contest,
    .metatileAttributes = gMetatileAttributes_Contest,
    .callback = NULL,
};

const struct Tileset gTileset_LilycoveMuseum =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_LilycoveMuseum,
    .palettes = gTilesetPalettes_LilycoveMuseum,
    .metatiles = gMetatiles_LilycoveMuseum,
    .metatileAttributes = gMetatileAttributes_LilycoveMuseum,
    .callback = NULL,
};

const struct Tileset gTileset_BrendansMaysHouse =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_BrendansMaysHouse,
    .palettes = gTilesetPalettes_BrendansMaysHouse,
    .metatiles = gMetatiles_BrendansMaysHouse,
    .metatileAttributes = gMetatileAttributes_BrendansMaysHouse,
    .callback = NULL,
};

const struct Tileset gTileset_Lab =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_Lab,
    .palettes = gTilesetPalettes_Lab,
    .metatiles = gMetatiles_Lab,
    .metatileAttributes = gMetatileAttributes_Lab,
    .callback = NULL,
};

const struct Tileset gTileset_Underwater =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_Underwater,
    .palettes = gTilesetPalettes_Underwater,
    .metatiles = gMetatiles_Underwater,
    .metatileAttributes = gMetatileAttributes_Underwater,
    .callback = InitTilesetAnim_Underwater,
};

const struct Tileset gTileset_PetalburgGym =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_PetalburgGym,
    .palettes = gTilesetPalettes_PetalburgGym,
    .metatiles = gMetatiles_PetalburgGym,
    .metatileAttributes = gMetatileAttributes_PetalburgGym,
    .callback = NULL,
};

const struct Tileset gTileset_SootopolisGym =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_SootopolisGym,
    .palettes = gTilesetPalettes_SootopolisGym,
    .metatiles = gMetatiles_SootopolisGym,
    .metatileAttributes = gMetatileAttributes_SootopolisGym,
    .callback = InitTilesetAnim_SootopolisGym,
};

const struct Tileset gTileset_GenericBuilding =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_GenericBuilding,
    .palettes = gTilesetPalettes_GenericBuilding,
    .metatiles = gMetatiles_GenericBuilding,
    .metatileAttributes = gMetatileAttributes_GenericBuilding,
    .callback = NULL,
};

const struct Tileset gTileset_MauvilleGameCorner =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_MauvilleGameCorner,
    .palettes = gTilesetPalettes_MauvilleGameCorner,
    .metatiles = gMetatiles_MauvilleGameCorner,
    .metatileAttributes = gMetatileAttributes_MauvilleGameCorner,
    .callback = NULL,
};

const struct Tileset gTileset_RustboroGym =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_RustboroGym,
    .palettes = gTilesetPalettes_RustboroGym,
    .metatiles = gMetatiles_RustboroGym,
    .metatileAttributes = gMetatileAttributes_RustboroGym,
    .callback = NULL,
};

const struct Tileset gTileset_DewfordGym =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_DewfordGym,
    .palettes = gTilesetPalettes_DewfordGym,
    .metatiles = gMetatiles_DewfordGym,
    .metatileAttributes = gMetatileAttributes_DewfordGym,
    .callback = NULL,
};

const struct Tileset gTileset_MauvilleGym =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_MauvilleGym,
    .palettes = gTilesetPalettes_MauvilleGym,
    .metatiles = gMetatiles_MauvilleGym,
    .metatileAttributes = gMetatileAttributes_MauvilleGym,
    .callback = InitTilesetAnim_MauvilleGym,
};

const struct Tileset gTileset_LavaridgeGym =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_LavaridgeGym,
    .palettes = gTilesetPalettes_LavaridgeGym,
    .metatiles = gMetatiles_LavaridgeGym,
    .metatileAttributes = gMetatileAttributes_LavaridgeGym,
    .callback = NULL,
};

const struct Tileset gTileset_TrickHousePuzzle =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_TrickHousePuzzle,
    .palettes = gTilesetPalettes_TrickHousePuzzle,
    .metatiles = gMetatiles_TrickHousePuzzle,
    .metatileAttributes = gMetatileAttributes_TrickHousePuzzle,
    .callback = NULL,
};

const struct Tileset gTileset_FortreeGym =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_FortreeGym,
    .palettes = gTilesetPalettes_FortreeGym,
    .metatiles = gMetatiles_FortreeGym,
    .metatileAttributes = gMetatileAttributes_FortreeGym,
    .callback = NULL,
};

const struct Tileset gTileset_MossdeepGym =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_MossdeepGym,
    .palettes = gTilesetPalettes_MossdeepGym,
    .metatiles = gMetatiles_MossdeepGym,
    .metatileAttributes = gMetatileAttributes_MossdeepGym,
    .callback = NULL,
};

const struct Tileset gTileset_InsideShip =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_InsideShip,
    .palettes = gTilesetPalettes_InsideShip,
    .metatiles = gMetatiles_InsideShip,
    .metatileAttributes = gMetatileAttributes_InsideShip,
    .callback = NULL,
};

const struct Tileset gTileset_EliteFour =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_EliteFour,
    .palettes = gTilesetPalettes_EliteFour,
    .metatiles = gMetatiles_EliteFour,
    .metatileAttributes = gMetatileAttributes_EliteFour,
    .callback = InitTilesetAnim_EliteFour,
};

const struct Tileset gTileset_BattleFrontier =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_BattleFrontier,
    .palettes = gTilesetPalettes_BattleFrontier,
    .metatiles = gMetatiles_BattleFrontier,
    .metatileAttributes = gMetatileAttributes_BattleFrontier,
    .callback = NULL,
};

const struct Tileset gTileset_BattlePalace =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_BattlePalace,
    .palettes = gTilesetPalettes_BattlePalace,
    .metatiles = gMetatiles_BattlePalace,
    .metatileAttributes = gMetatileAttributes_BattlePalace,
    .callback = NULL,
};

const struct Tileset gTileset_BattleDome =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_BattleDome,
    .palettes = gTilesetPalettes_BattleDome,
    .metatiles = gMetatiles_BattleDome,
    .metatileAttributes = gMetatileAttributes_BattleDome,
    .callback = InitTilesetAnim_BattleDome,
};

const struct Tileset gTileset_BattleFactory =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_BattleFactory,
    .palettes = gTilesetPalettes_BattleFactory,
    .metatiles = gMetatiles_BattleFactory,
    .metatileAttributes = gMetatileAttributes_BattleFactory,
    .callback = NULL,
};

const struct Tileset gTileset_BattlePike =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_BattlePike,
    .palettes = gTilesetPalettes_BattlePike,
    .metatiles = gMetatiles_BattlePike,
    .metatileAttributes = gMetatileAttributes_BattlePike,
    .callback = NULL,
};

const struct Tileset gTileset_BattleArena =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_BattleArena,
    .palettes = gTilesetPalettes_BattleArena,
    .metatiles = gMetatiles_BattleArena,
    .metatileAttributes = gMetatileAttributes_BattleArena,
    .callback = NULL,
};

const struct Tileset gTileset_BattlePyramid =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_BattlePyramid,
    .palettes = gTilesetPalettes_BattlePyramid,
    .metatiles = gMetatiles_BattlePyramid,
    .metatileAttributes = gMetatileAttributes_BattlePyramid,
    .callback = InitTilesetAnim_BattlePyramid,
};

const struct Tileset gTileset_MirageTower =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_MirageTower,
    .palettes = gTilesetPalettes_MirageTower,
    .metatiles = gMetatiles_MirageTower,
    .metatileAttributes = gMetatileAttributes_MirageTower,
    .callback = NULL,
};

const struct Tileset gTileset_MossdeepGameCorner =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_MossdeepGameCorner,
    .palettes = gTilesetPalettes_MossdeepGameCorner,
    .metatiles = gMetatiles_MossdeepGameCorner,
    .metatileAttributes = gMetatileAttributes_MossdeepGameCorner,
    .callback = NULL,
};

const struct Tileset gTileset_IslandHarbor =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_IslandHarbor,
    .palettes = gTilesetPalettes_IslandHarbor,
    .metatiles = gMetatiles_IslandHarbor,
    .metatileAttributes = gMetatileAttributes_IslandHarbor,
    .callback = NULL,
};

const struct Tileset gTileset_TrainerHill =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_TrainerHill,
    .palettes = gTilesetPalettes_TrainerHill,
    .metatiles = gMetatiles_TrainerHill,
    .metatileAttributes = gMetatileAttributes_TrainerHill,
    .callback = NULL,
};

const struct Tileset gTileset_NavelRock =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_NavelRock,
    .palettes = gTilesetPalettes_NavelRock,
    .metatiles = gMetatiles_NavelRock,
    .metatileAttributes = gMetatileAttributes_NavelRock,
    .callback = NULL,
};

const struct Tileset gTileset_BattleFrontierRankingHall =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_BattleFrontierRankingHall,
    .palettes = gTilesetPalettes_BattleFrontierRankingHall,
    .metatiles = gMetatiles_BattleFrontierRankingHall,
    .metatileAttributes = gMetatileAttributes_BattleFrontierRankingHall,
    .callback = NULL,
};

const struct Tileset gTileset_BattleTent =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_BattleTent,
    .palettes = gTilesetPalettes_BattleTent,
    .metatiles = gMetatiles_BattleTent,
    .metatileAttributes = gMetatileAttributes_BattleTent,
    .callback = NULL,
};

const struct Tileset gTileset_MysteryEventsHouse =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_MysteryEventsHouse,
    .palettes = gTilesetPalettes_MysteryEventsHouse,
    .metatiles = gMetatiles_MysteryEventsHouse,
    .metatileAttributes = gMetatileAttributes_MysteryEventsHouse,
    .callback = NULL,
};


// Kanto tilesets
const struct Tileset gTileset_BuildingKanto =
{
    .isCompressed = TRUE,
    .isSecondary = FALSE,
    .tiles = gTilesetTiles_BuildingKanto,
    .palettes = gTilesetPalettes_BuildingKanto,
    .metatiles = gMetatiles_BuildingKanto,
    .metatileAttributes = gMetatileAttributes_BuildingKanto,
    .callback = NULL,
};

const struct Tileset gTileset_GeneralKanto =
{
    .isCompressed = TRUE,
    .isSecondary = FALSE,
    .tiles = gTilesetTiles_GeneralKanto,
    .palettes = gTilesetPalettes_GeneralKanto,
    .metatiles = gMetatiles_GeneralKanto,
    .metatileAttributes = gMetatileAttributes_GeneralKanto,
    .callback = InitTilesetAnim_General_Frlg,
};

const struct Tileset gTileset_PalletTown =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_PalletTown,
    .palettes = gTilesetPalettes_PalletTown,
    .metatiles = gMetatiles_PalletTown,
    .metatileAttributes = gMetatileAttributes_PalletTown,
    .callback = NULL,
};

const struct Tileset gTileset_ViridianCity =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_ViridianCity,
    .palettes = gTilesetPalettes_ViridianCity,
    .metatiles = gMetatiles_ViridianCity,
    .metatileAttributes = gMetatileAttributes_ViridianCity,
    .callback = NULL,
};

const struct Tileset gTileset_PewterCity =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_PewterCity,
    .palettes = gTilesetPalettes_PewterCity,
    .metatiles = gMetatiles_PewterCity,
    .metatileAttributes = gMetatileAttributes_PewterCity,
    .callback = NULL,
};

const struct Tileset gTileset_CeruleanCity =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_CeruleanCity,
    .palettes = gTilesetPalettes_CeruleanCity,
    .metatiles = gMetatiles_CeruleanCity,
    .metatileAttributes = gMetatileAttributes_CeruleanCity,
    .callback = NULL,
};

const struct Tileset gTileset_LavenderTown =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_LavenderTown,
    .palettes = gTilesetPalettes_LavenderTown,
    .metatiles = gMetatiles_LavenderTown,
    .metatileAttributes = gMetatileAttributes_LavenderTown,
    .callback = NULL,
};

const struct Tileset gTileset_VermilionCity =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_VermilionCity,
    .palettes = gTilesetPalettes_VermilionCity,
    .metatiles = gMetatiles_VermilionCity,
    .metatileAttributes = gMetatileAttributes_VermilionCity,
    .callback = NULL,
};

const struct Tileset gTileset_CeladonCity =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_CeladonCity,
    .palettes = gTilesetPalettes_CeladonCity,
    .metatiles = gMetatiles_CeladonCity,
    .metatileAttributes = gMetatileAttributes_CeladonCity,
    .callback = InitTilesetAnim_CeladonCity,
};

const struct Tileset gTileset_FuchsiaCity =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_FuchsiaCity,
    .palettes = gTilesetPalettes_FuchsiaCity,
    .metatiles = gMetatiles_FuchsiaCity,
    .metatileAttributes = gMetatileAttributes_FuchsiaCity,
    .callback = NULL,
};

const struct Tileset gTileset_CinnabarIsland =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_CinnabarIsland,
    .palettes = gTilesetPalettes_CinnabarIsland,
    .metatiles = gMetatiles_CinnabarIsland,
    .metatileAttributes = gMetatileAttributes_CinnabarIsland,
    .callback = NULL,
};

const struct Tileset gTileset_IndigoPlateau =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_IndigoPlateau,
    .palettes = gTilesetPalettes_IndigoPlateau,
    .metatiles = gMetatiles_IndigoPlateau,
    .metatileAttributes = gMetatileAttributes_IndigoPlateau,
    .callback = NULL,
};

const struct Tileset gTileset_SaffronCity =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_SaffronCity,
    .palettes = gTilesetPalettes_SaffronCity,
    .metatiles = gMetatiles_SaffronCity,
    .metatileAttributes = gMetatileAttributes_SaffronCity,
    .callback = NULL,
};

const struct Tileset gTileset_Mart =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_Mart,
    .palettes = gTilesetPalettes_Mart,
    .metatiles = gMetatiles_Mart,
    .metatileAttributes = gMetatileAttributes_Mart,
    .callback = NULL,
};

const struct Tileset gTileset_PokemonCenterFrlg =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_PokemonCenter_Frlg,
    .palettes = gTilesetPalettes_PokemonCenter_Frlg,
    .metatiles = gMetatiles_PokemonCenter_Frlg,
    .metatileAttributes = gMetatileAttributes_PokemonCenter_Frlg,
    .callback = NULL,
};

const struct Tileset gTileset_Cave_Frlg =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_Cave_Frlg,
    .palettes = gTilesetPalettes_Cave_Frlg,
    .metatiles = gMetatiles_Cave_Frlg,
    .metatileAttributes = gMetatileAttributes_Cave_Frlg,
    .callback = NULL,
};

const struct Tileset gTileset_Museum =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_Museum,
    .palettes = gTilesetPalettes_Museum,
    .metatiles = gMetatiles_Museum,
    .metatileAttributes = gMetatileAttributes_Museum,
    .callback = NULL,
};

const struct Tileset gTileset_BikeShop_Frlg =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_BikeShop_Frlg,
    .palettes = gTilesetPalettes_BikeShop_Frlg,
    .metatiles = gMetatiles_BikeShop_Frlg,
    .metatileAttributes = gMetatileAttributes_BikeShop_Frlg,
    .callback = NULL,
};

const struct Tileset gTileset_GenericBuilding1 =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_GenericBuilding1,
    .palettes = gTilesetPalettes_GenericBuilding1,
    .metatiles = gMetatiles_GenericBuilding1,
    .metatileAttributes = gMetatileAttributes_GenericBuilding1,
    .callback = NULL,
};

const struct Tileset gTileset_Lab_Frlg =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_Lab_Frlg,
    .palettes = gTilesetPalettes_Lab_Frlg,
    .metatiles = gMetatiles_Lab_Frlg,
    .metatileAttributes = gMetatileAttributes_Lab_Frlg,
    .callback = NULL,
};

const struct Tileset gTileset_FuchsiaGym =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_FuchsiaGym,
    .palettes = gTilesetPalettes_FuchsiaGym,
    .metatiles = gMetatiles_FuchsiaGym,
    .metatileAttributes = gMetatileAttributes_FuchsiaGym,
    .callback = NULL,
};

const struct Tileset gTileset_ViridianGym =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_ViridianGym,
    .palettes = gTilesetPalettes_ViridianGym,
    .metatiles = gMetatiles_ViridianGym,
    .metatileAttributes = gMetatileAttributes_ViridianGym,
    .callback = NULL,
};

const struct Tileset gTileset_HoennBuilding =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_HoennBuilding,
    .palettes = gTilesetPalettes_HoennBuilding,
    .metatiles = gMetatiles_HoennBuilding,
    .metatileAttributes = gMetatileAttributes_HoennBuilding,
    .callback = NULL,
};

const struct Tileset gTileset_GameCorner =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_GameCorner,
    .palettes = gTilesetPalettes_GameCorner,
    .metatiles = gMetatiles_GameCorner,
    .metatileAttributes = gMetatileAttributes_GameCorner,
    .callback = NULL,
};

const struct Tileset gTileset_PewterGym =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_PewterGym,
    .palettes = gTilesetPalettes_PewterGym,
    .metatiles = gMetatiles_PewterGym,
    .metatileAttributes = gMetatileAttributes_PewterGym,
    .callback = NULL,
};

const struct Tileset gTileset_CeruleanGym =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_CeruleanGym,
    .palettes = gTilesetPalettes_CeruleanGym,
    .metatiles = gMetatiles_CeruleanGym,
    .metatileAttributes = gMetatileAttributes_CeruleanGym,
    .callback = NULL,
};

const struct Tileset gTileset_VermilionGym =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_VermilionGym,
    .palettes = gTilesetPalettes_VermilionGym,
    .metatiles = gMetatiles_VermilionGym,
    .metatileAttributes = gMetatileAttributes_VermilionGym,
    .callback = InitTilesetAnim_VermilionGym,
};

const struct Tileset gTileset_CeladonGym =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_CeladonGym,
    .palettes = gTilesetPalettes_CeladonGym,
    .metatiles = gMetatiles_CeladonGym,
    .metatileAttributes = gMetatileAttributes_CeladonGym,
    .callback = InitTilesetAnim_CeladonGym,
};

const struct Tileset gTileset_SaffronGym =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_SaffronGym,
    .palettes = gTilesetPalettes_SaffronGym,
    .metatiles = gMetatiles_SaffronGym,
    .metatileAttributes = gMetatileAttributes_SaffronGym,
    .callback = NULL,
};

const struct Tileset gTileset_CinnabarGym =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_CinnabarGym,
    .palettes = gTilesetPalettes_CinnabarGym,
    .metatiles = gMetatiles_CinnabarGym,
    .metatileAttributes = gMetatileAttributes_CinnabarGym,
    .callback = NULL,
};

const struct Tileset gTileset_SSAnne =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_SSAnne,
    .palettes = gTilesetPalettes_SSAnne,
    .metatiles = gMetatiles_SSAnne,
    .metatileAttributes = gMetatileAttributes_SSAnne,
    .callback = NULL,
};

const struct Tileset gTileset_ViridianForest =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_ViridianForest,
    .palettes = gTilesetPalettes_ViridianForest,
    .metatiles = gMetatiles_ViridianForest,
    .metatileAttributes = gMetatileAttributes_ViridianForest,
    .callback = NULL,
};

const struct Tileset gTileset_RockTunnel =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_RockTunnel,
    .palettes = gTilesetPalettes_RockTunnel,
    .metatiles = gMetatiles_RockTunnel,
    .metatileAttributes = gMetatileAttributes_RockTunnel,
    .callback = NULL,
};

const struct Tileset gTileset_DiglettsCave =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_DiglettsCave,
    .palettes = gTilesetPalettes_DiglettsCave,
    .metatiles = gMetatiles_DiglettsCave,
    .metatileAttributes = gMetatileAttributes_DiglettsCave,
    .callback = NULL,
};

const struct Tileset gTileset_SeafoamIslands =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_SeafoamIslands,
    .palettes = gTilesetPalettes_SeafoamIslands,
    .metatiles = gMetatiles_SeafoamIslands,
    .metatileAttributes = gMetatileAttributes_SeafoamIslands,
    .callback = NULL,
};

const struct Tileset gTileset_SeafoamIslandsBuilding =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_SeafoamIslandsBuilding,
    .palettes = gTilesetPalettes_SeafoamIslandsBuilding,
    .metatiles = gMetatiles_SeafoamIslandsBuilding,
    .metatileAttributes = gMetatileAttributes_SeafoamIslandsBuilding,
    .callback = NULL,
};

const struct Tileset gTileset_CeruleanCave =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_CeruleanCave,
    .palettes = gTilesetPalettes_CeruleanCave,
    .metatiles = gMetatiles_CeruleanCave,
    .metatileAttributes = gMetatileAttributes_CeruleanCave,
    .callback = NULL,
};

const struct Tileset gTileset_DepartmentStore =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_DepartmentStore,
    .palettes = gTilesetPalettes_DepartmentStore,
    .metatiles = gMetatiles_DepartmentStore,
    .metatileAttributes = gMetatileAttributes_DepartmentStore,
    .callback = NULL,
};

const struct Tileset gTileset_GenericBuilding2 =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_GenericBuilding2,
    .palettes = gTilesetPalettes_GenericBuilding2,
    .metatiles = gMetatiles_GenericBuilding2,
    .metatileAttributes = gMetatileAttributes_GenericBuilding2,
    .callback = NULL,
};

const struct Tileset gTileset_PowerPlant =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_PowerPlant,
    .palettes = gTilesetPalettes_PowerPlant,
    .metatiles = gMetatiles_PowerPlant,
    .metatileAttributes = gMetatileAttributes_PowerPlant,
    .callback = NULL,
};

const struct Tileset gTileset_SeaCottage =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_SeaCottage,
    .palettes = gTilesetPalettes_SeaCottage,
    .metatiles = gMetatiles_SeaCottage,
    .metatileAttributes = gMetatileAttributes_SeaCottage,
    .callback = NULL,
};

const struct Tileset gTileset_SilphCo =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_Condominiums,
    .palettes = gTilesetPalettes_Condominiums,
    .metatiles = gMetatiles_SilphCo,
    .metatileAttributes = gMetatileAttributes_SilphCo,
    .callback = InitTilesetAnim_SilphCo,
};

const struct Tileset gTileset_UndergroundPath =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_UndergroundPath,
    .palettes = gTilesetPalettes_UndergroundPath,
    .metatiles = gMetatiles_UndergroundPath,
    .metatileAttributes = gMetatileAttributes_UndergroundPath,
    .callback = NULL,
};

const struct Tileset gTileset_PokemonTower =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_PokemonTower,
    .palettes = gTilesetPalettes_PokemonTower,
    .metatiles = gMetatiles_PokemonTower,
    .metatileAttributes = gMetatileAttributes_PokemonTower,
    .callback = NULL,
};

const struct Tileset gTileset_SafariZoneBuilding =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_SafariZoneBuilding,
    .palettes = gTilesetPalettes_SafariZoneBuilding,
    .metatiles = gMetatiles_SafariZoneBuilding,
    .metatileAttributes = gMetatileAttributes_SafariZoneBuilding,
    .callback = NULL,
};

const struct Tileset gTileset_PokemonMansion =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_PokemonMansion,
    .palettes = gTilesetPalettes_PokemonMansion,
    .metatiles = gMetatiles_PokemonMansion,
    .metatileAttributes = gMetatileAttributes_PokemonMansion,
    .callback = NULL,
};

const struct Tileset gTileset_RestaurantHotel =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_RestaurantHotel,
    .palettes = gTilesetPalettes_RestaurantHotel,
    .metatiles = gMetatiles_RestaurantHotel,
    .metatileAttributes = gMetatileAttributes_RestaurantHotel,
    .callback = NULL,
};

const struct Tileset gTileset_School =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_School,
    .palettes = gTilesetPalettes_School,
    .metatiles = gMetatiles_School,
    .metatileAttributes = gMetatileAttributes_School,
    .callback = NULL,
};

const struct Tileset gTileset_FanClubDaycare =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_FanClubDaycare,
    .palettes = gTilesetPalettes_FanClubDaycare,
    .metatiles = gMetatiles_FanClubDaycare,
    .metatileAttributes = gMetatileAttributes_FanClubDaycare,
    .callback = NULL,
};

const struct Tileset gTileset_Condominiums =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_Condominiums,
    .palettes = gTilesetPalettes_Condominiums,
    .metatiles = gMetatiles_Condominiums,
    .metatileAttributes = gMetatileAttributes_Condominiums,
    .callback = NULL,
};

const struct Tileset gTileset_BurgledHouse =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_BurgledHouse,
    .palettes = gTilesetPalettes_BurgledHouse,
    .metatiles = gMetatiles_BurgledHouse,
    .metatileAttributes = gMetatileAttributes_BurgledHouse,
    .callback = NULL,
};

const struct Tileset gTileset_MtEmber =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_MtEmber,
    .palettes = gTilesetPalettes_MtEmber,
    .metatiles = gMetatiles_MtEmber,
    .metatileAttributes = gMetatileAttributes_MtEmber,
    .callback = InitTilesetAnim_MtEmber,
};

const struct Tileset gTileset_BerryForest =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_BerryForest,
    .palettes = gTilesetPalettes_BerryForest,
    .metatiles = gMetatiles_BerryForest,
    .metatileAttributes = gMetatileAttributes_BerryForest,
    .callback = NULL,
};

const struct Tileset gTileset_NavelRock_Frlg =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_NavelRock_Frlg,
    .palettes = gTilesetPalettes_NavelRock_Frlg,
    .metatiles = gMetatiles_NavelRock_Frlg,
    .metatileAttributes = gMetatileAttributes_NavelRock_Frlg,
    .callback = NULL,
};

const struct Tileset gTileset_TanobyRuins =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_TanobyRuins,
    .palettes = gTilesetPalettes_TanobyRuins,
    .metatiles = gMetatiles_TanobyRuins,
    .metatileAttributes = gMetatileAttributes_TanobyRuins,
    .callback = NULL,
};

const struct Tileset gTileset_SeviiIslands123 =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_SeviiIslands123,
    .palettes = gTilesetPalettes_SeviiIslands123,
    .metatiles = gMetatiles_SeviiIslands123,
    .metatileAttributes = gMetatileAttributes_SeviiIslands123,
    .callback = NULL,
};

const struct Tileset gTileset_SeviiIslands45 =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_SeviiIslands45,
    .palettes = gTilesetPalettes_SeviiIslands45,
    .metatiles = gMetatiles_SeviiIslands45,
    .metatileAttributes = gMetatileAttributes_SeviiIslands45,
    .callback = NULL,
};

const struct Tileset gTileset_SeviiIslands67 =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_SeviiIslands67,
    .palettes = gTilesetPalettes_SeviiIslands67,
    .metatiles = gMetatiles_SeviiIslands67,
    .metatileAttributes = gMetatileAttributes_SeviiIslands67,
    .callback = NULL,
};

const struct Tileset gTileset_TrainerTower =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_TrainerTower,
    .palettes = gTilesetPalettes_TrainerTower,
    .metatiles = gMetatiles_TrainerTower,
    .metatileAttributes = gMetatileAttributes_TrainerTower,
    .callback = NULL,
};

const struct Tileset gTileset_IslandHarbor_Frlg =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_IslandHarbor_Frlg,
    .palettes = gTilesetPalettes_IslandHarbor_Frlg,
    .metatiles = gMetatiles_IslandHarbor_Frlg,
    .metatileAttributes = gMetatileAttributes_IslandHarbor_Frlg,
    .callback = NULL,
};

const struct Tileset gTileset_PokemonLeague =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_PokemonLeague,
    .palettes = gTilesetPalettes_PokemonLeague,
    .metatiles = gMetatiles_PokemonLeague,
    .metatileAttributes = gMetatileAttributes_PokemonLeague,
    .callback = NULL,
};

const struct Tileset gTileset_HallOfFame =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_HallOfFame,
    .palettes = gTilesetPalettes_HallOfFame,
    .metatiles = gMetatiles_HallOfFame,
    .metatileAttributes = gMetatileAttributes_HallOfFame,
    .callback = NULL,
};


// Johto tilesets (GS Stage 8)

const struct Tileset gTileset_AzaleaTown_Gym_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_AzaleaTown_Gym_Johto,
    .palettes = gTilesetPalettes_AzaleaTown_Gym_Johto,
    .metatiles = gMetatiles_AzaleaTown_Gym_Johto,
    .metatileAttributes = gMetatileAttributes_AzaleaTown_Gym_Johto,
    .callback = InitTilesetAnim_AzaleaTownGym,
};

const struct Tileset gTileset_AzaleaTown_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_AzaleaTown_Johto,
    .palettes = gTilesetPalettes_AzaleaTown_Johto,
    .metatiles = gMetatiles_AzaleaTown_Johto,
    .metatileAttributes = gMetatileAttributes_AzaleaTown_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_Barn_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_Barn_Johto,
    .palettes = gTilesetPalettes_Barn_Johto,
    .metatiles = gMetatiles_Barn_Johto,
    .metatileAttributes = gMetatileAttributes_Barn_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_BellchimeTrail_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_BellchimeTrail_Johto,
    .palettes = gTilesetPalettes_BellchimeTrail_Johto,
    .metatiles = gMetatiles_BellchimeTrail_Johto,
    .metatileAttributes = gMetatileAttributes_BellchimeTrail_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_BikeShop_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_BikeShop_Johto,
    .palettes = gTilesetPalettes_BikeShop_Johto,
    .metatiles = gMetatiles_BikeShop_Johto,
    .metatileAttributes = gMetatileAttributes_BikeShop_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_BlackthornGym_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_BlackthornGym_Johto,
    .palettes = gTilesetPalettes_BlackthornGym_Johto,
    .metatiles = gMetatiles_BlackthornGym_Johto,
    .metatileAttributes = gMetatileAttributes_BlackthornGym_Johto,
    .callback = InitTilesetAnim_BlackthornGym,
};

const struct Tileset gTileset_Blackthorn_General_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_Blackthorn_General_Johto,
    .palettes = gTilesetPalettes_Blackthorn_General_Johto,
    .metatiles = gMetatiles_Blackthorn_General_Johto,
    .metatileAttributes = gMetatileAttributes_Blackthorn_General_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_Blackthorn_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_Blackthorn_General_Johto,
    .palettes = gTilesetPalettes_Blackthorn_Johto,
    .metatiles = gMetatiles_Blackthorn_Johto,
    .metatileAttributes = gMetatileAttributes_Blackthorn_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_BurnedTower_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_BurnedTower_Johto,
    .palettes = gTilesetPalettes_BurnedTower_Johto,
    .metatiles = gMetatiles_BurnedTower_Johto,
    .metatileAttributes = gMetatileAttributes_BurnedTower_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_Cafe_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_Cafe_Johto,
    .palettes = gTilesetPalettes_Cafe_Johto,
    .metatiles = gMetatiles_Cafe_Johto,
    .metatileAttributes = gMetatileAttributes_Cafe_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_Cave_Default_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_Cave_Default_Johto,
    .palettes = gTilesetPalettes_Cave_Default_Johto,
    .metatiles = gMetatiles_Cave_Default_Johto,
    .metatileAttributes = gMetatileAttributes_Cave_Default_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_Cave_DragonsDen_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_Cave_DragonsDen_Johto,
    .palettes = gTilesetPalettes_Cave_DragonsDen_Johto,
    .metatiles = gMetatiles_Cave_DragonsDen_Johto,
    .metatileAttributes = gMetatileAttributes_Cave_DragonsDen_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_Cave_DragonsDen_NorthEast_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_Cave_DragonsDen_Johto,
    .palettes = gTilesetPalettes_Cave_DragonsDen_NorthEast_Johto,
    .metatiles = gMetatiles_Cave_DragonsDen_NorthEast_Johto,
    .metatileAttributes = gMetatileAttributes_Cave_DragonsDen_NorthEast_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_Cave_Gray_General_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_Cave_Gray_General_Johto,
    .palettes = gTilesetPalettes_Cave_Gray_General_Johto,
    .metatiles = gMetatiles_Cave_Gray_General_Johto,
    .metatileAttributes = gMetatileAttributes_Cave_Gray_General_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_Cave_Gray_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_Cave_Gray_General_Johto,
    .palettes = gTilesetPalettes_Cave_Gray_Johto,
    .metatiles = gMetatiles_Cave_Gray_Johto,
    .metatileAttributes = gMetatileAttributes_Cave_Gray_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_Cave_Ice_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_Cave_Ice_Johto,
    .palettes = gTilesetPalettes_Cave_Ice_Johto,
    .metatiles = gMetatiles_Cave_Ice_Johto,
    .metatileAttributes = gMetatileAttributes_Cave_Ice_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_CherrygroveCity_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_CherrygroveCity_Johto,
    .palettes = gTilesetPalettes_CherrygroveCity_Johto,
    .metatiles = gMetatiles_CherrygroveCity_Johto,
    .metatileAttributes = gMetatileAttributes_CherrygroveCity_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_CherrygroveCity_NorthEast_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_CherrygroveCity_Johto,
    .palettes = gTilesetPalettes_CherrygroveCity_NorthEast_Johto,
    .metatiles = gMetatiles_CherrygroveCity_NorthEast_Johto,
    .metatileAttributes = gMetatileAttributes_CherrygroveCity_NorthEast_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_CianwoodCity_Gym_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_CianwoodCity_Gym_Johto,
    .palettes = gTilesetPalettes_CianwoodCity_Gym_Johto,
    .metatiles = gMetatiles_CianwoodCity_Gym_Johto,
    .metatileAttributes = gMetatileAttributes_CianwoodCity_Gym_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_CianwoodCity_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_CianwoodCity_Johto,
    .palettes = gTilesetPalettes_CianwoodCity_Johto,
    .metatiles = gMetatiles_CianwoodCity_Johto,
    .metatileAttributes = gMetatileAttributes_CianwoodCity_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_DepartmentStore_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_DepartmentStore_Johto,
    .palettes = gTilesetPalettes_DepartmentStore_Johto,
    .metatiles = gMetatiles_DepartmentStore_Johto,
    .metatileAttributes = gMetatileAttributes_DepartmentStore_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_DragonsDen_Shrine_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_DragonsDen_Shrine_Johto,
    .palettes = gTilesetPalettes_DragonsDen_Shrine_Johto,
    .metatiles = gMetatiles_DragonsDen_Shrine_Johto,
    .metatileAttributes = gMetatileAttributes_DragonsDen_Shrine_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_EcruteakCity_Gym_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_EcruteakCity_Gym_Johto,
    .palettes = gTilesetPalettes_EcruteakCity_Gym_Johto,
    .metatiles = gMetatiles_EcruteakCity_Gym_Johto,
    .metatileAttributes = gMetatileAttributes_EcruteakCity_Gym_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_EcruteakTheater_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_EcruteakTheater_Johto,
    .palettes = gTilesetPalettes_EcruteakTheater_Johto,
    .metatiles = gMetatiles_EcruteakTheater_Johto,
    .metatileAttributes = gMetatileAttributes_EcruteakTheater_Johto,
    .callback = InitTilesetAnim_EcruteakTheater,
};

const struct Tileset gTileset_Ecruteak_City_General_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_Ecruteak_City_General_Johto,
    .palettes = gTilesetPalettes_Ecruteak_City_General_Johto,
    .metatiles = gMetatiles_Ecruteak_City_General_Johto,
    .metatileAttributes = gMetatileAttributes_Ecruteak_City_General_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_Ecruteak_City_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_Ecruteak_City_Johto,
    .palettes = gTilesetPalettes_Ecruteak_City_Johto,
    .metatiles = gMetatiles_Ecruteak_City_Johto,
    .metatileAttributes = gMetatileAttributes_Ecruteak_City_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_GameCorner_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_GameCorner_Johto,
    .palettes = gTilesetPalettes_GameCorner_Johto,
    .metatiles = gMetatiles_GameCorner_Johto,
    .metatileAttributes = gMetatileAttributes_GameCorner_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_Gate_Standard_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_Gate_Standard_Johto,
    .palettes = gTilesetPalettes_Gate_Standard_Johto,
    .metatiles = gMetatiles_Gate_Standard_Johto,
    .metatileAttributes = gMetatileAttributes_Gate_Standard_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_GoldenrodCity_TrainStation_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_GoldenrodCity_TrainStation_Johto,
    .palettes = gTilesetPalettes_GoldenrodCity_TrainStation_Johto,
    .metatiles = gMetatiles_GoldenrodCity_TrainStation_Johto,
    .metatileAttributes = gMetatileAttributes_GoldenrodCity_TrainStation_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_GoldenrodUndergroundRocket_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_GoldenrodUndergroundRocket_Johto,
    .palettes = gTilesetPalettes_GoldenrodUndergroundRocket_Johto,
    .metatiles = gMetatiles_GoldenrodUndergroundRocket_Johto,
    .metatileAttributes = gMetatileAttributes_GoldenrodUndergroundRocket_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_GoldenrodUndergroundTunnel_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_GoldenrodUndergroundTunnel_Johto,
    .palettes = gTilesetPalettes_GoldenrodUndergroundTunnel_Johto,
    .metatiles = gMetatiles_GoldenrodUndergroundTunnel_Johto,
    .metatileAttributes = gMetatileAttributes_GoldenrodUndergroundTunnel_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_Goldenrod_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_Goldenrod_Johto,
    .palettes = gTilesetPalettes_Goldenrod_Johto,
    .metatiles = gMetatiles_Goldenrod_Johto,
    .metatileAttributes = gMetatileAttributes_Goldenrod_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_Goldenrod_Underground_Storage_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_Goldenrod_Underground_Storage_Johto,
    .palettes = gTilesetPalettes_Goldenrod_Underground_Storage_Johto,
    .metatiles = gMetatiles_Goldenrod_Underground_Storage_Johto,
    .metatileAttributes = gMetatileAttributes_Goldenrod_Underground_Storage_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_HallOfFame_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_HallOfFame_Johto,
    .palettes = gTilesetPalettes_HallOfFame_Johto,
    .metatiles = gMetatiles_HallOfFame_Johto,
    .metatileAttributes = gMetatileAttributes_HallOfFame_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_House_2_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_House_2_Johto,
    .palettes = gTilesetPalettes_House_2_Johto,
    .metatiles = gMetatiles_House_2_Johto,
    .metatileAttributes = gMetatileAttributes_House_2_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_House_Lab_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_House_Lab_Johto,
    .palettes = gTilesetPalettes_House_Lab_Johto,
    .metatiles = gMetatiles_House_Lab_Johto,
    .metatileAttributes = gMetatileAttributes_House_Lab_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_IlexForest_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_IlexForest_Johto,
    .palettes = gTilesetPalettes_IlexForest_Johto,
    .metatiles = gMetatiles_IlexForest_Johto,
    .metatileAttributes = gMetatileAttributes_IlexForest_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_IndigoPlateau_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_IndigoPlateau_Johto,
    .palettes = gTilesetPalettes_IndigoPlateau_Johto,
    .metatiles = gMetatiles_IndigoPlateau_Johto,
    .metatileAttributes = gMetatileAttributes_IndigoPlateau_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_JohtoBikeShop_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_BikeShop_Johto,
    .palettes = gTilesetPalettes_JohtoBikeShop_Johto,
    .metatiles = gMetatiles_BikeShop_Johto,
    .metatileAttributes = gMetatileAttributes_BikeShop_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_JohtoBuilding =
{
    .isCompressed = TRUE,
    .isSecondary = FALSE,
    .tiles = gTilesetTiles_JohtoBuilding,
    .palettes = gTilesetPalettes_JohtoBuilding,
    .metatiles = gMetatiles_JohtoBuilding,
    .metatileAttributes = gMetatileAttributes_JohtoBuilding,
    .callback = NULL,
};

const struct Tileset gTileset_JohtoGeneral =
{
    .isCompressed = TRUE,
    .isSecondary = FALSE,
    .tiles = gTilesetTiles_JohtoGeneral,
    .palettes = gTilesetPalettes_JohtoGeneral,
    .metatiles = gMetatiles_JohtoGeneral,
    .metatileAttributes = gMetatileAttributes_JohtoGeneral,
    .callback = InitTilesetAnim_JohtoGeneral,
};

const struct Tileset gTileset_JohtoKantoGeneral =
{
    .isCompressed = TRUE,
    .isSecondary = FALSE,
    .tiles = gTilesetTiles_JohtoKantoGeneral,
    .palettes = gTilesetPalettes_JohtoKantoGeneral,
    .metatiles = gMetatiles_JohtoKantoGeneral,
    .metatileAttributes = gMetatileAttributes_JohtoKantoGeneral,
    .callback = InitTilesetAnim_JohtoGeneral,
};

const struct Tileset gTileset_JohtoMart_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_JohtoMart_Johto,
    .palettes = gTilesetPalettes_JohtoMart_Johto,
    .metatiles = gMetatiles_JohtoMart_Johto,
    .metatileAttributes = gMetatileAttributes_JohtoMart_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_JohtoNorthEast =
{
    .isCompressed = TRUE,
    .isSecondary = FALSE,
    .tiles = gTilesetTiles_JohtoNorthEast,
    .palettes = gTilesetPalettes_JohtoNorthEast,
    .metatiles = gMetatiles_JohtoNorthEast,
    .metatileAttributes = gMetatileAttributes_JohtoNorthEast,
    .callback = InitTilesetAnim_JohtoGeneral,
};

const struct Tileset gTileset_JohtoNorthWest =
{
    .isCompressed = TRUE,
    .isSecondary = FALSE,
    .tiles = gTilesetTiles_JohtoNorthWest,
    .palettes = gTilesetPalettes_JohtoNorthWest,
    .metatiles = gMetatiles_JohtoNorthWest,
    .metatileAttributes = gMetatileAttributes_JohtoNorthWest,
    .callback = InitTilesetAnim_JohtoGeneral,
};

const struct Tileset gTileset_JohtoSouth =
{
    .isCompressed = TRUE,
    .isSecondary = FALSE,
    .tiles = gTilesetTiles_JohtoSouth,
    .palettes = gTilesetPalettes_JohtoSouth,
    .metatiles = gMetatiles_JohtoSouth,
    .metatileAttributes = gMetatileAttributes_JohtoSouth,
    .callback = InitTilesetAnim_JohtoGeneral,
};

const struct Tileset gTileset_Kanto_PokemonCenter_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_Kanto_PokemonCenter_Johto,
    .palettes = gTilesetPalettes_Kanto_PokemonCenter_Johto,
    .metatiles = gMetatiles_Kanto_PokemonCenter_Johto,
    .metatileAttributes = gMetatileAttributes_Kanto_PokemonCenter_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_KurtsHouse_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_KurtsHouse_Johto,
    .palettes = gTilesetPalettes_KurtsHouse_Johto,
    .metatiles = gMetatiles_KurtsHouse_Johto,
    .metatileAttributes = gMetatileAttributes_KurtsHouse_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_Lighthouse_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_Lighthouse_Johto,
    .palettes = gTilesetPalettes_Lighthouse_Johto,
    .metatiles = gMetatiles_Lighthouse_Johto,
    .metatileAttributes = gMetatileAttributes_Lighthouse_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_MahoganyTown_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_MahoganyTown_Johto,
    .palettes = gTilesetPalettes_MahoganyTown_Johto,
    .metatiles = gMetatiles_MahoganyTown_Johto,
    .metatileAttributes = gMetatileAttributes_MahoganyTown_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_MtSilverSnow_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_MtSilverSnow_Johto,
    .palettes = gTilesetPalettes_MtSilverSnow_Johto,
    .metatiles = gMetatiles_MtSilverSnow_Johto,
    .metatileAttributes = gMetatileAttributes_MtSilverSnow_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_NationalPark_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_NationalPark_Johto,
    .palettes = gTilesetPalettes_NationalPark_Johto,
    .metatiles = gMetatiles_NationalPark_Johto,
    .metatileAttributes = gMetatileAttributes_NationalPark_Johto,
    .callback = InitTilesetAnim_NationalPark,
};

const struct Tileset gTileset_NewBarkTown_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_NewBarkTown_Johto,
    .palettes = gTilesetPalettes_NewBarkTown_Johto,
    .metatiles = gMetatiles_NewBarkTown_Johto,
    .metatileAttributes = gMetatileAttributes_NewBarkTown_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_OlivineCity_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_OlivineCity_Johto,
    .palettes = gTilesetPalettes_OlivineCity_Johto,
    .metatiles = gMetatiles_OlivineCity_Johto,
    .metatileAttributes = gMetatileAttributes_OlivineCity_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_PlayersHouse_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_PlayersHouse_Johto,
    .palettes = gTilesetPalettes_PlayersHouse_Johto,
    .metatiles = gMetatiles_PlayersHouse_Johto,
    .metatileAttributes = gMetatileAttributes_PlayersHouse_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_PokemonCenter_White_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_PokemonCenter_White_Johto,
    .palettes = gTilesetPalettes_PokemonCenter_White_Johto,
    .metatiles = gMetatiles_PokemonCenter_White_Johto,
    .metatileAttributes = gMetatileAttributes_PokemonCenter_White_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_PokemonDayCare_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_PokemonDayCare_Johto,
    .palettes = gTilesetPalettes_PokemonDayCare_Johto,
    .metatiles = gMetatiles_PokemonDayCare_Johto,
    .metatileAttributes = gMetatileAttributes_PokemonDayCare_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_PokemonLeague_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_PokemonLeague_Johto,
    .palettes = gTilesetPalettes_PokemonLeague_Johto,
    .metatiles = gMetatiles_PokemonLeague_Johto,
    .metatileAttributes = gMetatileAttributes_PokemonLeague_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_PortIndoor_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_PortIndoor_Johto,
    .palettes = gTilesetPalettes_PortIndoor_Johto,
    .metatiles = gMetatiles_PortIndoor_Johto,
    .metatileAttributes = gMetatileAttributes_PortIndoor_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_PowerPlant_GeneratorRoom_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_PowerPlant_GeneratorRoom_Johto,
    .palettes = gTilesetPalettes_PowerPlant_GeneratorRoom_Johto,
    .metatiles = gMetatiles_PowerPlant_GeneratorRoom_Johto,
    .metatileAttributes = gMetatileAttributes_PowerPlant_GeneratorRoom_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_Route32_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_Route32_Johto,
    .palettes = gTilesetPalettes_Route32_Johto,
    .metatiles = gMetatiles_Route32_Johto,
    .metatileAttributes = gMetatileAttributes_Route32_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_Route38_Farmland_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_Route38_Farmland_Johto,
    .palettes = gTilesetPalettes_Route38_Farmland_Johto,
    .metatiles = gMetatiles_Route38_Farmland_Johto,
    .metatileAttributes = gMetatileAttributes_Route38_Farmland_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_Route40_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_Route40_Johto,
    .palettes = gTilesetPalettes_Route40_Johto,
    .metatiles = gMetatiles_Route40_Johto,
    .metatileAttributes = gMetatileAttributes_Route40_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_RuinsOfAlphWriting_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_RuinsOfAlphWriting_Johto,
    .palettes = gTilesetPalettes_RuinsOfAlphWriting_Johto,
    .metatiles = gMetatiles_RuinsOfAlphWriting_Johto,
    .metatileAttributes = gMetatileAttributes_RuinsOfAlphWriting_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_RuinsOfAlph_B1F_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_RuinsOfAlph_B1F_Johto,
    .palettes = gTilesetPalettes_RuinsOfAlph_B1F_Johto,
    .metatiles = gMetatiles_RuinsOfAlph_B1F_Johto,
    .metatileAttributes = gMetatileAttributes_RuinsOfAlph_B1F_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_RuinsOfAlph_Outside_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_RuinsOfAlph_Outside_Johto,
    .palettes = gTilesetPalettes_RuinsOfAlph_Outside_Johto,
    .metatiles = gMetatiles_RuinsOfAlph_Outside_Johto,
    .metatileAttributes = gMetatileAttributes_RuinsOfAlph_Outside_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_SafariZoneJohto_General_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_SafariZoneJohto_General_Johto,
    .palettes = gTilesetPalettes_SafariZoneJohto_General_Johto,
    .metatiles = gMetatiles_SafariZoneJohto_General_Johto,
    .metatileAttributes = gMetatileAttributes_SafariZoneJohto_General_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_SafariZoneJohto_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_SafariZoneJohto_General_Johto,
    .palettes = gTilesetPalettes_SafariZoneJohto_Johto,
    .metatiles = gMetatiles_SafariZoneJohto_Johto,
    .metatileAttributes = gMetatileAttributes_SafariZoneJohto_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_SafariZone_Entrance_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_SafariZone_Entrance_Johto,
    .palettes = gTilesetPalettes_SafariZone_Entrance_Johto,
    .metatiles = gMetatiles_SafariZone_Entrance_Johto,
    .metatileAttributes = gMetatileAttributes_SafariZone_Entrance_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_ShopRooftop_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_ShopRooftop_Johto,
    .palettes = gTilesetPalettes_ShopRooftop_Johto,
    .metatiles = gMetatiles_ShopRooftop_Johto,
    .metatileAttributes = gMetatileAttributes_ShopRooftop_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_SootopolisGym_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_SootopolisGym_Johto,
    .palettes = gTilesetPalettes_SootopolisGym_Johto,
    .metatiles = gMetatiles_SootopolisGym_Johto,
    .metatileAttributes = gMetatileAttributes_SootopolisGym_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_TrainerSchool_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_TrainerSchool_Johto,
    .palettes = gTilesetPalettes_TrainerSchool_Johto,
    .metatiles = gMetatiles_TrainerSchool_Johto,
    .metatileAttributes = gMetatileAttributes_TrainerSchool_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_VioletCity_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_VioletCity_Johto,
    .palettes = gTilesetPalettes_VioletCity_Johto,
    .metatiles = gMetatiles_VioletCity_Johto,
    .metatileAttributes = gMetatileAttributes_VioletCity_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_ViridianCity_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_ViridianCity_Johto,
    .palettes = gTilesetPalettes_ViridianCity_Johto,
    .metatiles = gMetatiles_ViridianCity_Johto,
    .metatileAttributes = gMetatileAttributes_ViridianCity_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_ViridianCity_NorthEast_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_ViridianCity_Johto,
    .palettes = gTilesetPalettes_ViridianCity_NorthEast_Johto,
    .metatiles = gMetatiles_ViridianCity_NorthEast_Johto,
    .metatileAttributes = gMetatileAttributes_ViridianCity_NorthEast_Johto,
    .callback = NULL,
};

const struct Tileset gTileset_WhirlIslands_Johto =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_WhirlIslands_Johto,
    .palettes = gTilesetPalettes_WhirlIslands_Johto,
    .metatiles = gMetatiles_WhirlIslands_Johto,
    .metatileAttributes = gMetatileAttributes_WhirlIslands_Johto,
    .callback = NULL,
};

