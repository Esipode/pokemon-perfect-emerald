#ifndef GUARD_WORLD_MAP_H
#define GUARD_WORLD_MAP_H

#include "main.h"

// Must match WORLD_MAP_CELLS_W/H and the bank count in world_map_layout.h (checked in world_map.c).
#define WORLD_MAP_DEX_W 56
#define WORLD_MAP_DEX_H 36
#define WORLD_MAP_DEX_BANKS 7

void CB2_OpenWorldMap(void);
void FieldInitWorldMap(MainCallback callback);
mapsec_u16_t GetWorldMapSecIdAt(u16 x, u16 y);

// Pokédex area screen backend.
void LoadWorldMapForDex(u32 charBase, u32 mapBase, u32 firstBank);
u32 GetWorldMapSecGroup(mapsec_u16_t mapSec);
bool32 IsWorldMapSecUnlocked(mapsec_u16_t mapSec);
bool32 GetWorldMapPlayerPos(s32 *x, s32 *y);
bool32 GetWorldMapSecCenter(mapsec_u16_t mapSec, s32 *x, s32 *y);
void GetWorldMapGroupAnchor(u32 group, s32 *x, s32 *y);

#endif // GUARD_WORLD_MAP_H
