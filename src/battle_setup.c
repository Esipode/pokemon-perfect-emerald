#include "global.h"
#include "achievements.h"
#include "data.h"
#include "main.h"
#include "battle.h"
#include "battle_emporium.h"
#include "battle_frontier.h"
#include "battle_pike.h"
#include "battle_pyramid.h"
#include "battle_setup.h"
#include "battle_partner.h"
#include "battle_tower.h"
#include "battle_transition.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "event_scripts.h"
#include "fieldmap.h"
#include "field_name_box.h"
#include "field_control_avatar.h"
#include "field_message_box.h"
#include "field_player_avatar.h"
#include "field_screen_effect.h"
#include "field_weather.h"
#include "fishing.h"
#include "fldeff.h"
#include "fldeff_misc.h"
#include "follower_npc.h"
#include "item.h"
#include "load_save.h"
#include "malloc.h"
#include "metatile_behavior.h"
#include "mirage_tower.h"
#include "palette.h"
#include "random.h"
#include "safari_zone.h"
#include "script.h"
#include "script_pokemon_util.h"
#include "sound.h"
#include "starter_choose.h"
#include "strings.h"
#include "string_util.h"
#include "task.h"
#include "trainer_hill.h"
#include "trainer_pools.h"
#include "trainer_see.h"
#include "trainer_util.h"
#include "tv.h"
#include "overworld.h"
#include "wild_encounter_ow.h"
#include "window.h"
#include "constants/battle_frontier.h"
#include "constants/battle_special.h"
#include "constants/event_objects.h"
#include "constants/game_stat.h"
#include "constants/items.h"
#include "constants/songs.h"
#include "constants/script_commands.h"
#include "constants/trainers.h"
#include "constants/trainer_hill.h"
#include "constants/weather.h"

enum TransitionType
{
    TRANSITION_TYPE_NORMAL,
    TRANSITION_TYPE_CAVE,
    TRANSITION_TYPE_FLASH,
    TRANSITION_TYPE_WATER,
};

// this file's functions
static void DoBattlePikeWildBattle(void);
static void DoSafariBattle(void);
static void DoGhostBattle(void);
static void DoStandardWildBattle(bool32 isDouble);
static void CB2_EndWildBattle(void);
static void CB2_EndScriptedWildBattle(void);
static void CB2_EndMarowakBattle(void);
static void CB2_GiveStarter(void);
static void CB2_StartFirstBattle(void);
static void CB2_EndFirstBattle(void);
static void SaveChangesToPlayerParty(void);
static void HandleBattleVariantEndParty(void);
static void CB2_EndTrainerBattle(void);
static bool32 IsPlayerDefeated(u32 battleOutcome);
static const u8 *GetIntroSpeechOfApproachingTrainer(void);
static const u8 *GetTrainerCantBattleSpeech(void);
static void CreateNPCTrainerParty(struct Pokemon *party, u16 trainerNum);
static void DoTrainerBattle(void);

EWRAM_DATA TrainerBattleParameter gTrainerBattleParameter = {0};
EWRAM_DATA u16 gPartnerTrainerId = 0;
EWRAM_DATA static u8 *sTrainerBattleEndScript = NULL;
EWRAM_DATA static bool8 sShouldCheckTrainerBScript = FALSE;
EWRAM_DATA static u8 sNoOfPossibleTrainerRetScripts = 0;

// The first transition is used if the enemy Pokémon are lower level than our Pokémon.
// Otherwise, the second transition is used.
static const u8 sBattleTransitionTable_Wild[][2] =
{
    [TRANSITION_TYPE_NORMAL] = {B_TRANSITION_SLICE,          B_TRANSITION_WHITE_BARS_FADE},
    [TRANSITION_TYPE_CAVE]   = {B_TRANSITION_CLOCKWISE_WIPE, B_TRANSITION_GRID_SQUARES},
    [TRANSITION_TYPE_FLASH]  = {B_TRANSITION_BLUR,           B_TRANSITION_GRID_SQUARES},
    [TRANSITION_TYPE_WATER]  = {B_TRANSITION_WAVE,           B_TRANSITION_RIPPLE},
};

static const u8 sBattleTransitionTable_Trainer[][2] =
{
    [TRANSITION_TYPE_NORMAL] = {B_TRANSITION_POKEBALLS_TRAIL, B_TRANSITION_ANGLED_WIPES},
    [TRANSITION_TYPE_CAVE]   = {B_TRANSITION_SHUFFLE,         B_TRANSITION_BIG_POKEBALL},
    [TRANSITION_TYPE_FLASH]  = {B_TRANSITION_BLUR,            B_TRANSITION_GRID_SQUARES},
    [TRANSITION_TYPE_WATER]  = {B_TRANSITION_SWIRL,           B_TRANSITION_RIPPLE},
};

// Battle Frontier (excluding Pyramid and Dome, which have their own tables below)
static const u8 sBattleTransitionTable_BattleFrontier[] =
{
    B_TRANSITION_FRONTIER_LOGO_WIGGLE,
    B_TRANSITION_FRONTIER_LOGO_WAVE,
    B_TRANSITION_FRONTIER_SQUARES,
    B_TRANSITION_FRONTIER_SQUARES_SCROLL,
    B_TRANSITION_FRONTIER_CIRCLES_MEET,
    B_TRANSITION_FRONTIER_CIRCLES_CROSS,
    B_TRANSITION_FRONTIER_CIRCLES_ASYMMETRIC_SPIRAL,
    B_TRANSITION_FRONTIER_CIRCLES_SYMMETRIC_SPIRAL,
    B_TRANSITION_FRONTIER_CIRCLES_MEET_IN_SEQ,
    B_TRANSITION_FRONTIER_CIRCLES_CROSS_IN_SEQ,
    B_TRANSITION_FRONTIER_CIRCLES_ASYMMETRIC_SPIRAL_IN_SEQ,
    B_TRANSITION_FRONTIER_CIRCLES_SYMMETRIC_SPIRAL_IN_SEQ
};

static const u8 sBattleTransitionTable_BattlePyramid[] =
{
    B_TRANSITION_FRONTIER_SQUARES,
    B_TRANSITION_FRONTIER_SQUARES_SCROLL,
    B_TRANSITION_FRONTIER_SQUARES_SPIRAL
};

static const u8 sBattleTransitionTable_BattleDome[] =
{
    B_TRANSITION_FRONTIER_LOGO_WIGGLE,
    B_TRANSITION_FRONTIER_SQUARES,
    B_TRANSITION_FRONTIER_SQUARES_SCROLL,
    B_TRANSITION_FRONTIER_SQUARES_SPIRAL
};

#define tState data[0]
#define tTransition data[1]

static void Task_BattleStart(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    switch (tState)
    {
    case 0:
        if (!FldEffPoison_IsActive()) // is poison not active?
        {
            BattleTransition_StartOnField(tTransition);
            ClearMirageTowerPulseBlendEffect();
            tState++; // go to case 1.
        }
        break;
    case 1:
        if (IsBattleTransitionDone() == TRUE)
        {
            PrepareForFollowerNPCBattle();
            CleanupOverworldWindowsAndTilemaps();
            SetMainCallback2(CB2_InitBattle);
            RestartWildEncounterImmunitySteps();
            ClearPoisonStepCounter();
            DespawnOWEOnBattleStart();
            DestroyTask(taskId);
        }
        break;
    }
}

static void CreateBattleStartTask(enum BattleTransition transition, u16 song)
{
    u8 taskId = CreateTask(Task_BattleStart, 1);

    gTasks[taskId].tTransition = transition;
    PlayMapChosenOrBattleBGM(song);
}

static void Task_BattleStart_Debug(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    switch (tState)
    {
    case 0:
        if (!FldEffPoison_IsActive()) // is poison not active?
        {
            BattleTransition_StartOnField(tTransition);
            ClearMirageTowerPulseBlendEffect();
            tState++; // go to case 1.
        }
        break;
    case 1:
        if (IsBattleTransitionDone() == TRUE)
        {
            CleanupOverworldWindowsAndTilemaps();
            SetMainCallback2(CB2_InitBattle);
            RestartWildEncounterImmunitySteps();
            ClearPoisonStepCounter();
            DestroyTask(taskId);
        }
        break;
    }
}

static void CreateBattleStartTask_Debug(u8 transition, u16 song)
{
    u8 taskId = CreateTask(Task_BattleStart_Debug, 1);

    gTasks[taskId].tTransition = transition;
    PlayMapChosenOrBattleBGM(song);
}

#undef tState
#undef tTransition

static bool8 CheckSilphScopeInPokemonTower(u16 mapGroup, u16 mapNum)
{
    if (mapGroup == MAP_GROUP(MAP_POKEMON_TOWER_1F)
     && (mapNum == MAP_NUM(MAP_POKEMON_TOWER_1F)
      || mapNum == MAP_NUM(MAP_POKEMON_TOWER_2F)
      || mapNum == MAP_NUM(MAP_POKEMON_TOWER_3F)
      || mapNum == MAP_NUM(MAP_POKEMON_TOWER_4F)
      || mapNum == MAP_NUM(MAP_POKEMON_TOWER_5F)
      || mapNum == MAP_NUM(MAP_POKEMON_TOWER_6F)
      || mapNum == MAP_NUM(MAP_POKEMON_TOWER_7F))
     && !(CheckBagHasItem(ITEM_SILPH_SCOPE, 1)))
        return TRUE;
    else
        return FALSE;
}

