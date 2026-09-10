#include "global.h"
#include "battle_emporium.h"
#include "data.h"
#include "event_data.h"
#include "random.h"
#include "string_util.h"
#include "trainer_pools.h"
#include "caps.h"
#include "constants/battle_ai.h"
#include "constants/event_objects.h"
#include "constants/flags.h"
#include "constants/moves.h"
#include "constants/pokemon.h"
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

bool32 EmporiumMonMatchesReward(const struct TrainerMon *mon)
{
    u32 rewardIndex = VarGet(VAR_EMPORIUM_REWARD);
    const struct EmporiumReward *reward;

    if (rewardIndex >= EMPORIUM_REWARD_COUNT)
        return FALSE;

    reward = &gEmporiumRewards[rewardIndex];
    if (reward->emporium == EMPORIUM_TERA)
        return mon->teraType == reward->aceKey;
    return mon->heldItem == reward->item;
}

// The Emporium opponent is not a gTrainers entry. It is this one struct, filled
// in when the player accepts a challenge and swapped in for TRAINER_EMPORIUM by
// the gEmporiumBattleActive redirect in GetTrainerStructFromId (include/data.h).
// The flag lives in EWRAM and never in the save block, so a reload always clears
// the redirect. See ClearEmporiumBattle for the teardown contract.
EWRAM_DATA static struct Trainer sEmporiumTrainer = {0};
EWRAM_DATA bool8 gEmporiumBattleActive = FALSE;

struct EmporiumPoolInfo
{
    const struct TrainerMon *party;
    u8 poolSize;
    u8 partySize;
};

static const struct EmporiumPoolInfo sEmporiumPools[EMPORIUM_COUNT] =
{
    [EMPORIUM_ZMOVE] = { sEmporiumZPool,    EMPORIUM_ZMOVE_POOL_SIZE, EMPORIUM_PARTY_SIZE_ZMOVE },
    [EMPORIUM_MEGA]  = { sEmporiumMegaPool, EMPORIUM_MEGA_POOL_SIZE,  EMPORIUM_PARTY_SIZE_MEGA  },
    [EMPORIUM_TERA]  = { sEmporiumTeraPool, EMPORIUM_TERA_POOL_SIZE,  EMPORIUM_PARTY_SIZE_TERA  },
};

const struct Trainer *GetEmporiumTrainer(void)
{
    return &sEmporiumTrainer;
}

// Emporium challenger teams always fight at the story/badge progression level
// cap, so they keep pace with the player regardless of difficulty or New Game+.
// The per-building offset is a balance hook (e.g. make the Tera building the
// hardest) and is applied in CreateNPCTrainerPartyFromTrainer.
u32 GetEmporiumBattleLevel(void)
{
    static const s8 sEmporiumLevelOffset[EMPORIUM_COUNT] =
    {
        [EMPORIUM_ZMOVE] = 0,
        [EMPORIUM_MEGA]  = 0,
        [EMPORIUM_TERA]  = 0,
    };
    u32 emporium = GetEmporiumRewardEmporium(VarGet(VAR_EMPORIUM_REWARD));
    s32 level = (s32)GetProgressionLevelCap();

    if (emporium < EMPORIUM_COUNT)
        level += sEmporiumLevelOffset[emporium];

    if (level < 1)
        level = 1;
    if (level > MAX_LEVEL)
        level = MAX_LEVEL;

    return level;
}

// Rolls a challenger identity, fills sEmporiumTrainer from it and the emporium's
// mon pool, and arms the redirect. Returns the identity's overworld graphics id
// so the caller can write VAR_OBJ_GFX_ID_0 for the back-room challenger object.
u16 BuildEmporiumTrainer(u32 emporium)
{
    const struct EmporiumIdentity *identity;
    const struct EmporiumPoolInfo *pool;

    if (emporium >= EMPORIUM_COUNT || emporium == EMPORIUM_NONE)
        emporium = EMPORIUM_ZMOVE;

    identity = &sEmporiumIdentities[Random() % EMPORIUM_IDENTITY_COUNT];
    pool = &sEmporiumPools[emporium];

    memset(&sEmporiumTrainer, 0, sizeof(sEmporiumTrainer));
    StringCopy(sEmporiumTrainer.trainerName, identity->name);
    sEmporiumTrainer.trainerClass = identity->trainerClass;
    sEmporiumTrainer.trainerPic = identity->trainerPic;
    sEmporiumTrainer.encounterMusic = identity->encounterMusic;
    sEmporiumTrainer.gender = identity->gender;
    sEmporiumTrainer.battleType = TRAINER_BATTLE_TYPE_SINGLES;
    sEmporiumTrainer.aiFlags = AI_FLAG_SMART_TRAINER;
    sEmporiumTrainer.party = pool->party;
    sEmporiumTrainer.partySize = pool->partySize;
    sEmporiumTrainer.poolSize = pool->poolSize;
    sEmporiumTrainer.poolRuleIndex = POOL_RULESET_EMPORIUM;
    sEmporiumTrainer.poolPickIndex = POOL_PICK_EMPORIUM;
    sEmporiumTrainer.poolPruneIndex = POOL_PRUNE_NONE;
    sEmporiumTrainer.overrideTrainer = TRAINER_NONE;

    gEmporiumBattleActive = TRUE;
    return identity->objectGfxId;
}

// Disarms the redirect and drops the pending-challenge state. Called after the
// battle (win or loss), on menu cancel, and defensively from each emporium map's
// ON_TRANSITION, so a crash or white-out can never leave every trainer in the
// game pointed at sEmporiumTrainer.
void ClearEmporiumBattle(void)
{
    gEmporiumBattleActive = FALSE;
    VarSet(VAR_EMPORIUM_ID, EMPORIUM_NONE);
    VarSet(VAR_EMPORIUM_REWARD, 0);
    FlagClear(TRAINER_FLAGS_START + TRAINER_EMPORIUM);
}
