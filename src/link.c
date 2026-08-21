#include "global.h"
#include "m4a.h"
#include "malloc.h"
#include "reload_save.h"
#include "save.h"
#include "bg.h"
#include "window.h"
#include "random.h"
#include "decompress.h"
#include "string_util.h"
#include "event_data.h"
#include "item_menu.h"
#include "overworld.h"
#include "gpu_regs.h"
#include "palette.h"
#include "task.h"
#include "scanline_effect.h"
#include "menu.h"
#include "text.h"
#include "strings.h"
#include "constants/songs.h"
#include "sound.h"
#include "trade.h"
#include "battle.h"
#include "link.h"
#include "constants/rgb.h"
#include "constants/trade.h"

// Window IDs for the link error screens
enum {
    WIN_LINK_ERROR_TOP,
    WIN_LINK_ERROR_MID,
    WIN_LINK_ERROR_BOTTOM,
};

struct BlockTransfer
{
    u16 pos;
    u16 size;
    const u8 *src;
    bool8 active;
    u8 multiplayerId;
};

struct LinkTestBGInfo
{
    u32 screenBaseBlock;
    u32 paletteNum;
    u32 baseChar;
    u32 unused;
};

static struct BlockTransfer sBlockSend;
static struct BlockTransfer sBlockRecv[MAX_LINK_PLAYERS];
static u32 sBlockSendDelayCounter;
static u32 sPlayerDataExchangeStatus;
static u8 sLinkTestLastBlockSendPos;
static u8 sLinkTestLastBlockRecvPos[MAX_LINK_PLAYERS];

COMMON_DATA u16 gLinkPartnersHeldKeys[6] = {0};
COMMON_DATA u32 gLinkDebugSeed = 0;
COMMON_DATA struct LinkPlayerBlock gLocalLinkPlayerBlock = {0};
COMMON_DATA bool8 gLinkErrorOccurred = 0;
COMMON_DATA u32 gLinkDebugFlags = 0;
COMMON_DATA bool8 gRemoteLinkPlayersNotReceived[MAX_LINK_PLAYERS] = {0};
COMMON_DATA u8 gBlockReceivedStatus[MAX_LINK_PLAYERS] = {0};
COMMON_DATA u16 gLinkHeldKeys = 0;
// Still a live comms scratchpad for src/berry_blender.c's own protocol, independent of the
// LinkMain1 packet queues removed above.
COMMON_DATA u16 ALIGNED(4) gRecvCmds[MAX_RFU_PLAYERS][CMD_LENGTH] = {0};
COMMON_DATA u32 gLinkStatus = 0;
COMMON_DATA bool8 gReadyToExitStandby[MAX_LINK_PLAYERS] = {0};
COMMON_DATA bool8 gReadyToCloseLink[MAX_LINK_PLAYERS] = {0};
COMMON_DATA u16 gReadyCloseLinkType = 0; // Never read
COMMON_DATA u8 gSuppressLinkErrorMessage = 0;
COMMON_DATA bool8 gWirelessCommType = 0;

// crt0.s's master interrupt dispatcher unconditionally reads gSTWIStatus->timerSelect (offset 0xA)
// to decide which timer IRQ may preempt a nested interrupt handler -- baked into every retail
// Emerald ROM's boot code regardless of whether a wireless adapter is ever connected. The RFU
// driver that used to allocate and zero this struct is gone, so keep a permanently zeroed dummy
// for the interrupt handler to point at instead of dereferencing a null/uninitialized pointer.
// Pointed at sDummySTWIStatus during early boot, before interrupts can fire (see AgbMain).
COMMON_DATA void *gSTWIStatus = NULL;
EWRAM_DATA static u8 sDummySTWIStatus[16] = {0};
COMMON_DATA bool8 gSavedLinkPlayerCount = 0;
COMMON_DATA u16 gSendCmd[CMD_LENGTH] = {0};
COMMON_DATA u8 gSavedMultiplayerId = 0;
COMMON_DATA bool8 gReceivedRemoteLinkPlayers = 0;
COMMON_DATA struct LinkTestBGInfo gLinkTestBGInfo = {0};
COMMON_DATA void (*gLinkCallback)(void) = NULL;
COMMON_DATA u8 gShouldAdvanceLinkState = 0;
COMMON_DATA u16 gLinkTestBlockChecksums[MAX_LINK_PLAYERS] = {0};
COMMON_DATA u8 gBlockRequestType = 0;
COMMON_DATA u8 gLastSendQueueCount = 0;
COMMON_DATA struct Link gLink = {0};
COMMON_DATA u8 gLastRecvQueueCount = 0;
COMMON_DATA u16 gLinkSavedIme = 0;

static EWRAM_DATA u8 sLinkTestDebugValuesEnabled = 0;
EWRAM_DATA u32 gBerryBlenderKeySendAttempts = 0;
EWRAM_DATA u16 gBlockRecvBuffer[MAX_RFU_PLAYERS][BLOCK_BUFFER_SIZE / 2] = {};
EWRAM_DATA u8 gBlockSendBuffer[BLOCK_BUFFER_SIZE] = {};
EWRAM_DATA u16 gLinkType = 0;
static EWRAM_DATA u16 sTimeOutCounter = 0;
EWRAM_DATA struct LinkPlayer gLocalLinkPlayer = {};
EWRAM_DATA struct LinkPlayer gLinkPlayers[MAX_RFU_PLAYERS] = {};
static EWRAM_DATA struct LinkPlayer sSavedLinkPlayers[MAX_RFU_PLAYERS] = {};
static EWRAM_DATA struct {
    u32 status;
    u8 lastRecvQueueCount;
    u8 lastSendQueueCount;
    bool8 disconnected;
} sLinkErrorBuffer = {};
static EWRAM_DATA u16 sReadyCloseLinkAttempts = 0; // never read

static void InitLocalLinkPlayer(void);
static void VBlankCB_LinkError(void);
static void CB2_LinkTest(void);
static void LinkCB_SendHeldKeys(void);
static void ResetBlockSend(void);
static bool32 InitBlockSend(const void *, size_t);
static void LinkCB_BlockSendBegin(void);
static void LinkCB_BlockSend(void);
static void LinkCB_BlockSendEnd(void);
static void SetBlockReceivedFlag(u8);
static u16 LinkTestCalcBlockChecksum(const u16 *, u16);
static void LinkTest_PrintHex(u32, u8, u8, u8);
static void LinkCB_RequestPlayerDataExchange(void);
static void Task_PrintTestData(u8);

static void LinkCB_ReadyCloseLink(void);
static void LinkCB_WaitCloseLink(void);
static void LinkCB_ReadyCloseLinkWithJP(void);
static void LinkCB_WaitCloseLinkWithJP(void);
static void LinkCB_Standby(void);
static void LinkCB_StandbyForAll(void);

static void CB2_PrintErrorMessage(void);
static bool8 IsSioMultiMaster(void);
static void DisableSerial(void);
static void EnableSerial(void);

