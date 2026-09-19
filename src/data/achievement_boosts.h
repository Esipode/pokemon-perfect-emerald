// One entry per enum BoostId (constants/achievements.h), keyed by designated
// initializer. Included from src/achievements.c only; go through the public API in
// include/achievements.h instead of referencing gAchievementBoosts directly.
#define BOOST_NAME(str) COMPOUND_STRING_SIZE_LIMIT(str, BOOST_NAME_LENGTH)

// Cost curves are tuned so the total cost to max every boost equals the catalog's
// total achievement points (30,000, see src/data/achievements.h): 7 five-level
// curves (EXP Gain + the six that share sBoostSharedCosts) at 2,200 each, 2
// four-level boosts at 1,350, 10 three-level boosts at 770, and 6 binary boosts
// at 700 -- 15,400 + 2,700 + 7,700 + 4,200 = 30,000.
static const u16 sBoostExpGainCosts[]   = {120, 250, 400, 580, 850};
// effects[0] is never read: AchievementBoost_ApplyExp short-circuits on level == 0.
// effects[level] is the percent bonus at that level.
static const u16 sBoostExpGainEffects[] = {0, 10, 20, 30, 40, 50};

// Shared by every other leveled boost. CanPurchase only indexes costs[level] for
// level < maxLevel, so a 3- or 4-level boost reads the leading entries.
static const u16 sBoostSharedCosts[] = {120, 250, 400, 580, 850};

// Extra shiny rerolls per level, stacking with the rerolls ComputePlayerShinyOdds
// (src/pokemon.c) already accumulates.
static const u16 sBoostShinyChanceEffects[] = {0, 1, 2, 3, 4, 5};

// Percent bonus applied to ComputeCaptureOdds' 0-255 result.
static const u16 sBoostCatchRateEffects[] = {0, 10, 20, 30, 40, 50};

// Percent bonus applied to the battle money reward.
static const u16 sBoostMoneyGainEffects[] = {0, 10, 20, 30, 40, 50};

// Flat addition to GetEggCyclesToSubtract's result (normally 1, or 2 with Magma
// Armor/Flame Body/Steam Engine), not a percent.
static const u16 sBoostEggHatchSpeedEffects[] = {0, 1, 2, 3, 4, 5};

// Percent bonus applied to CalculateFriendshipBonuses' positive result.
static const u16 sBoostFriendshipGainEffects[] = {0, 10, 20, 30, 40, 50};

// Rolled directly as a percent by AchievementBoost_ShouldRoamerSeekPlayer: a flat 1%
// chance per level, per roamer move, that RoamerMove (src/roamer.c) draws the roamer
// onto the player's current route instead of its normal random relocation.
static const u16 sBoostLegendaryEncounterEffects[] = {0, 1, 2, 3, 4, 5};

// One shared price for all six binary boosts: each is a single one-time purchase.
// 700 apiece is their slice of the 30,000 maxed-boost target.
static const u16 sBoostSharedBinaryCosts[] = {700};

// Flat percent chance to upgrade a non-critical hit in IsCriticalHit
// (src/battle_util.c). Never overrides a hard block (Battle Armor, Lucky Chant).
static const u16 sBoostCritChanceEffects[] = {0, 3, 6, 9};

// Flat berries added to GetBerryCountByBerryTreeId's result (src/berry.c), applied
// at read time and never written into the saved berryYield field.
static const u16 sBoostBerryYieldEffects[] = {0, 1, 2, 3};

// Percent faster, applied to a berry stage's duration in minutes as
// minutes * 100 / (100 + percent), so level 4 (+100%) halves the wait
// rather than reaching zero.
static const u16 sBoostBerryGrowthEffects[] = {0, 25, 50, 75, 100};

// Flat percent chance to skip a move's PP cost in CancelerPPDeduction
// (src/battle_move_resolution.c).
static const u16 sBoostPpSaverEffects[] = {0, 5, 10, 15};

