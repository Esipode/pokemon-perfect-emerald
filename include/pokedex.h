#ifndef GUARD_POKEDEX_H
#define GUARD_POKEDEX_H

#include "bg.h"
#include "window.h"

extern void (*gPokedexVBlankCB)(void);

void ResetPokedex(void);
u16 GetNationalPokedexCount(u8 caseID);
u32 GetRegionalPokedexCount(u8 caseID);
u16 GetHoennPokedexCount(u8 caseID);
u16 GetKantoPokedexCount(u8 caseID);
u16 GetDexModePokedexCount(u8 dexMode, u8 caseID);
u8 DisplayCaughtMonDexPage(enum Species species, bool32 isShiny, u32 personality);
s8 GetSetPokedexFlag(enum NationalDexOrder nationalDexNo, u8 caseID);
u32 SpeciesToDexFlagSlot(enum Species species);
s8 GetSetPokedexFlagBySpecies(enum Species species, u8 caseID);

enum DexCaughtState
{
    DEX_CAUGHT_NONE,      // keeps existing `if (owned)` tests working
    DEX_CAUGHT_PARTIAL,
    DEX_CAUGHT_ALL,
};
enum DexCaughtState GetDexEntryCaughtState(enum Species baseSpecies);
enum Species GetDexEntryDisplaySpecies(enum Species baseSpecies);
bool32 GetDexEntrySeenState(enum Species baseSpecies);

bool32 IsSpeciesInDexMode(enum Species species, u8 dexMode);
u32 GetDexModeEntryCount(u8 dexMode);
u8 SanitizeDexMode(u8 dexMode);
void DrawFootprint(u8 windowId, enum Species species);
u16 CreateMonSpriteFromNationalDexNumber(enum NationalDexOrder nationalNum, s16 x, s16 y, u16 paletteSlot);
bool16 HasAllRegionalMons(void);
bool16 HasAllHoennMons(void);
bool16 HasAllKantoMons(void);
void ResetPokedexScrollPositions(void);
bool16 HasAllMons(void);
void CB2_OpenPokedex(void);
void PrintMonMeasurements(enum Species species, u32 owned);
u8* ConvertMonHeightToString(u32 height);
u8* ConvertMonWeightToString(u32 weight);
const u8 *GetPokedexCategoryName(u16 dexNum);
bool32 ShouldSkipPokedexListEntry(enum NationalDexOrder dexNum);

#endif // GUARD_POKEDEX_H
