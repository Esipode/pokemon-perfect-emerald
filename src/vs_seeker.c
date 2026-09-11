#include "global.h"
#include "task.h"
#include "event_object_movement.h"
#include "item_use.h"
#include "event_scripts.h"
#include "event_data.h"
#include "script.h"
#include "event_object_lock.h"
#include "field_specials.h"
#include "item.h"
#include "item_menu.h"
#include "field_effect.h"
#include "script_movement.h"
#include "battle.h"
#include "battle_setup.h"
#include "random.h"
#include "field_player_avatar.h"
#include "fieldmap.h"
#include "vs_seeker.h"
#include "menu.h"
#include "string_util.h"
#include "tv.h"
#include "malloc.h"
#include "field_screen_effect.h"
#include "sound.h"
#include "constants/event_object_movement.h"
#include "constants/event_objects.h"
#include "constants/items.h"
#include "constants/maps.h"
#include "constants/songs.h"
#include "constants/trainer_types.h"
#include "constants/field_effects.h"

// Documentation for the Vs. Seeker can be found in docs/tutorials/vs_seeker.md.
// The scan only reports which nearby trainers have yet to be fought; it no longer
// marks anyone for a rematch, because the rematch table is gone.

enum
{
   VSSEEKER_NO_ONE_IN_RANGE,
   VSSEEKER_CAN_USE,
};

typedef enum
{
    VSSEEKER_RESPONSE_NO_RESPONSE,
    VSSEEKER_RESPONSE_UNFOUGHT_TRAINERS
} VsSeekerResponseCode;

struct VsSeekerTrainerInfo
{
    const u8 *script;
    u16 trainerIdx;
    u8 localId;
    u8 objectEventId;
    s16 xCoord;
    s16 yCoord;
    u8 graphicsId;
};

struct VsSeekerStruct
{
    struct VsSeekerTrainerInfo trainerInfo[OBJECT_EVENTS_COUNT];
};

#define END_TRAINER_INFO 0xFF
// static declarations
static EWRAM_DATA struct VsSeekerStruct *sVsSeeker = NULL;

static void Task_VsSeekerFrameCountdown(u8 taskId);
static void Task_VsSeeker_PlaySoundAndGetResponseCode(u8 taskId);
static void GatherNearbyTrainerInfo(void);
static void Task_VsSeeker_ShowResponseToPlayer(u8 taskId);
static bool8 CanUseVsSeeker(void);
static u8 GetVsSeekerResponseInArea(void);
static bool8 IsTrainerVisibleOnScreen(struct VsSeekerTrainerInfo * trainerInfo);
static bool32 HasFightableTrainers(void);
static void StartTrainerObjectMovementScript(struct VsSeekerTrainerInfo * trainerInfo, const u8 * script);
static bool8 ObjectEventIdIsSane(u8 objectEventId);

static const u8 sMovementScript_Wait48[] = {
    MOVEMENT_ACTION_DELAY_16,
    MOVEMENT_ACTION_DELAY_16,
    MOVEMENT_ACTION_DELAY_16,
    MOVEMENT_ACTION_STEP_END
};

static const u8 sMovementScript_TrainerUnfought[] = {
    MOVEMENT_ACTION_EMOTE_EXCLAMATION_MARK,
    MOVEMENT_ACTION_STEP_END
};

static const u8 sMovementScript_TrainerUnknown[] = {
    MOVEMENT_ACTION_EMOTE_QUESTION_MARK,
    MOVEMENT_ACTION_STEP_END
};

static const u8 sMovementScript_TrainerNoResponse[] = {
    MOVEMENT_ACTION_EMOTE_X,
    MOVEMENT_ACTION_STEP_END
};

#define tCountdown      data[0]
#define tBeepDelay      data[1]
#define tNumBeeps       data[2]
#define tResponseCode   data[3]