// Flat percent chance per turn per battler at ENDTURN_STATUS_RECOVERY
// (src/battle_end_turn.c) to shake off a non-volatile status, like Shed Skin.
static const u16 sBoostStatusRecoveryEffects[] = {0, 5, 10, 15};

// Percent added to a Repel/Lure's step count as steps * (100 + percent) / 100,
// at every VAR_REPEL_STEP_COUNT write site.
static const u16 sBoostSprayDurationEffects[] = {0, 25, 50, 75, 100};

// Flat percent chance, per use, that a POCKET_ITEMS consumable isn't removed from
// the bag (AchievementBoost_ShouldConsumeItem).
static const u16 sBoostConsumableSaveEffects[] = {0, 10, 20, 30};

// Extra IV-spread reroll count for AchievementBoost_ApplyEggIvReroll/_ApplyWildIvReroll
// (src/achievements.c): the IVs are rolled once, then rerolled this many more times,
// keeping the spread with the highest stat total.
static const u16 sBoostIvRerollEffects[] = {0, 1, 2, 3};

// Percent knocked off every mart price.
static const u16 sBoostShopDiscountEffects[] = {0, 10, 20, 30};

// Flat percent chance a lethal hit on the player's side leaves 1 HP instead.
static const u16 sBoostSurvive1HpEffects[] = {0, 5, 10, 15};

