#include "global.h"
#include "achievements.h"
#include "battle_emporium.h"
#include "battle_main.h"
#include "data.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "fieldmap.h"
#include "item.h"
#include "list_menu.h"
#include "malloc.h"
#include "pokemon.h"
#include "random.h"
#include "script.h"
#include "script_menu.h"
#include "sprite.h"
#include "string_util.h"
#include "trainer_pools.h"
#include "caps.h"
#include "config/battle.h"
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

extern u32 GetTotalBaseStat(enum Species species);

// Lobby -> battle room entry trace. Logs to the mGBA console on debug builds and
// compiles out on release. Set to 0 to silence without dropping the asserts.
#define EMPORIUM_TRACE_ENABLED  1
#if EMPORIUM_TRACE_ENABLED
#define EMPORIUM_TRACE(fmt, ...) DebugPrintf("emporium " fmt, ##__VA_ARGS__)
#else
#define EMPORIUM_TRACE(fmt, ...)
#endif

// Every emporium battle room places its challenger as the map's only object event.
#define EMPORIUM_CHALLENGER_LOCAL_ID  1

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

static bool32 MonMatchesRewardRow(const struct TrainerMon *mon, const struct EmporiumReward *reward)
{
    if (reward->emporium == EMPORIUM_TERA)
        return mon->teraType == reward->aceKey;
    return mon->heldItem == reward->item;
}

bool32 EmporiumMonMatchesReward(const struct TrainerMon *mon)
{
    u32 rewardIndex = VarGet(VAR_EMPORIUM_REWARD);

    if (rewardIndex >= EMPORIUM_REWARD_COUNT)
        return FALSE;

    return MonMatchesRewardRow(mon, &gEmporiumRewards[rewardIndex]);
}

// Lowest level `species` can plausibly be reached at: walk its pre-evolution
// chain and take the highest EVO_LEVEL* threshold along it. Non-level methods
// (stone, trade, friendship - the last stored as EVO_LEVEL with param 0)
// contribute nothing, so a stone evolution reports level 1.
u32 EmporiumSpeciesMinLevel(enum Species species)
{
    u32 minLevel = 1;
    enum Species child = species;
    u32 guard;

    for (guard = 0; guard < NUM_SPECIES; guard++)
    {
        enum Species parent = GetSpeciesPreEvolution(child);
        const struct Evolution *evos;

        if (parent == SPECIES_NONE || parent == child)
            break;

        evos = GetSpeciesEvolutions(parent);
        if (evos != NULL)
        {
            u32 i;
            for (i = 0; evos[i].method != EVOLUTIONS_END; i++)
            {
                if (SanitizeSpeciesId(evos[i].targetSpecies) != child)
                    continue;
                if ((evos[i].method == EVO_LEVEL || evos[i].method == EVO_LEVEL_BATTLE_ONLY)
                 && evos[i].param > minLevel)
                    minLevel = evos[i].param;
            }
        }
        child = parent;
    }

    return minLevel;
}

// The Emporium opponent is not a gTrainers entry. It is this one struct, filled
// in when the player accepts a challenge and swapped in for TRAINER_EMPORIUM by
// the gEmporiumBattleActive redirect in GetTrainerStructFromId (include/data.h).
// The flag lives in EWRAM and never in the save block, so a reload always clears
// the redirect. See ClearEmporiumBattle for the teardown contract.
EWRAM_DATA static struct Trainer sEmporiumTrainer = {0};
EWRAM_DATA bool8 gEmporiumBattleActive = FALSE;
EWRAM_DATA static u8 sEmporiumIntroLine = 0;

// Challenger intro lines, rolled per attempt (Stage 9). Labels live in
// data/scripts/battle_emporium.inc; EmporiumBufferChallengerIntro copies the
// chosen one into gStringVar1 for Emporium_Text_ChallengerIntroTemplate.
extern const u8 Emporium_Text_ChallengerIntro1[];
extern const u8 Emporium_Text_ChallengerIntro2[];
extern const u8 Emporium_Text_ChallengerIntro3[];
extern const u8 Emporium_Text_ChallengerIntro4[];
extern const u8 Emporium_Text_ChallengerIntro5[];
extern const u8 Emporium_Text_ChallengerIntro6[];

static const u8 *const sEmporiumIntroLines[] =
{
    Emporium_Text_ChallengerIntro1,
    Emporium_Text_ChallengerIntro2,
    Emporium_Text_ChallengerIntro3,
    Emporium_Text_ChallengerIntro4,
    Emporium_Text_ChallengerIntro5,
    Emporium_Text_ChallengerIntro6,
};

