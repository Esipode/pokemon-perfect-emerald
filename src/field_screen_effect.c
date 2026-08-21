#include "global.h"
#include "cable_club.h"
#include "event_data.h"
#include "fieldmap.h"
#include "field_camera.h"
#include "field_door.h"
#include "field_effect.h"
#include "event_object_lock.h"
#include "event_object_movement.h"
#include "event_scripts.h"
#include "field_player_avatar.h"
#include "field_screen_effect.h"
#include "field_special_scene.h"
#include "field_weather.h"
#include "follower_npc.h"
#include "gpu_regs.h"
#include "heal_location.h"
#include "international_string_util.h"
#include "io_reg.h"
#include "keep_storage_prompt.h"
#include "link.h"
#include "load_save.h"
#include "main.h"
#include "map_preview_screen.h"
#include "menu.h"
#include "mirage_tower.h"
#include "metatile_behavior.h"
#include "new_game_settings_menu.h"
#include "palette.h"
#include "oras_dowse.h"
#include "overworld.h"
#include "pokemon.h"
#include "pokemon_storage_system.h"
#include "recruits_mode.h"
#include "scanline_effect.h"
#include "script.h"
#include "sound.h"
#include "start_menu.h"
#include "strings.h"
#include "string_util.h"
#include "task.h"
#include "text.h"
#include "title_screen.h"
#include "constants/event_object_movement.h"
#include "constants/event_objects.h"
#include "constants/heal_locations.h"
#include "constants/songs.h"
#include "constants/rgb.h"
#include "trainer_hill.h"
#include "fldeff.h"
#include "battle.h"

static void Task_ExitNonAnimDoor(u8);
static void Task_ExitNonDoor(u8);
static void Task_DoContestHallWarp(u8);
static void FillPalBufferWhite(void);
static void Task_ExitDoor(u8);
static bool32 WaitForWeatherFadeIn(void);
static void Task_SpinEnterWarp(u8 taskId);
static void Task_EnableScriptAfterMusicFade(u8 taskId);

static void ExitStairsMovement(s16*, s16*, s16*, s16*, s16*);
static void GetStairsMovementDirection(u32, s16*, s16*);
static void Task_ExitStairs(u8);
static bool8 WaitStairExitMovementFinished(s16*, s16*, s16*, s16*, s16*);
static void UpdateStairsMovement(s16, s16, s16*, s16*, s16*);
static void Task_StairWarp(u8);
static void ForceStairsMovement(u32, s16*, s16*);

static const u8 sText_PlayerScurriedToCenter[] = _("{PLAYER} scurried to a POKéMON CENTER,\nprotecting the exhausted and fainted\nPOKéMON from further harm…\p");
static const u8 sText_PlayerScurriedBackHome[] = _("{PLAYER} scurried back home, protecting\nthe exhausted and fainted POKéMON from\nfurther harm…\p");
static const u8 sText_PlayerRegroupCenter[] = _("{PLAYER} scurried to a POKéMON CENTER,\nto regroup and reconsider the battle\nstrategy…\p");
static const u8 sText_PlayerRegroupHome[] = _("{PLAYER} scurried back home, to regroup\nand reconsider the battle strategy…\p");

static const u8 sText_NuzlockeRunFailed[] = _("The NUZLOCKE run has ended in defeat!\nAll of {PLAYER}'s POKéMON have fainted…\p");
static const u8 sText_NuzlockeBeginNewRun[] = _("Begin a new NUZLOCKE run?");
// Recruits variants of the two texts above - this screen is shared between
// both modes, see PrintNuzlockeFailedMessage's callers in Task_NuzlockeRunFailed.
static const u8 sText_RecruitsRunFailed[] = _("Your last recruit's service has ended!\nAll of {PLAYER}'s POKéMON have fainted…\p");
static const u8 sText_RecruitsBeginNewRun[] = _("Begin a new RECRUITS run?");
// Split into two single-line pages rather than one two-line page: the box
// below sits at a fixed screen position that only leaves room for a single
// line above it once this text is the last thing on screen (see
// sText_NuzlockeBeginNewRun, the only other question this screen shows
// directly before its Yes/No box) -- a two-line page here would run its
// second line straight into the box.
static const u8 sText_NuzlockeKeepStorageInfo[] = _("You have POKéMON stored in your PC.\p");
static const u8 sText_NuzlockeKeepStorageQuestion[] = _("Keep them for your new adventure?");

// data[0] is used universally by tasks in this file as a state for switches
#define tState       data[0]

// Smaller flash level -> larger flash radius
static const u16 sFlashLevelToRadius[] = { 200, 72, 64, 56, 48, 40, 32, 24, 0 };
const s32 gMaxFlashLevel = ARRAY_COUNT(sFlashLevelToRadius) - 1;

static const struct ScanlineEffectParams sFlashEffectParams =
{
    &REG_WIN0H,
    ((DMA_ENABLE | DMA_START_HBLANK | DMA_REPEAT | DMA_DEST_RELOAD) << 16) | 1,
    1
};

// code
static void FillPalBufferWhite(void)
{
    CpuFastFill16(RGB_WHITE, gPlttBufferFaded, PLTT_SIZE);
}

static void FillPalBufferBlack(void)
{
    CpuFastFill16(RGB_BLACK, gPlttBufferFaded, PLTT_SIZE);
}

void WarpFadeInScreen(void)
{
    enum MapType previousMapType = GetLastUsedWarpMapType();
    switch (GetMapPairFadeFromType(previousMapType, GetCurrentMapType()))
    {
    case 0:
        FillPalBufferBlack();
        FadeScreen(FADE_FROM_BLACK, 0);
        break;
    case 1:
        FillPalBufferWhite();
        FadeScreen(FADE_FROM_WHITE, 0);
    }
}

void FadeInFromWhite(void)
{
    FillPalBufferWhite();
    FadeScreen(FADE_FROM_WHITE, 8);
}

void FadeInFromBlack(void)
{
    FillPalBufferBlack();
    FadeScreen(FADE_FROM_BLACK, 0);
}

void WarpFadeOutScreen(void)
{
    enum MapType currentMapType = GetCurrentMapType();
    switch (GetMapPairFadeToType(currentMapType, GetDestinationWarpMapHeader()->mapType))
    {
    case 0:
        FadeScreen(FADE_TO_BLACK, 0);
        break;
    case 1:
        FadeScreen(FADE_TO_WHITE, 0);
    }
}

void SetPlayerVisibility(bool8 visible)
{
    SetPlayerInvisibility(!visible);
}

static void Task_WaitForFadeAndEnableScriptCtx(u8 taskID)
{
    if (WaitForWeatherFadeIn() == TRUE)
    {
        DestroyTask(taskID);
        ScriptContext_Enable();
    }
}

void FieldCB_ContinueScriptHandleMusic(void)
{
    LockPlayerFieldControls();
    Overworld_PlaySpecialMapMusic();
    FadeInFromBlack();
    CreateTask(Task_WaitForFadeAndEnableScriptCtx, 10);
}

void FieldCB_ContinueScript(void)
{
    LockPlayerFieldControls();
    FadeInFromBlack();
    CreateTask(Task_WaitForFadeAndEnableScriptCtx, 10);
}

static void Task_ReturnToFieldCableLink(u8 taskId)
{
    struct Task *task = &gTasks[taskId];

    switch (task->tState)
    {
    case 0:
        task->data[1] = CreateTask_ReestablishCableClubLink();
        task->tState++;
        break;
    case 1:
        if (gTasks[task->data[1]].isActive != TRUE)
        {
            WarpFadeInScreen();
            task->tState++;
        }
        break;
    case 2:
        if (WaitForWeatherFadeIn() == TRUE)
        {
            UnlockPlayerFieldControls();
            DestroyTask(taskId);
        }
        break;
    }
}

void FieldCB_ReturnToFieldCableLink(void)
{
    LockPlayerFieldControls();
    Overworld_PlaySpecialMapMusic();
    FillPalBufferBlack();
    CreateTask(Task_ReturnToFieldCableLink, 10);
}

void Task_ReturnToFieldRecordMixing(u8 taskId)
{
    struct Task *task = &gTasks[taskId];

    switch (task->tState)
    {
    case 0:
        SetLinkStandbyCallback();
        task->tState++;
        break;
    case 1:
        if (IsLinkTaskFinished())
            task->tState++;
        break;
    case 2:
        StartSendingKeysToLink();
        ResetAllMultiplayerState();
        UnlockPlayerFieldControls();
        DestroyTask(taskId);
        break;
    }
}

static void SetUpWarpExitTask(void)
{
    s16 x, y;
    u8 behavior;
    TaskFunc func;

    PlayerGetDestCoords(&x, &y);
    behavior = MapGridGetMetatileBehaviorAt(x, y);
    if (MetatileBehavior_IsDoor(behavior) == TRUE)
        func = Task_ExitDoor;
    else if (MetatileBehavior_IsDirectionalStairWarp(behavior) == TRUE && !gExitStairsMovementDisabled)
        func = Task_ExitStairs;
    else if (MetatileBehavior_IsNonAnimDoor(behavior) == TRUE)
        func = Task_ExitNonAnimDoor;
    else
        func = Task_ExitNonDoor;

    gExitStairsMovementDisabled = FALSE;
    CreateTask(func, 10);
}

