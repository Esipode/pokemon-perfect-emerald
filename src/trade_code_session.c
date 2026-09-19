#include "global.h"
#include "trade_code_session.h"
#include "trade_code.h"
#include "trade_code_display.h"
#include "trade_code_entry.h"
#include "trade_code_prompt.h"
#include "trade_code_receive.h"
#include "draft_mode.h"
#include "malloc.h"
#include "overworld.h"
#include "party_menu.h"
#include "pokemon.h"
#include "pokemon_storage_system.h"
#include "pokemon_summary_screen.h"
#include "random.h"
#include "recruits_mode.h"
#include "save.h"
#include "script_pokemon_util.h"
#include "string_util.h"
#include "strings.h"
#include "constants/battle.h"
#include "constants/species.h"

// Steps 1-3 of the protocol. See include/trade_code_session.h for the scope
// and entry-point rationale.
//
// Every screen this file transitions through (ChooseMonForTradingBoard,
// TradeCodeEntry_Init, ShowPokemonSummaryScreen, TradeCodeDisplay_Init,
// TradeCodePrompt_Init) is a full-screen takeover that replaces
// gMain.callback2 and resets the task list, chaining into the next via an
// explicit MainCallback parameter. This file never goes through
// CB2_ReturnToField/the overworld's field-callback machinery mid-session, and
// never returns to a walkable overworld until the session ends (cancelled, or
// Step 3 complete with the confirm code shown).
//
// Bouncing through CB2_ReturnToField + gFieldCallback for native prompts hung
// on real hardware (a visibly-normal overworld with the player locked), so
// every prompt uses the self-contained TradeCodePrompt_Init instead.



// Rounds up to a whole byte. Byte alignment (not 5-bit Base32 symbol
// alignment) is required: see TradeCodeSession_BuildOffer.
#define ROUND_UP_TO_BYTE(n) ((((n) + 7) / 8) * 8)

// Worst case per TRADE_CODE_MAX_CHARS' derivation (include/config/
// trade_code.h): header (22) + worst-case mon payload (374, presence included)
// = 396, rounded up to a byte = 400, + a 32-bit seal = 432 bits = 54 bytes.
// This is all this file writes into myOfferBytes or copies out of a validated
// partner payload into partnerOfferBytes (never the trailing Base32 pad).
//
// Rounded up to 448 (56 bytes) for slack. TradeCode_WriteBits/ReadBits latch an
// error rather than overrun, so the slack costs a few bytes of EWRAM per buffer
// without removing a check.
//
// BYTES reuses TRADE_CODE_OFFER_PAYLOAD_BYTES (include/config/trade_code.h)
// instead of a second literal: struct PendingTrade persists the player's own
// offer in a buffer of the same shape for "view offer code".
#define TRADE_CODE_SESSION_OFFER_MAX_BITS 448
#define TRADE_CODE_SESSION_OFFER_BYTES TRADE_CODE_OFFER_PAYLOAD_BYTES

// entryScratch (below) is TradeCodeEntry_Init's outBits target. It can hold up
// to TRADE_CODE_ENTRY_MAX_SYMBOLS (87, src/trade_code_entry.c) symbols' worth of
// raw decoded bits (87*5 = 435), a few bits past
// TRADE_CODE_SESSION_OFFER_MAX_BITS because of TradeCode_Decode's trailing
// Base32 padding. Sized to match TRADE_CODE_ENTRY_SCRATCH_BYTES: a smaller
// buffer would make TradeCodeEntry_Init reject a legitimate worst-case partner
// code as TRADE_CODE_ENTRY_WRONG_LENGTH. TRADE_CODE_ENTRY_MAX_SYMBOLS is
// file-local to trade_code_entry.c, so this mirrors it; update both together.
#define TRADE_CODE_ENTRY_MAX_SYMBOLS_MIRROR 87
#define TRADE_CODE_SESSION_ENTRY_SCRATCH_BYTES ((TRADE_CODE_ENTRY_MAX_SYMBOLS_MIRROR * 5 + 7) / 8)

// A confirm code's payload (codeKind 2 + a 28-bit tag = 30 bits) is fixed and
// tiny.
#define TRADE_CODE_SESSION_CONFIRM_BYTES 4

struct TradeCodeSessionState
{
    // ---- Step 1: the offer I generate ----
    u8 partySlot;
    u32 myOtId;
    u16 myNonce;
    u32 myOfferBits;    // exact bit length of header+mon+pad+seal
    u8 myOfferBytes[TRADE_CODE_SESSION_OFFER_BYTES];
    u16 myOfferSpecies;                          // carried into pendingTrade at commit, for "view offer code"'s redisplay icon
    u8 myOfferNickname[POKEMON_NAME_LENGTH + 1]; // same as above

