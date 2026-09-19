#ifndef GUARD_CONSTANTS_ACHIEVEMENTS_H
#define GUARD_CONSTANTS_ACHIEVEMENTS_H

// Category roster. Each category is tagged onto its entries in src/data/achievements.h
// via enum AchievementCategory. Hook doc comments are in include/achievements.h;
// per-entry hook sites are in src/achievements.c.
//
//   A. ACHIEVEMENT_STORY_RIVAL_ROUTE103 .. ACHIEVEMENT_STORY_CHAMPION (15)
//      Badges/story milestones -- Achievement_CheckStoryMilestones,
//      callnative'd from Common_EventScript_CheckLevelCapIncrease.
//   B. ACHIEVEMENT_DEX_SEEN_10 .. ACHIEVEMENT_DEX_SEEN_100 (4)
//      Pokedex seen percentage -- Achievement_CheckPokedexMilestones,
//      HandleSetPokedexFlag (src/pokemon.c).
//   C. ACHIEVEMENT_CATCH_100 .. ACHIEVEMENT_CATCH_ALL (4)
//      Bronze/Silver/Gold are raw counts (Achievement_CheckCaptureMilestones,
//      GiveCapturedMonToPlayer); Diamond is a distinct-species check
//      (Achievement_CheckPokedexMilestones).
//   D. ACHIEVEMENT_SHINY_1 .. ACHIEVEMENT_SHINY_25 (3)
//      shiniesObtained count -- Achievement_OnShinyObtained, GiveCapturedMonToPlayer.
//   E. ACHIEVEMENT_TRAINERS_10 .. ACHIEVEMENT_TRAINERS_500 (5)
//      Trainer battle count -- Achievement_CheckTrainerBattleMilestones,
//      CB2_EndTrainerBattle (src/battle_setup.c).
//   F. ACHIEVEMENT_WILD_BATTLES_50 .. ACHIEVEMENT_WILD_BATTLES_500 (3)
//      Wild battle count -- Achievement_CheckWildBattleMilestones,
//      CB2_EndWildBattle (src/battle_setup.c).
//   G. ACHIEVEMENT_ITEM_MASTER_BALL .. ACHIEVEMENT_ITEM_HEART_SCALE (4)
//      Obtain a specific item -- Achievement_CheckItemMilestones, AddBagItem (src/item.c).
//   H. ACHIEVEMENT_MONEY_10K .. ACHIEVEMENT_MONEY_MAX (3)
//      Money held -- Achievement_CheckMoneyMilestones, AddMoney (src/money.c).
//   I. ACHIEVEMENT_EGG_1 .. ACHIEVEMENT_EGG_SHINY (4)
//      Hatched egg count/shiny -- Achievement_CheckEggMilestones,
//      Task_EggHatch (src/egg_hatch.c).
//   J. ACHIEVEMENT_NG_PLUS_CYCLE_COMPLETE .. ACHIEVEMENT_POINTS_6000 (4)
//      Multi-run/persistent-profile milestones -- checked from
//      Achievement_OnFirstPlaythroughComplete / Achievement_OnNewGamePlusStarted /
//      Achievement_OnNewGamePlusCycleCompleted; ACHIEVEMENT_POINTS_6000 from
//      Achievement_TryComplete itself.
//   K. ACHIEVEMENT_BATTLE_CRITICAL_SUCCESS .. ACHIEVEMENT_BATTLE_LAST_ONE_STANDING
//      Battle Mastery -- Achievement_CheckBattleMilestones, HandleEndTurn_BattleWon
//      (src/battle_main.c). struct AchievementBattleData (src/achievements.c) is an
//      EWRAM-only per-battle scratchpad, never saved.
//   L. ACHIEVEMENT_TEAM_MONO_TYPE_TRIAL .. ACHIEVEMENT_TEAM_ACE_ROTATION
//      Team Building & Composition. Uses struct AchievementRunData (include/global.h).
//      Achievement_IsGymBattle() (TRAINER_CLASS_LEADER only) builds on
//      Achievement_IsMajorBattle(). Checked from Achievement_CheckTeamMilestones
//      (same call site as K), Achievement_CheckPartyStateMilestones (rides category
//      A's callnative), and Achievement_CheckTeamCompletionMilestones (GameClear).
//   M. ACHIEVEMENT_EXPLORE_FIRST_STEPS_ABROAD .. ACHIEVEMENT_COLLECT_ANGLER
//      Exploration, Economy & Collection. Tagged across the EXPLORATION/ECONOMY/
//      COLLECTION/ADVENTURE categories. Run-scoped fields live in
//      AchievementRunDataExt (SaveBlock2), not AchievementRunData: SaveBlock1 has
//      little slack left (see AchievementRunDataExt's mapsVisited comment in
//      include/global.h for why a raw mapNum bitfield would collide across map
//      groups). Each entry rides an existing single-fire event (LoadCurrentMapData,
//      GetInteractionScript, SetHiddenItemFlag, BuyMenuSubtractMoney, the sell-item
//      AddMoney, AddBagItem, berry harvest, evolution, fishing, GameClear). Four
//      entries (Save Your Change, Frugal Trainer, No Shopping, Resourceful) ride
//      Achievement_CheckGymEconomyMilestones at category L's evaluation point.
//   N. ACHIEVEMENT_CHALLENGE_SELF_IMPOSED .. ACHIEVEMENT_NUZLOCKE_GRAVEYARD
//      Challenge Runs & Nuzlocke. Tagged across the CHALLENGE/NUZLOCKE categories.
//      Achievement_CountChallengeModifiers (src/achievements.c) counts the New Game
//      Settings that make a run harder (see its comment). Nuzlocke entries key off explicit
//      state only (nuzlockeModeEnabled plus SaveBlock2's nuzlockeZoneCaughtFlags/
//      nuzlockeZoneExtraEncounterFlags). Checked from
//      Achievement_CheckChallengeMilestones/Achievement_CheckNuzlockeMilestones
//      (HandleEndTurn_BattleWon) and Achievement_CheckChallengeCompletionMilestones/
//      Achievement_CheckNuzlockeCompletionMilestones (GameClear).
//      GAME_STAT_USED_POKECENTER is incremented at FldEff_PokecenterHeal
//      (src/field_effect.c). No Freebies is narrowed to the starter (tracked by
//      personality so it survives evolution): there is no single funnel point for
//      "this Pokemon was a gift".
//   O. ACHIEVEMENT_RANDOMIZER_CHAOS_BEGINS .. ACHIEVEMENT_RANDOMIZER_ROOKIE
//      Randomizer & New Game+. Tagged across the RANDOMIZER/NUZLOCKE/NG_PLUS/PROFILE
//      categories. Reads the three FLAG_RANDOMIZE_* flags and ngPlusCyclesCompleted/
//      highestNgPlusCycle, so it reuses category J's wrapper functions and category
//      N's checks; GiveCapturedMonToPlayer gets one more call. Per-cycle run-scoped
//      fields live in AchievementRunDataExt (SaveBlock2).
//   P. ACHIEVEMENT_RECORD_HOT_STREAK .. ACHIEVEMENT_RECORD_NURSES_NIGHTMARE
//      Streaks, Records & Collection Remainder. Tagged across the RECORDS/COLLECTION/
//      ADVENTURE categories. A win streak spans battles, so it lives in
//      AchievementRunDataExt (SaveBlock2), with a high-water mark mirrored into
//      AchievementProfile so a streak earned in one run stays earned. Hooks:
//        Achievement_CheckBattleRecordsMilestones    HandleEndTurn_BattleWon (streak/KO entries)
//        Achievement_CheckRecordsMilestones          LoadCurrentMapData (live-state entries)
//        Achievement_CheckRecordsCompletionMilestones GameClear (Legend of the Run)
//        Achievement_RecordPartyWipe                 the two IsPartyEmpty()-gated sites
//                                                    RemoveFaintedMonsFromParty/FldEff_PokecenterHeal
//        Achievement_CheckFamilyMilestone            HandleSetPokedexFlag (Family Reunion)
//        Achievement_RecordTMTaught/_CheckPokecenterMilestone  Task_LearnedMove/FldEff_PokecenterHeal
//      The other backfills (steps, total battles, hatched eggs) read an existing
//      GAME_STAT_* value live.
//   Q. ACHIEVEMENT_PROFILE_WELL_ROUNDED .. ACHIEVEMENT_MASTERY_DIAMOND_STANDARD
//      Profile Meta, Mastery & Prestige. All ACHIEVEMENT_CATEGORY_PROFILE, defined
//      over the finished catalog. Checked with no external call site: the tail of
//      Achievement_TryComplete (Achievement_CheckMasteryMilestones, alongside
//      Achievement_CheckPointMilestones) and AchievementBoost_Purchase/_Reset
//      (Achievement_CheckBoostMilestones). Live profile field: pointsFromGoldOrBetter
//      (No Easy Path).
//
// Diamond Standard quantifies over "every Diamond-tier achievement", of which it is
// one, so Achievement_AllDiamondCompleted (src/achievements.c) excludes it from its
// own total/completed count.
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

    // B. Pokedex (4)
    ACHIEVEMENT_DEX_SEEN_10,
    ACHIEVEMENT_DEX_SEEN_25,
    ACHIEVEMENT_DEX_SEEN_50,
    ACHIEVEMENT_DEX_SEEN_100,

    // C. Captures (4)
    ACHIEVEMENT_CATCH_100,
    ACHIEVEMENT_CATCH_350,
    ACHIEVEMENT_CATCH_700,
    ACHIEVEMENT_CATCH_ALL,

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

    // J. Multi-Run / Persistent Profile (4).
    ACHIEVEMENT_NG_PLUS_CYCLE_COMPLETE,
    ACHIEVEMENT_NUZLOCKE_1,
    ACHIEVEMENT_RANDOMIZED_1,
    ACHIEVEMENT_POINTS_6000,

    // K. Battle Mastery (29).
    ACHIEVEMENT_BATTLE_CRITICAL_SUCCESS,
    ACHIEVEMENT_BATTLE_TYPE_ADVANTAGE,
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

    // L. Team Building & Composition (27).
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
    ACHIEVEMENT_TEAM_LINK_IN_THE_CHAIN,
    ACHIEVEMENT_TEAM_DREAM_TEAM,
    ACHIEVEMENT_TEAM_EVERYONE_GETS_A_TURN,
    ACHIEVEMENT_TEAM_REBUILD,
    ACHIEVEMENT_TEAM_RADICAL_REBUILD,
    ACHIEVEMENT_TEAM_FEATHERWEIGHT,
    ACHIEVEMENT_TEAM_UNDERDOG_RUN,
    ACHIEVEMENT_TEAM_DIVERSE_ROOTS,
    ACHIEVEMENT_TEAM_FRESH_START,
    ACHIEVEMENT_TEAM_SAME_SIX,
    ACHIEVEMENT_TEAM_BALANCED_ROSTER,
    ACHIEVEMENT_TEAM_NOBODY_BENCHED,
    ACHIEVEMENT_TEAM_ACE_ROTATION,

    // M. Exploration, Economy & Collection (28).
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
    ACHIEVEMENT_ECONOMY_TREASURE_PAYS,
    ACHIEVEMENT_ECONOMY_INVESTOR,
    ACHIEVEMENT_EXPLORE_PACK_RAT,
    ACHIEVEMENT_EXPLORE_NO_LOOSE_ENDS,
    ACHIEVEMENT_COLLECT_EVOLUTIONARY_PATH,
    ACHIEVEMENT_COLLECT_EVOLUTION_EXPERT,
    ACHIEVEMENT_COLLECT_FRIENDSHIP_BLOSSOMS,
    ACHIEVEMENT_COLLECT_STONE_AGE,
    ACHIEVEMENT_COLLECT_RARE_FIND,
    ACHIEVEMENT_COLLECT_GREEN_THUMB,
    ACHIEVEMENT_COLLECT_ANGLER,

    // N. Challenge Runs (12).
    ACHIEVEMENT_CHALLENGE_SELF_IMPOSED,
    ACHIEVEMENT_CHALLENGE_NIGHTMARE_MODE,
    ACHIEVEMENT_CHALLENGE_NO_SHOPPING_RUN,
    ACHIEVEMENT_CHALLENGE_NO_HEALING_ITEMS,
    ACHIEVEMENT_CHALLENGE_ITEMLESS_BATTLE,
    ACHIEVEMENT_CHALLENGE_WHO_NEEDS_CENTERS,
    ACHIEVEMENT_CHALLENGE_NO_CENTERS,
    ACHIEVEMENT_CHALLENGE_SET_IN_STONE,
    ACHIEVEMENT_CHALLENGE_HARDCORE_SET,
    ACHIEVEMENT_CHALLENGE_MINIMALIST,
    ACHIEVEMENT_CHALLENGE_SOLO_JOURNEY,
    ACHIEVEMENT_CHALLENGE_NO_FREEBIES,

    // N. Nuzlocke (5).
    ACHIEVEMENT_NUZLOCKE_FIRST_GYM,
    ACHIEVEMENT_NUZLOCKE_PERFECT,
    ACHIEVEMENT_NUZLOCKE_CLOSE_CALL,
    ACHIEVEMENT_NUZLOCKE_SCRAPPY,
    ACHIEVEMENT_NUZLOCKE_GRAVEYARD,

    // O. Randomizer & New Game+ (21). Chaos Begins/Random by Nature/Truly Random's
    // descriptions spell out which of the three randomizer settings (species/type/move)
    // each one needs; see their catalog comment in src/data/achievements.h.
    ACHIEVEMENT_RANDOMIZER_CHAOS_BEGINS,
    ACHIEVEMENT_RANDOMIZER_RANDOM_BY_NATURE,
    ACHIEVEMENT_RANDOMIZER_TRULY_RANDOM,
    ACHIEVEMENT_RANDOMIZER_CHAOS_TEAM,
    ACHIEVEMENT_RANDOMIZER_PATCHWORK_TEAM,
    ACHIEVEMENT_RANDOMIZER_PURE_CHAOS,
    ACHIEVEMENT_NUZLOCKE_ACROSS_WORLDS,
    ACHIEVEMENT_NUZLOCKE_CHAOS_SURVIVOR,
    ACHIEVEMENT_NG_PLUS_FRESH_FACES,
    ACHIEVEMENT_NG_PLUS_NEVER_THE_SAME_FIGHT,
    ACHIEVEMENT_NG_PLUS_CYCLE_SPECIALIST,
    ACHIEVEMENT_NG_PLUS_CYCLE_NUZLOCKE,
    ACHIEVEMENT_RANDOMIZER_SPECIES_CHAOS,
    ACHIEVEMENT_RANDOMIZER_TYPE_CHAOS,
    ACHIEVEMENT_RANDOMIZER_MOVE_CHAOS,
    ACHIEVEMENT_RANDOMIZER_ROOKIE,

    // P. Streaks, Records & Collection Remainder (26).
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
    ACHIEVEMENT_RECORD_GROWING_STRONG,
    ACHIEVEMENT_COLLECT_ONE_OF_EACH,
    ACHIEVEMENT_COLLECT_FAMILY_REUNION,
    ACHIEVEMENT_COLLECT_ODDBALL,
    ACHIEVEMENT_COLLECT_UNDERESTIMATED,
    ACHIEVEMENT_RECORD_MARATHON_TRAINER,
    ACHIEVEMENT_RECORD_LONG_HAUL,
    ACHIEVEMENT_RECORD_PROLIFIC,
    ACHIEVEMENT_RECORD_BATTLE_MACHINE,
    ACHIEVEMENT_RECORD_CENTURY_CLUB,
    ACHIEVEMENT_RECORD_FULL_CENTURY,
    ACHIEVEMENT_RECORD_DEVOTED,
    ACHIEVEMENT_RECORD_INSEPARABLE,
    ACHIEVEMENT_RECORD_MOVE_TUTOR,
    ACHIEVEMENT_RECORD_EGG_MARATHON,
    ACHIEVEMENT_RECORD_NURSES_NIGHTMARE,

    // Q. Profile Meta, Mastery & Prestige (9).
    ACHIEVEMENT_PROFILE_WELL_ROUNDED,
    ACHIEVEMENT_PROFILE_POINT_HOARDER,
    ACHIEVEMENT_PROFILE_POINT_LEGEND,
    ACHIEVEMENT_PROFILE_NO_EASY_PATH,
    ACHIEVEMENT_PROFILE_BOOST_INVESTOR,
    ACHIEVEMENT_PROFILE_FULL_INVESTMENT,
    ACHIEVEMENT_PROFILE_RECONFIGURED,
    ACHIEVEMENT_MASTERY_DIAMOND_STANDARD,

    // R. Recruits Mode (5). Achievement_CheckNewModeBattleMilestones
    // (HandleEndTurn_BattleWon), Achievement_RecordRecruitRetirement
    // (recruits_mode.c), and Achievement_CheckNewModeCompletionMilestones
    // (GameClear).
    ACHIEVEMENT_RECRUITS_FRESH_RECRUITS,
    ACHIEVEMENT_RECRUITS_HONORABLE_DISCHARGE,
    ACHIEVEMENT_RECRUITS_REVOLVING_DOOR,
    ACHIEVEMENT_RECRUITS_FULL_TURNOVER,
    ACHIEVEMENT_RECRUITS_ENDLESS_RECRUITMENT_DRIVE,

    // S. Limited Party (5). Achievement_CheckNewModeBattleMilestones,
    // Achievement_CheckStoryMilestones's badge checkpoints (Earned Your
    // Keep/Full Roster Restored -- both read the live derived cap), and
    // Achievement_CheckNewModeCompletionMilestones.
    ACHIEVEMENT_LIMITED_PARTY_TIGHT_SQUAD,
    ACHIEVEMENT_LIMITED_PARTY_EARNED_YOUR_KEEP,
    ACHIEVEMENT_LIMITED_PARTY_FULL_ROSTER_RESTORED,
    ACHIEVEMENT_LIMITED_PARTY_NO_ROOM_TO_SPARE,
    ACHIEVEMENT_LIMITED_PARTY_BARE_MINIMUM_CHAMPION,

    // T. Draft Mode (6). Achievement_RecordDraftCompleted/
    // _RecordDraftReplacement (draft_mode.c) and
    // Achievement_CheckNewModeCompletionMilestones.
    ACHIEVEMENT_DRAFT_FIRST_PICK,
    ACHIEVEMENT_DRAFT_TOUGH_CALL,
    ACHIEVEMENT_DRAFT_THE_CASE_IS_CLOSED,
    ACHIEVEMENT_DRAFT_FULL_CASE_CLEAR,
    ACHIEVEMENT_DRAFT_DRAFTED_NOT_CAUGHT,
    ACHIEVEMENT_DRAFT_NO_BALL_NEEDED,

    // U. Rotation Mode (5). Achievement_CheckNewModeBattleMilestones and
    // Achievement_CheckNewModeCompletionMilestones.
    ACHIEVEMENT_ROTATION_SPIN_THE_WHEEL,
    ACHIEVEMENT_ROTATION_ON_A_ROTATION,
    ACHIEVEMENT_ROTATION_GYM_LEADER_ROULETTE,
    ACHIEVEMENT_ROTATION_FULL_CIRCUIT,
    ACHIEVEMENT_ROTATION_CHAOS_ROTATION,

    // V. Mono Type Mode (6). Gated on MonoType_IsEnabled() specifically
    // (not team composition) -- distinct from category L's existing
    // MONO_TYPE_TRIAL/_ONE_TYPE_JOURNEY/_MONO_TYPE_CHAMPION/_TRIAL_BY_FIRE,
    // which check party composition regardless of the challenge toggle.
    // Achievement_CheckMonoStarterMilestones (ui_birch_case.c),
    // Achievement_CheckStoryMilestones's Gym 4 checkpoint,
    // Achievement_RecordMonoModeObtain (pokemon.c/egg_hatch.c), and
    // Achievement_CheckNewModeCompletionMilestones.
    ACHIEVEMENT_MONO_TYPE_COMMITTED_TO_THE_BIT,
    ACHIEVEMENT_MONO_TYPE_TYPE_SPECIALIST,
    ACHIEVEMENT_MONO_TYPE_PERFECT_FIT,
    ACHIEVEMENT_MONO_TYPE_TRUE_BELIEVER,
    ACHIEVEMENT_MONO_TYPE_SECOND_VERSE,
    ACHIEVEMENT_MONO_TYPE_ONE_TYPE_TO_RULE_THEM_ALL,

    // W. Mono Gen Mode (5). Parallel structure to category V.
    ACHIEVEMENT_MONO_GEN_GENERATION_LOYALIST,
    ACHIEVEMENT_MONO_GEN_REGIONAL_PURIST,
    ACHIEVEMENT_MONO_GEN_GOTTA_CATCH_SOME_OF_THEM,
    ACHIEVEMENT_MONO_GEN_TRUE_TO_THE_ROOTS,
    ACHIEVEMENT_MONO_GEN_OLD_SCHOOL_HARD_MODE,

    // X. Cross-Mode Stacking (3). Recruits/Limited Party/Draft/Rotation/Mono
    // Type/Mono Gen are independent toggles (GAME MODE -- Nuzlocke/Draft/
    // Recruits -- is the only mutually exclusive row), so up to 5 of them
    // plus GAME MODE can be active together. Tagged
    // ACHIEVEMENT_CATEGORY_CHALLENGE (stacking-challenge concept, same as
    // Brutal Rules/Nightmare Mode). Achievement_CheckNewModeCompletionMilestones.
    ACHIEVEMENT_CROSSMODE_MODE_COLLECTOR,
    ACHIEVEMENT_CROSSMODE_KITCHEN_SINK,
    ACHIEVEMENT_CROSSMODE_THE_FULL_STACK,

    // Y. Emporium Rewards (8). Per-save reward collection, tracked in
    // AchievementRunDataExt.emporiumRewardsWon[] (SaveBlock2).
    // Achievement_OnEmporiumRewardWon (src/achievements.c), called from
    // EmporiumBufferRewardItem (src/battle_emporium.c), win branch only.
    // All ACHIEVEMENT_CATEGORY_COLLECTION.
    ACHIEVEMENT_EMPORIUM_FIRST_PRIZE,
    ACHIEVEMENT_EMPORIUM_GRAND_TOUR,
    ACHIEVEMENT_EMPORIUM_CRYSTAL_COLLECTOR,
    ACHIEVEMENT_EMPORIUM_FULL_SPECTRUM,
    ACHIEVEMENT_EMPORIUM_STONE_TRADER,
    ACHIEVEMENT_EMPORIUM_MEGA_MAGNATE,
    ACHIEVEMENT_EMPORIUM_EVERY_TYPE_COVERED,
    ACHIEVEMENT_EMPORIUM_EMPTIED,

    // Z. Legendary Collection (7). Counted per evolution family via
    // Achievement_GetEvolutionRoot/_GetFamilyMembers + caught Pokedex flags.
    // Achievement_CheckLegendaryMilestones (src/achievements.c), called from
    // the FLAG_SET_CAUGHT branch of HandleSetPokedexFlag (src/pokemon.c)
    // alongside Achievement_CheckFamilyMilestone; running count in
    // AchievementRunDataExt.legendaryFamiliesCaught (SaveBlock2). One-shot
    // Achievement_BackfillLegendaryFamilies (same hook + LoadCurrentMapData,
    // src/overworld.c) recomputes that count for a pre-feature save, guarded
    // by AchievementRunDataExt.legendaryCountBackfilled.
    // All ACHIEVEMENT_CATEGORY_COLLECTION.
    ACHIEVEMENT_LEGENDARY_MYTH_CONFIRMED,
    ACHIEVEMENT_LEGENDARY_RARE_COMPANY,
    ACHIEVEMENT_LEGENDARY_LEGEND_SEEKER,
    ACHIEVEMENT_LEGENDARY_HALL_OF_LEGENDS,
    ACHIEVEMENT_LEGENDARY_LIVING_LEGEND,
    ACHIEVEMENT_LEGENDARY_MYTHICAL_MENAGERIE,
    ACHIEVEMENT_LEGENDARY_LEGEND_OF_LEGENDS,

    ACHIEVEMENTS_COUNT,
};

