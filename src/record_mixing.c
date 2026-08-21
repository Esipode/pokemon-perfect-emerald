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

struct PlayerRecordRS
{
    struct SecretBase secretBases[SECRET_BASES_COUNT];
    TVShow tvShows[TV_SHOWS_COUNT];
    PokeNews pokeNews[POKE_NEWS_COUNT];
    OldMan oldMan;
    struct DewfordTrend dewfordTrends[SAVED_TRENDS_COUNT];
    struct RecordMixingDaycareMail daycareMail;
    struct RSBattleTowerRecord battleTowerRecord;
    u16 giftItem;
    u16 filler[50];
};

struct PlayerRecordEmerald
{
    /* 0x0000 */ struct SecretBase secretBases[SECRET_BASES_COUNT];
    /* 0x0C80 */ TVShow tvShows[TV_SHOWS_COUNT];
    /* 0x1004 */ PokeNews pokeNews[POKE_NEWS_COUNT];
    /* 0x1044 */ OldMan oldMan;
    /* 0x1084 */ struct DewfordTrend dewfordTrends[SAVED_TRENDS_COUNT];
    /* 0x10AC */ struct RecordMixingDaycareMail daycareMail;
    /* 0x1124 */ struct EmeraldBattleTowerRecord battleTowerRecord;
    /* 0x1210 */ u16 giftItem;
    /* 0x1214 */ LilycoveLady lilycoveLady;
    /* 0x1254 */ struct Apprentice apprentices[2];
    /* 0x12DC */ struct PlayerHallRecords hallRecords;
    /* 0x1434 */ u8 filler_1434[16];
}; // 0x1444

union PlayerRecord
{
    struct PlayerRecordRS ruby;
    struct PlayerRecordEmerald emerald;
};

// Record mixing is a link-only feature; when the FREE_* toggles below remove
// their save data, these dummy (always-empty) buffers keep the record mixing
// packet-copying logic below compiling without special-casing every site.
#if FREE_SECRET_BASES == TRUE
static EWRAM_DATA struct SecretBase sDummySecretBasesSave[SECRET_BASES_COUNT] = {0};
#endif //FREE_SECRET_BASES
#if FREE_OLD_MAN == TRUE
static EWRAM_DATA OldMan sDummyOldManSave = {0};
#endif //FREE_OLD_MAN
#if FREE_DEWFORD_TRENDS == TRUE
static EWRAM_DATA struct DewfordTrend sDummyDewfordTrendsSave[SAVED_TRENDS_COUNT] = {0};
#endif //FREE_DEWFORD_TRENDS
#if FREE_LILYCOVE_LADY == TRUE
static EWRAM_DATA LilycoveLady sDummyLilycoveLadySave = {0};
#endif //FREE_LILYCOVE_LADY
#if FREE_BATTLE_FRONTIER == TRUE
static EWRAM_DATA struct EmeraldBattleTowerRecord sDummyBattleTowerSave = {0};
static EWRAM_DATA struct Apprentice sDummyApprenticesSave[APPRENTICE_COUNT] = {0};
#endif //FREE_BATTLE_FRONTIER

static struct SecretBase *sSecretBasesSave;
static TVShow *sTvShowsSave;
static PokeNews *sPokeNewsSave;
static OldMan *sOldManSave;
static struct DewfordTrend *sDewfordTrendsSave;
static struct RecordMixingDaycareMail *sRecordMixMailSave;
static void *sBattleTowerSave;
static LilycoveLady *sLilycoveLadySave;
static void *sApprenticesSave;

static EWRAM_DATA struct RecordMixingDaycareMail sRecordMixMail = {0};
static EWRAM_DATA union PlayerRecord *sSentRecord = NULL;

static void Task_RecordMixing_Main(u8);
static void Task_MixingRecordsRecv(u8);
static void GetSavedApprentices(struct Apprentice *, struct Apprentice *);
static void GetRecordMixingDaycareMail(struct RecordMixingDaycareMail *);
static void SanitizeDaycareMailForRuby(struct RecordMixingDaycareMail *);
static void SanitizeEmeraldBattleTowerRecord(struct EmeraldBattleTowerRecord *);
static void SanitizeRubyBattleTowerRecord(struct RSBattleTowerRecord *);

void RecordMixingPlayerSpotTriggered(void)
{
    CreateTask_EnterCableClubSeat(Task_RecordMixing_Main);
}

