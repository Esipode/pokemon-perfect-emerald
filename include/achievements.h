#ifndef GUARD_ACHIEVEMENTS_H
#define GUARD_ACHIEVEMENTS_H

#include "global.h"
#include "constants/achievements.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/battle.h"

#define ACHIEVEMENT_PROFILE_MAGIC   0x50454143  // 'PEAC'
#define ACHIEVEMENT_PROFILE_VERSION 1
#define MAX_ACHIEVEMENTS            512   // reserved ceiling -> 64 bytes of flags
#define MAX_BOOSTS                  32

// Fee, in Poké money, to reset every boostLevels[] entry and refund pointsInvested in full.
#define ACHIEVEMENT_BOOST_RESET_FEE 5000

// Enforced at compile time via ACHIEVEMENT_NAME() in src/data/achievements.h.
#define ACHIEVEMENT_NAME_LENGTH     24

// Enforced at compile time via BOOST_NAME() in src/data/achievement_boosts.h.
#define BOOST_NAME_LENGTH           24

// Bits for struct AchievementBattleData.statusesInflicted (src/achievements.c).
// Match enum MoveEffect's non-volatile statuses one-for-one (Toxic folds into
// POISON, Frostbite into FREEZE). Computed at each hook's call site so this
// header doesn't need the full move effect table.
#define ACHIEVEMENT_STATUS_BIT_SLEEP      (1 << 0)
#define ACHIEVEMENT_STATUS_BIT_POISON     (1 << 1)
#define ACHIEVEMENT_STATUS_BIT_BURN       (1 << 2)
#define ACHIEVEMENT_STATUS_BIT_PARALYSIS  (1 << 3)
#define ACHIEVEMENT_STATUS_BIT_FREEZE     (1 << 4)

// Definition data, one const entry per enum AchievementId (src/data/achievements.h).
// Kept separate from struct AchievementProfile, which is completion state only.
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

// Boost definition data, one const entry per enum BoostId (src/data/achievement_boosts.h).
// Kept separate from struct AchievementProfile, which stores only the player's state.
struct AchievementBoost
{
    const u8 *name;
    const u8 *description;
    u8 type;                  // BOOST_TYPE_LEVELED | BOOST_TYPE_BINARY
    u8 maxLevel;               // 1 for binary
    const u16 *costs;          // costs[level] -- cost to go from level to level+1
    const u16 *effects;        // effects[level]; units are per-boost, not rendered generically
    const u8 *effectFormat;    // Appended to the description for the active level; {STR_VAR_2} = effects[level]. NULL for binary
};

// Saved to flash. New fields go at the front of what was reserved[]: those bytes
// are zero in older profiles, so every earlier field keeps its offset. Do not insert
// fields earlier in the struct. WriteAchievementProfile zero-fills the whole sector
// before writing, so a field missing from an older profile also reads back as 0.
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

    // Fresh-save Hall of Fame clears only (newGamePlus == 0 at the clear); New Game+
    // clears are counted in ngPlusCyclesCompleted. No achievement reads this, but the
    // debug profile dump (src/debug.c) still shows it.
    u16 playthroughsCompleted;
    u16 ngPlusCyclesCompleted;
    u8  highestNgPlusCycle;
    u16 nuzlockesCompleted;
    u16 randomizedRunsCompleted;
    u16 shiniesObtained;
    u16 boostResets;

    // How many purchased levels below boostLevels[i] the boost's active effect is
    // dialed back to (0 = fully active). Binary boosts only use 0 or 1 (off/on).
    // See AchievementBoost_GetActiveLevel/_TryChangeActiveLevel.
    u8  boostLevelReduction[MAX_BOOSTS];

    u16 trainersDefeatedAcrossNgPlus;    // for Never the Same Fight (NGP-008)
    // Unused fields below are left in place to preserve later offsets.
    u8  consecutiveNgPlusCyclesCompleted; // unused, backed the removed ACHIEVEMENT_NG_PLUS_ESCALATION
    u8  ngPlusConfigsSeen[4];            // unused, backed the removed Cycle Collector
    u8  ngPlusConfigsSeenCount;          // unused, same removal as above
    bool8 completedConventionalRun;      // unused, backed the removed Full Circle

    // Persistent high-water mark of AchievementRunDataExt.bestTrainerWinStreakThisRun,
    // mirrored on every party wipe (Achievement_RecordPartyWipe). No roster entry reads it.
    u16 bestTrainerWinStreakEver;

    u32 pointsFromGoldOrBetter;   // sum of .points over every Gold-or-better achievement completed, for No Easy Path (PRO-012)

    // Unused, backed the removed Replay Master.
    u8  playthroughConfigsSeen[5];
    u8  playthroughConfigsSeenCount;

    // Lifetime counters. GAME_STAT_* values live in SaveBlock1, which ClearSav1
    // zeroes at every new game; these never reset. The game stats themselves are
    // still incremented at their existing call sites.
    u16 trainerBattlesLifetime;   // Trainer Trouncer .. Unstoppable Force
    u16 wildBattlesLifetime;      // Into the Wild .. One with the Wild
    u16 eggsHatchedLifetime;      // It's Hatching! .. Egg Factory, and Egg Marathon
    u16 hiddenItemsFoundLifetime; // Treasure Hunter / Treasure Hoard
    u16 npcsTalkedToLifetime;     // Talk to the Locals / People Person
    u16 shopPurchasesLifetime;    // First Purchase / Regular Customer
    u32 moneySpentLifetime;       // Big Spender / Whale

    // Set the first time boosts are unlocked on this profile; cleared when the
    // bedroom message announcing them is shown.
    bool8 boostsUnlockNoticePending;

    u8  reserved[7];              // forward compatibility (was 11, then 8)
};

