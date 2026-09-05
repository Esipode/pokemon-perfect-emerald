#ifndef GUARD_CONTEST_H
#define GUARD_CONTEST_H

#include "constants/contest.h"

// The contest minigame was removed (RAM reclamation, stage 3c). What remains here is only
// what's still needed by code outside the old contest subsystem:
//  - battle_anim*.c branches on IsContest()/gContestResources for the contest move-animation
//    path; that path is unreachable now (IsContest() can never return TRUE), but the branches
//    still need to compile.
//  - gContestCategoryInfo is shown by non-contest move-info UI (Pokedex, Move Relearner).
//  - the handful of scalars below are read by TV's (also unreachable) Contest Live Updates show.

struct ContestMoveAnimData
{
    enum Species species;
    enum Species targetSpecies;
    bool8 hasTargetAnim:1;
    u8 isShiny:1;
    u8 targetIsShiny:1;
    u8 contestant;
    u32 personality;
    u32 otId;
    u32 targetPersonality;
};

struct ContestResources
{
    struct ContestMoveAnimData *moveAnim;
};

extern struct ContestResources *gContestResources;

struct ContestCategory
{
    const u8 *name;
    const u8 *condition;
    const u8 *generic;
    const u8 *negativeTrait;
    u8 palette;
    u16 tile;
    u8 stdString;
    const u8 *text;
    u8 tvShowState;
    u8 tvShowStateExciting;
};

extern const struct ContestCategory gContestCategoryInfo[CONTEST_CATEGORIES_COUNT + 1];

extern u8 gContestMonPartyIndex;
extern u8 gContestPlayerMonIndex;
extern u8 gLinkContestFlags;
extern u8 gContestLinkLeaderIndex;
extern enum ContestCategories gSpecialVar_ContestCategory;
extern u16 gSpecialVar_ContestRank;
extern u8 gNumLinkContestPlayers;

u8 GetContestEntryEligibility(struct Pokemon *pkmn);
bool8 IsSpeciesNotUnown(enum Species species);
void ResetLinkContestBoolean(void);
void ResetContestLinkResults(void);
void ClearContestWinnerPicsInContestHall(void);

#endif //GUARD_CONTEST_H
