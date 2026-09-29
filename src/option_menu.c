#include "global.h"
#include "option_menu.h"
#include "achievements.h"
#include "ai_battles.h"
#include "battle_util.h"
#include "bg.h"
#include "gpu_regs.h"
#include "healthbox.h"
#include "international_string_util.h"
#include "line_break.h"
#include "main.h"
#include "menu.h"
#include "new_game_settings_menu.h"
#include "palette.h"
#include "player_palette_menu.h"
#include "scanline_effect.h"
#include "sprite.h"
#include "strings.h"
#include "task.h"
#include "text.h"
#include "text_window.h"
#include "title_screen.h"
#include "window.h"
#include "gba/m4a_internal.h"
#include "constants/rgb.h"
#include "event_data.h"
#include "string_util.h"

enum
{
    WIN_HEADER,
    WIN_OPTIONS,
    WIN_DESC
};

#define VISIBLE_ROWS 4
#define ROW_PITCH    16

static void Task_OptionMenuSave(u8 taskId);
static void Task_OptionMenuBackToTitle(u8 taskId);
static void Task_OptionMenuFadeOutToTitle(u8 taskId);
static void Task_OptionMenuOpenPlayerColors(u8 taskId);
static void Task_OptionMenuOpenHealthbox(u8 taskId);
static void Task_OptionMenuFadeOut(u8 taskId);
static void Task_OptionMenuFadeOutToPlayerColors(u8 taskId);
static void Task_OptionMenuFadeOutToHealthbox(u8 taskId);
static void HighlightOptionMenuItem(u8 selection);
static void Task_CategoryMenuFadeIn(u8 taskId);
static void Task_CategoryMenuProcessInput(u8 taskId);
static void DrawCategoryList(void);
static void Task_SettingsMenuFadeIn(u8 taskId);
static void Task_OptionMenuConfirmReset(u8 taskId);
static void StartResetPrompt(u8 taskId, u8 target);
static void Task_SettingsMenuProcessInput(u8 taskId);
static void DrawSettingsList(void);
static bool8 IsAutosaveHidden(void);
static bool8 IsAchievementBoostsHidden(void);
static void DrawHeaderText(void);
static void DrawBgWindowFrames(void);
static void DrawOptionMenuChoice(const u8 *text, u8 x, u8 y, u8 style);
static void DrawOptionMenuValue(const u8 *text, u8 y, bool8 isActive);

EWRAM_DATA static bool8 sArrowPressed = FALSE;
// Stashes the real caller across the round trip through CB2_InitPlayerPaletteMenu (whose
// savedCallback points back here). Restored into gMain.savedCallback by CB2_InitOptionMenu;
// NULL otherwise.
EWRAM_DATA static MainCallback sSavedCallback = NULL;
// Set when hopping to a submenu; makes CB2_InitOptionMenu re-enter the settings list
// at the preserved category and cursor.
EWRAM_DATA static bool8 sReturningFromSubmenu = FALSE;

// The new-game sequence is the only flow that hands this menu CB2_InitNewGameSettingsMenu
// as its return callback (see keep_storage_prompt.c).
static bool32 IsNewGameSequence(void)
{
    return gMain.savedCallback == CB2_InitNewGameSettingsMenu;
}

static const u8 gText_Option[]             = _("OPTION");
static const u8 gText_Confirm[]            = _("CONFIRM");
static const u8 gText_ResetAll[]           = _("RESET ALL");
static const u8 gText_ResetPrefix[]        = _("RESET ");
static const u8 gText_HintBack[]           = _("{B_BUTTON} BACK");
static const u8 gText_HintSelectExit[]     = _("{A_BUTTON} SELECT  {B_BUTTON} EXIT");
static const u8 gText_HintSelectConfirm[]  = _("{A_BUTTON} SELECT  {START_BUTTON} CONFIRM");

static const u8 gText_TextSpeedSlow[]      = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}SLOW");
static const u8 gText_TextSpeedMid[]       = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}MID");
static const u8 gText_TextSpeedFast[]      = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}FAST");
static const u8 gText_BattleSceneOn[]      = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}ON");
static const u8 gText_BattleSceneOff[]     = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}OFF");
static const u8 gText_BattleStyleShift[]   = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}SHIFT");
static const u8 gText_BattleStyleSet[]     = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}SET");
static const u8 gText_SoundMono[]          = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}MONO");
static const u8 gText_SoundStereo[]        = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}STEREO");
static const u8 gText_FrameTypeNumber[]    = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}");

static const u8 gText_AIBattlesOff[]       = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}OFF");
static const u8 gText_AIBattlesOn[]        = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}ON");
static const u8 gText_AutoScrollOff[]      = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}OFF");
static const u8 gText_AutoScrollOn[]       = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}ON");
static const u8 gText_AutosaveOff[]        = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}OFF");
static const u8 gText_AutosaveOn[]         = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}ON");
static const u8 gText_AchievementBoostsOff[] = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}OFF");
static const u8 gText_AchievementBoostsOn[]  = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}ON");

static const u8 gText_RouteTrackerOff[]    = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}OFF");
static const u8 gText_RouteTrackerOn[]     = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}ON");
static const u8 gText_ExpShareOptionOff[]  = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}OFF");
static const u8 gText_ExpShareOptionOn[]   = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}ON");
static const u8 gText_BattleSpeed1x[]      = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}1x");
static const u8 gText_BattleSpeed1_5x[]    = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}1.5x");
static const u8 gText_BattleSpeed2x[]      = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}2x");
static const u8 gText_BattleSpeed3x[]      = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}3x");
static const u8 gText_BattleSpeed4x[]      = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}4x");
static const u8 gText_BattleSpeed5x[]      = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}5x");

static const u8 sText_ChevronLeft[]        = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}{LEFT_ARROW}");
static const u8 sText_ChevronRight[]       = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}{RIGHT_ARROW}");

static const u16 sOptionMenuText_Pal[] = INCGFX_U16("graphics/interface/option_menu_text.pal", ".gbapal");
// note: this is only used in the Japanese release
static const u8 sEqualSignGfx[] = INCGFX_U8("graphics/interface/option_menu_equals_sign.png", ".4bpp");

enum OptionId
{
    OPTION_TEXT_SPEED,
    OPTION_SOUND,
    OPTION_FRAME,
    OPTION_AUTOSAVE,
    OPTION_AUTO_SCROLL,
    OPTION_BATTLE_SCENE,
    OPTION_BATTLE_STYLE,
    OPTION_BATTLE_SPEED,
    OPTION_AI_TRAINER,
    OPTION_AI_WILD,
    OPTION_EXP_SHARE,
    OPTION_ACHIEVEMENT_BOOSTS,
    OPTION_ROUTE_TRACKER,
    OPTION_PLAYER_COLOURS,
    OPTION_HEALTHBOX,
    OPTIONS_COUNT,
};

enum CategoryId
{
    CATEGORY_GENERAL,
    CATEGORY_BATTLE,
    CATEGORY_GAMEPLAY,
    CATEGORY_DISPLAY,
    CATEGORIES_COUNT,
};

struct OptionEntry
{
    const u8 *name;
    const u8 *description;       // Sentence case, auto-wrapped at runtime.
    u8 type;                     // enum OptionType
    u8 valueCount;               // 1 for submenu rows
    u8 defaultValue;
    const u8 *const *valueTexts; // NULL for numeric / submenu
    bool8 (*isHidden)(void);     // NULL = always visible
};

