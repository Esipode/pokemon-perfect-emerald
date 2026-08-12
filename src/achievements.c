#include "global.h"
#include "gba/flash_internal.h"
#include "agb_flash.h"
#include "event_data.h"
#include "load_save.h"
#include "random.h"
#include "save.h"
#include "achievements.h"
#include "achievement_popup.h"
#include "money.h"               // IsEnoughMoney/RemoveMoney, for AchievementBoost_Reset
#include "overworld.h"           // GetGameStat, for threshold checks
#include "pokedex.h"             // GetNationalPokedexCount, for Achievement_CheckPokedexMilestones
#include "pokemon.h"             // GetMonData/gParties/gPartiesCount, for evaluation-time party queries
#include "battle.h"               // gBattleMons/gBattlerPartyIndexes/gLastMoves/gBattleWeather/gBattleTypeFlags/gBattleResults
#include "battle_setup.h"        // TRAINER_BATTLE_PARAM, for Achievement_IsMajorBattle
#include "data.h"                 // GetTrainerClassFromId, for Achievement_IsMajorBattle
#include "move.h"                 // GetMovePriority, for Achievement_RecordOpposingFaint
#include "caps.h"                 // GetCurrentLevelCap, for level-cap checks
#include "pokemon_storage_system.h" // TOTAL_BOXES_COUNT/IN_BOX_COUNT/GetBoxMonDataAt, for No Ace
#include "constants/difficulty.h" // DIFFICULTY_HARD, for Trial by Fire
#include "item.h"                 // gBagPockets/POCKETS_COUNT, for Pack Rat/Resourceful
#include "wild_encounter.h"       // gWildMonHeaders/GetCurrentMapWildMonHeaderId, for Local Expert
#include "battle_main.h"          // GetCurrentMapId, for Full Encounter bookkeeping
#include "constants/flags.h"
#include "constants/item.h"     // REPEL_LURE_MASK, for AchievementBoost_ApplySprayStepCount
#include "constants/game_stat.h" // GAME_STAT_*, for threshold checks
#include "constants/pokedex.h"   // NATIONAL_DEX_COUNT, FLAG_GET_SEEN/FLAG_GET_CAUGHT
#include "constants/pokemon.h"    // MON_DATA_*, for evaluation-time party queries
#include "constants/trainers.h"   // TRAINER_CLASS_*, for Achievement_IsMajorBattle
#include "data/achievements.h"
#include "data/achievement_boosts.h"

// Forward declarations: helpers defined further down this file that the
// Achievement_OnFirstPlaythroughComplete/_OnNewGamePlusStarted/
// _OnNewGamePlusCycleCompleted wrapper functions above them need (those
// wrapper functions are the Randomizer & New Game+ category's hooks -- see
// include/constants/achievements.h's category O comment) and stayed where
// they were originally placed rather than moving down next to everything
// they now call.
static void Achievement_SnapshotPartySpecies(struct Pokemon *party, u8 count, u16 *dest);
static bool8 Achievement_SpeciesSetsDisjoint(const u16 *a, const u16 *b);
static u8 Achievement_CountChallengeModifiers(void);

// Achievement_CheckMasteryMilestones
// is called from the tail of Achievement_TryComplete, defined far below it;
// Achievement_CheckBoostMilestones is called from AchievementBoost_Purchase/
// _Reset, both defined above where it lives. See include/achievements.h's
// comment on category Q for why neither needs a header declaration.
static void Achievement_CheckMasteryMilestones(void);
static void Achievement_CheckBoostMilestones(void);

// Achievement_CountDistinctOwnedSpecies backs One of Each, whose check lives
// in Achievement_RecordMonObtained (category N, far above where this helper is
// defined down among the category M/records helpers). It stayed where it was
// originally placed, next to the other collection helpers, rather than moving
// up to its one remaining caller.
static u32 Achievement_CountDistinctOwnedSpecies(struct Pokemon *party, u8 playerCount, u32 stopAt);

// ---- Randomizer & New Game+ (category O) -------------------------------
//
// Small, self-contained helpers with no ordering dependency of their own,
// defined up here (rather than down with Achievement_CheckRandomizerCaptureMilestone
// and the rest of this category's code) so both the wrapper functions
// above and Achievement_CheckChallengeMilestones/
// _CheckNuzlockeCompletionMilestones further down can call them.

static bool8 Achievement_AnyRandomizerFlagSet(void)
{
    return FlagGet(FLAG_RANDOMIZE_MON) || FlagGet(FLAG_RANDOMIZE_TYPE) || FlagGet(FLAG_RANDOMIZE_MOVES);
}

// Achievement_ChallengeConfigSignature (a bitmask twin of
// Achievement_CountChallengeModifiers -- same seven New Game Settings, but
// distinguishing configurations rather than just counting them) removed --
// it only ever backed Replay Master (ACHIEVEMENT_VARIETY_REPLAY_MASTER),
// which is removed too. See src/data/achievements.h's own comment.

// Bitmask over Achievement_IsMajorBattle's six trainer classes, for Boss
// Gauntlet (NGP-014). "Every major boss" is simplified to "every major-boss
// TRAINER CLASS" rather than every individual trainer -- Emerald's story
// already mandates fighting at least one of each (all 8 Gym Leaders, the
// full Elite Four, the Champion, every rival battle, and both Team Aqua/
// Magma leader confrontations), so this is a real but low-friction check,
// the same flavor as several other completion entries in this wave.
#define ACHIEVEMENT_BOSS_GAUNTLET_ALL_CLASSES 0x3F
static u8 Achievement_MajorBossClassBit(u8 trainerClass)
{
    switch (trainerClass)
    {
    case TRAINER_CLASS_LEADER:       return 1 << 0;
    case TRAINER_CLASS_ELITE_FOUR:   return 1 << 1;
    case TRAINER_CLASS_CHAMPION:     return 1 << 2;
    case TRAINER_CLASS_RIVAL:        return 1 << 3;
    case TRAINER_CLASS_MAGMA_LEADER: return 1 << 4;
    case TRAINER_CLASS_AQUA_LEADER:  return 1 << 5;
    default:                         return 0;
    }
}

// Patchwork Team (RND-007): six party members caught on six different
// routes. MON_DATA_MET_LOCATION is the region map section a Pokemon was
// caught/received on -- a coarser unit than "route" for town/city entries,
// but a reasonable and already-available proxy, the same kind of
// simplification Achievement_AllPrimaryTypesDistinct's "primary type only"
// already accepts for team-composition entries.
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

// Complete Reinvention (NGP-012): records the current Gym's party species
// into AchievementRunDataExt.gymSpeciesUsedThisCycle (deduplicated), and
// reports whether any of them had already appeared at an earlier Gym this
// cycle -- the caller latches that into the sticky reinventionBroken flag,
// the same idiom used elsewhere for mono-type/type-roulette/etc.
static bool8 Achievement_RecordGymSpeciesUsed(struct AchievementRunDataExt *runDataExt, struct Pokemon *party, u8 playerCount)
{
    u16 curSpecies[PARTY_SIZE];
    bool8 overlap = FALSE;
    u8 i, j;

    Achievement_SnapshotPartySpecies(party, playerCount, curSpecies);

    for (i = 0; i < PARTY_SIZE; i++)
    {
        bool8 alreadyListed = FALSE;

        if (curSpecies[i] == SPECIES_NONE)
            continue;

        for (j = 0; j < runDataExt->gymSpeciesUsedThisCycleCount; j++)
        {
            if (runDataExt->gymSpeciesUsedThisCycle[j] == curSpecies[i])
            {
                alreadyListed = TRUE;
                break;
            }
        }

        if (alreadyListed)
            overlap = TRUE;
        else if (runDataExt->gymSpeciesUsedThisCycleCount < ARRAY_COUNT(runDataExt->gymSpeciesUsedThisCycle))
            runDataExt->gymSpeciesUsedThisCycle[runDataExt->gymSpeciesUsedThisCycleCount++] = curSpecies[i];
    }

    return overlap;
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

// Hands off to src/achievement_popup.c's own ring buffer, which drains one
// popup at a time once the field is in a safe state to show it.
// Achievement_TryComplete has already committed the flag and points
// unconditionally by the time this runs, so nothing here can ever withhold
// an award -- at worst, a full queue drops the toast, never the achievement
// itself.
static void QueueAchievementNotification(u16 achievementId)
{
    AchievementPopup_Enqueue(achievementId);
}

bool8 Achievement_ProfileWriteFailed(void)
{
    return sAchievementProfileWriteFailed;
}

// Call sites: the overworld at the same safe point the
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

// Category J: the one self-referential achievement. Called from
// the tail of Achievement_TryComplete, after totalPointsEarned has already
// been updated for whatever achievement just completed. Safe to recurse
// through Achievement_TryComplete -- its own Achievement_IsCompleted guard
// makes the recursive call a no-op after the first time, and there's only
// one such meta-achievement, so there's no chain to unwind.
// Scaled up from 1000 to 2000 (10% of the catalog's 20,000-point total, see
// src/data/achievements.h's own comment on the rescale) -- the old threshold
// was sized for the pre-rescale ~8,600-point catalog.
static void Achievement_CheckPointMilestones(void)
{
    if (gAchievementProfile.totalPointsEarned >= 2000)
        Achievement_TryComplete(ACHIEVEMENT_POINTS_2000);
}

// The flag and the points are written together, before any
// UI is involved, so a UI failure can never withhold an already-earned
// reward, and a reset can never cause a double award.
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
    // No Easy Path (PRO-012): a separate running total of points
    // earned specifically from Gold-or-better achievements.
    if (gAchievements[achievementId].tier >= ACHIEVEMENT_TIER_GOLD)
        gAchievementProfile.pointsFromGoldOrBetter += gAchievements[achievementId].points;
    sAchievementProfileDirty = TRUE;

    QueueAchievementNotification(achievementId);

    Achievement_CheckPointMilestones();
    Achievement_CheckMasteryMilestones();

    return TRUE;
}

