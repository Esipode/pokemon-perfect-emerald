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

// Skeleton copied from src/new_game_settings_menu.c: BG/window templates,
// staged CB2 init, ListMenu + scroll arrows.
//
// bg1 is a dedicated art layer (own charBaseIndex) showing one of two
// full-screen pictures from graphics/achievements/ui/bg_main.png: TIER SELECT
// gets the "achievements" screen, LIST and DETAIL both get the "detail" screen
// (LoadMenuBackground, called from EnterTierSelectLevel/EnterListLevel). bg0
// holds all three windows in front of it; windows fill with PIXEL_FILL(0), the
// see-through index, so the art shows wherever there is no glyph ink. See
// src/ui_stat_editor.c for the same bg1-art/bg0-window split.
//
// The TIER SELECT / LIST / DETAIL levels swap the task's func and rebuild the
// same WIN_HEADER/WIN_LIST/WIN_DESCRIPTION trio in place rather than re-running
// the CB2 state machine. TIER SELECT and LIST each load their own bg1 screen
// and palette, so moving between them fades out/in (Task_TierSelect_ToListLevel/
// Task_List_ToTierSelectLevel): LoadPalette writes immediately but
// ScheduleBgCopyTilemapToVram lands on the next vblank, so the fade hides the
// frame with a mismatched palette and tilemap. LIST and DETAIL share one bg1
// screen and do not fade.
//
// The "Boosts" row on TIER SELECT (id TIER_SELECT_ITEM_BOOSTS) is appended only
// when Achievement_BoostsUnlocked() && Achievement_BoostsEnabled(). Selecting
// it jumps to src/achievement_boost_menu.c with gMain.savedCallback pointed at
// CB2_InitAchievementsMenu. That overwrites the slot this screen's own [B]
// Back reads, so sAchievementsMenuReturnCallback/sReturningFromBoostShop stash
// the real caller (Start Menu/debug menu) and restore it on re-entry.

enum
{
    WIN_HEADER,
    WIN_LIST,
    WIN_DESCRIPTION,
};


#define ACHIEVEMENTS_MENU_MAX_SHOWED 5
#define ACHIEVEMENTS_MENU_ITEM_COUNT (ACHIEVEMENTS_COUNT - 1) // excludes ACHIEVEMENT_NONE

#define tListTaskId        data[0]
#define tScrollArrowTaskId data[1]

#define TAG_ACHIEVEMENTS_SCROLL_ARROWS 6001

#define ACHIEVEMENTS_POINTS_RIGHT_X 190
// 0: rows draw no arrow cursor (CURSOR_INVISIBLE), so they sit flush with the box's left edge.
#define ACHIEVEMENTS_LIST_ITEM_X    0
// Fixed column so every tier's medal icon lines up regardless of name width,
// with the completed/total count following it.
#define ACHIEVEMENTS_TIER_ICON_X    122
// TierSelect_DrawRow's completed/total count right-aligns here rather than at
// ACHIEVEMENTS_POINTS_RIGHT_X, nudged 10px right of it. The achievement list's
// points column still uses ACHIEVEMENTS_POINTS_RIGHT_X.
#define ACHIEVEMENTS_TIER_COUNT_RIGHT_X (ACHIEVEMENTS_POINTS_RIGHT_X + 14)
#define ACHIEVEMENTS_ARROW_X        200
// 4px inside WIN_LIST's top/bottom edge (see sAchievementsMenuWinTemplates).
#define ACHIEVEMENTS_ARROW_TOP_Y    20
#define ACHIEVEMENTS_ARROW_BOTTOM_Y 100

// WIN_DESCRIPTION/WIN_LIST are 208px wide with text at x=8. Text printers never
// wrap, so a longer unwrapped description bleeds into the next row's tile
// memory.
//
// Only safe for WIN_DESCRIPTION, whose art is one continuous panel. DETAIL
// prints into WIN_LIST, so it uses ACHIEVEMENTS_DETAIL_DESC_MAX_WIDTH.
#define ACHIEVEMENTS_DESC_MAX_WIDTH 190

// WIN_LIST's art is two boxes side by side with a divider around screen
// x=165-170; names and DETAIL text sit in the left box, which ends near screen
// x=163 (window-relative ~147 after WIN_LIST's 16px tilemapLeft). 155 leaves
// margin short of that edge.
#define ACHIEVEMENTS_DETAIL_DESC_MAX_WIDTH 155

// DETAIL reuses WIN_LIST's box: the first row (y=1) holds the name and the
// remaining 4 rows (y=17 onward, see EnterDetailLevel) hold the description.
#define ACHIEVEMENTS_DETAIL_DESC_LINES 4

// FONT_NORMAL lines are 16px and the window is 40px tall, leaving an 8px margin
// spent entirely below LINE2_Y. LINE1_Y must stay flush with the window's top
// edge: RunTextPrinters' scroll (src/text.c RENDER_STATE_SCROLL) shifts the
// whole pixel buffer up one 16px line, so a gap above line 1 leaves the bottom
// of the retiring line visible.
#define ACHIEVEMENTS_DESC_LINE1_Y 1
#define ACHIEVEMENTS_DESC_LINE2_Y 17

