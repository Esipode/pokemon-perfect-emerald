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

// Stage 3.1 template (design doc §3.1): src/new_game_settings_menu.c's
// skeleton copied wholesale -- BG/window templates, staged CB2 init,
// ListMenu + scroll arrows.
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
// Stage 3.2 (design doc §3.2): the three-level TIER SELECT / LIST / DETAIL
// flow. One CB2 boots the screen straight into TIER SELECT; the three levels
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
// Stage 3.3 (Start Menu entry point) is wired separately in
// src/start_menu.c (MENU_ACTION_ACHIEVEMENTS / StartMenuAchievementsCallback).
//
// The "Boosts" row on the TIER SELECT mockup (Stage 7): appended to the tier
// list as one extra row, id TIER_SELECT_ITEM_BOOSTS, only when
// Achievement_BoostsUnlocked() && Achievement_BoostsEnabled() (Stage 6: OFF
// hides the shop, not just the toggle). Selecting it fades out and jumps to
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
// (constants/achievements.h, Stage 21) -- this used to be its own local
// derivation before that wave gave the rest of the codebase a shared one.

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
// AddTextPrinterParameterized never clips or wraps on its own (design doc/
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
// WIN_LIST's 16px tilemapLeft is subtracted. 140 leaves a few px of margin
// short of that measured edge, matching the achievement list's own text
// column instead of the wider window/WIN_DESCRIPTION's own box.
#define ACHIEVEMENTS_DETAIL_DESC_MAX_WIDTH 140

// WIN_DESCRIPTION's two text lines. FONT_NORMAL's line height is exactly 16px
// (src/text.c's fontAttributes[FONT_NORMAL].maxLetterHeight) and the window
// itself is 5 tiles/40px tall (see sAchievementsMenuWinTemplates) rather than
// the 4 tiles/32px the two lines alone need, so there's an 8px margin split
// 6px above LINE1_Y and 2px below LINE2_Y -- shifts the text up within the
// box rather than leaving it hugging the top with zero margin, closer to
// centered against the art's own description box.
#define ACHIEVEMENTS_DESC_LINE1_Y 7
#define ACHIEVEMENTS_DESC_LINE2_Y 23

// Checkbox/tier-name prefix plus the item text itself; the longest real
// content (an achievement name) is already capped at ACHIEVEMENT_NAME_LENGTH
// (including its terminator) by ACHIEVEMENT_NAME(), so this leaves generous
// headroom rather than computing the exact minimum. Reused for both the
// achievement list rows and the (shorter) tier select rows.
#define ACHIEVEMENTS_LIST_NAME_BUFFER_SIZE (ACHIEVEMENT_NAME_LENGTH + 8)

// TIER SELECT's own row count once the Stage 7 "BOOSTS" row is visible --
// one past the last real tier ID, reused as that row's ListMenuItem.id too
// (see TIER_SELECT_ITEM_BOOSTS below).
#define TIER_SELECT_ROW_COUNT (ACHIEVEMENT_TIER_COUNT + 1)

// Shared by both lists this menu ever shows (tier select's up to
// TIER_SELECT_ROW_COUNT rows, or one tier's worth of achievement rows),
// sized to whichever is larger. Originally just ACHIEVEMENTS_MENU_ITEM_COUNT
// (the *whole* catalog) -- correct (never undersized, unlike an earlier
// version of this macro that corrupted memory past the array's end when tier
// select wrote all its rows) but wasteful, since BuildAchievementListItems
// below only ever shows one tier's rows at once, never the whole catalog.
//
// Stage 15 (catalog wave 2): replaced with a manually-tracked worst-case
// single-tier count instead. At the end of Stage 15 the largest tier
// (Silver) holds 32 entries; ACHIEVEMENTS_MENU_MAX_PER_TIER gives headroom
// above that so it doesn't need bumping on every wave, but MUST be raised if
// a future wave ever pushes a single tier's count above it.
// BuildAchievementListItems' bounds check fails safe (truncates the list
// rather than corrupting EWRAM) if this is ever wrong, but a truncated
// achievement list is still a bug worth catching early -- bump this the
// moment a future wave's tier totals approach it.
#define ACHIEVEMENTS_MENU_MAX_PER_TIER 60

