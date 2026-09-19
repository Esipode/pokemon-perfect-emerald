#include "global.h"
#include "achievements.h"
#include "achievement_popup.h"
#include "constants/songs.h"
#include "decompress.h"
#include "field_message_box.h"
#include "line_break.h"
#include "main.h"
#include "map_name_popup.h"
#include "menu.h"
#include "overworld.h"
#include "palette.h"
#include "script.h"
#include "sound.h"
#include "sprite.h"
#include "string_util.h"
#include "task.h"
#include "text.h"
#include "text_window.h"
#include "window.h"

// Layout mirrors src/overworld.c's ScriptShowItemDescription box, with a tier
// icon and achievement text. Show/hide is driven by a task timer since there
// is no script context to call a hide.
//
// Awards go through a ring buffer drained by AchievementPopup_UpdateQueue,
// polled per frame from CB2_Overworld. It is a poll rather than a task because
// ResetTasks() at battle start/end and most menu transitions would destroy a
// drain task while it waits for a safe frame.
//
// Popups lock player field controls for their lifetime, as battle intros and
// cable club links do.

// Grace period before a button press can dismiss the popup, so the press that
// triggered the achievement does not instantly close it.
#define ACHIEVEMENT_POPUP_DISMISS_DELAY_FRAMES 30 // ~0.5 seconds

// Idle time on an overlong description's last screenful before it loops back
// to the top. Same value as src/achievements_menu.c's ACHIEVEMENTS_DESC_RESTART_DELAY.
#define ACHIEVEMENT_POPUP_DESC_RESTART_DELAY 120 // ~2 seconds

#define tTimer data[0]

#define ACHIEVEMENT_POPUP_TILEMAP_LEFT 1
#define ACHIEVEMENT_POPUP_TILEMAP_TOP  1
#define ACHIEVEMENT_POPUP_WIDTH        28
// FONT_SMALL lines are 12px, so 24px fits the name/points row plus a single
// description line.
#define ACHIEVEMENT_POPUP_HEIGHT       3
#define ACHIEVEMENT_POPUP_TEXT_PAL     15
// Same frame tile/palette as ScriptShowItemDescription.
#define ACHIEVEMENT_POPUP_FRAME_TILE   0x214
#define ACHIEVEMENT_POPUP_FRAME_PAL    14
// 28x3 = 84 tiles of window pixel data at bg-0 tiles BASE_BLOCK..+0x53.
// Must clear the other bg-0 overworld popups (map name 0x107, its secondary
// window 0x161, start menu 0x139, money box/safari balls/pyramid floor 0x141,
// item description box and field message box 0x8, standard text box
// 0x194..0x1FF) and the frame tile regions in include/menu.h
// (DLG_WINDOW_BASE_TILE_NUM 0x200..0x20D, STD_WINDOW_BASE_TILE_NUM 0x214..0x21C).
// Overlapping the frame regions overwrites the border pattern with flat fill.
// bg 0 has room up to 0x300.
#define ACHIEVEMENT_POPUP_BASE_BLOCK   0x220

// ITEM_ICON_X/Y from src/overworld.c. These are a sprite's x2/y2, which is the
// sprite's CENTER, not its top-left (oam.x = x + x2 - halfWidth). The old
// 32x32 tier icon held 24x24 of visible content top-left-aligned, putting its
// visible corner at (26 - 16, 24 - 16) = (10, 8) in OAM coordinates.
#define ACHIEVEMENT_POPUP_ICON_SLOT_CENTER_X 26
#define ACHIEVEMENT_POPUP_ICON_SLOT_CENTER_Y 24
#define ACHIEVEMENT_POPUP_ICON_SLOT_SPRITE_SIZE 32 // Old sprite shape, for its halfWidth
#define ACHIEVEMENT_POPUP_ICON_SLOT_SIZE 24 // Old visible content size; the box the icon is centered in

