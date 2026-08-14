#include "global.h"
#include "trade_code_session.h"
#include "trade_code.h"
#include "trade_code_display.h"
#include "trade_code_entry.h"
#include "trade_code_prompt.h"
#include "trade_code_receive.h"
#include "draft_mode.h"
#include "link_rfu.h"
#include "malloc.h"
#include "overworld.h"
#include "party_menu.h"
#include "pokemon.h"
#include "pokemon_storage_system.h"
#include "pokemon_summary_screen.h"
#include "random.h"
#include "save.h"
#include "script_pokemon_util.h"
#include "string_util.h"
#include "strings.h"
#include "constants/battle.h"
#include "constants/species.h"
#include "constants/union_room.h"

// Stage 7 of "Trading Codes.md": Steps 1-3 of the protocol. See
// include/trade_code_session.h for the scope/entry-point rationale.
//
// Every screen this file transitions through - ChooseMonForTradingBoard,
// TradeCodeEntry_Init, ShowPokemonSummaryScreen, TradeCodeDisplay_Init,
// and this stage's own TradeCodePrompt_Init - is a full-screen takeover
// that fully replaces gMain.callback2 and resets the task list as part of
// its own setup, chaining into the next one via an explicit MainCallback
// parameter (never through CB2_ReturnToField/the overworld's own field-
// callback machinery mid-session). This file never returns to a walkable
// overworld until the session genuinely ends - cancelled, or Step 3
// completes and the confirm code has been shown - which also happens to
// satisfy the plan doc's own "the session owns the screen - no returning
// to the overworld" wording more literally than an earlier draft of this
// file did.
//
// An earlier draft *did* bounce through CB2_ReturnToField + gFieldCallback
// for every native message/yes-no prompt, reusing the overworld's own
// standard dialogue-box system (window 0, DrawDialogueFrame, etc.) the way
// src/union_room.c's own native Task_-driven state machine does. That hung
// on real hardware: after Step 1's offer code screen, pressing A returned
// to a visibly-normal overworld (NPCs still animating) with the player
// locked and totally unresponsive. Even after finding and fixing one real
// bug in that approach (window 0 not being the field's own message-box
// window after a custom screen's own InitWindows call - see this stage's
// status block for the first fix attempt), the hang persisted, meaning
// something else about reusing CB2_ReturnToField's own field-callback
// machinery this way still isn't safe to rely on. Rather than keep
// patching around a class of problem this environment can't reproduce or
// debug interactively, every prompt now uses TradeCodePrompt_Init (Stage
// 7's own small addition) - a fully self-contained screen with no
// dependency on the overworld's own state at all, the same proven shape
// Stage 5/6 already use successfully.

//==========DEFINES==========//

// enum TradeCodeKind (TRADE_CODE_KIND_OFFER/_CONFIRM) used to be defined
// locally here (Stage 7) but has moved to trade_code.h - Stage 8's
// trade_code_receive.c needs the same two values for its own confirm-code
// validator, and an enum defined in a .c file with no header declaration
// has no visibility outside that translation unit. See trade_code.h's own
// comment on the enum for the full reasoning.

// Rounds up to a whole byte. See TradeCodeSession_BuildOffer's own comment
// for why this has to be byte alignment, not the plan doc's own suggested
// 5-bit (one Base32 symbol) alignment.
#define ROUND_UP_TO_BYTE(n) ((((n) + 7) / 8) * 8)

