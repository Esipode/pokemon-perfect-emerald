#include "global.h"
#include "option_menu.h"
#include "achievements.h"
#include "ai_battles.h"
#include "bg.h"
#include "gpu_regs.h"
#include "international_string_util.h"
#include "main.h"
#include "menu.h"
#include "palette.h"
#include "player_palette_menu.h"
#include "scanline_effect.h"
#include "sprite.h"
#include "strings.h"
#include "task.h"
#include "text.h"
#include "text_window.h"
#include "window.h"
#include "gba/m4a_internal.h"
#include "constants/rgb.h"
#include "event_data.h"
#include "string_util.h"

/* Forward declarations */
static void ReadAllCurrentSettings(u8 taskId);
static void DrawOptionsPg1(u8 taskId);
static void DrawOptionsPg2(u8 taskId);

#define tMenuSelection    data[0]
#define tButtonMode       data[1]
#define tWindowFrameType  data[2]
#define tAIBattles        data[3]  // bit0 = trainer AI, bit1 = wild AI
#define tTextSpeed        data[5]

// Packed flags for all boolean options (data[6], bits 0-15)
#define BATTLE_SCENE_SHIFT     0
#define BATTLE_STYLE_SHIFT     1
#define SOUND_SHIFT            2
#define AUTO_SCROLL_SHIFT      3
#define AUTOSAVE_SHIFT         8
#define ACHIEVEMENT_BOOSTS_SHIFT 9

#define tPackedFlags          data[6]  // booleans packed into 16 bits

// Helper macros for packed flag access
#define GET_FLAG(name) ((gTasks[taskId].tPackedFlags >> name##_SHIFT) & 1)
#define SET_FLAG(name, val) do { \
    if (val) gTasks[taskId].tPackedFlags |= (1 << name##_SHIFT); \
    else     gTasks[taskId].tPackedFlags &= ~(1 << name##_SHIFT); \
} while(0)

enum
{
    MENUITEM_TEXTSPEED,
    MENUITEM_BATTLESCENE,
    MENUITEM_BATTLESTYLE,
    MENUITEM_SOUND,
    MENUITEM_BUTTONMODE,
    MENUITEM_FRAMETYPE,
    MENUITEM_CANCEL,
    MENUITEM_COUNT,
};

// Menu items Pg2
enum
{
    MENUITEM_AIBATTLES_TRAINER,
    MENUITEM_AIBATTLES_WILD,
    MENUITEM_AUTOSCROLL,
    MENUITEM_AUTOSAVE,
    MENUITEM_ACHIEVEMENT_BOOSTS,
    MENUITEM_PLAYER_COLORS,
    MENUITEM_CANCEL_PG2,
    MENUITEM_COUNT_PG2,
};

enum
{
    WIN_HEADER,
    WIN_OPTIONS
};

//Pg 1
#define YPOS_TEXTSPEED    (MENUITEM_TEXTSPEED * 16)
#define YPOS_BATTLESCENE  (MENUITEM_BATTLESCENE * 16)
#define YPOS_BATTLESTYLE  (MENUITEM_BATTLESTYLE * 16)
#define YPOS_SOUND        (MENUITEM_SOUND * 16)
#define YPOS_BUTTONMODE   (MENUITEM_BUTTONMODE * 16)
#define YPOS_FRAMETYPE    (MENUITEM_FRAMETYPE * 16)

//Pg2
#define YPOS_AIBATTLES_TRAINER    (MENUITEM_AIBATTLES_TRAINER * 16)
#define YPOS_AIBATTLES_WILD       (MENUITEM_AIBATTLES_WILD * 16)
#define YPOS_AUTOSCROLL           (MENUITEM_AUTOSCROLL * 16)
#define YPOS_AUTOSAVE             (MENUITEM_AUTOSAVE * 16)

#define PAGE_COUNT 2

static void Task_OptionMenuFadeIn(u8 taskId);
static void Task_OptionMenuProcessInput(u8 taskId);
static void Task_OptionMenuFadeIn_Pg2(u8 taskId);
static void Task_OptionMenuProcessInput_Pg2(u8 taskId);
static void Task_OptionMenuSave(u8 taskId);
static void Task_OptionMenuOpenPlayerColors(u8 taskId);
static void Task_OptionMenuFadeOut(u8 taskId);
static void Task_OptionMenuFadeOutToPlayerColors(u8 taskId);
static void HighlightOptionMenuItem(u8 selection);
static u8 TextSpeed_ProcessInput(u8 selection);
static void TextSpeed_DrawChoices(u8 selection, bool8 isActive);
static u8 BattleScene_ProcessInput(u8 selection);
static void BattleScene_DrawChoices(u8 selection, bool8 isActive);
static u8 BattleStyle_ProcessInput(u8 selection);
static void BattleStyle_DrawChoices(u8 selection, bool8 isActive);
static u8 AIBattles_ProcessInput(u8 selection);
static void AIBattles_DrawChoices(u8 selection, bool8 isActive);
static void WildAIBattles_DrawChoices(u8 selection, bool8 isActive);
static u8   AutoScroll_ProcessInput(u8 selection);
static void AutoScroll_DrawChoices(u8 selection, bool8 isActive);
static u8   Autosave_ProcessInput(u8 selection);
static void Autosave_DrawChoices(u8 selection, bool8 isActive);
static u8   AchievementBoosts_ProcessInput(u8 selection);
static void AchievementBoosts_DrawChoices(u8 selection, bool8 isActive);
static u8 Sound_ProcessInput(u8 selection);
static void Sound_DrawChoices(u8 selection, bool8 isActive);
static u8 FrameType_ProcessInput(u8 selection);
static void FrameType_DrawChoices(u8 selection, bool8 isActive);
static u8 ButtonMode_ProcessInput(u8 selection);
static void ButtonMode_DrawChoices(u8 selection, bool8 isActive);
static bool8 IsAutosaveHidden(void);
static bool8 IsAchievementBoostsHidden(void);
static bool8 IsPg2ItemHidden(u8 item);
static u8 GetPg2DisplayRow(u8 item);
static u8 GetNextVisiblePg2Item(u8 sel);
static u8 GetPrevVisiblePg2Item(u8 sel);
static void DrawHeaderText(void);
static void DrawOptionMenuTexts(void);
static void DrawBgWindowFrames(void);
static void DrawOptionMenuChoice(const u8 *text, u8 x, u8 y, u8 style);
static void DrawOptionMenuValue(const u8 *text, u8 y, bool8 isActive);

EWRAM_DATA static bool8 sArrowPressed = FALSE;
EWRAM_DATA static u8 sCurrPage = 0;
// Stashes the real caller (field/main-menu CB2) across the round trip
// through CB2_InitPlayerPaletteMenu, whose own savedCallback is pointed at
// CB2_InitOptionMenu so it lands back on this screen.
// Restored into gMain.savedCallback the next time CB2_InitOptionMenu
// runs; NULL the rest of the time, when gMain.savedCallback already holds
// the real caller and needs no help.
EWRAM_DATA static MainCallback sSavedCallback = NULL;

static const u8 gText_Option[]             = _("OPTION");
static const u8 gText_PageNav[]            = _("PAGE");
static const u8 gText_SmallDot[]           = _("·");
static const u8 gText_LargeDot[]           = _("{EMOJI_CIRCLE}");

static const u8 gText_TextSpeedSlow[]      = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}SLOW");
static const u8 gText_TextSpeedMid[]       = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}MID");
static const u8 gText_TextSpeedFast[]      = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}FAST");
static const u8 gText_BattleSceneOn[]      = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}ON");
static const u8 gText_BattleSceneOff[]     = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}OFF");
static const u8 gText_BattleStyleShift[]   = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}SHIFT");
static const u8 gText_BattleStyleSet[]     = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}SET");
static const u8 gText_SoundMono[]          = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}MONO");
static const u8 gText_SoundStereo[]        = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}STEREO");
static const u8 gText_FrameType[]          = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}TYPE");
static const u8 gText_FrameTypeNumber[]    = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}");
static const u8 gText_ButtonTypeNormal[]   = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}NORMAL");
static const u8 gText_ButtonTypeLR[]       = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}LR");
static const u8 gText_ButtonTypeLEqualsA[] = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}L=A");