extern struct AchievementProfile gAchievementProfile;

// A profile read/write failure must never trigger the save-failed screen
// (that's gDamagedSaveSectors/gSaveFileStatus's job).
bool8 Achievement_ProfileWriteFailed(void);

// Writes the profile to flash only if dirty. Safe to call unconditionally.
void Achievement_FlushProfile(void);

// Public API. Nothing outside src/achievements.c writes the profile.

// Loads the profile from flash. Call once at boot, before any save is loaded.
void  Achievement_Init(void);
bool8 Achievement_IsCompleted(u16 achievementId);

// Whether achievementId can still be earned on the CURRENT save. FALSE for an
// achievement gated on a game-mode toggle (Nuzlocke/Randomizer/Mono Type/
// Mono Gen/Limited Party/Draft/Recruits/Rotation/HARD difficulty) that isn't
// active, or whose "never do X" condition is already violated. Doesn't consider
// prior completion; test Achievement_IsCompleted first. Used by the Achievements
// Menu to grey out titles the player can no longer earn.
bool8 Achievement_IsEligible(u16 achievementId);

// Returns the ACHIEVEMENT_NONE entry for an out-of-range ID rather than NULL.
// gAchievements[] is only included from src/achievements.c.
const struct Achievement *Achievement_GetInfo(u16 achievementId);

u32   Achievement_GetTotalPoints(void);
u32   Achievement_GetAvailablePoints(void);

// Loop gAchievements[] each call; not cached.
u32   Achievement_GetCompletedCount(void);
u32   Achievement_GetCompletedCountInTier(enum AchievementTier tier);
u32   Achievement_GetTotalCountInTier(enum AchievementTier tier);

bool8 Achievement_BoostsUnlocked(void);

// Script specials for the post-Hall of Fame bedroom message announcing the boost
// unlock. GetBoostsUnlockNoticePending returns TRUE only between the first boost
// unlock on this profile (Achievement_OnFirstPlaythroughComplete) and the message
// being shown. ClearBoostsUnlockNotice clears it and flushes immediately.
u16   GetBoostsUnlockNoticePending(void);
void  ClearBoostsUnlockNotice(void);
bool8 Achievement_BoostsEnabled(void);
void  Achievement_SetBoostsEnabled(bool8 enabled);

u8    AchievementBoost_GetLevel(u16 boostId);

// The level a boost's effect is currently dialed to, between 0 and the purchased
// level (AchievementBoost_GetLevel). Every AchievementBoost_Apply*/Get*Percent/Has*
// effect function reads this, so dialing back takes effect immediately without
// touching what was bought. Purchasing a new level comes in already active.
u8    AchievementBoost_GetActiveLevel(u16 boostId);

// Moves the active level by delta, clamped to [0, purchased level]. Returns FALSE
// with no state change if delta would leave that range. The boost menu's dpad L/R
// calls it with -1/+1; its [A] toggle on an owned binary boost flips between 0 and 1.
bool8 AchievementBoost_TryChangeActiveLevel(u16 boostId, s8 delta);

// Returns the BOOST_NONE entry for an out-of-range ID rather than NULL.
const struct AchievementBoost *AchievementBoost_GetInfo(u16 boostId);

// Checks run eligibility and prior completion in order, then commits the flag and
// points together before queuing a notification. Returns FALSE without side
// effects if the run is ineligible, the ID is out of range, or already completed.
bool8 Achievement_TryComplete(u16 achievementId);

// The one-time first-playthrough unlock. Call from GameClear()
// (src/post_battle_event_funcs.c) in the branch that only runs the first time
// FLAG_SYS_GAME_CLEAR is set for this save; that flag's set-once semantics
// guarantee this runs once per playthrough, so it has no completion guard.
//
// Also handles Chaos Begins/Truly Random/Pure Chaos/Species-Type-Move Chaos
// (all read FlagGet(FLAG_RANDOMIZE_*)/gSaveBlock1Ptr->difficulty at this moment).
void Achievement_OnFirstPlaythroughComplete(void);

// Called from NewGameInitData (src/new_game.c) right after
// gSaveBlock2Ptr->newGamePlus++, so `cycle` is the just-started cycle's number.
// highestNgPlusCycle is a high-water mark, so this only ever grows it.
//
// Also checks Chaos Begins (TryComplete's guard makes the duplicate check with
// Achievement_OnFirstPlaythroughComplete's cycle-0 case harmless). Also zeroes
// trainersDefeatedThisCycle (AchievementRunDataExt); SaveBlock1 has little slack
// left, so it lives in SaveBlock2, which ClearSav1 can't reset, and this call is
// its reset point.
void Achievement_OnNewGamePlusStarted(u8 cycle);

// Called from the same GameClear() branch as Achievement_OnFirstPlaythroughComplete,
// additionally gated on gSaveBlock2Ptr->newGamePlus > 0. FLAG_SYS_GAME_CLEAR isn't
// preserved across New Game+ (see NewGameInitData), so that branch re-runs on every
// NG+ cycle's clear. This counts the subset of clears during an NG+ loop, distinct
// from playthroughsCompleted.
//
// Checks ACHIEVEMENT_NG_PLUS_CYCLE_COMPLETE unconditionally, and every "complete an
// NG+ cycle with X" entry: Cycle Specialist (Achievement_CountChallengeModifiers)
// and Cycle Nuzlocke (nuzlockeModeEnabled).
void Achievement_OnNewGamePlusCycleCompleted(void);

