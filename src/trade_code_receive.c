#include "global.h"
#include "trade_code_receive.h"
#include "trade_code.h"
#include "trade_code_entry.h"
#include "trade_code_prompt.h"
#include "caps.h"
#include "evolution_scene.h"
#include "limited_party.h"
#include "malloc.h"
#include "overworld.h"
#include "pokedex.h"
#include "pokemon.h"
#include "save.h"
#include "string_util.h"
#include "strings.h"
#include "constants/battle.h"
#include "constants/pokedex.h"
#include "constants/species.h"

// Step 4, the commit and swap. See include/trade_code_receive.h for the scope
// and entry-point rationale, including why this does not reuse
// CB2_InitInGameTrade.
//
// TradeMons (src/trade.c) swaps the chosen party slot with
// gParties[B_TRAINER_OPPONENT_A][0] as part of that animation, which would
// corrupt an already-empty slot, and BufferTradeSceneStrings reads OT
// name/nickname from sIngameTrades[], which would print the wrong names.
//
// Trade evolution uses BeginEvolutionScene, which builds its own sprite and
// needs no trade-animation setup. GetEvolutionTargetSpecies is called with
// tradePartner = NULL, which DoesMonMeetAdditionalConditions supports
// (src/pokemon.c): plain and held-item trade evolutions (Machoke, King's
// Rock/Metal Coat) resolve as normal, but a partner-species-specific
// evolution cannot fire, since no partner mon exists at Step 4.
//
// gSaveBlock2Ptr->pendingTrade.abandonedCount is intentionally never written;
// see trade_code_receive.h for why give-ups are not tracked. The field stays
// in place to avoid changing struct SaveBlock2's layout and test/save.c's size
// pin, and is always zero.

// outBits target for the confirm-code field: TRADE_CODE_CONFIRM_CHARS (6)
// symbols * 5 bits = 30 bits, rounded up to a whole byte.
#define TRADE_CODE_RECEIVE_CONFIRM_SCRATCH_BYTES ((TRADE_CODE_CONFIRM_CHARS * 5 + 7) / 8)

struct TradeCodeReceiveState
{
    MainCallback returnCallback; // see include/trade_code_receive.h
    struct TradeCodeBits entryBits;
    u8 entryScratch[TRADE_CODE_RECEIVE_CONFIRM_SCRATCH_BYTES];
    enum TradeCodeEntryStatus entryStatus;
    enum TradeCodePromptResult promptResult;
    u8 partyIndex;    // valid only when !wentToPC
    bool8 wentToPC;
    // Distinguishes why wentToPC is set, so CB2_TradeCodeReceive_AfterTakeCareMsg
    // shows the right "sent to a Box" message (see TradeCodeReceive_DoSwap).
    bool8 overLevelCap;
};

static EWRAM_DATA struct TradeCodeReceiveState *sTradeCodeReceivePtr = NULL;

static void TradeCodeReceive_ShowEntry(void);
static enum TradeCodeEntryStatus TradeCodeReceive_ValidateConfirmEntry(struct TradeCodeBits *decoded);
static void TradeCodeReceive_DoSwap(void);
static void TradeCodeReceive_CheckEvolution(void);
static void TradeCodeReceive_ClearPendingTradeFields(void);
static void TradeCodeReceive_SaveThenFinish(void);
static void TradeCodeReceive_FinishToReturnCallback(void);
static void TradeCodeReceive_ShowGiveUpPrompt(void);
static void TradeCodeReceive_DoGiveUp(void);
static void CB2_TradeCodeReceive_AfterNoTradeAck(void);
static void CB2_TradeCodeReceive_AfterCorruptAck(void);
static void CB2_TradeCodeReceive_AfterConfirmEntry(void);
static void CB2_TradeCodeReceive_AfterGiveUpPrompt(void);
static void CB2_TradeCodeReceive_AfterSentOverMsg(void);
static void CB2_TradeCodeReceive_AfterTakeCareMsg(void);
static void CB2_TradeCodeReceive_AfterBoxMsg(void);
static void CB2_TradeCodeReceive_AfterEvolution(void);
static void CB2_TradeCodeReceive_AfterSaveFailedAck(void);