// Tier balance cap (Stage 9): Emporium filler mons may not exceed this base stat
// total. Aces are exempt. Read by EmporiumMonAllowedAsFiller via POOL_PRUNE_EMPORIUM.
u32 GetEmporiumFillerBstCap(void)
{
    switch (VarGet(VAR_EMPORIUM_ID))
    {
    case EMPORIUM_ZMOVE: return 400;
    case EMPORIUM_MEGA:  return 500;
    case EMPORIUM_TERA:  return 600;
    default:             return 600;
    }
}

// FALSE if mon is too strong for its building's tier or is a legendary /
// mythical / paradox / Ultra Beast. Mega stones use the base species' stat total,
// not the Mega form's, because the pool stores the base species.
bool32 EmporiumMonAllowedAsFiller(const struct TrainerMon *mon)
{
    const struct SpeciesInfo *info = &gSpeciesInfo[mon->species];

    if (info->isRestrictedLegendary || info->isSubLegendary || info->isMythical
     || info->isUltraBeast || info->isParadox)
        return FALSE;
    if (GetTotalBaseStat(mon->species) > GetEmporiumFillerBstCap())
        return FALSE;
    return TRUE;
}

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

// Emporium challenger teams fight at the player's current level cap - the same
// cap the player's own Pokemon obey (GetCurrentLevelCap, honouring
// FLAG_LEVEL_CAP_OFF) - so a challenger can never out-level a legal player team.
// The per-building offset is a balance hook (e.g. make the Tera building the
// hardest) and is applied in CreateNPCTrainerPartyFromTrainer.
u32 GetEmporiumBattleLevelForEmporium(u32 emporium)
{
    static const s8 sEmporiumLevelOffset[EMPORIUM_COUNT] =
    {
        [EMPORIUM_ZMOVE] = 0,
        [EMPORIUM_MEGA]  = 0,
        [EMPORIUM_TERA]  = 0,
    };
    s32 level = (s32)GetCurrentLevelCap();

    if (emporium < EMPORIUM_COUNT)
        level += sEmporiumLevelOffset[emporium];

    if (level < 1)
        level = 1;
    if (level > MAX_LEVEL)
        level = MAX_LEVEL;

    return level;
}

u32 GetEmporiumBattleLevel(void)
{
    return GetEmporiumBattleLevelForEmporium(GetEmporiumRewardEmporium(VarGet(VAR_EMPORIUM_REWARD)));
}

// TRUE once the challenger's battle level reaches the lowest cap at which the
// pool holds a legal ace for `reward` (sEmporiumRewardAceMinLevel, precomputed in
// src/data/battle_emporium.h). Rewards below that are hidden from the instructor
// menu until the level cap catches up.
static bool32 EmporiumRewardAceAvailable(u32 rewardIndex)
{
    u32 emporium;

    if (rewardIndex >= EMPORIUM_REWARD_COUNT)
        return FALSE;

    emporium = gEmporiumRewards[rewardIndex].emporium;
    if (emporium >= EMPORIUM_COUNT)
        return FALSE;

    return sEmporiumRewardAceMinLevel[rewardIndex] <= GetEmporiumBattleLevelForEmporium(emporium);
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
    sEmporiumIntroLine = Random() % ARRAY_COUNT(sEmporiumIntroLines);
    pool = &sEmporiumPools[emporium];

    memset(&sEmporiumTrainer, 0, sizeof(sEmporiumTrainer));
    StringCopy(sEmporiumTrainer.trainerName, identity->name);
    sEmporiumTrainer.trainerClass = identity->trainerClass;
    sEmporiumTrainer.trainerPic = identity->trainerPic;
    sEmporiumTrainer.encounterMusic = identity->encounterMusic;
    sEmporiumTrainer.gender = identity->gender;
    sEmporiumTrainer.battleType = TRAINER_BATTLE_TYPE_SINGLES;
    sEmporiumTrainer.aiFlags = AI_FLAG_SMART_TRAINER | AI_FLAG_ACE_POKEMON;
    sEmporiumTrainer.party = pool->party;
    sEmporiumTrainer.partySize = pool->partySize;
    sEmporiumTrainer.poolSize = pool->poolSize;
    sEmporiumTrainer.poolRuleIndex = POOL_RULESET_EMPORIUM;
    sEmporiumTrainer.poolPickIndex = POOL_PICK_EMPORIUM;
    sEmporiumTrainer.poolPruneIndex = POOL_PRUNE_EMPORIUM;
    sEmporiumTrainer.overrideTrainer = TRAINER_NONE;

    gEmporiumBattleActive = TRUE;

    EMPORIUM_TRACE("roll: bldg=%d gfx=%d pic=%d class=%d music=%d gender=%d intro=%d party=%d pool=%d level=%d",
                   emporium, identity->objectGfxId, identity->trainerPic, identity->trainerClass,
                   identity->encounterMusic, identity->gender, sEmporiumIntroLine,
                   sEmporiumTrainer.partySize, sEmporiumTrainer.poolSize, GetEmporiumBattleLevel());
    // Every field below is a narrow bitfield or a table index; a value that does
    // not survive the store is silent corruption at battle setup, not here.
    assertf(sEmporiumTrainer.partySize == pool->partySize, "partySize %d truncated to %d", pool->partySize, sEmporiumTrainer.partySize);
    assertf(sEmporiumTrainer.poolSize == pool->poolSize, "poolSize %d truncated to %d", pool->poolSize, sEmporiumTrainer.poolSize);
    assertf(sEmporiumTrainer.encounterMusic == identity->encounterMusic, "encounterMusic %d truncated to %d", identity->encounterMusic, sEmporiumTrainer.encounterMusic);
    assertf(sEmporiumTrainer.trainerPic < TRAINER_PIC_COUNT, "trainerPic %d out of range", sEmporiumTrainer.trainerPic);
    assertf(StringLength(identity->name) <= TRAINER_NAME_LENGTH, "identity name longer than %d", TRAINER_NAME_LENGTH);
    assertf(identity->objectGfxId < NUM_OBJ_EVENT_GFX, "objectGfxId %d out of range", identity->objectGfxId);
    assertf(pool->party != NULL, "emporium %d has no pool", emporium);

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
#if B_FLAG_NO_WHITEOUT != 0
    FlagClear(B_FLAG_NO_WHITEOUT);
#endif
}

