#include "global.h"
#include "battle.h"
#include "battle_action_menu.h"
#include "battle_util.h"
#include "event_data.h"
#include "constants/battle.h"
#include "constants/flags.h"
#include "constants/rgb.h"
#include "test/test.h"
#include "string_util.h"
#include "text.h"

#define B_WIN_YESNO_BASE_BLOCK_END 0x10C // B_WIN_YESNO occupies 0x100-0x10B

static bool32 RectsOverlap(struct ActionMenuRect a, struct ActionMenuRect b)
{
    return a.left <= b.right && b.left <= a.right && a.top <= b.bottom && b.top <= a.bottom;
}

TEST("(Action menu) Standard menu navigation matches the 2x2 grid rules")
{
    u32 slot;

    for (slot = 0; slot < ACTION_MENU_SLOT_COUNT; slot++)
    {
        EXPECT_EQ(ActionMenu_GetNextSlot(ACTION_MENU_STANDARD, slot, ACTION_DIR_UP), (slot & 2) ? slot ^ 2 : slot);
        EXPECT_EQ(ActionMenu_GetNextSlot(ACTION_MENU_STANDARD, slot, ACTION_DIR_DOWN), (slot & 2) ? slot : slot ^ 2);
        EXPECT_EQ(ActionMenu_GetNextSlot(ACTION_MENU_STANDARD, slot, ACTION_DIR_LEFT), (slot & 1) ? slot ^ 1 : slot);
        EXPECT_EQ(ActionMenu_GetNextSlot(ACTION_MENU_STANDARD, slot, ACTION_DIR_RIGHT), (slot & 1) ? slot : slot ^ 1);
    }
}

TEST("(Action menu) Safari slot 3 is unreachable")
{
    u32 slot, dir;

    for (slot = 0; slot < 3; slot++)
    {
        for (dir = ACTION_DIR_UP; dir <= ACTION_DIR_RIGHT; dir++)
            EXPECT_NE(ActionMenu_GetNextSlot(ACTION_MENU_SAFARI, slot, dir), 3);
    }
    // Moves that would land on slot 3 stay put.
    EXPECT_EQ(ActionMenu_GetNextSlot(ACTION_MENU_SAFARI, 1, ACTION_DIR_DOWN), 1);
    EXPECT_EQ(ActionMenu_GetNextSlot(ACTION_MENU_SAFARI, 2, ACTION_DIR_RIGHT), 2);
    // The remaining Safari moves still work.
    EXPECT_EQ(ActionMenu_GetNextSlot(ACTION_MENU_SAFARI, 0, ACTION_DIR_RIGHT), 1);
    EXPECT_EQ(ActionMenu_GetNextSlot(ACTION_MENU_SAFARI, 0, ACTION_DIR_DOWN), 2);
    EXPECT_EQ(ActionMenu_GetNextSlot(ACTION_MENU_SAFARI, 2, ACTION_DIR_UP), 0);
    EXPECT_EQ(ActionMenu_GetNextSlot(ACTION_MENU_SAFARI, 1, ACTION_DIR_LEFT), 0);
}

TEST("(Action menu) Chips never share an 8x8 tile")
{
    u32 a, b;

    for (a = 0; a < ACTION_MENU_SLOT_COUNT; a++)
    {
        for (b = a + 1; b < ACTION_MENU_SLOT_COUNT; b++)
            EXPECT(!RectsOverlap(ActionMenu_GetChipTileRect(a), ActionMenu_GetChipTileRect(b)));
    }
}

TEST("(Action menu) Chip rects have the chip size and fit the chip grid window")
{
    u32 slot;

    for (slot = 0; slot < ACTION_MENU_SLOT_COUNT; slot++)
    {
        struct ActionMenuRect px = ActionMenu_GetChipPixelRect(slot);
        struct ActionMenuRect tiles = ActionMenu_GetChipTileRect(slot);

        EXPECT_EQ(px.right - px.left + 1, ACTION_CHIP_WIDTH);
        EXPECT_EQ(px.bottom - px.top + 1, ACTION_CHIP_HEIGHT);
        EXPECT_LT(tiles.right, ACTION_GRID_WIDTH_TILES);
        EXPECT_LT(tiles.bottom, ACTION_PANEL_HEIGHT_TILES);
    }
}

TEST("(Action menu) Every label fits the label width")
{
    u32 menu, slot;

    for (menu = 0; menu < ACTION_MENU_COUNT; menu++)
    {
        for (slot = 0; slot < ACTION_MENU_SLOT_COUNT; slot++)
        {
            const struct ActionMenuSlot *entry = ActionMenu_GetSlot(menu, slot);

            if (entry->label == NULL)
                continue;
            EXPECT_LE(GetStringWidth(FONT_NARROW, entry->label, 0), ACTION_LABEL_WIDTH);
        }
    }
}

TEST("(Action menu) Windows fit in the free BG0 tile range")
{
    EXPECT_GE(ACTION_PROMPT_BASE_BLOCK, B_WIN_YESNO_BASE_BLOCK_END);
    EXPECT_EQ(ACTION_GRID_BASE_BLOCK, ACTION_PROMPT_BASE_BLOCK + ACTION_PROMPT_TILE_COUNT);
    EXPECT_LE(ACTION_GRID_BASE_BLOCK + ACTION_GRID_TILE_COUNT, ACTION_PANEL_TILE_LIMIT);
    EXPECT_EQ(ACTION_PROMPT_WIDTH_TILES + ACTION_GRID_WIDTH_TILES, DISPLAY_WIDTH / 8 );
}