// Page 2 strings
static const u8 gText_AIBattlesTrainer[]   = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}TRAINER");
static const u8 gText_AIBattlesWild[]      = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}WILD");
static const u8 gText_AIBattlesOff[]       = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}OFF");
static const u8 gText_AIBattlesOn[]        = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}ON");
static const u8 gText_AutoScroll[]         = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}AUTO-SCROLL");
static const u8 gText_AutoScrollOff[]      = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}OFF");
static const u8 gText_AutoScrollOn[]       = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}ON");
static const u8 gText_Autosave[]           = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}AUTOSAVE");
static const u8 gText_AutosaveOff[]        = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}OFF");
static const u8 gText_AutosaveOn[]         = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}ON");
static const u8 gText_AchievementBoostsOff[] = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}OFF");
static const u8 gText_AchievementBoostsOn[]  = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}ON");

static const u8 sText_ChevronLeft[]        = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}{LEFT_ARROW}");
static const u8 sText_ChevronRight[]       = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}{RIGHT_ARROW}");

static const u16 sOptionMenuText_Pal[] = INCGFX_U16("graphics/interface/option_menu_text.pal", ".gbapal");
// note: this is only used in the Japanese release
static const u8 sEqualSignGfx[] = INCGFX_U8("graphics/interface/option_menu_equals_sign.png", ".4bpp");

static const u8 *const sOptionMenuItemsNames[MENUITEM_COUNT] =
{
    [MENUITEM_TEXTSPEED]   = COMPOUND_STRING("TEXT SPEED"),
    [MENUITEM_BATTLESCENE] = COMPOUND_STRING("BATTLE SCENE"),
    [MENUITEM_BATTLESTYLE] = COMPOUND_STRING("BATTLE STYLE"),
    [MENUITEM_SOUND]       = COMPOUND_STRING("SOUND"),
    [MENUITEM_BUTTONMODE]  = COMPOUND_STRING("BUTTON MODE"),
    [MENUITEM_FRAMETYPE]   = COMPOUND_STRING("FRAME"),
    [MENUITEM_CANCEL]      = COMPOUND_STRING("CANCEL"),
};

