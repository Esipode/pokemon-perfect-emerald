#ifndef GUARD_ACHIEVEMENTS_H
#define GUARD_ACHIEVEMENTS_H

#include "global.h"
#include "constants/achievements.h"
#include "constants/items.h" // enum Item, for Achievement_CheckItemMilestones (category G)
#include "constants/moves.h" // enum Move, for Achievement_RecordMoveUsed (category K)
#include "constants/battle.h" // enum BattlerId, for Achievement_RecordOpposingFaint (category K)

#define ACHIEVEMENT_PROFILE_MAGIC   0x50454143  // 'PEAC'
#define ACHIEVEMENT_PROFILE_VERSION 1
#define MAX_ACHIEVEMENTS            512   // reserved ceiling -> 64 bytes of flags
#define MAX_BOOSTS                  32

// Single tunable fee, in Poké money, to reset every boostLevels[] entry and
// refund pointsInvested in full. Deliberately not an escalating fee, and
// deliberately conservative until playtesting suggests a better value.
#define ACHIEVEMENT_BOOST_RESET_FEE 5000

// Cap for .name in gAchievements[] (src/data/achievements.h), enforced at
// compile time via ACHIEVEMENT_NAME() there -- same pattern as ITEM_NAME_LENGTH.
#define ACHIEVEMENT_NAME_LENGTH     24

// Cap for .name in gAchievementBoosts[] (src/data/achievement_boosts.h),
// enforced at compile time via BOOST_NAME() there.
#define BOOST_NAME_LENGTH           24

// Bits for struct AchievementBattleData.statusesInflicted (src/achievements.c)
// -- SLP/PSN/BRN/PRZ/FRZ, matching enum MoveEffect's non-volatile
// statuses one-for-one (Toxic folds into POISON, Frostbite into FREEZE).
// Computed at each hook's call site (battle_script_commands.c) rather than
// passing an enum MoveEffect in, so this header doesn't need move.h's full
// effect table just to declare Achievement_RecordStatusInflicted below.
#define ACHIEVEMENT_STATUS_BIT_SLEEP      (1 << 0)
#define ACHIEVEMENT_STATUS_BIT_POISON     (1 << 1)
#define ACHIEVEMENT_STATUS_BIT_BURN       (1 << 2)
#define ACHIEVEMENT_STATUS_BIT_PARALYSIS  (1 << 3)
#define ACHIEVEMENT_STATUS_BIT_FREEZE     (1 << 4)

// Definition data -- one const entry per enum AchievementId, in
// src/data/achievements.h. Kept strictly separate
// from struct AchievementProfile below, which is completion *state* only.
struct Achievement
{
    const u8 *name;
    const u8 *description;
    enum AchievementTier tier;
    enum AchievementScope scope;
    enum AchievementCategory category; // mirrors the catalog's own section headers
    u16 points;
    bool8 hidden;
};

// Boost definition data -- one const entry per enum BoostId, in
// src/data/achievement_boosts.h. Kept strictly separate
// from struct AchievementProfile below, which is completion *state* only
// (mirrors the Achievement/AchievementProfile split above): "a boost's
// definition should describe its behavior; the persistent profile should
// only store the player's state for that boost."
struct AchievementBoost
{
    const u8 *name;
    const u8 *description;
    u8 type;                  // BOOST_TYPE_LEVELED | BOOST_TYPE_BINARY
    u8 maxLevel;               // 1 for binary
    const u16 *costs;          // costs[level] -- cost to go from level to level+1
    const u16 *effects;        // effects[level]; units are per-boost, not rendered generically
};

struct AchievementProfile
{
    u32 magic;
    u16 version;
    u16 checksum;                 // over every byte after this field

    u32 totalPointsEarned;
    u32 pointsInvested;           // available = totalPointsEarned - pointsInvested

    u8  achievementFlags[MAX_ACHIEVEMENTS / 8];
    u8  boostLevels[MAX_BOOSTS];

    bool8 boostsUnlocked;
    bool8 boostsEnabled;          // defaults TRUE

    // Fresh-save Hall of Fame clears only (newGamePlus == 0 at the time of
    // the clear) -- New Game+ cycle clears are deliberately excluded and
    // counted separately by ngPlusCyclesCompleted below. See
    // Achievement_OnFirstPlaythroughComplete. ACHIEVEMENT_PLAYTHROUGHS_2/_5
    // and Frequent Flyer/Veteran Trainer/Resident Champion, the achievements
    // this counter used to back, are all removed -- no achievement reads it
    // anymore, but it's still shown on the debug profile dump (src/debug.c)
    // independent of any achievement, so the counter stays live.
    u16 playthroughsCompleted;
    u16 ngPlusCyclesCompleted;
    u8  highestNgPlusCycle;
    u16 nuzlockesCompleted;
    u16 randomizedRunsCompleted;
    u16 shiniesObtained;
    u16 boostResets;

    // Boost menu polish: how many purchased levels below boostLevels[i] that
    // boost's *active* effect is currently dialed back to (0 = fully active
    // at the purchased level). Deliberately placed here, at the front of what
    // used to be reserved[64] rather than inserted earlier in the struct: it
    // lands on bytes that were always zero in a profile saved before this
    // field existed, so every field before it keeps its old offset and old
    // saves aren't misread. (An earlier version of this field sat between
    // boostLevels and boostsUnlocked, which shifted every later field's
    // offset and made boostsUnlocked read as FALSE on old saves despite the
    // player having actually unlocked boosts -- don't repeat that mistake.)
    // Binary boosts (maxLevel 1) only ever use 0 or 1, i.e. on/off. See
    // AchievementBoost_GetActiveLevel/_TryChangeActiveLevel below.
    u8  boostLevelReduction[MAX_BOOSTS];

    // Randomizer & New Game+ fields. Same "front of reserved[]" precedent as
    // boostLevelReduction above -- these bytes are guaranteed zero in any
    // profile saved before these fields existed, so every existing field
    // keeps its offset. A "randomized runs completed per flag combination"
    // counter was considered and dropped, because every roster entry that
    // reads a specific flag combination (Truly Random, Pure Chaos,
    // Species/Type/Move Chaos) is a one-shot completion check where
    // Achievement_TryComplete's own idempotency guard is already sufficient
    // -- no counter needed. ngPlusConfigsSeen ended up needing that same
    // shape of tracking anyway, just for Cycle Collector's broader
    // "challenge configuration" (all 7 Achievement_CountChallengeModifiers
    // modifiers, not only the randomizer ones).
    u16 trainersDefeatedAcrossNgPlus;    // for Never the Same Fight (NGP-008)
    // No longer read by anything -- backed ACHIEVEMENT_NG_PLUS_ESCALATION
    // (NGP-010), removed along with the rest of the old NG+ repeat-count
    // ladder. Left in place, unused, rather than removed outright, to avoid
    // reshuffling every later field's offset.
    u8  consecutiveNgPlusCyclesCompleted;
    u8  ngPlusConfigsSeen[4];            // unused -- backed Cycle Collector (ACHIEVEMENT_NG_PLUS_CYCLE_COLLECTOR), now removed; left in place rather than reflowing this struct's fields
    u8  ngPlusConfigsSeenCount;          // unused, same removal as above
    bool8 completedConventionalRun;      // neither Nuzlocke nor randomized -- for Full Circle (VAR-007)

