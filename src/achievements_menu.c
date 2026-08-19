#include "global.h"
#include "achievements.h"
#include "achievements_menu.h"
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

// src/new_game_settings_menu.c's skeleton copied wholesale -- BG/window
// templates, staged CB2 init, ListMenu + scroll arrows.
//
// UI art pass: bg1 is a dedicated art layer (its own charBaseIndex, separate
// from every window's font tiles) showing one of two full-screen pictures --
// TIER SELECT gets the "achievements" screen, LIST and DETAIL both get the
// "detail" screen (LoadMenuBackground, called from EnterTierSelectLevel/
// EnterListLevel). bg0 holds all three windows and sits in front of it
// (lower priority number); windows are filled with PIXEL_FILL(0), palette
// index 0 being the one index every BG layer treats as see-through, so the
// art shows through everywhere there isn't glyph ink. This replaced an
// earlier message-box-style layout (bg1 = frame border tiles + WIN_HEADER,
// bg0 = WIN_LIST/WIN_DESCRIPTION behind it, PIXEL_FILL(1) as an opaque box
// colour) -- see graphics/achievements/ui/bg_main.png for the source art
// mockup and src/ui_stat_editor.c for the same bg1-art/bg0-window split this
// borrows from.
//
// The three-level TIER SELECT / LIST / DETAIL flow. One CB2 boots the
// screen straight into TIER SELECT; the three levels
// then swap the task's func and rebuild the same WIN_HEADER/WIN_LIST/
// WIN_DESCRIPTION trio in place rather than re-running the CB2 state machine.
// TIER SELECT and LIST each load their own bg1 screen (LoadMenuBackground,
// ACHIEVEMENTS_BG_SCREEN_MAIN vs _DETAIL) with its own palette, so moving
// between those two fades out/in (Task_TierSelect_ToListLevel/
// Task_List_ToTierSelectLevel) to hide the frame where the new palette and
// old tilemap (or vice versa) would otherwise be on screen together --
// LoadPalette writes immediately but ScheduleBgCopyTilemapToVram's copy
// doesn't land until the next vblank. LIST and DETAIL share one bg1 screen
// and never swap it, so moving between those two still doesn't fade.
//
// The Start Menu entry point is wired separately in src/start_menu.c
// (MENU_ACTION_ACHIEVEMENTS / StartMenuAchievementsCallback).
//
// The "Boosts" row on the TIER SELECT screen: appended to the tier list as
// one extra row, id TIER_SELECT_ITEM_BOOSTS, only when
// Achievement_BoostsUnlocked() && Achievement_BoostsEnabled() (OFF hides the
// shop, not just the toggle). Selecting it fades out and jumps to
// src/achievement_boost_menu.c's CB2_InitAchievementBoostMenu, with
// gMain.savedCallback pointed back at CB2_InitAchievementsMenu so its own
// [B] Back re-enters here at a fresh TIER SELECT. Since that overwrites
// gMain.savedCallback -- the same slot this screen's own [B] Back reads on
// its way out -- sAchievementsMenuReturnCallback/sReturningFromBoostShop
// stash the real caller (Start Menu/debug menu) beforehand and restore it on
// the way back in, so leaving from TIER SELECT after a boost-shop visit
// still returns to the real caller instead of looping back into this menu.

enum
{
    WIN_HEADER,
    WIN_LIST,
    WIN_DESCRIPTION,
};

// ACHIEVEMENT_TIER_COUNT itself now comes from enum AchievementTier
// (constants/achievements.h) -- this used to be its own local derivation
// before it was given a shared one.

#define ACHIEVEMENTS_MENU_MAX_SHOWED 5
#define ACHIEVEMENTS_MENU_ITEM_COUNT (ACHIEVEMENTS_COUNT - 1) // excludes ACHIEVEMENT_NONE

#define tListTaskId        data[0]
#define tScrollArrowTaskId data[1]

#define TAG_ACHIEVEMENTS_SCROLL_ARROWS 6001

#define ACHIEVEMENTS_POINTS_RIGHT_X 190
// 0, not 2/8: the row's own arrow cursor used to need that space (cursor_X=0,
// CURSOR_BLACK_ARROW's glyph is ~8px wide) -- now that both TIER SELECT and
// the achievement list highlight the selected row's text colour instead
// (CURSOR_INVISIBLE, see EnterTierSelectLevel/EnterListLevel), nothing draws
// there anymore, so rows can sit flush with the box's left edge (confirmed
// clear of the art's border trim, which ends well before this column -- see
// this file's earlier pixel-sampling of graphics/achievements/ui/bg_main.png).
#define ACHIEVEMENTS_LIST_ITEM_X    0
// Fixed column so every tier's medal icon lines up regardless of how wide
// that tier's name is, with the completed/total count following directly
// after it (see TierSelect_ItemPrintCallback) -- chosen to leave the count
// text roughly the same right-hand budget ACHIEVEMENTS_POINTS_RIGHT_X used
// to reserve when it was right-aligned instead.
#define ACHIEVEMENTS_TIER_ICON_X    122
// TierSelect_DrawRow's completed/total count right-aligns to this rather
// than ACHIEVEMENTS_POINTS_RIGHT_X directly -- sat too close to the medal
// icon/left edge of its own column at that value, so it's nudged 10px
// further right. Kept separate from ACHIEVEMENTS_POINTS_RIGHT_X since that
// one still governs the achievement list's points column (AchievementsMenu_
// DrawRow), which isn't affected by this.
#define ACHIEVEMENTS_TIER_COUNT_RIGHT_X (ACHIEVEMENTS_POINTS_RIGHT_X + 14)
#define ACHIEVEMENTS_ARROW_X        200
// Follows WIN_LIST's tilemapTop/height (see sAchievementsMenuWinTemplates):
// 4px inside the window's top/bottom edge, same offset on both ends.
#define ACHIEVEMENTS_ARROW_TOP_Y    20
#define ACHIEVEMENTS_ARROW_BOTTOM_Y 100

// WIN_DESCRIPTION/WIN_LIST are 26 tiles (208px) wide, text starts at x=8 --
// AddTextPrinterParameterized never clips or wraps on its own (see
// src/achievement_popup.c's own ACHIEVEMENT_POPUP_DESC_MAX_WIDTH precedent),
// so an unwrapped achievement description longer than this bleeds past the
// window's right edge into the tile memory of the row below it.
//
// This width is only actually safe for WIN_DESCRIPTION -- pixel-sampling
// graphics/achievements/ui/bg_main.png's LIST/DETAIL panel (the same
// pixel-sampling ACHIEVEMENTS_LIST_ITEM_X's own comment used) shows its
// underlying box is one continuous panel nearly the full screen wide, well
// past this window's own edges either way. WIN_LIST's box is narrower and
// split in two -- see ACHIEVEMENTS_DETAIL_DESC_MAX_WIDTH below -- so DETAIL's
// own name/description print does *not* reuse this constant.
#define ACHIEVEMENTS_DESC_MAX_WIDTH 190

// Bug (reported after initial delivery): EnterDetailLevel's name/description
// still overflowed even after their x got fixed to ACHIEVEMENTS_LIST_ITEM_X,
// because unlike WIN_DESCRIPTION's box (see ACHIEVEMENTS_DESC_MAX_WIDTH just
// above), WIN_LIST's underlying art is actually *two* boxes side by side --
// the same pixel-sampling shows a divider around screen x=165-170, with a
// second, narrower box picking up right after it that's where the
// achievement list's own points column (right-aligned to
// ACHIEVEMENTS_POINTS_RIGHT_X) actually lives. The left box -- the one
// achievement names/DETAIL's text sit in -- only runs from the window's own
// left edge to screen x=~163, i.e. window-relative x=~147 once
// WIN_LIST's 16px tilemapLeft is subtracted.
//
// Bug (reported after initial delivery, round 2): even inside that measured
// edge, some descriptions were still wrapping a line earlier than the box
// actually has room for -- in-game testing showed ~15px of margin still
// unused short of the divider. Raised from the original 140 to 155 to
// reclaim it; still short of the ~163 hard edge above.
#define ACHIEVEMENTS_DETAIL_DESC_MAX_WIDTH 155

// DETAIL reuses WIN_LIST's own box -- the same one the achievement/tier
// lists fit ACHIEVEMENTS_MENU_MAX_SHOWED (5) 16px rows into. DETAIL spends
// the first row (y=1) on the name, leaving the remaining 4 rows (y=17
// onward, see EnterDetailLevel) for the description -- one more line than
// this used to be capped at.
#define ACHIEVEMENTS_DETAIL_DESC_LINES 4

// WIN_DESCRIPTION's two text lines. FONT_NORMAL's line height is exactly 16px
// (src/text.c's fontAttributes[FONT_NORMAL].maxLetterHeight) and the window
// itself is 5 tiles/40px tall (see sAchievementsMenuWinTemplates) rather than
// the 4 tiles/32px the two lines alone need, so there's an 8px margin --
// spent entirely below LINE2_Y rather than split above/below.
//
// Bug (reported after initial delivery): with the margin split 6px above
// LINE1_Y/2px below LINE2_Y, an overlong description's auto-scroll (see
// PrintAchievementDescription) left the bottom sliver of the retiring line
// visibly sticking out above the new line 1. RunTextPrinters' scroll
// (src/text.c RENDER_STATE_SCROLL, ScrollWindow) shifts the *whole window's*
// pixel buffer up by one line height (16px) regardless of where the text
// inside it actually sits -- with a 7px gap above line 1, only the top 9px of
// the retiring line's 16px scrolled off the window's top edge, leaving its
// bottom 7px behind in that gap. Standard 2-line message boxes elsewhere in
// the game avoid this because their text starts flush with the window's own
// top edge (y=1), so the same 16px shift clears the retiring line entirely.
// Moving LINE1_Y to that same flush position fixes it here too, leaving the
// window's extra tile as pure padding below LINE2_Y instead.
#define ACHIEVEMENTS_DESC_LINE1_Y 1
#define ACHIEVEMENTS_DESC_LINE2_Y 17

// How long an overlong description sits idle on its last screenful (see
// PrintAchievementDescription/MainCB2's descriptionScrolling) before looping
// back to the top and scrolling through again -- long enough to actually
// read it before it resets. 120 frames/~2 seconds, same ballpark as other
// one-off UI pauses elsewhere (e.g. src/hall_of_fame.c's own tFrameCount).
#define ACHIEVEMENTS_DESC_RESTART_DELAY 120

// Checkbox/tier-name prefix plus the item text itself; the longest real
// content (an achievement name) is already capped at ACHIEVEMENT_NAME_LENGTH
// (including its terminator) by ACHIEVEMENT_NAME(), so this leaves generous
// headroom rather than computing the exact minimum. Reused for both the
// achievement list rows and the (shorter) tier select rows.
#define ACHIEVEMENTS_LIST_NAME_BUFFER_SIZE (ACHIEVEMENT_NAME_LENGTH + 8)

// TIER SELECT's own row count once the "BOOSTS" row is visible -- one past
// the last real tier ID, reused as that row's ListMenuItem.id too
// (see TIER_SELECT_ITEM_BOOSTS below).
#define TIER_SELECT_ROW_COUNT (ACHIEVEMENT_TIER_COUNT + 1)