// Percent of each party mon's max HP restored after winning a trainer battle.
static const u16 sBoostPostBattleHealEffects[] = {0, 5, 10, 15};

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
        .effectFormat = COMPOUND_STRING("(+{STR_VAR_2}%)"),
    },
    [BOOST_SHINY_CHANCE] = {
        .name        = BOOST_NAME("Shiny Chance"),
        .description = COMPOUND_STRING("Increases the chance of finding a shiny Pokemon."),
        .type        = BOOST_TYPE_LEVELED,
        .maxLevel    = 5,
        .costs       = sBoostSharedCosts,
        .effects     = sBoostShinyChanceEffects,
        .effectFormat = COMPOUND_STRING("(+{STR_VAR_2} rerolls)"),
    },
    [BOOST_CATCH_RATE] = {
        .name        = BOOST_NAME("Catch Rate"),
        .description = COMPOUND_STRING("Increases the odds of catching wild Pokemon."),
        .type        = BOOST_TYPE_LEVELED,
        .maxLevel    = 5,
        .costs       = sBoostSharedCosts,
        .effects     = sBoostCatchRateEffects,
        .effectFormat = COMPOUND_STRING("(+{STR_VAR_2}%)"),
    },
    [BOOST_MONEY_GAIN] = {
        .name        = BOOST_NAME("Money Gain"),
        .description = COMPOUND_STRING("Increases money earned from trainer battles."),
        .type        = BOOST_TYPE_LEVELED,
        .maxLevel    = 5,
        .costs       = sBoostSharedCosts,
        .effects     = sBoostMoneyGainEffects,
        .effectFormat = COMPOUND_STRING("(+{STR_VAR_2}%)"),
    },
    [BOOST_EGG_HATCH_SPEED] = {
        .name        = BOOST_NAME("Egg Hatch Speed"),
        .description = COMPOUND_STRING("Reduces the number of steps needed to hatch eggs."),
        .type        = BOOST_TYPE_LEVELED,
        .maxLevel    = 5,
        .costs       = sBoostSharedCosts,
        .effects     = sBoostEggHatchSpeedEffects,
        .effectFormat = COMPOUND_STRING("(+{STR_VAR_2} cycles)"),
    },
    [BOOST_FRIENDSHIP_GAIN] = {
        .name        = BOOST_NAME("Friendship Gain"),
        .description = COMPOUND_STRING("Increases friendship gained by your Pokemon."),
        .type        = BOOST_TYPE_LEVELED,
        .maxLevel    = 5,
        .costs       = sBoostSharedCosts,
        .effects     = sBoostFriendshipGainEffects,
        .effectFormat = COMPOUND_STRING("(+{STR_VAR_2}%)"),
    },
    [BOOST_LEGENDARY_ENCOUNTER] = {
        .name        = BOOST_NAME("Legendary Encounter"),
        .description = COMPOUND_STRING("Increases the chance of a roaming legendary appearing."),
        .type        = BOOST_TYPE_LEVELED,
        .maxLevel    = 5,
        .costs       = sBoostSharedCosts,
        .effects     = sBoostLegendaryEncounterEffects,
        .effectFormat = COMPOUND_STRING("({STR_VAR_2}% per move)"),
    },
    [BOOST_CRIT_CHANCE] = {
        .name        = BOOST_NAME("Critical Hit"),
        .description = COMPOUND_STRING("Increases your chance of landing critical hits."),
        .type        = BOOST_TYPE_LEVELED,
        .maxLevel    = 3,
        .costs       = sBoostSharedCosts,
        .effects     = sBoostCritChanceEffects,
        .effectFormat = COMPOUND_STRING("(+{STR_VAR_2}%)"),
    },
    [BOOST_BERRY_YIELD] = {
        .name        = BOOST_NAME("Berry Yield"),
        .description = COMPOUND_STRING("Berry trees give more Berries per harvest."),
        .type        = BOOST_TYPE_LEVELED,
        .maxLevel    = 3,
        .costs       = sBoostSharedCosts,
        .effects     = sBoostBerryYieldEffects,
        .effectFormat = COMPOUND_STRING("(+{STR_VAR_2} berries)"),
    },
    [BOOST_BERRY_GROWTH] = {
        .name        = BOOST_NAME("Berry Growth"),
        .description = COMPOUND_STRING("Berry trees grow to maturity faster."),
        .type        = BOOST_TYPE_LEVELED,
        .maxLevel    = 4,
        .costs       = sBoostSharedCosts,
        .effects     = sBoostBerryGrowthEffects,
        .effectFormat = COMPOUND_STRING("({STR_VAR_2}% faster)"),
    },
    [BOOST_PP_SAVER] = {
        .name        = BOOST_NAME("PP Saver"),
        .description = COMPOUND_STRING("Moves sometimes cost no PP to use."),
        .type        = BOOST_TYPE_LEVELED,
        .maxLevel    = 3,
        .costs       = sBoostSharedCosts,
        .effects     = sBoostPpSaverEffects,
        .effectFormat = COMPOUND_STRING("({STR_VAR_2}% chance)"),
    },
    [BOOST_STATUS_RECOVERY] = {
        .name        = BOOST_NAME("Status Recovery"),
        .description = COMPOUND_STRING("Your Pokemon may shake off status each turn."),
        .type        = BOOST_TYPE_LEVELED,
        .maxLevel    = 3,
        .costs       = sBoostSharedCosts,
        .effects     = sBoostStatusRecoveryEffects,
        .effectFormat = COMPOUND_STRING("({STR_VAR_2}% per turn)"),
    },
    [BOOST_SPRAY_DURATION] = {
        .name        = BOOST_NAME("Spray Duration"),
        .description = COMPOUND_STRING("Repels and Lures last for more steps."),
        .type        = BOOST_TYPE_LEVELED,
        .maxLevel    = 4,
        .costs       = sBoostSharedCosts,
        .effects     = sBoostSprayDurationEffects,
        .effectFormat = COMPOUND_STRING("(+{STR_VAR_2}% steps)"),
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
    [BOOST_SHINY_CHARM_START] = {
        .name        = BOOST_NAME("Shiny Charm"),
        .description = COMPOUND_STRING("Begin a new game with the Shiny Charm."),
        .type        = BOOST_TYPE_BINARY,
        .maxLevel    = 1,
        .costs       = sBoostSharedBinaryCosts,
        .effects     = NULL,
    },
    [BOOST_ABILITY_CAPSULE_START] = {
        .name        = BOOST_NAME("Ability Capsule"),
        .description = COMPOUND_STRING("Begin a new game with an Ability Capsule."),
        .type        = BOOST_TYPE_BINARY,
        .maxLevel    = 1,
        .costs       = sBoostSharedBinaryCosts,
        .effects     = NULL,
    },
    [BOOST_ABILITY_PATCH_START] = {
        .name        = BOOST_NAME("Ability Patch"),
        .description = COMPOUND_STRING("Begin a new game with an Ability Patch."),
        .type        = BOOST_TYPE_BINARY,
        .maxLevel    = 1,
        .costs       = sBoostSharedBinaryCosts,
        .effects     = NULL,
    },
    [BOOST_CONSUMABLE_SAVE] = {
        .name        = BOOST_NAME("Frugal Use"),
        .description = COMPOUND_STRING("Chance a used consumable isn't spent."),
        .type        = BOOST_TYPE_LEVELED,
        .maxLevel    = 3,
        .costs       = sBoostSharedCosts,
        .effects     = sBoostConsumableSaveEffects,
        .effectFormat = COMPOUND_STRING("({STR_VAR_2}% chance)"),
    },
    [BOOST_EGG_IV_REROLL] = {
        .name        = BOOST_NAME("Egg IV Reroll"),
        .description = COMPOUND_STRING("Hatched eggs get the best of several IV rolls."),
        .type        = BOOST_TYPE_LEVELED,
        .maxLevel    = 3,
        .costs       = sBoostSharedCosts,
        .effects     = sBoostIvRerollEffects,
        .effectFormat = COMPOUND_STRING("(+{STR_VAR_2} rerolls)"),
    },
    [BOOST_WILD_IV_REROLL] = {
        .name        = BOOST_NAME("Wild IV Reroll"),
        .description = COMPOUND_STRING("Wild Pokemon get the best of several IV rolls."),
        .type        = BOOST_TYPE_LEVELED,
        .maxLevel    = 3,
        .costs       = sBoostSharedCosts,
        .effects     = sBoostIvRerollEffects,
        .effectFormat = COMPOUND_STRING("(+{STR_VAR_2} rerolls)"),
    },
    [BOOST_SHOP_DISCOUNT] = {
        .name        = BOOST_NAME("Shop Discount"),
        .description = COMPOUND_STRING("Reduces prices at the Poke Mart."),
        .type        = BOOST_TYPE_LEVELED,
        .maxLevel    = 3,
        .costs       = sBoostSharedCosts,
        .effects     = sBoostShopDiscountEffects,
        .effectFormat = COMPOUND_STRING("(-{STR_VAR_2}% price)"),
    },
    [BOOST_SURVIVE_1HP] = {
        .name        = BOOST_NAME("Second Wind"),
        .description = COMPOUND_STRING("Chance to survive a KO hit with 1 HP."),
        .type        = BOOST_TYPE_LEVELED,
        .maxLevel    = 3,
        .costs       = sBoostSharedCosts,
        .effects     = sBoostSurvive1HpEffects,
        .effectFormat = COMPOUND_STRING("({STR_VAR_2}% chance)"),
    },
    [BOOST_POST_BATTLE_HEAL] = {
        .name        = BOOST_NAME("Battle Recovery"),
        .description = COMPOUND_STRING("Party recovers HP after trainer battles."),
        .type        = BOOST_TYPE_LEVELED,
        .maxLevel    = 3,
        .costs       = sBoostSharedCosts,
        .effects     = sBoostPostBattleHealEffects,
        .effectFormat = COMPOUND_STRING("({STR_VAR_2}% HP)"),
    },
};

STATIC_ASSERT(BOOSTS_COUNT <= MAX_BOOSTS, BoostCountFitsProfile);