// Called from GameClear() the one time
// FLAG_SYS_GAME_CLEAR is newly set for this save (see the declaration in
// achievements.h for why no completion guard is needed here). Flushes
// immediately rather than waiting for the next flush point, same as
// the boost purchase/reset mutators -- this is a rare, important state
// change, not a hot path.
void Achievement_OnFirstPlaythroughComplete(void)
{
    gAchievementProfile.boostsUnlocked = TRUE;

    // This function runs on every Hall of Fame clear,
    // including every NG+ cycle's clear (see the call site's comment in
    // post_battle_event_funcs.c), so an ungated increment here would let
    // playthroughsCompleted creep up from NG+ cycles alone. NG+ progress
    // already has its own dedicated counter and achievements
    // (ngPlusCyclesCompleted, ACHIEVEMENT_NG_PLUS_*), so gate this one to
    // cycle 0 to keep "playthroughs" meaning distinct fresh saves
    // specifically, with no overlap between the two tracks.
    // ACHIEVEMENT_PLAYTHROUGHS_2/_5, the two achievements this counter used
    // to back, are removed -- see src/data/achievements.h's own comment.
    // playthroughsCompleted itself still increments; it's shown on the debug
    // profile dump (src/debug.c) independent of any achievement.
    if (gSaveBlock2Ptr->newGamePlus == 0)
        gAchievementProfile.playthroughsCompleted++;

    if (gSaveBlock1Ptr->nuzlockeModeEnabled)
        gAchievementProfile.nuzlockesCompleted++;

    if (FlagGet(FLAG_RANDOMIZE_MON) || FlagGet(FLAG_RANDOMIZE_TYPE) || FlagGet(FLAG_RANDOMIZE_MOVES))
        gAchievementProfile.randomizedRunsCompleted++;

    // Category J: multi-run milestones derived from the counters
    // just updated above.
    if (gAchievementProfile.nuzlockesCompleted >= 1)
        Achievement_TryComplete(ACHIEVEMENT_NUZLOCKE_1);
    // ACHIEVEMENT_NUZLOCKE_3 ("complete 3 Nuzlocke runs")
    // removed -- NUZLOCKE_1 above already is the "do it once" version, and
    // asking for the same challenge repeated is exactly what this
    // removes across the catalog. nuzlockesCompleted itself is left alone;
    // Full Circle (VARIETY_FULL_CIRCLE) below still reads it.
    if (gAchievementProfile.randomizedRunsCompleted >= 1)
        Achievement_TryComplete(ACHIEVEMENT_RANDOMIZED_1);

    // Chaos Begins/Truly Random/Pure Chaos/
    // Species-Type-Move Chaos, all read directly off the flags/difficulty
    // this exact completion ran under. Chaos Begins is also checked in
    // Achievement_OnNewGamePlusStarted (the actual "begin" event for cycle
    // >= 1); this is the only equivalent for the very
    // first playthrough, which never calls that function.
    // ACHIEVEMENT_RANDOMIZER_SEED_EXPLORER (2 randomized
    // playthroughs) and _VETERAN (5) removed -- ACHIEVEMENT_RANDOMIZED_1
    // above is already the "do it once" version of this same ladder.
    // Chaos Begins deliberately only needs ANY ONE of the
    // three randomizer flags (Achievement_AnyRandomizerFlagSet) -- it's the
    // bronze entry point to the ladder Truly Random gold-tiers below with
    // ALL three. Catalog descriptions (src/data/achievements.h) now spell
    // this out explicitly instead of the old ambiguous "a randomized
    // playthrough" wording.
    if (Achievement_AnyRandomizerFlagSet())
        Achievement_TryComplete(ACHIEVEMENT_RANDOMIZER_CHAOS_BEGINS);
    if (FlagGet(FLAG_RANDOMIZE_MON) && FlagGet(FLAG_RANDOMIZE_TYPE) && FlagGet(FLAG_RANDOMIZE_MOVES))
    {
        Achievement_TryComplete(ACHIEVEMENT_RANDOMIZER_TRULY_RANDOM);
        if (gSaveBlock1Ptr->difficulty == DIFFICULTY_HARD && !FlagGet(FLAG_LEVEL_CAP_OFF))
            Achievement_TryComplete(ACHIEVEMENT_RANDOMIZER_PURE_CHAOS);
    }
    if (FlagGet(FLAG_RANDOMIZE_MON))
        Achievement_TryComplete(ACHIEVEMENT_RANDOMIZER_SPECIES_CHAOS);
    if (FlagGet(FLAG_RANDOMIZE_TYPE))
        Achievement_TryComplete(ACHIEVEMENT_RANDOMIZER_TYPE_CHAOS);
    if (FlagGet(FLAG_RANDOMIZE_MOVES))
        Achievement_TryComplete(ACHIEVEMENT_RANDOMIZER_MOVE_CHAOS);

    // Full Circle (VAR-007) bookkeeping: a "conventional" completion is one
    // that was neither a Nuzlocke nor randomized.
    if (!gSaveBlock1Ptr->nuzlockeModeEnabled && !Achievement_AnyRandomizerFlagSet())
        gAchievementProfile.completedConventionalRun = TRUE;
    if (gAchievementProfile.completedConventionalRun
     && gAchievementProfile.nuzlockesCompleted >= 1
     && gAchievementProfile.randomizedRunsCompleted >= 1)
        Achievement_TryComplete(ACHIEVEMENT_VARIETY_FULL_CIRCLE);

    // No Nostalgia (NGP-011) bookkeeping: only when this completion was NOT
    // an NG+ cycle -- Achievement_OnNewGamePlusCycleCompleted (called right
    // after this, from the same GameClear branch, whenever newGamePlus > 0)
    // owns previousCyclePartySpecies the rest of the time.
    // previousCyclePartySpecies is seeded here (cycle 0 has no earlier
    // Achievement_OnNewGamePlusCycleCompleted call to do it) so cycle 1's
    // completion always has something valid to compare against.
    //
    // This branch used to also reset
    // consecutiveNgPlusCyclesCompleted for ACHIEVEMENT_NG_PLUS_ESCALATION
    // ("3 consecutive NG+ cycles"), removed along with the rest of the NG+
    // repeat-count ladder -- see Achievement_OnNewGamePlusCycleCompleted for
    // the single "beat one NG+ cycle" achievement that replaced it. The
    // consecutiveNgPlusCyclesCompleted field itself is left in place, unused,
    // to avoid reshuffling every later AchievementProfile field's offset.
    if (gSaveBlock2Ptr->newGamePlus == 0)
    {
        struct AchievementRunDataExt *runDataExt = &gSaveBlock2Ptr->achievementRunDataExt;
        u16 curSpecies[PARTY_SIZE];

        // ACHIEVEMENT_VARIETY_NEW_TEAM_NEW_ME's own
        // disjoint-species comparison/TryComplete removed here (see
        // src/data/achievements.h's own comment) -- but this snapshot must
        // stay: it's how previousCyclePartySpecies gets seeded for cycle 0
        // (Achievement_OnNewGamePlusCycleCompleted's No Nostalgia check has
        // no earlier call to do it otherwise -- see this function's own
        // comment above).
        Achievement_SnapshotPartySpecies(gParties[B_TRAINER_PLAYER], gPartiesCount[B_TRAINER_PLAYER], curSpecies);
        memcpy(runDataExt->previousCyclePartySpecies, curSpecies, sizeof(curSpecies));
        runDataExt->previousCyclePartySpeciesSet = TRUE;
    }

    // ACHIEVEMENT_VARIETY_REPLAY_MASTER ("complete five
    // playthroughs under five different rule configurations") removed here
    // -- see src/data/achievements.h's own comment. Its dedicated tracking
    // (Achievement_ChallengeConfigSignature, playthroughConfigsSeen[]/_Count)
    // is removed with it; the persisted profile fields are left in place,
    // marked unused (include/achievements.h).

    sAchievementProfileDirty = TRUE;
    Achievement_FlushProfile();
}

// Flushes immediately, same as the function above --
// starting a new NG+ cycle is rare and important, not a hot path.
void Achievement_OnNewGamePlusStarted(u8 cycle)
{
    struct AchievementRunDataExt *runDataExt = &gSaveBlock2Ptr->achievementRunDataExt;

    if (cycle > gAchievementProfile.highestNgPlusCycle)
        gAchievementProfile.highestNgPlusCycle = cycle;

    // ACHIEVEMENT_NG_PLUS_STARTED ("start a New Game+"),
    // _CYCLE_3 ("reach cycle 3"), _CYCLE_5, and _BEYOND_THE_BEGINNING
    // ("reach cycle 10") -- the whole "start/reach" half of the old NG+
    // repeat-count ladder -- are removed, folded into the single
    // ACHIEVEMENT_NG_PLUS_CYCLE_COMPLETE achievement checked in
    // Achievement_OnNewGamePlusCycleCompleted below (that function only
    // fires once a cycle is actually beaten, which is a more meaningful
    // "did the thing" moment than merely starting one). highestNgPlusCycle
    // is left as a high-water mark for the debug dump; Chaos Begins (the
    // only achievement this function still checks) is still relevant here
    // since NG+ can be started with randomizer flags freshly toggled on.
    if (Achievement_AnyRandomizerFlagSet())
        Achievement_TryComplete(ACHIEVEMENT_RANDOMIZER_CHAOS_BEGINS);

    // Zero every per-cycle-scoped AchievementRunDataExt field -- see that
    // struct's own comment for why ClearSav1 can't do this for us here.
    // previousCyclePartySpecies is deliberately left untouched.
    runDataExt->trainersDefeatedThisCycle = 0;
    runDataExt->gymSpeciesUsedThisCycleCount = 0;
    runDataExt->reinventionBroken = FALSE;
    runDataExt->majorBossClassesDefeatedThisCycle = 0;

    sAchievementProfileDirty = TRUE;
    Achievement_FlushProfile();
}