struct OptionCategory
{
    const u8 *name;
    const u8 *description;
    const u8 *optionIds;         // enum OptionId, in display order
    u8 optionCount;
};

static const u8 *const sValueTexts_TextSpeed[] = {gText_TextSpeedSlow, gText_TextSpeedMid, gText_TextSpeedFast};
static const u8 *const sValueTexts_Sound[] = {gText_SoundMono, gText_SoundStereo};
static const u8 *const sValueTexts_Autosave[] = {gText_AutosaveOff, gText_AutosaveOn};
static const u8 *const sValueTexts_AutoScroll[] = {gText_AutoScrollOff, gText_AutoScrollOn};
static const u8 *const sValueTexts_BattleScene[] = {gText_BattleSceneOn, gText_BattleSceneOff};
static const u8 *const sValueTexts_BattleStyle[] = {gText_BattleStyleShift, gText_BattleStyleSet};
static const u8 *const sValueTexts_BattleSpeed[OPTIONS_BATTLE_SPEED_COUNT] =
{
    [OPTIONS_BATTLE_SPEED_1X]   = gText_BattleSpeed1x,
    [OPTIONS_BATTLE_SPEED_1_5X] = gText_BattleSpeed1_5x,
    [OPTIONS_BATTLE_SPEED_2X]   = gText_BattleSpeed2x,
    [OPTIONS_BATTLE_SPEED_3X]   = gText_BattleSpeed3x,
    [OPTIONS_BATTLE_SPEED_4X]   = gText_BattleSpeed4x,
    [OPTIONS_BATTLE_SPEED_5X]   = gText_BattleSpeed5x,
};
static const u8 *const sValueTexts_AiBattles[] = {gText_AIBattlesOff, gText_AIBattlesOn};
static const u8 *const sValueTexts_ExpShare[] = {gText_ExpShareOptionOff, gText_ExpShareOptionOn};
static const u8 *const sValueTexts_AchievementBoosts[] = {gText_AchievementBoostsOff, gText_AchievementBoostsOn};
static const u8 *const sValueTexts_RouteTracker[] = {gText_RouteTrackerOff, gText_RouteTrackerOn};

static const struct OptionEntry sOptions[OPTIONS_COUNT] =
{
    [OPTION_TEXT_SPEED] = {
        .name = COMPOUND_STRING("TEXT SPEED"),
        .description = COMPOUND_STRING("How fast dialogue and battle text is printed."),
        .type = OPTION_TYPE_ENUM,
        .valueCount = ARRAY_COUNT(sValueTexts_TextSpeed),
        .defaultValue = OPTIONS_TEXT_SPEED_FAST,
        .valueTexts = sValueTexts_TextSpeed,
    },
    [OPTION_SOUND] = {
        .name = COMPOUND_STRING("SOUND"),
        .description = COMPOUND_STRING("Plays audio in mono or stereo. Stereo sounds best on headphones."),
        .type = OPTION_TYPE_ENUM,
        .valueCount = ARRAY_COUNT(sValueTexts_Sound),
        .defaultValue = OPTIONS_SOUND_STEREO,
        .valueTexts = sValueTexts_Sound,
    },
    [OPTION_FRAME] = {
        .name = COMPOUND_STRING("FRAME"),
        .description = COMPOUND_STRING("Changes the border style of text boxes and menus."),
        .type = OPTION_TYPE_NUMERIC,
        .valueCount = WINDOW_FRAMES_COUNT,
        .defaultValue = 0,
    },
    [OPTION_AUTOSAVE] = {
        .name = COMPOUND_STRING("AUTOSAVE"),
        .description = COMPOUND_STRING("Saves the game automatically at key moments, such as after healing or entering a new area."),
        .type = OPTION_TYPE_BOOL,
        .valueCount = ARRAY_COUNT(sValueTexts_Autosave),
        .defaultValue = FALSE,
        .valueTexts = sValueTexts_Autosave,
        .isHidden = IsAutosaveHidden,
    },
    [OPTION_AUTO_SCROLL] = {
        .name = COMPOUND_STRING("AUTO SCROLL"),
        .description = COMPOUND_STRING("Advances dialogue automatically once a message has finished printing."),
        .type = OPTION_TYPE_BOOL,
        .valueCount = ARRAY_COUNT(sValueTexts_AutoScroll),
        .defaultValue = FALSE,
        .valueTexts = sValueTexts_AutoScroll,
    },
    [OPTION_BATTLE_SCENE] = {
        .name = COMPOUND_STRING("BATTLE SCENE"),
        .description = COMPOUND_STRING("Turns move and status animations in battle on or off."),
        .type = OPTION_TYPE_BOOL,
        .valueCount = ARRAY_COUNT(sValueTexts_BattleScene),
        .defaultValue = 0, // ON
        .valueTexts = sValueTexts_BattleScene,
    },
    [OPTION_BATTLE_STYLE] = {
        .name = COMPOUND_STRING("BATTLE STYLE"),
        .description = COMPOUND_STRING("Shift offers a free switch after the opponent's Pokémon faints. Set does not."),
        .type = OPTION_TYPE_ENUM,
        .valueCount = ARRAY_COUNT(sValueTexts_BattleStyle),
        .defaultValue = OPTIONS_BATTLE_STYLE_SHIFT,
        .valueTexts = sValueTexts_BattleStyle,
    },
    [OPTION_BATTLE_SPEED] = {
        .name = COMPOUND_STRING("BATTLE SPEED"),
        .description = COMPOUND_STRING("Speeds up battle animations and text. Higher values make battles finish faster."),
        .type = OPTION_TYPE_ENUM,
        .valueCount = OPTIONS_BATTLE_SPEED_COUNT,
        .defaultValue = OPTIONS_BATTLE_SPEED_1X,
        .valueTexts = sValueTexts_BattleSpeed,
    },
    [OPTION_AI_TRAINER] = {
        .name = COMPOUND_STRING("AI TRAINER BATTLES"),
        .description = COMPOUND_STRING("Lets the game control your side in trainer battles."),
        .type = OPTION_TYPE_BOOL,
        .valueCount = ARRAY_COUNT(sValueTexts_AiBattles),
        .defaultValue = FALSE,
        .valueTexts = sValueTexts_AiBattles,
    },
    [OPTION_AI_WILD] = {
        .name = COMPOUND_STRING("AI WILD BATTLES"),
        .description = COMPOUND_STRING("Lets the game control your side in wild battles."),
        .type = OPTION_TYPE_BOOL,
        .valueCount = ARRAY_COUNT(sValueTexts_AiBattles),
        .defaultValue = FALSE,
        .valueTexts = sValueTexts_AiBattles,
    },
    [OPTION_EXP_SHARE] = {
        .name = COMPOUND_STRING("EXP SHARE"),
        .description = COMPOUND_STRING("Shares Exp. Points with every Pokémon in your party, not only those that battled."),
        .type = OPTION_TYPE_BOOL,
        .valueCount = ARRAY_COUNT(sValueTexts_ExpShare),
        .defaultValue = TRUE,
        .valueTexts = sValueTexts_ExpShare,
    },
    [OPTION_ACHIEVEMENT_BOOSTS] = {
        .name = COMPOUND_STRING("ACHIEVEMENT BOOSTS"),
        .description = COMPOUND_STRING("Enables the bonuses earned from completed achievements."),
        .type = OPTION_TYPE_BOOL,
        .valueCount = ARRAY_COUNT(sValueTexts_AchievementBoosts),
        .defaultValue = FALSE,
        .valueTexts = sValueTexts_AchievementBoosts,
        .isHidden = IsAchievementBoostsHidden,
    },
    [OPTION_ROUTE_TRACKER] = {
        .name = COMPOUND_STRING("ROUTE TRACKER"),
        .description = COMPOUND_STRING("Shows which Pokémon you have encountered on the current route."),
        .type = OPTION_TYPE_BOOL,
        .valueCount = ARRAY_COUNT(sValueTexts_RouteTracker),
        .defaultValue = FALSE,
        .valueTexts = sValueTexts_RouteTracker,
    },
    [OPTION_PLAYER_COLOURS] = {
        .name = COMPOUND_STRING("PLAYER COLOURS"),
        .description = COMPOUND_STRING("Opens the menu for customising your character's hat, outfit, and other colours."),
        .type = OPTION_TYPE_SUBMENU,
        .valueCount = 1,
    },
    [OPTION_HEALTHBOX] = {
        .name = COMPOUND_STRING("HEALTHBOX"),
        .description = COMPOUND_STRING("Opens the menu for customising how HP boxes look in battle."),
        .type = OPTION_TYPE_SUBMENU,
        .valueCount = 1,
    },
};