static const u8 sText_NoTradeAwaiting[]  = _("There's no trade code waiting\nto be completed.");
// Shown when pendingTrade.incoming fails TradeCode_ValidatePendingBoxMon on
// resume (see CB2_TradeCodeReceive_AfterCorruptAck).
static const u8 sText_TradeCodeCorrupt[] = _("Something went wrong with a\npending trade. It's been cancelled.");
// The one way out of a COMMITTED trade (B on an empty confirm-code field).
// The prompt defaults to NO: this is irreversible, and an accidental
// double-B-then-A must not confirm it.
static const u8 sText_ConfirmGiveUp[] = _("Give up on this trade?\nYou will not get {STR_VAR_1}.");
// Two short ACK messages rather than one \p-paged one, like vanilla's
// gText_XSentOverY/gText_TakeGoodCareOfX split (src/trade.c), avoiding \p
// pagination in the prompt screen.
static const u8 sText_SentOver[]      = _("{STR_VAR_1} sent over\n{STR_VAR_2}!");
static const u8 sText_TakeGoodCareOfIt[] = _("Take good care of\n{STR_VAR_2}!");
static const u8 sText_SentToBox[]     = _("Your party is full, so\n{STR_VAR_2} was sent to a Box.");
// Shown instead of sText_SentToBox when the incoming mon is above the level
// cap, whether or not the party had room (see TradeCodeReceive_DoSwap).
static const u8 sText_SentToBoxLevelCap[] = _("{STR_VAR_2} is above your\nlevel cap, so it was boxed.");

void TradeCodeReceive_Start(MainCallback returnCallback)
{
    struct TradeCodeReceiveState *s;

    if ((s = AllocZeroed(sizeof(struct TradeCodeReceiveState))) == NULL)
    {
        SetMainCallback2(returnCallback);
        return;
    }
    sTradeCodeReceivePtr = s;
    s->returnCallback = returnCallback;

    // The debug menu can call this directly regardless of what
    // gSaveBlock2Ptr->pendingTrade holds, so guard here too.
    if (gSaveBlock2Ptr->pendingTrade.state != TRADE_CODE_STATE_COMMITTED)
    {
        TradeCodePrompt_Init(sText_NoTradeAwaiting, FALSE, FALSE, &s->promptResult, CB2_TradeCodeReceive_AfterNoTradeAck);
        return;
    }

    // Guard against a corrupted or hand-tampered pendingTrade before it reaches
    // the entry screen/preview/party-insert code. Checked here, in the one real
    // entry point, so the debug menu's path gets the same protection as the
    // boot hook (src/overworld.c).
    {
        struct BoxPokemon incoming;
        memcpy(&incoming, gSaveBlock2Ptr->pendingTrade.incoming, sizeof(incoming));
        if (!TradeCode_ValidatePendingBoxMon(&incoming))
        {
            TradeCodePrompt_Init(sText_TradeCodeCorrupt, FALSE, FALSE, &s->promptResult, CB2_TradeCodeReceive_AfterCorruptAck);
            return;
        }
    }

    TradeCodeReceive_ShowEntry();
}

static void CB2_TradeCodeReceive_AfterNoTradeAck(void)
{
    TradeCodeReceive_FinishToReturnCallback();
}

// pendingTrade.incoming failed TradeCode_ValidatePendingBoxMon. Never
// materialised; cleared back to TRADE_CODE_STATE_NONE (keeping the replay ring)
// and force-saved so the error does not recur on every boot.
static void CB2_TradeCodeReceive_AfterCorruptAck(void)
{
    TradeCodeReceive_ClearPendingTradeFields();
    TradeCodeReceive_SaveThenFinish();
}

static void TradeCodeReceive_ShowEntry(void)
{
    struct TradeCodeReceiveState *s = sTradeCodeReceivePtr;

    s->entryBits.data = s->entryScratch;
    s->entryBits.capacity = sizeof(s->entryScratch) * 8;
    TradeCodeEntry_Init(&s->entryBits, TRADE_CODE_CONFIRM_CHARS, TradeCodeReceive_ValidateConfirmEntry,
                         &s->entryStatus, CB2_TradeCodeReceive_AfterConfirmEntry);
}

// The TradeCodeEntryValidator for the confirm-code entry screen. A confirm
// code is codeKind (2 bits) + a 28-bit combined tag with no seal of its own:
// the tag (TradeCode_ConfirmTag) is the anti-forgery check, compared against
// pendingTrade.expectedConfirmTag stored at Step 3.
static enum TradeCodeEntryStatus TradeCodeReceive_ValidateConfirmEntry(struct TradeCodeBits *decoded)
{
    u32 codeKind, tag;

