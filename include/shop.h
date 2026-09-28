#ifndef GUARD_SHOP_H
#define GUARD_SHOP_H

extern struct ItemSlot gMartPurchaseHistory[3];

void EnableShopEvResetOption(void);
void CreatePokemartMenu(const u16 *itemsForSale);
// Item mart paid for in Infinity Cave shards. prices holds one shard price per
// itemsForSale entry, in the same order.
void CreateShardMartMenu(const u16 *itemsForSale, const u16 *prices);
void CreateDecorationShop1Menu(const u16 *itemsForSale);
void CreateDecorationShop2Menu(const u16 *itemsForSale);
void CB2_ExitSellMenu(void);

#endif // GUARD_SHOP_H