static const u8 sCategoryOptions_General[] = {OPTION_TEXT_SPEED, OPTION_SOUND, OPTION_FRAME, OPTION_AUTOSAVE, OPTION_AUTO_SCROLL};
static const u8 sCategoryOptions_Battle[] = {OPTION_BATTLE_SCENE, OPTION_BATTLE_STYLE, OPTION_BATTLE_SPEED, OPTION_AI_TRAINER, OPTION_AI_WILD};
static const u8 sCategoryOptions_Gameplay[] = {OPTION_EXP_SHARE, OPTION_ACHIEVEMENT_BOOSTS, OPTION_ROUTE_TRACKER};
static const u8 sCategoryOptions_Display[] = {OPTION_PLAYER_COLOURS, OPTION_HEALTHBOX};

static const struct OptionCategory sCategories[CATEGORIES_COUNT] =
{
    [CATEGORY_GENERAL] = {
        .name = COMPOUND_STRING("GENERAL"),
        .description = COMPOUND_STRING("Text, sound, saving, and other everyday settings."),
        .optionIds = sCategoryOptions_General,
        .optionCount = ARRAY_COUNT(sCategoryOptions_General),
    },
    [CATEGORY_BATTLE] = {
        .name = COMPOUND_STRING("BATTLE"),
        .description = COMPOUND_STRING("Battle animations, style, speed, and AI control."),
        .optionIds = sCategoryOptions_Battle,
        .optionCount = ARRAY_COUNT(sCategoryOptions_Battle),
    },
    [CATEGORY_GAMEPLAY] = {
        .name = COMPOUND_STRING("GAMEPLAY"),
        .description = COMPOUND_STRING("Rules and helpers that change how the game plays."),
        .optionIds = sCategoryOptions_Gameplay,
        .optionCount = ARRAY_COUNT(sCategoryOptions_Gameplay),
    },
    [CATEGORY_DISPLAY] = {
        .name = COMPOUND_STRING("DISPLAY"),
        .description = COMPOUND_STRING("Player colours and the battle HP box style."),
        .optionIds = sCategoryOptions_Display,
        .optionCount = ARRAY_COUNT(sCategoryOptions_Display),
    },
};

EWRAM_DATA static u8 sPendingValues[OPTIONS_COUNT] = {0};

static u8 LoadOptionValue(u8 optionId)
{
    switch (optionId)
    {
    case OPTION_TEXT_SPEED:
        return gSaveBlock2Ptr->optionsTextSpeed;
    case OPTION_SOUND:
        return gSaveBlock2Ptr->optionsSound;
    case OPTION_FRAME:
        return gSaveBlock2Ptr->optionsWindowFrameType;
    case OPTION_AUTOSAVE:
        return gSaveBlock1Ptr->autosaveModeEnabled ? 1 : 0;
    case OPTION_AUTO_SCROLL:
        return FlagGet(FLAG_AUTO_SCROLL_TEXT) ? 1 : 0;
    case OPTION_BATTLE_SCENE:
        return gSaveBlock2Ptr->optionsBattleSceneOff ? 1 : 0;
    case OPTION_BATTLE_STYLE:
        return gSaveBlock2Ptr->optionsBattleStyle ? 1 : 0;
    case OPTION_BATTLE_SPEED:
        // The save field holds 3 bits but only 6 are valid values; keep it in range.
        return (gSaveBlock2Ptr->optionsBattleSpeed < OPTIONS_BATTLE_SPEED_COUNT)
             ? gSaveBlock2Ptr->optionsBattleSpeed : OPTIONS_BATTLE_SPEED_1X;
    case OPTION_AI_TRAINER:
        return AiBattles_GetSetting(AI_BATTLES_SETTING_TRAINER) ? 1 : 0;
    case OPTION_AI_WILD:
        return AiBattles_GetSetting(AI_BATTLES_SETTING_WILD) ? 1 : 0;
    case OPTION_EXP_SHARE:
        return IsGen6ExpShareEnabled() ? 1 : 0;
    case OPTION_ACHIEVEMENT_BOOSTS:
        return Achievement_BoostsEnabled() ? 1 : 0;
    case OPTION_ROUTE_TRACKER:
        return gSaveBlock2Ptr->optionsRouteTracker ? 1 : 0;
    default:
        return 0;
    }
}

static void StoreOptionValue(u8 optionId, u8 value)
{
    switch (optionId)
    {
    case OPTION_TEXT_SPEED:
        gSaveBlock2Ptr->optionsTextSpeed = value;
        break;
    case OPTION_SOUND:
        gSaveBlock2Ptr->optionsSound = value;
        break;
    case OPTION_FRAME:
        gSaveBlock2Ptr->optionsWindowFrameType = value;
        break;
    case OPTION_AUTOSAVE:
        gSaveBlock1Ptr->autosaveModeEnabled = value ? 1 : 0;
        break;
    case OPTION_AUTO_SCROLL:
        if (value)
            FlagSet(FLAG_AUTO_SCROLL_TEXT);
        else
            FlagClear(FLAG_AUTO_SCROLL_TEXT);
        break;
    case OPTION_BATTLE_SCENE:
        gSaveBlock2Ptr->optionsBattleSceneOff = value;
        break;
    case OPTION_BATTLE_STYLE:
        gSaveBlock2Ptr->optionsBattleStyle = value;
        break;
    case OPTION_BATTLE_SPEED:
        gSaveBlock2Ptr->optionsBattleSpeed = value;
        break;
    case OPTION_AI_TRAINER:
        AiBattles_SetSetting(AI_BATTLES_SETTING_TRAINER, value != 0);
        break;
    case OPTION_AI_WILD:
        AiBattles_SetSetting(AI_BATTLES_SETTING_WILD, value != 0);
        break;
    case OPTION_EXP_SHARE:
        // Same field the Exp. Share key item toggles (IsGen6ExpShareEnabled), so both controls stay in sync.
        gSaveBlock2Ptr->optionsExpShare = value;
        break;
    case OPTION_ACHIEVEMENT_BOOSTS:
        Achievement_SetBoostsEnabled(value);
        Achievement_FlushProfile();
        break;
    case OPTION_ROUTE_TRACKER:
        gSaveBlock2Ptr->optionsRouteTracker = value;
        break;
    }
}

