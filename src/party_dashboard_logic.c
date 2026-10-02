#include "global.h"
#include "party_dashboard.h"
#include "pokemon.h"

// Reference stat: base 160, 31 IV, 252 EV, nature-boosted, at the soft-capped stat level.
#define STAT_BAR_REF_BASE     160
#define STAT_BAR_REF_IV       31
#define STAT_BAR_REF_EV_QUART 63

static u32 GetStatBarReference(enum PartyDashboardStat statIndex, u32 level)
{
    u32 levelScaled = GetScaledStatLevel(level);
    u32 core = (2 * STAT_BAR_REF_BASE + STAT_BAR_REF_IV + STAT_BAR_REF_EV_QUART) * levelScaled / 100000;

    if (statIndex == PARTY_DASH_STAT_HP)
        return core + levelScaled / 1000 + 10;
    return (core + 5) * 110 / 100;
}

// Slots are ordered 0 1 2 / 3 4 5. Returns the current slot when the input does not move the cursor.
u32 PartyDashboard_NextSlot(u32 slot, enum PartyDashboardDir dir, u32 count)
{
    u32 target;

    if (count == 0)
        return slot;

    switch (dir)
    {
    case PARTY_DASH_DIR_LEFT:
        return (slot + count - 1) % count;
    case PARTY_DASH_DIR_RIGHT:
        return (slot + 1) % count;
    default:
        if (count <= PARTY_DASH_COLUMNS)
            return slot;
        target = (slot < PARTY_DASH_COLUMNS) ? slot + PARTY_DASH_COLUMNS : slot - PARTY_DASH_COLUMNS;
        if (target >= count)
            target = count - 1;
        return target;
    }
}

u32 PartyDashboard_StatBarWidth(u32 stat, enum PartyDashboardStat statIndex, u32 level, u32 width)
{
    u32 ref, result;

    if (stat == 0)
        return 0;
    // Max HP of 1 (Shedinja) fills the bar.
    if (statIndex == PARTY_DASH_STAT_HP && stat == 1)
        return width;

    ref = GetStatBarReference(statIndex, level != 0 ? level : 1);
    result = stat * width / ref;
    if (result == 0)
        result = 1;
    return min(result, width);
}

// cyclesLeft is the egg's remaining hatch cycles (stored in the friendship field).
u32 PartyDashboard_HatchPercent(u32 cyclesLeft, u32 eggCycles, bool32 isBadEgg)
{
    if (isBadEgg)
        return 0;
    if (eggCycles == 0)
        return 100;
    if (cyclesLeft >= eggCycles)
        return 0;
    return 100 - cyclesLeft * 100 / eggCycles;
}

u32 PartyDashboard_ExpTicks(u32 exp, u32 level, enum Species species, u32 width)
{
    u32 prevXP, nextXP, sinceLast, ticks;

    if (level >= MAX_LEVEL)
        return 0;

    prevXP = GetExperienceAtLevel(gSpeciesInfo[species].growthRate, level);
    nextXP = GetExperienceAtLevel(gSpeciesInfo[species].growthRate, level + 1);
    if (exp <= prevXP || nextXP <= prevXP)
        return 0;

    sinceLast = exp - prevXP;
    ticks = sinceLast * width / (nextXP - prevXP);
    if (ticks == 0)
        ticks = 1;
    return min(ticks, width);
}

// Eggs have no STATS tab.
u32 PartyDashboard_WrapTab(u32 tab, s32 delta, bool32 isEgg)
{
    s32 next;

    if (isEgg)
        return PARTY_DASH_TAB_INFO;

    next = ((s32)tab + delta) % PARTY_DASH_TAB_COUNT;
    if (next < 0)
        next += PARTY_DASH_TAB_COUNT;
    return next;
}