// Worst case per TRADE_CODE_MAX_CHARS' own derivation (include/config/
// trade_code.h): header-minus-presence (22) + worst-case mon payload (374,
// presence bit included) = 396, rounded up to a whole byte (see
// TradeCodeSession_BuildOffer's own comment for why byte alignment, not
// the doc's originally-suggested 5-bit alignment) -> 400, + a 32-bit seal
// = 432 bits = 54 bytes exactly. This is what this file ever writes into
// myOfferBytes, or copies out of a validated partner payload into
// partnerOfferBytes (both stop at paddedBits+32, never including any
// trailing Base32-symbol pad).
//
// Rounded up to 448 (56 bytes) rather than the exact 432, deliberately -
// this file can't be verified by a real build (see CLAUDE.md), and 374's
// own derivation lives in a different file's status block (Stage 5's),
// not re-proven bit-for-bit here. TradeCode_WriteBits/ReadBits already
// fail safe on a too-small buffer (latching an error flag, never
// overrunning it - see Stage 1), so this costs a few bytes of EWRAM per
// buffer to turn "silently truncates the one worst-case Pokemon that
// happens to hit this exactly" into "has slack," not "removes a check."
//
// Reuses TRADE_CODE_OFFER_PAYLOAD_BYTES (include/config/trade_code.h)
// rather than an independent literal - struct PendingTrade needs a buffer
// of this exact same shape post-Stage-10 (to persist a player's own
// already-built offer for the attendant's "view offer code" option), and
// the two would otherwise be two unlinked places encoding the same 56.
#define TRADE_CODE_SESSION_OFFER_MAX_BITS 448
#define TRADE_CODE_SESSION_OFFER_BYTES TRADE_CODE_OFFER_PAYLOAD_BYTES

// entryScratch (below) is different: it's TradeCodeEntry_Init's own outBits
// target (include/trade_code_entry.h), which that screen fills with
// whatever it decoded - up to TRADE_CODE_ENTRY_MAX_SYMBOLS (87, src/
// trade_code_entry.c) Base32 symbols' worth of *raw decoded bits*
// (87*5 = 435), which can run a few bits past TRADE_CODE_SESSION_OFFER_
// MAX_BITS thanks to TradeCode_Decode's own trailing Base32-alignment
// padding (see trade_code.h's TradeCode_Decode contract) - a real payload
// this file writes never has that trailing slack, but a partner's code as
// entered on the keyboard does. Sized to match src/trade_code_entry.c's
// own TRADE_CODE_ENTRY_SCRATCH_BYTES for exactly this reason (a smaller
// buffer here would make TradeCodeEntry_Init's own outBits-capacity guard
// reject a legitimate worst-case partner code as TRADE_CODE_ENTRY_WRONG_
// LENGTH before this file's validator ever saw it).
// Mirrors src/trade_code_entry.c's own TRADE_CODE_ENTRY_MAX_SYMBOLS -
// that constant is file-local to trade_code_entry.c (not part of trade_
// code_entry.h's public contract), so it can't be referenced directly;
// this is kept in one place and commented so the two can't silently drift
// without at least one obvious place to update.
#define TRADE_CODE_ENTRY_MAX_SYMBOLS_MIRROR 87
#define TRADE_CODE_SESSION_ENTRY_SCRATCH_BYTES ((TRADE_CODE_ENTRY_MAX_SYMBOLS_MIRROR * 5 + 7) / 8)

// A confirm code's own payload (codeKind 2 + a 28-bit tag = 30 bits, see
// the payload spec) is fixed and tiny - no worst-case derivation needed.
#define TRADE_CODE_SESSION_CONFIRM_BYTES 4

struct TradeCodeSessionState
{
    // ---- Step 1: the offer I generate ----
    u8 partySlot;
    u32 myOtId;
    u16 myNonce;
    u32 myOfferBits;    // exact bit length of header+mon+pad+seal
    u8 myOfferBytes[TRADE_CODE_SESSION_OFFER_BYTES];
    u16 myOfferSpecies;                          // post-Stage-10: carried into pendingTrade at commit, for "view offer code"'s redisplay icon
    u8 myOfferNickname[POKEMON_NAME_LENGTH + 1]; // post-Stage-10: same as above

    // ---- Step 2: the partner's offer, filled in by the validator ----
    struct BoxPokemon partnerBoxMon;
    u32 partnerOtId;
    u16 partnerNonce;
    u32 partnerOfferBits;
    u8 partnerOfferBytes[TRADE_CODE_SESSION_OFFER_BYTES];
    u32 partnerOfferSeal;