void BattleSetup_StartWildBattle(void)
{
    if (GetSafariZoneFlag())
        DoSafariBattle();
    else if (CheckSilphScopeInPokemonTower(gSaveBlock1Ptr->location.mapGroup, gSaveBlock1Ptr->location.mapNum))
        DoGhostBattle();
    else
        DoStandardWildBattle(FALSE);
}

void BattleSetup_StartDoubleWildBattle(void)
{
    DoStandardWildBattle(TRUE);
}

void BattleSetup_StartMultiBattle(void)
{
    if (gSpecialVar_0x8005 & MULTI_BATTLE_2_VS_WILD) // Player + AI against wild mon
    {
        gBattleTypeFlags = BATTLE_TYPE_DOUBLE | BATTLE_TYPE_MULTI | BATTLE_TYPE_INGAME_PARTNER;
    }
    else if (gSpecialVar_0x8005 & MULTI_BATTLE_2_VS_1) // Player + AI against one trainer
    {
        TRAINER_BATTLE_PARAM.opponentB = 0xFFFF;
        gBattleTypeFlags = BATTLE_TYPE_TRAINER | BATTLE_TYPE_DOUBLE | BATTLE_TYPE_MULTI | BATTLE_TYPE_INGAME_PARTNER;
    }
    else // MULTI_BATTLE_2_VS_2
    {
        gBattleTypeFlags = BATTLE_TYPE_TRAINER | BATTLE_TYPE_DOUBLE | BATTLE_TYPE_TWO_OPPONENTS | BATTLE_TYPE_MULTI | BATTLE_TYPE_INGAME_PARTNER;
    }

    FillPartnerParty(gPartnerTrainerId);
    if (gSpecialVar_0x8005 & MULTI_BATTLE_CHOOSE_MONS) // Skip mons restoring(done in the script)
        gBattleScripting.specialTrainerBattleType = 0xFF;

    if (gSpecialVar_0x8005 & MULTI_BATTLE_2_VS_WILD)
    {
        CreateBattleStartTask(GetWildBattleTransition(), 0);
        IncrementGameStat(GAME_STAT_TOTAL_BATTLES);
        IncrementGameStat(GAME_STAT_WILD_BATTLES);
        IncrementDailyWildBattles();
    }
    else
    {
        DoTrainerBattle();
    }
}


void BattleSetup_StartBattlePikeWildBattle(void)
{
    DoBattlePikeWildBattle();
}

static void DoStandardWildBattle(bool32 isDouble)
{
    LockPlayerFieldControls();
    FreezeObjectEvents();
    StopPlayerAvatar();
    gMain.savedCallback = CB2_EndWildBattle;
    gBattleTypeFlags = 0;
    if (IsNPCFollowerWildBattle())
    {
        gBattleTypeFlags |= BATTLE_TYPE_MULTI | BATTLE_TYPE_INGAME_PARTNER | BATTLE_TYPE_DOUBLE;
    }
    else if (isDouble)
        gBattleTypeFlags |= BATTLE_TYPE_DOUBLE;
    if (CurrentBattlePyramidLocation() != PYRAMID_LOCATION_NONE)
    {
        VarSet(VAR_TEMP_E, 0);
        gBattleTypeFlags |= BATTLE_TYPE_PYRAMID;
    }
    CreateBattleStartTask(GetWildBattleTransition(), 0);
    IncrementGameStat(GAME_STAT_TOTAL_BATTLES);
    IncrementGameStat(GAME_STAT_WILD_BATTLES);
    IncrementDailyWildBattles();
}

void DoStandardWildBattle_Debug(void)
{
    LockPlayerFieldControls();
    FreezeObjectEvents();
    StopPlayerAvatar();
    gMain.savedCallback = CB2_EndWildBattle;
    gBattleTypeFlags = 0;
    if (CurrentBattlePyramidLocation() != PYRAMID_LOCATION_NONE)
    {
        VarSet(VAR_TEMP_PLAYING_PYRAMID_MUSIC, 0);
        gBattleTypeFlags |= BATTLE_TYPE_PYRAMID;
    }
    CreateBattleStartTask_Debug(GetWildBattleTransition(), 0);
    //IncrementGameStat(GAME_STAT_TOTAL_BATTLES);
    //IncrementGameStat(GAME_STAT_WILD_BATTLES);
    //IncrementDailyWildBattles();
}

void BattleSetup_StartRoamerBattle(void)
{
    LockPlayerFieldControls();
    FreezeObjectEvents();
    StopPlayerAvatar();
    gMain.savedCallback = CB2_EndWildBattle;
    gBattleTypeFlags = BATTLE_TYPE_ROAMER;
    CreateBattleStartTask(GetWildBattleTransition(), 0);
    IncrementGameStat(GAME_STAT_TOTAL_BATTLES);
    IncrementGameStat(GAME_STAT_WILD_BATTLES);
    IncrementDailyWildBattles();
}

static void DoSafariBattle(void)
{
    LockPlayerFieldControls();
    FreezeObjectEvents();
    StopPlayerAvatar();
    gMain.savedCallback = CB2_EndSafariBattle;
    gBattleTypeFlags = BATTLE_TYPE_SAFARI;
    CreateBattleStartTask(GetWildBattleTransition(), 0);
}

static void DoGhostBattle(void)
{
    LockPlayerFieldControls();
    FreezeObjectEvents();
    StopPlayerAvatar();
    gMain.savedCallback = CB2_EndWildBattle;
    gBattleTypeFlags = BATTLE_TYPE_GHOST;
    CreateBattleStartTask(GetWildBattleTransition(), 0);
    SetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_NICKNAME, gText_Ghost);
    IncrementGameStat(GAME_STAT_TOTAL_BATTLES);
    IncrementGameStat(GAME_STAT_WILD_BATTLES);
}

static void DoBattlePikeWildBattle(void)
{
    LockPlayerFieldControls();
    FreezeObjectEvents();
    StopPlayerAvatar();
    gMain.savedCallback = CB2_EndWildBattle;
    gBattleTypeFlags = BATTLE_TYPE_PIKE;
    CreateBattleStartTask(GetWildBattleTransition(), 0);
    IncrementGameStat(GAME_STAT_TOTAL_BATTLES);
    IncrementGameStat(GAME_STAT_WILD_BATTLES);
    IncrementDailyWildBattles();
}

static void DoTrainerBattle(void)
{
    CreateNPCTrainerParty(&gParties[B_TRAINER_OPPONENT_A][0], TRAINER_BATTLE_PARAM.opponentA);
    if (gBattleTypeFlags & BATTLE_TYPE_TWO_OPPONENTS && !BATTLE_TWO_VS_ONE_OPPONENT)
        CreateNPCTrainerParty(&gParties[B_TRAINER_OPPONENT_B][0], TRAINER_BATTLE_PARAM.opponentB);
    CreateBattleStartTask(GetTrainerBattleTransition(), 0);
    IncrementGameStat(GAME_STAT_TOTAL_BATTLES);
    IncrementGameStat(GAME_STAT_TRAINER_BATTLES);
}

static void DoBattlePyramidTrainerHillBattle(void)
{
    if (CurrentBattlePyramidLocation() != PYRAMID_LOCATION_NONE)
        CreateBattleStartTask(GetSpecialBattleTransition(B_TRANSITION_GROUP_B_PYRAMID), 0);
    else
        CreateBattleStartTask(GetSpecialBattleTransition(B_TRANSITION_GROUP_TRAINER_HILL), 0);

    IncrementGameStat(GAME_STAT_TOTAL_BATTLES);
    IncrementGameStat(GAME_STAT_TRAINER_BATTLES);
}

// Initiates battle where Wally catches Ralts
void StartWallyTutorialBattle(void)
{
    CreateMaleMon(&gParties[B_TRAINER_OPPONENT_A][0], SPECIES_RALTS, 5);
    LockPlayerFieldControls();
    gMain.savedCallback = CB2_ReturnToFieldContinueScriptPlayMapMusic;
    gBattleTypeFlags = BATTLE_TYPE_CATCH_TUTORIAL;
    CreateBattleStartTask(B_TRANSITION_SLICE, 0);
}

void StartOldManTutorialBattle(void)
{
    CreateMaleMon(&gParties[B_TRAINER_OPPONENT_A][0], SPECIES_WEEDLE, 5);
    LockPlayerFieldControls();
    gMain.savedCallback = CB2_ReturnToFieldContinueScriptPlayMapMusic;
    gBattleTypeFlags = BATTLE_TYPE_CATCH_TUTORIAL;
    CreateBattleStartTask(B_TRANSITION_SLICE, 0);
}

void BattleSetup_StartScriptedWildBattle(void)
{
    LockPlayerFieldControls();
    gMain.savedCallback = CB2_EndScriptedWildBattle;
    gBattleTypeFlags = 0;
    CreateBattleStartTask(GetWildBattleTransition(), 0);
    IncrementGameStat(GAME_STAT_TOTAL_BATTLES);
    IncrementGameStat(GAME_STAT_WILD_BATTLES);
    IncrementDailyWildBattles();
}