void FieldCB_DefaultWarpExit(void)
{
    Overworld_PlaySpecialMapMusic();
    WarpFadeInScreen();
    SetUpWarpExitTask();
    FollowerNPC_WarpSetEnd();
    LockPlayerFieldControls();
}

void FieldCB_WarpExitFadeFromWhite(void)
{
    Overworld_PlaySpecialMapMusic();
    FadeInFromWhite();
    SetUpWarpExitTask();
    LockPlayerFieldControls();
}

void FieldCB_WarpExitFadeFromBlack(void)
{
    if (!OnTrainerHillEReaderChallengeFloor()) // always false
        Overworld_PlaySpecialMapMusic();
    FadeInFromBlack();
    SetUpWarpExitTask();
    LockPlayerFieldControls();
}

static void FieldCB_SpinEnterWarp(void)
{
    Overworld_PlaySpecialMapMusic();
    WarpFadeInScreen();
    PlaySE(SE_WARP_OUT);
    CreateTask(Task_SpinEnterWarp, 10);
    LockPlayerFieldControls();
}

static void FieldCB_MossdeepGymWarpExit(void)
{
    Overworld_PlaySpecialMapMusic();
    WarpFadeInScreen();
    PlaySE(SE_WARP_OUT);
    CreateTask(Task_ExitNonDoor, 10);
    LockPlayerFieldControls();
    SetObjectEventLoadFlag((~SKIP_OBJECT_EVENT_LOAD) & 0xF);
}

static void Task_ExitDoor(u8 taskId)
{
    struct Task *task = &gTasks[taskId];
    s16 *x = &task->data[2];
    s16 *y = &task->data[3];

    switch (task->tState)
    {
    case 0:
        HideNPCFollower();
        SetPlayerVisibility(FALSE);
        FreezeObjectEvents();
        PlayerGetDestCoords(x, y);
        FieldSetDoorOpened(*x, *y);
        task->tState = 1;
        break;
    case 1:
        if (WaitForWeatherFadeIn())
        {
            u8 objEventId;
            SetPlayerVisibility(TRUE);
            objEventId = GetObjectEventIdByLocalIdAndMap(LOCALID_PLAYER, 0, 0);
            ObjectEventSetHeldMovement(&gObjectEvents[objEventId], MOVEMENT_ACTION_WALK_NORMAL_DOWN);
            task->tState = 2;
        }
        break;
    case 2:
        if (IsPlayerStandingStill())
        {
            u8 objEventId;
            task->data[1] = FieldAnimateDoorClose(*x, *y);
            objEventId = GetObjectEventIdByLocalIdAndMap(LOCALID_PLAYER, 0, 0);
            ObjectEventClearHeldMovementIfFinished(&gObjectEvents[objEventId]);
            task->tState = 3;
        }
        break;
    case 3:
        if (task->data[1] < 0 || gTasks[task->data[1]].isActive != TRUE)
        {
            FollowerNPC_SetIndicatorToComeOutDoor();
            FollowerNPC_WarpSetEnd();
            UnfreezeObjectEvents();
            task->tState = 4;
        }
        break;
    case 4:
        // Don't unlock controls until the map preview has finished.
        if (!FadeInMapPreviewScreenIsRunning())
            UnlockPlayerFieldControls();

        DestroyTask(taskId);
        break;
    }
}

static void Task_ExitNonAnimDoor(u8 taskId)
{
    struct Task *task = &gTasks[taskId];
    s16 *x = &task->data[2];
    s16 *y = &task->data[3];

    switch (task->tState)
    {
    case 0:
        HideNPCFollower();
        SetPlayerVisibility(FALSE);
        FreezeObjectEvents();
        PlayerGetDestCoords(x, y);
        task->tState = 1;
        break;
    case 1:
        if (WaitForWeatherFadeIn())
        {
            u8 objEventId;
            SetPlayerVisibility(TRUE);
            objEventId = GetObjectEventIdByLocalIdAndMap(LOCALID_PLAYER, 0, 0);
            ObjectEventSetHeldMovement(&gObjectEvents[objEventId], GetWalkNormalMovementAction(GetPlayerFacingDirection()));
            task->tState = 2;
        }
        break;
    case 2:
        if (IsPlayerStandingStill())
        {
            s16 x, y;

            PlayerGetDestCoords(&x, &y);
            if (!MetatileBehavior_IsDeepSouthWarp(MapGridGetMetatileBehaviorAt(x, y + 1)))
                FollowerNPC_SetIndicatorToComeOutDoor();
            // TODO: Add specific follower door warp behavior for MB_DEEP_SOUTH_WARP.

            FollowerNPC_WarpSetEnd();
            UnfreezeObjectEvents();
            task->tState = 3;
        }
        break;
    case 3:
        // Don't unlock controls until the map preview has finished.
        if (!FadeInMapPreviewScreenIsRunning())
            UnlockPlayerFieldControls();

        DestroyTask(taskId);
        break;
    }
}

static void Task_ExitNonDoor(u8 taskId)
{
    switch (gTasks[taskId].tState)
    {
    case 0:
        FreezeObjectEvents();
        LockPlayerFieldControls();
        gTasks[taskId].tState++;
        break;
    case 1:
        if (WaitForWeatherFadeIn())
        {
            UnfreezeObjectEvents();
            // Don't unlock controls until the map preview has finished.
            if (!FadeInMapPreviewScreenIsRunning())
                UnlockPlayerFieldControls();

            DestroyTask(taskId);
        }
        break;
    }
}

static void Task_WaitForFadeShowStartMenu(u8 taskId)
{
    if (WaitForWeatherFadeIn() == TRUE)
    {
        DestroyTask(taskId);
        CreateTask(Task_ShowStartMenu, 80);
    }
}

void ReturnToFieldOpenStartMenu(void)
{
    FadeInFromBlack();
    CreateTask(Task_WaitForFadeShowStartMenu, 0x50);
    LockPlayerFieldControls();
}

bool8 FieldCB_ReturnToFieldOpenStartMenu(void)
{
    ShowReturnToFieldStartMenu();
    return FALSE;
}

static void Task_ReturnToFieldNoScript(u8 taskId)
{
    if (WaitForWeatherFadeIn() == 1)
    {
        UnlockPlayerFieldControls();
        DestroyTask(taskId);
        ScriptUnfreezeObjectEvents();
    }
}

void FieldCB_ReturnToFieldNoScript(void)
{
    LockPlayerFieldControls();
    FadeInFromBlack();
    CreateTask(Task_ReturnToFieldNoScript, 10);
}

void FieldCB_ReturnToFieldNoScriptCheckMusic(void)
{
    LockPlayerFieldControls();
    Overworld_PlaySpecialMapMusic();
    FadeInFromBlack();
    CreateTask(Task_ReturnToFieldNoScript, 10);
}

static bool32 PaletteFadeActive(void)
{
    return gPaletteFade.active;
}

static bool32 WaitForWeatherFadeIn(void)
{
    if (IsWeatherNotFadingIn() == TRUE)
        return TRUE;
    else
        return FALSE;
}

void DoWarp(void)
{
    LockPlayerFieldControls();
    TryFadeOutOldMapMusic();
    WarpFadeOutScreen();
    PlayRainStoppingSoundEffect();
    PlaySE(SE_EXIT);
    gFieldCallback = FieldCB_DefaultWarpExit;
    CreateTask(Task_WarpAndLoadMap, 10);
}

void DoDiveWarp(void)
{
    LockPlayerFieldControls();
    TryFadeOutOldMapMusic();
    WarpFadeOutScreen();
    PlayRainStoppingSoundEffect();
    SetFollowerNPCData(FNPC_DATA_COME_OUT_DOOR, FNPC_DOOR_NONE);
    gFieldCallback = FieldCB_DefaultWarpExit;
    CreateTask(Task_WarpAndLoadMap, 10);
}

void DoWhiteFadeWarp(void)
{
    LockPlayerFieldControls();
    TryFadeOutOldMapMusic();
    FadeScreen(FADE_TO_WHITE, 8);
    PlayRainStoppingSoundEffect();
    gFieldCallback = FieldCB_WarpExitFadeFromWhite;
    CreateTask(Task_WarpAndLoadMap, 10);
}

void DoDoorWarp(void)
{
    LockPlayerFieldControls();
    gFieldCallback = FieldCB_DefaultWarpExit;
    CreateTask(Task_DoDoorWarp, 10);
}

void DoFallWarp(void)
{
    DoDiveWarp();
    gFieldCallback = FieldCB_FallWarpExit;
}

void DoEscalatorWarp(u8 metatileBehavior)
{
    LockPlayerFieldControls();
    StartEscalatorWarp(metatileBehavior, 10);
}

void DoLavaridgeGymB1FWarp(void)
{
    LockPlayerFieldControls();
    StartLavaridgeGymB1FWarp(10);
}

void DoLavaridgeGym1FWarp(void)
{
    LockPlayerFieldControls();
    StartLavaridgeGym1FWarp(10);
}