// See the header doc comment for why this is separate
// from Achievement_OnFirstPlaythroughComplete rather than folded into it.
void Achievement_OnNewGamePlusCycleCompleted(void)
{
    struct AchievementRunData *runData = &gSaveBlock1Ptr->achievementRunData;
    struct AchievementRunDataExt *runDataExt = &gSaveBlock2Ptr->achievementRunDataExt;
    struct Pokemon *party = gParties[B_TRAINER_PLAYER];
    u8 playerCount = gPartiesCount[B_TRAINER_PLAYER];
    u16 curSpecies[PARTY_SIZE];

    gAchievementProfile.ngPlusCyclesCompleted++;

    // This function only ever runs when an NG+ cycle was
    // just beaten, so -- like ACHIEVEMENT_NG_PLUS_STARTED before it (see
    // Achievement_OnNewGamePlusStarted) -- the "beat one NG+ cycle"
    // achievement needs no threshold guard. This single achievement replaces
    // the old ACHIEVEMENT_NG_PLUS_STARTED/_CYCLE_3/_CYCLE_5/_COMPLETED_3
    // (category J) and _ONE_MORE_TIME/_BEYOND_THE_BEGINNING/_ESCALATION
    // (category O) seven-entry ladder -- asking players to repeat the same
    // long task (another full playthrough) over and over for more points
    // was grindy rather than a genuine additional challenge.
    Achievement_TryComplete(ACHIEVEMENT_NG_PLUS_CYCLE_COMPLETE);

    // ACHIEVEMENT_NG_PLUS_TEN_CYCLES_DEEP
    // (ngPlusCyclesCompleted >= 10) removed -- see the catalog entry's own
    // comment (src/data/achievements.h).
    // Unassisted Cycle used to require this be
    // specifically cycle 2 (an exact-equality check, since "boosts
    // disabled" isn't monotonic across cycles the way a plain count is) --
    // now that the ladder above only asks for one cycle, any cycle
    // completed with boosts off qualifies, same "do it once" treatment.
    if (!gAchievementProfile.boostsEnabled)
        Achievement_TryComplete(ACHIEVEMENT_NG_PLUS_UNASSISTED_CYCLE);

    if (Achievement_CountChallengeModifiers() >= 3)
        Achievement_TryComplete(ACHIEVEMENT_NG_PLUS_CYCLE_SPECIALIST);

    if (gSaveBlock1Ptr->nuzlockeModeEnabled)
        Achievement_TryComplete(ACHIEVEMENT_NG_PLUS_CYCLE_NUZLOCKE);
    // ACHIEVEMENT_NG_PLUS_ENDLESS_SURVIVOR (NG+ cycle 5+
    // with Nuzlocke and the randomizer) removed -- see the catalog entry's
    // own comment (src/data/achievements.h).

    // Complete Reinvention/Boss Gauntlet: bookkeeping accumulated all cycle
    // by Achievement_CheckChallengeMilestones (HandleEndTurn_BattleWon).
    if (runData->gymBattlesWon >= NUM_BADGES && !runDataExt->reinventionBroken)
        Achievement_TryComplete(ACHIEVEMENT_NG_PLUS_COMPLETE_REINVENTION);
    if (runDataExt->majorBossClassesDefeatedThisCycle == ACHIEVEMENT_BOSS_GAUNTLET_ALL_CLASSES)
        Achievement_TryComplete(ACHIEVEMENT_NG_PLUS_BOSS_GAUNTLET);

    // No Nostalgia (NGP-011): compare against the snapshot from the PRIOR
    // completion (seeded either by this same function last cycle, or by
    // Achievement_OnFirstPlaythroughComplete's cycle-0 case for cycle 1's
    // first comparison) before overwriting it with this cycle's.
    Achievement_SnapshotPartySpecies(party, playerCount, curSpecies);
    if (runDataExt->previousCyclePartySpeciesSet
     && Achievement_SpeciesSetsDisjoint(curSpecies, runDataExt->previousCyclePartySpecies))
    {
        Achievement_TryComplete(ACHIEVEMENT_NG_PLUS_NO_NOSTALGIA);
        // ACHIEVEMENT_VARIETY_NEW_TEAM_NEW_ME removed --
        // this was its NG+-cycle half (Achievement_OnFirstPlaythroughComplete's
        // cycle-0 branch had the other half). See src/data/achievements.h's
        // own comment.
    }
    memcpy(runDataExt->previousCyclePartySpecies, curSpecies, sizeof(curSpecies));
    runDataExt->previousCyclePartySpeciesSet = TRUE;

    // ACHIEVEMENT_NG_PLUS_CYCLE_COLLECTOR (three distinct
    // challenge-configuration signatures across completed NG+ cycles)
    // removed -- see the catalog entry's own comment (src/data/achievements.h).
    // gAchievementProfile.ngPlusConfigsSeen[]/ngPlusConfigsSeenCount are now
    // unwritten but left in place.

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

// Refuses at the first failed check rather than
// collecting all failures, since the caller (the boost shop) only
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
// achievements/points, not spending points already
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

// pointsInvested can only grow here, and
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

    // Boost Investor/Full Investment/Selective Mastery/Meta-Prog
    // Master all depend on boostLevels[]/pointsInvested, which only ever
    // change here and in AchievementBoost_Reset below -- neither is reached
    // from Achievement_TryComplete's tail, so it needs its own call site.
    Achievement_CheckBoostMilestones();

    Achievement_FlushProfile();

    return TRUE;
}

// Refuses at the first failed check, same style as
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

// The refund is exactly
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

    // Reconfigured only ever needs re-checking after a reset (its
    // "rebuild" half is re-checked from AchievementBoost_Purchase above).
    Achievement_CheckBoostMilestones();

    Achievement_FlushProfile();

    return TRUE;
}

// The first real boost effect, and the shape every
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

// ---- The remaining numerical/binary boosts ------------------------------
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

// ---- The first ten catalog hook functions -------------------------------

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

    // Who Needs Centers?, checked at the exact
    // moment of the 5th badge -- the same checkpoint the table above already
    // uses for ACHIEVEMENT_BADGE_HEAT.
    if (FlagGet(FLAG_BADGE05_GET) && GetGameStat(GAME_STAT_USED_POKECENTER) == 0)
        Achievement_TryComplete(ACHIEVEMENT_CHALLENGE_WHO_NEEDS_CENTERS);

    // Piggyback on this same callnative for party-state checks that
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

        // Local Expert. Piggybacks on this
        // existing FLAG_SET_SEEN branch (this same HandleSetPokedexFlag
        // call site) rather than adding a new hook.
        Achievement_CheckLocalExpert();
    }
    else
    {
        // The 10/25/50% catch thresholds were folded into
        // Achievement_CheckCaptureMilestones's hard-number ladder below.
        // Full dex completion stays here, since it's a distinct-species
        // count (this function's own GetNationalPokedexCount) rather than
        // the raw capture count that ladder tracks.
        count = GetNationalPokedexCount(FLAG_GET_CAUGHT);
        if (count >= NATIONAL_DEX_COUNT)
            Achievement_TryComplete(ACHIEVEMENT_CATCH_ALL);
    }
}

// GAME_STAT_POKEMON_CAPTURES is already incremented for the current catch by
// the time GiveCapturedMonToPlayer (this function's only caller) runs --
// confirmed against data/battle_scripts_2.s, where incrementgamestat
// precedes givecaughtmon.
//
// Collapsed from five thresholds (1/25/100/250/500) down to
// three -- the fourth tier, Diamond, is ACHIEVEMENT_CATCH_ALL, checked by
// Achievement_CheckPokedexMilestones instead, since "catch them all" is a
// distinct-species condition, not a raw-count one.
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
//
// Reads gAchievementProfile.trainerBattlesLifetime instead
// of GAME_STAT_TRAINER_BATTLES -- the game stat lives in SaveBlock1, which
// ClearSav1 zeroes at every new game, so these thresholds used to reset
// every playthrough instead of counting across all of them. Incremented
// once here, since this function runs exactly once per battle end.
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

// Same reasoning as the trainer version above, reading
// gAchievementProfile.wildBattlesLifetime from CB2_EndWildBattle instead of
// GAME_STAT_WILD_BATTLES.
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

// GAME_STAT_HATCHED_EGGS is already incremented well before Task_EggHatch
// (this function's only caller) reaches the point where the hatched mon's
// data is valid -- see src/field_control_avatar.c, at the very start of the
// hatch sequence.
//
// The count-based thresholds (It's Hatching! / Daycare
// Regular / Egg Factory, and Egg Marathon below) read
// gAchievementProfile.eggsHatchedLifetime, incremented once per call, rather
// than GAME_STAT_HATCHED_EGGS -- the game stat lives in SaveBlock1, which
// ClearSav1 zeroes at every new game. Shiny From the Shell is a boolean
// condition and is unaffected either way.
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

    // Egg Marathon. Same lifetime count as the
    // thresholds above.
    if (count >= 100)
        Achievement_TryComplete(ACHIEVEMENT_RECORD_EGG_MARATHON);
}

