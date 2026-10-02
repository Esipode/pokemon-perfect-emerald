#ifndef GUARD_WORLD_MAP_H
#define GUARD_WORLD_MAP_H

#include "main.h"

void CB2_OpenWorldMap(void);
void FieldInitWorldMap(MainCallback callback);
mapsec_u16_t GetWorldMapSecIdAt(u16 x, u16 y);

#endif // GUARD_WORLD_MAP_H