// DoSpinEnterWarp but with a fade out
// Screen fades out to exit current map, player spins down from top to enter new map
// Used by teleporting tiles, e.g. in Aqua Hideout (For the move Teleport see FldEff_TeleportWarpOut)
void DoTeleportTileWarp(void)
{
    LockPlayerFieldControls();
    TryFadeOutOldMapMusic();
    WarpFadeOutScreen();
    PlaySE(SE_WARP_IN);
    CreateTask(Task_WarpAndLoadMap, 10);
    gFieldCallback = FieldCB_SpinEnterWarp;
}

void DoMossdeepGymWarp(void)
{
    SetObjectEventLoadFlag(SKIP_OBJECT_EVENT_LOAD);
    LockPlayerFieldControls();
    SaveObjectEvents();
    TryFadeOutOldMapMusic();
    WarpFadeOutScreen();
    SetFollowerNPCData(FNPC_DATA_WARP_END, FNPC_WARP_REAPPEAR);
    PlaySE(SE_WARP_IN);
    CreateTask(Task_WarpAndLoadMap, 10);
    gFieldCallback = FieldCB_MossdeepGymWarpExit;
}

void DoPortholeWarp(void)
{
    LockPlayerFieldControls();
    WarpFadeOutScreen();
    CreateTask(Task_WarpAndLoadMap, 10);
    gFieldCallback = FieldCB_ShowPortholeView;
}

static void Task_DoCableClubWarp(u8 taskId)
{
    struct Task *task = &gTasks[taskId];

    switch (task->tState)
    {
    case 0:
        LockPlayerFieldControls();
        task->tState++;
        break;
    case 1:
        if (!PaletteFadeActive() && BGMusicStopped())
            task->tState++;
        break;
    case 2:
        WarpIntoMap();
        SetMainCallback2(CB2_ReturnToFieldCableClub);
        DestroyTask(taskId);
        break;
    }
}

void DoCableClubWarp(void)
{
    LockPlayerFieldControls();
    TryFadeOutOldMapMusic();
    WarpFadeOutScreen();
    PlaySE(SE_EXIT);
    CreateTask(Task_DoCableClubWarp, 10);
}

static void Task_ReturnToWorldFromLinkRoom(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    switch (tState)
    {
    case 0:
        ClearLinkCallback_2();
        FadeScreen(FADE_TO_BLACK, 0);
        TryFadeOutOldMapMusic();
        PlaySE(SE_EXIT);
        tState++;
        break;
    case 1:
        if (!PaletteFadeActive() && BGMusicStopped())
        {
            SetCloseLinkCallback();
            tState++;
        }
        break;
    case 2:
        if (!gReceivedRemoteLinkPlayers)
        {
            WarpIntoMap();
            SetMainCallback2(CB2_LoadMap);
            DestroyTask(taskId);
        }
        break;
    }
}

void ReturnFromLinkRoom(void)
{
    CreateTask(Task_ReturnToWorldFromLinkRoom, 10);
}

void Task_WarpAndLoadMap(u8 taskId)
{
    struct Task *task = &gTasks[taskId];

    switch (task->tState)
    {
    case 0:
        FreezeObjectEvents();
        LockPlayerFieldControls();
        EndORASDowsing();
        task->tState++;
        break;
    case 1:
        if (!PaletteFadeActive())
        {
            if (task->data[1] == 0)
            {
                ClearMirageTowerPulseBlendEffect();
                task->data[1] = 1;
            }
            if (BGMusicStopped())
                task->tState++;
        }
        break;
    case 2:
        WarpIntoMap();
        SetMainCallback2(CB2_LoadMap);
        DestroyTask(taskId);
        break;
    }
}

#define tDoorTask   data[1]

enum
{
    DOORWARP_OPEN_DOOR,
    DOORWARP_START_WALK_UP,
    DOORWARP_HIDE_PLAYER,
    DOORWARP_WAIT_DOOR_ANIM_TASK,
    DOORWARP_DO_WARP
};

void Task_DoDoorWarp(u8 taskId)
{
    struct Task *task = &gTasks[taskId];
    s16 *x = &task->data[2];
    s16 *y = &task->data[3];
    u8 playerObjId = gPlayerAvatar.objectEventId;
    u8 followerObjId = GetFollowerNPCObjectId();
    struct ObjectEvent *followerObject = GetFollowerObject();

    switch (task->tState)
    {
    case DOORWARP_OPEN_DOOR:
        // Stop running.
        if (TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_DASH))
            SetPlayerAvatarTransitionFlags(PLAYER_AVATAR_FLAG_ON_FOOT);

        // Just in case came out and went right back in, reset follower NPC door state.
        SetFollowerNPCData(FNPC_DATA_COME_OUT_DOOR, FNPC_DOOR_NONE);
        FreezeObjectEvents();
        PlayerGetDestCoords(x, y);
        PlaySE(GetDoorSoundEffect(*x, *y - 1));
        if (followerObject)
        {
            // Put follower into pokeball
            ClearObjectEventMovement(followerObject, &gSprites[followerObject->spriteId]);
            ObjectEventSetHeldMovement(followerObject, MOVEMENT_ACTION_ENTER_POKEBALL);
        }
        task->tDoorTask = FieldAnimateDoorOpen(*x, *y - 1);
        EndORASDowsing();
        task->tState = DOORWARP_START_WALK_UP;
        break;
    case DOORWARP_START_WALK_UP:
        if (task->tDoorTask < 0 || gTasks[task->tDoorTask].isActive != TRUE)
        {
            ObjectEventClearHeldMovementIfActive(&gObjectEvents[playerObjId]);
            ObjectEventSetHeldMovement(&gObjectEvents[playerObjId], MOVEMENT_ACTION_WALK_NORMAL_UP);

            if (PlayerHasFollowerNPC() && !gObjectEvents[followerObjId].invisible)
            {
                u8 newState = DetermineFollowerNPCState(&gObjectEvents[followerObjId], MOVEMENT_ACTION_WALK_NORMAL_UP,
                                                        DetermineFollowerNPCDirection(&gObjectEvents[playerObjId], &gObjectEvents[followerObjId]));
                ObjectEventClearHeldMovementIfActive(&gObjectEvents[followerObjId]);
                ObjectEventSetHeldMovement(&gObjectEvents[followerObjId], newState);
            }

            task->tState = DOORWARP_HIDE_PLAYER;
        }
        break;
    case DOORWARP_HIDE_PLAYER:
        if (IsPlayerStandingStill())
        {
            // Don't close door on NPC follower.
            if (!PlayerHasFollowerNPC() || gObjectEvents[followerObjId].invisible)
                task->tDoorTask = FieldAnimateDoorClose(*x, *y - 1);

            ObjectEventClearHeldMovementIfFinished(&gObjectEvents[playerObjId]);
            SetPlayerVisibility(FALSE);
            task->tState = DOORWARP_WAIT_DOOR_ANIM_TASK;
        }
        break;
    case DOORWARP_WAIT_DOOR_ANIM_TASK:
        if (task->tDoorTask < 0 || gTasks[task->tDoorTask].isActive != TRUE)
            task->tState = DOORWARP_DO_WARP;
        break;
    case DOORWARP_DO_WARP:
        if (PlayerHasFollowerNPC())
        {
            ObjectEventClearHeldMovementIfActive(&gObjectEvents[followerObjId]);
            ObjectEventSetHeldMovement(&gObjectEvents[followerObjId], MOVEMENT_ACTION_WALK_NORMAL_UP);
        }

        TryFadeOutOldMapMusic();
        WarpFadeOutScreen();
        PlayRainStoppingSoundEffect();
        task->tState = 0;
        task->func = Task_WarpAndLoadMap;
        break;
    }
}

static void Task_DoContestHallWarp(u8 taskId)
{
    struct Task *task = &gTasks[taskId];

    switch (task->tState)
    {
    case 0:
        FreezeObjectEvents();
        LockPlayerFieldControls();
        task->tState++;
        break;
    case 1:
        if (!PaletteFadeActive() && BGMusicStopped())
        {
            task->tState++;
        }
        break;
    case 2:
        WarpIntoMap();
        SetMainCallback2(CB2_ReturnToFieldContestHall);
        DestroyTask(taskId);
        break;
    }
}

void DoContestHallWarp(void)
{
    LockPlayerFieldControls();
    TryFadeOutOldMapMusic();
    WarpFadeOutScreen();
    PlayRainStoppingSoundEffect();
    PlaySE(SE_EXIT);
    gFieldCallback = FieldCB_WarpExitFadeFromBlack;
    CreateTask(Task_DoContestHallWarp, 10);
}

static void SetFlashScanlineEffectWindowBoundary(u16 *dest, u32 y, s32 left, s32 right)
{
    if (y <= 160)
    {
        if (left < 0)
            left = 0;
        if (left > 255)
            left = 255;
        if (right < 0)
            right = 0;
        if (right > 255)
            right = 255;
        dest[y] = (left << 8) | right;
    }
}