    // ---- Step 2: the partner's offer, filled in by the validator ----
    struct BoxPokemon partnerBoxMon;
    u32 partnerOtId;
    u16 partnerNonce;
    u32 partnerOfferBits;
    u8 partnerOfferBytes[TRADE_CODE_SESSION_OFFER_BYTES];
    u32 partnerOfferSeal;

    // BoxMonToMon target for the preview screen, deliberately NOT
    // gParties[B_TRAINER_OPPONENT_A][0]: DoesMonOTMatchOwner()
    // (pokemon_summary_screen.c) treats that exact array, by pointer identity,
    // as "active link battle" and reads the comparison OT from
    // gLinkPlayers[GetMultiplayerId() ^ 1]. With no link session that is
    // meaningless state, and the summary screen renders blank/garbled. A
    // dedicated buffer never aliases that array, so the normal non-link branch
    // runs.
    struct Pokemon previewMon;

    // outBits target for TradeCodeEntry_Init. Its contents go unused: the
    // validator extracts everything, since `decoded` is only valid during the
    // validator call.
    struct TradeCodeBits entryBits;
    u8 entryScratch[TRADE_CODE_SESSION_ENTRY_SCRATCH_BYTES];
    enum TradeCodeEntryStatus entryStatus;

    // ---- TradeCodePrompt_Init's own out-param, and cancel-confirm bookkeeping ----
    enum TradeCodePromptResult promptResult;
    MainCallback cancelReturnCallback; // where "No" at the cancel-confirm goes back to
};

static EWRAM_DATA struct TradeCodeSessionState *sTradeCodeSessionPtr = NULL;
// TradeCodePrompt_Init's out-param for the two standalone "view code" entry
// points, which are reached from the attendant's menu with no live
// sTradeCodeSessionPtr. Never read back (both prompts are ACK-only); it exists
// because TradeCodePrompt_Init needs an out-pointer that outlives the call.
static enum TradeCodePromptResult sViewCodePromptResult;

static bool8 TradeCodeSession_WouldLeavePartyEmpty(u8 slot);
static void TradeCodeSession_BuildOffer(struct Pokemon *mon);
static enum TradeCodeEntryStatus TradeCodeSession_ValidateOfferEntry(struct TradeCodeBits *decoded);
static void TradeCodeSession_EncodeConfirmTag(u32 tag, u8 *outEncoded);
static bool8 TradeCodeSession_DoCommit(void);
static void TradeCodeSession_ShowCancelConfirm(MainCallback returnCallback);
static void TradeCodeSession_ShowOfferReadyPrompt(void);
static void TradeCodeSession_ShowCommitPrompt(void);
static void TradeCodeSession_ShowSaveFailedPrompt(void);
static void TradeCodeSession_AbortToField(void);
static void CB2_TradeCodeSession_AfterGateFailAck(void);
static void CB2_TradeCodeSession_AfterRejectAck(void);
static void CB2_TradeCodeSession_AfterChooseMon(void);
static void CB2_TradeCodeSession_AfterOfferShown(void);
static void CB2_TradeCodeSession_AfterOfferReadyPrompt(void);
static void CB2_TradeCodeSession_AfterOfferEntry(void);
static void CB2_TradeCodeSession_AfterPreview(void);
static void CB2_TradeCodeSession_AfterCommitPrompt(void);
static void CB2_TradeCodeSession_AfterCancelConfirm(void);
static void CB2_TradeCodeSession_AfterSaveFailedAck(void);

