#ifndef GUARD_UI_BIRCH_MENU_H
#define GUARD_UI_BIRCH_MENU_H

#include "main.h"

void Task_OpenBirchCase(u8 taskId);
void BirchCase_Init(MainCallback callback);

u16 GetRandomSpecies(u8 setIndex, u8 slotIndex);
u8 GetRandomType(u16 species, u32 typeOffset);
u16 GetRandomMove(u16 species, u16 originalMove);
u8 GetRandomMoveType(u16 moveId);

#endif // GUARD_UI_MENU_H