// Validates in order -- boosts unlocked, current level < maxLevel,
// availablePoints >= costs[level] -- refusing at the first failure. Purchase
// re-runs the same checks before committing pointsInvested += cost,
// boostLevels[id]++, and flushing immediately.
// Deliberately NOT gated on gSaveBlock1Ptr->achievementsBlocked: that flag stops a
// run from *earning* achievements/points (enforced in Achievement_TryComplete),
// not from spending points already held.
bool8 AchievementBoost_CanPurchase(u16 boostId);
bool8 AchievementBoost_Purchase(u16 boostId);

// Non-mutating query so the boost shop can gate its confirmation prompt without
// spending the fee. Refuses in order: boosts not unlocked, nothing invested
// (a reset would cost the fee for zero refund), can't afford the fee. Like
// CanPurchase, not gated on achievementsBlocked: a reset only refunds/clears
// what's already there.
bool8 AchievementBoost_CanReset(void);

// Re-runs AchievementBoost_CanReset before committing pointsInvested = 0, zeroing
// every boostLevels[] entry, and deducting ACHIEVEMENT_BOOST_RESET_FEE. The refund
// is exactly the pointsInvested that was there, so a reset can never generate points.
bool8 AchievementBoost_Reset(void);

// Wraps a raw exp value. Call on the pre soft-level-cap amount, not the post-cap
// result, so a purchased boost is still throttled by the level cap. Returns
// expValue unchanged when boosts are disabled or BOOST_EXP_GAIN is at level 0.
u32 AchievementBoost_ApplyExp(u32 expValue);

// The remaining numerical boosts follow AchievementBoost_ApplyExp's shape: a no-op
// when boosts are disabled or the boost is at level 0.

// ComputePlayerShinyOdds (src/pokemon.c) adds this to totalRerolls before its
// reroll loop, alongside the Shiny Charm/Lure/chain-fishing/DexNav rerolls.
// Not a flat probability multiplier.
u32 AchievementBoost_ExtraShinyRerolls(void);

// ComputeCaptureOdds (src/battle_script_commands.c) applies this to its final 0-255
// odds, after the ball.guaranteedCapture (Master Ball) early return. A boosted
// value can still cross the call site's odds > 254 "guaranteed" threshold, so no
// clamp is needed here.
u32 AchievementBoost_ApplyCatchOdds(u32 odds);

// Cmd_getmoneyreward (src/battle_script_commands.c) applies this to the combined
// trainer money reward on a win, before AddMoney. AddMoney's MAX_MONEY clamp is
// the only clamp needed.
u32 AchievementBoost_ApplyMoneyReward(u32 money);

// TryProduceOrHatchEgg (src/daycare.c) applies this to GetEggCyclesToSubtract's
// result (src/egg_hatch.c). A flat addition, like Magma Armor/Flame Body/Steam
// Engine doubling the base value from 1 to 2.
u8 AchievementBoost_ApplyEggCyclesToSubtract(u8 toSub);

// CalculateFriendshipBonuses (src/pokemon.c) applies this to its final bonus.
// Only scales positive gains; a negative bonus (fainting, bitter herb) passes
// through unchanged.
s32 AchievementBoost_ApplyFriendshipGain(s32 bonus);

// RoamerMove (src/roamer.c) rolls this once per move (called on every map
// transition): a flat 1% chance per level (maxLevel 5) that a roamer skips its
// random relocation and is placed on the player's current route. Only fires on a
// route the roamer table covers. Doesn't make an absent roamer more likely to exist.
bool8 AchievementBoost_ShouldRoamerSeekPlayer(void);

// The three Get*Percent functions return a raw 0-100 percent instead of rolling
// internally, because their call sites are in battle, where randomness must go
// through the tagged RandomChance/RandomPercentage helpers to keep the test harness
// and recorded-battle playback deterministic. They return 0 on the baseline path
// so the caller can skip its roll and consume no RNG. All three are also gated at
// their call sites on IsOnPlayerSide() and on the battle not being link/recorded --
// boost levels differ between players, so an ungated roll would desync a link battle.

// IsCriticalHit (src/battle_util.c): a flat extra chance to upgrade a hit the
// normal crit-stage roll declined. Applied after the CRITICAL_HIT_BLOCKED check
// (Battle Armor/Shell Armor/Lucky Chant still block) and before the
// gPartyCriticalHits counter, so a boosted crit counts toward IF_CRITICAL_HITS_GE.
u32 AchievementBoost_GetCritChancePercent(void);

// CancelerPPDeduction (src/battle_move_resolution.c): a flat chance that a move's
// PP cost (including Pressure) is skipped. The canceler's early-outs (multi-turn
// moves, Dancer, bounced, snatched, Bide, Struggle) already return before this.
u32 AchievementBoost_GetPpSavePercent(void);

// ENDTURN_STATUS_RECOVERY (src/battle_end_turn.c): rolled once per turn per living
// battler as a flat chance to clear a non-volatile status, using Shed Skin's cure
// sequence. Runs after the poison/burn/frostbite handlers, so status damage ticks first.
u32 AchievementBoost_GetStatusRecoveryPercent(void);

// GetBerryCountByBerryTreeId (src/berry.c): flat extra berries per harvest. Hooks
// the read rather than the saved berryYield, so switching the boost off restores
// baseline immediately. Returns 0 unchanged, so an empty tree stays empty.
u8 AchievementBoost_ApplyBerryYield(u8 count);