// Shared by both lists this menu ever shows (tier select's up to
// TIER_SELECT_ROW_COUNT rows, or one tier's worth of achievement rows),
// sized to whichever is larger.
//
// Bug (reported after the catalog grew further): this used to be a
// manually-tracked worst-case single-tier count (ACHIEVEMENTS_MENU_MAX_PER_
// TIER, fixed at 60 with a comment claiming the largest tier -- Silver --
// held only 32 entries). BuildAchievementListItems' bounds check fails safe
// (truncates rather than corrupting EWRAM past the array's end), so nothing
// crashed -- the SILVER and GOLD lists just silently stopped rendering past
// their first 60 entries once those tiers' catalogs grew past that guess
// (Silver: 70, Gold: 98 as of this fix), with no compiler warning to catch
// it. A fixed guess needs a human to notice and bump it every time the
// catalog grows; nothing forced that to happen.
//
// Back to ACHIEVEMENTS_MENU_ITEM_COUNT (the whole catalog) instead, so this
// is always at least as large as any single tier's rows can ever be --
// derived from ACHIEVEMENTS_COUNT, so it grows with the catalog
// automatically and can't go stale again. Costs a few KB more EWRAM than a
// tight per-tier bound would (listNameBuffers/listItems on
// struct AchievementsMenuState, further down, are sized off this), which is
// well within budget for a menu that owns none of the game's other
// EWRAM-heavy state.
#define ACHIEVEMENTS_MENU_LIST_CAPACITY \
    (ACHIEVEMENTS_MENU_ITEM_COUNT > TIER_SELECT_ROW_COUNT ? ACHIEVEMENTS_MENU_ITEM_COUNT : TIER_SELECT_ROW_COUNT)

// Bug (found via playtesting): PrintAchievementDescription/PrintDetail
// Description used to build their wrapped text straight into gStringVar1,
// same as every other string in this file. That's fine for everything else
// here, since those are always instant TEXT_SKIP_DRAW prints fully consumed
// before anything else can touch the buffer -- but an overlong description's
// auto-scroll print (needsScroll, both functions' own GetPlayerTextSpeedDelay
// branch) runs across many frames, and AddTextPrinterParameterized3 only ever
// keeps a raw pointer into whatever buffer it's given (TextPrinterTemplate.
// currentChar, src/text.c), never a copy. gStringVar1 is the whole engine's
// scratch space, reused everywhere -- most immediately by this file's own
// AchievementsMenu_DrawRow, which rebuilds it with a row's points figure on
// every single list repaint (RedrawListMenu/RepaintListRow, both called from
// Task_List_ProcessInput right after the moveCursorFunc callback that starts
// a new description printing). Scrolling the list while a description was
// still mid-scroll let that write land underneath the still-running printer:
// it kept reading from the same address and showed the row's points figure
// -- followed by whatever leftover bytes gStringVar1's last unrelated use
// elsewhere in the engine left past that shorter string's terminator,
// occasionally including a stray colour-control byte, hence the orange tint
// -- instead of the description it started out printing. Dedicated buffers
// nothing else in the engine ever writes to can't be clobbered out from
// under a printer still reading them.
#define ACHIEVEMENTS_DESC_BUFFER_SIZE 0x100
// Stage 7: descriptionBuffer/detailDescriptionBuffer used to be their own
// top-level EWRAM_DATA statics here -- they're now fields on
// struct AchievementsMenuState (further down). The reasoning just above for
// why they can't be gStringVar1 still applies unchanged.

// Stage 7: the per-screen state and the per-tier completion counts used to
// each be their own anonymous-struct EWRAM_DATA static declared right here.
// Both are now named fields on struct AchievementsMenuState -- see its own
// comment, further down, for why.

// This menu's own CB2 doubles as the boost shop's return point
// (Task_TierSelect_OpenBoostMenu sets gMain.savedCallback =
// CB2_InitAchievementsMenu before jumping there), which would otherwise
// clobber the *real* caller (Start Menu/debug menu) recorded in
// gMain.savedCallback on entry. sAchievementsMenuReturnCallback is that real
// caller, stashed away before the overwrite and restored into
// gMain.savedCallback the moment this screen is re-entered from the boost
// shop -- sReturningFromBoostShop is what tells case 0 which of those two
// things is happening.
//
// Deliberately kept as their own plain EWRAM_DATA statics rather than folded
// into struct AchievementsMenuState below: both need to survive
// Task_TierSelect_OpenBoostMenu's Free(sAchievementsMenuStatePtr) (that
// exit path tears this whole screen down before jumping to the boost shop)
// and still be readable by CB2_InitAchievementsMenu's case 0 on the way back
// in, *before* that case's own AllocZeroed reallocates the struct.
EWRAM_DATA static bool8 sReturningFromBoostShop = FALSE;
EWRAM_DATA static void (*sAchievementsMenuReturnCallback)(void) = NULL;

static void Task_AchievementsMenuFadeIn(u8 taskId);
static void Task_AchievementsMenuCancel(u8 taskId);
static void LoadMenuBackground(u8 screen);
static void Task_TierSelect_ProcessInput(u8 taskId);
static void Task_TierSelect_OpenBoostMenu(u8 taskId);
static void Task_TierSelect_ToListLevel(u8 taskId);
static void Task_List_ProcessInput(u8 taskId);
static void Task_List_ToTierSelectLevel(u8 taskId);
static void Task_Detail_ProcessInput(u8 taskId);
static void EnterTierSelectLevel(u8 taskId);
static bool8 IsBoostShopRowVisible(void);
static void EnterListLevel(u8 taskId, u8 tier);
static void EnterDetailLevel(u8 taskId, u16 achievementId);
static void DestroyCurrentAchievementsList(u8 taskId);
static void TierSelect_MoveCursorCallback(s32 itemIndex, bool8 onInit, struct ListMenu *list);
static void TierSelect_ItemPrintCallback(u8 windowId, u32 tier, u8 y);
static void TierSelect_DrawRow(u8 windowId, u32 tier, u8 y, const u8 *colors);
static void AchievementsMenu_MoveCursorCallback(s32 itemIndex, bool8 onInit, struct ListMenu *list);
static void AchievementsMenu_ItemPrintCallback(u8 windowId, u32 achievementId, u8 y);
static void AchievementsMenu_DrawRow(u8 windowId, u32 achievementId, u8 y, const u8 *colors);
static void RepaintListRow(void (*drawRow)(u8, u32, u8, const u8 *), u32 arrayIndex, u8 y);
static void BuildTierSelectListItems(void);
static void BuildAchievementListItems(u8 tier);
static void DrawTierSelectHeaderText(void);
static bool8 StringHasScrollPrompt(const u8 *str);
static void PrintAchievementDescription(s32 achievementId);
static void PrintDetailDescription(s32 achievementId);
static void DrawHeaderText(const u8 *title);
static void LoadTierIcons(void);
static void BlitTierIcon(u8 tier, u8 windowId, u16 x, u16 y);

static const u8 sText_AchievementsTitle[]  = _("ACHIEVEMENTS");
static const u8 sText_ControlHint[]        = _("{B_BUTTON} BACK");
// '[' and ']' aren't in charmap.txt -- use the existing filled/hollow circle
// glyphs instead of literal brackets.
static const u8 sText_CompletedPrefix[]    = _("{CIRCLE_DOT} ");
static const u8 sText_IncompletePrefix[]   = _("{CIRCLE_HOLLOW} ");
// Hidden achievements show as "???" -- name and description both -- until
// completed. Their point value and tier are not withheld
// (e.g. "[ ] ???                50").
static const u8 sText_HiddenName[]         = _("???");
static const u8 sText_HiddenDescription[]  = _("???");

static const u8 sText_TierBronze[]  = _("BRONZE");
static const u8 sText_TierSilver[]  = _("SILVER");
static const u8 sText_TierGold[]    = _("GOLD");
static const u8 sText_TierDiamond[] = _("DIAMOND");

static const u8 *const sTierNames[ACHIEVEMENT_TIER_COUNT] =
{
    [ACHIEVEMENT_TIER_BRONZE]  = sText_TierBronze,
    [ACHIEVEMENT_TIER_SILVER]  = sText_TierSilver,
    [ACHIEVEMENT_TIER_GOLD]    = sText_TierGold,
    [ACHIEVEMENT_TIER_DIAMOND] = sText_TierDiamond,
};

static const u8 sText_TierCountSeparator[]  = _(" / ");
static const u8 sText_BoostsMenuRowLabel[]  = _("BOOSTS");

// The extra TIER SELECT row sits one past the last real tier ID --
// safe as a ListMenuItem.id since tier IDs and this are otherwise disjoint,
// and TierSelect_ItemPrintCallback/Task_TierSelect_ProcessInput both check
// for it before treating an itemId as a tier.
#define TIER_SELECT_ITEM_BOOSTS ACHIEVEMENT_TIER_COUNT
// Both of these used to spell out "Points"/"Points:"; the points icon now
// stands in for the word, blitted next to the figure it belongs to (see
// DrawTierSelectHeaderText and EnterDetailLevel), so the strings themselves
// carry only what the icon can't say.
static const u8 sText_PointsSummaryFormat[] = _("{STR_VAR_1}/{STR_VAR_2}");
// Used in place of sText_PointsSummaryFormat until boosts unlock -- before
// that, points can't be spent, so the "{available}/{total}" fraction would
// always read "{total}/{total}" and just be noise; show the bare total.
static const u8 sText_TotalPointsFormat[]   = _("{STR_VAR_1}");
static const u8 sText_RewardFormat[]        = _("Reward: {STR_VAR_1}");
static const u8 sText_StatusCompleted[]     = _("Status: Completed");
static const u8 sText_StatusIncomplete[]    = _("Status: Not completed");

static const struct WindowTemplate sAchievementsMenuWinTemplates[] =
{
    // tilemapTop 0, not 1 -- graphics/achievements/ui/bg_main.png's dark-navy
    // header band is genuinely y=0-15 (2 tiles), not offset a tile down like
    // this used to assume.
    [WIN_HEADER] = {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 0,
        .width = 26,
        .height = 2,
        .paletteNum = 1,
        .baseBlock = 2
    },
    // tilemapTop 3 (not 5): the art's inset body box actually starts at
    // pixel row ~23 (tile 3), not row 40 (tile 5) -- rows 0-2 here were
    // sitting on the header's border trim, not the lighter-blue body.
    // height 10 (not 8): 5 rows at 16px/row (see ACHIEVEMENTS_MENU_MAX_SHOWED)
    // instead of 4, so the tier list (BRONZE/SILVER/GOLD/DIAMOND/BOOSTS) never
    // needs to scroll.
    // paletteNum 2, not 1: four tier-medal icons (see LoadTierIcons) need 12
    // palette slots for their own colours, which don't fit in the 8 free
    // entries bank 1's icons already share with the points/lock icons
    // (src/achievement_icons.c) on top of that bank's own 8 text colours.
    // WIN_LIST gets a whole bank to itself instead, with only the 3 text
    // colours it actually needs (see sAchievementsListTextColors and
    // sAchievementsListHighlightTextColors) at low indices and the icons
    // filling the rest -- see LoadTierIcons.
    [WIN_LIST] = {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 3,
        .width = 26,
        .height = 10,
        .paletteNum = 2,
        .baseBlock = 0x36
    },
    // baseBlock follows WIN_LIST's own (0x36 + 26*10 tiles = 0x13A) now that
    // WIN_LIST is taller.
    //
    // tilemapTop 14, height 5 (not 15/4) -- the box only needs to fit two
    // 16px lines (32px), but a window sized exactly that tall leaves the text
    // hugging the bottom edge with no margin at all. Growing the window
    // upward by a tile while keeping its bottom edge fixed (14 + 5 == 15 + 4)
    // adds that margin below LINE2_Y instead of above LINE1_Y -- see
    // ACHIEVEMENTS_DESC_LINE1_Y's own comment for why it has to stay flush
    // with the window's top edge rather than shifted down too.
    [WIN_DESCRIPTION] = {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 14,
        .width = 26,
        .height = 5,
        .paletteNum = 1,
        .baseBlock = 0x13A
    },
    DUMMY_WIN_TEMPLATE
};

