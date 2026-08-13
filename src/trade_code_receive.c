#include "global.h"
#include "trade_code_receive.h"
#include "trade_code.h"
#include "trade_code_entry.h"
#include "trade_code_prompt.h"
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

// Stage 8 of "Trading Codes.md": Step 4, the commit and swap. See
// include/trade_code_receive.h for the scope/entry-point rationale, and
// this stage's own status block in the plan doc for why this does NOT
// reuse CB2_InitInGameTrade (src/trade.c) the way the doc's own Stage 8
// bullet list originally sketched.
//
// Short version: by the time this file ever runs, Step 3 (trade_code_
// session.c) has already ZeroMonData/CompactPartySlots'd the offered mon
// out of the party - possibly a real-world while ago, on a completely
// separate play session. There is no "outgoing" mon left to show leaving,
// which is the entire visual premise of CB2_InitInGameTrade's give-and-
// receive animation - it isn't just cosmetic filler, TradeMons (src/
// trade.c) SWAPs the player's chosen party slot with gParties[B_TRAINER_
// OPPONENT_A][0] as part of resolving that animation, which would corrupt
// an already-empty slot rather than genuinely materialise anything. Worse,
// its scene-text buffering (BufferTradeSceneStrings, non-link branch)
// reads the departing/arriving Pokemon's OT name and nickname from the
// hardcoded sIngameTrades[] NPC-trade table via gSpecialVar_0x8005, not
// from the actual partner mon's real data - our partner isn't in that
// table at all, so reusing it as-is would print the wrong trainer and
// Pokemon names on screen.
//
// Trade evolution instead reuses evolution_scene.h's own general-purpose
// BeginEvolutionScene - the exact same self-contained "your Pokemon is
// evolving!" cutscene every other evolution trigger in this codebase
// already uses (level-up, a Rare Candy, a stone) - which builds its own
// fresh sprite from scratch and needs no pre-existing trade-animation
// sprite/BG setup at all, unlike TradeEvolutionScene (which assumes a
// sprite the dual trade animation already created). GetEvolutionTarget
// Species is called with tradePartner = NULL, a value it already supports
// explicitly (see DoesMonMeetAdditionalConditions, src/pokemon.c) - plain
// and held-item trade evolutions (e.g. Machoke -> Machamp, King's Rock/
// Metal Coat) resolve exactly as they would with a partner, since neither
// depends on tradePartner; only a partner-*species*-specific evolution
// (Karrablast/Shelmet-style, if this fork has any) can't fire without a
// real partner mon, which no longer exists at Step 4 to offer as one. Not
// spending a design point on a fabricated stand-in for that partner mon
// is deliberate - see the plan doc's own precedent for this kind of
// honestly-flagged, small lossy tradeoff (Stage 2's personality/exp
// quantisation).
//
// gSaveBlock2Ptr->pendingTrade.abandonedCount (struct PendingTrade, Stage
// 4) is intentionally never written by this file. Stage 4 added it
// anticipating Stage 11's own "abandoned-trade counter... soft social
// deterrent" bullet, and an earlier draft of this file's own give-up path
// (Stage 9, below) incremented it. Dropped after a design discussion: a
// save file is trivially duplicable outside the game entirely, so nothing
// here can actually stop a determined duper, and a player who trips the
// give-up path is at least as likely to be the one who got ghosted as the
// one doing the ghosting - tracking/surfacing it would punish the wrong
// population to deter a threat it doesn't even catch. The field itself is
// left in place (removing it would touch struct SaveBlock2's layout and
// test/save.c's own size pin for no functional gain) but is now dead
// weight - always zero, read by nothing.

// TradeCodeEntry_Init's own outBits target for the confirm-code field:
// TRADE_CODE_CONFIRM_CHARS (6) symbols * 5 bits = 30 bits, rounded up to a
// whole byte.
#define TRADE_CODE_RECEIVE_CONFIRM_SCRATCH_BYTES ((TRADE_CODE_CONFIRM_CHARS * 5 + 7) / 8)

