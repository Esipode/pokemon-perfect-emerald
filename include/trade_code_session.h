#ifndef GUARD_TRADE_CODE_SESSION_H
#define GUARD_TRADE_CODE_SESSION_H

// Stage 7 of "Trading Codes.md": the session state machine that ties
// Stages 1-6 together for Steps 1-3 of the protocol (offer -> preview ->
// the irreversible commit). Step 4 (materialising the incoming mon) is
// Stage 8's job, and reset-resistant re-entry into a COMMITTED session is
// Stage 9's - this stage stops the moment the confirm code has been
// revealed post-save.
//
// Deliberately a single parameterless entry point, mirroring how the old
// link-trade path is entered (CableClub_EventScript_TradeCenter calls a
// `special` with no arguments and reacts only to VAR_RESULT/state that the
// special itself manages) - everything this session needs (which mon, the
// generated codes, the decoded partner mon) is session-local state, not
// caller-supplied. Registered as a `special` (Stage 10) and called from the
// Cable Club attendant script in place of `TryTradeLinkup`; also reachable
// from the debug menu (src/debug.c) for testing, exactly like every Stage
// 5/6 screen was before its own real entry point existed.
//
// Post-Stage-10 fix: this is the attendant's *only* trade-code entry point,
// so it also has to cover Step 4 (entering a partner's confirm code), not
// just Steps 1-3 - a real playtest showed a player who'd already committed
// (state == TRADE_CODE_STATE_COMMITTED) talking to the attendant again had
// nowhere to go; Stage 9's boot-hook re-entry existed, but power-cycling
// just to type in a code you already have in hand is a bad way to require
// that. TradeCodeSession_Start now checks pendingTrade.state itself first
// and, if COMMITTED, delegates straight to TradeCodeReceive_Start instead
// of opening a new offer (starting a second offer on top of a pending
// receive would be wrong regardless of the UX question - the mon already
// escrowed by Step 3 needs to be resolved, one way or another, before
// another can be given up). The boot hook remains as the reset-resistant
// backstop for a player who saves and quits mid-COMMITTED, not the only
// way in.
void TradeCodeSession_Start(void);

// Post-Stage-10 follow-up: two more attendant-menu options, "view offer
// code" and "view confirm code" - re-display a code the player has already
// been shown once (Step 1's offer, or Step 3's confirm reveal) without
// re-running any part of the trade. Both are effectively no-ops unless
// gSaveBlock2Ptr->pendingTrade.state == TRADE_CODE_STATE_COMMITTED (the
// only state the attendant's menu can ever be reached in that has anything
// to show - see TradeCodeSession_Start's own comment above for why there's
// no third state to worry about), in which case each shows a plain "no
// trade code to show right now" message instead. Parameterless and
// `special`-callable, same shape as TradeCodeSession_Start itself.
void TradeCodeSession_ViewOfferCode(void);
void TradeCodeSession_ViewConfirmCode(void);

#endif // GUARD_TRADE_CODE_SESSION_H