#define ACHIEVEMENTS_MENU_LIST_CAPACITY \
    (ACHIEVEMENTS_MENU_MAX_PER_TIER > TIER_SELECT_ROW_COUNT ? ACHIEVEMENTS_MENU_MAX_PER_TIER : TIER_SELECT_ROW_COUNT)

EWRAM_DATA static u8 sAchievementsListNameBuffers[ACHIEVEMENTS_MENU_LIST_CAPACITY][ACHIEVEMENTS_LIST_NAME_BUFFER_SIZE] = {0};
EWRAM_DATA static struct ListMenuItem sAchievementsListItems[ACHIEVEMENTS_MENU_LIST_CAPACITY] = {0};

EWRAM_DATA static struct
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
} sAchievementsMenu = {0};

// Cached once per TIER SELECT build (see BuildTierSelectListItems) so the
// per-row itemPrintFunc doesn't re-scan every achievement on every redraw.
EWRAM_DATA static struct
{
    u16 completed;
    u16 total;
} sTierCounts[ACHIEVEMENT_TIER_COUNT] = {0};

// Stage 7: this menu's own CB2 doubles as the boost shop's return point
// (Task_TierSelect_OpenBoostMenu sets gMain.savedCallback =
// CB2_InitAchievementsMenu before jumping there), which would otherwise
// clobber the *real* caller (Start Menu/debug menu) recorded in
// gMain.savedCallback on entry. sAchievementsMenuReturnCallback is that real
// caller, stashed away before the overwrite and restored into
// gMain.savedCallback the moment this screen is re-entered from the boost
// shop -- sReturningFromBoostShop is what tells case 0 which of those two
// things is happening.
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
static void DrawHeaderText(const u8 *title);
static void LoadTierIcons(void);
static void BlitTierIcon(u8 tier, u8 windowId, u16 x, u16 y);

static const u8 sText_AchievementsTitle[]  = _("ACHIEVEMENTS");
static const u8 sText_ControlHint[]        = _("{B_BUTTON} BACK");
// '[' and ']' aren't in charmap.txt -- use the existing filled/hollow circle
// glyphs instead of literal brackets.
static const u8 sText_CompletedPrefix[]    = _("{CIRCLE_DOT} ");
static const u8 sText_IncompletePrefix[]   = _("{CIRCLE_HOLLOW} ");
// design doc §17: hidden achievements show as "???" -- name and description
// both -- until completed. Their point value and tier are not withheld
// (mirrors the design doc §3.2 mockup: "[ ] ???                50").
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

// The extra TIER SELECT row (Stage 7) sits one past the last real tier ID --
// safe as a ListMenuItem.id since tier IDs and this are otherwise disjoint,
// and TierSelect_ItemPrintCallback/Task_TierSelect_ProcessInput both check
// for it before treating an itemId as a tier.
#define TIER_SELECT_ITEM_BOOSTS ACHIEVEMENT_TIER_COUNT
// Both of these used to spell out "Points"/"Points:"; the points icon now
// stands in for the word, blitted next to the figure it belongs to (see
// DrawTierSelectHeaderText and EnterDetailLevel), so the strings themselves
// carry only what the icon can't say.
static const u8 sText_PointsSummaryFormat[] = _("{STR_VAR_1}/{STR_VAR_2}");
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
    // hugging the top edge with no margin at all (see ACHIEVEMENTS_DESC_LINE1_Y).
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

EWRAM_DATA static u8 sTierIconPixels[ACHIEVEMENT_TIER_COUNT][TIER_ICON_BYTE_COUNT] = {0};

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
            sTierIconPixels[tier][i] = remap[src[i] & 0xF] | (remap[(src[i] >> 4) & 0xF] << 4);
    }
}