    // BoxMonToMon target for the preview screen (Step 2's "show a preview
    // screen" step) - deliberately NOT gParties[B_TRAINER_OPPONENT_A][0].
    // pokemon_summary_screen.c's DoesMonOTMatchOwner() special-cases that
    // exact array by pointer identity ("sMonSummaryScreen->monList.mons ==
    // gParties[B_TRAINER_OPPONENT_A]") to mean "we're in an active link
    // battle," and on that branch pulls the comparison OT from
    // gLinkPlayers[GetMultiplayerId() ^ 1] instead of the mon's own data -
    // there's no real link session here, so that reads meaningless
    // link-session state (confirmed by a controlled test: the player's own
    // known-good mon renders blank/garbled the exact same way once pushed
    // through BoxMonToMon into gParties[B_TRAINER_OPPONENT_A], despite every
    // field of the actual offer data checking out clean beforehand - see
    // Trading Codes.md's Stage 7 status block). A dedicated buffer here
    // means the pointer can never alias gParties[B_TRAINER_OPPONENT_A], so
    // DoesMonOTMatchOwner() takes its normal (correct, for a mon that
    // genuinely isn't the player's own) non-link branch instead.
    struct Pokemon previewMon;

    // outBits target for TradeCodeEntry_Init - see trade_code_entry.h.
    // Its own contents aren't used after the fact (the validator already
    // did the real extraction into the fields above, since `decoded` is
    // only valid for the duration of the validator call) - it exists
    // purely because TradeCodeEntry_Init requires a caller-owned buffer.
    struct TradeCodeBits entryBits;
    u8 entryScratch[TRADE_CODE_SESSION_ENTRY_SCRATCH_BYTES];
    enum TradeCodeEntryStatus entryStatus;

    // ---- TradeCodePrompt_Init's own out-param, and cancel-confirm bookkeeping ----
    enum TradeCodePromptResult promptResult;
    MainCallback cancelReturnCallback; // where "No" at the cancel-confirm goes back to
};

//==========EWRAM==========//
static EWRAM_DATA struct TradeCodeSessionState *sTradeCodeSessionPtr = NULL;
// Post-Stage-10: TradeCodePrompt_Init's out-param for the two standalone
// "view code" entry points below. Unlike every other TradeCodePrompt_Init
// call in this file, neither of those has a live sTradeCodeSessionPtr to
// hang this off - they're reachable directly from the attendant's menu,
// entirely outside the Steps 1-3 session state machine - so this gets its
// own small, permanent slot instead. Never actually read back (both calls
// are ACK-only - hasYesNo FALSE - so the only possible result is TRADE_
// CODE_PROMPT_ACK), it exists purely because TradeCodePrompt_Init requires
// a caller-owned out-pointer that outlives the call.
static enum TradeCodePromptResult sViewCodePromptResult;

//==========STATIC=DEFINES==========//
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

