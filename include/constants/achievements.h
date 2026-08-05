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
//
// Stage 17 (design doc catalog wave 4, plan Stage 17): the fourth catalog
// wave -- Exploration, Economy & Collection. Cheapest wave to evaluate (no
// new battle hooks), but not infrastructure-free like the plan sketch first
// assumed: it adds a small, corrected run-scoped map tracker (see
// AchievementRunDataExt's mapsVisited comment, include/global.h, for why a
// raw mapNum bitfield -- the original sketch -- would have collided across
// map groups) plus two shop-tracking fields, and five new gameStats[] slots
// (indices 53-57, still well under NUM_GAME_STATS). Those run-scoped fields
// live in a new struct AchievementRunDataExt in SaveBlock2, NOT in
// AchievementRunData (SaveBlock1) -- a real build caught that SaveBlock1 had
// only 12 bytes of slack left after Stage 16, not enough for this wave's 163
// bytes; SaveBlock2 had 1304 free instead. See AchievementRunDataExt's
// comment for the full story. Entries are checked from
// eleven call sites, each reusing an existing single-fire event rather than
// adding a new one: LoadCurrentMapData (src/overworld.c), the object-event
// branch of GetInteractionScript (src/field_control_avatar.c),
// SetHiddenItemFlag (src/field_specials.c), BuyMenuSubtractMoney
// (src/shop.c), the sell-item AddMoney call (src/item_menu.c), AddBagItem
// (src/item.c), ObjectEventInteractionPickBerryTree (src/berry.c), both
// GAME_STAT_POKEMON_TRADES sites (src/trade.c), both GAME_STAT_EVOLVED_POKEMON
// sites (src/evolution_scene.c), GetEvolutionTargetSpecies's DO_EVO path and
// GiveCapturedMonToPlayer (src/pokemon.c), the fishing-encounter stat
// increment (src/wild_encounter.c), and GameClear
// (src/post_battle_event_funcs.c). Local Expert piggybacks on Stage 13's
// existing Achievement_CheckPokedexMilestones FLAG_SET_SEEN branch (in
// src/achievements.c itself) rather than a new hook. Four entries (Save Your
// Change, Frugal Trainer, No Shopping, Resourceful) ride Stage 16's existing
// HandleEndTurn_BattleWon evaluation point via a new sibling function,
// Achievement_CheckGymEconomyMilestones, rather than a new battle hook.
//
//   M. ACHIEVEMENT_EXPLORE_FIRST_STEPS_ABROAD .. ACHIEVEMENT_COLLECT_ANGLER (30)
//      Exploration, Economy & Collection -- see src/achievements.c for the
//      per-entry hook-site breakdown. Tagged across the existing
//      EXPLORATION/ECONOMY/COLLECTION/ADVENTURE categories, not a new one.
//
// Stage 18 (design doc catalog wave 5, plan Stage 18): Challenge Runs &
// Nuzlocke. The New Game Settings menu (src/new_game_settings_menu.c)
// already *is* a challenge-modifier list, so Achievement_CountChallengeModifiers
// (src/achievements.c) -- Nuzlocke, HARD difficulty, the three FLAG_RANDOMIZE_*
// flags, the level cap, and the Stat Editor -- turns CHA-001/002/003/004 into
// a literal count/all-seven check. Nuzlocke entries key off explicit state
// only (nuzlockeModeEnabled plus Stage 10.1's nuzlockeCaughtFlags/
// nuzlockeExtraEncounterFlags), never incidental behaviour. Checked from four
// call sites, each reusing an existing hook: Achievement_CheckChallengeMilestones/
// Achievement_CheckNuzlockeMilestones (HandleEndTurn_BattleWon, alongside
// category M's Achievement_CheckGymEconomyMilestones) for the mid-run
// entries; Achievement_CheckChallengeCompletionMilestones/
// Achievement_CheckNuzlockeCompletionMilestones (GameClear, alongside Stage
// 16/17's completion checks) for the "complete the story"/"complete a
// Nuzlocke" entries; and Achievement_CheckNuzlockeExplorationMilestones
// (LoadCurrentMapData, alongside category M's exploration hook) for Full
// Encounter. GAME_STAT_USED_POKECENTER, declared since early on but never
// incremented, is made live at FldEff_PokecenterHeal (src/field_effect.c).
// gBattleResults.numHealingItemsUsed (include/battle.h) had the same problem
// -- declared, read by src/tv.c, never written -- and is wired up at
// BS_ItemRestoreHP (src/battle_script_commands.c) alongside this wave's new
// Achievement_RecordReviveUsed hook. No Freebies is narrowed to the starter
// specifically (by personality, so it survives evolution) rather than every
// scripted gift Pokemon in the game -- there's no single funnel point for
// "this Pokemon was a gift" the way catches and hatches already have one.
//
//   N. ACHIEVEMENT_CHALLENGE_SELF_IMPOSED .. ACHIEVEMENT_NUZLOCKE_UNASSISTED_SURVIVOR (30)
//      Challenge Runs & Nuzlocke -- see src/achievements.c for the per-entry
//      hook-site breakdown. Tagged across the existing CHALLENGE/NUZLOCKE
//      categories.
//
// Stage 19 (design doc catalog wave 6, plan Stage 19): Randomizer & New
// Game+. Both halves read state that already exists -- the three
// FLAG_RANDOMIZE_* flags and Stage 12's ngPlusCyclesCompleted/
// highestNgPlusCycle -- so per the plan doc this wave's hooks are the three
// existing Stage 5/12 wrapper functions (Achievement_OnFirstPlaythroughComplete/
// _OnNewGamePlusStarted/_OnNewGamePlusCycleCompleted) and Stage 18's existing
// Achievement_CheckChallengeMilestones/_CheckNuzlockeCompletionMilestones
// (HandleEndTurn_BattleWon/GameClear) -- no call site outside src/achievements.c
// is touched except GiveCapturedMonToPlayer (src/pokemon.c), which gets one
// more line alongside the four calls already there, the same "ride the
// funnel" idiom Stage 16-18 already established. New profile counters
// (AchievementProfile.reserved[]): trainersDefeatedAcrossNgPlus, consecutive
// NG+ cycles completed, and challenge-configuration signatures seen (a
// broader version of the plan's "randomized runs completed per flag
// combination" -- see that field's own comment for why). New per-cycle
// run-scoped fields live in AchievementRunDataExt (SaveBlock2), not
// AchievementRunData (SaveBlock1, zero bytes of slack left after Stage 18) --
// see that struct's own comment.
//
//   O. ACHIEVEMENT_RANDOMIZER_CHAOS_BEGINS .. ACHIEVEMENT_VARIETY_FULL_CIRCLE (30)
//      Randomizer & New Game+ -- see src/achievements.c for the per-entry
//      hook-site breakdown. Tagged across the existing RANDOMIZER/NUZLOCKE/
//      NG_PLUS/PROFILE categories, not a new one.
//
// Stage 20 (design doc catalog wave 7, plan Stage 20): Streaks, Records &
// Collection Remainder. The one remaining wave that needs genuinely new
// persistent counters -- a win streak spans battles, so it can't live in
// Stage 15's EWRAM-only AchievementBattleData; it belongs in
// AchievementRunDataExt (SaveBlock2, same "SaveBlock1 has zero slack left"
// detour Stage 19 already took) with a high-water mark mirrored into
// AchievementProfile.reserved[] so a streak earned in one run stays earned.
// Checked from nine call sites, most reusing an existing hook:
// Achievement_CheckBattleRecordsMilestones/Achievement_RecordPlayerFaint
// (HandleEndTurn_BattleWon/SetValuesOnFaint, alongside category K/N's battle
// hooks) for the streak/KO/comeback entries; Achievement_CheckRecordsMilestones
// (LoadCurrentMapData, alongside category M's exploration hook) for the
// "live state, any time is fine" entries; Achievement_CheckRecordsCompletionMilestones
// (GameClear, alongside category N's completion checks) for Legend of the
// Run; Achievement_RecordPartyWipe (the same two IsPartyEmpty()-gated sites
// Stage 18's Nuzlocke wipe detection already uses, RemoveFaintedMonsFromParty/
// FldEff_PokecenterHeal -- no third detector added) for the streak reset;
// Achievement_CheckFamilyMilestone (HandleSetPokedexFlag, alongside category
// B's Pokedex checks) for Family Reunion; Achievement_CheckPerfectIvMilestone
// (GiveCapturedMonToPlayer/Task_EggHatch, alongside categories C/I) for
// Perfect Specimen; and Achievement_RecordTMTaught/Achievement_CheckPokecenterMilestone
// (Task_LearnedMove/FldEff_PokecenterHeal) for the two remaining backfills
// that needed a hook of their own. Every other backfill (steps, total
// battles, hatched eggs) reads an existing GAME_STAT_* value live and needed
// no new tracking at all.
//
//   P. ACHIEVEMENT_RECORD_HOT_STREAK .. ACHIEVEMENT_RECORD_NURSES_NIGHTMARE (30)
//      Streaks, Records & Collection Remainder -- see src/achievements.c for
//      the per-entry hook-site breakdown. Tagged across the existing
//      RECORDS/COLLECTION/ADVENTURE categories, not a new one.
//
// Stage 21 (design doc catalog wave 8, plan Stage 21): Profile Meta, Mastery
// & Prestige, the last wave -- every entry here is defined over the finished
// catalog ("every Bronze in a category", "90% of all non-hidden
// achievements"), so it had to be authored after waves 2-7 landed. Every
// entry is tagged ACHIEVEMENT_CATEGORY_PROFILE, the same as Stage 19's
// ACHIEVEMENT_VARIETY_FULL_CIRCLE -- there is no separate "Mastery" category,
// these ARE the profile-meta category. Two static helpers in
// src/achievements.c, Achievement_CountInCategory/
// Achievement_CountCompletedInCategory (pass ACHIEVEMENT_TIER_COUNT for "any
// tier"), are the entire enabler; everything else is built from those two
// plus fields the profile already carries (totalPointsEarned, pointsInvested,
// boostLevels[], boostResets, playthroughsCompleted). Checked from the tail
// of Achievement_TryComplete (Achievement_CheckMasteryMilestones, alongside
// the existing Achievement_CheckPointMilestones) and from
// AchievementBoost_Purchase/_Reset (Achievement_CheckBoostMilestones) for the
// boost-state entries -- no new external call site, unlike every prior wave.
// New profile fields: pointsFromGoldOrBetter (No Easy Path) and
// playthroughConfigsSeen[]/_Count (Replay Master -- distinct challenge
// configurations seen across EVERY completed playthrough, not only NG+
// cycles like Stage 19's ngPlusConfigsSeen[]). New Team, New Me and Replay
// Master's sibling, Cycle Collector's own NGP-011 "No Nostalgia" check,
// reuse AchievementRunDataExt.previousCyclePartySpecies rather than adding
// another snapshot field -- see Achievement_OnFirstPlaythroughComplete's
// comment for why that field already means "the previous completion's
// party", NG+ or not.
//
// Self-reference note: a handful of entries here quantify over "every X",
// where the entry itself is a member of X (Nothing Left to Prove over every
// non-hidden achievement; Diamond Standard over every Diamond-tier
// achievement, and it is one). Each of those explicitly excludes itself from
// its own total/completed count -- without that, the condition could never
// become true, since the achievement's own flag is always still unset at the
// moment its condition is evaluated (Achievement_TryComplete sets the flag
// before running these checks, but a nested Achievement_TryComplete call for
// the SAME id is refused by Achievement_IsCompleted's guard, so "itself" can
// never contribute to its own count on the completing check). Master of the
// Game's 90% target is left un-excluded from Nothing Left to Prove's count on
// purpose -- the two entries only exclude themselves, not each other.
//
// Known catalog gaps, left as literally specified rather than reinterpreted:
// Achievement Hunter (Bronze in every category) is currently unattainable --
// CHALLENGE, NUZLOCKE and PROFILE have zero Bronze-tier entries between them
// -- until a future wave gives each of those three at least one. Easter Egg
// Hunter (five hidden achievements) is currently unattainable too -- it is
// the only hidden achievement anywhere in the catalog (see the "Dropped"
// note on Stage 20's HID-001..008 above); hidden achievements are meant to be
// authored alongside whatever secret content they gate, not invented here to
// fill the count.
//
//   Q. ACHIEVEMENT_MASTERY_BRONZE_MASTER .. ACHIEVEMENT_HIDDEN_EASTER_EGG_HUNTER (30)
//      Profile Meta, Mastery & Prestige -- see src/achievements.c for the
//      per-entry check-function breakdown. All ACHIEVEMENT_CATEGORY_PROFILE.
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

    // M. Exploration, Economy & Collection (30)
    ACHIEVEMENT_EXPLORE_FIRST_STEPS_ABROAD,
    ACHIEVEMENT_EXPLORE_OFF_THE_BEATEN_PATH,
    ACHIEVEMENT_EXPLORE_CARTOGRAPHER,
    ACHIEVEMENT_EXPLORE_COMPLETIONIST_TOURIST,
    ACHIEVEMENT_EXPLORE_ON_THE_ROAD,
    ACHIEVEMENT_EXPLORE_TREASURE_HUNTER,
    ACHIEVEMENT_EXPLORE_TREASURE_HOARD,
    ACHIEVEMENT_EXPLORE_TALK_TO_THE_LOCALS,
    ACHIEVEMENT_EXPLORE_PEOPLE_PERSON,
    ACHIEVEMENT_EXPLORE_LOCAL_EXPERT,
    ACHIEVEMENT_ECONOMY_FIRST_PURCHASE,
    ACHIEVEMENT_ECONOMY_REGULAR_CUSTOMER,
    ACHIEVEMENT_ECONOMY_BIG_SPENDER,
    ACHIEVEMENT_ECONOMY_WHALE,
    ACHIEVEMENT_ECONOMY_SAVE_YOUR_CHANGE,
    ACHIEVEMENT_ECONOMY_FRUGAL_TRAINER,
    ACHIEVEMENT_ECONOMY_NO_SHOPPING,
    ACHIEVEMENT_ECONOMY_RESOURCEFUL,
    ACHIEVEMENT_ECONOMY_TREASURE_PAYS,
    ACHIEVEMENT_ECONOMY_INVESTOR,
    ACHIEVEMENT_EXPLORE_PACK_RAT,
    ACHIEVEMENT_EXPLORE_NO_LOOSE_ENDS,
    ACHIEVEMENT_COLLECT_EVOLUTIONARY_PATH,
    ACHIEVEMENT_COLLECT_EVOLUTION_EXPERT,
    ACHIEVEMENT_COLLECT_FRIENDSHIP_BLOSSOMS,
    ACHIEVEMENT_COLLECT_STONE_AGE,
    ACHIEVEMENT_COLLECT_TRADE_SECRETS,
    ACHIEVEMENT_COLLECT_RARE_FIND,
    ACHIEVEMENT_COLLECT_GREEN_THUMB,
    ACHIEVEMENT_COLLECT_ANGLER,

    // N. Challenge Runs (19)
    ACHIEVEMENT_CHALLENGE_SELF_IMPOSED,
    ACHIEVEMENT_CHALLENGE_HARD_WAY,
    ACHIEVEMENT_CHALLENGE_BRUTAL_RULES,
    ACHIEVEMENT_CHALLENGE_NIGHTMARE_MODE,
    ACHIEVEMENT_CHALLENGE_NO_SHOPPING_RUN,
    ACHIEVEMENT_CHALLENGE_NO_HEALING_ITEMS,
    ACHIEVEMENT_CHALLENGE_ITEMLESS_BATTLE,
    ACHIEVEMENT_CHALLENGE_WHO_NEEDS_CENTERS,
    ACHIEVEMENT_CHALLENGE_NO_CENTERS,
    ACHIEVEMENT_CHALLENGE_SET_IN_STONE,
    ACHIEVEMENT_CHALLENGE_HARDCORE_SET,
    ACHIEVEMENT_CHALLENGE_LEVEL_DISCIPLINE,
    ACHIEVEMENT_CHALLENGE_CAPSTONE,
    ACHIEVEMENT_CHALLENGE_PERFECTLY_CAPPED,
    ACHIEVEMENT_CHALLENGE_MINIMALIST,
    ACHIEVEMENT_CHALLENGE_THREE_POKEMON,
    ACHIEVEMENT_CHALLENGE_SOLO_JOURNEY,
    ACHIEVEMENT_CHALLENGE_NO_FREEBIES,
    ACHIEVEMENT_CHALLENGE_HARDLY_ANY_HELP,

    // N. Nuzlocke (11)
    ACHIEVEMENT_NUZLOCKE_FIRST_GYM,
    ACHIEVEMENT_NUZLOCKE_HARDCORE_SURVIVOR,
    ACHIEVEMENT_NUZLOCKE_PERFECT,
    ACHIEVEMENT_NUZLOCKE_CLOSE_CALL,
    ACHIEVEMENT_NUZLOCKE_SPECIES_CLAUSE,
    ACHIEVEMENT_NUZLOCKE_NO_REVIVES,
    ACHIEVEMENT_NUZLOCKE_SCRAPPY,
    ACHIEVEMENT_NUZLOCKE_NO_ACE_ALLOWED,
    ACHIEVEMENT_NUZLOCKE_GRAVEYARD,
    ACHIEVEMENT_NUZLOCKE_FULL_ENCOUNTER,
    ACHIEVEMENT_NUZLOCKE_UNASSISTED_SURVIVOR,

    // O. Randomizer & New Game+ (30)
    ACHIEVEMENT_RANDOMIZER_CHAOS_BEGINS,
    ACHIEVEMENT_RANDOMIZER_RANDOM_BY_NATURE,
    ACHIEVEMENT_RANDOMIZER_TRULY_RANDOM,
    ACHIEVEMENT_RANDOMIZER_CHAOS_TEAM,
    ACHIEVEMENT_RANDOMIZER_PATCHWORK_TEAM,
    ACHIEVEMENT_RANDOMIZER_NEVER_SEEN_IT_COMING,
    ACHIEVEMENT_RANDOMIZER_SEED_EXPLORER,
    ACHIEVEMENT_RANDOMIZER_VETERAN,
    ACHIEVEMENT_RANDOMIZER_PURE_CHAOS,
    ACHIEVEMENT_NUZLOCKE_ACROSS_WORLDS,
    ACHIEVEMENT_NUZLOCKE_CHAOS_SURVIVOR,
    ACHIEVEMENT_NG_PLUS_ONE_MORE_TIME,
    ACHIEVEMENT_NG_PLUS_BEYOND_THE_BEGINNING,
    ACHIEVEMENT_NG_PLUS_FRESH_FACES,
    ACHIEVEMENT_NG_PLUS_NEVER_THE_SAME_FIGHT,
    ACHIEVEMENT_NG_PLUS_CYCLE_SPECIALIST,
    ACHIEVEMENT_NG_PLUS_ESCALATION,
    ACHIEVEMENT_NG_PLUS_NO_NOSTALGIA,
    ACHIEVEMENT_NG_PLUS_COMPLETE_REINVENTION,
    ACHIEVEMENT_NG_PLUS_BOSS_GAUNTLET,
    ACHIEVEMENT_NG_PLUS_CYCLE_NUZLOCKE,
    ACHIEVEMENT_NG_PLUS_ENDLESS_SURVIVOR,
    ACHIEVEMENT_RANDOMIZER_SPECIES_CHAOS,
    ACHIEVEMENT_RANDOMIZER_TYPE_CHAOS,
    ACHIEVEMENT_RANDOMIZER_MOVE_CHAOS,
    ACHIEVEMENT_RANDOMIZER_ROOKIE,
    ACHIEVEMENT_NG_PLUS_UNASSISTED_CYCLE,
    ACHIEVEMENT_NG_PLUS_TEN_CYCLES_DEEP,
    ACHIEVEMENT_NG_PLUS_CYCLE_COLLECTOR,
    ACHIEVEMENT_VARIETY_FULL_CIRCLE,

    // P. Streaks, Records & Collection Remainder (30)
    ACHIEVEMENT_RECORD_HOT_STREAK,
    ACHIEVEMENT_RECORD_UNBROKEN,
    ACHIEVEMENT_RECORD_ON_A_ROLL,
    ACHIEVEMENT_RECORD_UNTOUCHABLE_STREAK,
    ACHIEVEMENT_RECORD_THREE_GYM_STREAK,
    ACHIEVEMENT_RECORD_EIGHT_GYM_STREAK,
    ACHIEVEMENT_RECORD_LEAGUE_STREAK,
    ACHIEVEMENT_RECORD_VETERAN_TEAM,
    ACHIEVEMENT_RECORD_OLD_RELIABLE,
    ACHIEVEMENT_RECORD_LEGEND_OF_THE_RUN,
    ACHIEVEMENT_RECORD_COMEBACK_COUNT,
    ACHIEVEMENT_RECORD_GROWING_STRONG,
    ACHIEVEMENT_COLLECT_ONE_OF_EACH,
    ACHIEVEMENT_COLLECT_FAMILY_REUNION,
    ACHIEVEMENT_COLLECT_PERFECT_SPECIMEN,
    ACHIEVEMENT_COLLECT_ODDBALL,
    ACHIEVEMENT_COLLECT_UNDERESTIMATED,
    ACHIEVEMENT_RECORD_MARATHON_TRAINER,
    ACHIEVEMENT_RECORD_LONG_HAUL,
    ACHIEVEMENT_RECORD_PROLIFIC,
    ACHIEVEMENT_RECORD_BATTLE_MACHINE,
    ACHIEVEMENT_RECORD_CENTURY_CLUB,
    ACHIEVEMENT_RECORD_FULL_CENTURY,
    ACHIEVEMENT_COLLECT_BOX_FILLER,
    ACHIEVEMENT_COLLECT_STORAGE_BARON,
    ACHIEVEMENT_RECORD_DEVOTED,
    ACHIEVEMENT_RECORD_INSEPARABLE,
    ACHIEVEMENT_RECORD_MOVE_TUTOR,
    ACHIEVEMENT_RECORD_EGG_MARATHON,
    ACHIEVEMENT_RECORD_NURSES_NIGHTMARE,

    // Q. Profile Meta, Mastery & Prestige (30)
    ACHIEVEMENT_MASTERY_BRONZE_MASTER,
    ACHIEVEMENT_MASTERY_SILVER_MASTER,
    ACHIEVEMENT_MASTERY_GOLD_MASTER,
    ACHIEVEMENT_MASTERY_DIAMOND_MASTER,
    ACHIEVEMENT_MASTERY_CATEGORY_CONQUEROR,
    ACHIEVEMENT_MASTERY_MASTER_OF_THE_GAME,
    ACHIEVEMENT_MASTERY_NOTHING_LEFT_TO_PROVE,
    ACHIEVEMENT_MASTERY_ENDGAME_EXPLORER,
    ACHIEVEMENT_MASTERY_CHALLENGE_CONQUEROR,
    ACHIEVEMENT_MASTERY_UNBROKEN_WILL,
    ACHIEVEMENT_MASTERY_CHAOS_MASTER,
    ACHIEVEMENT_MASTERY_REPLAY_ARCHITECT,
    ACHIEVEMENT_PROFILE_FREQUENT_FLYER,
    ACHIEVEMENT_PROFILE_VETERAN_TRAINER,
    ACHIEVEMENT_PROFILE_RESIDENT_CHAMPION,
    ACHIEVEMENT_PROFILE_ACHIEVEMENT_HUNTER,
    ACHIEVEMENT_PROFILE_WELL_ROUNDED,
    ACHIEVEMENT_PROFILE_MASTER_OF_ALL,
    ACHIEVEMENT_PROFILE_POINT_HOARDER,
    ACHIEVEMENT_PROFILE_POINT_LEGEND,
    ACHIEVEMENT_PROFILE_NO_EASY_PATH,
    ACHIEVEMENT_PROFILE_BOOST_INVESTOR,
    ACHIEVEMENT_PROFILE_FULL_INVESTMENT,
    ACHIEVEMENT_PROFILE_RECONFIGURED,
    ACHIEVEMENT_PROFILE_SELECTIVE_MASTERY,
    ACHIEVEMENT_PROFILE_META_PROG_MASTER,
    ACHIEVEMENT_MASTERY_DIAMOND_STANDARD,
    ACHIEVEMENT_VARIETY_NEW_TEAM_NEW_ME,
    ACHIEVEMENT_VARIETY_REPLAY_MASTER,
    ACHIEVEMENT_HIDDEN_EASTER_EGG_HUNTER,

    ACHIEVEMENTS_COUNT,
};