void Task_InitVsSeekerAndCheckForTrainersOnScreen(u8 taskId)
{
    u32 i;
    u32 respval;

    if (!I_VS_SEEKER_CHARGING) return;

    for (i = 0; i < 16; i++)
        gTasks[taskId].data[i] = 0;

    sVsSeeker = AllocZeroed(sizeof(struct VsSeekerStruct));
    GatherNearbyTrainerInfo();
    respval = CanUseVsSeeker();
    if (respval == VSSEEKER_NO_ONE_IN_RANGE)
    {
        Free(sVsSeeker);
        DisplayItemMessageOnField(taskId, VSSeeker_Text_NoTrainersWithinRange, Task_ItemUse_CloseMessageBoxAndReturnToField_VsSeeker);
    }
    else if (respval == VSSEEKER_CAN_USE)
    {
        FieldEffectStart(FLDEFF_USE_VS_SEEKER);
        gTasks[taskId].func = Task_VsSeekerFrameCountdown;
        gTasks[taskId].tCountdown = 15;
    }
}

static void Task_VsSeekerFrameCountdown(u8 taskId)
{
    if (--gTasks[taskId].tCountdown == 0)
    {
        gTasks[taskId].func = Task_VsSeeker_PlaySoundAndGetResponseCode;
        gTasks[taskId].tBeepDelay = 16;
    }
}

static void Task_VsSeeker_PlaySoundAndGetResponseCode(u8 taskId)
{
    s16 * data = gTasks[taskId].data;

    if (tNumBeeps != 2 && --tBeepDelay == 0)
    {
        PlaySE(SE_CONTEST_MONS_TURN);
        tBeepDelay = 11;
        tNumBeeps++;
    }

    if (!FieldEffectActiveListContains(FLDEFF_USE_VS_SEEKER))
    {
        tResponseCode = GetVsSeekerResponseInArea();
        ScriptMovement_StartObjectMovementScript(LOCALID_PLAYER, gSaveBlock1Ptr->location.mapNum, gSaveBlock1Ptr->location.mapGroup, sMovementScript_Wait48);
        gTasks[taskId].func = Task_VsSeeker_ShowResponseToPlayer;
    }
}

static void Task_VsSeeker_ShowResponseToPlayer(u8 taskId)
{
    if (!ScriptMovement_IsObjectMovementFinished(LOCALID_PLAYER, gSaveBlock1Ptr->location.mapNum, gSaveBlock1Ptr->location.mapGroup))
        return;

    if (gTasks[taskId].tResponseCode == VSSEEKER_RESPONSE_NO_RESPONSE)
    {
        DisplayItemMessageOnField(taskId, VSSeeker_Text_TrainersNotReady, Task_ItemUse_CloseMessageBoxAndReturnToField_VsSeeker);
    }
    else
    {
        ClearDialogWindowAndFrame(0, TRUE);
        ScriptUnfreezeObjectEvents();
        UnlockPlayerFieldControls();
        DestroyTask(taskId);
    }
    Free(sVsSeeker);
}

#undef tCountdown
#undef tBeepDelay
#undef tNumBeeps
#undef tResponseCode

static void GatherNearbyTrainerInfo(void)
{
    struct ObjectEventTemplate *templates = gSaveBlock1Ptr->objectEventTemplates;
    u8 objectEventId = 0;
    u8 vsSeekerObjectIdx = 0;
    s32 objectEventIdx;

    for (objectEventIdx = 0; objectEventIdx < gMapHeader.events->objectEventCount; objectEventIdx++)
    {
        u16 trainerIdx = GetTrainerFlagFromScript(templates[objectEventIdx].script);
        if (trainerIdx == TRAINER_NONE && (!I_SHOW_NO_ID_TRAINER || templates[objectEventIdx].trainerType == TRAINER_TYPE_NONE))
            continue;
        if (trainerIdx == TRAINER_NONE && templates[objectEventIdx].trainerType != TRAINER_TYPE_NONE)
            DebugPrintf("Object event with local id %d is not TRAINER_TYPE_NONE but doesn't have a visible trainerID", templates[objectEventIdx].localId);
        sVsSeeker->trainerInfo[vsSeekerObjectIdx].script = templates[objectEventIdx].script;
        sVsSeeker->trainerInfo[vsSeekerObjectIdx].trainerIdx = trainerIdx;
        sVsSeeker->trainerInfo[vsSeekerObjectIdx].localId = templates[objectEventIdx].localId;
        TryGetObjectEventIdByLocalIdAndMap(templates[objectEventIdx].localId, gSaveBlock1Ptr->location.mapNum, gSaveBlock1Ptr->location.mapGroup, &objectEventId);
        sVsSeeker->trainerInfo[vsSeekerObjectIdx].objectEventId = objectEventId;
        sVsSeeker->trainerInfo[vsSeekerObjectIdx].xCoord = gObjectEvents[objectEventId].currentCoords.x - MAP_OFFSET;
        sVsSeeker->trainerInfo[vsSeekerObjectIdx].yCoord = gObjectEvents[objectEventId].currentCoords.y - MAP_OFFSET;
        sVsSeeker->trainerInfo[vsSeekerObjectIdx].graphicsId = templates[objectEventIdx].graphicsId;
        vsSeekerObjectIdx++;
    }
    sVsSeeker->trainerInfo[vsSeekerObjectIdx].localId = END_TRAINER_INFO;
}