// BerryTreeTimeUpdate and PlantBerryTree (src/berry.c): shortens the wait for a
// tree's next growth stage. Deliberately NOT applied to the BERRY_STAGE_BERRIES
// window or the unattended-tree death threshold; shortening either would be a nerf.
u16 AchievementBoost_ApplyBerryStageDuration(u16 minutes);

// Every VAR_REPEL_STEP_COUNT write site (src/item_use.c, src/sprays.c). Clamped
// below REPEL_LURE_MASK so a boosted count can't bleed into the Lure flag bit.
u16 AchievementBoost_ApplySprayStepCount(u16 steps);

// The three binary boosts: "purchased" is the entire effect.

// CB2_EndWildBattle (src/battle_setup.c): in nuzlocke mode, an encounter not
// converted into a catch spends a one-time per-route free pass instead of locking
// the route. Catching still locks it immediately.
bool8 AchievementBoost_HasNuzlockeSecondChance(void);

// NewGameInitData (src/new_game.c): grants starting items and extra money on a
// fresh game. Never applies to New Game+, which restores the previous bag and money.
bool8 AchievementBoost_HasStarterKit(void);

// GenerateIVs (src/ui_birch_case.c): the starter rolls 31 in every stat. Hooked at
// generation so the Birch Case preview and the received Pokemon can't disagree.
bool8 AchievementBoost_HasPerfectStarterIvs(void);

// NewGameInitData (src/new_game.c): grants the Shiny Charm/Ability Capsule/Ability
// Patch to a fresh game's key items. Never applies to New Game+.
bool8 AchievementBoost_HasShinyCharmStart(void);
bool8 AchievementBoost_HasAbilityCapsuleStart(void);
bool8 AchievementBoost_HasAbilityPatchStart(void);

// Every genuine "use a consumable" RemoveBagItem call site (src/item_use.c,
// src/party_menu.c), NOT the sell/give/discard sites in src/item_menu.c. Rolls a
// flat percent chance (BOOST_CONSUMABLE_SAVE) that the item is spared. Returns TRUE
// (consume normally) for anything outside POCKET_ITEMS, when boosts are disabled,
// the boost is at level 0, or the roll fails. Callers still do the RemoveBagItem.
bool8 AchievementBoost_ShouldConsumeItem(enum Item itemId);

// SetInitialEggData (src/daycare.c): rerolls the egg's random IV spread
// BOOST_EGG_IV_REROLL's level worth of extra times, keeping the highest stat
// total. Runs before InheritIVs, so it only improves the IVs that would've been
// random anyway.
void AchievementBoost_ApplyEggIvReroll(struct Pokemon *mon);

// CreateWildMon (src/wild_encounter.c): same reroll-and-keep-best, using
// BOOST_WILD_IV_REROLL.
void AchievementBoost_ApplyWildIvReroll(struct Pokemon *mon);

// Every price computed for a Poke Mart purchase (src/shop.c), both the displayed
// price and the confirmed cost, so a discount is never display-only. Also applied
// to decoration prices. Knocks a flat percent off via BOOST_SHOP_DISCOUNT.
u32 AchievementBoost_ApplyShopPrice(u32 price);

// GetAdjustedDamage (src/battle_util.c): a flat extra chance, on top of Sturdy/
// Focus Band/Focus Sash/Affection, that a hit which would KO a player-side Pokemon
// leaves it at 1 HP. Raw percent, same reasoning as the Get*Percent battle boosts.
u32 AchievementBoost_GetSurviveChancePercent(void);

// CB2_EndTrainerBattle (src/battle_setup.c), win branches only: restores
// BOOST_POST_BATTLE_HEAL's percent of max HP to every living, non-egg party
// Pokemon.
void AchievementBoost_ApplyPostBattleHeal(void);

// Catalog hook functions. Each checks one category's thresholds against
// Achievement_TryComplete, which is idempotent, so every one is safe to call
// unconditionally every time its call site runs.

// Common_EventScript_CheckLevelCapIncrease (data/scripts/level_cap.inc), via a
// callnative: loops a static {flag, achievementId} table covering all 8 badges and
// the 7 non-badge story beats that funnel through this script. Every call site sets
// its own milestone's flag immediately before, so checking all 15 flags is correct.
// Also checks Who Needs Centers? at the FLAG_BADGE05_GET checkpoint.
void Achievement_CheckStoryMilestones(void);

// HandleSetPokedexFlag (src/pokemon.c), inside its "not already set" guard.
// caught should be TRUE only when caseId was FLAG_SET_CAUGHT, so a newly-seen entry
// only checks the seen thresholds. Uses GetNationalPokedexCount as a percentage of
// NATIONAL_DEX_COUNT so thresholds hold for any expansion configuration. The caught
// branch only checks ACHIEVEMENT_CATCH_ALL; the smaller caught thresholds are in
// Achievement_CheckCaptureMilestones.
void Achievement_CheckPokedexMilestones(bool8 caught);

// GiveCapturedMonToPlayer (src/pokemon.c): reads GAME_STAT_POKEMON_CAPTURES, already
// incremented for the current catch (incrementgamestat precedes givecaughtmon in
// data/battle_scripts_2.s). A hard-number ladder (100/350/700); the top tier,
// ACHIEVEMENT_CATCH_ALL, is a distinct-species check in Achievement_CheckPokedexMilestones.
void Achievement_CheckCaptureMilestones(void);

// GiveCapturedMonToPlayer, only when the mon is shiny (MON_DATA_IS_SHINY).
// Increments gAchievementProfile.shiniesObtained, flushes, then checks the three
// shiny-count thresholds. It also fires for scripted gift mons, hence "obtained"
// in the catalog text.
void Achievement_OnShinyObtained(void);