    // Same "front of reserved[]" precedent as every field above. No roster
    // entry reads it directly -- every REC-xxx streak threshold is checked
    // against AchievementRunDataExt.currentTrainerWinStreak, which is
    // run-scoped and already sufficient -- but a persistent high-water mark
    // alongside the run-scoped one lets a player's best streak survive a
    // new game, and gives the profile Mastery/Prestige category (defined
    // over the finished catalog) a hook it can read later. Mirrored here
    // from AchievementRunDataExt.bestTrainerWinStreakThisRun on every party
    // wipe (Achievement_RecordPartyWipe, src/achievements.c).
    u16 bestTrainerWinStreakEver;

    // Same "front of reserved[]" precedent as every field above.
    u32 pointsFromGoldOrBetter;   // sum of .points over every Gold-or-better achievement completed, for No Easy Path (PRO-012)

    // unused -- backed Replay Master (ACHIEVEMENT_VARIETY_REPLAY_MASTER),
    // removed along with the Achievement_ChallengeConfigSignature helper
    // that fed it; left in place rather than reflowing this struct's
    // fields. Used to track distinct challenge-configuration signatures seen
    // across EVERY completed playthrough, deliberately its own array rather
    // than a reuse of ngPlusConfigsSeen[]/_Count above (that field is
    // NG+-cycle-only and backed a different, narrower achievement, Cycle
    // Collector, also removed).
    u8  playthroughConfigsSeen[5];       // unused, see above
    u8  playthroughConfigsSeenCount;     // unused, same removal as above

    // Lifetime counters for achievements that used to read
    // GetGameStat() directly and were scoped ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH
    // as a result -- GAME_STAT_* values live in SaveBlock1, which ClearSav1
    // zeroes at every new game, so progress toward these thresholds used to
    // vanish on a fresh save. Same "front of reserved[]" precedent as every
    // field above, except this batch doesn't need to fit inside the old
    // reserved[11] tail to stay compatible with old saves: WriteAchievementProfile
    // zero-fills the whole sector before writing, so any profile saved before
    // this field existed reads back as 0 here no matter where in the struct
    // it lands, not only within the old reserved run. The game stats
    // themselves are left alone (still incremented at their existing call
    // sites) in case anything else reads them -- these are the
    // achievement-only, never-resets copies. See src/achievements.c for the
    // per-field increment sites.
    u16 trainerBattlesLifetime;   // Trainer Trouncer .. Unstoppable Force
    u16 wildBattlesLifetime;      // Into the Wild .. One with the Wild
    u16 eggsHatchedLifetime;      // It's Hatching! .. Egg Factory, and Egg Marathon
    u16 hiddenItemsFoundLifetime; // Treasure Hunter / Treasure Hoard
    u16 npcsTalkedToLifetime;     // Talk to the Locals / People Person
    u16 shopPurchasesLifetime;    // First Purchase / Regular Customer
    u32 moneySpentLifetime;       // Big Spender / Whale

    u8  reserved[8];              // forward compatibility (was 11)
};

extern struct AchievementProfile gAchievementProfile;

// A profile read/write failure must never be able to trigger the save-failed
// screen (that's gDamagedSaveSectors/gSaveFileStatus's job, not this one).
bool8 Achievement_ProfileWriteFailed(void);

// Writes the profile to flash only if it's been marked dirty since the last
// flush (see src/achievements.c). Safe to call unconditionally from any of
// the flush points.
void Achievement_FlushProfile(void);

// Public API. Nothing outside src/achievements.c writes the profile.

// Loads the profile from flash. Call once at boot, before any save is loaded.
void  Achievement_Init(void);
bool8 Achievement_IsCompleted(u16 achievementId);

// gAchievements[] (src/data/achievements.h) is only ever included from
// src/achievements.c -- this is how the rest of the game (e.g. the
// Achievements Menu) reads definition data (name/description/tier/
// points) for a given ID. Returns the ACHIEVEMENT_NONE entry for an
// out-of-range ID rather than NULL, so callers never need a null check.
const struct Achievement *Achievement_GetInfo(u16 achievementId);

u32   Achievement_GetTotalPoints(void);
u32   Achievement_GetAvailablePoints(void);

bool8 Achievement_BoostsUnlocked(void);
bool8 Achievement_BoostsEnabled(void);
void  Achievement_SetBoostsEnabled(bool8 enabled);

u8    AchievementBoost_GetLevel(u16 boostId);

// The level a boost's effect is currently dialed to, distinct from
// AchievementBoost_GetLevel's *purchased* level -- always between 0 and the
// purchased level, inclusive. Every AchievementBoost_Apply*/Get*Percent/Has*
// effect function reads this, not the purchased level, so dialing a boost
// back (or a binary boost off) takes effect immediately without touching
// what was actually bought. Purchasing a new level leaves this unchanged
// relative to the top of the purchased range (see
// AchievementBoost_TryChangeActiveLevel), so it comes in already active.
u8    AchievementBoost_GetActiveLevel(u16 boostId);

// Boost menu polish: moves the active level by delta, clamped to
// [0, purchased level] -- refuses (returns FALSE, no state change) if delta
// would go outside that range, including on a boost with nothing purchased
// yet. The boost menu's dpad L/R calls this with delta = -1/+1 to dial a
// leveled boost's active level; its [A] toggle on an already-owned binary
// boost (maxLevel 1) calls it with delta = -1 or +1 to flip between the only
// two levels that exist, 0 (off) and 1 (on).
bool8 AchievementBoost_TryChangeActiveLevel(u16 boostId, s8 delta);

// gAchievementBoosts[] (src/data/achievement_boosts.h) is only ever included
// from src/achievements.c -- mirrors Achievement_GetInfo above. Returns the
// BOOST_NONE entry for an out-of-range ID rather than NULL.
const struct AchievementBoost *AchievementBoost_GetInfo(u16 boostId);

// Checks run eligibility and prior completion in order, then commits the
// flag and points together
// before queuing a notification. Returns FALSE without side effects if the
// run is ineligible, the ID is out of range, or it's already completed.
bool8 Achievement_TryComplete(u16 achievementId);

