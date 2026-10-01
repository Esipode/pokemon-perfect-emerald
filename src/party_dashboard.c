#include "global.h"
#include "battle.h"
#include "battle_factory.h"
#include "battle_interface.h"
#include "battle_message.h"
#include "battle_tent.h"
#include "bg.h"
#include "caps.h"
#include "decompress.h"
#include "event_data.h"
#include "graphics.h"
#include "healthbox.h"
#include "item.h"
#include "item_icon.h"
#include "malloc.h"
#include "menu.h"
#include "move.h"
#include "move_relearner.h"
#include "party_dashboard.h"
#include "pokemon.h"
#include "pokemon_summary_screen.h"
#include "randomization.h"
#include "palette.h"
#include "sprite.h"
#include "recruits_mode.h"
#include "string_util.h"
#include "strings.h"
#include "text.h"
#include "type_icons.h"
#include "window.h"
#include "constants/flags.h"
#include "constants/party_menu.h"
#include "constants/rgb.h"

#include "data/party_dashboard.h"

struct PartyDashboard
{
    struct PartyDashboardMon mon;
    u8 selectedSlot;
    bool8 hasSelection;
    u8 tab;
    bool8 pendingBody;
    u8 pulseTimer;
    u8 pulseStep;
    u8 slotRow[PARTY_DASH_MAX_SLOTS];
    u8 slotState[PARTY_DASH_MAX_SLOTS];
    u8 hintWindowId;
    u8 statusSpriteId;
    u8 itemSpriteId;
    enum Item itemIconItem;
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
    if (sDashboard == NULL)
        return FALSE;

