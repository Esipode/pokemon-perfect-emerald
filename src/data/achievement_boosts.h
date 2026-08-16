// One entry per enum BoostId (constants/achievements.h), keyed by designated
// initializer -- mirrors src/data/achievements.h's convention. Included from
// src/achievements.c only; nothing else should reference
// gAchievementBoosts directly -- go through the public API in
// include/achievements.h instead.
#define BOOST_NAME(str) COMPOUND_STRING_SIZE_LIMIT(str, BOOST_NAME_LENGTH)

// The first real boost. Costs rise steeply so the player can't acquire
// every useful boost immediately.
//
// Every cost curve below is scaled so the total cost to max every boost in
// the catalog equals the catalog's total achievement points (20,000, see
// src/data/achievements.h's own note on that total) -- each curve is the
// original placeholder shape ({200, 400, 700, 1100, 1600}) scaled by
// ~0.4706x (20,000 / 42,500, the placeholder curves' own total) and rounded
// to a clean number, not re-derived from scratch.
static const u16 sBoostExpGainCosts[]   = {100, 200, 350, 500, 750};
// effects[0] (level 0) is never read -- AchievementBoost_ApplyExp short-
// circuits on level == 0 before indexing this array. effects[level] is the
// percent bonus applied at that level, matching the example curve
// (Level 1: +10% ... Level 5: +50%).
static const u16 sBoostExpGainEffects[] = {0, 10, 20, 30, 40, 50};

// The rest of the example boost list. Same cost curve as EXP Gain above --
// every boost below shares one curve rather than six independently-tuned
// ones. See the cost-curve comment above.
static const u16 sBoostSharedCosts[] = {100, 200, 350, 500, 750};

// AchievementBoost_ExtraShinyRerolls returns this directly: an extra shiny
// reroll per level, stacking with the Shiny Charm/Lure/chain-fishing/DexNav
// rerolls ComputePlayerShinyOdds (src/pokemon.c) already accumulates.
static const u16 sBoostShinyChanceEffects[] = {0, 1, 2, 3, 4, 5};

// Percent bonus applied to ComputeCaptureOdds' 0-255 result -- same shape as
// EXP Gain's percent bonus.
static const u16 sBoostCatchRateEffects[] = {0, 10, 20, 30, 40, 50};

// Percent bonus applied to the battle money reward -- same shape as EXP
// Gain's percent bonus.
static const u16 sBoostMoneyGainEffects[] = {0, 10, 20, 30, 40, 50};

// Flat addition to GetEggCyclesToSubtract's result (normally 1, or 2 with
// Magma Armor/Flame Body/Steam Engine) -- the same "add to the subtraction"
// shape those abilities already use, not a percent.
static const u16 sBoostEggHatchSpeedEffects[] = {0, 1, 2, 3, 4, 5};

// Percent bonus applied to CalculateFriendshipBonuses' positive result --
// same shape as EXP Gain's percent bonus.
static const u16 sBoostFriendshipGainEffects[] = {0, 10, 20, 30, 40, 50};

// AchievementBoost_ShouldRoamerSeekPlayer rolls this directly as a percent:
// a flat 1% chance per level, per roamer move, that RoamerMove (src/roamer.c)
// draws the roamer straight onto the player's current route instead of its
// normal random relocation.
static const u16 sBoostLegendaryEncounterEffects[] = {0, 1, 2, 3, 4, 5};

// The six leveled boosts below reuse sBoostSharedCosts above rather than
// declaring 3- and 4-entry curves of their own -- AchievementBoost_CanPurchase
// only ever indexes costs[level] for level < maxLevel, so a 3-level boost
// simply reads the leading three entries.
//
// One shared price for all three binary boosts, for the same reason: each is a
// single one-time purchase, so there's nothing to shape a curve around yet.
// Scaled down from 1500 to 600, the same ~0.4706x scale as sBoostSharedCosts
// above (the binary boosts' own slice of the 20,000 target).
static const u16 sBoostSharedBinaryCosts[] = {600};

