#ifndef GUARD_CONSTANTS_ACHIEVEMENTS_H
#define GUARD_CONSTANTS_ACHIEVEMENTS_H

// Real entries land here in Stage 2.2/2.3, keyed to designated initializers
// in src/data/achievements.h. ACHIEVEMENT_NONE is the reserved zero value.
//
// Stage 13 (design doc §15/§16, plan Stage 13): the first real catalog wave,
// 60 entries across ten categories, each derived from state that already
// exists (gameStats[], Pokedex flags, AchievementProfile counters, or a
// handful of event flags) -- see src/achievements.c for each category's hook
// function and include/achievements.h for the per-function doc comments.
//
//   A. ACHIEVEMENT_STORY_RIVAL_ROUTE103 .. ACHIEVEMENT_STORY_CHAMPION (15)
//      Badges/story milestones -- Achievement_CheckStoryMilestones,
//      callnative'd from Common_EventScript_CheckLevelCapIncrease.
//   B. ACHIEVEMENT_DEX_SEEN_10 .. ACHIEVEMENT_DEX_CAUGHT_100 (8)
//      Pokedex seen/caught percentage -- Achievement_CheckPokedexMilestones,
//      HandleSetPokedexFlag (src/pokemon.c).
//   C. ACHIEVEMENT_CATCH_1 .. ACHIEVEMENT_CATCH_500 (5)
//      Total capture count -- Achievement_CheckCaptureMilestones,
//      GiveCapturedMonToPlayer (src/pokemon.c).
//   D. ACHIEVEMENT_SHINY_1 .. ACHIEVEMENT_SHINY_25 (3)
//      shiniesObtained count -- Achievement_OnShinyObtained, also
//      GiveCapturedMonToPlayer.
//   E. ACHIEVEMENT_TRAINERS_10 .. ACHIEVEMENT_TRAINERS_500 (5)
//      Trainer battle count -- Achievement_CheckTrainerBattleMilestones,
//      CB2_EndTrainerBattle (src/battle_setup.c).
//   F. ACHIEVEMENT_WILD_BATTLES_50 .. ACHIEVEMENT_WILD_BATTLES_500 (3)
//      Wild battle count -- Achievement_CheckWildBattleMilestones,
//      CB2_EndWildBattle (src/battle_setup.c).
//   G. ACHIEVEMENT_ITEM_MASTER_BALL .. ACHIEVEMENT_ITEM_HEART_SCALE (4)
//      Obtain a specific item -- Achievement_CheckItemMilestones,
//      AddBagItem (src/item.c).
//   H. ACHIEVEMENT_MONEY_10K .. ACHIEVEMENT_MONEY_MAX (3)
//      Money held -- Achievement_CheckMoneyMilestones, AddMoney (src/money.c).
//   I. ACHIEVEMENT_EGG_1 .. ACHIEVEMENT_EGG_SHINY (4)
//      Hatched egg count/shiny -- Achievement_CheckEggMilestones,
//      Task_EggHatch (src/egg_hatch.c).
//   J. ACHIEVEMENT_PLAYTHROUGHS_2 .. ACHIEVEMENT_POINTS_1000 (10)
//      Multi-run/persistent-profile milestones -- checked from inside the
//      existing Achievement_OnFirstPlaythroughComplete /
//      Achievement_OnNewGamePlusStarted / Achievement_OnNewGamePlusCycleCompleted
//      wrapper functions (Stages 5/12); ACHIEVEMENT_POINTS_1000 is checked
//      from inside Achievement_TryComplete itself.
//
// Stage 15 (design doc catalog wave 2, plan Stage 15): the second catalog
// wave, the first to observe a battle rather than derive from state that
// already existed. Introduces enum AchievementCategory (below) -- every
// entry above is backfilled with one in src/data/achievements.h -- and
// struct AchievementBattleData (src/achievements.c), an EWRAM-only per-battle
// scratchpad, never saved.
//
//   K. ACHIEVEMENT_BATTLE_CRITICAL_SUCCESS .. ACHIEVEMENT_BATTLE_LAST_ONE_STANDING (30)
//      Battle Mastery -- Achievement_CheckBattleMilestones, called from
//      HandleEndTurn_BattleWon (src/battle_main.c).
//
// Stage 16 (design doc catalog wave 3, plan Stage 16): the third catalog
// wave, the first real user of struct AchievementRunData (include/global.h).
// Builds on Stage 15's Achievement_IsMajorBattle() and adds
// Achievement_IsGymBattle() (TRAINER_CLASS_LEADER specifically). Checked from
// three sites: Achievement_CheckTeamMilestones (same HandleEndTurn_BattleWon
// call site as category K), Achievement_CheckPartyStateMilestones (piggybacks
// on category A's existing Common_EventScript_CheckLevelCapIncrease
// callnative), and Achievement_CheckTeamCompletionMilestones (GameClear,
// src/post_battle_event_funcs.c).
//
//   L. ACHIEVEMENT_TEAM_MONO_TYPE_TRIAL .. ACHIEVEMENT_TEAM_ACE_ROTATION (30)
//      Team Building & Composition -- see src/achievements.c for the
//      per-entry hook-site breakdown.
enum AchievementId
{
    ACHIEVEMENT_NONE,

