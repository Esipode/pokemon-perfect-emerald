#include "global.h"
#include "gba/flash_internal.h"
#include "agb_flash.h"
#include "event_data.h"
#include "load_save.h"
#include "random.h"
#include "save.h"
#include "achievements.h"
#include "achievement_popup.h"
#include "money.h"               // IsEnoughMoney/RemoveMoney, for AchievementBoost_Reset (Stage 11)
#include "overworld.h"           // GetGameStat, for Stage 13's threshold checks
#include "pokedex.h"             // GetNationalPokedexCount, for Achievement_CheckPokedexMilestones
#include "pokemon.h"             // GetMonData/gParties/gPartiesCount, for Stage 15's evaluation-time party queries
#include "battle.h"               // gBattleMons/gBattlerPartyIndexes/gLastMoves/gBattleWeather/gBattleTypeFlags/gBattleResults, Stage 15
#include "battle_setup.h"        // TRAINER_BATTLE_PARAM, for Achievement_IsMajorBattle (Stage 15)
#include "data.h"                 // GetTrainerClassFromId, for Achievement_IsMajorBattle (Stage 15)
#include "move.h"                 // GetMovePriority, for Achievement_RecordOpposingFaint (Stage 15)
#include "caps.h"                 // GetCurrentLevelCap, for Stage 16's level-cap checks
#include "pokemon_storage_system.h" // TOTAL_BOXES_COUNT/IN_BOX_COUNT/GetBoxMonDataAt, for Stage 16's No Ace
#include "constants/difficulty.h" // DIFFICULTY_HARD, for Stage 16's Trial by Fire
#include "constants/flags.h"
#include "constants/item.h"     // REPEL_LURE_MASK, for AchievementBoost_ApplySprayStepCount
#include "constants/game_stat.h" // GAME_STAT_*, for Stage 13's threshold checks
#include "constants/pokedex.h"   // NATIONAL_DEX_COUNT, FLAG_GET_SEEN/FLAG_GET_CAUGHT
#include "constants/pokemon.h"    // MON_DATA_*, for Stage 15's evaluation-time party queries
#include "constants/trainers.h"   // TRAINER_CLASS_*, for Achievement_IsMajorBattle (Stage 15)
#include "data/achievements.h"
#include "data/achievement_boosts.h"

// The whole struct is written as one blob to a sector (see WriteAchievementProfile).
STATIC_ASSERT(sizeof(struct AchievementProfile) <= SECTOR_SIZE, AchievementProfileFreeSpace);

EWRAM_DATA struct AchievementProfile gAchievementProfile = {0};

// Separate from gDamagedSaveSectors on purpose (see achievements.h): a profile
// read/write failure must never be able to trigger the save-failed screen.
// Must default to FALSE (zero-initialized): non-zero static initializers land
// in a plain .data section that ld_script_modern.ld has no rule for and the
// trailing /DISCARD/ silently drops, leaving .text with a dangling reference.
static bool8 sAchievementProfileWriteFailed = FALSE;

// Set by mutators in this file (Stage 1.4's Achievement_TryComplete, boost
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

    // NOTE: once Stage 7 adds struct AchievementBoost (with its per-boost
    // maxLevel), clamp each boostLevels[i] to that ceiling here so corrupt
    // flash data can never hand back an out-of-range boost level.
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

// Hands off to src/achievement_popup.c's own ring buffer (design doc §4.2),
// which drains one popup at a time once the field is in a safe state to
// show it. Achievement_TryComplete has already committed the flag and
// points unconditionally by the time this runs, so nothing here can ever
// withhold an award (design doc §6, §4.30 "Important rule") -- at worst, a
// full queue drops the toast, never the achievement itself.
static void QueueAchievementNotification(u16 achievementId)
{
    AchievementPopup_Enqueue(achievementId);
}

bool8 Achievement_ProfileWriteFailed(void)
{
    return sAchievementProfileWriteFailed;
}