static const u16 sWirelessLinkDisplayPal[] = INCGFX_U16("graphics/link/wireless_display.png", ".gbapal");
static const u32 sWirelessLinkDisplayGfx[] = INCGFX_U32("graphics/link/wireless_display.png", ".4bpp.smol");
static const u32 sWirelessLinkDisplayTilemap[] = INCGFX_U32("graphics/link/wireless_display.bin", ".smolTM");
static const u16 sLinkTestDigitsPal[] = INCGFX_U16("graphics/link/test_digits.png", ".gbapal");
static const u16 sLinkTestDigitsGfx[] = INCGFX_U16("graphics/link/test_digits.png", ".4bpp");
static const u8 sUnusedTransparentWhite[] = _("{BACKGROUND TRANSPARENT}{ACCENT TRANSPARENT}{COLOR WHITE}");
static const u16 sCommErrorBg_Gfx[] = INCGFX_U16("graphics/link/comm_error_bg.png", ".4bpp");
static const struct BlockRequest sBlockRequests[] = {
    [BLOCK_REQ_SIZE_NONE] = {gBlockSendBuffer, 200},
    [BLOCK_REQ_SIZE_200]  = {gBlockSendBuffer, 200},
    [BLOCK_REQ_SIZE_100]  = {gBlockSendBuffer, 100},
    [BLOCK_REQ_SIZE_220]  = {gBlockSendBuffer, 220},
    [BLOCK_REQ_SIZE_40]   = {gBlockSendBuffer,  40}
};
static const u8 sBGControlRegs[] = {
    REG_OFFSET_BG0CNT,
    REG_OFFSET_BG1CNT,
    REG_OFFSET_BG2CNT,
    REG_OFFSET_BG3CNT
};
static const char sASCIIGameFreakInc[] = "GameFreak inc.";
static const char sASCIITestPrint[] = "TEST PRINT\nP0\nP1\nP2\nP3";
static const struct BgTemplate sLinkErrorBgTemplates[] = {
    {
        .bg = 0,
        .charBaseIndex = 2,
        .mapBaseIndex = 31,
        .priority = 0
    }, {
        .bg = 1,
        .charBaseIndex = 0,
        .mapBaseIndex = 8,
        .priority = 1
    }
};

static const struct WindowTemplate sLinkErrorWindowTemplates[] = {
    [WIN_LINK_ERROR_TOP] = {
        .bg = 0,
        .tilemapLeft = 0,
        .tilemapTop = 0,
        .width = DISPLAY_TILE_WIDTH,
        .height = 5,
        .paletteNum = 15,
        .baseBlock = 0x002
    },
    [WIN_LINK_ERROR_MID] = {
        .bg = 0,
        .tilemapLeft = 0,
        .tilemapTop = 6,
        .width = DISPLAY_TILE_WIDTH,
        .height = 7,
        .paletteNum = 15,
        .baseBlock = 0x098
    },
    [WIN_LINK_ERROR_BOTTOM] = {
        .bg = 0,
        .tilemapLeft = 0,
        .tilemapTop = 13,
        .width = DISPLAY_TILE_WIDTH,
        .height = 7,
        .paletteNum = 15,
        .baseBlock = 0x16A
    }, DUMMY_WIN_TEMPLATE
};

static const u8 sTextColors[] = { TEXT_COLOR_TRANSPARENT, TEXT_COLOR_WHITE, TEXT_COLOR_DARK_GRAY };
static const u8 sUnusedData[] = {0x00, 0xFF, 0xFE, 0xFF, 0x00};

// The Wireless Adapter no longer exists in this fork -- never detected.
bool8 IsWirelessAdapterConnected(void)
{
    return FALSE;
}

// Must run before interrupts are enabled -- see gSTWIStatus's declaration above.
void InitSTWIStatusDummy(void)
{
    gSTWIStatus = sDummySTWIStatus;
}

void Task_DestroySelf(u8 taskId)
{
    DestroyTask(taskId);
}

static void InitLinkTestBG(u8 paletteNum, u8 bgNum, u8 screenBaseBlock, u8 charBaseBlock, u16 baseChar)
{
    LoadPalette(sLinkTestDigitsPal, BG_PLTT_ID(paletteNum), PLTT_SIZE_4BPP);
    DmaCopy16(3, sLinkTestDigitsGfx, (u16 *)BG_CHAR_ADDR(charBaseBlock) + (16 * baseChar), sizeof sLinkTestDigitsGfx);
    gLinkTestBGInfo.screenBaseBlock = screenBaseBlock;
    gLinkTestBGInfo.paletteNum = paletteNum;
    gLinkTestBGInfo.baseChar = baseChar;
    switch (bgNum)
    {
    case 1:
        SetGpuReg(REG_OFFSET_BG1CNT, BGCNT_SCREENBASE(screenBaseBlock) | BGCNT_PRIORITY(1) | BGCNT_CHARBASE(charBaseBlock));
        break;
    case 2:
        SetGpuReg(REG_OFFSET_BG2CNT, BGCNT_SCREENBASE(screenBaseBlock) | BGCNT_PRIORITY(1) | BGCNT_CHARBASE(charBaseBlock));
        break;
    case 3:
        SetGpuReg(REG_OFFSET_BG3CNT, BGCNT_SCREENBASE(screenBaseBlock) | BGCNT_PRIORITY(1) | BGCNT_CHARBASE(charBaseBlock));
        break;
    }
    SetGpuReg(REG_OFFSET_BG0HOFS + bgNum * 4, 0);
    SetGpuReg(REG_OFFSET_BG0VOFS + bgNum * 4, 0);
}

static void UNUSED LoadLinkTestBgGfx(u8 paletteNum, u8 bgNum, u8 screenBaseBlock, u8 charBaseBlock)
{
    LoadPalette(sLinkTestDigitsPal, BG_PLTT_ID(paletteNum), PLTT_SIZE_4BPP);
    DmaCopy16(3, sLinkTestDigitsGfx, (u16 *)BG_CHAR_ADDR(charBaseBlock), sizeof sLinkTestDigitsGfx);
    gLinkTestBGInfo.screenBaseBlock = screenBaseBlock;
    gLinkTestBGInfo.paletteNum = paletteNum;
    gLinkTestBGInfo.baseChar = 0;
    SetGpuReg(sBGControlRegs[bgNum], BGCNT_SCREENBASE(screenBaseBlock) | BGCNT_CHARBASE(charBaseBlock));
}

static void UNUSED LinkTestScreen(void)
{
    int i;

    ResetSpriteData();
    FreeAllSpritePalettes();
    ResetTasks();
    SetVBlankCallback(VBlankCB_LinkError);
    ResetBlockSend();
    gLinkType = LINKTYPE_TRADE;
    OpenLink();
    SeedRng(gMain.vblankCounter2);
    for (i = 0; i < TRAINER_ID_LENGTH; i++)
        gSaveBlock2Ptr->playerTrainerId[i] = Random() % 256;

    InitLinkTestBG(0, 2, 4, 0, 0);
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_0 | DISPCNT_OBJ_1D_MAP | DISPCNT_BG0_ON | DISPCNT_BG2_ON | DISPCNT_OBJ_ON);
    CreateTask(Task_DestroySelf, 0);
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
    InitLocalLinkPlayer();
    CreateTask(Task_PrintTestData, 0);
    SetMainCallback2(CB2_LinkTest);
}