    // A. Badges & Story (15)
    ACHIEVEMENT_STORY_RIVAL_ROUTE103,
    ACHIEVEMENT_BADGE_STONE,
    ACHIEVEMENT_STORY_PETALBURG_WOODS,
    ACHIEVEMENT_BADGE_KNUCKLE,
    ACHIEVEMENT_BADGE_DYNAMO,
    ACHIEVEMENT_BADGE_HEAT,
    ACHIEVEMENT_BADGE_BALANCE,
    ACHIEVEMENT_STORY_AQUA_HIDEOUT,
    ACHIEVEMENT_STORY_MT_PYRE,
    ACHIEVEMENT_STORY_MAGMA_HIDEOUT,
    ACHIEVEMENT_BADGE_FEATHER,
    ACHIEVEMENT_STORY_SEAFLOOR_CAVERN,
    ACHIEVEMENT_BADGE_MIND,
    ACHIEVEMENT_BADGE_RAIN,
    ACHIEVEMENT_STORY_CHAMPION,

    // B. Pokedex (8)
    ACHIEVEMENT_DEX_SEEN_10,
    ACHIEVEMENT_DEX_SEEN_25,
    ACHIEVEMENT_DEX_SEEN_50,
    ACHIEVEMENT_DEX_SEEN_100,
    ACHIEVEMENT_DEX_CAUGHT_10,
    ACHIEVEMENT_DEX_CAUGHT_25,
    ACHIEVEMENT_DEX_CAUGHT_50,
    ACHIEVEMENT_DEX_CAUGHT_100,

    // C. Captures (5)
    ACHIEVEMENT_CATCH_1,
    ACHIEVEMENT_CATCH_25,
    ACHIEVEMENT_CATCH_100,
    ACHIEVEMENT_CATCH_250,
    ACHIEVEMENT_CATCH_500,

    // D. Shiny (3)
    ACHIEVEMENT_SHINY_1,
    ACHIEVEMENT_SHINY_5,
    ACHIEVEMENT_SHINY_25,

    // E. Trainers Defeated (5)
    ACHIEVEMENT_TRAINERS_10,
    ACHIEVEMENT_TRAINERS_50,
    ACHIEVEMENT_TRAINERS_150,
    ACHIEVEMENT_TRAINERS_300,
    ACHIEVEMENT_TRAINERS_500,

    // F. Wild Battles (3)
    ACHIEVEMENT_WILD_BATTLES_50,
    ACHIEVEMENT_WILD_BATTLES_250,
    ACHIEVEMENT_WILD_BATTLES_500,