// these variables were const in R/S, but had to become changeable because of saveblocks changing RAM position
static void SetSrcLookupPointers(void)
{
#if FREE_SECRET_BASES == FALSE
    sSecretBasesSave = gSaveBlock1Ptr->secretBases;
#else
    sSecretBasesSave = sDummySecretBasesSave;
#endif //FREE_SECRET_BASES
    sTvShowsSave = gSaveBlock1Ptr->tvShows;
    sPokeNewsSave = gSaveBlock1Ptr->pokeNews;
#if FREE_OLD_MAN == FALSE
    sOldManSave = &gSaveBlock1Ptr->oldMan;
#else
    sOldManSave = &sDummyOldManSave;
#endif //FREE_OLD_MAN
#if FREE_DEWFORD_TRENDS == FALSE
    sDewfordTrendsSave = gSaveBlock1Ptr->dewfordTrends;
#else
    sDewfordTrendsSave = sDummyDewfordTrendsSave;
#endif //FREE_DEWFORD_TRENDS
    sRecordMixMailSave = &sRecordMixMail;
#if FREE_BATTLE_FRONTIER == FALSE
    sBattleTowerSave = &gSaveBlock2Ptr->frontier.towerPlayer;
#else
    sBattleTowerSave = &sDummyBattleTowerSave;
#endif //FREE_BATTLE_FRONTIER
#if FREE_LILYCOVE_LADY == FALSE
    sLilycoveLadySave = &gSaveBlock1Ptr->lilycoveLady;
#else
    sLilycoveLadySave = &sDummyLilycoveLadySave;
#endif //FREE_LILYCOVE_LADY
#if FREE_BATTLE_FRONTIER == FALSE
    sApprenticesSave = gSaveBlock2Ptr->apprentices;
#else
    sApprenticesSave = sDummyApprenticesSave;
#endif //FREE_BATTLE_FRONTIER
}

static void PrepareUnknownExchangePacket(struct PlayerRecordRS *dest)
{
    memcpy(dest->secretBases, sSecretBasesSave, sizeof(dest->secretBases));
    memcpy(dest->tvShows, sTvShowsSave, sizeof(dest->tvShows));
    SanitizeTVShowLocationsForRuby(dest->tvShows);
    memcpy(dest->pokeNews, sPokeNewsSave, sizeof(dest->pokeNews));
    memcpy(&dest->oldMan, sOldManSave, sizeof(dest->oldMan));
    memcpy(dest->dewfordTrends, sDewfordTrendsSave, sizeof(dest->dewfordTrends));
    GetRecordMixingDaycareMail(&dest->daycareMail);
    EmeraldBattleTowerRecordToRuby(sBattleTowerSave, &dest->battleTowerRecord);

    if (GetMultiplayerId() == 0)
        dest->giftItem = GetRecordMixingGift();
}

static void PrepareExchangePacketForRubySapphire(struct PlayerRecordRS *dest)
{
    memcpy(dest->secretBases, sSecretBasesSave, sizeof(dest->secretBases));
    memcpy(dest->tvShows, sTvShowsSave, sizeof(dest->tvShows));
    SanitizeTVShowsForRuby(dest->tvShows);
    memcpy(dest->pokeNews, sPokeNewsSave, sizeof(dest->pokeNews));
    memcpy(&dest->oldMan, sOldManSave, sizeof(dest->oldMan));
    SanitizeMauvilleOldManForRuby(&dest->oldMan);
    memcpy(dest->dewfordTrends, sDewfordTrendsSave, sizeof(dest->dewfordTrends));
    GetRecordMixingDaycareMail(&dest->daycareMail);
    SanitizeDaycareMailForRuby(&dest->daycareMail);
    EmeraldBattleTowerRecordToRuby(sBattleTowerSave, &dest->battleTowerRecord);
    SanitizeRubyBattleTowerRecord(&dest->battleTowerRecord);

    if (GetMultiplayerId() == 0)
        dest->giftItem = GetRecordMixingGift();
}

