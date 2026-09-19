#include "global.h"
#include "gba/flash_internal.h"
#include "agb_flash.h"
#include "event_data.h"
#include "load_save.h"
#include "random.h"
#include "save.h"
#include "achievements.h"
#include "achievement_popup.h"
#include "money.h"
#include "overworld.h"
#include "pokedex.h"
#include "pokemon.h"
#include "battle.h"
#include "battle_setup.h"
#include "data.h"
#include "move.h"
#include "caps.h"
#include "draft_mode.h"
#include "limited_party.h"
#include "mono_gen.h"
#include "mono_type.h"
#include "rotation_mode.h"
#include "recruits_mode.h"
#include "badge_mart.h"
#include "pokemon_storage_system.h"
#include "constants/difficulty.h"
#include "item.h"
#include "wild_encounter.h"
#include "battle_emporium.h"
#include "constants/battle_emporium.h"
#include "constants/flags.h"
#include "constants/item.h"
#include "constants/event_objects.h"
#include "constants/game_stat.h"
#include "constants/pokedex.h"
#include "constants/pokemon.h"
#include "constants/trainers.h"
#include "data/battle_emporium.h"
#include "data/achievements.h"
#include "data/achievement_boosts.h"

static void Achievement_SnapshotPartySpecies(struct Pokemon *party, u8 count, u16 *dest);
static bool8 Achievement_SpeciesSetsDisjoint(const u16 *a, const u16 *b);
static u8 Achievement_CountChallengeModifiers(void);

static void Achievement_CheckMasteryMilestones(void);
static void Achievement_CheckBoostMilestones(void);

static u32 Achievement_CountDistinctOwnedSpecies(struct Pokemon *party, u8 playerCount, u32 stopAt);

// ---- Randomizer & New Game+ (category O) -------------------------------

static bool8 Achievement_AnyRandomizerFlagSet(void)
{
    return FlagGet(FLAG_RANDOMIZE_MON) || FlagGet(FLAG_RANDOMIZE_TYPE) || FlagGet(FLAG_RANDOMIZE_MOVES);
}

static bool8 Achievement_IsLimitedPartyFirstRun(void)
{
    return gSaveBlock2Ptr->newGamePlus == 0 && LimitedParty_IsEnabled();
}

// Patchwork Team: six party members caught on six different routes.
// MON_DATA_MET_LOCATION is a region map section, so towns/cities count as a
// "route" -- an accepted approximation.
static bool8 Achievement_AllMetLocationsDistinct(struct Pokemon *party, u8 count)
{
    u8 i, j;

    if (count == 0)
        return FALSE;

    for (i = 0; i < count; i++)
    {
        u32 locI = GetMonData(&party[i], MON_DATA_MET_LOCATION);

        for (j = i + 1; j < count; j++)
        {
            if (locI == GetMonData(&party[j], MON_DATA_MET_LOCATION))
                return FALSE;
        }
    }

    return TRUE;
}

// The whole struct is written as one blob to a sector (see WriteAchievementProfile).
STATIC_ASSERT(sizeof(struct AchievementProfile) <= SECTOR_SIZE, AchievementProfileFreeSpace);

EWRAM_DATA struct AchievementProfile gAchievementProfile = {0};

// Separate from gDamagedSaveSectors on purpose (see achievements.h): a profile
// read/write failure must never be able to trigger the save-failed screen.
// Must default to FALSE (zero-initialized): non-zero static initializers land
// in a plain .data section that ld_script_modern.ld has no rule for and the
// trailing /DISCARD/ silently drops, leaving .text with a dangling reference.
static bool8 sAchievementProfileWriteFailed = FALSE;

// Set by mutators in this file (Achievement_TryComplete, boost
// purchase/reset, etc.) and cleared once Achievement_FlushProfile writes the
// profile out. Keeps flash writes off the hot path: a mutation only costs a
// flash write once, at the next safe flush point, not on every call.
static bool8 sAchievementProfileDirty = FALSE;

static u16 CalculateProfileChecksum(const struct AchievementProfile *profile)
{
    u16 offset = offsetof(struct AchievementProfile, totalPointsEarned);
    const u32 *data = (const u32 *)((const u8 *)profile + offset);
    u16 size = sizeof(*profile) - offset;
    u32 checksum = 0;
    u16 i;

    for (i = 0; i < size / 4; i++)
        checksum += data[i];

    return (checksum >> 16) + checksum;
}

static void InitDefaultAchievementProfile(void)
{
    memset(&gAchievementProfile, 0, sizeof(gAchievementProfile));
    gAchievementProfile.magic = ACHIEVEMENT_PROFILE_MAGIC;
    gAchievementProfile.version = ACHIEVEMENT_PROFILE_VERSION;
    gAchievementProfile.boostsEnabled = TRUE;
}

// Reads directly into a scratch struct rather than gAchievementProfile so a
// bad sector can never partially clobber the live profile.
static bool8 TryLoadAchievementProfileSector(u16 sector)
{
    struct AchievementProfile buffer;

    ReadFlash(sector, 0, (u8 *)&buffer, sizeof(buffer));

    if (buffer.magic != ACHIEVEMENT_PROFILE_MAGIC)
        return FALSE;
    if (buffer.version != ACHIEVEMENT_PROFILE_VERSION)
        return FALSE;
    if (buffer.checksum != CalculateProfileChecksum(&buffer))
        return FALSE;

    gAchievementProfile = buffer;
    return TRUE;
}

static void ReadAchievementProfile(void)
{
    if (gFlashMemoryPresent != TRUE)
    {
        InitDefaultAchievementProfile();
        return;
    }

    if (!TryLoadAchievementProfileSector(SECTOR_ID_ACHIEVEMENTS)
     && !TryLoadAchievementProfileSector(SECTOR_ID_ACHIEVEMENTS_BACKUP))
    {
        InitDefaultAchievementProfile();
    }

    // NOTE: clamp each boostLevels[i] to struct AchievementBoost's own
    // per-boost maxLevel ceiling here so corrupt flash data can never hand
    // back an out-of-range boost level.
}

// ProgramFlashSectorAndVerify always writes a full flash sector's worth of
// bytes from src, so it's given a full SECTOR_SIZE scratch buffer rather than
// &gAchievementProfile directly. gSaveDataBuffer is save.c's own sector-sized
// scratch space, already reused the same way by TryWriteSpecialSaveSector.
static void WriteAchievementProfile(void)
{
    if (gFlashMemoryPresent != TRUE)
        return;

    gAchievementProfile.checksum = CalculateProfileChecksum(&gAchievementProfile);

    memset(&gSaveDataBuffer, 0, SECTOR_SIZE);
    memcpy(&gSaveDataBuffer, &gAchievementProfile, sizeof(gAchievementProfile));

    sAchievementProfileWriteFailed = FALSE;

    if (ProgramFlashSectorAndVerify(SECTOR_ID_ACHIEVEMENTS, (u8 *)&gSaveDataBuffer))
        sAchievementProfileWriteFailed = TRUE;

    // Mirror. Either sector failing marks the whole write as an error.
    if (ProgramFlashSectorAndVerify(SECTOR_ID_ACHIEVEMENTS_BACKUP, (u8 *)&gSaveDataBuffer))
        sAchievementProfileWriteFailed = TRUE;
}

// Flag and points are already committed by the caller; a full popup queue
// drops only the toast, never the award.
static void QueueAchievementNotification(u16 achievementId)
{
    AchievementPopup_Enqueue(achievementId);
}

bool8 Achievement_ProfileWriteFailed(void)
{
    return sAchievementProfileWriteFailed;
}

// Called from the overworld popup safe point, TrySavingData, and after a boost
// purchase/reset. Awards earned mid-battle flush on return to the field: a hard
// reset can lose an award but never double-award it.
void Achievement_FlushProfile(void)
{
    if (!sAchievementProfileDirty)
        return;

    WriteAchievementProfile();
    sAchievementProfileDirty = FALSE;
}

// ---- Public API ---------------------------------------------------------

void Achievement_Init(void)
{
    ReadAchievementProfile();
}

bool8 Achievement_IsCompleted(u16 achievementId)
{
    if (achievementId >= MAX_ACHIEVEMENTS)
        return FALSE;

    return (gAchievementProfile.achievementFlags[achievementId / 8] >> (achievementId % 8)) & 1;
}

const struct Achievement *Achievement_GetInfo(u16 achievementId)
{
    if (achievementId >= ACHIEVEMENTS_COUNT)
        return &gAchievements[ACHIEVEMENT_NONE];

    return &gAchievements[achievementId];
}

// Lists only entries gated on a game-mode toggle fixed for the save's lifetime
// (ApplyPendingNewGameSettings runs only on a non-NG+ start) or on an existing
// sticky field proving a "never do X" condition is already broken. Everything
// else defaults to TRUE; tallies too far behind to finish are not detected.
bool8 Achievement_IsEligible(u16 achievementId)
{
    struct AchievementRunData *runData = &gSaveBlock1Ptr->achievementRunData;
    struct AchievementRunDataExt *runDataExt = &gSaveBlock2Ptr->achievementRunDataExt;
    bool8 isHard = (gSaveBlock1Ptr->difficulty == DIFFICULTY_HARD);
    bool8 anyRandomizer = Achievement_AnyRandomizerFlagSet();
    bool8 stackedGameMode = gSaveBlock1Ptr->nuzlockeModeEnabled || Draft_IsEnabled() || Recruits_IsEnabled();

    switch (achievementId)
    {
    // J. Multi-Run / Persistent Profile
    case ACHIEVEMENT_NG_PLUS_CYCLE_COMPLETE:
        return gSaveBlock2Ptr->newGamePlus > 0;
    case ACHIEVEMENT_NUZLOCKE_1:
        return gSaveBlock1Ptr->nuzlockeModeEnabled;
    case ACHIEVEMENT_RANDOMIZED_1:
        return anyRandomizer;

    // L. Team Building & Composition -- sticky "broken" flags, set the
    // instant the achievement's own no-longer-reversible condition is
    // violated and never cleared for the rest of the run.
    case ACHIEVEMENT_TEAM_MONO_TYPE_CHAMPION:
        return !runData->monoTypeBroken;
    case ACHIEVEMENT_TEAM_TRIAL_BY_FIRE:
        return isHard && !runData->monoTypeBroken;
    case ACHIEVEMENT_TEAM_TYPE_ROULETTE:
        return !runData->typeRouletteBroken;
    case ACHIEVEMENT_TEAM_UNDERDOG_RUN:
        return !runData->bstEverExceeded450;
    case ACHIEVEMENT_TEAM_SAME_SIX:
        return !runData->sameSixBroken;
    case ACHIEVEMENT_TEAM_NOBODY_BENCHED:
        return !runData->nobodyBenchedBroken;

    // N. Challenge Runs
    case ACHIEVEMENT_CHALLENGE_SELF_IMPOSED:
        return Achievement_CountChallengeModifiers() >= 3;
    case ACHIEVEMENT_CHALLENGE_NIGHTMARE_MODE:
        return gSaveBlock1Ptr->nuzlockeModeEnabled && isHard
            && FlagGet(FLAG_RANDOMIZE_MON) && FlagGet(FLAG_RANDOMIZE_TYPE) && FlagGet(FLAG_RANDOMIZE_MOVES);
    case ACHIEVEMENT_CHALLENGE_NO_SHOPPING_RUN:
        return !runData->boughtConsumableItem;
    case ACHIEVEMENT_CHALLENGE_WHO_NEEDS_CENTERS:
    case ACHIEVEMENT_CHALLENGE_NO_CENTERS:
        return GetGameStat(GAME_STAT_USED_POKECENTER) == 0;
    case ACHIEVEMENT_CHALLENGE_HARDCORE_SET:
        return isHard;
    case ACHIEVEMENT_CHALLENGE_SOLO_JOURNEY:
        return runData->highestPartySizeThisRun <= 1;
    case ACHIEVEMENT_CHALLENGE_NO_FREEBIES:
        return !runData->starterActedInMajorBattle;

    // N. Nuzlocke
    case ACHIEVEMENT_NUZLOCKE_FIRST_GYM:
    case ACHIEVEMENT_NUZLOCKE_CLOSE_CALL:
    case ACHIEVEMENT_NUZLOCKE_SCRAPPY:
    case ACHIEVEMENT_NUZLOCKE_GRAVEYARD:
        return gSaveBlock1Ptr->nuzlockeModeEnabled;
    case ACHIEVEMENT_NUZLOCKE_PERFECT:
        return gSaveBlock1Ptr->nuzlockeModeEnabled && runData->nuzlockeMonsLost == 0;

    // O. Randomizer & New Game+
    case ACHIEVEMENT_RANDOMIZER_CHAOS_BEGINS:
    case ACHIEVEMENT_RANDOMIZER_RANDOM_BY_NATURE:
    case ACHIEVEMENT_RANDOMIZER_CHAOS_TEAM:
    case ACHIEVEMENT_RANDOMIZER_ROOKIE:
        return anyRandomizer;
    case ACHIEVEMENT_RANDOMIZER_TRULY_RANDOM:
        return FlagGet(FLAG_RANDOMIZE_MON) && FlagGet(FLAG_RANDOMIZE_TYPE) && FlagGet(FLAG_RANDOMIZE_MOVES);
    case ACHIEVEMENT_RANDOMIZER_SPECIES_CHAOS:
        return FlagGet(FLAG_RANDOMIZE_MON);
    case ACHIEVEMENT_RANDOMIZER_TYPE_CHAOS:
        return FlagGet(FLAG_RANDOMIZE_TYPE);
    case ACHIEVEMENT_RANDOMIZER_MOVE_CHAOS:
        return FlagGet(FLAG_RANDOMIZE_MOVES);
    case ACHIEVEMENT_RANDOMIZER_PURE_CHAOS:
        return FlagGet(FLAG_RANDOMIZE_MON) && LimitedParty_IsEnabled() && MonoType_IsEnabled();
    case ACHIEVEMENT_NUZLOCKE_ACROSS_WORLDS:
        return gSaveBlock1Ptr->nuzlockeModeEnabled && anyRandomizer;
    case ACHIEVEMENT_NUZLOCKE_CHAOS_SURVIVOR:
        return gSaveBlock1Ptr->nuzlockeModeEnabled && anyRandomizer && isHard;
    case ACHIEVEMENT_NG_PLUS_FRESH_FACES:
    case ACHIEVEMENT_NG_PLUS_NEVER_THE_SAME_FIGHT:
        return gSaveBlock2Ptr->newGamePlus > 0;
    case ACHIEVEMENT_NG_PLUS_CYCLE_SPECIALIST:
        return gSaveBlock2Ptr->newGamePlus > 0 && Achievement_CountChallengeModifiers() >= 3;
    case ACHIEVEMENT_NG_PLUS_CYCLE_NUZLOCKE:
        return gSaveBlock1Ptr->nuzlockeModeEnabled;

    // M. Exploration -- "before entering the League" is gone the instant
    // FLAG_IS_CHAMPION is set without every town visited yet.
    case ACHIEVEMENT_EXPLORE_COMPLETIONIST_TOURIST:
        return !FlagGet(FLAG_IS_CHAMPION);

    // P. Streaks, Records & Collection Remainder
    case ACHIEVEMENT_RECORD_LEGEND_OF_THE_RUN:
        return !(runDataExt->anyMajorBattleThisRun && runDataExt->legendCandidateCount == 0);

    // R. Recruits Mode
    case ACHIEVEMENT_RECRUITS_FRESH_RECRUITS:
    case ACHIEVEMENT_RECRUITS_HONORABLE_DISCHARGE:
    case ACHIEVEMENT_RECRUITS_REVOLVING_DOOR:
    case ACHIEVEMENT_RECRUITS_FULL_TURNOVER:
        return Recruits_IsEnabled();
    case ACHIEVEMENT_RECRUITS_ENDLESS_RECRUITMENT_DRIVE:
        return Recruits_IsEnabled() && isHard;

    // S. Limited Party
    case ACHIEVEMENT_LIMITED_PARTY_TIGHT_SQUAD:
    case ACHIEVEMENT_LIMITED_PARTY_NO_ROOM_TO_SPARE:
    case ACHIEVEMENT_LIMITED_PARTY_EARNED_YOUR_KEEP:
    case ACHIEVEMENT_LIMITED_PARTY_FULL_ROSTER_RESTORED:
        return Achievement_IsLimitedPartyFirstRun();
    case ACHIEVEMENT_LIMITED_PARTY_BARE_MINIMUM_CHAMPION:
        // Not gated on LimitedParty_IsEnabled() -- any HARD run that never
        // carried more than LIMITED_PARTY_BASE_SIZE qualifies, per
        // Achievement_CheckNewModeCompletionMilestones. Still restricted to the
        // first fresh save so NG+ cycles cannot retroactively qualify.
        return gSaveBlock2Ptr->newGamePlus == 0 && isHard && runData->highestPartySizeThisRun <= LIMITED_PARTY_BASE_SIZE;

    // T. Draft Mode
    case ACHIEVEMENT_DRAFT_FIRST_PICK:
    case ACHIEVEMENT_DRAFT_TOUGH_CALL:
    case ACHIEVEMENT_DRAFT_THE_CASE_IS_CLOSED:
    case ACHIEVEMENT_DRAFT_FULL_CASE_CLEAR:
    case ACHIEVEMENT_DRAFT_DRAFTED_NOT_CAUGHT:
        return Draft_IsEnabled();
    case ACHIEVEMENT_DRAFT_NO_BALL_NEEDED:
        return Draft_IsEnabled() && isHard;

    // U. Rotation Mode
    case ACHIEVEMENT_ROTATION_SPIN_THE_WHEEL:
    case ACHIEVEMENT_ROTATION_ON_A_ROTATION:
    case ACHIEVEMENT_ROTATION_GYM_LEADER_ROULETTE:
    case ACHIEVEMENT_ROTATION_FULL_CIRCUIT:
        return RotationMode_IsEnabled();
    case ACHIEVEMENT_ROTATION_CHAOS_ROTATION:
        return RotationMode_IsEnabled() && stackedGameMode && FlagGet(FLAG_RANDOMIZE_MON);

    // V. Mono Type Mode
    case ACHIEVEMENT_MONO_TYPE_COMMITTED_TO_THE_BIT:
    case ACHIEVEMENT_MONO_TYPE_TYPE_SPECIALIST:
    case ACHIEVEMENT_MONO_TYPE_PERFECT_FIT:
    case ACHIEVEMENT_MONO_TYPE_TRUE_BELIEVER:
        return MonoType_IsEnabled();
    case ACHIEVEMENT_MONO_TYPE_ONE_TYPE_TO_RULE_THEM_ALL:
        return MonoType_IsEnabled() && isHard;
    case ACHIEVEMENT_MONO_TYPE_SECOND_VERSE:
        return MonoType_IsEnabled() && MonoGen_IsEnabled();

    // W. Mono Gen Mode
    case ACHIEVEMENT_MONO_GEN_GENERATION_LOYALIST:
    case ACHIEVEMENT_MONO_GEN_REGIONAL_PURIST:
    case ACHIEVEMENT_MONO_GEN_GOTTA_CATCH_SOME_OF_THEM:
    case ACHIEVEMENT_MONO_GEN_TRUE_TO_THE_ROOTS:
        return MonoGen_IsEnabled();
    case ACHIEVEMENT_MONO_GEN_OLD_SCHOOL_HARD_MODE:
        return MonoGen_IsEnabled() && isHard;

    // X. Cross-Mode Stacking
    case ACHIEVEMENT_CROSSMODE_MODE_COLLECTOR:
        return (Recruits_IsEnabled() + LimitedParty_IsEnabled() + Draft_IsEnabled()
              + RotationMode_IsEnabled() + MonoType_IsEnabled() + MonoGen_IsEnabled()) >= 2;
    case ACHIEVEMENT_CROSSMODE_KITCHEN_SINK:
        return LimitedParty_IsEnabled() && MonoType_IsEnabled() && MonoGen_IsEnabled() && RotationMode_IsEnabled();
    case ACHIEVEMENT_CROSSMODE_THE_FULL_STACK:
        return LimitedParty_IsEnabled() && MonoType_IsEnabled() && MonoGen_IsEnabled() && RotationMode_IsEnabled()
            && stackedGameMode;

    default:
        return TRUE;
    }
}