    sDashboard->statusSpriteId = SPRITE_NONE;
    sDashboard->itemSpriteId = SPRITE_NONE;
    return TRUE;
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

// Loads the row, then recolours the slot fill for its eligibility state.
static void LoadSlotPaletteRow(u32 slot, enum PartyDashPalRow row)
{
    u32 pal = BG_PLTT_ID(PARTY_DASH_PAL_SLOT_FIRST + slot);
    bool32 selected = row == PAL_ROW_SLOT_SELECTED || row == PAL_ROW_SLOT_FAINTED_SELECTED;

    LoadPalette(&gPartyDashboard_Pal[row * 16], pal, PLTT_SIZE_4BPP);
    if (row == PAL_ROW_SLOT_EMPTY)
        return;
    if (sDashboard->slotState[slot] == SLOT_STATE_DIM)
        LoadPalette(&sSlotDimFill[selected], pal + SLOT_PIX_FILL, sizeof(u16));
    else if (sDashboard->slotState[slot] == SLOT_STATE_PICKED)
        LoadPalette(&sSlotPickedFill[selected], pal + SLOT_PIX_FILL, sizeof(u16));
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

// The active tab is underlined and drawn in the text colour; eggs have no tabs to switch, so all labels dim.
static void DrawTabs(void)
{
    const bool32 locked = sDashboard->mon.isEgg;

    FillWindowPixelBuffer(WIN_DASH_TABS, PIXEL_FILL(0));
    for (u32 tab = 0; tab < PARTY_DASH_TAB_COUNT; tab++)
    {
        const bool32 active = !locked && tab == sDashboard->tab;

        AddTextPrinterParameterized3(WIN_DASH_TABS, FONT_SMALL, sDashTabX[tab], TAB_LABEL_Y, active ? sIdentTextColors : sIdentLabelColors,
                                     TEXT_SKIP_DRAW, sDashTabLabels[tab]);
        if (active)
            FillWindowPixelRect(WIN_DASH_TABS, INFO_PIX_ACCENT, sDashTabX[tab], TAB_UNDERLINE_Y, GetStringWidth(FONT_SMALL, sDashTabLabels[tab], 0), TAB_UNDERLINE_H);
    }

    // Doubles: the first two party slots are the battlers on the field.
    if (sDashboard->hasSelection && gPartyMenu.layout == PARTY_LAYOUT_DOUBLE && sDashboard->selectedSlot < 2)
        AddTextPrinterParameterized3(WIN_DASH_TABS, FONT_SMALL, TAB_ACTIVE_RIGHT_X - GetStringWidth(FONT_SMALL, sText_DashActive, 0), TAB_LABEL_Y,
                                     sIdentLabelColors, TEXT_SKIP_DRAW, sText_DashActive);
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

    DrawTabs();

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

void PartyDashboard_ClearSlotText(u32 slot)
{
    ClearSlotWindow(slot);
    CommitSlotWindow(slot);
}

// Thin bar on a track, in the slot window's bar row.
static void DrawSlotBar(u32 slot, u32 width, u32 fill, u32 pix)
{
    FillWindowPixelRect(slot, SLOT_PIX_HP_TRACK, SLOT_BAR_X, SLOT_BAR_Y, width, SLOT_BAR_H);
    if (fill != 0)
        FillWindowPixelRect(slot, pix, SLOT_BAR_X, SLOT_BAR_Y, fill, SLOT_BAR_H);
}

// Recruits mode: battles left before retirement, as a red "-N" label right of the bar, in the slot's BG3 label window.
static void DrawSlotRecruitLabel(u32 slot, struct Pokemon *mon)
{
    u32 windowId = WIN_DASH_LABEL_FIRST + slot;

    ConvertIntToDecimalStringN(gStringVar2, Recruits_GetBattlesLeft(mon), STR_CONV_MODE_LEFT_ALIGN, 2);
    StringCopy(gStringVar1, gText_Dash);
    StringAppend(gStringVar1, gStringVar2);
    FillWindowPixelBuffer(windowId, PIXEL_FILL(0));
    AddTextPrinterParameterized3(windowId, FONT_SMALL_NARROWER, SLOT_LABEL_X, RECRUIT_TEXT_Y, sSlotRecruitColors, TEXT_SKIP_DRAW, gStringVar1);
    PutWindowTilemap(windowId);
    CopyWindowToVram(windowId, COPYWIN_FULL);
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

static void SetSlotState(u32 slot, u32 state)
{
    if (sDashboard->slotState[slot] == state)
        return;

    sDashboard->slotState[slot] = state;
    if (sDashboard->slotRow[slot] != PAL_ROW_CHROME)
        LoadSlotPaletteRow(slot, sDashboard->slotRow[slot]);
}

void PartyDashboard_ClearSlotState(u32 slot)
{
    SetSlotState(slot, SLOT_STATE_NONE);
}

// Shows a description as a slot colour instead of text: dim = unavailable, green = chosen. The bar stays.
void PartyDashboard_SetSlotDescription(u32 slot, u32 descId, struct Pokemon *mon)
{
    switch (descId)
    {
    case PARTYBOX_DESC_NO_USE:
    case PARTYBOX_DESC_NOT_ABLE:
    case PARTYBOX_DESC_NOT_ABLE_2:
    case PARTYBOX_DESC_LEARNED:
        SetSlotState(slot, SLOT_STATE_DIM);
        break;
    case PARTYBOX_DESC_FIRST:
    case PARTYBOX_DESC_SECOND:
    case PARTYBOX_DESC_THIRD:
    case PARTYBOX_DESC_FOURTH:
    case PARTYBOX_DESC_HAVE:
    case PARTYBOX_DESC_ABLE:
    case PARTYBOX_DESC_ABLE_2:
        SetSlotState(slot, SLOT_STATE_PICKED);
        break;
    default:
        SetSlotState(slot, SLOT_STATE_NONE);
        break;
    }
    PartyDashboard_DrawSlot(slot, mon);
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
    CommitSlotWindow(slot);
    if (locked)
    {
        u32 windowId = WIN_DASH_LABEL_FIRST + slot;
        s32 width = GetStringWidth(FONT_SMALL_NARROWER, sText_DashLocked, 0);

        FillWindowPixelBuffer(windowId, PIXEL_FILL(0));
        AddTextPrinterParameterized3(windowId, FONT_SMALL_NARROWER, max(0, (SLOT_WIN_W - width) / 2), LOCKED_TEXT_Y, sSlotTextColors, TEXT_SKIP_DRAW, sText_DashLocked);
        PutWindowTilemap(windowId);
        CopyWindowToVram(windowId, COPYWIN_FULL);
    }
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

    sDashboard->slotRow[slot] = row;
    LoadSlotPaletteRow(slot, row);
}

static void PrintIdentRight(u32 fontId, u32 y, const u8 *colors, const u8 *str)
{
    AddTextPrinterParameterized3(WIN_DASH_IDENT, fontId, IDENT_RIGHT_X - GetStringWidth(fontId, str, 0), y, colors, TEXT_SKIP_DRAW, str);
}

// Bar on a track; a zero fill leaves the track empty.
static void DrawIdentBar(u32 y, u32 height, u32 fill, u32 pix)
{
    FillWindowPixelRect(WIN_DASH_IDENT, INFO_PIX_TRACK, IDENT_BAR_X, y, PARTY_DASH_HP_BAR_W, height);
    if (fill != 0)
        FillWindowPixelRect(WIN_DASH_IDENT, pix, IDENT_BAR_X, y, fill, height);
}

static bool32 ShouldShowGender(struct PartyDashboardMon *mon)
{
    if (mon->gender != MON_MALE && mon->gender != MON_FEMALE)
        return FALSE;
    // A Nidoran keeping its default name already shows its gender in the name.
    if ((mon->species == SPECIES_NIDORAN_M || mon->species == SPECIES_NIDORAN_F)
     && StringCompare(mon->nickname, GetSpeciesName(mon->species)) == 0)
        return FALSE;
    return TRUE;
}

static void DrawIdentityEgg(struct PartyDashboardMon *mon)
{
    AddTextPrinterParameterized3(WIN_DASH_IDENT, FONT_NORMAL, IDENT_TEXT_X, IDENT_NAME_Y, sIdentTextColors, TEXT_SKIP_DRAW, sText_DashEgg);

    AddTextPrinterParameterized3(WIN_DASH_IDENT, FONT_SMALL_NARROW, IDENT_TEXT_X, IDENT_EXP_Y, sIdentLabelColors, TEXT_SKIP_DRAW, sText_DashHatch);
    ConvertIntToDecimalStringN(gStringVar1, mon->hatchPercent, STR_CONV_MODE_LEFT_ALIGN, 3);
    StringAppend(gStringVar1, sText_DashPercent);
    PrintIdentRight(FONT_SMALL_NARROW, IDENT_EXP_Y, sIdentTextColors, gStringVar1);
    DrawIdentBar(IDENT_EXP_BAR_Y, IDENT_EXP_BAR_H, mon->hatchPercent * PARTY_DASH_EXP_BAR_W / 100, INFO_PIX_HATCH);
}

static void DrawIdentityMon(struct PartyDashboardMon *mon)
{
    const bool32 fainted = mon->hp == 0;
    u32 nameFont = GetFontIdToFit(mon->nickname, FONT_NORMAL, 0, IDENT_NAME_MAX_W);

    AddTextPrinterParameterized3(WIN_DASH_IDENT, nameFont, IDENT_TEXT_X, IDENT_NAME_Y, sIdentTextColors, TEXT_SKIP_DRAW, mon->nickname);
    if (ShouldShowGender(mon))
        PrintIdentRight(FONT_SMALL, IDENT_NAME_Y, mon->gender == MON_MALE ? sIdentMaleColors : sIdentFemaleColors,
                        mon->gender == MON_MALE ? gText_MaleSymbol : gText_FemaleSymbol);

    StringCopy(gStringVar1, gText_LevelSymbol);
    ConvertIntToDecimalStringN(gStringVar2, mon->level, STR_CONV_MODE_LEFT_ALIGN, 3);
    StringAppend(gStringVar1, gStringVar2);
    AddTextPrinterParameterized3(WIN_DASH_IDENT, FONT_SMALL, IDENT_TEXT_X, IDENT_LEVEL_Y, sIdentTextColors, TEXT_SKIP_DRAW, gStringVar1);

    AddTextPrinterParameterized3(WIN_DASH_IDENT, FONT_SMALL_NARROW, IDENT_TEXT_X, IDENT_HP_Y, sIdentLabelColors, TEXT_SKIP_DRAW, sText_DashHp);
    ConvertIntToDecimalStringN(gStringVar1, mon->hp, STR_CONV_MODE_LEFT_ALIGN, 4);
    StringAppend(gStringVar1, sText_DashSlash);
    ConvertIntToDecimalStringN(gStringVar2, mon->maxHp, STR_CONV_MODE_LEFT_ALIGN, 4);
    StringAppend(gStringVar1, gStringVar2);
    PrintIdentRight(FONT_SMALL_NARROW, IDENT_HP_Y, fainted ? sIdentFaintedColors : sIdentTextColors, gStringVar1);
    DrawIdentBar(IDENT_HP_BAR_Y, IDENT_HP_BAR_H, GetScaledHPFraction(mon->hp, mon->maxHp, PARTY_DASH_HP_BAR_W), sIdentHpBarPix[mon->hpLevel]);

    AddTextPrinterParameterized3(WIN_DASH_IDENT, FONT_SMALL_NARROW, IDENT_TEXT_X, IDENT_EXP_Y, sIdentLabelColors, TEXT_SKIP_DRAW, sText_DashExp);
    if (mon->level >= MAX_LEVEL)
    {
        StringCopy(gStringVar1, sText_DashMax);
    }
    else
    {
        ConvertIntToDecimalStringN(gStringVar1, mon->expToNext, STR_CONV_MODE_LEFT_ALIGN, 7);
        StringAppend(gStringVar1, sText_DashToNext);
    }
    PrintIdentRight(FONT_SMALL_NARROW, IDENT_EXP_Y, sIdentTextColors, gStringVar1);
    DrawIdentBar(IDENT_EXP_BAR_Y, IDENT_EXP_BAR_H, mon->expTicks, INFO_PIX_EXP);
}

// Icons are two BG3 tiles (8x16) from the active style's sheets. Stellar reuses the Mystery icon, as in battle.
static void PlaceTypeIcon(enum Type type, u32 col, u32 row)
{
    u32 tile, pal;

    if (type == TYPE_STELLAR)
        type = TYPE_MYSTERY;
    if (type == TYPE_NONE || type >= NUMBER_OF_MON_TYPES)
        return;

    if (gTypesInfo[type].useSecondTypeIconPalette)
    {
        tile = BG3_TILE_SHEET_2 + TYPE_ICON_2_FRAME(type);
        pal = PARTY_DASH_PAL_TYPE_2;
    }
    else
    {
        tile = BG3_TILE_SHEET_1 + TYPE_ICON_1_FRAME(type);
        pal = PARTY_DASH_PAL_TYPE_1;
    }
    FillBgTilemapBufferRect(3, tile, col, row, 1, 1, pal);
    FillBgTilemapBufferRect(3, tile + 1, col, row + 1, 1, 1, pal);
}

static void DrawIdentityTypeIcons(struct PartyDashboardMon *mon)
{
    FillBgTilemapBufferRect(3, BG3_TILE_BLANK, IDENT_ICON_COL, IDENT_ICON_ROW, IDENT_TERA_COL - IDENT_ICON_COL + 1, 2, 0);
    if (!mon->isEgg)
    {
        PlaceTypeIcon(mon->type1, IDENT_ICON_COL, IDENT_ICON_ROW);
        if (mon->type2 != mon->type1)
            PlaceTypeIcon(mon->type2, IDENT_ICON_COL + 1, IDENT_ICON_ROW);
        if (mon->showTera)
            PlaceTypeIcon(mon->teraType, IDENT_TERA_COL, IDENT_ICON_ROW);
    }
    ScheduleBgCopyTilemapToVram(3);
}

// The chip is created on first use and reused; it is hidden when there is no status to show.
static void UpdateIdentityStatusChip(struct PartyDashboardMon *mon)
{
    struct Sprite *sprite;

    if (sDashboard->statusSpriteId == SPRITE_NONE)
    {
        u32 spriteId = CreateSprite(&gSpriteTemplate_StatusIcons, IDENT_CHIP_X, IDENT_CHIP_Y, 0);

        if (spriteId == MAX_SPRITES)
            return;
        sDashboard->statusSpriteId = spriteId;
    }

    sprite = &gSprites[sDashboard->statusSpriteId];
    if (mon->isEgg || mon->ailment == AILMENT_NONE || mon->ailment == AILMENT_PKRS)
    {
        sprite->invisible = TRUE;
        return;
    }
    StartSpriteAnim(sprite, mon->ailment - 1);
    sprite->invisible = FALSE;
}

static void DrawIdentity(void)
{
    struct PartyDashboardMon *mon = &sDashboard->mon;

    FillWindowPixelBuffer(WIN_DASH_IDENT, PIXEL_FILL(0));
    if (mon->isEgg)
        DrawIdentityEgg(mon);
    else
        DrawIdentityMon(mon);
    CopyWindowToVram(WIN_DASH_IDENT, COPYWIN_GFX);

    DrawIdentityTypeIcons(mon);
    UpdateIdentityStatusChip(mon);
}

static void PrintBody(u32 fontId, u32 x, u32 y, const u8 *colors, const u8 *str)
{
    AddTextPrinterParameterized3(WIN_DASH_BODY, fontId, x, y, colors, TEXT_SKIP_DRAW, str);
}

static void PrintBodyRight(u32 fontId, u32 y, const u8 *colors, const u8 *str)
{
    PrintBody(fontId, INFO_RIGHT_X - GetStringWidth(fontId, str, 0), y, colors, str);
}

// Fits the value into the space right of the label column.
static void PrintInfoValue(u32 y, u32 maxWidth, const u8 *str)
{
    PrintBody(GetFontIdToFit(str, FONT_NORMAL, 0, maxWidth), INFO_VALUE_X, y, sIdentTextColors, str);
}

static void DestroyItemIcon(void)
{
    if (sDashboard->itemSpriteId == SPRITE_NONE)
        return;

    DestroySprite(&gSprites[sDashboard->itemSpriteId]);
    FreeSpriteTilesByTag(TAG_DASH_ITEM_ICON);
    FreeSpritePaletteByTag(TAG_DASH_ITEM_ICON);
    sDashboard->itemSpriteId = SPRITE_NONE;
    sDashboard->itemIconItem = ITEM_NONE;
}

// Keeps the icon of an unchanged item; otherwise replaces it.
static void UpdateItemIcon(enum Item item)
{
    u32 spriteId;

    if (item == ITEM_NONE)
    {
        DestroyItemIcon();
        return;
    }
    if (sDashboard->itemSpriteId != SPRITE_NONE && sDashboard->itemIconItem == item)
        return;

    DestroyItemIcon();
    spriteId = AddItemIconSprite(TAG_DASH_ITEM_ICON, TAG_DASH_ITEM_ICON, item);
    if (spriteId == MAX_SPRITES)
        return;

    gSprites[spriteId].x = INFO_ITEM_ICON_X;
    gSprites[spriteId].y = INFO_ITEM_ICON_Y;
    gSprites[spriteId].oam.priority = 1;
    sDashboard->itemSpriteId = spriteId;
    sDashboard->itemIconItem = item;
}

// Raised and lowered stat of the mint-adjusted nature, right-aligned. Neutral natures print nothing.
static void DrawNatureStats(struct PartyDashboardMon *mon)
{
    enum Stat up = gNaturesInfo[mon->mintNature].statUp;
    enum Stat down = gNaturesInfo[mon->mintNature].statDown;
    u32 downWidth;

    if (up == down)
        return;

    StringCopy(gStringVar1, sText_DashNatureDown);
    StringAppend(gStringVar1, sDashStatLabels[down]);
    downWidth = GetStringWidth(FONT_SMALL_NARROW, gStringVar1, 0);
    PrintBodyRight(FONT_SMALL_NARROW, INFO_NATURE_Y + INFO_LABEL_DY, P_SUMMARY_SCREEN_NATURE_COLORS ? sInfoDownColors : sIdentTextColors, gStringVar1);

    StringCopy(gStringVar1, sText_DashNatureUp);
    StringAppend(gStringVar1, sDashStatLabels[up]);
    PrintBody(FONT_SMALL_NARROW, INFO_RIGHT_X - downWidth - 4 - GetStringWidth(FONT_SMALL_NARROW, gStringVar1, 0), INFO_NATURE_Y + INFO_LABEL_DY,
              P_SUMMARY_SCREEN_NATURE_COLORS ? sInfoUpColors : sIdentTextColors, gStringVar1);
}

static void DrawInfoRows(struct PartyDashboardMon *mon)
{
    const u8 *itemName;

    PrintBody(FONT_SMALL, INFO_LABEL_X, INFO_NATURE_Y + INFO_LABEL_DY, sIdentLabelColors, sText_DashNature);
    PrintInfoValue(INFO_NATURE_Y, INFO_VALUE_W / 2, gNaturesInfo[mon->nature].name);
    DrawNatureStats(mon);

    PrintBody(FONT_SMALL, INFO_LABEL_X, INFO_ABILITY_Y + INFO_LABEL_DY, sIdentLabelColors, sText_DashAbility);
    PrintInfoValue(INFO_ABILITY_Y, INFO_VALUE_W, gAbilitiesInfo[mon->ability].name);

    PrintBody(FONT_SMALL, INFO_LABEL_X, INFO_ITEM_Y + INFO_LABEL_DY, sIdentLabelColors, sText_DashItem);
    if (mon->heldItem == ITEM_NONE)
    {
        itemName = gText_None;
    }
    else
    {
        CopyItemName(mon->heldItem, gStringVar1);
        itemName = gStringVar1;
    }
    PrintInfoValue(INFO_ITEM_Y, INFO_ITEM_NAME_W, itemName);
}

static const u8 *GetPpColors(u32 pp, u32 maxPp)
{
    switch (GetCurrentPPToMaxPPState(pp, maxPp))
    {
    case 2:
        return sIdentFaintedColors;
    case 1:
    case 0:
        return sInfoPpLowColors;
    default:
        return sIdentTextColors;
    }
}

static void DrawInfoMoves(struct PartyDashboardMon *mon)
{
    for (u32 i = 0; i < MAX_MON_MOVES; i++)
    {
        u32 y = INFO_MOVES_Y + i * INFO_MOVE_STEP;
        enum Move move = mon->moves[i];

        if (move == MOVE_NONE)
        {
            PrintBody(FONT_NORMAL, INFO_MOVE_NAME_X, y + 1, sIdentTextColors, gText_OneDash);
            continue;
        }

        PlaceTypeIcon(mon->moveTypes[i], BODY_X, BODY_Y + y / TILE_HEIGHT);
        PrintBody(GetFontIdToFit(GetMoveName(move), FONT_NORMAL, 0, INFO_MOVE_NAME_W), INFO_MOVE_NAME_X, y + 1, sIdentTextColors, GetMoveName(move));

        ConvertIntToDecimalStringN(gStringVar1, mon->pp[i], STR_CONV_MODE_LEFT_ALIGN, 2);
        StringAppend(gStringVar1, sText_DashSlash);
        ConvertIntToDecimalStringN(gStringVar2, mon->maxPp[i], STR_CONV_MODE_LEFT_ALIGN, 2);
        StringAppend(gStringVar1, gStringVar2);
        PrintBodyRight(FONT_NORMAL, y + 1, GetPpColors(mon->pp[i], mon->maxPp[i]), gStringVar1);
    }
}

static void PrintStatsRight(u32 fontId, u32 rightX, u32 y, const u8 *colors, const u8 *str)
{
    PrintBody(fontId, rightX - GetStringWidth(fontId, str, 0), y, colors, str);
}

// Stat label colour follows the mint-adjusted nature when P_SUMMARY_SCREEN_NATURE_COLORS is on.
static const u8 *GetStatLabelColors(struct PartyDashboardMon *mon, enum Stat stat)
{
    if (!P_SUMMARY_SCREEN_NATURE_COLORS || gNaturesInfo[mon->mintNature].statUp == gNaturesInfo[mon->mintNature].statDown)
        return sIdentLabelColors;
    if (stat == gNaturesInfo[mon->mintNature].statUp)
        return sInfoUpColors;
    if (stat == gNaturesInfo[mon->mintNature].statDown)
        return sInfoDownColors;
    return sIdentLabelColors;
}

static void DrawStatsHeader(bool32 showIv, bool32 showEv, u32 valueRightX)
{
    PrintBody(FONT_SMALL, INFO_LABEL_X, STATS_HEADER_Y + INFO_LABEL_DY, sIdentLabelColors, sText_DashStatHeader);
    PrintStatsRight(FONT_SMALL, valueRightX, STATS_HEADER_Y + INFO_LABEL_DY, sIdentLabelColors, sText_DashValueHeader);
    if (showIv)
        PrintStatsRight(FONT_SMALL, STATS_IV_RIGHT_X, STATS_HEADER_Y + INFO_LABEL_DY, sIdentLabelColors, sText_DashIvHeader);
    if (showEv)
        PrintStatsRight(FONT_SMALL, STATS_EV_RIGHT_X, STATS_HEADER_Y + INFO_LABEL_DY, sIdentLabelColors, sText_DashEvHeader);
}

static void DrawBodyStats(struct PartyDashboardMon *mon)
{
    bool32 showIv, showEv;
    u32 barWidth, valueRightX;

    SummaryScreen_ShowIvEv(FALSE, &showIv, &showEv);
    barWidth = (showIv || showEv) ? STATS_BAR_W : STATS_BAR_W_WIDE;
    valueRightX = (showIv || showEv) ? STATS_VALUE_RIGHT_X : STATS_VALUE_WIDE_X;
    DrawStatsHeader(showIv, showEv, valueRightX);

    for (u32 i = 0; i < PARTY_DASH_STAT_COUNT; i++)
    {
        u32 y = STATS_ROWS_Y + i * STATS_ROW_STEP;
        enum Stat stat = sPartyDashStatOrder[i];

        PrintBody(FONT_SMALL, INFO_LABEL_X, y + INFO_LABEL_DY, GetStatLabelColors(mon, stat), sDashStatLabels[stat]);

        FillWindowPixelRect(WIN_DASH_BODY, INFO_PIX_TRACK, STATS_BAR_X, y + STATS_BAR_DY, barWidth, STATS_BAR_H);
        FillWindowPixelRect(WIN_DASH_BODY, sStatBarPix[i], STATS_BAR_X, y + STATS_BAR_DY,
                            PartyDashboard_StatBarWidth(mon->stats[i], i, mon->level, barWidth), STATS_BAR_H);

        ConvertIntToDecimalStringN(gStringVar1, mon->stats[i], STR_CONV_MODE_LEFT_ALIGN, 4);
        PrintStatsRight(FONT_NORMAL, valueRightX, y, sIdentTextColors, gStringVar1);

        if (showIv)
        {
            if (P_SUMMARY_SCREEN_IV_EV_VALUES)
            {
                ConvertIntToDecimalStringN(gStringVar1, mon->ivs[i], STR_CONV_MODE_LEFT_ALIGN, 2);
                PrintStatsRight(FONT_SMALL, STATS_IV_RIGHT_X, y + INFO_LABEL_DY, sIdentTextColors, gStringVar1);
            }
            else
            {
                PrintStatsRight(FONT_SMALL, STATS_IV_RIGHT_X, y + INFO_LABEL_DY, sIdentTextColors, GetIvLetterGrade(mon->ivs[i]));
            }
        }
        if (showEv)
        {
            ConvertIntToDecimalStringN(gStringVar1, mon->evs[i], STR_CONV_MODE_LEFT_ALIGN, 3);
            PrintStatsRight(FONT_SMALL, STATS_EV_RIGHT_X, y + INFO_LABEL_DY, sIdentTextColors, gStringVar1);
        }
    }

    if (showEv)
    {
        PrintBody(FONT_SMALL, INFO_LABEL_X, STATS_EV_TOTAL_Y + INFO_LABEL_DY, sIdentLabelColors, sText_DashEvTotal);
        ConvertIntToDecimalStringN(gStringVar1, mon->evTotal, STR_CONV_MODE_LEFT_ALIGN, 3);
        StringAppend(gStringVar1, sText_DashSlash);
        ConvertIntToDecimalStringN(gStringVar2, GetCurrentEVCap(), STR_CONV_MODE_LEFT_ALIGN, 3);
        StringAppend(gStringVar1, gStringVar2);
        PrintStatsRight(FONT_SMALL, STATS_EV_RIGHT_X, STATS_EV_TOTAL_Y + INFO_LABEL_DY, sIdentTextColors, gStringVar1);
    }
}

// The whole body shows the egg state text instead of the tab content.
static void DrawBodyEgg(struct PartyDashboardMon *mon)
{
    PrintBody(FONT_SMALL_NARROW, INFO_LABEL_X, INFO_EGG_TEXT_Y, sIdentTextColors, mon->eggText);
}

static void DrawBodyInfo(struct PartyDashboardMon *mon)
{
    DrawInfoRows(mon);
    FillWindowPixelRect(WIN_DASH_BODY, INFO_PIX_TRACK, INFO_LABEL_X, INFO_DIVIDER_Y, BODY_W * TILE_WIDTH - 2 * INFO_LABEL_X, 1);
    DrawInfoMoves(mon);
}

// Redraws the tab body. Move type icons live on BG3 in the body's first column.
static void DrawBody(void)
{
    struct PartyDashboardMon *mon = &sDashboard->mon;
    bool32 showItemIcon = !mon->isEgg && sDashboard->tab == PARTY_DASH_TAB_INFO;

    sDashboard->pendingBody = FALSE;
    FillWindowPixelBuffer(WIN_DASH_BODY, PIXEL_FILL(0));
    FillBgTilemapBufferRect(3, BG3_TILE_BLANK, BODY_X, BODY_Y + INFO_MOVES_Y / TILE_HEIGHT, 1, INFO_MOVE_STEP * MAX_MON_MOVES / TILE_HEIGHT, 0);

    if (mon->isEgg)
        DrawBodyEgg(mon);
    else if (sDashboard->tab == PARTY_DASH_TAB_INFO)
        DrawBodyInfo(mon);
    else
        DrawBodyStats(mon);

    UpdateItemIcon(showItemIcon ? mon->heldItem : ITEM_NONE);
    CopyWindowToVram(WIN_DASH_BODY, COPYWIN_GFX);
    ScheduleBgCopyTilemapToVram(3);
}

// With animate, the identity block draws now and the tab body one frame later (see PartyDashboard_Update).
static void SelectMon(u32 slot, struct Pokemon *mon, bool32 animate)
{
    animate = animate && sDashboard->hasSelection;
    sDashboard->selectedSlot = slot;
    sDashboard->hasSelection = TRUE;
    PartyDashboard_BuildMonData(mon, slot, &sDashboard->mon);
    DrawIdentity();
    DrawTabs();
    CopyWindowToVram(WIN_DASH_TABS, COPYWIN_GFX);
    if (animate)
        sDashboard->pendingBody = TRUE;
    else
        DrawBody();
}

void PartyDashboard_Select(u32 slot, struct Pokemon *mon)
{
    SelectMon(slot, mon, TRUE);
}

// Returns FALSE when there is no tab to switch to (eggs).
bool32 PartyDashboard_SetTab(s32 delta)
{
    if (sDashboard->mon.isEgg || !sDashboard->hasSelection)
        return FALSE;

    sDashboard->tab = PartyDashboard_WrapTab(sDashboard->tab, delta, FALSE);
    DrawTabs();
    CopyWindowToVram(WIN_DASH_TABS, COPYWIN_GFX);
    DrawBody();
    return TRUE;
}

// Redraws the info area when the changed slot is the selected one.
void PartyDashboard_RefreshSlot(u32 slot, struct Pokemon *mon)
{
    if (sDashboard->hasSelection && sDashboard->selectedSlot == slot)
        SelectMon(slot, mon, FALSE);
}

// HP tick: redraws the slot bar and, for the selected slot, only the identity block.
void PartyDashboard_RefreshHp(u32 slot, struct Pokemon *mon)
{
    struct PartyDashboardMon *dashMon = &sDashboard->mon;

    PartyDashboard_DrawSlot(slot, mon);
    if (!sDashboard->hasSelection || sDashboard->selectedSlot != slot || dashMon->isEgg)
        return;

    dashMon->hp = GetMonData(mon, MON_DATA_HP);
    dashMon->maxHp = GetMonData(mon, MON_DATA_MAX_HP);
    dashMon->hpLevel = GetHPBarLevel(dashMon->hp, dashMon->maxHp);
    dashMon->ailment = GetMonAilment(mon);
    DrawIdentity();
}

// ResetSpriteData destroys the status chip and item icon; forget their ids.
void PartyDashboard_ResetSprites(void)
{
    sDashboard->statusSpriteId = SPRITE_NONE;
    sDashboard->itemSpriteId = SPRITE_NONE;
    sDashboard->itemIconItem = ITEM_NONE;
    FreeSpriteTilesByTag(TAG_DASH_ITEM_ICON);
    FreeSpritePaletteByTag(TAG_DASH_ITEM_ICON);
}

// Prints a prompt in the hint bar; NULL prints the idle key hints.
void PartyDashboard_ShowHint(const u8 *text)
{
    if (text == NULL)
        text = sText_DashIdleHint;

    PartyDashboard_ClearHint();
    StringExpandPlaceholders(gStringVar4, text);
    AddTextPrinterParameterized3(sDashboard->hintWindowId, GetFontIdToFit(gStringVar4, FONT_NORMAL, 0, HINT_W * TILE_WIDTH - 2 * HINT_TEXT_X),
                                 HINT_TEXT_X, HINT_TEXT_Y, sIdentTextColors, TEXT_SKIP_DRAW, gStringVar4);
    CopyWindowToVram(sDashboard->hintWindowId, COPYWIN_GFX);
}

void PartyDashboard_ClearHint(void)
{
    FillWindowPixelBuffer(sDashboard->hintWindowId, PIXEL_FILL(0));
    CopyWindowToVram(sDashboard->hintWindowId, COPYWIN_GFX);
}

// Cycles the frame colour of every slot drawn in a selected palette row.
static void UpdatePulse(void)
{
    if (gPaletteFade.active || ++sDashboard->pulseTimer < PULSE_FRAMES)
        return;

    sDashboard->pulseTimer = 0;
    sDashboard->pulseStep = (sDashboard->pulseStep + 1) % ARRAY_COUNT(sPulseColors);
    for (u32 slot = 0; slot < PARTY_DASH_MAX_SLOTS; slot++)
    {
        u32 row = sDashboard->slotRow[slot];

        if (row == PAL_ROW_SLOT_SELECTED || row == PAL_ROW_SLOT_FAINTED_SELECTED)
            LoadPalette(&sPulseColors[sDashboard->pulseStep], BG_PLTT_ID(PARTY_DASH_PAL_SLOT_FIRST + slot) + SLOT_PIX_FRAME, sizeof(u16));
    }
}

// Runs at the start of each frame, before the tasks that change the selection.
void PartyDashboard_Update(void)
{
    if (sDashboard == NULL)
        return;

    if (sDashboard->pendingBody)
        DrawBody();
    UpdatePulse();
}