void SetLocalLinkPlayerId(u8 playerId)
{
    gLocalLinkPlayer.id = playerId;
}

static void InitLocalLinkPlayer(void)
{
    gLocalLinkPlayer.trainerId = READ_OTID_FROM_SAVE;
    StringCopy(gLocalLinkPlayer.name, gSaveBlock2Ptr->playerName);
    gLocalLinkPlayer.gender = gSaveBlock2Ptr->playerGender;
    gLocalLinkPlayer.linkType = gLinkType;
    gLocalLinkPlayer.language = gGameLanguage;
    gLocalLinkPlayer.version = gGameVersion + 0x4000;
    gLocalLinkPlayer.lp_field_2 = 0x8000;
    gLocalLinkPlayer.progressFlags = IsNationalPokedexEnabled();
    if (FlagGet(FLAG_IS_CHAMPION))
    {
        gLocalLinkPlayer.progressFlags |= 0x10;
    }
}

static void VBlankCB_LinkError(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

static void InitLink(void)
{
    int i;

    for (i = 0; i < CMD_LENGTH; i++)
        gSendCmd[i] = LINKCMD_NONE;

    EnableSerial();
}

static void Task_TriggerHandshake(u8 taskId)
{
    if (++gTasks[taskId].data[0] == 5)
    {
        gShouldAdvanceLinkState = 1;
        DestroyTask(taskId);
    }
}

void OpenLink(void)
{
    int i;

    ResetSerial();
    InitLink();
    gLinkCallback = LinkCB_RequestPlayerDataExchange;
    gLinkVSyncDisabled = FALSE;
    gLinkErrorOccurred = FALSE;
    gSuppressLinkErrorMessage = FALSE;
    ResetBlockReceivedFlags();
    ResetBlockSend();
    gReadyCloseLinkType = 0;
    CreateTask(Task_TriggerHandshake, 2);
    gReceivedRemoteLinkPlayers = 0;
    for (i = 0; i < MAX_LINK_PLAYERS; i++)
    {
        gRemoteLinkPlayersNotReceived[i] = TRUE;
        gReadyToCloseLink[i] = FALSE;
        gReadyToExitStandby[i] = FALSE;
    }
}

void CloseLink(void)
{
    gReceivedRemoteLinkPlayers = FALSE;
    DisableSerial();
}

static void TestBlockTransfer(u8 nothing, u8 is, u8 used)
{
    u8 i;
    u8 status;

    if (sLinkTestLastBlockSendPos != sBlockSend.pos)
    {
        LinkTest_PrintHex(sBlockSend.pos, 2, 3, 2);
        sLinkTestLastBlockSendPos = sBlockSend.pos;
    }
    for (i = 0; i < MAX_LINK_PLAYERS; i++)
    {
        if (sLinkTestLastBlockRecvPos[i] != sBlockRecv[i].pos)
        {
            LinkTest_PrintHex(sBlockRecv[i].pos, 2, i + 4, 2);
            sLinkTestLastBlockRecvPos[i] = sBlockRecv[i].pos;
        }
    }
    status = GetBlockReceivedStatus();
    if (status == 0xF) // 0b1111
    {
        for (i = 0; i < MAX_LINK_PLAYERS; i++)
        {
            if ((status >> i) & 1)
            {
                gLinkTestBlockChecksums[i] = LinkTestCalcBlockChecksum(gBlockRecvBuffer[i], sBlockRecv[i].size);
                ResetBlockReceivedFlag(i);
                if (gLinkTestBlockChecksums[i] != 0x0342)
                    sLinkTestDebugValuesEnabled = FALSE;
            }
        }
    }
}

static void LinkTestProcessKeyInput(void)
{
    if (JOY_NEW(A_BUTTON))
    {
        gShouldAdvanceLinkState = 1;
    }
    if (JOY_HELD(B_BUTTON))
    {
        InitBlockSend(gHeap + 0x4000, 0x00002004);
    }
    if (JOY_NEW(L_BUTTON))
    {
        BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB(2, 0, 0));
    }
    if (JOY_NEW(START_BUTTON))
    {
        SetSuppressLinkErrorMessage(TRUE);
    }
    if (JOY_NEW(R_BUTTON))
    {
        TrySavingData(SAVE_LINK);
    }
    if (JOY_NEW(SELECT_BUTTON))
    {
        SetCloseLinkCallback();
    }
    if (sLinkTestDebugValuesEnabled)
    {
        SetLinkDebugValues(gMain.vblankCounter2, gLinkCallback ? gLinkVSyncDisabled : gLinkVSyncDisabled | 0x10);
    }
}