// CB2_EndTrainerBattle (src/battle_setup.c), called unconditionally at the top
// regardless of outcome. Uses gAchievementProfile.trainerBattlesLifetime so
// progress counts across every playthrough.
void Achievement_CheckTrainerBattleMilestones(void);

// CB2_EndWildBattle (src/battle_setup.c). Uses gAchievementProfile.wildBattlesLifetime.
void Achievement_CheckWildBattleMilestones(void);

// AddBagItem (src/item.c): one-off "obtain this specific item" checks, gated by the
// caller on the add having succeeded.
void Achievement_CheckItemMilestones(enum Item itemId);

// AddMoney (src/money.c), called with the post-clamp balance (GetMoney after
// SetMoney); the raw amount would under-count near MAX_MONEY.
void Achievement_CheckMoneyMilestones(u32 money);

// Task_EggHatch (src/egg_hatch.c), right after AddHatchedMonToParty, with whether
// the hatched mon is shiny. Count thresholds use gAchievementProfile.eggsHatchedLifetime
// so progress counts across every playthrough.
void Achievement_CheckEggMilestones(bool8 isShiny);

// Battle-tracking infrastructure. See src/achievements.c for struct
// AchievementBattleData and the "one entry point" discipline: battle-side code
// never calls Achievement_TryComplete directly for a category K entry.

// GetTrainerClassFromId(TRAINER_BATTLE_PARAM.opponentA) against the "boss" trainer
// classes: gym leaders, Elite Four, Champion, rival, Team Aqua/Magma leaders.
// FALSE outside a trainer battle.
bool8 Achievement_IsMajorBattle(void);

// BattleStartClearSetData (src/battle_main.c), right after gBattleResults is zeroed:
// resets the per-battle tracking struct. EWRAM only, never saved.
void Achievement_ClearBattleData(void);

// CancelerPPDeduction (src/battle_move_resolution.c), the same funnel BOOST_PP_SAVER
// hooks; every early-out above that call means this only fires for a move actually
// being used. Player side only, never link/recorded (checked by the caller).
// typeBit is 1u << GetMoveType(move), computed at the call site. movePosition >=
// MAX_MON_MOVES is tolerated (only the per-slot move-variety bit is skipped) since
// Metronome/Z-move/Max Move paths can reach this with an unusual position.
void Achievement_RecordMoveUsed(u8 partyIndex, enum Move move, u32 typeBit, u32 movePosition, bool8 isSTAB, bool8 isSetupMove);

// CalcTypeEffectivenessMultiplier (src/battle_util.c), gated by the caller on
// ctx->updateFlags (a real hit, not an AI damage estimate).
void Achievement_RecordSuperEffectiveHit(void);

// IsCriticalHit (src/battle_util.c), the same funnel and gating as BOOST_CRIT_CHANCE.
void Achievement_RecordCriticalHit(void);

// SetNonVolatileStatus (src/battle_script_commands.c), gated by the caller on the
// status landing on an opponent from a player-side move. statusBit is one of the
// ACHIEVEMENT_STATUS_BIT_* constants.
void Achievement_RecordStatusInflicted(u8 statusBit);

// SetValuesOnFaint (src/battle_util.c)'s opponent-faint branch, gated by the caller
// (never link/recorded). attackerBattler is gBattlerAttacker at that moment. The two
// battlers are equal for a self-inflicted/passive cause (status tick, confusion,
// recoil, Life Orb, ...); this reads the still-intact status1 on victimBattler to
// decide whether that counts as a status KO instead of crediting a party slot.
void Achievement_RecordOpposingFaint(enum BattlerId victimBattler, enum BattlerId attackerBattler);

// HandleEndTurn_BattleWon (src/battle_main.c): evaluates every category K entry
// against the battle that just ended. The caller gates this on not being link or
// recorded: gBattleResults (unlike AchievementBattleData) is maintained
// unconditionally by the vanilla engine, so a recorded-battle replay could otherwise
// satisfy a gBattleResults-derived entry (e.g. Clean Sweep) from stale data.
void Achievement_CheckBattleMilestones(void);

// ---- Team Building & Composition (category L) -------------------------
//
// The first user of struct AchievementRunData (include/global.h). Each call site
// reuses an existing hook:
//   Achievement_CheckTeamMilestones          same site as Achievement_CheckBattleMilestones
//   Achievement_CheckPartyStateMilestones    same callnative as Achievement_CheckStoryMilestones
//   Achievement_CheckTeamCompletionMilestones GameClear (src/post_battle_event_funcs.c)

// GetTrainerClassFromId(TRAINER_BATTLE_PARAM.opponentA) == TRAINER_CLASS_LEADER:
// the subset of Achievement_IsMajorBattle() that's an actual Gym battle.
bool8 Achievement_IsGymBattle(void);

// HandleEndTurn_BattleWon (src/battle_main.c), immediately after
// Achievement_CheckBattleMilestones, gated the same way. Reads
// AchievementBattleData.slotsThatActed/lastThreeKoSlots alongside the live party.
void Achievement_CheckTeamMilestones(void);

// Called at the tail of Achievement_CheckStoryMilestones. Checks party state not tied
// to a specific battle (held items, level-cap standing).
void Achievement_CheckPartyStateMilestones(void);

// GameClear (src/post_battle_event_funcs.c), alongside
// Achievement_OnFirstPlaythroughComplete. Fires once per completed run, including
// every New Game+ cycle (see Achievement_OnNewGamePlusCycleCompleted).
void Achievement_CheckTeamCompletionMilestones(void);

