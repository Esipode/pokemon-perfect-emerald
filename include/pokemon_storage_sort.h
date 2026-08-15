#ifndef GUARD_POKEMON_STORAGE_SORT_H
#define GUARD_POKEMON_STORAGE_SORT_H

#include "global.h"

// Sorting for the PC boxes. All 28 boxes are treated as one flat 840-slot
// list (flatIndex = boxId * IN_BOX_COUNT + boxPosition); every Pokémon is
// repacked from box 0 slot 0 forward with no gaps and the trailing slots are
// zeroed. Box names and wallpapers belong to the box, not its contents, and
// do not move.
//
// Nothing here touches the PC UI or sStorage - it depends only on
// gPokemonStoragePtr and the pokemon.h accessors, so the caller owns
// rebuilding whatever is on screen afterwards.
//
// The sort is a pure permutation of the occupied slots: no mon is created,
// destroyed, re-encrypted or converted to a bad egg. Mons move as raw struct
// copies, never through Set(Box)MonData, which would re-encrypt them and turn
// any checksum mismatch into a bad egg.
//
// The one thing the sort does not preserve is a mon that is *already* broken:
// reading its species runs the engine's own checksum test, which flags a
// mismatched mon as a bad egg. That happens on any read anywhere in the PC,
// not just here - the sort only makes sure such a mon is then grouped as the
// bad egg it has become.
//
// Eggs sort after every normal Pokémon, and bad eggs after eggs, in every
// mode - their species, level and type are hidden or meaningless in the UI,
// so interleaving them by those values reads as a bug. Within a group they
// keep their original order.
//
// Sorting is not saved until the player saves. A player who dislikes the
// result can soft-reset; there is no in-game undo.

enum StorageSortType
{
    STORAGE_SORT_DEX,    // National Pokédex number, ascending
    STORAGE_SORT_NAME,   // Nickname A-Z, case-insensitive
    STORAGE_SORT_TYPE1,  // Primary type, ascending by TYPE_ order
    // Secondary type, same order. Single-type species carry types[1] ==
    // types[0] (MON_TYPES in species_info.h), so monotype Pokémon group under
    // their own type instead of a "no secondary type" bucket. Deliberate.
    STORAGE_SORT_TYPE2,
    STORAGE_SORT_LEVEL,  // Level, descending - highest first
    STORAGE_SORT_COUNT,
};

// Repacks every box into one flat, sorted list starting at box 0 slot 0.
// Returns FALSE without modifying anything if the scratch allocation failed.
bool32 SortPokemonStorage(enum StorageSortType type);

#endif // GUARD_POKEMON_STORAGE_SORT_H
