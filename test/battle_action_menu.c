#include "global.h"
#include "battle.h"
#include "battle_action_menu.h"
#include "battle_util.h"
#include "event_data.h"
#include "fpmath.h"
#include "move.h"
#include "constants/battle.h"
#include "constants/flags.h"
#include "constants/moves.h"
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

TEST("(Action menu) Move navigation never selects a slot past the move count")
{
    u32 count, slot, dir, next;

    for (count = 1; count <= MOVE_MENU_SLOT_COUNT; count++)
    {
        for (slot = 0; slot < count; slot++)
        {
            for (dir = MOVE_DIR_UP; dir <= MOVE_DIR_RIGHT; dir++)
            {
                next = MoveMenu_GetNextSlot(count, slot, dir);
                EXPECT_LT(next, count);
            }
        }
    }
}

TEST("(Action menu) Move navigation matches the 2x2 grid with four moves")
{
    u32 slot;

    for (slot = 0; slot < MOVE_MENU_SLOT_COUNT; slot++)
    {
        EXPECT_EQ(MoveMenu_GetNextSlot(4, slot, MOVE_DIR_UP), (slot & 2) ? slot ^ 2 : slot);
        EXPECT_EQ(MoveMenu_GetNextSlot(4, slot, MOVE_DIR_DOWN), (slot & 2) ? slot : slot ^ 2);
        EXPECT_EQ(MoveMenu_GetNextSlot(4, slot, MOVE_DIR_LEFT), (slot & 1) ? slot ^ 1 : slot);
        EXPECT_EQ(MoveMenu_GetNextSlot(4, slot, MOVE_DIR_RIGHT), (slot & 1) ? slot : slot ^ 1);
    }
}

TEST("(Action menu) Move navigation refuses steps onto missing moves")
{
    // Two moves: only the top row exists.
    EXPECT_EQ(MoveMenu_GetNextSlot(2, 0, MOVE_DIR_DOWN), 0);
    EXPECT_EQ(MoveMenu_GetNextSlot(2, 1, MOVE_DIR_DOWN), 1);
    EXPECT_EQ(MoveMenu_GetNextSlot(2, 0, MOVE_DIR_RIGHT), 1);
    // Three moves: slot 3 is missing.
    EXPECT_EQ(MoveMenu_GetNextSlot(3, 1, MOVE_DIR_DOWN), 1);
    EXPECT_EQ(MoveMenu_GetNextSlot(3, 2, MOVE_DIR_RIGHT), 2);
    EXPECT_EQ(MoveMenu_GetNextSlot(3, 2, MOVE_DIR_UP), 0);
    // One move: nowhere to go.
    EXPECT_EQ(MoveMenu_GetNextSlot(1, 0, MOVE_DIR_RIGHT), 0);
    EXPECT_EQ(MoveMenu_GetNextSlot(1, 0, MOVE_DIR_DOWN), 0);
}

TEST("(Action menu) Move cells never share an 8x8 tile")
{
    u32 a, b;

    for (a = 0; a < MOVE_MENU_SLOT_COUNT; a++)
    {
        for (b = a + 1; b < MOVE_MENU_SLOT_COUNT; b++)
            EXPECT(!RectsOverlap(MoveMenu_GetCellTileRect(a), MoveMenu_GetCellTileRect(b)));
    }
}

TEST("(Action menu) Move cells have the cell size and fit the move grid window")
{
    u32 slot;

    for (slot = 0; slot < MOVE_MENU_SLOT_COUNT; slot++)
    {
        struct ActionMenuRect px = MoveMenu_GetCellPixelRect(slot);
        struct ActionMenuRect tiles = MoveMenu_GetCellTileRect(slot);

        EXPECT_EQ(px.right - px.left + 1, MOVE_CELL_WIDTH);
        EXPECT_EQ(px.bottom - px.top + 1, MOVE_CELL_HEIGHT);
        EXPECT_LT(tiles.right, MOVE_GRID_WIDTH_TILES);
        EXPECT_LT(tiles.bottom, MOVE_PANEL_HEIGHT_TILES);
    }
}