// CableClub_Text_NeedTwoMonsToTrade / _CantTradeEnigmaBerry (data/text/
// cable_club.inc) carry the vanilla wording for these gates, but are script
// .string symbols with no C declaration, so these C constants match that
// phrasing instead.
static const u8 sText_NeedTwoMons[]         = _("For trading, you must have at\nleast two Pokémon with you.");
static const u8 sText_CantTradeEnigmaBerry[] = _("A Pokémon holding the {STR_VAR_1}\nBerry can't be traded.");
static const u8 sText_CantTradeEgg[]        = _("An Egg can't be traded like\nthis.");
static const u8 sText_CantTradeLastMon[]    = _("You can't trade your last\nPokémon!");
// Nuzlocke disables trading entirely. Uses the plain
// gSaveBlock1Ptr->nuzlockeModeEnabled check used elsewhere (src/overworld.c,
// src/daycare.c, src/item_use.c).
static const u8 sText_CantTradeNuzlocke[]   = _("Trading isn't allowed during\na Nuzlocke run.");
// Draft Mode: in-game trades (CreateInGameTradePokemon) are untouched, since
// they swap in place and do not change party size, but this attendant-initiated
// code-trade flow is a real acquisition path and is refused outright.
static const u8 sText_CantTradeDraft[]      = _("Trading isn't allowed during\na Draft run.");
// Recruits Mode: a traded-in mon carries its own recruitBattles counter, so a
// mon from a non-Recruits save would arrive at 0/10, a laundering vector around
// the PC lock. Same shape as the Draft gate above.
static const u8 sText_CantTradeRecruits[]   = _("Trading isn't allowed during\na Recruits run.");
static const u8 sText_CantTradeFusedMon[]   = _("A fused Pokémon can't be\ntraded like this.");
static const u8 sText_ReadyForPartnerCode[] = _("Ready to enter your partner's\ntrade code?");
// This screen's message window (see sTradeCodePromptWindowTemplates in
// trade_code_prompt.c) is only 2 text lines tall. This message has 5 lines of
// content, so it uses \p (wait for A, then clear) between each 2-line page;
// AddTextPrinterForMessage handles \p natively.
static const u8 sText_ConfirmCommit[]       = _("{STR_VAR_1} will be given up\nnow. You will only receive\p{STR_VAR_2} once you enter\nyour partner's confirm code.\pContinue?");
// Shown by both TradeCodeSession_ViewOfferCode and
// TradeCodeSession_ViewConfirmCode when pendingTrade.state !=
// TRADE_CODE_STATE_COMMITTED: the attendant's menu is reachable with no trade
// in progress.
static const u8 sText_NoTradeCodeToShow[]   = _("You don't have a trade code\nto show right now.");
static const u8 sText_CancelConfirm[]       = _("Cancel this trade? Your\npartner may be waiting.");

void TradeCodeSession_Start(void)
{
    // Nuzlocke disables trading entirely. Checked before the COMMITTED-trade
    // redirect below: nuzlockeModeEnabled is a new-game-only setting
    // (ApplyPendingNewGameSettings, src/new_game_settings_menu.c), so a
    // Nuzlocke save can never have a COMMITTED trade. The boot hook
    // (src/overworld.c) calls TradeCodeReceive_Start directly, so a stale
    // COMMITTED trade still resolves normally on the next boot.
    if (gSaveBlock1Ptr->nuzlockeModeEnabled)
    {
        if ((sTradeCodeSessionPtr = AllocZeroed(sizeof(struct TradeCodeSessionState))) == NULL)
            return;
        TradeCodePrompt_Init(sText_CantTradeNuzlocke, FALSE, FALSE, &sTradeCodeSessionPtr->promptResult, CB2_TradeCodeSession_AfterGateFailAck);
        return;
    }

    // Draft Mode disables this flow too. Draft and Nuzlocke are mutually
    // exclusive (src/new_game_settings_menu.c), but each gate has its own
    // message, so they stay separate conditions.
    if (Draft_IsEnabled())
    {
        if ((sTradeCodeSessionPtr = AllocZeroed(sizeof(struct TradeCodeSessionState))) == NULL)
            return;
        TradeCodePrompt_Init(sText_CantTradeDraft, FALSE, FALSE, &sTradeCodeSessionPtr->promptResult, CB2_TradeCodeSession_AfterGateFailAck);
        return;
    }

    // Same shape as the Draft gate, with its own message. Recruits_IsEnabled()
    // (not _IsActive()) matches Draft's whole-run gate.
    if (Recruits_IsEnabled())
    {
        if ((sTradeCodeSessionPtr = AllocZeroed(sizeof(struct TradeCodeSessionState))) == NULL)
            return;
        TradeCodePrompt_Init(sText_CantTradeRecruits, FALSE, FALSE, &sTradeCodeSessionPtr->promptResult, CB2_TradeCodeSession_AfterGateFailAck);
        return;
    }

    // The attendant is this feature's only entry point, so a player with an
    // already-COMMITTED trade enters their partner's confirm code here (see
    // include/trade_code_session.h). Checked first and before the AllocZeroed
    // below: starting a new offer while a mon is escrowed awaiting Step 4 would
    // be wrong, and this path needs no session struct.
    if (gSaveBlock2Ptr->pendingTrade.state == TRADE_CODE_STATE_COMMITTED)
    {
        TradeCodeReceive_Start(CB2_ReturnToField);
        return;
    }

    if ((sTradeCodeSessionPtr = AllocZeroed(sizeof(struct TradeCodeSessionState))) == NULL)
        return; // couldn't even allocate - nothing was touched, nothing to undo

    // Mirrors CableClub_EventScript_CheckPartyTradeRequirements
    // (data/scripts/cable_club.inc), the gates the old link-trade path runs.
    // DoesPartyHaveEnigmaBerry() fills gStringVar1 with the berry's name on
    // TRUE (src/script_pokemon_util.c).
    if (CalculatePlayerPartyCount() < 2)
    {
        TradeCodePrompt_Init(sText_NeedTwoMons, FALSE, FALSE, &sTradeCodeSessionPtr->promptResult, CB2_TradeCodeSession_AfterGateFailAck);
        return;
    }
    if (DoesPartyHaveEnigmaBerry())
    {
        // TradeCodePrompt_Init only StringCopy's its message and does not
        // expand placeholders, so {STR_VAR_1} is substituted here first.
        StringExpandPlaceholders(gStringVar4, sText_CantTradeEnigmaBerry);
        TradeCodePrompt_Init(gStringVar4, FALSE, FALSE, &sTradeCodeSessionPtr->promptResult, CB2_TradeCodeSession_AfterGateFailAck);
        return;
    }

    // PARTY_MENU_TYPE_UNION_ROOM_REGISTER's CanRegisterMonForTradingBoard gate
    // (src/party_menu.c CursorCb_Register) reads IsNationalPokedexEnabled()
    // directly, so nothing needs priming.
    ChooseMonForTradingBoard(PARTY_MENU_TYPE_UNION_ROOM_REGISTER, CB2_TradeCodeSession_AfterChooseMon);
}

