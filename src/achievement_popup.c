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
// ---- Stage 4.2 (design doc §4.2) -------------------------------------
// AchievementPopup_Enqueue() adds a small ring buffer in front of
// ShowAchievementPopup(): src/achievements.c's QueueAchievementNotification
// calls it instead of the popup directly, and AchievementPopup_UpdateQueue
// (polled once per frame from CB2_Overworld, src/overworld.c) shows one
// entry at a time, only once the previous popup has fully finished and the
// field is in a safe state (see IsAchievementPopupSafeToShow) -- simultaneous
// awards each get their own full display window instead of cutting each
// other short. ShowAchievementPopup itself is unchanged and still reachable
// directly for an ungated, immediate render (the debug menu's "Test
// Achievement Popup" action still uses it that way).
//
// ---- Bug fix (post-Stage 13) -----------------------------------------
// The queue used to be drained by a self-perpetuating task instead of a
// per-frame poll. That broke when an achievement was queued off the field
// (e.g. mid-battle): ResetTasks() runs unconditionally at battle start/end
// and in most other menu transitions, which silently destroyed the drain
// task while it was still waiting for a safe frame. The queued id itself
// survived (plain EWRAM ring buffer), but nothing was left to drain it until
// the next achievement re-armed the task and flushed both at once -- see
// AchievementPopup_UpdateQueue's comment for the fix.
//
// Also added: PlayFanfare(MUS_OBTAIN_SYMBOL) on every show, and
// Lock/UnlockPlayerFieldControls() around the popup's lifetime so the player
// can't walk through it -- the same mechanism battle intros and cable club
// links use (src/battle_setup.c, src/cable_club.c), instead of the debug
// menu's old ScriptContext_Enable() workaround (removed; see src/debug.c).

#define ACHIEVEMENT_POPUP_DISPLAY_FRAMES 150 // ~2.5 seconds

#define tTimer data[0]

#define ACHIEVEMENT_POPUP_TILEMAP_LEFT 1
#define ACHIEVEMENT_POPUP_TILEMAP_TOP  1
#define ACHIEVEMENT_POPUP_WIDTH        28
// 3 tiles/24px tall. FONT_SMALL's line height is exactly 12px with no extra
// lineSpacing (src/text.c's sFontInfos[FONT_SMALL]), so that's room for
// exactly 2 lines of text -- the name/points row (ACHIEVEMENT_POPUP_TEXT_Y)
// and a single line below it for the description. See
// sAchievementPopupNeedsScroll's own comment for what happens once a
// description needs more than that one line.
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

// ITEM_ICON_X/Y from src/overworld.c. Two things about these matter here:
//
// 1. They're a sprite's x2/y2, which this engine treats as the sprite's
//    CENTER, not its top-left corner (src/sprite.c:342: oam.x = sprite->x +
//    x2 + centerToCornerVecX, where centerToCornerVecX is -halfWidth, from
//    CalcCenterToCornerVec's sCenterToCornerVecTable). The old tier icon
//    sprite was 32x32 (halfWidth 16) with its visible 24x24 content
//    top-left-aligned inside that buffer, so its visible top-left corner
//    actually sat at (26 - 16, 24 - 16) = (10, 8) in absolute/OAM
//    coordinates, occupying a 24x24px box from there.
// 2. ACHIEVEMENT_POPUP_TEXT_X's "+2" below is unrelated to #1 -- it's a
//    window-local pixel coordinate (AddTextPrinterParameterized draws into
//    the window's own pixel buffer), a different coordinate space than the
//    sprite math entirely. Kept as a literal reference to the original
//    constant so reflowing the icon-centering math below can't silently
//    drag the text position along with it.
#define ACHIEVEMENT_POPUP_ICON_SLOT_CENTER_X 26
#define ACHIEVEMENT_POPUP_ICON_SLOT_CENTER_Y 24
#define ACHIEVEMENT_POPUP_ICON_SLOT_SPRITE_SIZE 32 // old sprite's own shape (for the halfWidth in #1)
#define ACHIEVEMENT_POPUP_ICON_SLOT_SIZE 24 // old sprite's visible content size -- the box being centered within below