// The one-time first-playthrough unlock. Call this from GameClear()
// (src/post_battle_event_funcs.c) in the branch that only runs the first
// time FLAG_SYS_GAME_CLEAR is set for this save -- that flag's own
// set-once semantics are what guarantee this runs exactly once per
// playthrough, so this function has no completion guard of its own.
//
// Also handles: Chaos Begins/Truly Random/Pure Chaos/Species-Type-Move
// Chaos (all read FlagGet(FLAG_RANDOMIZE_*)/gSaveBlock1Ptr->difficulty at
// this exact completion moment); Full Circle's completedConventionalRun
// bookkeeping; and, gated on gSaveBlock2Ptr->newGamePlus == 0 specifically
// (i.e. this completion was NOT an NG+ cycle), seeding
// previousCyclePartySpecies for No Nostalgia -- see
// Achievement_OnNewGamePlusCycleCompleted's comment for why that lives on
// the opposite side of that gate.
//
// Used to also check Seed Explorer/Randomizer Veteran (removed --
// ACHIEVEMENT_RANDOMIZED_1 is already the "do it once" version) and reset
// consecutiveNgPlusCyclesCompleted for Escalation (removed along with the
// rest of the NG+ repeat-count ladder).
void Achievement_OnFirstPlaythroughComplete(void);

// Called from NewGameInitData (src/new_game.c) right after
// gSaveBlock2Ptr->newGamePlus++, so `cycle` is always the just-started
// cycle's number (1 for the first NG+ loop, 2 for the second, ...).
// highestNgPlusCycle is a high-water mark, not a live counter --
// gSaveBlock2Ptr->newGamePlus already is that -- so this only ever grows it.
// Used to also check ACHIEVEMENT_NG_PLUS_STARTED/_CYCLE_3/_CYCLE_5 here (the
// "start/reach" half of the old NG+ ladder); removed -- see
// Achievement_OnNewGamePlusCycleCompleted's ACHIEVEMENT_NG_PLUS_CYCLE_COMPLETE.
//
// Also checks Chaos Begins (checked here too, since this is the one real
// "a run/cycle just began" event this system has -- TryComplete's own
// guard makes the duplicate check with Achievement_OnFirstPlaythroughComplete's
// cycle-0 case harmless). Also zeroes every per-cycle-scoped field in
// AchievementRunDataExt (trainersDefeatedThisCycle, gymSpeciesUsedThisCycle/
// Count, reinventionBroken, majorBossClassesDefeatedThisCycle) --
// SaveBlock1 has very little slack left (see AchievementRunDataExt's own
// comment), so these live in SaveBlock2 instead, which ClearSav1 can't
// reset for us; this call is their substitute reset point. Deliberately NOT
// zeroed: previousCyclePartySpecies, which is supposed to survive the cycle
// boundary (see Achievement_OnNewGamePlusCycleCompleted).
void Achievement_OnNewGamePlusStarted(u8 cycle);

// Called from the same GameClear() branch as
// Achievement_OnFirstPlaythroughComplete above, additionally gated there on
// gSaveBlock2Ptr->newGamePlus > 0 -- FLAG_SYS_GAME_CLEAR isn't preserved
// across New Game+ (see NewGameInitData), so that branch already re-runs on
// every NG+ cycle's clear, not just the save's very first playthrough. This
// counts specifically the subset of those clears that happened during an NG+
// loop, distinct from the plain playthroughsCompleted total.
//
// Checks ACHIEVEMENT_NG_PLUS_CYCLE_COMPLETE unconditionally here, same "no
// threshold guard needed, this function only runs when it just happened"
// reasoning ACHIEVEMENT_NG_PLUS_STARTED used to rely on -- this single
// achievement replaces the old seven-entry NG+ ladder (see
// src/data/achievements.h for the full list). Escalation
// (consecutiveNgPlusCyclesCompleted) was part of that ladder and is gone too.
//
// Every "complete an NG+ cycle with X" entry lives here, since this
// function only ever runs when that's exactly what just happened --
// Unassisted Cycle (ngPlusCyclesCompleted), Cycle Specialist
// (Achievement_CountChallengeModifiers), Cycle Nuzlocke (nuzlockeModeEnabled),
// Complete Reinvention/Boss Gauntlet (AchievementRunDataExt's per-cycle
// fields), and No Nostalgia (compares the current final party against
// AchievementRunDataExt.previousCyclePartySpecies -- the PRIOR cycle's
// snapshot, seeded either by this same function last cycle or, for cycle 1,
// by Achievement_OnFirstPlaythroughComplete's cycle-0 case -- then
// overwrites it with the current party for the next comparison). Ten Cycles
// Deep, Cycle Collector, Endless Survivor, and New Team New Me's NG+-cycle
// half, all once checked here too, are removed -- see
// src/data/achievements.h for each one's own comment.
void Achievement_OnNewGamePlusCycleCompleted(void);

// Validates in order -- boosts unlocked, current
// level < maxLevel, availablePoints >= costs[level] -- refusing at the first
// failure. Purchase re-runs the same checks (so it can never be called
// directly around a stale CanPurchase result) before committing
// pointsInvested += cost, boostLevels[id]++, and flushing immediately.
// Deliberately NOT gated on gSaveBlock1Ptr->achievementsBlocked -- that
// flag disqualifies a run from *earning* new achievements/points (enforced
// in Achievement_TryComplete), not from spending points already on the
// books.
bool8 AchievementBoost_CanPurchase(u16 boostId);
bool8 AchievementBoost_Purchase(u16 boostId);

// A non-mutating query so the boost shop can gate entry into its
// confirmation prompt without spending the fee just to find out the
// answer -- mirrors AchievementBoost_CanPurchase's
// role for AchievementBoost_Purchase. Refuses in order: boosts not unlocked,
// nothing invested (pointsInvested == 0 -- resetting a no-op configuration
// would only ever cost the fee for zero refund), can't afford the fee. Like
// CanPurchase, not gated on gSaveBlock1Ptr->achievementsBlocked -- a blocked
// run can still hold boost levels purchased before the block took effect,
// and a reset only ever refunds/clears what's already there, so it can't
// grant anything a blocked run shouldn't have.
bool8 AchievementBoost_CanReset(void);

// Re-runs AchievementBoost_CanReset (same "never trust a
// stale Can* result" precedent as AchievementBoost_Purchase) before
// committing pointsInvested = 0, zeroing every boostLevels[] entry, and
// deducting ACHIEVEMENT_BOOST_RESET_FEE -- refund is always exactly the
// pointsInvested that was there, so a reset can never generate points.
bool8 AchievementBoost_Reset(void);

// The first real boost effect. Wraps a raw exp value -- call this on the
// pre soft-level-cap amount ("wrap the input"), not the post-cap result, so
// a purchased boost still gets throttled by the level cap exactly like
// baseline exp does, rather than letting it push past the ceiling the cap
// exists to enforce. Returns expValue unchanged whenever boosts are
// disabled or BOOST_EXP_GAIN is at level 0, so the disabled path is provably
// identical to baseline.
u32 AchievementBoost_ApplyExp(u32 expValue);

// The remaining numerical boosts from the example boost list. Each follows
// AchievementBoost_ApplyExp's shape -- a no-op
// when boosts are disabled or the boost is at level 0, so the disabled path
// is always provably identical to baseline.

