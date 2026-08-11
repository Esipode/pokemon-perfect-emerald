// One entry per enum AchievementId (constants/achievements.h), keyed by
// designated initializer. Included from src/achievements.c only; nothing
// else should reference gAchievements directly -- go through the public API
// in include/achievements.h instead.
//
// Matches the src/data/items.h:632 / src/data/moves_info.h:46 convention:
// designated initializers keyed by ID, .name wrapped in a *_NAME() macro that
// enforces the length cap at compile time, .description left as a plain
// COMPOUND_STRING (no cap).
//
// Every .points value below is scaled by a single factor (~2.317x) from the
// raw per-tier values the catalog originally shipped with, so the catalog's
// total (20,000) lands on the same figure as maxing every boost in
// src/data/achievement_boosts.h -- see that file's own note on the scaling.
// Scaling preserves every entry's relative difficulty ranking (the tier
// system -- Bronze < Silver < Gold < Diamond -- already tracked actual
// difficulty reasonably well); a handful of entries that were made
// meaningfully harder without revisiting their tier (the "full 6-Pokemon
// team" rewrites) were re-tiered first, so the scale applies to already-
// corrected relative values, not the stale ones. Values are rounded to the
// nearest 5 for readability, with a handful adjusted by a further +/-5 to
// close the last few points of rounding error against the 20,000 target
// exactly.
#define ACHIEVEMENT_NAME(str) COMPOUND_STRING_SIZE_LIMIT(str, ACHIEVEMENT_NAME_LENGTH)

