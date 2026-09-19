#ifndef GUARD_RECRUITS_MODE_H
#define GUARD_RECRUITS_MODE_H

#include "global.h"

// Recruits challenge mode. Every party Pokémon is a temporary hire: one that
// participates in a won trainer battle earns a point toward RECRUITS_MAX_BATTLES
// and retires (removed from the party, not boxed) once the cap is reached and
// field control returns. Losing or sitting out a battle grants no credit.
//
// Enabled state lives in gSaveBlock1Ptr->recruitsModeEnabled; 0 (also what old
// saves read back) means off.

// Wins needed before a mon retires; MON_DATA_RECRUIT_BATTLES is clamped to it.
#define RECRUITS_MAX_BATTLES 10

bool32 Recruits_IsEnabled(void);

// Recruits_IsEnabled() plus FLAG_SYS_POKEDEX_GET. Engages at the same script
// node as FLAG_NUZLOCKE_CATCH_MODE, i.e. when Nuzlocke's restrictions start.
bool32 Recruits_IsActive(void);

// Won battles `mon` can still participate in before it retires.
u32 Recruits_GetBattlesLeft(struct Pokemon *mon);

// In-battle: increments MON_DATA_RECRUIT_BATTLES for every party slot sent out
// this trainer battle, clamped to RECRUITS_MAX_BATTLES. Called from
// HandleEndTurn_BattleWon (src/battle_main.c); see Recruits_BattleCounts for
// which battles count.
void Recruits_TallyParticipants(void);

// Field hook for ProcessPlayerFieldInput (src/field_control_avatar.c), run every
// frame the player has field control. On finding the first mon at
// RECRUITS_MAX_BATTLES, buffers its name and slot, starts
// Recruits_EventScript_Retire (data/scripts/recruits.inc) and returns TRUE.
// Rescans each call, so several qualifying mons retire one per frame. Returns
// FALSE when none qualify, including when Recruits_IsActive() is false.
//
// Known gap: the hook only runs when no script is active, so a scripted chain
// of trainer battles can run a mon a few battles past 10/10. The clamp in
// Recruits_TallyParticipants prevents overflow; it retires once field control returns.
bool32 Recruits_TryStartFieldScript(void);

// Script native (data/scripts/recruits.inc): removes the party mon at
// gSpecialVar_0x8004 (buffered by Recruits_TryStartFieldScript) and compacts
// the party. Arms autosave when the player has it enabled.
void Recruits_DoRetirement(void);

// Script native: VAR_RESULT = TRUE if the last retirement emptied the party.
void Recruits_IsRunFailed(void);

// Script native: persists the emptied party and hands off to the run-failed
// prompt, mirroring Nuzlocke's whiteout without CB2_WhiteOut (see
// data/scripts/recruits.inc).
void Recruits_StartRunFailedScreen(void);

#endif // GUARD_RECRUITS_MODE_H
