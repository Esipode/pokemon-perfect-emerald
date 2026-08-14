#ifndef GUARD_RECRUITS_MODE_H
#define GUARD_RECRUITS_MODE_H

#include "global.h"

// Recruits challenge mode. Every party Pokémon is a temporary hire: a mon
// that participates in a won trainer battle earns a point toward
// RECRUITS_MAX_BATTLES, and retires - is permanently removed from the party,
// not boxed - the moment that cap is reached and field control returns.
// Losing a battle, or sitting one out, never grants credit.
//
// Whether the mode is on lives in gSaveBlock1Ptr->recruitsModeEnabled. 0
// means the mode is off, which is also what old saves read back.

// Wins needed before a mon retires. Also the value MON_DATA_RECRUIT_BATTLES
// is clamped to - see Recruits_TallyParticipants.
#define RECRUITS_MAX_BATTLES 10

// TRUE when the player has Recruits mode turned on for this save, independent
// of whether any mon is close to retiring.
bool32 Recruits_IsEnabled(void);

// Recruits_IsEnabled() plus FLAG_SYS_POKEDEX_GET - Recruits doesn't engage
// until the player has their Pokédex from Birch, after the first rival
// battle, not merely their starter. This is the same script node that sets
// FLAG_NUZLOCKE_CATCH_MODE, so it mirrors when Nuzlocke's own restrictions
// actually start too - see the comment on this function in recruits_mode.c.
bool32 Recruits_IsActive(void);

// RECRUITS_MAX_BATTLES minus `mon`'s tallied MON_DATA_RECRUIT_BATTLES - how
// many more won battles it can participate in before it retires.
u32 Recruits_GetBattlesLeft(struct Pokemon *mon);

// In-battle: increments MON_DATA_RECRUIT_BATTLES for every player party slot
// that was sent out this trainer battle, clamped to RECRUITS_MAX_BATTLES.
// Called from HandleEndTurn_BattleWon (src/battle_main.c) once a win is
// confirmed - see Recruits_BattleCounts (recruits_mode.c) for what kinds of
// battles actually count.
void Recruits_TallyParticipants(void);

// Field hook for src/field_control_avatar.c's ProcessPlayerFieldInput, called
// every frame the player has field control. Scans the party for the first
// mon at RECRUITS_MAX_BATTLES and, on a hit, buffers its name and slot and
// starts Recruits_EventScript_Retire (data/scripts/recruits.inc), returning
// TRUE. Idempotent - rescans from scratch each call, so several qualifying
// mons retire one at a time as the hook re-fires next frame. Returns FALSE,
// touching nothing, when no mon qualifies (including whenever
// Recruits_IsActive() is false).
bool32 Recruits_TryStartFieldScript(void);

// Script native (data/scripts/recruits.inc): permanently removes the party
// mon at gSpecialVar_0x8004, as buffered by Recruits_TryStartFieldScript,
// and compacts the party. Arms autosave when the player has it enabled.
void Recruits_DoRetirement(void);

// Script native: VAR_RESULT = TRUE if the retirement just performed by
// Recruits_DoRetirement emptied the party.
void Recruits_IsRunFailed(void);

// Script native: persists the emptied party and hands off to the run-failed
// prompt, mirroring Nuzlocke's whiteout screen without routing through
// CB2_WhiteOut - see the header comment above Recruits_DoRetirement's caller
// in data/scripts/recruits.inc for why that matters here.
void Recruits_StartRunFailedScreen(void);

#endif // GUARD_RECRUITS_MODE_H
