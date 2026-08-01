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
#include "constants/flags.h"
#include "constants/item.h"     // REPEL_LURE_MASK, for AchievementBoost_ApplySprayStepCount
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

    sAchievementProfileDirty = TRUE;
    Achievement_FlushProfile();
}

// design doc Stage 12: flushes immediately, same as the function above --
// starting a new NG+ cycle is rare and important, not a hot path.
void Achievement_OnNewGamePlusStarted(u8 cycle)
{
    if (cycle > gAchievementProfile.highestNgPlusCycle)
        gAchievementProfile.highestNgPlusCycle = cycle;

    sAchievementProfileDirty = TRUE;
    Achievement_FlushProfile();
}

// design doc Stage 12: see the header doc comment for why this is separate
// from Achievement_OnFirstPlaythroughComplete rather than folded into it.
void Achievement_OnNewGamePlusCycleCompleted(void)
{
    gAchievementProfile.ngPlusCyclesCompleted++;

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

// design doc §1.5: exposes the same gSaveBlock1Ptr->achievementsBlocked flag
// Achievement_TryComplete and AchievementBoost_CanPurchase already gate on,
// so UI (the boost shop) can explain *why* a purchase is refused instead of
// just failing silently. Set once by Debug_ShowMainMenu (src/debug.c) the
// moment the debug menu is opened at all, for the rest of that save -- by
// design, not reversible from within the game.
bool8 Achievement_RunBlocked(void)
{
    return gSaveBlock1Ptr->achievementsBlocked;
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
bool8 AchievementBoost_CanPurchase(u16 boostId)
{
    const struct AchievementBoost *info;
    u8 level;

    if (!gAchievementProfile.boostsUnlocked)
        return FALSE;

    // design doc §1.5: debug mode disqualifies the current run, same rule
    // Achievement_TryComplete enforces for earning points in the first place.
    if (gSaveBlock1Ptr->achievementsBlocked)
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
