#ifndef GUARD_DATA_INFINITY_CAVE_REWARDS_H
#define GUARD_DATA_INFINITY_CAVE_REWARDS_H

// Depth milestones and the lobby shop. Included from src/infinity_cave.c only;
// the in-run merchant's stock and the item-ball drops live in
// src/data/infinity_cave_rooms.h.

// One item a milestone pays. amount is the stack handed over in one go.
struct InfCaveMilestoneItem
{
    u16 item;
    u8 amount;
};

// One depth milestone. Paid once per save, at the end of the first run that
// reaches depth, so a lost descent still collects what it earned. ITEM_NONE ends
// the item list.
struct InfCaveMilestone
{
    u16 depth;
    struct InfCaveMilestoneItem items[INFCAVE_MILESTONE_MAX_ITEMS];
};

// Deepest first is not required, but the rows must stay in ascending depth order:
// the payout walks them low to high, and a milestone's bit index is its row index.
// At most INFCAVE_MILESTONE_MAX_ROWS rows, since the owed and paid sets are one
// byte each.
//
// These are the training items the main game rations, so nothing here is also an
// item-ball drop or merchant stock. Mints are sold in the lobby instead of paid
// out, since which nature a player wants is not something a table can pick.
static const struct InfCaveMilestone sInfCaveMilestones[] =
{
    {
        .depth = 10,
        .items =
        {
            { ITEM_EXP_CANDY_XL,   3 },
            { ITEM_BOTTLE_CAP,     1 },
        },
    },
    {
        .depth = 25,
        .items =
        {
            { ITEM_BOTTLE_CAP,     3 },
            { ITEM_ABILITY_CAPSULE, 1 },
        },
    },
    {
        .depth = 50,
        .items =
        {
            { ITEM_ABILITY_PATCH,  1 },
            { ITEM_EXP_CANDY_XL,   5 },
        },
    },
    {
        .depth = 75,
        .items =
        {
            { ITEM_GOLD_BOTTLE_CAP, 1 },
            { ITEM_ABILITY_PATCH,   2 },
        },
    },
    {
        .depth = 100,
        .items =
        {
            { ITEM_MASTER_BALL,     1 },
            { ITEM_GOLD_BOTTLE_CAP, 3 },
        },
    },
};

STATIC_ASSERT(ARRAY_COUNT(sInfCaveMilestones) <= INFCAVE_MILESTONE_MAX_ROWS,
              InfCaveMilestonesFitOneByte);

// One row of the lobby shop. Gated on best depth rather than on the live depth:
// the stock is meta progression, so pushing deeper unlocks better purchases and
// keeps them unlocked.
struct InfCaveLobbyEntry
{
    u16 item;
    u16 price;      // shards, per unit
    u16 minBestDepth;
};

// Priced against a whole descent rather than a room: a shallow room clears for
// about 25 shards, so the cheap rows are one good run and the deep rows are
// several. The gates are set one tier below the milestone that pays the same
// item, so the shop is the way to buy a second one.
static const struct InfCaveLobbyEntry sInfCaveLobbyStock[] =
{
    { ITEM_ABILITY_SHIELD,  .price = 500,   .minBestDepth = 0 },
    { ITEM_MIRROR_HERB,     .price = 1000,  .minBestDepth = 0 },
    { ITEM_LOADED_DICE,     .price = 1000,  .minBestDepth = 0 },
    { ITEM_COVERT_CLOAK,    .price = 1000,  .minBestDepth = 0 },
    { ITEM_CLEAR_AMULET,    .price = 2000,  .minBestDepth = 0 },
    { ITEM_GOLD_BOTTLE_CAP, .price = 500,   .minBestDepth = 0 },
    { ITEM_MAX_MUSHROOMS,   .price = 300,   .minBestDepth = 10 },
    { ITEM_ABILITY_CAPSULE, .price = 300,   .minBestDepth = 10 },
    { ITEM_ABILITY_PATCH,   .price = 1000,  .minBestDepth = 20 },
    { ITEM_PP_UP,           .price = 500,   .minBestDepth = 20 },
    { ITEM_SACRED_ASH,      .price = 1000,  .minBestDepth = 50 },
    { ITEM_MASTER_BALL,     .price = 10000, .minBestDepth = 80 },
};

#endif // GUARD_DATA_INFINITY_CAVE_REWARDS_H