static void TradeCodeSession_AbortToField(void)
{
    Free(sTradeCodeSessionPtr);
    sTradeCodeSessionPtr = NULL;
    SetMainCallback2(CB2_ReturnToField);
}

static void CB2_TradeCodeSession_AfterGateFailAck(void)
{
    TradeCodeSession_AbortToField();
}

static void CB2_TradeCodeSession_AfterRejectAck(void)
{
    TradeCodeSession_AbortToField();
}

// Shows "Cancel this trade? Your partner may be waiting." - Yes ends the
// session entirely; No re-invokes `returnCallback` (whichever prompt asked
// to cancel in the first place), so the player lands right back where
// they were instead of being dropped somewhere unrelated.
static void TradeCodeSession_ShowCancelConfirm(MainCallback returnCallback)
{
    sTradeCodeSessionPtr->cancelReturnCallback = returnCallback;
    TradeCodePrompt_Init(sText_CancelConfirm, TRUE, TRUE, &sTradeCodeSessionPtr->promptResult, CB2_TradeCodeSession_AfterCancelConfirm);
}

static void CB2_TradeCodeSession_AfterCancelConfirm(void)
{
    struct TradeCodeSessionState *s = sTradeCodeSessionPtr;

    if (s->promptResult == TRADE_CODE_PROMPT_YES)
        TradeCodeSession_AbortToField();
    else
        s->cancelReturnCallback();
}

// Mirrors the numMonsLeft check in src/trade.c's file-local
// CanTradeSelectedMon: at least one non-egg mon must remain. Stricter than
// CableClub_EventScript_CheckPartyTradeRequirements' ">=2 total" gate, which
// counts eggs. Its National-Dex-gated cross-cartridge checks do not apply to
// an offline trade.
static bool8 TradeCodeSession_WouldLeavePartyEmpty(u8 slot)
{
    u32 i, count = CalculatePlayerPartyCount();
    u32 remaining = 0;

    for (i = 0; i < count; i++)
    {
        enum Species species;

        if (i == slot)
            continue;
        species = GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_SPECIES_OR_EGG);
        if (species != SPECIES_NONE && species != SPECIES_EGG)
            remaining++;
    }
    return (remaining == 0);
}

// Builds the full offer payload (header + TradeCode_SerializeMon's fields +
// zero-pad to a byte boundary + the 32-bit seal) into
// sTradeCodeSessionPtr->myOfferBytes/myOfferBits.
//
// The pad before the seal must reach a byte boundary, not merely a 5-bit
// (Base32 symbol) one. TradeCode_SealOffer hashes ceil(nBits/8) whole bytes and
// requires the trailing bits of a partial final byte to be zero; if nBits were
// not byte-aligned, the sender's zeros and the receiver's decoded bytes (mon
// fields followed by the real seal) would hash different content and never
// agree. Byte alignment leaves no partial final byte, and is also unambiguous
// against TradeCode_Decode's Base32 padding at the end of the code. This
// function and TradeCodeSession_ValidateOfferEntry derive paddedBits the same
// way (round up TradeCode_SerializeMon's end-of-mon bit position), so both
// sides agree where the seal starts.
static void TradeCodeSession_BuildOffer(struct Pokemon *mon)
{
    struct TradeCodeSessionState *s = sTradeCodeSessionPtr;
    struct TradeCodeBits stream;
    u32 paddedBits, padAmount, seal;

    memset(s->myOfferBytes, 0, sizeof(s->myOfferBytes));
    stream.data = s->myOfferBytes;
    stream.capacity = sizeof(s->myOfferBytes) * 8;
    stream.bitPos = 0;
    stream.error = FALSE;

    TradeCode_WriteBits(&stream, TRADE_CODE_FORMAT_VERSION, 4);
    TradeCode_WriteBits(&stream, TRADE_CODE_KIND_OFFER, 2);
    TradeCode_WriteBits(&stream, s->myNonce, 16);
    TradeCode_SerializeMon(&mon->box, &stream);

    paddedBits = ROUND_UP_TO_BYTE(stream.bitPos);
    padAmount = paddedBits - stream.bitPos;
    if (padAmount != 0)
        TradeCode_WriteBits(&stream, 0, padAmount);

    seal = TradeCode_SealOffer(stream.data, paddedBits);
    TradeCode_WriteBits(&stream, seal, 32);

    s->myOfferBits = stream.bitPos;
}

