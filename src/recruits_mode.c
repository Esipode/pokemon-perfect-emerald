#include "global.h"
#include "recruits_mode.h"
#include "event_data.h"
#include "pokemon.h"
#include "constants/flags.h"

// Shared rules for the Recruits challenge. See include/recruits_mode.h.

bool32 Recruits_IsEnabled(void)
{
    return gSaveBlock1Ptr->recruitsModeEnabled != 0;
}

bool32 Recruits_IsActive(void)
{
    // Not FLAG_SYS_POKEMON_GET (set the moment the starter is chosen) -
    // Recruits doesn't engage until the player is back in the lab with their
    // Pokédex in hand, after the first rival battle. FLAG_SYS_POKEDEX_GET is
    // set at LittlerootTown_ProfessorBirchsLab_EventScript_ReceivePokedex,
    // the same script node that sets FLAG_NUZLOCKE_CATCH_MODE - so this
    // genuinely mirrors when Nuzlocke's own catch restrictions start
    // engaging, not just the "player has a Pokémon" convention the old flag
    // suggested.
    return Recruits_IsEnabled() && FlagGet(FLAG_SYS_POKEDEX_GET);
}

u32 Recruits_GetBattlesLeft(struct Pokemon *mon)
{
    return RECRUITS_MAX_BATTLES - GetMonData(mon, MON_DATA_RECRUIT_BATTLES);
}
