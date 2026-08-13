#include "global.h"
#include "trade_code_session.h"
#include "trade_code.h"
#include "trade_code_display.h"
#include "trade_code_entry.h"
#include "event_object_lock.h"
#include "field_screen_effect.h"
#include "link_rfu.h"
#include "malloc.h"
#include "menu.h"
#include "overworld.h"
#include "palette.h"
#include "party_menu.h"
#include "pokemon.h"
#include "pokemon_storage_system.h"
#include "pokemon_summary_screen.h"
#include "random.h"
#include "save.h"
#include "script.h"
#include "script_pokemon_util.h"
#include "sound.h"
#include "string_util.h"
#include "strings.h"
#include "task.h"
#include "constants/battle.h"
#include "constants/songs.h"
#include "constants/species.h"
#include "constants/union_room.h"

// Stage 7 of "Trading Codes.md": Steps 1-3 of the protocol. See
// include/trade_code_session.h for the scope/entry-point rationale.
//
// Screen transitions in this file fall into two shapes:
//  - Full-screen takeovers (ChooseMonForTradingBoard, TradeCodeEntry_Init,
//    ShowPokemonSummaryScreen, TradeCodeDisplay_Init) each fully replace
//    gMain.callback2 and reset the task list as part of their own setup
//    (mirrors ui_stat_editor.c's convention, already relied on by Stage
//    5/6's own debug wiring chaining straight from one into the next with
//    no explicit teardown in between) - so this file never needs to
//    DestroyTask() before calling into one of them.
//  - Native field prompts (a plain message, or a yes/no) reuse the
//    overworld's own standard dialogue-box system (LoadMessageBoxAndBorderGfx
//    / DrawDialogueFrame / AddTextPrinterForMessage / DisplayYesNoMenu* /
//    Menu_ProcessInputNoWrapClearOnChoose - all public, all already used
//    this same way by src/union_room.c's own native Task_-driven state
//    machine, whose file-local PrintOnTextbox/UnionRoomHandleYesNo this
//    file's TradeCodeSession_PrintMessage/_HandleYesNo mirror). Window 0
//    and its dialogue-frame graphics are only guaranteed valid while
//    CB2_Overworld is the active main callback - never inside this
//    feature's own custom BG screens - so every prompt in this file is
//    driven by one shared Task (Task_TradeCodeSession_FieldUI) reached via
//    TradeCodeSession_GoToField(), which always re-enters through
//    CB2_ReturnToField + gFieldCallback first. The one exception is this
//    module's own very first entry (TradeCodeSession_Start): called
//    directly from the debug menu (src/debug.c) after Debug_DestroyMenu_
//    Full(), which never touches gMain.callback2 (see that function) - so
//    CB2_Overworld is already active and the bounce isn't needed there.

//==========DEFINES==========//

enum TradeCodeKind
{
    TRADE_CODE_KIND_OFFER   = 0,
    TRADE_CODE_KIND_CONFIRM = 1,
};

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
#define TRADE_CODE_SESSION_OFFER_MAX_BITS 448
#define TRADE_CODE_SESSION_OFFER_BYTES ((TRADE_CODE_SESSION_OFFER_MAX_BITS + 7) / 8)

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

enum TradeCodeSessionUiStep
{
    UI_STEP_GATE_CHECK,       // party-count / Enigma Berry gate, then ChooseMonForTradingBoard
    UI_STEP_MESSAGE_THEN_ABORT, // print s->pendingMessage, wait for A, unlock + done
    UI_STEP_OFFER_READY,      // "Ready to enter your partner's trade code?"
    UI_STEP_COMMIT,           // the irreversible commit prompt (doc's own wording)
    UI_STEP_CANCEL_CONFIRM,   // "Cancel this trade? Your partner may be waiting."
    UI_STEP_SAVE_FAILED,      // TrySavingData didn't return SAVE_STATUS_OK - retry on A
};