// Tags each entry's .category in src/data/achievements.h; read by
// achievements_menu.c's tier lists, etc.
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
    ACHIEVEMENT_CATEGORY_RECRUITS,
    ACHIEVEMENT_CATEGORY_LIMITED_PARTY,
    ACHIEVEMENT_CATEGORY_DRAFT,
    ACHIEVEMENT_CATEGORY_ROTATION,
    ACHIEVEMENT_CATEGORY_MONO_TYPE,
    ACHIEVEMENT_CATEGORY_MONO_GEN,
    ACHIEVEMENT_CATEGORIES_COUNT,
};

enum AchievementTier
{
    ACHIEVEMENT_TIER_BRONZE,
    ACHIEVEMENT_TIER_SILVER,
    ACHIEVEMENT_TIER_GOLD,
    ACHIEVEMENT_TIER_DIAMOND,

    // Loop bounds and array sizing for the four real tiers (see
    // src/achievements_menu.c and src/achievement_popup.c).
    ACHIEVEMENT_TIER_COUNT,
};

// How often the progress behind an achievement resets.
enum AchievementScope
{
    ACHIEVEMENT_SCOPE_CURRENT_RUN,         // AchievementRunData; zeroed at new game
    ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH, // tracked for the current save until it's beaten or reset
    ACHIEVEMENT_SCOPE_NG_PLUS,             // spans New Game+ cycles on the same save
    ACHIEVEMENT_SCOPE_PERSISTENT_PROFILE,  // never resets -- lives only in AchievementProfile
};