// ComputePlayerShinyOdds (src/pokemon.c) adds this to totalRerolls before
// its reroll loop, alongside the Shiny Charm/Lure/chain-fishing/DexNav
// rerolls it already accumulates -- one more independent source of rerolls,
// not a flat probability multiplier.
u32 AchievementBoost_ExtraShinyRerolls(void);

// ComputeCaptureOdds (src/battle_script_commands.c) applies this to its
// final 0-255 odds value, after the ball.guaranteedCapture (Master Ball)
// early return -- so a Master Ball catch is untouched, but a boosted value
// can still cross the existing odds > 254 "treat as guaranteed" threshold at
// the call site. No separate clamp needed here.
u32 AchievementBoost_ApplyCatchOdds(u32 odds);

// Cmd_getmoneyreward (src/battle_script_commands.c) applies this to the
// combined trainer money reward on a win, before AddMoney -- AddMoney's own
// MAX_MONEY clamp (src/money.c) is the only clamp a boosted value needs.
u32 AchievementBoost_ApplyMoneyReward(u32 money);

// TryProduceOrHatchEgg (src/daycare.c) applies this to GetEggCyclesToSubtract
// (src/egg_hatch.c)'s result before subtracting it from an egg's remaining
// cycle count -- a flat addition, the same way Magma Armor/Flame Body/Steam
// Engine already double the base value from 1 to 2.
u8 AchievementBoost_ApplyEggCyclesToSubtract(u8 toSub);

// CalculateFriendshipBonuses (src/pokemon.c) applies this to its final
// bonus, right before returning it. Only scales positive gains -- a negative
// bonus (e.g. fainting, a bitter herb) passes through unchanged, since this
// is a "friendship gain" boost, not a friendship-loss shield.
s32 AchievementBoost_ApplyFriendshipGain(s32 bonus);

// RoamerMove (src/roamer.c) rolls this once per move (itself called on
// every map transition) -- a flat 1% chance per level (maxLevel 5, so up to
// 5%) that a roamer skips its normal random relocation and is drawn
// straight onto the player's current route instead. Only ever fires while
// the player is on a route the roamer table actually covers, never a town/
// city/cave. Makes an inactive or absent roamer no more likely to exist or
// to be assigned to your region -- only changes where an already-active one
// wanders to.
bool8 AchievementBoost_ShouldRoamerSeekPlayer(void);

// Same no-op-when-disabled guarantee as everything above.
//
// The three Get*Percent functions return a raw 0-100 percent instead of
// rolling internally, because their call sites are all in battle, where
// randomness must go through the tagged RandomChance/RandomPercentage helpers
// to keep the test harness and recorded-battle playback deterministic. They
// return 0 on the baseline path so the caller can skip its roll and consume no
// RNG at all. All three are additionally gated at their call sites on
// IsOnPlayerSide() and on the battle not being a link/recorded battle -- boost
// levels differ between players, so an ungated roll would desync a link battle.

// IsCriticalHit (src/battle_util.c): a flat extra chance to upgrade a hit that
// the normal crit-stage roll already declined. Applied after the
// CRITICAL_HIT_BLOCKED check, so Battle Armor/Shell Armor/Lucky Chant still
// hard-block, and before the gPartyCriticalHits counter, so a boosted crit
// still counts toward the IF_CRITICAL_HITS_GE evolution condition.
u32 AchievementBoost_GetCritChancePercent(void);

// CancelerPPDeduction (src/battle_move_resolution.c): a flat chance that a
// move's PP cost (including any Pressure surcharge) is skipped for that use.
// The canceler's existing early-outs -- multi-turn moves, Dancer, bounced,
// snatched, Bide, Struggle -- already return before this point, so the boost
// only ever applies to a use that was actually going to cost PP.
u32 AchievementBoost_GetPpSavePercent(void);

// ENDTURN_STATUS_RECOVERY (src/battle_end_turn.c): rolled once per turn per
// living battler as a flat chance to clear a non-volatile status, using the
// same cure sequence Shed Skin does. Sits after the poison/burn/frostbite
// handlers, so status damage still ticks before the mon gets its chance to
// shake it off.
u32 AchievementBoost_GetStatusRecoveryPercent(void);

// GetBerryCountByBerryTreeId (src/berry.c): flat extra berries per harvest.
// Hooks the read rather than the saved berryYield field, so the bonus is
// recomputed every time -- switching the boost off restores baseline
// immediately instead of leaving it baked into already-grown trees. Returns 0
// unchanged, so a tree with no berries stays empty.
u8 AchievementBoost_ApplyBerryYield(u8 count);

// BerryTreeTimeUpdate and PlantBerryTree (src/berry.c): shortens the wait for
// a berry tree's next growth stage. Deliberately NOT applied to the
// BERRY_STAGE_BERRIES window (how long berries sit ready to pick) or to the
// unattended-tree death threshold -- shortening either would be a nerf.
u16 AchievementBoost_ApplyBerryStageDuration(u16 minutes);

// Every VAR_REPEL_STEP_COUNT write site (src/item_use.c, src/sprays.c):
// extends how many steps a Repel or Lure lasts. Clamped below REPEL_LURE_MASK
// so a boosted count can never bleed into the "this is a Lure" flag bit.
u16 AchievementBoost_ApplySprayStepCount(u16 steps);

// The three binary boosts. For these, "purchased" is the entire effect -- no
// effects[] value, no level to scale.

// CB2_EndWildBattle (src/battle_setup.c): in nuzlocke mode, an encounter the
// player didn't convert into a catch spends a one-time per-route free pass
// instead of locking the route. Catching still locks it immediately, and the
// free pass is only ever granted once per route.
bool8 AchievementBoost_HasNuzlockeSecondChance(void);

// NewGameInitData (src/new_game.c): grants starting items and extra money on a
// fresh game. Never applies to New Game+, which restores the previous save's
// bag and money afterward regardless.
bool8 AchievementBoost_HasStarterKit(void);

// GenerateIVs (src/ui_birch_case.c): the starter rolls 31 in every stat.
// Hooked at generation rather than at the grant, so the Birch Case's preview
// and the Pokemon actually received can never disagree.
bool8 AchievementBoost_HasPerfectStarterIvs(void);

// The first ten catalog hook functions. Each checks one category's
// thresholds against Achievement_TryComplete -- already idempotent, so
// every one of these is
// safe to call unconditionally every time its call site runs, not just when
// a threshold might have just been crossed.

// Common_EventScript_CheckLevelCapIncrease (data/scripts/level_cap.inc), via
// a single callnative: loops a static {flag, achievementId} table covering
// all 8 badges and the 7 non-badge story beats that already funnel through
// this shared script. Every one of the 16 call sites already sets its own
// milestone's flag on the line immediately before calling this script, so
// checking all 15 flags unconditionally here is correct and cheap. Also
// checks Who Needs Centers? at the FLAG_BADGE05_GET checkpoint.
void Achievement_CheckStoryMilestones(void);

