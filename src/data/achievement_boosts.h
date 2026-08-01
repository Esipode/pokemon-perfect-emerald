// One entry per enum BoostId (constants/achievements.h), keyed by designated
// initializer -- mirrors src/data/achievements.h's convention. Included from
// src/achievements.c only (Stage 7); nothing else should reference
// gAchievementBoosts directly -- go through the public API in
// include/achievements.h instead.
#define BOOST_NAME(str) COMPOUND_STRING_SIZE_LIMIT(str, BOOST_NAME_LENGTH)

// Stage 7 (design doc Stage 7: "Use temporary test boosts"): TEST_LEVELED
// exercises the numerical/leveled path (design doc §10.1), TEST_BINARY the
// locked -> purchased -> unlocked path (§10.2). Real boosts replace these
// starting Stage 8; nothing here has a gameplay effect yet.
static const u16 sBoostTestLeveledCosts[]   = {100, 200, 300};
static const u16 sBoostTestBinaryCosts[]    = {500};

static const struct AchievementBoost gAchievementBoosts[BOOSTS_COUNT] =
{
    [BOOST_NONE] = {
        .name        = BOOST_NAME("-"),
        .description = COMPOUND_STRING(""),
        .type        = BOOST_TYPE_LEVELED,
        .maxLevel    = 0,
        .costs       = NULL,
        .effects     = NULL,
    },
    [BOOST_TEST_LEVELED] = {
        .name        = BOOST_NAME("Test Leveled Boost"),
        .description = COMPOUND_STRING("Framework test: a numerical boost with 3 levels."),
        .type        = BOOST_TYPE_LEVELED,
        .maxLevel    = 3,
        .costs       = sBoostTestLeveledCosts,
        .effects     = NULL,
    },
    [BOOST_TEST_BINARY] = {
        .name        = BOOST_NAME("Test Binary Boost"),
        .description = COMPOUND_STRING("Framework test: a one-time locked/unlocked reward."),
        .type        = BOOST_TYPE_BINARY,
        .maxLevel    = 1,
        .costs       = sBoostTestBinaryCosts,
        .effects     = NULL,
    },
};

STATIC_ASSERT(BOOSTS_COUNT <= MAX_BOOSTS, BoostCountFitsProfile);
