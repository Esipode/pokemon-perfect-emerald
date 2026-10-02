#include "global.h"
#include "battle.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "limited_party.h"
#include "overworld.h"
#include "pokemon.h"
#include "tv.h"
#include "constants/pokemon.h"
#include "constants/tv.h"

// Script specials used only by the Johto maps.

// Challenge hooks. The target has no Nuzlocke-nickname, Pokecenter-challenge or random-moves option.
u16 IsNuzlockeNicknamingActive(void)
{
    return FALSE;
}

u16 IsPokecenterChallengeActivated(void)
{
    return FALSE;
}

u16 IsRandomMovesActivated(void)
{
    return FALSE;
}

u16 IsPartyLimitChallengeActive(void)
{
    return LimitedParty_IsEnabled();
}

u16 GetMaxPartySize(void)
{
    return LimitedParty_GetMaxPartySize();
}

void HaircutBrother1(void)
{
    AdjustFriendship(&gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004], FRIENDSHIP_EVENT_HAIRCUT1);
}

void HaircutBrother2(void)
{
    AdjustFriendship(&gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004], FRIENDSHIP_EVENT_HAIRCUT2);
}

// Gives every party member the ribbon. The Johto scripts set their own "received" flag.
static void GivePartyRibbon(u8 ribbonDataId)
{
    bool8 ribbonSet = TRUE;

    IncrementGameStat(GAME_STAT_RECEIVED_RIBBONS);
    for (u32 i = 0; i < gPartiesCount[B_TRAINER_PLAYER]; i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];

        if (GetMonData(mon, MON_DATA_SPECIES) == SPECIES_NONE)
            continue;
        SetMonData(mon, ribbonDataId, &ribbonSet);
        if (GetRibbonCount(mon) > NUM_CUTIES_RIBBONS)
            TryPutSpotTheCutiesOnAir(mon, ribbonDataId);
    }
}

void GivePartyMonChampionRibbon(void)
{
    GivePartyRibbon(MON_DATA_CHAMPION_RIBBON);
}

void GivePartyMonLandRibbon(void)
{
    GivePartyRibbon(MON_DATA_LAND_RIBBON);
}

void GivePartyMonNationalRibbon(void)
{
    GivePartyRibbon(MON_DATA_NATIONAL_RIBBON);
}

// True when a full-HP Celebi leads the party and is visible as the follower.
bool8 CheckCelebi(void)
{
    struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][0];
    struct ObjectEvent *follower;

    if (GetMonData(mon, MON_DATA_SPECIES_OR_EGG) != SPECIES_CELEBI)
        return FALSE;
    if (GetMonData(mon, MON_DATA_HP) != GetMonData(mon, MON_DATA_MAX_HP))
        return FALSE;

    follower = GetFollowerObject();
    return follower != NULL && !follower->invisible;
}