// Category J. Called from the tail of Achievement_TryComplete after
// totalPointsEarned is updated. Recursion is safe: the Achievement_IsCompleted
// guard makes a repeat call a no-op.
static void Achievement_CheckPointMilestones(void)
{
    if (gAchievementProfile.totalPointsEarned >= 6000)
        Achievement_TryComplete(ACHIEVEMENT_POINTS_6000);
}

// The flag and the points are written together, before any UI is involved,
// so a UI failure can never withhold a reward and a reset can never double-award.
bool8 Achievement_TryComplete(u16 achievementId)
{
    if (achievementId >= ACHIEVEMENTS_COUNT)
        return FALSE;

    // Debug mode disqualifies the current run.
    if (gSaveBlock1Ptr->achievementsBlocked)
        return FALSE;

    if (Achievement_IsCompleted(achievementId))
        return FALSE;

    gAchievementProfile.achievementFlags[achievementId / 8] |= 1 << (achievementId % 8);
    gAchievementProfile.totalPointsEarned += gAchievements[achievementId].points;
    // No Easy Path: running total of points from Gold-or-better achievements.
    if (gAchievements[achievementId].tier >= ACHIEVEMENT_TIER_GOLD)
        gAchievementProfile.pointsFromGoldOrBetter += gAchievements[achievementId].points;
    sAchievementProfileDirty = TRUE;

    QueueAchievementNotification(achievementId);

    Achievement_CheckPointMilestones();
    Achievement_CheckMasteryMilestones();

    return TRUE;
}

// Called from GameClear() when FLAG_SYS_GAME_CLEAR is newly set. Flushes
// immediately: rare, important state change, not a hot path.
void Achievement_OnFirstPlaythroughComplete(void)
{
    // Only the first unlock on this profile queues the announcement; later
    // clears (including NG+ cycles) also reach here.
    if (!gAchievementProfile.boostsUnlocked)
    {
        gAchievementProfile.boostsUnlocked = TRUE;
        gAchievementProfile.boostsUnlockNoticePending = TRUE;
    }

    // Runs on every Hall of Fame clear, including NG+ cycles. Gated to cycle 0
    // so "playthroughs" counts fresh saves only; NG+ has ngPlusCyclesCompleted.
    // Shown on the debug profile dump (src/debug.c).
    if (gSaveBlock2Ptr->newGamePlus == 0)
        gAchievementProfile.playthroughsCompleted++;

    if (gSaveBlock1Ptr->nuzlockeModeEnabled)
        gAchievementProfile.nuzlockesCompleted++;

    if (FlagGet(FLAG_RANDOMIZE_MON) || FlagGet(FLAG_RANDOMIZE_TYPE) || FlagGet(FLAG_RANDOMIZE_MOVES))
        gAchievementProfile.randomizedRunsCompleted++;

    // Category J: multi-run milestones from the counters above.
    if (gAchievementProfile.nuzlockesCompleted >= 1)
        Achievement_TryComplete(ACHIEVEMENT_NUZLOCKE_1);
    if (gAchievementProfile.randomizedRunsCompleted >= 1)
        Achievement_TryComplete(ACHIEVEMENT_RANDOMIZED_1);

    // Read off the flags this completion ran under. Chaos Begins is also
    // checked in Achievement_OnNewGamePlusStarted; the first playthrough never
    // calls that, so it is checked here too. Chaos Begins needs any one
    // randomizer flag; Truly Random needs all three.
    if (Achievement_AnyRandomizerFlagSet())
        Achievement_TryComplete(ACHIEVEMENT_RANDOMIZER_CHAOS_BEGINS);
    if (FlagGet(FLAG_RANDOMIZE_MON) && FlagGet(FLAG_RANDOMIZE_TYPE) && FlagGet(FLAG_RANDOMIZE_MOVES))
        Achievement_TryComplete(ACHIEVEMENT_RANDOMIZER_TRULY_RANDOM);
    if (FlagGet(FLAG_RANDOMIZE_MON))
    {
        Achievement_TryComplete(ACHIEVEMENT_RANDOMIZER_SPECIES_CHAOS);
        // Pure Chaos: species randomizer stacked with Limited Party and Mono
        // Type -- distinct from Nightmare Mode's Nuzlocke/HARD/all-3-flags combo.
        if (LimitedParty_IsEnabled() && MonoType_IsEnabled())
            Achievement_TryComplete(ACHIEVEMENT_RANDOMIZER_PURE_CHAOS);
    }
    if (FlagGet(FLAG_RANDOMIZE_TYPE))
        Achievement_TryComplete(ACHIEVEMENT_RANDOMIZER_TYPE_CHAOS);
    if (FlagGet(FLAG_RANDOMIZE_MOVES))
        Achievement_TryComplete(ACHIEVEMENT_RANDOMIZER_MOVE_CHAOS);

    sAchievementProfileDirty = TRUE;
    Achievement_FlushProfile();
}

// Flushes immediately: rare, important state change.
void Achievement_OnNewGamePlusStarted(u8 cycle)
{
    struct AchievementRunDataExt *runDataExt = &gSaveBlock2Ptr->achievementRunDataExt;

    if (cycle > gAchievementProfile.highestNgPlusCycle)
        gAchievementProfile.highestNgPlusCycle = cycle;

    // NG+ can start with randomizer flags freshly toggled on.
    if (Achievement_AnyRandomizerFlagSet())
        Achievement_TryComplete(ACHIEVEMENT_RANDOMIZER_CHAOS_BEGINS);

    // Zero per-cycle AchievementRunDataExt fields; ClearSav1 does not cover
    // SaveBlock2 (see that struct's comment).
    runDataExt->trainersDefeatedThisCycle = 0;

    // Game-mode achievements (categories R-W).
    runDataExt->recruitsRetiredThisCycle = 0;
    runDataExt->limitedPartyWinsAtCap = 0;
    runDataExt->draftsCompletedThisCycle = 0;
    runDataExt->rotationTrainerWinsThisCycle = 0;
    runDataExt->monoTypeObtainedThisCycle = 0;
    runDataExt->monoGenObtainedThisCycle = 0;

    // Veteran Team/Old Reliable are CURRENT_PLAYTHROUGH scoped.
    memset(runDataExt->koCountPerSlot, 0, sizeof(runDataExt->koCountPerSlot));
    memset(runDataExt->majorKoCountPerSlot, 0, sizeof(runDataExt->majorKoCountPerSlot));

    // Three/Eight Gym Streak are CURRENT_PLAYTHROUGH scoped.
    runDataExt->gymLeadersSinceWipe = 0;

    sAchievementProfileDirty = TRUE;
    Achievement_FlushProfile();
}

void Achievement_OnNewGamePlusCycleCompleted(void)
{
    gAchievementProfile.ngPlusCyclesCompleted++;

    // Runs only when an NG+ cycle was just beaten; no threshold needed.
    Achievement_TryComplete(ACHIEVEMENT_NG_PLUS_CYCLE_COMPLETE);

    if (Achievement_CountChallengeModifiers() >= 3)
        Achievement_TryComplete(ACHIEVEMENT_NG_PLUS_CYCLE_SPECIALIST);

    if (gSaveBlock1Ptr->nuzlockeModeEnabled)
        Achievement_TryComplete(ACHIEVEMENT_NG_PLUS_CYCLE_NUZLOCKE);

    sAchievementProfileDirty = TRUE;
    Achievement_FlushProfile();
}

u32 Achievement_GetTotalPoints(void)
{
    return gAchievementProfile.totalPointsEarned;
}

u32 Achievement_GetAvailablePoints(void)
{
    return gAchievementProfile.totalPointsEarned - gAchievementProfile.pointsInvested;
}

u32 Achievement_GetCompletedCount(void)
{
    u32 id, count = 0;

    for (id = ACHIEVEMENT_NONE + 1; id < ACHIEVEMENTS_COUNT; id++)
    {
        if (Achievement_IsCompleted(id))
            count++;
    }
    return count;
}

u32 Achievement_GetCompletedCountInTier(enum AchievementTier tier)
{
    u32 id, count = 0;

    for (id = ACHIEVEMENT_NONE + 1; id < ACHIEVEMENTS_COUNT; id++)
    {
        if (Achievement_GetInfo(id)->tier == tier && Achievement_IsCompleted(id))
            count++;
    }
    return count;
}

u32 Achievement_GetTotalCountInTier(enum AchievementTier tier)
{
    u32 id, count = 0;

    for (id = ACHIEVEMENT_NONE + 1; id < ACHIEVEMENTS_COUNT; id++)
    {
        if (Achievement_GetInfo(id)->tier == tier)
            count++;
    }
    return count;
}

bool8 Achievement_BoostsUnlocked(void)
{
    return gAchievementProfile.boostsUnlocked;
}

// Script specials for the post-credits bedroom message
// (data/scripts/players_house.inc).
u16 GetBoostsUnlockNoticePending(void)
{
    return gAchievementProfile.boostsUnlockNoticePending;
}

void ClearBoostsUnlockNotice(void)
{
    gAchievementProfile.boostsUnlockNoticePending = FALSE;
    sAchievementProfileDirty = TRUE;
    Achievement_FlushProfile();
}

bool8 Achievement_BoostsEnabled(void)
{
    return gAchievementProfile.boostsEnabled;
}

void Achievement_SetBoostsEnabled(bool8 enabled)
{
    gAchievementProfile.boostsEnabled = enabled;
    sAchievementProfileDirty = TRUE;
}

u8 AchievementBoost_GetLevel(u16 boostId)
{
    if (boostId >= MAX_BOOSTS)
        return 0;

    return gAchievementProfile.boostLevels[boostId];
}

u8 AchievementBoost_GetActiveLevel(u16 boostId)
{
    u8 owned = AchievementBoost_GetLevel(boostId);
    u8 reduction;

    if (boostId >= MAX_BOOSTS)
        return 0;

    // A reduction >= owned (e.g. a debug tool dropping the purchased level
    // below what was previously dialed back) means fully off, not a
    // wrapped-around active level.
    reduction = gAchievementProfile.boostLevelReduction[boostId];
    if (reduction >= owned)
        return 0;

    return owned - reduction;
}

bool8 AchievementBoost_TryChangeActiveLevel(u16 boostId, s8 delta)
{
    u8 owned;
    s16 newActive;

    if (boostId >= MAX_BOOSTS)
        return FALSE;

    owned = AchievementBoost_GetLevel(boostId);
    if (owned == 0)
        return FALSE;

    newActive = (s16)AchievementBoost_GetActiveLevel(boostId) + delta;
    if (newActive < 0 || newActive > owned)
        return FALSE;

    gAchievementProfile.boostLevelReduction[boostId] = owned - (u8)newActive;
    sAchievementProfileDirty = TRUE;
    Achievement_FlushProfile();

    return TRUE;
}

const struct AchievementBoost *AchievementBoost_GetInfo(u16 boostId)
{
    if (boostId >= BOOSTS_COUNT)
        return &gAchievementBoosts[BOOST_NONE];

    return &gAchievementBoosts[boostId];
}

// The only non-debug path that increments boostLevels[id]. The debug menu can
// set a level past maxLevel, so ">= maxLevel" is treated as maxed.
//
// Not gated on achievementsBlocked: debug mode disqualifies earning points,
// not spending points already earned.
bool8 AchievementBoost_CanPurchase(u16 boostId)
{
    const struct AchievementBoost *info;
    u8 level;

    if (!gAchievementProfile.boostsUnlocked)
        return FALSE;

    if (boostId >= BOOSTS_COUNT)
        return FALSE;

    info = AchievementBoost_GetInfo(boostId);
    level = AchievementBoost_GetLevel(boostId);

    if (level >= info->maxLevel)
        return FALSE;

    if (Achievement_GetAvailablePoints() < info->costs[level])
        return FALSE;

    return TRUE;
}

// pointsInvested grows only by an amount CanPurchase verified is <= the
// available balance, so totalPointsEarned - pointsInvested never goes negative.
bool8 AchievementBoost_Purchase(u16 boostId)
{
    const struct AchievementBoost *info;
    u8 level;

    if (!AchievementBoost_CanPurchase(boostId))
        return FALSE;

    info = AchievementBoost_GetInfo(boostId);
    level = AchievementBoost_GetLevel(boostId);

    gAchievementProfile.pointsInvested += info->costs[level];
    gAchievementProfile.boostLevels[boostId]++;
    sAchievementProfileDirty = TRUE;

    // Boost achievements depend on boostLevels[]/pointsInvested, which change
    // only here and in AchievementBoost_Reset.
    Achievement_CheckBoostMilestones();

    Achievement_FlushProfile();

    return TRUE;
}

// "Nothing invested" is checked before the fee so a player with no boosts is
// never told they need money for a reset that refunds nothing.
bool8 AchievementBoost_CanReset(void)
{
    if (!gAchievementProfile.boostsUnlocked)
        return FALSE;

    if (gAchievementProfile.pointsInvested == 0)
        return FALSE;

    if (!IsEnoughMoney(&gSaveBlock1Ptr->money, ACHIEVEMENT_BOOST_RESET_FEE))
        return FALSE;

    return TRUE;
}

// The refund is exactly pointsInvested, so a reset can never generate points.
// CanReset is re-checked here; on failure money, points and levels are all
// left untouched.
bool8 AchievementBoost_Reset(void)
{
    if (!AchievementBoost_CanReset())
        return FALSE;

    gAchievementProfile.pointsInvested = 0;
    memset(gAchievementProfile.boostLevels, 0, sizeof(gAchievementProfile.boostLevels));
    memset(gAchievementProfile.boostLevelReduction, 0, sizeof(gAchievementProfile.boostLevelReduction));
    RemoveMoney(&gSaveBlock1Ptr->money, ACHIEVEMENT_BOOST_RESET_FEE);
    gAchievementProfile.boostResets++;
    sAchievementProfileDirty = TRUE;

    // Reconfigured.
    Achievement_CheckBoostMilestones();

    Achievement_FlushProfile();

    return TRUE;
}

// Pattern for every boost effect: return baseline when boosts are disabled or
// the level is 0. u64 math keeps an NG+-inflated expValue * percent from
// overflowing before the /100.
u32 AchievementBoost_ApplyExp(u32 expValue)
{
    u8 level;
    u32 percent;

    if (!gAchievementProfile.boostsEnabled)
        return expValue;

    level = AchievementBoost_GetActiveLevel(BOOST_EXP_GAIN);
    if (level == 0)
        return expValue;

    percent = 100 + AchievementBoost_GetInfo(BOOST_EXP_GAIN)->effects[level];
    return (u32)(((u64)expValue * percent) / 100);
}

u32 AchievementBoost_ExtraShinyRerolls(void)
{
    u8 level;

    if (!gAchievementProfile.boostsEnabled)
        return 0;

    level = AchievementBoost_GetActiveLevel(BOOST_SHINY_CHANCE);
    if (level == 0)
        return 0;

    return AchievementBoost_GetInfo(BOOST_SHINY_CHANCE)->effects[level];
}

u32 AchievementBoost_ApplyCatchOdds(u32 odds)
{
    u8 level;
    u32 percent;

    if (!gAchievementProfile.boostsEnabled)
        return odds;

    level = AchievementBoost_GetActiveLevel(BOOST_CATCH_RATE);
    if (level == 0)
        return odds;

    percent = 100 + AchievementBoost_GetInfo(BOOST_CATCH_RATE)->effects[level];
    return (u32)(((u64)odds * percent) / 100);
}

u32 AchievementBoost_ApplyMoneyReward(u32 money)
{
    u8 level;
    u32 percent;

    if (!gAchievementProfile.boostsEnabled)
        return money;

    level = AchievementBoost_GetActiveLevel(BOOST_MONEY_GAIN);
    if (level == 0)
        return money;

    percent = 100 + AchievementBoost_GetInfo(BOOST_MONEY_GAIN)->effects[level];
    return (u32)(((u64)money * percent) / 100);
}

u8 AchievementBoost_ApplyEggCyclesToSubtract(u8 toSub)
{
    u8 level;

    if (!gAchievementProfile.boostsEnabled)
        return toSub;

    level = AchievementBoost_GetActiveLevel(BOOST_EGG_HATCH_SPEED);
    if (level == 0)
        return toSub;

    return toSub + AchievementBoost_GetInfo(BOOST_EGG_HATCH_SPEED)->effects[level];
}

s32 AchievementBoost_ApplyFriendshipGain(s32 bonus)
{
    u8 level;
    u32 percent;

    if (bonus <= 0 || !gAchievementProfile.boostsEnabled)
        return bonus;

    level = AchievementBoost_GetActiveLevel(BOOST_FRIENDSHIP_GAIN);
    if (level == 0)
        return bonus;

    percent = 100 + AchievementBoost_GetInfo(BOOST_FRIENDSHIP_GAIN)->effects[level];
    return (s32)(((s64)bonus * percent) / 100);
}

bool8 AchievementBoost_ShouldRoamerSeekPlayer(void)
{
    u8 level;
    u32 percent;

    if (!gAchievementProfile.boostsEnabled)
        return FALSE;

    level = AchievementBoost_GetActiveLevel(BOOST_LEGENDARY_ENCOUNTER);
    if (level == 0)
        return FALSE;

    percent = AchievementBoost_GetInfo(BOOST_LEGENDARY_ENCOUNTER)->effects[level];
    return (Random() % 100) < percent;
}

// ---- The remaining numerical/binary boosts ------------------------------
//
// The battle boosts (crit, PP saver, status recovery, survive) return a raw
// percent instead of rolling here. Battle randomness must go through the tagged
// RandomChance/RandomPercentage helpers so tests and recorded-battle playback
// stay deterministic. Returning 0 lets the call site skip its roll, so the
// baseline path consumes no RNG.

// BOOST_TYPE_BINARY: "purchased" is the whole effect.
static bool8 IsBinaryBoostActive(u16 boostId)
{
    return gAchievementProfile.boostsEnabled && AchievementBoost_GetActiveLevel(boostId) != 0;
}

static u32 GetBoostEffectValue(u16 boostId)
{
    u8 level;

    if (!gAchievementProfile.boostsEnabled)
        return 0;

    level = AchievementBoost_GetActiveLevel(boostId);
    if (level == 0)
        return 0;

    return AchievementBoost_GetInfo(boostId)->effects[level];
}

u32 AchievementBoost_GetCritChancePercent(void)
{
    return GetBoostEffectValue(BOOST_CRIT_CHANCE);
}

u32 AchievementBoost_GetPpSavePercent(void)
{
    return GetBoostEffectValue(BOOST_PP_SAVER);
}

u32 AchievementBoost_GetStatusRecoveryPercent(void)
{
    return GetBoostEffectValue(BOOST_STATUS_RECOVERY);
}

u8 AchievementBoost_ApplyBerryYield(u8 count)
{
    u32 boosted;

    // An empty tree stays empty.
    if (count == 0)
        return 0;

    boosted = count + GetBoostEffectValue(BOOST_BERRY_YIELD);
    return (boosted > 255) ? 255 : (u8)boosted;
}

u16 AchievementBoost_ApplyBerryStageDuration(u16 minutes)
{
    u32 percent = GetBoostEffectValue(BOOST_BERRY_GROWTH);
    u32 boosted;

    if (percent == 0)
        return minutes;

    // Divide rather than subtract, so the top level (+100%) halves the wait
    // instead of reaching zero. The floor of 1 keeps BerryTreeTimeUpdate's
    // growth loop from ever seeing a zero countdown.
    boosted = ((u32)minutes * 100) / (100 + percent);
    return (boosted == 0) ? 1 : (u16)boosted;
}

u16 AchievementBoost_ApplySprayStepCount(u16 steps)
{
    u32 percent = GetBoostEffectValue(BOOST_SPRAY_DURATION);
    u32 boosted;

    if (percent == 0)
        return steps;

    // Clamped below bit 15 (REPEL_LURE_MASK, constants/item.h) so a boosted
    // count can never bleed into the "this is a Lure" flag.
    boosted = ((u32)steps * (100 + percent)) / 100;
    return (boosted >= REPEL_LURE_MASK) ? (REPEL_LURE_MASK - 1) : (u16)boosted;
}

bool8 AchievementBoost_HasNuzlockeSecondChance(void)
{
    return IsBinaryBoostActive(BOOST_NUZLOCKE_SECOND_CHANCE);
}

