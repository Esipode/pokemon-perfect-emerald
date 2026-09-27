#include "global.h"
#include "infinity_cave.h"
#include "battle_setup.h"
#include "event_data.h"
#include "item.h"
#include "list_menu.h"
#include "malloc.h"
#include "overworld_overlay.h"
#include "random.h"
#include "script_menu.h"
#include "script_pokemon_util.h"
#include "string_util.h"
#include "config/battle.h"
#include "constants/characters.h"
#include "constants/flags.h"
#include "constants/items.h"
#include "constants/opponents.h"
#include "constants/overworld_overlay.h"
#include "constants/vars.h"
#include "data/infinity_cave_rooms.h"

static struct InfinityCaveRun *Run(void)
{
    return &gSaveBlock1Ptr->infinityCaveRun;
}

bool32 InfCave_IsInRun(void)
{
    return Run()->active != 0;
}

u32 InfCave_GetDepth(void)
{
    return Run()->depth;
}

u32 InfCave_GetShards(void)
{
    return Run()->shards;
}

void InfCave_StartRun(void)
{
    struct InfinityCaveRun *run = Run();

    memset(run, 0, sizeof(*run));
    InfCave_ClearTrainers();
    run->runSeed = Random32();
    run->active = TRUE;
    run->depth = 0;
    run->roomSeed = run->runSeed;
}

void InfCave_EndRun(enum InfCaveEndReason reason)
{
    // reason is what the caller already acted on (payout, warp, messaging); the
    // run struct keeps no history of it.
    (void)reason;
    InfCave_ClearTrainers();
#if B_FLAG_NO_WHITEOUT != 0
    FlagClear(B_FLAG_NO_WHITEOUT);
#endif
    memset(Run(), 0, sizeof(struct InfinityCaveRun));
}

// Facility-style loss handling, armed by the room generator so it also covers the
// continue-from-save path: a cave defeat ends the descent and returns the player
// to the lobby instead of whiting out. Skipped in Nuzlocke mode, where a loss
// keeps the normal white-out consequence (src/battle_setup.c branches on the same
// condition). Cleared by InfCave_EndRun.
void InfCave_ArmNoWhiteout(void)
{
#if B_FLAG_NO_WHITEOUT != 0
    if (!gSaveBlock1Ptr->nuzlockeModeEnabled)
        FlagSet(B_FLAG_NO_WHITEOUT);
#endif
}

void InfCave_EndRunQuit(void)
{
    InfCave_EndRun(INFCAVE_END_QUIT);
}

// A save made inside a room only replays if the run struct still describes that
// room. Reject anything the generator and the room scripts cannot act on: no
// active run, the lobby depth, a zeroed room seed, or an out-of-range room type
// or modifier.
bool32 InfCave_IsRunConsistent(void)
{
    struct InfinityCaveRun *run = Run();
    u32 i;

    if (!run->active || run->depth == 0 || run->roomSeed == 0)
        return FALSE;

    if (run->roomType >= INFCAVE_ROOM_COUNT)
        return FALSE;

    for (i = 0; i < INFCAVE_MAX_MODIFIERS; i++)
    {
        if (run->modifier[i] >= INFCAVE_MOD_COUNT)
            return FALSE;
    }

    return TRUE;
}

// Room ON_LOAD special. ON_TRANSITION does not run on the continue-from-save
// path, so the check lives in ON_LOAD, which both the warp and the reload paths
// reach through InfCave_GenerateRoom. VAR_TEMP_1 hands the eject to the room's
// ON_FRAME script, since a warp cannot run from ON_LOAD.
void InfCave_ValidateRoom(void)
{
    VarSet(VAR_TEMP_1, InfCave_IsRunConsistent() ? 0 : 1);
}

void InfCave_AdvanceDepth(void)
{
    struct InfinityCaveRun *run = Run();

    // The next room rolls its own opponents into the same slots, so the stub ids'
    // defeat flags have to go with the depth the player just left. Clearing them
    // here rather than in the generator keeps a reload inside a room from
    // resurrecting the trainers already beaten in it.
    InfCave_ClearTrainers();

    // What the player took out of the room they are leaving goes with the depth
    // too: the next room rebuilds its own item balls and shrine from its seed.
    run->roomFlags = 0;

    run->depth++;
    // Derived from runSeed and depth alone, so the room is identical no matter
    // which nodes were taken to reach this depth.
    run->roomSeed = ISO_RANDOMIZE2(run->runSeed + run->depth * 2654435761u);

    // Default room type for the new depth: a boss on the cadence, a battle
    // otherwise. The node roll overwrites this with the option the player picked,
    // and the debug room forcer overwrites it too, since both call InfCave_SetRoom
    // after this.
    InfCave_SetRoom((run->depth % INFCAVE_BOSS_INTERVAL) == 0 ? INFCAVE_ROOM_BOSS
                                                             : INFCAVE_ROOM_BATTLE,
                    NULL, NULL);
}

