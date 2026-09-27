#ifndef GUARD_DATA_INFINITY_CAVE_ROOMS_H
#define GUARD_DATA_INFINITY_CAVE_ROOMS_H

// Rest, treasure and shop room data. Included from src/infinity_cave.c only; the
// room generator's own tables live in src/data/infinity_cave.h.

// One item an item ball may hold. The common rows fill every ball but the first;
// the premium rows fill ball 0 and the merchant's shard-for-item trade. Nothing
// the Stage 21 depth milestones pay out appears here, so a milestone stays the
// only source of the items the main game rations.
struct InfCaveItemDrop
{
    u16 item;
    u8 amount;
    u8 weight;  // relative share of one roll; the roller normalises them
    u8 minTier; // not offered before this item tier
};

static const struct InfCaveItemDrop sInfCaveCommonDrops[] =
{
    { ITEM_POTION,       .amount = 2, .weight = 16, .minTier = 0 },
    { ITEM_SUPER_POTION, .amount = 2, .weight = 16, .minTier = 0 },
    { ITEM_ETHER,        .amount = 1, .weight = 12, .minTier = 0 },
    { ITEM_FULL_HEAL,    .amount = 1, .weight = 10, .minTier = 0 },
    { ITEM_REVIVE,       .amount = 1, .weight = 10, .minTier = 0 },
    { ITEM_HYPER_POTION, .amount = 2, .weight = 14, .minTier = 1 },
    { ITEM_ELIXIR,       .amount = 1, .weight = 10, .minTier = 1 },
    { ITEM_MAX_POTION,   .amount = 1, .weight = 12, .minTier = 2 },
    { ITEM_MAX_ETHER,    .amount = 1, .weight = 10, .minTier = 2 },
};

static const struct InfCaveItemDrop sInfCavePremiumDrops[] =
{
    { ITEM_MAX_POTION,   .amount = 1, .weight = 14, .minTier = 0 },
    { ITEM_MAX_REVIVE,   .amount = 1, .weight = 12, .minTier = 0 },
    { ITEM_RARE_CANDY,   .amount = 1, .weight = 12, .minTier = 0 },
    { ITEM_FULL_RESTORE, .amount = 2, .weight = 12, .minTier = 1 },
    { ITEM_MAX_ELIXIR,   .amount = 1, .weight = 12, .minTier = 1 },
    { ITEM_PP_UP,        .amount = 1, .weight = 10, .minTier = 1 },
    { ITEM_EXP_CANDY_L,  .amount = 2, .weight = 10, .minTier = 2 },
    { ITEM_RARE_CANDY,   .amount = 3, .weight = 8,  .minTier = 2 },
};

// One row of the merchant's stock. item is the bag item an INFCAVE_SHOP_KIND_ITEM
// row sells; the service rows carry ITEM_NONE and name themselves instead.
struct InfCaveShopEntry
{
    u16 item;
    u16 price;      // shards
    const u8 *name; // NULL uses the item's own name
    u8 amount;
    u8 kind;        // enum InfCaveShopKind
    u8 minDepth;    // not stocked before this depth
};

static const u8 sInfCaveShopName_FullHeal[] = _("REST HERE");
static const u8 sInfCaveShopName_Trade[] = _("CAVE FIND");

// Prices are set against the shard income of the rooms that pay: a battle room's
// trainer is worth 10 shards before the depth growth, so a shallow room clears
// for about 25. One restock is a room or two of fighting; the trade is several.
static const struct InfCaveShopEntry sInfCaveShopStock[] =
{
    { ITEM_SUPER_POTION, .price = 10, .amount = 2, .kind = INFCAVE_SHOP_KIND_ITEM,      .minDepth = 0 },
    { ITEM_HYPER_POTION, .price = 18, .amount = 2, .kind = INFCAVE_SHOP_KIND_ITEM,      .minDepth = 0 },
    { ITEM_FULL_HEAL,    .price = 10, .amount = 2, .kind = INFCAVE_SHOP_KIND_ITEM,      .minDepth = 0 },
    { ITEM_REVIVE,       .price = 22, .amount = 1, .kind = INFCAVE_SHOP_KIND_ITEM,      .minDepth = 0 },
    { ITEM_MAX_POTION,   .price = 30, .amount = 1, .kind = INFCAVE_SHOP_KIND_ITEM,      .minDepth = 8 },
    { ITEM_MAX_ELIXIR,   .price = 38, .amount = 1, .kind = INFCAVE_SHOP_KIND_ITEM,      .minDepth = 8 },
    { ITEM_NONE,         .price = 60, .amount = 1, .kind = INFCAVE_SHOP_KIND_FULL_HEAL, .minDepth = 0, .name = sInfCaveShopName_FullHeal },
    { ITEM_NONE,         .price = 90, .amount = 1, .kind = INFCAVE_SHOP_KIND_TRADE,     .minDepth = 5, .name = sInfCaveShopName_Trade },
};

#endif // GUARD_DATA_INFINITY_CAVE_ROOMS_H