// IsCriticalHit (src/battle_util.c) rolls this as a flat percent chance to
// upgrade a non-critical hit, on top of whatever the normal crit-stage roll
// already decided. Never overrides a hard block (Battle Armor, Lucky Chant).
static const u16 sBoostCritChanceEffects[] = {0, 3, 6, 9};

// Flat berries added to GetBerryCountByBerryTreeId's result (src/berry.c) --
// applied at read time, never written into the saved berryYield field.
static const u16 sBoostBerryYieldEffects[] = {0, 1, 2, 3};

// Percent faster, applied to a berry stage's duration in minutes as
// minutes * 100 / (100 + percent) -- so level 4 (+100%) halves the wait
// rather than reaching zero.
static const u16 sBoostBerryGrowthEffects[] = {0, 25, 50, 75, 100};

// CancelerPPDeduction (src/battle_move_resolution.c) rolls this as a flat
// percent chance to skip a move's PP cost entirely for that use.
static const u16 sBoostPpSaverEffects[] = {0, 5, 10, 15};

// Rolled once per turn per battler at ENDTURN_STATUS_RECOVERY
// (src/battle_end_turn.c) as a flat percent chance to shake off a
// non-volatile status, the same way Shed Skin does.
static const u16 sBoostStatusRecoveryEffects[] = {0, 5, 10, 15};