    codeKind = TradeCode_ReadBits(decoded, 2);
    tag = TradeCode_ReadBits(decoded, 28);
    if (decoded->error)
        return TRADE_CODE_ENTRY_WRONG_LENGTH;
    if (codeKind != TRADE_CODE_KIND_CONFIRM)
        return TRADE_CODE_ENTRY_INVALID; // e.g. an offer code typed into the confirm field
    if (tag != gSaveBlock2Ptr->pendingTrade.expectedConfirmTag)
        return TRADE_CODE_ENTRY_INVALID; // typo, or genuinely not the matching partner
    return TRADE_CODE_ENTRY_OK;
}

static void CB2_TradeCodeReceive_AfterConfirmEntry(void)
{
    struct TradeCodeReceiveState *s = sTradeCodeReceivePtr;

    if (s->entryStatus != TRADE_CODE_ENTRY_OK)
    {
        // The only other status this callback can see is
        // TRADE_CODE_ENTRY_CANCELLED (B on an empty field); a validator
        // rejection is handled inside the entry screen, which loops back into
        // the field without reaching this callback.
        //
        // Step 4 has no ordinary cancel (the offered mon already left in
        // Step 3), but a partner who never sends a valid confirm code would
        // trap the player here on every boot, so offer forfeiting the trade.
        TradeCodeReceive_ShowGiveUpPrompt();
        return;
    }

    TradeCodeReceive_DoSwap();
}

// See TradeCodeReceive_Start (include/trade_code_receive.h) for why this exists.
static void TradeCodeReceive_ShowGiveUpPrompt(void)
{
    struct TradeCodeReceiveState *s = sTradeCodeReceivePtr;
    struct BoxPokemon boxMon;

    memcpy(&boxMon, gSaveBlock2Ptr->pendingTrade.incoming, sizeof(boxMon));
    GetBoxMonData(&boxMon, MON_DATA_NICKNAME, gStringVar1);
    StripExtCtrlCodes(gStringVar1);
    // TradeCodePrompt_Init doesn't expand placeholders; see TradeCodeReceive_DoSwap.
    StringExpandPlaceholders(gStringVar4, sText_ConfirmGiveUp);
    TradeCodePrompt_Init(gStringVar4, TRUE, TRUE, &s->promptResult, CB2_TradeCodeReceive_AfterGiveUpPrompt);
}

static void CB2_TradeCodeReceive_AfterGiveUpPrompt(void)
{
    struct TradeCodeReceiveState *s = sTradeCodeReceivePtr;

    if (s->promptResult == TRADE_CODE_PROMPT_YES)
        TradeCodeReceive_DoGiveUp();
    else
        TradeCodeReceive_ShowEntry(); // "No" - keep waiting, back to the field
}

// The forfeit itself: permanently gives up the incoming mon. Step 3's escrow
// has no undo (both sides gave up their mon before either received anything),
// so this acknowledges a loss that already happened rather than applying a
// penalty. Nothing is recorded (see trade_code_receive.h). Keeps the replay
// ring: the partner's offer seal stays burned, like a normal completion.
static void TradeCodeReceive_DoGiveUp(void)
{
    TradeCodeReceive_ClearPendingTradeFields();
    TradeCodeReceive_SaveThenFinish();
}