static const struct BgTemplate sAchievementsMenuBgTemplates[] =
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

static const u16 sAchievementsMenuText_Pal[] = INCGFX_U16("graphics/interface/option_menu_text.pal", ".gbapal");

// {background, foreground, shadow} indices into the palette above: transparent
// (so bg1 art shows through), white text, dark-gray shadow. The generic
// TEXT_COLOR_* constants past WHITE don't match this palette's actual colors
// (DARK_GRAY/LIGHT_GRAY are really orange/amber here -- see
// option_menu_text.pal), so the shadow index (this palette's 7th entry,
// 74 74 74) is written directly rather than through a misleading constant.
static const u8 sAchievementsMenuTextColors[3] = {
    TEXT_COLOR_TRANSPARENT, TEXT_COLOR_WHITE, 6
};

// WIN_LIST's own bank (see the .paletteNum comment on
// sAchievementsMenuWinTemplates) only has white and dark-gray copied in --
// LoadTierIcons copies them from sAchievementsMenuText_Pal's own entries 1
// and 6 so there's exactly one source of truth for what "white"/"dark-gray"
// mean -- so unlike sAchievementsMenuTextColors the dark-gray shadow index
// here is 2, not 6.
static const u8 sAchievementsListTextColors[3] = {
    TEXT_COLOR_TRANSPARENT, TEXT_COLOR_WHITE, 2
};

// Selected-row highlight for both WIN_LIST lists (TIER SELECT and the
// achievement list): reuses the exact orange option_menu_text.pal already
// had at raw index 2 -- the same color that used to show up by accident as
// this file's "orange text" bug (see sAchievementsMenuTextColors's comment)
// before cursorPal/cursorShadowPal got fixed to point at white/dark-gray.
// Now that CURSOR_INVISIBLE (see EnterTierSelectLevel/EnterListLevel)
// removes the arrow cursor entirely, that same orange is repurposed on
// purpose as the thing that marks the selected row instead.
static const u8 sAchievementsListHighlightTextColors[3] = {
    TEXT_COLOR_TRANSPARENT, 3, 2
};

// Four 16x16 tier-medal icons (one per ACHIEVEMENT_TIER_* row on TIER
// SELECT), each with 3 real colours of its own -- 12 slots that don't fit
// bank 1's 8 free entries (src/achievement_icons.c) alongside that bank's own
// 8 text colours and the points/lock icons already living there. Loaded into
// WIN_LIST's dedicated bank 2 instead (see sAchievementsMenuWinTemplates),
// starting right after that bank's 3 text colours -- same per-icon nibble
// remap AchievementIcons_Load uses, just against a different bank/budget, so
// it's kept local here rather than folded into that shared, single-budget
// module.
#define TIER_ICON_SIZE 16
#define TIER_ICON_BYTE_COUNT (TIER_ICON_SIZE * TIER_ICON_SIZE / 2) // 4bpp
#define TIER_ICON_FIRST_FREE_PLTT_INDEX 4 // 0 transparent, 1 white, 2 dark-gray, 3 orange highlight

static const u32 sTierBronzeIconGfx[]  = INCGFX_U32("graphics/achievements/icons/star_bronze.png", ".4bpp");
static const u16 sTierBronzeIconPal[]  = INCGFX_U16("graphics/achievements/icons/star_bronze.png", ".gbapal");
static const u32 sTierSilverIconGfx[]  = INCGFX_U32("graphics/achievements/icons/star_silver.png", ".4bpp");
static const u16 sTierSilverIconPal[]  = INCGFX_U16("graphics/achievements/icons/star_silver.png", ".gbapal");
static const u32 sTierGoldIconGfx[]    = INCGFX_U32("graphics/achievements/icons/star_gold.png", ".4bpp");
static const u16 sTierGoldIconPal[]    = INCGFX_U16("graphics/achievements/icons/star_gold.png", ".gbapal");
static const u32 sTierDiamondIconGfx[] = INCGFX_U32("graphics/achievements/icons/star_diamond.png", ".4bpp");
static const u16 sTierDiamondIconPal[] = INCGFX_U16("graphics/achievements/icons/star_diamond.png", ".gbapal");

static const struct
{
    const u32 *gfx;
    const u16 *pal;
} sTierIconSources[ACHIEVEMENT_TIER_COUNT] =
{
    [ACHIEVEMENT_TIER_BRONZE]  = { sTierBronzeIconGfx, sTierBronzeIconPal },
    [ACHIEVEMENT_TIER_SILVER]  = { sTierSilverIconGfx, sTierSilverIconPal },
    [ACHIEVEMENT_TIER_GOLD]    = { sTierGoldIconGfx, sTierGoldIconPal },
    [ACHIEVEMENT_TIER_DIAMOND] = { sTierDiamondIconGfx, sTierDiamondIconPal },
};

// Stage 7: every EWRAM_DATA static this menu used to keep around for its own
// lifetime -- the two list buffers, the two description scratch buffers, the
// tier-medal icon pixels (sTierIconPixels used to be its own top-level static
// right here), the per-screen state, and the per-tier completion counts --
// lives in one heap block instead, ~9.7 KB total. AllocZeroed'd on open
// (CB2_InitAchievementsMenu case 0) and Free'd on every exit path
// (Task_AchievementsMenuCancel, Task_TierSelect_OpenBoostMenu) -- same
// pattern src/ui_stat_editor.c's sStatEditorDataPtr already uses. This menu
// is entirely transient (nothing here needs to survive it being closed --
// contrast sReturningFromBoostShop/sAchievementsMenuReturnCallback above,
// which do), so there's no reason this sat in .ewram.sbss for the whole game
// instead of only while the screen is actually open.
//
// Declared here, right before the two functions (LoadTierIcons/BlitTierIcon)
// that need tierIconPixels, rather than up near this file's other #defines:
// this is the first point every size constant a field below depends on --
// ACHIEVEMENTS_MENU_LIST_CAPACITY, ACHIEVEMENTS_LIST_NAME_BUFFER_SIZE,
// ACHIEVEMENTS_DESC_BUFFER_SIZE, ACHIEVEMENT_TIER_COUNT, and
// TIER_ICON_BYTE_COUNT -- is actually in scope.
struct AchievementsMenuState
{
    u8 selectedTier;
    u16 listItemCount; // filtered count for the tier currently shown at LIST level
    u16 tierScrollOffset;
    u16 tierSelectedRow;
    u16 listScrollOffset;
    u16 listSelectedRow;
    // id of the row currently highlighted in whichever list is on screen
    // (a tier id / TIER_SELECT_ITEM_BOOSTS, or an achievement id) -- set
    // right before ListMenuInit and kept in sync by the moveCursorFunc
    // callbacks below. Needed because CURSOR_INVISIBLE means the row's own
    // text colour is the only thing marking it selected, and itemPrintFunc
    // (list_menu.c's ListMenuPrintEntries) has no other way to know which
    // row that is -- it's only ever given the row's own id.
    u16 highlightedId;
    // Set by PrintAchievementDescription whenever the row currently on
    // WIN_DESCRIPTION needed its 2-line auto-scroll (see that function's own
    // needsScroll) -- MainCB2 watches this alongside the printer's own
    // IsTextPrinterActiveOnWindow to know when a still-selected overlong
    // description has finished scrolling and should loop back to the top
    // after a pause, rather than leaving the last screenful on display
    // forever. Cleared by DestroyCurrentAchievementsList so a leftover TRUE
    // from LIST doesn't make MainCB2 try to restart a printer against
    // DETAIL's differently laid out WIN_DESCRIPTION content.
    bool8 descriptionScrolling;
    // Frames since the printer above went idle at the end of its scroll --
    // reset to 0 every time PrintAchievementDescription (re)starts one.
    // MainCB2 waits ACHIEVEMENTS_DESC_RESTART_DELAY frames before looping.
    u16 descriptionRestartTimer;
    // Same pair as descriptionScrolling/descriptionRestartTimer just above,
    // but for DETAIL's own overlong-description auto-scroll -- PrintDetail
    // Description prints into WIN_LIST rather than WIN_DESCRIPTION, so it
    // needs its own flag/timer to avoid colliding with (or being clobbered
    // by) LIST's. Cleared alongside it in DestroyCurrentAchievementsList,
    // and also when backing out of DETAIL back to LIST (see
    // Task_Detail_ProcessInput), since DETAIL never calls
    // DestroyCurrentAchievementsList itself (it owns no ListMenuTask).
    bool8 detailDescriptionScrolling;
    u16 detailDescriptionRestartTimer;

    // Cached once per TIER SELECT build (see BuildTierSelectListItems) so the
    // per-row itemPrintFunc doesn't re-scan every achievement on every
    // redraw.
    struct
    {
        u16 completed;
        u16 total;
    } tierCounts[ACHIEVEMENT_TIER_COUNT];

    // Backing storage for both lists this menu ever shows (tier select's up
    // to TIER_SELECT_ROW_COUNT rows, or one tier's worth of achievement
    // rows) -- see ACHIEVEMENTS_MENU_LIST_CAPACITY's own comment for why it's
    // sized off the whole catalog rather than a tight per-tier bound.
    u8 listNameBuffers[ACHIEVEMENTS_MENU_LIST_CAPACITY][ACHIEVEMENTS_LIST_NAME_BUFFER_SIZE];
    struct ListMenuItem listItems[ACHIEVEMENTS_MENU_LIST_CAPACITY];

    // Dedicated description scratch nothing else in the engine ever writes
    // to -- see the full explanation on ACHIEVEMENTS_DESC_BUFFER_SIZE's
    // #define above for why gStringVar1 isn't safe for an overlong
    // (auto-scrolling) description.
    u8 descriptionBuffer[ACHIEVEMENTS_DESC_BUFFER_SIZE];
    u8 detailDescriptionBuffer[ACHIEVEMENTS_DESC_BUFFER_SIZE];

    // Four 16x16 tier-medal icon bitmaps, remapped into WIN_LIST's own
    // palette bank -- see LoadTierIcons just below.
    u8 tierIconPixels[ACHIEVEMENT_TIER_COUNT][TIER_ICON_BYTE_COUNT];
};

// NULL whenever this menu is closed; AllocZeroed'd in CB2_InitAchievementsMenu
// (case 0) and Free'd on every exit path (Task_AchievementsMenuCancel,
// Task_TierSelect_OpenBoostMenu) -- see struct AchievementsMenuState's own
// comment.
EWRAM_DATA static struct AchievementsMenuState *sAchievementsMenuStatePtr = NULL;

static void LoadTierIcons(void)
{
    u32 nextPlttIndex = TIER_ICON_FIRST_FREE_PLTT_INDEX;
    u32 tier;

    LoadPalette(&sAchievementsMenuText_Pal[1], BG_PLTT_ID(2) + 1, PLTT_SIZEOF(1)); // white
    LoadPalette(&sAchievementsMenuText_Pal[6], BG_PLTT_ID(2) + 2, PLTT_SIZEOF(1)); // dark-gray
    LoadPalette(&sAchievementsMenuText_Pal[2], BG_PLTT_ID(2) + 3, PLTT_SIZEOF(1)); // orange (selected-row highlight)

    for (tier = 0; tier < ACHIEVEMENT_TIER_COUNT; tier++)
    {
        const u8 *src = (const u8 *)sTierIconSources[tier].gfx;
        const u16 *pal = sTierIconSources[tier].pal;
        u8 remap[16] = {0};
        u32 i, nibble;

        for (i = 0; i < TIER_ICON_BYTE_COUNT; i++)
        {
            for (nibble = 0; nibble < 2; nibble++)
            {
                u32 value = (src[i] >> (nibble * 4)) & 0xF;

                if (value == 0 || remap[value] != 0)
                    continue;
                if (nextPlttIndex > 15)
                    continue;

                remap[value] = nextPlttIndex;
                LoadPalette(&pal[value], BG_PLTT_ID(2) + nextPlttIndex, PLTT_SIZEOF(1));
                nextPlttIndex++;
            }
        }

        for (i = 0; i < TIER_ICON_BYTE_COUNT; i++)
            sAchievementsMenuStatePtr->tierIconPixels[tier][i] = remap[src[i] & 0xF] | (remap[(src[i] >> 4) & 0xF] << 4);
    }
}

