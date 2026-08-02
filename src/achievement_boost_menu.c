#include "global.h"
#include "achievements.h"
#include "achievement_boost_menu.h"
#include "achievement_icons.h"
#include "bg.h"
#include "decompress.h"
#include "gpu_regs.h"
#include "international_string_util.h"
#include "line_break.h"
#include "list_menu.h"
#include "main.h"
#include "malloc.h"
#include "menu.h"
#include "money.h"               // IsEnoughMoney, for the Stage 11 reset row's status text
#include "palette.h"
#include "scanline_effect.h"
#include "sound.h"
#include "sprite.h"
#include "string_util.h"
#include "task.h"
#include "text.h"
#include "text_window.h"
#include "window.h"
#include "constants/rgb.h"
#include "constants/songs.h"

// Stage 7 (design doc Stage 7 / plan §7): skeleton copied wholesale from
// src/achievements_menu.c (same BG/window templates, staged CB2 init,
// ListMenu + scroll arrows) -- same precedent achievements_menu.c itself
// followed from src/new_game_settings_menu.c.
//
// UI art pass: bg1 is a dedicated art layer (own charBaseIndex, separate
// from every window's font tiles) showing the "boosts" screen of
// graphics/achievements/ui/bg_main.png, loaded once at CB2 init -- this menu
// has no sub-levels besides the reset-confirmation flow, which reuses
// WIN_LIST/WIN_DESCRIPTION on the same screen, so there's no background
// swap to do. bg0 holds all three windows and sits in front of it (lower
// priority number); windows are filled with PIXEL_FILL(0), palette index 0
// being the one index every BG layer treats as see-through, so the art shows
// through everywhere there isn't glyph ink. See src/achievements_menu.c's
// own header comment for the fuller version of this (it swaps between two
// such screens) and src/ui_stat_editor.c for the same bg1-art/bg0-window
// split this borrows from.
// Unlike achievements_menu.c there's no tier grouping here: BOOSTS_COUNT is
// small and boosts aren't tiered, so this is a single flat list rather than
// a tier -> list -> detail flow. [A] on a row attempts to purchase that
// boost's next level directly; there's no separate confirmation screen. Once
// a boost is owned, [A] on a binary one instead flips it on/off, and L/R on
// a leveled one dials its *active* level up/down without spending or
// refunding anything (see AchievementBoost_GetActiveLevel/
// _TryChangeActiveLevel in src/achievements.c and
// TryPurchaseOrToggleBoost/TryChangeHighlightedBoostActiveLevel below).
//
// Reached from src/achievements_menu.c's TIER SELECT level via a "BOOSTS"
// row appended when Achievement_BoostsUnlocked() && Achievement_BoostsEnabled()
// (design doc Stage 6: OFF hides the shop, not just the toggle). Also
// reachable unconditionally from the debug menu for testing before Stage 5's
// unlock gate has actually been cleared on a given save.
//
// Stage 11 (design doc §13/Stage 11): a synthetic "RESET BOOSTS" row is
// appended after the real boosts (BOOST_MENU_ITEM_RESET, same trick as
// achievements_menu.c's own BOOSTS row). Unlike a purchase, [A] on it does
// show a confirmation screen -- it swaps WIN_LIST's ListMenu for a throwaway
// Yes/No list (see EnterResetConfirmLevel) rather than committing directly,
// since AchievementBoost_Reset() is destructive to every purchased level at
// once and costs real Poké money.

enum
{
    WIN_HEADER,
    WIN_LIST,
    WIN_DESCRIPTION,
};

// Five rows, not three: WIN_LIST's own comment (see sBoostMenuWinTemplates)
// explains the underlying height change -- the teal box has room for 5 rows
// at 16px apiece, the same as src/achievements_menu.c's own TIER SELECT list,
// now that WIN_DESCRIPTION no longer needs a whole extra row's worth of
// height reserved for a status line (see that window's own comment).
#define BOOST_MENU_MAX_SHOWED 5

// Stage 11: a synthetic row appended after the real boosts, same "one past
// the last real enum value" trick src/achievements_menu.c uses for its own
// TIER_SELECT_ITEM_BOOSTS row -- never a real BoostId, never passed to
// AchievementBoost_GetInfo.
#define BOOST_MENU_ITEM_RESET BOOSTS_COUNT
#define BOOST_MENU_ITEM_COUNT (BOOSTS_COUNT) // (BOOSTS_COUNT - 1) real boosts (excludes BOOST_NONE) + the reset row

#define tListTaskId        data[0]
#define tScrollArrowTaskId data[1]

#define TAG_BOOST_MENU_SCROLL_ARROWS 6002

// Two centered columns on each list row, sitting in the "boosts" screen's own
// second/third light-blue box (graphics/achievements/ui/bg_main.png): the
// boost's active level/toggle state (a lock icon if nothing's owned yet, else
// an active-vs-owned fraction, MAX, or ON/OFF -- see BoostMenu_ItemPrintCallback
// and DrawBoostMenuLevelValue), then what the next level costs, shown only
// when a purchase is actually the next thing this boost would do. Centers
// pixel-sampled from that art (screen-relative box borders at x=167/202/238,
// window-relative -- WIN_LIST's tilemapLeft is 2 tiles/16px -- centers of the
// two ~35px-wide boxes land at 168 and 204). Names top out around 100px from
// item_X 8 ("Legendary Encounter", the longest in the catalog), so neither
// column collides with them.
#define BOOST_MENU_LEVEL_CENTER_X 168
#define BOOST_MENU_COST_CENTER_X  204

// WIN_DESCRIPTION's two text lines. FONT_NORMAL's line height is exactly 16px
// (src/text.c's fontAttributes[FONT_NORMAL].maxLetterHeight) and the window
// itself is 5 tiles/40px tall (see sBoostMenuWinTemplates) rather than the
// 4 tiles/32px the two lines alone need, so there's an 8px margin split 6px
// above LINE1_Y and 2px below LINE2_Y -- shifts the text up within the box
// rather than leaving it hugging the top with zero margin, closer to
// centered against the art's own description box.
#define BOOST_MENU_LINE1_Y 7
// LINE2_Y is a fixed y rather than wherever a wrapped first line happened to
// stop -- used only by the RESET BOOSTS row's own status message and
// PrintResetConfirmText, the two places that still need two independent
// lines rather than one wrapped description. See PrintBoostStatus.
#define BOOST_MENU_LINE2_Y 23