static void SetFlashScanlineEffectWindowBoundaries(u16 *dest, s32 centerX, s32 centerY, s32 radius)
{
    s32 r = radius;
    s32 v2 = radius;
    s32 v3 = 0;
    while (r >= v3)
    {
        SetFlashScanlineEffectWindowBoundary(dest, centerY - v3, centerX - r, centerX + r);
        SetFlashScanlineEffectWindowBoundary(dest, centerY + v3, centerX - r, centerX + r);
        SetFlashScanlineEffectWindowBoundary(dest, centerY - r, centerX - v3, centerX + v3);
        SetFlashScanlineEffectWindowBoundary(dest, centerY + r, centerX - v3, centerX + v3);
        v2 -= (v3 * 2) - 1;
        v3++;
        if (v2 < 0)
        {
            v2 += 2 * (r - 1);
            r--;
        }
    }
}

static void SetOrbFlashScanlineEffectWindowBoundary(u16 *dest, u32 y, s32 left, s32 right)
{
    if (y <= 160)
    {
        if (left < 0)
            left = 0;
        if (left > 240)
            left = 240;
        if (right < 0)
            right = 0;
        if (right > 240)
            right = 240;
        dest[y] = (left << 8) | right;
    }
}

static void SetOrbFlashScanlineEffectWindowBoundaries(u16 *dest, s32 centerX, s32 centerY, s32 radius)
{
    s32 r = radius;
    s32 v2 = radius;
    s32 v3 = 0;
    while (r >= v3)
    {
        SetOrbFlashScanlineEffectWindowBoundary(dest, centerY - v3, centerX - r, centerX + r);
        SetOrbFlashScanlineEffectWindowBoundary(dest, centerY + v3, centerX - r, centerX + r);
        SetOrbFlashScanlineEffectWindowBoundary(dest, centerY - r, centerX - v3, centerX + v3);
        SetOrbFlashScanlineEffectWindowBoundary(dest, centerY + r, centerX - v3, centerX + v3);
        v2 -= (v3 * 2) - 1;
        v3++;
        if (v2 < 0)
        {
            v2 += 2 * (r - 1);
            r--;
        }
    }
}

#define tFlashCenterX        data[1]
#define tFlashCenterY        data[2]
#define tCurFlashRadius      data[3]
#define tDestFlashRadius     data[4]
#define tFlashRadiusDelta    data[5]
#define tClearScanlineEffect data[6]

static void UpdateFlashLevelEffect(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    switch (tState)
    {
    case 0:
        SetFlashScanlineEffectWindowBoundaries(gScanlineEffectRegBuffers[gScanlineEffect.srcBuffer], tFlashCenterX, tFlashCenterY, tCurFlashRadius);
        tState = 1;
        break;
    case 1:
        SetFlashScanlineEffectWindowBoundaries(gScanlineEffectRegBuffers[gScanlineEffect.srcBuffer], tFlashCenterX, tFlashCenterY, tCurFlashRadius);
        tState = 0;
        tCurFlashRadius += tFlashRadiusDelta;
        if (tCurFlashRadius > tDestFlashRadius)
        {
            if (tClearScanlineEffect == 1)
            {
                ScanlineEffect_Stop();
                tState = 2;
            }
            else
            {
                DestroyTask(taskId);
            }
        }
        break;
    case 2:
        ScanlineEffect_Clear();
        DestroyTask(taskId);
        break;
    }
}

static void UpdateOrbFlashEffect(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    switch (tState)
    {
    case 0:
        SetOrbFlashScanlineEffectWindowBoundaries(gScanlineEffectRegBuffers[gScanlineEffect.srcBuffer], tFlashCenterX, tFlashCenterY, tCurFlashRadius);
        tState = 1;
        break;
    case 1:
        SetOrbFlashScanlineEffectWindowBoundaries(gScanlineEffectRegBuffers[gScanlineEffect.srcBuffer], tFlashCenterX, tFlashCenterY, tCurFlashRadius);
        tState = 0;
        tCurFlashRadius += tFlashRadiusDelta;
        if (tCurFlashRadius > tDestFlashRadius)
        {
            if (tClearScanlineEffect == 1)
            {
                ScanlineEffect_Stop();
                tState = 2;
            }
            else
            {
                DestroyTask(taskId);
            }
        }
        break;
    case 2:
        ScanlineEffect_Clear();
        DestroyTask(taskId);
        break;
    }
}

static void Task_WaitForFlashUpdate(u8 taskId)
{
    if (!FuncIsActiveTask(UpdateFlashLevelEffect))
    {
        ScriptContext_Enable();
        DestroyTask(taskId);
    }
}

static void StartWaitForFlashUpdate(void)
{
    if (!FuncIsActiveTask(Task_WaitForFlashUpdate))
        CreateTask(Task_WaitForFlashUpdate, 80);
}

static u8 StartUpdateFlashLevelEffect(s32 centerX, s32 centerY, s32 initialFlashRadius, s32 destFlashRadius, s32 clearScanlineEffect, u8 delta)
{
    u8 taskId = CreateTask(UpdateFlashLevelEffect, 80);
    s16 *data = gTasks[taskId].data;

    tCurFlashRadius = initialFlashRadius;
    tDestFlashRadius = destFlashRadius;
    tFlashCenterX = centerX;
    tFlashCenterY = centerY;
    tClearScanlineEffect = clearScanlineEffect;

    if (initialFlashRadius < destFlashRadius)
        tFlashRadiusDelta = delta;
    else
        tFlashRadiusDelta = -delta;

    return taskId;
}

static u8 StartUpdateOrbFlashEffect(s32 centerX, s32 centerY, s32 initialFlashRadius, s32 destFlashRadius, s32 clearScanlineEffect, u8 delta)
{
    u8 taskId = CreateTask(UpdateOrbFlashEffect, 80);
    s16 *data = gTasks[taskId].data;

    tCurFlashRadius = initialFlashRadius;
    tDestFlashRadius = destFlashRadius;
    tFlashCenterX = centerX;
    tFlashCenterY = centerY;
    tClearScanlineEffect = clearScanlineEffect;

    if (initialFlashRadius < destFlashRadius)
        tFlashRadiusDelta = delta;
    else
        tFlashRadiusDelta = -delta;

    return taskId;
}

#undef tCurFlashRadius
#undef tDestFlashRadius
#undef tFlashRadiusDelta
#undef tClearScanlineEffect

// A higher flash level is a smaller flash radius (more darkness). 0 is full brightness
void AnimateFlash(u8 newFlashLevel)
{
    u8 curFlashLevel = GetFlashLevel();
    bool8 fullBrightness = FALSE;
    if (newFlashLevel == 0)
        fullBrightness = TRUE;
    StartUpdateFlashLevelEffect(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2, sFlashLevelToRadius[curFlashLevel], sFlashLevelToRadius[newFlashLevel], fullBrightness, 1);
    StartWaitForFlashUpdate();
    LockPlayerFieldControls();
}

void WriteFlashScanlineEffectBuffer(u8 flashLevel)
{
    if (flashLevel)
    {
        SetFlashScanlineEffectWindowBoundaries(&gScanlineEffectRegBuffers[0][0], DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2, sFlashLevelToRadius[flashLevel]);
        CpuFastSet(&gScanlineEffectRegBuffers[0], &gScanlineEffectRegBuffers[1], 480);
    }
}

void WriteBattlePyramidViewScanlineEffectBuffer(void)
{
#if FREE_BATTLE_FRONTIER == FALSE
    SetFlashScanlineEffectWindowBoundaries(&gScanlineEffectRegBuffers[0][0], DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2, gSaveBlock2Ptr->frontier.pyramidLightRadius);
    CpuFastSet(&gScanlineEffectRegBuffers[0], &gScanlineEffectRegBuffers[1], 480);
#endif //FREE_BATTLE_FRONTIER
}

static void Task_SpinEnterWarp(u8 taskId)
{
    switch (gTasks[taskId].tState)
    {
    case 0:
        FreezeObjectEvents();
        LockPlayerFieldControls();
        DoPlayerSpinEntrance();
        gTasks[taskId].tState++;
        break;
    case 1:
        if (WaitForWeatherFadeIn() && IsPlayerSpinEntranceActive() != TRUE)
        {
            FollowerNPC_WarpSetEnd();
            UnfreezeObjectEvents();
            UnlockPlayerFieldControls();
            DestroyTask(taskId);
        }
        break;
    }
}

static void Task_SpinExitWarp(u8 taskId)
{
    struct Task *task = &gTasks[taskId];

    switch (task->tState)
    {
    case 0:
        FreezeObjectEvents();
        LockPlayerFieldControls();
        PlaySE(SE_WARP_IN);
        DoPlayerSpinExit();
        task->tState++;
        break;
    case 1:
        if (!IsPlayerSpinExitActive())
        {
            WarpFadeOutScreen();
            task->tState++;
        }
        break;
    case 2:
        if (!PaletteFadeActive() && BGMusicStopped())
            task->tState++;
        break;
    case 3:
        WarpIntoMap();
        SetMainCallback2(CB2_LoadMap);
        DestroyTask(taskId);
        break;
    }
}

// Only called by an unused function
// DoTeleportTileWarp is used instead
void DoSpinEnterWarp(void)
{
    LockPlayerFieldControls();
    CreateTask(Task_WarpAndLoadMap, 10);
    gFieldCallback = FieldCB_SpinEnterWarp;
}