// GiveCapturedMonToPlayer (src/pokemon.c) and Task_EggHatch (src/egg_hatch.c), the
// same call sites as Achievement_CheckCaptureMilestones/Achievement_CheckEggMilestones.
// Gift and traded-in Pokemon aren't tracked (no single funnel exists for them), so
// Fresh Start undercounts rather than overcounts, the safer direction to be wrong.
//
// Also checks One of Each: obtaining a Pokemon is the only thing that can raise the
// distinct-species count. The same gift/trade gap applies: a gift or traded-in
// Pokemon that takes the player to 10 distinct species won't award it until their
// next catch or hatch.
void Achievement_RecordMonObtained(u32 personality);

// ---- Exploration, Economy & Collection (category M) --------------------

// LoadCurrentMapData (src/overworld.c), the one function all three map-transition
// paths funnel through (see its own comment). Updates AchievementRunData.mapsVisited
// and checks every map-count/FLAG_VISITED_*/No Loose Ends threshold.
void Achievement_CheckExplorationMilestones(void);

// Achievement_CheckPokedexMilestones's FLAG_SET_SEEN branch (category B). Checked
// only when a species is newly seen, so Local Expert completes at the exact moment
// the last species on the player's current route becomes seen.
void Achievement_CheckLocalExpert(void);

// SetHiddenItemFlag (src/field_specials.c), the native the hidden-item script calls;
// the overworld gates the script on the flag not being set, so it's reached once per
// item. Uses gAchievementProfile.hiddenItemsFoundLifetime so progress counts across
// every playthrough.
void Achievement_CheckHiddenItemMilestones(void);

// GetInteractionScript's object-event branch (src/field_control_avatar.c): fires once
// per NPC interaction started, not once per msgbox in the script. Uses
// gAchievementProfile.npcsTalkedToLifetime so progress counts across every playthrough.
void Achievement_RecordNpcTalkedTo(void);

// BuyMenuSubtractMoney (src/shop.c), right after IncrementGameStat(GAME_STAT_SHOPPED);
// amountSpent is sShopData->totalCost. Checks First Purchase/Regular Customer and Big
// Spender/Whale, and sets AchievementRunData.shoppedSinceLastGym for
// Achievement_CheckGymEconomyMilestones. Those thresholds use
// gAchievementProfile.shopPurchasesLifetime/moneySpentLifetime so progress counts
// across every playthrough; GAME_STAT_MONEY_SPENT is still updated.
void Achievement_RecordMoneySpent(u32 amountSpent);

// The sell-item AddMoney call in src/item_menu.c. Separate from
// Achievement_RecordMoneySpent because Treasure Pays tracks proceeds, not spending,
// and must not be confused with it or with Achievement_CheckMoneyMilestones's
// held-balance checks.
void Achievement_RecordItemSaleProceeds(u32 amount);

// AddBagItem (src/item.c), behind the same "add succeeded" guard as
// Achievement_CheckItemMilestones. Scans every non-key-item Bag pocket for Pack Rat.
void Achievement_CheckPackRatMilestone(void);

// ObjectEventInteractionPickBerryTree (src/berry.c): one harvest action, same
// "count the action, not the yield" convention as GAME_STAT_PLANTED_BERRIES.
void Achievement_RecordBerryHarvest(void);

// Both GAME_STAT_EVOLVED_POKEMON sites (src/evolution_scene.c): checks Evolutionary
// Path/Evolution Expert against the count vanilla already incremented.
void Achievement_CheckEvolutionCountMilestones(void);

// GetEvolutionTargetSpecies's EVO_MODE_NORMAL/EVO_MODE_BATTLE_ONLY case
// (src/pokemon.c), gated by the caller on evoState == DO_EVO; CHECK_EVO runs first
// and must never award anything, or eligibility checks alone (e.g. opening the
// party menu) would complete this. Fires when the evolution about to happen matched
// via an IF_MIN_FRIENDSHIP condition.
void Achievement_RecordFriendshipEvolution(void);

// PokemonUseItemEffects's ITEM4_EVO_STONE case (src/pokemon.c), gated by the caller
// on the item being one of the twelve stone items (not every EVO_ITEM item is a
// stone in every expansion configuration).
void Achievement_RecordStoneEvolution(void);

// GiveCapturedMonToPlayer (src/pokemon.c), alongside Achievement_RecordMonObtained.
// gDexNavSpecies is nonzero only during a battle a DexNav scan started, so this
// can't fire for an unrelated catch or a gift mon.
void Achievement_CheckDexNavCaptureMilestone(void);

// The GAME_STAT_FISHING_ENCOUNTERS increment in src/wild_encounter.c. Angler's
// threshold check against the count vanilla already incremented.
void Achievement_CheckFishingMilestone(void);

// HandleEndTurn_BattleWon (src/battle_main.c), immediately after
// Achievement_CheckTeamMilestones, gated the same way. Branches internally on
// Achievement_IsGymBattle()/Achievement_IsMajorBattle().
void Achievement_CheckGymEconomyMilestones(void);

// GameClear (src/post_battle_event_funcs.c), alongside
// Achievement_CheckTeamCompletionMilestones. Investor's "finish the story holding
// >= 500000" check.
void Achievement_CheckEconomyCompletionMilestones(void);

// ---- Challenge Runs & Nuzlocke (category N) ----------------------------
//
// Each call site reuses an existing hook; see include/constants/achievements.h's
// category N comment for the overview.

