#ifndef GUARD_ACHIEVEMENTS_H
#define GUARD_ACHIEVEMENTS_H

#include "global.h"
#include "constants/achievements.h"
#include "constants/items.h" // enum Item, for Achievement_CheckItemMilestones (Stage 13 category G)

#define ACHIEVEMENT_PROFILE_MAGIC   0x50454143  // 'PEAC'
#define ACHIEVEMENT_PROFILE_VERSION 1
#define MAX_ACHIEVEMENTS            512   // reserved ceiling -> 64 bytes of flags
#define MAX_BOOSTS                  32

// Stage 11 (design doc §13): single tunable fee, in Poké money, to reset every
// boostLevels[] entry and refund pointsInvested in full. Real balancing is
// Stage 14's job (design doc §13: "start conservative and tune after
// playtesting") -- deliberately not an escalating fee yet.
#define ACHIEVEMENT_BOOST_RESET_FEE 5000

// Cap for .name in gAchievements[] (src/data/achievements.h), enforced at
// compile time via ACHIEVEMENT_NAME() there -- same pattern as ITEM_NAME_LENGTH.
#define ACHIEVEMENT_NAME_LENGTH     24

// Cap for .name in gAchievementBoosts[] (src/data/achievement_boosts.h),
// enforced at compile time via BOOST_NAME() there.
#define BOOST_NAME_LENGTH           24

// Definition data (design doc §4, Stage 2.1/2.2) -- one const entry per
// enum AchievementId, in src/data/achievements.h. Kept strictly separate
// from struct AchievementProfile below, which is completion *state* only.
struct Achievement
{
    const u8 *name;
    const u8 *description;
    enum AchievementTier tier;
    enum AchievementScope scope;
    u16 points;
    bool8 hidden;
};

// Boost definition data (design doc §11, Stage 7) -- one const entry per
// enum BoostId, in src/data/achievement_boosts.h. Kept strictly separate
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

    u8  reserved[32];             // forward compatibility
};

extern struct AchievementProfile gAchievementProfile;

// A profile read/write failure must never be able to trigger the save-failed
// screen (that's gDamagedSaveSectors/gSaveFileStatus's job, not this one).
bool8 Achievement_ProfileWriteFailed(void);

// Writes the profile to flash only if it's been marked dirty since the last
// flush (see src/achievements.c). Safe to call unconditionally from any of
// the flush points in design doc §1.3.
void Achievement_FlushProfile(void);

// Public API (design doc §24). Nothing outside src/achievements.c writes the
// profile.

// Loads the profile from flash. Call once at boot, before any save is loaded.
void  Achievement_Init(void);
bool8 Achievement_IsCompleted(u16 achievementId);

// gAchievements[] (src/data/achievements.h) is only ever included from
// src/achievements.c -- this is how the rest of the game (e.g. the
// Achievements Menu, Stage 3) reads definition data (name/description/tier/
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

// Boost menu (Stage 7 UI polish): moves the active level by delta, clamped to
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

// Runs the order in design doc §4.30 (Stage 2.3): checks run eligibility
// (§1.5) and prior completion, then commits the flag and points together
// before queuing a notification. Returns FALSE without side effects if the
// run is ineligible, the ID is out of range, or it's already completed.
bool8 Achievement_TryComplete(u16 achievementId);

// design doc §5.1/§5.2 (Stage 5): the one-time first-playthrough unlock.
// Call this from GameClear() (src/post_battle_event_funcs.c) in the branch
// that only runs the first time FLAG_SYS_GAME_CLEAR is set for this save --
// that flag's own set-once semantics are what guarantee this runs exactly
// once per playthrough, so this function has no completion guard of its own.
void Achievement_OnFirstPlaythroughComplete(void);

// design doc Stage 12: called from NewGameInitData (src/new_game.c) right
// after gSaveBlock2Ptr->newGamePlus++, so `cycle` is always the just-started
// cycle's number (1 for the first NG+ loop, 2 for the second, ...).
// highestNgPlusCycle is a high-water mark, not a live counter --
// gSaveBlock2Ptr->newGamePlus already is that -- so this only ever grows it.
void Achievement_OnNewGamePlusStarted(u8 cycle);

// design doc Stage 12: called from the same GameClear() branch as
// Achievement_OnFirstPlaythroughComplete above, additionally gated there on
// gSaveBlock2Ptr->newGamePlus > 0 -- FLAG_SYS_GAME_CLEAR isn't preserved
// across New Game+ (see NewGameInitData), so that branch already re-runs on
// every NG+ cycle's clear, not just the save's very first playthrough. This
// counts specifically the subset of those clears that happened during an NG+
// loop, distinct from the plain playthroughsCompleted total.
void Achievement_OnNewGamePlusCycleCompleted(void);

// design doc §7 (Stage 7): validates in order -- boosts unlocked, current
// level < maxLevel, availablePoints >= costs[level] -- refusing at the first
// failure. Purchase re-runs the same checks (so it can never be called
// directly around a stale CanPurchase result) before committing
// pointsInvested += cost, boostLevels[id]++, and flushing immediately.
// Deliberately NOT gated on gSaveBlock1Ptr->achievementsBlocked (§1.5) --
// that flag disqualifies a run from *earning* new achievements/points
// (enforced in Achievement_TryComplete), not from spending points already
// on the books.
bool8 AchievementBoost_CanPurchase(u16 boostId);
bool8 AchievementBoost_Purchase(u16 boostId);