// HandleSetPokedexFlag (src/pokemon.c), from inside its existing
// "not already set" guard -- caught should be TRUE only when caseId was
// FLAG_SET_CAUGHT, so a newly-seen (not caught) entry only ever checks the
// seen thresholds. Reads GetNationalPokedexCount as a percentage of
// NATIONAL_DEX_COUNT rather than a hardcoded species count, so the
// thresholds stay correct regardless of which expansion level a given build
// is compiled with. The caught branch only checks full completion
// (ACHIEVEMENT_CATCH_ALL) -- the 10/25/50% caught thresholds were dropped in
// favor of Achievement_CheckCaptureMilestones's hard-number ladder.
void Achievement_CheckPokedexMilestones(bool8 caught);

// GiveCapturedMonToPlayer (src/pokemon.c): reads
// GetGameStat(GAME_STAT_POKEMON_CAPTURES), already incremented for the
// current catch by the time this function runs (confirmed against
// data/battle_scripts_2.s: incrementgamestat precedes givecaughtmon).
// A hard-number ladder (100/350/700); the top tier,
// ACHIEVEMENT_CATCH_ALL, is a distinct-species check and lives in
// Achievement_CheckPokedexMilestones instead.
void Achievement_CheckCaptureMilestones(void);

// Also GiveCapturedMonToPlayer, called only when the mon being given is
// shiny (MON_DATA_IS_SHINY). Increments gAchievementProfile.shiniesObtained
// -- declared since early on but never wired up until now -- flushes,
// then checks the three shiny-count thresholds. GiveCapturedMonToPlayer also
// fires for a handful of scripted gift mons, not only wild catches, which is
// why this is "obtained" in the catalog text, not "caught".
void Achievement_OnShinyObtained(void);

// CB2_EndTrainerBattle (src/battle_setup.c), called unconditionally at the
// top regardless of outcome. Reads/increments
// gAchievementProfile.trainerBattlesLifetime instead of
// GAME_STAT_TRAINER_BATTLES, so progress counts across every playthrough on
// this profile rather than resetting each new game.
void Achievement_CheckTrainerBattleMilestones(void);

// CB2_EndWildBattle (src/battle_setup.c), same reasoning as the trainer
// version above -- reads/increments gAchievementProfile.wildBattlesLifetime.
void Achievement_CheckWildBattleMilestones(void);

// AddBagItem (src/item.c) -- one-off "obtain this specific item" checks,
// gated by the caller on the add having actually succeeded.
void Achievement_CheckItemMilestones(enum Item itemId);

// AddMoney (src/money.c), called with the post-clamp balance
// (GetMoney(moneyPtr) after SetMoney) -- checking the raw amount being added
// would under-count once the player is near MAX_MONEY.
void Achievement_CheckMoneyMilestones(u32 money);

// Task_EggHatch (src/egg_hatch.c), called right after AddHatchedMonToParty
// with whether the newly hatched mon is shiny. The
// count-based thresholds read/increment
// gAchievementProfile.eggsHatchedLifetime instead of GAME_STAT_HATCHED_EGGS,
// so progress counts across every playthrough on this profile rather than
// resetting each new game.
void Achievement_CheckEggMilestones(bool8 isShiny);

// Battle-tracking infrastructure. See src/achievements.c for struct
// AchievementBattleData and the "one entry point" discipline the hooks
// below follow -- battle-side code
// never calls Achievement_TryComplete directly for a category K entry.

// GetTrainerClassFromId(TRAINER_BATTLE_PARAM.opponentA) against the "boss"
// trainer classes -- gym leaders, Elite Four, Champion, rival, Team
// Aqua/Magma leaders. FALSE outside a trainer battle. Reused by many later
// hook functions, not just this one.
bool8 Achievement_IsMajorBattle(void);

// BattleStartClearSetData (src/battle_main.c), right after gBattleResults is
// zeroed: resets the whole per-battle tracking struct. EWRAM only, never
// saved -- a battle never spans a save, so nothing here belongs in
// AchievementRunData.
void Achievement_ClearBattleData(void);

// CancelerPPDeduction (src/battle_move_resolution.c), the same funnel Stage
// 10.1's BOOST_PP_SAVER hooks -- every early-out above that call means this
// only ever fires for a move that's actually being used. Player side only,
// never in a link or recorded battle (checked by the caller). typeBit is
// 1u << GetMoveType(move), computed at the call site so this header doesn't
// need enum Type's shape. movePosition >= MAX_MON_MOVES is tolerated (only
// the per-slot move-variety bit is skipped; typesUsed/STAB/setup/repeat still
// update) since Metronome/Z-move/Max Move call paths can reach this with an
// unusual position.
void Achievement_RecordMoveUsed(u8 partyIndex, enum Move move, u32 typeBit, u32 movePosition, bool8 isSTAB, bool8 isSetupMove);

// CalcTypeEffectivenessMultiplier (src/battle_util.c), gated by the caller on
// ctx->updateFlags (a real hit, not an AI damage estimate) -- same "is this
// real" flag TryInitializeFirstSTABMoveTrainerSlide already keys off of in
// that function.
void Achievement_RecordSuperEffectiveHit(void);

// IsCriticalHit (src/battle_util.c), the same funnel BOOST_CRIT_CHANCE
// hooks, gated the same way (player side, not link/recorded).
void Achievement_RecordCriticalHit(void);

// SetNonVolatileStatus (src/battle_script_commands.c), gated by the caller on
// the status landing on an opponent from a player-side move. statusBit is one
// of the ACHIEVEMENT_STATUS_BIT_* constants above.
void Achievement_RecordStatusInflicted(u8 statusBit);

// SetValuesOnFaint (src/battle_util.c)'s opponent-faint branch, gated by the
// caller the same way as every other battle-data write (never link/recorded).
// victimBattler is the battler that just fainted, attackerBattler is
// gBattlerAttacker at that exact moment. The two are equal for a
// self-inflicted/passive cause (a non-volatile status tick, confusion,
// recoil, Life Orb, ...); this function reads the still-intact status1 on
// victimBattler to decide whether that counts as a status KO instead of
// crediting a party slot.
void Achievement_RecordOpposingFaint(enum BattlerId victimBattler, enum BattlerId attackerBattler);

// HandleEndTurn_BattleWon (src/battle_main.c) -- evaluates every category K
// entry against the battle that just ended, once. The caller gates this on
// not being a link or recorded battle: gBattleResults (unlike
// AchievementBattleData) is maintained unconditionally by the vanilla engine,
// so without that gate a recorded-battle replay could satisfy a
// gBattleResults-derived entry (e.g. Clean Sweep) a second time from stale
// data rather than a live result.
void Achievement_CheckBattleMilestones(void);

// ---- Team Building & Composition (category L) -------------------------
//
// The first real user of struct AchievementRunData (include/global.h). Three
// call sites, each reusing an existing hook rather than adding a new one:
//   Achievement_CheckTeamMilestones          same site as Achievement_CheckBattleMilestones
//   Achievement_CheckPartyStateMilestones    same callnative as Achievement_CheckStoryMilestones
//   Achievement_CheckTeamCompletionMilestones GameClear (src/post_battle_event_funcs.c)

