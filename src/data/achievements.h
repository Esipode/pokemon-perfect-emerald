// One entry per enum AchievementId (constants/achievements.h), keyed by
// designated initializer -- see design doc §4 and Stage 2.2 for the table
// format. Included from src/achievements.c only (Stage 2.1); nothing else
// should reference gAchievements directly -- go through the public API in
// include/achievements.h instead.
//
// Matches the src/data/items.h:632 / src/data/moves_info.h:46 convention:
// designated initializers keyed by ID, .name wrapped in a *_NAME() macro that
// enforces the length cap at compile time, .description left as a plain
// COMPOUND_STRING (no cap).
#define ACHIEVEMENT_NAME(str) COMPOUND_STRING_SIZE_LIMIT(str, ACHIEVEMENT_NAME_LENGTH)

static const struct Achievement gAchievements[ACHIEVEMENTS_COUNT] =
{
    [ACHIEVEMENT_NONE] = {
        .name        = ACHIEVEMENT_NAME("-"),
        .description = COMPOUND_STRING(""),
        .tier        = ACHIEVEMENT_TIER_BRONZE,
        .scope       = ACHIEVEMENT_SCOPE_PERSISTENT_PROFILE,
        .points      = 0,
        .hidden      = TRUE,
    },

    // Throwaway test achievements (Stage 2.3): verify the award pipeline end
    // to end (flag + points + persistence) before the real catalog exists.
    // Replace with real entries once Stage 3+ needs actual content.
    [ACHIEVEMENT_TEST_OBTAIN_POTION] = {
        .name        = ACHIEVEMENT_NAME("Potion Collector"),
        .description = COMPOUND_STRING("Obtain a Potion."),
        .tier        = ACHIEVEMENT_TIER_BRONZE,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .points      = 10,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_TEST_WIN_BATTLE] = {
        .name        = ACHIEVEMENT_NAME("First Victory"),
        .description = COMPOUND_STRING("Win a battle."),
        .tier        = ACHIEVEMENT_TIER_BRONZE,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .points      = 10,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_TEST_COMPLETE_GAME] = {
        .name        = ACHIEVEMENT_NAME("Champion"),
        .description = COMPOUND_STRING("Complete the game."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .points      = 50,
        .hidden      = FALSE,
    },
};

STATIC_ASSERT(ACHIEVEMENTS_COUNT <= MAX_ACHIEVEMENTS, AchievementCountFitsProfile);