// The TradeCodeEntryValidator for the offer-code entry screen. Everything
// Step 3 needs is extracted here, since `decoded` is only valid for the
// duration of this call.
static enum TradeCodeEntryStatus TradeCodeSession_ValidateOfferEntry(struct TradeCodeBits *decoded)
{
    struct TradeCodeSessionState *s = sTradeCodeSessionPtr;
    u32 formatVersion, codeKind, paddedBits, actualSeal, expectedSeal, byteLen;
    u16 nonce;
    struct BoxPokemon tempMon;
    enum TradeCodeMonStatus monStatus;

    formatVersion = TradeCode_ReadBits(decoded, 4);
    codeKind = TradeCode_ReadBits(decoded, 2);
    nonce = (u16)TradeCode_ReadBits(decoded, 16);
    if (decoded->error)
        return TRADE_CODE_ENTRY_WRONG_LENGTH;
    if (formatVersion != TRADE_CODE_FORMAT_VERSION)
        return TRADE_CODE_ENTRY_WRONG_VERSION;
    if (codeKind != TRADE_CODE_KIND_OFFER)
        return TRADE_CODE_ENTRY_INVALID; // e.g. a confirm code typed into the offer field

    monStatus = TradeCode_DeserializeMon(decoded, &tempMon);
    if (monStatus != TRADE_CODE_MON_OK)
        return TRADE_CODE_ENTRY_INVALID;

    // Byte alignment, matching TradeCode_SealOffer's ceil(nBits/8) hashing; see
    // TradeCodeSession_BuildOffer.
    paddedBits = ROUND_UP_TO_BYTE(decoded->bitPos);
    if (paddedBits + 32 > decoded->capacity)
        return TRADE_CODE_ENTRY_WRONG_LENGTH; // truncated before the seal

    expectedSeal = TradeCode_SealOffer(decoded->data, paddedBits);
    decoded->bitPos = paddedBits;
    actualSeal = TradeCode_ReadBits(decoded, 32);
    if (decoded->error)
        return TRADE_CODE_ENTRY_WRONG_LENGTH;
    if (actualSeal != expectedSeal)
        return TRADE_CODE_ENTRY_INVALID; // typo, or a genuinely forged/tampered code

    if (TradeCode_IsOfferSealUsed(gSaveBlock2Ptr->pendingTrade.recentOfferSeals, actualSeal))
        return TRADE_CODE_ENTRY_ALREADY_USED;

    s->partnerBoxMon = tempMon;
    s->partnerNonce = nonce;
    s->partnerOtId = GetBoxMonData(&tempMon, MON_DATA_OT_ID);
    s->partnerOfferBits = paddedBits + 32;
    byteLen = (s->partnerOfferBits + 7) / 8;
    memcpy(s->partnerOfferBytes, decoded->data, byteLen);
    s->partnerOfferSeal = actualSeal;

    return TRADE_CODE_ENTRY_OK;
}

// Builds a confirm code's displayable text from a 28-bit tag. Shared by
// Step 3's reveal (TradeCodeSession_DoCommit) and the "view confirm code"
// option (TradeCodeSession_ViewConfirmCode), so the codeKind+tag packing
// documented at TradeCode_ConfirmTag stays in one place.
static void TradeCodeSession_EncodeConfirmTag(u32 tag, u8 *outEncoded)
{
    struct TradeCodeBits confirmStream;
    u8 confirmBuf[TRADE_CODE_SESSION_CONFIRM_BYTES];

    memset(confirmBuf, 0, sizeof(confirmBuf));
    confirmStream.data = confirmBuf;
    confirmStream.capacity = sizeof(confirmBuf) * 8;
    confirmStream.bitPos = 0;
    confirmStream.error = FALSE;
    TradeCode_WriteBits(&confirmStream, TRADE_CODE_KIND_CONFIRM, 2);
    TradeCode_WriteBits(&confirmStream, tag, 28);
    TradeCode_Encode(confirmBuf, confirmStream.bitPos, outEncoded);
}

