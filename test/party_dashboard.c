#include "global.h"
#include "party_dashboard.h"
#include "pokemon.h"
#include "test/test.h"

#define L PARTY_DASH_DIR_LEFT
#define R PARTY_DASH_DIR_RIGHT
#define U PARTY_DASH_DIR_UP
#define D PARTY_DASH_DIR_DOWN

TEST("(Party dashboard) Left and right wrap through populated slots")
{
    for (u32 count = 1; count <= PARTY_SIZE; count++)
    {
        for (u32 slot = 0; slot < count; slot++)
        {
            EXPECT_EQ(PartyDashboard_NextSlot(slot, R, count), (slot + 1) % count);
            EXPECT_EQ(PartyDashboard_NextSlot(slot, L, count), (slot + count - 1) % count);
        }
    }
}

TEST("(Party dashboard) Up and down do nothing with three or fewer Pokémon")
{
    for (u32 count = 1; count <= PARTY_DASH_COLUMNS; count++)
    {
        for (u32 slot = 0; slot < count; slot++)
        {
            EXPECT_EQ(PartyDashboard_NextSlot(slot, U, count), slot);
            EXPECT_EQ(PartyDashboard_NextSlot(slot, D, count), slot);
        }
    }
}

TEST("(Party dashboard) Up and down toggle rows in a full party")
{
    for (u32 slot = 0; slot < PARTY_SIZE; slot++)
    {
        u32 other = (slot + PARTY_DASH_COLUMNS) % PARTY_SIZE;

        EXPECT_EQ(PartyDashboard_NextSlot(slot, U, PARTY_SIZE), other);
        EXPECT_EQ(PartyDashboard_NextSlot(slot, D, PARTY_SIZE), other);
    }
}

TEST("(Party dashboard) Up and down clamp to the last populated slot")
{
    EXPECT_EQ(PartyDashboard_NextSlot(2, D, 4), 3);
    EXPECT_EQ(PartyDashboard_NextSlot(1, D, 4), 4 - 1);
    EXPECT_EQ(PartyDashboard_NextSlot(2, D, 5), 4);
    EXPECT_EQ(PartyDashboard_NextSlot(4, U, 5), 1);
    EXPECT_EQ(PartyDashboard_NextSlot(3, U, 4), 0);
    EXPECT_EQ(PartyDashboard_NextSlot(0, D, 4), 3);
}

TEST("(Party dashboard) Navigation never returns an empty slot")
{
    for (u32 count = 1; count <= PARTY_SIZE; count++)
    {
        for (u32 slot = 0; slot < count; slot++)
        {
            for (u32 dir = L; dir <= D; dir++)
                EXPECT_LT(PartyDashboard_NextSlot(slot, dir, count), count);
        }
    }
}

TEST("(Party dashboard) Stat bars clamp to the bar width")
{
    for (u32 stat = 0; stat < PARTY_DASH_STAT_COUNT; stat++)
    {
        EXPECT_EQ(PartyDashboard_StatBarWidth(5000, stat, 50, 56), 56);
        EXPECT_EQ(PartyDashboard_StatBarWidth(0, stat, 50, 56), 0);
    }
}

TEST("(Party dashboard) Stat bars grow monotonically and never vanish for a nonzero stat")
{
    for (u32 stat = 0; stat < PARTY_DASH_STAT_COUNT; stat++)
    {
        u32 prev = 0;

        for (u32 value = 1; value <= 700; value++)
        {
            u32 width = PartyDashboard_StatBarWidth(value, stat, 100, 56);

            EXPECT_GE(width, prev);
            EXPECT_GE(width, 1);
            prev = width;
        }
    }
}

TEST("(Party dashboard) A one-HP mon fills the HP bar")
{
    EXPECT_EQ(PartyDashboard_StatBarWidth(1, PARTY_DASH_STAT_HP, 100, 56), 56);
    EXPECT_LT(PartyDashboard_StatBarWidth(1, PARTY_DASH_STAT_ATK, 100, 56), 56);
}

TEST("(Party dashboard) Hatch percent covers fresh, partial, full and bad eggs")
{
    EXPECT_EQ(PartyDashboard_HatchPercent(20, 20, FALSE), 0);
    EXPECT_EQ(PartyDashboard_HatchPercent(10, 20, FALSE), 50);
    EXPECT_EQ(PartyDashboard_HatchPercent(0, 20, FALSE), 100);
    EXPECT_EQ(PartyDashboard_HatchPercent(0, 20, TRUE), 0);
    EXPECT_EQ(PartyDashboard_HatchPercent(5, 0, FALSE), 100);
}

TEST("(Party dashboard) EXP ticks are empty at level 1 with no progress and at max level")
{
    u32 base = GetExperienceAtLevel(gSpeciesInfo[SPECIES_BULBASAUR].growthRate, 1);

    EXPECT_EQ(PartyDashboard_ExpTicks(base, 1, SPECIES_BULBASAUR, 88), 0);
    EXPECT_EQ(PartyDashboard_ExpTicks(GetExperienceAtLevel(gSpeciesInfo[SPECIES_BULBASAUR].growthRate, MAX_LEVEL), MAX_LEVEL, SPECIES_BULBASAUR, 88), 0);
}

TEST("(Party dashboard) EXP ticks are exact on a threshold and never exceed the width")
{
    enum Species species = SPECIES_BULBASAUR;
    u32 growth = gSpeciesInfo[species].growthRate;
    u32 prev = GetExperienceAtLevel(growth, 30);
    u32 next = GetExperienceAtLevel(growth, 31);

    EXPECT_EQ(PartyDashboard_ExpTicks(prev, 30, species, 88), 0);
    EXPECT_EQ(PartyDashboard_ExpTicks(prev + 1, 30, species, 88), 1);
    EXPECT_EQ(PartyDashboard_ExpTicks(next - 1, 30, species, 88), 87);
    EXPECT_EQ(PartyDashboard_ExpTicks(next, 30, species, 88), 88);
    EXPECT_GE(PartyDashboard_ExpTicks(prev + (next - prev) / 2, 30, species, 88), 43);
    EXPECT_LE(PartyDashboard_ExpTicks(prev + (next - prev) / 2, 30, species, 88), 44);
}

TEST("(Party dashboard) Tabs wrap in both directions")
{
    EXPECT_EQ(PartyDashboard_WrapTab(PARTY_DASH_TAB_INFO, 1, FALSE), PARTY_DASH_TAB_STATS);
    EXPECT_EQ(PartyDashboard_WrapTab(PARTY_DASH_TAB_STATS, 1, FALSE), PARTY_DASH_TAB_INFO);
    EXPECT_EQ(PartyDashboard_WrapTab(PARTY_DASH_TAB_INFO, -1, FALSE), PARTY_DASH_TAB_STATS);
    EXPECT_EQ(PartyDashboard_WrapTab(PARTY_DASH_TAB_STATS, -1, FALSE), PARTY_DASH_TAB_INFO);
}

TEST("(Party dashboard) Eggs lock the tab to INFO")
{
    EXPECT_EQ(PartyDashboard_WrapTab(PARTY_DASH_TAB_STATS, 1, TRUE), PARTY_DASH_TAB_INFO);
    EXPECT_EQ(PartyDashboard_WrapTab(PARTY_DASH_TAB_INFO, -1, TRUE), PARTY_DASH_TAB_INFO);
}