void BattleSetup_StartScriptedDoubleWildBattle(void)
{
    LockPlayerFieldControls();
    gMain.savedCallback = CB2_EndScriptedWildBattle;
    gBattleTypeFlags = BATTLE_TYPE_DOUBLE;
    CreateBattleStartTask(GetWildBattleTransition(), 0);
    IncrementGameStat(GAME_STAT_TOTAL_BATTLES);
    IncrementGameStat(GAME_STAT_WILD_BATTLES);
    IncrementDailyWildBattles();
}

void StartMarowakBattle(void)
{
    LockPlayerFieldControls();
    gMain.savedCallback = CB2_EndMarowakBattle;
    gBattleTypeFlags = BATTLE_TYPE_GHOST;

    if (CheckBagHasItem(ITEM_SILPH_SCOPE, 1))
    {
        u32 personality = GetMonPersonality(SPECIES_MAROWAK, MON_FEMALE, NATURE_SERIOUS, RANDOM_UNOWN_LETTER);

        CreateMonWithIVsPersonality(&gParties[B_TRAINER_OPPONENT_A][0], SPECIES_MAROWAK, 30, 31, personality);
    }

    CreateBattleStartTask(GetWildBattleTransition(), 0);
    SetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_NICKNAME, gText_Ghost);
    IncrementGameStat(GAME_STAT_TOTAL_BATTLES);
    IncrementGameStat(GAME_STAT_WILD_BATTLES);
}

void BattleSetup_StartLatiBattle(void)
{
    LockPlayerFieldControls();
    gMain.savedCallback = CB2_EndScriptedWildBattle;
    gBattleTypeFlags = BATTLE_TYPE_LEGENDARY;
    CreateBattleStartTask(GetWildBattleTransition(), 0);
    IncrementGameStat(GAME_STAT_TOTAL_BATTLES);
    IncrementGameStat(GAME_STAT_WILD_BATTLES);
    IncrementDailyWildBattles();
}

void BattleSetup_StartLegendaryBattle(void)
{
    LockPlayerFieldControls();
    gMain.savedCallback = CB2_EndScriptedWildBattle;
    gBattleTypeFlags = BATTLE_TYPE_LEGENDARY;

    switch (GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_SPECIES))
    {
    case SPECIES_GROUDON:
    case SPECIES_GROUDON_PRIMAL:
        CreateBattleStartTask(B_TRANSITION_GROUDON, MUS_VS_KYOGRE_GROUDON);
        break;
    case SPECIES_KYOGRE:
    case SPECIES_KYOGRE_PRIMAL:
        CreateBattleStartTask(B_TRANSITION_KYOGRE, MUS_VS_KYOGRE_GROUDON);
        break;
    case SPECIES_RAYQUAZA:
    case SPECIES_RAYQUAZA_MEGA:
        CreateBattleStartTask(B_TRANSITION_RAYQUAZA, MUS_VS_RAYQUAZA);
        break;
    case SPECIES_DEOXYS_NORMAL:
    case SPECIES_DEOXYS_ATTACK:
    case SPECIES_DEOXYS_DEFENSE:
    case SPECIES_DEOXYS_SPEED:
        CreateBattleStartTask(B_TRANSITION_BLUR, MUS_RG_VS_DEOXYS);
        break;
    case SPECIES_LUGIA:
    case SPECIES_HO_OH:
    default:
        CreateBattleStartTask(B_TRANSITION_BLUR, MUS_RG_VS_LEGEND);
        break;
    case SPECIES_MEW:
        CreateBattleStartTask(B_TRANSITION_GRID_SQUARES, MUS_VS_MEW);
        break;
    }

    IncrementGameStat(GAME_STAT_TOTAL_BATTLES);
    IncrementGameStat(GAME_STAT_WILD_BATTLES);
    IncrementDailyWildBattles();
}

void StartGroudonKyogreBattle(void)
{
    LockPlayerFieldControls();
    gMain.savedCallback = CB2_EndScriptedWildBattle;
    gBattleTypeFlags = BATTLE_TYPE_LEGENDARY;

    if (gGameVersion == VERSION_RUBY)
        CreateBattleStartTask(B_TRANSITION_ANGLED_WIPES, MUS_VS_KYOGRE_GROUDON); // GROUDON
    else
        CreateBattleStartTask(B_TRANSITION_RIPPLE, MUS_VS_KYOGRE_GROUDON); // KYOGRE

    IncrementGameStat(GAME_STAT_TOTAL_BATTLES);
    IncrementGameStat(GAME_STAT_WILD_BATTLES);
    IncrementDailyWildBattles();
}

void StartRegiBattle(void)
{
    enum BattleTransition transitionId;
    enum Species species;

    LockPlayerFieldControls();
    gMain.savedCallback = CB2_EndScriptedWildBattle;
    gBattleTypeFlags = BATTLE_TYPE_LEGENDARY;

    species = GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_SPECIES);
    switch (species)
    {
    case SPECIES_REGIROCK:
        transitionId = B_TRANSITION_REGIROCK;
        break;
    case SPECIES_REGICE:
        transitionId = B_TRANSITION_REGICE;
        break;
    case SPECIES_REGISTEEL:
        transitionId = B_TRANSITION_REGISTEEL;
        break;
    default:
        transitionId = B_TRANSITION_GRID_SQUARES;
        break;
    }
    CreateBattleStartTask(transitionId, MUS_VS_REGI);

    IncrementGameStat(GAME_STAT_TOTAL_BATTLES);
    IncrementGameStat(GAME_STAT_WILD_BATTLES);
    IncrementDailyWildBattles();
}

static void DowngradeBadPoison(void)
{
    u8 i;
    u32 status = STATUS1_POISON;
    if (B_TOXIC_REVERSAL < GEN_5)
        return;
    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_SANITY_HAS_SPECIES) && GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_STATUS) == STATUS1_TOXIC_POISON)
            SetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_STATUS, &status);
    }
}

static void CB2_EndWildBattle(void)
{
    // GAME_STAT_WILD_BATTLES is incremented at battle start, so it's current here regardless of outcome.
    Achievement_CheckWildBattleMilestones();

    CpuFill16(0, (void *)(BG_PLTT), BG_PLTT_SIZE);
    ResetOamRange(0, 128);

    if (IsNPCFollowerWildBattle())
    {
        RestorePartyAfterFollowerNPCBattle();
        if (FNPC_FLAG_HEAL_AFTER_FOLLOWER_BATTLE != 0
         && (FNPC_FLAG_HEAL_AFTER_FOLLOWER_BATTLE == FNPC_ALWAYS
         || FlagGet(FNPC_FLAG_HEAL_AFTER_FOLLOWER_BATTLE)))
            HealPlayerParty();
    }

    if (IsPlayerDefeated(gBattleOutcome) == TRUE && CurrentBattlePyramidLocation() == PYRAMID_LOCATION_NONE && !InBattlePike())
    {
        SetMainCallback2(CB2_WhiteOut);
    }
    else
    {
        // In a Draft run SET_NUZLOCKE_ZONE_FLAG means "this area's draft is spent", so this block must
        // never run for a wild battle that merely ended; only a draft pick (Draft_MarkAreaSpent) may
        // spend an area. Gating on nuzlockeModeEnabled does that: Draft and Nuzlocke are mutually
        // exclusive (src/new_game_settings_menu.c). Do not widen this condition to match Draft saves.
        if ((gSaveBlock1Ptr->nuzlockeModeEnabled && FlagGet(FLAG_NUZLOCKE_CATCH_MODE)))
        {
            u16 zone = GetCurrentRegionMapSectionId();

            // This branch runs for any wild battle that ended without a whiteout (caught, KO'd or
            // fled), so the flag means "you've had your encounter here".
            // BOOST_NUZLOCKE_SECOND_CHANCE: a non-catch spends a one-time per-zone free pass instead of
            // locking the zone. Catching still locks immediately, so this is a single retry.
            if (gBattleOutcome != B_OUTCOME_CAUGHT
             && AchievementBoost_HasNuzlockeSecondChance()
             && !GET_NUZLOCKE_ZONE_EXTRA_FLAG(zone))
                SET_NUZLOCKE_ZONE_EXTRA_FLAG(zone);
            else
                SET_NUZLOCKE_ZONE_FLAG(zone);

            gDoAutosaveAfterBattle = TRUE;
        }
        else if (gSaveBlock1Ptr->autosaveModeEnabled) {
            gDoAutosaveAfterBattle = TRUE;
        }
        SetMainCallback2(CB2_ReturnToField);
        DowngradeBadPoison();
        gFieldCallback = FieldCB_ReturnToFieldNoScriptCheckMusic;
    }
}

static void CB2_EndScriptedWildBattle(void)
{
    CpuFill16(0, (void *)(BG_PLTT), BG_PLTT_SIZE);
    ResetOamRange(0, 128);

    if (IsPlayerDefeated(gBattleOutcome) == TRUE)
    {
        if (CurrentBattlePyramidLocation() != PYRAMID_LOCATION_NONE)
            SetMainCallback2(CB2_ReturnToFieldContinueScriptPlayMapMusic);
        else
            SetMainCallback2(CB2_WhiteOut);
    }
    else
    {
        DowngradeBadPoison();
        SetMainCallback2(CB2_ReturnToFieldContinueScriptPlayMapMusic);
    }
}