static void BlitTierIcon(u8 tier, u8 windowId, u16 x, u16 y)
{
    BlitBitmapToWindow(windowId, sTierIconPixels[tier], x, y, TIER_ICON_SIZE, TIER_ICON_SIZE);
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
        memset(&sAchievementsMenu, 0, sizeof(sAchievementsMenu));
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
    sAchievementsMenu.highlightedId = sAchievementsListItems[sAchievementsMenu.tierScrollOffset + sAchievementsMenu.tierSelectedRow].id;

    template.items = sAchievementsListItems;
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

    gTasks[taskId].tListTaskId = ListMenuInit(&template, sAchievementsMenu.tierScrollOffset, sAchievementsMenu.tierSelectedRow);
    gTasks[taskId].tScrollArrowTaskId = AddScrollIndicatorArrowPairParameterized(
        SCROLL_ARROW_UP, ACHIEVEMENTS_ARROW_X, ACHIEVEMENTS_ARROW_TOP_Y, ACHIEVEMENTS_ARROW_BOTTOM_Y,
        // Clamped like EnterListLevel's own scroll arrows below -- now that
        // ACHIEVEMENTS_MENU_MAX_SHOWED is 5, itemCount is 4 whenever the
        // Stage 7 BOOSTS row is hidden, and a bare subtraction would go
        // negative (see EnterListLevel's own comment on this exact bug).
        (itemCount > ACHIEVEMENTS_MENU_MAX_SHOWED) ? (itemCount - ACHIEVEMENTS_MENU_MAX_SHOWED) : 0,
        TAG_ACHIEVEMENTS_SCROLL_ARROWS, TAG_ACHIEVEMENTS_SCROLL_ARROWS,
        &sAchievementsMenu.tierScrollOffset);
}

static void Task_TierSelect_ProcessInput(u8 taskId)
{
    u16 prevScrollOffset = sAchievementsMenu.tierScrollOffset;
    u16 prevSelectedRow = sAchievementsMenu.tierSelectedRow;
    u8 prevRowY = ListMenuGetYCoordForPrintingArrowCursor(gTasks[taskId].tListTaskId);
    s32 itemId = ListMenu_ProcessInput(gTasks[taskId].tListTaskId);
    ListMenuGetScrollAndRow(gTasks[taskId].tListTaskId, &sAchievementsMenu.tierScrollOffset, &sAchievementsMenu.tierSelectedRow);

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
    if (prevScrollOffset != sAchievementsMenu.tierScrollOffset)
    {
        RedrawListMenu(gTasks[taskId].tListTaskId);
    }
    else if (prevSelectedRow != sAchievementsMenu.tierSelectedRow)
    {
        u8 newRowY = ListMenuGetYCoordForPrintingArrowCursor(gTasks[taskId].tListTaskId);

        RepaintListRow(TierSelect_DrawRow, prevScrollOffset + prevSelectedRow, prevRowY);
        RepaintListRow(TierSelect_DrawRow, prevScrollOffset + sAchievementsMenu.tierSelectedRow, newRowY);
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
        sAchievementsMenu.selectedTier = itemId;
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
        sAchievementsMenu.listScrollOffset = 0;
        sAchievementsMenu.listSelectedRow = 0;
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
        EnterListLevel(taskId, sAchievementsMenu.selectedTier);
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
        sReturningFromBoostShop = TRUE;
        gMain.savedCallback = CB2_InitAchievementsMenu;
        SetMainCallback2(CB2_InitAchievementBoostMenu);
    }
}