// GetTrainerClassFromId(TRAINER_BATTLE_PARAM.opponentA) == TRAINER_CLASS_LEADER
// specifically -- the subset of Achievement_IsMajorBattle() that's an actual
// Gym battle, not Elite Four/Champion/rival/Team leaders.
bool8 Achievement_IsGymBattle(void);

// HandleEndTurn_BattleWon (src/battle_main.c), immediately after
// Achievement_CheckBattleMilestones, gated the same way (never link/
// recorded). Reads AchievementBattleData.slotsThatActed/lastThreeKoSlots
// alongside the live party, so this adds no new battle-side hooks of its
// own -- it rides Achievement_CheckBattleMilestones's.
void Achievement_CheckTeamMilestones(void);

// Achievement_CheckStoryMilestones (src/achievements.c), called at the tail
// of that function -- the same callnative already threaded through all 16
// Common_EventScript_CheckLevelCapIncrease call sites (data/scripts/level_cap.inc).
// Checks party state that isn't tied to a specific battle (held items,
// level-cap standing).
void Achievement_CheckPartyStateMilestones(void);

// GameClear (src/post_battle_event_funcs.c), alongside
// Achievement_OnFirstPlaythroughComplete -- fires once per completed run,
// including every New Game+ cycle (FLAG_SYS_GAME_CLEAR isn't preserved across
// NG+, so that branch already re-runs each cycle; see
// Achievement_OnNewGamePlusStarted's notes).
void Achievement_CheckTeamCompletionMilestones(void);

// GiveCapturedMonToPlayer (src/pokemon.c) and Task_EggHatch (src/egg_hatch.c)
// -- the same two call sites Achievement_CheckCaptureMilestones/
// Achievement_CheckEggMilestones already hook. Gift and traded-in Pokemon
// aren't tracked by this (no single funnel point exists for them the way
// catches and hatches already have one); Fresh Start undercounts rather than
// overcounts as a result, which is the safer direction for an achievement
// condition to be wrong in.
//
// Also checks One of Each (moved here from Achievement_CheckRecordsMilestones):
// obtaining a Pokemon is the only thing that can raise the distinct-species
// count, so this funnel fires exactly when that threshold can be crossed. The
// same gift/trade gap described above applies -- a gift or traded-in Pokemon
// that takes the player to 10 distinct species won't award it until their next
// catch or hatch, the same undercount-not-overcount tradeoff already accepted
// for Fresh Start.
void Achievement_RecordMonObtained(u32 personality);

// ---- Exploration, Economy & Collection (category M) --------------------
// Nine call sites, each reusing an existing single-fire event.

// LoadCurrentMapData (src/overworld.c), the one function all three real
// map-transition paths funnel through -- see that function's own comment
// for why hooking there instead of its three callers avoids tripling the
// count. Updates AchievementRunData.mapsVisited and checks every
// map-count/FLAG_VISITED_*/No Loose Ends threshold; none of those need a
// battle or story hook of their own.
void Achievement_CheckExplorationMilestones(void);

// Achievement_CheckPokedexMilestones's FLAG_SET_SEEN branch (src/achievements.c,
// category B) -- checked only when a species is newly seen, so
// Local Expert can only complete at the exact moment the last species on the
// player's current route becomes seen, on this route or any other.
void Achievement_CheckLocalExpert(void);

// SetHiddenItemFlag (src/field_specials.c), the native the hidden-item
// script calls to mark a hidden item found -- already only reached once per
// item (the overworld gates the script itself on the flag not being set
// yet, src/field_control_avatar.c). Reads/increments
// gAchievementProfile.hiddenItemsFoundLifetime instead of
// GAME_STAT_HIDDEN_ITEMS_FOUND, so progress counts across every playthrough.
void Achievement_CheckHiddenItemMilestones(void);

// GetInteractionScript's object-event branch (src/field_control_avatar.c) --
// fires once per NPC interaction *started*, not once per msgbox inside that
// conversation's script, which would otherwise over-count a single
// conversation. Reads/increments gAchievementProfile.npcsTalkedToLifetime
// instead of GAME_STAT_NPCS_TALKED_TO, so progress counts across every
// playthrough.
void Achievement_RecordNpcTalkedTo(void);

// BuyMenuSubtractMoney (src/shop.c), right after the vanilla
// IncrementGameStat(GAME_STAT_SHOPPED) call -- amountSpent is
// sShopData->totalCost. Checks First Purchase/Regular Customer and Big
// Spender/Whale, and sets AchievementRunData.shoppedSinceLastGym for
// Achievement_CheckGymEconomyMilestones. The four
// thresholds read/increment gAchievementProfile.shopPurchasesLifetime/
// moneySpentLifetime instead of GAME_STAT_SHOPPED/GAME_STAT_MONEY_SPENT, so
// progress counts across every playthrough rather than resetting each new
// game; GAME_STAT_MONEY_SPENT is still updated in case anything else reads
// it.
void Achievement_RecordMoneySpent(u32 amountSpent);

// The sell-item AddMoney call in src/item_menu.c -- separate from
// Achievement_RecordMoneySpent because Treasure Pays tracks proceeds, not
// spending, and the two must never be confused with each other or with
// Achievement_CheckMoneyMilestones's held-balance checks.
void Achievement_RecordItemSaleProceeds(u32 amount);

// AddBagItem (src/item.c), the same "added succeeded" guard
// Achievement_CheckItemMilestones already sits behind (category G) --
// scans every non-key-item Bag pocket for Pack Rat.
void Achievement_CheckPackRatMilestone(void);

// ObjectEventInteractionPickBerryTree (src/berry.c) -- one harvest action,
// same "count the action, not the yield" convention GAME_STAT_PLANTED_BERRIES
// already uses for planting.
void Achievement_RecordBerryHarvest(void);

// Achievement_CheckTradeMilestones (called from both
// GAME_STAT_POKEMON_TRADES sites in src/trade.c) removed along with its sole
// achievement, ACHIEVEMENT_COLLECT_TRADE_SECRETS -- see src/data/achievements.h.

// Both GAME_STAT_EVOLVED_POKEMON sites (src/evolution_scene.c) -- checks
// Evolutionary Path/Evolution Expert against the count vanilla already
// incremented at that exact point.
void Achievement_CheckEvolutionCountMilestones(void);

// GetEvolutionTargetSpecies's EVO_MODE_NORMAL/EVO_MODE_BATTLE_ONLY case
// (src/pokemon.c), gated by the caller on evoState == DO_EVO -- CHECK_EVO
// runs first and must never award anything, or eligibility checks alone
// (e.g. opening the party menu) would complete this. Fires when the
// evolution that's actually about to happen matched via an IF_MIN_FRIENDSHIP
// condition.
void Achievement_RecordFriendshipEvolution(void);