#define ACHIEVEMENT_TIER_ICON_SIZE 16

#define ACHIEVEMENT_POPUP_ICON_SLOT_LEFT (ACHIEVEMENT_POPUP_ICON_SLOT_CENTER_X - ACHIEVEMENT_POPUP_ICON_SLOT_SPRITE_SIZE / 2)
#define ACHIEVEMENT_POPUP_ICON_SLOT_TOP  (ACHIEVEMENT_POPUP_ICON_SLOT_CENTER_Y - ACHIEVEMENT_POPUP_ICON_SLOT_SPRITE_SIZE / 2)

// Centers the 16x16 sprite in the 24x24 box, converted to the center
// coordinate x2/y2 needs.
#define ACHIEVEMENT_POPUP_ICON_X (ACHIEVEMENT_POPUP_ICON_SLOT_LEFT + (ACHIEVEMENT_POPUP_ICON_SLOT_SIZE - ACHIEVEMENT_TIER_ICON_SIZE) / 2 + ACHIEVEMENT_TIER_ICON_SIZE / 2)
#define ACHIEVEMENT_POPUP_ICON_Y (ACHIEVEMENT_POPUP_ICON_SLOT_TOP + (ACHIEVEMENT_POPUP_ICON_SLOT_SIZE - ACHIEVEMENT_TIER_ICON_SIZE) / 2 + ACHIEVEMENT_TIER_ICON_SIZE / 2)

// Window-local pixel coordinate, independent of the sprite math above.
#define ACHIEVEMENT_POPUP_TEXT_X (ACHIEVEMENT_POPUP_ICON_SLOT_CENTER_X + 2)
#define ACHIEVEMENT_POPUP_TEXT_Y 0
#define ACHIEVEMENT_POPUP_DESC_MAX_WIDTH 196 // ScriptShowItemDescription wrap width

// No tag registry exists, so this only needs to avoid collisions in practice.
#define ACHIEVEMENT_POPUP_ICON_TAG 0xACE1

// Drops on overflow. Safe because Achievement_TryComplete commits the flag and
// points before enqueueing, so a dropped entry only misses the toast.
#define ACHIEVEMENT_POPUP_QUEUE_SIZE 8

// Overlong descriptions auto-scroll (SHOW_SCROLL_PROMPT) instead of printing
// past the window's pixel buffer. Read by Task_WaitAchievementPopupDismiss to
// delay the dismiss grace period until every line was seen, and to loop the
// scroll back to the top.
EWRAM_DATA static bool8 sAchievementPopupNeedsScroll = FALSE;

// Frames since an overlong description's printer went idle at the end of its
// scroll. Reset on every (re)start of the print.
EWRAM_DATA static u16 sAchievementPopupDescRestartTimer = 0;

// Content on display: an achievement (value is the id) or a level cap
// increase (value is the new cap).
enum PopupKind
{
    POPUP_KIND_ACHIEVEMENT,
    POPUP_KIND_LEVEL_CAP,
};

// Value on display, so the dismiss task can reprint it for the loop restart.
EWRAM_DATA static u16 sAchievementPopupCurrentId = 0;
EWRAM_DATA static u8 sAchievementPopupCurrentKind = POPUP_KIND_ACHIEVEMENT;

// Latches TRUE once the initial grace period passes, so a loop restart does not
// make an already-dismissible popup undismissable. Reset when new content is
// shown.
EWRAM_DATA static bool8 sAchievementPopupDismissible = FALSE;

// Not gStringVar4: an overlong popup's printer reads the buffer across many
// frames, during which anything else may write to gStringVar4.
#define ACHIEVEMENT_POPUP_TEXT_BUFFER_SIZE 0x100
EWRAM_DATA static u8 sAchievementPopupTextBuffer[ACHIEVEMENT_POPUP_TEXT_BUFFER_SIZE] = {0};

