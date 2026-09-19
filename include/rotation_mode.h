#ifndef GUARD_ROTATION_MODE_H
#define GUARD_ROTATION_MODE_H

#include "global.h"
#include "constants/battle.h" // enum BattlerId

// Rotation Mode. After the player's action resolves each turn, a random
// eligible party Pokémon is switched in for free (it costs neither the turn
// that ended nor the one it lands on). An independent toggle, outside the
// mutually-exclusive Nuzlocke / Draft / Recruits GAME MODE row.
//
// In doubles, exactly one of the two active player battlers rotates each turn,
// chosen at random among the eligible ones.
//
// Enabled state lives in gSaveBlock2Ptr->rotationModeSetting; 0 (also what old
// saves read back) means off.

bool32 RotationMode_IsEnabled(void);

// PARTY_SIZE when no rotation should happen this turn (fewer than two
// eligible Pokémon); otherwise the party index of a randomly chosen alive,
// non-egg Pokémon that is not currently on the field.
u32 RotationMode_PickReplacement(enum BattlerId battler);

// Whether Rotation Mode's end-turn switch is allowed for this battler right now:
// trainer battles only (wild battles never rotate), and even then excludes
// link/recorded battles, Multi/Ingame Partner battles and Frontier/Trainer Hill
// facilities, plus a battler that is Commanded (Dondozo) or the target of Sky Drop.
bool32 RotationMode_IsBattleEligible(enum BattlerId battler);

// Doubles arbitration: whether this battler is the one picked to rotate this
// turn. Always TRUE in singles. The random pick is cached in
// gBattleStruct->rotationModeChosenBattler on the first query each turn.
bool32 RotationMode_ShouldRotate(enum BattlerId battler);

#endif // GUARD_ROTATION_MODE_H