#define ACHIEVEMENT_TIER_ICON_SIZE 16

#define ACHIEVEMENT_POPUP_ICON_SLOT_LEFT (ACHIEVEMENT_POPUP_ICON_SLOT_CENTER_X - ACHIEVEMENT_POPUP_ICON_SLOT_SPRITE_SIZE / 2)
#define ACHIEVEMENT_POPUP_ICON_SLOT_TOP  (ACHIEVEMENT_POPUP_ICON_SLOT_CENTER_Y - ACHIEVEMENT_POPUP_ICON_SLOT_SPRITE_SIZE / 2)

// Centers the new, smaller, no-longer-padded 16x16 sprite within that same
// 24x24 box: box corner + half the leftover space to get the new sprite's
// own top-left, then + its own halfWidth to convert back to the center
// coordinate x2/y2 actually needs.
#define ACHIEVEMENT_POPUP_ICON_X (ACHIEVEMENT_POPUP_ICON_SLOT_LEFT + (ACHIEVEMENT_POPUP_ICON_SLOT_SIZE - ACHIEVEMENT_TIER_ICON_SIZE) / 2 + ACHIEVEMENT_TIER_ICON_SIZE / 2)
#define ACHIEVEMENT_POPUP_ICON_Y (ACHIEVEMENT_POPUP_ICON_SLOT_TOP + (ACHIEVEMENT_POPUP_ICON_SLOT_SIZE - ACHIEVEMENT_TIER_ICON_SIZE) / 2 + ACHIEVEMENT_TIER_ICON_SIZE / 2)

// Untouched by the icon-centering math above -- see point #2 above.
#define ACHIEVEMENT_POPUP_TEXT_X (ACHIEVEMENT_POPUP_ICON_SLOT_CENTER_X + 2)
#define ACHIEVEMENT_POPUP_TEXT_Y 0
#define ACHIEVEMENT_POPUP_DESC_MAX_WIDTH 196 // matches ScriptShowItemDescription's own wrap width

// Arbitrary and only needs to not collide with whatever else can be loaded at
// the same time as this popup (no registry of tags exists to check against).
#define ACHIEVEMENT_POPUP_ICON_TAG 0xACE1

// ACHIEVEMENT_TIER_COUNT itself now comes from enum AchievementTier
// (constants/achievements.h, Stage 21) -- this and src/achievements_menu.c
// used to each carry their own local derivation before that wave gave the
// rest of the codebase a shared one.

// Sized generously above anything realistic (simultaneous awards are rare,
// and only happen a handful at a time even off something like a Pokedex-
// completion check). AchievementPopup_Enqueue drops on overflow rather than
// blocking or overwriting the oldest entry -- safe because Achievement_
// TryComplete already committed the flag and points before this queue ever
// sees the id (design doc §4.30/§6): a dropped entry only means a missed
// toast, never a missed award.
#define ACHIEVEMENT_POPUP_QUEUE_SIZE 8

// Bug fix: descriptions longer than the single line left below the name/
// points row (see ACHIEVEMENT_POPUP_HEIGHT's own comment on the window only
// having room for 2 lines of FONT_SMALL text total) used to just keep
// printing past the window's own 3-tile-tall pixel buffer instead of
// stopping -- same overflow bug src/achievements_menu.c's
// PrintAchievementDescription hit and fixed the same way (see that
// function's own comment): BreakStringAutomatic(..., SHOW_SCROLL_PROMPT)
// instead of HIDE_SCROLL_PROMPT below, so an overlong description now
// auto-scrolls through the rest a line at a time instead of overflowing.
// sAchievementPopupNeedsScroll mirrors that file's descriptionScrolling --
// Task_HideAchievementPopupAfterDelay reads it to hold off starting the
// hide countdown until the player has actually seen every line.
EWRAM_DATA static bool8 sAchievementPopupNeedsScroll = FALSE;