// Depth tint. Each floor rotates the cave's hue another
// INFCAVE_HUE_PERCENT_PER_DEPTH of the circle, so a deep run reads as a
// different place than a shallow one. The overlay is OVERLAY_LAYER_WORLD, so it
// moves the map tiles only; NPCs, the player and the UI keep their own colours.
// Called from the room generator, which both the warp and the reload path reach
// after Overlay_ResetAll has cleared the previous floor's overlay.
void InfCave_ApplyDepthHue(void)
{
    struct OverlayConfig config = {0};
    u32 hue = (InfCave_GetDepth() * OVERLAY_HUE_FULL_TURN * INFCAVE_HUE_PERCENT_PER_DEPTH) / 100;

    // The angle is a full u8 turn, so the shift wraps rather than saturating.
    hue &= OVERLAY_HUE_FULL_TURN - 1;
    if (hue == 0)
        return; // depth 0, and every wrap back to it, needs no overlay

    config.color = hue;
    config.opacity = OVERLAY_OPACITY_MAX;
    config.layer = OVERLAY_LAYER_WORLD;
    config.scope = OVERLAY_SCOPE_MAP_LOCAL;
    config.effect = OVERLAY_EFFECT_HUE_SHIFT;
    Overlay_Create(&config);
}

void InfCave_SetRoom(u32 roomType, const u8 *modifiers, const u8 *modifierArgs)
{
    struct InfinityCaveRun *run = Run();
    u32 i;

    run->roomType = roomType;
    for (i = 0; i < INFCAVE_MAX_MODIFIERS; i++)
    {
        run->modifier[i] = (modifiers != NULL) ? modifiers[i] : (u8)INFCAVE_MOD_NONE;
        run->modifierArg[i] = (modifierArgs != NULL) ? modifierArgs[i] : 0;
    }
}

// Descent ladder gate, read with specialvar. A boss room's ladder stays sealed
// until the boss is beaten, so the arena fight cannot be walked past. The stub
// id's defeat flag is cleared on every depth change, so the next room's ladder
// is free again.
u16 InfCave_IsExitLocked(void)
{
    if (Run()->roomType != INFCAVE_ROOM_BOSS)
        return FALSE;

    // A room that stood no trainer at all has no boss to beat; leaving the gate
    // armed there would strand the player.
    if (InfCave_GetRoomTrainerCount() == 0)
        return FALSE;

    return !HasTrainerBeenFought(TRAINER_INFCAVE_BOSS);
}

u32 InfCave_GetRoomType(void)
{
    return Run()->roomType;
}

u32 InfCave_GetModifier(u32 slot)
{
    if (slot >= INFCAVE_MAX_MODIFIERS)
        return INFCAVE_MOD_NONE;
    return Run()->modifier[slot];
}

u32 InfCave_GetModifierArg(u32 slot)
{
    if (slot >= INFCAVE_MAX_MODIFIERS)
        return 0;
    return Run()->modifierArg[slot];
}

bool32 InfCave_HasModifier(u32 modifier)
{
    u32 i;

    if (modifier == INFCAVE_MOD_NONE)
        return FALSE;

    for (i = 0; i < INFCAVE_MAX_MODIFIERS; i++)
    {
        if (Run()->modifier[i] == modifier)
            return TRUE;
    }
    return FALSE;
}

void InfCave_AddShards(u32 amount)
{
    struct InfinityCaveRun *run = Run();
    u32 total = run->shards + amount;

    run->shards = total > 0xFFFF ? 0xFFFF : total;
}

bool32 InfCave_SpendShards(u32 amount)
{
    struct InfinityCaveRun *run = Run();

    if (run->shards < amount)
        return FALSE;

    run->shards -= amount;
    return TRUE;
}

// salt keeps unrelated consumers of one room (mask, decoration, trainers) on
// separate streams; pass a distinct constant from each.
rng_value_t InfCave_SeedRoomRng(u32 salt)
{
    return LocalRandomSeed(Run()->roomSeed ^ salt);
}

// Rolls that must be stable for a whole run rather than rebuilt per room draw
// from runSeed instead, so advancing the depth does not reshuffle them.
rng_value_t InfCave_SeedRunRng(u32 salt)
{
    return LocalRandomSeed(Run()->runSeed ^ salt);
}

u32 InfCave_Rand(rng_value_t *rng)
{
    return LocalRandom32(rng);
}