static void PrepareExchangePacket(void)
{
    DeactivateAllNormalTVShows();
    SetSrcLookupPointers();

    if (Link_AnyPartnersPlayingRubyOrSapphire())
    {
        if (LinkDummy_Return2() == 0)
            PrepareUnknownExchangePacket(&sSentRecord->ruby);
        else
            PrepareExchangePacketForRubySapphire(&sSentRecord->ruby);
    }
    else
    {
        memcpy(sSentRecord->emerald.secretBases, sSecretBasesSave, sizeof(sSentRecord->emerald.secretBases));
        memcpy(sSentRecord->emerald.tvShows, sTvShowsSave, sizeof(sSentRecord->emerald.tvShows));
        memcpy(sSentRecord->emerald.pokeNews, sPokeNewsSave, sizeof(sSentRecord->emerald.pokeNews));
        memcpy(&sSentRecord->emerald.oldMan, sOldManSave, sizeof(sSentRecord->emerald.oldMan));
        memcpy(&sSentRecord->emerald.lilycoveLady, sLilycoveLadySave, sizeof(sSentRecord->emerald.lilycoveLady));
        memcpy(sSentRecord->emerald.dewfordTrends, sDewfordTrendsSave, sizeof(sSentRecord->emerald.dewfordTrends));
        GetRecordMixingDaycareMail(&sSentRecord->emerald.daycareMail);
        memcpy(&sSentRecord->emerald.battleTowerRecord, sBattleTowerSave, sizeof(sSentRecord->emerald.battleTowerRecord));
        SanitizeEmeraldBattleTowerRecord(&sSentRecord->emerald.battleTowerRecord);

        if (GetMultiplayerId() == 0)
            sSentRecord->emerald.giftItem = GetRecordMixingGift();

        GetSavedApprentices(sSentRecord->emerald.apprentices, sApprenticesSave);
        GetPlayerHallRecords(&sSentRecord->emerald.hallRecords);
    }
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
        sSentRecord = Alloc(sizeof(*sSentRecord));
        SetLocalLinkPlayerId(gSpecialVar_0x8005);
        VarSet(VAR_TEMP_MIXED_RECORDS, 1);
        PrepareExchangePacket();
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
            Free(sSentRecord);
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

static void GetSavedApprentices(struct Apprentice *dst, struct Apprentice *src)
{
    dst[0].playerName[0] = EOS;
    dst[1].playerName[0] = EOS;
#if FREE_BATTLE_FRONTIER == TRUE
    // Stage 4: apprentices[] no longer exists -- nothing to send.
    return;
#else
    s32 i, id;
    s32 apprenticeSaveId, oldPlayerApprenticeSaveId;
    s32 numOldPlayerApprentices, numMixApprentices;

    dst[0] = src[0];

    oldPlayerApprenticeSaveId = 0;
    numOldPlayerApprentices = 0;
    apprenticeSaveId = 0;
    numMixApprentices = 0;
    for (i = 0; i < 2; i++)
    {
        id = (i + gSaveBlock2Ptr->playerApprentice.saveId) % (APPRENTICE_COUNT - 1) + 1;
        if (src[id].playerName[0] != EOS)
        {
            if (GetTrainerId(src[id].playerId) != GetTrainerId(gSaveBlock2Ptr->playerTrainerId))
            {
                numMixApprentices++;
                apprenticeSaveId = id;
            }
            if (GetTrainerId(src[id].playerId) == GetTrainerId(gSaveBlock2Ptr->playerTrainerId))
            {
                numOldPlayerApprentices++;
                oldPlayerApprenticeSaveId = id;
            }
        }
    }

    // Prefer passing on other mixed Apprentices rather than old player's Apprentices
    if (numMixApprentices == 0 && numOldPlayerApprentices != 0)
    {
        numMixApprentices = numOldPlayerApprentices;
        apprenticeSaveId = oldPlayerApprenticeSaveId;
    }

    switch (numMixApprentices)
    {
    case 1:
        dst[1] = src[apprenticeSaveId];
        break;
    case 2:
        if (Random2() > 0x3333)
            dst[1] = src[gSaveBlock2Ptr->playerApprentice.saveId + 1];
        else
            dst[1] = src[((gSaveBlock2Ptr->playerApprentice.saveId + 1) % (APPRENTICE_COUNT - 1) + 1)];
        break;
    }
#endif //FREE_BATTLE_FRONTIER
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
    // Stage 4: no frontier facility exists to have streaks or two-player opponent records for.
    // sSentRecord (the caller's dst) comes from a plain Alloc(), not AllocZeroed(), so explicitly
    // zero these fields rather than send whatever was left on the heap over the link.
    memset(dst->twoPlayers, 0, sizeof(dst->twoPlayers));
    for (i = 0; i < HALL_FACILITIES_COUNT; i++)
        for (j = 0; j < FRONTIER_LVL_MODE_COUNT; j++)
            dst->onePlayer[i][j].winStreak = 0;
#endif //FREE_BATTLE_FRONTIER
}

static void GetRecordMixingDaycareMail(struct RecordMixingDaycareMail *dst)
{
    sRecordMixMail.mail[0] = gSaveBlock1Ptr->daycare.mons[0].mail;
    sRecordMixMail.mail[1] = gSaveBlock1Ptr->daycare.mons[1].mail;
    InitDaycareMailRecordMixing(&gSaveBlock1Ptr->daycare, &sRecordMixMail);
    *dst = *sRecordMixMailSave;
}

static void SanitizeDaycareMailForRuby(struct RecordMixingDaycareMail *src)
{
    s32 i;

    for (i = 0; i < src->numDaycareMons; i++)
    {
        struct DaycareMail *mail = &src->mail[i];
#if FREE_MAIL == FALSE
        if (mail->message.itemId != ITEM_NONE)
#endif //FREE_MAIL
        {
            if (mail->gameLanguage != LANGUAGE_JAPANESE)
                PadNameString(mail->otName, EXT_CTRL_CODE_BEGIN);

            ConvertInternationalString(mail->monName, mail->monLanguage);
        }
    }
}

static void SanitizeRubyBattleTowerRecord(struct RSBattleTowerRecord *src)
{

}

static void SanitizeEmeraldBattleTowerRecord(struct EmeraldBattleTowerRecord *dst)
{
    s32 i;

    for (i = 0; i < MAX_FRONTIER_PARTY_SIZE; i++)
    {
        struct BattleTowerPokemon *towerMon = &dst->party[i];
        if (towerMon->species != SPECIES_NONE)
            StripExtCtrlCodes(towerMon->nickname);
    }

    CalcEmeraldBattleTowerChecksum(dst);
}
