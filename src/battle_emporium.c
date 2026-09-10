#include "global.h"
#include "battle_emporium.h"
#include "data.h"
#include "event_data.h"
#include "trainer_pools.h"
#include "constants/event_objects.h"
#include "constants/flags.h"
#include "constants/moves.h"
#include "constants/species.h"
#include "constants/trainers.h"
#include "constants/vars.h"

#include "data/battle_emporium.h"

struct EmporiumRewardRange
{
    u16 start;
    u16 count;
};

static const struct EmporiumRewardRange sEmporiumRanges[EMPORIUM_COUNT] =
{
    [EMPORIUM_ZMOVE] = { EMPORIUM_ZMOVE_REWARD_START, EMPORIUM_ZMOVE_REWARD_COUNT },
    [EMPORIUM_MEGA]  = { EMPORIUM_MEGA_REWARD_START,  EMPORIUM_MEGA_REWARD_COUNT },
    [EMPORIUM_TERA]  = { EMPORIUM_TERA_REWARD_START,  EMPORIUM_TERA_REWARD_COUNT },
};

u32 GetEmporiumRewardCount(u32 emporium)
{
    if (emporium >= EMPORIUM_COUNT)
        return 0;
    return sEmporiumRanges[emporium].count;
}

u32 GetEmporiumRewardStart(u32 emporium)
{
    if (emporium >= EMPORIUM_COUNT)
        return 0;
    return sEmporiumRanges[emporium].start;
}

enum Item GetEmporiumRewardItem(u32 rewardIndex)
{
    if (rewardIndex >= EMPORIUM_REWARD_COUNT)
        return ITEM_NONE;
    return gEmporiumRewards[rewardIndex].item;
}

u32 GetEmporiumRewardEmporium(u32 rewardIndex)
{
    if (rewardIndex >= EMPORIUM_REWARD_COUNT)
        return EMPORIUM_NONE;
    return gEmporiumRewards[rewardIndex].emporium;
}

u16 GetEmporiumRewardRequiredFlag(u32 rewardIndex)
{
    if (rewardIndex >= EMPORIUM_REWARD_COUNT)
        return EMPORIUM_FLAG_NONE;
    return gEmporiumRewards[rewardIndex].requiredFlag;
}

u16 GetEmporiumAceKey(void)
{
    u32 rewardIndex = VarGet(VAR_EMPORIUM_REWARD);
    const struct EmporiumReward *reward;

    if (rewardIndex >= EMPORIUM_REWARD_COUNT)
        return 0;

    reward = &gEmporiumRewards[rewardIndex];
    if (reward->emporium == EMPORIUM_TERA)
        return reward->aceKey;
    return reward->item;
}