#define BOOST_MENU_ARROW_X        200
// Follows WIN_LIST's tilemapTop/height (see sBoostMenuWinTemplates), 4px
// outside the window's top/bottom edge on both ends -- same offset
// src/achievements_menu.c's own ACHIEVEMENTS_ARROW_TOP_Y/_BOTTOM_Y use.
#define BOOST_MENU_ARROW_TOP_Y    20
#define BOOST_MENU_ARROW_BOTTOM_Y 108

// WIN_DESCRIPTION is 26 tiles (208px) wide, text starts at x=8 --
// AddTextPrinterParameterized never clips or wraps on its own (same
// precedent as src/achievement_popup.c's ACHIEVEMENT_POPUP_DESC_MAX_WIDTH),
// so an unwrapped boost description longer than this bleeds past the
// window's right edge into the tile memory of the line below it.
#define BOOST_MENU_DESC_MAX_WIDTH 190

// Checkbox-free version of achievements_menu.c's equivalent buffer sizing
// comment: the longest real content is a boost name, already capped at
// BOOST_NAME_LENGTH (including its terminator) by BOOST_NAME().
#define BOOST_MENU_NAME_BUFFER_SIZE (BOOST_NAME_LENGTH + 8)

EWRAM_DATA static u8 sBoostMenuNameBuffers[BOOST_MENU_ITEM_COUNT][BOOST_MENU_NAME_BUFFER_SIZE] = {0};
EWRAM_DATA static struct ListMenuItem sBoostMenuListItems[BOOST_MENU_ITEM_COUNT] = {0};

EWRAM_DATA static struct
{
    u16 scrollOffset;
    u16 selectedRow;
    u16 highlightedId;
} sBoostMenu = {0};

static void Task_BoostMenuFadeIn(u8 taskId);
static void Task_BoostMenuCancel(u8 taskId);
static void Task_BoostMenu_ProcessInput(u8 taskId);
static void EnterBoostMenuLevel(u8 taskId);
static void DestroyCurrentBoostList(u8 taskId);
static void TryPurchaseBoost(u8 taskId, u16 boostId);
static void TryPurchaseOrToggleBoost(u8 taskId, u16 boostId);
static void TryChangeHighlightedBoostActiveLevel(u8 taskId);
static void BoostMenu_MoveCursorCallback(s32 itemIndex, bool8 onInit, struct ListMenu *list);
static void BoostMenu_ItemPrintCallback(u8 windowId, u32 boostId, u8 y);
static void BuildBoostMenuListItems(void);
static void PrintBoostDescription(const u8 *description);
static s32 PrintBoostLineText(const u8 *text, s32 x, u8 y);
static s32 BlitBoostLinePointsIcon(s32 x, u8 y);
static void PrintBoostStatus(s32 boostId);
static void DrawHeaderText(void);

// Stage 11: the reset-confirmation sub-flow. Reuses WIN_LIST/WIN_DESCRIPTION
// and the same ListMenu plumbing as the main boost list -- "tear down and
// re-enter with a different item set" is the same trick TryPurchaseBoost
// already uses to refresh the list after a purchase, just pointed at a
// throwaway 2-item Yes/No list instead of the real boost catalog.
static void TryResetBoosts(u8 taskId);
static void EnterResetConfirmLevel(u8 taskId);
static void DestroyResetConfirmList(u8 taskId);
static void ReturnToBoostList(u8 taskId);
static void Task_BoostMenu_ConfirmResetInput(u8 taskId);
static void ResetConfirmMoveCursorCallback(s32 itemIndex, bool8 onInit, struct ListMenu *list);
static void PrintResetConfirmText(void);

static const u8 sText_BoostMenuTitle[]  = _("BOOSTS");
static const u8 sText_ControlHint[]     = _("{B_BUTTON} BACK");
// Matches src/achievements_menu.c's own sText_PointsSummaryFormat -- the
// header's points balance is now a fraction ({available}/{total earned}),
// not just the available half on its own (see DrawHeaderText).
static const u8 sText_PointsSummaryFormat[] = _("{STR_VAR_1}/{STR_VAR_2}");
static const u8 sText_BoostOn[]         = _("ON");
static const u8 sText_BoostOff[]        = _("OFF");
static const u8 sText_BoostMax[]        = _("MAX");
static const u8 sText_BoostLevelSep[]   = _("/");

// Only shown for the (debug-only) case of the menu being reachable before
// Achievement_BoostsUnlocked() -- see this file's own header comment. Every
// other per-boost status (level/toggle/cost) used to live on a third
// description-window line; it's conveyed inline in the list now (see
// BoostMenu_ItemPrintCallback), so there's nothing left to say about a real
// boost row beyond its plain description.
static const u8 sText_BoostSystemLockedStatus[] = _("Boosts are locked.");

// Stage 11: the synthetic "RESET BOOSTS" row and its confirmation sub-flow.
// The two figures that used to be suffixed "pts" are drawn with the points
// icon instead, which is why the refund and the fee are separate strings now
// rather than one format -- the icon goes between them.
static const u8 sText_ResetBoostsRowLabel[]       = _("RESET BOOSTS");
// One line, not a wrapped two -- WIN_DESCRIPTION only has two lines total now
// (see sBoostMenuWinTemplates), and the reset row's own status (refund/fee,
// or why it's unavailable) needs the second one to itself.
static const u8 sText_ResetBoostsDescription[]    = _("Refunds points spent on boosts.");
static const u8 sText_ResetNothingStatus[]        = _("Nothing to reset.");
static const u8 sText_ResetCantAffordStatus[]     = _("Not enough money.");
static const u8 sText_ResetRefundFormat[]         = _("Refund: {STR_VAR_1}");
static const u8 sText_ResetFeeFormat[]            = _("Fee: ¥{STR_VAR_1}");
static const u8 sText_ResetConfirmQuestion[]      = _("Reset all boosts?");
static const u8 sText_ResetConfirmYes[]           = _("YES");
static const u8 sText_ResetConfirmNo[]            = _("NO");

enum
{
    RESET_CONFIRM_YES,
    RESET_CONFIRM_NO,
};

static const struct ListMenuItem sResetConfirmListItems[] =
{
    [RESET_CONFIRM_YES] = { .name = sText_ResetConfirmYes, .id = RESET_CONFIRM_YES },
    [RESET_CONFIRM_NO]  = { .name = sText_ResetConfirmNo,  .id = RESET_CONFIRM_NO },
};

