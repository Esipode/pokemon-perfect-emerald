// One entry per enum BoostId (constants/achievements.h), keyed by designated
// initializer -- mirrors src/data/achievements.h's convention. Included from
// src/achievements.c only (Stage 7); nothing else should reference
// gAchievementBoosts directly -- go through the public API in
// include/achievements.h instead.
#define BOOST_NAME(str) COMPOUND_STRING_SIZE_LIMIT(str, BOOST_NAME_LENGTH)

// Stage 7 (design doc Stage 7: "Use temporary test boosts"): TEST_LEVELED
// exercises the numerical/leveled path (design doc §10.1), TEST_BINARY the
// locked -> purchased -> unlocked path (§10.2). Kept alongside the real
// catalog below (starting with BOOST_EXP_GAIN, Stage 8) for continued
// framework testing -- not part of the design doc's actual boost list.
static const u16 sBoostTestLeveledCosts[]   = {100, 200, 300};
static const u16 sBoostTestBinaryCosts[]    = {500};

// Stage 8 (design doc Stage 8): the first real boost. Costs rise steeply
// per §12 ("prevents the player from acquiring every useful boost
// immediately"); exact tuning is explicitly deferred to Stage 13/14
// balancing, so these are placeholders in the same shape the real curve
// will take, not a final balance pass.
static const u16 sBoostExpGainCosts[]   = {200, 400, 700, 1100, 1600};
// effects[0] (level 0) is never read -- AchievementBoost_ApplyExp short-
// circuits on level == 0 before indexing this array. effects[level] is the
// percent bonus applied at that level, matching the design doc §10.1
// example (Level 1: +10% ... Level 5: +50%).
static const u16 sBoostExpGainEffects[] = {0, 10, 20, 30, 40, 50};

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
    [BOOST_EXP_GAIN] = {
        .name        = BOOST_NAME("EXP Gain"),
        .description = COMPOUND_STRING("Increases EXP earned from battles."),
        .type        = BOOST_TYPE_LEVELED,
        .maxLevel    = 5,
        .costs       = sBoostExpGainCosts,
        .effects     = sBoostExpGainEffects,
    },
};

STATIC_ASSERT(BOOSTS_COUNT <= MAX_BOOSTS, BoostCountFitsProfile);