// Idle time on an overlong description's last screenful before it loops back to
// the top (see PrintAchievementDescription/MainCB2's descriptionScrolling).
#define ACHIEVEMENTS_DESC_RESTART_DELAY 120

// Checkbox/tier-name prefix plus item text. Achievement names are capped at
// ACHIEVEMENT_NAME_LENGTH by ACHIEVEMENT_NAME(), so this leaves headroom.
// Shared by the achievement list rows and the shorter tier select rows.
#define ACHIEVEMENTS_LIST_NAME_BUFFER_SIZE (ACHIEVEMENT_NAME_LENGTH + 8)

// TIER SELECT's row count with the "BOOSTS" row visible: one past the last real
// tier ID, also that row's ListMenuItem.id (see TIER_SELECT_ITEM_BOOSTS).
#define TIER_SELECT_ROW_COUNT (ACHIEVEMENT_TIER_COUNT + 1)

// Shared by both lists this menu shows. Sized off the whole catalog so it is
// always at least as large as any single tier and grows with ACHIEVEMENTS_COUNT.
// BuildAchievementListItems truncates rather than overflowing if this is ever
// too small.
#define ACHIEVEMENTS_MENU_LIST_CAPACITY \
    (ACHIEVEMENTS_MENU_ITEM_COUNT > TIER_SELECT_ROW_COUNT ? ACHIEVEMENTS_MENU_ITEM_COUNT : TIER_SELECT_ROW_COUNT)

// Descriptions are built into dedicated buffers, not gStringVar1. An overlong
// description's auto-scroll print runs across many frames, and
// AddTextPrinterParameterized3 keeps only a raw pointer into the buffer
// (TextPrinterTemplate.currentChar, src/text.c). gStringVar1 is engine-wide
// scratch, rewritten on every list repaint by AchievementsMenu_DrawRow, so
// scrolling the list mid-scroll made the printer read the points figure plus
// stale bytes (including stray colour-control bytes, hence an orange tint).
#define ACHIEVEMENTS_DESC_BUFFER_SIZE 0x100

// This menu's CB2 doubles as the boost shop's return point
// (Task_TierSelect_OpenBoostMenu sets gMain.savedCallback =
// CB2_InitAchievementsMenu), which would clobber the real caller recorded on
// entry. sAchievementsMenuReturnCallback stashes that caller and is restored
// into gMain.savedCallback on re-entry from the shop; sReturningFromBoostShop
// tells case 0 which case applies.
//
// Plain EWRAM statics rather than struct AchievementsMenuState fields: both
// must survive Task_TierSelect_OpenBoostMenu's Free(sAchievementsMenuStatePtr)
// and be readable in case 0 before its AllocZeroed.
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
// '[' and ']' aren't in charmap.txt; use the filled/hollow circle glyphs instead.
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

// One past the last real tier ID, so disjoint from tier IDs as a ListMenuItem.id.
// TierSelect_ItemPrintCallback/Task_TierSelect_ProcessInput check for it before
// treating an itemId as a tier.
#define TIER_SELECT_ITEM_BOOSTS ACHIEVEMENT_TIER_COUNT
// The points icon stands in for the word "Points" (see DrawTierSelectHeaderText
// and EnterDetailLevel), so these strings carry only what the icon can't say.
static const u8 sText_PointsSummaryFormat[] = _("{STR_VAR_1}/{STR_VAR_2}");
// Used until boosts unlock. Points cannot be spent before then, so the
// "{available}/{total}" fraction would always read "{total}/{total}".
static const u8 sText_TotalPointsFormat[]   = _("{STR_VAR_1}");
static const u8 sText_RewardFormat[]        = _("Reward: {STR_VAR_1}");
static const u8 sText_StatusCompleted[]     = _("Status: Completed");
static const u8 sText_StatusIncomplete[]    = _("Status: Not completed");