static void CB2_EndMarowakBattle(void)
{
    CpuFill16(0, (void *)BG_PLTT, BG_PLTT_SIZE);
    ResetOamRange(0, 128);

    if (IsPlayerDefeated(gBattleOutcome))
    {
        SetMainCallback2(CB2_WhiteOut);
    }
    else
    {
        // If result is TRUE player didnt defeat Marowak, force player back from stairs
        if (gBattleOutcome == B_OUTCOME_WON)
            gSpecialVar_Result = FALSE;
        else
            gSpecialVar_Result = TRUE;
        DowngradeBadPoison();
        SetMainCallback2(CB2_ReturnToFieldContinueScriptPlayMapMusic);
    }
}

enum BattleEnvironments BattleSetup_GetEnvironmentId(void)
{
    u16 tileBehavior;
    s16 x, y;

    if (ShouldUseFishingEnvironmentInBattle())
        GetXYCoordsOneStepInFrontOfPlayer(&x, &y);
    else
        PlayerGetDestCoords(&x, &y);

    tileBehavior = MapGridGetMetatileBehaviorAt(x, y);

    if (MetatileBehavior_IsTallGrass(tileBehavior))
        return BATTLE_ENVIRONMENT_GRASS;
    if (MetatileBehavior_IsLongGrass(tileBehavior))
        return BATTLE_ENVIRONMENT_LONG_GRASS;
    if (MetatileBehavior_IsSandOrDeepSand(tileBehavior))
        return BATTLE_ENVIRONMENT_SAND;

    switch (gMapHeader.mapType)
    {
    case MAP_TYPE_TOWN:
    case MAP_TYPE_CITY:
    case MAP_TYPE_ROUTE:
        break;
    case MAP_TYPE_UNDERGROUND:
        if (MetatileBehavior_IsIndoorEncounter(tileBehavior))
            return BATTLE_ENVIRONMENT_BUILDING;
        if (MetatileBehavior_IsSurfableWaterOrUnderwater(tileBehavior))
            return BATTLE_ENVIRONMENT_POND;
        return BATTLE_ENVIRONMENT_CAVE;
    case MAP_TYPE_INDOOR:
    case MAP_TYPE_SECRET_BASE:
        return BATTLE_ENVIRONMENT_BUILDING;
    case MAP_TYPE_UNDERWATER:
        return BATTLE_ENVIRONMENT_UNDERWATER;
    case MAP_TYPE_OCEAN_ROUTE:
        if (MetatileBehavior_IsSurfableWaterOrUnderwater(tileBehavior))
            return BATTLE_ENVIRONMENT_WATER;
        return BATTLE_ENVIRONMENT_PLAIN;
    }
    if (MetatileBehavior_IsDeepOrOceanWater(tileBehavior))
        return BATTLE_ENVIRONMENT_WATER;
    if (MetatileBehavior_IsSurfableWaterOrUnderwater(tileBehavior))
        return BATTLE_ENVIRONMENT_POND;
    if (MetatileBehavior_IsMountain(tileBehavior))
        return BATTLE_ENVIRONMENT_MOUNTAIN;
    if (TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_SURFING))
    {
        // Is BRIDGE_TYPE_POND_*?
        if (MetatileBehavior_GetBridgeType(tileBehavior) != BRIDGE_TYPE_OCEAN)
            return BATTLE_ENVIRONMENT_POND;

        if (MetatileBehavior_IsBridgeOverWater(tileBehavior) == TRUE)
            return BATTLE_ENVIRONMENT_WATER;
    }
    if (gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(MAP_ROUTE113) && gSaveBlock1Ptr->location.mapNum == MAP_NUM(MAP_ROUTE113))
        return BATTLE_ENVIRONMENT_SAND;
    if (GetSavedWeather() == WEATHER_SANDSTORM)
        return BATTLE_ENVIRONMENT_SAND;

    return BATTLE_ENVIRONMENT_PLAIN;
}

static enum TransitionType GetBattleTransitionTypeByMap(void)
{
    u16 tileBehavior;
    s16 x, y;

    PlayerGetDestCoords(&x, &y);
    tileBehavior = MapGridGetMetatileBehaviorAt(x, y);

    if (GetFlashLevel())
        return TRANSITION_TYPE_FLASH;

    if (MetatileBehavior_IsSurfableWaterOrUnderwater(tileBehavior))
        return TRANSITION_TYPE_WATER;

    switch (gMapHeader.mapType)
    {
    case MAP_TYPE_UNDERGROUND:
        return TRANSITION_TYPE_CAVE;
    case MAP_TYPE_UNDERWATER:
        return TRANSITION_TYPE_WATER;
    default:
        return TRANSITION_TYPE_NORMAL;
    }
}

static u16 GetSumOfPlayerPartyLevel(u8 numMons)
{
    u8 sum = 0;
    int i;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        enum Species species = GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_SPECIES_OR_EGG);

        if (species != SPECIES_EGG && species != SPECIES_NONE && GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_HP) != 0)
        {
            sum += GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_LEVEL);
            if (--numMons == 0)
                break;
        }
    }
    return sum;
}

static u16 GetSumOfEnemyPartyLevel(u16 opponentId, u8 numMons)
{
    u8 i;
    u16 sum;
    u32 count = numMons;
    const struct TrainerMon *party;

    if (GetTrainerPartySizeFromId(opponentId) < count)
        count = GetTrainerPartySizeFromId(opponentId);

    sum = 0;

    party = GetTrainerPartyFromId(opponentId);
    for (i = 0; i < count && party != NULL; i++)
        sum += party[i].lvl;

    return sum;
}

enum BattleTransition GetWildBattleTransition(void)
{
    u8 transitionType = GetBattleTransitionTypeByMap();
    u16 enemyLevel = GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_LEVEL);
    u16 playerLevel = GetSumOfPlayerPartyLevel(1);

    if (enemyLevel < playerLevel)
    {
        if (CurrentBattlePyramidLocation() != PYRAMID_LOCATION_NONE)
            return B_TRANSITION_BLUR;
        else
            return sBattleTransitionTable_Wild[transitionType][0];
    }
    else
    {
        if (CurrentBattlePyramidLocation() != PYRAMID_LOCATION_NONE)
            return B_TRANSITION_GRID_SQUARES;
        else
            return sBattleTransitionTable_Wild[transitionType][1];
    }
}

enum BattleTransition GetTrainerBattleTransition(void)
{
    u8 minPartyCount = 1;
    u8 transitionType;
    u16 enemyLevel;
    u16 playerLevel;
    u32 trainerId = SanitizeTrainerId(TRAINER_BATTLE_PARAM.opponentA);
    enum TrainerClassID trainerClass = GetTrainerClassFromId(TRAINER_BATTLE_PARAM.opponentA);

    if (DoesTrainerHaveMugshot(trainerId))
        return B_TRANSITION_MUGSHOT;

    if (trainerClass == TRAINER_CLASS_TEAM_MAGMA
        || trainerClass == TRAINER_CLASS_MAGMA_LEADER
        || trainerClass == TRAINER_CLASS_MAGMA_ADMIN)
        return B_TRANSITION_MAGMA;

    if (trainerClass == TRAINER_CLASS_TEAM_AQUA
        || trainerClass == TRAINER_CLASS_AQUA_LEADER
        || trainerClass == TRAINER_CLASS_AQUA_ADMIN)
        return B_TRANSITION_AQUA;

    switch (GetTrainerBattleType(trainerId))
    {
    case TRAINER_BATTLE_TYPE_SINGLES:
        minPartyCount = 1;
        break;
    case TRAINER_BATTLE_TYPE_DOUBLES:
        minPartyCount = 2; // double battles always at least have 2 Pokémon.
        break;
    }

    transitionType = GetBattleTransitionTypeByMap();
    enemyLevel = GetSumOfEnemyPartyLevel(trainerId, minPartyCount);
    playerLevel = GetSumOfPlayerPartyLevel(minPartyCount);

    if (enemyLevel < playerLevel)
        return sBattleTransitionTable_Trainer[transitionType][0];
    else
        return sBattleTransitionTable_Trainer[transitionType][1];
}

