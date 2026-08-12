#ifndef GUARD_MONO_GEN_H
#define GUARD_MONO_GEN_H

#include "global.h"

// Mono Gen challenge mode. The player commits to a single Pokémon generation
// (1-9) for the whole playthrough and may only obtain Pokémon introduced in
// it (wild catches, scripted gifts, eggs, in-game trades, and evolutions).
// It is the sibling of Mono Type (see mono_type.h) and reuses that feature's
// plumbing wherever possible; both restrictions apply independently and
// simultaneously when both are on.
//
// The chosen generation lives in gSaveBlock2Ptr->monoGenSetting. 0 means the
// mode is off, which is also what old saves read back.
//
// Every gate in the game (bag ball throw, Cmd_handleballthrow, the healthbox
// indicator, the givemon/giveegg hooks, the trade scripts and
// GetEvolutionTargetSpecies) must go through MonoGen_IsSpeciesAllowed so the
// rules and the HUD cannot disagree.

// GEN_1 .. GEN_9; 0 means OFF.
#define MONO_GEN_COUNT 9

bool32 MonoGen_IsEnabled(void);

// The generation (1-9) the player committed to, or 0 when the mode is off.
u8 MonoGen_GetGen(void);

// Which generation introduced this species, 1-9. Regional/battle-only forms
// (Alolan, Galarian, Hisuian, Paldean, Mega, Primal, Ultra Burst, Gigantamax,
// Tera) resolve to the generation that introduced the FORM, not the
// generation of the base species' dex slot. Returns 0 when the species is
// SPECIES_NONE, out of range, or otherwise has no National Dex number -
// callers should treat 0 as "unknown, never blocks".
u8 MonoGen_GetSpeciesGeneration(u16 species);

// TRUE when mono gen is off, otherwise TRUE only if the species' generation
// (see MonoGen_GetSpeciesGeneration) matches the chosen one. Species IDs
// outside the real species range are always allowed, so a bad caller can
// never soft-lock the player out of a grant. Unlike MonoType_IsSpeciesAllowed
// there is no resolver layer to go through here - randomization.h has no
// species resolver, and every gate this is used from is already handed the
// post-randomization species.
bool32 MonoGen_IsSpeciesAllowed(u16 species);

// Fills out[0..2] with the chosen generation's 3 canonical starters, read
// straight out of the Birch case's own starter table (no new data).
void MonoGen_GetCanonicalStarters(u16 *out);

// Steps the settings-menu value: 0 ("OFF") -> GEN_1 .. GEN_9 -> OFF, wrapping
// in both directions. No skip list is needed - every generation 1-9 is valid.
u8 MonoGen_CycleGen(u8 current, bool8 forward);

#endif // GUARD_MONO_GEN_H