// Percent added to a Repel/Lure's step count as steps * (100 + percent) / 100,
// at every VAR_REPEL_STEP_COUNT write site.
static const u16 sBoostSprayDurationEffects[] = {0, 25, 50, 75, 100};

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
    [BOOST_EXP_GAIN] = {
        .name        = BOOST_NAME("EXP Gain"),
        .description = COMPOUND_STRING("Increases EXP earned from battles."),
        .type        = BOOST_TYPE_LEVELED,
        .maxLevel    = 5,
        .costs       = sBoostExpGainCosts,
        .effects     = sBoostExpGainEffects,
    },
    [BOOST_SHINY_CHANCE] = {
        .name        = BOOST_NAME("Shiny Chance"),
        .description = COMPOUND_STRING("Increases the chance of finding a shiny Pokemon."),
        .type        = BOOST_TYPE_LEVELED,
        .maxLevel    = 5,
        .costs       = sBoostSharedCosts,
        .effects     = sBoostShinyChanceEffects,
    },
    [BOOST_CATCH_RATE] = {
        .name        = BOOST_NAME("Catch Rate"),
        .description = COMPOUND_STRING("Increases the odds of catching wild Pokemon."),
        .type        = BOOST_TYPE_LEVELED,
        .maxLevel    = 5,
        .costs       = sBoostSharedCosts,
        .effects     = sBoostCatchRateEffects,
    },
    [BOOST_MONEY_GAIN] = {
        .name        = BOOST_NAME("Money Gain"),
        .description = COMPOUND_STRING("Increases money earned from trainer battles."),
        .type        = BOOST_TYPE_LEVELED,
        .maxLevel    = 5,
        .costs       = sBoostSharedCosts,
        .effects     = sBoostMoneyGainEffects,
    },
    [BOOST_EGG_HATCH_SPEED] = {
        .name        = BOOST_NAME("Egg Hatch Speed"),
        .description = COMPOUND_STRING("Reduces the number of steps needed to hatch eggs."),
        .type        = BOOST_TYPE_LEVELED,
        .maxLevel    = 5,
        .costs       = sBoostSharedCosts,
        .effects     = sBoostEggHatchSpeedEffects,
    },
    [BOOST_FRIENDSHIP_GAIN] = {
        .name        = BOOST_NAME("Friendship Gain"),
        .description = COMPOUND_STRING("Increases friendship gained by your Pokemon."),
        .type        = BOOST_TYPE_LEVELED,
        .maxLevel    = 5,
        .costs       = sBoostSharedCosts,
        .effects     = sBoostFriendshipGainEffects,
    },
    [BOOST_LEGENDARY_ENCOUNTER] = {
        .name        = BOOST_NAME("Legendary Encounter"),
        .description = COMPOUND_STRING("Increases the chance of a roaming legendary appearing."),
        .type        = BOOST_TYPE_LEVELED,
        .maxLevel    = 5,
        .costs       = sBoostSharedCosts,
        .effects     = sBoostLegendaryEncounterEffects,
    },
    [BOOST_CRIT_CHANCE] = {
        .name        = BOOST_NAME("Critical Hit"),
        .description = COMPOUND_STRING("Increases your chance of landing critical hits."),
        .type        = BOOST_TYPE_LEVELED,
        .maxLevel    = 3,
        .costs       = sBoostSharedCosts,
        .effects     = sBoostCritChanceEffects,
    },
    [BOOST_BERRY_YIELD] = {
        .name        = BOOST_NAME("Berry Yield"),
        .description = COMPOUND_STRING("Berry trees give more Berries per harvest."),
        .type        = BOOST_TYPE_LEVELED,
        .maxLevel    = 3,
        .costs       = sBoostSharedCosts,
        .effects     = sBoostBerryYieldEffects,
    },
    [BOOST_BERRY_GROWTH] = {
        .name        = BOOST_NAME("Berry Growth"),
        .description = COMPOUND_STRING("Berry trees grow to maturity faster."),
        .type        = BOOST_TYPE_LEVELED,
        .maxLevel    = 4,
        .costs       = sBoostSharedCosts,
        .effects     = sBoostBerryGrowthEffects,
    },
    [BOOST_PP_SAVER] = {
        .name        = BOOST_NAME("PP Saver"),
        .description = COMPOUND_STRING("Moves sometimes cost no PP to use."),
        .type        = BOOST_TYPE_LEVELED,
        .maxLevel    = 3,
        .costs       = sBoostSharedCosts,
        .effects     = sBoostPpSaverEffects,
    },
    [BOOST_STATUS_RECOVERY] = {
        .name        = BOOST_NAME("Status Recovery"),
        .description = COMPOUND_STRING("Your Pokemon may shake off status each turn."),
        .type        = BOOST_TYPE_LEVELED,
        .maxLevel    = 3,
        .costs       = sBoostSharedCosts,
        .effects     = sBoostStatusRecoveryEffects,
    },
    [BOOST_SPRAY_DURATION] = {
        .name        = BOOST_NAME("Spray Duration"),
        .description = COMPOUND_STRING("Repels and Lures last for more steps."),
        .type        = BOOST_TYPE_LEVELED,
        .maxLevel    = 4,
        .costs       = sBoostSharedCosts,
        .effects     = sBoostSprayDurationEffects,
    },
    [BOOST_NUZLOCKE_SECOND_CHANCE] = {
        .name        = BOOST_NAME("Second Chance"),
        .description = COMPOUND_STRING("Nuzlocke: one retry per route if you don't catch."),
        .type        = BOOST_TYPE_BINARY,
        .maxLevel    = 1,
        .costs       = sBoostSharedBinaryCosts,
        .effects     = NULL,
    },
    [BOOST_STARTER_KIT] = {
        .name        = BOOST_NAME("Starter Kit"),
        .description = COMPOUND_STRING("Begin a new game with items and extra money."),
        .type        = BOOST_TYPE_BINARY,
        .maxLevel    = 1,
        .costs       = sBoostSharedBinaryCosts,
        .effects     = NULL,
    },
    [BOOST_PERFECT_STARTER_IVS] = {
        .name        = BOOST_NAME("Perfect Starter"),
        .description = COMPOUND_STRING("Your starter Pokemon has perfect IVs."),
        .type        = BOOST_TYPE_BINARY,
        .maxLevel    = 1,
        .costs       = sBoostSharedBinaryCosts,
        .effects     = NULL,
    },
};

STATIC_ASSERT(BOOSTS_COUNT <= MAX_BOOSTS, BoostCountFitsProfile);