// ---- Battle Mastery (category K) ----------------------------------------
//
// struct AchievementBattleData is EWRAM-only and never saved -- a battle
// never spans a save, so nothing here belongs in AchievementRunData. Zeroed
// by Achievement_ClearBattleData (BattleStartClearSetData, src/battle_main.c)
// at the start of every battle, and read exactly once, by
// Achievement_CheckBattleMilestones (HandleEndTurn_BattleWon, same file).
// That single evaluation point means every category K entry is only
// ever checked in a battle the player actually won (landing a crit in a
// battle that's then lost doesn't earn Critical Success) -- a deliberate
// simplification to keep this to the "one entry point" discipline this file
// uses throughout, not an attempt to track "did this ever happen this run".
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
    // Set by Achievement_RecordPlayerFaint the
    // moment the player is down to exactly one conscious Pokemon, for
    // Comeback Count. Like every other field here, per-battle only -- read
    // (and implicitly reset, since the whole struct is zeroed at the start
    // of the next battle) by Achievement_CheckBattleRecordsMilestones.
    bool8 wasDownToLastMon:1;
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
    // ACHIEVEMENT_BATTLE_TYPE_MASTER ("win a trainer battle
    // without landing a super-effective hit") removed -- most trainer teams
    // aren't built to counter the player, so this happens by chance.

    // ACHIEVEMENT_BATTLE_CLEAN_SWEEP/_PERFECT_SWEEP were
    // trivial against the many trainers who only field one or two Pokemon,
    // so both now additionally require the opponent to have brought a full
    // 6-Pokemon team.
    if (isTrainerBattle && gBattleResults.playerSwitchesCounter == 0
        && gPartiesCount[B_TRAINER_OPPONENT_A] == PARTY_SIZE)
    {
        Achievement_TryComplete(ACHIEVEMENT_BATTLE_CLEAN_SWEEP);
        if (isMajorBattle)
            Achievement_TryComplete(ACHIEVEMENT_BATTLE_PERFECT_SWEEP);
    }

    if (isTrainerBattle && !gBattleResults.playerMonWasDamaged)
    {
        // ACHIEVEMENT_BATTLE_NO_DAMAGE, same full-team
        // requirement as CLEAN_SWEEP above. ACHIEVEMENT_BATTLE_UNTOUCHABLE
        // below is unaffected -- it never got this requirement.
        if (gPartiesCount[B_TRAINER_OPPONENT_A] == PARTY_SIZE)
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

    if (isMajorBattle && gBattleResults.battleTurnCounter >= 30)
        Achievement_TryComplete(ACHIEVEMENT_BATTLE_ATTRITION);

    // Strategic Victory requires a full 6-Pokemon opposing team --
    // trivial to avoid any faints against a boss who only fields one or two.
    if (isMajorBattle && gBattleResults.playerFaintCounter == 0
     && gPartiesCount[B_TRAINER_OPPONENT_A] == PARTY_SIZE)
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

    // No STAB Needed requires a full 6-Pokemon opposing team --
    // trivial to avoid STAB for a battle or two against a small trainer team.
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

    // Both require a full 6-Pokemon party -- trivial to
    // end a battle with only one Pokemon conscious if that's all you brought.
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
// The first real user of struct AchievementRunData (include/global.h) --
// see that struct's own comment for what each field tracks. Species sets are
// tracked by species ID, not full individual identity (personality/OT), the
// same granularity struct AchievementBattleData already tracks party members at.

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

    // One of Each, moved here from Achievement_CheckRecordsMilestones (see the
    // note left in its place there for why the map-transition sampling point
    // was too expensive). Obtaining a Pokemon is the only thing that can raise
    // the distinct-species count, so this is a strictly better trigger than
    // sampling live state: it fires exactly when the threshold can be crossed.
    //
    // The Achievement_IsCompleted guard is what keeps the storage walk from
    // running again on every subsequent catch once the achievement is already
    // earned -- Achievement_TryComplete is idempotent, but the scan feeding it
    // is not free.
    if (!Achievement_IsCompleted(ACHIEVEMENT_COLLECT_ONE_OF_EACH)
     && Achievement_CountDistinctOwnedSpecies(gParties[B_TRAINER_PLAYER], gPartiesCount[B_TRAINER_PLAYER], 10) >= 10)
        Achievement_TryComplete(ACHIEVEMENT_COLLECT_ONE_OF_EACH);
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
            // Requires a full 6-Pokemon party --
            // trivial to keep 1-2 Pokemon mono-type by accident. Doesn't
            // gate monoTypeGymsCleared itself, since ACHIEVEMENT_TEAM_ONE_TYPE_JOURNEY
            // below never got this requirement.
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

        // Requires a full 6-Pokemon party -- a small
        // party trivially has a low combined base stat total.
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
        // Requires a full 6-Pokemon party -- trivial
        // for no two party members to share a type if there's barely any
        // party to begin with.
        if (playerCount == PARTY_SIZE && Achievement_AllTypesDisjoint(party, playerCount))
            Achievement_TryComplete(ACHIEVEMENT_TEAM_NO_DUPLICATES);

        if (playerCount == PARTY_SIZE && monoType != NUMBER_OF_MON_TYPES)
            Achievement_TryComplete(ACHIEVEMENT_TEAM_SIX_OF_A_KIND);

        // ACHIEVEMENT_TEAM_VARIETY_IS_POWER ("win a major
        // battle without two of the same species") removed -- most players
        // never deliberately catch duplicate species for their party, so
        // this was true of nearly every team. Achievement_HasDuplicateSpecies
        // (the only caller of which this was) removed along with it.

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
// and be missed, the same fidelity tradeoff struct AchievementBattleData's
// per-battle snapshots already accept elsewhere in this file.
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

// ---- Exploration, Economy & Collection (category M) --------------------
// See include/achievements.h for the call-site breakdown; this
// section only adds one battle-hook-free helper style beyond what's already
// established elsewhere -- Achievement_AddToGameStat, since
// IncrementGameStat (src/overworld.c) only ever adds 1 and GAME_STAT_MONEY_SPENT/
// GAME_STAT_ITEM_SALES_MONEY both need to add a variable amount.
static void Achievement_AddToGameStat(u8 index, u32 amount)
{
    u32 value = GetGameStat(index);

    if (0xFFFFFFFF - value < amount)
        value = 0xFFFFFFFF;
    else
        value += amount;

    SetGameStat(index, value);
}

// The 16 town/city flags this fork already tracks (include/constants/flags.h),
// reused as-is -- see that header for why On the Road/Completionist Tourist
// need no tracking of their own.
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

// LoadCurrentMapData (src/overworld.c) -- see that function's comment and
// AchievementRunDataExt.mapsVisited's comment (include/global.h, SaveBlock2)
// for why this tracks (mapGroup, mapNum) pairs instead of a raw-mapNum
// bitfield, and why this data lives in SaveBlock2 rather than
// AchievementRunData (SaveBlock1).
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

    // "Before entering the League": gated on the Champion not yet beaten,
    // the same flag category A's ACHIEVEMENT_STORY_CHAMPION already keys
    // off. Checked on every map load, so it can only ever
    // complete while that's still true.
    if (allTownsVisited && !FlagGet(FLAG_IS_CHAMPION))
        Achievement_TryComplete(ACHIEVEMENT_EXPLORE_COMPLETIONIST_TOURIST);

    if (FlagGet(FLAG_SYS_POKEDEX_GET) && FlagGet(FLAG_SYS_POKENAV_GET) && FlagGet(FLAG_SYS_B_DASH))
        Achievement_TryComplete(ACHIEVEMENT_EXPLORE_NO_LOOSE_ENDS);

    // Same call site -- map transitions are
    // frequent enough to double as a "live state" sampling point.
    // See that function's own doc comment.
    Achievement_CheckRecordsMilestones();
}

// Achievement_CheckPokedexMilestones's FLAG_SET_SEEN branch (category B)
// -- only the current map's wild encounter table, unioned
// across every time-of-day variant (species availability changes by time of
// day; Pokedex "seen" state does not, so the achievement needs the union to
// avoid missing a night-only species from an achievement checked at noon).
// Land/water/rock/fishing tables only -- hiddenMonsInfo (DexNav-only
// encounters) is deliberately excluded, the same "not a normal wild
// encounter" reasoning Rare Find (below) treats as a distinct condition.
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
        u8 areaCounts[4] = { LAND_WILD_COUNT, WATER_WILD_COUNT, ROCK_WILD_COUNT, FISH_WILD_COUNT };

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
        if (!GetSetPokedexFlag(SpeciesToNationalPokedexNum(scratch[i]), FLAG_GET_SEEN))
            return;
    }

    Achievement_TryComplete(ACHIEVEMENT_EXPLORE_LOCAL_EXPERT);
}

// SetHiddenItemFlag (src/field_specials.c) -- already only reached once per
// item (see that function's own comment).
//
// Treasure Hunter/Treasure Hoard read
// gAchievementProfile.hiddenItemsFoundLifetime instead of
// GAME_STAT_HIDDEN_ITEMS_FOUND -- the game stat lives in SaveBlock1, which
// ClearSav1 zeroes at every new game. The game stat is still incremented
// below in case anything else reads it.
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

