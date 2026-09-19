#include "global.h"
#include "recruits_mode.h"
#include "achievements.h"
#include "battle.h"
#include "battle_pyramid.h"
#include "event_data.h"
#include "event_scripts.h"
#include "field_screen_effect.h"
#include "overworld.h"
#include "party_menu.h"
#include "pokemon.h"
#include "pokemon_storage_system.h"
#include "save.h"
#include "script.h"
#include "string_util.h"
#include "trainer_hill.h"
#include "constants/battle.h"
#include "constants/flags.h"


bool32 Recruits_IsEnabled(void)
{
    return gSaveBlock1Ptr->recruitsModeEnabled != 0;
}

bool32 Recruits_IsActive(void)
{
    // Not FLAG_SYS_POKEMON_GET (set when the starter is chosen): Recruits waits
    // for the Pokédex, the script node that also sets FLAG_NUZLOCKE_CATCH_MODE.
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

// Field hook for ProcessPlayerFieldInput (src/field_control_avatar.c). Rescans
// each call instead of queueing, so soft-resets and RemoveFaintedMonsFromParty
// compacting the party cannot invalidate a cached slot.
bool32 Recruits_TryStartFieldScript(void)
{
    u32 i;

    if (!Recruits_IsActive())
        return FALSE;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];

        if (GetMonData(mon, MON_DATA_SPECIES) == SPECIES_NONE)
            continue;
        if (GetMonData(mon, MON_DATA_RECRUIT_BATTLES) < RECRUITS_MAX_BATTLES)
            continue;

        GetMonNickname(mon, gStringVar1);
        gSpecialVar_0x8004 = i;
        ScriptContext_SetupScript(Recruits_EventScript_Retire);
        return TRUE;
    }

    return FALSE;
}

// Removes the party mon at gSpecialVar_0x8004 and compacts the party. Skips
// TryRevertPartyMonFormChange (unlike Draft): it dereferences gBattleStruct,
// which is NULL in the overworld, and HandleEndTurn_FinishBattle already reverted forms.
void Recruits_DoRetirement(void)
{
    u8 slot = gSpecialVar_0x8004;

    ZeroMonData(&gParties[B_TRAINER_PLAYER][slot]);
    // CompactPartySlots keeps gPartiesCount in sync, unlike RemoveFaintedMonsFromParty.
    CompactPartySlots();
    CalculatePlayerPartyCount();

    Achievement_RecordRecruitRetirement();

    if (gSaveBlock1Ptr->autosaveModeEnabled)
        gDoAutosave = TRUE;
}

void Recruits_IsRunFailed(void)
{
    gSpecialVar_Result = IsPartyEmpty();
}

// Persists the emptied party (so the title screen's CONTINUE gate reads true
// state) and hands off to the run-failed prompt Nuzlocke uses. Never goes
// through CB2_WhiteOut/DoWhiteOut: that resets FLAG_DEFEATED_ELITE_4_* progress
// (data/event_scripts.s), erasing E4 progress on the winning battle.
void Recruits_StartRunFailedScreen(void)
{
    TrySavingData(SAVE_NORMAL);
    ScriptContext_Stop();
    FieldCB_NuzlockeRunFailed();
}
