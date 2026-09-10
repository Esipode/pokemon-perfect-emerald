#include "global.h"
#include "battle_emporium.h"
#include "battle_main.h"
#include "data.h"
#include "event_data.h"
#include "item.h"
#include "list_menu.h"
#include "malloc.h"
#include "pokemon.h"
#include "random.h"
#include "script_menu.h"
#include "string_util.h"
#include "trainer_pools.h"
#include "caps.h"
#include "constants/battle_ai.h"
#include "constants/event_objects.h"
#include "constants/flags.h"
#include "constants/items.h"
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

// ---- Stage 7: instructor reward menu, ace preview, opponent roll ----
//
// The instructor script passes the building's enum EmporiumId in VAR_0x8004. It
// stays set for the whole menu interaction; VAR_EMPORIUM_ID / VAR_EMPORIUM_REWARD
// are only written once the player commits to a challenge.

// First ACE-tagged pool member whose key matches the reward, or SPECIES_NONE.
// Mirrors the eligibility test in EmporiumAcePickFunction (src/trainer_pools.c).
static enum Species GetEmporiumPreviewAce(const struct EmporiumReward *reward)
{
    const struct TrainerMon *pool;
    u32 poolSize, i;

    if (reward->emporium >= EMPORIUM_COUNT)
        return SPECIES_NONE;

    pool = sEmporiumPools[reward->emporium].party;
    poolSize = sEmporiumPools[reward->emporium].poolSize;
    for (i = 0; i < poolSize; i++)
    {
        if (!(pool[i].tags & MON_POOL_TAG_ACE))
            continue;
        if (reward->emporium == EMPORIUM_TERA)
        {
            if (pool[i].teraType == reward->aceKey)
                return pool[i].species;
        }
        else if (pool[i].heldItem == reward->item)
        {
            return pool[i].species;
        }
    }
    return SPECIES_NONE;
}

// Pushes every reward for the building in VAR_0x8004 onto the dynamic multichoice
// stack (consumed by dynmultistack). A row is skipped when its unlock flag is
// unset or the player already holds that item (owned rewards are hidden, per plan
// section 5.3). The option id is the item constant, so DYN_MULTICHOICE_CB_SHOW_ITEM
// draws its icon and EmporiumMenu_CommitReward can map the pick back to a row.
// VAR_RESULT is set to the number of rows pushed (0 when none are available).
void EmporiumMenu_BuildList(void)
{
    u32 emporium = VarGet(VAR_0x8004);
    u32 start = GetEmporiumRewardStart(emporium);
    u32 count = GetEmporiumRewardCount(emporium);
    u32 i, pushed = 0;

    for (i = 0; i < count; i++)
    {
        const struct EmporiumReward *reward = &gEmporiumRewards[start + i];
        struct ListMenuItem item;
        u8 *name;

        if (reward->requiredFlag != EMPORIUM_FLAG_NONE && !FlagGet(reward->requiredFlag))
            continue;
        if (CheckBagHasItem(reward->item, 1))
            continue;

        name = Alloc(32);
        CopyItemName(reward->item, name);
        item.name = name;
        item.id = reward->item;
        MultichoiceDynamic_PushElement(item);
        pushed++;
    }

    gSpecialVar_Result = pushed;
}

// Maps the item id the menu returned (still in VAR_RESULT) back to its catalogue
// row and stores that index in VAR_EMPORIUM_REWARD. VAR_RESULT becomes TRUE on a
// hit, FALSE otherwise (menu only lists valid rows, so FALSE means re-open it).
void EmporiumMenu_CommitReward(void)
{
    u32 emporium = VarGet(VAR_0x8004);
    u32 start = GetEmporiumRewardStart(emporium);
    u32 count = GetEmporiumRewardCount(emporium);
    enum Item picked = gSpecialVar_Result;
    u32 i;

    for (i = 0; i < count; i++)
    {
        if (gEmporiumRewards[start + i].item == picked)
        {
            VarSet(VAR_EMPORIUM_REWARD, start + i);
            gSpecialVar_Result = TRUE;
            return;
        }
    }
    gSpecialVar_Result = FALSE;
}

// Fills gStringVar1 with the chosen reward's name and gStringVar2 with a preview
// of the challenger's ace: the Tera type for the Tera building, otherwise the
// species that brings the selected stone / crystal. Used by the confirm prompt.
void EmporiumMenu_BufferConfirm(void)
{
    u32 rewardIndex = VarGet(VAR_EMPORIUM_REWARD);
    const struct EmporiumReward *reward;
    enum Species ace;

    if (rewardIndex >= EMPORIUM_REWARD_COUNT)
        return;

    reward = &gEmporiumRewards[rewardIndex];
    CopyItemName(reward->item, gStringVar1);

    if (reward->emporium == EMPORIUM_TERA)
    {
        StringCopy(gStringVar2, gTypesInfo[reward->aceKey].name);
        return;
    }

    ace = GetEmporiumPreviewAce(reward);
    if (ace != SPECIES_NONE)
        StringCopy(gStringVar2, GetSpeciesName(ace));
    else
        StringCopy(gStringVar2, COMPOUND_STRING("its ace"));
}

// Rolls the challenger for the pending challenge (VAR_EMPORIUM_ID) and writes the
// identity's overworld graphics id to VAR_OBJ_GFX_ID_0 for the back-room object.
// BuildEmporiumTrainer arms the gEmporiumBattleActive redirect.
void EmporiumRollChallenger(void)
{
    VarSet(VAR_OBJ_GFX_ID_0, BuildEmporiumTrainer(VarGet(VAR_EMPORIUM_ID)));
}

// Reveals the battle-room challenger for the building the challenge is in.
void EmporiumShowChallenger(void)
{
    switch (VarGet(VAR_EMPORIUM_ID))
    {
    case EMPORIUM_ZMOVE:
        FlagClear(FLAG_EMPORIUM_ZMOVE_CHALLENGER_HIDDEN);
        break;
    case EMPORIUM_MEGA:
        FlagClear(FLAG_EMPORIUM_MEGA_CHALLENGER_HIDDEN);
        break;
    case EMPORIUM_TERA:
        FlagClear(FLAG_EMPORIUM_TERA_CHALLENGER_HIDDEN);
        break;
    }
}