// GetInteractionScript's object-event branch (src/field_control_avatar.c).
//
// Talk to the Locals/People Person read
// gAchievementProfile.npcsTalkedToLifetime instead of
// GAME_STAT_NPCS_TALKED_TO -- the game stat lives in SaveBlock1, which
// ClearSav1 zeroes at every new game. The game stat is still incremented
// below in case anything else reads it.
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

// BuyMenuSubtractMoney (src/shop.c), called after the vanilla
// IncrementGameStat(GAME_STAT_SHOPPED) that site already does.
//
// First Purchase/Regular Customer and Big Spender/Whale
// read gAchievementProfile.shopPurchasesLifetime/moneySpentLifetime instead
// of GAME_STAT_SHOPPED/GAME_STAT_MONEY_SPENT -- both game stats live in
// SaveBlock1, which ClearSav1 zeroes at every new game. GAME_STAT_MONEY_SPENT
// is still updated below in case anything else reads it; GAME_STAT_SHOPPED's
// own increment is unchanged, at its existing call site in src/shop.c.
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

    if (total >= 50000)
        Achievement_TryComplete(ACHIEVEMENT_ECONOMY_TREASURE_PAYS);
}

// Every non-key-item Bag pocket, via gBagPockets (include/item.h) rather
// than struct Bag's named arrays directly -- one loop over POCKETS_COUNT
// instead of five hardcoded ones.
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

// AddBagItem (src/item.c), the same "added succeeded" guard
// Achievement_CheckItemMilestones already sits behind.
void Achievement_CheckPackRatMilestone(void)
{
    if (Achievement_CountDistinctBagItems() >= 20)
        Achievement_TryComplete(ACHIEVEMENT_EXPLORE_PACK_RAT);
}

// ObjectEventInteractionPickBerryTree (src/berry.c).
void Achievement_RecordBerryHarvest(void)
{
    IncrementGameStat(GAME_STAT_BERRIES_HARVESTED);

    if (GetGameStat(GAME_STAT_BERRIES_HARVESTED) >= 50)
        Achievement_TryComplete(ACHIEVEMENT_COLLECT_GREEN_THUMB);
}

// Achievement_CheckTradeMilestones removed along with its
// sole achievement, ACHIEVEMENT_COLLECT_TRADE_SECRETS -- see
// include/achievements.h and its two former call sites in src/trade.c.

// Both GAME_STAT_EVOLVED_POKEMON sites (src/evolution_scene.c).
void Achievement_CheckEvolutionCountMilestones(void)
{
    u32 count = GetGameStat(GAME_STAT_EVOLVED_POKEMON);

    if (count >= 10)
        Achievement_TryComplete(ACHIEVEMENT_COLLECT_EVOLUTIONARY_PATH);
    if (count >= 25)
        Achievement_TryComplete(ACHIEVEMENT_COLLECT_EVOLUTION_EXPERT);
}

// GetEvolutionTargetSpecies's DO_EVO path (src/pokemon.c) -- gating (the
// matched evolution's params actually containing IF_MIN_FRIENDSHIP, and
// evoState == DO_EVO so a mere eligibility check never awards this) lives at
// the call site; see the header doc comment.
void Achievement_RecordFriendshipEvolution(void)
{
    Achievement_TryComplete(ACHIEVEMENT_COLLECT_FRIENDSHIP_BLOSSOMS);
}

// PokemonUseItemEffects's ITEM4_EVO_STONE case (src/pokemon.c) -- gating
// (the item actually being one of the stone items) lives at the call site.
void Achievement_RecordStoneEvolution(void)
{
    Achievement_TryComplete(ACHIEVEMENT_COLLECT_STONE_AGE);
}

// GiveCapturedMonToPlayer (src/pokemon.c) -- gating (gDexNavSpecies != SPECIES_NONE)
// lives at the call site; see the header doc comment.
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

// HandleEndTurn_BattleWon (src/battle_main.c), immediately after
// Achievement_CheckTeamMilestones -- not a new battle hook, see the header
// doc comment. shoppedSinceLastGym/consecutiveGymsNoShopping mirror Fresh
// Start's "since the last Gym" window.
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

        // The window always resets here, win or not -- same convention
        // Fresh Start's ring buffer uses.
        runData->shoppedSinceLastGym = FALSE;
    }

    // ACHIEVEMENT_ECONOMY_RESOURCEFUL ("win a major battle
    // carrying fewer than five consumables") removed -- most players don't
    // stock up on more than a few consumables to begin with.
    // Achievement_CountConsumableItems (the only caller of which this was)
    // removed along with it.
}

// GameClear (src/post_battle_event_funcs.c), alongside
// Achievement_CheckTeamCompletionMilestones.
void Achievement_CheckEconomyCompletionMilestones(void)
{
    if (GetMoney(&gSaveBlock1Ptr->money) >= 500000)
        Achievement_TryComplete(ACHIEVEMENT_ECONOMY_INVESTOR);
}

// ---- Challenge Runs & Nuzlocke (category N) -----------------------------
//
// See include/constants/achievements.h's category N doc comment for the
// four call sites. Two latent gaps had to be fixed before this roster
// could read anything real from them: GAME_STAT_USED_POKECENTER (declared
// since early on, never incremented -- fixed at FldEff_PokecenterHeal,
// src/field_effect.c) and gBattleResults.numHealingItemsUsed (declared,
// read by src/tv.c, never written -- fixed at BS_ItemRestoreHP,
// src/battle_script_commands.c, alongside the
// Achievement_RecordReviveUsed hook).

// The seven New Game Settings that make a run harder (explicit state only,
// never incidental behaviour). Debug Mode is deliberately excluded -- it
// doesn't make a run harder, it makes it ineligible (achievementsBlocked).
// Stat Editor and Level Cap Off are counted here only in their harder state
// (disallowed / still on); flipping either the other way sets
// achievementsBlocked too, same as Debug Mode -- see
// ApplyPendingNewGameSettings, src/new_game_settings_menu.c.
static u8 Achievement_CountChallengeModifiers(void)
{
    u8 count = 0;

    if (gSaveBlock1Ptr->nuzlockeModeEnabled)
        count++;
    if (gSaveBlock1Ptr->difficulty == DIFFICULTY_HARD)
        count++;
    if (FlagGet(FLAG_RANDOMIZE_MON))
        count++;
    if (FlagGet(FLAG_RANDOMIZE_TYPE))
        count++;
    if (FlagGet(FLAG_RANDOMIZE_MOVES))
        count++;
    if (!FlagGet(FLAG_LEVEL_CAP_OFF))
        count++;
    if (!FlagGet(FLAG_ALLOW_STAT_EDITOR))
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

// Achievement_HasDuplicateEvolutionFamilyAmongOwned (Species
// Clause's sole helper, along with its static EWRAM scratch array) removed
// along with ACHIEVEMENT_NUZLOCKE_SPECIES_CLAUSE -- see
// Achievement_CheckNuzlockeCompletionMilestones.

// HandleEndTurn_BattleWon (src/battle_main.c), immediately after
// Achievement_CheckGymEconomyMilestones, gated the same way (never link/
// recorded). Covers every Challenge-category entry evaluated battle-by-
// battle, plus the running bookkeeping Achievement_CheckChallengeCompletionMilestones
// reads at GameClear.
//
// Also rides this same call site (see include/constants/achievements.h's
// category O comment for why) rather than adding a new one -- Random by
// Nature/Chaos Team/Never Seen It Coming/Patchwork Team, the
// trainer-win/Boss-Gauntlet/Complete-Reinvention bookkeeping
// Achievement_OnNewGamePlusCycleCompleted reads at GameClear, and Fresh
// Faces/Never the Same Fight, checked immediately on crossing their
// threshold rather than waiting for cycle-complete.
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

    // Fresh Faces/Never the Same Fight -- every trainer win, not
    // only major/Gym ones.
    if (isTrainerBattle)
    {
        if (runDataExt->trainersDefeatedThisCycle < 0xFFFF)
            runDataExt->trainersDefeatedThisCycle++;
        if (gSaveBlock2Ptr->newGamePlus > 0 && runDataExt->trainersDefeatedThisCycle >= 50)
            Achievement_TryComplete(ACHIEVEMENT_NG_PLUS_FRESH_FACES);

        if (gSaveBlock2Ptr->newGamePlus > 0)
        {
            if (gAchievementProfile.trainersDefeatedAcrossNgPlus < 0xFFFF)
                gAchievementProfile.trainersDefeatedAcrossNgPlus++;
            if (gAchievementProfile.trainersDefeatedAcrossNgPlus >= 300)
                Achievement_TryComplete(ACHIEVEMENT_NG_PLUS_NEVER_THE_SAME_FIGHT);
            sAchievementProfileDirty = TRUE;
        }
    }

    if (isMajorBattle)
    {
        bool8 noBagItemsUsed = (gBattleResults.numHealingItemsUsed == 0 && gBattleResults.numRevivesUsed == 0);
        bool8 noHeldItems = TRUE;

        // Both require a full 6-Pokemon party -- with
        // only one or two Pokemon along, there's barely any HP pool to
        // dip into and barely any held items to check, making both
        // trivial to earn by accident.
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

        // No Freebies bookkeeping: did the starter (tracked by personality,
        // so it survives evolution) act in this major battle? Sticky once
        // set, same "Broken" idiom used elsewhere.
        if (runData->starterPersonality != 0 && !runData->starterActedInMajorBattle)
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

        // Patchwork Team -- no randomizer gate, per the roster's
        // own condition text.
        if (playerCount == PARTY_SIZE && Achievement_AllMetLocationsDistinct(party, playerCount))
            Achievement_TryComplete(ACHIEVEMENT_RANDOMIZER_PATCHWORK_TEAM);

        // Boss Gauntlet bookkeeping -- accumulates all cycle, checked at
        // Achievement_OnNewGamePlusCycleCompleted.
        runDataExt->majorBossClassesDefeatedThisCycle |= Achievement_MajorBossClassBit(GetTrainerClassFromId(TRAINER_BATTLE_PARAM.opponentA));

        // ACHIEVEMENT_RANDOMIZER_NEVER_SEEN_IT_COMING ("beat
        // a randomized major battle with no super-effective move available")
        // removed here -- with move/type randomization scrambling coverage,
        // this just happens by chance over a run's worth of major battles.
        if (Achievement_AnyRandomizerFlagSet())
        {
            if (playerCount == PARTY_SIZE && Achievement_AllPrimaryTypesDistinct(party, playerCount))
                Achievement_TryComplete(ACHIEVEMENT_RANDOMIZER_CHAOS_TEAM);
        }
    }

    if (isGymBattle)
    {
        // ACHIEVEMENT_CHALLENGE_LEVEL_DISCIPLINE ("beat a
        // Gym Leader with no party member above the level cap") removed --
        // a player playing through normally, without deliberately grinding,
        // rarely ends up over the level cap anyway. The level-cap scan that
        // used to back this check is removed along with it.

        // Random by Nature, and Complete Reinvention's cumulative
        // species tracking (checked at Achievement_OnNewGamePlusCycleCompleted).
        // Same ANY-one-flag criteria as Chaos Begins above
        // (Achievement_AnyRandomizerFlagSet) -- see that call site's comment.
        if (Achievement_AnyRandomizerFlagSet())
            Achievement_TryComplete(ACHIEVEMENT_RANDOMIZER_RANDOM_BY_NATURE);

        if (Achievement_RecordGymSpeciesUsed(runDataExt, party, playerCount))
            runDataExt->reinventionBroken = TRUE;
    }
}