// Inclusive on both ends. lo > hi returns lo.
u32 InfCave_RandRange(rng_value_t *rng, u32 lo, u32 hi)
{
    if (hi <= lo)
        return lo;
    return lo + LocalRandom32(rng) % (hi - lo + 1);
}

// --- Rest, treasure and shop rooms ------------------------------------------

// Separate RNG streams inside one room, following the generator's convention: a
// new consumer must not shift the numbers an existing one draws.
#define INFCAVE_SALT_BALLS 0x42414C4Cu // 'BALL', the ball count
#define INFCAVE_SALT_DROP  0x44524F50u // 'DROP', plus the ball slot
#define INFCAVE_SALT_TRADE 0x54524144u // 'TRAD', the merchant's trade item

static bool32 RoomFlagGet(u32 mask)
{
    return (Run()->roomFlags & mask) != 0;
}

static void RoomFlagSet(u32 mask)
{
    Run()->roomFlags |= mask;
}

// Item tier the current depth rolls on, saturating at the deepest tier.
static u32 ItemTier(void)
{
    u32 tier = InfCave_GetDepth() / INFCAVE_ITEM_TIER_DEPTH;

    return tier >= INFCAVE_ITEM_TIER_COUNT ? INFCAVE_ITEM_TIER_COUNT - 1 : tier;
}

// Picks one row of table by weight, from the rows the depth's tier has unlocked.
// NULL only when the tier unlocked none, which the tables are authored against:
// both hold tier 0 rows.
static const struct InfCaveItemDrop *RollDrop(const struct InfCaveItemDrop *table, u32 count, rng_value_t *rng)
{
    u32 tier = ItemTier();
    u32 total = 0, roll, i;

    for (i = 0; i < count; i++)
    {
        if (table[i].minTier <= tier)
            total += table[i].weight;
    }

    if (total == 0)
        return NULL;

    roll = InfCave_RandRange(rng, 0, total - 1);
    for (i = 0; i < count; i++)
    {
        if (table[i].minTier > tier)
            continue;
        if (roll < table[i].weight)
            return &table[i];
        roll -= table[i].weight;
    }
    return NULL;
}

u32 InfCave_RollItemBallCount(void)
{
    rng_value_t rng;

    if (InfCave_GetRoomType() != INFCAVE_ROOM_TREASURE)
        return 0;

    rng = InfCave_SeedRoomRng(INFCAVE_SALT_BALLS);
    return InfCave_RandRange(&rng, INFCAVE_TREASURE_MIN_BALLS, INFCAVE_TREASURE_MAX_BALLS);
}

bool32 InfCave_IsItemBallTaken(u32 slot)
{
    if (slot >= INFCAVE_MAX_ITEM_BALLS)
        return TRUE;

    return RoomFlagGet(INFCAVE_ROOMFLAG_BALL(slot));
}

// Item ball script special. The ball's slot arrives in VAR_0x800A; the item and
// the amount leave in the vars STD_FIND_ITEM reads. The roll is a pure function
// of the room seed and the slot, so a ball holds the same item after a reload and
// a save-scum cannot reroll it.
void InfCave_SetItemBallItem(void)
{
    u32 slot = VarGet(VAR_0x800A);
    rng_value_t rng = InfCave_SeedRoomRng(INFCAVE_SALT_DROP + slot);
    const struct InfCaveItemDrop *drop;

    // Ball 0 holds the room's depth-scaled drop; the rest hold ordinary supplies.
    if (slot == 0)
        drop = RollDrop(sInfCavePremiumDrops, ARRAY_COUNT(sInfCavePremiumDrops), &rng);
    else
        drop = RollDrop(sInfCaveCommonDrops, ARRAY_COUNT(sInfCaveCommonDrops), &rng);

    gSpecialVar_0x8000 = (drop != NULL) ? drop->item : ITEM_POTION;
    gSpecialVar_0x8001 = (drop != NULL) ? drop->amount : 1;
}

// Records that the ball in VAR_0x800A was emptied, so the generator leaves it out
// when the room is rebuilt.
void InfCave_MarkItemBallTaken(void)
{
    u32 slot = VarGet(VAR_0x800A);

    if (slot < INFCAVE_MAX_ITEM_BALLS)
        RoomFlagSet(INFCAVE_ROOMFLAG_BALL(slot));
}

// Rest room shrine, read with specialvar. One heal per room; the bit lives in the
// run struct, so stepping back onto the pad after a reload finds it spent.
u16 InfCave_IsShrineUsed(void)
{
    return RoomFlagGet(INFCAVE_ROOMFLAG_SHRINE);
}

void InfCave_UseShrine(void)
{
    RoomFlagSet(INFCAVE_ROOMFLAG_SHRINE);
}

