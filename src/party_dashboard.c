#include "global.h"
#include "battle.h"
#include "battle_factory.h"
#include "battle_interface.h"
#include "battle_tent.h"
#include "event_data.h"
#include "malloc.h"
#include "move.h"
#include "move_relearner.h"
#include "party_dashboard.h"
#include "pokemon.h"
#include "pokemon_summary_screen.h"
#include "randomization.h"
#include "recruits_mode.h"
#include "constants/flags.h"
#include "constants/party_menu.h"

struct PartyDashboard
{
    struct PartyDashboardMon mon;
    u8 selectedSlot;
    u8 tab;
};

static EWRAM_DATA struct PartyDashboard *sDashboard = NULL;

#include "data/party_dashboard.h"

static bool32 CanMonRelearn(struct Pokemon *mon)
{
    if (GetMonData(mon, MON_DATA_IS_EGG) || InBattleFactory() || InSlateportBattleTent())
        return FALSE;

    for (u32 state = 0; state < MOVE_RELEARNER_COUNT; state++)
    {
        if (CanBoxMonRelearnMoves(&mon->box, state))
            return TRUE;
    }
    return FALSE;
}

void PartyDashboard_Alloc(void)
{
    sDashboard = AllocZeroed(sizeof(*sDashboard));
}

void PartyDashboard_Free(void)
{
    TRY_FREE_AND_SET_NULL(sDashboard);
}

void PartyDashboard_BuildMonData(struct Pokemon *mon, u32 slot, struct PartyDashboardMon *out)
{
    u16 resolvedMoves[MAX_MON_MOVES];
    u16 originalMoves[MAX_MON_MOVES];
    u8 type1, type2;

    memset(out, 0, sizeof(*out));

    out->species = GetMonData(mon, MON_DATA_SPECIES);
    GetMonData(mon, MON_DATA_NICKNAME, out->nickname);
    out->gender = GetMonGender(mon);
    out->level = GetMonData(mon, MON_DATA_LEVEL);
    out->isBadEgg = GetMonData(mon, MON_DATA_SANITY_IS_BAD_EGG);
    out->isEgg = out->isBadEgg || GetMonData(mon, MON_DATA_IS_EGG);
    out->canRename = CanRenamePartyMon(mon);
    out->canRelearn = CanMonRelearn(mon);
    out->ailment = GetMonAilment(mon);
    out->ball = GetMonData(mon, MON_DATA_POKEBALL);

    out->recruitActive = Recruits_IsActive();
    if (out->recruitActive && !out->isEgg)
        out->recruitBattlesLeft = Recruits_GetBattlesLeft(mon);

    if (out->isEgg)
    {
        u32 cyclesLeft = GetMonData(mon, MON_DATA_FRIENDSHIP);

        out->hatchPercent = PartyDashboard_HatchPercent(cyclesLeft, gSpeciesInfo[out->species].eggCycles, out->isBadEgg);
        out->eggText = GetEggStateText(mon);
        return;
    }

    out->hp = GetMonData(mon, MON_DATA_HP);
    out->maxHp = GetMonData(mon, MON_DATA_MAX_HP);
    out->hpLevel = GetHPBarLevel(out->hp, out->maxHp);

    out->exp = GetMonData(mon, MON_DATA_EXP);
    if (out->level < MAX_LEVEL)
    {
        out->expToNext = GetExperienceAtLevel(gSpeciesInfo[out->species].growthRate, out->level + 1) - out->exp;
    }
    out->expTicks = PartyDashboard_ExpTicks(out->exp, out->level, out->species, PARTY_DASH_EXP_BAR_W);

    // Resolved as a pure function of species, so it matches battle and the Summary.
    GetResolvedTypePair(out->species, &type1, &type2);
    out->type1 = type1;
    out->type2 = type2;
    out->teraType = GetMonData(mon, MON_DATA_TERA_TYPE);
    out->showTera = P_SHOW_TERA_TYPE >= GEN_9 && FlagGet(FLAG_BADGE07_GET);

    out->nature = GetNature(mon);
    out->mintNature = GetMonData(mon, MON_DATA_HIDDEN_NATURE);
    out->ability = GetAbilityBySpecies(out->species, GetMonData(mon, MON_DATA_ABILITY_NUM));
    out->heldItem = GetMonData(mon, MON_DATA_HELD_ITEM);

    for (u32 i = 0; i < PARTY_DASH_STAT_COUNT; i++)
    {
        u32 stat = sPartyDashStatOrder[i];

        out->stats[i] = GetMonData(mon, MON_DATA_MAX_HP + stat);
        out->ivs[i] = GetAdjustedIvData(mon, stat);
        out->evs[i] = GetMonData(mon, MON_DATA_HP_EV + stat);
        out->evTotal += out->evs[i];
    }

    for (u32 i = 0; i < MAX_MON_MOVES; i++)
        originalMoves[i] = GetMonData(mon, MON_DATA_MOVE1 + i);
    ResolveMonMoves(out->species, originalMoves, resolvedMoves);

    for (u32 i = 0; i < MAX_MON_MOVES; i++)
    {
        enum Move move = resolvedMoves[i];
        u8 ppBonuses = GetMonData(mon, MON_DATA_PP_BONUSES);

        out->moves[i] = move;
        if (move == MOVE_NONE)
            continue;

        // Same order as the Summary: dynamic type, then type randomization.
        out->moveTypes[i] = GetMonDisplayMoveType(mon, move, GetMoveType(move), slot);
        out->moveTypes[i] = GetResolvedMoveType(move, out->moveTypes[i]);
        out->maxPp[i] = CalculatePPWithBonus(move, ppBonuses, i);
        out->pp[i] = min((u8)GetMonData(mon, MON_DATA_PP1 + i), out->maxPp[i]);
    }
}

// Rendering entry points. Implemented in later stages.
void PartyDashboard_InitBgs(void)
{
}

void PartyDashboard_LoadGfx(void)
{
}

void PartyDashboard_InitWindows(void)
{
}

void PartyDashboard_GetSpriteCoords(u32 slot, s16 *x, s16 *y)
{
    *x = 0;
    *y = 0;
}

void PartyDashboard_DrawSlot(u32 slot)
{
}

void PartyDashboard_SetSlotPalette(u32 slot, u32 palFlags)
{
}

void PartyDashboard_Select(u32 slot)
{
    sDashboard->selectedSlot = slot;
}

void PartyDashboard_SetTab(s32 delta)
{
    sDashboard->tab = PartyDashboard_WrapTab(sDashboard->tab, delta, sDashboard->mon.isEgg);
}

void PartyDashboard_RefreshSlot(u32 slot)
{
}

void PartyDashboard_ShowHint(u32 stringId)
{
}

void PartyDashboard_Update(void)
{
}