#define RANDOM_TRANSITION(table) (table[Random() % ARRAY_COUNT(table)])
enum BattleTransition GetSpecialBattleTransition(enum BattleTransitionGroup id)
{
#if FREE_BATTLE_FRONTIER == FALSE
    u16 var;
#endif //FREE_BATTLE_FRONTIER
    u16 enemyLevel = GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_LEVEL);
    u16 playerLevel = GetSumOfPlayerPartyLevel(1);

    if (enemyLevel < playerLevel)
    {
        switch (id)
        {
        case B_TRANSITION_GROUP_TRAINER_HILL:
        case B_TRANSITION_GROUP_SECRET_BASE:
        case B_TRANSITION_GROUP_E_READER:
            return B_TRANSITION_POKEBALLS_TRAIL;
        case B_TRANSITION_GROUP_B_PYRAMID:
            return RANDOM_TRANSITION(sBattleTransitionTable_BattlePyramid);
        case B_TRANSITION_GROUP_B_DOME:
            return RANDOM_TRANSITION(sBattleTransitionTable_BattleDome);
        default:
            break;
        }

        if (VarGet(VAR_FRONTIER_BATTLE_MODE) != FRONTIER_MODE_LINK_MULTIS)
            return RANDOM_TRANSITION(sBattleTransitionTable_BattleFrontier);
    }
    else
    {
        switch (id)
        {
        case B_TRANSITION_GROUP_TRAINER_HILL:
        case B_TRANSITION_GROUP_SECRET_BASE:
        case B_TRANSITION_GROUP_E_READER:
            return B_TRANSITION_BIG_POKEBALL;
        case B_TRANSITION_GROUP_B_PYRAMID:
            return RANDOM_TRANSITION(sBattleTransitionTable_BattlePyramid);
        case B_TRANSITION_GROUP_B_DOME:
            return RANDOM_TRANSITION(sBattleTransitionTable_BattleDome);
        default:
            break;
        }

        if (VarGet(VAR_FRONTIER_BATTLE_MODE) != FRONTIER_MODE_LINK_MULTIS)
            return RANDOM_TRANSITION(sBattleTransitionTable_BattleFrontier);
    }

#if FREE_BATTLE_FRONTIER == FALSE
    var = gSaveBlock2Ptr->frontier.trainerIds[gSaveBlock2Ptr->frontier.curChallengeBattleNum * 2 + 0]
        + gSaveBlock2Ptr->frontier.trainerIds[gSaveBlock2Ptr->frontier.curChallengeBattleNum * 2 + 1];

    return sBattleTransitionTable_BattleFrontier[var % ARRAY_COUNT(sBattleTransitionTable_BattleFrontier)];
#else
    return RANDOM_TRANSITION(sBattleTransitionTable_BattleFrontier);
#endif //FREE_BATTLE_FRONTIER
}

void ChooseStarter(void)
{
    SetMainCallback2(CB2_ChooseStarter);
    gMain.savedCallback = CB2_GiveStarter;
}

static void CB2_GiveStarter(void)
{
    // CUSTOM - Disabled starter mon selection in order to use custom all 9 gen mon selection script

    // u16 starterMon;

    // *GetVarPointer(VAR_STARTER_MON) = gSpecialVar_Result;
    // starterMon = GetStarterPokemon(gSpecialVar_Result);
    // ScriptGiveMon(starterMon, 5, ITEM_NONE);
    // ResetTasks();
    PlayBattleBGM();
    SetMainCallback2(CB2_StartFirstBattle);
    BattleTransition_Start(B_TRANSITION_BLUR);
}

static void CB2_StartFirstBattle(void)
{
    UpdatePaletteFade();
    RunTasks();

    if (IsBattleTransitionDone() == TRUE)
    {
        gBattleTypeFlags = BATTLE_TYPE_FIRST_BATTLE;
        gMain.savedCallback = CB2_EndFirstBattle;
        FreeAllWindowBuffers();
        SetMainCallback2(CB2_InitBattle);
        RestartWildEncounterImmunitySteps();
        ClearPoisonStepCounter();
        IncrementGameStat(GAME_STAT_TOTAL_BATTLES);
        IncrementGameStat(GAME_STAT_WILD_BATTLES);
        IncrementDailyWildBattles();
    }
}

static void CB2_EndFirstBattle(void)
{
    Overworld_ClearSavedMusic();
    DowngradeBadPoison();
    SetMainCallback2(CB2_ReturnToFieldContinueScriptPlayMapMusic);
}

static u16 GetTrainerAFlag(void)
{
    return TRAINER_FLAGS_START + TRAINER_BATTLE_PARAM.opponentA;
}

static u16 GetTrainerBFlag(void)
{
    return TRAINER_FLAGS_START + TRAINER_BATTLE_PARAM.opponentB;
}

static bool32 IsPlayerDefeated(u32 battleOutcome)
{
    switch (battleOutcome)
    {
    case B_OUTCOME_LOST:
    case B_OUTCOME_DREW:
    case B_OUTCOME_FORFEITED:
        return TRUE;
    case B_OUTCOME_WON:
    case B_OUTCOME_RAN:
    case B_OUTCOME_PLAYER_TELEPORTED:
    case B_OUTCOME_MON_FLED:
    case B_OUTCOME_CAUGHT:
        return FALSE;
    default:
        return FALSE;
    }
}

void InitTrainerBattleParameter(void)
{
    memset(gTrainerBattleParameter.data, 0, sizeof(TrainerBattleParameter));
    sTrainerBattleEndScript = NULL;
}

void TrainerBattleLoadArgs(const u8 *data)
{
    InitTrainerBattleParameter();
    memcpy(gTrainerBattleParameter.data, data, sizeof(TrainerBattleParameter));
    sTrainerBattleEndScript = (u8*)data + sizeof(TrainerBattleParameter);
}

// loads trainer A parameter to trainer B. Used for second trainer in trainer_see.c
void TrainerBattleLoadArgsSecondTrainer(const u8 *data)
{
    TrainerBattleParameter *temp = (TrainerBattleParameter*)data;

    TRAINER_BATTLE_PARAM.playMusicB = temp->params.playMusicA;
    TRAINER_BATTLE_PARAM.objEventLocalIdB = temp->params.objEventLocalIdA;
    TRAINER_BATTLE_PARAM.opponentB = temp->params.opponentA;
    TRAINER_BATTLE_PARAM.introTextB = temp->params.introTextA;
    TRAINER_BATTLE_PARAM.defeatTextB = temp->params.defeatTextA;
    TRAINER_BATTLE_PARAM.battleScriptRetAddrB = temp->params.battleScriptRetAddrA;
}

void SetMapVarsToTrainerA(void)
{
    if (TRAINER_BATTLE_PARAM.objEventLocalIdA != LOCALID_NONE)
    {
        gSpecialVar_LastTalked = TRAINER_BATTLE_PARAM.objEventLocalIdA;
        gSelectedObjectEvent = GetObjectEventIdByLocalIdAndMap(TRAINER_BATTLE_PARAM.objEventLocalIdA, gSaveBlock1Ptr->location.mapNum, gSaveBlock1Ptr->location.mapGroup);
    }
}

void SetMapVarsToTrainerB(void)
{
    if (TRAINER_BATTLE_PARAM.objEventLocalIdB != LOCALID_NONE)
    {
        gSpecialVar_LastTalked = TRAINER_BATTLE_PARAM.objEventLocalIdB;
        gSelectedObjectEvent = GetObjectEventIdByLocalIdAndMap(TRAINER_BATTLE_PARAM.objEventLocalIdB, gSaveBlock1Ptr->location.mapNum, gSaveBlock1Ptr->location.mapGroup);
    }
}

#define PUSH(script) ScriptStackPush(scrStack, script);

#define PUSH_IF_SET(script, cond) \
if (cond)           \
{                   \
    PUSH(script)    \
}

#define PUSH_IF_ELSE(script, alt, cond) \
if (cond)           \
{                   \
    PUSH(script)    \
}                   \
else                \
{                   \
    PUSH(alt)       \
}                   \

static void BattleSetup_ConfigureApproachingFacilityTrainerBattle(TrainerBattleParameter *battleParams, struct ScriptStack *scrStack)
{
    SetMapVarsToTrainerA();

    PUSH(EventSnippet_StartTrainerApproach)
    PUSH(EventSnippet_PlayTrainerEncounterMusic)
    PUSH(EventSnippet_TrainerApproach)
    PUSH(EventSnippet_ShowTrainerIntroMsg)

    if (gNoOfApproachingTrainers > 1) 
    {
        SetMapVarsToTrainerB();

        PUSH(EventSnippet_PrepareSecondTrainerApproach)
        PUSH(EventSnippet_PlayTrainerEncounterMusic)
        PUSH(EventSnippet_TrainerApproach)
        PUSH(EventSnippet_ShowTrainerIntroMsg)
    }

    PUSH(EventSnippet_DoTrainerBattle)
    PUSH(EventSnippet_EndTrainerBattle)
}

static void BattleSetup_ConfigureFacilityTrainerBattle(TrainerBattleParameter *battleParams, struct ScriptStack *scrStack)
{
    SetMapVarsToTrainerA();

    PUSH(EventSnippet_Lock)
    PUSH(EventSnippet_FacePlayer)
    PUSH(EventSnippet_RevealTrainer)

    if (GetTrainerFlag()) {
        PUSH(EventSnippet_GotoPostBattleScript)
        return;
    }

    PUSH(EventSnippet_PlayTrainerEncounterMusic)
    PUSH(EventSnippet_SetTrainerFacingDirection)
    PUSH(EventSnippet_ShowTrainerIntroMsg)
    PUSH(EventSnippet_DoTrainerBattle)
    PUSH(EventSnippet_EndTrainerBattle)

    return;
}

