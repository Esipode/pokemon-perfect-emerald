#include "global.h"
#include "battle.h"
#include "bug_contest.h"
#include "event_data.h"
#include "field_screen_effect.h"
#include "item.h"
#include "limited_party.h"
#include "main.h"
#include "overworld.h"
#include "pokemon.h"
#include "pokemon_storage_system.h"
#include "random.h"
#include "script.h"
#include "string_util.h"
#include "constants/flags.h"
#include "constants/items.h"

// Johto Bug-Catching Contest. The party swap (SavePlayerParty/LoadPlayerParty) is done by the contest scripts.

static u32 sBugContestStartTime;
static bool8 sBugContestTimerActive;

bool32 GetBugContestFlag(void)
{
    return FlagGet(FLAG_SYS_BUG_CONTEST_MODE);
}

void BugContestRetirePrompt(void)
{
    ScriptContext_SetupScript(BugContest_EventScript_TimesUp);
}

void EnterBugContestMode(void)
{
    FlagSet(FLAG_SYS_BUG_CONTEST_MODE);
    sBugContestStartTime = gMain.vblankCounter1;
    sBugContestTimerActive = TRUE;
}

void ExitBugContestMode(void)
{
    FlagClear(FLAG_SYS_BUG_CONTEST_MODE);
    sBugContestTimerActive = FALSE;
}

// Runs the time-up script once the contest has lasted BUG_CONTEST_TIME_LIMIT_FRAMES.
bool8 BugContestCheckTimeLimit(void)
{
    if (!FlagGet(FLAG_SYS_BUG_CONTEST_MODE) || !sBugContestTimerActive)
        return FALSE;

    if (gMain.vblankCounter1 - sBugContestStartTime >= BUG_CONTEST_TIME_LIMIT_FRAMES)
    {
        sBugContestTimerActive = FALSE;
        ScriptContext_SetupScript(BugContest_EventScript_TimesUp);
        return TRUE;
    }
    return FALSE;
}

// Moves the party member at VAR_0x8004 to the PC. VAR_RESULT: MON_GIVEN_TO_PC or MON_CANT_GIVE.
bool8 TransferBugContestMon(void)
{
    struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][VarGet(VAR_0x8004)];

    if (CopyMonToPC(mon) == MON_GIVEN_TO_PC)
    {
        ZeroMonData(mon);
        CompactPartySlots();
        gSpecialVar_Result = MON_GIVEN_TO_PC;
    }
    else
    {
        gSpecialVar_Result = MON_CANT_GIVE;
    }
    return FALSE;
}

static const u16 sFirstPlaceRewards[] = { ITEM_MOON_STONE, ITEM_SUN_STONE, ITEM_LEAF_STONE };
static const u16 sSecondPlaceRewards[] = { ITEM_FIRE_STONE, ITEM_THUNDER_STONE, ITEM_WATER_STONE };
static const u16 sThirdPlaceRewards[] =
{
    ITEM_ORAN_BERRY, ITEM_CHERI_BERRY, ITEM_PERSIM_BERRY,
    ITEM_PECHA_BERRY, ITEM_RAWST_BERRY, ITEM_ASPEAR_BERRY, ITEM_CHESTO_BERRY
};

// Ranks the party member at VAR_0x8004 by max HP. VAR_RESULT: placement 1-3. VAR_0x8005: reward item.
bool8 JudgeBugContestMon(void)
{
    u32 maxHP = GetMonData(&gParties[B_TRAINER_PLAYER][VarGet(VAR_0x8004)], MON_DATA_MAX_HP);
    u32 roll = Random() % 100;

    if (maxHP < 41)
        gSpecialVar_Result = 3;
    else if (maxHP <= 46)
        gSpecialVar_Result = (roll < 50) ? 2 : 3;
    else if (maxHP <= 47)
        gSpecialVar_Result = (roll < 75) ? 1 : 2;
    else
        gSpecialVar_Result = 1;

    switch (gSpecialVar_Result)
    {
    case 1:
        VarSet(VAR_0x8005, sFirstPlaceRewards[Random() % ARRAY_COUNT(sFirstPlaceRewards)]);
        break;
    case 2:
        VarSet(VAR_0x8005, sSecondPlaceRewards[Random() % ARRAY_COUNT(sSecondPlaceRewards)]);
        break;
    default:
        VarSet(VAR_0x8005, sThirdPlaceRewards[Random() % ARRAY_COUNT(sThirdPlaceRewards)]);
        break;
    }
    return FALSE;
}

void CB2_EndBugContestBattle(void)
{
    u32 partyCount = 0;

    CpuFill16(0, (void *)BG_PLTT, BG_PLTT_SIZE);
    ResetOamRange(0, 128);
    for (u32 i = 0; i < PARTY_SIZE; i++)
    {
        if (GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_SPECIES) != SPECIES_NONE)
            partyCount++;
    }

    if (gBattleOutcome == B_OUTCOME_LOST || gBattleOutcome == B_OUTCOME_DREW)
    {
        SetMainCallback2(CB2_BugContestWhiteOut);
        return;
    }

    // A catch with a full party ends the contest.
    if (gBattleOutcome == B_OUTCOME_CAUGHT && partyCount == LimitedParty_GetMaxPartySize())
        ScriptContext_SetupScript(BugContest_EventScript_TimesUp);
    SetMainCallback2(CB2_ReturnToField);
    gFieldCallback = FieldCB_ReturnToFieldNoScriptCheckMusic;
}

bool8 RemoveSportBalls(void)
{
    u32 count = CountTotalItemQuantityInBag(ITEM_SPORT_BALL);

    if (count > 0)
        RemoveBagItem(ITEM_SPORT_BALL, count);
    return FALSE;
}

// Buffers the species name of the party member at VAR_0x8004 in STR_VAR_1. VAR_RESULT: text index 22-31 for
// the contest bugs, 0 otherwise.
bool8 ShowBugContestChosenMon(void)
{
    enum Species species = GetMonData(&gParties[B_TRAINER_PLAYER][VarGet(VAR_0x8004)], MON_DATA_SPECIES);

    StringCopy(gStringVar1, GetSpeciesName(species));

    switch (species)
    {
    case SPECIES_CATERPIE:   gSpecialVar_Result = 22; break;
    case SPECIES_WEEDLE:     gSpecialVar_Result = 23; break;
    case SPECIES_METAPOD:    gSpecialVar_Result = 24; break;
    case SPECIES_KAKUNA:     gSpecialVar_Result = 25; break;
    case SPECIES_PARAS:      gSpecialVar_Result = 26; break;
    case SPECIES_VENONAT:    gSpecialVar_Result = 27; break;
    case SPECIES_BUTTERFREE: gSpecialVar_Result = 28; break;
    case SPECIES_BEEDRILL:   gSpecialVar_Result = 29; break;
    case SPECIES_SCYTHER:    gSpecialVar_Result = 30; break;
    case SPECIES_PINSIR:     gSpecialVar_Result = 31; break;
    default:                 gSpecialVar_Result = 0;  break;
    }
    return FALSE;
}