// Same call site as above, immediately after it. Every entry here is
// additionally gated on nuzlockeModeEnabled. Self-contained rather than
// reusing Achievement_CheckTeamMilestones's locals -- that function's own
// gym branch belongs to category L, not this one.
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

        // ACHIEVEMENT_NUZLOCKE_NO_ACE_ALLOWED removed --
        // duplicate of ACHIEVEMENT_TEAM_UNDERSTUDY (Achievement_CheckTeamMilestones),
        // the same "highest-level party member didn't act" check on the same
        // gym battle, which isn't gated on nuzlockeModeEnabled so it already
        // fires for Nuzlocke runs too.
        if (sBattleData.lastThreeKoSlots[2] != 0
         && (sBattleData.lastThreeKoSlots[2] - 1) == lowestSlot)
            Achievement_TryComplete(ACHIEVEMENT_NUZLOCKE_SCRAPPY);
    }
}

// Achievement_CheckNuzlockeExplorationMilestones removed --
// it existed solely for ACHIEVEMENT_NUZLOCKE_FULL_ENCOUNTER's route-tracking,
// which was removed too (see include/constants/achievements.h's Nuzlocke
// category comment). Its call site in LoadCurrentMapData (src/overworld.c)
// is removed along with it; runData->nuzlockePendingRoute/nuzlockeRouteSkipped
// (include/global.h) are now unread but left in place.

// GameClear (src/post_battle_event_funcs.c), alongside
// Achievement_CheckTeamCompletionMilestones/Achievement_CheckEconomyCompletionMilestones
// -- same re-runs-every-NG+-cycle gating.
void Achievement_CheckChallengeCompletionMilestones(void)
{
    struct AchievementRunData *runData = &gSaveBlock1Ptr->achievementRunData;
    u8 modifierCount = Achievement_CountChallengeModifiers();

    if (modifierCount >= 3)
        Achievement_TryComplete(ACHIEVEMENT_CHALLENGE_SELF_IMPOSED);
    if (modifierCount >= 5)
        Achievement_TryComplete(ACHIEVEMENT_CHALLENGE_HARD_WAY);
    if (modifierCount >= 7)
    {
        Achievement_TryComplete(ACHIEVEMENT_CHALLENGE_BRUTAL_RULES);
        if (!gAchievementProfile.boostsEnabled)
            Achievement_TryComplete(ACHIEVEMENT_CHALLENGE_NIGHTMARE_MODE);
    }

    if (!runData->boughtConsumableItem)
        Achievement_TryComplete(ACHIEVEMENT_CHALLENGE_NO_SHOPPING_RUN);

    if (GetGameStat(GAME_STAT_USED_POKECENTER) == 0)
        Achievement_TryComplete(ACHIEVEMENT_CHALLENGE_NO_CENTERS);

    if (gSaveBlock2Ptr->optionsBattleStyle == OPTIONS_BATTLE_STYLE_SET
     && gSaveBlock1Ptr->difficulty == DIFFICULTY_HARD)
        Achievement_TryComplete(ACHIEVEMENT_CHALLENGE_HARDCORE_SET);

    // ACHIEVEMENT_CHALLENGE_CAPSTONE removed -- it was this
    // same !levelCapEverExceeded condition alone, a duplicate of
    // ACHIEVEMENT_CHALLENGE_PERFECTLY_CAPPED minus its extra HARD/randomizer
    // requirement.
    if (!runData->levelCapEverExceeded
     && (gSaveBlock1Ptr->difficulty == DIFFICULTY_HARD
      || FlagGet(FLAG_RANDOMIZE_MON) || FlagGet(FLAG_RANDOMIZE_TYPE) || FlagGet(FLAG_RANDOMIZE_MOVES)))
        Achievement_TryComplete(ACHIEVEMENT_CHALLENGE_PERFECTLY_CAPPED);

    if (runData->highestPartySizeThisRun != 0 && runData->highestPartySizeThisRun <= 3)
        Achievement_TryComplete(ACHIEVEMENT_CHALLENGE_THREE_POKEMON);
    if (runData->highestPartySizeThisRun != 0 && runData->highestPartySizeThisRun <= 1)
        Achievement_TryComplete(ACHIEVEMENT_CHALLENGE_SOLO_JOURNEY);

    if (runData->starterPersonality != 0 && !runData->starterActedInMajorBattle)
        Achievement_TryComplete(ACHIEVEMENT_CHALLENGE_NO_FREEBIES);

    if (!gAchievementProfile.boostsEnabled && !FlagGet(FLAG_ALLOW_STAT_EDITOR))
        Achievement_TryComplete(ACHIEVEMENT_CHALLENGE_HARDLY_ANY_HELP);
}

// Same call site as above. Every entry here is gated on nuzlockeModeEnabled.
void Achievement_CheckNuzlockeCompletionMilestones(void)
{
    struct AchievementRunData *runData = &gSaveBlock1Ptr->achievementRunData;
    struct Pokemon *party = gParties[B_TRAINER_PLAYER];
    u8 playerCount = gPartiesCount[B_TRAINER_PLAYER];

    if (!gSaveBlock1Ptr->nuzlockeModeEnabled)
        return;

    if (gSaveBlock1Ptr->difficulty == DIFFICULTY_HARD && !FlagGet(FLAG_LEVEL_CAP_OFF))
        Achievement_TryComplete(ACHIEVEMENT_NUZLOCKE_HARDCORE_SURVIVOR);

    if (runData->nuzlockeMonsLost == 0)
        Achievement_TryComplete(ACHIEVEMENT_NUZLOCKE_PERFECT);
    if (runData->nuzlockeMonsLost >= 5)
        Achievement_TryComplete(ACHIEVEMENT_NUZLOCKE_GRAVEYARD);

    // ACHIEVEMENT_NUZLOCKE_SPECIES_CLAUSE ("no two catches
    // from the same family") and ACHIEVEMENT_NUZLOCKE_NO_REVIVES ("never used
    // a Revive") removed -- a genuine Nuzlocke already only keeps one catch
    // per route and treats a fainted Pokemon as permanently boxed, so both
    // conditions tend to hold on their own. Their sole helpers,
    // Achievement_HasDuplicateEvolutionFamilyAmongOwned and
    // Achievement_RecordReviveUsed, are removed along with them --
    // runData->nuzlockeRevivesUsed is now unread but left in place (see the
    // struct's own comment).

    // ACHIEVEMENT_NUZLOCKE_FULL_ENCOUNTER removed -- see
    // include/constants/achievements.h's Nuzlocke category comment.
    // runData->nuzlockeRouteSkipped is now unread but left in place.

    // ACHIEVEMENT_NUZLOCKE_UNASSISTED_SURVIVOR removed --
    // too similar to ACHIEVEMENT_CHALLENGE_HARDLY_ANY_HELP
    // (Achievement_CheckChallengeCompletionMilestones, same GameClear call
    // site): its !boostsEnabled condition is a strict subset of that
    // achievement's, and it isn't gated on nuzlockeModeEnabled either, so it
    // already fires for completed Nuzlocke runs too.

    // Nuzlocke Across Worlds/Chaos Survivor.
    if (Achievement_AnyRandomizerFlagSet())
    {
        Achievement_TryComplete(ACHIEVEMENT_NUZLOCKE_ACROSS_WORLDS);
        if (gSaveBlock1Ptr->difficulty == DIFFICULTY_HARD)
            Achievement_TryComplete(ACHIEVEMENT_NUZLOCKE_CHAOS_SURVIVOR);
    }
}