// Step 3's commit: escrow, compute both confirm tags, force-save, and -
// only once the save reports success - reveal the confirm code. Returns
// FALSE (nothing further done) if TrySavingData didn't return SAVE_STATUS_
// OK, so the caller can show a retry prompt instead of pretending the
// point of no return was reached.
static bool8 TradeCodeSession_DoCommit(void)
{
    struct TradeCodeSessionState *s = sTradeCodeSessionPtr;
    struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][s->partySlot];
    u32 myTag, expectedTag;
    u8 saveStatus;
    u8 encoded[TRADE_CODE_CONFIRM_CHARS + 1];

    // 1. Escrow, mirroring src/daycare.c's StorePokemonInEmptyDaycareSlot
    // (copy mon->box out, ZeroMonData, CompactPartySlots,
    // CalculatePlayerPartyCount).
    ZeroMonData(mon);
    CompactPartySlots();
    CalculatePlayerPartyCount();

    memcpy(gSaveBlock2Ptr->pendingTrade.incoming, &s->partnerBoxMon, sizeof(struct BoxPokemon));

    // 2. Both confirm tags (see TradeCode_ConfirmTag): (mine, partner's) for
    // the tag revealed now, and swapped for the tag expected back from the
    // partner at Step 4.
    myTag = TradeCode_ConfirmTag(s->myOfferBytes, s->myOfferBits, s->myOtId, s->myNonce,
                                  s->partnerOfferBytes, s->partnerOfferBits, s->partnerOtId, s->partnerNonce);
    expectedTag = TradeCode_ConfirmTag(s->partnerOfferBytes, s->partnerOfferBits, s->partnerOtId, s->partnerNonce,
                                        s->myOfferBytes, s->myOfferBits, s->myOtId, s->myNonce);

    gSaveBlock2Ptr->pendingTrade.expectedConfirmTag = expectedTag;
    gSaveBlock2Ptr->pendingTrade.nonce = s->partnerNonce;
    gSaveBlock2Ptr->pendingTrade.partySlot = s->partySlot;
    TradeCode_RecordOfferSeal(gSaveBlock2Ptr->pendingTrade.recentOfferSeals, s->partnerOfferSeal);

    // Also persist my own offer/confirm codes verbatim, so "view offer code"/
    // "view confirm code" can redisplay them without `mon` (already escrowed)
    // or this session's struct (freed below). The buffers are sized identically
    // to their session-state counterparts, so a plain full-width memcpy is safe.
    gSaveBlock2Ptr->pendingTrade.myConfirmTag = myTag;
    gSaveBlock2Ptr->pendingTrade.myOfferBits = (u16)s->myOfferBits;
    gSaveBlock2Ptr->pendingTrade.myOfferSpecies = s->myOfferSpecies;
    memcpy(gSaveBlock2Ptr->pendingTrade.myOfferBytes, s->myOfferBytes, sizeof(gSaveBlock2Ptr->pendingTrade.myOfferBytes));
    memcpy(gSaveBlock2Ptr->pendingTrade.myOfferNickname, s->myOfferNickname, sizeof(gSaveBlock2Ptr->pendingTrade.myOfferNickname));

    gSaveBlock2Ptr->pendingTrade.state = TRADE_CODE_STATE_COMMITTED;

    // 3. Force-save, and reveal the confirm code only on success, so a
    // power-cut mid-save rolls back to pre-escrow with no code ever shown.
    saveStatus = TrySavingData(SAVE_NORMAL);
    if (saveStatus != SAVE_STATUS_OK)
        return FALSE;

    TradeCodeSession_EncodeConfirmTag(myTag, encoded);

    Free(s);
    sTradeCodeSessionPtr = NULL;
    // Step 4 (materialising the incoming mon) lives in trade_code_receive.c.
    // Pressing A on the display screen returns to CB2_ReturnToField.
    TradeCodeDisplay_Init(encoded, SPECIES_NONE, NULL, TRUE, CB2_ReturnToField);
    return TRUE;
}

static void TradeCodeSession_ShowSaveFailedPrompt(void)
{
    TradeCodePrompt_Init(gText_SaveError, FALSE, FALSE, &sTradeCodeSessionPtr->promptResult, CB2_TradeCodeSession_AfterSaveFailedAck);
}

static void CB2_TradeCodeSession_AfterSaveFailedAck(void)
{
    // Safe to retry unconditionally: every write DoCommit makes before the save
    // is either to gSaveBlock2Ptr (nothing reaches the save file until
    // TrySavingData succeeds) or idempotent (re-zeroing an empty party slot,
    // recomputing the same tags).
    if (!TradeCodeSession_DoCommit())
        TradeCodeSession_ShowSaveFailedPrompt();
}