static const struct WindowTemplate sBoostMenuWinTemplates[] =
{
    // tilemapTop 0, not 1 -- graphics/achievements/ui/bg_main.png's dark-navy
    // header band is genuinely y=0-15 (2 tiles), matching the same fix
    // src/achievements_menu.c's own WIN_HEADER got.
    [WIN_HEADER] = {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 0,
        .width = 26,
        .height = 2,
        .paletteNum = 1,
        .baseBlock = 2
    },
    // tilemapTop 3, not 5 -- same fix as WIN_HEADER above: the art's inset
    // teal body box actually starts at pixel row ~23 (tile 3) on every
    // bg_main.png screen, not row 40 (tile 5).
    //
    // height 10, not 6 -- the teal box runs down to about pixel row 107
    // (tile ~13.4, where the divider into the purple footer starts), the
    // same box src/achievements_menu.c's own WIN_LIST already grew into for
    // its 5-row TIER SELECT list. BOOST_MENU_MAX_SHOWED follows this up to 5
    // rows (see its own comment) now that there's no status line eating a
    // row's worth of the description window below (see WIN_DESCRIPTION).
    //
    // width 28, not 26 -- the art's third column (the boost's price) runs
    // out to screen x=238, almost the full 240px screen width; at
    // tilemapLeft 2 (16px) a 26-tile/208px-wide window's own pixel buffer
    // stopped short of that, clipping the cost text drawn around
    // BOOST_MENU_COST_CENTER_X (204). 28 tiles reaches screen x=240.
    [WIN_LIST] = {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 3,
        .width = 28,
        .height = 10,
        .paletteNum = 1,
        .baseBlock = 0x36
    },
    // baseBlock follows WIN_LIST's own (0x36 + 28*10 tiles = 0x14E) now that
    // WIN_LIST is taller and wider.
    //
    // tilemapTop 14, height 5 (not 15/4) -- the box only needs to fit two
    // 16px lines (32px), but a window sized exactly that tall leaves the text
    // hugging the top edge with no margin at all (see BOOST_MENU_LINE1_Y).
    // Growing the window upward by a tile while keeping its bottom edge fixed
    // (14 + 5 == 15 + 4) leaves room to shift both lines down off the very
    // top without moving where the box itself ends on screen.
    [WIN_DESCRIPTION] = {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 14,
        .width = 26,
        .height = 5,
        .paletteNum = 1,
        .baseBlock = 0x14E
    },
    DUMMY_WIN_TEMPLATE
};

static const struct BgTemplate sBoostMenuBgTemplates[] =
{
    {
        // Art layer -- its own charBaseIndex, never touched by window text.
        .bg = 1,
        .charBaseIndex = 3,
        .mapBaseIndex = 30,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 1,
        .baseTile = 0
    },
    {
        // Windows only. Lower priority number than bg1 so text draws in
        // front of the art; PIXEL_FILL(0) everywhere there's no glyph ink
        // lets bg1 show through.
        .bg = 0,
        .charBaseIndex = 1,
        .mapBaseIndex = 31,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0
    }
};

static const u16 sBoostMenuText_Pal[] = INCGFX_U16("graphics/interface/option_menu_text.pal", ".gbapal");

// {background, foreground, shadow} indices into the palette above: transparent
// (so bg1 art shows through), white text, dark-gray shadow. The generic
// TEXT_COLOR_* constants past WHITE don't match this palette's actual colors
// (DARK_GRAY/LIGHT_GRAY are really orange/amber here -- see
// option_menu_text.pal), so the shadow index (this palette's 7th entry,
// 74 74 74) is written directly rather than through a misleading constant.
static const u8 sBoostMenuTextColors[3] = {
    TEXT_COLOR_TRANSPARENT, TEXT_COLOR_WHITE, 6
};

// Selected-row highlight, same trick and the same source palette as
// src/achievements_menu.c's own sAchievementsListHighlightTextColors: raw
// index 2 is the orange this palette file actually has there (see the
// comment above). Unlike that file's own WIN_LIST, this menu's WIN_LIST
// uses the shared bank (paletteNum 1, no per-window remap -- see
// sBoostMenuWinTemplates), so the shadow index is the same raw 6 every
// other window here already uses, not a remapped low index.
static const u8 sBoostMenuListHighlightTextColors[3] = {
    TEXT_COLOR_TRANSPARENT, 2, 6
};

// Wraps the level/toggle column's value on the highlighted row only, same
// precedent as src/option_menu.c's DrawOptionMenuValue -- signals the value
// can be dialed (L/R for a leveled boost's active level, {A_BUTTON} for a
// binary's on/off) without needing its own text.
static const u8 sText_ChevronLeft[]  = _("{LEFT_ARROW}");
static const u8 sText_ChevronRight[] = _("{RIGHT_ARROW}");
#define BOOST_MENU_CHEVRON_GAP 2

// Deduped from graphics/achievements/ui/bg_main.png (the "boosts" screen) by
// a one-off tool, same as src/achievements_menu.c's own screens -- see that
// file's header comment and src/ui_stat_editor.c for the loading pattern.
static const u32 sBoostsScreenTiles[]   = INCBIN_U32("graphics/achievements/ui/boosts_tileset.4bpp.smol");
static const u32 sBoostsScreenTilemap[] = INCBIN_U32("graphics/achievements/ui/boosts_tileset.bin.smolTM");
static const u16 sBoostsScreenPal[]     = INCBIN_U16("graphics/achievements/ui/boosts_tileset.gbapal");

// bg1's WRAM tilemap buffer -- allocated at CB2 init, freed in
// Task_BoostMenuCancel. Same pattern as src/ui_stat_editor.c's
// sBg1TilemapBuffer; unlike src/achievements_menu.c's equivalent this menu
// never reloads it after the initial load, since there's only one screen.
EWRAM_DATA static u8 *sBoostMenuBg1Tilemap = NULL;

static void MainCB2(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    // Flushes bg1's art tilemap, which CB2_InitAchievementBoostMenu only
    // *schedules* via ScheduleBgCopyTilemapToVram. Windows reach VRAM on their
    // own (CopyWindowToVram copies immediately), so without this the text shows
    // but the background stays as whatever VRAM held at init. Same as
    // src/ui_stat_editor.c's main callback.
    DoScheduledBgTilemapCopiesToVram();
    UpdatePaletteFade();
}