bool8 AchievementBoost_HasStarterKit(void)
{
    return IsBinaryBoostActive(BOOST_STARTER_KIT);
}

bool8 AchievementBoost_HasPerfectStarterIvs(void)
{
    return IsBinaryBoostActive(BOOST_PERFECT_STARTER_IVS);
}

// ---- Starting items, rerolls, discounts ---------------------------------

bool8 AchievementBoost_HasShinyCharmStart(void)
{
    return IsBinaryBoostActive(BOOST_SHINY_CHARM_START);
}

bool8 AchievementBoost_HasAbilityCapsuleStart(void)
{
    return IsBinaryBoostActive(BOOST_ABILITY_CAPSULE_START);
}

bool8 AchievementBoost_HasAbilityPatchStart(void)
{
    return IsBinaryBoostActive(BOOST_ABILITY_PATCH_START);
}

bool8 AchievementBoost_ShouldConsumeItem(enum Item itemId)
{
    u32 percent;

    if (GetItemPocket(itemId) != POCKET_ITEMS)
        return TRUE;

    percent = GetBoostEffectValue(BOOST_CONSUMABLE_SAVE);
    if (percent == 0)
        return TRUE;

    return (Random() % 100) >= percent;
}

// Rolls `rerolls` extra IV spreads on top of `mon`'s current one and keeps the
// spread with the highest total, then recalculates stats.
static void RerollMonIvsKeepBest(struct Pokemon *mon, u8 rerolls)
{
    u8 bestIvs[NUM_STATS];
    u32 bestSum, i, r;

    if (rerolls == 0)
        return;

    bestSum = 0;
    for (i = 0; i < NUM_STATS; i++)
    {
        bestIvs[i] = GetMonData(mon, MON_DATA_HP_IV + i);
        bestSum += bestIvs[i];
    }

    for (r = 0; r < rerolls; r++)
    {
        u32 curSum = 0;

        SetBoxMonIVs(&mon->box, USE_RANDOM_IVS);
        for (i = 0; i < NUM_STATS; i++)
            curSum += GetMonData(mon, MON_DATA_HP_IV + i);

        if (curSum > bestSum)
        {
            bestSum = curSum;
            for (i = 0; i < NUM_STATS; i++)
                bestIvs[i] = GetMonData(mon, MON_DATA_HP_IV + i);
        }
    }

    for (i = 0; i < NUM_STATS; i++)
        SetMonData(mon, MON_DATA_HP_IV + i, &bestIvs[i]);
    CalculateMonStats(mon);
}

void AchievementBoost_ApplyEggIvReroll(struct Pokemon *mon)
{
    if (!gAchievementProfile.boostsEnabled)
        return;

    RerollMonIvsKeepBest(mon, GetBoostEffectValue(BOOST_EGG_IV_REROLL));
}

void AchievementBoost_ApplyWildIvReroll(struct Pokemon *mon)
{
    if (!gAchievementProfile.boostsEnabled)
        return;

    RerollMonIvsKeepBest(mon, GetBoostEffectValue(BOOST_WILD_IV_REROLL));
}

u32 AchievementBoost_ApplyShopPrice(u32 price)
{
    u32 percent = GetBoostEffectValue(BOOST_SHOP_DISCOUNT);

    if (percent == 0)
        return price;

    return price - (price * percent) / 100;
}

u32 AchievementBoost_GetSurviveChancePercent(void)
{
    return GetBoostEffectValue(BOOST_SURVIVE_1HP);
}

void AchievementBoost_ApplyPostBattleHeal(void)
{
    u32 percent = GetBoostEffectValue(BOOST_POST_BATTLE_HEAL);
    struct Pokemon *party = gParties[B_TRAINER_PLAYER];
    u32 i;

    if (percent == 0)
        return;

    for (i = 0; i < gPartiesCount[B_TRAINER_PLAYER]; i++)
    {
        u32 maxHp, hp, heal, newHp;

        if (GetMonData(&party[i], MON_DATA_IS_EGG))
            continue;

        hp = GetMonData(&party[i], MON_DATA_HP);
        if (hp == 0)
            continue;

        maxHp = GetMonData(&party[i], MON_DATA_MAX_HP);
        heal = (maxHp * percent) / 100;
        if (heal == 0)
            continue;

        newHp = min(hp + heal, maxHp);
        SetMonData(&party[i], MON_DATA_HP, &newHp);
    }
}

// ---- Catalog hook functions (categories A-J) ----------------------------

// Badge/story milestones funnelled through Common_EventScript_CheckLevelCapIncrease
// (data/scripts/level_cap.inc). Each call site sets its flag right before the
// call, so checking every entry unconditionally is correct.
static const struct
{
    u16 flag;
    u16 achievementId;
} sStoryMilestones[] =
{
    { FLAG_BEAT_RIVAL_ROUTE_103,                ACHIEVEMENT_STORY_RIVAL_ROUTE103 },
    { FLAG_BADGE01_GET,                         ACHIEVEMENT_BADGE_STONE },
    { FLAG_BEAT_FIRST_GRUNT,                    ACHIEVEMENT_STORY_PETALBURG_WOODS },
    { FLAG_BADGE02_GET,                         ACHIEVEMENT_BADGE_KNUCKLE },
    { FLAG_BADGE03_GET,                         ACHIEVEMENT_BADGE_DYNAMO },
    { FLAG_BADGE04_GET,                         ACHIEVEMENT_BADGE_HEAT },
    { FLAG_BADGE05_GET,                         ACHIEVEMENT_BADGE_BALANCE },
    { FLAG_TEAM_AQUA_ESCAPED_IN_SUBMARINE,       ACHIEVEMENT_STORY_AQUA_HIDEOUT },
    { FLAG_RECEIVED_RED_OR_BLUE_ORB,            ACHIEVEMENT_STORY_MT_PYRE },
    { FLAG_HIDE_MAGMA_HIDEOUT_GRUNTS,           ACHIEVEMENT_STORY_MAGMA_HIDEOUT },
    { FLAG_BADGE06_GET,                         ACHIEVEMENT_BADGE_FEATHER },
    { FLAG_HIDE_SEAFLOOR_CAVERN_AQUA_GRUNTS,    ACHIEVEMENT_STORY_SEAFLOOR_CAVERN },
    { FLAG_BADGE07_GET,                         ACHIEVEMENT_BADGE_MIND },
    { FLAG_BADGE08_GET,                         ACHIEVEMENT_BADGE_RAIN },
    { FLAG_IS_CHAMPION,                         ACHIEVEMENT_STORY_CHAMPION },
};

void Achievement_CheckStoryMilestones(void)
{
    u32 i;

    for (i = 0; i < ARRAY_COUNT(sStoryMilestones); i++)
    {
        if (FlagGet(sStoryMilestones[i].flag))
            Achievement_TryComplete(sStoryMilestones[i].achievementId);
    }

    // Who Needs Centers?: checked at the 5th badge.
    if (FlagGet(FLAG_BADGE05_GET) && GetGameStat(GAME_STAT_USED_POKECENTER) == 0)
        Achievement_TryComplete(ACHIEVEMENT_CHALLENGE_WHO_NEEDS_CENTERS);

    // Earned Your Keep/Full Roster Restored read the live derived cap, which
    // only grows. Scoped to the first fresh save, not NG+ cycles reusing the
    // same flags.
    if (Achievement_IsLimitedPartyFirstRun() && CountPlayerBadges() >= 2)
        Achievement_TryComplete(ACHIEVEMENT_LIMITED_PARTY_EARNED_YOUR_KEEP);
    if (Achievement_IsLimitedPartyFirstRun() && LimitedParty_GetMaxPartySize() == PARTY_SIZE)
        Achievement_TryComplete(ACHIEVEMENT_LIMITED_PARTY_FULL_ROSTER_RESTORED);

    // Type Specialist/Regional Purist: Gym 4 checkpoint.
    if (FlagGet(FLAG_BADGE04_GET) && MonoType_IsEnabled())
        Achievement_TryComplete(ACHIEVEMENT_MONO_TYPE_TYPE_SPECIALIST);
    if (FlagGet(FLAG_BADGE04_GET) && MonoGen_IsEnabled())
        Achievement_TryComplete(ACHIEVEMENT_MONO_GEN_REGIONAL_PURIST);

    // Party-state checks not tied to a specific battle.
    Achievement_CheckPartyStateMilestones();
}

// Percentages of NATIONAL_DEX_COUNT so thresholds follow the build's species
// configuration.
void Achievement_CheckPokedexMilestones(bool8 caught)
{
    u16 count;

    if (!caught)
    {
        count = GetNationalPokedexCount(FLAG_GET_SEEN);
        if (count >= NATIONAL_DEX_COUNT * 10 / 100)
            Achievement_TryComplete(ACHIEVEMENT_DEX_SEEN_10);
        if (count >= NATIONAL_DEX_COUNT * 25 / 100)
            Achievement_TryComplete(ACHIEVEMENT_DEX_SEEN_25);
        if (count >= NATIONAL_DEX_COUNT * 50 / 100)
            Achievement_TryComplete(ACHIEVEMENT_DEX_SEEN_50);
        if (count >= NATIONAL_DEX_COUNT)
            Achievement_TryComplete(ACHIEVEMENT_DEX_SEEN_100);

        // Local Expert.
        Achievement_CheckLocalExpert();
    }
    else
    {
        // Diamond tier of the capture ladder: a distinct-species count, unlike
        // the raw counts in Achievement_CheckCaptureMilestones.
        count = GetNationalPokedexCount(FLAG_GET_CAUGHT);
        if (count >= NATIONAL_DEX_COUNT)
            Achievement_TryComplete(ACHIEVEMENT_CATCH_ALL);
    }
}

// GAME_STAT_POKEMON_CAPTURES already includes the current catch: in
// data/battle_scripts_2.s incrementgamestat precedes givecaughtmon. The Diamond
// tier (ACHIEVEMENT_CATCH_ALL) is in Achievement_CheckPokedexMilestones.
void Achievement_CheckCaptureMilestones(void)
{
    u32 count = GetGameStat(GAME_STAT_POKEMON_CAPTURES);

    if (count >= 100)
        Achievement_TryComplete(ACHIEVEMENT_CATCH_100);
    if (count >= 350)
        Achievement_TryComplete(ACHIEVEMENT_CATCH_350);
    if (count >= 700)
        Achievement_TryComplete(ACHIEVEMENT_CATCH_700);
}

void Achievement_OnShinyObtained(void)
{
    gAchievementProfile.shiniesObtained++;
    sAchievementProfileDirty = TRUE;
    Achievement_FlushProfile();

    if (gAchievementProfile.shiniesObtained >= 1)
        Achievement_TryComplete(ACHIEVEMENT_SHINY_1);
    if (gAchievementProfile.shiniesObtained >= 5)
        Achievement_TryComplete(ACHIEVEMENT_SHINY_5);
    if (gAchievementProfile.shiniesObtained >= 25)
        Achievement_TryComplete(ACHIEVEMENT_SHINY_25);
}

// Uses the lifetime profile counter, not GAME_STAT_TRAINER_BATTLES: game stats
// live in SaveBlock1, which ClearSav1 zeroes every new game. Runs once per
// battle end (CB2_EndTrainerBattle).
void Achievement_CheckTrainerBattleMilestones(void)
{
    u32 count;

    if (gAchievementProfile.trainerBattlesLifetime < 0xFFFF)
        gAchievementProfile.trainerBattlesLifetime++;
    sAchievementProfileDirty = TRUE;
    count = gAchievementProfile.trainerBattlesLifetime;

    if (count >= 10)
        Achievement_TryComplete(ACHIEVEMENT_TRAINERS_10);
    if (count >= 50)
        Achievement_TryComplete(ACHIEVEMENT_TRAINERS_50);
    if (count >= 150)
        Achievement_TryComplete(ACHIEVEMENT_TRAINERS_150);
    if (count >= 300)
        Achievement_TryComplete(ACHIEVEMENT_TRAINERS_300);
    if (count >= 500)
        Achievement_TryComplete(ACHIEVEMENT_TRAINERS_500);
}

// Lifetime counter for the same reason as the trainer version above.
void Achievement_CheckWildBattleMilestones(void)
{
    u32 count;

    if (gAchievementProfile.wildBattlesLifetime < 0xFFFF)
        gAchievementProfile.wildBattlesLifetime++;
    sAchievementProfileDirty = TRUE;
    count = gAchievementProfile.wildBattlesLifetime;

    if (count >= 50)
        Achievement_TryComplete(ACHIEVEMENT_WILD_BATTLES_50);
    if (count >= 250)
        Achievement_TryComplete(ACHIEVEMENT_WILD_BATTLES_250);
    if (count >= 500)
        Achievement_TryComplete(ACHIEVEMENT_WILD_BATTLES_500);
}

void Achievement_CheckItemMilestones(enum Item itemId)
{
    switch (itemId)
    {
    case ITEM_MASTER_BALL:
        Achievement_TryComplete(ACHIEVEMENT_ITEM_MASTER_BALL);
        break;
    case ITEM_RARE_CANDY:
        Achievement_TryComplete(ACHIEVEMENT_ITEM_RARE_CANDY);
        break;
    case ITEM_PP_UP:
        Achievement_TryComplete(ACHIEVEMENT_ITEM_PP_UP);
        break;
    case ITEM_HEART_SCALE:
        Achievement_TryComplete(ACHIEVEMENT_ITEM_HEART_SCALE);
        break;
    default:
        break;
    }
}

// Called with the post-clamp balance (GetMoney(moneyPtr) after SetMoney) --
// checking the raw amount being added would under-count once the player is
// near MAX_MONEY.
void Achievement_CheckMoneyMilestones(u32 money)
{
    if (money >= 10000)
        Achievement_TryComplete(ACHIEVEMENT_MONEY_10K);
    if (money >= 100000)
        Achievement_TryComplete(ACHIEVEMENT_MONEY_100K);
    if (money >= MAX_MONEY)
        Achievement_TryComplete(ACHIEVEMENT_MONEY_MAX);
}

// Uses the lifetime profile counter, not GAME_STAT_HATCHED_EGGS (SaveBlock1,
// zeroed by ClearSav1 every new game).
void Achievement_CheckEggMilestones(bool8 isShiny)
{
    u32 count;

    if (gAchievementProfile.eggsHatchedLifetime < 0xFFFF)
        gAchievementProfile.eggsHatchedLifetime++;
    sAchievementProfileDirty = TRUE;
    count = gAchievementProfile.eggsHatchedLifetime;

    if (count >= 1)
        Achievement_TryComplete(ACHIEVEMENT_EGG_1);
    if (count >= 10)
        Achievement_TryComplete(ACHIEVEMENT_EGG_10);
    if (count >= 50)
        Achievement_TryComplete(ACHIEVEMENT_EGG_50);
    if (isShiny)
        Achievement_TryComplete(ACHIEVEMENT_EGG_SHINY);

    // Egg Marathon.
    if (count >= 100)
        Achievement_TryComplete(ACHIEVEMENT_RECORD_EGG_MARATHON);
}

// ---- Battle Mastery (category K) ----------------------------------------
//
// EWRAM-only, never saved: a battle never spans a save. Zeroed at battle start
// (Achievement_ClearBattleData, BattleStartClearSetData) and read once by
// Achievement_CheckBattleMilestones on a win, so category K entries only count
// in battles the player won.
struct AchievementBattleData
{
    u32 typesUsed;                  // bitmask over enum Type -- the player's move types
    u8  statusesInflicted;          // bitmask of ACHIEVEMENT_STATUS_BIT_*
    u8  kosPerSlot[PARTY_SIZE];     // opposing KOs credited to each party slot
    u8  slotsThatActed;             // bitmask over party slots -- "acted" means "used a move"
    u8  moveSlotsUsed[PARTY_SIZE];  // bitmask over the 4 move slots, per party slot
    u16 prevPlayerMove;             // for the "never the same move twice in a row" check
    u8  lastThreeKoSlots[3];        // rolling window, party index + 1 (0 = no KO yet)
    u8  statusKoCount;              // opposing mons that fainted to status damage
    u8  pendingSetupBattler;        // 0 = none, else battlerId + 1 -- bookkeeping for setupThenKo
    bool8 currentMoveFollowsSetup:1; // bookkeeping for setupThenKo, see Achievement_RecordMoveUsed
    bool8 repeatedMove:1;
    bool8 superEffectiveUsed:1;
    bool8 stabUsed:1;
    bool8 setupMoveUsed:1;
    bool8 setupThenKo:1;
    bool8 critLanded:1;
    bool8 priorityKo:1;
};

EWRAM_DATA static struct AchievementBattleData sBattleData = {0};

bool8 Achievement_IsMajorBattle(void)
{
    if (!(gBattleTypeFlags & BATTLE_TYPE_TRAINER))
        return FALSE;

    switch (GetTrainerClassFromId(TRAINER_BATTLE_PARAM.opponentA))
    {
    case TRAINER_CLASS_LEADER:
    case TRAINER_CLASS_ELITE_FOUR:
    case TRAINER_CLASS_CHAMPION:
    case TRAINER_CLASS_RIVAL:
    case TRAINER_CLASS_MAGMA_LEADER:
    case TRAINER_CLASS_AQUA_LEADER:
        return TRUE;
    default:
        return FALSE;
    }
}

void Achievement_ClearBattleData(void)
{
    memset(&sBattleData, 0, sizeof(sBattleData));
}

// Called from CancelerPPDeduction (src/battle_move_resolution.c).
void Achievement_RecordMoveUsed(u8 partyIndex, enum Move move, u32 typeBit, u32 movePosition, bool8 isSTAB, bool8 isSetupMove)
{
    if (partyIndex >= PARTY_SIZE)
        return;

    sBattleData.typesUsed |= typeBit;
    sBattleData.slotsThatActed |= 1 << partyIndex;
    if (movePosition < MAX_MON_MOVES)
        sBattleData.moveSlotsUsed[partyIndex] |= 1 << movePosition;

    if (isSTAB)
        sBattleData.stabUsed = TRUE;

    if (sBattleData.prevPlayerMove != MOVE_NONE && sBattleData.prevPlayerMove == move)
        sBattleData.repeatedMove = TRUE;
    sBattleData.prevPlayerMove = move;

    // For setupThenKo: whether this move follows this battler's own setup
    // move. Read before pendingSetupBattler is overwritten below.
    sBattleData.currentMoveFollowsSetup = (sBattleData.pendingSetupBattler == (u8)(partyIndex + 1));

    if (isSetupMove)
    {
        sBattleData.setupMoveUsed = TRUE;
        sBattleData.pendingSetupBattler = partyIndex + 1;
    }
    else
    {
        sBattleData.pendingSetupBattler = 0;
    }
}

void Achievement_RecordSuperEffectiveHit(void)
{
    sBattleData.superEffectiveUsed = TRUE;
}

void Achievement_RecordCriticalHit(void)
{
    sBattleData.critLanded = TRUE;
}

void Achievement_RecordStatusInflicted(u8 statusBit)
{
    sBattleData.statusesInflicted |= statusBit;
}

// Called from SetValuesOnFaint's opponent-faint branch (src/battle_util.c).
void Achievement_RecordOpposingFaint(enum BattlerId victimBattler, enum BattlerId attackerBattler)
{
    u8 partyIndex;

    if (attackerBattler == victimBattler
     && (gBattleMons[victimBattler].status1 & (STATUS1_POISON | STATUS1_TOXIC_POISON | STATUS1_BURN | STATUS1_FROSTBITE)))
    {
        if (sBattleData.statusKoCount < 255)
            sBattleData.statusKoCount++;
        return;
    }

    partyIndex = gBattlerPartyIndexes[attackerBattler];
    if (partyIndex >= PARTY_SIZE)
        return;

    if (sBattleData.kosPerSlot[partyIndex] < 255)
        sBattleData.kosPerSlot[partyIndex]++;

    sBattleData.lastThreeKoSlots[0] = sBattleData.lastThreeKoSlots[1];
    sBattleData.lastThreeKoSlots[1] = sBattleData.lastThreeKoSlots[2];
    sBattleData.lastThreeKoSlots[2] = partyIndex + 1;

    if (GetMovePriority(gLastMoves[attackerBattler]) > 0)
        sBattleData.priorityKo = TRUE;

    if (sBattleData.currentMoveFollowsSetup)
        sBattleData.setupThenKo = TRUE;
}