// sAchievementPopupTextBuffer, not gStringVar4 -- see that buffer's own
// comment (ShowAchievementPopUpWindow) for why an overlong (needsScroll)
// popup can't be printed out of a buffer anything else in the engine might
// write to while this printer is still reading it across multiple frames.
// Same fix, same reasoning, as src/achievements_menu.c's own
// sAchievementsDescriptionBuffer.
#define ACHIEVEMENT_POPUP_TEXT_BUFFER_SIZE 0x100
EWRAM_DATA static u8 sAchievementPopupTextBuffer[ACHIEVEMENT_POPUP_TEXT_BUFFER_SIZE] = {0};

EWRAM_DATA static u8 sAchievementPopupTaskId = 0;
EWRAM_DATA static u8 sAchievementPopupWindowId = 0;
EWRAM_DATA static u8 sAchievementPopupIconSpriteId = 0;
// EWRAM's .sbss only allows zero initializers, so none of the three above can
// default to WINDOW_NONE/TASK_NONE/MAX_SPRITES the way non-EWRAM sentinels
// would -- this flag is the actual source of truth for whether a popup is
// currently up (and therefore whether the ids above are meaningful yet),
// rather than comparing them against a sentinel value.
EWRAM_DATA static bool8 sAchievementPopupActive = FALSE;

// Ring buffer: sAchievementPopupQueueHead is the next id to dequeue,
// sAchievementPopupQueueCount is how many are pending (mod-indexing off of
// head + count rather than tracking a separate tail).
EWRAM_DATA static u16 sAchievementPopupQueue[ACHIEVEMENT_POPUP_QUEUE_SIZE] = {0};
EWRAM_DATA static u8 sAchievementPopupQueueHead = 0;
EWRAM_DATA static u8 sAchievementPopupQueueCount = 0;

static void Task_HideAchievementPopupAfterDelay(u8 taskId);
static bool8 IsAchievementPopupSafeToShow(void);
static void ShowAchievementPopUpWindow(u16 achievementId);
static void HideAchievementPopUpWindow(void);
static u8 AddAchievementTierIconSprite(enum AchievementTier tier);
static void DestroyAchievementTierIconSprite(u8 spriteId);
static bool8 StringHasScrollPrompt(const u8 *str);

static const u8 sText_AchievementPopupFormat[] = _("{STR_VAR_2} (+{STR_VAR_1})\n{STR_VAR_3}");

// Tier icons are 16x16 (graphics/achievements/icons/*.png) -- a plain 2x2
// tile block, unlike this fork's 24x24 (3x3 tile) item icons, so there's no
// need for item_icon.c's DecompressDataWithHeaderWram-into-a-scratch-buffer/
// CopyItemIconPicTo4x4Buffer dance that pads a non-square tile count out to
// a 4x4 sprite shape. LoadCompressedSpriteSheet decompresses straight into
// VRAM in one call, same as most other plain compressed icon sprites in the
// engine (e.g. gSpriteSheet_CategoryIcons in src/pokemon_summary_screen.c).
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

// Plain single-frame 16x16 icon sprite -- tileTag/paletteTag are hardcoded
// (rather than runtime-parameterized like AddItemIconSprite's) since this
// popup only ever needs the one fixed tag, unlike the item icon API which
// has to serve arbitrary callers.
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
    PlayFanfare(MUS_OBTAIN_SYMBOL);

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

// Stage 4.2 entry point (design doc §4.2) -- src/achievements.c's
// QueueAchievementNotification calls this, not ShowAchievementPopup
// directly, so back-to-back awards each get a full, un-truncated display.
void AchievementPopup_Enqueue(u16 achievementId)
{
    u8 tail;

    if (sAchievementPopupQueueCount >= ACHIEVEMENT_POPUP_QUEUE_SIZE)
        return;

    tail = (sAchievementPopupQueueHead + sAchievementPopupQueueCount) % ACHIEVEMENT_POPUP_QUEUE_SIZE;
    sAchievementPopupQueue[tail] = achievementId;
    sAchievementPopupQueueCount++;

    // Draining itself happens from AchievementPopup_UpdateQueue, polled every
    // frame by CB2_Overworld -- see that function's comment for why this
    // isn't a self-perpetuating task.
}