static void BattleSetup_ConfigureApproachingTrainerBattle(TrainerBattleParameter *battleParams, struct ScriptStack *scrStack)
{
    SetMapVarsToTrainerA();

    PUSH       (EventSnippet_StartTrainerApproach)
    PUSH_IF_SET(EventSnippet_PlayTrainerEncounterMusic, battleParams->params.playMusicA)
    PUSH       (EventSnippet_TrainerApproach)
    PUSH_IF_SET(EventSnippet_ShowTrainerIntroMsg, battleParams->params.introTextA)

    if (gNoOfApproachingTrainers > 1) 
    {
        SetMapVarsToTrainerB();

        PUSH       (EventSnippet_PrepareSecondTrainerApproach)
        PUSH_IF_SET(EventSnippet_PlayTrainerEncounterMusic, battleParams->params.playMusicB)
        PUSH       (EventSnippet_TrainerApproach)
        PUSH_IF_SET(EventSnippet_ShowTrainerIntroMsg, battleParams->params.introTextB)
    }

    PUSH(EventSnippet_DoTrainerBattle)
    PUSH(EventSnippet_EndTrainerBattle)
    return;
}

static void BattleSetup_ConfigureTrainerBattle(TrainerBattleParameter *battleParams, struct ScriptStack *scrStack)
{
    SetMapVarsToTrainerA();

    PUSH       (EventSnippet_Lock)
    PUSH_IF_SET(EventSnippet_FacePlayer, battleParams->params.facePlayer)
    PUSH       (EventSnippet_RevealTrainer)

    bool32 isTrainerDefeated = !battleParams->params.skipFlagCheck && GetTrainerFlag();

    if (isTrainerDefeated) {
        PUSH(EventSnippet_GotoPostBattleScript)
        return;
    }

    if (battleParams->params.isDoubleBattle && !HasEnoughMonsForDoubleBattle2())
    {
        PUSH(EventSnippet_NotEnoughMonsForDoubleBattle)
        return;
    }

    if (battleParams->params.opponentA != 0 && battleParams->params.opponentB != 0)
    {
        gNoOfApproachingTrainers = 2;
    }

    PUSH_IF_SET(EventSnippet_PlayTrainerEncounterMusic, battleParams->params.playMusicA)
    PUSH_IF_SET(EventSnippet_SetTrainerFacingDirection, battleParams->params.facePlayer);
    PUSH_IF_SET(EventSnippet_ShowTrainerIntroMsg, battleParams->params.introTextA)
    PUSH(EventSnippet_DoTrainerBattle)
    PUSH_IF_ELSE(EventSnippet_GotoPostBattleScript, EventSnippet_EndTrainerBattle, battleParams->params.continueScript)

    return;
}

void ConfigureTrainerBattle(struct ScriptContext *ctx)
{
    InitTrainerBattleParameter();

    struct ScriptStack trainerBattleScriptStack;
    InitScriptStack(&trainerBattleScriptStack);

    TrainerBattleParameter *battleParams = (TrainerBattleParameter*)(ctx->scriptPtr);
    TrainerBattleLoadArgs(battleParams->data);

    BattleSetup_ConfigureTrainerBattle(&gTrainerBattleParameter, &trainerBattleScriptStack);
    ScriptContext_SetupContextFromStack(&trainerBattleScriptStack, ctx);
    ScriptContext_Enable();
}

void ConfigureApproachingTrainerBattle(struct ApproachingTrainer *approachingTrainer)
{
    InitTrainerBattleParameter();

    struct ScriptStack trainerBattleScriptStack;
    InitScriptStack(&trainerBattleScriptStack);

    TrainerBattleParameter *battleParams = (TrainerBattleParameter*)(approachingTrainer[0].trainerScriptPtr + TRAINERBATTLE_OPCODE_OFFSET);
    TrainerBattleLoadArgs(battleParams->data);

    if (gNoOfApproachingTrainers > 1)
    {
        battleParams = (TrainerBattleParameter*)(approachingTrainer[1].trainerScriptPtr + TRAINERBATTLE_OPCODE_OFFSET);
        TrainerBattleLoadArgsSecondTrainer(battleParams->data);
    }

    BattleSetup_ConfigureApproachingTrainerBattle(&gTrainerBattleParameter, &trainerBattleScriptStack);
    ScriptContext_SetupGlobalContextFromStack(&trainerBattleScriptStack);
    ScriptContext_Enable();
}

static void SetFacilityOpponent(u8 facility, u8 localId, bool8 isTrainerA)
{
    u16 trainerId = TRAINER_NONE;

    switch (facility) {
        case FACILITY_BATTLE_PYRAMID:
            trainerId = LocalIdToPyramidTrainerId(localId);
            break;
        case FACILITY_BATTLE_TRAINER_HILL:
            trainerId = LocalIdToHillTrainerId(localId);
            break;
        default:
            errorf("Invalid facility: %d", facility);
    } 

    if (isTrainerA) {
        TRAINER_BATTLE_PARAM.opponentA = trainerId;
        TRAINER_BATTLE_PARAM.objEventLocalIdA = localId;
    } else {
        TRAINER_BATTLE_PARAM.opponentB = trainerId;
        TRAINER_BATTLE_PARAM.objEventLocalIdB = localId;
    }
   
}

void ConfigureFacilityTrainerBattle(u8 facility, const u8* scriptEndPtr)
{
    InitTrainerBattleParameter();

    struct ScriptStack trainerBattleScriptStack;
    InitScriptStack(&trainerBattleScriptStack);

    SetFacilityOpponent(facility, gSpecialVar_LastTalked, TRUE);

    sTrainerBattleEndScript = (u8*)scriptEndPtr;

    BattleSetup_ConfigureFacilityTrainerBattle(&gTrainerBattleParameter, &trainerBattleScriptStack);
    ScriptContext_SetupGlobalContextFromStack(&trainerBattleScriptStack);
    ScriptContext_Enable();
}

void ConfigureApproachingFacilityTrainerBattle(struct ApproachingTrainer *approachingTrainer)
{
    u8 facility, localId;
    const u8 *scriptEndPtr;

    InitTrainerBattleParameter();

    struct ScriptStack trainerBattleScriptStack;
    InitScriptStack(&trainerBattleScriptStack);

    facility = *(approachingTrainer[0].trainerScriptPtr + FACILITYBATTLE_OPCODE_OFFSET);
    localId = gObjectEvents[approachingTrainer[0].objectEventId].localId;
    scriptEndPtr = approachingTrainer[0].trainerScriptPtr + FACILITYBATTLE_OPCODE_OFFSET + 1; 

    SetFacilityOpponent(facility, localId, TRUE);
    
    if (gNoOfApproachingTrainers > 1)
    {
        gApproachingTrainerId++;
        facility = *(approachingTrainer[1].trainerScriptPtr + FACILITYBATTLE_OPCODE_OFFSET);
        localId = gObjectEvents[approachingTrainer[1].objectEventId].localId;
        scriptEndPtr = approachingTrainer[1].trainerScriptPtr + FACILITYBATTLE_OPCODE_OFFSET + 1; 

        SetFacilityOpponent(facility, localId, FALSE);
    }

    sTrainerBattleEndScript = (u8*)scriptEndPtr;
    gApproachingTrainerId = 0;

    BattleSetup_ConfigureApproachingFacilityTrainerBattle(&gTrainerBattleParameter, &trainerBattleScriptStack);
    ScriptContext_SetupGlobalContextFromStack(&trainerBattleScriptStack);
    ScriptContext_Enable();
}

bool32 GetTrainerFlagFromScriptPointer(const u8 *data)
{
    TrainerBattleParameter *temp = (TrainerBattleParameter*)(data + TRAINERBATTLE_OPCODE_OFFSET);
    return FlagGet(TRAINER_FLAGS_START + temp->params.opponentA);
}

u16 GetTrainerFlagFromScript(const u8 *script)
{
    // The trainer flag is located 3 bytes (command + flags + localIdA) from the script pointer, assuming the trainerbattle command is first in the script.
    // Because scripts are unaligned, and because the ARM processor requires shorts to be 16-bit aligned, this function needs to perform explicit bitwise operations to get the correct flag.
    u16 trainerFlag = TRAINER_NONE;
    struct ScriptContext *ctx = AllocZeroed(sizeof(struct ScriptContext));
    if (script[0] == SCR_OP_TRAINERBATTLE)
    {
        ctx->scriptPtr = script + 3;
        trainerFlag = ScriptPeekHalfword(ctx);
    }
    Free(ctx);
    return trainerFlag;
}

// Set trainer's movement type so they stop and remain facing that direction
// Note: Only for trainers who are spoken to directly
//       For trainers who spot the player this is handled by PlayerFaceApproachingTrainer
void SetTrainerFacingDirection(void)
{
    assertf(gSelectedObjectEvent != gPlayerAvatar.objectEventId, "trainer script that needs to be used from an object event was called from player")
    {
        return;
    }
    struct ObjectEvent *objectEvent = &gObjectEvents[gSelectedObjectEvent];
    SetTrainerMovementType(objectEvent, GetTrainerFacingDirectionMovementType(objectEvent->facingDirection));
}

u8 GetRivalBattleFlags(void)
{
    return TRAINER_BATTLE_PARAM.rivalBattleFlags;
}

bool8 GetTrainerFlag(void)
{
    if (CurrentBattlePyramidLocation() != PYRAMID_LOCATION_NONE)
        return GetBattlePyramidTrainerFlag(gSelectedObjectEvent);
    else if (InTrainerHill())
        return GetHillTrainerFlag(gSelectedObjectEvent);
    else
        return FlagGet(GetTrainerAFlag());
}