// Call sites (design doc §1.3): the overworld at the same safe point the
// achievement popup task runs, TrySavingData (so a normal save always
// flushes), and immediately after a boost purchase or boost reset. Achievements
// earned mid-battle flush on return to the field, not in-battle: this fails in
// the safe direction, where a hard reset can lose an award but never double-award it.
void Achievement_FlushProfile(void)
{
    if (!sAchievementProfileDirty)
        return;

    WriteAchievementProfile();
    sAchievementProfileDirty = FALSE;
}

// ---- Public API (design doc §24) -------------------------------------

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

// Stage 13, category J: the one self-referential achievement. Called from
// the tail of Achievement_TryComplete, after totalPointsEarned has already
// been updated for whatever achievement just completed. Safe to recurse
// through Achievement_TryComplete -- its own Achievement_IsCompleted guard
// makes the recursive call a no-op after the first time, and this wave only
// has one such meta-achievement, so there's no chain to unwind.
static void Achievement_CheckPointMilestones(void)
{
    if (gAchievementProfile.totalPointsEarned >= 1000)
        Achievement_TryComplete(ACHIEVEMENT_POINTS_1000);
}

// design doc §4.30: the flag and the points are written together, before any
// UI is involved, so a UI failure can never withhold an already-earned
// reward, and a reset can never cause a double award (design doc §6).
bool8 Achievement_TryComplete(u16 achievementId)
{
    if (achievementId >= ACHIEVEMENTS_COUNT)
        return FALSE;

    // design doc §1.5: debug mode disqualifies the current run.
    if (gSaveBlock1Ptr->achievementsBlocked)
        return FALSE;

    if (Achievement_IsCompleted(achievementId))
        return FALSE;

    gAchievementProfile.achievementFlags[achievementId / 8] |= 1 << (achievementId % 8);
    gAchievementProfile.totalPointsEarned += gAchievements[achievementId].points;
    sAchievementProfileDirty = TRUE;

    QueueAchievementNotification(achievementId);

    Achievement_CheckPointMilestones();

    return TRUE;
}

// design doc §5.2 (Stage 5): called from GameClear() the one time
// FLAG_SYS_GAME_CLEAR is newly set for this save (see the declaration in
// achievements.h for why no completion guard is needed here). Flushes
// immediately rather than waiting for the next §1.3 flush point, same as
// the boost purchase/reset mutators -- this is a rare, important state
// change, not a hot path.
void Achievement_OnFirstPlaythroughComplete(void)
{
    gAchievementProfile.boostsUnlocked = TRUE;
    gAchievementProfile.playthroughsCompleted++;

    if (gSaveBlock1Ptr->nuzlockeModeEnabled)
        gAchievementProfile.nuzlockesCompleted++;

    if (FlagGet(FLAG_RANDOMIZE_MON) || FlagGet(FLAG_RANDOMIZE_TYPE) || FlagGet(FLAG_RANDOMIZE_MOVES))
        gAchievementProfile.randomizedRunsCompleted++;

    // Stage 13, category J: multi-run milestones derived from the counters
    // just updated above.
    if (gAchievementProfile.playthroughsCompleted >= 2)
        Achievement_TryComplete(ACHIEVEMENT_PLAYTHROUGHS_2);
    if (gAchievementProfile.playthroughsCompleted >= 5)
        Achievement_TryComplete(ACHIEVEMENT_PLAYTHROUGHS_5);
    if (gAchievementProfile.nuzlockesCompleted >= 1)
        Achievement_TryComplete(ACHIEVEMENT_NUZLOCKE_1);
    if (gAchievementProfile.nuzlockesCompleted >= 3)
        Achievement_TryComplete(ACHIEVEMENT_NUZLOCKE_3);
    if (gAchievementProfile.randomizedRunsCompleted >= 1)
        Achievement_TryComplete(ACHIEVEMENT_RANDOMIZED_1);

    sAchievementProfileDirty = TRUE;
    Achievement_FlushProfile();
}

