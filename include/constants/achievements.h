#ifndef GUARD_CONSTANTS_ACHIEVEMENTS_H
#define GUARD_CONSTANTS_ACHIEVEMENTS_H

// Real entries land here in Stage 2.2/2.3, keyed to designated initializers
// in src/data/achievements.h. ACHIEVEMENT_NONE is the reserved zero value.
enum AchievementId
{
    ACHIEVEMENT_NONE,
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

#endif // GUARD_CONSTANTS_ACHIEVEMENTS_H