// BuyMenuSubtractMoney (src/shop.c), alongside Achievement_RecordMoneySpent
// -- called only when the purchased item is POCKET_ITEMS (the same
// "consumable" definition Resourceful uses).
void Achievement_RecordConsumableItemPurchase(void)
{
    gSaveBlock1Ptr->achievementRunData.boughtConsumableItem = TRUE;
}

// Achievement_RecordReviveUsed (formerly called from
// BS_ItemRestoreHP in src/battle_script_commands.c and PokemonUseItemEffects
// in src/pokemon.c) removed along with its sole achievement,
// ACHIEVEMENT_NUZLOCKE_NO_REVIVES -- see
// Achievement_CheckNuzlockeCompletionMilestones. nuzlockeRevivesUsed is now
// unread but left in AchievementRunData (see the struct's own comment).

// RemoveFaintedMonsFromParty (src/overworld.c) -- the single function every
// Nuzlocke fainted-mon removal funnels through, called once per Pokemon
// actually removed.
void Achievement_RecordNuzlockeMonLost(void)
{
    struct AchievementRunData *runData = &gSaveBlock1Ptr->achievementRunData;

    if (runData->nuzlockeMonsLost < 255)
        runData->nuzlockeMonsLost++;
}

// ui_birch_case.c, right after the starter is granted -- personality
// survives evolution, unlike species, which is why No Freebies tracks it
// instead. 0 is treated as "not yet recorded" (like Fresh Start's ring
// buffer) rather than adding a separate bool -- a real starter
// rolling personality 0 is a 1-in-4-billion coincidence, the same order of
// risk already accepted elsewhere in this file.
void Achievement_RecordStarterPersonality(u32 personality)
{
    gSaveBlock1Ptr->achievementRunData.starterPersonality = personality;
}

// GiveCapturedMonToPlayer (src/pokemon.c), alongside
// Achievement_CheckCaptureMilestones -- one more call at that same funnel.
// GAME_STAT_POKEMON_CAPTURES is this-run count (ResetGameStats zeroes every
// game stat at the start of every new game and every NG+ cycle,
// src/overworld.c), already incremented by the time this runs. See the top
// of this file for Achievement_AnyRandomizerFlagSet and its neighboring
// helpers.
void Achievement_CheckRandomizerCaptureMilestone(void)
{
    if (Achievement_AnyRandomizerFlagSet() && GetGameStat(GAME_STAT_POKEMON_CAPTURES) >= 25)
        Achievement_TryComplete(ACHIEVEMENT_RANDOMIZER_ROOKIE);
}

// ---- Streaks, Records & Collection Remainder (category P) --------------
//
// The one entry with genuinely new persistent state is the trainer win
// streak (AchievementRunDataExt, SaveBlock2 -- see that struct's own comment
// for why SaveBlock1 isn't an option). Every other entry here reads live
// party/box/game-stat state, the same "cheap to evaluate" shape category M
// established -- Marathon Trainer/Long Haul/Prolific/Battle
// Machine/Egg Marathon/Nurse's Nightmare need no tracking of their own at
// all, just an existing GAME_STAT_* value.

static u16 Achievement_SaturatingAddU16(u16 value, u8 amount)
{
    u32 sum = (u32)value + amount;
    return (sum > 0xFFFF) ? 0xFFFF : (u16)sum;
}

// Short-circuits the moment `stopAt` distinct species have been seen.
// `seen`'s size only needs to cover the largest `stopAt` any caller passes.
//
// Note which direction that short-circuit actually helps in: it only fires
// once the player already owns `stopAt` distinct species. Below the
// threshold there is nothing to stop early on, so this walks all
// TOTAL_BOXES_COUNT * IN_BOX_COUNT slots, decrypting every one -- and that is
// exactly the state the achievement is still open in. Treating it as "cheap
// because the threshold is low" is what made this affordable-looking enough
// to sit on the map-transition path; call it from an event that can actually
// change the count instead (see Achievement_RecordMonObtained).
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

// Walks an evolution family outward from `root` (assumed already the
// family's root -- Achievement_GetEvolutionRoot) via
// GetSpeciesEvolutions, BFS with de-duplication so a branching family
// (Eevee) is only ever recorded once per target. Capped well above the
// largest real family, so the cap is never actually hit.
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

// HandleSetPokedexFlag (src/pokemon.c)'s FLAG_SET_CAUGHT branch, alongside
// Achievement_CheckPokedexMilestones -- species is the species that was just
// newly caught. "Register" is read as "caught" (the more demanding of the
// two Pokedex flags), matching this entry's Gold value.
void Achievement_CheckFamilyMilestone(enum Species species)
{
    enum Species members[ACHIEVEMENT_MAX_FAMILY_MEMBERS];
    enum Species root = Achievement_GetEvolutionRoot(species);
    u8 count = Achievement_GetFamilyMembers(root, members);
    u8 i;

    for (i = 0; i < count; i++)
    {
        enum NationalDexOrder dexNum = SpeciesToNationalPokedexNum(members[i]);

        if (dexNum == NATIONAL_DEX_NONE || !GetSetPokedexFlag(dexNum, FLAG_GET_CAUGHT))
            return;
    }

    Achievement_TryComplete(ACHIEVEMENT_COLLECT_FAMILY_REUNION);
}

// Achievement_CheckPerfectIvMilestone (formerly called from
// GiveCapturedMonToPlayer in src/pokemon.c and Task_EggHatch in
// src/egg_hatch.c) removed along with its sole achievement,
// ACHIEVEMENT_COLLECT_PERFECT_SPECIMEN -- see src/data/achievements.h.

// RemoveFaintedMonsFromParty (src/overworld.c) and FldEff_PokecenterHeal
// (src/field_effect.c), both called from inside their existing
// IsPartyEmpty() branch -- see Achievement_CheckNuzlockeMilestones's own
// comment on those two functions for why no third detector is added here
// either. Mirrors this run's streak
// high-water mark into the persistent profile before zeroing the counters
// this wipe just broke.
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

// SetValuesOnFaint (src/battle_util.c)'s player-faint branch, gated by the
// caller the same way as every other battle-data write (never link/
// recorded). The fainted mon's HP is already 0 by the time this runs, so
// "exactly one conscious mon left" here really does mean the player is down
// to their last one.
void Achievement_RecordPlayerFaint(void)
{
    struct Pokemon *party = gParties[B_TRAINER_PLAYER];
    u8 playerCount = gPartiesCount[B_TRAINER_PLAYER];

    if (CountConsciousPartyMons(party, playerCount) == 1)
        sBattleData.wasDownToLastMon = TRUE;
}

// HandleEndTurn_BattleWon (src/battle_main.c), immediately after
// Achievement_CheckNuzlockeMilestones, gated the same way (never link/
// recorded).
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

        // League Streak -- Elite Four/Champion wins specifically, the
        // subset of Achievement_IsMajorBattle() the roster's "full League
        // sequence" means.
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
            if (GetSpeciesBaseStatTotal(GetMonData(&party[i], MON_DATA_SPECIES)) < 350)
            {
                Achievement_TryComplete(ACHIEVEMENT_COLLECT_ODDBALL);
                break;
            }
        }
    }

    // Veteran Team/Old Reliable: sBattleData.kosPerSlot holds
    // this battle's KOs only -- fold them into the cumulative per-slot
    // totals here, once, right before that struct is cleared for the next
    // battle (Achievement_ClearBattleData, BattleStartClearSetData).
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
        // Legend of the Run bookkeeping -- checked at GameClear
        // (Achievement_CheckRecordsCompletionMilestones), since it only
        // means anything for a completed run.
        //
        // Fixed to track actual Pokemon (by personality,
        // survives evolution), not party slots -- the old presentSlots
        // bitmask (see its own field comment in include/global.h) tracked
        // occupied slots instead, and slot 0 is never empty while you're
        // able to battle at all, so it trivially always fired. This keeps
        // legendCandidatePersonalities as the set of Pokemon that have
        // appeared in every major battle so far, shrinking it to the
        // intersection with the current party each time.
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

        // Underestimated: the party slot credited with the very last
        // opposing faint of a won battle is exactly the one that ended it.
        // Requires a full 6-Pokemon opposing team -- trivial to land the
        // finishing blow with a weak Pokemon against a one- or two-mon boss.
        if (sBattleData.lastThreeKoSlots[2] != 0
         && gPartiesCount[B_TRAINER_OPPONENT_A] == PARTY_SIZE)
        {
            u8 finalKoSlot = sBattleData.lastThreeKoSlots[2] - 1;

            if (finalKoSlot < playerCount
             && GetSpeciesBaseStatTotal(GetMonData(&party[finalKoSlot], MON_DATA_SPECIES)) < 400)
                Achievement_TryComplete(ACHIEVEMENT_COLLECT_UNDERESTIMATED);
        }
    }

    // Comeback Count.
    if (sBattleData.wasDownToLastMon)
    {
        if (runDataExt->comebackWinsThisRun < 0xFF)
            runDataExt->comebackWinsThisRun++;
        if (runDataExt->comebackWinsThisRun >= 10)
            Achievement_TryComplete(ACHIEVEMENT_RECORD_COMEBACK_COUNT);
    }
}