// Opposite of DoSpinEnterWarp / DoTeleportTileWarp
// Player exits current map by spinning up offscreen, enters new map with a fade in
void DoSpinExitWarp(void)
{
    LockPlayerFieldControls();
    gFieldCallback = FieldCB_DefaultWarpExit;
    CreateTask(Task_SpinExitWarp, 10);
}

static void LoadOrbEffectPalette(bool8 blueOrb)
{
    int i;
    u16 color[1];

    if (!blueOrb)
        color[0] = RGB_RED;
    else
        color[0] = RGB_BLUE;

    for (i = 0; i < 16; i++)
        LoadPalette(color, BG_PLTT_ID(15) + i, PLTT_SIZEOF(1));
}

static bool8 UpdateOrbEffectBlend(u16 shakeDir)
{
    u8 lo = REG_BLDALPHA & 0xFF;
    u8 hi = REG_BLDALPHA >> 8;

    if (shakeDir != 0)
    {
        if (lo)
            lo--;
    }
    else
    {
        if (hi < 16)
            hi++;
    }

    SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(lo, hi));

    if (lo == 0 && hi == 16)
        return TRUE;
    else
        return FALSE;
}

#define tBlueOrb     data[1]
#define tCenterX     data[2]
#define tCenterY     data[3]
#define tShakeDelay  data[4]
#define tShakeDir    data[5]
#define tDispCnt     data[6]
#define tBldCnt      data[7]
#define tBldAlpha    data[8]
#define tWinIn       data[9]
#define tWinOut      data[10]

static void Task_OrbEffect(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    switch (tState)
    {
    case 0:
        tDispCnt = REG_DISPCNT;
        tBldCnt = REG_BLDCNT;
        tBldAlpha = REG_BLDALPHA;
        tWinIn = REG_WININ;
        tWinOut = REG_WINOUT;
        ClearGpuRegBits(REG_OFFSET_DISPCNT, DISPCNT_WIN1_ON);
        SetGpuRegBits(REG_OFFSET_BLDCNT, gOrbEffectBackgroundLayerFlags[0]);
        SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(12, 7));
        UpdateShadowColor(RGB(9, 8, 8));
        SetGpuReg(REG_OFFSET_WININ, WININ_WIN0_BG_ALL | WININ_WIN0_OBJ | WININ_WIN0_CLR);
        SetGpuReg(REG_OFFSET_WINOUT, WINOUT_WIN01_BG1 | WINOUT_WIN01_BG2 | WINOUT_WIN01_BG3 | WINOUT_WIN01_OBJ);
        SetBgTilemapPalette(0, 0, 0, DISPLAY_TILE_WIDTH, DISPLAY_TILE_HEIGHT, 0xF);
        ScheduleBgCopyTilemapToVram(0);
        SetOrbFlashScanlineEffectWindowBoundaries(&gScanlineEffectRegBuffers[0][0], tCenterX, tCenterY, 1);
        CpuFastSet(&gScanlineEffectRegBuffers[0], &gScanlineEffectRegBuffers[1], 480);
        ScanlineEffect_SetParams(sFlashEffectParams);
        tState = 1;
        break;
    case 1:
        BgDmaFill(0, PIXEL_FILL(1), 0, 1);
        LoadOrbEffectPalette(tBlueOrb);
        StartUpdateOrbFlashEffect(tCenterX, tCenterY, 1, 160, 1, 2);
        tState = 2;
        break;
    case 2:
        if (!FuncIsActiveTask(UpdateOrbFlashEffect))
        {
            ScriptContext_Enable();
            tState = 3;
        }
        break;
    case 3:
        InstallCameraPanAheadCallback();
        SetCameraPanningCallback(NULL);
        tShakeDir = 0;
        tShakeDelay = 4;
        tState = 4;
        break;
    case 4:
        // If the caller script is delayed after starting the orb effect, a `waitstate` might be reached *after*
        // we enable the ScriptContext in case 2; enabling it here as well avoids softlocks in this scenario
        ScriptContext_Enable();
        if (--tShakeDelay == 0)
        {
            s32 panning;
            tShakeDelay = 4;
            tShakeDir ^= 1;
            if (tShakeDir)
                panning = 4;
            else
                panning = -4;
            SetCameraPanning(0, panning);
        }
        break;
    case 6:
        InstallCameraPanAheadCallback();
        tShakeDelay = 8;
        tState = 7;
        break;
    case 7:
        if (--tShakeDelay == 0)
        {
            tShakeDelay = 8;
            tShakeDir ^= 1;
            if (UpdateOrbEffectBlend(tShakeDir) == TRUE)
            {
                tState = 5;
                BgDmaFill(0, PIXEL_FILL(0), 0, 1);
            }
        }
        break;
    case 5:
        SetGpuReg(REG_OFFSET_WIN0H, 255);
        SetGpuReg(REG_OFFSET_DISPCNT, tDispCnt);
        SetGpuReg(REG_OFFSET_BLDCNT, tBldCnt);
        SetGpuReg(REG_OFFSET_BLDALPHA, tBldAlpha);
        UpdateShadowColor(RGB_BLACK);
        SetGpuReg(REG_OFFSET_WININ, tWinIn);
        SetGpuReg(REG_OFFSET_WINOUT, tWinOut);
        ScriptContext_Enable();
        DestroyTask(taskId);
        break;
    }
}

void DoOrbEffect(void)
{
    u8 taskId = CreateTask(Task_OrbEffect, 80);
    s16 *data = gTasks[taskId].data;

    if (gSpecialVar_Result == 0)
    {
        tBlueOrb = FALSE;
        tCenterX = 104;
    }
    else if (gSpecialVar_Result == 1)
    {
        tBlueOrb = TRUE;
        tCenterX = 136;
    }
    else if (gSpecialVar_Result == 2)
    {
        tBlueOrb = FALSE;
        tCenterX = 120;
    }
    else
    {
        tBlueOrb = TRUE;
        tCenterX = 120;
    }

    tCenterY = 80;
}

void FadeOutOrbEffect(void)
{
    u8 taskId = FindTaskIdByFunc(Task_OrbEffect);
    gTasks[taskId].tState = 6;
}

#undef tBlueOrb
#undef tCenterX
#undef tCenterY
#undef tShakeDelay
#undef tShakeDir
#undef tDispCnt
#undef tBldCnt
#undef tBldAlpha
#undef tWinIn
#undef tWinOut

void Script_FadeOutMapMusic(void)
{
    Overworld_FadeOutMapMusic();
    CreateTask(Task_EnableScriptAfterMusicFade, 80);
}

static void Task_EnableScriptAfterMusicFade(u8 taskId)
{
    if (BGMusicStopped() == TRUE)
    {
        DestroyTask(taskId);
        ScriptContext_Enable();
    }
}

static const struct WindowTemplate sWindowTemplate_WhiteoutText =
{
    .bg = 0,
    .tilemapLeft = 0,
    .tilemapTop = 5,
    .width = 30,
    .height = 11,
    .paletteNum = 15,
    .baseBlock = 1,
};

static const u8 sWhiteoutTextColors[] = { TEXT_COLOR_TRANSPARENT, TEXT_COLOR_WHITE, TEXT_COLOR_DARK_GRAY };

#define tState         data[0]
#define tWindowId      data[1]
#define tPrintState    data[2]
#define tIsPlayerHouse data[3]

static bool32 PrintWhiteOutRecoveryMessage(u8 taskId, const u8 *text, u32 x, u32 y)
{
    u32 windowId = gTasks[taskId].tWindowId;

    switch (gTasks[taskId].tPrintState)
    {
    case 0:
        FillWindowPixelBuffer(windowId, PIXEL_FILL(0));
        StringExpandPlaceholders(gStringVar4, text);
        AddTextPrinterParameterized4(windowId, FONT_NORMAL, x, y, 1, 0, sWhiteoutTextColors, 1, gStringVar4);
        gTextFlags.canABSpeedUpPrint = FALSE;
        gTasks[taskId].tPrintState = 1;
        break;
    case 1:
        RunTextPrinters();
        if (!IsTextPrinterActiveOnWindow(windowId))
        {
            gTasks[taskId].tPrintState = 0;
            return TRUE;
        }
        break;
    }
    return FALSE;
}

enum {
    WHITEOUT_CUTSCENE_ENTER_MSG_SCREEN,
    WHITEOUT_CUTSCENE_PRINT_MSG,
    WHITEOUT_CUTSCENE_LEAVE_MSG_SCREEN,
    WHITEOUT_CUTSCENE_HEAL_SCRIPT,
};

static const u8 *GenerateRecoveryMessage(u8 taskId)
{
    bool32 forfeitTrainer = DidPlayerForfeitNormalTrainerBattle();
    bool32 destinationIsPlayersHouse = (gTasks[taskId].tIsPlayerHouse == TRUE);

    if (forfeitTrainer && destinationIsPlayersHouse)
        return sText_PlayerRegroupHome;
    else if (forfeitTrainer && !destinationIsPlayersHouse)
        return sText_PlayerRegroupCenter;
    else if (!forfeitTrainer && destinationIsPlayersHouse)
        return sText_PlayerScurriedBackHome;
    else
        return sText_PlayerScurriedToCenter;
}