// design doc Stage 12: flushes immediately, same as the function above --
// starting a new NG+ cycle is rare and important, not a hot path.
void Achievement_OnNewGamePlusStarted(u8 cycle)
{
    if (cycle > gAchievementProfile.highestNgPlusCycle)
        gAchievementProfile.highestNgPlusCycle = cycle;

    // Stage 13, category J: this function only ever runs when NG+ actually
    // just started, so the "started" achievement needs no threshold guard.
    Achievement_TryComplete(ACHIEVEMENT_NG_PLUS_STARTED);
    if (gAchievementProfile.highestNgPlusCycle >= 3)
        Achievement_TryComplete(ACHIEVEMENT_NG_PLUS_CYCLE_3);
    if (gAchievementProfile.highestNgPlusCycle >= 5)
        Achievement_TryComplete(ACHIEVEMENT_NG_PLUS_CYCLE_5);

    sAchievementProfileDirty = TRUE;
    Achievement_FlushProfile();
}

// design doc Stage 12: see the header doc comment for why this is separate
// from Achievement_OnFirstPlaythroughComplete rather than folded into it.
void Achievement_OnNewGamePlusCycleCompleted(void)
{
    gAchievementProfile.ngPlusCyclesCompleted++;

    // Stage 13, category J.
    if (gAchievementProfile.ngPlusCyclesCompleted >= 3)
        Achievement_TryComplete(ACHIEVEMENT_NG_PLUS_COMPLETED_3);

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

bool8 Achievement_BoostsUnlocked(void)
{
    return gAchievementProfile.boostsUnlocked;
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

// design doc §7/§23: refuses at the first failed check rather than
// collecting all failures, since the caller (the boost shop, Stage 7) only
// needs a yes/no to decide whether [A] Purchase is valid. This is the only
// real (non-debug) path that increments boostLevels[id], and it never does
// so past maxLevel -- AchievementBoost_DebugSetLevel (src/debug.c) can still
// stuff an out-of-range level in directly, by design (debug tools bypass
// this validation), which is why AchievementBoost_GetInfo/GetLevel and this
// function are the ones responsible for treating that as "already maxed"
// rather than assuming level < maxLevel always holds.
//
// Deliberately NOT gated on gSaveBlock1Ptr->achievementsBlocked like
// Achievement_TryComplete is: debug mode only disqualifies *earning* new
// achievements/points (design doc §1.5), not spending points already
// earned. Without this, opening the debug menu at all -- which several of
// the achievement debug tools themselves require -- permanently locked out
// the purchase flow those same tools exist to test.
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

// design doc §23 "point underflow": pointsInvested can only grow here, and
// only by an amount CanPurchase already verified is <= the available
// balance, so totalPointsEarned - pointsInvested can never go negative.
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
    Achievement_FlushProfile();

    return TRUE;
}

// design doc §13/Stage 11: refuses at the first failed check, same style as
// AchievementBoost_CanPurchase. "Nothing invested" is checked before "can
// afford the fee" so a player with no boosts purchased is never told they
// need more money for a reset that would refund them nothing anyway.
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

// design doc Stage 11/§23 "reset exploits": the refund is exactly
// pointsInvested -- the same value AchievementBoost_Purchase only ever grew
// it by -- so a reset can never generate points. Order matters: the refund
// and level clear happen before RemoveMoney, so a failed CanReset (re-checked
// here, not trusted from a stale caller-side result) leaves money, points and
// levels all untouched together.
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
    Achievement_FlushProfile();

    return TRUE;
}

// design doc Stage 8: the first real boost effect, and the shape every
// subsequent one should follow -- centralized here rather than scattered at
// each call site, an early return to plain baseline whenever boosts are
// disabled or the level is 0, and u64 math so a New Game+-inflated expValue
// (src/pokemon.c, commit 959a51b21a's reworked growth curves) can never
// overflow computing expValue * percent before the /100 brings it back down.
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

// Stages 9-10: same shape as AchievementBoost_ApplyExp above -- each is a
// provable no-op when boosts are disabled or the boost is at level 0.

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

