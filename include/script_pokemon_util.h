#ifndef GUARD_SCRIPT_POKEMON_UTIL_H
#define GUARD_SCRIPT_POKEMON_UTIL_H

#include "constants/pokeball.h"
#include "constants/pokemon.h"

u32 ScriptGiveMon(enum Species species, u16 level, enum Item item);
u8 ScriptGiveEgg(enum Species species);
void CreateScriptedWildMon(enum Species species, u16 level, enum Item item);
void CreateScriptedDoubleWildMon(enum Species species, u16 level, enum Item item, enum Species species2, u16 level2, enum Item item2);
void ScriptSetMonMoveSlot(u8 monIndex, enum Move move, u8 slot);
void ReducePlayerPartyToSelectedMons(void);
void HealPlayerParty(void);
void Script_GetChosenMonOffensiveEVs(void);
void Script_GetChosenMonDefensiveEVs(void);
void Script_GetChosenMonOffensiveIVs(void);
void Script_GetChosenMonDefensiveIVs(void);
u32 ScriptGiveMonParameterized(u8 side, u8 slot, enum Species species, u16 level, enum Item item, enum PokeBall ball, u8 nature, u8 abilityNum, u8 gender, u16 *evs, u16 *ivs, enum Move *moves, enum ShinyMode shinyMode, bool8 gmaxFactor, enum Type teraType, u8 dmaxLevel);
// Was previously only reachable through the `specialvar` script dispatch
// table (data/specials.inc), which doesn't need a header prototype - never
// had a C-callable declaration. Added here (its natural home, alongside
// every other function this file already declares) so src/trade_code_
// session.c (Trading Codes.md Stage 7) can call it directly to mirror
// CableClub_EventScript_CheckPartyTradeRequirements's own Enigma Berry gate
// without duplicating CheckPartyMonHasHeldItem's logic.
bool8 DoesPartyHaveEnigmaBerry(void);

#endif // GUARD_SCRIPT_POKEMON_UTIL_H