static void Task_HideAchievementPopupAfterDelay(u8 taskId)
{
    // Drives the popup's own text printer -- unlike a field message box
    // (src/field_message_box.c's Task_DrawFieldMessage), nothing else on the
    // field calls RunTextPrinters() every frame the way this popup needs
    // once a long description is auto-scrolling through it (see
    // sAchievementPopupNeedsScroll's own comment). Harmless to call even
    // when the current content printed instantly (TEXT_SKIP_DRAW) and left
    // no printer active.
    RunTextPrinters();

    // Hold the hide countdown at zero for as long as an overlong
    // description is still auto-scrolling through its own lines
    // (IsTextPrinterActiveOnWindow), so the display window below always
    // starts counting from "the player has now seen every line" instead of
    // racing the scroll and cutting it off partway through. A description
    // that fit on one line never sets sAchievementPopupNeedsScroll, so this
    // is a no-op for the common case -- same instant countdown as before.
    if (sAchievementPopupNeedsScroll && IsTextPrinterActiveOnWindow(sAchievementPopupWindowId))
    {
        gTasks[taskId].tTimer = 0;
        return;
    }

    if (++gTasks[taskId].tTimer > ACHIEVEMENT_POPUP_DISPLAY_FRAMES)
    {
        HideAchievementPopUpWindow();
        DestroyTask(taskId);
    }
}

// Bug fix: this used to be driven by a self-perpetuating task
// (Task_DrainAchievementPopupQueue), created on demand by
// AchievementPopup_Enqueue and destroyed once the queue emptied. That broke
// whenever an achievement was queued while off the field (e.g. mid-battle):
// ResetTasks() is called unconditionally at battle start/end (and by nearly
// every other menu/minigame transition in the codebase) with no way for this
// file to know or intervene, so the drain task got wiped out while it was
// still waiting for a safe frame. The queued entry itself survived (it's a
// plain EWRAM ring buffer, not a task), but nothing was left to drain it --
// until the *next* achievement re-armed the task and flushed both at once.
//
// Polling this once per frame from CB2_Overworld instead sidesteps the whole
// class of bug: it needs no task to survive a transition it doesn't control,
// and CB2_Overworld is already exactly "the player is back on the field,"
// which IsAchievementPopupSafeToShow narrows down further to "and free to
// act."
void AchievementPopup_UpdateQueue(void)
{
    u16 achievementId;

    if (sAchievementPopupQueueCount == 0)
        return;

    // Waits for the current popup (if any) to finish on its own rather than
    // cutting it short, and for the field to be in a state where it's safe
    // to bring one up at all.
    if (sAchievementPopupActive || !IsAchievementPopupSafeToShow())
        return;

    achievementId = sAchievementPopupQueue[sAchievementPopupQueueHead];
    sAchievementPopupQueueHead = (sAchievementPopupQueueHead + 1) % ACHIEVEMENT_POPUP_QUEUE_SIZE;
    sAchievementPopupQueueCount--;

    ShowAchievementPopup(achievementId);
}

