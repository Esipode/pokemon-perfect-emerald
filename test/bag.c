#include "global.h"
#include "battle.h"
#include "event_data.h"
#include "item.h"
#include "item_menu.h"
#include "pokemon.h"
#include "test/overworld_script.h"
#include "test/test.h"

TEST("TMs and HMs are sorted correctly in the bag")
{
    struct BagPocket *pocket = &gBagPockets[POCKET_TM_HM];

    ASSUME(GetItemPocket(ITEM_HM07) == POCKET_TM_HM);
    ASSUME(GetItemPocket(ITEM_TM25) == POCKET_TM_HM);
    ASSUME(GetItemPocket(ITEM_TM14) == POCKET_TM_HM);
    ASSUME(GetItemPocket(ITEM_TM42) == POCKET_TM_HM);
    ASSUME(GetItemPocket(ITEM_HM05) == POCKET_TM_HM);
    ASSUME(GetItemPocket(ITEM_TM05) == POCKET_TM_HM);
    ASSUME(GetItemPocket(ITEM_TM01) == POCKET_TM_HM);
    ASSUME(GetItemPocket(ITEM_HM02) == POCKET_TM_HM);
    ASSUME(GetItemPocket(ITEM_TM101) == POCKET_TM_HM);
    ASSUME(GetItemPocket(ITEM_TM160) == POCKET_TM_HM);

    /*
     * Note: I would add a test to make sure that TMs are sorted correctly by move name,
     * but downstream users are likely to rearrange TMs so this would just be a nuisance.
     */

    RUN_OVERWORLD_SCRIPT(
        additem ITEM_HM07;
        additem ITEM_TM25;
        additem ITEM_TM14;
        additem ITEM_TM42;
        additem ITEM_HM05;
        additem ITEM_TM05;
        additem ITEM_TM01;
        additem ITEM_HM02;
        additem ITEM_TM160;
        additem ITEM_TM101;
    );

    SortItemsInBag(&gBagPockets[POCKET_TM_HM], SORT_BY_INDEX);

    EXPECT_EQ(pocket->itemSlots[0].itemId, ITEM_TM01);
    EXPECT_EQ(pocket->itemSlots[1].itemId, ITEM_TM05);
    EXPECT_EQ(pocket->itemSlots[2].itemId, ITEM_TM14);
    EXPECT_EQ(pocket->itemSlots[3].itemId, ITEM_TM25);
    EXPECT_EQ(pocket->itemSlots[4].itemId, ITEM_TM42);
    EXPECT_EQ(pocket->itemSlots[5].itemId, ITEM_TM101);
    EXPECT_EQ(pocket->itemSlots[6].itemId, ITEM_TM160);
    EXPECT_EQ(pocket->itemSlots[7].itemId, ITEM_HM02);
    EXPECT_EQ(pocket->itemSlots[8].itemId, ITEM_HM05);
    EXPECT_EQ(pocket->itemSlots[9].itemId, ITEM_HM07);
    EXPECT_EQ(pocket->itemSlots[10].itemId, ITEM_NONE);
}