static void VBlankCB(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

void CB2_InitAchievementBoostMenu(void)
{
    u8 taskId;
    switch (gMain.state)
    {
    default:
    case 0:
        SetVBlankCallback(NULL);
        memset(&sBoostMenu, 0, sizeof(sBoostMenu));
        gMain.state++;
        break;
    case 1:
        DmaClearLarge16(3, (void *)(VRAM), VRAM_SIZE, 0x1000);
        DmaClear32(3, OAM, OAM_SIZE);
        DmaClear16(3, PLTT, PLTT_SIZE);
        SetGpuReg(REG_OFFSET_DISPCNT, 0);
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(0, sBoostMenuBgTemplates, ARRAY_COUNT(sBoostMenuBgTemplates));
        sBoostMenuBg1Tilemap = Alloc(0x800);
        memset(sBoostMenuBg1Tilemap, 0, 0x800);
        SetBgTilemapBuffer(1, sBoostMenuBg1Tilemap);
        ChangeBgX(0, 0, BG_COORD_SET);
        ChangeBgY(0, 0, BG_COORD_SET);
        ChangeBgX(1, 0, BG_COORD_SET);
        ChangeBgY(1, 0, BG_COORD_SET);
        InitWindows(sBoostMenuWinTemplates);
        DeactivateAllTextPrinters();
        SetGpuReg(REG_OFFSET_WIN0H, 0);
        SetGpuReg(REG_OFFSET_WIN0V, 0);
        SetGpuReg(REG_OFFSET_WININ, 0);
        SetGpuReg(REG_OFFSET_WINOUT, 0);
        SetGpuReg(REG_OFFSET_BLDCNT, 0);
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
        ShowBg(0);
        ShowBg(1);
        gMain.state++;
        break;
    case 2:
        ResetPaletteFade();
        ScanlineEffect_Stop();
        ResetTasks();
        ResetSpriteData();
        gMain.state++;
        break;
    case 3:
        DecompressAndCopyTileDataToVram(1, sBoostsScreenTiles, 0, 0, 0);
        gMain.state++;
        break;
    case 4:
        FreeTempTileDataBuffersIfPossible();
        DecompressDataWithHeaderWram(sBoostsScreenTilemap, sBoostMenuBg1Tilemap);
        ScheduleBgCopyTilemapToVram(1);
        LoadPalette(sBoostsScreenPal, BG_PLTT_ID(0), PLTT_SIZE_4BPP);
        gMain.state++;
        break;
    case 5:
        LoadPalette(sBoostMenuText_Pal, BG_PLTT_ID(1), sizeof(sBoostMenuText_Pal));
        // Must follow the LoadPalette above, not precede it -- this appends
        // the points icon's colours to that same palette's unused high
        // entries (src/achievement_icons.c).
        AchievementIcons_Load(1);
        gMain.state++;
        break;
    case 6:
        PutWindowTilemap(WIN_HEADER);
        gMain.state++;
        break;
    case 7:
        PutWindowTilemap(WIN_LIST);
        PutWindowTilemap(WIN_DESCRIPTION);
        CopyBgTilemapBufferToVram(0);
        gMain.state++;
        break;
    case 8:
        taskId = CreateTask(Task_BoostMenuFadeIn, 0);
        EnterBoostMenuLevel(taskId);
        gMain.state++;
        break;
    case 9:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
        SetVBlankCallback(VBlankCB);
        SetMainCallback2(MainCB2);
        return;
    }
}

static void Task_BoostMenuFadeIn(u8 taskId)
{
    if (!gPaletteFade.active)
        gTasks[taskId].func = Task_BoostMenu_ProcessInput;
}

static void Task_BoostMenuCancel(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        DestroyCurrentBoostList(taskId);
        DestroyTask(taskId);
        FreeAllWindowBuffers();
        Free(sBoostMenuBg1Tilemap);
        sBoostMenuBg1Tilemap = NULL;
        SetMainCallback2(gMain.savedCallback);
    }
}

static void EnterBoostMenuLevel(u8 taskId)
{
    struct ListMenuTemplate template = {0};

    DrawHeaderText();
    BuildBoostMenuListItems();

    template.items = sBoostMenuListItems;
    template.moveCursorFunc = BoostMenu_MoveCursorCallback;
    template.itemPrintFunc = BoostMenu_ItemPrintCallback;
    template.totalItems = BOOST_MENU_ITEM_COUNT;
    template.maxShowed = BOOST_MENU_MAX_SHOWED;
    template.windowId = WIN_LIST;
    template.header_X = 0;
    template.item_X = 8;
    template.cursor_X = 0;
    template.upText_Y = 1;
    template.cursorPal = 1;
    template.fillValue = 0;
    template.cursorShadowPal = 6;
    template.lettersSpacing = 0;
    template.itemVerticalPadding = 0;
    template.scrollMultiple = LIST_NO_MULTIPLE_SCROLL;
    template.fontId = FONT_NORMAL;
    // No arrow cursor -- matches src/achievements_menu.c's own TIER SELECT/
    // LIST screens (see their CURSOR_INVISIBLE comment). BoostMenu_ItemPrintCallback's
    // orange ListMenuOverrideSetColors call marks the selected row instead.
    template.cursorKind = CURSOR_INVISIBLE;

    gTasks[taskId].tListTaskId = ListMenuInit(&template, sBoostMenu.scrollOffset, sBoostMenu.selectedRow);
    gTasks[taskId].tScrollArrowTaskId = AddScrollIndicatorArrowPairParameterized(
        SCROLL_ARROW_UP, BOOST_MENU_ARROW_X, BOOST_MENU_ARROW_TOP_Y, BOOST_MENU_ARROW_BOTTOM_Y,
        // Clamped rather than a bare subtraction: a catalog smaller than
        // BOOST_MENU_MAX_SHOWED (as it was back when this file was written
        // against 2 test boosts) makes the raw difference negative. That gets
        // stored into struct ScrollIndicatorPair's fullyDownThreshold, which
        // is a u16 (src/list_menu.c:29) -- a negative s32 truncates into a
        // huge unsigned threshold the real (always-0) scroll offset can never
        // match, leaving the down arrow permanently visible with nothing left
        // to scroll to.
        (BOOST_MENU_ITEM_COUNT > BOOST_MENU_MAX_SHOWED) ? (BOOST_MENU_ITEM_COUNT - BOOST_MENU_MAX_SHOWED) : 0,
        TAG_BOOST_MENU_SCROLL_ARROWS, TAG_BOOST_MENU_SCROLL_ARROWS,
        &sBoostMenu.scrollOffset);
}