static void CB2_TradeCodeSession_AfterChooseMon(void)
{
    struct TradeCodeSessionState *s = sTradeCodeSessionPtr;
    u8 slot = GetCursorSelectionMonId();
    struct Pokemon *mon;
    u8 encoded[TRADE_CODE_MAX_CHARS + 1];

    if (slot >= PARTY_SIZE)
    {
        // Cancelled from the party menu: nothing was shown or escrowed. Silent
        // abort, like other ChooseMonForTradingBoard callers (src/union_room.c).
        TradeCodeSession_AbortToField();
        return;
    }

    mon = &gParties[B_TRAINER_PLAYER][slot];

    // The party menu's CanRegisterMonForTradingBoard gate (src/trade.c)
    // rejects fork-forbidden species always, but eggs only without the National
    // Dex. Eggs are always rejected here.
    if (GetMonData(mon, MON_DATA_IS_EGG))
    {
        TradeCodePrompt_Init(sText_CantTradeEgg, FALSE, FALSE, &s->promptResult, CB2_TradeCodeSession_AfterRejectAck);
        return;
    }
    // Fusion Pokemon cannot be traded. A fused mon (e.g. Black/White Kyurem)
    // has its other half in gPokemonStoragePtr->fusions[], outside its
    // BoxPokemon, so TradeCode_SerializeMon cannot carry it and unfusing after
    // it has gone to a partner would fail or desync. IsFusionMon
    // (src/party_menu.c) returns UNFUSE_MON for a currently-merged mon;
    // FUSE_MON/SECOND_FUSE_MON carry no hidden state and trade normally.
    if (IsFusionMon(GetMonData(mon, MON_DATA_SPECIES)) == UNFUSE_MON)
    {
        TradeCodePrompt_Init(sText_CantTradeFusedMon, FALSE, FALSE, &s->promptResult, CB2_TradeCodeSession_AfterRejectAck);
        return;
    }
    if (TradeCodeSession_WouldLeavePartyEmpty(slot))
    {
        TradeCodePrompt_Init(sText_CantTradeLastMon, FALSE, FALSE, &s->promptResult, CB2_TradeCodeSession_AfterRejectAck);
        return;
    }

    s->partySlot = slot;
    s->myOtId = GetMonData(mon, MON_DATA_OT_ID);
    s->myNonce = (u16)Random32();
    TradeCodeSession_BuildOffer(mon);

    // Species/nickname are captured here, the last point this file has `mon`
    // before Step 3 escrows it. They are carried into pendingTrade at commit so
    // "view offer code" can show them alongside the redisplayed code.
    s->myOfferSpecies = GetMonData(mon, MON_DATA_SPECIES);
    GetMonData(mon, MON_DATA_NICKNAME, s->myOfferNickname);

    TradeCode_Encode(s->myOfferBytes, s->myOfferBits, encoded);
    TradeCodeDisplay_Init(encoded, s->myOfferSpecies, s->myOfferNickname, FALSE, CB2_TradeCodeSession_AfterOfferShown);
}

static void CB2_TradeCodeSession_AfterOfferShown(void)
{
    TradeCodeSession_ShowOfferReadyPrompt();
}

static void TradeCodeSession_ShowOfferReadyPrompt(void)
{
    TradeCodePrompt_Init(sText_ReadyForPartnerCode, TRUE, FALSE, &sTradeCodeSessionPtr->promptResult, CB2_TradeCodeSession_AfterOfferReadyPrompt);
}

static void CB2_TradeCodeSession_AfterOfferReadyPrompt(void)
{
    struct TradeCodeSessionState *s = sTradeCodeSessionPtr;

    if (s->promptResult == TRADE_CODE_PROMPT_YES)
    {
        s->entryBits.data = s->entryScratch;
        s->entryBits.capacity = sizeof(s->entryScratch) * 8;
        TradeCodeEntry_Init(&s->entryBits, 0, TradeCodeSession_ValidateOfferEntry, &s->entryStatus, CB2_TradeCodeSession_AfterOfferEntry);
    }
    else
    {
        TradeCodeSession_ShowCancelConfirm(TradeCodeSession_ShowOfferReadyPrompt);
    }
}

