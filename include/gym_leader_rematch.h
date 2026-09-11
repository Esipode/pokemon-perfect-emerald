#ifndef GUARD_TRAINER_REMATCH_H
#define GUARD_TRAINER_REMATCH_H

#include "constants/rematches.h"

void UpdateGymLeaderRematch(void);
s32 GetCurrentGymLeaderRematchLevel(void);
void SetTrainerRematchStepCounter(u32 value);
u32 GetActiveTrainerRematches(u32 matchCallId);
void SetActiveTrainerRematches(u32 matchCallId, u32 value);

#endif //GUARD_TRAINER_REMATCH_H