struct TradeCodeSessionState
{
    // ---- Step 1: the offer I generate ----
    u8 partySlot;
    u32 myOtId;
    u16 myNonce;
    u32 myOfferBits;    // exact bit length of header+mon+pad+seal
    u8 myOfferBytes[TRADE_CODE_SESSION_OFFER_BYTES];

    // ---- Step 2: the partner's offer, filled in by the validator ----
    struct BoxPokemon partnerBoxMon;
    u32 partnerOtId;
    u16 partnerNonce;
    u32 partnerOfferBits;
    u8 partnerOfferBytes[TRADE_CODE_SESSION_OFFER_BYTES];
    u32 partnerOfferSeal;

    // outBits target for TradeCodeEntry_Init - see trade_code_entry.h.
    // Its own contents aren't used after the fact (the validator already
    // did the real extraction into the fields above, since `decoded` is
    // only valid for the duration of the validator call) - it exists
    // purely because TradeCodeEntry_Init requires a caller-owned buffer.
    struct TradeCodeBits entryBits;
    u8 entryScratch[TRADE_CODE_SESSION_ENTRY_SCRATCH_BYTES];
    enum TradeCodeEntryStatus entryStatus;

    // ---- native field prompt bookkeeping (Task_TradeCodeSession_FieldUI) ----
    u8 uiStep;                    // enum TradeCodeSessionUiStep
    u8 promptPhase;                // 0 = printing the message, 1 = awaiting input
    u8 msgState;                   // TradeCodeSession_PrintMessage's own state
    u8 yesNoState;                 // TradeCodeSession_HandleYesNo's own state
    u8 cancelReturnStep;           // where UI_STEP_CANCEL_CONFIRM's "No" goes back to
    const u8 *pendingMessage;      // for UI_STEP_MESSAGE_THEN_ABORT
};

//==========EWRAM==========//
static EWRAM_DATA struct TradeCodeSessionState *sTradeCodeSessionPtr = NULL;

//==========STATIC=DEFINES==========//
static void FieldCB_TradeCodeSession_Continue(void);
static void Task_TradeCodeSession_FieldUI(u8 taskId);
static void TradeCodeSession_GotoStep(enum TradeCodeSessionUiStep step);
static void TradeCodeSession_GoToField(enum TradeCodeSessionUiStep step);
static void TradeCodeSession_EndReturnToField(u8 taskId);
static bool8 TradeCodeSession_PrintMessage(u8 *state, const u8 *str);
static s8 TradeCodeSession_HandleYesNo(u8 *state, bool8 defaultNo);
static bool8 TradeCodeSession_WouldLeavePartyEmpty(u8 slot);
static void TradeCodeSession_BuildOffer(struct Pokemon *mon);
static enum TradeCodeEntryStatus TradeCodeSession_ValidateOfferEntry(struct TradeCodeBits *decoded);
static bool8 TradeCodeSession_DoCommit(void);
static void CB2_TradeCodeSession_AfterChooseMon(void);
static void CB2_TradeCodeSession_AfterOfferShown(void);
static void CB2_TradeCodeSession_AfterOfferEntry(void);
static void CB2_TradeCodeSession_AfterPreview(void);

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
static const u8 sText_ReadyForPartnerCode[] = _("Ready to enter your partner's\ntrade code?");
static const u8 sText_ConfirmCommit[]       = _("{STR_VAR_1} will be given up\nnow. You will only receive\n{STR_VAR_2} once you enter\nyour partner's confirm code.\nContinue?");
static const u8 sText_CancelConfirm[]       = _("Cancel this trade? Your\npartner may be waiting.");

//==========UI=SETUP==========//
void TradeCodeSession_Start(void)
{
    if ((sTradeCodeSessionPtr = AllocZeroed(sizeof(struct TradeCodeSessionState))) == NULL)
        return; // couldn't even allocate - nothing was touched, nothing to undo

    sTradeCodeSessionPtr->uiStep = UI_STEP_GATE_CHECK;
    // Called directly from the debug menu, right after Debug_DestroyMenu_
    // Full() - which never changes gMain.callback2 (see that function) -
    // so CB2_Overworld is already the active main callback and this task
    // can be created directly, with no CB2_ReturnToField bounce needed for
    // this one first entry (see this file's own top-of-file comment for
    // why every *later* re-entry does need that bounce).
    LockPlayerFieldControls();
    CreateTask(Task_TradeCodeSession_FieldUI, 10);
}