//==========CONST=DATA==========//
// CableClub_Text_NeedTwoMonsToTrade / _CantTradeEnigmaBerry (data/text/
// cable_club.inc) carry the equivalent vanilla wording for these same two
// gates, but only exist as script-land .string symbols with no C
// declaration anywhere - there's no precedent in this codebase for a
// native C file reaching across to a script text symbol like that, so
// these are this file's own C string constants instead, matching that
// existing phrasing rather than referencing it directly.
static const u8 sText_NeedTwoMons[]         = _("For trading, you must have at\nleast two Pokémon with you.");
static const u8 sText_CantTradeEnigmaBerry[] = _("A Pokémon holding the {STR_VAR_1}\nBerry can't be traded.");
static const u8 sText_CantTradeEgg[]        = _("An Egg can't be traded like\nthis.");
static const u8 sText_CantTradeLastMon[]    = _("You can't trade your last\nPokémon!");
// Stage 11 (dev decision, Trading Codes.md's "Nuzlocke" bullet: "Nuzlocke
// should disable trading entirely"). Matches this codebase's own existing
// convention for the flag (a plain gSaveBlock1Ptr->nuzlockeModeEnabled
// check at the call site, e.g. src/overworld.c/src/daycare.c/src/item_use.c)
// rather than the docs/ai/systems/NUZLOCKE.md file's own aspirational
// GameRules::CanX() wrapper wording, which no system in this codebase
// (checked before writing this) actually implements.
static const u8 sText_CantTradeNuzlocke[]   = _("Trading isn't allowed during\na Nuzlocke run.");
// Draft Mode.md §3d: in-game trades (CreateInGameTradePokemon) are untouched
// -- they swap in place and don't change party size -- but this attendant-
// initiated code-trade flow is a real acquisition path and is refused
// outright, same shape as the Nuzlocke gate right above.
static const u8 sText_CantTradeDraft[]      = _("Trading isn't allowed during\na Draft run.");
static const u8 sText_CantTradeFusedMon[]   = _("A fused Pokémon can't be\ntraded like this.");
static const u8 sText_ReadyForPartnerCode[] = _("Ready to enter your partner's\ntrade code?");
// This screen's window (see trade_code_prompt.c's sTradeCodePromptWindow
// Templates) is only 2 text-lines tall, same as every other message string
// in this file - all of which are exactly 2 lines. This one alone has 5
// lines' worth of content, so plain \n (a same-page line break) isn't
// enough; it needs \p (the standard field-message "wait for A, then clear
// and continue" page break - see charmap.txt's own "'\p' = FB @ new
// paragraph") between each 2-line page. AddTextPrinterForMessage (called
// by trade_code_prompt.c, same as any vanilla NPC message box) already
// understands \p natively - this is a plain content fix, not a new code
// path - the previous version simply had 5 lines of \n-joined text
// silently overflowing a 2-line window with no pause in between.
static const u8 sText_ConfirmCommit[]       = _("{STR_VAR_1} will be given up\nnow. You will only receive\p{STR_VAR_2} once you enter\nyour partner's confirm code.\pContinue?");
// Post-Stage-10: shown by both TradeCodeSession_ViewOfferCode and
// TradeCodeSession_ViewConfirmCode when there's nothing to show
// (pendingTrade.state != TRADE_CODE_STATE_COMMITTED) - the attendant's
// menu is reachable with no trade in progress at all, and this is that
// case's own plain "nothing waiting" message, the same shape trade_code_
// receive.h's own contract already uses this file's "no trade pending"
// case for.
static const u8 sText_NoTradeCodeToShow[]   = _("You don't have a trade code\nto show right now.");
static const u8 sText_CancelConfirm[]       = _("Cancel this trade? Your\npartner may be waiting.");

