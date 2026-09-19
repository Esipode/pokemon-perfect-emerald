#ifndef GUARD_POKEMON_STORAGE_SORT_H
#define GUARD_POKEMON_STORAGE_SORT_H

#include "global.h"

// Sorting for the PC boxes. All 28 boxes are treated as one flat 840-slot list
// (flatIndex = boxId * IN_BOX_COUNT + boxPosition); every Pokémon is repacked
// from box 0 slot 0 with no gaps and trailing slots are zeroed. Box names and
// wallpapers belong to the box and do not move.
//
// Touches neither the PC UI nor sStorage (only gPokemonStoragePtr and pokemon.h
// accessors); the caller rebuilds whatever is on screen.
//
// A pure permutation of occupied slots. Mons move as raw struct copies, never
// through Set(Box)MonData, which would re-encrypt them and turn any checksum
// mismatch into a bad egg. A mon that is already broken is grouped as the bad
// egg it becomes: reading its species runs the engine's checksum test, on any
// read anywhere in the PC.
//
// Eggs sort after every normal Pokémon, bad eggs after eggs, in every mode
// (their species, level and type are hidden or meaningless). Within a group the
// original order is kept.
//
// Not saved until the player saves; there is no in-game undo.

enum StorageSortType
{
    STORAGE_SORT_DEX,    // National Pokédex number, ascending
    STORAGE_SORT_NAME,   // Nickname A-Z, case-insensitive
    STORAGE_SORT_TYPE1,  // Primary type, ascending by TYPE_ order
    // Secondary type. Single-type species carry types[1] == types[0]
    // (MON_TYPES in species_info.h), so monotype Pokémon group under their own
    // type rather than a "no secondary type" bucket. Deliberate.
    STORAGE_SORT_TYPE2,
    STORAGE_SORT_LEVEL,  // Level, descending - highest first
    STORAGE_SORT_COUNT,
};

// Repacks every box into one flat, sorted list starting at box 0 slot 0.
// Returns FALSE without modifying anything if the scratch allocation failed.
bool32 SortPokemonStorage(enum StorageSortType type);

#endif // GUARD_POKEMON_STORAGE_SORT_H