// ---- Stage 10.1: catalog wave 2 ---------------------------------------
//
// Same shape as everything above: a provable no-op when boosts are disabled
// or the boost is at level 0.
//
// The three battle boosts (crit, PP saver, status recovery) return a raw
// percent instead of rolling here, unlike AchievementBoost_ShouldRoamerSeekPlayer
// above. Battle randomness in this fork goes through the tagged
// RandomChance/RandomPercentage helpers so the test harness and recorded-battle
// playback stay deterministic; rolling with a bare Random() from this file
// would sidestep that. Returning 0 lets each call site skip its roll entirely,
// so the baseline path consumes no RNG at all.

// Shared by the three BOOST_TYPE_BINARY boosts below -- for those, "purchased"
// is the whole effect, so there's no effects[] value to look up.
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

    // A tree with nothing on it stays empty -- this adds to a harvest, it
    // doesn't conjure one.
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

// ---- Stage 13: catalog wave 1 hook functions ----------------------------

// {flag, achievementId} pairs for every badge/story milestone that already
// funnels through Common_EventScript_CheckLevelCapIncrease
// (data/scripts/level_cap.inc). Each of the 16 call sites already sets its
// own flag on the line immediately before calling that shared script, so
// checking all 15 unconditionally on every call is correct -- Route 103's
// two call sites (May/Brendan) both set FLAG_BEAT_RIVAL_ROUTE_103 and so
// collapse into the one achievement below.
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

    // Stage 16: piggyback on this same callnative for party-state checks that
    // aren't tied to a specific battle. See that function's own doc comment.
    Achievement_CheckPartyStateMilestones();
}

// Percentages of NATIONAL_DEX_COUNT rather than hardcoded species counts, so
// the thresholds stay correct regardless of which expansion level a given
// build is compiled with -- the same approach the Pokedex UI itself already
// uses for its own percentage display.
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
    }
    else
    {
        count = GetNationalPokedexCount(FLAG_GET_CAUGHT);
        if (count >= NATIONAL_DEX_COUNT * 10 / 100)
            Achievement_TryComplete(ACHIEVEMENT_DEX_CAUGHT_10);
        if (count >= NATIONAL_DEX_COUNT * 25 / 100)
            Achievement_TryComplete(ACHIEVEMENT_DEX_CAUGHT_25);
        if (count >= NATIONAL_DEX_COUNT * 50 / 100)
            Achievement_TryComplete(ACHIEVEMENT_DEX_CAUGHT_50);
        if (count >= NATIONAL_DEX_COUNT)
            Achievement_TryComplete(ACHIEVEMENT_DEX_CAUGHT_100);
    }
}

// GAME_STAT_POKEMON_CAPTURES is already incremented for the current catch by
// the time GiveCapturedMonToPlayer (this function's only caller) runs --
// confirmed against data/battle_scripts_2.s, where incrementgamestat
// precedes givecaughtmon.
void Achievement_CheckCaptureMilestones(void)
{
    u32 count = GetGameStat(GAME_STAT_POKEMON_CAPTURES);

    if (count >= 1)
        Achievement_TryComplete(ACHIEVEMENT_CATCH_1);
    if (count >= 25)
        Achievement_TryComplete(ACHIEVEMENT_CATCH_25);
    if (count >= 100)
        Achievement_TryComplete(ACHIEVEMENT_CATCH_100);
    if (count >= 250)
        Achievement_TryComplete(ACHIEVEMENT_CATCH_250);
    if (count >= 500)
        Achievement_TryComplete(ACHIEVEMENT_CATCH_500);
}

// shiniesObtained has existed in the profile since early on (already
// surfaced in the debug menu's profile dump) but nothing ever incremented
// it until this stage.
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