// PokemonUseItemEffects's ITEM4_EVO_STONE case (src/pokemon.c), gated by the
// caller on the item actually being one of the twelve stone items (not
// every EVO_ITEM item is a stone in every expansion configuration).
void Achievement_RecordStoneEvolution(void);

// GiveCapturedMonToPlayer (src/pokemon.c), alongside
// Achievement_RecordMonObtained -- gDexNavSpecies is nonzero only
// during a battle that a DexNav scan actually started (src/dexnav.c), so
// this can't fire for an unrelated catch or a gift mon.
void Achievement_CheckDexNavCaptureMilestone(void);

// The GAME_STAT_FISHING_ENCOUNTERS increment in src/wild_encounter.c --
// Angler's threshold check against the count vanilla already incremented.
void Achievement_CheckFishingMilestone(void);

// HandleEndTurn_BattleWon (src/battle_main.c), immediately after
// Achievement_CheckTeamMilestones, gated the same way (never link/recorded).
// Branches internally on Achievement_IsGymBattle()/Achievement_IsMajorBattle(),
// the same style Achievement_CheckTeamMilestones uses -- not a new battle
// hook, just more entries riding the one that already exists.
void Achievement_CheckGymEconomyMilestones(void);

// GameClear (src/post_battle_event_funcs.c), alongside
// Achievement_CheckTeamCompletionMilestones -- Investor's "finish the story
// holding >= 500000" check.
void Achievement_CheckEconomyCompletionMilestones(void);

// ---- Challenge Runs & Nuzlocke (category N) ----------------------------
//
// Four call sites, each reusing an existing hook rather than adding a new
// one -- see include/constants/achievements.h's category N doc comment for
// the overview.

// HandleEndTurn_BattleWon (src/battle_main.c), immediately after
// Achievement_CheckGymEconomyMilestones, gated the same way (never link/
// recorded). Covers every Challenge-category entry that's evaluated
// battle-by-battle -- No Healing Items/Itemless Battle/Set in Stone/
// Minimalist/Level Discipline -- plus the running high-water mark
// (highestPartySizeThisRun) and starter-tracking bookkeeping that
// Achievement_CheckChallengeCompletionMilestones reads at GameClear.
//
// Randomizer & New Game+ entries ride this same call site rather than
// adding a new one -- Random by Nature (Gym clear), Chaos Team/Never Seen It Coming/
// Patchwork Team (major battle win), trainer-win bookkeeping
// (AchievementRunDataExt.trainersDefeatedThisCycle/gAchievementProfile's
// trainersDefeatedAcrossNgPlus for Fresh Faces/Never the Same Fight), and
// the per-cycle Boss Gauntlet/Complete Reinvention bookkeeping that
// Achievement_OnNewGamePlusCycleCompleted reads at GameClear.
void Achievement_CheckChallengeMilestones(void);

// Same call site as above, immediately after it. Every entry here is
// additionally gated on gSaveBlock1Ptr->nuzlockeModeEnabled: First Nuzlocke,
// Close Call, Scrappy, No Ace Allowed. Self-contained rather than reusing
// Achievement_CheckTeamMilestones's locals (that function owns those locals).
void Achievement_CheckNuzlockeMilestones(void);

// Achievement_CheckNuzlockeExplorationMilestones removed -- see
// src/achievements.c's own comment where the function used to be.

// GameClear (src/post_battle_event_funcs.c), alongside
// Achievement_CheckTeamCompletionMilestones/Achievement_CheckEconomyCompletionMilestones
// -- same re-runs-every-NG+-cycle gating. Covers every "complete the story"
// Challenge-category entry: Self-Imposed/Hard Way/Brutal Rules/Nightmare Mode
// (Achievement_CountChallengeModifiers), No Shopping Run, No Centers,
// Hardcore Set, Capstone/Perfectly Capped (reads
// levelCapEverExceeded), Three-Pokemon Challenge/Solo Journey, No Freebies,
// Hardly Any Help.
void Achievement_CheckChallengeCompletionMilestones(void);

// Same call site as above. Every entry here is gated on nuzlockeModeEnabled:
// Hardcore Survivor, Perfect Nuzlocke/The Graveyard, Species Clause (scans
// party + every PC box -- under Nuzlocke rules that's exactly the set of
// Pokemon caught this run), No Second Chances, Full Encounter, Unassisted
// Survivor.
//
// Also Nuzlocke Across Worlds/Chaos Survivor -- a completed Nuzlocke run
// with any FLAG_RANDOMIZE_* flag set (plus HARD difficulty for the latter).
void Achievement_CheckNuzlockeCompletionMilestones(void);

// BuyMenuSubtractMoney (src/shop.c), alongside Achievement_RecordMoneySpent
// -- called only when the purchased item is in POCKET_ITEMS (the general
// Items pocket specifically -- potions, revives, status healers, repels,
// battle items). Sets AchievementRunData.boughtConsumableItem for No
// Shopping Run.
void Achievement_RecordConsumableItemPurchase(void);

// Achievement_RecordReviveUsed (formerly called from
// BS_ItemRestoreHP in src/battle_script_commands.c and PokemonUseItemEffects
// in src/pokemon.c) removed along with its sole achievement,
// ACHIEVEMENT_NUZLOCKE_NO_REVIVES -- see src/data/achievements.h.

// RemoveFaintedMonsFromParty (src/overworld.c), the single function every
// Nuzlocke fainted-mon removal funnels through -- called once per Pokemon
// actually removed. Increments AchievementRunData.nuzlockeMonsLost for
// Perfect Nuzlocke/The Graveyard.
void Achievement_RecordNuzlockeMonLost(void);

// ui_birch_case.c, right after the starter is granted (ScriptGiveMonParameterized)
// -- records its personality (survives evolution, unlike species) so
// Achievement_CheckChallengeMilestones can tell whether it ever acts in a
// major battle, for No Freebies.
void Achievement_RecordStarterPersonality(u32 personality);

// ---- Randomizer & New Game+ ---------------------------------------------
//
// GiveCapturedMonToPlayer (src/pokemon.c), alongside
// Achievement_CheckCaptureMilestones -- one more call at that same funnel,
// not a new one. Randomized Rookie: GetGameStat(GAME_STAT_POKEMON_CAPTURES)
// (already incremented, this-run count -- see comment on ResetGameStats,
// src/overworld.c) >= 25 while any FLAG_RANDOMIZE_* flag is set.
void Achievement_CheckRandomizerCaptureMilestone(void);

// ---- Streaks, Records & Collection Remainder (category P) --------------
// Nine call sites; see include/constants/achievements.h's
// category P comment for the full breakdown and src/achievements.c for
// each function's own doc comment.

// HandleEndTurn_BattleWon (src/battle_main.c), immediately after
// Achievement_CheckNuzlockeMilestones, gated the same way (never link/
// recorded). Covers every entry evaluated battle-by-battle: the trainer win
// streak and Gym/League streaks, per-slot KO totals, Comeback Count, Oddball
// and Underestimated.
void Achievement_CheckBattleRecordsMilestones(void);