// HandleEndTurn_BattleWon (src/battle_main.c), immediately after
// Achievement_CheckGymEconomyMilestones, gated the same way. Covers every
// Challenge-category entry evaluated battle-by-battle (No Healing Items/Itemless
// Battle/Set in Stone/Minimalist/Level Discipline), plus the running high-water mark
// (highestPartySizeThisRun) and starter-tracking bookkeeping that
// Achievement_CheckChallengeCompletionMilestones reads at GameClear.
//
// Randomizer & New Game+ entries ride this call site: Random by Nature (Gym clear),
// Chaos Team/Never Seen It Coming/Patchwork Team (major battle win), and trainer-win
// bookkeeping (AchievementRunDataExt.trainersDefeatedThisCycle/
// gAchievementProfile.trainersDefeatedAcrossNgPlus for Fresh Faces/Never the Same Fight).
void Achievement_CheckChallengeMilestones(void);

// Same call site, immediately after. Every entry is additionally gated on
// gSaveBlock1Ptr->nuzlockeModeEnabled: First Nuzlocke, Close Call, Scrappy, No Ace
// Allowed. Self-contained; doesn't reuse Achievement_CheckTeamMilestones's locals.
void Achievement_CheckNuzlockeMilestones(void);

// GameClear (src/post_battle_event_funcs.c), alongside
// Achievement_CheckTeamCompletionMilestones/Achievement_CheckEconomyCompletionMilestones,
// with the same re-runs-every-NG+-cycle gating. Covers every "complete the story"
// Challenge-category entry: Self-Imposed/Nightmare Mode
// (Achievement_CountChallengeModifiers), No Shopping Run, No Centers, Hardcore Set,
// Solo Journey, No Freebies.
void Achievement_CheckChallengeCompletionMilestones(void);

// Same call site as above. Every entry is gated on nuzlockeModeEnabled: Perfect
// Nuzlocke/The Graveyard, Species Clause (scans party + every PC box, which under
// Nuzlocke rules is exactly the set caught this run), No Second Chances, Full
// Encounter, Unassisted Survivor.
//
// Also Nuzlocke Across Worlds/Chaos Survivor: a completed Nuzlocke run with any
// FLAG_RANDOMIZE_* flag set (plus HARD difficulty for the latter).
void Achievement_CheckNuzlockeCompletionMilestones(void);

// BuyMenuSubtractMoney (src/shop.c), alongside Achievement_RecordMoneySpent; called
// only when the purchased item is in POCKET_ITEMS. Sets
// AchievementRunData.boughtConsumableItem for No Shopping Run.
void Achievement_RecordConsumableItemPurchase(void);

// RemoveFaintedMonsFromParty (src/overworld.c), the single function every Nuzlocke
// fainted-mon removal funnels through; called once per Pokemon removed. Increments
// AchievementRunData.nuzlockeMonsLost for Perfect Nuzlocke/The Graveyard.
void Achievement_RecordNuzlockeMonLost(void);

// ui_birch_case.c, right after the starter is granted (ScriptGiveMonParameterized).
// Records its personality (survives evolution, unlike species) so
// Achievement_CheckChallengeMilestones can tell whether it ever acts in a major
// battle, for No Freebies.
void Achievement_RecordStarterPersonality(u32 personality);

// ---- Randomizer & New Game+ ---------------------------------------------
//
// GiveCapturedMonToPlayer (src/pokemon.c), alongside Achievement_CheckCaptureMilestones.
// Randomized Rookie: GAME_STAT_POKEMON_CAPTURES (already incremented; this-run count,
// see ResetGameStats in src/overworld.c) >= 25 while any FLAG_RANDOMIZE_* flag is set.
void Achievement_CheckRandomizerCaptureMilestone(void);

// ---- Streaks, Records & Collection Remainder (category P) --------------
// See include/constants/achievements.h's category P comment for the breakdown and
// src/achievements.c for each function's own doc comment.

// HandleEndTurn_BattleWon (src/battle_main.c), immediately after
// Achievement_CheckNuzlockeMilestones, gated the same way. Covers every entry
// evaluated battle-by-battle: the trainer win streak and Gym/League streaks,
// per-slot KO totals, Oddball and Underestimated.
void Achievement_CheckBattleRecordsMilestones(void);

// LoadCurrentMapData (src/overworld.c), alongside Achievement_CheckExplorationMilestones.
// Map transitions are frequent enough to catch these live-state thresholds, which
// aren't tied to a specific event: Growing Strong, Century Club/Full Century,
// Box Filler/Storage Baron, Devoted/Inseparable.
//
// One of Each lives in Achievement_RecordMonObtained instead: it walks storage,
// which is too expensive to re-run on every map transition.
void Achievement_CheckRecordsMilestones(void);

// GameClear (src/post_battle_event_funcs.c), alongside
// Achievement_CheckNuzlockeCompletionMilestones. Legend of the Run reads
// AchievementRunDataExt.presentAtEveryMajorBattleSlots, which is only meaningful
// once a completed run's major battles are all in.
void Achievement_CheckRecordsCompletionMilestones(void);

// RemoveFaintedMonsFromParty (src/overworld.c) and FldEff_PokecenterHeal
// (src/field_effect.c), the same two IsPartyEmpty()-gated sites as the Nuzlocke wipe
// detection (see Achievement_CheckNuzlockeMilestones). Mirrors the run's win-streak
// high-water mark into gAchievementProfile.bestTrainerWinStreakEver, then zeroes the
// streak counters this wipe broke.
void Achievement_RecordPartyWipe(void);