static void SetBattledTrainersFlags(void)
{
    if (TRAINER_BATTLE_PARAM.opponentB != 0)
        FlagSet(GetTrainerBFlag());
    FlagSet(GetTrainerAFlag());
}

static void UNUSED SetBattledTrainerFlag(void)
{
    FlagSet(GetTrainerAFlag());
}

bool8 HasTrainerBeenFought(u16 trainerId)
{
    return FlagGet(TRAINER_FLAGS_START + trainerId);
}

void SetTrainerFlag(u16 trainerId)
{
    FlagSet(TRAINER_FLAGS_START + trainerId);
}

void ClearTrainerFlag(u16 trainerId)
{
    FlagClear(TRAINER_FLAGS_START + trainerId);
}

void BattleSetup_StartTrainerBattle(void)
{
    if (gNoOfApproachingTrainers == 2)
    {
        if (FollowerNPCIsBattlePartner())
            gBattleTypeFlags = (BATTLE_TYPE_MULTI | BATTLE_TYPE_DOUBLE | BATTLE_TYPE_INGAME_PARTNER | BATTLE_TYPE_TWO_OPPONENTS | BATTLE_TYPE_TRAINER);
        else
            gBattleTypeFlags = (BATTLE_TYPE_DOUBLE | BATTLE_TYPE_TWO_OPPONENTS | BATTLE_TYPE_TRAINER);
    }
    else
    {
        if (FollowerNPCIsBattlePartner())
        {
            gBattleTypeFlags = (BATTLE_TYPE_MULTI | BATTLE_TYPE_INGAME_PARTNER | BATTLE_TYPE_DOUBLE | BATTLE_TYPE_TRAINER);
            TRAINER_BATTLE_PARAM.opponentB = 0xFFFF;
        }
        else
        {
            gBattleTypeFlags = (BATTLE_TYPE_TRAINER);
        }
    }

    if (TRAINER_BATTLE_PARAM.earlyRival && GetRivalBattleFlags() & RIVAL_BATTLE_TUTORIAL)
        gBattleTypeFlags |= BATTLE_TYPE_FIRST_BATTLE;

    if (CurrentBattlePyramidLocation() != PYRAMID_LOCATION_NONE)
    {
        VarSet(VAR_TEMP_PLAYING_PYRAMID_MUSIC, 0);
        gBattleTypeFlags |= BATTLE_TYPE_PYRAMID;

        if (gNoOfApproachingTrainers == 2)
        {
            FillFrontierTrainersParties(1);
            ZeroMonData(&gParties[B_TRAINER_OPPONENT_A][1]);
            ZeroMonData(&gParties[B_TRAINER_OPPONENT_A][2]);
            ZeroMonData(&gParties[B_TRAINER_OPPONENT_B][1]);
            ZeroMonData(&gParties[B_TRAINER_OPPONENT_B][2]);
        }
        else
        {
            FillFrontierTrainerParty(1);
            ZeroMonData(&gParties[B_TRAINER_OPPONENT_A][1]);
            ZeroMonData(&gParties[B_TRAINER_OPPONENT_A][2]);
        }

        MarkApproachingPyramidTrainersAsBattled();
    }
    else if (InTrainerHillChallenge())
    {
        gBattleTypeFlags |= BATTLE_TYPE_TRAINER_HILL;

        if (gNoOfApproachingTrainers == 2)
            FillHillTrainersParties();
        else
            FillHillTrainerParty();

        SetHillTrainerFlag();
    }
    else if (GetTrainerBattleType(TRAINER_BATTLE_PARAM.opponentA) == TRAINER_BATTLE_TYPE_DOUBLES)
    {
        gBattleTypeFlags |= BATTLE_TYPE_DOUBLE;
    }

    sNoOfPossibleTrainerRetScripts = gNoOfApproachingTrainers;
    gNoOfApproachingTrainers = 0;
    sShouldCheckTrainerBScript = FALSE;
    gWhichTrainerToFaceAfterBattle = 0;
    gMain.savedCallback = CB2_EndTrainerBattle;

    if (CurrentBattlePyramidLocation() != PYRAMID_LOCATION_NONE || InTrainerHillChallenge())
        DoBattlePyramidTrainerHillBattle();
    else
        DoTrainerBattle();

    ScriptContext_Stop();
}

static void CB2_EndDebugBattle(void)
{
    if (gBattleTypeFlags & BATTLE_TYPE_INGAME_PARTNER)
    {
        for (u32 i = 0; i < 3; i++)
        {
            u16 monId = gSaveBlock2Ptr->selectedPartyMons[i] - 1;
            if (monId < PARTY_SIZE)
                SavePlayerPartyMon(gSaveBlock2Ptr->selectedPartyMons[i] - 1, &gParties[B_TRAINER_PLAYER][i]);
        }
        LoadPlayerParty();
    }
    SetMainCallback2(CB2_EndTrainerBattle);
}

void BattleSetup_StartTrainerBattle_Debug(void)
{
    sNoOfPossibleTrainerRetScripts = gNoOfApproachingTrainers;
    gNoOfApproachingTrainers = 0;
    sShouldCheckTrainerBScript = FALSE;
    gWhichTrainerToFaceAfterBattle = 0;
    gMain.savedCallback = CB2_EndDebugBattle;

    CreateBattleStartTask_Debug(GetWildBattleTransition(), 0);

    ScriptContext_Stop();
}

static void SaveChangesToPlayerParty(void)
{
    u8 i = 0, j = 0;
    u8 participatedPokemon = VarGet(B_VAR_SKY_BATTLE);
    for (i = 0; i < PARTY_SIZE; i++)
    {
        if ((participatedPokemon >> i & 1) == 1)
        {
            SavePlayerPartyMon(i, &gParties[B_TRAINER_PLAYER][j]);
            j++;
        }
    }
}

static void HandleBattleVariantEndParty(void)
{
    if (B_FLAG_SKY_BATTLE == 0 || !FlagGet(B_FLAG_SKY_BATTLE))
        return;
    SaveChangesToPlayerParty();
    LoadPlayerParty();
    FlagClear(B_FLAG_SKY_BATTLE);
}

static void CB2_EndTrainerBattle(void)
{
    // GAME_STAT_TRAINER_BATTLES is incremented at battle start, so it's current here regardless of outcome.
    Achievement_CheckTrainerBattleMilestones();

    HandleBattleVariantEndParty();

    if (gSaveBlock1Ptr->nuzlockeModeEnabled || gSaveBlock1Ptr->autosaveModeEnabled)
    {
        gDoAutosaveAfterBattle = TRUE;
    }

    gIsDebugBattle = FALSE;
    // The TRAINER_EMPORIUM redirect is only needed while the opponent party is built. Disarm it so a
    // white-out can't leave every trainer pointed at the runtime Emporium struct.
    gEmporiumBattleActive = FALSE;
    if (FollowerNPCIsBattlePartner())
    {
        RestorePartyAfterFollowerNPCBattle();
        if (FNPC_FLAG_HEAL_AFTER_FOLLOWER_BATTLE != 0
         && (FNPC_FLAG_HEAL_AFTER_FOLLOWER_BATTLE == FNPC_ALWAYS
         || FlagGet(FNPC_FLAG_HEAL_AFTER_FOLLOWER_BATTLE)))
            HealPlayerParty();
    }

    if (TRAINER_BATTLE_PARAM.earlyRival)
    {
        if (IsPlayerDefeated(gBattleOutcome) == TRUE)
        {
            gSpecialVar_Result = TRUE;
            if (GetRivalBattleFlags() & RIVAL_BATTLE_HEAL_AFTER)
            {
                HealPlayerParty();
            }
            else
            {
                SetMainCallback2(CB2_WhiteOut);
                return;
            }
        }
        else
        {
            gSpecialVar_Result = FALSE;
        }
        DowngradeBadPoison();
        SetMainCallback2(CB2_ReturnToFieldContinueScriptPlayMapMusic);
        SetBattledTrainerFlag();
    }
    else if (TRAINER_BATTLE_PARAM.opponentA == TRAINER_SECRET_BASE)
    {
        DowngradeBadPoison();
        SetMainCallback2(CB2_ReturnToFieldContinueScriptPlayMapMusic);
    }
    else if (IsPlayerDefeated(gBattleOutcome))
    {
        if (CurrentBattlePyramidLocation() != PYRAMID_LOCATION_NONE || InTrainerHillChallenge() || FlagGet(B_FLAG_NO_WHITEOUT))
            SetMainCallback2(CB2_ReturnToFieldContinueScriptPlayMapMusic);
        else
            SetMainCallback2(CB2_WhiteOut);
    }
    else
    {
        SetMainCallback2(CB2_ReturnToFieldContinueScriptPlayMapMusic);
        DowngradeBadPoison();
        // BOOST_POST_BATTLE_HEAL: win branch only; the forfeit/defeat/early-rival-loss branches above
        // are unrewarded or fully healed elsewhere.
        AchievementBoost_ApplyPostBattleHeal();
        if (CurrentBattlePyramidLocation() == PYRAMID_LOCATION_NONE && !InTrainerHillChallenge())
        {
            SetBattledTrainersFlags();
        }
    }
}

