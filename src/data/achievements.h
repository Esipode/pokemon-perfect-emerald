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

// UNUSED until Stage 2.3's Achievement_TryComplete reads from this table --
// -Wunused-const-variable (part of -Wall for C) would otherwise fail the
// build over an empty placeholder table. Remove UNUSED once that lands.
static const struct Achievement gAchievements[ACHIEVEMENTS_COUNT] UNUSED =
{
    [ACHIEVEMENT_NONE] = {
        .name        = ACHIEVEMENT_NAME("-"),
        .description = COMPOUND_STRING(""),
        .tier        = ACHIEVEMENT_TIER_BRONZE,
        .scope       = ACHIEVEMENT_SCOPE_PERSISTENT_PROFILE,
        .points      = 0,
        .hidden      = TRUE,
    },
    // Real entries populated in Stage 2.3.
};

STATIC_ASSERT(ACHIEVEMENTS_COUNT <= MAX_ACHIEVEMENTS, AchievementCountFitsProfile);