EWRAM_DATA static u8 sAchievementPopupTaskId = 0;
EWRAM_DATA static u8 sAchievementPopupWindowId = 0;
EWRAM_DATA static u8 sAchievementPopupIconSpriteId = 0;
// EWRAM .sbss only allows zero initializers, so this flag, not a sentinel id,
// says whether the ids above are meaningful.
EWRAM_DATA static bool8 sAchievementPopupActive = FALSE;

// RunTasks() in OverworldBasic() can release another field lock and tear down a
// menu's windows earlier in the same CB2_Overworld tick. Requiring the field
// to have been safe on the previous frame too avoids drawing over that
// teardown.
EWRAM_DATA static bool8 sAchievementPopupWasSafeLastFrame = FALSE;

// Ring buffer indexed from Head + Count. Entries carry a kind so achievements
// and level cap increases share one queue.
struct PopupQueueEntry
{
    u8 kind; // enum PopupKind
    u16 value; // achievement id, or new level cap
};
EWRAM_DATA static struct PopupQueueEntry sAchievementPopupQueue[ACHIEVEMENT_POPUP_QUEUE_SIZE] = {0};
EWRAM_DATA static u8 sAchievementPopupQueueHead = 0;
EWRAM_DATA static u8 sAchievementPopupQueueCount = 0;

static void Task_WaitAchievementPopupDismiss(u8 taskId);
static bool8 IsAchievementPopupSafeToShow(void);
static void ShowPopupCommon(enum PopupKind kind, u16 value);
static void EnqueuePopup(enum PopupKind kind, u16 value);
static void ShowAchievementPopUpWindow(enum PopupKind kind, u16 value);
static void PrintAchievementPopupText(enum PopupKind kind, u16 value);
static void HideAchievementPopUpWindow(void);
static u8 AddAchievementTierIconSprite(enum AchievementTier tier);
static void DestroyAchievementTierIconSprite(u8 spriteId);
static bool8 StringHasScrollPrompt(const u8 *str);

static const u8 sText_AchievementPopupFormat[] = _("{STR_VAR_2} (+{STR_VAR_1})\n{STR_VAR_3}");
static const u8 sText_LevelCapPopupFormat[] = _("The level cap has increased!\nLv {STR_VAR_1}");

// Tier icons are plain 16x16 (2x2 tile) sprites, so LoadCompressedSpriteSheet
// suffices without item_icon.c's scratch-buffer padding.
static const u32 sAchievementTierIconGfx_Bronze[]  = INCGFX_U32("graphics/achievements/icons/star_bronze.png", ".4bpp.smol");
static const u16 sAchievementTierIconPal_Bronze[]  = INCGFX_U16("graphics/achievements/icons/star_bronze.png", ".gbapal");
static const u32 sAchievementTierIconGfx_Silver[]  = INCGFX_U32("graphics/achievements/icons/star_silver.png", ".4bpp.smol");
static const u16 sAchievementTierIconPal_Silver[]  = INCGFX_U16("graphics/achievements/icons/star_silver.png", ".gbapal");
static const u32 sAchievementTierIconGfx_Gold[]    = INCGFX_U32("graphics/achievements/icons/star_gold.png", ".4bpp.smol");
static const u16 sAchievementTierIconPal_Gold[]    = INCGFX_U16("graphics/achievements/icons/star_gold.png", ".gbapal");
static const u32 sAchievementTierIconGfx_Diamond[] = INCGFX_U32("graphics/achievements/icons/star_diamond.png", ".4bpp.smol");
static const u16 sAchievementTierIconPal_Diamond[] = INCGFX_U16("graphics/achievements/icons/star_diamond.png", ".gbapal");

static const u32 *const sAchievementTierIconGfx[ACHIEVEMENT_TIER_COUNT] =
{
    [ACHIEVEMENT_TIER_BRONZE]  = sAchievementTierIconGfx_Bronze,
    [ACHIEVEMENT_TIER_SILVER]  = sAchievementTierIconGfx_Silver,
    [ACHIEVEMENT_TIER_GOLD]    = sAchievementTierIconGfx_Gold,
    [ACHIEVEMENT_TIER_DIAMOND] = sAchievementTierIconGfx_Diamond,
};