void ShowTrainerIntroSpeech(void)
{
    if (CurrentBattlePyramidLocation() != PYRAMID_LOCATION_NONE)
    {
        if (gNoOfApproachingTrainers == 0 || gNoOfApproachingTrainers == 1)
            CopyPyramidTrainerSpeechBefore(LocalIdToPyramidTrainerId(gSpecialVar_LastTalked));
        else
            CopyPyramidTrainerSpeechBefore(LocalIdToPyramidTrainerId(gObjectEvents[gApproachingTrainers[gApproachingTrainerId].objectEventId].localId));

        ShowFieldMessageFromBuffer();
    }
    else if (InTrainerHillChallenge())
    {
        if (gNoOfApproachingTrainers == 0 || gNoOfApproachingTrainers == 1)
            CopyTrainerHillTrainerText(TRAINER_HILL_TEXT_INTRO, LocalIdToHillTrainerId(gSpecialVar_LastTalked));
        else
            CopyTrainerHillTrainerText(TRAINER_HILL_TEXT_INTRO, LocalIdToHillTrainerId(gObjectEvents[gApproachingTrainers[gApproachingTrainerId].objectEventId].localId));

        ShowFieldMessageFromBuffer();
    }
    else
    {
        ShowFieldMessage(GetIntroSpeechOfApproachingTrainer());
    }
}

const u8 *BattleSetup_GetScriptAddrAfterBattle(void)
{
    if (sTrainerBattleEndScript != NULL)
        return sTrainerBattleEndScript;
    else
        return EventScript_TestSignpostMsg;
}

const u8 *BattleSetup_GetTrainerPostBattleScript(void)
{
    if (sShouldCheckTrainerBScript)
    {
        sShouldCheckTrainerBScript = FALSE;
        if (TRAINER_BATTLE_PARAM.battleScriptRetAddrB != NULL)
        {
            gWhichTrainerToFaceAfterBattle = 1;
            return TRAINER_BATTLE_PARAM.battleScriptRetAddrB;
        }
    }
    else
    {
        if (TRAINER_BATTLE_PARAM.battleScriptRetAddrA != NULL)
        {
            gWhichTrainerToFaceAfterBattle = 0;
            return TRAINER_BATTLE_PARAM.battleScriptRetAddrA;
        }
    }

    return EventScript_TryGetTrainerScript;
}

void ShowTrainerCantBattleSpeech(void)
{
    ShowFieldMessage(GetTrainerCantBattleSpeech());
}

void PlayTrainerEncounterMusic(void)
{
    u16 trainerId;
    u16 music;

    if (gApproachingTrainerId == 0)
        trainerId = TRAINER_BATTLE_PARAM.opponentA;
    else
        trainerId = TRAINER_BATTLE_PARAM.opponentB;

    switch (GetTrainerEncounterMusicId(trainerId))
    {
    case TRAINER_ENCOUNTER_MUSIC_MALE:
        music = MUS_ENCOUNTER_MALE;
        break;
    case TRAINER_ENCOUNTER_MUSIC_FEMALE:
        music = MUS_ENCOUNTER_FEMALE;
        break;
    case TRAINER_ENCOUNTER_MUSIC_GIRL:
        music = MUS_ENCOUNTER_GIRL;
        break;
    case TRAINER_ENCOUNTER_MUSIC_INTENSE:
        music = MUS_ENCOUNTER_INTENSE;
        break;
    case TRAINER_ENCOUNTER_MUSIC_COOL:
        music = MUS_ENCOUNTER_COOL;
        break;
    case TRAINER_ENCOUNTER_MUSIC_AQUA:
        music = MUS_ENCOUNTER_AQUA;
        break;
    case TRAINER_ENCOUNTER_MUSIC_MAGMA:
        music = MUS_ENCOUNTER_MAGMA;
        break;
    case TRAINER_ENCOUNTER_MUSIC_SWIMMER:
        music = MUS_ENCOUNTER_SWIMMER;
        break;
    case TRAINER_ENCOUNTER_MUSIC_TWINS:
        music = MUS_ENCOUNTER_TWINS;
        break;
    case TRAINER_ENCOUNTER_MUSIC_ELITE_FOUR:
        music = MUS_ENCOUNTER_ELITE_FOUR;
        break;
    case TRAINER_ENCOUNTER_MUSIC_HIKER:
        music = MUS_ENCOUNTER_HIKER;
        break;
    case TRAINER_ENCOUNTER_MUSIC_INTERVIEWER:
        music = MUS_ENCOUNTER_INTERVIEWER;
        break;
    case TRAINER_ENCOUNTER_MUSIC_RICH:
        music = MUS_ENCOUNTER_RICH;
        break;
    default:
        music = MUS_ENCOUNTER_SUSPICIOUS;
    }
    PlayNewMapMusic(music);

}

static const u8 *ReturnEmptyStringIfNull(const u8 *string)
{
    if (string == NULL)
        return gText_EmptyString2;
    else
        return string;
}

static const u8 *GetIntroSpeechOfApproachingTrainer(void)
{
    if (gApproachingTrainerId == 0)
    {
        if (OW_NAME_BOX_NPC_TRAINER)
            gSpeakerName = GetTrainerNameFromId(TRAINER_BATTLE_PARAM.opponentA);

        return ReturnEmptyStringIfNull(TRAINER_BATTLE_PARAM.introTextA);
    }
    else
    {
        if (OW_NAME_BOX_NPC_TRAINER)
            gSpeakerName = GetTrainerNameFromId(TRAINER_BATTLE_PARAM.opponentB);

        return ReturnEmptyStringIfNull(TRAINER_BATTLE_PARAM.introTextB);
    }
}

const u8 *GetTrainerALoseText(void)
{
    const u8 *string = TRAINER_BATTLE_PARAM.defeatTextA;

    StringExpandPlaceholders(gStringVar4, ReturnEmptyStringIfNull(string));
    return gStringVar4;
}

const u8 *GetTrainerBLoseText(void)
{
    StringExpandPlaceholders(gStringVar4, ReturnEmptyStringIfNull(TRAINER_BATTLE_PARAM.defeatTextB));
    return gStringVar4;
}

const u8 *GetTrainerWonSpeech(void)
{
    StringExpandPlaceholders(gStringVar4, ReturnEmptyStringIfNull(TRAINER_BATTLE_PARAM.victoryText));
    return gStringVar4;
}

static const u8 *GetTrainerCantBattleSpeech(void)
{
    return ReturnEmptyStringIfNull(TRAINER_BATTLE_PARAM.cannotBattleText);
}

void ShouldTryGetTrainerScript(void)
{
    if (sNoOfPossibleTrainerRetScripts > 1)
    {
        sNoOfPossibleTrainerRetScripts = 0;
        sShouldCheckTrainerBScript = TRUE;
        gSpecialVar_Result = TRUE;
    }
    else
    {
        sShouldCheckTrainerBScript = FALSE;
        gSpecialVar_Result = FALSE;
    }
}

void SetMultiTrainerBattle(struct ScriptContext *ctx)
{
    InitTrainerBattleParameter();

    TRAINER_BATTLE_PARAM.opponentA = ScriptReadHalfword(ctx);
    TRAINER_BATTLE_PARAM.defeatTextA = (u8*)ScriptReadWord(ctx);
    TRAINER_BATTLE_PARAM.opponentB = ScriptReadHalfword(ctx);
    TRAINER_BATTLE_PARAM.defeatTextB = (u8*)ScriptReadWord(ctx);
    gPartnerTrainerId = TRAINER_PARTNER(ScriptReadHalfword(ctx));

    // The multi_do macro calls AreMultiPartiesFullTeams() before the battle type is built, and it reads
    // BATTLE_TYPE_TRAINER. Seed it from the opponents above so the script-time answer matches the
    // in-battle one; BattleSetup_StartMultiBattle assigns gBattleTypeFlags outright afterwards.
    if (TRAINER_BATTLE_PARAM.opponentA == TRAINER_NONE)
        gBattleTypeFlags &= ~BATTLE_TYPE_TRAINER;
    else
        gBattleTypeFlags |= BATTLE_TYPE_TRAINER;
};

static void CreateNPCTrainerParty(struct Pokemon *party, u16 trainerNum)
{
    if (!GetTrainerStructFromId(trainerNum)->overrideTrainer)
    {
        CreateNPCTrainerPartyFromTrainer(party, GetTrainerStructFromId(trainerNum));
        return;
    }

    struct Trainer tempTrainer;
    memcpy(&tempTrainer, GetTrainerStructFromId(trainerNum), sizeof(struct Trainer));
    const struct Trainer *origTrainer = GetTrainerStructFromId(tempTrainer.overrideTrainer);

    tempTrainer.party = origTrainer->party;

    tempTrainer.poolSize = origTrainer->poolSize;
    if (tempTrainer.partySize == 0)
        tempTrainer.partySize = origTrainer->partySize;
    CreateNPCTrainerPartyFromTrainer(party, (const struct Trainer *)(&tempTrainer));
}

void CreateTrainerPartyForPlayer(void)
{
    Script_RequestEffects(SCREFF_V1);

    gPartnerTrainerId = gSpecialVar_0x8004;
    CreateNPCTrainerPartyFromTrainer(gParties[B_TRAINER_PLAYER], GetTrainerStructFromId(gSpecialVar_0x8004));
}