// Suppressed during battles/cutscenes/transitions (design doc §4.2): the
// popup draws straight onto overworld bg 0 using tiles/palette rows that
// only mean what this file assumes while CB2_Overworld is actually running.
// Mirrors the same idle-point check src/overworld.c:892's
// Task_ShowRoamerMessageDelayed uses before starting its own script, plus
// the CB2_Overworld check src/dexnav.c:1829 uses for the same "are we
// actually on the walkable field right now" question.
static bool8 IsAchievementPopupSafeToShow(void)
{
    return (gMain.callback2 == CB2_Overworld
         && !ScriptContext_IsEnabled()
         && !ArePlayerFieldControlsLocked()
         && !gPaletteFade.active
         && IsFieldMessageBoxHidden()
         && !FuncIsActiveTask(Task_MapNamePopUpWindow));
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

        // Stage 4.2: block movement while the popup is up, same mechanism
        // battle intros/cable club links use (src/battle_setup.c,
        // src/cable_club.c). Paired with UnlockPlayerFieldControls() in
        // HideAchievementPopUpWindow. Only on the fresh-show path -- a
        // content swap on an already-active popup is already locked.
        LockPlayerFieldControls();
    }
    else
    {
        FillWindowPixelBuffer(sAchievementPopupWindowId, PIXEL_FILL(1));
        DestroyAchievementTierIconSprite(sAchievementPopupIconSpriteId);
        // Cancels whatever scroll printer the previously-shown entry's
        // description registered below -- without this, a back-to-back
        // award landing while the last one was still mid-scroll leaves that
        // printer still ticking away against a window this
        // FillWindowPixelBuffer just cleared for the new one (same fix as
        // src/achievements_menu.c's PrintAchievementDescription).
        DeactivateSingleTextPrinter(sAchievementPopupWindowId, WINDOW_TEXT_PRINTER);
    }

    sAchievementPopupIconSpriteId = AddAchievementTierIconSprite(info->tier);
    if (sAchievementPopupIconSpriteId != MAX_SPRITES)
    {
        gSprites[sAchievementPopupIconSpriteId].x2 = ACHIEVEMENT_POPUP_ICON_X;
        gSprites[sAchievementPopupIconSpriteId].y2 = ACHIEVEMENT_POPUP_ICON_Y;
        gSprites[sAchievementPopupIconSpriteId].oam.priority = 0;
    }

    ConvertIntToDecimalStringN(gStringVar1, info->points, STR_CONV_MODE_LEFT_ALIGN, 5);
    StringCopy(gStringVar2, info->name);
    StringCopy(gStringVar3, info->description);
    StripLineBreaks(gStringVar3);
    // SHOW_SCROLL_PROMPT, not HIDE_SCROLL_PROMPT -- see
    // sAchievementPopupNeedsScroll's own comment above. StringHasScrollPrompt
    // below tells a description that fit the single line apart from one
    // that needed to scroll, same as src/achievements_menu.c's own
    // StringHasScrollPrompt/needsScroll pairing.
    BreakStringAutomatic(gStringVar3, ACHIEVEMENT_POPUP_DESC_MAX_WIDTH, 1, FONT_SMALL, SHOW_SCROLL_PROMPT);
    sAchievementPopupNeedsScroll = StringHasScrollPrompt(gStringVar3);

    // sAchievementPopupTextBuffer, not gStringVar4 -- see that buffer's own
    // comment.
    StringExpandPlaceholders(sAchievementPopupTextBuffer, sText_AchievementPopupFormat);

    // Only descriptions that actually need it pay for the letter-by-letter
    // typing delay -- one that already fit the single line still prints
    // instantly (TEXT_SKIP_DRAW), same as before.
    gTextFlags.autoScroll = sAchievementPopupNeedsScroll;
    AddTextPrinterParameterized(sAchievementPopupWindowId, FONT_SMALL, sAchievementPopupTextBuffer, ACHIEVEMENT_POPUP_TEXT_X, ACHIEVEMENT_POPUP_TEXT_Y,
        sAchievementPopupNeedsScroll ? GetPlayerTextSpeedDelay() : TEXT_SKIP_DRAW, NULL);

    CopyWindowToVram(sAchievementPopupWindowId, COPYWIN_FULL);
}

static void HideAchievementPopUpWindow(void)
{
    DestroyAchievementTierIconSprite(sAchievementPopupIconSpriteId);

    ClearStdWindowAndFrameToTransparent(sAchievementPopupWindowId, TRUE);
    RemoveWindow(sAchievementPopupWindowId);
    UnlockPlayerFieldControls();
    sAchievementPopupActive = FALSE;
    // gTextFlags.autoScroll is a shared global (see sAchievementPopupNeedsScroll's
    // own comment) -- clear it along with the flag that gated it so a
    // scrolled popup can't leak autoScroll into some unrelated field message
    // box the next script prints, the same leak
    // src/achievements_menu.c's DestroyCurrentAchievementsList guards against
    // on its own way out.
    gTextFlags.autoScroll = FALSE;
    sAchievementPopupNeedsScroll = FALSE;
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

// True for CHAR_PROMPT_SCROLL specifically (not CHAR_NEWLINE) -- lets
// ShowAchievementPopUpWindow tell a description that fit the popup's single
// line apart from one that needed BreakStringAutomatic's SHOW_SCROLL_PROMPT
// treatment to scroll through the rest. Same helper, same reasoning, as
// src/achievements_menu.c's own StringHasScrollPrompt.
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