static void LoadAllOptions(void)
{
    u8 i;

    for (i = 0; i < OPTIONS_COUNT; i++)
        sPendingValues[i] = LoadOptionValue(i);
}

static void CommitAllOptions(void)
{
    u8 i;

    for (i = 0; i < OPTIONS_COUNT; i++)
    {
        if (sOptions[i].type == OPTION_TYPE_SUBMENU)
            continue;
        if (sOptions[i].isHidden != NULL && sOptions[i].isHidden())
            continue;
        StoreOptionValue(i, sPendingValues[i]);
    }
}

// Menu levels. Item indices are raw positions within the current level's list, hidden
// items included:
//   LEVEL_CATEGORIES: 0..CATEGORIES_COUNT-1 = categories, then RESET ALL, then
//                     CANCEL (CONFIRM in the new-game sequence).
//   LEVEL_SETTINGS:   0..optionCount-1 = the category's options, then RESET <CATEGORY>.
// sCursor holds a raw item index; sScrollOffset is in visible-row units (hidden items
// don't occupy a row).
#define LEVEL_CATEGORIES 0
#define LEVEL_SETTINGS   1

#define CATEGORY_ITEM_RESET_ALL CATEGORIES_COUNT
#define CATEGORY_ITEM_CANCEL    (CATEGORIES_COUNT + 1)

EWRAM_DATA static u8 sMenuLevel = LEVEL_CATEGORIES;
EWRAM_DATA static u8 sCurrentCategory = 0;
EWRAM_DATA static u8 sCursor[2] = {0};
EWRAM_DATA static u8 sScrollOffset[2] = {0};

static u8 GetItemCount(void)
{
    if (sMenuLevel == LEVEL_CATEGORIES)
        return CATEGORIES_COUNT + 2;
    return sCategories[sCurrentCategory].optionCount + 1;
}

static bool8 IsItemHidden(u8 item)
{
    const struct OptionCategory *category;
    bool8 (*isHidden)(void);

    if (sMenuLevel != LEVEL_SETTINGS)
        return FALSE;
    category = &sCategories[sCurrentCategory];
    if (item >= category->optionCount)
        return FALSE;
    isHidden = sOptions[category->optionIds[item]].isHidden;
    return isHidden != NULL && isHidden();
}

static u8 GetVisibleCount(void)
{
    u8 i, count = 0;
    u8 total = GetItemCount();

    for (i = 0; i < total; i++)
    {
        if (!IsItemHidden(i))
            count++;
    }
    return count;
}

// Returns the raw item index of the nth visible row; 0 if n is out of range.
static u8 GetNthVisibleItem(u8 n)
{
    u8 i;
    u8 total = GetItemCount();

    for (i = 0; i < total; i++)
    {
        if (IsItemHidden(i))
            continue;
        if (n == 0)
            return i;
        n--;
    }
    return 0;
}

// Returns the visible-row position of a raw item index (hidden items before it are skipped).
static u8 GetVisibleIndexOf(u8 item)
{
    u8 i, row = 0;

    for (i = 0; i < item; i++)
    {
        if (!IsItemHidden(i))
            row++;
    }
    return row;
}

static void ClampScroll(void)
{
    u8 row = GetVisibleIndexOf(sCursor[sMenuLevel]);
    u8 count = GetVisibleCount();
    u8 maxOffset = count > VISIBLE_ROWS ? count - VISIBLE_ROWS : 0;
    u8 *offset = &sScrollOffset[sMenuLevel];

    if (row < *offset)
        *offset = row;
    else if (row > *offset + VISIBLE_ROWS - 1)
        *offset = row - (VISIBLE_ROWS - 1);
    if (*offset > maxOffset)
        *offset = maxOffset;
}

static void MoveCursor(s8 delta)
{
    s16 row = GetVisibleIndexOf(sCursor[sMenuLevel]);
    s16 count = GetVisibleCount();

    row = (row + delta) % count;
    if (row < 0)
        row += count;
    sCursor[sMenuLevel] = GetNthVisibleItem(row);
    ClampScroll();
}

static const struct WindowTemplate sOptionMenuWinTemplates[] =
{
    [WIN_HEADER] = {
        .bg = 1,
        .tilemapLeft = 2,
        .tilemapTop = 1,
        .width = 26,
        .height = 2,
        .paletteNum = 1,
        .baseBlock = 2
    },
    [WIN_OPTIONS] = {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 5,
        .width = 26,
        .height = 8,
        .paletteNum = 1,
        .baseBlock = 0x36
    },
    [WIN_DESC] = {
        .bg = 1, // BG0 is darkened outside the cursor window; BG1 is not
        .tilemapLeft = 2,
        .tilemapTop = 15,
        .width = 26,
        .height = 4,
        .paletteNum = 1,
        .baseBlock = 0x106
    },
    DUMMY_WIN_TEMPLATE
};

static const struct BgTemplate sOptionMenuBgTemplates[] =
{
    {
        .bg = 1,
        .charBaseIndex = 1,
        .mapBaseIndex = 30,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0
    },
    {
        .bg = 0,
        .charBaseIndex = 1,
        .mapBaseIndex = 31,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 1,
        .baseTile = 0
    }
};

static const u16 sOptionMenuBg_Pal[] = {RGB(17, 18, 31)};

#define DESC_MAX_WIDTH      190
#define DESC_RESTART_DELAY  120

static const u8 sText_DescResetAll[]  = _("Resets every setting to its default value.");
static const u8 sText_DescResetCategory[] = _("Resets the settings in this category to their default values.");
static const u8 sText_DescCancel[]    = _("Saves your changes and leaves the settings menu.");
static const u8 sText_DescConfirm[]   = _("Confirms your settings and continues.");
static const u8 sDescTextColors[3]    = {1, 2, 3};

EWRAM_DATA static u8 sDescBuffer[0x100] = {0};
EWRAM_DATA static bool8 sDescScrolling = FALSE;
EWRAM_DATA static u16 sDescRestartTimer = 0;

// True if BreakStringAutomatic inserted a CHAR_PROMPT_SCROLL, i.e. the text
// does not fit the 2-line description window.
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

// Overlong text auto-scrolls; MainCB2 restarts it once it finishes.
static void PrintOptionDescription(const u8 *description)
{
    bool8 needsScroll;

    FillWindowPixelBuffer(WIN_DESC, PIXEL_FILL(1));
    DeactivateSingleTextPrinter(WIN_DESC, WINDOW_TEXT_PRINTER);

    StringCopy(sDescBuffer, description);
    StripLineBreaks(sDescBuffer);
    BreakStringAutomatic(sDescBuffer, DESC_MAX_WIDTH, 2, FONT_NORMAL, SHOW_SCROLL_PROMPT);
    needsScroll = StringHasScrollPrompt(sDescBuffer);

    gTextFlags.autoScroll = needsScroll;
    AddTextPrinterParameterized3(WIN_DESC, FONT_NORMAL, 8, 1, sDescTextColors,
        needsScroll ? GetPlayerTextSpeedDelay() : TEXT_SKIP_DRAW, sDescBuffer);

    sDescScrolling = needsScroll;
    sDescRestartTimer = 0;
    CopyWindowToVram(WIN_DESC, COPYWIN_GFX);
}