struct TradeCodeReceiveState
{
    MainCallback returnCallback; // Stage 9: see include/trade_code_receive.h
    struct TradeCodeBits entryBits;
    u8 entryScratch[TRADE_CODE_RECEIVE_CONFIRM_SCRATCH_BYTES];
    enum TradeCodeEntryStatus entryStatus;
    enum TradeCodePromptResult promptResult;
    u8 partyIndex;    // valid only when !wentToPC
    bool8 wentToPC;
};

//==========EWRAM==========//
static EWRAM_DATA struct TradeCodeReceiveState *sTradeCodeReceivePtr = NULL;

//==========STATIC=DEFINES==========//
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

//==========CONST=DATA==========//
static const u8 sText_NoTradeAwaiting[]  = _("There's no trade code waiting\nto be completed.");
// Stage 9: shown when gSaveBlock2Ptr->pendingTrade.incoming fails
// TradeCode_ValidatePendingBoxMon on resume (a corrupted save sector, or a
// hand-tampered one) - see this file's own CB2_TradeCodeReceive_AfterCorruptAck.
static const u8 sText_TradeCodeCorrupt[] = _("Something went wrong with a\npending trade. It's been cancelled.");
// Stage 9: the one way out of a COMMITTED trade, reached by pressing B on
// an empty confirm-code field - see include/trade_code_receive.h's own
// comment on TradeCodeReceive_Start for why this exists at all. Defaults
// to NO (TradeCodeReceive_ShowGiveUpPrompt's own TradeCodePrompt_Init
// call) - this is the irreversible half of an already-irreversible step,
// and an accidental double-B-then-A must not be able to confirm it.
static const u8 sText_ConfirmGiveUp[] = _("Give up on this trade?\nYou will not get {STR_VAR_1}.");
// Split into two short messages rather than one \p-paged one - mirrors
// vanilla's own two-message split for this exact moment (src/trade.c's
// STATE_SEND_MSG/STATE_TAKE_CARE_OF_MON, gText_XSentOverY/gText_TakeGood
// CareOfX), and sidesteps Stage 7's own still-unverified-on-hardware \p
// pagination fix (see that stage's status block's last entry) entirely by
// reusing the exact "one full ACK screen per short message" shape that
// stage's own diagnostic tooling settled on once \p caused it real
// trouble.
static const u8 sText_SentOver[]      = _("{STR_VAR_1} sent over\n{STR_VAR_2}!");
static const u8 sText_TakeGoodCareOfIt[] = _("Take good care of\n{STR_VAR_2}!");
static const u8 sText_SentToBox[]     = _("Your party is full, so\n{STR_VAR_2} was sent to a Box.");

//==========UI=SETUP==========//
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

    // Shouldn't be reachable through the real entry point once Stage 10
    // gates it behind this same state check, but the debug menu can call
    // this directly regardless of what gSaveBlock2Ptr->pendingTrade
    // actually holds, so guard here too rather than trust the caller.
    if (gSaveBlock2Ptr->pendingTrade.state != TRADE_CODE_STATE_COMMITTED)
    {
        TradeCodePrompt_Init(sText_NoTradeAwaiting, FALSE, FALSE, &s->promptResult, CB2_TradeCodeReceive_AfterNoTradeAck);
        return;
    }

    // Stage 9: guard against a corrupted or hand-tampered pendingTrade
    // before it's ever handed to the entry screen/preview/party-insert
    // machinery below - see TradeCode_ValidatePendingBoxMon's own comment
    // (include/trade_code.h) for exactly what this checks. Checked here,
    // inside the one real entry point, rather than only at the Stage 9
    // boot-hook call site (src/overworld.c) - the debug menu's own "Receive
    // Trade Code (Step 4)..." action reaches this exact code path too, and
    // deserves the same protection.
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

