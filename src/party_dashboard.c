#include "global.h"
#include "battle.h"
#include "battle_factory.h"
#include "battle_interface.h"
#include "battle_tent.h"
#include "bg.h"
#include "decompress.h"
#include "event_data.h"
#include "graphics.h"
#include "healthbox.h"
#include "malloc.h"
#include "menu.h"
#include "move.h"
#include "move_relearner.h"
#include "party_dashboard.h"
#include "pokemon.h"
#include "pokemon_summary_screen.h"
#include "randomization.h"
#include "palette.h"
#include "recruits_mode.h"
#include "string_util.h"
#include "strings.h"
#include "text.h"
#include "window.h"
#include "constants/flags.h"
#include "constants/party_menu.h"

#include "data/party_dashboard.h"

struct PartyDashboard
{
    struct PartyDashboardMon mon;
    u8 selectedSlot;
    u8 tab;
    u8 hintWindowId;
    // DMA sources: must outlive the queued VRAM copies.
    u8 chromeTiles[CHROME_TILE_COUNT * TILE_SIZE_4BPP];
    u8 iconTiles[BG3_TILE_COUNT * TILE_SIZE_4BPP];
    u16 bg3Tilemap[BG_SCREEN_SIZE / sizeof(u16)];
};

static EWRAM_DATA struct PartyDashboard *sDashboard = NULL;

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

