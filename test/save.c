#include "global.h"
#include "pokemon_storage_system.h"
#include "test/test.h"

// If you would like to ensure save compatibility, update the values below with those for your hack. You can find these through the debug menu.
// Please note that this simple check is not 100% foolproof, but should be able to catch most unintended shifts.
// The size changes below are hand-calculated, not measured; if a size mismatches, suspect
// compiler padding shifts first. Later fork field additions account for any gap between
// a step's result and the next step's starting value.
//
// SaveBlock1 (starts at 11764, ends at 7518):
//   ROAMER_COUNT 120 -> 1: -119 * sizeof(struct Roamer) (28) = -3332 -> 8432.
//   FREE_CONTESTS/DECORATIONS/MAIL/POKEBLOCKS: contestWinners[8] (256), decoration arrays
//     (102), mail[8] (272), DaycareMail struct Mail x2 (68), pokeblocks[10] (70) -> 7664.
//     The 102- and 34-byte removals are not multiples of 4, so they can shift padding
//     around neighboring 4-byte-aligned fields (struct DayCare's BoxPokemon, struct TVShow).
//   sizeof(struct Pokemon) 120 -> 104 and struct BoxPokemon 96 -> 80 (see include/pokemon.h):
//     playerParty[6] (-96) and daycare BoxPokemon x2 (-32) -> 7536. Both sizes stay
//     multiples of 4, so no padding drift is expected.
//   Dex regional-form slots (POKEMON_SLOTS_NUMBER 1026 -> 1083): dexSeen and dexCaught
//     grow 129 -> 136 bytes each. u8[] arrays need no padding: 7504 + 7 + 7 = 7518.
//     Assumes the default species config (all regional-form families enabled, as in
//     test.h); disabling a P_*_FORMS changes this number.
//
// SaveBlock2 (starts at 3008, ends at 744):
//   FREE_CONTESTS: contestLinkResults[5][4] (40) -> 2968.
//   FREE_BATTLE_FRONTIER: struct BattleFrontier (~2172), apprentices[4] (272) and
//     playerApprentice (44) removed, ~2488 total. Adds back disableRecordBattle:1 +
//     lvlMode:2 (1 byte, likely 1 pad byte after) and selectedPartyMons[4] (u16, 8 bytes):
//     2968 - 2488 + 1 + 1 + 8 = 490. Least certain figure: it leans on an estimated
//     BattleFrontier size, which also varies with FREE_BATTLE_TOWER_E_READER.
//   struct PendingTrade appended to a measured 544-byte SaveBlock2. 544 is a multiple of 4,
//     so trailing pad before it is at most 3 bytes and the 4-byte-aligned struct (it has u32
//     members) must start at exactly 544. It is 200 bytes with no internal padding (see its
//     comment in include/global.h): 544 + 200 = 744, already a multiple of 4.
//
// PokemonStorage (starts at 40944, ends at 67900):
//   TOTAL_BOXES_COUNT 14 -> 16: +2 * (30 * sizeof(BoxPokemon) + BOX_NAME_LENGTH + 1 + 1
//     wallpaper byte) = 2 * (2880 + 9 + 1) = 5780 -> 46724.
//   BoxPokemon 96 -> 80 and Pokemon 120 -> 104: boxes -7680 (16*30*16), fusions[4] -64
//     -> 38980.
//   TOTAL_BOXES_COUNT 16 -> 28: +12 * (2400 + 9 + 1) = 28920 -> 67900. Checked against the
//     byte offsets: currentBox (1) + 3 pad (aligns `boxes` to 4, since BoxPokemon's `secure`
//     union has u32s) + boxes (N*30*80) + boxNames (N*9) + boxWallpapers (N) + fusions
//     (4*104). Names plus wallpapers cost 10 bytes per box, so fusions' offset stays
//     4-aligned only when N is even; 16 and 28 both are.
#define T_SAVEBLOCK1_SIZE 7518
#define T_SAVEBLOCK2_SIZE 744
#define T_SAVEBLOCK3_SIZE 1576
#define T_POKEMONSTORAGE_SIZE 67900

TEST("SaveBlock1 is backwards compatible")
{
    EXPECT_EQ(sizeof(struct SaveBlock1), T_SAVEBLOCK1_SIZE);
}

TEST("SaveBlock2 is backwards compatible")
{
    EXPECT_EQ(sizeof(struct SaveBlock2), T_SAVEBLOCK2_SIZE);
}

TEST("SaveBlock3 is backwards compatible")
{
    EXPECT_EQ(sizeof(struct SaveBlock3), T_SAVEBLOCK3_SIZE);
}

TEST("PokemonStorage is backwards compatible")
{
    EXPECT_EQ(sizeof(struct PokemonStorage), T_POKEMONSTORAGE_SIZE);
}

#undef T_SAVEBLOCK1_SIZE
#undef T_SAVEBLOCK2_SIZE
#undef T_SAVEBLOCK3_SIZE
#undef T_POKEMONSTORAGE_SIZE
