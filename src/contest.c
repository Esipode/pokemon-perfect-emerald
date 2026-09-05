// The contest minigame was removed (RAM reclamation, stage 3c): contest entry points are
// stubbed to safely hand control back to the field/script instead of running the engine.
// What's kept here is only the state still read by code outside the old subsystem -- see
// contest.h for why each symbol survives.
#include "global.h"
#include "contest.h"
#include "pokemon.h"
#include "strings.h"
#include "constants/script_menu.h"
#include "constants/tv.h"

EWRAM_DATA u8 gContestMonPartyIndex = 0;
EWRAM_DATA u8 gContestPlayerMonIndex = 0;
EWRAM_DATA u8 gLinkContestFlags = 0;
EWRAM_DATA u8 gContestLinkLeaderIndex = 0;
EWRAM_DATA enum ContestCategories gSpecialVar_ContestCategory = 0;
EWRAM_DATA u16 gSpecialVar_ContestRank = 0;
EWRAM_DATA u8 gNumLinkContestPlayers = 0;
EWRAM_DATA struct ContestResources *gContestResources = NULL;

const struct ContestCategory gContestCategoryInfo[CONTEST_CATEGORIES_COUNT + 1] =
{
    [CONTEST_CATEGORY_COOL] =
    {
        .name = COMPOUND_STRING("COOL"),
        .condition = COMPOUND_STRING("coolness"),
        .generic = COMPOUND_STRING("COOL Move"),
        .negativeTrait = COMPOUND_STRING("shyness"),
        .palette = 13,
        .tile = 0x4040,
        .stdString = STDSTRING_COOL,
        .text = gText_Cool,
        .tvShowState = CONTESTLIVE_STATE_COOL,
        .tvShowStateExciting = CONTESTLIVE_STATE_VERY_COOL,
    },

    [CONTEST_CATEGORY_BEAUTY] =
    {
        .name = COMPOUND_STRING("BEAUTY"),
        .condition = COMPOUND_STRING("beauty"),
        .generic = COMPOUND_STRING("BEAUTY Move"),
        .negativeTrait = COMPOUND_STRING("anxiety"),
        .palette = 14,
        .tile = 0x4045,
        .stdString = STDSTRING_BEAUTY,
        .text = gText_Beauty,
        .tvShowState = CONTESTLIVE_STATE_BEAUTIFUL,
        .tvShowStateExciting = CONTESTLIVE_STATE_VERY_BEAUTIFUL,
    },

    [CONTEST_CATEGORY_CUTE] =
    {
        .name = COMPOUND_STRING("CUTE"),
        .condition = COMPOUND_STRING("cuteness"),
        .generic = COMPOUND_STRING("CUTE Move"),
        .negativeTrait = COMPOUND_STRING("laziness"),
        .palette = 14,
        .tile = 0x404A,
        .stdString = STDSTRING_CUTE,
        .text = gText_Cute,
        .tvShowState = CONTESTLIVE_STATE_CUTE,
        .tvShowStateExciting = CONTESTLIVE_STATE_VERY_CUTE,
    },

    [CONTEST_CATEGORY_SMART] =
    {
        .name = COMPOUND_STRING("SMART"),
        .condition = COMPOUND_STRING("smartness"),
        .generic = COMPOUND_STRING("SMART Move"),
        .negativeTrait = COMPOUND_STRING("hesitancy"),
        .palette = 15,
        .tile = 0x406A,
        .stdString = STDSTRING_SMART,
        .text = gText_Smart,
        .tvShowState = CONTESTLIVE_STATE_SMART,
        .tvShowStateExciting = CONTESTLIVE_STATE_VERY_SMART,
    },

    [CONTEST_CATEGORY_TOUGH] =
    {
        .name = COMPOUND_STRING("TOUGH"),
        .condition = COMPOUND_STRING("toughness"),
        .generic = COMPOUND_STRING("TOUGH Move"),
        .negativeTrait = COMPOUND_STRING("fear"),
        .palette = 13,
        .tile = 0x408A,
        .stdString = STDSTRING_TOUGH,
        .text = gText_Tough,
        .tvShowState = CONTESTLIVE_STATE_TOUGH,
        .tvShowStateExciting = CONTESTLIVE_STATE_VERY_TOUGH,
    },

    [CONTEST_CATEGORIES_COUNT] =
    {
        .generic = COMPOUND_STRING("???"),
    },
};

u8 GetContestEntryEligibility(struct Pokemon *pkmn)
{
    return CANT_ENTER_CONTEST;
}

bool8 IsSpeciesNotUnown(enum Species species)
{
    return species != SPECIES_UNOWN;
}

void ResetLinkContestBoolean(void)
{
    gLinkContestFlags = 0;
}

void ResetContestLinkResults(void)
{
    // FREE_CONTESTS already removed contestLinkResults from the saveblock.
}

void ClearContestWinnerPicsInContestHall(void)
{
    // FREE_CONTESTS already removed the museum painting save slots.
}