bool32 PartyDashboard_Alloc(void)
{
    sDashboard = AllocZeroed(sizeof(*sDashboard));
    return sDashboard != NULL;
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

static u32 GetChromePixel(u32 tile, u32 x, u32 y)
{
    u32 edges = sChromeTileEdges[tile];

    switch (tile)
    {
    case CHROME_TILE_BLANK:
        return 0;
    case CHROME_TILE_TAB_FILL:
        return CHROME_PIX_EDGE;
    case CHROME_TILE_HINT_FILL:
        return CHROME_PIX_HINT;
    }

    if (((edges & CHROME_EDGE_TOP) && y < CHROME_EDGE_PX)
     || ((edges & CHROME_EDGE_BOTTOM) && y >= TILE_HEIGHT - CHROME_EDGE_PX)
     || ((edges & CHROME_EDGE_LEFT) && x < CHROME_EDGE_PX)
     || ((edges & CHROME_EDGE_RIGHT) && x >= TILE_WIDTH - CHROME_EDGE_PX))
        return CHROME_PIX_EDGE;
    return CHROME_PIX_FILL;
}

static void GenerateChromeTiles(void)
{
    u32 tile, x, y;

    for (tile = 0; tile < CHROME_TILE_COUNT; tile++)
    {
        u8 *dst = &sDashboard->chromeTiles[tile * TILE_SIZE_4BPP];

        for (y = 0; y < TILE_HEIGHT; y++)
        {
            for (x = 0; x < TILE_WIDTH; x += 2)
                *dst++ = GetChromePixel(tile, x, y) | (GetChromePixel(tile, x + 1, y) << 4);
        }
    }
}

// Top and bottom lines only: the 32 px ball sprite spans the slot width, so side edges would be covered.
static void DrawSlotFrame(u32 slot)
{
    u32 left = (slot % PARTY_DASH_COLUMNS) * SLOT_W_TILES;
    u32 top = (slot / PARTY_DASH_COLUMNS) * SLOT_H_TILES;

    FillBgTilemapBufferRect(1, CHROME_TILE_FILL, left, top, SLOT_W_TILES, SLOT_H_TILES, PARTY_DASH_PAL_SLOT_FIRST + slot);
    FillBgTilemapBufferRect(1, CHROME_TILE_TOP, left, top, SLOT_W_TILES, 1, PARTY_DASH_PAL_SLOT_FIRST + slot);
    FillBgTilemapBufferRect(1, CHROME_TILE_BOTTOM, left, top + SLOT_H_TILES - 1, SLOT_W_TILES, 1, PARTY_DASH_PAL_SLOT_FIRST + slot);
}

static void LoadSlotPaletteRow(u32 slot, enum PartyDashPalRow row)
{
    LoadPalette(&gPartyDashboard_Pal[row * 16], BG_PLTT_ID(PARTY_DASH_PAL_SLOT_FIRST + slot), PLTT_SIZE_4BPP);
}

void PartyDashboard_InitBgs(void)
{
    SetBgTilemapBuffer(3, sDashboard->bg3Tilemap);
    ShowBg(3);
}

// Loads chrome tiles, palettes and type-icon tiles, and draws the static BG1 layout.
void PartyDashboard_LoadGfx(void)
{
    const bool32 newIcons = Healthbox_IsNewStyle();
    u32 slot;

    GenerateChromeTiles();
    LoadBgTiles(1, sDashboard->chromeTiles, sizeof(sDashboard->chromeTiles), 0);

    FillBgTilemapBufferRect(1, CHROME_TILE_BLANK, 0, 0, 32, 32, PARTY_DASH_PAL_CHROME);
    for (slot = 0; slot < PARTY_DASH_MAX_SLOTS; slot++)
        DrawSlotFrame(slot);
    FillBgTilemapBufferRect(1, CHROME_TILE_FILL, IDENT_X, IDENT_Y, IDENT_W, IDENT_H, PARTY_DASH_PAL_CHROME);
    FillBgTilemapBufferRect(1, CHROME_TILE_TAB_FILL, TABS_X, TABS_Y, TABS_W, TABS_H, PARTY_DASH_PAL_CHROME);
    FillBgTilemapBufferRect(1, CHROME_TILE_FILL, BODY_X, BODY_Y, BODY_W, BODY_H, PARTY_DASH_PAL_CHROME);
    FillBgTilemapBufferRect(1, CHROME_TILE_HINT_FILL, HINT_X, HINT_Y, HINT_W, HINT_H, PARTY_DASH_PAL_CHROME);
    ScheduleBgCopyTilemapToVram(1);

    // Type icons: both sheets of the active healthbox style go to BG3 char base 2.
    DecompressDataWithHeaderWram(newIcons ? gBattleIconsNew_Gfx1 : gBattleIcons_Gfx1, &sDashboard->iconTiles[BG3_TILE_SHEET_1 * TILE_SIZE_4BPP]);
    DecompressDataWithHeaderWram(newIcons ? gBattleIconsNew_Gfx2 : gBattleIcons_Gfx2, &sDashboard->iconTiles[BG3_TILE_SHEET_2 * TILE_SIZE_4BPP]);
    LoadBgTiles(3, sDashboard->iconTiles, sizeof(sDashboard->iconTiles), 0);
    ScheduleBgCopyTilemapToVram(3);

    LoadPalette(&gPartyDashboard_Pal[PAL_ROW_CHROME * 16], BG_PLTT_ID(PARTY_DASH_PAL_CHROME), PLTT_SIZE_4BPP);
    LoadPalette(&gPartyDashboard_Pal[PAL_ROW_INFO * 16], BG_PLTT_ID(PARTY_DASH_PAL_INFO), PLTT_SIZE_4BPP);
    LoadPalette(newIcons ? gBattleIconsNew_Pal1 : gBattleIcons_Pal1, BG_PLTT_ID(PARTY_DASH_PAL_TYPE_1), PLTT_SIZE_4BPP);
    LoadPalette(newIcons ? gBattleIconsNew_Pal2 : gBattleIcons_Pal2, BG_PLTT_ID(PARTY_DASH_PAL_TYPE_2), PLTT_SIZE_4BPP);
    for (slot = 0; slot < PARTY_DASH_MAX_SLOTS; slot++)
        LoadSlotPaletteRow(slot, PAL_ROW_SLOT_NORMAL);
}

// Creates the dashboard windows (slot windows use ids 0-5, WIN_MSG is id 6) and draws the tab labels.
void PartyDashboard_InitWindows(void)
{
    u32 id;

    InitWindows(sPartyDashboardWindowTemplate);
    sDashboard->hintWindowId = AddWindow(&sPartyDashboardHintWindowTemplate);

    for (id = WIN_DASH_IDENT; id <= WIN_DASH_BODY; id++)
        FillWindowPixelBuffer(id, PIXEL_FILL(0));
    FillWindowPixelBuffer(sDashboard->hintWindowId, PIXEL_FILL(0));

    AddTextPrinterParameterized3(WIN_DASH_TABS, FONT_SMALL, 8, 1, sDashTabColors, TEXT_SKIP_DRAW, sText_DashTabInfo);
    AddTextPrinterParameterized3(WIN_DASH_TABS, FONT_SMALL, 48, 1, sDashTabColors, TEXT_SKIP_DRAW, sText_DashTabStats);

    for (id = WIN_DASH_IDENT; id <= WIN_DASH_BODY; id++)
    {
        PutWindowTilemap(id);
        CopyWindowToVram(id, COPYWIN_FULL);
    }
    PutWindowTilemap(sDashboard->hintWindowId);
    CopyWindowToVram(sDashboard->hintWindowId, COPYWIN_FULL);
}

const u8 *PartyDashboard_GetSpriteCoords(u32 slot)
{
    return sPartyDashSpriteCoords[slot];
}

static void ClearSlotWindow(u32 slot)
{
    FillWindowPixelBuffer(slot, PIXEL_FILL(0));
}

static void CommitSlotWindow(u32 slot)
{
    PutWindowTilemap(slot);
    CopyWindowToVram(slot, COPYWIN_FULL);
}

// Thin bar on a track, in the slot window's bar row.
static void DrawSlotBar(u32 slot, u32 width, u32 fill, u32 pix)
{
    FillWindowPixelRect(slot, SLOT_PIX_HP_TRACK, SLOT_BAR_X, SLOT_BAR_Y, width, SLOT_BAR_H);
    if (fill != 0)
        FillWindowPixelRect(slot, pix, SLOT_BAR_X, SLOT_BAR_Y, fill, SLOT_BAR_H);
}

// Recruits mode: battles left before retirement, as a red "-N" label right of the bar.
static void DrawSlotRecruitLabel(u32 slot, struct Pokemon *mon)
{
    ConvertIntToDecimalStringN(gStringVar2, Recruits_GetBattlesLeft(mon), STR_CONV_MODE_LEFT_ALIGN, 2);
    StringCopy(gStringVar1, gText_Dash);
    StringAppend(gStringVar1, gStringVar2);
    AddTextPrinterParameterized3(slot, FONT_SMALL_NARROWER, SLOT_LABEL_X, 0, sSlotRecruitColors, TEXT_SKIP_DRAW, gStringVar1);
}

// HP line (or egg hatch line) plus the Recruits label.
void PartyDashboard_DrawSlot(u32 slot, struct Pokemon *mon)
{
    ClearSlotWindow(slot);

    if (GetMonData(mon, MON_DATA_SANITY_IS_BAD_EGG) || GetMonData(mon, MON_DATA_IS_EGG))
    {
        u32 species = GetMonData(mon, MON_DATA_SPECIES);
        u32 percent = PartyDashboard_HatchPercent(GetMonData(mon, MON_DATA_FRIENDSHIP), gSpeciesInfo[species].eggCycles,
                                                  GetMonData(mon, MON_DATA_SANITY_IS_BAD_EGG));

        DrawSlotBar(slot, SLOT_BAR_W, percent * SLOT_BAR_W / 100, SLOT_PIX_HATCH);
    }
    else
    {
        u32 hp = GetMonData(mon, MON_DATA_HP);
        u32 maxHp = GetMonData(mon, MON_DATA_MAX_HP);
        u32 width = Recruits_IsActive() ? SLOT_BAR_RECRUIT_W : SLOT_BAR_W;

        DrawSlotBar(slot, width, GetScaledHPFraction(hp, maxHp, width), sSlotHpBarPix[GetHPBarLevel(hp, maxHp)]);
        if (Recruits_IsActive())
            DrawSlotRecruitLabel(slot, mon);
    }
    CommitSlotWindow(slot);
}

// Replaces the bar with a short status word (LEARNED, ABLE, FIRST...), centred.
void PartyDashboard_DrawSlotDescription(u32 slot, const u8 *text)
{
    s32 width = GetStringWidth(FONT_SMALL_NARROWER, text, 0);

    ClearSlotWindow(slot);
    AddTextPrinterParameterized3(slot, FONT_SMALL_NARROWER, max(0, (SLOT_WIN_W - width) / 2), 0, sSlotTextColors, TEXT_SKIP_DRAW, text);
    CommitSlotWindow(slot);
}

void PartyDashboard_DrawEmptySlot(u32 slot, bool32 locked)
{
    ClearSlotWindow(slot);
    if (locked)
        PartyDashboard_DrawSlotDescription(slot, sText_DashLocked);
    else
        CommitSlotWindow(slot);
}

// Recolours the slot's BG palette only; frame and fill tiles never redraw.
void PartyDashboard_SetSlotPalette(u32 slot, u32 palFlags)
{
    bool32 selected = (palFlags & PARTY_PAL_SELECTED) != 0;
    enum PartyDashPalRow row;

    if (palFlags & PARTY_PAL_NO_MON)
        row = PAL_ROW_SLOT_EMPTY;
    else if (palFlags & (PARTY_PAL_TO_SOFTBOIL | PARTY_PAL_TO_SWITCH | PARTY_PAL_SWITCHING))
        row = selected ? PAL_ROW_SLOT_SELECTED : PAL_ROW_SLOT_ACTION;
    else if (palFlags & PARTY_PAL_FAINTED)
        row = selected ? PAL_ROW_SLOT_FAINTED_SELECTED : PAL_ROW_SLOT_FAINTED;
    else
        row = selected ? PAL_ROW_SLOT_SELECTED : PAL_ROW_SLOT_NORMAL;

    LoadSlotPaletteRow(slot, row);
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
