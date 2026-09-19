#ifndef GUARD_MONO_GEN_H
#define GUARD_MONO_GEN_H

#include "global.h"

// Mono Gen challenge mode. The player commits to one generation (1-9) and may
// only obtain Pokémon introduced in it (wild catches, gifts, eggs, in-game
// trades, evolutions). Sibling of Mono Type (mono_type.h), reusing its plumbing;
// both restrictions apply independently when both are on.
//
// The chosen generation lives in gSaveBlock2Ptr->monoGenSetting; 0 (also what
// old saves read back) means off.
//
// Every gate (bag ball throw, Cmd_handleballthrow, healthbox indicator,
// givemon/giveegg hooks, trade scripts, GetEvolutionTargetSpecies) must use
// MonoGen_IsSpeciesAllowed so the rules and the HUD cannot disagree.

// GEN_1 .. GEN_9; 0 means OFF.
#define MONO_GEN_COUNT 9

bool32 MonoGen_IsEnabled(void);

// The generation (1-9) the player committed to, or 0 when the mode is off.
u8 MonoGen_GetGen(void);

// Generation (1-9) that introduced this species. Regional/battle-only forms
// (Alolan, Galarian, Hisuian, Paldean, Mega, Primal, Ultra Burst, Gigantamax,
// Tera) resolve to the generation of the FORM. Returns 0 when there is no
// National Dex number; callers treat 0 as "unknown, never blocks".
u8 MonoGen_GetSpeciesGeneration(u16 species);

// TRUE when mono gen is off, or the species' generation matches the chosen
// one. Species IDs outside the real range are always allowed, so a bad caller
// can never soft-lock a grant. No resolver layer here: randomization.h has no
// species resolver, and every caller already holds the post-randomization species.
bool32 MonoGen_IsSpeciesAllowed(u16 species);

// Fills out[0..2] with the chosen generation's 3 canonical starters, read
// straight out of the Birch case's own starter table (no new data).
void MonoGen_GetCanonicalStarters(u16 *out);

// Steps the settings-menu value: 0 ("OFF") -> GEN_1 .. GEN_9 -> OFF, wrapping
// in both directions. No skip list is needed - every generation 1-9 is valid.
u8 MonoGen_CycleGen(u8 current, bool8 forward);

#endif // GUARD_MONO_GEN_H