static void Task_RushInjuredPokemonToCenter(u8 taskId)
{
    u32 windowId;

    switch (gTasks[taskId].tState)
    {
    case WHITEOUT_CUTSCENE_ENTER_MSG_SCREEN:
        windowId = AddWindow(&sWindowTemplate_WhiteoutText);
        gTasks[taskId].tWindowId = windowId;
        Menu_LoadStdPalAt(BG_PLTT_ID(15));
        FillWindowPixelBuffer(windowId, PIXEL_FILL(0));
        PutWindowTilemap(windowId);
        CopyWindowToVram(windowId, COPYWIN_FULL);

        gTasks[taskId].tIsPlayerHouse = IsLastHealLocationPlayerHouse();
        gTasks[taskId].tState = WHITEOUT_CUTSCENE_PRINT_MSG;
        break;
    case WHITEOUT_CUTSCENE_PRINT_MSG:
    {
        const u8 *recoveryMessage = GenerateRecoveryMessage(taskId);

        if (PrintWhiteOutRecoveryMessage(taskId, recoveryMessage, 2, 8))
        {
            ObjectEventTurn(&gObjectEvents[gPlayerAvatar.objectEventId], DIR_NORTH);
            gTasks[taskId].tState = WHITEOUT_CUTSCENE_LEAVE_MSG_SCREEN;
        }
        break;
    }
    case WHITEOUT_CUTSCENE_LEAVE_MSG_SCREEN:
        windowId = gTasks[taskId].tWindowId;
        ClearWindowTilemap(windowId);
        CopyWindowToVram(windowId, COPYWIN_MAP);
        RemoveWindow(windowId);
        FadeInFromBlack();
        gTasks[taskId].tState = WHITEOUT_CUTSCENE_HEAL_SCRIPT;
        break;
    case WHITEOUT_CUTSCENE_HEAL_SCRIPT:
        if (WaitForWeatherFadeIn() == TRUE)
        {
            DestroyTask(taskId);
            if (gTasks[taskId].tIsPlayerHouse)
            {
                if (IS_FRLG)
                    StringCopy(gStringVar1, COMPOUND_STRING("PROF. OAK"));
                else
                    StringCopy(gStringVar1, COMPOUND_STRING("PROF. BIRCH"));
                ScriptContext_SetupScript(EventScript_AfterWhiteOutMomHeal);
            }
            else if (IS_FRLG)
            {
                ScriptContext_SetupScript(EventScript_AfterWhiteOutHeal_Frlg);
            }
            else
            {
                ScriptContext_SetupScript(EventScript_AfterWhiteOutHeal);
            }
        }
        break;
    }
}

void FieldCB_RushInjuredPokemonToCenter(void)
{
    u8 taskId;

    LockPlayerFieldControls();
    FillPalBufferBlack();
    taskId = CreateTask(Task_RushInjuredPokemonToCenter, 10);
    gTasks[taskId].tState = WHITEOUT_CUTSCENE_ENTER_MSG_SCREEN;
}

enum {
    NUZLOCKE_FAILED_ENTER_MSG_SCREEN,
    NUZLOCKE_FAILED_PRINT_FAILURE_MSG,
    NUZLOCKE_FAILED_PRINT_QUESTION,
    NUZLOCKE_FAILED_OPEN_YESNO,
    NUZLOCKE_FAILED_PROCESS_YESNO,
    NUZLOCKE_FAILED_PRINT_KEEP_STORAGE_INFO,
    NUZLOCKE_FAILED_PRINT_KEEP_STORAGE_QUESTION,
    NUZLOCKE_FAILED_OPEN_KEEP_STORAGE_YESNO,
    NUZLOCKE_FAILED_PROCESS_KEEP_STORAGE_YESNO,
};

// baseBlock 331 starts right after the 30x11 = 330 tile range
// sWindowTemplate_WhiteoutText reserves starting at baseBlock 1 -- reusing
// sYesNo_WindowTemplates (menu.c) here would collide with that range (its
// baseBlock 0x125 falls inside it) and corrupt whichever window draws second.
// width 6 (rather than gText_YesNo's usual 5) leaves room to center "YES"/"NO"
// inside the box instead of hugging its left edge.
// tilemapLeft centers the box in the 30-tile-wide screen: (30 - 6) / 2 = 12.
static const struct WindowTemplate sNuzlockeFailedYesNoWindowTemplate =
{
    .bg = 0,
    .tilemapLeft = 12,
    .tilemapTop = 9,
    .width = 6,
    .height = 4,
    .paletteNum = 15,
    .baseBlock = 331,
};

#define tYesNoWindowId data[4]

// Same job as PrintWhiteOutRecoveryMessage above, except the text is
// horizontally centered in the window instead of left-aligned at a fixed x --
// kept separate so the original whiteout cutscene's layout is untouched.
static bool32 PrintNuzlockeFailedMessage(u8 taskId, const u8 *text, u32 y)
{
    u32 windowId = gTasks[taskId].tWindowId;

    switch (gTasks[taskId].tPrintState)
    {
    case 0:
    {
        s32 x;

        FillWindowPixelBuffer(windowId, PIXEL_FILL(0));
        StringExpandPlaceholders(gStringVar4, text);
        x = GetStringCenterAlignXOffset(FONT_NORMAL, gStringVar4, sWindowTemplate_WhiteoutText.width * 8);
        AddTextPrinterParameterized4(windowId, FONT_NORMAL, x, y, 1, 0, sWhiteoutTextColors, 1, gStringVar4);
        gTextFlags.canABSpeedUpPrint = FALSE;
        gTasks[taskId].tPrintState = 1;
        break;
    }
    case 1:
        RunTextPrinters();
        if (!IsTextPrinterActiveOnWindow(windowId))
        {
            gTasks[taskId].tPrintState = 0;
            return TRUE;
        }
        break;
    }
    return FALSE;
}

// InitMenuInUpperLeftCornerNormal / Menu_ProcessInputNoWrap draw the cursor
// arrow (and erase its previous position) using the standard menu font's
// default light background color, which is meant to blend into the tan
// message-box frame those functions normally run inside. On this screen's
// solid black backdrop that same fill shows up as a stray light rectangle
// around the arrow. Repaint it here with this screen's own white-on-black
// colors so nothing but the arrow itself is visible.
static void DrawNuzlockeFailedYesNoCursor(u8 windowId)
{
    // GetMenuCursorDimensionByFont's height (15px) is only what menu.c uses
    // to space cursor rows -- the glyph itself actually renders into a taller
    // cell, so clearing just two of those 15px bands left a 1px sliver of the
    // engine's redraw uncleared at the bottom of the window. Clear the full
    // height of the arrow's column instead so no such sliver can remain.
    u8 width = GetMenuCursorDimensionByFont(FONT_NORMAL, 0);
    u8 cursorPos = Menu_GetCursorPos();

    FillWindowPixelRect(windowId, PIXEL_FILL(0), 0, 0, width, sNuzlockeFailedYesNoWindowTemplate.height * 8);
    AddTextPrinterParameterized4(windowId, FONT_NORMAL, 0, 16 * cursorPos + 1, 0, 0, sWhiteoutTextColors, TEXT_SKIP_DRAW, gText_SelectorArrow3);
    CopyWindowToVram(windowId, COPYWIN_GFX);
}

// Opens this screen's black-backdrop Yes/No box (rather than CreateYesNoMenu,
// which hardcodes the tan message-box frame and default black-on-white
// colors -- that would look like a mismatched popup instead of part of this
// same full-screen black text). Shared by both questions this screen can
// ask -- restart the run, and (if there's anything worth it) keep PC
// storage -- so initialCursorPos picks the non-destructive default for
// whichever question is being asked.
static void OpenNuzlockeFailedYesNo(u8 taskId, u8 initialCursorPos)
{
    u8 yesNoWindowId = AddWindow(&sNuzlockeFailedYesNoWindowTemplate);
    // InitMenuInUpperLeftCornerNormal below draws the cursor arrow at the
    // window's left edge (its own x=0..7), same as CreateYesNoMenu -- so
    // centering starts after that 8px reserved for the arrow, not from
    // the window's true left edge.
    s32 x = 8 + GetStringCenterAlignXOffset(FONT_NORMAL, gText_YesNo, (sNuzlockeFailedYesNoWindowTemplate.width * 8) - 8);

    gTasks[taskId].tYesNoWindowId = yesNoWindowId;
    FillWindowPixelBuffer(yesNoWindowId, PIXEL_FILL(0));
    AddTextPrinterParameterized4(yesNoWindowId, FONT_NORMAL, x, 1, 0, 0, sWhiteoutTextColors, TEXT_SKIP_DRAW, gText_YesNo);
    PutWindowTilemap(yesNoWindowId);
    CopyWindowToVram(yesNoWindowId, COPYWIN_FULL);
    // Since we skipped CreateYesNoMenu, use InitMenuInUpperLeftCornerNormal
    // directly to wire up cursor movement, and Menu_ProcessInputNoWrap
    // (not the ClearOnChoose variant, which assumes CreateYesNoMenu's
    // internal window and would try to erase the wrong graphics).
    InitMenuInUpperLeftCornerNormal(yesNoWindowId, 2, initialCursorPos);
    DrawNuzlockeFailedYesNoCursor(yesNoWindowId);
}