static const struct Achievement gAchievements[ACHIEVEMENTS_COUNT] =
{
    [ACHIEVEMENT_NONE] = {
        .name        = ACHIEVEMENT_NAME("-"),
        .description = COMPOUND_STRING(""),
        .tier        = ACHIEVEMENT_TIER_BRONZE,
        .scope       = ACHIEVEMENT_SCOPE_PERSISTENT_PROFILE,
        .category    = ACHIEVEMENT_CATEGORY_ADVENTURE,
        .points      = 0,
        .hidden      = TRUE,
    },

    // See constants/achievements.h for the category breakdown and
    // src/achievements.c for each category's hook.

    // A. Badges & Story
    [ACHIEVEMENT_STORY_RIVAL_ROUTE103] = {
        .name        = ACHIEVEMENT_NAME("Rival Rumble"),
        .description = COMPOUND_STRING("Battle your rival on Route 103."),
        .tier        = ACHIEVEMENT_TIER_BRONZE,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_ADVENTURE,
        .points      = 25,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_BADGE_STONE] = {
        .name        = ACHIEVEMENT_NAME("Stone Badge"),
        .description = COMPOUND_STRING("Defeat the Rustboro Gym Leader."),
        .tier        = ACHIEVEMENT_TIER_BRONZE,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_ADVENTURE,
        .points      = 25,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_STORY_PETALBURG_WOODS] = {
        .name        = ACHIEVEMENT_NAME("Trouble in the Woods"),
        .description = COMPOUND_STRING("Drive off the grunt in Petalburg Woods."),
        .tier        = ACHIEVEMENT_TIER_BRONZE,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_ADVENTURE,
        .points      = 25,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_BADGE_KNUCKLE] = {
        .name        = ACHIEVEMENT_NAME("Knuckle Badge"),
        .description = COMPOUND_STRING("Defeat the Dewford Gym Leader."),
        .tier        = ACHIEVEMENT_TIER_BRONZE,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_ADVENTURE,
        .points      = 35,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_BADGE_DYNAMO] = {
        .name        = ACHIEVEMENT_NAME("Dynamo Badge"),
        .description = COMPOUND_STRING("Defeat the Mauville Gym Leader."),
        .tier        = ACHIEVEMENT_TIER_BRONZE,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_ADVENTURE,
        .points      = 35,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_BADGE_HEAT] = {
        .name        = ACHIEVEMENT_NAME("Heat Badge"),
        .description = COMPOUND_STRING("Defeat the Lavaridge Gym Leader."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_ADVENTURE,
        .points      = 45,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_BADGE_BALANCE] = {
        .name        = ACHIEVEMENT_NAME("Balance Badge"),
        .description = COMPOUND_STRING("Defeat the Petalburg Gym Leader."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_ADVENTURE,
        .points      = 45,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_STORY_AQUA_HIDEOUT] = {
        .name        = ACHIEVEMENT_NAME("Submarine Chase"),
        .description = COMPOUND_STRING("Chase Team Aqua out of their hideout."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_ADVENTURE,
        .points      = 60,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_STORY_MT_PYRE] = {
        .name        = ACHIEVEMENT_NAME("Orb Retrieved"),
        .description = COMPOUND_STRING("Recover the stolen orb atop Mt. Pyre."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_ADVENTURE,
        .points      = 60,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_STORY_MAGMA_HIDEOUT] = {
        .name        = ACHIEVEMENT_NAME("Hideout Showdown"),
        .description = COMPOUND_STRING("Clear out the Magma Hideout."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_ADVENTURE,
        .points      = 60,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_BADGE_FEATHER] = {
        .name        = ACHIEVEMENT_NAME("Feather Badge"),
        .description = COMPOUND_STRING("Defeat the Fortree Gym Leader."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_ADVENTURE,
        .points      = 60,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_STORY_SEAFLOOR_CAVERN] = {
        .name        = ACHIEVEMENT_NAME("Seafloor Standoff"),
        .description = COMPOUND_STRING("Confront Team Aqua in the Seafloor Cavern."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_ADVENTURE,
        .points      = 70,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_BADGE_MIND] = {
        .name        = ACHIEVEMENT_NAME("Mind Badge"),
        .description = COMPOUND_STRING("Defeat the Mossdeep Gym Leader."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_ADVENTURE,
        .points      = 80,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_BADGE_RAIN] = {
        .name        = ACHIEVEMENT_NAME("Rain Badge"),
        .description = COMPOUND_STRING("Defeat the Sootopolis Gym Leader."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_ADVENTURE,
        .points      = 95,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_STORY_CHAMPION] = {
        .name        = ACHIEVEMENT_NAME("Champion of Hoenn"),
        .description = COMPOUND_STRING("Defeat the Champion and enter the Hall of Fame."),
        .tier        = ACHIEVEMENT_TIER_DIAMOND,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_ADVENTURE,
        .points      = 250,
        .hidden      = FALSE,
    },

    // B. Pokédex
    [ACHIEVEMENT_DEX_SEEN_10] = {
        .name        = ACHIEVEMENT_NAME("Familiar Faces"),
        .description = COMPOUND_STRING("See 10% of the Pokédex."),
        .tier        = ACHIEVEMENT_TIER_BRONZE,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_COLLECTION,
        .points      = 25,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_DEX_SEEN_25] = {
        .name        = ACHIEVEMENT_NAME("Field Notes"),
        .description = COMPOUND_STRING("See 25% of the Pokédex."),
        .tier        = ACHIEVEMENT_TIER_BRONZE,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_COLLECTION,
        .points      = 35,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_DEX_SEEN_50] = {
        .name        = ACHIEVEMENT_NAME("Field Researcher"),
        .description = COMPOUND_STRING("See 50% of the Pokédex."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_COLLECTION,
        .points      = 45,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_DEX_SEEN_100] = {
        .name        = ACHIEVEMENT_NAME("Seen 'Em All"),
        .description = COMPOUND_STRING("See every Pokémon in the Pokédex."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_COLLECTION,
        .points      = 95,
        .hidden      = FALSE,
    },

    // C. Captures
    // The old percentage-based Pokedex-caught ladder (10/25/50/100%) and
    // the old raw-count ladder (1/25/100/250/500) are collapsed into this
    // single hard-number ladder, one entry per tier.
    [ACHIEVEMENT_CATCH_100] = {
        .name        = ACHIEVEMENT_NAME("Serial Catcher"),
        .description = COMPOUND_STRING("Catch 100 Pokémon."),
        .tier        = ACHIEVEMENT_TIER_BRONZE,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_COLLECTION,
        .points      = 35,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_CATCH_350] = {
        .name        = ACHIEVEMENT_NAME("Master Catcher"),
        .description = COMPOUND_STRING("Catch 350 Pokémon."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_COLLECTION,
        .points      = 70,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_CATCH_700] = {
        .name        = ACHIEVEMENT_NAME("Catch Legend"),
        .description = COMPOUND_STRING("Catch 700 Pokémon."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_COLLECTION,
        .points      = 115,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_CATCH_ALL] = {
        .name        = ACHIEVEMENT_NAME("Gotta Catch 'Em All"),
        .description = COMPOUND_STRING("Catch every Pokémon in the Pokédex."),
        .tier        = ACHIEVEMENT_TIER_DIAMOND,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_COLLECTION,
        .points      = 250,
        .hidden      = FALSE,
    },

    // D. Shiny
    [ACHIEVEMENT_SHINY_1] = {
        .name        = ACHIEVEMENT_NAME("Shiny Debut"),
        .description = COMPOUND_STRING("Obtain your first shiny Pokémon."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_PERSISTENT_PROFILE,
        .category    = ACHIEVEMENT_CATEGORY_COLLECTION,
        .points      = 60,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_SHINY_5] = {
        .name        = ACHIEVEMENT_NAME("Shiny Hunter"),
        .description = COMPOUND_STRING("Obtain 5 shiny Pokémon."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_PERSISTENT_PROFILE,
        .category    = ACHIEVEMENT_CATEGORY_COLLECTION,
        .points      = 115,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_SHINY_25] = {
        .name        = ACHIEVEMENT_NAME("Shiny Sensei"),
        .description = COMPOUND_STRING("Obtain 25 shiny Pokémon."),
        .tier        = ACHIEVEMENT_TIER_DIAMOND,
        .scope       = ACHIEVEMENT_SCOPE_PERSISTENT_PROFILE,
        .category    = ACHIEVEMENT_CATEGORY_COLLECTION,
        .points      = 250,
        .hidden      = FALSE,
    },

    // E. Trainers Defeated
    [ACHIEVEMENT_TRAINERS_10] = {
        .name        = ACHIEVEMENT_NAME("Trainer Trouncer"),
        .description = COMPOUND_STRING("Battle 10 Trainers."),
        .tier        = ACHIEVEMENT_TIER_BRONZE,
        .scope       = ACHIEVEMENT_SCOPE_PERSISTENT_PROFILE,
        .category    = ACHIEVEMENT_CATEGORY_BATTLE,
        .points      = 25,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_TRAINERS_50] = {
        .name        = ACHIEVEMENT_NAME("Veteran Battler"),
        .description = COMPOUND_STRING("Battle 50 Trainers."),
        .tier        = ACHIEVEMENT_TIER_BRONZE,
        .scope       = ACHIEVEMENT_SCOPE_PERSISTENT_PROFILE,
        .category    = ACHIEVEMENT_CATEGORY_BATTLE,
        .points      = 45,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_TRAINERS_150] = {
        .name        = ACHIEVEMENT_NAME("Battle Hardened"),
        .description = COMPOUND_STRING("Battle 150 Trainers."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_PERSISTENT_PROFILE,
        .category    = ACHIEVEMENT_CATEGORY_BATTLE,
        .points      = 80,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_TRAINERS_300] = {
        .name        = ACHIEVEMENT_NAME("Battle Legend"),
        .description = COMPOUND_STRING("Battle 300 Trainers."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_PERSISTENT_PROFILE,
        .category    = ACHIEVEMENT_CATEGORY_BATTLE,
        .points      = 140,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_TRAINERS_500] = {
        .name        = ACHIEVEMENT_NAME("Unstoppable Force"),
        .description = COMPOUND_STRING("Battle 500 Trainers."),
        .tier        = ACHIEVEMENT_TIER_DIAMOND,
        .scope       = ACHIEVEMENT_SCOPE_PERSISTENT_PROFILE,
        .category    = ACHIEVEMENT_CATEGORY_BATTLE,
        .points      = 215,
        .hidden      = FALSE,
    },

    // F. Wild Battles
    [ACHIEVEMENT_WILD_BATTLES_50] = {
        .name        = ACHIEVEMENT_NAME("Into the Wild"),
        .description = COMPOUND_STRING("Battle 50 wild Pokémon."),
        .tier        = ACHIEVEMENT_TIER_BRONZE,
        .scope       = ACHIEVEMENT_SCOPE_PERSISTENT_PROFILE,
        .category    = ACHIEVEMENT_CATEGORY_BATTLE,
        .points      = 25,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_WILD_BATTLES_250] = {
        .name        = ACHIEVEMENT_NAME("Wilderness Expert"),
        .description = COMPOUND_STRING("Battle 250 wild Pokémon."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_PERSISTENT_PROFILE,
        .category    = ACHIEVEMENT_CATEGORY_BATTLE,
        .points      = 60,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_WILD_BATTLES_500] = {
        .name        = ACHIEVEMENT_NAME("One with the Wild"),
        .description = COMPOUND_STRING("Battle 500 wild Pokémon."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_PERSISTENT_PROFILE,
        .category    = ACHIEVEMENT_CATEGORY_BATTLE,
        .points      = 105,
        .hidden      = FALSE,
    },

    // G. Items
    [ACHIEVEMENT_ITEM_MASTER_BALL] = {
        .name        = ACHIEVEMENT_NAME("Just in Case"),
        .description = COMPOUND_STRING("Obtain a Master Ball."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_COLLECTION,
        .points      = 45,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_ITEM_RARE_CANDY] = {
        .name        = ACHIEVEMENT_NAME("Sugar Rush"),
        .description = COMPOUND_STRING("Obtain a Rare Candy."),
        .tier        = ACHIEVEMENT_TIER_BRONZE,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_COLLECTION,
        .points      = 25,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_ITEM_PP_UP] = {
        .name        = ACHIEVEMENT_NAME("Power Booster"),
        .description = COMPOUND_STRING("Obtain a PP Up."),
        .tier        = ACHIEVEMENT_TIER_BRONZE,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_COLLECTION,
        .points      = 35,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_ITEM_HEART_SCALE] = {
        .name        = ACHIEVEMENT_NAME("Move Tutor's Friend"),
        .description = COMPOUND_STRING("Obtain a Heart Scale."),
        .tier        = ACHIEVEMENT_TIER_BRONZE,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_COLLECTION,
        .points      = 25,
        .hidden      = FALSE,
    },

    // H. Money
    [ACHIEVEMENT_MONEY_10K] = {
        .name        = ACHIEVEMENT_NAME("Small Fortune"),
        .description = COMPOUND_STRING("Accumulate ¥10,000."),
        .tier        = ACHIEVEMENT_TIER_BRONZE,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_ECONOMY,
        .points      = 25,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_MONEY_100K] = {
        .name        = ACHIEVEMENT_NAME("Wealthy Trainer"),
        .description = COMPOUND_STRING("Accumulate ¥100,000."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_ECONOMY,
        .points      = 70,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_MONEY_MAX] = {
        .name        = ACHIEVEMENT_NAME("Richest in Hoenn"),
        .description = COMPOUND_STRING("Max out your money."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_ECONOMY,
        .points      = 125,
        .hidden      = FALSE,
    },

    // I. Eggs
    [ACHIEVEMENT_EGG_1] = {
        .name        = ACHIEVEMENT_NAME("It's Hatching!"),
        .description = COMPOUND_STRING("Hatch your first egg."),
        .tier        = ACHIEVEMENT_TIER_BRONZE,
        .scope       = ACHIEVEMENT_SCOPE_PERSISTENT_PROFILE,
        .category    = ACHIEVEMENT_CATEGORY_COLLECTION,
        .points      = 25,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_EGG_10] = {
        .name        = ACHIEVEMENT_NAME("Daycare Regular"),
        .description = COMPOUND_STRING("Hatch 10 eggs."),
        .tier        = ACHIEVEMENT_TIER_BRONZE,
        .scope       = ACHIEVEMENT_SCOPE_PERSISTENT_PROFILE,
        .category    = ACHIEVEMENT_CATEGORY_COLLECTION,
        .points      = 45,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_EGG_50] = {
        .name        = ACHIEVEMENT_NAME("Egg Factory"),
        .description = COMPOUND_STRING("Hatch 50 eggs."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_PERSISTENT_PROFILE,
        .category    = ACHIEVEMENT_CATEGORY_COLLECTION,
        .points      = 80,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_EGG_SHINY] = {
        .name        = ACHIEVEMENT_NAME("Shiny From the Shell"),
        .description = COMPOUND_STRING("Hatch a shiny Pokémon."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_COLLECTION,
        .points      = 115,
        .hidden      = FALSE,
    },

    // J. Multi-Run / Persistent Profile
    // ACHIEVEMENT_PLAYTHROUGHS_2 ("Second Wind", complete
    // the game from a fresh save 2 times) and ACHIEVEMENT_PLAYTHROUGHS_5
    // ("Serial Champion", 5 times) removed -- see
    // Achievement_OnFirstPlaythroughComplete (src/achievements.c).
    // playthroughsCompleted itself is untouched: no achievement reads it
    // anymore after this and the Frequent Flyer/Veteran Trainer/Resident
    // Champion removals below (category Q), but it's still shown on the
    // debug profile dump (src/debug.c) independent of any achievement, so
    // the counter stays live.
    // Replaces the old seven-entry NG+ repeat-count ladder
    // (ACHIEVEMENT_NG_PLUS_STARTED/_CYCLE_3/_CYCLE_5/_COMPLETED_3 here, plus
    // _ONE_MORE_TIME/_BEYOND_THE_BEGINNING/_ESCALATION in category O) with a
    // single "beat one NG+ cycle" achievement -- checked, unconditionally,
    // in Achievement_OnNewGamePlusCycleCompleted. Gold/50 to match
    // ACHIEVEMENT_NUZLOCKE_1 below: both represent "fully beat the game
    // again, under one specific added condition."
    [ACHIEVEMENT_NG_PLUS_CYCLE_COMPLETE] = {
        .name        = ACHIEVEMENT_NAME("One More Time"),
        .description = COMPOUND_STRING("Complete a New Game+ cycle."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_PERSISTENT_PROFILE,
        .category    = ACHIEVEMENT_CATEGORY_NG_PLUS,
        .points      = 115,
        .hidden      = FALSE,
    },
    // ACHIEVEMENT_NUZLOCKE_3 ("complete 3 Nuzlocke runs")
    // removed -- this is already the "do it once" version of that ladder.
    [ACHIEVEMENT_NUZLOCKE_1] = {
        .name        = ACHIEVEMENT_NAME("Survivor"),
        .description = COMPOUND_STRING("Complete a Nuzlocke run."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_PERSISTENT_PROFILE,
        .category    = ACHIEVEMENT_CATEGORY_NUZLOCKE,
        .points      = 115,
        .hidden      = FALSE,
    },
    // ACHIEVEMENT_RANDOMIZER_SEED_EXPLORER (2 randomized
    // playthroughs) and _VETERAN (5), category O, removed -- this is already
    // the "do it once" version of that ladder.
    [ACHIEVEMENT_RANDOMIZED_1] = {
        .name        = ACHIEVEMENT_NAME("Into the Unknown"),
        .description = COMPOUND_STRING("Complete a randomized run."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_PERSISTENT_PROFILE,
        .category    = ACHIEVEMENT_CATEGORY_RANDOMIZER,
        .points      = 70,
        .hidden      = FALSE,
    },
    // Scaled up from 1,000 to 2,000 -- see Achievement_CheckPointMilestones
    // (src/achievements.c)'s own comment on the point-total rescale.
    [ACHIEVEMENT_POINTS_2000] = {
        .name        = ACHIEVEMENT_NAME("Point Collector"),
        .description = COMPOUND_STRING("Earn 2,000 total achievement points."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_PERSISTENT_PROFILE,
        .category    = ACHIEVEMENT_CATEGORY_PROFILE,
        .points      = 115,
        .hidden      = FALSE,
    },

    // Category K: Battle Mastery. See constants/achievements.h for the ID
    // list and src/achievements.c for struct AchievementBattleData and
    // Achievement_CheckBattleMilestones. Every entry below is
    // ACHIEVEMENT_SCOPE_CURRENT_RUN: the underlying data is a per-battle
    // EWRAM struct that resets every single battle, an even tighter cadence
    // than "current run" names, but there's no narrower scope value and
    // CURRENT_RUN is the closest fit -- these are also its first real
    // consumers (the scope has existed since early on with nothing using it
    // until now).

    [ACHIEVEMENT_BATTLE_CRITICAL_SUCCESS] = {
        .name        = ACHIEVEMENT_NAME("Critical Success"),
        .description = COMPOUND_STRING("Land a critical hit."),
        .tier        = ACHIEVEMENT_TIER_BRONZE,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_BATTLE,
        .points      = 25,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_BATTLE_TYPE_ADVANTAGE] = {
        .name        = ACHIEVEMENT_NAME("Type Advantage"),
        .description = COMPOUND_STRING("Win a battle after landing a super-effective hit."),
        .tier        = ACHIEVEMENT_TIER_BRONZE,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_BATTLE,
        .points      = 25,
        .hidden      = FALSE,
    },
    // ACHIEVEMENT_BATTLE_TYPE_MASTER ("win a trainer battle
    // without landing a super-effective hit") removed here -- most trainer
    // teams aren't built to counter the player, so this happens by chance
    // rather than deliberate effort. See include/constants/achievements.h's
    // category K comment.
    // Now requires the opponent to field a full 6-Pokemon
    // team (see Achievement_CheckBattleMilestones in src/achievements.c) --
    // was trivially easy against the many trainers who only carry one or two.
    // Scaled up from Silver/25 to Gold/45 -- the full-6-Pokemon-
    // opponent requirement made this meaningfully harder than its old Silver
    // points reflected.
    [ACHIEVEMENT_BATTLE_CLEAN_SWEEP] = {
        .name        = ACHIEVEMENT_NAME("Clean Sweep"),
        .description = COMPOUND_STRING("Defeat a trainer's full team of 6 Pokémon with a single Pokémon."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_BATTLE,
        .points      = 105,
        .hidden      = FALSE,
    },
    // Same full-6-Pokemon-opponent requirement as
    // ACHIEVEMENT_BATTLE_CLEAN_SWEEP above -- even major trainers can carry
    // fewer than 6 early on.
    // Scaled up from 50 to 60 -- same full-6-Pokemon-opponent bump as Clean Sweep
    // above, plus this one's already-harder "major trainer" requirement.
    [ACHIEVEMENT_BATTLE_PERFECT_SWEEP] = {
        .name        = ACHIEVEMENT_NAME("Perfect Sweep"),
        .description = COMPOUND_STRING("Defeat a major trainer's full team of 6 Pokémon with a single Pokémon."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_BATTLE,
        .points      = 140,
        .hidden      = FALSE,
    },
    // Same full-6-Pokemon-opponent requirement.
    // Scaled up from Silver/25 to Gold/45 -- same full-6-Pokemon-opponent
    // bump as Clean Sweep above.
    [ACHIEVEMENT_BATTLE_NO_DAMAGE] = {
        .name        = ACHIEVEMENT_NAME("No Damage"),
        .description = COMPOUND_STRING("Win a battle against a trainer's full team of 6 Pokémon without any of your Pokémon taking damage."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_BATTLE,
        .points      = 105,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_BATTLE_UNTOUCHABLE] = {
        .name        = ACHIEVEMENT_NAME("Untouchable"),
        .description = COMPOUND_STRING("Win a major battle without any of your Pokémon taking damage."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_BATTLE,
        .points      = 115,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_BATTLE_STATUS_SPECIALIST] = {
        .name        = ACHIEVEMENT_NAME("Status Specialist"),
        .description = COMPOUND_STRING("Win a battle after inflicting a status condition."),
        .tier        = ACHIEVEMENT_TIER_BRONZE,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_BATTLE,
        .points      = 25,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_BATTLE_STATUS_MASTER] = {
        .name        = ACHIEVEMENT_NAME("Status Master"),
        .description = COMPOUND_STRING("Inflict three different status conditions in one battle and win."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_BATTLE,
        .points      = 60,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_BATTLE_WEATHER_REPORT] = {
        .name        = ACHIEVEMENT_NAME("Weather Report"),
        .description = COMPOUND_STRING("Win a battle with the weather still active."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_BATTLE,
        .points      = 45,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_BATTLE_WEATHER_MASTER] = {
        .name        = ACHIEVEMENT_NAME("Weather Master"),
        .description = COMPOUND_STRING("Win a major battle with the weather still active."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_BATTLE,
        .points      = 105,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_BATTLE_SETUP_SWEEP] = {
        .name        = ACHIEVEMENT_NAME("Setup Sweep"),
        .description = COMPOUND_STRING("Win a battle after using a stat-raising move."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_BATTLE,
        .points      = 60,
        .hidden      = FALSE,
    },
    // The check (sBattleData.setupThenKo, Achievement_RecordMoveUsed/
    // _RecordOpposingFaint) fires on any KO in the won battle, not
    // specifically the finishing blow.
    [ACHIEVEMENT_BATTLE_ONE_TURN_FINISH] = {
        .name        = ACHIEVEMENT_NAME("One-Turn Finish"),
        .description = COMPOUND_STRING("Have a Pokémon land a KO on its very next move right after using a stat-boosting move."),
        .tier        = ACHIEVEMENT_TIER_BRONZE,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_BATTLE,
        .points      = 10,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_BATTLE_PRIORITY_MATTERS] = {
        .name        = ACHIEVEMENT_NAME("Priority Matters"),
        .description = COMPOUND_STRING("Land the winning blow with a priority move."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_BATTLE,
        .points      = 60,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_BATTLE_SPEED_DEMON] = {
        .name        = ACHIEVEMENT_NAME("Speed Demon"),
        .description = COMPOUND_STRING("KO three opponents with one Pokémon without switching out."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_BATTLE,
        .points      = 60,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_BATTLE_ATTRITION] = {
        .name        = ACHIEVEMENT_NAME("Battle of Attrition"),
        .description = COMPOUND_STRING("Win a major battle that lasts 30 or more turns."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_BATTLE,
        .points      = 105,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_BATTLE_STRATEGIC_VICTORY] = {
        .name        = ACHIEVEMENT_NAME("Strategic Victory"),
        .description = COMPOUND_STRING("Beat a major boss's full team of 6 Pokémon without any of your Pokémon fainting."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_BATTLE,
        .points      = 125,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_BATTLE_REVERSE_SWEEP] = {
        .name        = ACHIEVEMENT_NAME("Reverse Sweep"),
        .description = COMPOUND_STRING("Win a trainer battle after losing at least half your team."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_BATTLE,
        .points      = 115,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_BATTLE_CHAMPION_TACTICIAN] = {
        .name        = ACHIEVEMENT_NAME("Champion Tactician"),
        .description = COMPOUND_STRING("Beat the Champion with four or more Pokémon in action."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_BATTLE,
        .points      = 140,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_BATTLE_MOVE_VARIETY] = {
        .name        = ACHIEVEMENT_NAME("Move Variety"),
        .description = COMPOUND_STRING("Win a major battle where every Pokémon that fought used 2+ moves."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_BATTLE,
        .points      = 105,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_BATTLE_NO_REPEATS] = {
        .name        = ACHIEVEMENT_NAME("No Repeats"),
        .description = COMPOUND_STRING("Win a trainer battle without using the same move twice in a row."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_BATTLE,
        .points      = 60,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_BATTLE_AGAINST_THE_ODDS] = {
        .name        = ACHIEVEMENT_NAME("Against the Odds"),
        .description = COMPOUND_STRING("Beat a major boss with a team at least 5 levels below theirs."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_BATTLE,
        .points      = 140,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_BATTLE_FOUR_MOVE_PHILOSOPHER] = {
        .name        = ACHIEVEMENT_NAME("Four-Move Philosopher"),
        .description = COMPOUND_STRING("Win after using all four of one Pokémon's moves."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_BATTLE,
        .points      = 60,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_BATTLE_NO_STAB_NEEDED] = {
        .name        = ACHIEVEMENT_NAME("No STAB Needed"),
        .description = COMPOUND_STRING("Win a battle against a trainer's full team of 6 Pokémon without a single same-type move."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_BATTLE,
        .points      = 60,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_BATTLE_COVERAGE_ENJOYER] = {
        .name        = ACHIEVEMENT_NAME("Coverage Enjoyer"),
        .description = COMPOUND_STRING("Beat a major opponent using four or more move types."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_BATTLE,
        .points      = 105,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_BATTLE_STATUS_HOARDER] = {
        .name        = ACHIEVEMENT_NAME("Status Hoarder"),
        .description = COMPOUND_STRING("Have two or more opponents faint from status damage in one battle."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_BATTLE,
        .points      = 105,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_BATTLE_THREE_PUNCH_FINISH] = {
        .name        = ACHIEVEMENT_NAME("Three-Punch Finish"),
        .description = COMPOUND_STRING("Have three different Pokémon land the last three KOs."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_BATTLE,
        .points      = 115,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_BATTLE_TEAM_PLAYER] = {
        .name        = ACHIEVEMENT_NAME("Team Player"),
        .description = COMPOUND_STRING("Win a battle in which all six of your Pokémon fought."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_BATTLE,
        .points      = 60,
        .hidden      = FALSE,
    },
    // Now requires a full 6-Pokemon party (see
    // Achievement_CheckBattleMilestones in src/achievements.c) -- was
    // trivial to earn by accident with only one or two Pokemon along.
    // Scaled up from Silver/25 to Gold/45 -- the full-6-Pokemon-party
    // requirement made this meaningfully harder than its old Silver points
    // reflected.
    [ACHIEVEMENT_BATTLE_COMEBACK_KID] = {
        .name        = ACHIEVEMENT_NAME("Comeback Kid"),
        .description = COMPOUND_STRING("Win a battle with a full team of 6 Pokémon, with only one still conscious at the end."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_BATTLE,
        .points      = 105,
        .hidden      = FALSE,
    },
    // Same full-6-Pokemon-party requirement as
    // ACHIEVEMENT_BATTLE_COMEBACK_KID above.
    // Scaled up from 40 to 55 -- same full-6-Pokemon-party bump as Comeback Kid
    // above, plus this one's tighter 10%-HP threshold.
    [ACHIEVEMENT_BATTLE_LAST_ONE_STANDING] = {
        .name        = ACHIEVEMENT_NAME("Last One Standing"),
        .description = COMPOUND_STRING("Win a battle with a full team of 6 Pokémon, with your last one at 10% HP or less."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_BATTLE,
        .points      = 125,
        .hidden      = FALSE,
    },

    // Category L. All ACHIEVEMENT_SCOPE_CURRENT_RUN -- struct
    // AchievementRunData resets every new game and every New Game+ cycle,
    // and every condition below is meaningless once carried across a reset.
    // Now requires a full 6-Pokemon party (see
    // Achievement_CheckTeamMilestones in src/achievements.c) -- was trivial
    // to keep 1-2 Pokemon mono-type by accident.
    // Scaled up from Bronze/15 to Silver/25 -- the full-6-Pokemon-
    // party requirement made assembling a whole mono-type team this early
    // meaningfully harder than its old Bronze points reflected.
    [ACHIEVEMENT_TEAM_MONO_TYPE_TRIAL] = {
        .name        = ACHIEVEMENT_NAME("Mono-Type Trial"),
        .description = COMPOUND_STRING("Win a Gym battle with a full team of 6 Pokémon, all sharing a type."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_TEAM,
        .points      = 60,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_TEAM_ONE_TYPE_JOURNEY] = {
        .name        = ACHIEVEMENT_NAME("One-Type Journey"),
        .description = COMPOUND_STRING("Clear four Gym battles with a mono-type party."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_TEAM,
        .points      = 70,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_TEAM_MONO_TYPE_CHAMPION] = {
        .name        = ACHIEVEMENT_NAME("Mono-Type Champion"),
        .description = COMPOUND_STRING("Complete the story with a mono-type party the whole way."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_TEAM,
        .points      = 140,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_TEAM_TRIAL_BY_FIRE] = {
        .name        = ACHIEVEMENT_NAME("Trial by Fire"),
        .description = COMPOUND_STRING("Complete a mono-type story run on Hard difficulty."),
        .tier        = ACHIEVEMENT_TIER_DIAMOND,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_TEAM,
        .points      = 225,
        .hidden      = FALSE,
    },
    // Now requires a full 6-Pokemon party (see
    // Achievement_CheckTeamMilestones in src/achievements.c) -- was trivial
    // for no two party members to share a type with barely any party to
    // begin with.
    // Scaled up from Bronze/15 to Silver/25 -- same full-6-Pokemon-party
    // bump as Mono-Type Trial above.
    [ACHIEVEMENT_TEAM_NO_DUPLICATES] = {
        .name        = ACHIEVEMENT_NAME("No Duplicates"),
        .description = COMPOUND_STRING("Win a major battle with a full team of 6 Pokémon, no two sharing a type."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_TEAM,
        .points      = 60,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_TEAM_SIX_OF_A_KIND] = {
        .name        = ACHIEVEMENT_NAME("Six of a Kind"),
        .description = COMPOUND_STRING("Win a major battle with six Pokémon sharing a type."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_TEAM,
        .points      = 70,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_TEAM_UNDERSTUDY] = {
        .name        = ACHIEVEMENT_NAME("Understudy"),
        .description = COMPOUND_STRING("Win a Gym battle without your highest-level Pokémon being used."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_TEAM,
        .points      = 60,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_TEAM_BENCHWARMER] = {
        .name        = ACHIEVEMENT_NAME("Benchwarmer"),
        .description = COMPOUND_STRING("Win a major battle using only Pokémon that didn't act in the previous one."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_TEAM,
        .points      = 105,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_TEAM_BOX_ROTATION] = {
        .name        = ACHIEVEMENT_NAME("Box Rotation"),
        .description = COMPOUND_STRING("Use 12 different species in major battles in one run."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_TEAM,
        .points      = 70,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_TEAM_DEEP_BENCH] = {
        .name        = ACHIEVEMENT_NAME("Deep Bench"),
        .description = COMPOUND_STRING("Use 18 different species in major battles in one run."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_TEAM,
        .points      = 115,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_TEAM_FULL_ROTATION] = {
        .name        = ACHIEVEMENT_NAME("Full Rotation"),
        .description = COMPOUND_STRING("Use 30 different species in major battles in one run."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_TEAM,
        .points      = 155,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_TEAM_NO_ACE] = {
        .name        = ACHIEVEMENT_NAME("No Ace"),
        .description = COMPOUND_STRING("Clear a Gym with your highest-level Pokémon not in the party."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_TEAM,
        .points      = 60,
        .hidden      = FALSE,
    },
    // The check compares your party's type composition against the
    // immediately PREVIOUS Gym's, every single Gym, not just "at some point."
    [ACHIEVEMENT_TEAM_TYPE_ROULETTE] = {
        .name        = ACHIEVEMENT_NAME("Type Roulette"),
        .description = COMPOUND_STRING("Clear all 8 Gyms with your party's type makeup different from the previous Gym's, every time."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_TEAM,
        .points      = 115,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_TEAM_WELL_EQUIPPED] = {
        .name        = ACHIEVEMENT_NAME("Well Equipped"),
        .description = COMPOUND_STRING("Have all six party members holding an item at once."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_TEAM,
        .points      = 60,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_TEAM_FULL_HOUSE] = {
        .name        = ACHIEVEMENT_NAME("Full House"),
        .description = COMPOUND_STRING("Have six Pokémon at or above the current level cap at once."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_TEAM,
        .points      = 105,
        .hidden      = FALSE,
    },
    // ACHIEVEMENT_TEAM_VARIETY_IS_POWER ("win a major battle
    // without two of the same species") removed here -- most players never
    // deliberately catch duplicate species for their party, so this is true
    // of nearly every team without any effort. See
    // include/constants/achievements.h's category L comment.
    [ACHIEVEMENT_TEAM_LINK_IN_THE_CHAIN] = {
        .name        = ACHIEVEMENT_NAME("Link in the Chain"),
        .description = COMPOUND_STRING("Have three members of one evolution family in your party at once."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_TEAM,
        .points      = 60,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_TEAM_DREAM_TEAM] = {
        .name        = ACHIEVEMENT_NAME("Dream Team"),
        .description = COMPOUND_STRING("Complete the story with no two party members sharing a primary type."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_TEAM,
        .points      = 140,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_TEAM_EVERYONE_GETS_A_TURN] = {
        .name        = ACHIEVEMENT_NAME("Everyone Gets a Turn"),
        .description = COMPOUND_STRING("Use 24 different species in major battles in one run."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_TEAM,
        .points      = 125,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_TEAM_REBUILD] = {
        .name        = ACHIEVEMENT_NAME("Rebuild"),
        .description = COMPOUND_STRING("Replace four party members between two consecutive Gyms and still finish the story."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_TEAM,
        .points      = 105,
        .hidden      = FALSE,
    },
    // Your final party's species
    // must share none in common with the party you had right after
    // clearing the 4th Gym specifically.
    [ACHIEVEMENT_TEAM_RADICAL_REBUILD] = {
        .name        = ACHIEVEMENT_NAME("Radical Rebuild"),
        .description = COMPOUND_STRING("Finish the story with a final party that shares no species with the party you had right after the 4th Gym."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_TEAM,
        .points      = 140,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_TEAM_CAPPED_OUT] = {
        .name        = ACHIEVEMENT_NAME("Capped Out"),
        .description = COMPOUND_STRING("Finish the story with no party member ever above the level cap."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_TEAM,
        .points      = 115,
        .hidden      = FALSE,
    },
    // Now requires a full 6-Pokemon party (see
    // Achievement_CheckTeamMilestones in src/achievements.c) -- a small
    // party trivially has a low combined base stat total.
    // Scaled up from Silver/30 to Gold/45 -- the full-6-Pokemon-party
    // requirement made this a genuine deliberate-underdog build, not just a
    // side effect of a small party, harder than its old Silver points
    // reflected.
    [ACHIEVEMENT_TEAM_FEATHERWEIGHT] = {
        .name        = ACHIEVEMENT_NAME("Featherweight"),
        .description = COMPOUND_STRING("Clear a Gym with a full team of 6 Pokémon whose combined base stat total is under 1800."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_TEAM,
        .points      = 105,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_TEAM_UNDERDOG_RUN] = {
        .name        = ACHIEVEMENT_NAME("Underdog Run"),
        .description = COMPOUND_STRING("Finish the story with no party member ever above 450 base stat total."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_TEAM,
        .points      = 125,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_TEAM_DIVERSE_ROOTS] = {
        .name        = ACHIEVEMENT_NAME("Diverse Roots"),
        .description = COMPOUND_STRING("Win a major battle with six different egg groups represented."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_TEAM,
        .points      = 60,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_TEAM_FRESH_START] = {
        .name        = ACHIEVEMENT_NAME("Fresh Start"),
        .description = COMPOUND_STRING("Clear a Gym with six Pokémon all obtained since the previous Gym."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_TEAM,
        .points      = 60,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_TEAM_SAME_SIX] = {
        .name        = ACHIEVEMENT_NAME("Same Six"),
        .description = COMPOUND_STRING("Clear all eight Gyms without ever changing your party's six species."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_TEAM,
        .points      = 70,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_TEAM_BALANCED_ROSTER] = {
        .name        = ACHIEVEMENT_NAME("Balanced Roster"),
        .description = COMPOUND_STRING("Complete the story with a party covering ten or more different types."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_TEAM,
        .points      = 105,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_TEAM_NOBODY_BENCHED] = {
        .name        = ACHIEVEMENT_NAME("Nobody Benched"),
        .description = COMPOUND_STRING("Have every party member act in every Gym battle of one playthrough."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_TEAM,
        .points      = 70,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_TEAM_ACE_ROTATION] = {
        .name        = ACHIEVEMENT_NAME("Ace Rotation"),
        .description = COMPOUND_STRING("Have a different party member score the final KO in each of the eight Gym battles."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_TEAM,
        .points      = 115,
        .hidden      = FALSE,
    },

    // M. Exploration, Economy & Collection (30)
    [ACHIEVEMENT_EXPLORE_FIRST_STEPS_ABROAD] = {
        .name        = ACHIEVEMENT_NAME("First Steps Abroad"),
        .description = COMPOUND_STRING("Enter 30 different areas."),
        .tier        = ACHIEVEMENT_TIER_BRONZE,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_EXPLORATION,
        .points      = 25,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_EXPLORE_OFF_THE_BEATEN_PATH] = {
        .name        = ACHIEVEMENT_NAME("Off the Beaten Path"),
        .description = COMPOUND_STRING("Enter 70 different areas."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_EXPLORATION,
        .points      = 60,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_EXPLORE_CARTOGRAPHER] = {
        .name        = ACHIEVEMENT_NAME("Cartographer"),
        .description = COMPOUND_STRING("Enter 100 different areas in one playthrough."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_EXPLORATION,
        .points      = 105,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_EXPLORE_COMPLETIONIST_TOURIST] = {
        .name        = ACHIEVEMENT_NAME("Completionist Tourist"),
        .description = COMPOUND_STRING("Visit every town and city before entering the Pokémon League."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_EXPLORATION,
        .points      = 70,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_EXPLORE_ON_THE_ROAD] = {
        .name        = ACHIEVEMENT_NAME("On the Road"),
        .description = COMPOUND_STRING("Visit five towns or cities."),
        .tier        = ACHIEVEMENT_TIER_BRONZE,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_ADVENTURE,
        .points      = 25,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_EXPLORE_TREASURE_HUNTER] = {
        .name        = ACHIEVEMENT_NAME("Treasure Hunter"),
        .description = COMPOUND_STRING("Find 20 hidden items."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_PERSISTENT_PROFILE,
        .category    = ACHIEVEMENT_CATEGORY_EXPLORATION,
        .points      = 60,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_EXPLORE_TREASURE_HOARD] = {
        .name        = ACHIEVEMENT_NAME("Treasure Hoard"),
        .description = COMPOUND_STRING("Find 50 hidden items."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_PERSISTENT_PROFILE,
        .category    = ACHIEVEMENT_CATEGORY_EXPLORATION,
        .points      = 105,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_EXPLORE_TALK_TO_THE_LOCALS] = {
        .name        = ACHIEVEMENT_NAME("Talk to the Locals"),
        .description = COMPOUND_STRING("Talk to 50 NPCs."),
        .tier        = ACHIEVEMENT_TIER_BRONZE,
        .scope       = ACHIEVEMENT_SCOPE_PERSISTENT_PROFILE,
        .category    = ACHIEVEMENT_CATEGORY_EXPLORATION,
        .points      = 25,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_EXPLORE_PEOPLE_PERSON] = {
        .name        = ACHIEVEMENT_NAME("People Person"),
        .description = COMPOUND_STRING("Talk to 150 NPCs."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_PERSISTENT_PROFILE,
        .category    = ACHIEVEMENT_CATEGORY_EXPLORATION,
        .points      = 60,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_EXPLORE_LOCAL_EXPERT] = {
        .name        = ACHIEVEMENT_NAME("Local Expert"),
        .description = COMPOUND_STRING("See every wild Pokémon species available on a single route."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_EXPLORATION,
        .points      = 70,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_ECONOMY_FIRST_PURCHASE] = {
        .name        = ACHIEVEMENT_NAME("First Purchase"),
        .description = COMPOUND_STRING("Buy something from a shop."),
        .tier        = ACHIEVEMENT_TIER_BRONZE,
        .scope       = ACHIEVEMENT_SCOPE_PERSISTENT_PROFILE,
        .category    = ACHIEVEMENT_CATEGORY_ECONOMY,
        .points      = 25,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_ECONOMY_REGULAR_CUSTOMER] = {
        .name        = ACHIEVEMENT_NAME("Regular Customer"),
        .description = COMPOUND_STRING("Shop 50 times."),
        .tier        = ACHIEVEMENT_TIER_BRONZE,
        .scope       = ACHIEVEMENT_SCOPE_PERSISTENT_PROFILE,
        .category    = ACHIEVEMENT_CATEGORY_ECONOMY,
        .points      = 35,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_ECONOMY_BIG_SPENDER] = {
        .name        = ACHIEVEMENT_NAME("Big Spender"),
        .description = COMPOUND_STRING("Spend a cumulative ¥100,000 at shops."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_PERSISTENT_PROFILE,
        .category    = ACHIEVEMENT_CATEGORY_ECONOMY,
        .points      = 70,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_ECONOMY_WHALE] = {
        .name        = ACHIEVEMENT_NAME("Whale"),
        .description = COMPOUND_STRING("Spend a cumulative ¥1,000,000 at shops."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_PERSISTENT_PROFILE,
        .category    = ACHIEVEMENT_CATEGORY_ECONOMY,
        .points      = 115,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_ECONOMY_SAVE_YOUR_CHANGE] = {
        .name        = ACHIEVEMENT_NAME("Save Your Change"),
        .description = COMPOUND_STRING("Clear a Gym while holding ¥50,000 or more."),
        .tier        = ACHIEVEMENT_TIER_BRONZE,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_ECONOMY,
        .points      = 35,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_ECONOMY_FRUGAL_TRAINER] = {
        .name        = ACHIEVEMENT_NAME("Frugal Trainer"),
        .description = COMPOUND_STRING("Clear a Gym without shopping since your last Gym win."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_ECONOMY,
        .points      = 60,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_ECONOMY_NO_SHOPPING] = {
        .name        = ACHIEVEMENT_NAME("No Shopping"),
        .description = COMPOUND_STRING("Clear four consecutive Gyms without shopping in between."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_ECONOMY,
        .points      = 105,
        .hidden      = FALSE,
    },
    // ACHIEVEMENT_ECONOMY_RESOURCEFUL ("win a major battle
    // carrying fewer than five consumables") removed here -- most players
    // don't stock up on more than a few consumables to begin with, so this
    // holds without any deliberate effort. See
    // include/constants/achievements.h's category M comment.
    [ACHIEVEMENT_ECONOMY_TREASURE_PAYS] = {
        .name        = ACHIEVEMENT_NAME("Treasure Pays"),
        .description = COMPOUND_STRING("Earn ¥50,000 total from selling items."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_ECONOMY,
        .points      = 60,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_ECONOMY_INVESTOR] = {
        .name        = ACHIEVEMENT_NAME("Investor"),
        .description = COMPOUND_STRING("Finish the story holding ¥500,000 or more."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_ECONOMY,
        .points      = 115,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_EXPLORE_PACK_RAT] = {
        .name        = ACHIEVEMENT_NAME("Pack Rat"),
        .description = COMPOUND_STRING("Hold 20 different items in your Bag at once."),
        .tier        = ACHIEVEMENT_TIER_BRONZE,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_ADVENTURE,
        .points      = 25,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_EXPLORE_NO_LOOSE_ENDS] = {
        .name        = ACHIEVEMENT_NAME("No Loose Ends"),
        .description = COMPOUND_STRING("Obtain the Pokédex, the PokeNav and the Running Shoes."),
        .tier        = ACHIEVEMENT_TIER_BRONZE,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_ADVENTURE,
        .points      = 35,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_COLLECT_EVOLUTIONARY_PATH] = {
        .name        = ACHIEVEMENT_NAME("Evolutionary Path"),
        .description = COMPOUND_STRING("Evolve 10 Pokémon."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_COLLECTION,
        .points      = 60,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_COLLECT_EVOLUTION_EXPERT] = {
        .name        = ACHIEVEMENT_NAME("Evolution Expert"),
        .description = COMPOUND_STRING("Evolve 25 Pokémon."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_COLLECTION,
        .points      = 105,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_COLLECT_FRIENDSHIP_BLOSSOMS] = {
        .name        = ACHIEVEMENT_NAME("Friendship Blossoms"),
        .description = COMPOUND_STRING("Evolve a Pokémon through friendship."),
        .tier        = ACHIEVEMENT_TIER_BRONZE,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_COLLECTION,
        .points      = 35,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_COLLECT_STONE_AGE] = {
        .name        = ACHIEVEMENT_NAME("Stone Age"),
        .description = COMPOUND_STRING("Evolve a Pokémon with an evolution stone."),
        .tier        = ACHIEVEMENT_TIER_BRONZE,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_COLLECTION,
        .points      = 25,
        .hidden      = FALSE,
    },
    // ACHIEVEMENT_COLLECT_TRADE_SECRETS ("obtain a Pokemon
    // by trade") removed here -- even a single in-game NPC trade satisfies
    // this, so most playthroughs pick it up without any deliberate effort.
    // See include/constants/achievements.h's category M comment.
    [ACHIEVEMENT_COLLECT_RARE_FIND] = {
        .name        = ACHIEVEMENT_NAME("Rare Find"),
        .description = COMPOUND_STRING("Catch a Pokémon found by a DexNav scan."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_COLLECTION,
        .points      = 70,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_COLLECT_GREEN_THUMB] = {
        .name        = ACHIEVEMENT_NAME("Green Thumb"),
        .description = COMPOUND_STRING("Harvest 50 berries."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_COLLECTION,
        .points      = 60,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_COLLECT_ANGLER] = {
        .name        = ACHIEVEMENT_NAME("Angler"),
        .description = COMPOUND_STRING("Have 100 fishing encounters."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_COLLECTION,
        .points      = 60,
        .hidden      = FALSE,
    },

    // N. Challenge Runs & Nuzlocke (30). The 7 "challenge
    // modifiers" (Achievement_CountChallengeModifiers) are the New Game
    // settings menu's Nuzlocke Mode, HARD difficulty, each of the three
    // Randomize flags, Level Cap, and Stat Editor -- all on their harder
    // setting (Stat Editor's harder setting is OFF, unlike the rest).
    [ACHIEVEMENT_CHALLENGE_SELF_IMPOSED] = {
        .name        = ACHIEVEMENT_NAME("Self-Imposed"),
        .description = COMPOUND_STRING("Complete the game with 3+ of the 7 challenge settings on: Nuzlocke Mode, HARD, each Randomizer flag, Level Cap, or Stat Editor off."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_CHALLENGE,
        .points      = 70,
        .hidden      = FALSE,
    },
    // Same modifier list as
    // ACHIEVEMENT_CHALLENGE_SELF_IMPOSED above, higher threshold.
    [ACHIEVEMENT_CHALLENGE_HARD_WAY] = {
        .name        = ACHIEVEMENT_NAME("Hard Way"),
        .description = COMPOUND_STRING("Complete the game with 5+ of the 7 challenge settings on: Nuzlocke Mode, HARD, each Randomizer flag, Level Cap, or Stat Editor off."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_CHALLENGE,
        .points      = 125,
        .hidden      = FALSE,
    },
    // Same modifier list as
    // ACHIEVEMENT_CHALLENGE_SELF_IMPOSED above, all seven required.
    [ACHIEVEMENT_CHALLENGE_BRUTAL_RULES] = {
        .name        = ACHIEVEMENT_NAME("Brutal Rules"),
        .description = COMPOUND_STRING("Complete the game with all 7 challenge settings on: Nuzlocke Mode, HARD, all three Randomizer flags, Level Cap, and Stat Editor off."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_CHALLENGE,
        .points      = 160,
        .hidden      = FALSE,
    },
    // Same modifier list as
    // ACHIEVEMENT_CHALLENGE_SELF_IMPOSED above, all seven plus boosts off --
    // spelled out here rather than just referencing Brutal Rules by name --
    // self-contained descriptions, same principle
    // ACHIEVEMENT_CHALLENGE_PERFECTLY_CAPPED's description follows too.
    [ACHIEVEMENT_CHALLENGE_NIGHTMARE_MODE] = {
        .name        = ACHIEVEMENT_NAME("Nightmare Mode"),
        .description = COMPOUND_STRING("Complete the game with all 7 challenge settings on (Nuzlocke Mode, HARD, all three Randomizer flags, Level Cap, Stat Editor off) and the boost system disabled."),
        .tier        = ACHIEVEMENT_TIER_DIAMOND,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_CHALLENGE,
        .points      = 225,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_CHALLENGE_NO_SHOPPING_RUN] = {
        .name        = ACHIEVEMENT_NAME("No Shopping Run"),
        .description = COMPOUND_STRING("Complete the story without buying a consumable item from a shop."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_CHALLENGE,
        .points      = 115,
        .hidden      = FALSE,
    },
    // Now requires a full 6-Pokemon party (see
    // Achievement_CheckChallengeMilestones in src/achievements.c) -- with
    // only one or two Pokemon along there's barely any HP pool to dip
    // into, making this trivial to earn by accident.
    // Scaled up from Silver/25 to Gold/40 -- the full-6-Pokemon-party
    // requirement made this meaningfully harder than its old Silver points
    // reflected.
    [ACHIEVEMENT_CHALLENGE_NO_HEALING_ITEMS] = {
        .name        = ACHIEVEMENT_NAME("No Healing Items"),
        .description = COMPOUND_STRING("Win a major battle with a full team of 6 Pokémon, without using a single healing item."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_CHALLENGE,
        .points      = 95,
        .hidden      = FALSE,
    },
    // Same full-6-Pokemon-party requirement as
    // ACHIEVEMENT_CHALLENGE_NO_HEALING_ITEMS above -- also barely any held
    // items to check with a small party.
    // Scaled up from 45 to 55, same reasoning as No Healing Items above --
    // this one was already Gold, but under-priced relative to its
    // now-stricter cousin.
    [ACHIEVEMENT_CHALLENGE_ITEMLESS_BATTLE] = {
        .name        = ACHIEVEMENT_NAME("Itemless Battle"),
        .description = COMPOUND_STRING("Win a major battle with a full team of 6 Pokémon, using no Bag items and no held items."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_CHALLENGE,
        .points      = 125,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_CHALLENGE_WHO_NEEDS_CENTERS] = {
        .name        = ACHIEVEMENT_NAME("Who Needs Centers?"),
        .description = COMPOUND_STRING("Reach the fifth Badge without ever using a Pokémon Center."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_CHALLENGE,
        .points      = 105,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_CHALLENGE_NO_CENTERS] = {
        .name        = ACHIEVEMENT_NAME("No Centers"),
        .description = COMPOUND_STRING("Complete the story without ever using a Pokémon Center."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_CHALLENGE,
        .points      = 160,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_CHALLENGE_SET_IN_STONE] = {
        .name        = ACHIEVEMENT_NAME("Set in Stone"),
        .description = COMPOUND_STRING("Win a major battle on SET battle style."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_CHALLENGE,
        .points      = 60,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_CHALLENGE_HARDCORE_SET] = {
        .name        = ACHIEVEMENT_NAME("Hardcore Set"),
        .description = COMPOUND_STRING("Complete the story on SET battle style at HARD difficulty."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_CHALLENGE,
        .points      = 125,
        .hidden      = FALSE,
    },
    // ACHIEVEMENT_CHALLENGE_LEVEL_DISCIPLINE ("beat a Gym
    // Leader with no party member above the level cap") removed here -- a
    // player just playing through normally, without deliberately grinding,
    // rarely ends up over the level cap anyway. See
    // include/constants/achievements.h's category N comment.
    //
    // ACHIEVEMENT_CHALLENGE_CAPSTONE ("complete the story
    // without exceeding the level cap") removed here too -- it was the exact
    // same condition as ACHIEVEMENT_CHALLENGE_PERFECTLY_CAPPED below, minus
    // that achievement's extra HARD/randomizer requirement, so completing
    // Perfectly Capped always completed Capstone as a freebie. See
    // Achievement_CheckChallengeCompletionMilestones (src/achievements.c).
    [ACHIEVEMENT_CHALLENGE_PERFECTLY_CAPPED] = {
        .name        = ACHIEVEMENT_NAME("Perfectly Capped"),
        .description = COMPOUND_STRING("Complete the story without any party member exceeding the level cap in a randomized or HARD difficulty playthrough."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_CHALLENGE,
        .points      = 160,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_CHALLENGE_MINIMALIST] = {
        .name        = ACHIEVEMENT_NAME("Minimalist"),
        .description = COMPOUND_STRING("Win a major battle with only three Pokémon in your party."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_CHALLENGE,
        .points      = 60,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_CHALLENGE_THREE_POKEMON] = {
        .name        = ACHIEVEMENT_NAME("Three-Pokémon Challenge"),
        .description = COMPOUND_STRING("Complete the story never carrying more than three Pokémon."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_CHALLENGE,
        .points      = 125,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_CHALLENGE_SOLO_JOURNEY] = {
        .name        = ACHIEVEMENT_NAME("Solo Journey"),
        .description = COMPOUND_STRING("Complete the story with a single battle-eligible Pokémon."),
        .tier        = ACHIEVEMENT_TIER_DIAMOND,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_CHALLENGE,
        .points      = 225,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_CHALLENGE_NO_FREEBIES] = {
        .name        = ACHIEVEMENT_NAME("No Freebies"),
        .description = COMPOUND_STRING("Complete the story without your starter Pokémon acting in a major battle."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_CHALLENGE,
        .points      = 115,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_CHALLENGE_HARDLY_ANY_HELP] = {
        .name        = ACHIEVEMENT_NAME("Hardly Any Help"),
        .description = COMPOUND_STRING("Complete the story with the boost system off and the Stat Editor disabled."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_CHALLENGE,
        .points      = 125,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_NUZLOCKE_FIRST_GYM] = {
        .name        = ACHIEVEMENT_NAME("First Nuzlocke"),
        .description = COMPOUND_STRING("Clear a Gym under Nuzlocke rules."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_NUZLOCKE,
        .points      = 60,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_NUZLOCKE_HARDCORE_SURVIVOR] = {
        .name        = ACHIEVEMENT_NAME("Hardcore Survivor"),
        .description = COMPOUND_STRING("Complete a Nuzlocke at HARD difficulty with the level cap enforced."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_NUZLOCKE,
        .points      = 160,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_NUZLOCKE_PERFECT] = {
        .name        = ACHIEVEMENT_NAME("Perfect Nuzlocke"),
        .description = COMPOUND_STRING("Complete a Nuzlocke without losing a single Pokémon."),
        .tier        = ACHIEVEMENT_TIER_DIAMOND,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_NUZLOCKE,
        .points      = 225,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_NUZLOCKE_CLOSE_CALL] = {
        .name        = ACHIEVEMENT_NAME("Close Call"),
        .description = COMPOUND_STRING("Win a Nuzlocke battle after a party member drops below 10% HP."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_NUZLOCKE,
        .points      = 60,
        .hidden      = FALSE,
    },
    // ACHIEVEMENT_NUZLOCKE_SPECIES_CLAUSE ("no two catches
    // from the same family") and ACHIEVEMENT_NUZLOCKE_NO_REVIVES ("never
    // used a Revive") removed here -- a genuine Nuzlocke already only keeps
    // one catch per route and treats a fainted Pokemon as permanently boxed,
    // so both conditions tend to hold on their own. See
    // include/constants/achievements.h's category N (Nuzlocke) comment.
    //
    // ACHIEVEMENT_NUZLOCKE_NO_ACE_ALLOWED ("clear a Nuzlocke
    // Gym without your highest-level Pokemon acting") also removed here --
    // duplicate of ACHIEVEMENT_TEAM_UNDERSTUDY (same check, unconditional on
    // Nuzlocke mode), which already fires for Nuzlocke runs too. See
    // Achievement_CheckNuzlockeMilestones (src/achievements.c).
    [ACHIEVEMENT_NUZLOCKE_SCRAPPY] = {
        .name        = ACHIEVEMENT_NAME("Scrappy"),
        .description = COMPOUND_STRING("Win a Nuzlocke Gym battle with your lowest-level Pokémon landing the final blow."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_NUZLOCKE,
        .points      = 60,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_NUZLOCKE_GRAVEYARD] = {
        .name        = ACHIEVEMENT_NAME("The Graveyard"),
        .description = COMPOUND_STRING("Lose five or more Pokémon in one Nuzlocke and still complete it."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_NUZLOCKE,
        .points      = 115,
        .hidden      = FALSE,
    },
    // ACHIEVEMENT_NUZLOCKE_FULL_ENCOUNTER ("complete a
    // Nuzlocke having taken the encounter on every route you entered")
    // removed -- one missed/fled encounter anywhere in the whole run
    // permanently breaks it (a sticky flag, per its own bookkeeping comment),
    // which plays as punishing rather than as a genuine challenge. Its
    // helper, Achievement_CheckNuzlockeExplorationMilestones, and its
    // LoadCurrentMapData (src/overworld.c) call site are removed along with
    // it; runData->nuzlockePendingRoute/nuzlockeRouteSkipped are now unread
    // but left in place (see the struct's own comment).
    //
    // ACHIEVEMENT_NUZLOCKE_UNASSISTED_SURVIVOR ("complete a
    // Nuzlocke with the boost system disabled") removed here -- too similar
    // to ACHIEVEMENT_CHALLENGE_HARDLY_ANY_HELP (!boostsEnabled is a strict
    // subset of that achievement's condition, checked at the same GameClear
    // call site), which already fires for Nuzlocke runs too since it isn't
    // gated on nuzlockeModeEnabled. See Achievement_CheckNuzlockeCompletionMilestones
    // (src/achievements.c).

    // ---- Randomizer & New Game+ (category O) --------------------------
    // "A randomized playthrough" was ambiguous given there
    // are three independent randomizer settings (species/FLAG_RANDOMIZE_MON,
    // type/FLAG_RANDOMIZE_TYPE, move/FLAG_RANDOMIZE_MOVES). This is a
    // three-tier ladder and each tier now spells out its own criteria
    // explicitly rather than reusing that phrase: Chaos Begins and Random by
    // Nature only need ANY one of the three settings on (Achievement_AnyRandomizerFlagSet,
    // src/achievements.c), while Truly Random -- the tier that already
    // required it in code -- needs ALL three at once. Descriptions below are
    // worded to match each achievement's actual condition exactly.
    [ACHIEVEMENT_RANDOMIZER_CHAOS_BEGINS] = {
        .name        = ACHIEVEMENT_NAME("Chaos Begins"),
        .description = COMPOUND_STRING("Begin a playthrough with at least one randomizer setting enabled."),
        .tier        = ACHIEVEMENT_TIER_BRONZE,
        .scope       = ACHIEVEMENT_SCOPE_PERSISTENT_PROFILE,
        .category    = ACHIEVEMENT_CATEGORY_RANDOMIZER,
        .points      = 25,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_RANDOMIZER_RANDOM_BY_NATURE] = {
        .name        = ACHIEVEMENT_NAME("Random by Nature"),
        .description = COMPOUND_STRING("Clear a Gym Leader in a playthrough with at least one randomizer setting enabled."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_RANDOMIZER,
        .points      = 60,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_RANDOMIZER_TRULY_RANDOM] = {
        .name        = ACHIEVEMENT_NAME("Truly Random"),
        .description = COMPOUND_STRING("Complete a playthrough with all three randomizer settings enabled."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_RANDOMIZER,
        .points      = 140,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_RANDOMIZER_CHAOS_TEAM] = {
        .name        = ACHIEVEMENT_NAME("Chaos Team"),
        .description = COMPOUND_STRING("Win a randomized major battle with six different primary types."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_RANDOMIZER,
        .points      = 105,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_RANDOMIZER_PATCHWORK_TEAM] = {
        .name        = ACHIEVEMENT_NAME("Patchwork Team"),
        .description = COMPOUND_STRING("Win a major battle with six Pokémon caught on six different routes."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_RANDOMIZER,
        .points      = 105,
        .hidden      = FALSE,
    },
    // ACHIEVEMENT_RANDOMIZER_NEVER_SEEN_IT_COMING ("beat a
    // randomized major battle with no super-effective move available")
    // removed here -- with move/type randomization scrambling coverage,
    // having zero super-effective options against some boss just happens by
    // chance over a run's worth of major battles. See
    // include/constants/achievements.h's category O comment.
    // ACHIEVEMENT_RANDOMIZER_SEED_EXPLORER (2 randomized
    // playthroughs) and _VETERAN (5) removed here -- ACHIEVEMENT_RANDOMIZED_1
    // (category J) is already the "do it once" version of this ladder.
    [ACHIEVEMENT_RANDOMIZER_PURE_CHAOS] = {
        .name        = ACHIEVEMENT_NAME("Pure Chaos"),
        .description = COMPOUND_STRING("Complete a playthrough with all three randomizer flags, HARD, and the level cap on."),
        .tier        = ACHIEVEMENT_TIER_DIAMOND,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_RANDOMIZER,
        .points      = 225,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_NUZLOCKE_ACROSS_WORLDS] = {
        .name        = ACHIEVEMENT_NAME("Nuzlocke Across Worlds"),
        .description = COMPOUND_STRING("Complete a randomized Nuzlocke."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_NUZLOCKE,
        .points      = 160,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_NUZLOCKE_CHAOS_SURVIVOR] = {
        .name        = ACHIEVEMENT_NAME("Chaos Survivor"),
        .description = COMPOUND_STRING("Complete a randomized Nuzlocke on HARD difficulty."),
        .tier        = ACHIEVEMENT_TIER_DIAMOND,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_NUZLOCKE,
        .points      = 225,
        .hidden      = FALSE,
    },
    // ACHIEVEMENT_NG_PLUS_ONE_MORE_TIME (complete cycle 2)
    // and _BEYOND_THE_BEGINNING (reach cycle 10) removed here -- collapsed,
    // along with category J's old NG_PLUS_STARTED/_CYCLE_3/_CYCLE_5/
    // _COMPLETED_3, into the single ACHIEVEMENT_NG_PLUS_CYCLE_COMPLETE
    // (category J), which reused this entry's "One More Time" name.
    [ACHIEVEMENT_NG_PLUS_FRESH_FACES] = {
        .name        = ACHIEVEMENT_NAME("Fresh Faces"),
        .description = COMPOUND_STRING("Defeat 50 Trainers within a single New Game+ cycle."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_NG_PLUS,
        .points      = 70,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_NG_PLUS_NEVER_THE_SAME_FIGHT] = {
        .name        = ACHIEVEMENT_NAME("Never the Same Fight"),
        .description = COMPOUND_STRING("Defeat 300 Trainers across New Game+ cycles."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_NG_PLUS,
        .category    = ACHIEVEMENT_CATEGORY_NG_PLUS,
        .points      = 115,
        .hidden      = FALSE,
    },
    // Same modifier list as
    // ACHIEVEMENT_CHALLENGE_SELF_IMPOSED (src/data/achievements.h,
    // category N), checked at NG+ cycle completion instead of first clear.
    [ACHIEVEMENT_NG_PLUS_CYCLE_SPECIALIST] = {
        .name        = ACHIEVEMENT_NAME("Cycle Specialist"),
        .description = COMPOUND_STRING("Complete a New Game+ cycle with 3+ of the 7 challenge settings on: Nuzlocke Mode, HARD, each Randomizer flag, Level Cap, or Stat Editor off."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_NG_PLUS,
        .points      = 125,
        .hidden      = FALSE,
    },
    // ACHIEVEMENT_NG_PLUS_ESCALATION (3 consecutive NG+
    // cycles) removed here -- part of the same collapse-to-one-completion
    // consolidation as the rest of the NG+ ladder (see category J's
    // ACHIEVEMENT_NG_PLUS_CYCLE_COMPLETE).
    [ACHIEVEMENT_NG_PLUS_NO_NOSTALGIA] = {
        .name        = ACHIEVEMENT_NAME("No Nostalgia"),
        .description = COMPOUND_STRING("Complete an NG+ cycle sharing no party species with the previous one."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_NG_PLUS,
        .category    = ACHIEVEMENT_CATEGORY_NG_PLUS,
        .points      = 115,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_NG_PLUS_COMPLETE_REINVENTION] = {
        .name        = ACHIEVEMENT_NAME("Complete Reinvention"),
        .description = COMPOUND_STRING("Use a different party in every Gym of one NG+ cycle."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_NG_PLUS,
        .points      = 125,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_NG_PLUS_BOSS_GAUNTLET] = {
        .name        = ACHIEVEMENT_NAME("Boss Gauntlet"),
        .description = COMPOUND_STRING("Defeat every major battle within a single NG+ cycle."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_NG_PLUS,
        .points      = 115,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_NG_PLUS_CYCLE_NUZLOCKE] = {
        .name        = ACHIEVEMENT_NAME("Cycle Nuzlocke"),
        .description = COMPOUND_STRING("Complete an NG+ cycle with Nuzlocke enabled."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_NG_PLUS,
        .points      = 140,
        .hidden      = FALSE,
    },
    // ACHIEVEMENT_NG_PLUS_ENDLESS_SURVIVOR ("complete NG+
    // cycle 5 or higher with Nuzlocke and the randomizer enabled") removed
    // -- stacks a deep NG+ grind on top of a randomized Nuzlocke's own
    // permadeath pressure, closer to punishing than to a genuine challenge.
    [ACHIEVEMENT_RANDOMIZER_SPECIES_CHAOS] = {
        .name        = ACHIEVEMENT_NAME("Species Chaos"),
        .description = COMPOUND_STRING("Complete a playthrough with randomized species only."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_RANDOMIZER,
        .points      = 70,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_RANDOMIZER_TYPE_CHAOS] = {
        .name        = ACHIEVEMENT_NAME("Type Chaos"),
        .description = COMPOUND_STRING("Complete a playthrough with randomized types only."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_RANDOMIZER,
        .points      = 70,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_RANDOMIZER_MOVE_CHAOS] = {
        .name        = ACHIEVEMENT_NAME("Move Chaos"),
        .description = COMPOUND_STRING("Complete a playthrough with randomized movesets only."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_RANDOMIZER,
        .points      = 70,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_RANDOMIZER_ROOKIE] = {
        .name        = ACHIEVEMENT_NAME("Randomized Rookie"),
        .description = COMPOUND_STRING("Catch 25 Pokémon in a randomized playthrough."),
        .tier        = ACHIEVEMENT_TIER_BRONZE,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_RUN,
        .category    = ACHIEVEMENT_CATEGORY_RANDOMIZER,
        .points      = 35,
        .hidden      = FALSE,
    },
    // Used to require this be specifically NG+ cycle 2;
    // narrowed to "a" cycle -- same "do it once" treatment as the rest of
    // the NG+ ladder collapse, since the boost-disabled condition doesn't
    // get any harder to satisfy on a later cycle.
    [ACHIEVEMENT_NG_PLUS_UNASSISTED_CYCLE] = {
        .name        = ACHIEVEMENT_NAME("Unassisted Cycle"),
        .description = COMPOUND_STRING("Complete a New Game+ cycle with the boost system disabled."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_NG_PLUS,
        .points      = 125,
        .hidden      = FALSE,
    },
    // ACHIEVEMENT_NG_PLUS_TEN_CYCLES_DEEP ("complete ten
    // New Game+ cycles") and ACHIEVEMENT_NG_PLUS_CYCLE_COLLECTOR ("complete
    // NG+ cycles under three different challenge configurations") both
    // removed -- ten full replays (or three deliberately-varied ones) is a
    // grind for its own sake on top of everything category J/O already ask
    // for, not a genuine additional challenge. gAchievementProfile.ngPlusConfigsSeen[]/
    // ngPlusConfigsSeenCount (Cycle Collector's sole reader) are now unread
    // but left in place (see the struct's own comment).
    [ACHIEVEMENT_VARIETY_FULL_CIRCLE] = {
        .name        = ACHIEVEMENT_NAME("Full Circle"),
        .description = COMPOUND_STRING("Complete a normal, Nuzlocke, and a randomized playthrough."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_PERSISTENT_PROFILE,
        .category    = ACHIEVEMENT_CATEGORY_PROFILE,
        .points      = 140,
        .hidden      = FALSE,
    },

    // Streaks, Records & Collection Remainder.
    [ACHIEVEMENT_RECORD_HOT_STREAK] = {
        .name        = ACHIEVEMENT_NAME("Hot Streak"),
        .description = COMPOUND_STRING("Win five Trainer battles in a row."),
        .tier        = ACHIEVEMENT_TIER_BRONZE,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_RECORDS,
        .points      = 25,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_RECORD_UNBROKEN] = {
        .name        = ACHIEVEMENT_NAME("Unbroken"),
        .description = COMPOUND_STRING("Win twenty Trainer battles in a row."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_RECORDS,
        .points      = 60,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_RECORD_ON_A_ROLL] = {
        .name        = ACHIEVEMENT_NAME("On a Roll"),
        .description = COMPOUND_STRING("Win fifty Trainer battles in a row."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_RECORDS,
        .points      = 105,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_RECORD_UNTOUCHABLE_STREAK] = {
        .name        = ACHIEVEMENT_NAME("Untouchable Streak"),
        .description = COMPOUND_STRING("Win one hundred Trainer battles in a row."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_RECORDS,
        .points      = 160,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_RECORD_THREE_GYM_STREAK] = {
        .name        = ACHIEVEMENT_NAME("Three Gym Streak"),
        .description = COMPOUND_STRING("Defeat three Gym Leaders without a party wipe."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_RECORDS,
        .points      = 70,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_RECORD_EIGHT_GYM_STREAK] = {
        .name        = ACHIEVEMENT_NAME("Eight Gym Streak"),
        .description = COMPOUND_STRING("Defeat all eight Gym Leaders without a party wipe."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_RECORDS,
        .points      = 125,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_RECORD_LEAGUE_STREAK] = {
        .name        = ACHIEVEMENT_NAME("League Streak"),
        .description = COMPOUND_STRING("Defeat the entire Elite Four and Champion without a party wipe."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_RECORDS,
        .points      = 160,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_RECORD_VETERAN_TEAM] = {
        .name        = ACHIEVEMENT_NAME("Veteran Team"),
        .description = COMPOUND_STRING("Have one Pokémon score 100 KOs."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_RECORDS,
        .points      = 70,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_RECORD_OLD_RELIABLE] = {
        .name        = ACHIEVEMENT_NAME("Old Reliable"),
        .description = COMPOUND_STRING("Have one Pokémon score 50 KOs in major battles."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_RECORDS,
        .points      = 115,
        .hidden      = FALSE,
    },
    // The logic was fixed, not just the description -- the old
    // check (a bitmask over party SLOTS, ANDed down) trivially always
    // fired, since party slot 0 is never empty while you're able to
    // battle at all. Achievement_CheckBattleRecordsMilestones now tracks
    // actual Pokemon (by personality) instead -- see
    // legendCandidatePersonalities/legendCandidateCount in
    // include/global.h's struct AchievementRunDataExt.
    [ACHIEVEMENT_RECORD_LEGEND_OF_THE_RUN] = {
        .name        = ACHIEVEMENT_NAME("Legend of the Run"),
        .description = COMPOUND_STRING("Keep the same Pokémon in your party for every major battle of a completed playthrough."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_RECORDS,
        .points      = 160,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_RECORD_COMEBACK_COUNT] = {
        .name        = ACHIEVEMENT_NAME("Comeback Count"),
        .description = COMPOUND_STRING("Win ten battles after being down to your last Pokémon."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_RECORDS,
        .points      = 105,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_RECORD_GROWING_STRONG] = {
        .name        = ACHIEVEMENT_NAME("Growing Strong"),
        .description = COMPOUND_STRING("Raise a Pokémon ten levels above the level you obtained it at."),
        .tier        = ACHIEVEMENT_TIER_BRONZE,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_ADVENTURE,
        .points      = 25,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_COLLECT_ONE_OF_EACH] = {
        .name        = ACHIEVEMENT_NAME("One of Each"),
        .description = COMPOUND_STRING("Own ten different species at once, party and boxes combined."),
        .tier        = ACHIEVEMENT_TIER_BRONZE,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_COLLECTION,
        .points      = 35,
        .hidden      = FALSE,
    },
    // Walks the whole family
    // tree from the base form outward (Achievement_GetFamilyMembers),
    // including every branch, so a branching family like Eevee's needs
    // every one of its evolutions caught, not just one.
    [ACHIEVEMENT_COLLECT_FAMILY_REUNION] = {
        .name        = ACHIEVEMENT_NAME("Family Reunion"),
        .description = COMPOUND_STRING("Catch every stage of one Pokémon's evolutionary line, including every branch (e.g. all of Eevee's evolutions)."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_COLLECTION,
        .points      = 105,
        .hidden      = FALSE,
    },
    // ACHIEVEMENT_COLLECT_PERFECT_SPECIMEN ("obtain a
    // Pokemon with all six IVs at 31") removed here -- a lucky roll on any
    // catch or hatch, not something a player can deliberately work towards.
    // See include/constants/achievements.h's category P comment.
    [ACHIEVEMENT_COLLECT_ODDBALL] = {
        .name        = ACHIEVEMENT_NAME("Oddball"),
        .description = COMPOUND_STRING("Clear a Gym with a Pokémon below 350 base stat total in the party."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_COLLECTION,
        .points      = 70,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_COLLECT_UNDERESTIMATED] = {
        .name        = ACHIEVEMENT_NAME("Underestimated"),
        .description = COMPOUND_STRING("Have a Pokémon below 400 base stat total land the final KO against a major boss's full team of 6 Pokémon."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_COLLECTION,
        .points      = 115,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_RECORD_MARATHON_TRAINER] = {
        .name        = ACHIEVEMENT_NAME("Marathon Trainer"),
        .description = COMPOUND_STRING("Take 50,000 steps."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_RECORDS,
        .points      = 60,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_RECORD_LONG_HAUL] = {
        .name        = ACHIEVEMENT_NAME("Long Haul"),
        .description = COMPOUND_STRING("Take 200,000 steps."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_RECORDS,
        .points      = 105,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_RECORD_PROLIFIC] = {
        .name        = ACHIEVEMENT_NAME("Prolific"),
        .description = COMPOUND_STRING("Battle 1,000 times."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_RECORDS,
        .points      = 70,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_RECORD_BATTLE_MACHINE] = {
        .name        = ACHIEVEMENT_NAME("Battle Machine"),
        .description = COMPOUND_STRING("Battle 2,500 times."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_RECORDS,
        .points      = 125,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_RECORD_CENTURY_CLUB] = {
        .name        = ACHIEVEMENT_NAME("Century Club"),
        .description = COMPOUND_STRING("Raise a Pokémon to level 100."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_RECORDS,
        .points      = 70,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_RECORD_FULL_CENTURY] = {
        .name        = ACHIEVEMENT_NAME("Full Century"),
        .description = COMPOUND_STRING("Have six Pokémon at level 100 at once."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_RECORDS,
        .points      = 140,
        .hidden      = FALSE,
    },
    // ACHIEVEMENT_COLLECT_BOX_FILLER ("store 100 Pokemon at
    // once") and ACHIEVEMENT_COLLECT_STORAGE_BARON ("store 300 at once")
    // removed here -- a full playthrough's worth of catching fills PC boxes
    // up on its own, no deliberate collecting required. See
    // include/constants/achievements.h's category P comment.
    [ACHIEVEMENT_RECORD_DEVOTED] = {
        .name        = ACHIEVEMENT_NAME("Devoted"),
        .description = COMPOUND_STRING("Raise a Pokémon to maximum friendship."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_RECORDS,
        .points      = 60,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_RECORD_INSEPARABLE] = {
        .name        = ACHIEVEMENT_NAME("Inseparable"),
        .description = COMPOUND_STRING("Have six party members at maximum friendship at once."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_RECORDS,
        .points      = 105,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_RECORD_MOVE_TUTOR] = {
        .name        = ACHIEVEMENT_NAME("Move Tutor"),
        .description = COMPOUND_STRING("Teach 25 TMs."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_RECORDS,
        .points      = 60,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_RECORD_EGG_MARATHON] = {
        .name        = ACHIEVEMENT_NAME("Egg Marathon"),
        .description = COMPOUND_STRING("Hatch 100 Eggs."),
        .tier        = ACHIEVEMENT_TIER_SILVER,
        .scope       = ACHIEVEMENT_SCOPE_PERSISTENT_PROFILE,
        .category    = ACHIEVEMENT_CATEGORY_RECORDS,
        .points      = 70,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_RECORD_NURSES_NIGHTMARE] = {
        .name        = ACHIEVEMENT_NAME("Nurse's Nightmare"),
        .description = COMPOUND_STRING("Visit a Pokémon Center 200 times."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_CURRENT_PLAYTHROUGH,
        .category    = ACHIEVEMENT_CATEGORY_RECORDS,
        .points      = 105,
        .hidden      = FALSE,
    },

    // Profile Meta, Mastery & Prestige. All
    // ACHIEVEMENT_CATEGORY_PROFILE -- see constants/achievements.h's category
    // Q comment for why there's no separate Mastery category.
    //
    // Removed here -- Bronze/Silver/Gold/Diamond Master and
    // Category Conqueror (complete every achievement of a tier / entirely in
    // one category), Master of the Game (90% of all non-hidden
    // achievements), Nothing Left to Prove (100% of all non-hidden
    // achievements), Endgame Explorer (every New Game+ achievement),
    // Challenge Conqueror/Unbroken Will/Chaos Master (80% of the
    // Challenge/Nuzlocke/Randomizer category), Replay Architect (every
    // Gold-or-better in Team/Challenge/Nuzlocke/Randomizer), and Frequent
    // Flyer/Veteran Trainer/Resident Champion (10/25/50 playthroughs). See
    // Achievement_CheckMasteryMilestones (src/achievements.c) for the
    // code-side removal -- several helpers that existed solely for these
    // entries (Achievement_AnyCategoryFullyCompletedAtTier,
    // Achievement_CountInCategory, Achievement_CheckCategoryPercentMilestone,
    // Achievement_CountNonHiddenExcluding,
    // Achievement_GoldOrBetterFullyCompletedAcrossCategories) are removed
    // along with them.
    //
    // ACHIEVEMENT_PROFILE_ACHIEVEMENT_HUNTER ("Earn a Bronze
    // achievement in every category") removed here too -- it was unattainable
    // (Challenge, NG+, Nuzlocke and Profile have no Bronze-tier entry between
    // them), and
    // rather than force a tier onto categories that were never designed to
    // have an "easy" entry, the achievement itself goes. Its sole helpers,
    // Achievement_HasBronzeInEveryCategory and
    // Achievement_CountCompletedInCategory (src/achievements.c), are removed
    // along with it.
    [ACHIEVEMENT_PROFILE_WELL_ROUNDED] = {
        .name        = ACHIEVEMENT_NAME("Well Rounded"),
        .description = COMPOUND_STRING("Earn at least one achievement of every tier."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_PERSISTENT_PROFILE,
        .category    = ACHIEVEMENT_CATEGORY_PROFILE,
        .points      = 115,
        .hidden      = FALSE,
    },
    // ACHIEVEMENT_PROFILE_MASTER_OF_ALL ("earn a
    // Gold-or-better achievement in every category") removed -- see
    // Achievement_CheckMasteryMilestones (src/achievements.c);
    // Achievement_HasGoldOrBetterInEveryCategory, which existed solely for
    // this achievement, is removed along with it.
    // Scaled up from 5,000 to 10,000 (50% of the catalog's 20,000-point
    // total) -- see Achievement_CheckMasteryMilestones (src/achievements.c)'s
    // own comment on the rescale.
    [ACHIEVEMENT_PROFILE_POINT_HOARDER] = {
        .name        = ACHIEVEMENT_NAME("Point Hoarder"),
        .description = COMPOUND_STRING("Earn 10,000 total achievement points."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_PERSISTENT_PROFILE,
        .category    = ACHIEVEMENT_CATEGORY_PROFILE,
        .points      = 160,
        .hidden      = FALSE,
    },
    // Scaled up from 10,000 to 18,000 (90% of the 20,000-point total, so it
    // still means "you've all but finished the catalog," the same relative
    // bar the old 10,000/8,630-max threshold used to clear on its own before
    // the rescale made it literally unreachable -- see
    // src/data/achievements.h's top-of-file comment on the point-total
    // rescale).
    [ACHIEVEMENT_PROFILE_POINT_LEGEND] = {
        .name        = ACHIEVEMENT_NAME("Point Legend"),
        .description = COMPOUND_STRING("Earn 18,000 total achievement points."),
        .tier        = ACHIEVEMENT_TIER_DIAMOND,
        .scope       = ACHIEVEMENT_SCOPE_PERSISTENT_PROFILE,
        .category    = ACHIEVEMENT_CATEGORY_PROFILE,
        .points      = 345,
        .hidden      = FALSE,
    },
    // Scaled up from 3,000 to 7,000 -- the Gold-or-better pool is 14,580
    // (11,780 Gold + 2,800 Diamond), and 7,000 keeps this at roughly the same
    // ~49% share of that pool the old 3,000/6,090 threshold held.
    [ACHIEVEMENT_PROFILE_NO_EASY_PATH] = {
        .name        = ACHIEVEMENT_NAME("No Easy Path"),
        .description = COMPOUND_STRING("Earn 7,000 points from Gold-or-better achievements."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_PERSISTENT_PROFILE,
        .category    = ACHIEVEMENT_CATEGORY_PROFILE,
        .points      = 140,
        .hidden      = FALSE,
    },
    // Scaled down from 2,000 to 1,000 -- the boost economy this measures
    // against shrank from 42,500 to 20,000 total (src/data/achievement_boosts.h's
    // own comment on the rescale), and 1,000 keeps this at roughly the same
    // ~5% share of that total the old 2,000/42,500 threshold held.
    [ACHIEVEMENT_PROFILE_BOOST_INVESTOR] = {
        .name        = ACHIEVEMENT_NAME("Boost Investor"),
        .description = COMPOUND_STRING("Invest 1,000 points into boosts."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_PERSISTENT_PROFILE,
        .category    = ACHIEVEMENT_CATEGORY_PROFILE,
        .points      = 115,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_PROFILE_FULL_INVESTMENT] = {
        .name        = ACHIEVEMENT_NAME("Full Investment"),
        .description = COMPOUND_STRING("Reach 40 total purchased boost levels."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_PERSISTENT_PROFILE,
        .category    = ACHIEVEMENT_CATEGORY_PROFILE,
        .points      = 160,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_PROFILE_RECONFIGURED] = {
        .name        = ACHIEVEMENT_NAME("Reconfigured"),
        .description = COMPOUND_STRING("Reset your boosts, then invest points again."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_PERSISTENT_PROFILE,
        .category    = ACHIEVEMENT_CATEGORY_PROFILE,
        .points      = 105,
        .hidden      = FALSE,
    },
    [ACHIEVEMENT_PROFILE_SELECTIVE_MASTERY] = {
        .name        = ACHIEVEMENT_NAME("Selective Mastery"),
        .description = COMPOUND_STRING("Max out one boost while leaving five others unpurchased."),
        .tier        = ACHIEVEMENT_TIER_GOLD,
        .scope       = ACHIEVEMENT_SCOPE_PERSISTENT_PROFILE,
        .category    = ACHIEVEMENT_CATEGORY_PROFILE,
        .points      = 125,
        .hidden      = FALSE,
    },
    // ACHIEVEMENT_PROFILE_META_PROG_MASTER ("complete 200
    // achievements with every boost at max level") removed -- see
    // Achievement_CheckMetaProgMaster (src/achievements.c), which existed
    // solely for this achievement (called from both
    // Achievement_CheckMasteryMilestones and Achievement_CheckBoostMilestones)
    // and is removed along with it. Achievement_CountTotalCompleted and
    // AchievementBoost_AllMaxed, which existed solely to back it, are removed
    // too.
    [ACHIEVEMENT_MASTERY_DIAMOND_STANDARD] = {
        .name        = ACHIEVEMENT_NAME("Diamond Standard"),
        .description = COMPOUND_STRING("Complete every Diamond-tier achievement in the catalog."),
        .tier        = ACHIEVEMENT_TIER_DIAMOND,
        .scope       = ACHIEVEMENT_SCOPE_PERSISTENT_PROFILE,
        .category    = ACHIEVEMENT_CATEGORY_PROFILE,
        .points      = 225,
        .hidden      = FALSE,
    },
    // ACHIEVEMENT_VARIETY_NEW_TEAM_NEW_ME ("complete two
    // playthroughs sharing no party species") removed -- see
    // Achievement_OnFirstPlaythroughComplete (src/achievements.c); the
    // disjoint-species comparison that backed it is removed, but the
    // underlying party-species snapshot (AchievementRunDataExt.
    // previousCyclePartySpecies) stays -- No Nostalgia (ACHIEVEMENT_NG_PLUS_NO_NOSTALGIA)
    // still reads it every NG+ cycle.
    //
    // ACHIEVEMENT_VARIETY_REPLAY_MASTER ("complete five playthroughs under
    // five different rule configurations") removed -- see the same function.
    // Its dedicated tracking (Achievement_ChallengeConfigSignature,
    // playthroughConfigsSeen[]/_Count) had no other achievement reading it,
    // so the helper function is removed entirely and the persisted profile
    // fields are left in place, marked unused (see include/achievements.h).
    //
    // ACHIEVEMENT_HIDDEN_EASTER_EGG_HUNTER ("complete five hidden
    // achievements") removed -- it was the only hidden achievement anywhere
    // in the catalog, so it was permanently unattainable.
    // Achievement_CountHiddenCompleted, which existed solely for it, is
    // removed along with it.
};

STATIC_ASSERT(ACHIEVEMENTS_COUNT <= MAX_ACHIEVEMENTS, AchievementCountFitsProfile);