static const u8 *const sOptionMenuItemsNames_Pg2[MENUITEM_COUNT_PG2] =
{
    [MENUITEM_AIBATTLES_TRAINER] = COMPOUND_STRING("AI TRAINER BATTLES"),
    [MENUITEM_AIBATTLES_WILD]    = COMPOUND_STRING("AI WILD BATTLES"),
    [MENUITEM_AUTOSCROLL]        = COMPOUND_STRING("AUTO SCROLL"),
    [MENUITEM_AUTOSAVE]          = COMPOUND_STRING("AUTOSAVE"),
    [MENUITEM_ACHIEVEMENT_BOOSTS] = COMPOUND_STRING("ACHIEVEMENT BOOSTS"),
    [MENUITEM_PLAYER_COLORS]     = COMPOUND_STRING("PLAYER COLOURS"),
    [MENUITEM_CANCEL_PG2]        = COMPOUND_STRING("CANCEL"),
};

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
        .height = 14,
        .paletteNum = 1,
        .baseBlock = 0x36
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

static void MainCB2(void)
{
    RunTasks();
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

static void ReadAllCurrentSettings(u8 taskId)
{
    gTasks[taskId].tMenuSelection = 0;
    gTasks[taskId].tTextSpeed = gSaveBlock2Ptr->optionsTextSpeed;
    gTasks[taskId].tButtonMode = gSaveBlock2Ptr->optionsButtonMode;
    gTasks[taskId].tWindowFrameType = gSaveBlock2Ptr->optionsWindowFrameType;
    gTasks[taskId].tAIBattles = (AiBattles_GetSetting(AI_BATTLES_SETTING_TRAINER) ? 1 : 0) | (AiBattles_GetSetting(AI_BATTLES_SETTING_WILD) ? 2 : 0);
    gTasks[taskId].tPackedFlags = 0;
    if (gSaveBlock2Ptr->optionsBattleSceneOff)     SET_FLAG(BATTLE_SCENE, 1); else SET_FLAG(BATTLE_SCENE, 0);
    if (gSaveBlock2Ptr->optionsBattleStyle)        SET_FLAG(BATTLE_STYLE, 1); else SET_FLAG(BATTLE_STYLE, 0);
    if (gSaveBlock2Ptr->optionsSound)              SET_FLAG(SOUND, 1); else SET_FLAG(SOUND, 0);
    if (FlagGet(FLAG_AUTO_SCROLL_TEXT))            SET_FLAG(AUTO_SCROLL, 1); else SET_FLAG(AUTO_SCROLL, 0);
    if (gSaveBlock1Ptr->autosaveModeEnabled)       SET_FLAG(AUTOSAVE, 1); else SET_FLAG(AUTOSAVE, 0);
    if (Achievement_BoostsEnabled())               SET_FLAG(ACHIEVEMENT_BOOSTS, 1); else SET_FLAG(ACHIEVEMENT_BOOSTS, 0);
}

static void DrawOptionsPg1(u8 taskId)
{
    u8 sel = gTasks[taskId].tMenuSelection;

    TextSpeed_DrawChoices(gTasks[taskId].tTextSpeed, sel == MENUITEM_TEXTSPEED);
    BattleScene_DrawChoices(GET_FLAG(BATTLE_SCENE), sel == MENUITEM_BATTLESCENE);
    BattleStyle_DrawChoices(GET_FLAG(BATTLE_STYLE), sel == MENUITEM_BATTLESTYLE);
    Sound_DrawChoices(GET_FLAG(SOUND), sel == MENUITEM_SOUND);
    ButtonMode_DrawChoices(gTasks[taskId].tButtonMode, sel == MENUITEM_BUTTONMODE);
    FrameType_DrawChoices(gTasks[taskId].tWindowFrameType, sel == MENUITEM_FRAMETYPE);
    HighlightOptionMenuItem(sel);
    CopyWindowToVram(WIN_OPTIONS, COPYWIN_FULL);
}

static void DrawOptionsPg2(u8 taskId)
{
    u8 sel = gTasks[taskId].tMenuSelection;

    AIBattles_DrawChoices(gTasks[taskId].tAIBattles & 1, sel == MENUITEM_AIBATTLES_TRAINER);
    WildAIBattles_DrawChoices((gTasks[taskId].tAIBattles & 2) ? 1 : 0, sel == MENUITEM_AIBATTLES_WILD);
    AutoScroll_DrawChoices(GET_FLAG(AUTO_SCROLL), sel == MENUITEM_AUTOSCROLL);
    if (!IsAutosaveHidden())
        Autosave_DrawChoices(GET_FLAG(AUTOSAVE), sel == MENUITEM_AUTOSAVE);
    if (!IsAchievementBoostsHidden())
        AchievementBoosts_DrawChoices(GET_FLAG(ACHIEVEMENT_BOOSTS), sel == MENUITEM_ACHIEVEMENT_BOOSTS);
    HighlightOptionMenuItem(GetPg2DisplayRow(sel));
    CopyWindowToVram(WIN_OPTIONS, COPYWIN_FULL);
}

void CB2_InitOptionMenu(void)
{
    u8 taskId;
    switch (gMain.state)
    {
    default:
    case 0:
        SetVBlankCallback(NULL);
        // Landing back here from the player-colors screen -- restore the
        // real caller that Task_OptionMenuOpenPlayerColors stashed before
        // overwriting gMain.savedCallback with CB2_InitOptionMenu itself.
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
        DrawOptionMenuTexts();
        gMain.state++;
        break;
    case 9:
        DrawBgWindowFrames();
        gMain.state++;
        break;
    case 10:
    {
        taskId = CreateTask(Task_OptionMenuFadeIn, 0);
        ReadAllCurrentSettings(taskId);
        switch(sCurrPage)
        {
        case 0:
            DrawOptionsPg1(taskId);
            gTasks[taskId].func = Task_OptionMenuFadeIn;
            break;
        case 1:
            DrawOptionsPg2(taskId);
            gTasks[taskId].func = Task_OptionMenuFadeIn_Pg2;
            break;
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

static void Task_OptionMenuFadeIn(u8 taskId)
{
    if (!gPaletteFade.active)
        gTasks[taskId].func = Task_OptionMenuProcessInput;
}

static u8 Process_ChangePage(u8 CurrentPage)
{
    if (JOY_NEW(R_BUTTON))
    {
        if (CurrentPage < PAGE_COUNT - 1)
            CurrentPage++;
        else
            CurrentPage = 0;
    }
    if (JOY_NEW(L_BUTTON))
    {
        if (CurrentPage != 0)
            CurrentPage--;
        else
            CurrentPage = PAGE_COUNT - 1;
    }
    return CurrentPage;
}

static void Task_ChangePage(u8 taskId)
{
    DrawHeaderText();
    PutWindowTilemap(1);
    DrawOptionMenuTexts();
    switch(sCurrPage)
    {
    case 0:
        DrawOptionsPg1(taskId);
        gTasks[taskId].func = Task_OptionMenuFadeIn;
        break;
    case 1:
        DrawOptionsPg2(taskId);
        gTasks[taskId].func = Task_OptionMenuFadeIn_Pg2;
        break;
    }
}

static void Task_OptionMenuProcessInput(u8 taskId)
{
    if (JOY_NEW(L_BUTTON) || JOY_NEW(R_BUTTON))
    {
        FillWindowPixelBuffer(WIN_OPTIONS, PIXEL_FILL(1));
        ClearStdWindowAndFrame(WIN_OPTIONS, FALSE);
        sCurrPage = Process_ChangePage(sCurrPage);
        gTasks[taskId].func = Task_ChangePage;
    }
    else if (JOY_NEW(A_BUTTON))
    {
        if (gTasks[taskId].tMenuSelection == MENUITEM_CANCEL)
            gTasks[taskId].func = Task_OptionMenuSave;
    }
    else if (JOY_NEW(B_BUTTON))
    {
        gTasks[taskId].func = Task_OptionMenuSave;
    }
    else if (JOY_NEW(DPAD_UP))
    {
        if (gTasks[taskId].tMenuSelection > 0)
            gTasks[taskId].tMenuSelection--;
        else
            gTasks[taskId].tMenuSelection = MENUITEM_CANCEL;
        DrawOptionsPg1(taskId);
    }
    else if (JOY_NEW(DPAD_DOWN))
    {
        if (gTasks[taskId].tMenuSelection < MENUITEM_CANCEL)
            gTasks[taskId].tMenuSelection++;
        else
            gTasks[taskId].tMenuSelection = 0;
        DrawOptionsPg1(taskId);
    }
    else
    {
        u8 previousOption;

        switch (gTasks[taskId].tMenuSelection)
        {
        case MENUITEM_TEXTSPEED:
            previousOption = gTasks[taskId].tTextSpeed;
            gTasks[taskId].tTextSpeed = TextSpeed_ProcessInput(gTasks[taskId].tTextSpeed);

            if (previousOption != gTasks[taskId].tTextSpeed)
                TextSpeed_DrawChoices(gTasks[taskId].tTextSpeed, TRUE);
            break;
        case MENUITEM_BATTLESCENE:
            previousOption = GET_FLAG(BATTLE_SCENE);
            gTasks[taskId].tPackedFlags = (gTasks[taskId].tPackedFlags & ~(1 << BATTLE_SCENE_SHIFT)) | (BattleScene_ProcessInput(previousOption) << BATTLE_SCENE_SHIFT);

            if (previousOption != GET_FLAG(BATTLE_SCENE))
                BattleScene_DrawChoices(GET_FLAG(BATTLE_SCENE), TRUE);
            break;
        case MENUITEM_BATTLESTYLE:
            previousOption = GET_FLAG(BATTLE_STYLE);
            gTasks[taskId].tPackedFlags = (gTasks[taskId].tPackedFlags & ~(1 << BATTLE_STYLE_SHIFT)) | (BattleStyle_ProcessInput(previousOption) << BATTLE_STYLE_SHIFT);

            if (previousOption != GET_FLAG(BATTLE_STYLE))
                BattleStyle_DrawChoices(GET_FLAG(BATTLE_STYLE), TRUE);
            break;
        case MENUITEM_SOUND:
            previousOption = GET_FLAG(SOUND);
            gTasks[taskId].tPackedFlags = (gTasks[taskId].tPackedFlags & ~(1 << SOUND_SHIFT)) | (Sound_ProcessInput(previousOption) << SOUND_SHIFT);

            if (previousOption != GET_FLAG(SOUND))
                Sound_DrawChoices(GET_FLAG(SOUND), TRUE);
            break;
        case MENUITEM_BUTTONMODE:
            previousOption = gTasks[taskId].tButtonMode;
            gTasks[taskId].tButtonMode = ButtonMode_ProcessInput(gTasks[taskId].tButtonMode);

            if (previousOption != gTasks[taskId].tButtonMode)
                ButtonMode_DrawChoices(gTasks[taskId].tButtonMode, TRUE);
            break;
        case MENUITEM_FRAMETYPE:
            previousOption = gTasks[taskId].tWindowFrameType;
            gTasks[taskId].tWindowFrameType = FrameType_ProcessInput(gTasks[taskId].tWindowFrameType);

            if (previousOption != gTasks[taskId].tWindowFrameType)
                FrameType_DrawChoices(gTasks[taskId].tWindowFrameType, TRUE);
            break;
        default:
            return;
        }

        if (sArrowPressed)
        {
            sArrowPressed = FALSE;
            CopyWindowToVram(WIN_OPTIONS, COPYWIN_GFX);
        }
    }
}

static void Task_OptionMenuFadeIn_Pg2(u8 taskId)
{
    if (!gPaletteFade.active)
        gTasks[taskId].func = Task_OptionMenuProcessInput_Pg2;
}

static void Task_OptionMenuProcessInput_Pg2(u8 taskId)
{
    if (JOY_NEW(L_BUTTON) || JOY_NEW(R_BUTTON))
    {
        FillWindowPixelBuffer(WIN_OPTIONS, PIXEL_FILL(1));
        ClearStdWindowAndFrame(WIN_OPTIONS, FALSE);
        sCurrPage = Process_ChangePage(sCurrPage);
        gTasks[taskId].func = Task_ChangePage;
    }
    else if (JOY_NEW(A_BUTTON))
    {
        if (gTasks[taskId].tMenuSelection == MENUITEM_CANCEL_PG2)
            gTasks[taskId].func = Task_OptionMenuSave;
        else if (gTasks[taskId].tMenuSelection == MENUITEM_PLAYER_COLORS)
            gTasks[taskId].func = Task_OptionMenuOpenPlayerColors;
    }
    else if (JOY_NEW(B_BUTTON))
    {
        gTasks[taskId].func = Task_OptionMenuSave;
    }
    else if (JOY_NEW(DPAD_UP))
    {
        gTasks[taskId].tMenuSelection = GetPrevVisiblePg2Item(gTasks[taskId].tMenuSelection);
        DrawOptionsPg2(taskId);
    }
    else if (JOY_NEW(DPAD_DOWN))
    {
        gTasks[taskId].tMenuSelection = GetNextVisiblePg2Item(gTasks[taskId].tMenuSelection);
        DrawOptionsPg2(taskId);
    }
    else
    {
        u8 previousOption;

        switch (gTasks[taskId].tMenuSelection)
        {
        case MENUITEM_AIBATTLES_TRAINER:
        {
            u8 prev = gTasks[taskId].tAIBattles & 1;
            u8 sel = AIBattles_ProcessInput(prev);
            if (prev != sel)
            {
                if (sel)
                    gTasks[taskId].tAIBattles |= 1;
                else
                    gTasks[taskId].tAIBattles &= ~1;
                AIBattles_DrawChoices(sel, TRUE);
            }
            break;
        }
        case MENUITEM_AIBATTLES_WILD:
        {
            u8 prev = (gTasks[taskId].tAIBattles & 2) ? 1 : 0;
            u8 sel = AIBattles_ProcessInput(prev);
            if (prev != sel)
            {
                if (sel)
                    gTasks[taskId].tAIBattles |= 2;
                else
                    gTasks[taskId].tAIBattles &= ~2;
                WildAIBattles_DrawChoices(sel, TRUE);
            }
            break;
        }
        case MENUITEM_AUTOSCROLL:
            previousOption = GET_FLAG(AUTO_SCROLL);
            gTasks[taskId].tPackedFlags = (gTasks[taskId].tPackedFlags & ~(1 << AUTO_SCROLL_SHIFT)) | (AutoScroll_ProcessInput(previousOption) << AUTO_SCROLL_SHIFT);

            if (previousOption != GET_FLAG(AUTO_SCROLL))
                AutoScroll_DrawChoices(GET_FLAG(AUTO_SCROLL), TRUE);
            break;
        case MENUITEM_AUTOSAVE:
            if (IsAutosaveHidden())
                break;
            previousOption = GET_FLAG(AUTOSAVE);
            gTasks[taskId].tPackedFlags = (gTasks[taskId].tPackedFlags & ~(1 << AUTOSAVE_SHIFT)) | (Autosave_ProcessInput(previousOption) << AUTOSAVE_SHIFT);

            if (previousOption != GET_FLAG(AUTOSAVE))
                Autosave_DrawChoices(GET_FLAG(AUTOSAVE), TRUE);
            break;
        case MENUITEM_ACHIEVEMENT_BOOSTS:
            if (IsAchievementBoostsHidden())
                break;
            previousOption = GET_FLAG(ACHIEVEMENT_BOOSTS);
            gTasks[taskId].tPackedFlags = (gTasks[taskId].tPackedFlags & ~(1 << ACHIEVEMENT_BOOSTS_SHIFT)) | (AchievementBoosts_ProcessInput(previousOption) << ACHIEVEMENT_BOOSTS_SHIFT);

            if (previousOption != GET_FLAG(ACHIEVEMENT_BOOSTS))
                AchievementBoosts_DrawChoices(GET_FLAG(ACHIEVEMENT_BOOSTS), TRUE);
            break;
        default:
            return;
        }

        if (sArrowPressed)
        {
            sArrowPressed = FALSE;
            CopyWindowToVram(WIN_OPTIONS, COPYWIN_GFX);
        }
    }
}

// Shared by Task_OptionMenuSave and Task_OptionMenuOpenPlayerColors -- the
// pending settings need committing before either leaving the menu for good
// or hopping to the player-colors screen and back (so a change made just
// before opening PLAYER COLOURS isn't lost if the player never returns to
// CANCEL, e.g. quits from the palette screen's own exit some other way).
static void CommitPendingOptionSettings(u8 taskId)
{
    gSaveBlock2Ptr->optionsTextSpeed = gTasks[taskId].tTextSpeed;
    gSaveBlock2Ptr->optionsBattleSceneOff = GET_FLAG(BATTLE_SCENE);
    gSaveBlock2Ptr->optionsBattleStyle = GET_FLAG(BATTLE_STYLE);
    gSaveBlock2Ptr->optionsSound = GET_FLAG(SOUND);
    gSaveBlock2Ptr->optionsButtonMode = gTasks[taskId].tButtonMode;
    gSaveBlock2Ptr->optionsWindowFrameType = gTasks[taskId].tWindowFrameType;
    /* Save trainer and wild AI flags from the bitmask */
    AiBattles_SetSetting(AI_BATTLES_SETTING_TRAINER, (gTasks[taskId].tAIBattles & 1) != 0);
    AiBattles_SetSetting(AI_BATTLES_SETTING_WILD, (gTasks[taskId].tAIBattles & 2) != 0);
    GET_FLAG(AUTO_SCROLL) == 0 ? FlagClear(FLAG_AUTO_SCROLL_TEXT) : FlagSet(FLAG_AUTO_SCROLL_TEXT);
    if (!IsAutosaveHidden())
        GET_FLAG(AUTOSAVE) == 0 ? (gSaveBlock1Ptr->autosaveModeEnabled = 0) : (gSaveBlock1Ptr->autosaveModeEnabled = 1);
    if (!IsAchievementBoostsHidden())
    {
        Achievement_SetBoostsEnabled(GET_FLAG(ACHIEVEMENT_BOOSTS));
        Achievement_FlushProfile();
    }
}

static void Task_OptionMenuSave(u8 taskId)
{
    CommitPendingOptionSettings(taskId);
    BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
    gTasks[taskId].func = Task_OptionMenuFadeOut;
}

static void Task_OptionMenuFadeOut(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        DestroyTask(taskId);
        FreeAllWindowBuffers();
        SetMainCallback2(gMain.savedCallback);
    }
}

// PLAYER COLOURS is an action row like CANCEL, but instead of leaving the
// option menu for good it hops to CB2_InitPlayerPaletteMenu and back.
static void Task_OptionMenuOpenPlayerColors(u8 taskId)
{
    CommitPendingOptionSettings(taskId);
    BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
    gTasks[taskId].func = Task_OptionMenuFadeOutToPlayerColors;
}

static void Task_OptionMenuFadeOutToPlayerColors(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        DestroyTask(taskId);
        FreeAllWindowBuffers();
        // sCurrPage (EWRAM) is untouched, so CB2_InitOptionMenu redraws page 2
        // on return; sSavedCallback carries the real caller across the trip.
        sSavedCallback = gMain.savedCallback;
        gMain.savedCallback = CB2_InitOptionMenu;
        SetMainCallback2(CB2_InitPlayerPaletteMenu);
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

static u8 TextSpeed_ProcessInput(u8 selection)
{
    if (JOY_NEW(DPAD_RIGHT))
    {
        if (selection <= 1)
            selection++;
        else
            selection = 0;

        sArrowPressed = TRUE;
    }
    if (JOY_NEW(DPAD_LEFT))
    {
        if (selection != 0)
            selection--;
        else
            selection = 2;

        sArrowPressed = TRUE;
    }
    return selection;
}

static void TextSpeed_DrawChoices(u8 selection, bool8 isActive)
{
    static const u8 *const sTexts[3] = {gText_TextSpeedSlow, gText_TextSpeedMid, gText_TextSpeedFast};

    DrawOptionMenuValue(sTexts[selection], YPOS_TEXTSPEED, isActive);
}

static u8 BattleScene_ProcessInput(u8 selection)
{
    if (JOY_NEW(DPAD_LEFT | DPAD_RIGHT))
    {
        selection ^= 1;
        sArrowPressed = TRUE;
    }

    return selection;
}

static void BattleScene_DrawChoices(u8 selection, bool8 isActive)
{
    static const u8 *const sTexts[2] = {gText_BattleSceneOn, gText_BattleSceneOff};

    DrawOptionMenuValue(sTexts[selection], YPOS_BATTLESCENE, isActive);
}

static u8 BattleStyle_ProcessInput(u8 selection)
{
    if (JOY_NEW(DPAD_LEFT | DPAD_RIGHT))
    {
        selection ^= 1;
        sArrowPressed = TRUE;
    }

    return selection;
}

static void BattleStyle_DrawChoices(u8 selection, bool8 isActive)
{
    static const u8 *const sTexts[2] = {gText_BattleStyleShift, gText_BattleStyleSet};

    DrawOptionMenuValue(sTexts[selection], YPOS_BATTLESTYLE, isActive);
}

static u8 Sound_ProcessInput(u8 selection)
{
    if (JOY_NEW(DPAD_LEFT | DPAD_RIGHT))
    {
        selection ^= 1;
        SetPokemonCryStereo(selection);
        sArrowPressed = TRUE;
    }

    return selection;
}

static void Sound_DrawChoices(u8 selection, bool8 isActive)
{
    static const u8 *const sTexts[2] = {gText_SoundMono, gText_SoundStereo};

    DrawOptionMenuValue(sTexts[selection], YPOS_SOUND, isActive);
}

static u8 FrameType_ProcessInput(u8 selection)
{
    if (JOY_NEW(DPAD_RIGHT))
    {
        if (selection < WINDOW_FRAMES_COUNT - 1)
            selection++;
        else
            selection = 0;

        LoadBgTiles(1, GetWindowFrameTilesPal(selection)->tiles, 0x120, 0x1A2);
        LoadPalette(GetWindowFrameTilesPal(selection)->pal, BG_PLTT_ID(7), PLTT_SIZE_4BPP);
        sArrowPressed = TRUE;
    }
    if (JOY_NEW(DPAD_LEFT))
    {
        if (selection != 0)
            selection--;
        else
            selection = WINDOW_FRAMES_COUNT - 1;

        LoadBgTiles(1, GetWindowFrameTilesPal(selection)->tiles, 0x120, 0x1A2);
        LoadPalette(GetWindowFrameTilesPal(selection)->pal, BG_PLTT_ID(7), PLTT_SIZE_4BPP);
        sArrowPressed = TRUE;
    }
    return selection;
}

static void FrameType_DrawChoices(u8 selection, bool8 isActive)
{
    u8 text[16] = {EOS};
    u8 n = selection + 1;
    u16 i;

    for (i = 0; gText_FrameTypeNumber[i] != EOS && i <= 5; i++)
        text[i] = gText_FrameTypeNumber[i];

    // Convert a number to decimal string
    if (n / 10 != 0)
    {
        text[i] = n / 10 + CHAR_0;
        i++;
        text[i] = n % 10 + CHAR_0;
        i++;
    }
    else
    {
        text[i] = n % 10 + CHAR_0;
        i++;
    }

    text[i] = EOS;

    DrawOptionMenuValue(text, YPOS_FRAMETYPE, isActive);
}

static u8 ButtonMode_ProcessInput(u8 selection)
{
    if (JOY_NEW(DPAD_RIGHT))
    {
        if (selection <= 1)
            selection++;
        else
            selection = 0;

        sArrowPressed = TRUE;
    }
    if (JOY_NEW(DPAD_LEFT))
    {
        if (selection != 0)
            selection--;
        else
            selection = 2;

        sArrowPressed = TRUE;
    }
    return selection;
}

static void ButtonMode_DrawChoices(u8 selection, bool8 isActive)
{
    static const u8 *const sTexts[3] = {gText_ButtonTypeNormal, gText_ButtonTypeLR, gText_ButtonTypeLEqualsA};

    DrawOptionMenuValue(sTexts[selection], YPOS_BUTTONMODE, isActive);
}
static u8 AIBattles_ProcessInput(u8 selection)
{
    if (JOY_NEW(DPAD_LEFT | DPAD_RIGHT))
    {
        selection ^= 1;
        sArrowPressed = TRUE;
    }

    return selection;
}

static void AIBattles_DrawChoices(u8 selection, bool8 isActive)
{
    static const u8 *const sTexts[2] = {gText_AIBattlesOff, gText_AIBattlesOn};

    DrawOptionMenuValue(sTexts[selection], YPOS_AIBATTLES_TRAINER, isActive);
}

static void WildAIBattles_DrawChoices(u8 selection, bool8 isActive)
{
    static const u8 *const sTexts[2] = {gText_AIBattlesOff, gText_AIBattlesOn};

    DrawOptionMenuValue(sTexts[selection], YPOS_AIBATTLES_WILD, isActive);
}

static u8 AutoScroll_ProcessInput(u8 selection)
{
    if (JOY_NEW(DPAD_LEFT | DPAD_RIGHT))
    {
        selection ^= 1;
        sArrowPressed = TRUE;
    }

    return selection;
}

static void AutoScroll_DrawChoices(u8 selection, bool8 isActive)
{
    static const u8 *const sTexts[2] = {gText_AutoScrollOff, gText_AutoScrollOn};

    DrawOptionMenuValue(sTexts[selection], YPOS_AUTOSCROLL, isActive);
}


static u8 Autosave_ProcessInput(u8 selection)
{
    if (JOY_NEW(DPAD_LEFT | DPAD_RIGHT))
    {
        selection ^= 1;
        sArrowPressed = TRUE;
    }

    return selection;
}

static void Autosave_DrawChoices(u8 selection, bool8 isActive)
{
    static const u8 *const sTexts[2] = {gText_AutosaveOff, gText_AutosaveOn};

    DrawOptionMenuValue(sTexts[selection], YPOS_AUTOSAVE, isActive);
}

static u8 AchievementBoosts_ProcessInput(u8 selection)
{
    if (JOY_NEW(DPAD_LEFT | DPAD_RIGHT))
    {
        selection ^= 1;
        sArrowPressed = TRUE;
    }

    return selection;
}

// Row position depends on GetPg2DisplayRow rather than a fixed YPOS_* macro
// -- AUTOSAVE (the row above this one) can be hidden (see IsAutosaveHidden()),
// which would otherwise leave a gap between AUTO SCROLL and this row.
static void AchievementBoosts_DrawChoices(u8 selection, bool8 isActive)
{
    static const u8 *const sTexts[2] = {gText_AchievementBoostsOff, gText_AchievementBoostsOn};

    DrawOptionMenuValue(sTexts[selection], GetPg2DisplayRow(MENUITEM_ACHIEVEMENT_BOOSTS) * 16, isActive);
}

static void DrawHeaderText(void)
{
    u32 i, widthOptions, xMid;
    u8 pageDots[9] = _("");  // Array size should be at least (2 * PAGE_COUNT) -1
    widthOptions = GetStringWidth(FONT_NORMAL, gText_Option, 0);

    for (i = 0; i < PAGE_COUNT; i++)
    {
        if (i == sCurrPage)
            StringAppend(pageDots, gText_LargeDot);
        else
            StringAppend(pageDots, gText_SmallDot);
        if (i < PAGE_COUNT - 1)
            StringAppend(pageDots, gText_Space);            
    }
    xMid = (8 + widthOptions + 5);
    FillWindowPixelBuffer(WIN_HEADER, PIXEL_FILL(1));
    AddTextPrinterParameterized(WIN_HEADER, FONT_NORMAL, gText_Option, 8, 1, TEXT_SKIP_DRAW, NULL);
    AddTextPrinterParameterized(WIN_HEADER, FONT_NORMAL, pageDots, xMid, 1, TEXT_SKIP_DRAW, NULL);
    AddTextPrinterParameterized(WIN_HEADER, FONT_NORMAL, gText_PageNav, GetStringRightAlignXOffset(FONT_NORMAL, gText_PageNav, 198), 1, TEXT_SKIP_DRAW, NULL);
    CopyWindowToVram(WIN_HEADER, COPYWIN_FULL);
}

static bool8 IsAutosaveHidden(void)
{
    // Nuzlocke mode forcibly enables autosaving, so the option is redundant and hidden.
    // Draft mode leaves it as a normal, player-controlled option instead - unlike
    // Nuzlocke's permadeath, nothing about Draft requires autosaving to be forced on.
    return gSaveBlock1Ptr->nuzlockeModeEnabled;
}

// Hidden until the first-playthrough gate
// unlocks boosts -- Achievement_BoostsUnlocked() is the same profile flag
// Achievement_OnFirstPlaythroughComplete() sets.
static bool8 IsAchievementBoostsHidden(void)
{
    return !Achievement_BoostsUnlocked();
}

static bool8 IsPg2ItemHidden(u8 item)
{
    switch (item)
    {
    case MENUITEM_AUTOSAVE:
        return IsAutosaveHidden();
    case MENUITEM_ACHIEVEMENT_BOOSTS:
        return IsAchievementBoostsHidden();
    default:
        return FALSE;
    }
}

// Pg2 can now have more than one independently-hideable row above CANCEL
// (AUTOSAVE, ACHIEVEMENT_BOOSTS), so a hidden row's slot has to be
// compacted out of every row below it, not just the one right before
// CANCEL. This counts how many *visible* rows come before `item`, which is
// the row it actually gets drawn on -- DrawOptionMenuTexts's own `row`
// counter below computes the same thing inline while it prints labels.
static u8 GetPg2DisplayRow(u8 item)
{
    u8 row = 0, i;

    for (i = 0; i < item; i++)
    {
        if (!IsPg2ItemHidden(i))
            row++;
    }
    return row;
}

static u8 GetNextVisiblePg2Item(u8 sel)
{
    do
    {
        sel = (sel == MENUITEM_CANCEL_PG2) ? 0 : sel + 1;
    } while (IsPg2ItemHidden(sel));
    return sel;
}

static u8 GetPrevVisiblePg2Item(u8 sel)
{
    do
    {
        sel = (sel == 0) ? MENUITEM_CANCEL_PG2 : sel - 1;
    } while (IsPg2ItemHidden(sel));
    return sel;
}

static void DrawOptionMenuTexts(void)
{
    u8 i, row;

    FillWindowPixelBuffer(WIN_OPTIONS, PIXEL_FILL(1));

    switch (sCurrPage){
    default:
    case 0:
        for (i = 0; i < MENUITEM_COUNT; i++)
            AddTextPrinterParameterized(WIN_OPTIONS, FONT_NARROW, sOptionMenuItemsNames[i], 8, (i * 16) + 1, TEXT_SKIP_DRAW, NULL);
        break;
    case 1:
        row = 0;
        for (i = 0; i < MENUITEM_COUNT_PG2; i++)
        {
            if (IsPg2ItemHidden(i))
                continue;
            AddTextPrinterParameterized(WIN_OPTIONS, FONT_NARROW, sOptionMenuItemsNames_Pg2[i], 8, (row * 16) + 1, TEXT_SKIP_DRAW, NULL);
            row++;
        }
        break;
    }
    CopyWindowToVram(WIN_OPTIONS, COPYWIN_FULL);
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
    FillBgTilemapBufferRect(1, TILE_LEFT_EDGE,     1,  5,  1, 18,  7);
    FillBgTilemapBufferRect(1, TILE_RIGHT_EDGE,   28,  5,  1, 18,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_CORNER_L,  1, 19,  1,  1,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_EDGE,      2, 19, 26,  1,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_CORNER_R, 28, 19,  1,  1,  7);

    CopyBgTilemapBufferToVram(1);
}