static void TierSelect_MoveCursorCallback(s32 itemIndex, bool8 onInit, struct ListMenu *list)
{
    if (!onInit)
        PlaySE(SE_SELECT);
    sAchievementsMenu.highlightedId = itemIndex;
    // A same-page cursor move never reprints row text on its own (see
    // list_menu.c's ListMenuChangeSelectionFull case 1 -- with cursorKind
    // CURSOR_INVISIBLE, ListMenuDrawCursor is the only thing it calls, and
    // that's a no-op for this cursor kind), so without this the orange
    // highlight would only ever show on whichever row happened to be
    // selected when the list was first drawn.
    ListMenuRepaintItems(list);
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

    // The Stage 7 "BOOSTS" row (id TIER_SELECT_ITEM_BOOSTS) isn't a tier --
    // sTierCounts[] has no entry for it, and it has no medal icon or count
    // column of its own.
    if (tier >= ACHIEVEMENT_TIER_COUNT)
        return;

    BlitTierIcon(tier, windowId, ACHIEVEMENTS_TIER_ICON_X, ACHIEVEMENT_ICON_Y(y));

    ptr = ConvertIntToDecimalStringN(gStringVar4, sTierCounts[tier].completed, STR_CONV_MODE_LEFT_ALIGN, 3);
    ptr = StringCopy(ptr, sText_TierCountSeparator);
    ConvertIntToDecimalStringN(ptr, sTierCounts[tier].total, STR_CONV_MODE_LEFT_ALIGN, 3);

    width = GetStringWidth(FONT_NORMAL, gStringVar4, 0);
    AddTextPrinterParameterized3(windowId, FONT_NORMAL, ACHIEVEMENTS_TIER_COUNT_RIGHT_X - width, y, colors, TEXT_SKIP_DRAW, gStringVar4);
}

static void TierSelect_ItemPrintCallback(u8 windowId, u32 tier, u8 y)
{
    bool8 selected = (tier == sAchievementsMenu.highlightedId);
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
        sTierCounts[tier].completed = 0;
        sTierCounts[tier].total = 0;
    }

    for (id = ACHIEVEMENT_NONE + 1; id < ACHIEVEMENTS_COUNT; id++)
    {
        tier = Achievement_GetInfo(id)->tier;
        sTierCounts[tier].total++;
        if (Achievement_IsCompleted(id))
            sTierCounts[tier].completed++;
    }

    for (tier = 0; tier < ACHIEVEMENT_TIER_COUNT; tier++)
    {
        u8 *buffer = sAchievementsListNameBuffers[tier];

        StringCopy(buffer, sTierNames[tier]);
        sAchievementsListItems[tier].name = buffer;
        sAchievementsListItems[tier].id = tier;
    }

    if (IsBoostShopRowVisible())
    {
        u8 *buffer = sAchievementsListNameBuffers[ACHIEVEMENT_TIER_COUNT];

        StringCopy(buffer, sText_BoostsMenuRowLabel);
        sAchievementsListItems[ACHIEVEMENT_TIER_COUNT].name = buffer;
        sAchievementsListItems[ACHIEVEMENT_TIER_COUNT].id = TIER_SELECT_ITEM_BOOSTS;
    }
}