// Full Encounter bookkeeping (AchievementRunData.nuzlockePendingRoute,
// include/global.h; Achievement_CheckNuzlockeExplorationMilestones,
// src/achievements.c): sentinel for "no encounter-eligible route currently
// awaiting resolution." Outside GET_NUZLOCKE_FLAG's own NUM_NUZLOCKE_ROUTE_FLAGS
// (128) bounds check, so it can never collide with a real route id.
#define ACHIEVEMENT_NUZLOCKE_NO_PENDING_ROUTE 0xFFFF

// design doc catalog wave 8 (Stage 21, category Q): mirrors the draft
// catalog's own section headers, so "complete every Bronze in a
// category"-style Mastery/Prestige achievements can be checked with a single
// helper (Achievement_CountCompletedInCategory/Achievement_CountInCategory,
// src/achievements.c) now that wave exists. Added in Stage 15 and backfilled
// onto every existing entry rather than retrofitted later, since retrofitting
// it across ~270 entries would have been far worse than authoring it from
// there on.
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

    // Stage 21 (category Q): not a real tier -- passed to
    // Achievement_CountInCategory/Achievement_CountCompletedInCategory
    // (src/achievements.c) to mean "every tier" rather than one specific one.
    ACHIEVEMENT_TIER_COUNT,
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
