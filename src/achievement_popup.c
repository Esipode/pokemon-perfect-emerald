#include "global.h"
#include "achievements.h"
#include "achievement_popup.h"
#include "line_break.h"
#include "menu.h"
#include "palette.h"
#include "string_util.h"
#include "task.h"
#include "text.h"
#include "text_window.h"
#include "window.h"

// ---- Stage 4.1 (design doc §4.1, revised during implementation) -----------
// Reuses src/overworld.c's ScriptShowItemDescription/ShowItemIconSprite/
// ScriptHideItemDescription almost verbatim -- same window position/size
// (28x3 at tilemap (1,1)), same custom frame tile/palette (0x214/14), same
// icon-on-the-left-text-on-the-right layout, no slide animation. Only the
// content differs (tier icon + achievement name/points/description instead
// of an item icon + its description), and show/hide is driven by a task
// timer instead of paired script commands, since nothing here has a script
// context to call an explicit "hide" at the right moment.
//
// Still not here:
//   - the tier icon sprite itself. The layout reserves its slot
//     (ACHIEVEMENT_POPUP_ICON_X/Y, sized like AddItemIconSprite's 32x32) and
//     the two spots it needs wiring (see the TODOs in
//     ShowAchievementPopUpWindow/HideAchievementPopUpWindow), but there's no
//     tier icon art or an equivalent of gItemsInfo[].iconPic in this repo --
//     to be added separately.
//   - a real ring buffer for back-to-back awards (Stage 4.2). For now a
//     second call while one's showing just swaps the displayed content and
//     restarts the display timer -- there's no animation to hurry through
//     since there's no slide, so this is simpler than 4.1's original
//     map_name_popup.c-style re-entrancy, not a queue.
//   - PlayFanfare(MUS_OBTAIN_SYMBOL) and suppressing the popup during
//     battles/cutscenes (Stage 4.2).
//   - QueueAchievementNotification in src/achievements.c does not call this
//     yet -- that hookup is Stage 4.2's, once the real queue exists.

#define ACHIEVEMENT_POPUP_DISPLAY_FRAMES 150 // ~2.5 seconds

#define tTimer data[0]

#define ACHIEVEMENT_POPUP_TILEMAP_LEFT 1
#define ACHIEVEMENT_POPUP_TILEMAP_TOP  1
#define ACHIEVEMENT_POPUP_WIDTH        28
#define ACHIEVEMENT_POPUP_HEIGHT       3
#define ACHIEVEMENT_POPUP_TEXT_PAL     15
// Same custom frame tile/palette as ScriptShowItemDescription's item box.
#define ACHIEVEMENT_POPUP_FRAME_TILE   0x214
#define ACHIEVEMENT_POPUP_FRAME_PAL    14
// 28x3 = 84 tiles of window pixel data, so this reserves bg-0 tiles
// BASE_BLOCK .. BASE_BLOCK+0x53. It has to clear two separate things:
//
//   - the other bg-0 overworld popups that can be onscreen alongside the field
//     at the same time as this one (map name popup: 0x107, its secondary
//     window: 0x161, start menu: 0x139, money box/safari balls/pyramid floor:
//     0x141, ScriptShowItemDescription's own box and the field message box:
//     0x8, the standard text box: 0x194 + 0x6C = ..0x1FF), and
//   - the window *frame* tile regions reserved in include/menu.h:
//     DLG_WINDOW_BASE_TILE_NUM 0x200 (message box gfx, 0x1C0 bytes = 14 tiles
//     -> 0x200..0x20D) and STD_WINDOW_BASE_TILE_NUM 0x214 (frame gfx, 0x120
//     bytes = 9 tiles -> 0x214..0x21C), which is what the frame below is
//     drawn from.
//
// The second of those is easy to miss: sitting at 0x200 put the window's own
// pixel data straight on top of both frame regions, so CopyWindowToVram
// overwrote the frame tiles with flat fill right after they were loaded --
// the border kept its palette but lost its pattern, rendering as a solid
// block that still changed color with the Frame Type option.
//
// 0x220 sits just past the frame regions, and bg 0 has room up to 0x300
// (charBaseIndex 2 = VRAM 0x8000, first screen base 29 = VRAM 0xE800).
#define ACHIEVEMENT_POPUP_BASE_BLOCK   0x220

// Same offsets as ITEM_ICON_X/Y in src/overworld.c.
#define ACHIEVEMENT_POPUP_ICON_X 26
#define ACHIEVEMENT_POPUP_ICON_Y 24
#define ACHIEVEMENT_POPUP_TEXT_X (ACHIEVEMENT_POPUP_ICON_X + 2)
#define ACHIEVEMENT_POPUP_TEXT_Y 0
#define ACHIEVEMENT_POPUP_DESC_MAX_WIDTH 196 // matches ScriptShowItemDescription's own wrap width

EWRAM_DATA static u8 sAchievementPopupTaskId = 0;
EWRAM_DATA static u8 sAchievementPopupWindowId = 0;
// EWRAM's .sbss only allows zero initializers, so this can't default to
// WINDOW_NONE/TASK_NONE the way a non-EWRAM sentinel would -- this flag is
// the actual source of truth for whether a popup is currently up, rather
// than comparing sAchievementPopupWindowId/TaskId against a sentinel value.
EWRAM_DATA static bool8 sAchievementPopupActive = FALSE;