static void BlitTierIcon(u8 tier, u8 windowId, u16 x, u16 y)
{
    BlitBitmapToWindow(windowId, sAchievementsMenuStatePtr->tierIconPixels[tier], x, y, TIER_ICON_SIZE, TIER_ICON_SIZE);
}

// Deduped from graphics/achievements/ui/bg_main.png (a 720x160 mockup, three
// 240x160 screens side by side) by a one-off tool, the same way
// graphics/ui_menu/background_tileset.png/.bin were -- see src/ui_stat_editor.c
// for the loading pattern this mirrors. TIER SELECT shows the "achievements"
// screen; LIST and DETAIL both show "detail" (LoadMenuBackground).
enum
{
    ACHIEVEMENTS_BG_SCREEN_MAIN,
    ACHIEVEMENTS_BG_SCREEN_DETAIL,
};

static const u32 sAchievementsScreenTiles[]   = INCBIN_U32("graphics/achievements/ui/achievements_tileset.4bpp.smol");
static const u32 sAchievementsScreenTilemap[] = INCBIN_U32("graphics/achievements/ui/achievements_tileset.bin.smolTM");
static const u16 sAchievementsScreenPal[]     = INCBIN_U16("graphics/achievements/ui/achievements_tileset.gbapal");
static const u32 sDetailScreenTiles[]         = INCBIN_U32("graphics/achievements/ui/detail_tileset.4bpp.smol");
static const u32 sDetailScreenTilemap[]       = INCBIN_U32("graphics/achievements/ui/detail_tileset.bin.smolTM");
static const u16 sDetailScreenPal[]           = INCBIN_U16("graphics/achievements/ui/detail_tileset.gbapal");

static const struct
{
    const u32 *tiles;
    const u32 *tilemap;
    const u16 *palette;
} sAchievementsMenuBgGfx[] =
{
    [ACHIEVEMENTS_BG_SCREEN_MAIN] = {
        .tiles = sAchievementsScreenTiles,
        .tilemap = sAchievementsScreenTilemap,
        .palette = sAchievementsScreenPal,
    },
    [ACHIEVEMENTS_BG_SCREEN_DETAIL] = {
        .tiles = sDetailScreenTiles,
        .tilemap = sDetailScreenTilemap,
        .palette = sDetailScreenPal,
    },
};

// bg1's WRAM tilemap buffer, allocated once at CB2 init and reused for every
// LoadMenuBackground call (only its contents change between screens) --
// freed in Task_AchievementsMenuCancel. Same pattern as
// src/ui_stat_editor.c's sBg1TilemapBuffer.
EWRAM_DATA static u8 *sAchievementsMenuBg1Tilemap = NULL;

static void MainCB2(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    // Drives the WIN_DESCRIPTION scroll printer PrintAchievementDescription
    // registers for an overlong description (see its own comment) -- that
    // printer only advances/scrolls one tick per call to this, same as every
    // other screen with a live TextPrinter (e.g. src/berry_blender.c's own
    // main callback).
    RunTextPrinters();
    // Once that printer above finishes scrolling through an overlong
    // description (IsTextPrinterActiveOnWindow going FALSE), loop it back to
    // the top after a short pause instead of leaving the last screenful on
    // display forever -- descriptionScrolling gates this to only overlong
    // descriptions actually using the printer above (see its own comment on
    // struct AchievementsMenuState), so this can't misfire against DETAIL's
    // differently laid out WIN_DESCRIPTION content or a description that
    // already fit in 2 lines (TEXT_SKIP_DRAW, no printer left active to go
    // idle).
    if (sAchievementsMenuStatePtr->descriptionScrolling
     && !IsTextPrinterActiveOnWindow(WIN_DESCRIPTION)
     && ++sAchievementsMenuStatePtr->descriptionRestartTimer >= ACHIEVEMENTS_DESC_RESTART_DELAY)
        PrintAchievementDescription(sAchievementsMenuStatePtr->highlightedId);
    // Same loop as just above, for DETAIL's own overlong-description printer
    // (PrintDetailDescription, WIN_LIST rather than WIN_DESCRIPTION) -- see
    // detailDescriptionScrolling's own comment on struct AchievementsMenuState.
    if (sAchievementsMenuStatePtr->detailDescriptionScrolling
     && !IsTextPrinterActiveOnWindow(WIN_LIST)
     && ++sAchievementsMenuStatePtr->detailDescriptionRestartTimer >= ACHIEVEMENTS_DESC_RESTART_DELAY)
        PrintDetailDescription(sAchievementsMenuStatePtr->highlightedId);
    // Flushes bg1's art tilemap, which LoadMenuBackground only *schedules* via
    // ScheduleBgCopyTilemapToVram. Windows reach VRAM on their own (
    // CopyWindowToVram copies immediately), so without this the text shows but
    // the background stays as whatever VRAM held at init. Same as
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

// Swaps bg1's art to the given screen -- called once at CB2 init (via
// EnterTierSelectLevel) and again on every TIER SELECT <-> LIST transition.
// sAchievementsMenuBg1Tilemap must already be allocated and set as bg1's
// tilemap buffer (case 1 of CB2_InitAchievementsMenu) before this runs.
static void LoadMenuBackground(u8 screen)
{
    DecompressAndCopyTileDataToVram(1, sAchievementsMenuBgGfx[screen].tiles, 0, 0, 0);
    FreeTempTileDataBuffersIfPossible();
    DecompressDataWithHeaderWram(sAchievementsMenuBgGfx[screen].tilemap, sAchievementsMenuBg1Tilemap);
    ScheduleBgCopyTilemapToVram(1);
    LoadPalette(sAchievementsMenuBgGfx[screen].palette, BG_PLTT_ID(0), PLTT_SIZE_4BPP);
}

void CB2_InitAchievementsMenu(void)
{
    u8 taskId;
    switch (gMain.state)
    {
    default:
    case 0:
        SetVBlankCallback(NULL);
        if (sReturningFromBoostShop)
        {
            gMain.savedCallback = sAchievementsMenuReturnCallback;
            sReturningFromBoostShop = FALSE;
        }
        else
        {
            sAchievementsMenuReturnCallback = gMain.savedCallback;
        }
        // AllocZeroed, not memset -- struct AchievementsMenuState now lives
        // on the heap for exactly as long as this screen is open (see its
        // own comment) rather than sitting in .ewram.sbss for the whole
        // game, so it needs allocating here instead of just clearing.
        sAchievementsMenuStatePtr = AllocZeroed(sizeof(struct AchievementsMenuState));
        gMain.state++;
        break;
    case 1:
        DmaClearLarge16(3, (void *)(VRAM), VRAM_SIZE, 0x1000);
        DmaClear32(3, OAM, OAM_SIZE);
        DmaClear16(3, PLTT, PLTT_SIZE);
        SetGpuReg(REG_OFFSET_DISPCNT, 0);
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(0, sAchievementsMenuBgTemplates, ARRAY_COUNT(sAchievementsMenuBgTemplates));
        sAchievementsMenuBg1Tilemap = Alloc(0x800);
        memset(sAchievementsMenuBg1Tilemap, 0, 0x800);
        SetBgTilemapBuffer(1, sAchievementsMenuBg1Tilemap);
        ChangeBgX(0, 0, BG_COORD_SET);
        ChangeBgY(0, 0, BG_COORD_SET);
        ChangeBgX(1, 0, BG_COORD_SET);
        ChangeBgY(1, 0, BG_COORD_SET);
        InitWindows(sAchievementsMenuWinTemplates);
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
        LoadPalette(sAchievementsMenuText_Pal, BG_PLTT_ID(1), sizeof(sAchievementsMenuText_Pal));
        // Must follow the LoadPalette above, not precede it -- this appends
        // the points icon's colours to that same palette's unused high
        // entries (src/achievement_icons.c).
        AchievementIcons_Load(1);
        // Bank 2, not bank 1 -- see LoadTierIcons and the .paletteNum comment
        // on sAchievementsMenuWinTemplates's WIN_LIST entry.
        LoadTierIcons();
        gMain.state++;
        break;
    case 4:
        PutWindowTilemap(WIN_HEADER);
        gMain.state++;
        break;
    case 5:
        PutWindowTilemap(WIN_LIST);
        PutWindowTilemap(WIN_DESCRIPTION);
        CopyBgTilemapBufferToVram(0);
        gMain.state++;
        break;
    case 6:
        // Loads the "achievements" art (LoadMenuBackground) as a side effect.
        taskId = CreateTask(Task_AchievementsMenuFadeIn, 0);
        EnterTierSelectLevel(taskId);
        gMain.state++;
        break;
    case 7:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
        SetVBlankCallback(VBlankCB);
        SetMainCallback2(MainCB2);
        return;
    }
}

static void Task_AchievementsMenuFadeIn(u8 taskId)
{
    if (!gPaletteFade.active)
        gTasks[taskId].func = Task_TierSelect_ProcessInput;
}

static void Task_AchievementsMenuCancel(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        DestroyCurrentAchievementsList(taskId);
        DestroyTask(taskId);
        FreeAllWindowBuffers();
        Free(sAchievementsMenuBg1Tilemap);
        sAchievementsMenuBg1Tilemap = NULL;
        Free(sAchievementsMenuStatePtr);
        sAchievementsMenuStatePtr = NULL;
        SetMainCallback2(gMain.savedCallback);
    }
}

// ---- TIER SELECT ---------------------------------------------------------