// LoadCurrentMapData (src/overworld.c), alongside
// Achievement_CheckExplorationMilestones. Every entry here reads
// live party/box/game-stat state rather than a specific event -- map
// transitions are frequent enough during normal play to catch a threshold
// shortly after it's crossed, the same reasoning that function's own
// map-transition checks use.
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

    // ACHIEVEMENT_COLLECT_BOX_FILLER ("store 100 Pokemon at
    // once") and ACHIEVEMENT_COLLECT_STORAGE_BARON ("store 300 at once")
    // removed -- a full playthrough's worth of catching fills PC boxes up on
    // its own. The PC-box-scanning loop that used to back them (storedCount)
    // is removed along with them.

    // One of Each's check used to live right here. It moved to
    // Achievement_RecordMonObtained: unlike everything else in this function
    // it walks storage rather than just the party, and
    // Achievement_CountDistinctOwnedSpecies only short-circuits once it has
    // found its 10 distinct species -- so below the threshold (exactly when
    // the achievement is still open) it walked all
    // TOTAL_BOXES_COUNT * IN_BOX_COUNT slots, decrypting each one, on every
    // single map transition. That was ~15-50ms of hitch per map load at
    // TOTAL_BOXES_COUNT 28, and it grew with the box count.

    // Marathon Trainer/Long Haul, Prolific/Battle Machine -- existing
    // GAME_STAT_* values, no tracking of their own needed.
    if (GetGameStat(GAME_STAT_STEPS) >= 50000)
        Achievement_TryComplete(ACHIEVEMENT_RECORD_MARATHON_TRAINER);
    if (GetGameStat(GAME_STAT_STEPS) >= 200000)
        Achievement_TryComplete(ACHIEVEMENT_RECORD_LONG_HAUL);
    if (GetGameStat(GAME_STAT_TOTAL_BATTLES) >= 1000)
        Achievement_TryComplete(ACHIEVEMENT_RECORD_PROLIFIC);
    if (GetGameStat(GAME_STAT_TOTAL_BATTLES) >= 2500)
        Achievement_TryComplete(ACHIEVEMENT_RECORD_BATTLE_MACHINE);
}

// GameClear (src/post_battle_event_funcs.c), alongside
// Achievement_CheckNuzlockeCompletionMilestones -- Legend of the Run only
// means anything for a completed run.
void Achievement_CheckRecordsCompletionMilestones(void)
{
    struct AchievementRunDataExt *runDataExt = &gSaveBlock2Ptr->achievementRunDataExt;

    // Reads the fixed legendCandidateCount -- see the
    // bookkeeping's own comment in Achievement_CheckBattleRecordsMilestones.
    if (runDataExt->anyMajorBattleThisRun && runDataExt->legendCandidateCount != 0)
        Achievement_TryComplete(ACHIEVEMENT_RECORD_LEGEND_OF_THE_RUN);

    // Mirror the streak high-water mark here too, not only on a party wipe
    // (Achievement_RecordPartyWipe) -- a save that finishes a run without
    // ever wiping would otherwise never get its best streak recorded in the
    // persistent profile at all.
    if (runDataExt->currentTrainerWinStreak > runDataExt->bestTrainerWinStreakThisRun)
        runDataExt->bestTrainerWinStreakThisRun = runDataExt->currentTrainerWinStreak;
    if (runDataExt->bestTrainerWinStreakThisRun > gAchievementProfile.bestTrainerWinStreakEver)
    {
        gAchievementProfile.bestTrainerWinStreakEver = runDataExt->bestTrainerWinStreakThisRun;
        sAchievementProfileDirty = TRUE;
    }
}

// Task_LearnedMove (src/party_menu.c), gated by the caller on move[1] == 0
// (the TM/HM item-use path specifically -- see that function's own comment)
// and on the item actually being a TM rather than an HM.
void Achievement_RecordTMTaught(void)
{
    struct AchievementRunDataExt *runDataExt = &gSaveBlock2Ptr->achievementRunDataExt;

    if (runDataExt->tmsTaughtThisRun < 0xFF)
        runDataExt->tmsTaughtThisRun++;
    if (runDataExt->tmsTaughtThisRun >= 25)
        Achievement_TryComplete(ACHIEVEMENT_RECORD_MOVE_TUTOR);
}

// FldEff_PokecenterHeal (src/field_effect.c), right after the vanilla
// IncrementGameStat(GAME_STAT_USED_POKECENTER) call.
void Achievement_CheckPokecenterMilestone(void)
{
    if (GetGameStat(GAME_STAT_USED_POKECENTER) >= 200)
        Achievement_TryComplete(ACHIEVEMENT_RECORD_NURSES_NIGHTMARE);
}

// ---- Profile Meta, Mastery & Prestige (category Q) ----------------------
// See include/constants/achievements.h's category Q comment for
// the roster-to-condition breakdown. This category was cut down from 30
// entries to 10 -- Achievement_CountInCategory, Achievement_AnyCategoryFullyCompletedAtTier,
// Achievement_CheckCategoryPercentMilestone, Achievement_GoldOrBetterFullyCompletedAcrossCategories
// (and its sReplayArchitectCategories list), and Achievement_CountNonHiddenExcluding
// existed solely for entries removed along the way and are gone; see
// src/data/achievements.h's own comments on each removed entry for the full
// list and rationale.

// Achievement_CountCompletedInCategory and its sole caller,
// Achievement_HasBronzeInEveryCategory (Achievement Hunter), are both removed
// here -- Achievement Hunter was unattainable
// (CHALLENGE, NG+, NUZLOCKE and PROFILE have zero Bronze-tier entries between
// them), which is resolved by removing the achievement rather than
// forcing a Bronze tier onto categories that were never designed to have an
// "easy" entry.

// Diamond Standard (backfill): every Diamond-tier achievement, excluding
// itself -- it is itself Diamond-tier, and without the exclusion the
// condition could never become true (see the self-reference note in
// constants/achievements.h's category Q comment).
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

// Full Investment: needs to look across every real boost (BOOST_NONE
// excluded, hence starting at 1).
static u32 AchievementBoost_TotalPurchasedLevels(void)
{
    u32 total = 0;
    u16 boostId;

    for (boostId = BOOST_NONE + 1; boostId < BOOSTS_COUNT; boostId++)
        total += AchievementBoost_GetLevel(boostId);

    return total;
}

// Called from the tail of Achievement_TryComplete, alongside the existing
// Achievement_CheckPointMilestones -- every entry here is a meta-achievement
// over the finished catalog/profile state, so recomputing it after every
// single completion is the simplest correct implementation. The recursion
// through Achievement_TryComplete is bounded the same way
// Achievement_CheckPointMilestones already documents: each nested call
// either no-ops (already completed, per Achievement_IsCompleted's guard) or
// completes exactly one new achievement and recurses one level deeper, and
// there are only ACHIEVEMENTS_COUNT of those to ever exhaust.
static void Achievement_CheckMasteryMilestones(void)
{
    if (Achievement_HasCompletedEveryTier())
        Achievement_TryComplete(ACHIEVEMENT_PROFILE_WELL_ROUNDED);

    // Thresholds rescaled for the catalog's 20,000-point total
    // -- see each achievement's own comment on the rescale in
    // src/data/achievements.h.
    if (gAchievementProfile.totalPointsEarned >= 10000)
        Achievement_TryComplete(ACHIEVEMENT_PROFILE_POINT_HOARDER);
    if (gAchievementProfile.totalPointsEarned >= 18000)
        Achievement_TryComplete(ACHIEVEMENT_PROFILE_POINT_LEGEND);
    if (gAchievementProfile.pointsFromGoldOrBetter >= 7000)
        Achievement_TryComplete(ACHIEVEMENT_PROFILE_NO_EASY_PATH);

    if (Achievement_AllDiamondCompleted(ACHIEVEMENT_MASTERY_DIAMOND_STANDARD))
        Achievement_TryComplete(ACHIEVEMENT_MASTERY_DIAMOND_STANDARD);
}

// Called from AchievementBoost_Purchase/_Reset -- boostLevels[]/
// pointsInvested/boostResets only ever change in those two functions, never
// from Achievement_TryComplete's tail, so these boost-state entries need
// their own call site instead of Achievement_CheckMasteryMilestones.
static void Achievement_CheckBoostMilestones(void)
{
    // Scaled down from 2000 to 1000 -- the boost economy this measures
    // against shrank from 42,500 to 20,000 total (src/data/achievement_boosts.h's
    // own comment on the rescale).
    if (gAchievementProfile.pointsInvested >= 1000)
        Achievement_TryComplete(ACHIEVEMENT_PROFILE_BOOST_INVESTOR);

    if (AchievementBoost_TotalPurchasedLevels() >= 40)
        Achievement_TryComplete(ACHIEVEMENT_PROFILE_FULL_INVESTMENT);

    // Reconfigured: at least one reset, and points invested again since
    // (right after a reset pointsInvested is always 0, so this is only ever
    // true once a purchase follows a reset).
    if (gAchievementProfile.boostResets >= 1 && gAchievementProfile.pointsInvested > 0)
        Achievement_TryComplete(ACHIEVEMENT_PROFILE_RECONFIGURED);

    // Selective Mastery: exactly one boost at its max level, and at least
    // five other boosts still untouched (level 0).
    {
        u16 boostId;
        u8 maxedCount = 0;
        u8 zeroCount = 0;

        for (boostId = BOOST_NONE + 1; boostId < BOOSTS_COUNT; boostId++)
        {
            u8 level = AchievementBoost_GetLevel(boostId);

            if (level >= AchievementBoost_GetInfo(boostId)->maxLevel)
                maxedCount++;
            else if (level == 0)
                zeroCount++;
        }

        if (maxedCount == 1 && zeroCount >= 5)
            Achievement_TryComplete(ACHIEVEMENT_PROFILE_SELECTIVE_MASTERY);
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