static void Task_HideAchievementPopupAfterDelay(u8 taskId);
static void ShowAchievementPopUpWindow(u16 achievementId);
static void HideAchievementPopUpWindow(void);

static const u8 sText_AchievementPopupFormat[] = _("{STR_VAR_2} (+{STR_VAR_1})\n{STR_VAR_3}");

void ShowAchievementPopup(u16 achievementId)
{
    if (sAchievementPopupActive)
    {
        gTasks[sAchievementPopupTaskId].tTimer = 0;
    }
    else
    {
        sAchievementPopupTaskId = CreateTask(Task_HideAchievementPopupAfterDelay, 90);
    }

    // Reads sAchievementPopupActive itself to decide whether to create the
    // window/frame or just refresh the content of one that's already up.
    ShowAchievementPopUpWindow(achievementId);
    sAchievementPopupActive = TRUE;
}

static void Task_HideAchievementPopupAfterDelay(u8 taskId)
{
    if (++gTasks[taskId].tTimer > ACHIEVEMENT_POPUP_DISPLAY_FRAMES)
    {
        HideAchievementPopUpWindow();
        DestroyTask(taskId);
    }
}

static void ShowAchievementPopUpWindow(u16 achievementId)
{
    const struct Achievement *info = Achievement_GetInfo(achievementId);

    if (!sAchievementPopupActive)
    {
        struct WindowTemplate template;

        SetWindowTemplateFields(&template, 0, ACHIEVEMENT_POPUP_TILEMAP_LEFT, ACHIEVEMENT_POPUP_TILEMAP_TOP,
            ACHIEVEMENT_POPUP_WIDTH, ACHIEVEMENT_POPUP_HEIGHT, ACHIEVEMENT_POPUP_TEXT_PAL, ACHIEVEMENT_POPUP_BASE_BLOCK);
        sAchievementPopupWindowId = AddWindow(&template);

        // ScriptShowItemDescription never loads the frame gfx itself (its
        // SetStandardWindowBorderStyle call only *draws*, see src/menu.c:361)
        // -- it only looks right because it always runs next to an actual
        // msgbox script command, and the script engine's message box loads
        // the player's current Frame Type tiles into STD_WINDOW_BASE_TILE_NUM
        // / STD_WINDOW_PALETTE_NUM (== ACHIEVEMENT_POPUP_FRAME_TILE/_PAL) as
        // a side effect. An achievement can complete with no message box
        // anywhere in sight, so this has to load them itself.
        //
        // Using the OnBg variant rather than LoadMessageBoxAndBorderGfx()
        // (src/menu.c:223) because that one resolves its target bg through
        // GetWindowAttribute(0, WINDOW_BG) -- wherever window ID 0 happens to
        // be -- which is only reliably bg 0 while a field message box (which
        // *is* window 0) is up. Naming the bg directly drops that assumption.
        LoadUserWindowBorderGfxOnBg(0, ACHIEVEMENT_POPUP_FRAME_TILE, BG_PLTT_ID(ACHIEVEMENT_POPUP_FRAME_PAL));
        DrawStdFrameWithCustomTileAndPalette(sAchievementPopupWindowId, FALSE, ACHIEVEMENT_POPUP_FRAME_TILE, ACHIEVEMENT_POPUP_FRAME_PAL);
    }
    else
    {
        FillWindowPixelBuffer(sAchievementPopupWindowId, PIXEL_FILL(1));
        // TODO: if a tier icon sprite already exists from the previous
        // achievement, destroy it here before the new one's created below
        // (mirrors DestroyItemIconSprite in src/overworld.c).
    }

    // TODO: create the tier icon sprite here, positioned at
    // (ACHIEVEMENT_POPUP_ICON_X, ACHIEVEMENT_POPUP_ICON_Y) -- mirrors
    // ShowItemIconSprite in src/overworld.c.

    ConvertIntToDecimalStringN(gStringVar1, info->points, STR_CONV_MODE_LEFT_ALIGN, 5);
    StringCopy(gStringVar2, info->name);
    StringCopy(gStringVar3, info->description);
    StripLineBreaks(gStringVar3);
    BreakStringAutomatic(gStringVar3, ACHIEVEMENT_POPUP_DESC_MAX_WIDTH, 1, FONT_SMALL, HIDE_SCROLL_PROMPT);
    StringExpandPlaceholders(gStringVar4, sText_AchievementPopupFormat);
    AddTextPrinterParameterized(sAchievementPopupWindowId, FONT_SMALL, gStringVar4, ACHIEVEMENT_POPUP_TEXT_X, ACHIEVEMENT_POPUP_TEXT_Y, TEXT_SKIP_DRAW, NULL);

    CopyWindowToVram(sAchievementPopupWindowId, COPYWIN_FULL);
}

static void HideAchievementPopUpWindow(void)
{
    // TODO: destroy the tier icon sprite here (mirrors DestroyItemIconSprite
    // in src/overworld.c), once one exists.

    ClearStdWindowAndFrameToTransparent(sAchievementPopupWindowId, TRUE);
    RemoveWindow(sAchievementPopupWindowId);
    sAchievementPopupActive = FALSE;
}