TEST("Berries are sorted correctly in the bag")
{
    struct BagPocket *pocket = &gBagPockets[POCKET_BERRIES];

    ASSUME(GetItemPocket(ITEM_POMEG_BERRY) == POCKET_BERRIES);
    ASSUME(GetItemPocket(ITEM_MAGOST_BERRY) == POCKET_BERRIES);
    ASSUME(GetItemPocket(ITEM_KELPSY_BERRY) == POCKET_BERRIES);
    ASSUME(GetItemPocket(ITEM_MICLE_BERRY) == POCKET_BERRIES);
    ASSUME(GetItemPocket(ITEM_CHARTI_BERRY) == POCKET_BERRIES);
    ASSUME(GetItemPocket(ITEM_GANLON_BERRY) == POCKET_BERRIES);
    ASSUME(GetItemPocket(ITEM_ORAN_BERRY) == POCKET_BERRIES);
    ASSUME(GetItemPocket(ITEM_CHERI_BERRY) == POCKET_BERRIES);

    RUN_OVERWORLD_SCRIPT(
        additem ITEM_POMEG_BERRY;
        additem ITEM_MAGOST_BERRY;
        additem ITEM_KELPSY_BERRY;
        additem ITEM_MICLE_BERRY;
        additem ITEM_CHARTI_BERRY;
        additem ITEM_GANLON_BERRY;
        additem ITEM_ORAN_BERRY;
        additem ITEM_CHERI_BERRY;
    );

    SortItemsInBag(&gBagPockets[POCKET_BERRIES], SORT_BY_INDEX);

    EXPECT_EQ(pocket->itemSlots[0].itemId, ITEM_CHERI_BERRY);
    EXPECT_EQ(pocket->itemSlots[1].itemId, ITEM_ORAN_BERRY);
    EXPECT_EQ(pocket->itemSlots[2].itemId, ITEM_POMEG_BERRY);
    EXPECT_EQ(pocket->itemSlots[3].itemId, ITEM_KELPSY_BERRY);
    EXPECT_EQ(pocket->itemSlots[4].itemId, ITEM_MAGOST_BERRY);
    EXPECT_EQ(pocket->itemSlots[5].itemId, ITEM_CHARTI_BERRY);
    EXPECT_EQ(pocket->itemSlots[6].itemId, ITEM_GANLON_BERRY);
    EXPECT_EQ(pocket->itemSlots[7].itemId, ITEM_MICLE_BERRY);
    EXPECT_EQ(pocket->itemSlots[8].itemId, ITEM_NONE);

    SortItemsInBag(&gBagPockets[POCKET_BERRIES], SORT_ALPHABETICALLY);

    EXPECT_EQ(pocket->itemSlots[0].itemId, ITEM_CHARTI_BERRY);
    EXPECT_EQ(pocket->itemSlots[1].itemId, ITEM_CHERI_BERRY);
    EXPECT_EQ(pocket->itemSlots[2].itemId, ITEM_GANLON_BERRY);
    EXPECT_EQ(pocket->itemSlots[3].itemId, ITEM_KELPSY_BERRY);
    EXPECT_EQ(pocket->itemSlots[4].itemId, ITEM_MAGOST_BERRY);
    EXPECT_EQ(pocket->itemSlots[5].itemId, ITEM_MICLE_BERRY);
    EXPECT_EQ(pocket->itemSlots[6].itemId, ITEM_ORAN_BERRY);
    EXPECT_EQ(pocket->itemSlots[7].itemId, ITEM_POMEG_BERRY);
    EXPECT_EQ(pocket->itemSlots[8].itemId, ITEM_NONE);
}