static u8 CanUseVsSeeker(void)
{
    if (!HasFightableTrainers())
        return VSSEEKER_NO_ONE_IN_RANGE;

    return VSSEEKER_CAN_USE;
}

static u8 GetVsSeekerResponseInArea(void)
{
    u32 trainerIdx;
    s32 vsSeekerIdx = 0;
    bool32 trainerHasNotYetBeenFought = FALSE;

    for (vsSeekerIdx = 0; sVsSeeker->trainerInfo[vsSeekerIdx].localId != END_TRAINER_INFO; vsSeekerIdx++)
    {
        if (!IsTrainerVisibleOnScreen(&sVsSeeker->trainerInfo[vsSeekerIdx]))
            continue;

        trainerIdx = sVsSeeker->trainerInfo[vsSeekerIdx].trainerIdx;
        if (trainerIdx == TRAINER_NONE)
        {
            StartTrainerObjectMovementScript(&sVsSeeker->trainerInfo[vsSeekerIdx], sMovementScript_TrainerUnknown);
            continue;
        }

        if (!HasTrainerBeenFought(trainerIdx))
        {
            StartTrainerObjectMovementScript(&sVsSeeker->trainerInfo[vsSeekerIdx], sMovementScript_TrainerUnfought);
            trainerHasNotYetBeenFought = TRUE;
            continue;
        }

        StartTrainerObjectMovementScript(&sVsSeeker->trainerInfo[vsSeekerIdx], sMovementScript_TrainerNoResponse);
    }

    if (trainerHasNotYetBeenFought)
        return VSSEEKER_RESPONSE_UNFOUGHT_TRAINERS;

    return VSSEEKER_RESPONSE_NO_RESPONSE;
}

bool32 IsVsSeekerEnabled(void)
{
    if (I_VS_SEEKER_CHARGING == 0)
        return FALSE;

    return (CheckBagHasItem(ITEM_VS_SEEKER, 1));
}

static bool8 ObjectEventIdIsSane(u8 objectEventId)
{
    struct ObjectEvent *objectEvent = &gObjectEvents[objectEventId];

    if (objectEvent->active && gMapHeader.events->objectEventCount >= objectEvent->localId && gSprites[objectEvent->spriteId].data[0] == objectEventId)
        return TRUE;
    return FALSE;
}

static bool8 IsTrainerVisibleOnScreen(struct VsSeekerTrainerInfo * trainerInfo)
{
    s16 x;
    s16 y;

    PlayerGetDestCoords(&x, &y);
    x -= MAP_OFFSET;
    y -= MAP_OFFSET;

    if (   x - 7 <= trainerInfo->xCoord
        && x + 7 >= trainerInfo->xCoord
        && y - 5 <= trainerInfo->yCoord
        && y + 5 >= trainerInfo->yCoord
        && ObjectEventIdIsSane(trainerInfo->objectEventId))
        return TRUE;
    return FALSE;
}

static bool32 HasFightableTrainers(void)
{
    u32 i;

    for (i = 0; sVsSeeker->trainerInfo[i].localId != END_TRAINER_INFO; i++)
    {
        if (IsTrainerVisibleOnScreen(&sVsSeeker->trainerInfo[i]))
            return TRUE;
    }

    return FALSE;
}

static void StartTrainerObjectMovementScript(struct VsSeekerTrainerInfo * trainerInfo, const u8 * script)
{
    UnfreezeObjectEvent(&gObjectEvents[trainerInfo->objectEventId]);
    ScriptMovement_StartObjectMovementScript(trainerInfo->localId, gSaveBlock1Ptr->location.mapNum, gSaveBlock1Ptr->location.mapGroup, script);
}