// design doc Stage 6: OFF hides the shop (not just the toggle), so this
// checks both -- unlocked but disabled must not show the row.
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
    // fit the title and the [B] BACK hint.
    ConvertIntToDecimalStringN(gStringVar1, Achievement_GetAvailablePoints(), STR_CONV_MODE_LEFT_ALIGN, 6);
    ConvertIntToDecimalStringN(gStringVar2, Achievement_GetTotalPoints(), STR_CONV_MODE_LEFT_ALIGN, 6);
    StringExpandPlaceholders(gStringVar4, sText_PointsSummaryFormat);
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

    sAchievementsMenu.selectedTier = tier;

    LoadMenuBackground(ACHIEVEMENTS_BG_SCREEN_DETAIL);
    DrawHeaderText(sTierNames[tier]);
    BuildAchievementListItems(tier);

    // See the identical comment in EnterTierSelectLevel -- must be set
    // before ListMenuInit so the initially-selected row is highlighted from
    // the very first draw.
    sAchievementsMenu.highlightedId = sAchievementsListItems[sAchievementsMenu.listScrollOffset + sAchievementsMenu.listSelectedRow].id;

    template.items = sAchievementsListItems;
    template.moveCursorFunc = AchievementsMenu_MoveCursorCallback;
    template.itemPrintFunc = AchievementsMenu_ItemPrintCallback;
    template.totalItems = sAchievementsMenu.listItemCount;
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

    gTasks[taskId].tListTaskId = ListMenuInit(&template, sAchievementsMenu.listScrollOffset, sAchievementsMenu.listSelectedRow);
    gTasks[taskId].tScrollArrowTaskId = AddScrollIndicatorArrowPairParameterized(
        SCROLL_ARROW_UP, ACHIEVEMENTS_ARROW_X, ACHIEVEMENTS_ARROW_TOP_Y, ACHIEVEMENTS_ARROW_BOTTOM_Y,
        // Clamped, not a bare subtraction: every tier but BRONZE currently
        // has fewer than ACHIEVEMENTS_MENU_MAX_SHOWED achievements (Stage
        // 2.3's 3 test achievements are all BRONZE), so this can go
        // negative. A negative threshold truncates into a huge u16 when
        // stored (struct ScrollIndicatorPair.fullyDownThreshold,
        // src/list_menu.c:29) that the real scroll offset can never match,
        // leaving the down arrow stuck visible with nothing left to scroll
        // to (same issue fixed for the Stage 7 boost list in
        // src/achievement_boost_menu.c).
        (sAchievementsMenu.listItemCount > ACHIEVEMENTS_MENU_MAX_SHOWED) ? (sAchievementsMenu.listItemCount - ACHIEVEMENTS_MENU_MAX_SHOWED) : 0,
        TAG_ACHIEVEMENTS_SCROLL_ARROWS, TAG_ACHIEVEMENTS_SCROLL_ARROWS,
        &sAchievementsMenu.listScrollOffset);
}

static void Task_List_ProcessInput(u8 taskId)
{
    u16 prevScrollOffset = sAchievementsMenu.listScrollOffset;
    u16 prevSelectedRow = sAchievementsMenu.listSelectedRow;
    u8 prevRowY = ListMenuGetYCoordForPrintingArrowCursor(gTasks[taskId].tListTaskId);
    s32 itemId = ListMenu_ProcessInput(gTasks[taskId].tListTaskId);
    ListMenuGetScrollAndRow(gTasks[taskId].tListTaskId, &sAchievementsMenu.listScrollOffset, &sAchievementsMenu.listSelectedRow);

    // See the identical comment in Task_TierSelect_ProcessInput.
    if (prevScrollOffset != sAchievementsMenu.listScrollOffset)
    {
        RedrawListMenu(gTasks[taskId].tListTaskId);
    }
    else if (prevSelectedRow != sAchievementsMenu.listSelectedRow)
    {
        u8 newRowY = ListMenuGetYCoordForPrintingArrowCursor(gTasks[taskId].tListTaskId);

        RepaintListRow(AchievementsMenu_DrawRow, prevScrollOffset + prevSelectedRow, prevRowY);
        RepaintListRow(AchievementsMenu_DrawRow, prevScrollOffset + sAchievementsMenu.listSelectedRow, newRowY);
        CopyWindowToVram(WIN_LIST, COPYWIN_GFX);
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
    sAchievementsMenu.highlightedId = itemIndex;
    // See the identical comment in TierSelect_MoveCursorCallback.
    ListMenuRepaintItems(list);
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
    bool8 selected = (achievementId == sAchievementsMenu.highlightedId);
    const u8 *colors = selected ? sAchievementsListHighlightTextColors : sAchievementsListTextColors;

    // See the identical comment in TierSelect_ItemPrintCallback.
    if (selected)
        ListMenuOverrideSetColors(colors[1], colors[0], colors[2]);

    AchievementsMenu_DrawRow(windowId, achievementId, y, colors);
}

// Builds the tier-filtered item list (skips ACHIEVEMENT_NONE and any ID
// outside this tier) and bakes the completion checkbox into each row's label
// text. Hidden achievements (design doc §17) render their name as "???"
// until completed. Achievement completion can't change while this menu is
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

        // Stage 15 (catalog wave 2): ACHIEVEMENTS_MENU_MAX_PER_TIER is a
        // manually-tracked bound, not a derived one -- fail safe (truncate)
        // rather than write past the array if a future wave ever exceeds it.
        if (index >= ACHIEVEMENTS_MENU_LIST_CAPACITY)
            break;

        completed = Achievement_IsCompleted(id);
        buffer = sAchievementsListNameBuffers[index];

        StringCopy(buffer, completed ? sText_CompletedPrefix : sText_IncompletePrefix);
        StringAppend(buffer, (info->hidden && !completed) ? sText_HiddenName : info->name);

        sAchievementsListItems[index].name = buffer;
        sAchievementsListItems[index].id = id;
        index++;
    }

    sAchievementsMenu.listItemCount = index;
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

        StringCopy(gStringVar1, masked ? sText_HiddenDescription : info->description);
        StripLineBreaks(gStringVar1);
        BreakStringAutomatic(gStringVar1, ACHIEVEMENTS_DESC_MAX_WIDTH, 2, FONT_NORMAL, SHOW_SCROLL_PROMPT);
        needsScroll = StringHasScrollPrompt(gStringVar1);

        gTextFlags.autoScroll = needsScroll;
        AddTextPrinterParameterized3(WIN_DESCRIPTION, FONT_NORMAL, 8, ACHIEVEMENTS_DESC_LINE1_Y, sAchievementsMenuTextColors,
            needsScroll ? GetPlayerTextSpeedDelay() : TEXT_SKIP_DRAW, gStringVar1);
    }
    CopyWindowToVram(WIN_DESCRIPTION, COPYWIN_GFX);
}