// LoadCurrentMapData (src/overworld.c), alongside
// Achievement_CheckExplorationMilestones -- map transitions are
// frequent enough during normal play to catch these live-state thresholds
// (nothing here is tied causally to a specific event) without a hook of
// their own: Growing Strong, Century Club/Full Century,
// Box Filler/Storage Baron, Devoted/Inseparable.
//
// One of Each was checked here too until it moved to
// Achievement_RecordMonObtained -- it was the only entry that walked storage
// rather than just the party, which made it far too expensive to re-run on
// every map transition. See the note left at its old site in src/achievements.c.
void Achievement_CheckRecordsMilestones(void);

// GameClear (src/post_battle_event_funcs.c), alongside
// Achievement_CheckNuzlockeCompletionMilestones -- Legend of the Run reads
// AchievementRunDataExt.presentAtEveryMajorBattleSlots, which is only
// meaningful once a completed run's major battles are all in.
void Achievement_CheckRecordsCompletionMilestones(void);

// RemoveFaintedMonsFromParty (src/overworld.c) and FldEff_PokecenterHeal
// (src/field_effect.c), the same two IsPartyEmpty()-gated sites the
// Nuzlocke wipe detection (Achievement_CheckNuzlockeMilestones) already
// uses -- see that function's own comment for why no third detector is
// added. Mirrors the run's win-streak high-water
// mark into gAchievementProfile.bestTrainerWinStreakEver, then zeroes the
// streak counters this wipe just broke.
void Achievement_RecordPartyWipe(void);

// SetValuesOnFaint (src/battle_util.c)'s player-faint branch, gated by the
// caller the same way as every other battle-data write (never link/
// recorded). Sets sBattleData.wasDownToLastMon once the player is down to
// their last conscious Pokemon, for Comeback Count -- read (and reset) by
// Achievement_CheckBattleRecordsMilestones on the next win.
void Achievement_RecordPlayerFaint(void);

// HandleSetPokedexFlag (src/pokemon.c)'s FLAG_SET_CAUGHT branch, alongside
// Achievement_CheckPokedexMilestones -- Family Reunion. species is the
// species that was just newly caught; walks its evolution family (both
// directions) and completes if every stage is also caught.
void Achievement_CheckFamilyMilestone(enum Species species);

// Achievement_CheckPerfectIvMilestone (formerly called from
// GiveCapturedMonToPlayer in src/pokemon.c and Task_EggHatch in
// src/egg_hatch.c) removed along with its sole achievement,
// ACHIEVEMENT_COLLECT_PERFECT_SPECIMEN -- see src/data/achievements.h.

// Task_LearnedMove (src/party_menu.c), gated by the caller on move[1] == 0
// (the TM/HM item-use path specifically, not the move relearner or a move
// tutor NPC -- see that function's own comment) and on the item actually
// being a TM rather than an HM. Move Tutor (backfill).
void Achievement_RecordTMTaught(void);

// FldEff_PokecenterHeal (src/field_effect.c), right after the vanilla
// IncrementGameStat(GAME_STAT_USED_POKECENTER) call.
// Nurse's Nightmare (backfill).
void Achievement_CheckPokecenterMilestone(void);

// ---- Profile Meta, Mastery & Prestige (category Q) ----------------------
// No declarations here, unlike every prior category: every entry is
// a meta-achievement over state the rest of the system already exposes
// (Achievement_IsCompleted, gAchievements[].category/.tier, and the profile
// fields above), so it needs no new external call site. Checked entirely
// from within src/achievements.c -- the tail of Achievement_TryComplete
// (alongside Achievement_CheckPointMilestones) and AchievementBoost_Purchase/
// _Reset (the only two places boostLevels[]/pointsInvested/boostResets
// change). See include/constants/achievements.h's category Q comment for the
// full roster-to-condition breakdown.

// ---- Recruits/Limited Party/Draft/Rotation/Mono Type/Mono Gen -----------

// HandleEndTurn_BattleWon (src/battle_main.c), alongside
// Achievement_CheckBattleRecordsMilestones/Recruits_TallyParticipants. Every
// entry evaluated on a trainer-battle win for these six modes: Fresh
// Recruits/Tour of Duty (Recruits), Tight Squad/No Room to Spare (Limited
// Party), Spin the Wheel/On a Rotation/Gym Leader Roulette (Rotation).
void Achievement_CheckNewModeBattleMilestones(void);

// GameClear (src/post_battle_event_funcs.c), alongside every other
// completion check. Every "complete the story with X" entry for these six
// modes, plus the Cross-Mode stacking entries (Mode Collector/Kitchen Sink/
// The Full Stack).
void Achievement_CheckNewModeCompletionMilestones(void);

// Recruits_DoRetirement (src/recruits_mode.c). Honorable Discharge
// unconditionally, plus the Revolving Door/Full Turnover retirement-count
// ladder.
void Achievement_RecordRecruitRetirement(void);

// Recruits_StartRunFailedScreen (src/recruits_mode.c). Marks this cycle as
// having had a run failure, so Never Understaffed's GameClear check can
// refuse.
void Achievement_RecordRecruitRunFailed(void);

// Draft_MarkAreaSpent (src/draft_mode.c), only in the branch that just
// resolved a real draft pick (not a gift/egg or a no-offer area). First Pick
// unconditionally, plus the drafts-completed ladder (The Case is Closed/Full
// Case Clear).
void Achievement_RecordDraftCompleted(void);

// Draft_DoReplacement (src/draft_mode.c). Tough Call.
void Achievement_RecordDraftReplacement(void);

// BirchCase_GiveMon (src/ui_birch_case.c), alongside
// Achievement_RecordStarterPersonality -- the normal (non-Draft) starter
// grant. Committed to the Bit/Generation Loyalist.
void Achievement_CheckMonoStarterMilestones(void);

// GiveCapturedMonToPlayer (src/pokemon.c) and Task_EggHatch
// (src/egg_hatch.c), alongside Achievement_RecordMonObtained. Every mon
// obtainable while Mono Type/Mono Gen is enabled is already restricted to
// the chosen type/generation (or an unresolved gen-0 species), so this just
// counts obtains while each mode is on -- Perfect Fit/Gotta Catch Some of
// Them.
void Achievement_RecordMonoModeObtain(void);

// Debug-only. src/debug.c is the only caller.
// These bypass all the validation the real functions above add
// (achievement completion rules, boost costs/maxLevel, reset fee) by design,
// so the rest of the system can be exercised and tested independently of
// whatever real content/rules exist at a given point in development.
void  Achievement_DebugSetCompleted(u16 achievementId, bool8 completed);
void  Achievement_DebugSetPoints(u32 amount);
void  Achievement_DebugSetBoostsUnlocked(bool8 unlocked);
void  AchievementBoost_DebugSetLevel(u16 boostId, u8 level);
void  AchievementBoost_DebugReset(void);
void  Achievement_DebugMarkPlaythroughComplete(void);

#endif // GUARD_ACHIEVEMENTS_H