// A boost either scales across levels (LEVELED) or is a single locked ->
// purchased -> unlocked toggle (BINARY) -- the framework must not assume
// every boost has a meaningful level count.
enum BoostType
{
    BOOST_TYPE_LEVELED,
    BOOST_TYPE_BINARY,
};

// Keyed to designated initializers in src/data/achievement_boosts.h. BOOST_NONE is
// the reserved zero value AchievementBoost_GetInfo() falls back to for an
// out-of-range ID.
//
// BOOST_EXP_GAIN: AchievementBoost_ApplyExp() (src/achievements.c), hooked into the
// exp calculation in src/battle_script_commands.c.
//
// BOOST_SHINY_CHANCE .. BOOST_LEGENDARY_ENCOUNTER: each has its own
// AchievementBoost_Apply*/AchievementBoost_Extra* function in src/achievements.c.
// BOOST_LEGENDARY_ENCOUNTER hooks RoamerMove (src/roamer.c) so an active roamer is
// more likely to relocate onto the player's current route; TryStartRoamerEncounter
// is untouched.
//
// BOOST_CRIT_CHANCE .. BOOST_PERFECT_STARTER_IVS hooks, in order:
//   BOOST_CRIT_CHANCE            IsCriticalHit,             src/battle_util.c
//   BOOST_BERRY_YIELD            GetBerryCountByBerryTreeId, src/berry.c
//   BOOST_BERRY_GROWTH           BerryTreeTimeUpdate/PlantBerryTree, src/berry.c
//   BOOST_PP_SAVER               CancelerPPDeduction,       src/battle_move_resolution.c
//   BOOST_STATUS_RECOVERY        ENDTURN_STATUS_RECOVERY,   src/battle_end_turn.c
//   BOOST_SPRAY_DURATION         VAR_REPEL_STEP_COUNT sites, src/item_use.c + src/sprays.c
//   BOOST_NUZLOCKE_SECOND_CHANCE CB2_EndWildBattle,         src/battle_setup.c
//   BOOST_STARTER_KIT            NewGameInitData,           src/new_game.c
//   BOOST_PERFECT_STARTER_IVS    GenerateIVs,               src/ui_birch_case.c
//
// BOOST_SHINY_CHARM_START .. BOOST_POST_BATTLE_HEAL hooks, in order:
//   BOOST_SHINY_CHARM_START      NewGameInitData,             src/new_game.c
//   BOOST_ABILITY_CAPSULE_START  NewGameInitData,             src/new_game.c
//   BOOST_ABILITY_PATCH_START    NewGameInitData,             src/new_game.c
//   BOOST_CONSUMABLE_SAVE        every genuine item-use RemoveBagItem site
//                                 (not sell/give/discard), src/item_use.c +
//                                 src/party_menu.c
//   BOOST_EGG_IV_REROLL          SetInitialEggData,           src/daycare.c
//   BOOST_WILD_IV_REROLL         CreateWildMon,                src/wild_encounter.c
//   BOOST_SHOP_DISCOUNT          every price computed for a mart purchase, src/shop.c
//   BOOST_SURVIVE_1HP            GetAdjustedDamage,            src/battle_util.c
//   BOOST_POST_BATTLE_HEAL       CB2_EndTrainerBattle
//                                 (win branches only),          src/battle_setup.c
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
    BOOST_SHINY_CHARM_START,
    BOOST_ABILITY_CAPSULE_START,
    BOOST_ABILITY_PATCH_START,
    BOOST_CONSUMABLE_SAVE,
    BOOST_EGG_IV_REROLL,
    BOOST_WILD_IV_REROLL,
    BOOST_SHOP_DISCOUNT,
    BOOST_SURVIVE_1HP,
    BOOST_POST_BATTLE_HEAL,
    BOOSTS_COUNT,
};

#endif // GUARD_CONSTANTS_ACHIEVEMENTS_H