// ---- DETAIL ----------------------------------------------------------------

static void EnterDetailLevel(u8 taskId, u16 achievementId)
{
    const struct Achievement *info = Achievement_GetInfo(achievementId);
    bool8 completed = Achievement_IsCompleted(achievementId);
    bool8 masked = info->hidden && !completed;

    DrawHeaderText(sTierNames[info->tier]);

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
    FillWindowPixelBuffer(WIN_LIST, PIXEL_FILL(0));
    AddTextPrinterParameterized3(WIN_LIST, FONT_NORMAL, ACHIEVEMENTS_LIST_ITEM_X, 1, sAchievementsListHighlightTextColors, TEXT_SKIP_DRAW, masked ? sText_HiddenName : info->name);
    StringCopy(gStringVar1, masked ? sText_HiddenDescription : info->description);
    StripLineBreaks(gStringVar1);
    // ACHIEVEMENTS_DETAIL_DESC_MAX_WIDTH, not ACHIEVEMENTS_DESC_MAX_WIDTH --
    // see its own comment; WIN_LIST's box is narrower than WIN_DESCRIPTION's.
    BreakStringAutomatic(gStringVar1, ACHIEVEMENTS_DETAIL_DESC_MAX_WIDTH, 3, FONT_NORMAL, HIDE_SCROLL_PROMPT);
    AddTextPrinterParameterized3(WIN_LIST, FONT_NORMAL, ACHIEVEMENTS_LIST_ITEM_X, 17, sAchievementsListTextColors, TEXT_SKIP_DRAW, gStringVar1);
    CopyWindowToVram(WIN_LIST, COPYWIN_GFX);

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
}

static void Task_Detail_ProcessInput(u8 taskId)
{
    if (JOY_NEW(A_BUTTON | B_BUTTON))
    {
        PlaySE(SE_SELECT);
        EnterListLevel(taskId, sAchievementsMenu.selectedTier);
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
    const struct ListMenuItem *item = &sAchievementsListItems[arrayIndex];
    bool8 selected = (item->id == sAchievementsMenu.highlightedId);
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
