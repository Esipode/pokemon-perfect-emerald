#include "global.h"
#include "pokemon_storage_system.h"
#include "test/test.h"

// If you would like to ensure save compatibility, update the values below with those for your hack. You can find these through the debug menu.
// Please note that this simple check is not 100% foolproof, but should be able to catch most unintended shifts.
// Stage 1: ROAMER_COUNT 120 -> 1 removes 119 * sizeof(struct Roamer) (119 * 28 = 3332)
// bytes from SaveBlock1 (11764 -> 8432). Calculated, not yet confirmed by a real build.
// Stage 2: FREE_CONTESTS/FREE_DECORATIONS/FREE_MAIL/FREE_POKEBLOCKS remove:
//   SaveBlock1: contestWinners[8] (32B each = 256), 10 decoration*/playerRoomDecoration*
//     arrays (102), mail[8] (34B each = 272), the embedded struct Mail in both
//     DaycareMail slots (34B each = 68), pokeblocks[10] (7B each = 70).
//     8432 - 256 - 102 - 272 - 68 - 70 = 7664.
//   SaveBlock2: contestLinkResults[5][4] (5*4*2 = 40). 3008 - 40 = 2968.
// Calculated, not yet confirmed by a real build -- unlike the Stage 1 roamer
// removal, the decoration arrays (102 bytes) and each removed struct Mail
// (34 bytes) are NOT multiples of 4, so they can shift compiler-inserted
// padding around neighboring 4-byte-aligned fields (struct DayCare's
// BoxPokemon, struct TVShow, ...). If these don't match sizeof() on a real
// build, that padding shift -- not a miscount -- is almost certainly why.
// Stage 3: TOTAL_BOXES_COUNT 14 -> 16 adds 2 * (30 * sizeof(struct BoxPokemon)
//   + BOX_NAME_LENGTH + 1 + 1 wallpaper byte) = 2 * (2880 + 9 + 1) = 5780 bytes
//   to PokemonStorage. 40944 + 5780 = 46724. SaveBlock1/2/3 are untouched by
//   this stage. Calculated, not yet confirmed by a real build.
#define T_SAVEBLOCK1_SIZE 7664
#define T_SAVEBLOCK2_SIZE 2968
#define T_SAVEBLOCK3_SIZE 1576
#define T_POKEMONSTORAGE_SIZE 46724

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