TEST("Items are correctly sorted and compacted in the bag")
{
    struct BagPocket *pocket = &gBagPockets[POCKET_ITEMS];
    memset(pocket->itemSlots, 0, sizeof(gSaveBlock1Ptr->bag.items));

    ASSUME(GetItemPocket(ITEM_NUGGET) == POCKET_ITEMS);
    ASSUME(GetItemPocket(ITEM_BIG_NUGGET) == POCKET_ITEMS);
    ASSUME(GetItemPocket(ITEM_TINY_MUSHROOM) == POCKET_ITEMS);
    ASSUME(GetItemPocket(ITEM_BIG_MUSHROOM) == POCKET_ITEMS);
    ASSUME(GetItemPocket(ITEM_PEARL) == POCKET_ITEMS);
    ASSUME(GetItemPocket(ITEM_BIG_PEARL) == POCKET_ITEMS);

    RUN_OVERWORLD_SCRIPT(
        additem ITEM_NUGGET;
        additem ITEM_BIG_NUGGET;
        additem ITEM_TINY_MUSHROOM;
        additem ITEM_BIG_MUSHROOM;
        additem ITEM_PEARL;
        additem ITEM_BIG_PEARL;
    );

    EXPECT_EQ(pocket->itemSlots[0].itemId, ITEM_NUGGET);
    EXPECT_EQ(pocket->itemSlots[0].quantity, 1);
    EXPECT_EQ(pocket->itemSlots[1].itemId, ITEM_BIG_NUGGET);
    EXPECT_EQ(pocket->itemSlots[1].quantity, 1);
    EXPECT_EQ(pocket->itemSlots[2].itemId, ITEM_TINY_MUSHROOM);
    EXPECT_EQ(pocket->itemSlots[2].quantity, 1);
    EXPECT_EQ(pocket->itemSlots[3].itemId, ITEM_BIG_MUSHROOM);
    EXPECT_EQ(pocket->itemSlots[3].quantity, 1);
    EXPECT_EQ(pocket->itemSlots[4].itemId, ITEM_PEARL);
    EXPECT_EQ(pocket->itemSlots[4].quantity, 1);
    EXPECT_EQ(pocket->itemSlots[5].itemId, ITEM_BIG_PEARL);
    EXPECT_EQ(pocket->itemSlots[5].quantity, 1);
    EXPECT_EQ(pocket->itemSlots[6].itemId, ITEM_NONE);

    SortItemsInBag(&gBagPockets[POCKET_ITEMS], SORT_ALPHABETICALLY);

    EXPECT_EQ(pocket->itemSlots[0].itemId, ITEM_BIG_MUSHROOM);
    EXPECT_EQ(pocket->itemSlots[1].itemId, ITEM_BIG_NUGGET);
    EXPECT_EQ(pocket->itemSlots[2].itemId, ITEM_BIG_PEARL);
    EXPECT_EQ(pocket->itemSlots[3].itemId, ITEM_NUGGET);
    EXPECT_EQ(pocket->itemSlots[4].itemId, ITEM_PEARL);
    EXPECT_EQ(pocket->itemSlots[5].itemId, ITEM_TINY_MUSHROOM);
    EXPECT_EQ(pocket->itemSlots[6].itemId, ITEM_NONE);

    // Try removing the big items, check that everything is compacted correctly

    RUN_OVERWORLD_SCRIPT(
        removeitem ITEM_BIG_NUGGET;
        removeitem ITEM_BIG_MUSHROOM;
        removeitem ITEM_BIG_PEARL;
    );

    CompactItemsInBagPocket(POCKET_ITEMS);

    EXPECT_EQ(pocket->itemSlots[0].itemId, ITEM_NUGGET);
    EXPECT_EQ(pocket->itemSlots[0].quantity, 1);
    EXPECT_EQ(pocket->itemSlots[1].itemId, ITEM_PEARL);
    EXPECT_EQ(pocket->itemSlots[1].quantity, 1);
    EXPECT_EQ(pocket->itemSlots[2].itemId, ITEM_TINY_MUSHROOM);
    EXPECT_EQ(pocket->itemSlots[2].quantity, 1);
    EXPECT_EQ(pocket->itemSlots[3].itemId, ITEM_NONE);
    EXPECT_EQ(pocket->itemSlots[4].itemId, ITEM_NONE);
    EXPECT_EQ(pocket->itemSlots[5].itemId, ITEM_NONE);
    EXPECT_EQ(pocket->itemSlots[6].itemId, ITEM_NONE);
}

TEST("Every Mega Stone, Z-Crystal and Tera Shard is in its matching pocket")
{
    enum Item itemId;
    u32 megaStoneCount = 0, zCrystalCount = 0, teraShardCount = 0;

    for (itemId = ITEM_NONE; itemId < ITEMS_COUNT; itemId++)
    {
        switch (gItemsInfo[itemId].sortType)
        {
        case ITEM_TYPE_MEGA_STONE:
            EXPECT_EQ(GetItemPocket(itemId), POCKET_MEGA_STONES);
            megaStoneCount++;
            break;
        case ITEM_TYPE_Z_CRYSTAL:
            EXPECT_EQ(GetItemPocket(itemId), POCKET_Z_CRYSTALS);
            zCrystalCount++;
            break;
        case ITEM_TYPE_TERA_SHARD:
            EXPECT_EQ(GetItemPocket(itemId), POCKET_TERA_SHARDS);
            teraShardCount++;
            break;
        default:
            break;
        }
    }

    // D2 guard: capacity must cover every distinct item, so these pockets can never fill up.
    EXPECT(megaStoneCount <= BAG_MEGA_STONES_COUNT);
    EXPECT(zCrystalCount <= BAG_Z_CRYSTALS_COUNT);
    EXPECT(teraShardCount <= BAG_TERA_SHARDS_COUNT);
}