static u32 CountSetBits(u32 value)
{
    u32 count = 0;

    while (value)
    {
        count += value & 1;
        value >>= 1;
    }

    return count;
}

static u8 CountConsciousPartyMons(struct Pokemon *party, u8 count)
{
    u8 i, conscious = 0;

    for (i = 0; i < count; i++)
    {
        if (GetMonData(&party[i], MON_DATA_HP) > 0)
            conscious++;
    }

    return conscious;
}

// Called from HandleEndTurn_BattleWon; the caller excludes link and recorded
// battles.
void Achievement_CheckBattleMilestones(void)
{
    bool8 isTrainerBattle = (gBattleTypeFlags & BATTLE_TYPE_TRAINER) != 0;
    bool8 isMajorBattle = Achievement_IsMajorBattle();
    bool8 weatherActiveOnWin = (gBattleWeather != 0);
    u8 playerCount = gPartiesCount[B_TRAINER_PLAYER];
    u8 consciousCount = CountConsciousPartyMons(gParties[B_TRAINER_PLAYER], playerCount);
    u8 i;

    if (sBattleData.critLanded)
        Achievement_TryComplete(ACHIEVEMENT_BATTLE_CRITICAL_SUCCESS);

    if (sBattleData.superEffectiveUsed)
        Achievement_TryComplete(ACHIEVEMENT_BATTLE_TYPE_ADVANTAGE);

    // Full-team requirements below keep these from being trivial against
    // trainers who field only one or two Pokemon.
    if (isTrainerBattle && gBattleResults.playerSwitchesCounter == 0
        && gPartiesCount[B_TRAINER_OPPONENT_A] == PARTY_SIZE)
    {
        Achievement_TryComplete(ACHIEVEMENT_BATTLE_CLEAN_SWEEP);
        if (isMajorBattle)
            Achievement_TryComplete(ACHIEVEMENT_BATTLE_PERFECT_SWEEP);
    }

    if (isTrainerBattle && !gBattleResults.playerMonWasDamaged)
    {
        // Untouchable needs only a 3+ Pokemon opponent so it stays earnable
        // against major battles that don't field a full 6.
        if (gPartiesCount[B_TRAINER_OPPONENT_A] == PARTY_SIZE)
            Achievement_TryComplete(ACHIEVEMENT_BATTLE_NO_DAMAGE);
        if (isMajorBattle && gPartiesCount[B_TRAINER_OPPONENT_A] >= 3)
            Achievement_TryComplete(ACHIEVEMENT_BATTLE_UNTOUCHABLE);
    }

    if (sBattleData.statusesInflicted != 0)
        Achievement_TryComplete(ACHIEVEMENT_BATTLE_STATUS_SPECIALIST);
    if (CountSetBits(sBattleData.statusesInflicted) >= 3)
        Achievement_TryComplete(ACHIEVEMENT_BATTLE_STATUS_MASTER);

    if (weatherActiveOnWin)
    {
        Achievement_TryComplete(ACHIEVEMENT_BATTLE_WEATHER_REPORT);
        if (isMajorBattle)
            Achievement_TryComplete(ACHIEVEMENT_BATTLE_WEATHER_MASTER);
    }

    if (sBattleData.setupMoveUsed)
        Achievement_TryComplete(ACHIEVEMENT_BATTLE_SETUP_SWEEP);
    if (sBattleData.setupThenKo)
        Achievement_TryComplete(ACHIEVEMENT_BATTLE_ONE_TURN_FINISH);
    if (sBattleData.priorityKo)
        Achievement_TryComplete(ACHIEVEMENT_BATTLE_PRIORITY_MATTERS);

    if (gBattleResults.playerSwitchesCounter == 0)
    {
        for (i = 0; i < PARTY_SIZE; i++)
        {
            if (sBattleData.kosPerSlot[i] >= 3)
            {
                Achievement_TryComplete(ACHIEVEMENT_BATTLE_SPEED_DEMON);
                break;
            }
        }
    }

    if (isMajorBattle && gBattleResults.battleTurnCounter >= 30)
        Achievement_TryComplete(ACHIEVEMENT_BATTLE_ATTRITION);

    if (isMajorBattle && gBattleResults.playerFaintCounter == 0
     && gPartiesCount[B_TRAINER_OPPONENT_A] == PARTY_SIZE)
        Achievement_TryComplete(ACHIEVEMENT_BATTLE_STRATEGIC_VICTORY);

    // Full player team, so "half the team" is never one faint out of two.
    if (isTrainerBattle && playerCount == PARTY_SIZE && gBattleResults.playerFaintCounter * 2 >= playerCount)
        Achievement_TryComplete(ACHIEVEMENT_BATTLE_REVERSE_SWEEP);

    if (isTrainerBattle && GetTrainerClassFromId(TRAINER_BATTLE_PARAM.opponentA) == TRAINER_CLASS_CHAMPION
     && sBattleData.slotsThatActed != 0 && CountSetBits(sBattleData.slotsThatActed) <= 3)
        Achievement_TryComplete(ACHIEVEMENT_BATTLE_CHAMPION_TACTICIAN);

    // At least 3 members must act, else a 1v1 sweep trivially clears it.
    if (isMajorBattle && CountSetBits(sBattleData.slotsThatActed) >= 3)
    {
        bool8 allActedUsedTwoMoves = TRUE;

        for (i = 0; i < PARTY_SIZE; i++)
        {
            if ((sBattleData.slotsThatActed & (1 << i)) && CountSetBits(sBattleData.moveSlotsUsed[i]) < 2)
            {
                allActedUsedTwoMoves = FALSE;
                break;
            }
        }

        if (allActedUsedTwoMoves)
            Achievement_TryComplete(ACHIEVEMENT_BATTLE_MOVE_VARIETY);
    }

    if (isTrainerBattle && !sBattleData.repeatedMove
     && gPartiesCount[B_TRAINER_OPPONENT_A] == PARTY_SIZE)
        Achievement_TryComplete(ACHIEVEMENT_BATTLE_NO_REPEATS);

    // Every party member at least 5 levels below opponentA's highest-level
    // Pokemon. opponentB is ignored.
    if (isMajorBattle && playerCount != 0)
    {
        u8 maxEnemyLevel = 0;
        u8 enemyCount = gPartiesCount[B_TRAINER_OPPONENT_A];
        bool8 allUnderleveled = TRUE;

        for (i = 0; i < enemyCount; i++)
        {
            u8 level = GetMonData(&gParties[B_TRAINER_OPPONENT_A][i], MON_DATA_LEVEL);
            if (level > maxEnemyLevel)
                maxEnemyLevel = level;
        }

        for (i = 0; i < playerCount; i++)
        {
            u8 level = GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_LEVEL);
            if (level + 5 > maxEnemyLevel)
            {
                allUnderleveled = FALSE;
                break;
            }
        }

        if (allUnderleveled)
            Achievement_TryComplete(ACHIEVEMENT_BATTLE_AGAINST_THE_ODDS);
    }

    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (sBattleData.moveSlotsUsed[i] == ((1 << MAX_MON_MOVES) - 1))
        {
            Achievement_TryComplete(ACHIEVEMENT_BATTLE_FOUR_MOVE_PHILOSOPHER);
            break;
        }
    }

    if (isTrainerBattle && !sBattleData.stabUsed
     && gPartiesCount[B_TRAINER_OPPONENT_A] == PARTY_SIZE)
        Achievement_TryComplete(ACHIEVEMENT_BATTLE_NO_STAB_NEEDED);

    if (isMajorBattle && CountSetBits(sBattleData.typesUsed) >= 4)
        Achievement_TryComplete(ACHIEVEMENT_BATTLE_COVERAGE_ENJOYER);

    if (sBattleData.statusKoCount >= 2)
        Achievement_TryComplete(ACHIEVEMENT_BATTLE_STATUS_HOARDER);

    if (sBattleData.lastThreeKoSlots[0] != 0 && sBattleData.lastThreeKoSlots[1] != 0 && sBattleData.lastThreeKoSlots[2] != 0
     && sBattleData.lastThreeKoSlots[0] != sBattleData.lastThreeKoSlots[1]
     && sBattleData.lastThreeKoSlots[1] != sBattleData.lastThreeKoSlots[2]
     && sBattleData.lastThreeKoSlots[0] != sBattleData.lastThreeKoSlots[2])
        Achievement_TryComplete(ACHIEVEMENT_BATTLE_THREE_PUNCH_FINISH);

    if (sBattleData.slotsThatActed == ((1 << PARTY_SIZE) - 1))
        Achievement_TryComplete(ACHIEVEMENT_BATTLE_TEAM_PLAYER);

    // Full party required, else one conscious Pokemon is trivial.
    if (consciousCount == 1 && playerCount == PARTY_SIZE)
    {
        Achievement_TryComplete(ACHIEVEMENT_BATTLE_COMEBACK_KID);

        for (i = 0; i < playerCount; i++)
        {
            u32 hp = GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_HP);

            if (hp > 0)
            {
                u32 maxHp = GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_MAX_HP);

                if (maxHp != 0 && hp * 10 <= maxHp)
                    Achievement_TryComplete(ACHIEVEMENT_BATTLE_LAST_ONE_STANDING);
                break;
            }
        }
    }
}

// ---- Team Building & Composition (category L) ---------------------------
//
// Per-run state lives in struct AchievementRunData (include/global.h). Species
// sets are tracked by species ID, not individual identity (personality/OT).

bool8 Achievement_IsGymBattle(void)
{
    if (!(gBattleTypeFlags & BATTLE_TYPE_TRAINER))
        return FALSE;

    return GetTrainerClassFromId(TRAINER_BATTLE_PARAM.opponentA) == TRAINER_CLASS_LEADER;
}

static bool8 Achievement_PartyAllHaveType(struct Pokemon *party, u8 count, u32 type)
{
    u8 i;

    for (i = 0; i < count; i++)
    {
        enum Species species = GetMonData(&party[i], MON_DATA_SPECIES);

        if (gSpeciesInfo[species].types[0] != type && gSpeciesInfo[species].types[1] != type)
            return FALSE;
    }

    return TRUE;
}

// Returns the lowest-numbered type shared by every one of the count members
// (either of their up to two types), else NUMBER_OF_MON_TYPES. Starts at
// TYPE_NONE + 1 so two single-type members don't spuriously "share" TYPE_NONE
// via their unused second type slot.
static u8 Achievement_ComputePartyMonoType(struct Pokemon *party, u8 count)
{
    u32 type;

    if (count == 0)
        return NUMBER_OF_MON_TYPES;

    for (type = TYPE_NONE + 1; type < NUMBER_OF_MON_TYPES; type++)
    {
        if (Achievement_PartyAllHaveType(party, count, type))
            return (u8)type;
    }

    return NUMBER_OF_MON_TYPES;
}

// Union of every type held by the party. TYPE_NONE itself is never set, so a
// party of all single-type members doesn't inflate CountSetBits() of this.
static u32 Achievement_PartyTypeComposition(struct Pokemon *party, u8 count)
{
    u32 mask = 0;
    u8 i;

    for (i = 0; i < count; i++)
    {
        enum Species species = GetMonData(&party[i], MON_DATA_SPECIES);

        mask |= 1u << gSpeciesInfo[species].types[0];
        if (gSpeciesInfo[species].types[1] != TYPE_NONE)
            mask |= 1u << gSpeciesInfo[species].types[1];
    }

    return mask;
}

static bool8 Achievement_AllTypesDisjoint(struct Pokemon *party, u8 count)
{
    u8 i, j;

    if (count == 0)
        return FALSE;

    for (i = 0; i < count; i++)
    {
        enum Species speciesI = GetMonData(&party[i], MON_DATA_SPECIES);

        for (j = i + 1; j < count; j++)
        {
            enum Species speciesJ = GetMonData(&party[j], MON_DATA_SPECIES);
            u8 k;

            for (k = 0; k < 2; k++)
            {
                enum Type typeI = gSpeciesInfo[speciesI].types[k];

                if (typeI == TYPE_NONE)
                    continue;
                if (typeI == gSpeciesInfo[speciesJ].types[0] || typeI == gSpeciesInfo[speciesJ].types[1])
                    return FALSE;
            }
        }
    }

    return TRUE;
}

static bool8 Achievement_AllPrimaryTypesDistinct(struct Pokemon *party, u8 count)
{
    u8 i, j;

    if (count == 0)
        return FALSE;

    for (i = 0; i < count; i++)
    {
        enum Species speciesI = GetMonData(&party[i], MON_DATA_SPECIES);

        for (j = i + 1; j < count; j++)
        {
            enum Species speciesJ = GetMonData(&party[j], MON_DATA_SPECIES);

            if (gSpeciesInfo[speciesI].types[0] == gSpeciesInfo[speciesJ].types[0])
                return FALSE;
        }
    }

    return TRUE;
}

static bool8 Achievement_AllPrimaryEggGroupsDistinct(struct Pokemon *party, u8 count)
{
    u8 i, j;

    for (i = 0; i < count; i++)
    {
        enum Species speciesI = GetMonData(&party[i], MON_DATA_SPECIES);

        for (j = i + 1; j < count; j++)
        {
            enum Species speciesJ = GetMonData(&party[j], MON_DATA_SPECIES);

            if (gSpeciesInfo[speciesI].eggGroups[0] == gSpeciesInfo[speciesJ].eggGroups[0])
                return FALSE;
        }
    }

    return TRUE;
}

static u8 Achievement_HighestLevelPartySlot(struct Pokemon *party, u8 count)
{
    u8 i, bestSlot = 0, bestLevel = 0;

    for (i = 0; i < count; i++)
    {
        u8 level = GetMonData(&party[i], MON_DATA_LEVEL);

        if (level > bestLevel)
        {
            bestLevel = level;
            bestSlot = i;
        }
    }

    return bestSlot;
}

// No Ace: "highest-level Pokemon" spans every PC box, not just the party.
static bool8 Achievement_HighestLevelMonIsOutsideParty(struct Pokemon *party, u8 count)
{
    u8 maxPartyLevel = 0;
    u8 i, box, slot;

    for (i = 0; i < count; i++)
    {
        u8 level = GetMonData(&party[i], MON_DATA_LEVEL);
        if (level > maxPartyLevel)
            maxPartyLevel = level;
    }

    for (box = 0; box < TOTAL_BOXES_COUNT; box++)
    {
        for (slot = 0; slot < IN_BOX_COUNT; slot++)
        {
            if (GetBoxMonDataAt(box, slot, MON_DATA_SPECIES) != SPECIES_NONE
             && GetBoxMonDataAt(box, slot, MON_DATA_LEVEL) > maxPartyLevel)
                return TRUE;
        }
    }

    return FALSE;
}

static enum Species Achievement_GetEvolutionRoot(enum Species species)
{
    enum Species pre;

    while ((pre = GetSpeciesPreEvolution(species)) != SPECIES_NONE)
        species = pre;

    return species;
}

// Whether any evolution family has at least minSize members in the party.
static bool8 Achievement_HasEvolutionFamilyOfSize(struct Pokemon *party, u8 count, u8 minSize)
{
    u8 i, j;

    for (i = 0; i < count; i++)
    {
        enum Species rootI = Achievement_GetEvolutionRoot(GetMonData(&party[i], MON_DATA_SPECIES));
        u8 familyCount = 1;

        for (j = i + 1; j < count; j++)
        {
            enum Species rootJ = Achievement_GetEvolutionRoot(GetMonData(&party[j], MON_DATA_SPECIES));
            if (rootJ == rootI)
                familyCount++;
        }

        if (familyCount >= minSize)
            return TRUE;
    }

    return FALSE;
}

static u32 Achievement_PartyBaseStatTotal(struct Pokemon *party, u8 count)
{
    u32 sum = 0;
    u8 i;

    for (i = 0; i < count; i++)
        sum += GetSpeciesBaseStatTotal(GetMonData(&party[i], MON_DATA_SPECIES));

    return sum;
}

// Snapshots into a fixed PARTY_SIZE-length buffer, SPECIES_NONE-padded, so
// the set helpers below never need to carry a separate count alongside it.
static void Achievement_SnapshotPartySpecies(struct Pokemon *party, u8 count, u16 *dest)
{
    u8 i;

    for (i = 0; i < PARTY_SIZE; i++)
        dest[i] = (i < count) ? GetMonData(&party[i], MON_DATA_SPECIES) : SPECIES_NONE;
}

// Set equality (order-independent, SPECIES_NONE padding ignored). Duplicate
// species count as one set member.
static bool8 Achievement_SpeciesSetsEqual(const u16 *a, const u16 *b)
{
    u8 i, j;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (a[i] == SPECIES_NONE)
            continue;
        for (j = 0; j < PARTY_SIZE; j++)
        {
            if (b[j] == a[i])
                break;
        }
        if (j == PARTY_SIZE)
            return FALSE;
    }

    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (b[i] == SPECIES_NONE)
            continue;
        for (j = 0; j < PARTY_SIZE; j++)
        {
            if (a[j] == b[i])
                break;
        }
        if (j == PARTY_SIZE)
            return FALSE;
    }

    return TRUE;
}

static bool8 Achievement_SpeciesSetsDisjoint(const u16 *a, const u16 *b)
{
    u8 i, j;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (a[i] == SPECIES_NONE)
            continue;
        for (j = 0; j < PARTY_SIZE; j++)
        {
            if (b[j] == a[i])
                return FALSE;
        }
    }

    return TRUE;
}

// Count of species in cur that aren't present in prevSet, for Rebuild.
static u8 Achievement_CountSpeciesNotInSet(const u16 *cur, const u16 *prevSet)
{
    u8 i, j, diff = 0;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        bool8 found = FALSE;

        if (cur[i] == SPECIES_NONE)
            continue;

        for (j = 0; j < PARTY_SIZE; j++)
        {
            if (prevSet[j] == cur[i])
            {
                found = TRUE;
                break;
            }
        }

        if (!found)
            diff++;
    }

    return diff;
}

static bool8 Achievement_AllDistinctU16(const u16 *arr, u8 count)
{
    u8 i, j;

    for (i = 0; i < count; i++)
    {
        for (j = i + 1; j < count; j++)
        {
            if (arr[i] == arr[j])
                return FALSE;
        }
    }

    return TRUE;
}

static void Achievement_RecordMajorBattleSpecies(enum Species species)
{
    struct AchievementRunData *runData = &gSaveBlock1Ptr->achievementRunData;
    u8 i;

    if (species == SPECIES_NONE)
        return;

    for (i = 0; i < runData->majorBattleSpeciesCount; i++)
    {
        if (runData->majorBattleSpecies[i] == species)
            return;
    }

    if (runData->majorBattleSpeciesCount < ARRAY_COUNT(runData->majorBattleSpecies))
    {
        runData->majorBattleSpecies[runData->majorBattleSpeciesCount] = species;
        runData->majorBattleSpeciesCount++;
    }
}

static bool8 Achievement_WasRecentlyObtained(struct AchievementRunData *runData, u32 personality)
{
    u8 validCount = (runData->recentlyObtainedCount < ARRAY_COUNT(runData->recentlyObtainedPersonality))
                  ? runData->recentlyObtainedCount
                  : ARRAY_COUNT(runData->recentlyObtainedPersonality);
    u8 i;

    for (i = 0; i < validCount; i++)
    {
        if (runData->recentlyObtainedPersonality[i] == personality)
            return TRUE;
    }

    return FALSE;
}

// Called from GiveCapturedMonToPlayer (src/pokemon.c) and Task_EggHatch
// (src/egg_hatch.c).
void Achievement_RecordMonObtained(u32 personality)
{
    struct AchievementRunData *runData = &gSaveBlock1Ptr->achievementRunData;
    u8 slot = runData->recentlyObtainedCount % ARRAY_COUNT(runData->recentlyObtainedPersonality);

    runData->recentlyObtainedPersonality[slot] = personality;
    if (runData->recentlyObtainedCount < 0xFF)
        runData->recentlyObtainedCount++;

    // One of Each: obtaining a Pokemon is the only way the distinct-species
    // count rises. The IsCompleted guard skips the storage scan once earned.
    if (!Achievement_IsCompleted(ACHIEVEMENT_COLLECT_ONE_OF_EACH)
     && Achievement_CountDistinctOwnedSpecies(gParties[B_TRAINER_PLAYER], gPartiesCount[B_TRAINER_PLAYER], 10) >= 10)
        Achievement_TryComplete(ACHIEVEMENT_COLLECT_ONE_OF_EACH);
}

