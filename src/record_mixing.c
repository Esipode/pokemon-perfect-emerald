#include "global.h"
#include "malloc.h"
#include "random.h"
#include "constants/items.h"
#include "text.h"
#include "item.h"
#include "task.h"
#include "save.h"
#include "load_save.h"
#include "pokemon.h"
#include "cable_club.h"
#include "link.h"
#include "tv.h"
#include "battle_tower.h"
#include "window.h"
#include "mystery_event_script.h"
#include "mauville_old_man.h"
#include "sound.h"
#include "constants/songs.h"
#include "menu.h"
#include "overworld.h"
#include "field_screen_effect.h"
#include "fldeff_misc.h"
#include "script.h"
#include "event_data.h"
#include "lilycove_lady.h"
#include "strings.h"
#include "string_util.h"
#include "record_mixing.h"
#include "new_game.h"
#include "daycare.h"
#include "international_string_util.h"
#include "constants/battle_frontier.h"
#include "dewford_trend.h"

// Used by several tasks in this file
#define tState        data[0]

static void Task_RecordMixing_Main(u8);
static void Task_MixingRecordsRecv(u8);

void RecordMixingPlayerSpotTriggered(void)
{
    CreateTask_EnterCableClubSeat(Task_RecordMixing_Main);
}

static void PrintTextOnRecordMixing(const u8 *src)
{
    DrawDialogueFrame(0, FALSE);
    AddTextPrinterParameterized(0, FONT_NORMAL, src, 0, 1, 0, NULL);
    CopyWindowToVram(0, COPYWIN_FULL);
}

#define tCounter data[0]

static void Task_RecordMixing_SoundEffect(u8 taskId)
{
    if (++gTasks[taskId].tCounter == 50)
    {
        PlaySE(SE_M_ATTRACT);
        gTasks[taskId].tCounter = 0;
    }
}

#undef tCounter

#define tTimer       data[8]
#define tLinkTaskId  data[10]
#define tSoundTaskId data[15]

// Note: gSpecialVar_0x8005 here contains the player's spot id.
static void Task_RecordMixing_Main(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    switch (tState)
    {
    case 0: // init
        SetLocalLinkPlayerId(gSpecialVar_0x8005);
        VarSet(VAR_TEMP_MIXED_RECORDS, 1);
        DeactivateAllNormalTVShows();
        CreateRecordMixingLights();
        PrintTextOnRecordMixing(gText_LinkStandby2);
        tState = 1;
        tTimer = 0;
        tLinkTaskId = CreateTask(Task_MixingRecordsRecv, 80);
        tSoundTaskId = CreateTask(Task_RecordMixing_SoundEffect, 81);
        break;
    case 1: // The link handshake in Task_MixingRecordsRecv can never complete now that the
            // link stack is severed, so this used to hang forever with no way out. Give it
            // the same "no cable attached" treatment as every other Cable Club entry point:
            // let the player cancel, and bail out on a timeout regardless.
        if (JOY_NEW(B_BUTTON) || ++tTimer > 600)
        {
            DestroyTask(tLinkTaskId);
            DestroyTask(tSoundTaskId);
            DestroyRecordMixingLights();
            SetLinkWaitingForScript();
            ClearDialogWindowAndFrame(0, TRUE);
            DestroyTask(taskId);
            ScriptContext_Enable();
        }
        break;
    }
}

#undef tTimer

#undef tLinkTaskId
#undef tSoundTaskId

static void Task_MixingRecordsRecv(u8 taskId)
{
    struct Task *task = &gTasks[taskId];

    switch (task->tState)
    {
    case 0:
        PrintTextOnRecordMixing(gText_MixingRecords);
        task->data[8] = 0x708;
        task->tState = 400;
        ClearLinkCallback_2();
        break;
    case 100: // wait 20 frames
        if (++task->data[12] > 20)
        {
            task->data[12] = 0;
            task->tState = 101;
        }
        break;
    case 101:
        {
            u8 players = GetLinkPlayerCount_2();
            if (IsLinkMaster() == TRUE)
            {
                if (players == GetSavedPlayerCount())
                {
                    PlaySE(SE_PIN);
                    task->tState = 201;
                    task->data[12] = 0;
                }
            }
            else
            {
                PlaySE(SE_BOO);
                task->tState = 301;
            }
        }
        break;
    case 201:
        // We're the link master. Delay for 30 frames per connected player.
        if (GetSavedPlayerCount() == GetLinkPlayerCount_2() && ++task->data[12] > (GetLinkPlayerCount_2() * 30))
        {
            CheckShouldAdvanceLinkState();
            task->tState = 1;
        }
        break;
    case 301:
        if (GetSavedPlayerCount() == GetLinkPlayerCount_2())
            task->tState = 1;
        break;
    case 400: // wait 20 frames
        if (++task->data[12] > 20)
        {
            task->tState = 1;
            task->data[12] = 0;
        }
        break;
    case 1: // Handshake can never complete: gReceivedRemoteLinkPlayers is permanently FALSE
            // once the link stack was severed (see link.c) -- this state never advances.
            // The packet-send/receive machinery that used to live past this point (and the
            // EWRAM/IWRAM statics that only fed it) was unreachable and has been removed.
        break;
    }
}