TEST("(Action menu) Lit palette differs from idle only in indices 7-15")
{
    u16 idle[16], lit[16];
    u32 menu, i;

    for (menu = 0; menu < ACTION_MENU_COUNT; menu++)
    {
        ActionMenu_BuildPalette(menu, FALSE, idle);
        ActionMenu_BuildPalette(menu, TRUE, lit);
        for (i = 0; i < 7; i++)
            EXPECT_EQ(idle[i], lit[i]);
        EXPECT_NE(idle[7], lit[7]);
        for (i = 0; i < ACTION_MENU_SLOT_COUNT; i++)
        {
            if (!ActionMenu_GetSlot(menu, i)->navigable)
                continue;
            EXPECT_NE(idle[8 + i * 2], lit[8 + i * 2]);
        }
    }
}

TEST("(Action menu) Base palette loads from the generated asset")
{
    u16 pal[16];

    ActionMenu_BuildPalette(ACTION_MENU_STANDARD, FALSE, pal);
    EXPECT_EQ(pal[0], RGB_BLACK);
    EXPECT_EQ(pal[1], RGB(5, 6, 9));
    EXPECT_EQ(pal[4], RGB_WHITE);
    EXPECT_EQ(pal[7], RGB(14, 16, 20));
}

TEST("(Action menu) Safari ball label fits at the maximum ball count")
{
    u8 buffer[16];

    ConvertIntToDecimalStringN(StringCopy(buffer, COMPOUND_STRING("Ball ×")), 30, STR_CONV_MODE_LEFT_ALIGN, 3);
    EXPECT_LE(GetStringWidth(FONT_NARROW, buffer, 0), ACTION_LABEL_WIDTH);
}

TEST("(Action menu) Bag lock follows battle type, restrictions and Sky Drop")
{
    u32 savedFlags = gBattleTypeFlags;
    u32 savedVar = VarGet(B_VAR_NO_BAG_USE);
    enum SemiInvulnerableState savedState = gBattleMons[0].volatiles.semiInvulnerable;

    gBattleMons[0].volatiles.semiInvulnerable = STATE_NONE;
    gBattleTypeFlags = 0;
    // The restriction var and flag are disabled (0) in this config.
    if (B_VAR_NO_BAG_USE != 0)
    {
        VarSet(B_VAR_NO_BAG_USE, NO_BAG_RESTRICTION);
        EXPECT(!ActionMenu_IsBagLocked(0, TRUE));

        VarSet(B_VAR_NO_BAG_USE, NO_BAG_IN_BATTLE);
        EXPECT(ActionMenu_IsBagLocked(0, TRUE));
        EXPECT(!ActionMenu_IsBagLocked(0, FALSE));

        VarSet(B_VAR_NO_BAG_USE, NO_BAG_AGAINST_TRAINER);
        EXPECT(!ActionMenu_IsBagLocked(0, TRUE));
        gBattleTypeFlags = BATTLE_TYPE_TRAINER;
        EXPECT(ActionMenu_IsBagLocked(0, TRUE));
    }

    gBattleTypeFlags = BATTLE_TYPE_FRONTIER_NO_PYRAMID;
    EXPECT(ActionMenu_IsBagLocked(0, FALSE));
    gBattleTypeFlags = BATTLE_TYPE_PYRAMID;
    EXPECT(!ActionMenu_IsBagLocked(0, FALSE));

    gBattleTypeFlags = 0;
    gBattleMons[0].volatiles.semiInvulnerable = STATE_SKY_DROP_TARGET;
    EXPECT(ActionMenu_IsBagLocked(0, FALSE));

    gBattleMons[0].volatiles.semiInvulnerable = savedState;
    gBattleTypeFlags = savedFlags;
    if (B_VAR_NO_BAG_USE != 0)
        VarSet(B_VAR_NO_BAG_USE, savedVar);
}

TEST("(Action menu) Run lock follows trainer battles and the no-running flag")
{
    u32 savedFlags = gBattleTypeFlags;

    gBattleTypeFlags = 0;
    FlagClear(WE_FLAG_NO_RUNNING);
    EXPECT(!ActionMenu_IsRunLocked());

    gBattleTypeFlags = BATTLE_TYPE_TRAINER;
    EXPECT_EQ(ActionMenu_IsRunLocked(), !B_RUN_TRAINER_BATTLE);

    gBattleTypeFlags = 0;
    if (WE_FLAG_NO_RUNNING != 0)
    {
        FlagSet(WE_FLAG_NO_RUNNING);
        EXPECT(ActionMenu_IsRunLocked());
    }

    FlagClear(WE_FLAG_NO_RUNNING);
    gBattleTypeFlags = savedFlags;
}

TEST("(Action menu) Empty cells report EMPTY")
{
    EXPECT_EQ(ActionMenu_GetSlotState(0, ACTION_MENU_SAFARI, 3), ACTION_SLOT_EMPTY);
    EXPECT_EQ(ActionMenu_GetSlotState(0, ACTION_MENU_SAFARI, 0), ACTION_SLOT_ENABLED);
    EXPECT_EQ(ActionMenu_GetSlotState(0, ACTION_MENU_TUTORIAL, 3), ACTION_SLOT_ENABLED);
}