// Called from HandleEndTurn_BattleWon after Achievement_CheckBattleMilestones
// (never link/recorded). Mono-type discipline is tracked on every trainer win,
// not just major ones.
void Achievement_CheckTeamMilestones(void)
{
    struct AchievementRunData *runData = &gSaveBlock1Ptr->achievementRunData;
    bool8 isTrainerBattle = (gBattleTypeFlags & BATTLE_TYPE_TRAINER) != 0;
    bool8 isMajorBattle = Achievement_IsMajorBattle();
    bool8 isGymBattle = Achievement_IsGymBattle();
    u8 playerCount = gPartiesCount[B_TRAINER_PLAYER];
    struct Pokemon *party = gParties[B_TRAINER_PLAYER];
    u8 monoType = Achievement_ComputePartyMonoType(party, playerCount);
    u8 i;

    if (isTrainerBattle && !runData->monoTypeBroken)
    {
        // Mono Type mode fixes the committed type up front. Otherwise the
        // first battle's lowest shared type is locked in. Either way, later
        // parties only need to still contain the locked type: a dual-type
        // evolution (e.g. Torchic -> Combusken) can make a lower-numbered
        // type the party's "computed" mono type without breaking the run.
        if (runData->monoTypeType == TYPE_NONE)
        {
            if (MonoType_IsEnabled())
                runData->monoTypeType = MonoType_GetType();
            else if (monoType != NUMBER_OF_MON_TYPES)
                runData->monoTypeType = monoType;
            else
                runData->monoTypeBroken = TRUE;
        }

        if (runData->monoTypeType != TYPE_NONE
         && !Achievement_PartyAllHaveType(party, playerCount, runData->monoTypeType))
            runData->monoTypeBroken = TRUE;
    }

    if (isGymBattle)
    {
        u16 curSpecies[PARTY_SIZE];
        u32 composition = Achievement_PartyTypeComposition(party, playerCount);
        u8 highestSlot = Achievement_HighestLevelPartySlot(party, playerCount);

        if (runData->gymBattlesWon < 255)
            runData->gymBattlesWon++;

        Achievement_SnapshotPartySpecies(party, playerCount, curSpecies);

        if (monoType != NUMBER_OF_MON_TYPES)
        {
            // Mono Type Trial needs a full party; monoTypeGymsCleared
            // (One Type Journey) does not.
            if (playerCount == PARTY_SIZE)
                Achievement_TryComplete(ACHIEVEMENT_TEAM_MONO_TYPE_TRIAL);
            if (runData->monoTypeGymsCleared < 255)
                runData->monoTypeGymsCleared++;
        }
        if (runData->monoTypeGymsCleared >= 4)
            Achievement_TryComplete(ACHIEVEMENT_TEAM_ONE_TYPE_JOURNEY);

        if (playerCount != 0 && !(sBattleData.slotsThatActed & (1 << highestSlot)))
            Achievement_TryComplete(ACHIEVEMENT_TEAM_UNDERSTUDY);

        if (Achievement_HighestLevelMonIsOutsideParty(party, playerCount))
            Achievement_TryComplete(ACHIEVEMENT_TEAM_NO_ACE);

        // A small party trivially has a low base stat total.
        if (playerCount == PARTY_SIZE && Achievement_PartyBaseStatTotal(party, playerCount) < 1800)
            Achievement_TryComplete(ACHIEVEMENT_TEAM_FEATHERWEIGHT);

        // Type Roulette: composition must differ from the previous Gym's.
        if (runData->gymBattlesWon == 1)
        {
            runData->prevGymTypeComposition = composition;
        }
        else
        {
            if (composition == runData->prevGymTypeComposition)
                runData->typeRouletteBroken = TRUE;
            runData->prevGymTypeComposition = composition;
        }

        // Same Six: species set must match the Gym 1 baseline every time.
        if (!runData->sameSixBaselineSet)
        {
            memcpy(runData->firstGymPartySpecies, curSpecies, sizeof(curSpecies));
            runData->sameSixBaselineSet = TRUE;
        }
        else if (!Achievement_SpeciesSetsEqual(runData->firstGymPartySpecies, curSpecies))
        {
            runData->sameSixBroken = TRUE;
        }

        // Rebuild: >=4 species new since the immediately preceding Gym.
        if (runData->prevGymSnapshotSet
         && Achievement_CountSpeciesNotInSet(curSpecies, runData->prevGymPartySpecies) >= 4)
            runData->rebuildAchieved = TRUE;
        memcpy(runData->prevGymPartySpecies, curSpecies, sizeof(curSpecies));
        runData->prevGymSnapshotSet = TRUE;

        // Radical Rebuild's baseline.
        if (runData->gymBattlesWon == 4)
        {
            memcpy(runData->gym4PartySpecies, curSpecies, sizeof(curSpecies));
            runData->gym4SnapshotSet = TRUE;
        }

        if (playerCount == 0 || sBattleData.slotsThatActed != ((1 << playerCount) - 1))
            runData->nobodyBenchedBroken = TRUE;

        // Ace Rotation: the party slot that landed the final KO, translated
        // to a species while the just-won battle's party is still current.
        if (sBattleData.lastThreeKoSlots[2] != 0 && runData->gymFinalKoCount < NUM_BADGES)
        {
            u8 koSlot = sBattleData.lastThreeKoSlots[2] - 1;

            if (koSlot < playerCount)
            {
                runData->gymFinalKoSpecies[runData->gymFinalKoCount] = GetMonData(&party[koSlot], MON_DATA_SPECIES);
                runData->gymFinalKoCount++;
            }
        }

        if (playerCount == PARTY_SIZE)
        {
            bool8 allFresh = TRUE;

            for (i = 0; i < PARTY_SIZE; i++)
            {
                u32 personality = GetMonData(&party[i], MON_DATA_PERSONALITY);

                if (!Achievement_WasRecentlyObtained(runData, personality))
                {
                    allFresh = FALSE;
                    break;
                }
            }

            if (allFresh)
                Achievement_TryComplete(ACHIEVEMENT_TEAM_FRESH_START);
        }
        // The "since the previous Gym" window always resets here, win or not.
        runData->recentlyObtainedCount = 0;

        if (runData->gymBattlesWon >= NUM_BADGES)
        {
            if (!runData->typeRouletteBroken)
                Achievement_TryComplete(ACHIEVEMENT_TEAM_TYPE_ROULETTE);
            if (runData->sameSixBaselineSet && !runData->sameSixBroken)
                Achievement_TryComplete(ACHIEVEMENT_TEAM_SAME_SIX);
            if (!runData->nobodyBenchedBroken)
                Achievement_TryComplete(ACHIEVEMENT_TEAM_NOBODY_BENCHED);
            if (runData->gymFinalKoCount >= NUM_BADGES && Achievement_AllDistinctU16(runData->gymFinalKoSpecies, NUM_BADGES))
                Achievement_TryComplete(ACHIEVEMENT_TEAM_ACE_ROTATION);
        }
    }

    if (isMajorBattle)
    {
        if (playerCount == PARTY_SIZE && Achievement_AllTypesDisjoint(party, playerCount))
            Achievement_TryComplete(ACHIEVEMENT_TEAM_NO_DUPLICATES);

        if (playerCount == PARTY_SIZE && monoType != NUMBER_OF_MON_TYPES)
            Achievement_TryComplete(ACHIEVEMENT_TEAM_SIX_OF_A_KIND);

        if (playerCount == PARTY_SIZE && Achievement_AllPrimaryEggGroupsDistinct(party, playerCount))
            Achievement_TryComplete(ACHIEVEMENT_TEAM_DIVERSE_ROOTS);

        if (Achievement_HasEvolutionFamilyOfSize(party, playerCount, 3))
            Achievement_TryComplete(ACHIEVEMENT_TEAM_LINK_IN_THE_CHAIN);

        if (sBattleData.slotsThatActed != 0 && runData->prevMajorBattleSlots != 0
         && (sBattleData.slotsThatActed & runData->prevMajorBattleSlots) == 0)
            Achievement_TryComplete(ACHIEVEMENT_TEAM_BENCHWARMER);
        runData->prevMajorBattleSlots = sBattleData.slotsThatActed;

        for (i = 0; i < PARTY_SIZE; i++)
        {
            if (sBattleData.slotsThatActed & (1 << i))
                Achievement_RecordMajorBattleSpecies(GetMonData(&party[i], MON_DATA_SPECIES));
        }

        if (runData->majorBattleSpeciesCount >= 12)
            Achievement_TryComplete(ACHIEVEMENT_TEAM_BOX_ROTATION);
        if (runData->majorBattleSpeciesCount >= 18)
            Achievement_TryComplete(ACHIEVEMENT_TEAM_DEEP_BENCH);
        if (runData->majorBattleSpeciesCount >= 24)
            Achievement_TryComplete(ACHIEVEMENT_TEAM_EVERYONE_GETS_A_TURN);
        if (runData->majorBattleSpeciesCount >= 30)
            Achievement_TryComplete(ACHIEVEMENT_TEAM_FULL_ROTATION);
    }
}

// Called from the tail of Achievement_CheckStoryMilestones. bstEverExceeded450
// is only sampled at these story checkpoints, so a party member held briefly
// between two checkpoints can be missed.
void Achievement_CheckPartyStateMilestones(void)
{
    struct AchievementRunData *runData = &gSaveBlock1Ptr->achievementRunData;
    struct Pokemon *party = gParties[B_TRAINER_PLAYER];
    u8 playerCount = gPartiesCount[B_TRAINER_PLAYER];
    u8 holdingItemCount = 0;
    u8 i;

    for (i = 0; i < playerCount; i++)
    {
        if (GetMonData(&party[i], MON_DATA_HELD_ITEM) != ITEM_NONE)
            holdingItemCount++;

        if (GetSpeciesBaseStatTotal(GetMonData(&party[i], MON_DATA_SPECIES)) > 450)
            runData->bstEverExceeded450 = TRUE;
    }

    if (playerCount == PARTY_SIZE && holdingItemCount == PARTY_SIZE)
        Achievement_TryComplete(ACHIEVEMENT_TEAM_WELL_EQUIPPED);
}

// Called from GameClear (src/post_battle_event_funcs.c); re-fires once per
// NG+ cycle.
void Achievement_CheckTeamCompletionMilestones(void)
{
    struct AchievementRunData *runData = &gSaveBlock1Ptr->achievementRunData;
    struct Pokemon *party = gParties[B_TRAINER_PLAYER];
    u8 playerCount = gPartiesCount[B_TRAINER_PLAYER];

    if (runData->monoTypeType != TYPE_NONE && !runData->monoTypeBroken)
    {
        Achievement_TryComplete(ACHIEVEMENT_TEAM_MONO_TYPE_CHAMPION);
        if (gSaveBlock1Ptr->difficulty == DIFFICULTY_HARD)
            Achievement_TryComplete(ACHIEVEMENT_TEAM_TRIAL_BY_FIRE);
    }

    if (runData->rebuildAchieved)
        Achievement_TryComplete(ACHIEVEMENT_TEAM_REBUILD);

    if (runData->gym4SnapshotSet)
    {
        u16 curSpecies[PARTY_SIZE];

        Achievement_SnapshotPartySpecies(party, playerCount, curSpecies);
        if (Achievement_SpeciesSetsDisjoint(curSpecies, runData->gym4PartySpecies))
            Achievement_TryComplete(ACHIEVEMENT_TEAM_RADICAL_REBUILD);
    }

    if (!runData->bstEverExceeded450)
        Achievement_TryComplete(ACHIEVEMENT_TEAM_UNDERDOG_RUN);

    if (Achievement_AllPrimaryTypesDistinct(party, playerCount))
        Achievement_TryComplete(ACHIEVEMENT_TEAM_DREAM_TEAM);

    if (CountSetBits(Achievement_PartyTypeComposition(party, playerCount)) >= 10)
        Achievement_TryComplete(ACHIEVEMENT_TEAM_BALANCED_ROSTER);
}

// ---- Exploration, Economy & Collection (category M) --------------------

// IncrementGameStat only adds 1; money stats need a variable amount.
static void Achievement_AddToGameStat(u8 index, u32 amount)
{
    u32 value = GetGameStat(index);

    if (0xFFFFFFFF - value < amount)
        value = 0xFFFFFFFF;
    else
        value += amount;

    SetGameStat(index, value);
}

// On the Road/Completionist Tourist reuse the existing town/city visit flags.
static const u16 sVisitedTownFlags[] =
{
    FLAG_VISITED_LITTLEROOT_TOWN,
    FLAG_VISITED_OLDALE_TOWN,
    FLAG_VISITED_DEWFORD_TOWN,
    FLAG_VISITED_LAVARIDGE_TOWN,
    FLAG_VISITED_FALLARBOR_TOWN,
    FLAG_VISITED_VERDANTURF_TOWN,
    FLAG_VISITED_PACIFIDLOG_TOWN,
    FLAG_VISITED_PETALBURG_CITY,
    FLAG_VISITED_SLATEPORT_CITY,
    FLAG_VISITED_MAUVILLE_CITY,
    FLAG_VISITED_RUSTBORO_CITY,
    FLAG_VISITED_FORTREE_CITY,
    FLAG_VISITED_LILYCOVE_CITY,
    FLAG_VISITED_MOSSDEEP_CITY,
    FLAG_VISITED_SOOTOPOLIS_CITY,
    FLAG_VISITED_EVER_GRANDE_CITY,
};

// Called from LoadCurrentMapData (src/overworld.c). Maps are keyed by
// (mapGroup, mapNum): mapNum alone collides across groups.
void Achievement_CheckExplorationMilestones(void)
{
    struct AchievementRunDataExt *runData = &gSaveBlock2Ptr->achievementRunDataExt;
    u16 key = ((u16)gSaveBlock1Ptr->location.mapGroup << 8) | (u8)gSaveBlock1Ptr->location.mapNum;
    u8 visitedTowns = 0;
    bool8 allTownsVisited = TRUE;
    u8 i;

    for (i = 0; i < runData->mapsVisitedCount; i++)
    {
        if (runData->mapsVisited[i] == key)
            break;
    }
    if (i == runData->mapsVisitedCount && runData->mapsVisitedCount < ARRAY_COUNT(runData->mapsVisited))
    {
        runData->mapsVisited[runData->mapsVisitedCount] = key;
        runData->mapsVisitedCount++;
    }

    if (runData->mapsVisitedCount >= 30)
        Achievement_TryComplete(ACHIEVEMENT_EXPLORE_FIRST_STEPS_ABROAD);
    if (runData->mapsVisitedCount >= 70)
        Achievement_TryComplete(ACHIEVEMENT_EXPLORE_OFF_THE_BEATEN_PATH);
    if (runData->mapsVisitedCount >= 100)
        Achievement_TryComplete(ACHIEVEMENT_EXPLORE_CARTOGRAPHER);

    for (i = 0; i < ARRAY_COUNT(sVisitedTownFlags); i++)
    {
        if (FlagGet(sVisitedTownFlags[i]))
            visitedTowns++;
        else
            allTownsVisited = FALSE;
    }

    if (visitedTowns >= 5)
        Achievement_TryComplete(ACHIEVEMENT_EXPLORE_ON_THE_ROAD);

    // "Before entering the League": the Champion is not yet beaten.
    if (allTownsVisited && !FlagGet(FLAG_IS_CHAMPION))
        Achievement_TryComplete(ACHIEVEMENT_EXPLORE_COMPLETIONIST_TOURIST);

    if (FlagGet(FLAG_SYS_POKEDEX_GET) && FlagGet(FLAG_SYS_MAP_GET) && FlagGet(FLAG_SYS_B_DASH))
        Achievement_TryComplete(ACHIEVEMENT_EXPLORE_NO_LOOSE_ENDS);

    // Map transitions double as a live-state sampling point.
    Achievement_CheckRecordsMilestones();
}

// Called from Achievement_CheckPokedexMilestones's FLAG_SET_SEEN branch.
// Unions the current map's encounter table across every time of day, since
// availability varies by time but "seen" does not. Land/water/rock/fishing only;
// hiddenMonsInfo (DexNav-only encounters) is excluded.
void Achievement_CheckLocalExpert(void)
{
    u16 headerId = GetCurrentMapWildMonHeaderId();
    enum Species scratch[32];
    u8 count = 0;
    u8 t, i;

    if (headerId == HEADER_NONE)
        return;

    for (t = 0; t < TIMES_OF_DAY_COUNT; t++)
    {
        const struct WildEncounterTypes *types = &gWildMonHeaders[headerId].encounterTypes[t];
        const struct WildPokemonInfo *infoTables[4];
        u8 areaCounts[4] = { NUM_LAND_MONS_ENCOUNTER_SLOTS, NUM_WATER_MONS_ENCOUNTER_SLOTS, NUM_ROCK_SMASH_MONS_ENCOUNTER_SLOTS, NUM_FISHING_MONS_ENCOUNTER_SLOTS };

        infoTables[0] = types->landMonsInfo;
        infoTables[1] = types->waterMonsInfo;
        infoTables[2] = types->rockSmashMonsInfo;
        infoTables[3] = types->fishingMonsInfo;

        for (i = 0; i < 4; i++)
        {
            const struct WildPokemonInfo *info = infoTables[i];
            u8 j;

            if (info == NULL || info->wildPokemon == NULL)
                continue;

            for (j = 0; j < areaCounts[i]; j++)
            {
                enum Species species = info->wildPokemon[j].species;
                u8 k;
                bool8 found = FALSE;

                if (species == SPECIES_NONE)
                    continue;

                for (k = 0; k < count; k++)
                {
                    if (scratch[k] == species)
                    {
                        found = TRUE;
                        break;
                    }
                }

                if (!found && count < ARRAY_COUNT(scratch))
                    scratch[count++] = species;
            }
        }
    }

    if (count == 0)
        return;

    for (i = 0; i < count; i++)
    {
        if (!GetSetPokedexFlagBySpecies(scratch[i], FLAG_GET_SEEN))
            return;
    }

    Achievement_TryComplete(ACHIEVEMENT_EXPLORE_LOCAL_EXPERT);
}

// Called from SetHiddenItemFlag (src/field_specials.c), once per item. Uses the
// lifetime profile counter; the SaveBlock1 game stat resets every new game.
void Achievement_CheckHiddenItemMilestones(void)
{
    u32 count;

    IncrementGameStat(GAME_STAT_HIDDEN_ITEMS_FOUND);

    if (gAchievementProfile.hiddenItemsFoundLifetime < 0xFFFF)
        gAchievementProfile.hiddenItemsFoundLifetime++;
    sAchievementProfileDirty = TRUE;
    count = gAchievementProfile.hiddenItemsFoundLifetime;

    if (count >= 20)
        Achievement_TryComplete(ACHIEVEMENT_EXPLORE_TREASURE_HUNTER);
    if (count >= 50)
        Achievement_TryComplete(ACHIEVEMENT_EXPLORE_TREASURE_HOARD);
}

// Called from GetInteractionScript's object-event branch
// (src/field_control_avatar.c). Uses the lifetime profile counter.
void Achievement_RecordNpcTalkedTo(void)
{
    u32 count;

    IncrementGameStat(GAME_STAT_NPCS_TALKED_TO);

    if (gAchievementProfile.npcsTalkedToLifetime < 0xFFFF)
        gAchievementProfile.npcsTalkedToLifetime++;
    sAchievementProfileDirty = TRUE;
    count = gAchievementProfile.npcsTalkedToLifetime;

    if (count >= 50)
        Achievement_TryComplete(ACHIEVEMENT_EXPLORE_TALK_TO_THE_LOCALS);
    if (count >= 150)
        Achievement_TryComplete(ACHIEVEMENT_EXPLORE_PEOPLE_PERSON);
}