static void EnterTierSelectLevel(u8 taskId)
{
    struct ListMenuTemplate template = {0};
    u8 itemCount = ACHIEVEMENT_TIER_COUNT + (IsBoostShopRowVisible() ? 1 : 0);

    LoadMenuBackground(ACHIEVEMENTS_BG_SCREEN_MAIN);
    DrawTierSelectHeaderText();
    BuildTierSelectListItems();

    // TIER SELECT's own art has no third box for WIN_DESCRIPTION (see
    // DrawTierSelectHeaderText's comment) and nothing on this screen ever
    // prints to it -- but LIST level's AchievementsMenu_MoveCursorCallback
    // does, on every cursor move. Without this, backing out of LIST left
    // whatever achievement description was on screen last still sitting in
    // WIN_DESCRIPTION's pixel buffer, since nothing else ever overwrote it.
    FillWindowPixelBuffer(WIN_DESCRIPTION, PIXEL_FILL(0));
    CopyWindowToVram(WIN_DESCRIPTION, COPYWIN_GFX);

    // Must be set before ListMenuInit -- its own first draw runs before
    // TierSelect_MoveCursorCallback ever fires (see ListMenuInitInternal),
    // so without this the initially-selected row wouldn't be highlighted
    // until the first time the player actually moves the cursor.
    sAchievementsMenuStatePtr->highlightedId = sAchievementsMenuStatePtr->listItems[sAchievementsMenuStatePtr->tierScrollOffset + sAchievementsMenuStatePtr->tierSelectedRow].id;

    template.items = sAchievementsMenuStatePtr->listItems;
    template.moveCursorFunc = TierSelect_MoveCursorCallback;
    template.itemPrintFunc = TierSelect_ItemPrintCallback;
    template.totalItems = itemCount;
    template.maxShowed = ACHIEVEMENTS_MENU_MAX_SHOWED;
    template.windowId = WIN_LIST;
    template.header_X = 0;
    template.item_X = ACHIEVEMENTS_LIST_ITEM_X;
    template.cursor_X = 0;
    template.upText_Y = 1;
    template.cursorPal = 1;
    template.fillValue = 0;
    template.cursorShadowPal = 2;
    template.lettersSpacing = 0;
    template.itemVerticalPadding = 0;
    template.scrollMultiple = LIST_NO_MULTIPLE_SCROLL;
    template.fontId = FONT_NORMAL;
    // No arrow cursor -- the selected row highlights its own text instead
    // (see TierSelect_ItemPrintCallback/sAchievementsListHighlightTextColors).
    template.cursorKind = CURSOR_INVISIBLE;

    gTasks[taskId].tListTaskId = ListMenuInit(&template, sAchievementsMenuStatePtr->tierScrollOffset, sAchievementsMenuStatePtr->tierSelectedRow);
    gTasks[taskId].tScrollArrowTaskId = AddScrollIndicatorArrowPairParameterized(
        SCROLL_ARROW_UP, ACHIEVEMENTS_ARROW_X, ACHIEVEMENTS_ARROW_TOP_Y, ACHIEVEMENTS_ARROW_BOTTOM_Y,
        // Clamped like EnterListLevel's own scroll arrows below -- now that
        // ACHIEVEMENTS_MENU_MAX_SHOWED is 5, itemCount is 4 whenever the
        // BOOSTS row is hidden, and a bare subtraction would go negative
        // (see EnterListLevel's own comment on this exact bug).
        (itemCount > ACHIEVEMENTS_MENU_MAX_SHOWED) ? (itemCount - ACHIEVEMENTS_MENU_MAX_SHOWED) : 0,
        TAG_ACHIEVEMENTS_SCROLL_ARROWS, TAG_ACHIEVEMENTS_SCROLL_ARROWS,
        &sAchievementsMenuStatePtr->tierScrollOffset);
}

static void Task_TierSelect_ProcessInput(u8 taskId)
{
    u16 prevScrollOffset = sAchievementsMenuStatePtr->tierScrollOffset;
    u16 prevSelectedRow = sAchievementsMenuStatePtr->tierSelectedRow;
    u8 prevRowY = ListMenuGetYCoordForPrintingArrowCursor(gTasks[taskId].tListTaskId);
    s32 itemId = ListMenu_ProcessInput(gTasks[taskId].tListTaskId);
    ListMenuGetScrollAndRow(gTasks[taskId].tListTaskId, &sAchievementsMenuStatePtr->tierScrollOffset, &sAchievementsMenuStatePtr->tierSelectedRow);

    // CURSOR_INVISIBLE means moving the cursor within the same page (no
    // scroll) only updates ListMenu's internal state -- list_menu.c's own
    // ListMenuChangeSelectionFull skips its usual ListMenuPrintEntries call
    // whenever it doesn't have to scroll, since normally only the arrow
    // cursor's position needs to move, not the row text. Now that the
    // selected row's own text colour is what marks it (see
    // TierSelect_ItemPrintCallback), that text needs an explicit repaint on
    // every selection change, not just the scrolling ones.
    //
    // Scrolling still goes through the full RedrawListMenu -- every visible
    // row's *content* changes there, not just which one is highlighted. A
    // same-page move only changes two rows' colour, so RepaintListRow patches
    // just those in place instead (see its own comment for why that also
    // fixes the flicker a full redraw caused here).
    if (prevScrollOffset != sAchievementsMenuStatePtr->tierScrollOffset)
    {
        RedrawListMenu(gTasks[taskId].tListTaskId);
    }
    else if (prevSelectedRow != sAchievementsMenuStatePtr->tierSelectedRow)
    {
        u8 newRowY = ListMenuGetYCoordForPrintingArrowCursor(gTasks[taskId].tListTaskId);

        RepaintListRow(TierSelect_DrawRow, prevScrollOffset + prevSelectedRow, prevRowY);
        RepaintListRow(TierSelect_DrawRow, prevScrollOffset + sAchievementsMenuStatePtr->tierSelectedRow, newRowY);
        CopyWindowToVram(WIN_LIST, COPYWIN_GFX);
    }

    switch (itemId)
    {
    case LIST_NOTHING_CHOSEN:
        break;
    case LIST_CANCEL:
        PlaySE(SE_SELECT);
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        gTasks[taskId].func = Task_AchievementsMenuCancel;
        break;
    case TIER_SELECT_ITEM_BOOSTS:
        PlaySE(SE_SELECT);
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        gTasks[taskId].func = Task_TierSelect_OpenBoostMenu;
        break;
    default:
        PlaySE(SE_SELECT);
        sAchievementsMenuStatePtr->selectedTier = itemId;
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        gTasks[taskId].func = Task_TierSelect_ToListLevel;
        break;
    }
}

// Fades TIER SELECT out, then loads LIST level's own background/palette and
// fades back in -- see this file's header comment for why the swap has to
// happen while the screen is black.
static void Task_TierSelect_ToListLevel(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        sAchievementsMenuStatePtr->listScrollOffset = 0;
        sAchievementsMenuStatePtr->listSelectedRow = 0;
        DestroyCurrentAchievementsList(taskId);

        // Disabled around the swap: LoadPalette (called by EnterListLevel's
        // own LoadMenuBackground) writes straight into gPlttBufferFaded, not
        // just gPlttBufferUnfaded (see src/palette.c) -- gPlttBufferFaded is
        // also exactly what VBlankCB's TransferPlttBuffer DMAs to hardware
        // PLTT every frame. With no fade active (the guard above just
        // confirmed it), that buffer currently holds fully-faded-to-black
        // values from the fade-out that just finished; LoadPalette
        // overwrites it with LIST's full-brightness colours, and if a vblank
        // interrupt lands before BeginNormalPaletteFade below re-blends it
        // back to black, TransferPlttBuffer copies those full-brightness
        // values to the screen for a frame -- exactly the flash this is
        // fixing. Same idiom src/party_menu.c, src/pokedex.c etc. use around
        // their own mid-transition palette loads.
        gPaletteFade.bufferTransferDisabled = TRUE;
        EnterListLevel(taskId, sAchievementsMenuStatePtr->selectedTier);
        BlendPalettes(PALETTES_ALL, 16, RGB_BLACK);
        gPaletteFade.bufferTransferDisabled = FALSE;

        BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
        gTasks[taskId].func = Task_List_ProcessInput;
    }
}

// Fades out, tears down this screen exactly like Task_AchievementsMenuCancel
// does, then jumps straight into the boost shop (src/achievement_boost_menu.c)
// instead of gMain.savedCallback -- with gMain.savedCallback repointed at
// CB2_InitAchievementsMenu first, so the shop's own [B] Back re-enters here
// at a fresh TIER SELECT rather than returning to the field/debug menu.
static void Task_TierSelect_OpenBoostMenu(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        DestroyCurrentAchievementsList(taskId);
        DestroyTask(taskId);
        FreeAllWindowBuffers();
        Free(sAchievementsMenuBg1Tilemap);
        sAchievementsMenuBg1Tilemap = NULL;
        Free(sAchievementsMenuStatePtr);
        sAchievementsMenuStatePtr = NULL;
        sReturningFromBoostShop = TRUE;
        gMain.savedCallback = CB2_InitAchievementsMenu;
        SetMainCallback2(CB2_InitAchievementBoostMenu);
    }
}

static void TierSelect_MoveCursorCallback(s32 itemIndex, bool8 onInit, struct ListMenu *list)
{
    if (!onInit)
        PlaySE(SE_SELECT);
    sAchievementsMenuStatePtr->highlightedId = itemIndex;
    // A same-page cursor move never reprints row text on its own (see
    // list_menu.c's ListMenuChangeSelectionFull case 1 -- with cursorKind
    // CURSOR_INVISIBLE, ListMenuDrawCursor is the only thing it calls, and
    // that's a no-op for this cursor kind), so without this the orange
    // highlight would only ever show on whichever row happened to be
    // selected when the list was first drawn.
    //
    // This used to be a ListMenuRepaintItems(list) call, but that's the same
    // FillWindowPixelBuffer-then-redraw-every-visible-row RedrawListMenu does
    // -- exactly the flicker RepaintListRow's own comment describes, and it
    // ran on *every* selection change (including plain up/down, not just
    // scrolls), since list_menu.c calls this callback before returning
    // control here. Task_TierSelect_ProcessInput's own RepaintListRow/
    // RedrawListMenu calls, right after ListMenu_ProcessInput returns, are
    // what actually need to run this row's repaint -- highlightedId is
    // already updated above by the time they do, so removing the call here
    // costs nothing but the flicker.
}

// Icon stays at a fixed column (ACHIEVEMENTS_TIER_ICON_X) so every tier's
// icon lines up regardless of name length, but the count text next to it is
// right-aligned to the box's edge (ACHIEVEMENTS_POINTS_RIGHT_X) rather than
// flowing left-to-right off the icon -- matches AchievementsMenu_DrawRow's
// own points column below, and reads better than a ragged left edge sitting
// in the middle of the row.
static void TierSelect_DrawRow(u8 windowId, u32 tier, u8 y, const u8 *colors)
{
    u8 *ptr;
    s32 width;

    // The "BOOSTS" row (id TIER_SELECT_ITEM_BOOSTS) isn't a tier --
    // sAchievementsMenuStatePtr->tierCounts[] has no entry for it, and it has no medal icon or count
    // column of its own.
    if (tier >= ACHIEVEMENT_TIER_COUNT)
        return;

    BlitTierIcon(tier, windowId, ACHIEVEMENTS_TIER_ICON_X, ACHIEVEMENT_ICON_Y(y));

    ptr = ConvertIntToDecimalStringN(gStringVar4, sAchievementsMenuStatePtr->tierCounts[tier].completed, STR_CONV_MODE_LEFT_ALIGN, 3);
    ptr = StringCopy(ptr, sText_TierCountSeparator);
    ConvertIntToDecimalStringN(ptr, sAchievementsMenuStatePtr->tierCounts[tier].total, STR_CONV_MODE_LEFT_ALIGN, 3);

    width = GetStringWidth(FONT_NORMAL, gStringVar4, 0);
    AddTextPrinterParameterized3(windowId, FONT_NORMAL, ACHIEVEMENTS_TIER_COUNT_RIGHT_X - width, y, colors, TEXT_SKIP_DRAW, gStringVar4);
}

static void TierSelect_ItemPrintCallback(u8 windowId, u32 tier, u8 y)
{
    bool8 selected = (tier == sAchievementsMenuStatePtr->highlightedId);
    const u8 *colors = selected ? sAchievementsListHighlightTextColors : sAchievementsListTextColors;

    // Recolours the row name ListMenuPrint is about to draw right after this
    // returns (see list_menu.c's ListMenuPrintEntries) -- has to happen
    // before TierSelect_DrawRow's own BOOSTS-row early return, since that row
    // needs the same highlight treatment despite having no icon/count of its
    // own.
    if (selected)
        ListMenuOverrideSetColors(colors[1], colors[0], colors[2]);

    TierSelect_DrawRow(windowId, tier, y, colors);
}