static void FieldCB_TradeCodeSession_Continue(void)
{
    LockPlayerFieldControls();
    FadeInFromBlack();
    CreateTask(Task_TradeCodeSession_FieldUI, 10);
}

static void TradeCodeSession_GotoStep(enum TradeCodeSessionUiStep step)
{
    struct TradeCodeSessionState *s = sTradeCodeSessionPtr;

    s->uiStep = step;
    s->promptPhase = 0;
    s->msgState = 0;
    s->yesNoState = 0;
}

// Re-enters CB2_Overworld (if not already there) before showing a native
// message/yes-no prompt - see this file's top-of-file comment for why.
static void TradeCodeSession_GoToField(enum TradeCodeSessionUiStep step)
{
    TradeCodeSession_GotoStep(step);
    gFieldCallback = FieldCB_TradeCodeSession_Continue;
    SetMainCallback2(CB2_ReturnToField);
}

// The session is over (cancelled, or a gate/rejection message was
// dismissed) - give full control back, exactly the way FieldCB_
// ReturnToFieldNoScript's own Task_ReturnToFieldNoScript does (this file
// doesn't reuse that pair directly since it's file-local to src/
// field_screen_effect.c, but LockPlayerFieldControls/UnlockPlayerField
// Controls are a plain flag - not a counter, confirmed by reading src/
// script.c - so locking once up front and unlocking exactly once here,
// regardless of how many field bounces happened in between, is safe).
static void TradeCodeSession_EndReturnToField(u8 taskId)
{
    UnlockPlayerFieldControls();
    ScriptUnfreezeObjectEvents();
    Free(sTradeCodeSessionPtr);
    sTradeCodeSessionPtr = NULL;
    DestroyTask(taskId);
}

// Mirrors src/union_room.c's own file-local PrintOnTextbox - reimplemented
// here since that one isn't reachable from outside union_room.c - built
// entirely from public primitives (see this file's own top comment).
static bool8 TradeCodeSession_PrintMessage(u8 *state, const u8 *str)
{
    switch (*state)
    {
    case 0:
        LoadMessageBoxAndBorderGfx();
        DrawDialogueFrame(0, TRUE);
        StringExpandPlaceholders(gStringVar4, str);
        AddTextPrinterForMessage(TRUE);
        (*state)++;
        break;
    case 1:
        if (!RunTextPrintersAndIsPrinter0Active())
        {
            *state = 0;
            return TRUE;
        }
        break;
    }
    return FALSE;
}

// Mirrors src/union_room.c's own file-local UnionRoomHandleYesNo, minus its
// noDraw branch (not needed here - every call site in this file wants the
// box drawn). Returns MENU_NOTHING_CHOSEN while still choosing, or the
// final input (0 = YES, 1 = NO, MENU_B_PRESSED) once chosen.
static s8 TradeCodeSession_HandleYesNo(u8 *state, bool8 defaultNo)
{
    if (*state == 0)
    {
        if (defaultNo)
            DisplayYesNoMenuWithDefault(1);
        else
            DisplayYesNoMenuDefaultYes();
        *state = 1;
        return MENU_NOTHING_CHOSEN;
    }
    else
    {
        s8 input = Menu_ProcessInputNoWrapClearOnChoose();
        if (input != MENU_NOTHING_CHOSEN)
            *state = 0;
        return input;
    }
}