static const u16 *const sAchievementTierIconPal[ACHIEVEMENT_TIER_COUNT] =
{
    [ACHIEVEMENT_TIER_BRONZE]  = sAchievementTierIconPal_Bronze,
    [ACHIEVEMENT_TIER_SILVER]  = sAchievementTierIconPal_Silver,
    [ACHIEVEMENT_TIER_GOLD]    = sAchievementTierIconPal_Gold,
    [ACHIEVEMENT_TIER_DIAMOND] = sAchievementTierIconPal_Diamond,
};

// Tag hardcoded: the popup only ever needs one.
static const struct OamData sOamData_AchievementTierIcon =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(16x16),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(16x16),
    .tileNum = 0,
    .priority = 1,
    .paletteNum = 2,
    .affineParam = 0
};

static const union AnimCmd sSpriteAnim_AchievementTierIcon[] =
{
    ANIMCMD_FRAME(0, 0),
    ANIMCMD_END
};

static const union AnimCmd *const sSpriteAnimTable_AchievementTierIcon[] =
{
    sSpriteAnim_AchievementTierIcon
};

static const struct SpriteTemplate sAchievementTierIconSpriteTemplate =
{
    .tileTag = ACHIEVEMENT_POPUP_ICON_TAG,
    .paletteTag = ACHIEVEMENT_POPUP_ICON_TAG,
    .oam = &sOamData_AchievementTierIcon,
    .anims = sSpriteAnimTable_AchievementTierIcon,
};

void ShowAchievementPopup(u16 achievementId)
{
    ShowPopupCommon(POPUP_KIND_ACHIEVEMENT, achievementId);
}

// Called directly only by the debug menu; real increases use LevelCapPopup_Enqueue.
void ShowLevelCapPopup(u32 newLevelCap)
{
    ShowPopupCommon(POPUP_KIND_LEVEL_CAP, newLevelCap);
}

static void ShowPopupCommon(enum PopupKind kind, u16 value)
{
    if (kind == POPUP_KIND_LEVEL_CAP)
        PlaySE(SE_EXP_MAX);
    else
        PlayFanfare(MUS_OBTAIN_SYMBOL);

    if (sAchievementPopupActive)
    {
        gTasks[sAchievementPopupTaskId].tTimer = 0;
    }
    else
    {
        sAchievementPopupTaskId = CreateTask(Task_WaitAchievementPopupDismiss, 90);
    }

    // Reads sAchievementPopupActive itself to decide whether to create the
    // window/frame or just refresh the content of one that's already up.
    ShowAchievementPopUpWindow(kind, value);
    sAchievementPopupActive = TRUE;
}

// Entry point for real awards.
void AchievementPopup_Enqueue(u16 achievementId)
{
    EnqueuePopup(POPUP_KIND_ACHIEVEMENT, achievementId);
}

// Queued rather than shown directly: a level cap increase can land mid-script
// (Common_EventScript_CheckLevelCapIncrease), and must wait for the field to be
// free of any message box or cutscene.
void LevelCapPopup_Enqueue(u32 newLevelCap)
{
    EnqueuePopup(POPUP_KIND_LEVEL_CAP, newLevelCap);
}

static void EnqueuePopup(enum PopupKind kind, u16 value)
{
    u8 tail;

    if (sAchievementPopupQueueCount >= ACHIEVEMENT_POPUP_QUEUE_SIZE)
        return;

    tail = (sAchievementPopupQueueHead + sAchievementPopupQueueCount) % ACHIEVEMENT_POPUP_QUEUE_SIZE;
    sAchievementPopupQueue[tail].kind = kind;
    sAchievementPopupQueue[tail].value = value;
    sAchievementPopupQueueCount++;
}