// GAME_STAT_TRAINER_BATTLES is incremented at battle *start* (a dozen-plus
// scattered Do*Battle functions in src/battle_setup.c), so by the time any
// given trainer battle ends via CB2_EndTrainerBattle (this function's only
// caller) the count is already final -- no need to touch every start site.
void Achievement_CheckTrainerBattleMilestones(void)
{
    u32 count = GetGameStat(GAME_STAT_TRAINER_BATTLES);

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

// Same reasoning as the trainer version above, reading GAME_STAT_WILD_BATTLES
// from CB2_EndWildBattle.
void Achievement_CheckWildBattleMilestones(void)
{
    u32 count = GetGameStat(GAME_STAT_WILD_BATTLES);

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

// GAME_STAT_HATCHED_EGGS is already incremented well before Task_EggHatch
// (this function's only caller) reaches the point where the hatched mon's
// data is valid -- see src/field_control_avatar.c, at the very start of the
// hatch sequence.
void Achievement_CheckEggMilestones(bool8 isShiny)
{
    u32 count = GetGameStat(GAME_STAT_HATCHED_EGGS);

    if (count >= 1)
        Achievement_TryComplete(ACHIEVEMENT_EGG_1);
    if (count >= 10)
        Achievement_TryComplete(ACHIEVEMENT_EGG_10);
    if (count >= 50)
        Achievement_TryComplete(ACHIEVEMENT_EGG_50);
    if (isShiny)
        Achievement_TryComplete(ACHIEVEMENT_EGG_SHINY);
}

// ---- Stage 15: catalog wave 2 (category K, Battle Mastery) -------------
//
// struct AchievementBattleData is EWRAM-only and never saved -- a battle
// never spans a save, so nothing here belongs in AchievementRunData. Zeroed
// by Achievement_ClearBattleData (BattleStartClearSetData, src/battle_main.c)
// at the start of every battle, and read exactly once, by
// Achievement_CheckBattleMilestones (HandleEndTurn_BattleWon, same file).
// That single evaluation point means every category K entry is only
// ever checked in a battle the player actually won (landing a crit in a
// battle that's then lost doesn't earn Critical Success) -- a deliberate
// simplification to keep this to the "one entry point" discipline Stage 13
// established, not an attempt to track "did this ever happen this run".
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

// CancelerPPDeduction (src/battle_move_resolution.c). See the header doc
// comment (include/achievements.h) for why type/STAB/setup are pre-computed
// by the caller instead of looked up here.
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

    // Bookkeeping for Achievement_RecordOpposingFaint's setupThenKo check:
    // remember whether THIS move follows this same battler's own most recent
    // setup move, before pendingSetupBattler gets overwritten below.
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

// SetValuesOnFaint (src/battle_util.c)'s opponent-faint branch. See the
// header doc comment for the attackerBattler == victimBattler reasoning
// (status/passive damage vs. a real move-caused KO).
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

// HandleEndTurn_BattleWon (src/battle_main.c), gated by the caller on not
// being a link or recorded battle -- see the header doc comment for why.
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
    if (isTrainerBattle && !sBattleData.superEffectiveUsed)
        Achievement_TryComplete(ACHIEVEMENT_BATTLE_TYPE_MASTER);

    if (isTrainerBattle && gBattleResults.playerSwitchesCounter == 0)
    {
        Achievement_TryComplete(ACHIEVEMENT_BATTLE_CLEAN_SWEEP);
        if (isMajorBattle)
            Achievement_TryComplete(ACHIEVEMENT_BATTLE_PERFECT_SWEEP);
    }

    if (isTrainerBattle && !gBattleResults.playerMonWasDamaged)
    {
        Achievement_TryComplete(ACHIEVEMENT_BATTLE_NO_DAMAGE);
        if (isMajorBattle)
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

    if (isMajorBattle && gBattleResults.battleTurnCounter >= 15)
        Achievement_TryComplete(ACHIEVEMENT_BATTLE_ATTRITION);

    if (isMajorBattle && gBattleResults.playerFaintCounter == 0)
        Achievement_TryComplete(ACHIEVEMENT_BATTLE_STRATEGIC_VICTORY);

    if (isTrainerBattle && playerCount != 0 && gBattleResults.playerFaintCounter * 2 >= playerCount)
        Achievement_TryComplete(ACHIEVEMENT_BATTLE_REVERSE_SWEEP);

    if (isTrainerBattle && GetTrainerClassFromId(TRAINER_BATTLE_PARAM.opponentA) == TRAINER_CLASS_CHAMPION
     && CountSetBits(sBattleData.slotsThatActed) >= 4)
        Achievement_TryComplete(ACHIEVEMENT_BATTLE_CHAMPION_TACTICIAN);

    if (isMajorBattle && sBattleData.slotsThatActed != 0)
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

    if (isTrainerBattle && !sBattleData.repeatedMove)
        Achievement_TryComplete(ACHIEVEMENT_BATTLE_NO_REPEATS);

    // "Heavily underleveled": every party member at least 5 levels below the
    // highest-level Pokemon on opponentA's team. Doesn't look at opponentB in
    // a double/multi battle -- a rare enough case for a flavor achievement
    // that the simplification isn't worth the extra bookkeeping.
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

    if (isTrainerBattle && !sBattleData.stabUsed)
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

    if (consciousCount == 1)
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

// ---- Stage 16: catalog wave 3 (category L, Team Building & Composition) -
//
// The first real user of struct AchievementRunData (include/global.h) --
// see that struct's own comment for what each field tracks. Species sets are
// tracked by species ID, not full individual identity (personality/OT), the
// same granularity Stage 15 already tracks party members at.

bool8 Achievement_IsGymBattle(void)
{
    if (!(gBattleTypeFlags & BATTLE_TYPE_TRAINER))
        return FALSE;

    return GetTrainerClassFromId(TRAINER_BATTLE_PARAM.opponentA) == TRAINER_CLASS_LEADER;
}

// Returns a shared type if every one of the count members has it as one of
// their (up to two) types, else NUMBER_OF_MON_TYPES. Starts at TYPE_NONE + 1
// so two single-type members don't spuriously "share" TYPE_NONE via their
// unused second type slot.
static u8 Achievement_ComputePartyMonoType(struct Pokemon *party, u8 count)
{
    u32 type;

    if (count == 0)
        return NUMBER_OF_MON_TYPES;

    for (type = TYPE_NONE + 1; type < NUMBER_OF_MON_TYPES; type++)
    {
        u8 i;
        bool8 allHaveType = TRUE;

        for (i = 0; i < count; i++)
        {
            enum Species species = GetMonData(&party[i], MON_DATA_SPECIES);

            if (gSpeciesInfo[species].types[0] != type && gSpeciesInfo[species].types[1] != type)
            {
                allHaveType = FALSE;
                break;
            }
        }

        if (allHaveType)
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

static bool8 Achievement_HasDuplicateSpecies(struct Pokemon *party, u8 count)
{
    u8 i, j;

    for (i = 0; i < count; i++)
    {
        enum Species speciesI = GetMonData(&party[i], MON_DATA_SPECIES);

        for (j = i + 1; j < count; j++)
        {
            if (speciesI == GetMonData(&party[j], MON_DATA_SPECIES))
                return TRUE;
        }
    }

    return FALSE;
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

// Scans every PC box, not just the party -- "highest-level Pokemon" for No
// Ace means across everything the player owns, not just the active six.
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
// species within one snapshot are treated as a single set member -- an
// acceptable simplification for these flavor achievements.
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

// GiveCapturedMonToPlayer (src/pokemon.c) / Task_EggHatch (src/egg_hatch.c).
// See the header doc comment for why gift/traded-in mons aren't tracked.
void Achievement_RecordMonObtained(u32 personality)
{
    struct AchievementRunData *runData = &gSaveBlock1Ptr->achievementRunData;
    u8 slot = runData->recentlyObtainedCount % ARRAY_COUNT(runData->recentlyObtainedPersonality);

    runData->recentlyObtainedPersonality[slot] = personality;
    if (runData->recentlyObtainedCount < 0xFF)
        runData->recentlyObtainedCount++;
}

// HandleEndTurn_BattleWon (src/battle_main.c), right after
// Achievement_CheckBattleMilestones, gated the same way by the caller (never
// link/recorded). Mono-type discipline is tracked on every trainer win, not
// just major ones -- adding an off-type Pokemon for a throwaway early
// trainer breaks it just as much as adding one for a Gym.
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
        if (runData->monoTypeType == NUMBER_OF_MON_TYPES)
        {
            if (monoType != NUMBER_OF_MON_TYPES)
                runData->monoTypeType = monoType;
            else
                runData->monoTypeBroken = TRUE;
        }
        else if (monoType != runData->monoTypeType)
        {
            runData->monoTypeBroken = TRUE;
        }
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

        if (Achievement_PartyBaseStatTotal(party, playerCount) < 1800)
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
        if (Achievement_AllTypesDisjoint(party, playerCount))
            Achievement_TryComplete(ACHIEVEMENT_TEAM_NO_DUPLICATES);

        if (playerCount == PARTY_SIZE && monoType != NUMBER_OF_MON_TYPES)
            Achievement_TryComplete(ACHIEVEMENT_TEAM_SIX_OF_A_KIND);

        if (!Achievement_HasDuplicateSpecies(party, playerCount))
            Achievement_TryComplete(ACHIEVEMENT_TEAM_VARIETY_IS_POWER);

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

// Common_EventScript_CheckLevelCapIncrease's callnative, via the tail of
// Achievement_CheckStoryMilestones -- party state that isn't tied to a
// specific battle. levelCapEverExceeded/bstEverExceeded450 are bookkeeping
// only checked here, at these 16 checkpoints, rather than continuously; a
// party member could transiently cross a threshold between two checkpoints
// and be missed, the same fidelity tradeoff Stage 15's per-battle snapshots
// already accept elsewhere in this file.
void Achievement_CheckPartyStateMilestones(void)
{
    struct AchievementRunData *runData = &gSaveBlock1Ptr->achievementRunData;
    struct Pokemon *party = gParties[B_TRAINER_PLAYER];
    u8 playerCount = gPartiesCount[B_TRAINER_PLAYER];
    u32 levelCap = GetCurrentLevelCap();
    u8 holdingItemCount = 0;
    u8 atOrAboveCapCount = 0;
    u8 i;

    for (i = 0; i < playerCount; i++)
    {
        u32 level = GetMonData(&party[i], MON_DATA_LEVEL);

        if (GetMonData(&party[i], MON_DATA_HELD_ITEM) != ITEM_NONE)
            holdingItemCount++;

        if (level >= levelCap)
            atOrAboveCapCount++;
        if (level > levelCap)
            runData->levelCapEverExceeded = TRUE;

        if (GetSpeciesBaseStatTotal(GetMonData(&party[i], MON_DATA_SPECIES)) > 450)
            runData->bstEverExceeded450 = TRUE;
    }

    if (playerCount == PARTY_SIZE && holdingItemCount == PARTY_SIZE)
        Achievement_TryComplete(ACHIEVEMENT_TEAM_WELL_EQUIPPED);

    if (playerCount == PARTY_SIZE && atOrAboveCapCount == PARTY_SIZE)
        Achievement_TryComplete(ACHIEVEMENT_TEAM_FULL_HOUSE);
}

// GameClear (src/post_battle_event_funcs.c), alongside
// Achievement_OnFirstPlaythroughComplete -- see that call site for why this
// correctly re-fires once per New Game+ cycle, not just the save's first
// clear.
void Achievement_CheckTeamCompletionMilestones(void)
{
    struct AchievementRunData *runData = &gSaveBlock1Ptr->achievementRunData;
    struct Pokemon *party = gParties[B_TRAINER_PLAYER];
    u8 playerCount = gPartiesCount[B_TRAINER_PLAYER];

    if (runData->monoTypeType != NUMBER_OF_MON_TYPES && !runData->monoTypeBroken)
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

    if (!runData->levelCapEverExceeded)
        Achievement_TryComplete(ACHIEVEMENT_TEAM_CAPPED_OUT);

    if (!runData->bstEverExceeded450)
        Achievement_TryComplete(ACHIEVEMENT_TEAM_UNDERDOG_RUN);

    if (Achievement_AllPrimaryTypesDistinct(party, playerCount))
        Achievement_TryComplete(ACHIEVEMENT_TEAM_DREAM_TEAM);

    if (CountSetBits(Achievement_PartyTypeComposition(party, playerCount)) >= 10)
        Achievement_TryComplete(ACHIEVEMENT_TEAM_BALANCED_ROSTER);
}

// ---- Debug-only mutators (design doc §21, Stage 1.7) -------------------

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