//
//       Trade Code Session specific code
//
static void Task_TradeCodeSession_FieldUI(u8 taskId)
{
    struct TradeCodeSessionState *s = sTradeCodeSessionPtr;
    s8 input;

    if (gPaletteFade.active)
        return;

    switch (s->uiStep)
    {
    case UI_STEP_GATE_CHECK:
        // Mirrors CableClub_EventScript_CheckPartyTradeRequirements
        // (data/scripts/cable_club.inc) - the same two gates the old
        // link-trade path already runs before it will even attempt a
        // trade. DoesPartyHaveEnigmaBerry() already fills gStringVar1 with
        // the berry's name on TRUE (see src/script_pokemon_util.c) - no
        // separate placeholder setup needed here.
        if (CalculatePlayerPartyCount() < 2)
        {
            s->pendingMessage = sText_NeedTwoMons;
            TradeCodeSession_GotoStep(UI_STEP_MESSAGE_THEN_ABORT);
        }
        else if (DoesPartyHaveEnigmaBerry())
        {
            s->pendingMessage = sText_CantTradeEnigmaBerry;
            TradeCodeSession_GotoStep(UI_STEP_MESSAGE_THEN_ABORT);
        }
        else
        {
            // Populates gHostRfuGameData.compatibility.hasNationalDex (via
            // IsNationalPokedexEnabled() - a plain local save-flag read, no
            // RFU/link dependency - confirmed by reading src/link_rfu_3.c's
            // InitHostRfuGameData before relying on it) so PARTY_MENU_TYPE_
            // UNION_ROOM_REGISTER's own in-menu CanRegisterMonForTrading
            // Board gate (src/party_menu.c's CursorCb_Register) behaves
            // correctly instead of defaulting to "no National Dex" - the
            // rest of that struct (activity/partnerInfo/etc.) is irrelevant
            // here, this screen never touches RFU/link state otherwise.
            SetHostRfuGameData(ACTIVITY_NONE, 0, FALSE);
            ChooseMonForTradingBoard(PARTY_MENU_TYPE_UNION_ROOM_REGISTER, CB2_TradeCodeSession_AfterChooseMon);
        }
        break;

    case UI_STEP_MESSAGE_THEN_ABORT:
        if (s->promptPhase == 0)
        {
            if (TradeCodeSession_PrintMessage(&s->msgState, s->pendingMessage))
                s->promptPhase = 1;
        }
        else if (JOY_NEW(A_BUTTON))
        {
            PlaySE(SE_SELECT);
            TradeCodeSession_EndReturnToField(taskId);
        }
        break;

    case UI_STEP_OFFER_READY:
        if (s->promptPhase == 0)
        {
            if (TradeCodeSession_PrintMessage(&s->msgState, sText_ReadyForPartnerCode))
                s->promptPhase = 1;
        }
        else
        {
            input = TradeCodeSession_HandleYesNo(&s->yesNoState, FALSE);
            if (input == 0)
            {
                PlaySE(SE_SELECT);
                s->entryBits.data = s->entryScratch;
                s->entryBits.capacity = sizeof(s->entryScratch) * 8;
                TradeCodeEntry_Init(&s->entryBits, 0, TradeCodeSession_ValidateOfferEntry, &s->entryStatus, CB2_TradeCodeSession_AfterOfferEntry);
            }
            else if (input == 1 || input == MENU_B_PRESSED)
            {
                PlaySE(SE_SELECT);
                s->cancelReturnStep = UI_STEP_OFFER_READY;
                TradeCodeSession_GotoStep(UI_STEP_CANCEL_CONFIRM);
            }
        }
        break;

    case UI_STEP_COMMIT:
        if (s->promptPhase == 0)
        {
            if (TradeCodeSession_PrintMessage(&s->msgState, sText_ConfirmCommit))
                s->promptPhase = 1;
        }
        else
        {
            // Defaults to NO - this is the irreversible step, and an
            // accidental double-A-press must not be able to confirm it
            // (mirrors start_menu.c's own DisplayYesNoMenuWithDefault(1)
            // choice for its similarly consequential prompts).
            input = TradeCodeSession_HandleYesNo(&s->yesNoState, TRUE);
            if (input == 0)
            {
                PlaySE(SE_SELECT);
                if (!TradeCodeSession_DoCommit())
                    TradeCodeSession_GotoStep(UI_STEP_SAVE_FAILED);
                // On success, DoCommit() has already handed off to
                // TradeCodeDisplay_Init - this task sits inert until that
                // screen's own setup calls ResetTasks() (see this file's
                // top comment).
            }
            else if (input == 1 || input == MENU_B_PRESSED)
            {
                PlaySE(SE_SELECT);
                s->cancelReturnStep = UI_STEP_COMMIT;
                TradeCodeSession_GotoStep(UI_STEP_CANCEL_CONFIRM);
            }
        }
        break;

    case UI_STEP_CANCEL_CONFIRM:
        if (s->promptPhase == 0)
        {
            if (TradeCodeSession_PrintMessage(&s->msgState, sText_CancelConfirm))
                s->promptPhase = 1;
        }
        else
        {
            input = TradeCodeSession_HandleYesNo(&s->yesNoState, TRUE);
            if (input == 0)
            {
                PlaySE(SE_SELECT);
                TradeCodeSession_EndReturnToField(taskId);
            }
            else if (input == 1 || input == MENU_B_PRESSED)
            {
                PlaySE(SE_SELECT);
                TradeCodeSession_GotoStep(s->cancelReturnStep);
            }
        }
        break;

    case UI_STEP_SAVE_FAILED:
        if (s->promptPhase == 0)
        {
            if (TradeCodeSession_PrintMessage(&s->msgState, gText_SaveError))
                s->promptPhase = 1;
        }
        else if (JOY_NEW(A_BUTTON))
        {
            // Safe to just retry from the top: the escrow/tag/state writes
            // TradeCodeSession_DoCommit makes are all to gSaveBlock2Ptr (in
            // memory only) and are themselves idempotent (re-zeroing an
            // already-empty slot, recomputing the same deterministic
            // tags) - nothing has actually reached the save file yet,
            // which is exactly why it's safe to sit here retrying rather
            // than trying to roll anything back.
            PlaySE(SE_SELECT);
            if (!TradeCodeSession_DoCommit())
                TradeCodeSession_GotoStep(UI_STEP_SAVE_FAILED);
        }
        break;
    }
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
    struct TradeCodeBits confirmStream;
    u8 confirmBuf[TRADE_CODE_SESSION_CONFIRM_BYTES];
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
    gSaveBlock2Ptr->pendingTrade.state = TRADE_CODE_STATE_COMMITTED;

    // 3. Force-save, and only reveal the confirm code on success - the
    // whole point being that a power-cut mid-save rolls back to pre-
    // escrow on this cart, with no confirm code ever having been shown.
    saveStatus = TrySavingData(SAVE_NORMAL);
    if (saveStatus != SAVE_STATUS_OK)
        return FALSE;

    memset(confirmBuf, 0, sizeof(confirmBuf));
    confirmStream.data = confirmBuf;
    confirmStream.capacity = sizeof(confirmBuf) * 8;
    confirmStream.bitPos = 0;
    confirmStream.error = FALSE;
    TradeCode_WriteBits(&confirmStream, TRADE_CODE_KIND_CONFIRM, 2);
    TradeCode_WriteBits(&confirmStream, myTag, 28);
    TradeCode_Encode(confirmBuf, confirmStream.bitPos, encoded);

    Free(s);
    sTradeCodeSessionPtr = NULL;
    UnlockPlayerFieldControls();
    // Step 4 (materialising the incoming mon) is Stage 8's job - this
    // stage's own scope ends here, once the confirm code has been shown.
    // Pressing A on Stage 5's display screen returns straight to
    // CB2_ReturnToField, same as any other normal field return.
    TradeCodeDisplay_Init(encoded, SPECIES_NONE, NULL, TRUE, CB2_ReturnToField);
    return TRUE;
}