// Stage 9: pendingTrade.incoming failed TradeCode_ValidatePendingBoxMon.
// Never materialised - cleared back to TRADE_CODE_STATE_NONE (keeping the
// replay ring, same selective-clear as everywhere else in this feature)
// and force-saved so the error doesn't keep reappearing on every future
// boot once it's actually fixed.
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
// code's payload is fixed and tiny (see the payload spec): codeKind (2
// bits) + a 28-bit combined tag, no seal of its own - the tag itself
// already is the anti-tamper/anti-forgery check (TradeCode_ConfirmTag,
// Stage 3), computed and compared directly against what Step 3 already
// derived and stored as pendingTrade.expectedConfirmTag.
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
        // The only other status TradeCodeEntry_Init's own callback can
        // report is TRADE_CODE_ENTRY_CANCELLED (B on an empty field) - a
        // validator rejection (wrong/garbled tag) is handled entirely
        // inside the entry screen itself, which shows its own canned
        // message and loops the player back into the same field without
        // ever reaching this callback (see trade_code_entry.h's own
        // contract, and Stage 7's trade_code_session.c for the identical
        // reasoning at its own offer-entry callback).
        //
        // Step 4 has no ordinary "cancel" - the offered mon already left
        // in Step 3, and the doc's own "Lock-in" wording is explicit that
        // no cancel affordance exists once COMMITTED. But Stage 9's own
        // dev note raises a real, separate problem an unconditional
        // "just reopen the field" doesn't solve: a partner who never sends
        // back a valid confirm code would leave this player stuck
        // re-entering this exact screen every single boot, forever, with
        // no way out at all. Offer the one honest way out instead -
        // forfeit the trade - rather than none.
        TradeCodeReceive_ShowGiveUpPrompt();
        return;
    }

    TradeCodeReceive_DoSwap();
}

