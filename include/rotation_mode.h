#ifndef GUARD_ROTATION_MODE_H
#define GUARD_ROTATION_MODE_H

#include "global.h"
#include "constants/battle.h" // enum BattlerId

// Rotation Mode. After the player's action resolves each turn, a random
// eligible Pokémon from their party is automatically switched in - free of
// charge, so it does not cost the turn that just ended or the one it lands
// on. It is an independent toggle, not part of the mutually-exclusive
// Nuzlocke / Draft / Recruits GAME MODE row, so it can be combined with any
// other challenge mode.
//
// Whether the mode is on lives in gSaveBlock2Ptr->rotationModeSetting. 0
// means the mode is off, which is also what old saves read back.

bool32 RotationMode_IsEnabled(void);

// PARTY_SIZE when no rotation should happen this turn (fewer than two
// eligible Pokémon); otherwise the party index of a randomly chosen alive,
// non-egg Pokémon that is not currently on the field.
u32 RotationMode_PickReplacement(enum BattlerId battler);

// Whether Rotation Mode's end-turn switch is allowed for this battler right now:
// excludes link/recorded battles, Safari, Birch's rescue battle, Multi/Ingame
// Partner battles and Frontier/Trainer Hill facilities, plus a battler that is
// Commanded (Dondozo) or the target of Sky Drop.
bool32 RotationMode_IsBattleEligible(enum BattlerId battler);

#endif // GUARD_ROTATION_MODE_H