// Common final step once keep-storage carryover has been settled, whether
// by an explicit answer or because there was nothing worth asking about.
static void FinishNuzlockeRestart(u8 taskId)
{
    RemoveWindow(gTasks[taskId].tYesNoWindowId);
    RemoveWindow(gTasks[taskId].tWindowId);
    // Both CB2_NewGame and CB2_InitTitleScreen assume they're being
    // entered onto a blank slate (that's how every other caller of
    // either uses them -- e.g. New Game+ in start_menu.c calls this
    // same function before CB2_NewGame). We got here from a field
    // callback with a fully loaded overworld map still behind us
    // (windows, BG tilemap buffers, etc.), so skipping this leaves
    // that map's buffers dangling; the next screen allocates its own
    // over top of them, and whichever save gets continued afterwards
    // ends up reading/rendering through corrupted leftovers (this is
    // what caused the corrupted party/overworld graphics on "No").
    CleanupOverworldWindowsAndTilemaps();
    DestroyTask(taskId);
    // CB2_NewGame -> NewGameInitData unconditionally re-applies
    // gPendingNewGameSettings (nuzlocke mode, difficulty, randomizer
    // toggles, ...) on top of whatever's about to be wiped -- normally safe
    // because CB2_InitNewGameSettingsMenu is the only thing that ever sets
    // it, right before handing off to CB2_NewGame itself. We're skipping
    // that screen for a fast restart, so gPendingNewGameSettings could still
    // be sitting at its power-on default (or a stale earlier choice) if this
    // save was continued rather than freshly created this session. Snapshot
    // this run's actual settings into it first so restarting doesn't
    // silently reset them.
    CaptureCurrentSaveIntoPendingNewGameSettings();
    SetMainCallback2(CB2_NewGame);
}

// Shared run-failed screen for both modes that can empty the party: Nuzlocke
// (via CB2_WhiteOut, whenever the whiteout that got us here also emptied the
// party) and Recruits (via Recruits_StartRunFailedScreen, whenever a
// retirement does). Either way the emptied state is already persisted to
// flash by this point -- RemoveFaintedMonsFromParty for Nuzlocke,
// TrySavingData in Recruits_StartRunFailedScreen for Recruits -- and
// achievements/boosts live outside the save slots and are untouched either
// way, so this screen is purely about what the player does *next*, not about
// what happens to the save.
static void Task_NuzlockeRunFailed(u8 taskId)
{
    u32 windowId;

    switch (gTasks[taskId].tState)
    {
    case NUZLOCKE_FAILED_ENTER_MSG_SCREEN:
        windowId = AddWindow(&sWindowTemplate_WhiteoutText);
        gTasks[taskId].tWindowId = windowId;
        Menu_LoadStdPalAt(BG_PLTT_ID(15));
        FillWindowPixelBuffer(windowId, PIXEL_FILL(0));
        PutWindowTilemap(windowId);
        CopyWindowToVram(windowId, COPYWIN_FULL);
        gTasks[taskId].tState = NUZLOCKE_FAILED_PRINT_FAILURE_MSG;
        break;
    case NUZLOCKE_FAILED_PRINT_FAILURE_MSG:
        // Recruits shares this screen with Nuzlocke (see Run_IsFailed,
        // src/overworld.c) - pick the matching pair of strings by mode.
        if (PrintNuzlockeFailedMessage(taskId, Recruits_IsEnabled() ? sText_RecruitsRunFailed : sText_NuzlockeRunFailed, 8))
            gTasks[taskId].tState = NUZLOCKE_FAILED_PRINT_QUESTION;
        break;
    case NUZLOCKE_FAILED_PRINT_QUESTION:
        if (PrintNuzlockeFailedMessage(taskId, Recruits_IsEnabled() ? sText_RecruitsBeginNewRun : sText_NuzlockeBeginNewRun, 8))
            gTasks[taskId].tState = NUZLOCKE_FAILED_OPEN_YESNO;
        break;
    case NUZLOCKE_FAILED_OPEN_YESNO:
        // Default the cursor to NO -- there's no undo once a new run starts.
        OpenNuzlockeFailedYesNo(taskId, 1);
        gTasks[taskId].tState = NUZLOCKE_FAILED_PROCESS_YESNO;
        break;
    case NUZLOCKE_FAILED_PROCESS_YESNO:
        switch (Menu_ProcessInputNoWrap())
        {
        default: // MENU_NOTHING_CHOSEN -- cursor may have moved; repaint it to match this screen's colors
            DrawNuzlockeFailedYesNoCursor(gTasks[taskId].tYesNoWindowId);
            break;
        case 0: // YES -- start a new run, same as any other Nuzlocke-wipe entry point
            // ClearWindowTilemap before RemoveWindow: RemoveWindow only frees
            // the window's own buffer, it doesn't erase what was already
            // copied to the BG tilemap. Every other RemoveWindow call in this
            // function skips that step because a full screen teardown or
            // transition follows immediately either way -- but the
            // keep-storage branch below can stay on this same screen for a
            // few frames while the next question prints, and this box's
            // tiles would otherwise sit there stale (visible underneath the
            // new text) until the new Yes/No box happens to overwrite them.
            ClearWindowTilemap(gTasks[taskId].tYesNoWindowId);
            RemoveWindow(gTasks[taskId].tYesNoWindowId);
            if (CountAllStorageMons() == 0 && CalculatePlayerPartyCount() == 0)
            {
                // Nothing worth asking about -- same "nothing to keep"
                // shortcut CB2_InitKeepStoragePrompt takes on the title
                // screen's NEW GAME path, so just restart plainly.
                gKeepStorageOnNewGame = FALSE;
                FinishNuzlockeRestart(taskId);
            }
            else
            {
                // Ask fresh every time, rather than reusing
                // gSaveBlock2Ptr->keepStorageOnRestart: that field only
                // records whether *this* run's storage was itself carried
                // over from its predecessor, which is unconditionally
                // FALSE on a player's very first-ever run (there was
                // nothing to carry in yet) -- reading it here would
                // silently discard real PC storage the first time anyone
                // fails and restarts immediately. (As of Trading Codes.md
                // Stage 11, this field is otherwise write-only -- it used to
                // also gate the withdraw lock in pokemon_storage_system.c,
                // but that check now reads an explicit per-mon bit,
                // struct BoxPokemon's own legacyCarryOverLocked, instead.
                // Left in place regardless; a save-wide "has this file ever
                // carried storage over" record is reasonable bookkeeping to
                // keep even with no current reader.)
                gTasks[taskId].tState = NUZLOCKE_FAILED_PRINT_KEEP_STORAGE_INFO;
            }
            break;
        case 1: // NO
        case MENU_B_PRESSED:
            RemoveWindow(gTasks[taskId].tYesNoWindowId);
            RemoveWindow(gTasks[taskId].tWindowId);
            CleanupOverworldWindowsAndTilemaps();
            DestroyTask(taskId);
            // Nothing extra to do here: the emptied-party state was already
            // persisted to flash before this screen was ever reached
            // (RemoveFaintedMonsFromParty for Nuzlocke, TrySavingData in
            // Recruits_StartRunFailedScreen for Recruits), so gSaveFileStatus
            // stays a genuine SAVE_STATUS_OK -- keep-storage carryover keeps
            // working, on this boot or any later one. The title screen
            // (main_menu.c) is what keeps CONTINUE hidden, by checking
            // Run_IsFailed() against that same saved state directly.
            SetMainCallback2(CB2_InitTitleScreen);
            break;
        }
        break;
    case NUZLOCKE_FAILED_PRINT_KEEP_STORAGE_INFO:
        if (PrintNuzlockeFailedMessage(taskId, sText_NuzlockeKeepStorageInfo, 8))
            gTasks[taskId].tState = NUZLOCKE_FAILED_PRINT_KEEP_STORAGE_QUESTION;
        break;
    case NUZLOCKE_FAILED_PRINT_KEEP_STORAGE_QUESTION:
        if (PrintNuzlockeFailedMessage(taskId, sText_NuzlockeKeepStorageQuestion, 8))
            gTasks[taskId].tState = NUZLOCKE_FAILED_OPEN_KEEP_STORAGE_YESNO;
        break;
    case NUZLOCKE_FAILED_OPEN_KEEP_STORAGE_YESNO:
        // Default the cursor to YES here -- unlike restarting the run
        // itself, keeping stored POKéMON is the non-destructive answer
        // (same default the title-screen keep-storage prompt uses).
        OpenNuzlockeFailedYesNo(taskId, 0);
        gTasks[taskId].tState = NUZLOCKE_FAILED_PROCESS_KEEP_STORAGE_YESNO;
        break;
    case NUZLOCKE_FAILED_PROCESS_KEEP_STORAGE_YESNO:
        switch (Menu_ProcessInputNoWrap())
        {
        default: // MENU_NOTHING_CHOSEN -- cursor may have moved; repaint it to match this screen's colors
            DrawNuzlockeFailedYesNoCursor(gTasks[taskId].tYesNoWindowId);
            break;
        case 0: // YES -- keep this run's PC storage for the new one
            gKeepStorageOnNewGame = TRUE;
            FinishNuzlockeRestart(taskId);
            break;
        case 1: // NO
        case MENU_B_PRESSED:
            gKeepStorageOnNewGame = FALSE;
            FinishNuzlockeRestart(taskId);
            break;
        }
        break;
    }
}