static void BuildTierSelectListItems(void)
{
    u32 tier, id;

    for (tier = 0; tier < ACHIEVEMENT_TIER_COUNT; tier++)
    {
        sAchievementsMenuStatePtr->tierCounts[tier].completed = 0;
        sAchievementsMenuStatePtr->tierCounts[tier].total = 0;
    }

    for (id = ACHIEVEMENT_NONE + 1; id < ACHIEVEMENTS_COUNT; id++)
    {
        tier = Achievement_GetInfo(id)->tier;
        sAchievementsMenuStatePtr->tierCounts[tier].total++;
        if (Achievement_IsCompleted(id))
            sAchievementsMenuStatePtr->tierCounts[tier].completed++;
    }

    for (tier = 0; tier < ACHIEVEMENT_TIER_COUNT; tier++)
    {
        u8 *buffer = sAchievementsMenuStatePtr->listNameBuffers[tier];

        StringCopy(buffer, sTierNames[tier]);
        sAchievementsMenuStatePtr->listItems[tier].name = buffer;
        sAchievementsMenuStatePtr->listItems[tier].id = tier;
    }

    if (IsBoostShopRowVisible())
    {
        u8 *buffer = sAchievementsMenuStatePtr->listNameBuffers[ACHIEVEMENT_TIER_COUNT];

        StringCopy(buffer, sText_BoostsMenuRowLabel);
        sAchievementsMenuStatePtr->listItems[ACHIEVEMENT_TIER_COUNT].name = buffer;
        sAchievementsMenuStatePtr->listItems[ACHIEVEMENT_TIER_COUNT].id = TIER_SELECT_ITEM_BOOSTS;
    }
}

// OFF hides the shop (not just the toggle), so this checks both -- unlocked
// but disabled must not show the row.
static bool8 IsBoostShopRowVisible(void)
{
    return Achievement_BoostsUnlocked() && Achievement_BoostsEnabled();
}

// TIER SELECT's own header: title, then the points summary directly after it,
// then the [B] BACK hint. Unlike DrawHeaderText below (shared by LIST/
// DETAIL, which keep their own WIN_DESCRIPTION), TIER SELECT's art has no
// third box for the points summary to live in (see this file's header
// comment) -- it has to fit on the header's one line instead. The gap
// between the title and the hint is measured, not assumed, and the points
// text narrows its font (GetFontIdToFit, the same fallback list_menu.c uses
// for its own rows) if FONT_NORMAL wouldn't fit what's left of it.
static void DrawTierSelectHeaderText(void)
{
    // FONT_NARROW, not FONT_NORMAL: this line has three things to fit
    // (title, points summary, [B] BACK hint) and the title was eating enough
    // width that the points summary routinely had to fall back through
    // GetFontIdToFit below anyway -- narrowing the title directly leaves it
    // more room without shrinking the points text as far.
    s32 titleX = 2;
    s32 hintX = GetStringRightAlignXOffset(FONT_NARROW, sText_ControlHint, 198);
    s32 pointsIconX = titleX + GetStringWidth(FONT_NARROW, sText_AchievementsTitle, 0) + 4;
    s32 pointsTextX = pointsIconX + ACHIEVEMENT_ICON_SIZE + 2;
    s32 availWidth = (hintX - 8) - pointsTextX;
    u32 fontId;

    if (availWidth < 0)
        availWidth = 0;

    FillWindowPixelBuffer(WIN_HEADER, PIXEL_FILL(0));
    AddTextPrinterParameterized3(WIN_HEADER, FONT_NARROW, titleX, 0, sAchievementsMenuTextColors, TEXT_SKIP_DRAW, sText_AchievementsTitle);
    AddTextPrinterParameterized3(WIN_HEADER, FONT_NARROW, hintX, 0, sAchievementsMenuTextColors, TEXT_SKIP_DRAW, sText_ControlHint);

    // {available points}/{total points earned} -- a fraction reads faster
    // and takes less horizontal space than the old "{total} ({available}
    // free)" wording did, which matters here since this line also has to
    // fit the title and the [B] BACK hint. Boosts unlock is what makes
    // "available" a meaningful concept at all -- until then nothing has
    // ever been spent, so available == total and the fraction is just
    // "{total}/{total}" noise; show the bare total instead.
    if (Achievement_BoostsUnlocked())
    {
        ConvertIntToDecimalStringN(gStringVar1, Achievement_GetAvailablePoints(), STR_CONV_MODE_LEFT_ALIGN, 6);
        ConvertIntToDecimalStringN(gStringVar2, Achievement_GetTotalPoints(), STR_CONV_MODE_LEFT_ALIGN, 6);
        StringExpandPlaceholders(gStringVar4, sText_PointsSummaryFormat);
    }
    else
    {
        ConvertIntToDecimalStringN(gStringVar1, Achievement_GetTotalPoints(), STR_CONV_MODE_LEFT_ALIGN, 6);
        StringExpandPlaceholders(gStringVar4, sText_TotalPointsFormat);
    }
    fontId = GetFontIdToFit(gStringVar4, FONT_NORMAL, 0, availWidth);

    // Not ACHIEVEMENT_ICON_Y(0) -- that macro's -1 inset assumes text one
    // pixel below the icon's own top, which would underflow a u16 at the
    // header's y=0 (see the header's tilemapTop comment on why text sits at
    // 0 here, not the usual 1).
    AchievementIcons_Blit(ACHIEVEMENT_ICON_POINTS, WIN_HEADER, pointsIconX, 0);
    AddTextPrinterParameterized3(WIN_HEADER, fontId, pointsTextX, 0, sAchievementsMenuTextColors, TEXT_SKIP_DRAW, gStringVar4);

    CopyWindowToVram(WIN_HEADER, COPYWIN_FULL);
}

// ---- ACHIEVEMENT LIST -----------------------------------------------------

static void EnterListLevel(u8 taskId, u8 tier)
{
    struct ListMenuTemplate template = {0};

    sAchievementsMenuStatePtr->selectedTier = tier;

    LoadMenuBackground(ACHIEVEMENTS_BG_SCREEN_DETAIL);
    DrawHeaderText(sTierNames[tier]);
    BuildAchievementListItems(tier);

    // See the identical comment in EnterTierSelectLevel -- must be set
    // before ListMenuInit so the initially-selected row is highlighted from
    // the very first draw.
    sAchievementsMenuStatePtr->highlightedId = sAchievementsMenuStatePtr->listItems[sAchievementsMenuStatePtr->listScrollOffset + sAchievementsMenuStatePtr->listSelectedRow].id;

    template.items = sAchievementsMenuStatePtr->listItems;
    template.moveCursorFunc = AchievementsMenu_MoveCursorCallback;
    template.itemPrintFunc = AchievementsMenu_ItemPrintCallback;
    template.totalItems = sAchievementsMenuStatePtr->listItemCount;
    template.maxShowed = ACHIEVEMENTS_MENU_MAX_SHOWED;
    template.windowId = WIN_LIST;
    template.header_X = 0;
    template.item_X = ACHIEVEMENTS_LIST_ITEM_X;
    template.cursor_X = 0;
    template.upText_Y = 1;
    // 1/2, not 2/3 -- WIN_LIST is on its own palette bank now (see the
    // .paletteNum comment on sAchievementsMenuWinTemplates), where white and
    // dark-gray sit at indices 1 and 2, not this file's usual 1/6. Indices
    // 2/3 were never actually black/dark-gray in the old shared bank either
    // -- see sAchievementsMenuTextColors's comment -- this just fixes the
    // list's own cursor/row text to match everything else instead of
    // rendering orange.
    template.cursorPal = 1;
    template.fillValue = 0;
    template.cursorShadowPal = 2;
    template.lettersSpacing = 0;
    template.itemVerticalPadding = 0;
    template.scrollMultiple = LIST_NO_MULTIPLE_SCROLL;
    template.fontId = FONT_NORMAL;
    // No arrow cursor -- matches TIER SELECT (see its own cursorKind
    // comment); also avoids the arrow overlapping row text now that
    // ACHIEVEMENTS_LIST_ITEM_X sits closer to the cursor's old column.
    template.cursorKind = CURSOR_INVISIBLE;

    gTasks[taskId].tListTaskId = ListMenuInit(&template, sAchievementsMenuStatePtr->listScrollOffset, sAchievementsMenuStatePtr->listSelectedRow);
    gTasks[taskId].tScrollArrowTaskId = AddScrollIndicatorArrowPairParameterized(
        SCROLL_ARROW_UP, ACHIEVEMENTS_ARROW_X, ACHIEVEMENTS_ARROW_TOP_Y, ACHIEVEMENTS_ARROW_BOTTOM_Y,
        // Clamped, not a bare subtraction: a tier can have fewer than
        // ACHIEVEMENTS_MENU_MAX_SHOWED achievements, so this can go
        // negative. A negative threshold truncates into a huge u16 when
        // stored (struct ScrollIndicatorPair.fullyDownThreshold,
        // src/list_menu.c:29) that the real scroll offset can never match,
        // leaving the down arrow stuck visible with nothing left to scroll
        // to (same issue fixed for the boost list in
        // src/achievement_boost_menu.c).
        (sAchievementsMenuStatePtr->listItemCount > ACHIEVEMENTS_MENU_MAX_SHOWED) ? (sAchievementsMenuStatePtr->listItemCount - ACHIEVEMENTS_MENU_MAX_SHOWED) : 0,
        TAG_ACHIEVEMENTS_SCROLL_ARROWS, TAG_ACHIEVEMENTS_SCROLL_ARROWS,
        &sAchievementsMenuStatePtr->listScrollOffset);
}

static void Task_List_ProcessInput(u8 taskId)
{
    u16 prevScrollOffset = sAchievementsMenuStatePtr->listScrollOffset;
    u16 prevSelectedRow = sAchievementsMenuStatePtr->listSelectedRow;
    u8 prevRowY = ListMenuGetYCoordForPrintingArrowCursor(gTasks[taskId].tListTaskId);
    s32 itemId = ListMenu_ProcessInput(gTasks[taskId].tListTaskId);
    ListMenuGetScrollAndRow(gTasks[taskId].tListTaskId, &sAchievementsMenuStatePtr->listScrollOffset, &sAchievementsMenuStatePtr->listSelectedRow);

    // See the identical comment in Task_TierSelect_ProcessInput.
    if (prevScrollOffset != sAchievementsMenuStatePtr->listScrollOffset)
    {
        RedrawListMenu(gTasks[taskId].tListTaskId);
    }
    else if (prevSelectedRow != sAchievementsMenuStatePtr->listSelectedRow)
    {
        u8 newRowY = ListMenuGetYCoordForPrintingArrowCursor(gTasks[taskId].tListTaskId);

        RepaintListRow(AchievementsMenu_DrawRow, prevScrollOffset + prevSelectedRow, prevRowY);
        RepaintListRow(AchievementsMenu_DrawRow, prevScrollOffset + sAchievementsMenuStatePtr->listSelectedRow, newRowY);
        CopyWindowToVram(WIN_LIST, COPYWIN_GFX);
    }

    // Bug (found via playtesting): an overlong description's first
    // screenful sometimes rendered in this menu's orange row-highlight
    // colour instead of plain white. GenerateFontHalfRowLookupTable
    // (src/text.c) builds the glyph colour->pixel table every active
    // printer draws through -- one *global* table, shared by every window,
    // regenerated only when a printer is (re)registered or hits an
    // in-string colour escape, never per glyph. Moving the cursor onto a row
    // whose description needs the 2-line auto-scroll runs, in order, within
    // this very call: ListMenu_ProcessInput above (via moveCursorFunc ->
    // PrintAchievementDescription) registers WIN_DESCRIPTION's own overlong-
    // description printer, regenerating that table for its white -- then the
    // row repaint just above (RepaintListRow's second call is always the
    // newly selected row, in this menu's orange) regenerates the very same
    // table again for its own orange, as a side effect of its own instant
    // TEXT_SKIP_DRAW print. WIN_DESCRIPTION's printer hasn't drawn a single
    // glyph of its own yet at that point -- RunTextPrinters only runs once
    // MainCB2's RunTasks (this function) returns -- so its first screenful
    // came out in whatever colour was left behind above.
    //
    // Regenerating the table for WIN_DESCRIPTION's own colour here, every
    // time this function runs while a description is still scrolling, keeps
    // it the last word regardless of which row/colour got repainted last
    // above (or whether either branch above even ran this frame) --
    // GenerateFontHalfRowLookupTable's own unchanged-colour early-out makes
    // the extra call a no-op on every frame this didn't need to fix anything.
    if (sAchievementsMenuStatePtr->descriptionScrolling)
    {
        union TextColor color;

        color.background = sAchievementsMenuTextColors[0];
        color.foreground = sAchievementsMenuTextColors[1];
        color.shadow = sAchievementsMenuTextColors[2];
        color.accent = sAchievementsMenuTextColors[0];
        GenerateFontHalfRowLookupTable(color);
    }

    switch (itemId)
    {
    case LIST_NOTHING_CHOSEN:
        break;
    case LIST_CANCEL:
        PlaySE(SE_SELECT);
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        gTasks[taskId].func = Task_List_ToTierSelectLevel;
        break;
    default:
        PlaySE(SE_SELECT);
        DestroyCurrentAchievementsList(taskId);
        EnterDetailLevel(taskId, itemId);
        gTasks[taskId].func = Task_Detail_ProcessInput;
        break;
    }
}