// Lobby ON_TRANSITION teardown. Entering a lobby always means the back room holds
// no live challenge, so disarm the redirect. When the player is walking back in
// from a finished battle (VAR_EMPORIUM_RESULT set) the pending selection is kept
// so the lobby ON_FRAME script can still pay out the reward (win) or re-run the
// same challenge on a retry (loss); otherwise it is dropped, exactly as
// ClearEmporiumBattle would.
void EmporiumLobbyOnTransition(void)
{
    gEmporiumBattleActive = FALSE;
    FlagClear(TRAINER_FLAGS_START + TRAINER_EMPORIUM);
#if B_FLAG_NO_WHITEOUT != 0
    FlagClear(B_FLAG_NO_WHITEOUT);
#endif
    if (VarGet(VAR_EMPORIUM_RESULT) == EMPORIUM_RESULT_NONE)
    {
        VarSet(VAR_EMPORIUM_ID, EMPORIUM_NONE);
        VarSet(VAR_EMPORIUM_REWARD, 0);
    }
}

// Facility-style loss handling. Armed by the battle-room script just before the
// challenger fight so a loss returns to the lobby with the party intact instead
// of a white-out. Skipped in Nuzlocke mode: there an Emporium loss should carry
// the same white-out consequence as any other trainer battle. Cleared again by
// ClearEmporiumBattle (post-battle, on menu cancel, and from each lobby's
// ON_TRANSITION).
void EmporiumArmNoWhiteout(void)
{
#if B_FLAG_NO_WHITEOUT != 0
    if (!gSaveBlock1Ptr->nuzlockeModeEnabled)
        FlagSet(B_FLAG_NO_WHITEOUT);
#endif
}

// Victory payout helper: puts the pending reward's item id in VAR_0x8004 (for
// the giveitem macro) and its name in gStringVar1. Must run before
// ClearEmporiumBattle wipes VAR_EMPORIUM_REWARD.
void EmporiumBufferRewardItem(void)
{
    u32 rewardIndex = VarGet(VAR_EMPORIUM_REWARD);
    enum Item item = GetEmporiumRewardItem(rewardIndex);

    gSpecialVar_0x8004 = item;
    CopyItemName(item, gStringVar1);
    Achievement_OnEmporiumRewardWon(rewardIndex);
}

// Copies the challenger's rolled intro line into gStringVar1 for the battle-room
// trainerbattle intro template. Called just before the fight; the index is rolled
// in BuildEmporiumTrainer alongside the identity.
void EmporiumBufferChallengerIntro(void)
{
    if (sEmporiumIntroLine >= ARRAY_COUNT(sEmporiumIntroLines))
        sEmporiumIntroLine = 0;
    StringCopy(gStringVar1, sEmporiumIntroLines[sEmporiumIntroLine]);
    EMPORIUM_TRACE("pre-battle: intro=%d armed=%d party=%d pool=%d level=%d",
                   sEmporiumIntroLine, gEmporiumBattleActive, sEmporiumTrainer.partySize,
                   sEmporiumTrainer.poolSize, GetEmporiumBattleLevel());
}