static const struct WindowTemplate sAchievementsMenuWinTemplates[] =
{
// tilemapTop 0: the art's header band is y=0-15.
    [WIN_HEADER] = {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 0,
        .width = 26,
        .height = 2,
        .paletteNum = 1,
        .baseBlock = 2
    },
    // tilemapTop 3: the art's body box starts at pixel row ~23; rows 0-2 sit on
    // the header's border trim.
    // height 10: 5 rows at 16px (ACHIEVEMENTS_MENU_MAX_SHOWED), so the tier list
    // (BRONZE/SILVER/GOLD/DIAMOND/BOOSTS) never scrolls.
    // paletteNum 2: four tier-medal icons (see LoadTierIcons) need 12 palette
    // slots, which don't fit in bank 1's 8 free entries shared with the
    // points/lock icons (src/achievement_icons.c). WIN_LIST gets bank 2 to
    // itself: its 3 text colours (sAchievementsListTextColors and
    // sAchievementsListHighlightTextColors) at low indices, icons filling the
    // rest.
    [WIN_LIST] = {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 3,
        .width = 26,
        .height = 10,
        .paletteNum = 2,
        .baseBlock = 0x36
    },
    // baseBlock follows WIN_LIST's (0x36 + 26*10 tiles = 0x13A).
    //
    // tilemapTop 15, height 5: the window is a tile taller than its two 16px lines
    // so text does not hug the bottom edge, with the margin below LINE2_Y (see
    // ACHIEVEMENTS_DESC_LINE1_Y). 15 + 5 == 20 tiles == the screen's 160px height,
    // so no margin is left below the window.
    [WIN_DESCRIPTION] = {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 15,
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
// Art layer; own charBaseIndex, never touched by window text.
        .bg = 1,
        .charBaseIndex = 3,
        .mapBaseIndex = 30,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 1,
        .baseTile = 0
    },
    {
        // Windows only. Lower priority number than bg1 so text draws in front of the
        // art; PIXEL_FILL(0) lets bg1 show through.
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

// {background, foreground, shadow} palette indices: transparent, white, dark
// gray. TEXT_COLOR_* constants past WHITE don't match this palette
// (DARK_GRAY/LIGHT_GRAY are orange/amber, see option_menu_text.pal), so shadow
// index 7 (74 74 74) is written directly.
static const u8 sAchievementsMenuTextColors[3] = {
    TEXT_COLOR_TRANSPARENT, TEXT_COLOR_WHITE, 6
};

// WIN_LIST's bank 2 has only white and dark gray, copied by LoadTierIcons from
// sAchievementsMenuText_Pal entries 1 and 6, so the dark-gray shadow index is 2
// rather than 6.
static const u8 sAchievementsListTextColors[3] = {
    TEXT_COLOR_TRANSPARENT, TEXT_COLOR_WHITE, 2
};

// Selected-row highlight for both WIN_LIST lists: the orange at raw index 2 of
// option_menu_text.pal. CURSOR_INVISIBLE removes the arrow cursor, so this
// colour marks the selected row instead.
static const u8 sAchievementsListHighlightTextColors[3] = {
    TEXT_COLOR_TRANSPARENT, 3, 2
};

// Marks an incomplete achievement the player can no longer earn this
// playthrough (Achievement_IsEligible false). Reuses WIN_LIST's dark-gray index
// for foreground and shadow rather than a red: bank 2 is full (3 text colours
// + 4 tier icons x 3 colours fill all 16 slots).
static const u8 sAchievementsListIneligibleTextColors[3] = {
    TEXT_COLOR_TRANSPARENT, 2, 2
};

// Four 16x16 tier-medal icons, 3 colours each. The 12 slots don't fit bank 1's
// 8 free entries (src/achievement_icons.c), so they load into WIN_LIST's bank 2
// after its 3 text colours. Same nibble remap as AchievementIcons_Load, kept
// local because that module has a single palette budget.
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

// Every per-open buffer lives in one ~9.7 KB heap block: AllocZeroed'd in
// CB2_InitAchievementsMenu case 0 and Free'd on every exit path
// (Task_AchievementsMenuCancel, Task_TierSelect_OpenBoostMenu), as
// src/ui_stat_editor.c's sStatEditorDataPtr. Declared here, where every size
// constant its fields depend on is in scope.
struct AchievementsMenuState
{
    u8 selectedTier;
    u16 listItemCount; // filtered count for the tier currently shown at LIST level
    u16 tierScrollOffset;
    u16 tierSelectedRow;
    u16 listScrollOffset;
    u16 listSelectedRow;
    // Id of the highlighted row in the on-screen list (tier id /
    // TIER_SELECT_ITEM_BOOSTS, or achievement id), set before ListMenuInit and
    // kept in sync by the moveCursorFunc callbacks. CURSOR_INVISIBLE makes text
    // colour the only selection marker, and itemPrintFunc is only given the
    // row's own id.
    u16 highlightedId;
    // Set by PrintAchievementDescription when the description on WIN_DESCRIPTION
    // needed its 2-line auto-scroll. MainCB2 uses it with
    // IsTextPrinterActiveOnWindow to loop the description after a pause. Cleared
    // by DestroyCurrentAchievementsList so LIST's TRUE does not restart a
    // printer against DETAIL's content.
    bool8 descriptionScrolling;
    // Frames since the printer went idle at the end of its scroll; reset on each
    // (re)start. MainCB2 waits ACHIEVEMENTS_DESC_RESTART_DELAY before looping.
    u16 descriptionRestartTimer;
    // Same pair for DETAIL, which prints into WIN_LIST rather than
    // WIN_DESCRIPTION. Also cleared when backing out of DETAIL (see
    // Task_Detail_ProcessInput), since DETAIL never calls
    // DestroyCurrentAchievementsList.
    bool8 detailDescriptionScrolling;
    u16 detailDescriptionRestartTimer;

    // Cached once per TIER SELECT build so itemPrintFunc does not rescan every achievement per redraw.
    struct
    {
        u16 completed;
        u16 total;
    } tierCounts[ACHIEVEMENT_TIER_COUNT];

// Backing storage for both lists; see ACHIEVEMENTS_MENU_LIST_CAPACITY.
    u8 listNameBuffers[ACHIEVEMENTS_MENU_LIST_CAPACITY][ACHIEVEMENTS_LIST_NAME_BUFFER_SIZE];
    struct ListMenuItem listItems[ACHIEVEMENTS_MENU_LIST_CAPACITY];

    // Dedicated description scratch; see ACHIEVEMENTS_DESC_BUFFER_SIZE.
    u8 descriptionBuffer[ACHIEVEMENTS_DESC_BUFFER_SIZE];
    u8 detailDescriptionBuffer[ACHIEVEMENTS_DESC_BUFFER_SIZE];

    // Four 16x16 tier-medal bitmaps remapped into WIN_LIST's palette bank (see LoadTierIcons).
    u8 tierIconPixels[ACHIEVEMENT_TIER_COUNT][TIER_ICON_BYTE_COUNT];
};

// NULL whenever this menu is closed. See struct AchievementsMenuState.
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

// Deduped from bg_main.png (a 720x160 mockup of three 240x160 screens) by a
// one-off tool; see src/ui_stat_editor.c for the loading pattern. TIER SELECT
// shows the "achievements" screen; LIST and DETAIL show "detail".
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

// bg1's WRAM tilemap buffer, allocated at CB2 init and reused by every
// LoadMenuBackground call. Freed in Task_AchievementsMenuCancel.
EWRAM_DATA static u8 *sAchievementsMenuBg1Tilemap = NULL;

static void MainCB2(void)
{
    RunTasks();
    // Both teardown paths (Task_AchievementsMenuCancel and
    // Task_TierSelect_OpenBoostMenu) run inside RunTasks above: they free the
    // windows, sAchievementsMenuBg1Tilemap and sAchievementsMenuStatePtr and swap
    // gMain.callback2, but this function keeps executing for the frame. Bail out
    // rather than dereference freed memory. A NULL state pointer reads the GBA BIOS
    // open-bus latch, not zeroes, so PrintAchievementDescription and
    // PrintDetailDescription would otherwise fire with a garbage id, blitting into
    // freed window buffers and copying from an out-of-bounds description pointer.
    // That was an intermittent permanent black screen on exit.
    if (sAchievementsMenuStatePtr == NULL)
        return;
    AnimateSprites();
    BuildOamBuffer();
    // Drives the WIN_DESCRIPTION scroll printer PrintAchievementDescription
    // registers for an overlong description; it advances one tick per call.
    RunTextPrinters();
    // Once that printer finishes, loop it back to the top after a short pause.
    // descriptionScrolling limits this to overlong descriptions actually using the
    // printer, so it cannot misfire against DETAIL's content or a description that
    // fit in 2 lines (TEXT_SKIP_DRAW).
    if (sAchievementsMenuStatePtr->descriptionScrolling
     && !IsTextPrinterActiveOnWindow(WIN_DESCRIPTION)
     && ++sAchievementsMenuStatePtr->descriptionRestartTimer >= ACHIEVEMENTS_DESC_RESTART_DELAY)
        PrintAchievementDescription(sAchievementsMenuStatePtr->highlightedId);
    // Same loop for DETAIL's printer (PrintDetailDescription, WIN_LIST).
    if (sAchievementsMenuStatePtr->detailDescriptionScrolling
     && !IsTextPrinterActiveOnWindow(WIN_LIST)
     && ++sAchievementsMenuStatePtr->detailDescriptionRestartTimer >= ACHIEVEMENTS_DESC_RESTART_DELAY)
        PrintDetailDescription(sAchievementsMenuStatePtr->highlightedId);
    // Flushes bg1's art tilemap, which LoadMenuBackground only schedules. Windows
    // copy to VRAM on their own.
    DoScheduledBgTilemapCopiesToVram();
    // LoadMenuBackground's DecompressAndCopyTileDataToVram parks its tiles in a heap
    // buffer that only frees once the DMA manager finishes, which is never true
    // when LoadMenuBackground returns. Draining here (as src/hall_of_fame.c does)
    // keeps each TIER SELECT <-> LIST swap from stranding a ~1 KB block and filling
    // the 32-slot buffer table.
    FreeTempTileDataBuffersIfPossible();
    UpdatePaletteFade();
}

static void VBlankCB(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

// Swaps bg1's art to the given screen. Called at CB2 init (via
// EnterTierSelectLevel) and on every TIER SELECT <-> LIST transition.
// sAchievementsMenuBg1Tilemap must already be allocated and set as bg1's
// tilemap buffer.
static void LoadMenuBackground(u8 screen)
{
    DecompressAndCopyTileDataToVram(1, sAchievementsMenuBgGfx[screen].tiles, 0, 0, 0);
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
        // AllocZeroed: the state lives on the heap only while this screen is open.
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
        // Must follow the LoadPalette above; appends the points icon's colours to that palette.
        AchievementIcons_Load(1);
        // Bank 2, not bank 1: see LoadTierIcons and WIN_LIST's .paletteNum comment.
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

    // TIER SELECT's art has no third box for WIN_DESCRIPTION, but LIST level's
    // AchievementsMenu_MoveCursorCallback prints to it on every cursor move.
    // Clear it, or backing out of LIST leaves the last description on screen.
    FillWindowPixelBuffer(WIN_DESCRIPTION, PIXEL_FILL(0));
    CopyWindowToVram(WIN_DESCRIPTION, COPYWIN_GFX);

    // Must be set before ListMenuInit: its first draw runs before
    // TierSelect_MoveCursorCallback fires (ListMenuInitInternal), so the initial
    // row would not be highlighted until the cursor moves.
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
    // No arrow cursor; the selected row highlights its own text (see sAchievementsListHighlightTextColors).
    template.cursorKind = CURSOR_INVISIBLE;

    gTasks[taskId].tListTaskId = ListMenuInit(&template, sAchievementsMenuStatePtr->tierScrollOffset, sAchievementsMenuStatePtr->tierSelectedRow);
    gTasks[taskId].tScrollArrowTaskId = AddScrollIndicatorArrowPairParameterized(
        SCROLL_ARROW_UP, ACHIEVEMENTS_ARROW_X, ACHIEVEMENTS_ARROW_TOP_Y, ACHIEVEMENTS_ARROW_BOTTOM_Y,
        // Clamped like EnterListLevel's scroll arrows: itemCount is 4 while the BOOSTS
        // row is hidden, and a bare subtraction would go negative.
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

    // With CURSOR_INVISIBLE, ListMenuChangeSelectionFull skips
    // ListMenuPrintEntries on a same-page move, but the selected row's text colour
    // marks it, so repaint explicitly on every selection change.
    //
    // Scrolling goes through the full RedrawListMenu since every visible row's
    // content changes. A same-page move changes only two rows' colour, so
    // RepaintListRow patches just those (avoiding the full redraw's flicker).
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

// Fades TIER SELECT out, loads LIST level's background/palette, and fades back
// in. See the header comment for why the swap happens while the screen is black.
static void Task_TierSelect_ToListLevel(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        sAchievementsMenuStatePtr->listScrollOffset = 0;
        sAchievementsMenuStatePtr->listSelectedRow = 0;
        DestroyCurrentAchievementsList(taskId);

        // Disabled around the swap: LoadPalette writes into gPlttBufferFaded, which
        // VBlankCB's TransferPlttBuffer DMAs to PLTT every frame. That buffer holds
        // faded-to-black values from the fade-out; if a vblank lands after
        // LoadPalette but before BeginNormalPaletteFade re-blends, full-brightness
        // LIST colours flash for a frame. Same idiom as src/party_menu.c and
        // src/pokedex.c.
        gPaletteFade.bufferTransferDisabled = TRUE;
        EnterListLevel(taskId, sAchievementsMenuStatePtr->selectedTier);
        BlendPalettes(PALETTES_ALL, 16, RGB_BLACK);
        gPaletteFade.bufferTransferDisabled = FALSE;

        BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
        gTasks[taskId].func = Task_List_ProcessInput;
    }
}

// Fades out, tears down this screen like Task_AchievementsMenuCancel, then jumps
// to the boost shop (src/achievement_boost_menu.c) with gMain.savedCallback
// repointed at CB2_InitAchievementsMenu, so the shop's [B] Back re-enters at a
// fresh TIER SELECT.
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
    // A same-page cursor move never reprints row text (CURSOR_INVISIBLE), so
    // Task_TierSelect_ProcessInput's RepaintListRow/RedrawListMenu, run after
    // ListMenu_ProcessInput returns, do the repaint (highlightedId is already
    // updated). Repainting here too flickered on every selection change.
}

// Icon at a fixed column (ACHIEVEMENTS_TIER_ICON_X); the count is right-aligned
// to the box edge, matching AchievementsMenu_DrawRow's points column.
static void TierSelect_DrawRow(u8 windowId, u32 tier, u8 y, const u8 *colors)
{
    u8 *ptr;
    s32 width;

    // The "BOOSTS" row (id TIER_SELECT_ITEM_BOOSTS) isn't a tier: tierCounts[] has
    // no entry for it, and it has no medal icon or count.
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

    // Recolours the row name ListMenuPrint draws after this returns. Must precede
    // TierSelect_DrawRow's BOOSTS-row early return, since that row is highlighted
    // too.
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

// OFF hides the shop, not just the toggle, so check both.
static bool8 IsBoostShopRowVisible(void)
{
    return Achievement_BoostsUnlocked() && Achievement_BoostsEnabled();
}

// TIER SELECT's header on one line: title, points summary, then the [B] BACK
// hint. Its art has no third box for the points summary (unlike DrawHeaderText,
// shared by LIST/DETAIL). The gap between title and hint is measured, and the
// points text narrows its font (GetFontIdToFit, as list_menu.c does) if
// FONT_NORMAL does not fit.
static void DrawTierSelectHeaderText(void)
{
    // FONT_NARROW leaves the points summary more room before it falls back through
    // GetFontIdToFit.
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

    // {available points}/{total points earned}. Until boosts unlock nothing can be
    // spent, so available == total; show the bare total instead.
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

    // Not ACHIEVEMENT_ICON_Y(0): its -1 would underflow a u16 at the header's y=0.
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

    // Must be set before ListMenuInit so the initial row is highlighted (see EnterTierSelectLevel).
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
    // 1/2, not 2/3: WIN_LIST is on its own palette bank (see the .paletteNum
    // comment on sAchievementsMenuWinTemplates), where white and dark gray sit at
    // indices 1 and 2.
    template.cursorPal = 1;
    template.fillValue = 0;
    template.cursorShadowPal = 2;
    template.lettersSpacing = 0;
    template.itemVerticalPadding = 0;
    template.scrollMultiple = LIST_NO_MULTIPLE_SCROLL;
    template.fontId = FONT_NORMAL;
    // No arrow cursor, as on TIER SELECT; also avoids the arrow overlapping row
    // text.
    template.cursorKind = CURSOR_INVISIBLE;

    gTasks[taskId].tListTaskId = ListMenuInit(&template, sAchievementsMenuStatePtr->listScrollOffset, sAchievementsMenuStatePtr->listSelectedRow);
    gTasks[taskId].tScrollArrowTaskId = AddScrollIndicatorArrowPairParameterized(
        SCROLL_ARROW_UP, ACHIEVEMENTS_ARROW_X, ACHIEVEMENTS_ARROW_TOP_Y, ACHIEVEMENTS_ARROW_BOTTOM_Y,
        // Clamped: a tier can have fewer than ACHIEVEMENTS_MENU_MAX_SHOWED
        // achievements. A negative threshold truncates into a huge u16
        // (ScrollIndicatorPair.fullyDownThreshold, src/list_menu.c:29), leaving the
        // down arrow stuck visible (same as src/achievement_boost_menu.c).
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

    // See Task_TierSelect_ProcessInput.
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

    // An overlong description's first screenful sometimes rendered in the orange
    // row-highlight colour. GenerateFontHalfRowLookupTable (src/text.c) builds one
    // global glyph colour table, regenerated only when a printer is registered or
    // hits a colour escape. On a cursor move, moveCursorFunc registers
    // WIN_DESCRIPTION's printer (white), then RepaintListRow regenerates the table
    // for the newly selected row's orange. The printer has drawn nothing yet
    // (RunTextPrinters runs after RunTasks), so it draws in orange.
    //
    // Regenerating the table for WIN_DESCRIPTION's colour here on every scrolling
    // frame makes it the last word. The function's unchanged-colour early-out
    // makes the extra call a no-op otherwise.
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

// Same bufferTransferDisabled guard as Task_TierSelect_ToListLevel, for the
// opposite swap.
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
    // Row repaint is Task_List_ProcessInput's job (see TierSelect_MoveCursorCallback).
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
    // The selection highlight wins over ineligibility; grey shows only when not
    // selected.
    bool8 ineligible = !Achievement_IsCompleted(achievementId) && !Achievement_IsEligible(achievementId);
    const u8 *colors = selected ? sAchievementsListHighlightTextColors
        : ineligible ? sAchievementsListIneligibleTextColors : sAchievementsListTextColors;

    // See TierSelect_ItemPrintCallback.
    if (selected || ineligible)
        ListMenuOverrideSetColors(colors[1], colors[0], colors[2]);

    AchievementsMenu_DrawRow(windowId, achievementId, y, colors);
}

// Builds the tier-filtered item list (skips ACHIEVEMENT_NONE and IDs outside
// this tier) with the completion checkbox baked into each label. Hidden
// achievements render as "???" until completed. Completion cannot change while
// the menu is open, so this runs once at tier entry.
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

        // Fail-safe truncate; the capacity is derived from ACHIEVEMENTS_COUNT.
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

// AddTextPrinterParameterized neither clips nor wraps, so unbounded text bleeds
// into the next row's tile memory. StripLineBreaks + BreakStringAutomatic (as
// src/achievement_popup.c) strips manual breaks and wraps cleanly.
//
// True for CHAR_PROMPT_SCROLL specifically, not CHAR_NEWLINE (which
// StringHasManualBreaks/CountLineBreaks in src/line_break.c treat as a scroll
// prompt). Lets PrintAchievementDescription tell a description that fits
// WIN_DESCRIPTION's 2 lines from one that does not (see BuildNewString's
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

// WIN_DESCRIPTION shows only 2 lines. SHOW_SCROLL_PROMPT makes
// BreakStringAutomatic insert CHAR_PROMPT_SCROLL, which scrolls the window up a
// line (RENDER_STATE_SCROLL, driven by RunTextPrinters from MainCB2).
// gTextFlags.autoScroll resolves the pause after NUM_FRAMES_AUTO_SCROLL_DELAY
// frames, since Up/Down move to another row instead of advancing.
//
// Text that fits in 2 lines gets no prompt and prints instantly
// (TEXT_SKIP_DRAW), avoiding the typing delay on every cursor move.
static void PrintAchievementDescription(s32 achievementId)
{
    FillWindowPixelBuffer(WIN_DESCRIPTION, PIXEL_FILL(0));
    // Cancel the previous row's printer, which would otherwise keep ticking against
    // the window just cleared.
    DeactivateSingleTextPrinter(WIN_DESCRIPTION, WINDOW_TEXT_PRINTER);

    if (achievementId >= ACHIEVEMENT_NONE + 1 && achievementId < ACHIEVEMENTS_COUNT)
    {
        const struct Achievement *info = Achievement_GetInfo(achievementId);
        bool8 masked = info->hidden && !Achievement_IsCompleted(achievementId);
        bool8 needsScroll;

        // descriptionBuffer, not gStringVar1; see ACHIEVEMENTS_DESC_BUFFER_SIZE.
        StringCopy(sAchievementsMenuStatePtr->descriptionBuffer, masked ? sText_HiddenDescription : info->description);
        StripLineBreaks(sAchievementsMenuStatePtr->descriptionBuffer);
        BreakStringAutomatic(sAchievementsMenuStatePtr->descriptionBuffer, ACHIEVEMENTS_DESC_MAX_WIDTH, 2, FONT_NORMAL, SHOW_SCROLL_PROMPT);
        needsScroll = StringHasScrollPrompt(sAchievementsMenuStatePtr->descriptionBuffer);

        gTextFlags.autoScroll = needsScroll;
        AddTextPrinterParameterized3(WIN_DESCRIPTION, FONT_NORMAL, 8, ACHIEVEMENTS_DESC_LINE1_Y, sAchievementsMenuTextColors,
            needsScroll ? GetPlayerTextSpeedDelay() : TEXT_SKIP_DRAW, sAchievementsMenuStatePtr->descriptionBuffer);

        // (Re)starting the printer always resets the idle timer (see MainCB2).
        sAchievementsMenuStatePtr->descriptionScrolling = needsScroll;
        sAchievementsMenuStatePtr->descriptionRestartTimer = 0;
    }
    CopyWindowToVram(WIN_DESCRIPTION, COPYWIN_GFX);
}

// ---- DETAIL ----------------------------------------------------------------

// Mirrors PrintAchievementDescription (auto-scroll then loop, via
// detailDescriptionScrolling) for DETAIL's WIN_LIST box. Split from
// EnterDetailLevel so MainCB2's restart loop reprints just this part.
static void PrintDetailDescription(s32 achievementId)
{
    const struct Achievement *info;
    bool8 masked;
    bool8 needsScroll;

    // Range guard: an out-of-range id would hand StringCopy arbitrary ROM words.
    if (achievementId < ACHIEVEMENT_NONE + 1 || achievementId >= ACHIEVEMENTS_COUNT)
        return;

    info = Achievement_GetInfo(achievementId);
    masked = info->hidden && !Achievement_IsCompleted(achievementId);

    FillWindowPixelBuffer(WIN_LIST, PIXEL_FILL(0));
    // Cancel the previous printer (see PrintAchievementDescription).
    DeactivateSingleTextPrinter(WIN_LIST, WINDOW_TEXT_PRINTER);

    // x matches ACHIEVEMENTS_LIST_ITEM_X, since LIST and DETAIL share one bg1
    // screen; x=8 pushed long text past the box's right edge.
    //
    // The name prints in the selection-highlight orange so it stands apart from
    // the white description. Always orange, even when ineligible, since DETAIL
    // shows the one selected achievement.
    AddTextPrinterParameterized3(WIN_LIST, FONT_NORMAL, ACHIEVEMENTS_LIST_ITEM_X, 1, sAchievementsListHighlightTextColors, TEXT_SKIP_DRAW, masked ? sText_HiddenName : info->name);

    // detailDescriptionBuffer, not gStringVar1: EnterDetailLevel reuses
    // gStringVar1 for the reward figure right after this returns, which would
    // clobber a needsScroll printer before its first frame.
    StringCopy(sAchievementsMenuStatePtr->detailDescriptionBuffer, masked ? sText_HiddenDescription : info->description);
    StripLineBreaks(sAchievementsMenuStatePtr->detailDescriptionBuffer);
    // ACHIEVEMENTS_DETAIL_DESC_MAX_WIDTH: WIN_LIST's box is narrower than
    // WIN_DESCRIPTION's. SHOW_SCROLL_PROMPT scrolls past
    // ACHIEVEMENTS_DETAIL_DESC_LINES lines instead of running past WIN_LIST's
    // bottom edge.
    BreakStringAutomatic(sAchievementsMenuStatePtr->detailDescriptionBuffer, ACHIEVEMENTS_DETAIL_DESC_MAX_WIDTH, ACHIEVEMENTS_DETAIL_DESC_LINES, FONT_NORMAL, SHOW_SCROLL_PROMPT);
    needsScroll = StringHasScrollPrompt(sAchievementsMenuStatePtr->detailDescriptionBuffer);

    gTextFlags.autoScroll = needsScroll;
    AddTextPrinterParameterized3(WIN_LIST, FONT_NORMAL, ACHIEVEMENTS_LIST_ITEM_X, 17, sAchievementsListTextColors,
        needsScroll ? GetPlayerTextSpeedDelay() : TEXT_SKIP_DRAW, sAchievementsMenuStatePtr->detailDescriptionBuffer);
    CopyWindowToVram(WIN_LIST, COPYWIN_GFX);

    // See PrintAchievementDescription.
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
    // Trails the figure, measured off the expanded string since the digit count
    // varies.
    AchievementIcons_Blit(ACHIEVEMENT_ICON_POINTS, WIN_DESCRIPTION, 8 + GetStringWidth(FONT_NORMAL, gStringVar4, 0) + 2, ACHIEVEMENT_ICON_Y(ACHIEVEMENTS_DESC_LINE1_Y));
    AddTextPrinterParameterized3(WIN_DESCRIPTION, FONT_NORMAL, 8, ACHIEVEMENTS_DESC_LINE2_Y, sAchievementsMenuTextColors, TEXT_SKIP_DRAW, completed ? sText_StatusCompleted : sText_StatusIncomplete);
    CopyWindowToVram(WIN_DESCRIPTION, COPYWIN_GFX);

    // Same fix as Task_List_ProcessInput, for DETAIL's printer. The reward/status
    // prints above regenerate the shared glyph colour table with
    // sAchievementsMenuTextColors (shadow index 6) instead of
    // sAchievementsListTextColors (shadow index 2), so a pending name/description
    // would draw with the wrong shadow.
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
        // DETAIL owns no ListMenuTask, so this transition never routes through
        // DestroyCurrentAchievementsList. Stop PrintDetailDescription's printer/loop
        // here, before EnterListLevel reclaims WIN_LIST; otherwise MainCB2 would
        // reprint DETAIL's text over the row list.
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
    // Called on every level transition and both exits. Deactivate a still-scrolling
    // WIN_DESCRIPTION printer so it does not tick against a window this screen no
    // longer owns, and clear gTextFlags.autoScroll so ordinary dialogue elsewhere
    // does not auto-advance.
    DeactivateSingleTextPrinter(WIN_DESCRIPTION, WINDOW_TEXT_PRINTER);
    gTextFlags.autoScroll = FALSE;
    // Stops MainCB2 from restarting a printer against a window this screen no
    // longer owns.
    sAchievementsMenuStatePtr->descriptionScrolling = FALSE;
    // Same for DETAIL's WIN_LIST printer. Redundant on the DETAIL -> LIST path
    // (Task_Detail_ProcessInput stops it), but every other transition routes here.
    DeactivateSingleTextPrinter(WIN_LIST, WINDOW_TEXT_PRINTER);
    sAchievementsMenuStatePtr->detailDescriptionScrolling = FALSE;
}

// Repaints one WIN_LIST row in place when the cursor moves without scrolling,
// used by Task_TierSelect_ProcessInput/Task_List_ProcessInput. Only the old and
// new highlighted rows differ (colour only), so repaint just those instead of
// RedrawListMenu's full clear-and-redraw, which flickered on plain up/down.
//
// Does not go through TierSelect_ItemPrintCallback/
// AchievementsMenu_ItemPrintCallback or ListMenuPrint: those arm
// gListMenuOverride, a single-slot global consumed by the engine's next
// ListMenuPrint, which would recolour an unrelated row. drawRow takes an
// explicit colours array and this function prints the name directly, so
// nothing is armed.
static void RepaintListRow(void (*drawRow)(u8, u32, u8, const u8 *), u32 arrayIndex, u8 y)
{
    const struct ListMenuItem *item = &sAchievementsMenuStatePtr->listItems[arrayIndex];
    bool8 selected = (item->id == sAchievementsMenuStatePtr->highlightedId);
    // drawRow == AchievementsMenu_DrawRow specifically: TierSelect_DrawRow's item->id
    // is a tier id (or TIER_SELECT_ITEM_BOOSTS), not an achievement id, and must
    // never reach Achievement_IsCompleted/_IsEligible. Selection still wins over
    // ineligibility (see AchievementsMenu_ItemPrintCallback).
    bool8 ineligible = (drawRow == AchievementsMenu_DrawRow)
        && !Achievement_IsCompleted(item->id) && !Achievement_IsEligible(item->id);
    const u8 *colors = selected ? sAchievementsListHighlightTextColors
        : ineligible ? sAchievementsListIneligibleTextColors : sAchievementsListTextColors;

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