//==========UI=SETUP==========//
void TradeCodeSession_Start(void)
{
    // Stage 11 (dev decision): Nuzlocke disables trading entirely. Checked
    // before even the COMMITTED-trade redirect below - nuzlockeModeEnabled
    // is a new-game-only setting (see ApplyPendingNewGameSettings, src/
    // new_game_settings_menu.c) that never changes mid-save, so a Nuzlocke
    // save can never have a genuinely COMMITTED trade in the first place
    // (Step 1 would already have been refused here). This doesn't touch
    // Stage 9's own boot hook (src/overworld.c calls TradeCodeReceive_Start
    // directly, never through this function) so a stale COMMITTED trade
    // left over from before this gate existed still resolves normally on
    // the next boot regardless of what this check does.
    if (gSaveBlock1Ptr->nuzlockeModeEnabled)
    {
        if ((sTradeCodeSessionPtr = AllocZeroed(sizeof(struct TradeCodeSessionState))) == NULL)
            return;
        TradeCodePrompt_Init(sText_CantTradeNuzlocke, FALSE, FALSE, &sTradeCodeSessionPtr->promptResult, CB2_TradeCodeSession_AfterGateFailAck);
        return;
    }

    // Draft Mode.md §3d: Draft disables this code-trade flow entirely too,
    // for the same reason as Nuzlocke above -- Draft and Nuzlocke are
    // mutually exclusive (src/new_game_settings_menu.c), so this and the
    // block above never both apply, but each stands on its own here rather
    // than folding into a combined condition, so each gets its own message.
    if (Draft_IsEnabled())
    {
        if ((sTradeCodeSessionPtr = AllocZeroed(sizeof(struct TradeCodeSessionState))) == NULL)
            return;
        TradeCodePrompt_Init(sText_CantTradeDraft, FALSE, FALSE, &sTradeCodeSessionPtr->promptResult, CB2_TradeCodeSession_AfterGateFailAck);
        return;
    }

    // Post-Stage-10 fix: the attendant is this feature's only entry point,
    // so it has to also be where a player with an already-COMMITTED trade
    // goes to enter their partner's confirm code - see this function's own
    // header comment (include/trade_code_session.h) for why. Checked before
    // anything else, and before the AllocZeroed below - starting a brand
    // new offer while one mon is already escrowed awaiting Step 4 would be
    // wrong even setting the UX question aside, and this session's own
    // struct isn't needed at all for that path.
    if (gSaveBlock2Ptr->pendingTrade.state == TRADE_CODE_STATE_COMMITTED)
    {
        TradeCodeReceive_Start(CB2_ReturnToField);
        return;
    }

    if ((sTradeCodeSessionPtr = AllocZeroed(sizeof(struct TradeCodeSessionState))) == NULL)
        return; // couldn't even allocate - nothing was touched, nothing to undo

    // Mirrors CableClub_EventScript_CheckPartyTradeRequirements
    // (data/scripts/cable_club.inc) - the same two gates the old
    // link-trade path already runs before it will even attempt a trade.
    // DoesPartyHaveEnigmaBerry() already fills gStringVar1 with the
    // berry's name on TRUE (see src/script_pokemon_util.c) - no separate
    // placeholder setup needed here.
    if (CalculatePlayerPartyCount() < 2)
    {
        TradeCodePrompt_Init(sText_NeedTwoMons, FALSE, FALSE, &sTradeCodeSessionPtr->promptResult, CB2_TradeCodeSession_AfterGateFailAck);
        return;
    }
    if (DoesPartyHaveEnigmaBerry())
    {
        // DoesPartyHaveEnigmaBerry() already filled gStringVar1 with the
        // berry's name - TradeCodePrompt_Init itself only StringCopy's its
        // message (matching TradeCodeDisplay_Init's own contract, see
        // trade_code_display.c), it doesn't expand placeholders, so the
        // {STR_VAR_1} substitution has to happen here before the copy.
        StringExpandPlaceholders(gStringVar4, sText_CantTradeEnigmaBerry);
        TradeCodePrompt_Init(gStringVar4, FALSE, FALSE, &sTradeCodeSessionPtr->promptResult, CB2_TradeCodeSession_AfterGateFailAck);
        return;
    }

    // Populates gHostRfuGameData.compatibility.hasNationalDex (via
    // IsNationalPokedexEnabled() - a plain local save-flag read, no RFU/
    // link dependency - confirmed by reading src/link_rfu_3.c's
    // InitHostRfuGameData before relying on it) so PARTY_MENU_TYPE_UNION_
    // ROOM_REGISTER's own in-menu CanRegisterMonForTradingBoard gate
    // (src/party_menu.c's CursorCb_Register) behaves correctly instead of
    // defaulting to "no National Dex" - the rest of that struct (activity/
    // partnerInfo/etc.) is irrelevant here, this screen never touches
    // RFU/link state otherwise.
    SetHostRfuGameData(ACTIVITY_NONE, 0, FALSE);
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

// Mirrors src/trade.c's own (file-local) CanTradeSelectedMon's numMonsLeft
// check - the real precedent for "can this specific mon be traded away"
// used by the trade system itself, closer than CableClub_EventScript_
// CheckPartyTradeRequirements's own cruder ">=2 total" gate (which doesn't
// exclude eggs from the count). Reimplemented locally since CanTrade
// SelectedMon is static to trade.c; this only borrows its "does at least
// one real (non-egg) mon remain" shape, not its National-Dex-gated
// cross-cartridge compatibility checks, which don't apply to an offline
// trade with no live partner-version negotiation.
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

// Builds the full offer payload (header + TradeCode_SerializeMon's own
// fields + zero-pad to a byte boundary + the 32-bit seal) into
// sTradeCodeSessionPtr->myOfferBytes/myOfferBits.
//
// The pad-before-seal step is this stage resolving the discrepancy Stage
// 3's own status block flagged and left for here - but not quite the way
// that status block suggested. Stage 3 recommended padding to a 5-bit
// (one Base32 symbol) boundary, reasoning about TradeCode_Decode's own
// symbol-granularity padding. Reading TradeCode_SealOffer's actual body
// (src/trade_code.c) before relying on it surfaced a stricter requirement
// than that: it hashes ceil(nBits/8) whole *bytes*, so the seal's own
// documented precondition ("data's trailing bits past nBits in the final
// partial byte must be zero") means nBits has to be *byte*-aligned, not
// just 5-bit-aligned, or the sender and a receiver holding the same
// payload's full decoded bytes (mon fields immediately followed by the
// real seal, not zeros) would hash different byte content for the same
// logical boundary and never agree. Rounding up to a byte (ROUND_UP_TO_
// BYTE, not the doc's own suggested ROUND_UP_TO_5) sidesteps this
// entirely: there's no partial final byte left to reason about, so
// TradeCode_SealOffer's precondition is trivially satisfied on both ends.
// This still resolves Stage 3's original concern as a side effect (a
// byte-aligned boundary is unambiguous regardless of anything TradeCode_
// Decode does with 5-bit Base32 symbol padding at the very end of the
// whole code). Both this function and TradeCodeSession_ValidateOfferEntry
// derive the same paddedBits the same way (round up TradeCode_
// SerializeMon's own end-of-mon-fields bit position), so sender and
// receiver always agree on exactly where the seal starts.
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

// The TradeCodeEntryValidator for the offer-code entry screen (Stage 6).
// Everything Step 3 will need is extracted here, not re-derived later -
// `decoded` is only valid for the duration of this call (see trade_code_
// entry.h's own contract), so this is the one and only chance to copy
// anything out of it.
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

    // See TradeCodeSession_BuildOffer's own comment for why this has to be
    // byte alignment (matching TradeCode_SealOffer's own ceil(nBits/8)
    // byte-hashing) for sender and receiver to agree on the seal's start
    // bit with no ambiguity.
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

// Builds a confirm code's displayable text from a 28-bit tag - shared by
// Step 3's own reveal (TradeCodeSession_DoCommit, below) and the
// attendant's post-Stage-10 "view confirm code" option (TradeCodeSession_
// ViewConfirmCode). Both need the exact same codeKind+tag packing
// TradeCode_ConfirmTag's own comment (include/trade_code.h) documents -
// factored out rather than duplicated a second time, a real place for the
// two to quietly drift apart otherwise.
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

    // 1. Escrow - mirrors src/daycare.c's own StorePokemonInEmptyDaycareSlot
    // (mon->box copied out, then ZeroMonData + CompactPartySlots +
    // CalculatePlayerPartyCount) exactly, the established pattern for
    // "remove this party mon and account for the gap."
    ZeroMonData(mon);
    CompactPartySlots();
    CalculatePlayerPartyCount();

    memcpy(gSaveBlock2Ptr->pendingTrade.incoming, &s->partnerBoxMon, sizeof(struct BoxPokemon));

    // 2. Both confirm tags - see include/trade_code.h's own TradeCode_
    // ConfirmTag comment: call once with (mine, partner's) for the tag
    // revealed to me now, and once with the two swapped for the tag I
    // expect back from my partner later (Stage 8's Step 4).
    myTag = TradeCode_ConfirmTag(s->myOfferBytes, s->myOfferBits, s->myOtId, s->myNonce,
                                  s->partnerOfferBytes, s->partnerOfferBits, s->partnerOtId, s->partnerNonce);
    expectedTag = TradeCode_ConfirmTag(s->partnerOfferBytes, s->partnerOfferBits, s->partnerOtId, s->partnerNonce,
                                        s->myOfferBytes, s->myOfferBits, s->myOtId, s->myNonce);

    gSaveBlock2Ptr->pendingTrade.expectedConfirmTag = expectedTag;
    gSaveBlock2Ptr->pendingTrade.nonce = s->partnerNonce;
    gSaveBlock2Ptr->pendingTrade.partySlot = s->partySlot;
    TradeCode_RecordOfferSeal(gSaveBlock2Ptr->pendingTrade.recentOfferSeals, s->partnerOfferSeal);

    // Post-Stage-10: also persist my own offer/confirm codes verbatim, so
    // the attendant's "view offer code"/"view confirm code" options can
    // redisplay either one later without `mon` (already escrowed above by
    // the time either option could ever be reached) or this session's own
    // struct (freed a few lines down) still existing. Both buffers are
    // sized identically to their session-state counterparts (TRADE_CODE_
    // SESSION_OFFER_BYTES == TRADE_CODE_OFFER_PAYLOAD_BYTES, POKEMON_NAME_
    // LENGTH+1 either way), so this is a plain full-width memcpy, no
    // truncation to reason about.
    gSaveBlock2Ptr->pendingTrade.myConfirmTag = myTag;
    gSaveBlock2Ptr->pendingTrade.myOfferBits = (u16)s->myOfferBits;
    gSaveBlock2Ptr->pendingTrade.myOfferSpecies = s->myOfferSpecies;
    memcpy(gSaveBlock2Ptr->pendingTrade.myOfferBytes, s->myOfferBytes, sizeof(gSaveBlock2Ptr->pendingTrade.myOfferBytes));
    memcpy(gSaveBlock2Ptr->pendingTrade.myOfferNickname, s->myOfferNickname, sizeof(gSaveBlock2Ptr->pendingTrade.myOfferNickname));

    gSaveBlock2Ptr->pendingTrade.state = TRADE_CODE_STATE_COMMITTED;

    // 3. Force-save, and only reveal the confirm code on success - the
    // whole point being that a power-cut mid-save rolls back to pre-
    // escrow on this cart, with no confirm code ever having been shown.
    saveStatus = TrySavingData(SAVE_NORMAL);
    if (saveStatus != SAVE_STATUS_OK)
        return FALSE;

    TradeCodeSession_EncodeConfirmTag(myTag, encoded);

    Free(s);
    sTradeCodeSessionPtr = NULL;
    // Step 4 (materialising the incoming mon) is Stage 8's job - this
    // stage's own scope ends here, once the confirm code has been shown.
    // Pressing A on Stage 5's display screen returns straight to
    // CB2_ReturnToField, same as any other normal field return.
    TradeCodeDisplay_Init(encoded, SPECIES_NONE, NULL, TRUE, CB2_ReturnToField);
    return TRUE;
}