static void CB2_TradeCodeSession_AfterOfferEntry(void)
{
    struct TradeCodeSessionState *s = sTradeCodeSessionPtr;

    if (s->entryStatus != TRADE_CODE_ENTRY_OK)
    {
        // The only other status this callback can see is
        // TRADE_CODE_ENTRY_CANCELLED (B on an empty field); a failed validator
        // retries in place. Still pre-commit, so this goes to the same
        // cancel-confirm as the "ready?" prompt's "No" rather than dropping back
        // to the field on one B press.
        TradeCodeSession_ShowCancelConfirm(TradeCodeSession_ShowOfferReadyPrompt);
        return;
    }

    // Preview the reconstructed mon via BoxMonToMon into a dedicated
    // s->previewMon buffer, not gParties[B_TRAINER_OPPONENT_A][0]:
    // DoesMonOTMatchOwner() (pokemon_summary_screen.c) treats that array as an
    // active link battle and reads gLinkPlayers[]/GetMultiplayerId(), which
    // corrupted the summary screen's scratch buffers with no link session.
    // s->previewMon never aliases that array, so the non-link branch runs, which
    // is correct since this mon's OT is not the receiving player.
    BoxMonToMon(&s->partnerBoxMon, &s->previewMon);
    CalculateMonStats(&s->previewMon);
    ShowPokemonSummaryScreen(SUMMARY_MODE_LOCK_MOVES, &s->previewMon, 0, 0, CB2_TradeCodeSession_AfterPreview);
}

static void CB2_TradeCodeSession_AfterPreview(void)
{
    // The preview itself is the acceptance: there is no separate "accept this
    // offer?" prompt, since Step 3's irreversible commit prompt follows.
    TradeCodeSession_ShowCommitPrompt();
}

static void TradeCodeSession_ShowCommitPrompt(void)
{
    struct TradeCodeSessionState *s = sTradeCodeSessionPtr;
    struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][s->partySlot];

    // Recomputed every time this prompt is (re-)shown, including after
    // declining to cancel and looping back here - the offered mon is still
    // in the party at every point this can be reached from (nothing is
    // escrowed until YES is actually chosen), so this is always accurate.
    GetMonData(mon, MON_DATA_NICKNAME, gStringVar1);
    StripExtCtrlCodes(gStringVar1);
    GetBoxMonData(&s->partnerBoxMon, MON_DATA_NICKNAME, gStringVar2);
    StripExtCtrlCodes(gStringVar2);
    // TradeCodePrompt_Init does not expand placeholders, so {STR_VAR_1}/
    // {STR_VAR_2} are resolved here, before the copy.
    StringExpandPlaceholders(gStringVar4, sText_ConfirmCommit);
    TradeCodePrompt_Init(gStringVar4, TRUE, TRUE, &s->promptResult, CB2_TradeCodeSession_AfterCommitPrompt);
}

static void CB2_TradeCodeSession_AfterCommitPrompt(void)
{
    struct TradeCodeSessionState *s = sTradeCodeSessionPtr;

    if (s->promptResult == TRADE_CODE_PROMPT_YES)
    {
        // Defaults to NO (see TradeCodeSession_ShowCommitPrompt's own
        // TradeCodePrompt_Init call) - this is the irreversible step, and
        // an accidental double-A-press must not be able to confirm it.
        if (!TradeCodeSession_DoCommit())
            TradeCodeSession_ShowSaveFailedPrompt();
    }
    else
    {
        TradeCodeSession_ShowCancelConfirm(TradeCodeSession_ShowCommitPrompt);
    }
}

// Two more attendant menu options: re-display a code already shown once,
// without re-running any of the trade. Only meaningful once
// pendingTrade.state == TRADE_CODE_STATE_COMMITTED; the menu is reached either
// with no trade in progress or after Step 3's commit. Both are parameterless
// and return straight to CB2_ReturnToField.

void TradeCodeSession_ViewOfferCode(void)
{
    struct PendingTrade *pending = &gSaveBlock2Ptr->pendingTrade;
    u8 encoded[TRADE_CODE_MAX_CHARS + 1];

    if (pending->state != TRADE_CODE_STATE_COMMITTED)
    {
        TradeCodePrompt_Init(sText_NoTradeCodeToShow, FALSE, FALSE, &sViewCodePromptResult, CB2_ReturnToField);
        return;
    }

    TradeCode_Encode(pending->myOfferBytes, pending->myOfferBits, encoded);
    TradeCodeDisplay_Init(encoded, pending->myOfferSpecies, pending->myOfferNickname, FALSE, CB2_ReturnToField);
}

void TradeCodeSession_ViewConfirmCode(void)
{
    struct PendingTrade *pending = &gSaveBlock2Ptr->pendingTrade;
    u8 encoded[TRADE_CODE_CONFIRM_CHARS + 1];

    if (pending->state != TRADE_CODE_STATE_COMMITTED)
    {
        TradeCodePrompt_Init(sText_NoTradeCodeToShow, FALSE, FALSE, &sViewCodePromptResult, CB2_ReturnToField);
        return;
    }

    TradeCodeSession_EncodeConfirmTag(pending->myConfirmTag, encoded);
    TradeCodeDisplay_Init(encoded, SPECIES_NONE, NULL, TRUE, CB2_ReturnToField);
}