static void StopOptionDescription(void)
{
    DeactivateSingleTextPrinter(WIN_DESC, WINDOW_TEXT_PRINTER);
    gTextFlags.autoScroll = FALSE;
    sDescScrolling = FALSE;
}

// Shows the description for the highlighted item at the current menu level.
static void UpdateDescription(void)
{
    u8 item = sCursor[sMenuLevel];
    const u8 *text;

    if (sMenuLevel == LEVEL_CATEGORIES)
    {
        if (item < CATEGORIES_COUNT)
            text = sCategories[item].description;
        else if (item == CATEGORY_ITEM_RESET_ALL)
            text = sText_DescResetAll;
        else
            text = IsNewGameSequence() ? sText_DescConfirm : sText_DescCancel;
    }
    else
    {
        const struct OptionCategory *category = &sCategories[sCurrentCategory];

        text = item < category->optionCount ? sOptions[category->optionIds[item]].description : sText_DescResetCategory;
    }
    PrintOptionDescription(text);
}

static void MainCB2(void)
{
    RunTasks();
    RunTextPrinters();
    if (sDescScrolling && !IsTextPrinterActiveOnWindow(WIN_DESC) && ++sDescRestartTimer >= DESC_RESTART_DELAY)
        UpdateDescription();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

static void VBlankCB(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

void CB2_InitOptionMenu(void)
{
    u8 taskId;
    switch (gMain.state)
    {
    default:
    case 0:
        SetVBlankCallback(NULL);
        // Button Mode is no longer configurable; normalise old saves.
        gSaveBlock2Ptr->optionsButtonMode = OPTIONS_BUTTON_MODE_NORMAL;
        // Returning from a submenu; restore the caller stashed by the fade-out task.
        if (sSavedCallback != NULL)
        {
            gMain.savedCallback = sSavedCallback;
            sSavedCallback = NULL;
        }
        gMain.state++;
        break;
    case 1:
        DmaClearLarge16(3, (void *)(VRAM), VRAM_SIZE, 0x1000);
        DmaClear32(3, OAM, OAM_SIZE);
        DmaClear16(3, PLTT, PLTT_SIZE);
        SetGpuReg(REG_OFFSET_DISPCNT, 0);
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(0, sOptionMenuBgTemplates, ARRAY_COUNT(sOptionMenuBgTemplates));
        ChangeBgX(0, 0, BG_COORD_SET);
        ChangeBgY(0, 0, BG_COORD_SET);
        ChangeBgX(1, 0, BG_COORD_SET);
        ChangeBgY(1, 0, BG_COORD_SET);
        ChangeBgX(2, 0, BG_COORD_SET);
        ChangeBgY(2, 0, BG_COORD_SET);
        ChangeBgX(3, 0, BG_COORD_SET);
        ChangeBgY(3, 0, BG_COORD_SET);
        InitWindows(sOptionMenuWinTemplates);
        DeactivateAllTextPrinters();
        SetGpuReg(REG_OFFSET_WIN0H, 0);
        SetGpuReg(REG_OFFSET_WIN0V, 0);
        SetGpuReg(REG_OFFSET_WININ, WININ_WIN0_BG0);
        SetGpuReg(REG_OFFSET_WINOUT, WINOUT_WIN01_BG0 | WINOUT_WIN01_BG1 | WINOUT_WIN01_CLR);
        SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_BG0 | BLDCNT_EFFECT_DARKEN);
        SetGpuReg(REG_OFFSET_BLDALPHA, 0);
        SetGpuReg(REG_OFFSET_BLDY, 4);
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_WIN0_ON | DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
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
        LoadBgTiles(1, GetWindowFrameTilesPal(gSaveBlock2Ptr->optionsWindowFrameType)->tiles, 0x120, 0x1A2);
        gMain.state++;
        break;
    case 4:
        LoadPalette(sOptionMenuBg_Pal, BG_PLTT_ID(0), sizeof(sOptionMenuBg_Pal));
        LoadPalette(GetWindowFrameTilesPal(gSaveBlock2Ptr->optionsWindowFrameType)->pal, BG_PLTT_ID(7), PLTT_SIZE_4BPP);
        gMain.state++;
        break;
    case 5:
        LoadPalette(sOptionMenuText_Pal, BG_PLTT_ID(1), sizeof(sOptionMenuText_Pal));
        gMain.state++;
        break;
    case 6:
        PutWindowTilemap(WIN_HEADER);
        DrawHeaderText();
        gMain.state++;
        break;
    case 7:
        gMain.state++;
        break;
    case 8:
        PutWindowTilemap(WIN_OPTIONS);
        PutWindowTilemap(WIN_DESC);
        FillWindowPixelBuffer(WIN_DESC, PIXEL_FILL(1));
        CopyWindowToVram(WIN_DESC, COPYWIN_FULL);
        sDescScrolling = FALSE;
        gMain.state++;
        break;
    case 9:
        DrawBgWindowFrames();
        gMain.state++;
        break;
    case 10:
    {
        taskId = CreateTask(Task_CategoryMenuFadeIn, 0);
        LoadAllOptions();
        if (sReturningFromSubmenu)
        {
            sReturningFromSubmenu = FALSE;
            gTasks[taskId].func = Task_SettingsMenuFadeIn;
            sMenuLevel = LEVEL_SETTINGS;
            DrawHeaderText();
            DrawSettingsList();
        }
        else
        {
            sMenuLevel = LEVEL_CATEGORIES;
            sCursor[LEVEL_CATEGORIES] = 0;
            sScrollOffset[LEVEL_CATEGORIES] = 0;
            DrawCategoryList();
        }
        gMain.state++;
        break;
    }
    case 11:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
        SetVBlankCallback(VBlankCB);
        SetMainCallback2(MainCB2);
        return;
    }
}

static void Task_OptionMenuSave(u8 taskId)
{
    CommitAllOptions();
    BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
    gTasks[taskId].func = Task_OptionMenuFadeOut;
}

// B during the new game sequence: discard the pending option changes (the sequence is
// being abandoned) and return to the title screen.
static void Task_OptionMenuBackToTitle(u8 taskId)
{
    BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
    gTasks[taskId].func = Task_OptionMenuFadeOutToTitle;
}

static void Task_OptionMenuFadeOutToTitle(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        DestroyTask(taskId);
        StopOptionDescription();
        FreeAllWindowBuffers();
        SetMainCallback2(CB2_InitTitleScreen);
    }
}

static void Task_OptionMenuFadeOut(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        DestroyTask(taskId);
        StopOptionDescription();
        FreeAllWindowBuffers();
        SetMainCallback2(gMain.savedCallback);
    }
}

// PLAYER COLOURS is an action row like CANCEL, but instead of leaving the
// option menu for good it hops to CB2_InitPlayerPaletteMenu and back.
static void Task_OptionMenuOpenPlayerColors(u8 taskId)
{
    CommitAllOptions();
    BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
    gTasks[taskId].func = Task_OptionMenuFadeOutToPlayerColors;
}