    // G. Items (4)
    ACHIEVEMENT_ITEM_MASTER_BALL,
    ACHIEVEMENT_ITEM_RARE_CANDY,
    ACHIEVEMENT_ITEM_PP_UP,
    ACHIEVEMENT_ITEM_HEART_SCALE,

    // H. Money (3)
    ACHIEVEMENT_MONEY_10K,
    ACHIEVEMENT_MONEY_100K,
    ACHIEVEMENT_MONEY_MAX,

    // I. Eggs (4)
    ACHIEVEMENT_EGG_1,
    ACHIEVEMENT_EGG_10,
    ACHIEVEMENT_EGG_50,
    ACHIEVEMENT_EGG_SHINY,

    // J. Multi-Run / Persistent Profile (10)
    ACHIEVEMENT_PLAYTHROUGHS_2,
    ACHIEVEMENT_PLAYTHROUGHS_5,
    ACHIEVEMENT_NG_PLUS_STARTED,
    ACHIEVEMENT_NG_PLUS_CYCLE_3,
    ACHIEVEMENT_NG_PLUS_CYCLE_5,
    ACHIEVEMENT_NG_PLUS_COMPLETED_3,
    ACHIEVEMENT_NUZLOCKE_1,
    ACHIEVEMENT_NUZLOCKE_3,
    ACHIEVEMENT_RANDOMIZED_1,
    ACHIEVEMENT_POINTS_1000,

    // K. Battle Mastery (30)
    ACHIEVEMENT_BATTLE_CRITICAL_SUCCESS,
    ACHIEVEMENT_BATTLE_TYPE_ADVANTAGE,
    ACHIEVEMENT_BATTLE_TYPE_MASTER,
    ACHIEVEMENT_BATTLE_CLEAN_SWEEP,
    ACHIEVEMENT_BATTLE_PERFECT_SWEEP,
    ACHIEVEMENT_BATTLE_NO_DAMAGE,
    ACHIEVEMENT_BATTLE_UNTOUCHABLE,
    ACHIEVEMENT_BATTLE_STATUS_SPECIALIST,
    ACHIEVEMENT_BATTLE_STATUS_MASTER,
    ACHIEVEMENT_BATTLE_WEATHER_REPORT,
    ACHIEVEMENT_BATTLE_WEATHER_MASTER,
    ACHIEVEMENT_BATTLE_SETUP_SWEEP,
    ACHIEVEMENT_BATTLE_ONE_TURN_FINISH,
    ACHIEVEMENT_BATTLE_PRIORITY_MATTERS,
    ACHIEVEMENT_BATTLE_SPEED_DEMON,
    ACHIEVEMENT_BATTLE_ATTRITION,
    ACHIEVEMENT_BATTLE_STRATEGIC_VICTORY,
    ACHIEVEMENT_BATTLE_REVERSE_SWEEP,
    ACHIEVEMENT_BATTLE_CHAMPION_TACTICIAN,
    ACHIEVEMENT_BATTLE_MOVE_VARIETY,
    ACHIEVEMENT_BATTLE_NO_REPEATS,
    ACHIEVEMENT_BATTLE_AGAINST_THE_ODDS,
    ACHIEVEMENT_BATTLE_FOUR_MOVE_PHILOSOPHER,
    ACHIEVEMENT_BATTLE_NO_STAB_NEEDED,
    ACHIEVEMENT_BATTLE_COVERAGE_ENJOYER,
    ACHIEVEMENT_BATTLE_STATUS_HOARDER,
    ACHIEVEMENT_BATTLE_THREE_PUNCH_FINISH,
    ACHIEVEMENT_BATTLE_TEAM_PLAYER,
    ACHIEVEMENT_BATTLE_COMEBACK_KID,
    ACHIEVEMENT_BATTLE_LAST_ONE_STANDING,

