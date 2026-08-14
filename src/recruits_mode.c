#include "global.h"
#include "recruits_mode.h"
#include "battle.h"
#include "battle_pyramid.h"
#include "event_data.h"
#include "pokemon.h"
#include "trainer_hill.h"
#include "constants/battle.h"
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

static bool32 Recruits_BattleCounts(void)
{
    if (!Recruits_IsActive())
        return FALSE;
    if (!(gBattleTypeFlags & BATTLE_TYPE_TRAINER))
        return FALSE;
    // Excluded: battles where gParties[B_TRAINER_PLAYER] isn't really the
    // player's own party for the duration (Frontier/Pyramid/Trainer Hill swap
    // or reduce it), plus link/recorded/tutorial battles.
    if (gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_RECORDED
                          | BATTLE_TYPE_RECORDED_LINK | BATTLE_TYPE_FIRST_BATTLE
                          | BATTLE_TYPE_SAFARI | BATTLE_TYPE_CATCH_TUTORIAL
                          | BATTLE_TYPE_EREADER_TRAINER | BATTLE_TYPE_FRONTIER
                          | BATTLE_TYPE_TRAINER_HILL | BATTLE_TYPE_SECRET_BASE
                          | BATTLE_TYPE_POKEDUDE))
        return FALSE;
    if (CurrentBattlePyramidLocation() != PYRAMID_LOCATION_NONE || InTrainerHillChallenge())
        return FALSE;
    return TRUE;
}

void Recruits_TallyParticipants(void)
{
    u32 i;

    if (!Recruits_BattleCounts())
        return;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];
        u8 battles;

        if (GetMonData(mon, MON_DATA_SPECIES) == SPECIES_NONE)
            continue;
        if (!gBattleStruct->partyState[B_TRAINER_PLAYER][i].sentOut)
            continue;

        battles = min(GetMonData(mon, MON_DATA_RECRUIT_BATTLES) + 1, RECRUITS_MAX_BATTLES);
        SetMonData(mon, MON_DATA_RECRUIT_BATTLES, &battles);
    }
}
