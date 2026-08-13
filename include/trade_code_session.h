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
// caller-supplied. Stage 10 is expected to register this as a `special`
// and call it from the Cable Club attendant script in place of
// `TryTradeLinkup`; for now (Stage 10 not built yet) it's reachable only
// from the debug menu (src/debug.c) for testing, exactly like every
// Stage 5/6 screen was before its own real entry point existed.
void TradeCodeSession_Start(void);

#endif // GUARD_TRADE_CODE_SESSION_H