// Dismissal needs a new A/B press, once safe to treat as intentional:
//   1. An auto-scrolling description must finish its first pass, otherwise the
//      press would also advance the text printer (src/text.c).
//   2. ACHIEVEMENT_POPUP_DISMISS_DELAY_FRAMES then absorbs the triggering press.
//   3. sAchievementPopupDismissible latches so a loop restart does not re-arm
//      step 1.
//
// An overlong description pauses for ACHIEVEMENT_POPUP_DESC_RESTART_DELAY, then
// loops back to the top for as long as the popup stays up, dismissible or not.
//
// Reads JOY_NEW directly rather than TextPrinterWait, which folds in
// FLAG_AUTO_SCROLL_TEXT and would auto-close the popup. JOY_NEW fires only on
// press, so a held-over A does not dismiss until released and pressed again.
static void Task_WaitAchievementPopupDismiss(u8 taskId)
{
    // Nothing else on the field runs RunTextPrinters() for a long description
    // auto-scrolling here. No-op when no printer is active.
    RunTextPrinters();

    // Loop an overlong description back to the top after it sits idle.
    if (sAchievementPopupNeedsScroll
     && !IsTextPrinterActiveOnWindow(sAchievementPopupWindowId)
     && ++sAchievementPopupDescRestartTimer >= ACHIEVEMENT_POPUP_DESC_RESTART_DELAY)
    {
        PrintAchievementPopupText(sAchievementPopupCurrentKind, sAchievementPopupCurrentId);
    }

    // Once latched, skip to the input check even while a loop restart scrolls.
    if (!sAchievementPopupDismissible)
    {
        // Hold the grace period at zero while the first scroll pass runs.
        if (sAchievementPopupNeedsScroll && IsTextPrinterActiveOnWindow(sAchievementPopupWindowId))
        {
            gTasks[taskId].tTimer = 0;
            return;
        }

        // Still in the grace period; ignore input.
        if (gTasks[taskId].tTimer < ACHIEVEMENT_POPUP_DISMISS_DELAY_FRAMES)
        {
            gTasks[taskId].tTimer++;
            return;
        }

        sAchievementPopupDismissible = TRUE;
    }

    // A/B only, so a D-pad tap or Start/Select does not close the popup.
    if (JOY_NEW(A_BUTTON | B_BUTTON))
    {
        PlaySE(SE_SELECT);
        HideAchievementPopUpWindow();
        DestroyTask(taskId);
    }
}

void AchievementPopup_UpdateQueue(void)
{
    struct PopupQueueEntry entry;
    bool8 safeNow = IsAchievementPopupSafeToShow();
    bool8 safeLastFrame = sAchievementPopupWasSafeLastFrame;
    sAchievementPopupWasSafeLastFrame = safeNow;

    if (sAchievementPopupQueueCount == 0)
        return;

    // Wait for the current popup to finish, and for the field to have been safe
    // for two frames.
    if (sAchievementPopupActive || !safeNow || !safeLastFrame)
        return;

    entry = sAchievementPopupQueue[sAchievementPopupQueueHead];
    sAchievementPopupQueueHead = (sAchievementPopupQueueHead + 1) % ACHIEVEMENT_POPUP_QUEUE_SIZE;
    sAchievementPopupQueueCount--;

    ShowPopupCommon(entry.kind, entry.value);
}

// The popup draws onto overworld bg 0 tiles/palette rows that only hold while
// CB2_Overworld runs. Mirrors the idle checks in src/overworld.c's
// Task_ShowRoamerMessageDelayed and src/dexnav.c.
static bool8 IsAchievementPopupSafeToShow(void)
{
    return (gMain.callback2 == CB2_Overworld
         && !ScriptContext_IsEnabled()
         && !ArePlayerFieldControlsLocked()
         && !gPaletteFade.active
         && IsFieldMessageBoxHidden()
         && !FuncIsActiveTask(Task_MapNamePopUpWindow));
}