void FieldCB_NuzlockeRunFailed(void)
{
    u8 taskId;

    LockPlayerFieldControls();
    FillPalBufferBlack();
    taskId = CreateTask(Task_NuzlockeRunFailed, 10);
    gTasks[taskId].tState = NUZLOCKE_FAILED_ENTER_MSG_SCREEN;
}

static void GetStairsMovementDirection(u32 metatileBehavior, s16 *speedX, s16 *speedY)
{
    if (MetatileBehavior_IsDirectionalUpRightStairWarp(metatileBehavior))
    {
        *speedX = 16;
        *speedY = -10;
    }
    else if (MetatileBehavior_IsDirectionalUpLeftStairWarp(metatileBehavior))
    {
        *speedX = -17;
        *speedY = -10;
    }
    else if (MetatileBehavior_IsDirectionalDownRightStairWarp(metatileBehavior))
    {
        *speedX = 17;
        *speedY = 3;
    }
    else if (MetatileBehavior_IsDirectionalDownLeftStairWarp(metatileBehavior))
    {
        *speedX = -17;
        *speedY = 3;
    }
    else
    {
        *speedX = 0;
        *speedY = 0;
    }
}

static bool8 WaitStairExitMovementFinished(s16 *speedX, s16 *speedY, s16 *offsetX, s16 *offsetY, s16 *timer)
{
    struct Sprite *sprite = &gSprites[gPlayerAvatar.spriteId];
    if (*timer != 0)
    {
        *offsetX += *speedX;
        *offsetY += *speedY;
        sprite->x2 = *offsetX >> 5;
        sprite->y2 = *offsetY >> 5;
        (*timer)--;
        return TRUE;
    }
    else
    {
        sprite->x2 = 0;
        sprite->y2 = 0;
        return FALSE;
    }
}

static void ExitStairsMovement(s16 *speedX, s16 *speedY, s16 *offsetX, s16 *offsetY, s16 *timer)
{
    s16 x, y;
    u32 metatileBehavior;
    s32 direction;
    struct Sprite *sprite;

    PlayerGetDestCoords(&x, &y);
    metatileBehavior = MapGridGetMetatileBehaviorAt(x, y);
    if (MetatileBehavior_IsDirectionalDownRightStairWarp(metatileBehavior) || MetatileBehavior_IsDirectionalUpRightStairWarp(metatileBehavior))
        direction = DIR_WEST;
    else
        direction = DIR_EAST;

    ObjectEventForceSetHeldMovement(&gObjectEvents[gPlayerAvatar.objectEventId], GetWalkInPlaceSlowMovementAction(direction));
    GetStairsMovementDirection(metatileBehavior, speedX, speedY);
    *offsetX = *speedX * 16;
    *offsetY = *speedY * 16;
    *timer = 16;
    sprite = &gSprites[gPlayerAvatar.spriteId];
    sprite->x2 = *offsetX >> 5;
    sprite->y2 = *offsetY >> 5;
    *speedX *= -1;
    *speedY *= -1;
}

#define tState data[0]
#define tSpeedX data[1]
#define tSpeedY data[2]
#define tOffsetX data[3]
#define tOffsetY data[4]
#define tTimer data[5]

static void Task_ExitStairs(u8 taskId)
{
    s16 * data = gTasks[taskId].data;
    switch (tState)
    {
    default:
        if (WaitForWeatherFadeIn() == TRUE)
        {
            CameraObjectReset();
            UnlockPlayerFieldControls();
            DestroyTask(taskId);
        }
        break;
    case 0:
        Overworld_PlaySpecialMapMusic();
        WarpFadeInScreen();
        LockPlayerFieldControls();
        ExitStairsMovement(&tSpeedX, &tSpeedY, &tOffsetX, &tOffsetY, &tTimer);
        tState++;
        break;
    case 1:
        if (!WaitStairExitMovementFinished(&tSpeedX, &tSpeedY, &tOffsetX, &tOffsetY, &tTimer))
            tState++;
        break;
    }
    gObjectEvents[gPlayerAvatar.objectEventId].noShadow = FALSE;
}

static void ForceStairsMovement(u32 metatileBehavior, s16 *speedX, s16 *speedY)
{
    ObjectEventForceSetHeldMovement(&gObjectEvents[gPlayerAvatar.objectEventId], GetWalkInPlaceNormalMovementAction(GetPlayerFacingDirection()));
    GetStairsMovementDirection(metatileBehavior, speedX, speedY);
    gObjectEvents[gPlayerAvatar.objectEventId].noShadow = TRUE;
}
#undef tSpeedX
#undef tSpeedY
#undef tOffsetX
#undef tOffsetY
#undef tTimer

#define tMetatileBehavior data[1]
#define tSpeedX           data[2]
#define tSpeedY           data[3]
#define tOffsetX          data[4]
#define tOffsetY          data[5]
#define tTimer            data[6]
#define tDelay            data[15]

static void UpdateStairsMovement(s16 speedX, s16 speedY, s16 *offsetX, s16 *offsetY, s16 *timer)
{
    struct Sprite *playerSprite = &gSprites[gPlayerAvatar.spriteId];
    struct ObjectEvent *playerObjectEvent = &gObjectEvents[gPlayerAvatar.objectEventId];

    if (speedY > 0 || *timer > 6)
        *offsetY += speedY;

    *offsetX += speedX;
    (*timer)++;
    playerSprite->x2 = *offsetX >> 5;
    playerSprite->y2 = *offsetY >> 5;
    if (playerObjectEvent->heldMovementFinished)
        ObjectEventForceSetHeldMovement(playerObjectEvent, GetWalkInPlaceNormalMovementAction(GetPlayerFacingDirection()));
}

static void Task_StairWarp(u8 taskId)
{
    s16 * data = gTasks[taskId].data;
    struct ObjectEvent *playerObjectEvent = &gObjectEvents[gPlayerAvatar.objectEventId];
    struct Sprite *playerSprite = &gSprites[gPlayerAvatar.spriteId];

    switch (tState)
    {
    case 0:
        LockPlayerFieldControls();
        FreezeObjectEvents();
        CameraObjectFreeze();
        HideFollowerForFieldEffect();
        tState++;
        break;
    case 1:
        if (!ObjectEventIsMovementOverridden(playerObjectEvent) || ObjectEventClearHeldMovementIfFinished(playerObjectEvent))
        {
            if (tDelay != 0)
            {
                tDelay--;
            }
            else
            {
                TryFadeOutOldMapMusic();
                PlayRainStoppingSoundEffect();
                playerSprite->oam.priority = 1;
                ForceStairsMovement(tMetatileBehavior, &tSpeedX, &tSpeedY);
                PlaySE(SE_EXIT);
                tState++;
            }
        }
        break;
    case 2:
        UpdateStairsMovement(tSpeedX, tSpeedY, &tOffsetX, &tOffsetY, &tTimer);
        tDelay++;
        if (tDelay >= 12)
        {
            WarpFadeOutScreen();
            tState++;
        }
        break;
    case 3:
        UpdateStairsMovement(tSpeedX, tSpeedY, &tOffsetX, &tOffsetY, &tTimer);
        if (!PaletteFadeActive() && BGMusicStopped())
            tState++;
        break;
    default:
        gFieldCallback = FieldCB_DefaultWarpExit;
        WarpIntoMap();
        SetMainCallback2(CB2_LoadMap);
        DestroyTask(taskId);
        break;
    }
}

void DoStairWarp(u16 metatileBehavior, u16 delay)
{
    u8 taskId = CreateTask(Task_StairWarp, 10);
    gTasks[taskId].tMetatileBehavior = metatileBehavior;
    gTasks[taskId].tDelay = delay;
    Task_StairWarp(taskId);
}

#undef tMetatileBehavior
#undef tSpeedX
#undef tSpeedY
#undef tOffsetX
#undef tOffsetY
#undef tTimer
#undef tDelay

bool32 IsDirectionalStairWarpMetatileBehavior(u16 metatileBehavior, enum Direction playerDirection)
{
    if (playerDirection == DIR_WEST)
    {
        if (MetatileBehavior_IsDirectionalUpLeftStairWarp(metatileBehavior))
            return TRUE;
        if (MetatileBehavior_IsDirectionalDownLeftStairWarp(metatileBehavior))
            return TRUE;
    }
    else if (playerDirection == DIR_EAST)
    {
        if (MetatileBehavior_IsDirectionalUpRightStairWarp(metatileBehavior))
            return TRUE;
        if (MetatileBehavior_IsDirectionalDownRightStairWarp(metatileBehavior))
            return TRUE;
    }
    return FALSE;
}