// Called from BuyMenuSubtractMoney (src/shop.c). Uses lifetime profile
// counters; the SaveBlock1 game stats reset every new game.
void Achievement_RecordMoneySpent(u32 amountSpent)
{
    struct AchievementRunDataExt *runData = &gSaveBlock2Ptr->achievementRunDataExt;
    u32 shopCount;
    u32 spent;

    Achievement_AddToGameStat(GAME_STAT_MONEY_SPENT, amountSpent);

    if (gAchievementProfile.shopPurchasesLifetime < 0xFFFF)
        gAchievementProfile.shopPurchasesLifetime++;
    gAchievementProfile.moneySpentLifetime += amountSpent;
    sAchievementProfileDirty = TRUE;

    shopCount = gAchievementProfile.shopPurchasesLifetime;
    spent = gAchievementProfile.moneySpentLifetime;

    if (shopCount >= 1)
        Achievement_TryComplete(ACHIEVEMENT_ECONOMY_FIRST_PURCHASE);
    if (shopCount >= 50)
        Achievement_TryComplete(ACHIEVEMENT_ECONOMY_REGULAR_CUSTOMER);
    if (spent >= 100000)
        Achievement_TryComplete(ACHIEVEMENT_ECONOMY_BIG_SPENDER);
    if (spent >= 1000000)
        Achievement_TryComplete(ACHIEVEMENT_ECONOMY_WHALE);

    runData->shoppedSinceLastGym = TRUE;
}

// The sell-item AddMoney call in src/item_menu.c.
void Achievement_RecordItemSaleProceeds(u32 amount)
{
    u32 total;

    Achievement_AddToGameStat(GAME_STAT_ITEM_SALES_MONEY, amount);
    total = GetGameStat(GAME_STAT_ITEM_SALES_MONEY);

    if (total >= 20000)
        Achievement_TryComplete(ACHIEVEMENT_ECONOMY_TREASURE_PAYS);
}

// Every Bag pocket except Key Items.
static u8 Achievement_CountDistinctBagItems(void)
{
    u8 pocket;
    u16 count = 0;

    for (pocket = 0; pocket < POCKETS_COUNT; pocket++)
    {
        u16 slot;

        if (pocket == POCKET_KEY_ITEMS)
            continue;

        for (slot = 0; slot < gBagPockets[pocket].capacity; slot++)
        {
            if (gBagPockets[pocket].itemSlots[slot].itemId != ITEM_NONE)
                count++;
        }
    }

    return (count > 255) ? 255 : (u8)count;
}

// Called from AddBagItem (src/item.c) after a successful add.
void Achievement_CheckPackRatMilestone(void)
{
    if (Achievement_CountDistinctBagItems() >= 20)
        Achievement_TryComplete(ACHIEVEMENT_EXPLORE_PACK_RAT);
}

// Called from ObjectEventInteractionPickBerryTree (src/berry.c).
void Achievement_RecordBerryHarvest(void)
{
    IncrementGameStat(GAME_STAT_BERRIES_HARVESTED);

    if (GetGameStat(GAME_STAT_BERRIES_HARVESTED) >= 50)
        Achievement_TryComplete(ACHIEVEMENT_COLLECT_GREEN_THUMB);
}

// Called from both GAME_STAT_EVOLVED_POKEMON sites (src/evolution_scene.c).
void Achievement_CheckEvolutionCountMilestones(void)
{
    u32 count = GetGameStat(GAME_STAT_EVOLVED_POKEMON);

    if (count >= 10)
        Achievement_TryComplete(ACHIEVEMENT_COLLECT_EVOLUTIONARY_PATH);
    if (count >= 25)
        Achievement_TryComplete(ACHIEVEMENT_COLLECT_EVOLUTION_EXPERT);
}

// Called from GetEvolutionTargetSpecies's DO_EVO path (src/pokemon.c); the
// friendship gating lives at the call site.
void Achievement_RecordFriendshipEvolution(void)
{
    Achievement_TryComplete(ACHIEVEMENT_COLLECT_FRIENDSHIP_BLOSSOMS);
}

// Called from PokemonUseItemEffects's ITEM4_EVO_STONE case (src/pokemon.c);
// gating lives at the call site.
void Achievement_RecordStoneEvolution(void)
{
    Achievement_TryComplete(ACHIEVEMENT_COLLECT_STONE_AGE);
}

// Called from GiveCapturedMonToPlayer (src/pokemon.c) when
// gDexNavSpecies != SPECIES_NONE.
void Achievement_CheckDexNavCaptureMilestone(void)
{
    Achievement_TryComplete(ACHIEVEMENT_COLLECT_RARE_FIND);
}

// The GAME_STAT_FISHING_ENCOUNTERS increment in src/wild_encounter.c.
void Achievement_CheckFishingMilestone(void)
{
    if (GetGameStat(GAME_STAT_FISHING_ENCOUNTERS) >= 100)
        Achievement_TryComplete(ACHIEVEMENT_COLLECT_ANGLER);
}

// Called from HandleEndTurn_BattleWon after Achievement_CheckTeamMilestones.
// shoppedSinceLastGym mirrors Fresh Start's "since the last Gym" window.
void Achievement_CheckGymEconomyMilestones(void)
{
    struct AchievementRunDataExt *runData = &gSaveBlock2Ptr->achievementRunDataExt;

    if (Achievement_IsGymBattle())
    {
        if (GetMoney(&gSaveBlock1Ptr->money) >= 50000)
            Achievement_TryComplete(ACHIEVEMENT_ECONOMY_SAVE_YOUR_CHANGE);

        if (!runData->shoppedSinceLastGym)
        {
            Achievement_TryComplete(ACHIEVEMENT_ECONOMY_FRUGAL_TRAINER);
            if (runData->consecutiveGymsNoShopping < 255)
                runData->consecutiveGymsNoShopping++;
        }
        else
        {
            runData->consecutiveGymsNoShopping = 0;
        }

        if (runData->consecutiveGymsNoShopping >= 4)
            Achievement_TryComplete(ACHIEVEMENT_ECONOMY_NO_SHOPPING);

        // The window always resets here.
        runData->shoppedSinceLastGym = FALSE;
    }
}

// Called from GameClear (src/post_battle_event_funcs.c).
void Achievement_CheckEconomyCompletionMilestones(void)
{
    if (GetMoney(&gSaveBlock1Ptr->money) >= 500000)
        Achievement_TryComplete(ACHIEVEMENT_ECONOMY_INVESTOR);
}

// ---- Challenge Runs & Nuzlocke (category N) -----------------------------
//
// GAME_STAT_USED_POKECENTER is incremented at FldEff_PokecenterHeal
// (src/field_effect.c); gBattleResults.numHealingItemsUsed at BS_ItemRestoreHP
// (src/battle_script_commands.c). Vanilla declares both but never writes them.

// The ten New Game Settings that make a run harder. Debug Mode, Stat Editor
// and Level Cap Off are excluded: each sets achievementsBlocked
// (ApplyPendingNewGameSettings), so they never vary here. Nuzlocke and Draft
// are mutually exclusive. Rotation Mode counts: it costs switch control.
static u8 Achievement_CountChallengeModifiers(void)
{
    u8 count = 0;

    if (gSaveBlock1Ptr->nuzlockeModeEnabled)
        count++;
    if (Draft_IsEnabled())
        count++;
    if (MonoType_IsEnabled())
        count++;
    if (MonoGen_IsEnabled())
        count++;
    if (LimitedParty_IsEnabled())
        count++;
    if (RotationMode_IsEnabled())
        count++;
    if (gSaveBlock1Ptr->difficulty == DIFFICULTY_HARD)
        count++;
    if (FlagGet(FLAG_RANDOMIZE_MON))
        count++;
    if (FlagGet(FLAG_RANDOMIZE_TYPE))
        count++;
    if (FlagGet(FLAG_RANDOMIZE_MOVES))
        count++;

    return count;
}

// Symmetric to Achievement_HighestLevelPartySlot, for Scrappy.
static u8 Achievement_LowestLevelPartySlot(struct Pokemon *party, u8 count)
{
    u8 i, bestSlot = 0, bestLevel = 0xFF;

    for (i = 0; i < count; i++)
    {
        u8 level = GetMonData(&party[i], MON_DATA_LEVEL);

        if (level < bestLevel)
        {
            bestLevel = level;
            bestSlot = i;
        }
    }

    return bestSlot;
}

// Called from HandleEndTurn_BattleWon after Achievement_CheckGymEconomyMilestones
// (never link/recorded). Per-battle Challenge entries, the bookkeeping read at
// GameClear, and the battle-time category O entries.
void Achievement_CheckChallengeMilestones(void)
{
    struct AchievementRunData *runData = &gSaveBlock1Ptr->achievementRunData;
    struct AchievementRunDataExt *runDataExt = &gSaveBlock2Ptr->achievementRunDataExt;
    struct Pokemon *party = gParties[B_TRAINER_PLAYER];
    u8 playerCount = gPartiesCount[B_TRAINER_PLAYER];
    bool8 isTrainerBattle = (gBattleTypeFlags & BATTLE_TYPE_TRAINER) != 0;
    bool8 isMajorBattle = Achievement_IsMajorBattle();
    bool8 isGymBattle = Achievement_IsGymBattle();
    u8 i;

    if (playerCount > runData->highestPartySizeThisRun)
        runData->highestPartySizeThisRun = playerCount;

    // Fresh Faces/Never the Same Fight: every trainer win during an NG+ cycle.
    if (isTrainerBattle && gSaveBlock2Ptr->newGamePlus > 0)
    {
        if (runDataExt->trainersDefeatedThisCycle < 0xFFFF)
            runDataExt->trainersDefeatedThisCycle++;
        if (runDataExt->trainersDefeatedThisCycle >= 50)
            Achievement_TryComplete(ACHIEVEMENT_NG_PLUS_FRESH_FACES);

        if (gAchievementProfile.trainersDefeatedAcrossNgPlus < 0xFFFF)
            gAchievementProfile.trainersDefeatedAcrossNgPlus++;
        if (gAchievementProfile.trainersDefeatedAcrossNgPlus >= 300)
            Achievement_TryComplete(ACHIEVEMENT_NG_PLUS_NEVER_THE_SAME_FIGHT);
        sAchievementProfileDirty = TRUE;
    }

    if (isMajorBattle)
    {
        bool8 noBagItemsUsed = (gBattleResults.numHealingItemsUsed == 0 && gBattleResults.numRevivesUsed == 0);
        bool8 noHeldItems = TRUE;

        // Full party required, else both are trivial by accident.
        if (playerCount == PARTY_SIZE && gBattleResults.numHealingItemsUsed == 0)
            Achievement_TryComplete(ACHIEVEMENT_CHALLENGE_NO_HEALING_ITEMS);

        for (i = 0; i < playerCount; i++)
        {
            if (GetMonData(&party[i], MON_DATA_HELD_ITEM) != ITEM_NONE)
            {
                noHeldItems = FALSE;
                break;
            }
        }
        if (playerCount == PARTY_SIZE && noBagItemsUsed && noHeldItems)
            Achievement_TryComplete(ACHIEVEMENT_CHALLENGE_ITEMLESS_BATTLE);

        if (gSaveBlock2Ptr->optionsBattleStyle == OPTIONS_BATTLE_STYLE_SET)
            Achievement_TryComplete(ACHIEVEMENT_CHALLENGE_SET_IN_STONE);

        if (playerCount == 3)
            Achievement_TryComplete(ACHIEVEMENT_CHALLENGE_MINIMALIST);

        // No Freebies: did the starter act in this major battle? Sticky once
        // set. Gated on playerCount >= 2: before the first catch the starter
        // is forced to act, which would make this unwinnable.
        if (runData->starterPersonality != 0 && !runData->starterActedInMajorBattle
         && playerCount >= 2)
        {
            for (i = 0; i < playerCount; i++)
            {
                if ((sBattleData.slotsThatActed & (1 << i))
                 && GetMonData(&party[i], MON_DATA_PERSONALITY) == runData->starterPersonality)
                {
                    runData->starterActedInMajorBattle = TRUE;
                    break;
                }
            }
        }

        // Patchwork Team: no randomizer gate.
        if (playerCount == PARTY_SIZE && Achievement_AllMetLocationsDistinct(party, playerCount))
            Achievement_TryComplete(ACHIEVEMENT_RANDOMIZER_PATCHWORK_TEAM);

        if (Achievement_AnyRandomizerFlagSet())
        {
            if (playerCount == PARTY_SIZE && Achievement_AllPrimaryTypesDistinct(party, playerCount))
                Achievement_TryComplete(ACHIEVEMENT_RANDOMIZER_CHAOS_TEAM);
        }
    }

    if (isGymBattle)
    {
        // Random by Nature: any one randomizer flag.
        if (Achievement_AnyRandomizerFlagSet())
            Achievement_TryComplete(ACHIEVEMENT_RANDOMIZER_RANDOM_BY_NATURE);
    }
}

// Called right after Achievement_CheckChallengeMilestones. Every entry is
// gated on nuzlockeModeEnabled.
void Achievement_CheckNuzlockeMilestones(void)
{
    struct Pokemon *party = gParties[B_TRAINER_PLAYER];
    u8 playerCount = gPartiesCount[B_TRAINER_PLAYER];
    bool8 isGymBattle = Achievement_IsGymBattle();
    u8 i;

    if (!gSaveBlock1Ptr->nuzlockeModeEnabled)
        return;

    if (isGymBattle)
        Achievement_TryComplete(ACHIEVEMENT_NUZLOCKE_FIRST_GYM);

    // Close Call: any party member survived the battle below 10% HP.
    for (i = 0; i < playerCount; i++)
    {
        u32 hp = GetMonData(&party[i], MON_DATA_HP);

        if (hp > 0)
        {
            u32 maxHp = GetMonData(&party[i], MON_DATA_MAX_HP);

            if (maxHp != 0 && hp * 10 <= maxHp)
            {
                Achievement_TryComplete(ACHIEVEMENT_NUZLOCKE_CLOSE_CALL);
                break;
            }
        }
    }

    if (isGymBattle && playerCount != 0)
    {
        u8 lowestSlot = Achievement_LowestLevelPartySlot(party, playerCount);

        if (sBattleData.lastThreeKoSlots[2] != 0
         && (sBattleData.lastThreeKoSlots[2] - 1) == lowestSlot)
            Achievement_TryComplete(ACHIEVEMENT_NUZLOCKE_SCRAPPY);
    }
}

// Called from GameClear (src/post_battle_event_funcs.c); re-fires once per
// NG+ cycle.
void Achievement_CheckChallengeCompletionMilestones(void)
{
    struct AchievementRunData *runData = &gSaveBlock1Ptr->achievementRunData;
    u8 modifierCount = Achievement_CountChallengeModifiers();

    if (modifierCount >= 3)
        Achievement_TryComplete(ACHIEVEMENT_CHALLENGE_SELF_IMPOSED);

    // Nightmare Mode: Nuzlocke, HARD, all three randomizer flags, boosts off.
    if (gSaveBlock1Ptr->nuzlockeModeEnabled && gSaveBlock1Ptr->difficulty == DIFFICULTY_HARD
     && FlagGet(FLAG_RANDOMIZE_MON) && FlagGet(FLAG_RANDOMIZE_TYPE) && FlagGet(FLAG_RANDOMIZE_MOVES)
     && !gAchievementProfile.boostsEnabled)
        Achievement_TryComplete(ACHIEVEMENT_CHALLENGE_NIGHTMARE_MODE);

    if (!runData->boughtConsumableItem)
        Achievement_TryComplete(ACHIEVEMENT_CHALLENGE_NO_SHOPPING_RUN);

    if (GetGameStat(GAME_STAT_USED_POKECENTER) == 0)
        Achievement_TryComplete(ACHIEVEMENT_CHALLENGE_NO_CENTERS);

    if (gSaveBlock2Ptr->optionsBattleStyle == OPTIONS_BATTLE_STYLE_SET
     && gSaveBlock1Ptr->difficulty == DIFFICULTY_HARD)
        Achievement_TryComplete(ACHIEVEMENT_CHALLENGE_HARDCORE_SET);

    if (runData->highestPartySizeThisRun != 0 && runData->highestPartySizeThisRun <= 1)
        Achievement_TryComplete(ACHIEVEMENT_CHALLENGE_SOLO_JOURNEY);

    if (runData->starterPersonality != 0 && !runData->starterActedInMajorBattle)
        Achievement_TryComplete(ACHIEVEMENT_CHALLENGE_NO_FREEBIES);
}

// Same call site as above. Every entry here is gated on nuzlockeModeEnabled.
void Achievement_CheckNuzlockeCompletionMilestones(void)
{
    struct AchievementRunData *runData = &gSaveBlock1Ptr->achievementRunData;
    struct Pokemon *party = gParties[B_TRAINER_PLAYER];
    u8 playerCount = gPartiesCount[B_TRAINER_PLAYER];

    if (!gSaveBlock1Ptr->nuzlockeModeEnabled)
        return;

    if (runData->nuzlockeMonsLost == 0)
        Achievement_TryComplete(ACHIEVEMENT_NUZLOCKE_PERFECT);
    if (runData->nuzlockeMonsLost >= 5)
        Achievement_TryComplete(ACHIEVEMENT_NUZLOCKE_GRAVEYARD);

    // Nuzlocke Across Worlds/Chaos Survivor.
    if (Achievement_AnyRandomizerFlagSet())
    {
        Achievement_TryComplete(ACHIEVEMENT_NUZLOCKE_ACROSS_WORLDS);
        if (gSaveBlock1Ptr->difficulty == DIFFICULTY_HARD)
            Achievement_TryComplete(ACHIEVEMENT_NUZLOCKE_CHAOS_SURVIVOR);
    }
}

// Called from BuyMenuSubtractMoney (src/shop.c) for POCKET_ITEMS purchases.
void Achievement_RecordConsumableItemPurchase(void)
{
    gSaveBlock1Ptr->achievementRunData.boughtConsumableItem = TRUE;
}

// Called from RemoveFaintedMonsFromParty (src/overworld.c), once per Pokemon
// removed.
void Achievement_RecordNuzlockeMonLost(void)
{
    struct AchievementRunData *runData = &gSaveBlock1Ptr->achievementRunData;

    if (runData->nuzlockeMonsLost < 255)
        runData->nuzlockeMonsLost++;
}

// Called from ui_birch_case.c when the starter is granted. Personality
// survives evolution. 0 means "not recorded"; a real personality of 0 is a
// 1-in-4-billion case.
void Achievement_RecordStarterPersonality(u32 personality)
{
    gSaveBlock1Ptr->achievementRunData.starterPersonality = personality;
}

// Called from GiveCapturedMonToPlayer (src/pokemon.c).
// GAME_STAT_POKEMON_CAPTURES is a this-run count (reset every new game and NG+
// cycle) and already includes this catch.
void Achievement_CheckRandomizerCaptureMilestone(void)
{
    if (Achievement_AnyRandomizerFlagSet() && GetGameStat(GAME_STAT_POKEMON_CAPTURES) >= 25)
        Achievement_TryComplete(ACHIEVEMENT_RANDOMIZER_ROOKIE);
}

// ---- Streaks, Records & Collection Remainder (category P) --------------
//
// Only the trainer win streak has new persistent state (AchievementRunDataExt,
// SaveBlock2). Other entries read live party/box/game-stat state.

static u16 Achievement_SaturatingAddU16(u16 value, u8 amount)
{
    u32 sum = (u32)value + amount;
    return (sum > 0xFFFF) ? 0xFFFF : (u16)sum;
}

// Stops once `stopAt` distinct species are seen; `seen` only needs to cover
// the largest `stopAt`. Below the threshold this decrypts every box slot, so
// never call it on a hot path such as map transitions -- only from events that
// can change the count (Achievement_RecordMonObtained).
static u32 Achievement_CountDistinctOwnedSpecies(struct Pokemon *party, u8 playerCount, u32 stopAt)
{
    enum Species seen[16];
    u32 distinct = 0;
    u8 i, box, slot;

    if (stopAt > ARRAY_COUNT(seen))
        stopAt = ARRAY_COUNT(seen);

    for (i = 0; i < playerCount && distinct < stopAt; i++)
    {
        enum Species species = GetMonData(&party[i], MON_DATA_SPECIES);
        u32 j;
        bool8 alreadySeen = FALSE;

        if (species == SPECIES_NONE)
            continue;
        for (j = 0; j < distinct; j++)
        {
            if (seen[j] == species)
            {
                alreadySeen = TRUE;
                break;
            }
        }
        if (!alreadySeen)
            seen[distinct++] = species;
    }

    for (box = 0; box < TOTAL_BOXES_COUNT && distinct < stopAt; box++)
    {
        for (slot = 0; slot < IN_BOX_COUNT && distinct < stopAt; slot++)
        {
            enum Species species = GetBoxMonDataAt(box, slot, MON_DATA_SPECIES);
            u32 j;
            bool8 alreadySeen = FALSE;

            if (species == SPECIES_NONE)
                continue;
            for (j = 0; j < distinct; j++)
            {
                if (seen[j] == species)
                {
                    alreadySeen = TRUE;
                    break;
                }
            }
            if (!alreadySeen)
                seen[distinct++] = species;
        }
    }

    return distinct;
}