static void ShowAchievementPopUpWindow(enum PopupKind kind, u16 value)
{
    if (!sAchievementPopupActive)
    {
        struct WindowTemplate template;

        SetWindowTemplateFields(&template, 0, ACHIEVEMENT_POPUP_TILEMAP_LEFT, ACHIEVEMENT_POPUP_TILEMAP_TOP,
            ACHIEVEMENT_POPUP_WIDTH, ACHIEVEMENT_POPUP_HEIGHT, ACHIEVEMENT_POPUP_TEXT_PAL, ACHIEVEMENT_POPUP_BASE_BLOCK);
        sAchievementPopupWindowId = AddWindow(&template);

        // ScriptShowItemDescription relies on the script message box to load
        // the frame gfx as a side effect. An achievement can complete with no
        // message box, so load it here. The OnBg variant is used because
        // LoadMessageBoxAndBorderGfx resolves its bg through window 0, which is
        // only bg 0 while a field message box is up.
        LoadUserWindowBorderGfxOnBg(0, ACHIEVEMENT_POPUP_FRAME_TILE, BG_PLTT_ID(ACHIEVEMENT_POPUP_FRAME_PAL));
        DrawStdFrameWithCustomTileAndPalette(sAchievementPopupWindowId, FALSE, ACHIEVEMENT_POPUP_FRAME_TILE, ACHIEVEMENT_POPUP_FRAME_PAL);

        // Paired with UnlockPlayerFieldControls() in HideAchievementPopUpWindow.
        // Fresh-show path only; a content swap is already locked.
        LockPlayerFieldControls();
    }
    else
    {
        DestroyAchievementTierIconSprite(sAchievementPopupIconSpriteId);
    }

    // Level cap increases have no tier icon.
    if (kind == POPUP_KIND_ACHIEVEMENT)
    {
        const struct Achievement *info = Achievement_GetInfo(value);
        sAchievementPopupIconSpriteId = AddAchievementTierIconSprite(info->tier);
        if (sAchievementPopupIconSpriteId != MAX_SPRITES)
        {
            gSprites[sAchievementPopupIconSpriteId].x2 = ACHIEVEMENT_POPUP_ICON_X;
            gSprites[sAchievementPopupIconSpriteId].y2 = ACHIEVEMENT_POPUP_ICON_Y;
            gSprites[sAchievementPopupIconSpriteId].oam.priority = 0;
        }
    }
    else
    {
        sAchievementPopupIconSpriteId = MAX_SPRITES;
    }

    // New content must re-earn dismissibility.
    // own comment.
    sAchievementPopupCurrentKind = kind;
    sAchievementPopupCurrentId = value;
    sAchievementPopupDismissible = FALSE;
    PrintAchievementPopupText(kind, value);
}