// See the identical comment on Task_TierSelect_ToListLevel -- backing out of
// LIST swaps the same background/palette pair in the opposite direction, so
// it needs the same bufferTransferDisabled guard around the swap.
static void Task_List_ToTierSelectLevel(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        DestroyCurrentAchievementsList(taskId);

        gPaletteFade.bufferTransferDisabled = TRUE;
        EnterTierSelectLevel(taskId);
        BlendPalettes(PALETTES_ALL, 16, RGB_BLACK);
        gPaletteFade.bufferTransferDisabled = FALSE;

        BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
        gTasks[taskId].func = Task_TierSelect_ProcessInput;
    }
}

static void AchievementsMenu_MoveCursorCallback(s32 itemIndex, bool8 onInit, struct ListMenu *list)
{
    if (!onInit)
        PlaySE(SE_SELECT);
    sAchievementsMenuStatePtr->highlightedId = itemIndex;
    // See the identical comment in TierSelect_MoveCursorCallback -- row
    // repaint is Task_List_ProcessInput's own RepaintListRow/RedrawListMenu
    // calls' job, right after ListMenu_ProcessInput (and this callback)
    // return, not this callback's.
    PrintAchievementDescription(itemIndex);
}

static void AchievementsMenu_DrawRow(u8 windowId, u32 achievementId, u8 y, const u8 *colors)
{
    s32 width;

    ConvertIntToDecimalStringN(gStringVar1, Achievement_GetInfo(achievementId)->points, STR_CONV_MODE_LEFT_ALIGN, 5);
    width = GetStringWidth(FONT_NORMAL, gStringVar1, 0);

    AddTextPrinterParameterized3(windowId, FONT_NORMAL, ACHIEVEMENTS_POINTS_RIGHT_X - width, y, colors, TEXT_SKIP_DRAW, gStringVar1);
}

static void AchievementsMenu_ItemPrintCallback(u8 windowId, u32 achievementId, u8 y)
{
    bool8 selected = (achievementId == sAchievementsMenuStatePtr->highlightedId);
    const u8 *colors = selected ? sAchievementsListHighlightTextColors : sAchievementsListTextColors;

    // See the identical comment in TierSelect_ItemPrintCallback.
    if (selected)
        ListMenuOverrideSetColors(colors[1], colors[0], colors[2]);

    AchievementsMenu_DrawRow(windowId, achievementId, y, colors);
}

// Builds the tier-filtered item list (skips ACHIEVEMENT_NONE and any ID
// outside this tier) and bakes the completion checkbox into each row's label
// text. Hidden achievements render their name as "???" until completed.
// Achievement completion can't change while this menu is
// open, so this only needs to run once, at entry to the tier, rather than
// being recomputed per redraw.
static void BuildAchievementListItems(u8 tier)
{
    u32 id, index = 0;

    for (id = ACHIEVEMENT_NONE + 1; id < ACHIEVEMENTS_COUNT; id++)
    {
        const struct Achievement *info = Achievement_GetInfo(id);
        u8 *buffer;
        bool8 completed;

        if (info->tier != tier)
            continue;

        // ACHIEVEMENTS_MENU_LIST_CAPACITY is derived from ACHIEVEMENTS_COUNT
        // (see its own comment above) so this can't undersize as the catalog
        // grows, but keep the fail-safe truncate rather than trust that
        // invariant blindly.
        if (index >= ACHIEVEMENTS_MENU_LIST_CAPACITY)
            break;

        completed = Achievement_IsCompleted(id);
        buffer = sAchievementsMenuStatePtr->listNameBuffers[index];

        StringCopy(buffer, completed ? sText_CompletedPrefix : sText_IncompletePrefix);
        StringAppend(buffer, (info->hidden && !completed) ? sText_HiddenName : info->name);

        sAchievementsMenuStatePtr->listItems[index].name = buffer;
        sAchievementsMenuStatePtr->listItems[index].id = id;
        index++;
    }

    sAchievementsMenuStatePtr->listItemCount = index;
}

// Bug (reported after initial delivery): descriptions were printed raw, with
// no width limit. AddTextPrinterParameterized doesn't clip or wrap on its
// own, so anything wider than the window kept drawing past its right edge --
// which (given how the window's tile buffer is laid out) bled into the tile
// memory of the row below, showing up as leftover/overlapping text the next
// time that row was drawn. StripLineBreaks + BreakStringAutomatic is the
// same fix src/achievement_popup.c already uses for achievement descriptions
// (see its ACHIEVEMENT_POPUP_DESC_MAX_WIDTH) -- strip any pre-existing manual
// breaks so BreakStringAutomatic computes clean wrapping from scratch, then
// let it insert real line breaks.
//
// True for CHAR_PROMPT_SCROLL specifically (not CHAR_NEWLINE, which
// StringHasManualBreaks/CountLineBreaks in src/line_break.c both treat the
// same as a scroll prompt) -- lets PrintAchievementDescription below tell a
// description that fits WIN_DESCRIPTION's 2 lines apart from one that
// doesn't, since only the latter has one at all (see BuildNewString's own
// maxLines check).
static bool8 StringHasScrollPrompt(const u8 *str)
{
    u32 i;

    for (i = 0; str[i] != EOS; i++)
    {
        if (str[i] == CHAR_PROMPT_SCROLL)
            return TRUE;
    }
    return FALSE;
}

// Second bug (also reported after initial delivery): fixing the above just
// moved the overflow -- WIN_DESCRIPTION only ever shows 2 lines, so a
// description needing a 3rd line or more still had nowhere to go. Passing
// SHOW_SCROLL_PROMPT (rather than HIDE_SCROLL_PROMPT above) has
// BreakStringAutomatic insert a CHAR_PROMPT_SCROLL instead of a CHAR_NEWLINE
// at that point -- the exact control character every other multi-line
// message box in the game already uses to pause and scroll its window up by
// one line (src/text.c's RENDER_STATE_SCROLL_START/SCROLL, driven by
// RunTextPrinters -- see this file's own MainCB2). gTextFlags.autoScroll
// makes that pause resolve on its own after NUM_FRAMES_AUTO_SCROLL_DELAY
// frames instead of waiting on a button press, since nothing else about this
// box expects the player to press A/B to advance it -- Up/Down already move
// to a different row entirely.
//
// Only descriptions that actually need it pay for this: BreakStringAutomatic
// never inserts CHAR_PROMPT_SCROLL for text that already fits in 2 lines (see
// its own numLines > maxLines check), so those still print instantly
// (TEXT_SKIP_DRAW, same as before) rather than paying a letter-by-letter
// typing delay on every cursor move for the common case.
static void PrintAchievementDescription(s32 achievementId)
{
    FillWindowPixelBuffer(WIN_DESCRIPTION, PIXEL_FILL(0));
    // Cancels whatever printer the previously-selected row's description
    // registered below -- without this, moving off a long description before
    // its scroll finishes leaves that printer still ticking away against a
    // window this FillWindowPixelBuffer just cleared for the new row.
    DeactivateSingleTextPrinter(WIN_DESCRIPTION, WINDOW_TEXT_PRINTER);

    if (achievementId >= ACHIEVEMENT_NONE + 1 && achievementId < ACHIEVEMENTS_COUNT)
    {
        const struct Achievement *info = Achievement_GetInfo(achievementId);
        bool8 masked = info->hidden && !Achievement_IsCompleted(achievementId);
        bool8 needsScroll;

        // sAchievementsMenuStatePtr->descriptionBuffer, not gStringVar1 -- see that
        // buffer's own comment for why an overlong (needsScroll) description
        // can't be built in a buffer anything else in the engine might write
        // to while this printer is still reading it.
        StringCopy(sAchievementsMenuStatePtr->descriptionBuffer, masked ? sText_HiddenDescription : info->description);
        StripLineBreaks(sAchievementsMenuStatePtr->descriptionBuffer);
        BreakStringAutomatic(sAchievementsMenuStatePtr->descriptionBuffer, ACHIEVEMENTS_DESC_MAX_WIDTH, 2, FONT_NORMAL, SHOW_SCROLL_PROMPT);
        needsScroll = StringHasScrollPrompt(sAchievementsMenuStatePtr->descriptionBuffer);

        gTextFlags.autoScroll = needsScroll;
        AddTextPrinterParameterized3(WIN_DESCRIPTION, FONT_NORMAL, 8, ACHIEVEMENTS_DESC_LINE1_Y, sAchievementsMenuTextColors,
            needsScroll ? GetPlayerTextSpeedDelay() : TEXT_SKIP_DRAW, sAchievementsMenuStatePtr->descriptionBuffer);

        // See MainCB2's own use of these -- (re)starting this printer always
        // resets the idle timer, whether this is the first print for a newly
        // selected row or a loop back to the top of one already mid-scroll.
        sAchievementsMenuStatePtr->descriptionScrolling = needsScroll;
        sAchievementsMenuStatePtr->descriptionRestartTimer = 0;
    }
    CopyWindowToVram(WIN_DESCRIPTION, COPYWIN_GFX);
}

// ---- DETAIL ----------------------------------------------------------------