// Battle-room ON_FRAME trace (callnative), run just before the walk-in movements.
// Confirms the rolled challenger actually spawned: applymovement on a local id
// that is not on the map would otherwise fail silently one command later.
void EmporiumTraceArena(void)
{
    u32 gfxId = VarGet(VAR_OBJ_GFX_ID_0);
    u8 objectEventId = GetObjectEventIdByLocalIdAndMap(EMPORIUM_CHALLENGER_LOCAL_ID,
                                                       gSaveBlock1Ptr->location.mapNum,
                                                       gSaveBlock1Ptr->location.mapGroup);

    EMPORIUM_TRACE("arena: id=%d reward=%d gfxVar=%d objectEvent=%d playerX=%d playerY=%d",
                   VarGet(VAR_EMPORIUM_ID), VarGet(VAR_EMPORIUM_REWARD), gfxId, objectEventId,
                   gObjectEvents[gPlayerAvatar.objectEventId].currentCoords.x - MAP_OFFSET,
                   gObjectEvents[gPlayerAvatar.objectEventId].currentCoords.y - MAP_OFFSET);

    // The walk-in scene drives two objects while a follower Pokemon may still be
    // spawning, so dump every live object event: localId, graphics, movement type
    // and movement state are what the movement engine indexes its tables with.
    for (u32 i = 0; i < OBJECT_EVENTS_COUNT; i++)
    {
        const struct ObjectEvent *object = &gObjectEvents[i];

        if (!object->active)
            continue;
        EMPORIUM_TRACE("object %d: localId=%d map=%d/%d gfx=%d sprite=%d type=%d x=%d y=%d invisible=%d frozen=%d",
                       i, object->localId, object->mapGroup, object->mapNum, object->graphicsId,
                       object->spriteId, object->movementType,
                       object->currentCoords.x - MAP_OFFSET, object->currentCoords.y - MAP_OFFSET,
                       object->invisible, object->frozen);
        EMPORIUM_TRACE("object %d: action=%d held=%d single=%d copyable=%d typeFunc=%d actionFunc=%d timer=%d",
                       i, object->movementActionId, object->heldMovementActive,
                       object->singleMovementActive, object->playerCopyableMovement,
                       object->spriteId < MAX_SPRITES ? gSprites[object->spriteId].data[1] : -1,
                       object->spriteId < MAX_SPRITES ? gSprites[object->spriteId].data[2] : -1,
                       object->spriteId < MAX_SPRITES ? gSprites[object->spriteId].data[5] : -1);
    }

    // The challenger's overworld sheet is decompressed into dynamically allocated
    // VRAM tiles (OW_GFX_COMPRESS), and its size varies with the rolled identity.
    if (objectEventId < OBJECT_EVENTS_COUNT && gObjectEvents[objectEventId].spriteId < MAX_SPRITES)
    {
        const struct Sprite *sprite = &gSprites[gObjectEvents[objectEventId].spriteId];
        const struct ObjectEventGraphicsInfo *info = GetObjectEventGraphicsInfo(gObjectEvents[objectEventId].graphicsId);

        EMPORIUM_TRACE("challenger sprite: shape=%d size=%d tileNum=%d usingSheet=%d sheetTileStart=%d sheetSpan=%d infoSize=%d",
                       sprite->oam.shape, sprite->oam.size, sprite->oam.tileNum, sprite->usingSheet,
                       sprite->sheetTileStart, sprite->sheetSpan, info->size);
    }

    assertf(objectEventId < OBJECT_EVENTS_COUNT, "emporium challenger did not spawn");
    assertf(gEmporiumBattleActive, "emporium redirect disarmed before the battle");
    assertf(gfxId < NUM_OBJ_EVENT_GFX, "emporium challenger gfx %d out of range", gfxId);
}

