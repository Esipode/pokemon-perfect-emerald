#ifndef GUARD_MONO_TYPE_H
#define GUARD_MONO_TYPE_H

#include "global.h"

// Mono Type challenge mode. The player commits to a single Pokémon type for
// the whole playthrough and may only obtain Pokémon of that type (wild
// catches, scripted gifts, eggs and in-game trades).
//
// The chosen type lives in gSaveBlock2Ptr->monoTypeSetting. TYPE_NONE means
// the mode is off, which is also what old saves read back.
//
// Every gate in the game (bag ball throw, Cmd_handleballthrow, the healthbox
// indicator, the givemon/giveegg hooks and the trade scripts) must go through
// MonoType_IsSpeciesAllowed so the rules and the HUD cannot disagree.

// How many starters the Birch case shows in mono type mode.
#define MONO_TYPE_STARTER_COUNT 3

// Base stat total cap applied when drawing the starter pool.
#define MONO_TYPE_STARTER_MAX_BST 400

bool32 MonoType_IsEnabled(void);

// The type the player committed to, or TYPE_NONE when the mode is off.
u8 MonoType_GetType(void);

// TRUE when mono type is off, otherwise TRUE only if the species' resolved
// type pair (see GetResolvedTypePair in randomization.h) includes the chosen
// type. Species IDs outside the real species range are always allowed, so a
// bad caller can never soft-lock the player out of a grant.
bool32 MonoType_IsSpeciesAllowed(u16 species);

// Fills out[0..MONO_TYPE_STARTER_COUNT-1] with the starter pool draw for this
// save. Deterministic for a given trainer ID + New Game+ cycle, so backing out
// of the Birch case with B and reopening it cannot reroll the trio. Unused
// slots are left SPECIES_NONE.
void MonoType_PickStarterSpecies(u16 *out);

// Steps the settings-menu value: TYPE_NONE ("OFF") -> NORMAL .. FAIRY -> OFF,
// wrapping in both directions and skipping TYPE_MYSTERY and TYPE_STELLAR.
u8 MonoType_CycleType(u8 current, bool8 forward);

#endif // GUARD_MONO_TYPE_H