static void Task_OptionMenuFadeOutToPlayerColors(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        DestroyTask(taskId);
        StopOptionDescription();
        FreeAllWindowBuffers();
        // sSavedCallback carries the real caller across the trip.
        sReturningFromSubmenu = TRUE;
        sSavedCallback = gMain.savedCallback;
        gMain.savedCallback = CB2_InitOptionMenu;
        SetMainCallback2(CB2_InitPlayerPaletteMenu);
    }
}

// HEALTHBOX is an action row like PLAYER COLOURS: it hops to CB2_InitHealthboxSettings and back.
static void Task_OptionMenuOpenHealthbox(u8 taskId)
{
    CommitAllOptions();
    BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
    gTasks[taskId].func = Task_OptionMenuFadeOutToHealthbox;
}

static void Task_OptionMenuFadeOutToHealthbox(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        DestroyTask(taskId);
        StopOptionDescription();
        FreeAllWindowBuffers();
        sReturningFromSubmenu = TRUE;
        sSavedCallback = gMain.savedCallback;
        gMain.savedCallback = CB2_InitOptionMenu;
        SetMainCallback2(CB2_InitHealthboxSettings);
    }
}

static void HighlightOptionMenuItem(u8 index)
{
    SetGpuReg(REG_OFFSET_WIN0H, WIN_RANGE(16, DISPLAY_WIDTH - 16));
    SetGpuReg(REG_OFFSET_WIN0V, WIN_RANGE(index * 16 + 40, index * 16 + 56));
}

static void DrawOptionMenuChoice(const u8 *text, u8 x, u8 y, u8 style)
{
    u8 dst[16];
    u16 i;

    for (i = 0; *text != EOS && i < ARRAY_COUNT(dst) - 1; i++)
        dst[i] = *(text++);

    if (style != 0)
    {
        dst[2] = TEXT_COLOR_RED;
        dst[5] = TEXT_COLOR_LIGHT_RED;
    }

    dst[i] = EOS;
    AddTextPrinterParameterized(WIN_OPTIONS, FONT_NORMAL, dst, x, y + 1, TEXT_SKIP_DRAW, NULL);
}

#define OPTION_VALUE_ZONE_X      96
#define OPTION_VALUE_ZONE_WIDTH  112
#define OPTION_VALUE_LEFT        104
#define OPTION_VALUE_RIGHT       198
#define OPTION_VALUE_CHEVRON_GAP 4

// Clears the row's value area and draws only the currently selected choice,
// centered, with chevrons on either side when the row is the one being edited.
static void DrawOptionMenuValue(const u8 *text, u8 y, bool8 isActive)
{
    s32 width = GetStringWidth(FONT_NORMAL, text, 0);
    s32 x = OPTION_VALUE_LEFT + ((OPTION_VALUE_RIGHT - OPTION_VALUE_LEFT) - width) / 2;

    FillWindowPixelRect(WIN_OPTIONS, PIXEL_FILL(1), OPTION_VALUE_ZONE_X, y, OPTION_VALUE_ZONE_WIDTH, 16);
    DrawOptionMenuChoice(text, x, y, isActive);

    if (isActive)
    {
        s32 leftWidth = GetStringWidth(FONT_NORMAL, sText_ChevronLeft, 0);
        DrawOptionMenuChoice(sText_ChevronLeft, x - leftWidth - OPTION_VALUE_CHEVRON_GAP, y, TRUE);
        DrawOptionMenuChoice(sText_ChevronRight, x + width + OPTION_VALUE_CHEVRON_GAP, y, TRUE);
    }
}

static void DrawHeaderText(void)
{
    const u8 *title = gText_Option;
    const u8 *hint = IsNewGameSequence() ? gText_HintSelectConfirm : gText_HintSelectExit;

    if (sMenuLevel == LEVEL_SETTINGS)
    {
        title = sCategories[sCurrentCategory].name;
        hint = gText_HintBack;
    }
    FillWindowPixelBuffer(WIN_HEADER, PIXEL_FILL(1));
    AddTextPrinterParameterized(WIN_HEADER, FONT_NORMAL, title, 8, 1, TEXT_SKIP_DRAW, NULL);
    AddTextPrinterParameterized(WIN_HEADER, FONT_NORMAL, hint, GetStringRightAlignXOffset(FONT_NORMAL, hint, 198), 1, TEXT_SKIP_DRAW, NULL);
    CopyWindowToVram(WIN_HEADER, COPYWIN_FULL);
}

static bool8 IsAutosaveHidden(void)
{
    // Nuzlocke forces autosave on, so the option is hidden. Draft and Recruits leave it
    // player-controlled (Recruits_DoRetirement arms an autosave but doesn't require one).
    return gSaveBlock1Ptr->nuzlockeModeEnabled;
}

// Hidden until the first-playthrough gate unlocks boosts (Achievement_BoostsUnlocked).
static bool8 IsAchievementBoostsHidden(void)
{
    return !Achievement_BoostsUnlocked();
}

static const u8 *GetCancelOrItemName(const u8 *name, bool32 isCancelRow)
{
    return (isCancelRow && IsNewGameSequence()) ? gText_Confirm : name;
}

static void DrawCategoryList(void)
{
    u8 row, item;
    u8 count = GetVisibleCount();
    u8 offset = sScrollOffset[LEVEL_CATEGORIES];
    const u8 *name;

    FillWindowPixelBuffer(WIN_OPTIONS, PIXEL_FILL(1));
    for (row = 0; row < VISIBLE_ROWS && offset + row < count; row++)
    {
        item = GetNthVisibleItem(offset + row);
        if (item < CATEGORIES_COUNT)
            name = sCategories[item].name;
        else if (item == CATEGORY_ITEM_RESET_ALL)
            name = gText_ResetAll;
        else
            name = GetCancelOrItemName(gText_Cancel, TRUE);
        AddTextPrinterParameterized(WIN_OPTIONS, FONT_NARROW, name, 8, row * ROW_PITCH + 1, TEXT_SKIP_DRAW, NULL);
    }
    CopyWindowToVram(WIN_OPTIONS, COPYWIN_FULL);
    HighlightOptionMenuItem(GetVisibleIndexOf(sCursor[LEVEL_CATEGORIES]) - offset);
    UpdateDescription();
}

static void Task_CategoryMenuFadeIn(u8 taskId)
{
    if (!gPaletteFade.active)
        gTasks[taskId].func = Task_CategoryMenuProcessInput;
}

static void Task_CategoryMenuProcessInput(u8 taskId)
{
    u8 item = sCursor[LEVEL_CATEGORIES];

    if (JOY_NEW(A_BUTTON))
    {
        if (item < CATEGORIES_COUNT)
        {
            sCurrentCategory = item;
            sMenuLevel = LEVEL_SETTINGS;
            sCursor[LEVEL_SETTINGS] = 0;
            sScrollOffset[LEVEL_SETTINGS] = 0;
            DrawHeaderText();
            DrawSettingsList();
            gTasks[taskId].func = Task_SettingsMenuProcessInput;
        }
        else if (item == CATEGORY_ITEM_RESET_ALL)
            StartResetPrompt(taskId, CATEGORIES_COUNT);
        else if (item == CATEGORY_ITEM_CANCEL)
            gTasks[taskId].func = Task_OptionMenuSave;
    }
    else if (JOY_NEW(B_BUTTON))
    {
        // New game sequence: B backs out of the whole sequence; START confirms.
        gTasks[taskId].func = IsNewGameSequence() ? Task_OptionMenuBackToTitle : Task_OptionMenuSave;
    }
    else if (JOY_NEW(START_BUTTON) && IsNewGameSequence())
    {
        gTasks[taskId].func = Task_OptionMenuSave;
    }
    else if (JOY_NEW(DPAD_UP))
    {
        MoveCursor(-1);
        DrawCategoryList();
    }
    else if (JOY_NEW(DPAD_DOWN))
    {
        MoveCursor(1);
        DrawCategoryList();
    }
}