static void Task_BoostMenu_ProcessInput(u8 taskId)
{
    s32 itemId = ListMenu_ProcessInput(gTasks[taskId].tListTaskId);
    ListMenuGetScrollAndRow(gTasks[taskId].tListTaskId, &sBoostMenu.scrollOffset, &sBoostMenu.selectedRow);

    switch (itemId)
    {
    case LIST_NOTHING_CHOSEN:
        // Neither an item selection nor a cursor move -- the only other
        // input this screen reacts to is L/R dialing the highlighted
        // leveled boost's active level.
        TryChangeHighlightedBoostActiveLevel(taskId);
        break;
    case LIST_CANCEL:
        PlaySE(SE_SELECT);
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        gTasks[taskId].func = Task_BoostMenuCancel;
        break;
    case BOOST_MENU_ITEM_RESET:
        TryResetBoosts(taskId);
        break;
    default:
        TryPurchaseOrToggleBoost(taskId, itemId);
        break;
    }
}

// [A] on a real boost row. A binary boost (maxLevel 1) has no next level to
// buy once it's owned -- there's only the one copy of its effect -- so [A]
// there instead flips AchievementBoost_GetActiveLevel between 0 and 1 (see
// PrintBoostStatus/BoostMenu_ItemPrintCallback's ON/OFF). Everything else,
// including a not-yet-owned binary boost, still goes through the purchase
// path below unchanged.
static void TryPurchaseOrToggleBoost(u8 taskId, u16 boostId)
{
    const struct AchievementBoost *info = AchievementBoost_GetInfo(boostId);

    if (info->type == BOOST_TYPE_BINARY && AchievementBoost_GetLevel(boostId) > 0)
    {
        s8 delta = (AchievementBoost_GetActiveLevel(boostId) != 0) ? -1 : 1;

        // Full list rebuild rather than TryChangeHighlightedBoostActiveLevel's
        // lighter RedrawListMenu -- [A] isn't a repeating input the way L/R
        // is, so there's no cost to reusing the same rebuild a purchase does.
        if (AchievementBoost_TryChangeActiveLevel(boostId, delta))
        {
            PlaySE(SE_SELECT);
            DestroyCurrentBoostList(taskId);
            EnterBoostMenuLevel(taskId);
        }
        else
        {
            PlaySE(SE_FAILURE);
        }
        return;
    }

    TryPurchaseBoost(taskId, boostId);
}

// L/R dpad: dials the highlighted leveled boost's active level up or down
// (AchievementBoost_TryChangeActiveLevel) without touching what's been
// purchased. A no-op on the reset row and on binary boosts, which use [A]
// instead (see TryPurchaseOrToggleBoost above). The active level now shows
// in the list row itself (see BoostMenu_ItemPrintCallback), and whether the
// cost column is visible at all depends on it too, so this needs a
// RedrawListMenu on top of PrintBoostStatus's own description refresh --
// RedrawListMenu rather than a full DestroyCurrentBoostList/EnterBoostMenuLevel
// rebuild (what a purchase or toggle does) since L/R can repeat rapidly and
// nothing about the list's scroll arrows or item set actually changed.
static void TryChangeHighlightedBoostActiveLevel(u8 taskId)
{
    u16 boostId = sBoostMenuListItems[sBoostMenu.scrollOffset + sBoostMenu.selectedRow].id;
    s8 delta;

    if (boostId == BOOST_MENU_ITEM_RESET)
        return;
    if (AchievementBoost_GetInfo(boostId)->type == BOOST_TYPE_BINARY)
        return;

    if (JOY_NEW(DPAD_LEFT))
        delta = -1;
    else if (JOY_NEW(DPAD_RIGHT))
        delta = 1;
    else
        return;

    if (AchievementBoost_TryChangeActiveLevel(boostId, delta))
    {
        PlaySE(SE_SELECT);
        RedrawListMenu(gTasks[taskId].tListTaskId);
        PrintBoostStatus(boostId);
    }
    else
    {
        PlaySE(SE_FAILURE);
    }
}

// A successful purchase changes the highlighted row's level/cost and this
// boost's available-points balance, so the simplest correct refresh is the
// same "tear down and re-enter with the cached scroll/selected row" trick
// achievements_menu.c uses between its own levels, rather than trying to
// patch just the affected row/text in place.
static void TryPurchaseBoost(u8 taskId, u16 boostId)
{
    // A purchase always buys the next level past what's owned -- if the
    // active level's been dialed back below that (see
    // AchievementBoost_TryChangeActiveLevel), buying here would silently
    // jump the owned level past the one actually in effect, with no price
    // ever having been shown to explain why (BoostMenu_ItemPrintCallback
    // only shows the cost column under this same condition). [A] refuses
    // until L/R dials back up to the owned level.
    if (AchievementBoost_GetActiveLevel(boostId) != AchievementBoost_GetLevel(boostId))
    {
        PlaySE(SE_FAILURE);
        return;
    }

    if (AchievementBoost_Purchase(boostId))
    {
        PlaySE(SE_SHOP);
        DestroyCurrentBoostList(taskId);
        EnterBoostMenuLevel(taskId);
    }
    else
    {
        PlaySE(SE_FAILURE);
    }
}

// Stage 11: unlike TryPurchaseBoost, this doesn't commit anything by itself
// -- AchievementBoost_CanReset() gates entry into the confirmation sub-flow
// (Task_BoostMenu_ConfirmResetInput), which is the only place that actually
// calls AchievementBoost_Reset().
static void TryResetBoosts(u8 taskId)
{
    if (!AchievementBoost_CanReset())
    {
        PlaySE(SE_FAILURE);
        return;
    }

    PlaySE(SE_SELECT);
    DestroyCurrentBoostList(taskId);
    EnterResetConfirmLevel(taskId);
    gTasks[taskId].func = Task_BoostMenu_ConfirmResetInput;
}

static void BoostMenu_MoveCursorCallback(s32 itemIndex, bool8 onInit, struct ListMenu *list)
{
    if (!onInit)
        PlaySE(SE_SELECT);
    // Read by BoostMenu_ItemPrintCallback (itemIndex here is the item's id,
    // not its row position -- see list_menu.c's own moveCursorFunc call) to
    // decide which row gets the orange highlight. Same trick as
    // src/achievements_menu.c's sAchievementsMenu.highlightedId.
    sBoostMenu.highlightedId = itemIndex;
    // A same-page cursor move never reprints row text on its own (see
    // list_menu.c's ListMenuChangeSelectionFull case 1 -- with cursorKind
    // CURSOR_INVISIBLE, ListMenuDrawCursor is the only thing it calls, and
    // that's a no-op for this cursor kind), so without this the orange
    // highlight would only ever show on whichever row happened to be
    // selected when the list was first drawn.
    ListMenuRepaintItems(list);
    PrintBoostStatus(itemIndex);
}

