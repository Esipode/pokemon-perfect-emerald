#include "global.h"
#include "start_menu.h"
#include "test/test.h"

TEST("Start menu order: zeroed save gives the default order")
{
    u32 i;
    u8 saved[START_MENU_ORDER_SLOTS] = {0};
    u8 order[START_MENU_ITEM_COUNT];

    StartMenuOrder_Normalize(saved, order);
    for (i = 0; i < START_MENU_ITEM_COUNT; i++)
        EXPECT_EQ(order[i], i);
}

TEST("Start menu order: repeated and out-of-range entries are dropped")
{
    u32 i;
    u8 saved[START_MENU_ORDER_SLOTS] = {0};
    u8 order[START_MENU_ITEM_COUNT];

    saved[0] = START_MENU_ITEM_SAVE + 1;
    saved[1] = START_MENU_ITEM_COUNT + 1;
    saved[2] = START_MENU_ITEM_SAVE + 1;
    saved[3] = 0xFF;
    saved[4] = START_MENU_ITEM_BAG + 1;

    StartMenuOrder_Normalize(saved, order);
    EXPECT_EQ(order[0], START_MENU_ITEM_SAVE);
    EXPECT_EQ(order[1], START_MENU_ITEM_BAG);

    // Remaining ids follow in table order, skipping the two placed above.
    u32 pos = 2;
    for (i = 0; i < START_MENU_ITEM_COUNT; i++)
    {
        if (i == START_MENU_ITEM_SAVE || i == START_MENU_ITEM_BAG)
            continue;
        EXPECT_EQ(order[pos], i);
        pos++;
    }
    EXPECT_EQ(pos, START_MENU_ITEM_COUNT);
}

TEST("Start menu order: partial order gets missing ids at the end")
{
    u8 saved[START_MENU_ORDER_SLOTS] = {0};
    u8 order[START_MENU_ITEM_COUNT];
    u8 stored[START_MENU_ITEM_COUNT];
    u32 i;

    // Full reversed order with the last table id missing, as an older save would have.
    for (i = 0; i < START_MENU_ITEM_COUNT - 1; i++)
        saved[i] = (START_MENU_ITEM_COUNT - 2 - i) + 1;

    StartMenuOrder_Normalize(saved, order);
    for (i = 0; i < START_MENU_ITEM_COUNT - 1; i++)
        EXPECT_EQ(order[i], START_MENU_ITEM_COUNT - 2 - i);
    EXPECT_EQ(order[START_MENU_ITEM_COUNT - 1], START_MENU_ITEM_COUNT - 1);

    // Store then load round-trips.
    StartMenuOrder_Store(order);
    StartMenuOrder_Load(stored);
    for (i = 0; i < START_MENU_ITEM_COUNT; i++)
        EXPECT_EQ(stored[i], order[i]);
}

TEST("Start menu order: swap across a hidden id leaves it in place")
{
    u32 i;
    u8 saved[START_MENU_ORDER_SLOTS] = {0};
    u8 order[START_MENU_ITEM_COUNT];

    StartMenuOrder_Normalize(saved, order);
    // MAP sits between BAG and PLAYER; swapping those two must not move MAP.
    StartMenuOrder_Swap(order, START_MENU_ITEM_BAG, START_MENU_ITEM_PLAYER);

    EXPECT_EQ(order[START_MENU_ITEM_BAG], START_MENU_ITEM_PLAYER);
    EXPECT_EQ(order[START_MENU_ITEM_MAP], START_MENU_ITEM_MAP);
    EXPECT_EQ(order[START_MENU_ITEM_PLAYER], START_MENU_ITEM_BAG);
    for (i = 0; i < START_MENU_ITEM_COUNT; i++)
    {
        if (i != START_MENU_ITEM_BAG && i != START_MENU_ITEM_PLAYER)
            EXPECT_EQ(order[i], i);
    }
}

TEST("Start menu hidden: any item can be hidden with a zero mask")
{
    u32 i;

    for (i = 0; i < START_MENU_ITEM_COUNT; i++)
        EXPECT(StartMenuHidden_CanHide(0, i));
}

TEST("Start menu hidden: the last unhidden always-shown item cannot be hidden")
{
    u16 mask = (1u << START_MENU_ITEM_BAG)
             | (1u << START_MENU_ITEM_PLAYER)
             | (1u << START_MENU_ITEM_SAVE)
             | (1u << START_MENU_ITEM_OPTION);

    EXPECT(!StartMenuHidden_CanHide(mask, START_MENU_ITEM_ACHIEVEMENTS));
    // Gated items do not count toward the guard, so they can still be hidden.
    EXPECT(StartMenuHidden_CanHide(mask, START_MENU_ITEM_MAP));
}

TEST("Start menu hidden: an already hidden item can be unhidden")
{
    u16 mask = (1u << START_MENU_ITEM_BAG)
             | (1u << START_MENU_ITEM_PLAYER)
             | (1u << START_MENU_ITEM_SAVE)
             | (1u << START_MENU_ITEM_OPTION)
             | (1u << START_MENU_ITEM_ACHIEVEMENTS);

    EXPECT(StartMenuHidden_CanHide(mask, START_MENU_ITEM_SAVE));
}

TEST("Start menu hidden: toggle flips one bit only")
{
    u16 mask = (1u << START_MENU_ITEM_POKEDEX) | (1u << START_MENU_ITEM_OPTION);

    mask = StartMenuHidden_Toggle(mask, START_MENU_ITEM_MAP);
    EXPECT_EQ(mask, (1u << START_MENU_ITEM_POKEDEX) | (1u << START_MENU_ITEM_OPTION) | (1u << START_MENU_ITEM_MAP));
    mask = StartMenuHidden_Toggle(mask, START_MENU_ITEM_POKEDEX);
    EXPECT_EQ(mask, (1u << START_MENU_ITEM_OPTION) | (1u << START_MENU_ITEM_MAP));
}