// Split out so the dismiss task can reprint to loop the scroll. Does not touch
// the icon sprite or sAchievementPopupDismissible.
static void PrintAchievementPopupText(enum PopupKind kind, u16 value)
{
    FillWindowPixelBuffer(sAchievementPopupWindowId, PIXEL_FILL(1));
    // Cancel any scroll printer still running on the window just cleared. No-op
    // on first show.
    DeactivateSingleTextPrinter(sAchievementPopupWindowId, WINDOW_TEXT_PRINTER);

    if (kind == POPUP_KIND_ACHIEVEMENT)
    {
        const struct Achievement *info = Achievement_GetInfo(value);
        ConvertIntToDecimalStringN(gStringVar1, info->points, STR_CONV_MODE_LEFT_ALIGN, 5);
        StringCopy(gStringVar2, info->name);
        StringCopy(gStringVar3, info->description);
        StripLineBreaks(gStringVar3);
        // SHOW_SCROLL_PROMPT lets an overlong description scroll;
        // StringHasScrollPrompt detects whether it did.
        BreakStringAutomatic(gStringVar3, ACHIEVEMENT_POPUP_DESC_MAX_WIDTH, 1, FONT_SMALL, SHOW_SCROLL_PROMPT);
        sAchievementPopupNeedsScroll = StringHasScrollPrompt(gStringVar3);

        StringExpandPlaceholders(sAchievementPopupTextBuffer, sText_AchievementPopupFormat);
    }
    else
    {
        // n=4 because the cap can reach 4 digits (MAX_LEVEL is 1000, and New Game
        // Plus adds ngpRuns * 75, src/caps.c); a 3-digit width prints "?" for the
        // overflowing digit.
        ConvertIntToDecimalStringN(gStringVar1, value, STR_CONV_MODE_LEFT_ALIGN, 4);
        sAchievementPopupNeedsScroll = FALSE;
        StringExpandPlaceholders(sAchievementPopupTextBuffer, sText_LevelCapPopupFormat);
    }

    // Only scrolling descriptions pay for the typing delay.
    gTextFlags.autoScroll = sAchievementPopupNeedsScroll;
    AddTextPrinterParameterized(sAchievementPopupWindowId, FONT_SMALL, sAchievementPopupTextBuffer, ACHIEVEMENT_POPUP_TEXT_X, ACHIEVEMENT_POPUP_TEXT_Y,
        sAchievementPopupNeedsScroll ? GetPlayerTextSpeedDelay() : TEXT_SKIP_DRAW, NULL);

    CopyWindowToVram(sAchievementPopupWindowId, COPYWIN_FULL);

    // Reset unconditionally so a stale count from a longer description cannot linger.
    sAchievementPopupDescRestartTimer = 0;
}

static void HideAchievementPopUpWindow(void)
{
    DestroyAchievementTierIconSprite(sAchievementPopupIconSpriteId);

    ClearStdWindowAndFrameToTransparent(sAchievementPopupWindowId, TRUE);
    RemoveWindow(sAchievementPopupWindowId);
    UnlockPlayerFieldControls();
    sAchievementPopupActive = FALSE;
    // gTextFlags.autoScroll is shared; clear it so it does not leak into the next
    // field message box.
    gTextFlags.autoScroll = FALSE;
    sAchievementPopupNeedsScroll = FALSE;
    sAchievementPopupDismissible = FALSE;
    sAchievementPopupDescRestartTimer = 0;
}

static u8 AddAchievementTierIconSprite(enum AchievementTier tier)
{
    struct CompressedSpriteSheet spriteSheet;
    struct SpritePalette spritePalette;

    spriteSheet.data = sAchievementTierIconGfx[tier];
    spriteSheet.size = ACHIEVEMENT_TIER_ICON_SIZE * ACHIEVEMENT_TIER_ICON_SIZE / 2; // 4bpp
    spriteSheet.tag = ACHIEVEMENT_POPUP_ICON_TAG;
    LoadCompressedSpriteSheet(&spriteSheet);

    spritePalette.data = sAchievementTierIconPal[tier];
    spritePalette.tag = ACHIEVEMENT_POPUP_ICON_TAG;
    LoadSpritePalette(&spritePalette);

    return CreateSprite(&sAchievementTierIconSpriteTemplate, 0, 0, 0);
}

static void DestroyAchievementTierIconSprite(u8 spriteId)
{
    FreeSpriteTilesByTag(ACHIEVEMENT_POPUP_ICON_TAG);
    FreeSpritePaletteByTag(ACHIEVEMENT_POPUP_ICON_TAG);
    if (spriteId != MAX_SPRITES)
    {
        FreeSpriteOamMatrix(&gSprites[spriteId]);
        DestroySprite(&gSprites[spriteId]);
    }
}

// True for CHAR_PROMPT_SCROLL specifically, not CHAR_NEWLINE.
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