// HandleSetPokedexFlag (src/pokemon.c)'s FLAG_SET_CAUGHT branch, alongside
// Achievement_CheckPokedexMilestones. Family Reunion: species is the one just newly
// caught; walks its evolution family (both directions) and completes if every stage
// is also caught.
void Achievement_CheckFamilyMilestone(enum Species species);

// HandleSetPokedexFlagBySpecies (src/pokemon.c)'s FLAG_SET_CAUGHT branch, alongside
// Achievement_CheckFamilyMilestone. Legendary Collection (category Z). On a newly
// caught counted legendary (restricted legendary / sub-legendary / mythical; Ultra
// Beasts and Paradox excluded) whose evolution family had no caught member before,
// increments the per-save distinct-family count
// (AchievementRunDataExt.legendaryFamiliesCaught) and re-evaluates the seven
// category Z entries.
void Achievement_CheckLegendaryMilestones(enum Species species);

// LoadCurrentMapData (src/overworld.c), alongside Achievement_CheckExplorationMilestones.
// One-shot recompute of legendaryFamiliesCaught from the caught Pokedex flags for a
// save made before category Z existed; guarded by
// AchievementRunDataExt.legendaryCountBackfilled.
void Achievement_BackfillLegendaryFamilies(void);

// Task_LearnedMove (src/party_menu.c), gated by the caller on move[1] == 0 (the TM/HM
// item-use path, not the move relearner or a tutor NPC) and on the item being a TM
// rather than an HM. Move Tutor (backfill).
void Achievement_RecordTMTaught(void);

// FldEff_PokecenterHeal (src/field_effect.c), right after
// IncrementGameStat(GAME_STAT_USED_POKECENTER). Nurse's Nightmare (backfill).
void Achievement_CheckPokecenterMilestone(void);

// ---- Profile Meta, Mastery & Prestige (category Q) ----------------------
// No declarations: every entry is a meta-achievement over state the system already
// exposes (Achievement_IsCompleted, gAchievements[].category/.tier, the profile
// fields above). Checked from within src/achievements.c: the tail of
// Achievement_TryComplete (alongside Achievement_CheckPointMilestones) and
// AchievementBoost_Purchase/_Reset (the only places boostLevels[]/pointsInvested/
// boostResets change). See include/constants/achievements.h's category Q comment.

// ---- Recruits/Limited Party/Draft/Rotation/Mono Type/Mono Gen -----------

// HandleEndTurn_BattleWon (src/battle_main.c), alongside
// Achievement_CheckBattleRecordsMilestones/Recruits_TallyParticipants. Every entry
// evaluated on a trainer-battle win for these six modes: Fresh Recruits (Recruits),
// Tight Squad/No Room to Spare (Limited Party), Spin the Wheel/On a Rotation/Gym
// Leader Roulette (Rotation).
void Achievement_CheckNewModeBattleMilestones(void);

// GameClear (src/post_battle_event_funcs.c), alongside every other completion check.
// Every "complete the story with X" entry for these six modes, plus the Cross-Mode
// stacking entries (Mode Collector/Kitchen Sink/The Full Stack).
void Achievement_CheckNewModeCompletionMilestones(void);

// Recruits_DoRetirement (src/recruits_mode.c). Honorable Discharge unconditionally,
// plus the Revolving Door/Full Turnover retirement-count ladder.
void Achievement_RecordRecruitRetirement(void);

// Draft_MarkAreaSpent (src/draft_mode.c), only in the branch that resolved a real
// draft pick (not a gift/egg or a no-offer area). First Pick unconditionally, plus
// the drafts-completed ladder (The Case is Closed/Full Case Clear).
void Achievement_RecordDraftCompleted(void);

// Draft_DoReplacement (src/draft_mode.c). Tough Call.
void Achievement_RecordDraftReplacement(void);

// BirchCase_GiveMon (src/ui_birch_case.c), alongside Achievement_RecordStarterPersonality;
// the normal (non-Draft) starter grant. Committed to the Bit/Generation Loyalist.
void Achievement_CheckMonoStarterMilestones(void);

// GiveCapturedMonToPlayer (src/pokemon.c) and Task_EggHatch (src/egg_hatch.c),
// alongside Achievement_RecordMonObtained. Every mon obtainable in Mono Type/Mono
// Gen is already restricted to the chosen type/generation (or an unresolved gen-0
// species), so this just counts obtains while each mode is on: Perfect Fit/Gotta
// Catch Some of Them.
void Achievement_RecordMonoModeObtain(void);

// ---- Emporium Rewards (category Y) -------------------------------------

// EmporiumBufferRewardItem (src/battle_emporium.c), win branch only. rewardIndex is
// the global reward row index (VAR_EMPORIUM_REWARD, 0..EMPORIUM_REWARD_COUNT-1). Sets
// that row's bit in AchievementRunDataExt.emporiumRewardsWon[] (SaveBlock2), then
// evaluates the eight category Y entries by counting set bits within each Emporium's
// GetEmporiumRewardStart()/GetEmporiumRewardCount() range.
void Achievement_OnEmporiumRewardWon(u32 rewardIndex);

// Debug-only; src/debug.c is the only caller. These bypass the validation the real
// functions add (completion rules, boost costs/maxLevel, reset fee) by design.
void  Achievement_DebugSetCompleted(u16 achievementId, bool8 completed);
void  Achievement_DebugSetPoints(u32 amount);
void  Achievement_DebugSetBoostsUnlocked(bool8 unlocked);
void  AchievementBoost_DebugSetLevel(u16 boostId, u8 level);
void  AchievementBoost_DebugReset(void);
void  Achievement_DebugMarkPlaythroughComplete(void);

#endif // GUARD_ACHIEVEMENTS_H