static void TradeCodeSession_ShowSaveFailedPrompt(void)
{
    TradeCodePrompt_Init(gText_SaveError, FALSE, FALSE, &sTradeCodeSessionPtr->promptResult, CB2_TradeCodeSession_AfterSaveFailedAck);
}

static void CB2_TradeCodeSession_AfterSaveFailedAck(void)
{
    // Safe to just retry unconditionally: every write TradeCodeSession_
    // DoCommit makes before the save call is either to gSaveBlock2Ptr
    // (nothing reaches the physical save file until TrySavingData itself
    // succeeds) or idempotent (re-zeroing an already-empty party slot,
    // recomputing the same deterministic tags) - there's nothing to roll
    // back.
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
        // Cancelled from the party menu itself - nothing was ever shown or
        // escrowed. Silent abort, matching every other ChooseMonForTrading
        // Board caller's own cancel behaviour (e.g. src/union_room.c).
        TradeCodeSession_AbortToField();
        return;
    }

    mon = &gParties[B_TRAINER_PLAYER][slot];

    // PARTY_MENU_TYPE_UNION_ROOM_REGISTER's own in-menu gate (CanRegister
    // MonForTradingBoard) already rejects fork-forbidden species
    // unconditionally, and eggs *only* when the player lacks the National
    // Dex (see src/trade.c's CanRegisterMonForTradingBoard) - the doc's own
    // "Reject eggs" bullet is unconditional, so this is checked again here
    // regardless of National Dex status.
    if (GetMonData(mon, MON_DATA_IS_EGG))
    {
        TradeCodePrompt_Init(sText_CantTradeEgg, FALSE, FALSE, &s->promptResult, CB2_TradeCodeSession_AfterRejectAck);
        return;
    }
    // Stage 11 (dev decision, Trading Codes.md's "Fusions" bullet: "Do not
    // allow fusion pokemon"). A mon in its fused form (e.g. Black/White
    // Kyurem) has its "other half" sitting in gPokemonStoragePtr->
    // fusions[], entirely outside this mon's own BoxPokemon data -
    // TradeCode_SerializeMon has no way to carry that along, and unfusing
    // it back on this cart after it's already gone to a partner would
    // either silently fail or desync from whatever the receiving cart
    // reconstructs. IsFusionMon (src/party_menu.c, declared in party_menu.h
    // for this exact cross-file use) already tracks this for the item-
    // based fuse/unfuse UI - UNFUSE_MON is specifically its "currently
    // merged" return value; FUSE_MON/SECOND_FUSE_MON (an ordinary,
    // not-yet-fused Reshiram/Zekrom/etc.) carry no hidden state and trade
    // normally, so only UNFUSE_MON is rejected here.
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

    // Species/nickname captured here (not re-read later) for the same
    // reason myOtId/myNonce already are - this is the one point this file
    // still has `mon` itself, before Step 3 escrows it away. Carried into
    // pendingTrade at commit so the attendant's post-Stage-10 "view offer
    // code" option has something to show alongside the redisplayed code,
    // matching what this same screen showed the first time.
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
        // The only other status TradeCodeEntry_Init's callback can report
        // is TRADE_CODE_ENTRY_CANCELLED (B on an empty field) - a failed
        // validator retries in place without leaving the screen (see
        // trade_code_entry.h). Still pre-commit, nothing escrowed yet -
        // routed to the same cancel-confirm the "ready?" prompt's own "No"
        // uses, rather than silently dropping back to the field on one B
        // press (matches the doc's "Cancel... with a confirm" for this
        // state).
        TradeCodeSession_ShowCancelConfirm(TradeCodeSession_ShowOfferReadyPrompt);
        return;
    }

    // Preview the reconstructed mon (Step 2 of the doc: "show a preview
    // screen"). BoxMonToMon into a dedicated s->previewMon buffer, not
    // gParties[B_TRAINER_OPPONENT_A][0] - pokemon_summary_screen.c's
    // DoesMonOTMatchOwner() special-cases that exact array by pointer
    // identity to mean "we're in an active link battle" and reads
    // gLinkPlayers[]/GetMultiplayerId() instead of the mon's own data on
    // that branch. There's no real link session here, so that read
    // meaningless state and corrupted the summary screen's own scratch
    // buffers (root-caused this Stage - see Trading Codes.md's Stage 7
    // status block for the full diagnostic trail: every field of the
    // deserialized mon checked out clean, and a controlled test proved the
    // player's own known-good mon broke the exact same way once pushed
    // through gParties[B_TRAINER_OPPONENT_A], isolating the bug to that
    // array specifically rather than anything TradeCode_DeserializeMon
    // produced). s->previewMon can never alias that array, so
    // DoesMonOTMatchOwner() takes its normal non-link branch instead -
    // correctly, since this mon's OT genuinely isn't the receiving player.
    BoxMonToMon(&s->partnerBoxMon, &s->previewMon);
    CalculateMonStats(&s->previewMon);
    ShowPokemonSummaryScreen(SUMMARY_MODE_LOCK_MOVES, &s->previewMon, 0, 0, CB2_TradeCodeSession_AfterPreview);
}