// Stage 11 (design doc §13/§23 "reset exploits"): a non-mutating query so the
// boost shop can gate entry into its confirmation prompt without spending the
// fee just to find out the answer -- mirrors AchievementBoost_CanPurchase's
// role for AchievementBoost_Purchase. Refuses in order: boosts not unlocked,
// nothing invested (pointsInvested == 0 -- resetting a no-op configuration
// would only ever cost the fee for zero refund), can't afford the fee. Like
// CanPurchase, not gated on gSaveBlock1Ptr->achievementsBlocked -- a blocked
// run can still hold boost levels purchased before the block took effect,
// and a reset only ever refunds/clears what's already there, so it can't
// grant anything a blocked run shouldn't have.
bool8 AchievementBoost_CanReset(void);

// design doc Stage 11: re-runs AchievementBoost_CanReset (same "never trust a
// stale Can* result" precedent as AchievementBoost_Purchase) before
// committing pointsInvested = 0, zeroing every boostLevels[] entry, and
// deducting ACHIEVEMENT_BOOST_RESET_FEE -- refund is always exactly the
// pointsInvested that was there, so a reset can never generate points
// (design doc §23).
bool8 AchievementBoost_Reset(void);

// design doc Stage 8: the first real boost effect. Wraps a raw exp value --
// call this on the pre soft-level-cap amount (design doc's "wrap the
// input"), not the post-cap result, so a purchased boost still gets
// throttled by the level cap exactly like baseline exp does, rather than
// letting it push past the ceiling the cap exists to enforce. Returns
// expValue unchanged whenever boosts are disabled or BOOST_EXP_GAIN is at
// level 0, so the disabled path is provably identical to baseline.
u32 AchievementBoost_ApplyExp(u32 expValue);

// Stages 9-10: the remaining numerical boosts from the design doc's §10.1
// example list. Each follows AchievementBoost_ApplyExp's shape -- a no-op
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

// Stage 10.1: catalog wave 2. Same no-op-when-disabled guarantee as everything
// above.
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

// Stage 13 (design doc §15/§16, plan Stage 13): catalog wave 1's ten hook
// functions. Each checks one category's thresholds against
// Achievement_TryComplete -- already idempotent, so every one of these is
// safe to call unconditionally every time its call site runs, not just when
// a threshold might have just been crossed.

// Common_EventScript_CheckLevelCapIncrease (data/scripts/level_cap.inc), via
// a single callnative: loops a static {flag, achievementId} table covering
// all 8 badges and the 7 non-badge story beats that already funnel through
// this shared script. Every one of the 16 call sites already sets its own
// milestone's flag on the line immediately before calling this script, so
// checking all 15 flags unconditionally here is correct and cheap.
void Achievement_CheckStoryMilestones(void);

// HandleSetPokedexFlag (src/pokemon.c), from inside its existing
// "not already set" guard -- caught should be TRUE only when caseId was
// FLAG_SET_CAUGHT, so a newly-seen (not caught) entry only ever checks the
// seen thresholds. Reads GetNationalPokedexCount as a percentage of
// NATIONAL_DEX_COUNT rather than a hardcoded species count, so the
// thresholds stay correct regardless of which expansion level a given build
// is compiled with.
void Achievement_CheckPokedexMilestones(bool8 caught);

// GiveCapturedMonToPlayer (src/pokemon.c): reads
// GetGameStat(GAME_STAT_POKEMON_CAPTURES), already incremented for the
// current catch by the time this function runs (confirmed against
// data/battle_scripts_2.s: incrementgamestat precedes givecaughtmon).
void Achievement_CheckCaptureMilestones(void);

// Also GiveCapturedMonToPlayer, called only when the mon being given is
// shiny (MON_DATA_IS_SHINY). Increments gAchievementProfile.shiniesObtained
// -- declared since early on but never wired up until this stage -- flushes,
// then checks the three shiny-count thresholds. GiveCapturedMonToPlayer also
// fires for a handful of scripted gift mons, not only wild catches, which is
// why this is "obtained" in the catalog text, not "caught".
void Achievement_OnShinyObtained(void);

// CB2_EndTrainerBattle (src/battle_setup.c), called unconditionally at the
// top regardless of outcome: GAME_STAT_TRAINER_BATTLES is incremented at
// battle *start* (a dozen-plus scattered Do*Battle functions), so by the
// time any given trainer battle ends the count is already final -- no need
// to touch every start site.
void Achievement_CheckTrainerBattleMilestones(void);

// CB2_EndWildBattle (src/battle_setup.c), same reasoning as the trainer
// version above, reading GAME_STAT_WILD_BATTLES.
void Achievement_CheckWildBattleMilestones(void);

// AddBagItem (src/item.c), alongside the existing
// ACHIEVEMENT_TEST_OBTAIN_POTION hook -- one-off "obtain this specific item"
// checks, gated by the caller on the add having actually succeeded.
void Achievement_CheckItemMilestones(enum Item itemId);

// AddMoney (src/money.c), called with the post-clamp balance
// (GetMoney(moneyPtr) after SetMoney) -- checking the raw amount being added
// would under-count once the player is near MAX_MONEY.
void Achievement_CheckMoneyMilestones(u32 money);

// Task_EggHatch (src/egg_hatch.c), called right after AddHatchedMonToParty
// with whether the newly hatched mon is shiny -- GAME_STAT_HATCHED_EGGS is
// already incremented well before this point (see
// src/field_control_avatar.c, at the start of the whole hatch sequence).
void Achievement_CheckEggMilestones(bool8 isShiny);

// Debug-only (design doc §21, Stage 1.7). src/debug.c is the only caller.
// These bypass all the validation the real Stage 2/7/11 functions above add
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
