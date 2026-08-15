#include "global.h"
#include "achievements.h"
#include "main.h"
#include "credits.h"
#include "event_data.h"
#include "hall_of_fame.h"
#include "hall_of_fame_frlg.h"
#include "load_save.h"
#include "overworld.h"
#include "script_pokemon_util.h"
#include "tv.h"
#include "constants/heal_locations.h"

int GameClear(void)
{
    int i;
    bool32 ribbonGet;
    struct RibbonCounter {
        u8 partyIndex;
        u8 count;
    } ribbonCounts[6];

    HealPlayerParty();

    if (FlagGet(FLAG_SYS_GAME_CLEAR) == TRUE)
    {
        gHasHallOfFameRecords = TRUE;
    }
    else
    {
        gHasHallOfFameRecords = FALSE;
        FlagSet(FLAG_SYS_GAME_CLEAR);
        // This branch only runs the first time
        // FLAG_SYS_GAME_CLEAR is set for this save, which is what makes it
        // the trigger for the one-time first-playthrough unlock.
        Achievement_OnFirstPlaythroughComplete();
        // Category L's "complete the story" entries, gated on this
        // same re-runs-every-NG+-cycle branch (see the comment below) so a
        // different mono-type/rebuild/etc. run in NG+ is checked fresh.
        Achievement_CheckTeamCompletionMilestones();
        // Investor, same re-runs-every-NG+-cycle
        // gating as the team-completion check above.
        Achievement_CheckEconomyCompletionMilestones();
        // Challenge Runs & Nuzlocke completion
        // entries, same gating. The Nuzlocke half gates itself internally on
        // gSaveBlock1Ptr->nuzlockeModeEnabled.
        Achievement_CheckChallengeCompletionMilestones();
        Achievement_CheckNuzlockeCompletionMilestones();
        // Legend of the Run, same gating.
        Achievement_CheckRecordsCompletionMilestones();
        // Recruits/Limited Party/Draft/Rotation/Mono Type/Mono Gen
        // completion entries, plus Cross-Mode stacking. Same gating.
        Achievement_CheckNewModeCompletionMilestones();
        // FLAG_SYS_GAME_CLEAR isn't preserved across New Game+
        // (see NewGameInitData, src/new_game.c), so this branch already
        // re-runs on every NG+ cycle's clear, not just the save's very first
        // playthrough -- that's exactly what lets this count cycle
        // completions specifically, alongside the plain playthrough count
        // Achievement_OnFirstPlaythroughComplete already tracks above.
        if (gSaveBlock2Ptr->newGamePlus > 0)
            Achievement_OnNewGamePlusCycleCompleted();
    }

    if (GetGameStat(GAME_STAT_FIRST_HOF_PLAY_TIME) == 0)
        SetGameStat(GAME_STAT_FIRST_HOF_PLAY_TIME, (gSaveBlock2Ptr->playTimeHours << 16) | (gSaveBlock2Ptr->playTimeMinutes << 8) | gSaveBlock2Ptr->playTimeSeconds);

    SetContinueGameWarpStatus();

    if (gSaveBlock2Ptr->playerGender == MALE)
        SetContinueGameWarpToHealLocation(HEAL_LOCATION_LITTLEROOT_TOWN_BRENDANS_HOUSE_2F);
    else
        SetContinueGameWarpToHealLocation(HEAL_LOCATION_LITTLEROOT_TOWN_MAYS_HOUSE_2F);

    ribbonGet = FALSE;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];

        ribbonCounts[i].partyIndex = i;
        ribbonCounts[i].count = 0;

        if (GetMonData(mon, MON_DATA_SANITY_HAS_SPECIES)
         && !GetMonData(mon, MON_DATA_SANITY_IS_EGG)
         && !GetMonData(mon, MON_DATA_CHAMPION_RIBBON))
        {
            u8 val[1] = {TRUE};
            SetMonData(mon, MON_DATA_CHAMPION_RIBBON, val);
            ribbonCounts[i].count = GetRibbonCount(mon);
            ribbonGet = TRUE;
        }
    }

    if (ribbonGet == TRUE)
    {
        IncrementGameStat(GAME_STAT_RECEIVED_RIBBONS);
        FlagSet(FLAG_SYS_RIBBON_GET);

        for (i = 1; i < 6; i++)
        {
            if (ribbonCounts[i].count > ribbonCounts[0].count)
            {
                struct RibbonCounter prevBest = ribbonCounts[0];
                ribbonCounts[0] = ribbonCounts[i];
                ribbonCounts[i] = prevBest;
            }
        }

        if (ribbonCounts[0].count > NUM_CUTIES_RIBBONS)
        {
            TryPutSpotTheCutiesOnAir(&gParties[B_TRAINER_PLAYER][ribbonCounts[0].partyIndex], MON_DATA_CHAMPION_RIBBON);
        }
    }

    SetMainCallback2(CB2_DoHallOfFameScreen);
    return 0;
}

bool8 SetCB2WhiteOut(void)
{
    SetMainCallback2(CB2_WhiteOut);
    return FALSE;
}

bool8 EnterHallOfFame(void)
{
    bool8 ribbonState;
    bool8 *r7;
    int i;
    bool8 gaveAtLeastOneRibbon;
    HealPlayerParty();
    if (FlagGet(FLAG_SYS_GAME_CLEAR) == TRUE)
    {
        gHasHallOfFameRecords = TRUE;
    }
    else
    {
        gHasHallOfFameRecords = FALSE;
        FlagSet(FLAG_SYS_GAME_CLEAR);
    }
    if (GetGameStat(GAME_STAT_FIRST_HOF_PLAY_TIME) == 0)
    {
        SetGameStat(GAME_STAT_FIRST_HOF_PLAY_TIME, (gSaveBlock2Ptr->playTimeHours << 16) | (gSaveBlock2Ptr->playTimeMinutes << 8) | gSaveBlock2Ptr->playTimeSeconds);
    }
    SetContinueGameWarpStatus();
    SetContinueGameWarpToHealLocation(HEAL_LOCATION_PALLET_TOWN);
    gaveAtLeastOneRibbon = FALSE;
    for (i = 0, r7 = &ribbonState; i < PARTY_SIZE; i++)
    {
        if (GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_SANITY_HAS_SPECIES) && !GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_SANITY_IS_EGG))
        {
            if (!GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_CHAMPION_RIBBON))
            {
                *r7 = TRUE;
                SetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_CHAMPION_RIBBON, &ribbonState);
                gaveAtLeastOneRibbon = TRUE;
            }
        }
    }
    if (gaveAtLeastOneRibbon == TRUE)
    {
        IncrementGameStat(GAME_STAT_RECEIVED_RIBBONS);
        FlagSet(FLAG_SYS_RIBBON_GET);
    }
    SetMainCallback2(CB2_DoHallOfFameScreenFrlg);
    return FALSE;
}