// Centers `text` in the level/toggle column, wrapping it in chevrons on the
// highlighted row only (src/option_menu.c's DrawOptionMenuValue does the same
// for its own adjustable rows) -- everything reaching this function is
// something L/R (a leveled boost's active level) or {A_BUTTON} (a binary's
// on/off) can currently change, so the chevrons and the row's orange
// highlight always agree on which row they're pointing at.
static void DrawBoostMenuLevelValue(u8 windowId, u8 y, const u8 *text, const u8 *colors, bool8 selected)
{
    s32 width = GetStringWidth(FONT_NORMAL, text, 0);
    s32 x = BOOST_MENU_LEVEL_CENTER_X - width / 2;

    AddTextPrinterParameterized3(windowId, FONT_NORMAL, x, y, colors, TEXT_SKIP_DRAW, text);

    if (selected)
    {
        s32 leftWidth = GetStringWidth(FONT_NORMAL, sText_ChevronLeft, 0);
        AddTextPrinterParameterized3(windowId, FONT_NORMAL, x - leftWidth - BOOST_MENU_CHEVRON_GAP, y, colors, TEXT_SKIP_DRAW, sText_ChevronLeft);
        AddTextPrinterParameterized3(windowId, FONT_NORMAL, x + width + BOOST_MENU_CHEVRON_GAP, y, colors, TEXT_SKIP_DRAW, sText_ChevronRight);
    }
}

static void BoostMenu_ItemPrintCallback(u8 windowId, u32 boostId, u8 y)
{
    const struct AchievementBoost *info;
    u8 owned, active;
    s32 width;
    bool8 selected = (boostId == sBoostMenu.highlightedId);
    const u8 *colors = selected ? sBoostMenuListHighlightTextColors : sBoostMenuTextColors;

    // Recolours the row name ListMenuPrint is about to draw right after this
    // returns (see list_menu.c's ListMenuPrintEntries) -- same precedent as
    // src/achievements_menu.c's TierSelect_ItemPrintCallback, including the
    // RESET BOOSTS row getting the same treatment before its own early
    // return below despite having no level/cost columns of its own.
    if (selected)
        ListMenuOverrideSetColors(colors[1], colors[0], colors[2]);

    // Stage 11: the reset row has no level/cost to show on its right side --
    // AchievementBoost_GetInfo(BOOST_MENU_ITEM_RESET) would otherwise fall
    // back to the BOOST_NONE dummy entry (maxLevel 0) and misprint "MAX".
    if (boostId == BOOST_MENU_ITEM_RESET)
        return;

    info = AchievementBoost_GetInfo(boostId);
    owned = AchievementBoost_GetLevel(boostId);
    active = AchievementBoost_GetActiveLevel(boostId);

    // First column: locked (nothing purchased -- a binary not yet bought, or
    // a leveled boost still at 0) gets the lock icon and nothing else, same
    // as before. Owned gets whichever of ON/OFF, MAX, or the active/owned
    // fraction applies, wrapped in chevrons on the highlighted row.
    if (owned == 0)
    {
        AchievementIcons_Blit(ACHIEVEMENT_ICON_LOCK, windowId, BOOST_MENU_LEVEL_CENTER_X - ACHIEVEMENT_ICON_SIZE / 2, ACHIEVEMENT_ICON_Y(y));
    }
    else if (info->type == BOOST_TYPE_BINARY)
    {
        StringCopy(gStringVar4, active != 0 ? sText_BoostOn : sText_BoostOff);
        DrawBoostMenuLevelValue(windowId, y, gStringVar4, colors, selected);
    }
    else if (owned >= info->maxLevel)
    {
        DrawBoostMenuLevelValue(windowId, y, sText_BoostMax, colors, selected);
    }
    else
    {
        u8 *ptr = ConvertIntToDecimalStringN(gStringVar4, active, STR_CONV_MODE_LEFT_ALIGN, 2);
        ptr = StringCopy(ptr, sText_BoostLevelSep);
        ConvertIntToDecimalStringN(ptr, owned, STR_CONV_MODE_LEFT_ALIGN, 2);
        DrawBoostMenuLevelValue(windowId, y, gStringVar4, colors, selected);
    }

    // Second column: what the next level costs -- but only when a purchase
    // is actually the next thing [A] would do here. Nothing left to buy once
    // maxed, and nothing to buy right now if the active level's been dialed
    // back below what's owned (see TryPurchaseBoost's matching gate) -- L/R
    // back up to the owned level first.
    if (owned >= info->maxLevel || active != owned)
        return;

    ConvertIntToDecimalStringN(gStringVar4, info->costs[owned], STR_CONV_MODE_LEFT_ALIGN, 6);
    width = GetStringWidth(FONT_NORMAL, gStringVar4, 0);
    AddTextPrinterParameterized3(windowId, FONT_NORMAL, BOOST_MENU_COST_CENTER_X - width / 2, y, colors, TEXT_SKIP_DRAW, gStringVar4);
}

// Achievement completion (and therefore available points) can't change
// while this menu is open outside of a purchase, which already rebuilds the
// whole level -- same one-time-per-entry cost/benefit as
// achievements_menu.c's BuildAchievementListItems.
static void BuildBoostMenuListItems(void)
{
    u32 id, index = 0;

    for (id = BOOST_NONE + 1; id < BOOSTS_COUNT; id++)
    {
        const struct AchievementBoost *info = AchievementBoost_GetInfo(id);
        u8 *buffer = sBoostMenuNameBuffers[index];

        StringCopy(buffer, info->name);
        sBoostMenuListItems[index].name = buffer;
        sBoostMenuListItems[index].id = id;
        index++;
    }

    // Stage 11: the synthetic "RESET BOOSTS" row, always last.
    StringCopy(sBoostMenuNameBuffers[index], sText_ResetBoostsRowLabel);
    sBoostMenuListItems[index].name = sBoostMenuNameBuffers[index];
    sBoostMenuListItems[index].id = BOOST_MENU_ITEM_RESET;
}

