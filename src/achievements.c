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
#include "constants/flags.h"
#include "constants/item.h"     // REPEL_LURE_MASK, for AchievementBoost_ApplySprayStepCount
#include "constants/game_stat.h" // GAME_STAT_*, for Stage 13's threshold checks
#include "constants/pokedex.h"   // NATIONAL_DEX_COUNT, FLAG_GET_SEEN/FLAG_GET_CAUGHT
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

    level = AchievementBoost_GetLevel(BOOST_EXP_GAIN);
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

    level = AchievementBoost_GetLevel(BOOST_SHINY_CHANCE);
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

    level = AchievementBoost_GetLevel(BOOST_CATCH_RATE);
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

    level = AchievementBoost_GetLevel(BOOST_MONEY_GAIN);
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

    level = AchievementBoost_GetLevel(BOOST_EGG_HATCH_SPEED);
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

    level = AchievementBoost_GetLevel(BOOST_FRIENDSHIP_GAIN);
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

    level = AchievementBoost_GetLevel(BOOST_LEGENDARY_ENCOUNTER);
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
    return gAchievementProfile.boostsEnabled && AchievementBoost_GetLevel(boostId) != 0;
}

static u32 GetBoostEffectValue(u16 boostId)
{
    u8 level;

    if (!gAchievementProfile.boostsEnabled)
        return 0;

    level = AchievementBoost_GetLevel(boostId);
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
    gAchievementProfile.pointsInvested = 0;
    sAchievementProfileDirty = TRUE;
}

void Achievement_DebugMarkPlaythroughComplete(void)
{
    gAchievementProfile.playthroughsCompleted++;
    sAchievementProfileDirty = TRUE;
}
