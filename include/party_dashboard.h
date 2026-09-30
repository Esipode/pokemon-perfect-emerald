#ifndef GUARD_PARTY_DASHBOARD_H
#define GUARD_PARTY_DASHBOARD_H

#include "global.h"
#include "config/summary_screen.h"
#include "constants/pokemon.h"
#include "pokemon.h"
#include "constants/pokeball.h"
#include "party_menu.h"

#define PARTY_DASH_MAX_SLOTS   PARTY_SIZE
#define PARTY_DASH_COLUMNS     3
#define PARTY_DASH_HP_BAR_W    88
#define PARTY_DASH_EXP_BAR_W   88

enum PartyDashboardDir
{
    PARTY_DASH_DIR_LEFT,
    PARTY_DASH_DIR_RIGHT,
    PARTY_DASH_DIR_UP,
    PARTY_DASH_DIR_DOWN,
};

enum PartyDashboardTab
{
    PARTY_DASH_TAB_INFO,
    PARTY_DASH_TAB_STATS,
    PARTY_DASH_TAB_COUNT,
};

// Stats are stored in display order, not enum Stat order.
enum PartyDashboardStat
{
    PARTY_DASH_STAT_HP,
    PARTY_DASH_STAT_ATK,
    PARTY_DASH_STAT_DEF,
    PARTY_DASH_STAT_SPATK,
    PARTY_DASH_STAT_SPDEF,
    PARTY_DASH_STAT_SPEED,
    PARTY_DASH_STAT_COUNT,
};

// Display data of the selected Pokémon. Renderers read only this struct.
struct PartyDashboardMon
{
    u8 nickname[POKEMON_NAME_LENGTH + 1];
    enum Species species;
    u8 gender;
    u16 level;
    bool8 isEgg;
    bool8 isBadEgg;
    bool8 canRename;
    bool8 canRelearn;
    bool8 recruitActive;
    u8 recruitBattlesLeft;
    u8 ailment;
    u8 hpLevel;
    u16 hp;
    u16 maxHp;
    u32 exp;
    u32 expToNext;
    u8 expTicks;
    u8 hatchPercent;
    const u8 *eggText;
    enum Type type1;
    enum Type type2;
    enum Type teraType;
    bool8 showTera;
    u8 nature;
    u8 mintNature;
    enum Ability ability;
    enum Item heldItem;
    enum PokeBall ball;
    u16 stats[PARTY_DASH_STAT_COUNT];
    u8 ivs[PARTY_DASH_STAT_COUNT];
    u8 evs[PARTY_DASH_STAT_COUNT];
    u16 evTotal;
    enum Move moves[MAX_MON_MOVES];
    enum Type moveTypes[MAX_MON_MOVES];
    u8 pp[MAX_MON_MOVES];
    u8 maxPp[MAX_MON_MOVES];
};

// Pure logic (src/party_dashboard_logic.c).
u32 PartyDashboard_NextSlot(u32 slot, enum PartyDashboardDir dir, u32 count);
u32 PartyDashboard_StatBarWidth(u32 stat, enum PartyDashboardStat statIndex, u32 level, u32 width);
u32 PartyDashboard_HatchPercent(u32 cyclesLeft, u32 eggCycles, bool32 isBadEgg);
u32 PartyDashboard_ExpTicks(u32 exp, u32 level, enum Species species, u32 width);
u32 PartyDashboard_WrapTab(u32 tab, s32 delta, bool32 isEgg);

// Rendering (src/party_dashboard.c).
static inline bool32 IsPartyDashboard(void)
{
    return P_PARTY_DASHBOARD && gPartyMenu.layout == PARTY_LAYOUT_SINGLE;
}

bool32 PartyDashboard_Alloc(void);
void PartyDashboard_Free(void);
void PartyDashboard_BuildMonData(struct Pokemon *mon, u32 slot, struct PartyDashboardMon *out);
void PartyDashboard_InitBgs(void);
void PartyDashboard_LoadGfx(void);
void PartyDashboard_InitWindows(void);
const u8 *PartyDashboard_GetSpriteCoords(u32 slot);
void PartyDashboard_DrawSlot(u32 slot, struct Pokemon *mon);
void PartyDashboard_DrawSlotDescription(u32 slot, const u8 *text);
void PartyDashboard_DrawEmptySlot(u32 slot, bool32 locked);
void PartyDashboard_SetSlotPalette(u32 slot, u32 palFlags);
void PartyDashboard_Select(u32 slot, struct Pokemon *mon);
void PartyDashboard_SetTab(s32 delta);
void PartyDashboard_RefreshSlot(u32 slot);
void PartyDashboard_ShowHint(u32 stringId);
void PartyDashboard_Update(void);

#endif // GUARD_PARTY_DASHBOARD_H