// Wraps the description into the window's two lines. The wrapping
// (StripLineBreaks + BreakStringAutomatic, the same fix
// src/achievement_popup.c uses) is what stops a long description from
// drawing past the window's right edge and bleeding into the tile memory of
// the line below it.
static void PrintBoostDescription(const u8 *description)
{
    StringCopy(gStringVar1, description);
    StripLineBreaks(gStringVar1);
    BreakStringAutomatic(gStringVar1, BOOST_MENU_DESC_MAX_WIDTH, 2, FONT_NORMAL, HIDE_SCROLL_PROMPT);
    AddTextPrinterParameterized3(WIN_DESCRIPTION, FONT_NORMAL, 8, BOOST_MENU_LINE1_Y, sBoostMenuTextColors, TEXT_SKIP_DRAW, gStringVar1);
}

// Prints text at x on the given line and returns where the next thing after it
// should start, so a run of alternating strings and points icons can be laid
// out left to right without any of them needing to know the widths of the
// others. Returns an x rather than drawing everything itself because what
// follows a figure differs per line: sometimes an icon, sometimes nothing.
static s32 PrintBoostLineText(const u8 *text, s32 x, u8 y)
{
    AddTextPrinterParameterized3(WIN_DESCRIPTION, FONT_NORMAL, x, y, sBoostMenuTextColors, TEXT_SKIP_DRAW, text);
    return x + GetStringWidth(FONT_NORMAL, text, 0) + 2;
}

static s32 BlitBoostLinePointsIcon(s32 x, u8 y)
{
    AchievementIcons_Blit(ACHIEVEMENT_ICON_POINTS, WIN_DESCRIPTION, x, ACHIEVEMENT_ICON_Y(y));
    return x + ACHIEVEMENT_ICON_SIZE + 4;
}

// Real boosts (see BoostMenu_ItemPrintCallback) don't need to say anything
// about level/toggle/cost here any more -- that's all inline in the list
// row now -- so WIN_DESCRIPTION only has two jobs left: the plain
// description (either kind of row), and the RESET BOOSTS row's own status
// (refund/fee, or why it's unavailable), which has nowhere else to go since
// that row has no level/cost columns of its own.
static void PrintBoostStatus(s32 boostId)
{
    u8 statusBuf[40];

    FillWindowPixelBuffer(WIN_DESCRIPTION, PIXEL_FILL(0));

    // Debug-only in practice (see this file's own header comment on how the
    // menu is reachable before Achievement_BoostsUnlocked()) -- skips the
    // description entirely rather than trying to also fit it above this on
    // a window that only has two lines total.
    if (!Achievement_BoostsUnlocked())
    {
        PrintBoostLineText(sText_BoostSystemLockedStatus, 8, BOOST_MENU_LINE1_Y);
    }
    else if (boostId == BOOST_MENU_ITEM_RESET)
    {
        PrintBoostLineText(sText_ResetBoostsDescription, 8, BOOST_MENU_LINE1_Y);

        // Same priority order as AchievementBoost_CanReset (src/achievements.c),
        // reimplemented here for messaging -- same precedent
        // AchievementBoost_CanPurchase's own callers follow, so it can
        // explain *which* check failed rather than just refusing.
        if (Achievement_GetAvailablePoints() == Achievement_GetTotalPoints())
        {
            // available == total iff pointsInvested == 0 -- nothing purchased
            // to refund. Avoids needing a public pointsInvested accessor.
            PrintBoostLineText(sText_ResetNothingStatus, 8, BOOST_MENU_LINE2_Y);
        }
        else if (!IsEnoughMoney(&gSaveBlock1Ptr->money, ACHIEVEMENT_BOOST_RESET_FEE))
        {
            PrintBoostLineText(sText_ResetCantAffordStatus, 8, BOOST_MENU_LINE2_Y);
        }
        else
        {
            s32 x;

            ConvertIntToDecimalStringN(gStringVar1, Achievement_GetTotalPoints() - Achievement_GetAvailablePoints(), STR_CONV_MODE_LEFT_ALIGN, 6);
            StringExpandPlaceholders(statusBuf, sText_ResetRefundFormat);
            x = PrintBoostLineText(statusBuf, 8, BOOST_MENU_LINE2_Y);
            x = BlitBoostLinePointsIcon(x, BOOST_MENU_LINE2_Y);

            ConvertIntToDecimalStringN(gStringVar1, ACHIEVEMENT_BOOST_RESET_FEE, STR_CONV_MODE_LEFT_ALIGN, 6);
            StringExpandPlaceholders(statusBuf, sText_ResetFeeFormat);
            PrintBoostLineText(statusBuf, x, BOOST_MENU_LINE2_Y);
        }
    }
    else if (boostId >= BOOST_NONE + 1 && boostId < BOOSTS_COUNT)
    {
        PrintBoostDescription(AchievementBoost_GetInfo(boostId)->description);
    }

    CopyWindowToVram(WIN_DESCRIPTION, COPYWIN_GFX);
}

static void DestroyCurrentBoostList(u8 taskId)
{
    DestroyListMenuTask(gTasks[taskId].tListTaskId, NULL, NULL);
    RemoveScrollIndicatorArrowPair(gTasks[taskId].tScrollArrowTaskId);
}

// Stage 11: the two-item Yes/No confirmation list. Only 2 rows against
// BOOST_MENU_MAX_SHOWED 5, so unlike EnterBoostMenuLevel this never needs
// scroll arrows -- DestroyResetConfirmList (below) mirrors that by never
// touching tScrollArrowTaskId, which still holds the main list's arrow pair
// until DestroyCurrentBoostList tears it down on the way back in.
static void EnterResetConfirmLevel(u8 taskId)
{
    struct ListMenuTemplate template = {0};

    PrintResetConfirmText();

    template.items = sResetConfirmListItems;
    template.moveCursorFunc = ResetConfirmMoveCursorCallback;
    template.itemPrintFunc = NULL;
    template.totalItems = ARRAY_COUNT(sResetConfirmListItems);
    template.maxShowed = ARRAY_COUNT(sResetConfirmListItems);
    template.windowId = WIN_LIST;
    template.header_X = 0;
    template.item_X = 8;
    template.cursor_X = 0;
    template.upText_Y = 1;
    template.cursorPal = 2;
    template.fillValue = 0;
    template.cursorShadowPal = 3;
    template.lettersSpacing = 0;
    template.itemVerticalPadding = 0;
    template.scrollMultiple = LIST_NO_MULTIPLE_SCROLL;
    template.fontId = FONT_NORMAL;
    template.cursorKind = CURSOR_BLACK_ARROW;

    gTasks[taskId].tListTaskId = ListMenuInit(&template, 0, 0);
}