    // L. Team Building & Composition (30)
    ACHIEVEMENT_TEAM_MONO_TYPE_TRIAL,
    ACHIEVEMENT_TEAM_ONE_TYPE_JOURNEY,
    ACHIEVEMENT_TEAM_MONO_TYPE_CHAMPION,
    ACHIEVEMENT_TEAM_TRIAL_BY_FIRE,
    ACHIEVEMENT_TEAM_NO_DUPLICATES,
    ACHIEVEMENT_TEAM_SIX_OF_A_KIND,
    ACHIEVEMENT_TEAM_UNDERSTUDY,
    ACHIEVEMENT_TEAM_BENCHWARMER,
    ACHIEVEMENT_TEAM_BOX_ROTATION,
    ACHIEVEMENT_TEAM_DEEP_BENCH,
    ACHIEVEMENT_TEAM_FULL_ROTATION,
    ACHIEVEMENT_TEAM_NO_ACE,
    ACHIEVEMENT_TEAM_TYPE_ROULETTE,
    ACHIEVEMENT_TEAM_WELL_EQUIPPED,
    ACHIEVEMENT_TEAM_FULL_HOUSE,
    ACHIEVEMENT_TEAM_VARIETY_IS_POWER,
    ACHIEVEMENT_TEAM_LINK_IN_THE_CHAIN,
    ACHIEVEMENT_TEAM_DREAM_TEAM,
    ACHIEVEMENT_TEAM_EVERYONE_GETS_A_TURN,
    ACHIEVEMENT_TEAM_REBUILD,
    ACHIEVEMENT_TEAM_RADICAL_REBUILD,
    ACHIEVEMENT_TEAM_CAPPED_OUT,
    ACHIEVEMENT_TEAM_FEATHERWEIGHT,
    ACHIEVEMENT_TEAM_UNDERDOG_RUN,
    ACHIEVEMENT_TEAM_DIVERSE_ROOTS,
    ACHIEVEMENT_TEAM_FRESH_START,
    ACHIEVEMENT_TEAM_SAME_SIX,
    ACHIEVEMENT_TEAM_BALANCED_ROSTER,
    ACHIEVEMENT_TEAM_NOBODY_BENCHED,
    ACHIEVEMENT_TEAM_ACE_ROTATION,

    ACHIEVEMENTS_COUNT,
};

// design doc catalog wave 8 (Stage 21, not yet implemented): mirrors the
// draft catalog's own section headers, so "complete every Bronze in a
// category"-style Mastery/Prestige achievements can be checked with a single
// helper once that wave exists. Added in Stage 15 and backfilled onto every
// existing entry rather than retrofitted later, since retrofitting it across
// ~270 entries would be far worse than authoring it from here on.
enum AchievementCategory
{
    ACHIEVEMENT_CATEGORY_ADVENTURE,
    ACHIEVEMENT_CATEGORY_COLLECTION,
    ACHIEVEMENT_CATEGORY_BATTLE,
    ACHIEVEMENT_CATEGORY_TEAM,
    ACHIEVEMENT_CATEGORY_CHALLENGE,
    ACHIEVEMENT_CATEGORY_NUZLOCKE,
    ACHIEVEMENT_CATEGORY_RANDOMIZER,
    ACHIEVEMENT_CATEGORY_NG_PLUS,
    ACHIEVEMENT_CATEGORY_EXPLORATION,
    ACHIEVEMENT_CATEGORY_ECONOMY,
    ACHIEVEMENT_CATEGORY_RECORDS,
    ACHIEVEMENT_CATEGORY_PROFILE,
    ACHIEVEMENT_CATEGORIES_COUNT,
};

enum AchievementTier
{
    ACHIEVEMENT_TIER_BRONZE,
    ACHIEVEMENT_TIER_SILVER,
    ACHIEVEMENT_TIER_GOLD,
    ACHIEVEMENT_TIER_DIAMOND,
};

// design doc §4: how often the progress behind an achievement resets.
enum AchievementScope
{
    ACHIEVEMENT_SCOPE_CURRENT_RUN,         // AchievementRunData; zeroed at new game (Stage 1.6)
    ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH, // tracked for the current save until it's beaten or reset
    ACHIEVEMENT_SCOPE_NG_PLUS,             // spans New Game+ cycles on the same save
    ACHIEVEMENT_SCOPE_PERSISTENT_PROFILE,  // never resets -- lives only in AchievementProfile
};