// The actual Step 4 swap: build the incoming BoxPokemon into a struct Pokemon,
// apply the received-mon friendship reset, insert it into the party (or the PC
// if the party is full), update the Pokedex, and clear pendingTrade to NONE
// (except the replay ring and abandonedCount). Evolution and the second
// force-save are split out since evolution must finish (BeginEvolutionScene
// takes over the screen) before the save.
static void TradeCodeReceive_DoSwap(void)
{
    struct TradeCodeReceiveState *s = sTradeCodeReceivePtr;
    struct BoxPokemon boxMon;
    struct Pokemon mon;
    bool32 isEgg, overLevelCap;
    u8 friendship;
    u8 maxSize, i;

    memcpy(&boxMon, gSaveBlock2Ptr->pendingTrade.incoming, sizeof(boxMon));
    BoxMonToMon(&boxMon, &mon);
    CalculateMonStats(&mon);

    // Mirrors src/trade.c's TradeMons friendship=70 rule (src/trade.c:3113-3116).
    // Eggs use Friendship to track egg cycles, so it is left alone on one.
    isEgg = GetMonData(&mon, MON_DATA_IS_EGG);
    if (!isEgg)
    {
        friendship = 70;
        SetMonData(&mon, MON_DATA_FRIENDSHIP, &friendship);
    }

    // Eggs are exempt from the level cap: an egg's level is not a real level
    // until it hatches. Otherwise checked against MON_DATA_LEVEL after
    // CalculateMonStats, i.e. the transmitted level TradeCode_DeserializeMon
    // validated as 1..MAX_LEVEL, never the sender's live level.
    overLevelCap = !isEgg && B_EXP_CAP_TYPE != EXP_CAP_NONE
                 && GetMonData(&mon, MON_DATA_LEVEL) > GetCurrentLevelCap();
    s->overLevelCap = overLevelCap;

    if (overLevelCap)
    {
        // Marks this mon for IsBoxMonWithdrawLocked (src/pokemon_storage_system.c);
        // see struct BoxPokemon (include/pokemon.h) for the one-way lock. The
        // lock is its own explicit bit, not an OT-ID inference, so the
        // partner-synthesised OT ID stays untouched on every receipt, over cap
        // or not. IsTradedMon/IsOtherTrainer (src/pokemon.c) therefore apply the
        // traded-Pokemon EXP boost and obedience bypass normally.
        mon.box.tradeCodeAboveLevelCap = TRUE;
    }

    // Insert into the first empty party slot, else the PC, like
    // GiveCapturedMonToPlayer (src/pokemon.c) but without calling it: its
    // Achievement_CheckCaptureMilestones/Achievement_OnShinyObtained calls are
    // about catching, not trading. An over-level-cap mon always goes straight
    // to the PC, even with open party slots.
    if (overLevelCap)
    {
        CopyMonToPC(&mon);
        s->wentToPC = TRUE;
    }
    else
    {
        maxSize = LimitedParty_GetMaxPartySize();
        for (i = 0; i < maxSize; i++)
        {
            if (GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_SPECIES) == SPECIES_NONE)
                break;
        }
        if (i < maxSize)
        {
            CopyMon(&gParties[B_TRAINER_PLAYER][i], &mon, sizeof(mon));
            CalculatePlayerPartyCount();
            s->partyIndex = i;
            s->wentToPC = FALSE;
        }
        else
        {
            CopyMonToPC(&mon);
            s->wentToPC = TRUE;
        }
    }

    // GAME_STAT_POKEMON_TRADES: mirrors both src/trade.c increment sites,
    // which are unreachable under TRADE_CODES; without this the trainer card's
    // trade count (src/trainer_card.c) would stay at 0.
    IncrementGameStat(GAME_STAT_POKEMON_TRADES);

    // Pokedex registration, mirroring src/trade.c's static
    // UpdatePokedexForReceivedMon; HandleSetPokedexFlagFromMon does the
    // "caught" half.
    if (!isEgg)
    {
        GetSetPokedexFlagBySpecies(GetMonData(&mon, MON_DATA_SPECIES), FLAG_SET_SEEN);
        HandleSetPokedexFlagFromMon(&mon, FLAG_SET_CAUGHT);
    }

    // pendingTrade is done with, except the replay ring and abandonedCount.
    TradeCodeReceive_ClearPendingTradeFields();

    GetMonData(&mon, MON_DATA_OT_NAME, gStringVar1);
    StripExtCtrlCodes(gStringVar1);
    GetMonData(&mon, MON_DATA_NICKNAME, gStringVar2);
    StripExtCtrlCodes(gStringVar2);

    StringExpandPlaceholders(gStringVar4, sText_SentOver);
    TradeCodePrompt_Init(gStringVar4, FALSE, FALSE, &s->promptResult, CB2_TradeCodeReceive_AfterSentOverMsg);
}

static void CB2_TradeCodeReceive_AfterSentOverMsg(void)
{
    struct TradeCodeReceiveState *s = sTradeCodeReceivePtr;

    // gStringVar1/gStringVar2 are still what TradeCodeReceive_DoSwap set.
    StringExpandPlaceholders(gStringVar4, sText_TakeGoodCareOfIt);
    TradeCodePrompt_Init(gStringVar4, FALSE, FALSE, &s->promptResult, CB2_TradeCodeReceive_AfterTakeCareMsg);
}

static void CB2_TradeCodeReceive_AfterTakeCareMsg(void)
{
    struct TradeCodeReceiveState *s = sTradeCodeReceivePtr;

    if (s->wentToPC)
    {
        StringExpandPlaceholders(gStringVar4, s->overLevelCap ? sText_SentToBoxLevelCap : sText_SentToBox);
        TradeCodePrompt_Init(gStringVar4, FALSE, FALSE, &s->promptResult, CB2_TradeCodeReceive_AfterBoxMsg);
        return;
    }
    TradeCodeReceive_CheckEvolution();
}