// BFS over an evolution family from `root` (from Achievement_GetEvolutionRoot),
// de-duplicating branching targets. The cap is well above the largest family.
#define ACHIEVEMENT_MAX_FAMILY_MEMBERS 16

static u8 Achievement_GetFamilyMembers(enum Species root, enum Species *membersOut)
{
    u8 count = 0;
    u8 head = 0;

    membersOut[count++] = root;

    while (head < count && count < ACHIEVEMENT_MAX_FAMILY_MEMBERS)
    {
        const struct Evolution *evolutions = GetSpeciesEvolutions(membersOut[head++]);
        u8 i;

        if (evolutions == NULL)
            continue;

        for (i = 0; evolutions[i].method != EVOLUTIONS_END && count < ACHIEVEMENT_MAX_FAMILY_MEMBERS; i++)
        {
            enum Species target = SanitizeSpeciesId(evolutions[i].targetSpecies);
            u8 j;
            bool8 alreadyPresent = FALSE;

            if (target == SPECIES_NONE)
                continue;

            for (j = 0; j < count; j++)
            {
                if (membersOut[j] == target)
                {
                    alreadyPresent = TRUE;
                    break;
                }
            }
            if (!alreadyPresent)
                membersOut[count++] = target;
        }
    }

    return count;
}

// Family Reunion only counts families with a branching evolution (Eevee,
// Wurmple) or a regional form (Meowth/Alolan Meowth); others complete on any
// single catch.
static bool8 Achievement_FamilyQualifiesForReunion(const enum Species *members, u8 count)
{
    u8 i;

    for (i = 0; i < count; i++)
    {
        const struct Evolution *evolutions = GetSpeciesEvolutions(members[i]);
        enum Species firstTarget = SPECIES_NONE;
        u8 j;

        if (SpeciesHasRegionalForm(members[i]))
            return TRUE;

        if (evolutions == NULL)
            continue;

        for (j = 0; evolutions[j].method != EVOLUTIONS_END; j++)
        {
            enum Species target = SanitizeSpeciesId(evolutions[j].targetSpecies);

            if (target == SPECIES_NONE)
                continue;
            if (firstTarget == SPECIES_NONE)
                firstTarget = target;
            else if (target != firstTarget)
                return TRUE; // split evolution: more than one distinct target
        }
    }

    return FALSE;
}

// TRUE for species the Legendary Collection counts: restricted legendary,
// sub-legendary, or mythical. Ultra Beasts and Paradox species carry their
// own SpeciesInfo flags and are deliberately excluded, even in the rare case
// a species also sets one of the counted flags.
static bool8 Achievement_IsCountedLegendary(enum Species species)
{
    species = SanitizeSpeciesId(species);

    if (gSpeciesInfo[species].isUltraBeast || gSpeciesInfo[species].isParadox)
        return FALSE;

    return gSpeciesInfo[species].isRestrictedLegendary
        || gSpeciesInfo[species].isSubLegendary
        || gSpeciesInfo[species].isMythical;
}

// Generated by tools/misc/make_legendary_family_table.py: every family whose
// base form is a designated legendary, members followed by a SPECIES_NONE
// terminator. Avoids a runtime walk of all NUM_SPECIES, which is slow enough
// to freeze the overworld on a map load.
#include "data/pokemon/legendary_families.h"

// Total legendary families in sLegendaryFamilies, or -- with mythicalOnly --
// only the mythical ones. With caughtOnly, only families that already have a
// caught member; a family is recorded once however many members are caught.
// A family whose species are all disabled by config counts as nothing.
static u32 Achievement_CountLegendaryFamilies(bool8 mythicalOnly, bool8 caughtOnly)
{
    u32 count = 0;
    u32 i = 0;

    while (i < ARRAY_COUNT(sLegendaryFamilies))
    {
        // The first enabled member classifies the family -- normally its base
        // form, which is what carries the family's mythical/legendary flags.
        enum Species classifier = SPECIES_NONE;
        bool8 anyCaught = FALSE;

        for (; i < ARRAY_COUNT(sLegendaryFamilies) && sLegendaryFamilies[i] != SPECIES_NONE; i++)
        {
            enum Species member = sLegendaryFamilies[i];

            if (!IsSpeciesEnabled(member))
                continue;
            if (classifier == SPECIES_NONE)
                classifier = member;
            if (!anyCaught && GetSetPokedexFlagBySpecies(member, FLAG_GET_CAUGHT))
                anyCaught = TRUE;
        }
        i++; // step over the family's terminator

        if (classifier == SPECIES_NONE)
            continue;
        if (mythicalOnly && !gSpeciesInfo[classifier].isMythical)
            continue;
        if (caughtOnly && !anyCaught)
            continue;

        count++;
    }

    return count;
}

// Targets for Mythical Menagerie/Legend of Legends: every designated family,
// not filtered by obtainability. Fixed at build time, so memoized.
static u32 Achievement_CountDesignatedLegendaryFamilies(void)
{
    static u32 sCached = 0;

    if (sCached == 0)
        sCached = Achievement_CountLegendaryFamilies(FALSE, FALSE);

    return sCached;
}

static u32 Achievement_CountDesignatedMythicalFamilies(void)
{
    static u32 sCached = 0;

    if (sCached == 0)
        sCached = Achievement_CountLegendaryFamilies(TRUE, FALSE);

    return sCached;
}

// Called from HandleSetPokedexFlag's FLAG_SET_CAUGHT branch (src/pokemon.c)
// with the newly caught species. "Register" means caught, not seen.
void Achievement_CheckFamilyMilestone(enum Species species)
{
    enum Species members[ACHIEVEMENT_MAX_FAMILY_MEMBERS];
    enum Species root = Achievement_GetEvolutionRoot(species);
    u8 count = Achievement_GetFamilyMembers(root, members);
    u8 i;

    if (!Achievement_FamilyQualifiesForReunion(members, count))
        return;

    for (i = 0; i < count; i++)
    {
        enum NationalDexOrder dexNum = SpeciesToNationalPokedexNum(members[i]);

        if (dexNum == NATIONAL_DEX_NONE || !GetSetPokedexFlagBySpecies(members[i], FLAG_GET_CAUGHT))
            return;
    }

    Achievement_TryComplete(ACHIEVEMENT_COLLECT_FAMILY_REUNION);
}

// TRUE if no other member of `justCaught`'s legendary family (different
// National Dex number, so alternate forms don't count) is caught, so each
// family is counted once. A species absent from the table is its own family.
static bool8 Achievement_LegendaryFamilyNewlyCaught(enum Species justCaught)
{
    enum NationalDexOrder selfDex = SpeciesToNationalPokedexNum(justCaught);
    u32 start = 0;
    u32 i, j;

    for (i = 0; i < ARRAY_COUNT(sLegendaryFamilies); i++)
    {
        bool8 isOwnFamily = FALSE;

        if (sLegendaryFamilies[i] != SPECIES_NONE)
            continue;

        // sLegendaryFamilies[start .. i) is one family.
        for (j = start; j < i && !isOwnFamily; j++)
        {
            if (IsSpeciesEnabled(sLegendaryFamilies[j])
             && SpeciesToNationalPokedexNum(sLegendaryFamilies[j]) == selfDex)
                isOwnFamily = TRUE;
        }

        if (isOwnFamily)
        {
            for (j = start; j < i; j++)
            {
                if (!IsSpeciesEnabled(sLegendaryFamilies[j])
                 || SpeciesToNationalPokedexNum(sLegendaryFamilies[j]) == selfDex)
                    continue;
                if (GetSetPokedexFlagBySpecies(sLegendaryFamilies[j], FLAG_GET_CAUGHT))
                    return FALSE;
            }
            return TRUE;
        }

        start = i + 1;
    }

    return TRUE;
}

// Category Z. The two Diamond entries test against the fixed species-data
// totals.
static void Achievement_EvaluateLegendaryMilestones(u32 familiesCaught, u32 mythicalCaught)
{
    if (familiesCaught >= 1)
        Achievement_TryComplete(ACHIEVEMENT_LEGENDARY_MYTH_CONFIRMED);
    if (familiesCaught >= 5)
        Achievement_TryComplete(ACHIEVEMENT_LEGENDARY_RARE_COMPANY);
    if (familiesCaught >= 15)
        Achievement_TryComplete(ACHIEVEMENT_LEGENDARY_LEGEND_SEEKER);
    if (familiesCaught >= 30)
        Achievement_TryComplete(ACHIEVEMENT_LEGENDARY_HALL_OF_LEGENDS);
    if (familiesCaught >= 50)
        Achievement_TryComplete(ACHIEVEMENT_LEGENDARY_LIVING_LEGEND);

    if (mythicalCaught >= Achievement_CountDesignatedMythicalFamilies())
        Achievement_TryComplete(ACHIEVEMENT_LEGENDARY_MYTHICAL_MENAGERIE);
    if (familiesCaught >= Achievement_CountDesignatedLegendaryFamilies())
        Achievement_TryComplete(ACHIEVEMENT_LEGENDARY_LEGEND_OF_LEGENDS);
}

// One-shot recompute of legendaryFamiliesCaught from existing caught flags,
// for saves made before this counter existed. Also called from
// LoadCurrentMapData (src/overworld.c) so a save that never catches another
// legendary still gets backfilled.
void Achievement_BackfillLegendaryFamilies(void)
{
    struct AchievementRunDataExt *runDataExt = &gSaveBlock2Ptr->achievementRunDataExt;
    u32 caught;

    if (runDataExt->legendaryCountBackfilled)
        return;

    caught = Achievement_CountLegendaryFamilies(FALSE, TRUE);
    runDataExt->legendaryFamiliesCaught = (caught > 0xFF) ? 0xFF : caught;
    runDataExt->legendaryCountBackfilled = TRUE;

    Achievement_EvaluateLegendaryMilestones(runDataExt->legendaryFamiliesCaught,
                                            Achievement_CountLegendaryFamilies(TRUE, TRUE));
}

// Called from HandleSetPokedexFlagBySpecies's FLAG_SET_CAUGHT branch
// (src/pokemon.c); the caught flag is already set.
void Achievement_CheckLegendaryMilestones(enum Species species)
{
    struct AchievementRunDataExt *runDataExt = &gSaveBlock2Ptr->achievementRunDataExt;

    species = SanitizeSpeciesId(species);

    if (!runDataExt->legendaryCountBackfilled)
    {
        // The backfill counts from the caught flags, which already include
        // this catch, so it fully accounts for it.
        Achievement_BackfillLegendaryFamilies();
        return;
    }

    if (!Achievement_IsCountedLegendary(species))
        return;
    if (!Achievement_LegendaryFamilyNewlyCaught(species))
        return;

    if (runDataExt->legendaryFamiliesCaught < 0xFF)
        runDataExt->legendaryFamiliesCaught++;

    Achievement_EvaluateLegendaryMilestones(runDataExt->legendaryFamiliesCaught,
                                            Achievement_CountLegendaryFamilies(TRUE, TRUE));
}

// Called from EmporiumBufferRewardItem's win branch (src/battle_emporium.c).
// rewardIndex is the global reward row index (VAR_EMPORIUM_REWARD).
void Achievement_OnEmporiumRewardWon(u32 rewardIndex)
{
    struct AchievementRunDataExt *runDataExt = &gSaveBlock2Ptr->achievementRunDataExt;
    u32 emporium, i;
    u32 totalOwned = 0;
    u8 emporiumsWithAny = 0;

    if (rewardIndex >= EMPORIUM_REWARD_COUNT)
        return;

    runDataExt->emporiumRewardsWon[rewardIndex / 8] |= 1 << (rewardIndex % 8);

    for (emporium = EMPORIUM_ZMOVE; emporium < EMPORIUM_COUNT; emporium++)
    {
        u32 start = GetEmporiumRewardStart(emporium);
        u32 count = GetEmporiumRewardCount(emporium);
        u32 owned = 0;

        for (i = 0; i < count; i++)
        {
            u32 bit = start + i;

            if (runDataExt->emporiumRewardsWon[bit / 8] & (1 << (bit % 8)))
                owned++;
        }

        totalOwned += owned;
        if (owned > 0)
            emporiumsWithAny++;

        switch (emporium)
        {
        case EMPORIUM_ZMOVE:
            if (owned >= 10)
                Achievement_TryComplete(ACHIEVEMENT_EMPORIUM_CRYSTAL_COLLECTOR);
            if (owned >= count)
                Achievement_TryComplete(ACHIEVEMENT_EMPORIUM_FULL_SPECTRUM);
            break;
        case EMPORIUM_MEGA:
            if (owned >= 20)
                Achievement_TryComplete(ACHIEVEMENT_EMPORIUM_STONE_TRADER);
            if (owned >= count)
                Achievement_TryComplete(ACHIEVEMENT_EMPORIUM_MEGA_MAGNATE);
            break;
        case EMPORIUM_TERA:
            if (owned >= count)
                Achievement_TryComplete(ACHIEVEMENT_EMPORIUM_EVERY_TYPE_COVERED);
            break;
        }
    }

    Achievement_TryComplete(ACHIEVEMENT_EMPORIUM_FIRST_PRIZE);
    if (emporiumsWithAny >= EMPORIUM_COUNT - EMPORIUM_ZMOVE)
        Achievement_TryComplete(ACHIEVEMENT_EMPORIUM_GRAND_TOUR);
    if (totalOwned >= EMPORIUM_REWARD_COUNT)
        Achievement_TryComplete(ACHIEVEMENT_EMPORIUM_EMPTIED);
}

// Called from the IsPartyEmpty() branches of RemoveFaintedMonsFromParty
// (src/overworld.c) and FldEff_PokecenterHeal (src/field_effect.c). Mirrors
// the streak high-water mark into the profile before resetting.
void Achievement_RecordPartyWipe(void)
{
    struct AchievementRunDataExt *runDataExt = &gSaveBlock2Ptr->achievementRunDataExt;

    if (runDataExt->currentTrainerWinStreak > runDataExt->bestTrainerWinStreakThisRun)
        runDataExt->bestTrainerWinStreakThisRun = runDataExt->currentTrainerWinStreak;
    if (runDataExt->bestTrainerWinStreakThisRun > gAchievementProfile.bestTrainerWinStreakEver)
    {
        gAchievementProfile.bestTrainerWinStreakEver = runDataExt->bestTrainerWinStreakThisRun;
        sAchievementProfileDirty = TRUE;
    }

    runDataExt->currentTrainerWinStreak = 0;
    runDataExt->gymLeadersSinceWipe = 0;
    runDataExt->leagueWinsSinceWipe = 0;
}

// Called from HandleEndTurn_BattleWon after Achievement_CheckNuzlockeMilestones
// (never link/recorded).
void Achievement_CheckBattleRecordsMilestones(void)
{
    struct AchievementRunDataExt *runDataExt = &gSaveBlock2Ptr->achievementRunDataExt;
    struct Pokemon *party = gParties[B_TRAINER_PLAYER];
    u8 playerCount = gPartiesCount[B_TRAINER_PLAYER];
    bool8 isTrainerBattle = (gBattleTypeFlags & BATTLE_TYPE_TRAINER) != 0;
    bool8 isMajorBattle = Achievement_IsMajorBattle();
    bool8 isGymBattle = Achievement_IsGymBattle();
    u8 i;

    // Hot Streak..Untouchable Streak.
    if (isTrainerBattle)
    {
        runDataExt->currentTrainerWinStreak = Achievement_SaturatingAddU16(runDataExt->currentTrainerWinStreak, 1);
        if (runDataExt->currentTrainerWinStreak > runDataExt->bestTrainerWinStreakThisRun)
            runDataExt->bestTrainerWinStreakThisRun = runDataExt->currentTrainerWinStreak;

        if (runDataExt->currentTrainerWinStreak >= 5)
            Achievement_TryComplete(ACHIEVEMENT_RECORD_HOT_STREAK);
        if (runDataExt->currentTrainerWinStreak >= 20)
            Achievement_TryComplete(ACHIEVEMENT_RECORD_UNBROKEN);
        if (runDataExt->currentTrainerWinStreak >= 50)
            Achievement_TryComplete(ACHIEVEMENT_RECORD_ON_A_ROLL);
        if (runDataExt->currentTrainerWinStreak >= 100)
            Achievement_TryComplete(ACHIEVEMENT_RECORD_UNTOUCHABLE_STREAK);

        // League Streak: Elite Four/Champion wins.
        switch (GetTrainerClassFromId(TRAINER_BATTLE_PARAM.opponentA))
        {
        case TRAINER_CLASS_ELITE_FOUR:
        case TRAINER_CLASS_CHAMPION:
            if (runDataExt->leagueWinsSinceWipe < 0xFF)
                runDataExt->leagueWinsSinceWipe++;
            if (runDataExt->leagueWinsSinceWipe >= 5)
                Achievement_TryComplete(ACHIEVEMENT_RECORD_LEAGUE_STREAK);
            break;
        default:
            break;
        }
    }

    // Three/Eight Gym Streak, Oddball.
    if (isGymBattle)
    {
        if (runDataExt->gymLeadersSinceWipe < 0xFF)
            runDataExt->gymLeadersSinceWipe++;
        if (runDataExt->gymLeadersSinceWipe >= 3)
            Achievement_TryComplete(ACHIEVEMENT_RECORD_THREE_GYM_STREAK);
        if (runDataExt->gymLeadersSinceWipe >= 8)
            Achievement_TryComplete(ACHIEVEMENT_RECORD_EIGHT_GYM_STREAK);

        for (i = 0; i < playerCount; i++)
        {
            if (GetSpeciesBaseStatTotal(GetMonData(&party[i], MON_DATA_SPECIES)) < 200)
            {
                Achievement_TryComplete(ACHIEVEMENT_COLLECT_ODDBALL);
                break;
            }
        }
    }

    // Veteran Team/Old Reliable: fold this battle's KOs into the cumulative
    // per-slot totals before sBattleData is cleared.
    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (sBattleData.kosPerSlot[i] == 0)
            continue;

        runDataExt->koCountPerSlot[i] = Achievement_SaturatingAddU16(runDataExt->koCountPerSlot[i], sBattleData.kosPerSlot[i]);
        if (runDataExt->koCountPerSlot[i] >= 100)
            Achievement_TryComplete(ACHIEVEMENT_RECORD_VETERAN_TEAM);

        if (isMajorBattle)
        {
            runDataExt->majorKoCountPerSlot[i] = Achievement_SaturatingAddU16(runDataExt->majorKoCountPerSlot[i], sBattleData.kosPerSlot[i]);
            if (runDataExt->majorKoCountPerSlot[i] >= 50)
                Achievement_TryComplete(ACHIEVEMENT_RECORD_OLD_RELIABLE);
        }
    }

    if (isMajorBattle)
    {
        // Legend of the Run, checked at GameClear. legendCandidatePersonalities
        // is the set of Pokemon (by personality) in the party for every major
        // battle so far, intersected with the current party each time.
        if (!runDataExt->anyMajorBattleThisRun)
        {
            runDataExt->legendCandidateCount = 0;
            for (i = 0; i < playerCount && i < PARTY_SIZE; i++)
                runDataExt->legendCandidatePersonalities[runDataExt->legendCandidateCount++] = GetMonData(&party[i], MON_DATA_PERSONALITY);
            runDataExt->anyMajorBattleThisRun = TRUE;
        }
        else if (runDataExt->legendCandidateCount != 0)
        {
            u8 keep = 0;

            for (i = 0; i < runDataExt->legendCandidateCount; i++)
            {
                u32 personality = runDataExt->legendCandidatePersonalities[i];
                u8 j;

                for (j = 0; j < playerCount; j++)
                {
                    if (GetMonData(&party[j], MON_DATA_PERSONALITY) == personality)
                    {
                        runDataExt->legendCandidatePersonalities[keep++] = personality;
                        break;
                    }
                }
            }
            runDataExt->legendCandidateCount = keep;
        }

        // Underestimated: the slot credited with the last opposing faint
        // ended the battle. Full opposing team required.
        if (sBattleData.lastThreeKoSlots[2] != 0
         && gPartiesCount[B_TRAINER_OPPONENT_A] == PARTY_SIZE)
        {
            u8 finalKoSlot = sBattleData.lastThreeKoSlots[2] - 1;

            if (finalKoSlot < playerCount
             && GetSpeciesBaseStatTotal(GetMonData(&party[finalKoSlot], MON_DATA_SPECIES)) < 400)
                Achievement_TryComplete(ACHIEVEMENT_COLLECT_UNDERESTIMATED);
        }
    }
}

