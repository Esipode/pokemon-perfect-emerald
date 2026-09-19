#ifndef GUARD_TRADE_CODE_SESSION_H
#define GUARD_TRADE_CODE_SESSION_H

// The session state machine for Steps 1-3 of the protocol (offer -> preview ->
// the irreversible commit). Step 4 (materialising the incoming mon) and
// reset-resistant re-entry live in trade_code_receive.c; this stops once the
// confirm code has been revealed post-save.
//
// A single parameterless entry point, like the link-trade path
// (CableClub_EventScript_TradeCenter calls a `special` with no arguments):
// everything the session needs is session-local. Registered as a `special`
// and called from the Cable Club attendant script in place of
// `TryTradeLinkup`; also reachable from the debug menu (src/debug.c).
//
// This is the attendant's only trade-code entry point, so it also covers
// Step 4: if pendingTrade.state is COMMITTED it delegates to
// TradeCodeReceive_Start instead of opening a new offer. A second offer atop a
// pending receive would be wrong, since the mon escrowed by Step 3 must be
// resolved first. The boot hook remains as the backstop for a player who saves
// and quits mid-COMMITTED.
void TradeCodeSession_Start(void);

// Two more attendant-menu options, "view offer code" and "view confirm code",
// re-display a code already shown once (Step 1's offer, or Step 3's confirm
// reveal) without re-running any of the trade. Both show a plain "no trade
// code to show right now" message unless
// gSaveBlock2Ptr->pendingTrade.state == TRADE_CODE_STATE_COMMITTED, the only
// state with anything to show. Parameterless and `special`-callable.
void TradeCodeSession_ViewOfferCode(void);
void TradeCodeSession_ViewConfirmCode(void);

#endif // GUARD_TRADE_CODE_SESSION_H
