#ifndef GUARD_CONSTANTS_ACHIEVEMENTS_H
#define GUARD_CONSTANTS_ACHIEVEMENTS_H

// Real entries land here in Stage 2.2/2.3, keyed to designated initializers
// in src/data/achievements.h. ACHIEVEMENT_NONE is the reserved zero value.
//
// The three ACHIEVEMENT_TEST_* entries are the throwaway achievements called
// for in Stage 2.3, wired into AddBagItem/HandleEndTurn_BattleWon/GameClear
// to exercise Achievement_TryComplete end to end. They're placeholders for
// the real catalog, not part of the design doc's actual achievement list.
enum AchievementId
{
    ACHIEVEMENT_NONE,
    ACHIEVEMENT_TEST_OBTAIN_POTION,
    ACHIEVEMENT_TEST_WIN_BATTLE,
    ACHIEVEMENT_TEST_COMPLETE_GAME,
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
// toward its rarer end) with something more concrete: it raises the flat
// 25% per-route roamer encounter check in src/roamer.c, so a roaming
// legendary already on the player's current route is more likely to
// actually appear.
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
    BOOSTS_COUNT,
};

#endif // GUARD_CONSTANTS_ACHIEVEMENTS_H