// TRUE while a stock row is on offer here: the depth reaches it, and a one-off
// service has not already been bought in this room.
static bool32 ShopRowStocked(const struct InfCaveShopEntry *entry)
{
    if (InfCave_GetDepth() < entry->minDepth)
        return FALSE;
    if (entry->kind == INFCAVE_SHOP_KIND_FULL_HEAL && RoomFlagGet(INFCAVE_ROOMFLAG_FULL_HEAL))
        return FALSE;

    return TRUE;
}

// The premium item the shard-for-item trade hands over. Fixed per room, so the
// trade cannot be rerolled by leaving the menu or reloading the save.
static const struct InfCaveItemDrop *TradeDrop(void)
{
    rng_value_t rng = InfCave_SeedRoomRng(INFCAVE_SALT_TRADE);

    return RollDrop(sInfCavePremiumDrops, ARRAY_COUNT(sInfCavePremiumDrops), &rng);
}

// Pushes one dynamic multichoice row per stocked entry, each row its padded name
// followed by its shard price, reddened when the run cannot afford it. The row's
// id is its index in sInfCaveShopStock, which is what InfCaveShop_Buy reads back.
// VAR_RESULT is the number of rows pushed; gStringVar3 is the shard balance, for
// the message the menu opens under.
void InfCaveShop_BuildList(void)
{
    u32 i, pushed = 0;

    for (i = 0; i < ARRAY_COUNT(sInfCaveShopStock); i++)
    {
        const struct InfCaveShopEntry *entry = &sInfCaveShopStock[i];
        struct ListMenuItem row;
        u8 *text, *tail;

        if (!ShopRowStocked(entry))
            continue;

        text = Alloc(48);
        tail = text;
        if (InfCave_GetShards() < entry->price)
            tail = StringCopy(tail, COMPOUND_STRING("{COLOR RED}{SHADOW LIGHT_RED}"));

        tail = StringCopyPadded(tail, (entry->name != NULL) ? entry->name : GetItemName(entry->item),
                                CHAR_SPACE, INFCAVE_SHOP_NAME_WIDTH);
        ConvertIntToDecimalStringN(tail, entry->price, STR_CONV_MODE_RIGHT_ALIGN, 3);

        row.name = text;
        row.id = i;
        MultichoiceDynamic_PushElement(row);
        pushed++;
    }

    ConvertIntToDecimalStringN(gStringVar3, InfCave_GetShards(), STR_CONV_MODE_LEFT_ALIGN, 5);
    gSpecialVar_Result = pushed;
}

// Buys the row the menu left in VAR_RESULT and replaces it with an
// enum InfCaveBuyResult the script switches on. An item purchase leaves its name
// in gStringVar1 and its amount in gStringVar2.
void InfCaveShop_Buy(void)
{
    u32 pick = gSpecialVar_Result;
    const struct InfCaveShopEntry *entry;
    const struct InfCaveItemDrop *drop;
    enum Item item;
    u32 amount;

    if (pick >= ARRAY_COUNT(sInfCaveShopStock) || !ShopRowStocked(&sInfCaveShopStock[pick]))
    {
        gSpecialVar_Result = INFCAVE_BUY_INVALID;
        return;
    }

    entry = &sInfCaveShopStock[pick];
    if (InfCave_GetShards() < entry->price)
    {
        gSpecialVar_Result = INFCAVE_BUY_NO_SHARDS;
        return;
    }

    if (entry->kind == INFCAVE_SHOP_KIND_FULL_HEAL)
    {
        InfCave_SpendShards(entry->price);
        RoomFlagSet(INFCAVE_ROOMFLAG_FULL_HEAL);
        HealPlayerParty();
        gSpecialVar_Result = INFCAVE_BUY_HEALED;
        return;
    }

    if (entry->kind == INFCAVE_SHOP_KIND_TRADE)
    {
        drop = TradeDrop();
        item = (drop != NULL) ? (enum Item)drop->item : ITEM_RARE_CANDY;
        amount = (drop != NULL) ? drop->amount : 1;
    }
    else
    {
        item = (enum Item)entry->item;
        amount = entry->amount;
    }

    // Nothing is charged when the Bag cannot hold the purchase. The merchant is
    // mid-run, so an item diverted to the PC would be out of reach for the rest
    // of the descent.
    if (!CheckBagHasSpace(item, amount))
    {
        gSpecialVar_Result = INFCAVE_BUY_NO_ROOM;
        return;
    }

    InfCave_SpendShards(entry->price);
    AddBagItem(item, amount);
    CopyItemNameHandlePlural(item, gStringVar1, amount);
    ConvertIntToDecimalStringN(gStringVar2, amount, STR_CONV_MODE_LEFT_ALIGN, 2);
    gSpecialVar_Result = INFCAVE_BUY_ITEM;
}