static void CB2_LinkTest(void)
{
    LinkTestProcessKeyInput();
    TestBlockTransfer(1, 1, 0);
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

// No real link partner can ever connect (see LinkMain1 below), so the packet-processing loop
// that used to run here on an established connection can never fire. Kept as a stub since
// callers (src/main.c) still poll its return value every frame.
u16 LinkMain2(const u16 *heldKeys)
{
    return 0;
}

static void BuildSendCmd(u16 command)
{
    switch (command)
    {
    case LINKCMD_SEND_LINK_TYPE:
        gSendCmd[0] = LINKCMD_SEND_LINK_TYPE;
        gSendCmd[1] = gLinkType;
        break;
    case LINKCMD_READY_EXIT_STANDBY:
        gSendCmd[0] = LINKCMD_READY_EXIT_STANDBY;
        break;
    case LINKCMD_BLENDER_SEND_KEYS:
        gSendCmd[0] = LINKCMD_BLENDER_SEND_KEYS;
        gSendCmd[1] = gMain.heldKeys;
        break;
    case LINKCMD_DUMMY_1:
        gSendCmd[0] = LINKCMD_DUMMY_1;
        break;
    case LINKCMD_SEND_EMPTY:
        gSendCmd[0] = LINKCMD_SEND_EMPTY;
        gSendCmd[1] = 0;
        break;
    case LINKCMD_SEND_0xEE:
    {
        u8 i;
        gSendCmd[0] = LINKCMD_SEND_0xEE;
        for (i = 0; i < 5; i++)
            gSendCmd[i + 1] = 0xEE;
        break;
    }
    case LINKCMD_INIT_BLOCK:
        gSendCmd[0] = LINKCMD_INIT_BLOCK;
        gSendCmd[1] = sBlockSend.size;
        gSendCmd[2] = sBlockSend.multiplayerId + 0x80;
        break;
    case LINKCMD_BLENDER_NO_PBLOCK_SPACE:
        gSendCmd[0] = LINKCMD_BLENDER_NO_PBLOCK_SPACE;
        break;
    case LINKCMD_SEND_ITEM:
        gSendCmd[0] = LINKCMD_SEND_ITEM;
        gSendCmd[1] = gSpecialVar_ItemId;
        break;
    case LINKCMD_SEND_BLOCK_REQ:
        gSendCmd[0] = LINKCMD_SEND_BLOCK_REQ;
        gSendCmd[1] = gBlockRequestType;
        break;
    case LINKCMD_READY_CLOSE_LINK:
        gSendCmd[0] = LINKCMD_READY_CLOSE_LINK;
        gSendCmd[1] = gReadyCloseLinkType;
        break;
    case LINKCMD_DUMMY_2:
        gSendCmd[0] = LINKCMD_DUMMY_2;
        break;
    case LINKCMD_SEND_HELD_KEYS:
        if (gHeldKeyCodeToSend == 0 || gLinkTransferringData)
            break;

        gSendCmd[0] = LINKCMD_SEND_HELD_KEYS;
        gSendCmd[1] = gHeldKeyCodeToSend;
        break;
    }
}

void StartSendingKeysToLink(void)
{
    gLinkCallback = LinkCB_SendHeldKeys;
}

bool32 IsSendingKeysToLink(void)
{
    if (gLinkCallback == LinkCB_SendHeldKeys)
        return TRUE;

    return FALSE;
}

static void LinkCB_SendHeldKeys(void)
{
    if (gReceivedRemoteLinkPlayers == TRUE)
        BuildSendCmd(LINKCMD_SEND_HELD_KEYS);
}

void ClearLinkCallback(void)
{
    gLinkCallback = NULL;
}

void ClearLinkCallback_2(void)
{
    gLinkCallback = NULL;
}

u8 GetLinkPlayerCount(void)
{
    return EXTRACT_PLAYER_COUNT(gLinkStatus);
}

static int AreAnyLinkPlayersUsingVersions(enum GameVersion version1, enum GameVersion version2)
{
    int i;
    u8 nPlayers;

    nPlayers = GetLinkPlayerCount();
    for (i = 0; i < nPlayers; i++)
    {
        if ((gLinkPlayers[i].version & 0xFF) == version1
         || (gLinkPlayers[i].version & 0xFF) == version2)
            return 1;
    }
    return -1;
}

u32 LinkDummy_Return2(void)
{
    return 2;
}

static bool32 UNUSED IsFullLinkGroupWithNoRS(void)
{
    if (GetLinkPlayerCount() != MAX_LINK_PLAYERS || AreAnyLinkPlayersUsingVersions(VERSION_RUBY, VERSION_SAPPHIRE) < 0)
    {
        return FALSE;
    }
    return TRUE;
}

bool32 Link_AnyPartnersPlayingRubyOrSapphire(void)
{
    return (AreAnyLinkPlayersUsingVersions(VERSION_RUBY, VERSION_SAPPHIRE) >= 0);
}

bool32 Link_AnyPartnersPlayingFRLG_JP(void)
{
    int i = AreAnyLinkPlayersUsingVersions(VERSION_FIRE_RED, VERSION_LEAF_GREEN);
    return (i >= 0 && gLinkPlayers[i].language == LANGUAGE_JAPANESE);
}

void OpenLinkTimed(void)
{
    sPlayerDataExchangeStatus = EXCHANGE_NOT_STARTED;
    sTimeOutCounter = 0;
    OpenLink();
}

u8 GetLinkPlayerDataExchangeStatusTimed(int minPlayers, int maxPlayers)
{
    int i;
    int count;
    u32 index;
    u8 numPlayers;
    u32 linkType1;
    u32 linkType2;

    count = 0;
    if (gReceivedRemoteLinkPlayers == TRUE)
    {
        numPlayers = GetLinkPlayerCount_2();
        if (minPlayers > numPlayers || numPlayers > maxPlayers)
        {
            sPlayerDataExchangeStatus = EXCHANGE_WRONG_NUM_PLAYERS;
            return sPlayerDataExchangeStatus;
        }
        else
        {
            if (GetLinkPlayerCount() == 0)
            {
                gLinkErrorOccurred = TRUE;
                CloseLink();
            }
            for (i = 0, index = 0; i < GetLinkPlayerCount(); index++, i++)
            {
                if (gLinkPlayers[index].linkType == gLinkPlayers[0].linkType)
                {
                    count++;
                }
            }
            if (count == GetLinkPlayerCount())
            {
                if (gLinkPlayers[0].linkType == LINKTYPE_TRADE_SETUP)
                {
                    switch (GetGameProgressForLinkTrade())
                    {
                    case TRADE_PLAYER_NOT_READY:
                        sPlayerDataExchangeStatus = EXCHANGE_PLAYER_NOT_READY;
                        break;
                    case TRADE_PARTNER_NOT_READY:
                        sPlayerDataExchangeStatus = EXCHANGE_PARTNER_NOT_READY;
                        break;
                    case TRADE_BOTH_PLAYERS_READY:
                        sPlayerDataExchangeStatus = EXCHANGE_COMPLETE;
                        break;
                    }
                }
                else
                {
                    sPlayerDataExchangeStatus = EXCHANGE_COMPLETE;
                }
            }
            else
            {
                sPlayerDataExchangeStatus = EXCHANGE_DIFF_SELECTIONS;
                linkType1 = gLinkPlayers[GetMultiplayerId()].linkType;
                linkType2 = gLinkPlayers[GetMultiplayerId() ^ 1].linkType;
                if ((linkType1 == LINKTYPE_BATTLE_TOWER_50 && linkType2 == LINKTYPE_BATTLE_TOWER_OPEN)
                 || (linkType1 == LINKTYPE_BATTLE_TOWER_OPEN && linkType2 == LINKTYPE_BATTLE_TOWER_50))
                {
                    // 3 below indicates partner made different level mode selection
                    // See BattleFrontier_BattleTowerLobby_EventScript_AbortLinkDifferentSelections
                    gSpecialVar_0x8005 = 3;
                }
            }
        }
    }
    else if (++sTimeOutCounter > 600)
    {
        sPlayerDataExchangeStatus = EXCHANGE_TIMED_OUT;
    }
    return sPlayerDataExchangeStatus;
}

bool8 IsLinkPlayerDataExchangeComplete(void)
{
    u8 i;
    u8 count;
    bool8 retval;

    count = 0;
    for (i = 0; i < GetLinkPlayerCount(); i++)
    {
        if (gLinkPlayers[i].linkType == gLinkPlayers[0].linkType)
            count++;
    }
    if (count == GetLinkPlayerCount())
    {
        retval = TRUE;
        sPlayerDataExchangeStatus = EXCHANGE_COMPLETE;
    }
    else
    {
        retval = FALSE;
        sPlayerDataExchangeStatus = EXCHANGE_DIFF_SELECTIONS;
    }
    return retval;
}

u32 GetLinkPlayerTrainerId(u8 who)
{
    return gLinkPlayers[who].trainerId;
}

void ResetLinkPlayers(void)
{
    int i;

    for (i = 0; i <= MAX_LINK_PLAYERS; i++)
        gLinkPlayers[i] = (struct LinkPlayer){};
}

static void ResetBlockSend(void)
{
    sBlockSend.active = FALSE;
    sBlockSend.pos = 0;
    sBlockSend.size = 0;
    sBlockSend.src = NULL;
}

static bool32 InitBlockSend(const void *src, size_t size)
{
    if (sBlockSend.active)
    {
        return FALSE;
    }
    sBlockSend.multiplayerId = GetMultiplayerId();
    sBlockSend.active = TRUE;
    sBlockSend.size = size;
    sBlockSend.pos = 0;
    if (size > BLOCK_BUFFER_SIZE)
    {
        sBlockSend.src = src;
    }
    else
    {
        if (src != gBlockSendBuffer)
            memcpy(gBlockSendBuffer, src, size);

        sBlockSend.src = gBlockSendBuffer;
    }
    BuildSendCmd(LINKCMD_INIT_BLOCK);
    gLinkCallback = LinkCB_BlockSendBegin;
    sBlockSendDelayCounter = 0;
    return TRUE;
}

static void LinkCB_BlockSendBegin(void)
{
    if (++sBlockSendDelayCounter > 2)
        gLinkCallback = LinkCB_BlockSend;
}

static void LinkCB_BlockSend(void)
{
    int i;
    const u8 *src;

    src = sBlockSend.src;
    gSendCmd[0] = LINKCMD_CONT_BLOCK;
    for (i = 0; i < CMD_LENGTH - 1; i++)
    {
        gSendCmd[i + 1] = (src[sBlockSend.pos + i * 2 + 1] << 8) | src[sBlockSend.pos + i * 2];
    }
    sBlockSend.pos += 14;
    if (sBlockSend.size <= sBlockSend.pos)
    {
        sBlockSend.active = FALSE;
        gLinkCallback = LinkCB_BlockSendEnd;
    }
}

static void LinkCB_BlockSendEnd(void)
{
    gLinkCallback = NULL;
}

static void LinkCB_BerryBlenderSendHeldKeys(void)
{
    GetMultiplayerId();
    BuildSendCmd(LINKCMD_BLENDER_SEND_KEYS);
    gBerryBlenderKeySendAttempts++;
}

void SetBerryBlenderLinkCallback(void)
{
    gBerryBlenderKeySendAttempts = 0;
    gLinkCallback = LinkCB_BerryBlenderSendHeldKeys;
}

static u32 UNUSED GetBerryBlenderKeySendAttempts(void)
{
    return gBerryBlenderKeySendAttempts;
}

static void UNUSED SendBerryBlenderNoSpaceForPokeblocks(void)
{
    BuildSendCmd(LINKCMD_BLENDER_NO_PBLOCK_SPACE);
}

u8 GetMultiplayerId(void)
{
    return SIO_MULTI_CNT->id;
}

u8 BitmaskAllOtherLinkPlayers(void)
{
    u8 mpId;

    mpId = GetMultiplayerId();
    return ((1 << MAX_LINK_PLAYERS) - 1) ^ (1 << mpId);
}

bool8 SendBlock(u8 unused, const void *src, u16 size)
{
    return InitBlockSend(src, size);
}

bool8 SendBlockRequest(u8 blockReqType)
{
    if (gLinkCallback == NULL)
    {
        gBlockRequestType = blockReqType;
        BuildSendCmd(LINKCMD_SEND_BLOCK_REQ);
        return TRUE;
    }
    return FALSE;
}

bool8 IsLinkTaskFinished(void)
{
    return gLinkCallback == NULL;
}

u8 GetBlockReceivedStatus(void)
{
    return (gBlockReceivedStatus[3] << 3) | (gBlockReceivedStatus[2] << 2) | (gBlockReceivedStatus[1] << 1) | (gBlockReceivedStatus[0] << 0);
}

static void SetBlockReceivedFlag(u8 who)
{
    gBlockReceivedStatus[who] = TRUE;
}

void ResetBlockReceivedFlags(void)
{
    int i;

    for (i = 0; i < MAX_LINK_PLAYERS; i++)
        gBlockReceivedStatus[i] = FALSE;
}

void ResetBlockReceivedFlag(u8 who)
{
    if (gBlockReceivedStatus[who])
    {
        gBlockReceivedStatus[who] = FALSE;
    }
}

void CheckShouldAdvanceLinkState(void)
{
    if ((gLinkStatus & LINK_STAT_MASTER) && EXTRACT_PLAYER_COUNT(gLinkStatus) > 1)
        gShouldAdvanceLinkState = 1;
}

static u16 LinkTestCalcBlockChecksum(const u16 *src, u16 size)
{
    u16 chksum;
    u16 i;

    chksum = 0;
    for (i = 0; i < size / 2; i++)
        chksum += src[i];

    return chksum;
}

static void LinkTest_PrintNumChar(char val, u8 x, u8 y)
{
    u16 *vAddr;

    vAddr = (u16 *)BG_SCREEN_ADDR(gLinkTestBGInfo.screenBaseBlock);
    vAddr[y * 32 + x] = (gLinkTestBGInfo.paletteNum << 12) | (val + 1 + gLinkTestBGInfo.baseChar);
}

static void LinkTest_PrintChar(char val, u8 x, u8 y)
{
    u16 *vAddr;

    vAddr = (u16 *)BG_SCREEN_ADDR(gLinkTestBGInfo.screenBaseBlock);
    vAddr[y * 32 + x] = (gLinkTestBGInfo.paletteNum << 12) | (val + gLinkTestBGInfo.baseChar);
}

static void LinkTest_PrintHex(u32 num, u8 x, u8 y, u8 length)
{
    char buff[16];
    int i;

    for (i = 0; i < length; i++)
    {
        buff[i] = num & 0xF;
        num >>= 4;
    }
    for (i = length - 1; i >= 0; i--)
    {
        LinkTest_PrintNumChar(buff[i], x, y);
        x++;
    }
}

static void UNUSED LinkTest_PrintInt(int num, u8 x, u8 y, u8 length)
{
    char buff[16];
    int negX;
    int i;

    negX = -1;
    if (num < 0)
    {
        negX = x;
        num = -num;
    }
    for (i = 0; i < length; i++)
    {
        buff[i] = num % 10;
        num /= 10;
    }
    for (i = length - 1; i >= 0; i--)
    {
        LinkTest_PrintNumChar(buff[i], x, y);
        x++;
    }

    if (negX != -1)
        LinkTest_PrintNumChar(*"\n", negX, y);
}

static void LinkTest_PrintString(const char *str, u8 x, u8 y)
{
    int xOffset;
    int i;
    int yOffset;

    yOffset = 0;
    xOffset = 0;
    for (i = 0; str[i] != 0; str++)
    {
        if (str[i] == *"\n")
        {
            yOffset++;
            xOffset = 0;
        }
        else
        {
            LinkTest_PrintChar(str[i], x + xOffset, y + yOffset);
            xOffset++;
        }
    }
}

static void LinkCB_RequestPlayerDataExchange(void)
{
    if (gLinkStatus & LINK_STAT_MASTER)
    {
        BuildSendCmd(LINKCMD_SEND_LINK_TYPE);
    }
    gLinkCallback = NULL;
}

static void Task_PrintTestData(u8 taskId)
{
    char testTitle[32];
    int i;

    strcpy(testTitle, sASCIITestPrint);
    LinkTest_PrintString(testTitle, 5, 2);
    LinkTest_PrintHex(gShouldAdvanceLinkState, 2, 1, 2);
    LinkTest_PrintHex(gLinkStatus, 15, 1, 8);
    LinkTest_PrintHex(gLink.state, 2, 10, 2);
    LinkTest_PrintHex(EXTRACT_PLAYER_COUNT(gLinkStatus), 15, 10, 2);
    LinkTest_PrintHex(GetMultiplayerId(), 15, 12, 2);
    LinkTest_PrintHex(gLastSendQueueCount, 25, 1, 2);
    LinkTest_PrintHex(gLastRecvQueueCount, 25, 2, 2);
    LinkTest_PrintHex(GetBlockReceivedStatus(), 15, 5, 2);
    LinkTest_PrintHex(gLinkDebugSeed, 2, 12, 8);
    LinkTest_PrintHex(gLinkDebugFlags, 2, 13, 8);
    LinkTest_PrintHex(GetSioMultiSI(), 25, 5, 1);
    LinkTest_PrintHex(IsSioMultiMaster(), 25, 6, 1);
    LinkTest_PrintHex(IsLinkConnectionEstablished(), 25, 7, 1);
    LinkTest_PrintHex(HasLinkErrorOccurred(), 25, 8, 1);

    for (i = 0; i < MAX_LINK_PLAYERS; i++)
        LinkTest_PrintHex(gLinkTestBlockChecksums[i], 10, 4 + i, 4);
}

void SetLinkDebugValues(u32 seed, u32 flags)
{
    gLinkDebugSeed = seed;
    gLinkDebugFlags = flags;
}

u8 GetSavedLinkPlayerCountAsBitFlags(void)
{
    int i;
    u8 flags;

    flags = 0;
    for (i = 0; i < gSavedLinkPlayerCount; i++)
        flags |= (1 << i);

    return flags;
}

u8 GetLinkPlayerCountAsBitFlags(void)
{
    int i;
    u8 flags;

    flags = 0;
    for (i = 0; i < GetLinkPlayerCount(); i++)
        flags |= (1 << i);

    return flags;
}

void SaveLinkPlayers(u8 playerCount)
{
    int i;

    gSavedLinkPlayerCount = playerCount;
    gSavedMultiplayerId = GetMultiplayerId();
    for (i = 0; i < MAX_RFU_PLAYERS; i++)
        sSavedLinkPlayers[i] = gLinkPlayers[i];
}

// The number of players when trading began. This is frequently compared against the
// current number of connected players to check if anyone dropped out.
u8 GetSavedPlayerCount(void)
{
    return gSavedLinkPlayerCount;
}

static u8 UNUSED GetSavedMultiplayerId(void)
{
    return gSavedMultiplayerId;
}

bool8 DoesLinkPlayerCountMatchSaved(void)
{
    int i;
    u32 count = 0;

    for (i = 0; i < gSavedLinkPlayerCount; i++)
    {
        if (gLinkPlayers[i].trainerId == sSavedLinkPlayers[i].trainerId)
        {
            if (gLinkType == LINKTYPE_BATTLE_TOWER)
            {
                if (gLinkType == gLinkPlayers[i].linkType)
                    count++;
            }
            else
            {
                count++;
            }
        }
    }
    if (count == gSavedLinkPlayerCount)
    {
        if (GetLinkPlayerCount_2() == gSavedLinkPlayerCount)
        {
            return TRUE;
        }
    }
    return FALSE;
}

void ClearSavedLinkPlayers(void)
{
    // The CpuSet loop below is incorrectly writing to NULL
    // instead of sSavedLinkPlayers.
    // Additionally it's using the wrong array size.
#ifdef UBFIX
    memset(sSavedLinkPlayers, 0, sizeof(sSavedLinkPlayers));
#else
    int i;
    for (i = 0; i < MAX_LINK_PLAYERS; i++)
        CpuSet(&sSavedLinkPlayers[i], NULL, sizeof(struct LinkPlayer));
#endif
}

void CheckLinkPlayersMatchSaved(void)
{
    u8 i;

    for (i = 0; i < gSavedLinkPlayerCount; i++)
    {
        if (sSavedLinkPlayers[i].trainerId != gLinkPlayers[i].trainerId
         || StringCompare(sSavedLinkPlayers[i].name, gLinkPlayers[i].name) != 0)
        {
            gLinkErrorOccurred = TRUE;
            CloseLink();
            SetMainCallback2(CB2_LinkError);
        }
    }
}

void ResetLinkPlayerCount(void)
{
    gSavedLinkPlayerCount = 0;
    gSavedMultiplayerId = 0;
}

u8 GetLinkPlayerCount_2(void)
{
    return EXTRACT_PLAYER_COUNT(gLinkStatus);
}

bool8 IsLinkMaster(void)
{
    return EXTRACT_MASTER(gLinkStatus);
}

void SetCloseLinkCallbackAndType(u16 type)
{
    if (gLinkCallback == NULL)
    {
        gLinkCallback = LinkCB_ReadyCloseLink;
        gReadyCloseLinkType = type;
    }
}

void SetCloseLinkCallback(void)
{
    if (gLinkCallback != NULL)
    {
        sReadyCloseLinkAttempts++;
    }
    else
    {
        gLinkCallback = LinkCB_ReadyCloseLink;
        gReadyCloseLinkType = 0;
    }
}

static void LinkCB_ReadyCloseLink(void)
{
    if (gLastRecvQueueCount == 0)
    {
        BuildSendCmd(LINKCMD_READY_CLOSE_LINK);
        gLinkCallback = LinkCB_WaitCloseLink;
    }
}

static void LinkCB_WaitCloseLink(void)
{
    int i;
    unsigned count;

    // Wait for all players to be ready
    u8 linkPlayerCount = GetLinkPlayerCount();
    count = 0;
    for (i = 0; i < linkPlayerCount; i++)
    {
        if (gReadyToCloseLink[i])
            count++;
    }

    if (count == linkPlayerCount)
    {
        // All ready, close link
        gBattleTypeFlags &= ~BATTLE_TYPE_LINK_IN_BATTLE;
        gLinkVSyncDisabled = TRUE;
        CloseLink();
        gLinkCallback = NULL;
    }
}

// Used instead of SetCloseLinkCallback when disconnecting from an attempt to link with a foreign game
void SetCloseLinkCallbackHandleJP(void)
{
    if (gLinkCallback != NULL)
    {
        sReadyCloseLinkAttempts++;
    }
    else
    {
        gLinkCallback = LinkCB_ReadyCloseLinkWithJP;
        gReadyCloseLinkType = 0;
    }
}

static void LinkCB_ReadyCloseLinkWithJP(void)
{
    if (gLastRecvQueueCount == 0)
    {
        BuildSendCmd(LINKCMD_READY_CLOSE_LINK);
        gLinkCallback = LinkCB_WaitCloseLinkWithJP;
    }
}

static void LinkCB_WaitCloseLinkWithJP(void)
{
    int i;
    unsigned count;
    u8 linkPlayerCount;

    linkPlayerCount = GetLinkPlayerCount();
    count = 0;

    // Wait for all non-foreign players to be ready
    for (i = 0; i < linkPlayerCount; i++)
    {
        // Rather than communicate with the foreign game
        // just assume they're ready to disconnect
        if (gLinkPlayers[i].language == LANGUAGE_JAPANESE)
            count++;
        else if (gReadyToCloseLink[i])
            count++;
    }

    if (count == linkPlayerCount)
    {
        // All ready, close link
        gBattleTypeFlags &= ~BATTLE_TYPE_LINK_IN_BATTLE;
        gLinkVSyncDisabled = TRUE;
        CloseLink();
        gLinkCallback = NULL;
    }
}

void SetLinkStandbyCallback(void)
{
    if (gLinkCallback == NULL)
        gLinkCallback = LinkCB_Standby;
}

static void LinkCB_Standby(void)
{
    if (gLastRecvQueueCount == 0)
    {
        BuildSendCmd(LINKCMD_READY_EXIT_STANDBY);
        gLinkCallback = LinkCB_StandbyForAll;
    }
}

static void LinkCB_StandbyForAll(void)
{
    u8 i;
    u8 linkPlayerCount = GetLinkPlayerCount();
    for (i = 0; i < linkPlayerCount; i++)
    {
        if (!gReadyToExitStandby[i])
            break;
    }

    // If true, all players ready to exit standby
    if (i == linkPlayerCount)
    {
        for (i = 0; i < MAX_LINK_PLAYERS; i++)
            gReadyToExitStandby[i] = FALSE;

        gLinkCallback = NULL;
    }
}

void SetLinkErrorBuffer(u32 status, u8 lastSendQueueCount, u8 lastRecvQueueCount, bool8 disconnected)
{
    sLinkErrorBuffer.status = status;
    sLinkErrorBuffer.lastSendQueueCount = lastSendQueueCount;
    sLinkErrorBuffer.lastRecvQueueCount = lastRecvQueueCount;
    sLinkErrorBuffer.disconnected = disconnected;
}

void CB2_LinkError(void)
{
    u8 *tilemapBuffer;

    SetGpuReg(REG_OFFSET_DISPCNT, 0);
    m4aMPlayStop(&gMPlayInfo_SE1);
    m4aMPlayStop(&gMPlayInfo_SE2);
    m4aMPlayStop(&gMPlayInfo_SE3);
    InitHeap(gHeap, HEAP_SIZE);
    ResetSpriteData();
    FreeAllSpritePalettes();
    ResetPaletteFadeControl();
    SetBackdropFromColor(RGB_BLACK);
    ResetTasks();
    ScanlineEffect_Stop();
    SetVBlankCallback(VBlankCB_LinkError);
    ResetBgsAndClearDma3BusyFlags(0);
    InitBgsFromTemplates(0, sLinkErrorBgTemplates, ARRAY_COUNT(sLinkErrorBgTemplates));
    tilemapBuffer = Alloc(BG_SCREEN_SIZE);
    SetBgTilemapBuffer(1, tilemapBuffer);
    if (InitWindows(sLinkErrorWindowTemplates))
    {
        DeactivateAllTextPrinters();
        ResetTempTileDataBuffers();
        SetGpuReg(REG_OFFSET_BLDCNT, 0);
        SetGpuReg(REG_OFFSET_BLDALPHA, 0);
        SetGpuReg(REG_OFFSET_BG0HOFS, 0);
        SetGpuReg(REG_OFFSET_BG0VOFS, 0);
        SetGpuReg(REG_OFFSET_BG1HOFS, 0);
        SetGpuReg(REG_OFFSET_BG1VOFS, 0);
        ClearGpuRegBits(REG_OFFSET_DISPCNT, DISPCNT_WIN0_ON | DISPCNT_WIN1_ON | DISPCNT_OBJWIN_ON);
        LoadPalette(gStandardMenuPalette, BG_PLTT_ID(15), PLTT_SIZE_4BPP);
        gSoftResetDisabled = FALSE;
        CreateTask(Task_DestroySelf, 0);
        StopMapMusic();
        gMain.callback1 = NULL;
        RunTasks();
        AnimateSprites();
        BuildOamBuffer();
        UpdatePaletteFade();
        SetMainCallback2(CB2_PrintErrorMessage);
    }
}

static void ErrorMsg_MoveCloserToPartner(void)
{
    LoadBgTiles(0, sCommErrorBg_Gfx, 0x20, 0);
    DecompressAndLoadBgGfxUsingHeap(1, sWirelessLinkDisplayGfx, FALSE, 0, 0);
    CopyToBgTilemapBuffer(1, sWirelessLinkDisplayTilemap, 0, 0);
    CopyBgTilemapBufferToVram(1);
    LoadPalette(sWirelessLinkDisplayPal, BG_PLTT_ID(0), sizeof(sWirelessLinkDisplayPal));
    FillWindowPixelBuffer(WIN_LINK_ERROR_TOP, PIXEL_FILL(0));
    FillWindowPixelBuffer(WIN_LINK_ERROR_BOTTOM, PIXEL_FILL(0));
    AddTextPrinterParameterized3(WIN_LINK_ERROR_TOP, FONT_SHORT_COPY_1, 2, 6, sTextColors, 0, gText_CommErrorEllipsis);
    AddTextPrinterParameterized3(WIN_LINK_ERROR_BOTTOM, FONT_SHORT_COPY_1, 2, 1, sTextColors, 0, gText_MoveCloserToLinkPartner);
    PutWindowTilemap(WIN_LINK_ERROR_TOP);
    PutWindowTilemap(WIN_LINK_ERROR_BOTTOM);
    CopyWindowToVram(WIN_LINK_ERROR_TOP, COPYWIN_NONE); // Does nothing
    CopyWindowToVram(WIN_LINK_ERROR_BOTTOM, COPYWIN_FULL);
}

static void ErrorMsg_CheckConnections(void)
{
    LoadBgTiles(0, sCommErrorBg_Gfx, 0x20, 0);
    FillWindowPixelBuffer(WIN_LINK_ERROR_MID, PIXEL_FILL(0));
    FillWindowPixelBuffer(WIN_LINK_ERROR_BOTTOM, PIXEL_FILL(0));
    AddTextPrinterParameterized3(WIN_LINK_ERROR_MID, FONT_SHORT_COPY_1, 2, 0, sTextColors, 0, gText_CommErrorCheckConnections);
    PutWindowTilemap(WIN_LINK_ERROR_MID);
    PutWindowTilemap(WIN_LINK_ERROR_BOTTOM);
    CopyWindowToVram(WIN_LINK_ERROR_MID, COPYWIN_NONE); // Does nothing
    CopyWindowToVram(WIN_LINK_ERROR_BOTTOM, COPYWIN_FULL);
}

static void CB2_PrintErrorMessage(void)
{
    switch (gMain.state)
    {
    case  00:
        // Below is only true for the RFU, so the other error
        // type is inferred to be from a wired connection
        if (sLinkErrorBuffer.disconnected)
            ErrorMsg_MoveCloserToPartner();
        else
            ErrorMsg_CheckConnections();
        break;
    case  02:
        ShowBg(0);
        if (sLinkErrorBuffer.disconnected)
            ShowBg(1);
        break;
    case  30:
        PlaySE(SE_BOO);
        break;
    case  60:
        PlaySE(SE_BOO);
        break;
    case  90:
        PlaySE(SE_BOO);
        break;
    }

    if (gMain.state != 160)
        gMain.state++;
}

// TODO: there might be a file boundary here, let's name it

bool8 GetSioMultiSI(void)
{
    return (REG_SIOCNT & SIO_MULTI_SI) != 0;
}

static bool8 IsSioMultiMaster(void)
{
    return (REG_SIOCNT & SIO_MULTI_SD) && (REG_SIOCNT & SIO_MULTI_SI) == 0;
}

bool8 IsLinkConnectionEstablished(void)
{
    return EXTRACT_CONN_ESTABLISHED(gLinkStatus);
}

void SetSuppressLinkErrorMessage(bool8 flag)
{
    gSuppressLinkErrorMessage = flag;
}

bool8 HasLinkErrorOccurred(void)
{
    return gLinkErrorOccurred;
}

void LocalLinkPlayerToBlock(void)
{
    struct LinkPlayerBlock *block;

    InitLocalLinkPlayer();
    block = &gLocalLinkPlayerBlock;
    block->linkPlayer = gLocalLinkPlayer;
    memcpy(block->magic1, sASCIIGameFreakInc, sizeof(block->magic1) - 1);
    memcpy(block->magic2, sASCIIGameFreakInc, sizeof(block->magic2) - 1);
    memcpy(gBlockSendBuffer, block, sizeof(*block));
}

void LinkPlayerFromBlock(u32 who)
{
    u8 who_ = who;
    struct LinkPlayerBlock *block;
    struct LinkPlayer *player;

    block = (struct LinkPlayerBlock *)gBlockRecvBuffer[who_];
    player = &gLinkPlayers[who_];
    *player = block->linkPlayer;
    ConvertLinkPlayerName(player);

    if (strcmp(block->magic1, sASCIIGameFreakInc) != 0
     || strcmp(block->magic2, sASCIIGameFreakInc) != 0)
        SetMainCallback2(CB2_LinkError);
}

// When this function returns TRUE the callbacks are skipped
bool8 HandleLinkConnection(void)
{
    gLinkStatus = LinkMain1(&gShouldAdvanceLinkState, gSendCmd, NULL);
    LinkMain2(&gMain.heldKeys);
    if ((gLinkStatus & LINK_STAT_RECEIVED_NOTHING) && IsSendingKeysOverCable() == TRUE)
        return TRUE;
    return FALSE;
}

u32 GetLinkRecvQueueLength(void)
{
    return gLink.recvQueue.count;
}

bool32 IsLinkRecvQueueAtOverworldMax(void)
{
    if (GetLinkRecvQueueLength() >= OVERWORLD_RECV_QUEUE_MAX)
        return TRUE;

    return FALSE;
}

// Union Room no longer exists; nothing can ever be standing in it.
bool32 InUnionRoom(void)
{
    return FALSE;
}

// Unused
u8 GetWirelessCommType(void)
{
    return gWirelessCommType;
}

void ConvertLinkPlayerName(struct LinkPlayer *player)
{
    player->progressFlagsCopy = player->progressFlags; // ? Perhaps relocating for a longer name field
    ConvertInternationalString(player->name, player->language);
}

static void DisableSerial(void)
{
    DisableInterrupts(INTR_FLAG_TIMER3 | INTR_FLAG_SERIAL);
    REG_SIOCNT = SIO_MULTI_MODE;
    REG_TMCNT_H(3) = 0;
    REG_IF = INTR_FLAG_TIMER3 | INTR_FLAG_SERIAL;
    REG_SIOMLT_SEND = 0;
    REG_SIOMLT_RECV = 0;
    CpuFill32(0, &gLink, sizeof(gLink));
}

static void EnableSerial(void)
{
    DisableInterrupts(INTR_FLAG_TIMER3 | INTR_FLAG_SERIAL);
    REG_RCNT = 0;
    REG_SIOCNT = SIO_MULTI_MODE;
    REG_SIOCNT |= SIO_115200_BPS | SIO_INTR_ENABLE;
    EnableInterrupts(INTR_FLAG_SERIAL);
    REG_SIOMLT_SEND = 0;
    CpuFill32(0, &gLink, sizeof(gLink));
    gLastSendQueueCount = 0;
    gLastRecvQueueCount = 0;
}

void ResetSerial(void)
{
    EnableSerial();
    DisableSerial();
}

// The GBA hardware link cable is no longer emulated -- see the note on struct SendQueue/RecvQueue
// in link.h. This used to run the SIO multi-play handshake/transfer state machine; now it just
// reports "never connected" so every caller's existing no-partner/timeout handling takes over
// (identical to what happens today with no cable plugged in).
u32 LinkMain1(u8 *shouldAdvanceLinkState, u16 *sendCmd, u16 (*recvCmds)[CMD_LENGTH])
{
    *shouldAdvanceLinkState = 0;
    return 0;
}

// link_intr.c

// Real hardware interrupts only fire when actual SIO/Timer3 link signaling occurs, which requires
// a real link cable partner -- one no longer exists (see LinkMain1 above). Kept as no-op stubs
// since src/main.c's interrupt table and VBlank hook still reference them by symbol.
void LinkVSync(void)
{
}

void Timer3Intr(void)
{
}

void SerialCB(void)
{
}

bool32 ShouldCheckForUnionRoom(void)
{
    if (OW_UNION_DISABLE_CHECK)
        return FALSE;

    if (OW_FLAG_MOVE_UNION_ROOM_CHECK == 0)
        return TRUE;

    if (FlagGet(OW_FLAG_MOVE_UNION_ROOM_CHECK))
        return TRUE;

    return FALSE;
}