// Stage 9: see include/trade_code_receive.h's own comment on
// TradeCodeReceive_Start for why this exists.
static void TradeCodeReceive_ShowGiveUpPrompt(void)
{
    struct TradeCodeReceiveState *s = sTradeCodeReceivePtr;
    struct BoxPokemon boxMon;

    memcpy(&boxMon, gSaveBlock2Ptr->pendingTrade.incoming, sizeof(boxMon));
    GetBoxMonData(&boxMon, MON_DATA_NICKNAME, gStringVar1);
    StripExtCtrlCodes(gStringVar1);
    // TradeCodePrompt_Init doesn't expand placeholders itself - see this
    // file's own TradeCodeReceive_DoSwap for the identical reasoning.
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

// The forfeit itself: permanently gives up the incoming mon. There is no
// partial undo of Step 3's escrow - both sides already gave up their own
// mon before either received anything, per the plan doc's own protocol
// section - so this isn't a penalty being applied on top of anything; it's
// just acknowledging a loss that already happened on the partner's side
// and letting the player stop being blocked by it. No counter, no record
// of this kept anywhere - a player giving up here is at least as likely to
// be the one who got ghosted as the one doing the ghosting, and punishing
// that population to (ineffectually) deter a save-duplicating scammer who
// was never going to trip this path anyway isn't the goal. Keeps the
// replay ring: the partner's offer seal stays burned regardless of which
// side eventually walks away, the same anti-duplication guarantee a normal
// completion gets - that part of the fair-exchange design still holds.
static void TradeCodeReceive_DoGiveUp(void)
{
    TradeCodeReceive_ClearPendingTradeFields();
    TradeCodeReceive_SaveThenFinish();
}

// The actual Step 4 swap: build the incoming BoxPokemon into a real
// struct Pokemon, apply the received-mon friendship reset, insert it into
// the party (or the PC if the party's full), update the Pokedex, and
// clear pendingTrade back to NONE (short of the replay ring and Stage 11's
// abandonedCount, which this doesn't touch). Evolution and the second
// force-save are split into their own functions since evolution, if it
// happens, needs to run to completion (BeginEvolutionScene's own screen
// takeover) before the save - see this file's own header comment and the
// plan doc's own "save after the mon is in the party and after evolution
// resolves" ordering.
static void TradeCodeReceive_DoSwap(void)
{
    struct TradeCodeReceiveState *s = sTradeCodeReceivePtr;
    struct BoxPokemon boxMon;
    struct Pokemon mon;
    bool32 isEgg;
    u8 friendship;
    u8 maxSize, i;

    memcpy(&boxMon, gSaveBlock2Ptr->pendingTrade.incoming, sizeof(boxMon));
    BoxMonToMon(&boxMon, &mon);
    CalculateMonStats(&mon);

    // Mirrors src/trade.c's TradeMons friendship=70 rule exactly (see the
    // plan doc's own citation, src/trade.c:3113-3116) - Eggs use
    // Friendship to track egg cycles, so it's left alone on one.
    isEgg = GetMonData(&mon, MON_DATA_IS_EGG);
    if (!isEgg)
    {
        friendship = 70;
        SetMonData(&mon, MON_DATA_FRIENDSHIP, &friendship);
    }

    // Insert into the first empty party slot, else the PC - mirrors
    // GiveCapturedMonToPlayer's own party-then-PC shape (src/pokemon.c),
    // deliberately not calling that function directly since its
    // Achievement_CheckCaptureMilestones/Achievement_OnShinyObtained/etc.
    // calls are specifically about *catching*, not trading, and shouldn't
    // fire here.
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

    // Pokedex registration - mirrors src/trade.c's own (static, so not
    // reusable directly) UpdatePokedexForReceivedMon, but via Handle
    // SetPokedexFlagFromMon for the "caught" half (a small existing
    // wrapper around HandleSetPokedexFlag that does its own species/
    // personality extraction) rather than duplicating that extraction here.
    if (!isEgg)
    {
        enum NationalDexOrder dexNum = SpeciesToNationalPokedexNum(GetMonData(&mon, MON_DATA_SPECIES));
        GetSetPokedexFlag(dexNum, FLAG_SET_SEEN);
        HandleSetPokedexFlagFromMon(&mon, FLAG_SET_CAUGHT);
    }

    // pendingTrade is done with, except the replay ring and Stage 11's own
    // abandonedCount - see the plan doc's own Stage 8 bullet ("Clear
    // pendingTrade to NONE, keeping the replay ring and abandonedCount").
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

    // gStringVar1/gStringVar2 are still what TradeCodeReceive_DoSwap set -
    // nothing in between has touched them.
    StringExpandPlaceholders(gStringVar4, sText_TakeGoodCareOfIt);
    TradeCodePrompt_Init(gStringVar4, FALSE, FALSE, &s->promptResult, CB2_TradeCodeReceive_AfterTakeCareMsg);
}

static void CB2_TradeCodeReceive_AfterTakeCareMsg(void)
{
    struct TradeCodeReceiveState *s = sTradeCodeReceivePtr;

    if (s->wentToPC)
    {
        StringExpandPlaceholders(gStringVar4, sText_SentToBox);
        TradeCodePrompt_Init(gStringVar4, FALSE, FALSE, &s->promptResult, CB2_TradeCodeReceive_AfterBoxMsg);
        return;
    }
    TradeCodeReceive_CheckEvolution();
}

static void CB2_TradeCodeReceive_AfterBoxMsg(void)
{
    TradeCodeReceive_CheckEvolution();
}

// A boxed mon (party was full) never gets an evolution check here, same as
// how a normal wild catch that overflows straight to the PC doesn't either
// - there's no in-box evolution UI in this codebase to reuse or duplicate.
static void TradeCodeReceive_CheckEvolution(void)
{
    struct TradeCodeReceiveState *s = sTradeCodeReceivePtr;
    // NULL default, matching evolution_scene.c's own Task_BeginEvolutionScene
    // ("struct Pokemon *mon = NULL;") for the identical shape - only ever
    // dereferenced inside the same `evoTarget != SPECIES_NONE` branch that
    // requires wentToPC to be FALSE (the only branch that assigns it), but
    // an explicit default avoids relying on a compiler proving that itself.
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
        // tradePartner = NULL throughout - see this file's own header
        // comment for why, and what that does and doesn't cost.
        GetEvolutionTargetSpecies(mon, EVO_MODE_TRADE, ITEM_NONE, NULL, NULL, DO_EVO);
        gCB2_AfterEvolution = CB2_TradeCodeReceive_AfterEvolution;
        // BeginEvolutionScene's own `mon` parameter is unused internally
        // (Task_BeginEvolutionScene re-fetches &gParties[B_TRAINER_PLAYER]
        // [partyId] itself once its own fade-to-black finishes) - passed
        // for signature parity with EvolutionScene/TradeEvolutionScene,
        // not because it does anything here.
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
// the now-unused abandonedCount field, left untouched rather than reclaimed
// - see this file's own header comment) survive - see the plan doc's own
// Stage 8 bullet ("Clear pendingTrade to NONE, keeping the replay ring and
// abandonedCount"). Shared by every Step-4-ends-here path this file has:
// a real completed swap (TradeCodeReceive_DoSwap), Stage 9's give-up
// forfeit (TradeCodeReceive_DoGiveUp), and Stage 9's corrupted-pendingTrade
// recovery (CB2_TradeCodeReceive_AfterCorruptAck) - all three want the
// exact same fields cleared, they just differ in what (if anything) they
// do around the clear.
static void TradeCodeReceive_ClearPendingTradeFields(void)
{
    memset(gSaveBlock2Ptr->pendingTrade.incoming, 0, sizeof(gSaveBlock2Ptr->pendingTrade.incoming));
    gSaveBlock2Ptr->pendingTrade.expectedConfirmTag = 0;
    gSaveBlock2Ptr->pendingTrade.nonce = 0;
    gSaveBlock2Ptr->pendingTrade.partySlot = 0;
    gSaveBlock2Ptr->pendingTrade.state = TRADE_CODE_STATE_NONE;
}

// The force-save shared by every path through this file that ends by
// handing control back out (see Stage 4's TrySavingData in trade_code_
// session.c for the *first* force-save, at Step 3's commit - this is
// always the second, and for Stage 9's give-up/corrupt-clear paths, the
// only one). Deliberately after every RAM mutation the calling path makes
// (the mon already in the party/PC and evolution resolved, for a real
// swap; pendingTrade already cleared, for give-up or corrupt-clear) - a
// power cut before this succeeds can't lose or duplicate anything, since
// the *saved* file simply doesn't reflect whatever RAM-only change was in
// progress yet, and Stage 9's own reset-resistant boot hook re-runs
// whichever path was interrupted from scratch, cleanly, the next time the
// game boots.
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
    // Safe to retry unconditionally, same reasoning as Stage 7's own
    // CB2_TradeCodeSession_AfterSaveFailedAck: everything up to this point
    // only touched gSaveBlock2Ptr/gParties in RAM - nothing reaches the
    // physical save file until TrySavingData itself succeeds - and
    // re-running the save just tries to persist that exact same state
    // again, whichever path (swap/give-up/corrupt-clear) got here.
    TradeCodeReceive_SaveThenFinish();
}

// Frees this screen's own state and hands control to whatever
// TradeCodeReceive_Start was told to return to - see include/trade_code_
// receive.h's own comment on why that's not always CB2_ReturnToField.
static void TradeCodeReceive_FinishToReturnCallback(void)
{
    struct TradeCodeReceiveState *s = sTradeCodeReceivePtr;
    MainCallback returnCallback = s->returnCallback;

    Free(s);
    sTradeCodeReceivePtr = NULL;
    SetMainCallback2(returnCallback);
}
