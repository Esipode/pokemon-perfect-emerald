#ifndef GUARD_MONO_TYPE_H
#define GUARD_MONO_TYPE_H

#include "global.h"

// Mono Type challenge mode. The player commits to one type and may only obtain
// Pokémon of that type (wild catches, gifts, eggs, in-game trades).
//
// The chosen type lives in gSaveBlock2Ptr->monoTypeSetting; TYPE_NONE (also
// what old saves read back) means off.
//
// Every gate (bag ball throw, Cmd_handleballthrow, healthbox indicator,
// givemon/giveegg hooks, trade scripts) must use MonoType_IsSpeciesAllowed so
// the rules and the HUD cannot disagree.

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

// Fills out[0..MONO_TYPE_STARTER_COUNT-1] with the starter pool draw.
// Deterministic per trainer ID + New Game+ cycle, so reopening the Birch case
// cannot reroll the trio. Unused slots are left SPECIES_NONE.
void MonoType_PickStarterSpecies(u16 *out);

// Steps the settings-menu value: TYPE_NONE ("OFF") -> NORMAL .. FAIRY -> OFF,
// wrapping in both directions and skipping TYPE_MYSTERY and TYPE_STELLAR.
u8 MonoType_CycleType(u8 current, bool8 forward);

// One-shot flag set by the givemon/giveegg grant hooks (ScrCmd_createmon,
// ScriptGiveMon, ScriptGiveEgg) when a grant is skipped for wrong type. Read and
// cleared by Common_EventScript_NoMoreRoomForPokemon to show the Mono Type
// refusal instead of "no room".
void MonoType_SetGiveBlocked(void);
bool32 MonoType_ConsumeGiveBlockedFlag(void);

#endif // GUARD_MONO_TYPE_H