static void CB2_TradeCodeReceive_AfterBoxMsg(void)
{
    TradeCodeReceive_CheckEvolution();
}

// A boxed mon (party was full) gets no evolution check, like a wild catch that
// overflows to the PC; there is no in-box evolution UI to reuse.
static void TradeCodeReceive_CheckEvolution(void)
{
    struct TradeCodeReceiveState *s = sTradeCodeReceivePtr;
    // NULL default: only dereferenced in the `evoTarget != SPECIES_NONE` branch,
    // which requires wentToPC to be FALSE (the only branch that assigns it).
    struct Pokemon *mon = NULL;
    enum Species evoTarget = SPECIES_NONE;

    if (!s->wentToPC)
    {
        mon = &gParties[B_TRAINER_PLAYER][s->partyIndex];
        if (!GetMonData(mon, MON_DATA_IS_EGG))
            evoTarget = GetEvolutionTargetSpecies(mon, EVO_MODE_TRADE, ITEM_NONE, NULL, NULL, CHECK_EVO);
    }

    if (evoTarget != SPECIES_NONE)
    {
        // tradePartner = NULL throughout; see the header comment.
        GetEvolutionTargetSpecies(mon, EVO_MODE_TRADE, ITEM_NONE, NULL, NULL, DO_EVO);
        gCB2_AfterEvolution = CB2_TradeCodeReceive_AfterEvolution;
        // BeginEvolutionScene's `mon` parameter is unused internally
        // (Task_BeginEvolutionScene re-fetches the party mon); passed for
        // signature parity with EvolutionScene/TradeEvolutionScene.
        BeginEvolutionScene(mon, evoTarget, FALSE, s->partyIndex);
        return;
    }

    TradeCodeReceive_SaveThenFinish();
}

static void CB2_TradeCodeReceive_AfterEvolution(void)
{
    TradeCodeReceive_SaveThenFinish();
}

// Individual-field clear, not a whole-struct memset, so the replay ring (and
// the unused abandonedCount) survive. Shared by every Step-4-ends-here path:
// a completed swap (TradeCodeReceive_DoSwap), the give-up forfeit
// (TradeCodeReceive_DoGiveUp), and corrupted-pendingTrade recovery
// (CB2_TradeCodeReceive_AfterCorruptAck).
static void TradeCodeReceive_ClearPendingTradeFields(void)
{
    memset(gSaveBlock2Ptr->pendingTrade.incoming, 0, sizeof(gSaveBlock2Ptr->pendingTrade.incoming));
    gSaveBlock2Ptr->pendingTrade.expectedConfirmTag = 0;
    gSaveBlock2Ptr->pendingTrade.nonce = 0;
    gSaveBlock2Ptr->pendingTrade.partySlot = 0;
    gSaveBlock2Ptr->pendingTrade.state = TRADE_CODE_STATE_NONE;
}

// The force-save shared by every path that ends by handing control back out
// (the second force-save after Step 3's TrySavingData in trade_code_session.c,
// and the only one on the give-up/corrupt-clear paths). It runs after every RAM
// mutation the path makes, so a power cut before it succeeds loses or
// duplicates nothing: the saved file does not yet reflect the change, and the
// boot hook re-runs the interrupted path from scratch.
static void TradeCodeReceive_SaveThenFinish(void)
{
    struct TradeCodeReceiveState *s = sTradeCodeReceivePtr;
    u8 saveStatus = TrySavingData(SAVE_NORMAL);

    if (saveStatus != SAVE_STATUS_OK)
    {
        TradeCodePrompt_Init(gText_SaveError, FALSE, FALSE, &s->promptResult, CB2_TradeCodeReceive_AfterSaveFailedAck);
        return;
    }

    TradeCodeReceive_FinishToReturnCallback();
}

static void CB2_TradeCodeReceive_AfterSaveFailedAck(void)
{
    // Safe to retry unconditionally: everything so far only touched
    // gSaveBlock2Ptr/gParties in RAM, and nothing reaches the save file until
    // TrySavingData succeeds.
    TradeCodeReceive_SaveThenFinish();
}

// Frees this screen's state and hands control to the returnCallback given to
// TradeCodeReceive_Start (not always CB2_ReturnToField; see the header).
static void TradeCodeReceive_FinishToReturnCallback(void)
{
    struct TradeCodeReceiveState *s = sTradeCodeReceivePtr;
    MainCallback returnCallback = s->returnCallback;

    Free(s);
    sTradeCodeReceivePtr = NULL;
    SetMainCallback2(returnCallback);
}