// Called on every map load via Achievement_CheckExplorationMilestones. Reads
// live party/game-stat state only; keep it cheap.
void Achievement_CheckRecordsMilestones(void)
{
    struct Pokemon *party = gParties[B_TRAINER_PLAYER];
    u8 playerCount = gPartiesCount[B_TRAINER_PLAYER];
    u8 level100Count = 0;
    u8 maxFriendshipCount = 0;
    u8 i;

    // Growing Strong.
    for (i = 0; i < playerCount; i++)
    {
        u32 level = GetMonData(&party[i], MON_DATA_LEVEL);
        u32 metLevel = GetMonData(&party[i], MON_DATA_MET_LEVEL);

        if (level >= metLevel && level - metLevel >= 10)
        {
            Achievement_TryComplete(ACHIEVEMENT_RECORD_GROWING_STRONG);
            break;
        }
    }

    // Century Club/Full Century, Devoted/Inseparable.
    for (i = 0; i < playerCount; i++)
    {
        if (GetMonData(&party[i], MON_DATA_LEVEL) >= 100)
            level100Count++;
        if (GetMonData(&party[i], MON_DATA_FRIENDSHIP) >= MAX_FRIENDSHIP)
            maxFriendshipCount++;
    }
    if (level100Count >= 1)
        Achievement_TryComplete(ACHIEVEMENT_RECORD_CENTURY_CLUB);
    if (playerCount == PARTY_SIZE && level100Count == PARTY_SIZE)
        Achievement_TryComplete(ACHIEVEMENT_RECORD_FULL_CENTURY);
    if (maxFriendshipCount >= 1)
        Achievement_TryComplete(ACHIEVEMENT_RECORD_DEVOTED);
    if (playerCount == PARTY_SIZE && maxFriendshipCount == PARTY_SIZE)
        Achievement_TryComplete(ACHIEVEMENT_RECORD_INSEPARABLE);

    // Box scans do not belong here: decrypting every box slot costs ~15-50ms
    // per map load at TOTAL_BOXES_COUNT 28.

    // Marathon Trainer/Long Haul, Prolific/Battle Machine.
    if (GetGameStat(GAME_STAT_STEPS) >= 50000)
        Achievement_TryComplete(ACHIEVEMENT_RECORD_MARATHON_TRAINER);
    if (GetGameStat(GAME_STAT_STEPS) >= 200000)
        Achievement_TryComplete(ACHIEVEMENT_RECORD_LONG_HAUL);
    if (GetGameStat(GAME_STAT_TOTAL_BATTLES) >= 1000)
        Achievement_TryComplete(ACHIEVEMENT_RECORD_PROLIFIC);
    if (GetGameStat(GAME_STAT_TOTAL_BATTLES) >= 2500)
        Achievement_TryComplete(ACHIEVEMENT_RECORD_BATTLE_MACHINE);
}

// Called from GameClear (src/post_battle_event_funcs.c).
void Achievement_CheckRecordsCompletionMilestones(void)
{
    struct AchievementRunDataExt *runDataExt = &gSaveBlock2Ptr->achievementRunDataExt;

    if (runDataExt->anyMajorBattleThisRun && runDataExt->legendCandidateCount != 0)
        Achievement_TryComplete(ACHIEVEMENT_RECORD_LEGEND_OF_THE_RUN);

    // Mirror the streak high-water mark here too, so a run that never wipes
    // still records its best streak.
    if (runDataExt->currentTrainerWinStreak > runDataExt->bestTrainerWinStreakThisRun)
        runDataExt->bestTrainerWinStreakThisRun = runDataExt->currentTrainerWinStreak;
    if (runDataExt->bestTrainerWinStreakThisRun > gAchievementProfile.bestTrainerWinStreakEver)
    {
        gAchievementProfile.bestTrainerWinStreakEver = runDataExt->bestTrainerWinStreakThisRun;
        sAchievementProfileDirty = TRUE;
    }
}

// Called from Task_LearnedMove (src/party_menu.c) for TM use only (not HMs).
void Achievement_RecordTMTaught(void)
{
    struct AchievementRunDataExt *runDataExt = &gSaveBlock2Ptr->achievementRunDataExt;

    if (runDataExt->tmsTaughtThisRun < 0xFF)
        runDataExt->tmsTaughtThisRun++;
    if (runDataExt->tmsTaughtThisRun >= 25)
        Achievement_TryComplete(ACHIEVEMENT_RECORD_MOVE_TUTOR);
}

// Called from FldEff_PokecenterHeal (src/field_effect.c).
void Achievement_CheckPokecenterMilestone(void)
{
    if (GetGameStat(GAME_STAT_USED_POKECENTER) >= 200)
        Achievement_TryComplete(ACHIEVEMENT_RECORD_NURSES_NIGHTMARE);
}

// ---- Profile Meta, Mastery & Prestige (category Q) ----------------------

// Diamond Standard: every Diamond-tier achievement except itself -- its own
// flag is unset while its condition is evaluated.
static bool8 Achievement_AllDiamondCompleted(u16 excludeId)
{
    u16 total = 0;
    u16 completed = 0;
    u16 i;

    for (i = ACHIEVEMENT_NONE + 1; i < ACHIEVEMENTS_COUNT; i++)
    {
        if (i == excludeId)
            continue;
        if (gAchievements[i].tier != ACHIEVEMENT_TIER_DIAMOND)
            continue;
        total++;
        if (Achievement_IsCompleted(i))
            completed++;
    }

    return total > 0 && completed == total;
}

// Well Rounded: at least one completed achievement at every tier.
static bool8 Achievement_HasCompletedEveryTier(void)
{
    bool8 seenTier[ACHIEVEMENT_TIER_COUNT] = {FALSE};
    u16 i;

    for (i = ACHIEVEMENT_NONE + 1; i < ACHIEVEMENTS_COUNT; i++)
    {
        if (Achievement_IsCompleted(i))
            seenTier[gAchievements[i].tier] = TRUE;
    }

    for (i = 0; i < ACHIEVEMENT_TIER_COUNT; i++)
    {
        if (!seenTier[i])
            return FALSE;
    }

    return TRUE;
}

// Full Investment: every real boost (BOOST_NONE excluded).
static u32 AchievementBoost_TotalPurchasedLevels(void)
{
    u32 total = 0;
    u16 boostId;

    for (boostId = BOOST_NONE + 1; boostId < BOOSTS_COUNT; boostId++)
        total += AchievementBoost_GetLevel(boostId);

    return total;
}

// Called from the tail of Achievement_TryComplete. Recursion is bounded: each
// nested call either no-ops or completes one new achievement, and there are
// only ACHIEVEMENTS_COUNT of those.
static void Achievement_CheckMasteryMilestones(void)
{
    if (Achievement_HasCompletedEveryTier())
        Achievement_TryComplete(ACHIEVEMENT_PROFILE_WELL_ROUNDED);

    // Thresholds scaled to the catalog's 30,000-point total.
    if (gAchievementProfile.totalPointsEarned >= 15000)
        Achievement_TryComplete(ACHIEVEMENT_PROFILE_POINT_HOARDER);
    if (gAchievementProfile.totalPointsEarned >= 25000)
        Achievement_TryComplete(ACHIEVEMENT_PROFILE_POINT_LEGEND);
    if (gAchievementProfile.pointsFromGoldOrBetter >= 10000)
        Achievement_TryComplete(ACHIEVEMENT_PROFILE_NO_EASY_PATH);

    if (Achievement_AllDiamondCompleted(ACHIEVEMENT_MASTERY_DIAMOND_STANDARD))
        Achievement_TryComplete(ACHIEVEMENT_MASTERY_DIAMOND_STANDARD);
}

// Called from AchievementBoost_Purchase/_Reset, the only places boost state
// changes.
static void Achievement_CheckBoostMilestones(void)
{
    // ~17% of the 30,000 points needed to max every boost.
    if (gAchievementProfile.pointsInvested >= 5000)
        Achievement_TryComplete(ACHIEVEMENT_PROFILE_BOOST_INVESTOR);

    if (AchievementBoost_TotalPurchasedLevels() >= 40)
        Achievement_TryComplete(ACHIEVEMENT_PROFILE_FULL_INVESTMENT);

    // Reconfigured: a reset followed by a purchase (a reset zeroes
    // pointsInvested).
    if (gAchievementProfile.boostResets >= 1 && gAchievementProfile.pointsInvested > 0)
        Achievement_TryComplete(ACHIEVEMENT_PROFILE_RECONFIGURED);
}

// ---- Recruits/Limited Party/Draft/Rotation/Mono Type/Mono Gen -----------

// Called from HandleEndTurn_BattleWon (never link/recorded).
void Achievement_CheckNewModeBattleMilestones(void)
{
    struct AchievementRunDataExt *runDataExt = &gSaveBlock2Ptr->achievementRunDataExt;
    u8 playerCount = gPartiesCount[B_TRAINER_PLAYER];

    if (!(gBattleTypeFlags & BATTLE_TYPE_TRAINER))
        return;

    // Fresh Recruits.
    if (Recruits_IsActive())
        Achievement_TryComplete(ACHIEVEMENT_RECRUITS_FRESH_RECRUITS);

    // Tight Squad/No Room to Spare. Opponent B is ignored (undercounts, never
    // overcounts). First fresh save only; NG+ cycles reuse the mode flags.
    if (Achievement_IsLimitedPartyFirstRun())
    {
        u8 cap = LimitedParty_GetMaxPartySize();

        if (gPartiesCount[B_TRAINER_OPPONENT_A] > cap)
            Achievement_TryComplete(ACHIEVEMENT_LIMITED_PARTY_TIGHT_SQUAD);

        if (playerCount >= cap && runDataExt->limitedPartyWinsAtCap < 0xFF)
        {
            runDataExt->limitedPartyWinsAtCap++;
            if (runDataExt->limitedPartyWinsAtCap >= 50)
                Achievement_TryComplete(ACHIEVEMENT_LIMITED_PARTY_NO_ROOM_TO_SPARE);
        }
    }

    // Spin the Wheel/On a Rotation/Gym Leader Roulette.
    if (RotationMode_IsEnabled())
    {
        Achievement_TryComplete(ACHIEVEMENT_ROTATION_SPIN_THE_WHEEL);

        if (runDataExt->rotationTrainerWinsThisCycle < 0xFF)
        {
            runDataExt->rotationTrainerWinsThisCycle++;
            if (runDataExt->rotationTrainerWinsThisCycle >= 25)
                Achievement_TryComplete(ACHIEVEMENT_ROTATION_ON_A_ROTATION);
        }

        if (Achievement_IsGymBattle())
            Achievement_TryComplete(ACHIEVEMENT_ROTATION_GYM_LEADER_ROULETTE);
    }
}

// Called from GameClear (src/post_battle_event_funcs.c); re-fires once per
// NG+ cycle. Story-completion entries for categories R-X.
void Achievement_CheckNewModeCompletionMilestones(void)
{
    struct AchievementRunData *runData = &gSaveBlock1Ptr->achievementRunData;
    bool8 isHard = gSaveBlock1Ptr->difficulty == DIFFICULTY_HARD;
    bool8 stackedGameMode = gSaveBlock1Ptr->nuzlockeModeEnabled || Draft_IsEnabled() || Recruits_IsEnabled();
    u8 newModeCount;
    bool8 kitchenSink;

    if (Recruits_IsEnabled() && isHard)
        Achievement_TryComplete(ACHIEVEMENT_RECRUITS_ENDLESS_RECRUITMENT_DRIVE);

    // Not tied to Limited Party mode: any HARD run that never carried more
    // than LIMITED_PARTY_BASE_SIZE Pokemon qualifies. First fresh save only.
    if (gSaveBlock2Ptr->newGamePlus == 0 && isHard && runData->highestPartySizeThisRun != 0
     && runData->highestPartySizeThisRun <= LIMITED_PARTY_BASE_SIZE)
        Achievement_TryComplete(ACHIEVEMENT_LIMITED_PARTY_BARE_MINIMUM_CHAMPION);

    if (Draft_IsEnabled())
    {
        Achievement_TryComplete(ACHIEVEMENT_DRAFT_DRAFTED_NOT_CAUGHT);
        if (isHard)
            Achievement_TryComplete(ACHIEVEMENT_DRAFT_NO_BALL_NEEDED);
    }

    if (RotationMode_IsEnabled())
    {
        Achievement_TryComplete(ACHIEVEMENT_ROTATION_FULL_CIRCUIT);
        if (stackedGameMode && FlagGet(FLAG_RANDOMIZE_MON))
            Achievement_TryComplete(ACHIEVEMENT_ROTATION_CHAOS_ROTATION);
    }

    if (MonoType_IsEnabled())
    {
        Achievement_TryComplete(ACHIEVEMENT_MONO_TYPE_TRUE_BELIEVER);
        if (isHard)
            Achievement_TryComplete(ACHIEVEMENT_MONO_TYPE_ONE_TYPE_TO_RULE_THEM_ALL);
    }
    if (MonoGen_IsEnabled())
    {
        Achievement_TryComplete(ACHIEVEMENT_MONO_GEN_TRUE_TO_THE_ROOTS);
        if (isHard)
            Achievement_TryComplete(ACHIEVEMENT_MONO_GEN_OLD_SCHOOL_HARD_MODE);
    }
    if (MonoType_IsEnabled() && MonoGen_IsEnabled())
        Achievement_TryComplete(ACHIEVEMENT_MONO_TYPE_SECOND_VERSE);

    // Cross-Mode stacking. Each *_IsEnabled() returns exactly 0 or 1.
    newModeCount = Recruits_IsEnabled() + LimitedParty_IsEnabled() + Draft_IsEnabled()
                 + RotationMode_IsEnabled() + MonoType_IsEnabled() + MonoGen_IsEnabled();
    kitchenSink = LimitedParty_IsEnabled() && MonoType_IsEnabled()
               && MonoGen_IsEnabled() && RotationMode_IsEnabled();

    if (newModeCount >= 2)
        Achievement_TryComplete(ACHIEVEMENT_CROSSMODE_MODE_COLLECTOR);
    if (kitchenSink)
        Achievement_TryComplete(ACHIEVEMENT_CROSSMODE_KITCHEN_SINK);
    if (kitchenSink && stackedGameMode)
        Achievement_TryComplete(ACHIEVEMENT_CROSSMODE_THE_FULL_STACK);
}

// Called from Recruits_DoRetirement (src/recruits_mode.c), only while
// Recruits mode is on.
void Achievement_RecordRecruitRetirement(void)
{
    struct AchievementRunDataExt *runDataExt = &gSaveBlock2Ptr->achievementRunDataExt;

    Achievement_TryComplete(ACHIEVEMENT_RECRUITS_HONORABLE_DISCHARGE);

    if (runDataExt->recruitsRetiredThisCycle < 0xFF)
        runDataExt->recruitsRetiredThisCycle++;
    if (runDataExt->recruitsRetiredThisCycle >= 5)
        Achievement_TryComplete(ACHIEVEMENT_RECRUITS_REVOLVING_DOOR);
    if (runDataExt->recruitsRetiredThisCycle >= 10)
        Achievement_TryComplete(ACHIEVEMENT_RECRUITS_FULL_TURNOVER);
}

// Called from Draft_MarkAreaSpent (src/draft_mode.c) after a real draft pick.
void Achievement_RecordDraftCompleted(void)
{
    struct AchievementRunDataExt *runDataExt = &gSaveBlock2Ptr->achievementRunDataExt;

    Achievement_TryComplete(ACHIEVEMENT_DRAFT_FIRST_PICK);

    if (runDataExt->draftsCompletedThisCycle < 0xFF)
        runDataExt->draftsCompletedThisCycle++;
    if (runDataExt->draftsCompletedThisCycle >= 10)
        Achievement_TryComplete(ACHIEVEMENT_DRAFT_THE_CASE_IS_CLOSED);
    if (runDataExt->draftsCompletedThisCycle >= 30)
        Achievement_TryComplete(ACHIEVEMENT_DRAFT_FULL_CASE_CLEAR);
}

// Called from Draft_DoReplacement (src/draft_mode.c).
void Achievement_RecordDraftReplacement(void)
{
    Achievement_TryComplete(ACHIEVEMENT_DRAFT_TOUGH_CALL);
}

// Called from BirchCase_GiveMon (src/ui_birch_case.c), the non-Draft starter
// grant. Draft's first pick (BirchCase_QueueDraftMon) does not call this.
void Achievement_CheckMonoStarterMilestones(void)
{
    if (MonoType_IsEnabled())
        Achievement_TryComplete(ACHIEVEMENT_MONO_TYPE_COMMITTED_TO_THE_BIT);
    if (MonoGen_IsEnabled())
        Achievement_TryComplete(ACHIEVEMENT_MONO_GEN_GENERATION_LOYALIST);
}

// Called from GiveCapturedMonToPlayer (src/pokemon.c) and Task_EggHatch
// (src/egg_hatch.c). Obtainable mons are already restricted to the mode's
// type/generation, so every obtain counts.
void Achievement_RecordMonoModeObtain(void)
{
    struct AchievementRunDataExt *runDataExt = &gSaveBlock2Ptr->achievementRunDataExt;

    if (MonoType_IsEnabled() && runDataExt->monoTypeObtainedThisCycle < 0xFF)
    {
        runDataExt->monoTypeObtainedThisCycle++;
        if (runDataExt->monoTypeObtainedThisCycle >= 15)
            Achievement_TryComplete(ACHIEVEMENT_MONO_TYPE_PERFECT_FIT);
    }

    if (MonoGen_IsEnabled() && runDataExt->monoGenObtainedThisCycle < 0xFF)
    {
        runDataExt->monoGenObtainedThisCycle++;
        if (runDataExt->monoGenObtainedThisCycle >= 15)
            Achievement_TryComplete(ACHIEVEMENT_MONO_GEN_GOTTA_CATCH_SOME_OF_THEM);
    }
}

// ---- Debug-only mutators ------------------------------------------------

void Achievement_DebugSetCompleted(u16 achievementId, bool8 completed)
{
    if (achievementId >= MAX_ACHIEVEMENTS)
        return;

    if (completed)
        gAchievementProfile.achievementFlags[achievementId / 8] |= 1 << (achievementId % 8);
    else
        gAchievementProfile.achievementFlags[achievementId / 8] &= ~(1 << (achievementId % 8));

    sAchievementProfileDirty = TRUE;
}

void Achievement_DebugSetPoints(u32 amount)
{
    gAchievementProfile.totalPointsEarned = amount;
    sAchievementProfileDirty = TRUE;
}

void Achievement_DebugSetBoostsUnlocked(bool8 unlocked)
{
    gAchievementProfile.boostsUnlocked = unlocked;
    sAchievementProfileDirty = TRUE;
}

void AchievementBoost_DebugSetLevel(u16 boostId, u8 level)
{
    if (boostId >= MAX_BOOSTS)
        return;

    gAchievementProfile.boostLevels[boostId] = level;
    sAchievementProfileDirty = TRUE;
}

void AchievementBoost_DebugReset(void)
{
    memset(gAchievementProfile.boostLevels, 0, sizeof(gAchievementProfile.boostLevels));
    memset(gAchievementProfile.boostLevelReduction, 0, sizeof(gAchievementProfile.boostLevelReduction));
    gAchievementProfile.pointsInvested = 0;
    sAchievementProfileDirty = TRUE;
}

void Achievement_DebugMarkPlaythroughComplete(void)
{
    gAchievementProfile.playthroughsCompleted++;
    sAchievementProfileDirty = TRUE;
}
