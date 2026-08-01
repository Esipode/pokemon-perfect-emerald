#ifndef GUARD_CONSTANTS_ACHIEVEMENTS_H
#define GUARD_CONSTANTS_ACHIEVEMENTS_H

// Real entries land here in Stage 2.2/2.3, keyed to designated initializers
// in src/data/achievements.h. ACHIEVEMENT_NONE is the reserved zero value.
//
// The three ACHIEVEMENT_TEST_* entries are the throwaway achievements called
// for in Stage 2.3, wired into AddBagItem/HandleEndTurn_BattleWon/GameClear
// to exercise Achievement_TryComplete end to end. They're placeholders for
// the real catalog, not part of the design doc's actual achievement list.
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
enum AchievementId
{
    ACHIEVEMENT_NONE,
    ACHIEVEMENT_TEST_OBTAIN_POTION,
    ACHIEVEMENT_TEST_WIN_BATTLE,
    ACHIEVEMENT_TEST_COMPLETE_GAME,

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

    ACHIEVEMENTS_COUNT,
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
// The two BOOST_TEST_* entries are Stage 7's throwaway boosts (design doc
// Stage 7: "Use temporary test boosts") -- BOOST_TEST_LEVELED exercises the
// numerical/leveled path (§10.1), BOOST_TEST_BINARY the binary path (§10.2).
// Kept around after Stage 8 for continued framework testing; not part of
// the design doc's actual boost list.
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
// catalog wave, and the first real BOOST_TYPE_BINARY content (until now only
// BOOST_TEST_BINARY exercised that path). Hooks, in order:
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
    BOOST_TEST_LEVELED,
    BOOST_TEST_BINARY,
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