// Mirrors PrintAchievementDescription just above -- same overlong-
// description auto-scroll-then-loop treatment (see its own comment and
// MainCB2's use of detailDescriptionScrolling), applied to DETAIL's own
// name+description box (WIN_LIST) instead of LIST's (WIN_DESCRIPTION). Split
// out of EnterDetailLevel so MainCB2's restart loop can re-run just this
// part rather than redrawing the reward/status lines below it that never
// change while the row stays selected.
static void PrintDetailDescription(s32 achievementId)
{
    const struct Achievement *info = Achievement_GetInfo(achievementId);
    bool8 masked = info->hidden && !Achievement_IsCompleted(achievementId);
    bool8 needsScroll;

    FillWindowPixelBuffer(WIN_LIST, PIXEL_FILL(0));
    // Cancels whatever printer the previous restart/selection registered --
    // see the identical comment in PrintAchievementDescription.
    DeactivateSingleTextPrinter(WIN_LIST, WINDOW_TEXT_PRINTER);

    // Bug (reported after initial delivery): DETAIL's name/description sat at
    // x=8 while WIN_LIST's own box art is the exact same art the achievement
    // list draws its rows into at ACHIEVEMENTS_LIST_ITEM_X (LIST and DETAIL
    // share one bg1 screen -- see this file's header comment) -- the extra
    // 8px pushed long names/descriptions past that box's right edge instead
    // of the window's. Matching the list rows' own left margin fixes it.
    //
    // The name prints in the same orange used to highlight the selected row
    // elsewhere in this menu (sAchievementsListHighlightTextColors, not the
    // plain white sAchievementsListTextColors the description below still
    // uses) -- reported after initial delivery as hard to tell apart from
    // the description at a glance with both in plain white; this box has no
    // other visual cue (font size, box divider, etc.) marking which line is
    // the title.
    AddTextPrinterParameterized3(WIN_LIST, FONT_NORMAL, ACHIEVEMENTS_LIST_ITEM_X, 1, sAchievementsListHighlightTextColors, TEXT_SKIP_DRAW, masked ? sText_HiddenName : info->name);

    // sAchievementsMenuStatePtr->detailDescriptionBuffer, not gStringVar1 -- see that
    // buffer's own comment. Matters here too: EnterDetailLevel reuses
    // gStringVar1 for the reward figure immediately after this call returns,
    // which would clobber an in-progress needsScroll printer built on it
    // before a single frame had even drawn.
    StringCopy(sAchievementsMenuStatePtr->detailDescriptionBuffer, masked ? sText_HiddenDescription : info->description);
    StripLineBreaks(sAchievementsMenuStatePtr->detailDescriptionBuffer);
    // ACHIEVEMENTS_DETAIL_DESC_MAX_WIDTH, not ACHIEVEMENTS_DESC_MAX_WIDTH --
    // see its own comment; WIN_LIST's box is narrower than WIN_DESCRIPTION's.
    // SHOW_SCROLL_PROMPT/ACHIEVEMENTS_DETAIL_DESC_LINES, not the
    // HIDE_SCROLL_PROMPT/3 this used to pass -- same reasoning as
    // PrintAchievementDescription's own SHOW_SCROLL_PROMPT: a description
    // that needs more than ACHIEVEMENTS_DETAIL_DESC_LINES lines now scrolls
    // through the rest instead of silently running past WIN_LIST's own
    // bottom edge.
    BreakStringAutomatic(sAchievementsMenuStatePtr->detailDescriptionBuffer, ACHIEVEMENTS_DETAIL_DESC_MAX_WIDTH, ACHIEVEMENTS_DETAIL_DESC_LINES, FONT_NORMAL, SHOW_SCROLL_PROMPT);
    needsScroll = StringHasScrollPrompt(sAchievementsMenuStatePtr->detailDescriptionBuffer);

    gTextFlags.autoScroll = needsScroll;
    AddTextPrinterParameterized3(WIN_LIST, FONT_NORMAL, ACHIEVEMENTS_LIST_ITEM_X, 17, sAchievementsListTextColors,
        needsScroll ? GetPlayerTextSpeedDelay() : TEXT_SKIP_DRAW, sAchievementsMenuStatePtr->detailDescriptionBuffer);
    CopyWindowToVram(WIN_LIST, COPYWIN_GFX);

    // See PrintAchievementDescription's identical pair for why.
    sAchievementsMenuStatePtr->detailDescriptionScrolling = needsScroll;
    sAchievementsMenuStatePtr->detailDescriptionRestartTimer = 0;
}

static void EnterDetailLevel(u8 taskId, u16 achievementId)
{
    const struct Achievement *info = Achievement_GetInfo(achievementId);
    bool8 completed = Achievement_IsCompleted(achievementId);

    DrawHeaderText(sTierNames[info->tier]);

    PrintDetailDescription(achievementId);

    ConvertIntToDecimalStringN(gStringVar1, info->points, STR_CONV_MODE_LEFT_ALIGN, 5);
    StringExpandPlaceholders(gStringVar4, sText_RewardFormat);

    FillWindowPixelBuffer(WIN_DESCRIPTION, PIXEL_FILL(0));
    AddTextPrinterParameterized3(WIN_DESCRIPTION, FONT_NORMAL, 8, ACHIEVEMENTS_DESC_LINE1_Y, sAchievementsMenuTextColors, TEXT_SKIP_DRAW, gStringVar4);
    // Trails the figure, where the word "Points" used to. Measured off the
    // expanded string rather than a fixed offset, since the point value's
    // digit count varies.
    AchievementIcons_Blit(ACHIEVEMENT_ICON_POINTS, WIN_DESCRIPTION, 8 + GetStringWidth(FONT_NORMAL, gStringVar4, 0) + 2, ACHIEVEMENT_ICON_Y(ACHIEVEMENTS_DESC_LINE1_Y));
    AddTextPrinterParameterized3(WIN_DESCRIPTION, FONT_NORMAL, 8, ACHIEVEMENTS_DESC_LINE2_Y, sAchievementsMenuTextColors, TEXT_SKIP_DRAW, completed ? sText_StatusCompleted : sText_StatusIncomplete);
    CopyWindowToVram(WIN_DESCRIPTION, COPYWIN_GFX);

    // Same fix as the identical block in Task_List_ProcessInput, for
    // DETAIL's own overlong-description printer instead of LIST's -- see
    // that comment for the full mechanism. Here the reward/status prints
    // just above are the ones regenerating the shared glyph colour table
    // out from under PrintDetailDescription's own printer a few lines
    // earlier: sAchievementsMenuTextColors (shadow index 6) instead of
    // sAchievementsListTextColors (shadow index 2), so a still-pending
    // overlong name/description here would have its first screenful drawn
    // with the wrong shadow colour instead of the wrong foreground -- same
    // root cause, subtler result, but still wrong.
    if (sAchievementsMenuStatePtr->detailDescriptionScrolling)
    {
        union TextColor color;

        color.background = sAchievementsListTextColors[0];
        color.foreground = sAchievementsListTextColors[1];
        color.shadow = sAchievementsListTextColors[2];
        color.accent = sAchievementsListTextColors[0];
        GenerateFontHalfRowLookupTable(color);
    }
}

static void Task_Detail_ProcessInput(u8 taskId)
{
    if (JOY_NEW(A_BUTTON | B_BUTTON))
    {
        PlaySE(SE_SELECT);
        // DETAIL owns no ListMenuTask of its own, so unlike every other
        // level transition this one never routes through
        // DestroyCurrentAchievementsList -- stop PrintDetailDescription's
        // WIN_LIST printer/loop here instead, before EnterListLevel below
        // reclaims WIN_LIST for its row list. Without this, a still-
        // scrolling detailDescriptionScrolling would have MainCB2 call
        // PrintDetailDescription again after the switch, clobbering the row
        // list with DETAIL's name/description prints.
        DeactivateSingleTextPrinter(WIN_LIST, WINDOW_TEXT_PRINTER);
        gTextFlags.autoScroll = FALSE;
        sAchievementsMenuStatePtr->detailDescriptionScrolling = FALSE;
        EnterListLevel(taskId, sAchievementsMenuStatePtr->selectedTier);
        gTasks[taskId].func = Task_List_ProcessInput;
    }
}

// ---- Shared ----------------------------------------------------------------

static void DestroyCurrentAchievementsList(u8 taskId)
{
    DestroyListMenuTask(gTasks[taskId].tListTaskId, NULL, NULL);
    RemoveScrollIndicatorArrowPair(gTasks[taskId].tScrollArrowTaskId);
    // Called on every level transition (TIER SELECT <-> LIST, LIST -> DETAIL,
    // and both ways out of this menu entirely), which covers every point a
    // long description's WIN_DESCRIPTION scroll printer (see
    // PrintAchievementDescription) needs to stop: DeactivateSingleTextPrinter
    // so a still-scrolling printer doesn't keep ticking against a window this
    // screen no longer owns, and clearing gTextFlags.autoScroll so a
    // mid-scroll visit doesn't leave ordinary dialogue elsewhere in the game
    // auto-advancing without a button press afterward.
    DeactivateSingleTextPrinter(WIN_DESCRIPTION, WINDOW_TEXT_PRINTER);
    gTextFlags.autoScroll = FALSE;
    // Stops MainCB2 from restarting a printer against a window this screen
    // no longer owns once it goes idle, same reasoning as the two lines
    // above -- see its own comment on sAchievementsMenuStatePtr->descriptionScrolling.
    sAchievementsMenuStatePtr->descriptionScrolling = FALSE;
    // Same pair, for DETAIL's own WIN_LIST scroll printer (PrintDetail
    // Description) -- belt-and-suspenders here, since the current DETAIL ->
    // LIST path stops it itself (see Task_Detail_ProcessInput) before this
    // function ever runs, but this function is the one every *other*
    // transition already routes through to stop LIST's own printer above.
    DeactivateSingleTextPrinter(WIN_LIST, WINDOW_TEXT_PRINTER);
    sAchievementsMenuStatePtr->detailDescriptionScrolling = FALSE;
}

// Repaints one WIN_LIST row in place, used by Task_TierSelect_ProcessInput/
// Task_List_ProcessInput when the cursor moves without scrolling (see their
// own comments) -- only the previously- and newly-highlighted rows' pixels
// actually differ in that case (same icon/text, just a different colour), so
// this repaints just those two instead of RedrawListMenu's clear-then-
// redraw-every-visible-row, which is what caused the flicker on plain
// up/down navigation: a full FillWindowPixelBuffer blanks the whole window
// for a frame before ListMenuPrintEntries redraws it, even for rows that
// never changed.
//
// Deliberately doesn't go through TierSelect_ItemPrintCallback/
// AchievementsMenu_ItemPrintCallback (or list_menu.c's own ListMenuPrint) --
// those arm gListMenuOverride, a single-slot global that only the engine's
// own next ListMenuPrint call consumes. Calling itemPrintFunc here without a
// matching ListMenuPrint right after would leave that override armed and
// silently recolour whatever the *next* unrelated row happens to print
// through the real engine path (e.g. the next scroll). drawRow (the raw
// TierSelect_DrawRow/AchievementsMenu_DrawRow halves, which take an explicit
// colours array instead of touching the override) and this function's own
// direct AddTextPrinterParameterized3 call for the name sidestep the
// override machinery entirely, so nothing needs arming or resetting.
static void RepaintListRow(void (*drawRow)(u8, u32, u8, const u8 *), u32 arrayIndex, u8 y)
{
    const struct ListMenuItem *item = &sAchievementsMenuStatePtr->listItems[arrayIndex];
    bool8 selected = (item->id == sAchievementsMenuStatePtr->highlightedId);
    const u8 *colors = selected ? sAchievementsListHighlightTextColors : sAchievementsListTextColors;

    drawRow(WIN_LIST, item->id, y, colors);
    AddTextPrinterParameterized3(WIN_LIST, FONT_NORMAL, ACHIEVEMENTS_LIST_ITEM_X, y, colors, TEXT_SKIP_DRAW, item->name);
}

static void DrawHeaderText(const u8 *title)
{
    s32 hintX = GetStringRightAlignXOffset(FONT_NARROW, sText_ControlHint, 198);

    FillWindowPixelBuffer(WIN_HEADER, PIXEL_FILL(0));
    AddTextPrinterParameterized3(WIN_HEADER, FONT_NORMAL, 2, 0, sAchievementsMenuTextColors, TEXT_SKIP_DRAW, title);
    AddTextPrinterParameterized3(WIN_HEADER, FONT_NARROW, hintX, 0, sAchievementsMenuTextColors, TEXT_SKIP_DRAW, sText_ControlHint);
    CopyWindowToVram(WIN_HEADER, COPYWIN_FULL);
}