static const u8 *GetOptionValueText(u8 optionId, u8 value)
{
    static u8 sNumberText[16];
    u8 n = value + 1;
    u16 i;

    if (sOptions[optionId].type != OPTION_TYPE_NUMERIC)
        return sOptions[optionId].valueTexts[value];

    for (i = 0; gText_FrameTypeNumber[i] != EOS && i <= 5; i++)
        sNumberText[i] = gText_FrameTypeNumber[i];
    if (n / 10 != 0)
        sNumberText[i++] = n / 10 + CHAR_0;
    sNumberText[i++] = n % 10 + CHAR_0;
    sNumberText[i] = EOS;
    return sNumberText;
}

// Draws one settings-level row (label plus value column) at a visible row position.
static void DrawSettingsRow(u8 row, u8 item, bool8 isActive)
{
    const struct OptionCategory *category = &sCategories[sCurrentCategory];
    const u8 *name;
    u8 resetName[24];
    u8 y = row * ROW_PITCH;

    FillWindowPixelRect(WIN_OPTIONS, PIXEL_FILL(1), 0, y, 208, ROW_PITCH);
    if (item >= category->optionCount)
    {
        StringCopy(resetName, gText_ResetPrefix);
        StringAppend(resetName, category->name);
        AddTextPrinterParameterized(WIN_OPTIONS, FONT_NARROW, resetName, 8, y + 1, TEXT_SKIP_DRAW, NULL);
        return;
    }

    u8 optionId = category->optionIds[item];

    name = sOptions[optionId].name;
    AddTextPrinterParameterized(WIN_OPTIONS, FONT_NARROW, name, 8, y + 1, TEXT_SKIP_DRAW, NULL);
    if (sOptions[optionId].type == OPTION_TYPE_SUBMENU)
        DrawOptionMenuValue(sText_ChevronRight, y, FALSE);
    else
        DrawOptionMenuValue(GetOptionValueText(optionId, sPendingValues[optionId]), y, isActive);
}

static void DrawSettingsList(void)
{
    u8 row;
    u8 count = GetVisibleCount();
    u8 offset = sScrollOffset[LEVEL_SETTINGS];
    u8 cursorItem = sCursor[LEVEL_SETTINGS];

    for (row = 0; row < VISIBLE_ROWS && offset + row < count; row++)
    {
        u8 item = GetNthVisibleItem(offset + row);

        DrawSettingsRow(row, item, item == cursorItem);
    }
    // Clear any rows below the last visible item.
    if (row < VISIBLE_ROWS)
        FillWindowPixelRect(WIN_OPTIONS, PIXEL_FILL(1), 0, row * ROW_PITCH, 208, (VISIBLE_ROWS - row) * ROW_PITCH);
    CopyWindowToVram(WIN_OPTIONS, COPYWIN_FULL);
    HighlightOptionMenuItem(GetVisibleIndexOf(cursorItem) - offset);
    UpdateDescription();
}

static void MoveSettingsCursor(s8 delta)
{
    u8 oldItem = sCursor[LEVEL_SETTINGS];
    u8 oldOffset = sScrollOffset[LEVEL_SETTINGS];
    u8 newItem, newRow;

    MoveCursor(delta);
    newItem = sCursor[LEVEL_SETTINGS];
    if (sScrollOffset[LEVEL_SETTINGS] != oldOffset)
    {
        DrawSettingsList();
        return;
    }
    newRow = GetVisibleIndexOf(newItem) - oldOffset;
    DrawSettingsRow(GetVisibleIndexOf(oldItem) - oldOffset, oldItem, FALSE);
    DrawSettingsRow(newRow, newItem, TRUE);
    CopyWindowToVram(WIN_OPTIONS, COPYWIN_GFX);
    HighlightOptionMenuItem(newRow);
    UpdateDescription();
}

// Live preview of options that take effect before commit.
static void ApplyOptionSideEffect(u8 optionId, u8 value)
{
    switch (optionId)
    {
    case OPTION_SOUND:
        SetPokemonCryStereo(value);
        break;
    case OPTION_FRAME:
        LoadBgTiles(1, GetWindowFrameTilesPal(value)->tiles, 0x120, 0x1A2);
        LoadPalette(GetWindowFrameTilesPal(value)->pal, BG_PLTT_ID(7), PLTT_SIZE_4BPP);
        break;
    }
}

// Left/Right on the highlighted row. Applies the change to sPendingValues and any live
// side effect; returns TRUE if the value changed.
static bool8 ProcessOptionInput(u8 optionId)
{
    const struct OptionEntry *option = &sOptions[optionId];
    u8 value = sPendingValues[optionId];

    if (option->type == OPTION_TYPE_SUBMENU || option->type == OPTION_TYPE_ACTION)
        return FALSE;

    if (JOY_NEW(DPAD_RIGHT))
        value = (value + 1 >= option->valueCount) ? 0 : value + 1;
    else if (JOY_NEW(DPAD_LEFT))
        value = (value == 0) ? option->valueCount - 1 : value - 1;
    else
        return FALSE;

    sPendingValues[optionId] = value;
    sArrowPressed = TRUE;
    ApplyOptionSideEffect(optionId, value);
    return TRUE;
}

// Submenu rows own their storage (Healthbox, Player Colours) and are not reset here.
// Options absent from SetDefaultOptions() (Autosave, Auto Scroll, Achievement Boosts,
// Route Tracker, AI Trainer, AI Wild) take their default from sOptions.
static void ResetOptionToDefault(u8 optionId)
{
    if (sOptions[optionId].type == OPTION_TYPE_SUBMENU)
        return;
    sPendingValues[optionId] = sOptions[optionId].defaultValue;
    ApplyOptionSideEffect(optionId, sOptions[optionId].defaultValue);
}

static void ResetCategory(u8 categoryId)
{
    const struct OptionCategory *category = &sCategories[categoryId];
    u8 i;

    for (i = 0; i < category->optionCount; i++)
        ResetOptionToDefault(category->optionIds[i]);
}

static void ResetAllOptions(void)
{
    u8 i;

    for (i = 0; i < CATEGORIES_COUNT; i++)
        ResetCategory(i);
}

#define tResetTarget data[0] // Category id, or CATEGORIES_COUNT for reset-all
#define tConfirmSel  data[1] // 0 = NO, 1 = YES

static const u8 sText_ConfirmNo[]  = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}NO");
static const u8 sText_ConfirmYes[] = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}YES");
static const u8 sText_ConfirmResetAll[]      = _("Reset every setting to its default value?");
static const u8 sText_ConfirmResetCategory[] = _("Reset this category's settings to their default values?");

static u8 GetConfirmRowY(void)
{
    u8 item = sCursor[sMenuLevel];

    return (GetVisibleIndexOf(item) - sScrollOffset[sMenuLevel]) * ROW_PITCH;
}