TEST("A Mega Stone is added to the Mega Stones pocket")
{
    struct BagPocket *pocket = &gBagPockets[POCKET_MEGA_STONES];
    memset(pocket->itemSlots, 0, sizeof(gSaveBlock3Ptr->gimmickBag.megaStones));

    ASSUME(GetItemPocket(ITEM_VENUSAURITE) == POCKET_MEGA_STONES);

    EXPECT(AddBagItem(ITEM_VENUSAURITE, 1));
    EXPECT(CheckBagHasItem(ITEM_VENUSAURITE, 1));
    EXPECT_EQ(pocket->itemSlots[0].itemId, ITEM_VENUSAURITE);
}

TEST("MoveMisfiledBagItems relocates a Mega Stone out of the Items pocket")
{
    struct BagPocket *itemsPocket = &gBagPockets[POCKET_ITEMS];
    struct BagPocket *megaStonesPocket = &gBagPockets[POCKET_MEGA_STONES];

    memset(itemsPocket->itemSlots, 0, sizeof(gSaveBlock1Ptr->bag.items));
    memset(megaStonesPocket->itemSlots, 0, sizeof(gSaveBlock3Ptr->gimmickBag.megaStones));

    ASSUME(GetItemPocket(ITEM_VENUSAURITE) == POCKET_MEGA_STONES);

    // Misfile it directly, bypassing AddBagItem, to simulate a pre-Stage-4 save.
    BagPocket_SetSlotItemIdAndCount(itemsPocket, 0, ITEM_VENUSAURITE, 1);

    MoveMisfiledBagItems();

    EXPECT_EQ(itemsPocket->itemSlots[0].itemId, ITEM_NONE);
    EXPECT_EQ(megaStonesPocket->itemSlots[0].itemId, ITEM_VENUSAURITE);
    EXPECT_EQ(megaStonesPocket->itemSlots[0].quantity, 1);
}

TEST("MoveMisfiledBagItems migrates the legacy TM array into the TM pocket")
{
    struct BagPocket *pocket = &gBagPockets[POCKET_TM_HM];

    memset(pocket->itemSlots, 0, sizeof(gSaveBlock3Ptr->TMsHMs));
    memset(gSaveBlock1Ptr->bag.legacyTMsHMs, 0, sizeof(gSaveBlock1Ptr->bag.legacyTMsHMs));

    ASSUME(GetItemPocket(ITEM_TM01) == POCKET_TM_HM);
    ASSUME(GetItemPocket(ITEM_TM50) == POCKET_TM_HM);
    ASSUME(GetItemPocket(ITEM_HM03) == POCKET_TM_HM);

    // Garbage quantities: the migration must not depend on the legacy encryption.
    gSaveBlock1Ptr->bag.legacyTMsHMs[0] = (struct ItemSlot){ITEM_TM01, 0x1234};
    gSaveBlock1Ptr->bag.legacyTMsHMs[1] = (struct ItemSlot){ITEM_TM50, 0xBEEF};
    gSaveBlock1Ptr->bag.legacyTMsHMs[2] = (struct ItemSlot){ITEM_HM03, 0x0042};

    MoveMisfiledBagItems();

    EXPECT(CheckBagHasItem(ITEM_TM01, 1));
    EXPECT(CheckBagHasItem(ITEM_TM50, 1));
    EXPECT(CheckBagHasItem(ITEM_HM03, 1));
    EXPECT_EQ(CountTotalItemQuantityInBag(ITEM_TM01), 1);
    EXPECT_EQ(CountTotalItemQuantityInBag(ITEM_TM50), 1);
    EXPECT_EQ(CountTotalItemQuantityInBag(ITEM_HM03), 1);
    for (u32 i = 0; i < BAG_TMHM_LEGACY_COUNT; i++)
    {
        EXPECT_EQ(gSaveBlock1Ptr->bag.legacyTMsHMs[i].itemId, ITEM_NONE);
        EXPECT_EQ(gSaveBlock1Ptr->bag.legacyTMsHMs[i].quantity, 0);
    }
}
