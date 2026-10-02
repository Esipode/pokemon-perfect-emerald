#ifndef GUARD_POKEDEX_AREA_REGION_MAP_H
#define GUARD_POKEDEX_AREA_REGION_MAP_H

#define AREA_MAP_GLOW_PALETTE 13 // World map banks occupy 6-12

void SetUpPokedexAreaMapBgs(void);
void LoadPokedexAreaMapGfx(void);
bool32 TryShowPokedexAreaMap(void);
void ResetPokedexAreaMapBg(void);
void PokedexAreaMapCenterOn(s32 x, s32 y, bool32 snap);
void PokedexAreaMapPan(s32 dx, s32 dy);
void PokedexAreaMapUpdateScroll(void);
void PokedexAreaMapGetScroll(s32 *x, s32 *y);

#endif // GUARD_POKEDEX_AREA_REGION_MAP_H