TEST("(Action menu) PP tiers switch at 50 percent, 25 percent and zero")
{
    EXPECT_EQ(MoveMenu_GetPpTier(15, 15), MOVE_PP_NORMAL);
    EXPECT_EQ(MoveMenu_GetPpTier(8, 15), MOVE_PP_NORMAL);
    EXPECT_EQ(MoveMenu_GetPpTier(7, 15), MOVE_PP_AMBER);
    EXPECT_EQ(MoveMenu_GetPpTier(10, 20), MOVE_PP_AMBER);
    EXPECT_EQ(MoveMenu_GetPpTier(5, 20), MOVE_PP_RED);
    EXPECT_EQ(MoveMenu_GetPpTier(1, 5), MOVE_PP_RED);
    EXPECT_EQ(MoveMenu_GetPpTier(0, 20), MOVE_PP_EMPTY);
    EXPECT_EQ(MoveMenu_GetPpTier(0, 0), MOVE_PP_NORMAL);
}

TEST("(Action menu) Effectiveness badge follows the type multiplier")
{
    EXPECT_EQ(MoveMenu_GetEffectivenessBadge(UQ_4_12(1.0), FALSE), MOVE_EFF_NONE);
    EXPECT_EQ(MoveMenu_GetEffectivenessBadge(UQ_4_12(2.0), FALSE), MOVE_EFF_SUPER);
    EXPECT_EQ(MoveMenu_GetEffectivenessBadge(UQ_4_12(4.0), FALSE), MOVE_EFF_EXTREME);
    EXPECT_EQ(MoveMenu_GetEffectivenessBadge(UQ_4_12(0.5), FALSE), MOVE_EFF_RESISTED);
    EXPECT_EQ(MoveMenu_GetEffectivenessBadge(UQ_4_12(0.25), FALSE), MOVE_EFF_MOSTLY_RESISTED);
    EXPECT_EQ(MoveMenu_GetEffectivenessBadge(UQ_4_12(0.0), FALSE), MOVE_EFF_IMMUNE);
    EXPECT_EQ(MoveMenu_GetEffectivenessBadge(UQ_4_12(0.0), TRUE), MOVE_EFF_NONE);
    EXPECT_EQ(MoveMenu_GetEffectivenessBadge(UQ_4_12(2.0), TRUE), MOVE_EFF_NONE);
}

TEST("(Action menu) Move palette keeps the base colours and tints each slot by type")
{
    static const enum Type types[MOVE_MENU_SLOT_COUNT] = { TYPE_FIRE, TYPE_WATER, TYPE_NONE, TYPE_GRASS };
    u16 idle[16], lit[16];
    u32 i;

    MoveMenu_BuildPalette(types, FALSE, idle);
    MoveMenu_BuildPalette(types, TRUE, lit);
    for (i = 0; i < 7; i++)
        EXPECT_EQ(idle[i], lit[i]);
    EXPECT_NE(idle[7], lit[7]);
    for (i = 0; i < MOVE_MENU_SLOT_COUNT; i++)
    {
        EXPECT_NE(idle[8 + i * 2], lit[8 + i * 2]);
        EXPECT_NE(lit[8 + i * 2], lit[9 + i * 2]);
    }
    EXPECT_NE(lit[8], lit[10]);
    EXPECT_NE(lit[10], lit[14]);
}

TEST("(Action menu) Move panel windows fit between the action panel and the Move Info window")
{
    EXPECT_GE(MOVE_PLATE_BASE_BLOCK, ACTION_PANEL_TILE_LIMIT);
    EXPECT_EQ(MOVE_GRID_BASE_BLOCK, MOVE_PLATE_BASE_BLOCK + MOVE_PLATE_TILE_COUNT);
    EXPECT_LE(MOVE_STUB_BASE_BLOCK + MOVE_STUB_COUNT, MOVE_PANEL_TILE_LIMIT);
    EXPECT_EQ(MOVE_PLATE_WIDTH_TILES + MOVE_GRID_WIDTH_TILES, DISPLAY_WIDTH / 8);
    EXPECT_EQ(MOVE_PANEL_TOP_TILE + MOVE_PANEL_HEIGHT_TILES, 60);
}

TEST("(Action menu) Every move name fits a move cell with the narrowest font")
{
    enum Move move;
    enum Move firstMisfit = MOVE_NONE;

    for (move = 1; move < MOVES_COUNT; move++)
    {
        const u8 *name = GetMoveName(move);
        u32 font = GetFontIdToFit(name, FONT_NARROW, 0, MOVE_NAME_WIDTH);

        if (firstMisfit == MOVE_NONE && GetStringWidth(font, name, 0) > MOVE_NAME_WIDTH)
            firstMisfit = move;
    }
    EXPECT_EQ(firstMisfit, MOVE_NONE);
}