// Battle-room walk-in breadcrumb (callnative). The step number comes from
// VAR_0x8005; the script sets it before each call so the last logged step names
// the command that was running when a reset hit.
void EmporiumTraceStep(void)
{
    const struct ObjectEvent *player = &gObjectEvents[gPlayerAvatar.objectEventId];
    u8 challengerId = GetObjectEventIdByLocalIdAndMap(EMPORIUM_CHALLENGER_LOCAL_ID,
                                                      gSaveBlock1Ptr->location.mapNum,
                                                      gSaveBlock1Ptr->location.mapGroup);

    EMPORIUM_TRACE("step %d: player x=%d y=%d action=%d held=%d fin=%d single=%d locked=%d",
                   gSpecialVar_0x8005, player->currentCoords.x - MAP_OFFSET,
                   player->currentCoords.y - MAP_OFFSET, player->movementActionId,
                   player->heldMovementActive, player->heldMovementFinished,
                   player->singleMovementActive, ArePlayerFieldControlsLocked());
    if (challengerId < OBJECT_EVENTS_COUNT)
    {
        const struct ObjectEvent *challenger = &gObjectEvents[challengerId];

        EMPORIUM_TRACE("step %d: challenger x=%d y=%d action=%d held=%d fin=%d single=%d frozen=%d",
                       gSpecialVar_0x8005, challenger->currentCoords.x - MAP_OFFSET,
                       challenger->currentCoords.y - MAP_OFFSET, challenger->movementActionId,
                       challenger->heldMovementActive, challenger->heldMovementFinished,
                       challenger->singleMovementActive, challenger->frozen);
    }
}

// ---- Stage 7: instructor reward menu, ace preview, opponent roll ----
//
// The instructor script passes the building's enum EmporiumId in VAR_0x8004. It
// stays set for the whole menu interaction; VAR_EMPORIUM_ID / VAR_EMPORIUM_REWARD
// are only written once the player commits to a challenge.

// Reward "already owned" test. An item that overflowed to the PC on pickup, or is
// held by a party Pokemon, still counts as owned so it drops off the menu. Box
// storage is not scanned - decrypting every slot per reward row stalled the menu.
static bool32 PlayerOwnsRewardItem(enum Item item)
{
    u32 i;

    if (CheckBagHasItem(item, 1) || CheckPCHasItem(item, 1))
        return TRUE;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (GetMonData(&gPlayerParty[i], MON_DATA_SPECIES) == SPECIES_NONE)
            continue;
        if (GetMonData(&gPlayerParty[i], MON_DATA_HELD_ITEM) == item)
            return TRUE;
    }

    return FALSE;
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
        if (PlayerOwnsRewardItem(reward->item))
            continue;
        // Hidden until the level cap can field a legal ace for this reward.
        if (!EmporiumRewardAceAvailable(start + i))
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

// Fills gStringVar1 with the chosen reward's name for the confirm prompt. The
// challenger's ace is deliberately not previewed - it would spoil the surprise
// and, for the Tera building, only restate the shard's type.
void EmporiumMenu_BufferConfirm(void)
{
    u32 rewardIndex = VarGet(VAR_EMPORIUM_REWARD);

    if (rewardIndex >= EMPORIUM_REWARD_COUNT)
        return;

    CopyItemName(gEmporiumRewards[rewardIndex].item, gStringVar1);
}

// Rolls the challenger for the pending challenge (VAR_EMPORIUM_ID) and writes the
// identity's overworld graphics id to VAR_OBJ_GFX_ID_0 for the back-room object.
// BuildEmporiumTrainer arms the gEmporiumBattleActive redirect.
void EmporiumRollChallenger(void)
{
    VarSet(VAR_OBJ_GFX_ID_0, BuildEmporiumTrainer(VarGet(VAR_EMPORIUM_ID)));
}

// Battle-room ON_TRANSITION guard. gEmporiumBattleActive lives in EWRAM and is
// gone after any reload, but VAR_EMPORIUM_* and the shown challenger object are
// saved state. If the redirect is not armed there is no live challenge: re-hide
// the challengers and drop the stale vars, so a save made inside a battle room
// can never talk a challenger into fighting the empty TRAINER_EMPORIUM stub.
// Entering from the lobby keeps gEmporiumBattleActive set (EmporiumRollChallenger
// armed it), so this is a no-op on the legitimate path.
void EmporiumBattleRoomOnTransition(void)
{
    EMPORIUM_TRACE("room transition: armed=%d id=%d reward=%d gfxVar=%d level=%d",
                   gEmporiumBattleActive, VarGet(VAR_EMPORIUM_ID), VarGet(VAR_EMPORIUM_REWARD),
                   VarGet(VAR_OBJ_GFX_ID_0), GetEmporiumBattleLevel());

    if (!gEmporiumBattleActive)
    {
        FlagSet(FLAG_EMPORIUM_ZMOVE_CHALLENGER_HIDDEN);
        FlagSet(FLAG_EMPORIUM_MEGA_CHALLENGER_HIDDEN);
        FlagSet(FLAG_EMPORIUM_TERA_CHALLENGER_HIDDEN);
        ClearEmporiumBattle();
    }
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