static void CB2_TradeCodeSession_AfterPreview(void)
{
    // The preview itself IS the acceptance, per the doc's own Step 2
    // wording ("show a preview screen... State -> PARTNER_OFFER_ACCEPTED") -
    // no separate "accept this offer?" prompt once the player has looked
    // at it and pressed B to move on; the very next thing shown is Step
    // 3's own irreversible commit prompt, which already asks a yes/no
    // question of its own.
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
    // Same reasoning as TradeCodeSession_Start's own Enigma Berry message -
    // TradeCodePrompt_Init doesn't expand placeholders itself, so {STR_VAR_
    // 1}/{STR_VAR_2} have to be resolved here, before the copy.
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

//==========VIEW=CODE=(post-Stage-10)==========//
// Two more attendant menu options, alongside TradeCodeSession_Start itself:
// re-display a code the player has already been shown once, without
// re-running any part of the trade. Both are only ever meaningful once
// pendingTrade.state == TRADE_CODE_STATE_COMMITTED - the attendant's menu
// is reached either with no trade in progress at all, or after Step 3's
// commit (TradeCodeSession_Start's own screens own the entire in-between,
// per include/trade_code_session.h's own comment), so there's no third
// state either of these could usefully distinguish. Neither takes or
// returns anything - same parameterless `special`-callable shape as every
// other real entry point in this feature - and both go straight back to
// CB2_ReturnToField, matching TradeCodeSession_DoCommit's own confirm-code
// reveal this file already uses that exact callback for.

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
