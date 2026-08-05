#ifndef GUARD_UI_BIRCH_MENU_H
#define GUARD_UI_BIRCH_MENU_H

#include "main.h"
#include "random.h"

void Task_OpenBirchCase(u8 taskId);
void BirchCase_Init(MainCallback callback);

u16 PickRandomSpecies(u8 setIndex, u8 slotIndex);
u16 GetRandomBaseSpecies(rng_value_t *rngState);

// Move/type randomization primitives. These are the RNG source-of-truth,
// but gameplay/UI code should not call them directly - go through the
// resolver in randomization.h (GetResolvedTypePair/GetResolvedMove/
// GetResolvedMoveType) instead, so effective-value resolution stays
// centralized in one place and can't drift or double-randomize. Only
// randomization.c and this file's own internals should call these.
u8 GetRandomType(u16 species, u32 typeOffset);
u16 GetRandomMove(u16 species, u16 originalMove);
u8 GetRandomMoveType(u16 moveId);
u16 GetEffectiveMove(u16 move, u16 species);

#endif // GUARD_UI_MENU_H