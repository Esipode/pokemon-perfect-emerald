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
// Stage 4: FREE_BATTLE_FRONTIER removes struct BattleFrontier (~2,172, per this
//   doc's own pre-stage measurement), struct Apprentice apprentices[4] (272), and
//   struct PlayersApprentice playerApprentice (44) -- ~2,488 bytes -- and adds back
//   two fields relocated OUT of struct BattleFrontier so generic (non-frontier)
//   code still has somewhere to read/write them: disableRecordBattle:1 + lvlMode:2
//   (1 byte, packed into the same byte) and selectedPartyMons[MAX_FRONTIER_PARTY_SIZE]
//   (a u16[4], 8 bytes, 2-byte aligned -- likely costs 1 padding byte after the
//   1-byte bitfield). 2968 - 2488 + 1 + 1 + 8 = 490. This is the least confident of
//   any T_SAVEBLOCK size in this file: it wasn't hand-derived field-by-field the way
//   Stage 2/3's were, it leans on this doc's own pre-stage BattleFrontier estimate,
//   and struct BattleFrontier's #if FREE_BATTLE_TOWER_E_READER branch changes its
//   size depending on that separate flag. Confirm against a real build before
//   trusting this number over the doc's.
// Stage 5: sizeof(struct BoxPokemon) 96 -> 80 and sizeof(struct Pokemon) 120 -> 104
//   (see the STATIC_ASSERTs and their comments in include/pokemon.h for why 104,
//   not the planning doc's estimated 100). Two SaveBlock1 fields embed these:
//   struct Pokemon playerParty[PARTY_SIZE] (6 * 16 = 96) and
//   struct DayCare daycare.mons[DAYCARE_MON_COUNT].mon, a struct BoxPokemon
//   (2 * 16 = 32). Neither array's own alignment changes (80 and 104 are both
//   still multiples of 4), so this is a clean subtraction with no padding drift
//   expected: 7664 - 96 - 32 = 7536. PokemonStorage embeds both types too: its
//   TOTAL_BOXES_COUNT(16) * IN_BOX_COUNT(30) struct BoxPokemon boxes (16*30*16 =
//   7680) and its MAX_FUSION_STORAGE(4) struct Pokemon fusions (4*16 = 64):
//   46724 - 7680 - 64 = 38980. SaveBlock2/3 have no Pokemon/BoxPokemon fields and
//   are untouched by this stage. Calculated, not yet confirmed by a real build.
// Stage 6: pure flash-layout change (single-copy storage, journaled writes).
//   No struct shrinks -- all four T_*_SIZE values are unchanged from Stage 5.
// Stage 7: EWRAM-only change (achievements_menu.c statics moved to the heap).
//   No save-block struct changes -- all four T_*_SIZE values are unchanged.
// Stage 8: TOTAL_BOXES_COUNT 16 -> 28 adds 12 * (30 * sizeof(struct BoxPokemon)
//   + BOX_NAME_LENGTH + 1 + 1 wallpaper byte) = 12 * (2400 + 9 + 1) = 28920 bytes
//   to PokemonStorage. Unlike Stage 3's 14->16 jump, this was hand-verified against
//   the struct's actual byte offsets (not just added on top of the Stage 5 total):
//   currentBox(1) + 3 padding bytes (aligning `boxes` to 4, required because
//   struct BoxPokemon's `secure` union contains u32s) + boxes(N*30*80) +
//   boxNames(N*9) + boxWallpapers(N*1) + fusions(4*104). boxNames+boxWallpapers
//   together cost 10 bytes/box, which only stays a multiple of 4 -- and so avoids
//   shifting the offset fusions lands at -- when N is even; 16 and 28 both are,
//   so no extra padding beyond the existing 3-byte header pad is expected either
//   side of this stage. 38980 + 28920 = 67900. SaveBlock1/2/3 are untouched.
//   Calculated, not yet confirmed by a real build.
#define T_SAVEBLOCK1_SIZE 7504
#define T_SAVEBLOCK2_SIZE 544
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