static void DestroyResetConfirmList(u8 taskId)
{
    DestroyListMenuTask(gTasks[taskId].tListTaskId, NULL, NULL);
}

static void ReturnToBoostList(u8 taskId)
{
    DestroyResetConfirmList(taskId);
    EnterBoostMenuLevel(taskId);
    gTasks[taskId].func = Task_BoostMenu_ProcessInput;
}

static void Task_BoostMenu_ConfirmResetInput(u8 taskId)
{
    s32 itemId = ListMenu_ProcessInput(gTasks[taskId].tListTaskId);

    switch (itemId)
    {
    case LIST_NOTHING_CHOSEN:
        break;
    case LIST_CANCEL:
    case RESET_CONFIRM_NO:
        PlaySE(SE_SELECT);
        ReturnToBoostList(taskId);
        break;
    case RESET_CONFIRM_YES:
        // Re-verified inside AchievementBoost_Reset itself (same "never trust
        // a stale Can* result" precedent as AchievementBoost_Purchase) -- this
        // can still fail if something spent the fee's worth of money or the
        // profile's boosts got re-locked between opening this prompt and
        // choosing YES.
        if (AchievementBoost_Reset())
            PlaySE(SE_SHOP);
        else
            PlaySE(SE_FAILURE);
        ReturnToBoostList(taskId);
        break;
    }
}

static void ResetConfirmMoveCursorCallback(s32 itemIndex, bool8 onInit, struct ListMenu *list)
{
    if (!onInit)
        PlaySE(SE_SELECT);
}

// design doc §13/Stage 11 "the confirmation prompt shows the refund, the fee
// and the resulting totals before committing": this view is only ever
// entered when AchievementBoost_CanReset() already returned TRUE, so unlike
// PrintBoostStatus's reset branch, this doesn't need to handle the
// locked/nothing-to-reset/can't-afford cases -- there is always a real
// refund and fee to show.
// Two lines, one per text row of WIN_DESCRIPTION now has (see
// sBoostMenuWinTemplates) -- the question leads, refund and fee share the
// second line. No separate "new total" line any more: the header (redrawn
// by ReturnToBoostList's EnterBoostMenuLevel either way) already shows the
// available/total fraction, and after a full refund that's just the total.
static void PrintResetConfirmText(void)
{
    u8 lineBuf[40];
    s32 x;

    FillWindowPixelBuffer(WIN_DESCRIPTION, PIXEL_FILL(0));

    PrintBoostLineText(sText_ResetConfirmQuestion, 8, BOOST_MENU_LINE1_Y);

    ConvertIntToDecimalStringN(gStringVar1, Achievement_GetTotalPoints() - Achievement_GetAvailablePoints(), STR_CONV_MODE_LEFT_ALIGN, 6);
    StringExpandPlaceholders(lineBuf, sText_ResetRefundFormat);
    x = PrintBoostLineText(lineBuf, 8, BOOST_MENU_LINE2_Y);
    x = BlitBoostLinePointsIcon(x, BOOST_MENU_LINE2_Y);

    ConvertIntToDecimalStringN(gStringVar1, ACHIEVEMENT_BOOST_RESET_FEE, STR_CONV_MODE_LEFT_ALIGN, 6);
    StringExpandPlaceholders(lineBuf, sText_ResetFeeFormat);
    PrintBoostLineText(lineBuf, x, BOOST_MENU_LINE2_Y);

    CopyWindowToVram(WIN_DESCRIPTION, COPYWIN_GFX);
}

// Matches src/achievements_menu.c's own DrawTierSelectHeaderText layout: title,
// then the points summary directly after it, then the [B] BACK hint, all on
// WIN_HEADER's one line (tilemapTop 0, see sBoostMenuWinTemplates). The header
// carries the player's spendable balance -- now a fraction against the total
// ever earned, not just the available half on its own -- rather than the
// description window (where it used to tail every cost as "(have N)"), so
// it's on screen whatever row is highlighted. Redrawn by EnterBoostMenuLevel,
// which every purchase re-runs, so the figure tracks what was just spent.
static void DrawHeaderText(void)
{
    // FONT_NARROW, not FONT_NORMAL: "BOOSTS" plus the points summary and the
    // [B] BACK hint all have to share this one line -- narrowing the title
    // leaves the points summary more room before it has to fall back through
    // GetFontIdToFit below.
    s32 titleX = 2;
    s32 hintX = GetStringRightAlignXOffset(FONT_NARROW, sText_ControlHint, 198);
    s32 pointsIconX = titleX + GetStringWidth(FONT_NARROW, sText_BoostMenuTitle, 0) + 4;
    s32 pointsTextX = pointsIconX + ACHIEVEMENT_ICON_SIZE + 2;
    s32 availWidth = (hintX - 8) - pointsTextX;
    u32 fontId;

    if (availWidth < 0)
        availWidth = 0;

    FillWindowPixelBuffer(WIN_HEADER, PIXEL_FILL(0));
    AddTextPrinterParameterized3(WIN_HEADER, FONT_NARROW, titleX, 0, sBoostMenuTextColors, TEXT_SKIP_DRAW, sText_BoostMenuTitle);
    AddTextPrinterParameterized3(WIN_HEADER, FONT_NARROW, hintX, 0, sBoostMenuTextColors, TEXT_SKIP_DRAW, sText_ControlHint);

    // {available points}/{total points earned}.
    ConvertIntToDecimalStringN(gStringVar1, Achievement_GetAvailablePoints(), STR_CONV_MODE_LEFT_ALIGN, 6);
    ConvertIntToDecimalStringN(gStringVar2, Achievement_GetTotalPoints(), STR_CONV_MODE_LEFT_ALIGN, 6);
    StringExpandPlaceholders(gStringVar4, sText_PointsSummaryFormat);
    fontId = GetFontIdToFit(gStringVar4, FONT_NORMAL, 0, availWidth);

    // Not ACHIEVEMENT_ICON_Y(0) -- that macro's -1 inset assumes text one
    // pixel below the icon's own top, which would underflow a u16 at the
    // header's y=0 (same reasoning as achievements_menu.c's own header).
    AchievementIcons_Blit(ACHIEVEMENT_ICON_POINTS, WIN_HEADER, pointsIconX, 0);
    AddTextPrinterParameterized3(WIN_HEADER, fontId, pointsTextX, 0, sBoostMenuTextColors, TEXT_SKIP_DRAW, gStringVar4);

    CopyWindowToVram(WIN_HEADER, COPYWIN_FULL);
}