// design doc §10: a boost either scales across levels (LEVELED) or is a
// single locked -> purchased -> unlocked toggle (BINARY, §10.2) -- the
// framework must not assume every boost has a meaningful level count.
enum BoostType
{
    BOOST_TYPE_LEVELED,
    BOOST_TYPE_BINARY,
};

// Real entries land here as each stage implements its gameplay hook, keyed
// to designated initializers in src/data/achievement_boosts.h. BOOST_NONE is
// the reserved zero value AchievementBoost_GetInfo() falls back to for an
// out-of-range ID (mirrors ACHIEVEMENT_NONE above).
//
// BOOST_EXP_GAIN (Stage 8, design doc Stage 8): the first real boost --
// AchievementBoost_ApplyExp() (src/achievements.c) is its effect, hooked
// into src/battle_script_commands.c's exp calculation.
//
// BOOST_SHINY_CHANCE .. BOOST_LEGENDARY_ENCOUNTER (Stages 9-10, design doc
// §10.1's example list): each has its own AchievementBoost_Apply*/
// AchievementBoost_Extra* effect function in src/achievements.c, hooked at
// the location named in the Stages 9-10 table. BOOST_LEGENDARY_ENCOUNTER
// replaces an earlier BOOST_RARE_ENCOUNTER (biasing the wild-mon slot table
// toward its rarer end) with something more concrete: it hooks RoamerMove
// (src/roamer.c) so an active roamer is more likely to relocate onto the
// player's current route. TryStartRoamerEncounter -- whether an already
// present roamer's battle triggers -- is untouched.
//
// BOOST_CRIT_CHANCE .. BOOST_PERFECT_STARTER_IVS (Stage 10.1): the second
// catalog wave, and the first real BOOST_TYPE_BINARY content. Hooks, in order:
//   BOOST_CRIT_CHANCE            IsCriticalHit,             src/battle_util.c
//   BOOST_BERRY_YIELD            GetBerryCountByBerryTreeId, src/berry.c
//   BOOST_BERRY_GROWTH           BerryTreeTimeUpdate/PlantBerryTree, src/berry.c
//   BOOST_PP_SAVER               CancelerPPDeduction,       src/battle_move_resolution.c
//   BOOST_STATUS_RECOVERY        ENDTURN_STATUS_RECOVERY,   src/battle_end_turn.c
//   BOOST_SPRAY_DURATION         VAR_REPEL_STEP_COUNT sites, src/item_use.c + src/sprays.c
//   BOOST_NUZLOCKE_SECOND_CHANCE CB2_EndWildBattle,         src/battle_setup.c
//   BOOST_STARTER_KIT            NewGameInitData,           src/new_game.c
//   BOOST_PERFECT_STARTER_IVS    GenerateIVs,               src/ui_birch_case.c
enum BoostId
{
    BOOST_NONE,
    BOOST_EXP_GAIN,
    BOOST_SHINY_CHANCE,
    BOOST_CATCH_RATE,
    BOOST_MONEY_GAIN,
    BOOST_EGG_HATCH_SPEED,
    BOOST_FRIENDSHIP_GAIN,
    BOOST_LEGENDARY_ENCOUNTER,
    BOOST_CRIT_CHANCE,
    BOOST_BERRY_YIELD,
    BOOST_BERRY_GROWTH,
    BOOST_PP_SAVER,
    BOOST_STATUS_RECOVERY,
    BOOST_SPRAY_DURATION,
    BOOST_NUZLOCKE_SECOND_CHANCE,
    BOOST_STARTER_KIT,
    BOOST_PERFECT_STARTER_IVS,
    BOOSTS_COUNT,
};

#endif // GUARD_CONSTANTS_ACHIEVEMENTS_H