static void CB2_TradeCodeSession_AfterChooseMon(void)
{
    struct TradeCodeSessionState *s = sTradeCodeSessionPtr;
    u8 slot = GetCursorSelectionMonId();
    struct Pokemon *mon;
    u8 encoded[TRADE_CODE_MAX_CHARS + 1];
    u8 nickname[POKEMON_NAME_LENGTH + 1];

    if (slot >= PARTY_SIZE)
    {
        // Cancelled from the party menu itself - nothing was ever shown or
        // escrowed. Silent abort, matching every other ChooseMonForTrading
        // Board caller's own cancel behaviour (e.g. src/union_room.c).
        Free(s);
        sTradeCodeSessionPtr = NULL;
        SetMainCallback2(CB2_ReturnToField);
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
        s->pendingMessage = sText_CantTradeEgg;
        TradeCodeSession_GoToField(UI_STEP_MESSAGE_THEN_ABORT);
        return;
    }
    if (TradeCodeSession_WouldLeavePartyEmpty(slot))
    {
        s->pendingMessage = sText_CantTradeLastMon;
        TradeCodeSession_GoToField(UI_STEP_MESSAGE_THEN_ABORT);
        return;
    }

    s->partySlot = slot;
    s->myOtId = GetMonData(mon, MON_DATA_OT_ID);
    s->myNonce = (u16)Random32();
    TradeCodeSession_BuildOffer(mon);

    TradeCode_Encode(s->myOfferBytes, s->myOfferBits, encoded);
    GetMonData(mon, MON_DATA_NICKNAME, nickname);
    TradeCodeDisplay_Init(encoded, GetMonData(mon, MON_DATA_SPECIES), nickname, FALSE, CB2_TradeCodeSession_AfterOfferShown);
}