void GetPlayerHallRecords(struct PlayerHallRecords *dst)
{
    s32 i, j;

    for (i = 0; i < HALL_FACILITIES_COUNT; i++)
    {
        for (j = 0; j < FRONTIER_LVL_MODE_COUNT; j++)
        {
            CopyTrainerId(dst->onePlayer[i][j].id, gSaveBlock2Ptr->playerTrainerId);
            dst->onePlayer[i][j].language = GAME_LANGUAGE;
            StringCopy(dst->onePlayer[i][j].name, gSaveBlock2Ptr->playerName);
        }
    }

#if FREE_BATTLE_FRONTIER == FALSE
    for (j = 0; j < FRONTIER_LVL_MODE_COUNT; j++)
    {
        dst->twoPlayers[j].language = GAME_LANGUAGE;
        CopyTrainerId(dst->twoPlayers[j].id1, gSaveBlock2Ptr->playerTrainerId);
        CopyTrainerId(dst->twoPlayers[j].id2, gSaveBlock2Ptr->frontier.opponentTrainerIds[j]);
        StringCopy(dst->twoPlayers[j].name1, gSaveBlock2Ptr->playerName);
        StringCopy(dst->twoPlayers[j].name2, gSaveBlock2Ptr->frontier.opponentNames[j]);
    }

    for (i = 0; i < FRONTIER_LVL_MODE_COUNT; i++)
    {
        dst->onePlayer[RANKING_HALL_TOWER_SINGLES][i].winStreak = gSaveBlock2Ptr->frontier.towerRecordWinStreaks[FRONTIER_MODE_SINGLES][i];
        dst->onePlayer[RANKING_HALL_TOWER_DOUBLES][i].winStreak = gSaveBlock2Ptr->frontier.towerRecordWinStreaks[FRONTIER_MODE_DOUBLES][i];
        dst->onePlayer[RANKING_HALL_TOWER_MULTIS][i].winStreak = gSaveBlock2Ptr->frontier.towerRecordWinStreaks[FRONTIER_MODE_MULTIS][i];
        dst->onePlayer[RANKING_HALL_DOME][i].winStreak = gSaveBlock2Ptr->frontier.domeRecordWinStreaks[FRONTIER_MODE_SINGLES][i];
        dst->onePlayer[RANKING_HALL_PALACE][i].winStreak = gSaveBlock2Ptr->frontier.palaceRecordWinStreaks[FRONTIER_MODE_SINGLES][i];
        dst->onePlayer[RANKING_HALL_ARENA][i].winStreak = gSaveBlock2Ptr->frontier.arenaRecordStreaks[i];
        dst->onePlayer[RANKING_HALL_FACTORY][i].winStreak = gSaveBlock2Ptr->frontier.factoryRecordWinStreaks[FRONTIER_MODE_SINGLES][i];
        dst->onePlayer[RANKING_HALL_PIKE][i].winStreak = gSaveBlock2Ptr->frontier.pikeRecordStreaks[i];
        dst->onePlayer[RANKING_HALL_PYRAMID][i].winStreak = gSaveBlock2Ptr->frontier.pyramidRecordStreaks[i];

        dst->twoPlayers[i].winStreak = gSaveBlock2Ptr->frontier.towerRecordWinStreaks[FRONTIER_MODE_LINK_MULTIS][i];
    }
#else
    // No frontier facility exists to have streaks or two-player opponent records for.
    memset(dst->twoPlayers, 0, sizeof(dst->twoPlayers));
    for (i = 0; i < HALL_FACILITIES_COUNT; i++)
        for (j = 0; j < FRONTIER_LVL_MODE_COUNT; j++)
            dst->onePlayer[i][j].winStreak = 0;
#endif //FREE_BATTLE_FRONTIER
}