static void DrawConfirmChoice(u8 sel)
{
    DrawOptionMenuValue(sel ? sText_ConfirmYes : sText_ConfirmNo, GetConfirmRowY(), TRUE);
    CopyWindowToVram(WIN_OPTIONS, COPYWIN_GFX);
}

// YES/NO is drawn in the highlighted reset row's value column; the question uses WIN_DESC.
static void StartResetPrompt(u8 taskId, u8 target)
{
    gTasks[taskId].tResetTarget = target;
    gTasks[taskId].tConfirmSel = 0;
    PrintOptionDescription(target == CATEGORIES_COUNT ? sText_ConfirmResetAll : sText_ConfirmResetCategory);
    DrawConfirmChoice(0);
    gTasks[taskId].func = Task_OptionMenuConfirmReset;
}

static void Task_OptionMenuConfirmReset(u8 taskId)
{
    bool8 done = FALSE;
    u8 target = gTasks[taskId].tResetTarget;

    if (JOY_NEW(DPAD_LEFT | DPAD_RIGHT))
    {
        gTasks[taskId].tConfirmSel ^= 1;
        DrawConfirmChoice(gTasks[taskId].tConfirmSel);
        return;
    }
    if (JOY_NEW(A_BUTTON))
    {
        if (gTasks[taskId].tConfirmSel)
        {
            if (target == CATEGORIES_COUNT)
                ResetAllOptions();
            else
                ResetCategory(target);
        }
        done = TRUE;
    }
    else if (JOY_NEW(B_BUTTON))
    {
        done = TRUE;
    }
    if (!done)
        return;

    // Visibility may have changed; rebuild the list, which also re-prints the description.
    if (target == CATEGORIES_COUNT)
    {
        DrawCategoryList();
        gTasks[taskId].func = Task_CategoryMenuProcessInput;
    }
    else
    {
        ClampScroll();
        DrawSettingsList();
        gTasks[taskId].func = Task_SettingsMenuProcessInput;
    }
}

#undef tResetTarget
#undef tConfirmSel

static void EditSettingsRow(void)
{
    const struct OptionCategory *category = &sCategories[sCurrentCategory];
    u8 item = sCursor[LEVEL_SETTINGS];
    u8 countBefore, row;

    if (item >= category->optionCount)
        return;

    countBefore = GetVisibleCount();
    if (!ProcessOptionInput(category->optionIds[item]))
        return;

    // A change that shows or hides other rows needs a full rebuild.
    if (GetVisibleCount() != countBefore)
    {
        ClampScroll();
        DrawSettingsList();
    }
    else
    {
        row = GetVisibleIndexOf(item) - sScrollOffset[LEVEL_SETTINGS];
        DrawSettingsRow(row, item, TRUE);
    }
    if (sArrowPressed)
    {
        sArrowPressed = FALSE;
        CopyWindowToVram(WIN_OPTIONS, COPYWIN_GFX);
    }
}

static void Task_SettingsMenuFadeIn(u8 taskId)
{
    if (!gPaletteFade.active)
        gTasks[taskId].func = Task_SettingsMenuProcessInput;
}

static void Task_SettingsMenuProcessInput(u8 taskId)
{
    if (JOY_NEW(A_BUTTON))
    {
        const struct OptionCategory *category = &sCategories[sCurrentCategory];
        u8 item = sCursor[LEVEL_SETTINGS];

        if (item < category->optionCount)
        {
            switch (category->optionIds[item])
            {
            case OPTION_PLAYER_COLOURS:
                gTasks[taskId].func = Task_OptionMenuOpenPlayerColors;
                break;
            case OPTION_HEALTHBOX:
                gTasks[taskId].func = Task_OptionMenuOpenHealthbox;
                break;
            }
        }
        else
            StartResetPrompt(taskId, sCurrentCategory);
    }
    else if (JOY_NEW(B_BUTTON))
    {
        sMenuLevel = LEVEL_CATEGORIES;
        DrawHeaderText();
        DrawCategoryList();
        gTasks[taskId].func = Task_CategoryMenuProcessInput;
    }
    else if (JOY_NEW(DPAD_UP))
    {
        MoveSettingsCursor(-1);
    }
    else if (JOY_NEW(DPAD_DOWN))
    {
        MoveSettingsCursor(1);
    }
    else if (JOY_NEW(DPAD_LEFT | DPAD_RIGHT))
    {
        EditSettingsRow();
    }
}

#define TILE_TOP_CORNER_L 0x1A2
#define TILE_TOP_EDGE     0x1A3
#define TILE_TOP_CORNER_R 0x1A4
#define TILE_LEFT_EDGE    0x1A5
#define TILE_RIGHT_EDGE   0x1A7
#define TILE_BOT_CORNER_L 0x1A8
#define TILE_BOT_EDGE     0x1A9
#define TILE_BOT_CORNER_R 0x1AA

static void DrawBgWindowFrames(void)
{
    //                     bg, tile,              x, y, width, height, palNum
    // Draw title window frame
    FillBgTilemapBufferRect(1, TILE_TOP_CORNER_L,  1,  0,  1,  1,  7);
    FillBgTilemapBufferRect(1, TILE_TOP_EDGE,      2,  0, 27,  1,  7);
    FillBgTilemapBufferRect(1, TILE_TOP_CORNER_R, 28,  0,  1,  1,  7);
    FillBgTilemapBufferRect(1, TILE_LEFT_EDGE,     1,  1,  1,  2,  7);
    FillBgTilemapBufferRect(1, TILE_RIGHT_EDGE,   28,  1,  1,  2,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_CORNER_L,  1,  3,  1,  1,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_EDGE,      2,  3, 27,  1,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_CORNER_R, 28,  3,  1,  1,  7);

    // Draw options list window frame
    FillBgTilemapBufferRect(1, TILE_TOP_CORNER_L,  1,  4,  1,  1,  7);
    FillBgTilemapBufferRect(1, TILE_TOP_EDGE,      2,  4, 26,  1,  7);
    FillBgTilemapBufferRect(1, TILE_TOP_CORNER_R, 28,  4,  1,  1,  7);
    FillBgTilemapBufferRect(1, TILE_LEFT_EDGE,     1,  5,  1,  8,  7);
    FillBgTilemapBufferRect(1, TILE_RIGHT_EDGE,   28,  5,  1,  8,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_CORNER_L,  1, 13,  1,  1,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_EDGE,      2, 13, 26,  1,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_CORNER_R, 28, 13,  1,  1,  7);

    // Draw description window frame
    FillBgTilemapBufferRect(1, TILE_TOP_CORNER_L,  1, 14,  1,  1,  7);
    FillBgTilemapBufferRect(1, TILE_TOP_EDGE,      2, 14, 26,  1,  7);
    FillBgTilemapBufferRect(1, TILE_TOP_CORNER_R, 28, 14,  1,  1,  7);
    FillBgTilemapBufferRect(1, TILE_LEFT_EDGE,     1, 15,  1,  4,  7);
    FillBgTilemapBufferRect(1, TILE_RIGHT_EDGE,   28, 15,  1,  4,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_CORNER_L,  1, 19,  1,  1,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_EDGE,      2, 19, 26,  1,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_CORNER_R, 28, 19,  1,  1,  7);

    CopyBgTilemapBufferToVram(1);
}