static void CB2_TradeCodeSession_AfterOfferShown(void)
{
    TradeCodeSession_GoToField(UI_STEP_OFFER_READY);
}

static void CB2_TradeCodeSession_AfterOfferEntry(void)
{
    struct TradeCodeSessionState *s = sTradeCodeSessionPtr;

    if (s->entryStatus != TRADE_CODE_ENTRY_OK)
    {
        // The only other status TradeCodeEntry_Init's callback can report
        // is TRADE_CODE_ENTRY_CANCELLED (B on an empty field) - a failed
        // validator retries in place without leaving the screen (see
        // trade_code_entry.h). Still OFFER_SHOWN, nothing escrowed yet -
        // routed to the same cancel-confirm the "ready?" prompt's own "No"
        // uses, rather than silently dropping back to the field on one B
        // press (matches the doc's "Cancel... with a confirm" for this
        // state).
        s->cancelReturnStep = UI_STEP_OFFER_READY;
        TradeCodeSession_GoToField(UI_STEP_CANCEL_CONFIRM);
        return;
    }

    // Reuses gParties[B_TRAINER_OPPONENT_A][0] for the preview, the same
    // "opponent slot" convention Stage 8 will use for the real materialise
    // + CB2_InitInGameTrade animation - so both this preview and Stage 8's
    // eventual reuse of the old in-game-trade path agree on where a
    // not-yet-owned incoming mon temporarily lives.
    BoxMonToMon(&s->partnerBoxMon, &gParties[B_TRAINER_OPPONENT_A][0]);
    CalculateMonStats(&gParties[B_TRAINER_OPPONENT_A][0]);
    ShowPokemonSummaryScreen(SUMMARY_MODE_LOCK_MOVES, gParties[B_TRAINER_OPPONENT_A], 0, 0, CB2_TradeCodeSession_AfterPreview);
}

static void CB2_TradeCodeSession_AfterPreview(void)
{
    struct TradeCodeSessionState *s = sTradeCodeSessionPtr;
    struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][s->partySlot];

    // The preview itself IS the acceptance, per the doc's own Step 2
    // wording ("show a preview screen... State -> PARTNER_OFFER_ACCEPTED") -
    // no separate "accept this offer?" prompt once the player has looked
    // at it and pressed B to move on; the very next thing shown is Step
    // 3's own irreversible commit prompt, which already asks a yes/no
    // question of its own.
    GetMonData(mon, MON_DATA_NICKNAME, gStringVar1);
    StripExtCtrlCodes(gStringVar1);
    GetBoxMonData(&s->partnerBoxMon, MON_DATA_NICKNAME, gStringVar2);
    StripExtCtrlCodes(gStringVar2);
    TradeCodeSession_GoToField(UI_STEP_COMMIT);
}
