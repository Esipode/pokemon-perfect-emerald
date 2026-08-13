#ifndef GUARD_TRADE_CODE_ENTRY_H
#define GUARD_TRADE_CODE_ENTRY_H

#include "main.h"
#include "trade_code.h"

// Stage 6 of "Trading Codes.md": the purpose-built code entry screen. All
// 32 Base32/Crockford symbols on one page, laid out 8x4, plus BACK and OK -
// no page swapping, which is the whole reason this isn't naming_screen.c
// (see that stage's own bullet list, and the plan doc's "key findings"
// section on why NAMING_SCREEN_CODE's 3-page keyboard doesn't fit this
// use case). This module is deliberately kept independent of Stage 7's
// session/protocol logic - see `TradeCodeEntryValidator` below.

enum TradeCodeEntryStatus
{
    TRADE_CODE_ENTRY_OK,
    TRADE_CODE_ENTRY_BAD_CHAR,      // mirrors TRADE_CODE_BAD_CHAR (see trade_code.h) - not reachable
                                     // through the on-screen keyboard alone, since every keypress is
                                     // already a valid alphabet symbol, but kept for parity with
                                     // TradeCode_Decode's own status enum and any future paste/typed
                                     // entry path.
    TRADE_CODE_ENTRY_WRONG_LENGTH,  // mirrors TRADE_CODE_TOO_SHORT / TRADE_CODE_TOO_LONG
    TRADE_CODE_ENTRY_INVALID,       // validator rejected an otherwise well-formed code (tamper/seal/
                                     // "not made for you") - see TradeCodeEntryValidator
    TRADE_CODE_ENTRY_ALREADY_USED,  // validator: replay ring hit
    TRADE_CODE_ENTRY_WRONG_VERSION, // validator: formatVersion mismatch
    TRADE_CODE_ENTRY_CANCELLED,     // player backed out (B on an empty field) rather than submitting
};

// Called once a codec-level decode (TradeCode_Decode) succeeds, before this
// screen reports success to its own caller. This is how Stage 7's session
// logic layers format-version/seal/replay-ring checks on top of a
// syntactically valid code without this module (or a simpler caller, like
// the debug menu's round-trip test) needing to know anything about
// sessions, seals, or the replay ring - see the plan doc's own framing of
// Stage 6 as "separated precisely so it doesn't have to share context with
// the protocol logic."
//
// Return TRADE_CODE_ENTRY_OK to accept the code. Any other status clears
// the field, shows that status's canned message, and lets the player type
// again without leaving the screen. `decoded` is only valid for the
// duration of the call - copy out of it if the accept decision needs to
// keep anything.
typedef enum TradeCodeEntryStatus (*TradeCodeEntryValidator)(struct TradeCodeBits *decoded);

// Opens the code entry screen.
//
// `outBits` is caller-owned, exactly like TradeCode_Decode's own contract
// (include/trade_code.h): set `data`/`capacity` (buffer and its size in
// bits) before calling. Only written when the final result (written to
// `*outStatus`, see below) is TRADE_CODE_ENTRY_OK - left untouched on every
// other status, same "never touch the caller's output on failure" contract
// TradeCode_DeserializeMon already uses.
//
// `expectedSymbols` is 0 for a variable-length offer code (only OK/START
// submits), or an exact symbol count (TRADE_CODE_CONFIRM_CHARS for a
// confirm code) to auto-submit the instant that many symbols are entered
// and to format the field as one ungrouped run with no hyphens (see the
// payload spec's "Confirm code... formatted as one group of 6"). A
// variable-length field is grouped/hyphenated every TRADE_CODE_GROUP_SIZE
// symbols, mirroring TradeCodeDisplay's own layout (trade_code_display.h).
//
// `validator` may be NULL for a codec-only screen (what the debug menu's
// round-trip test uses) - the result is then whatever TradeCode_Decode
// itself returned, with no extra layer of checks.
//
// `outStatus` is caller-owned storage this screen writes into exactly once
// (right before the final SetMainCallback2), which is how the result gets
// back to the caller - `MainCallback` itself (see main.h) takes no
// arguments, so there's nowhere else to put it. Mirrors the "write the
// caller's output, then hand control back via the caller's own callback"
// shape already used elsewhere in this codebase (e.g.
// ChooseMonForTradingBoard, include/party_menu.h) rather than inventing a
// callback-with-arguments type.
//
// `callback` is where SetMainCallback2 goes once the screen is done -
// taken as an explicit parameter for the same reason as
// TradeCodeDisplay_Init (see trade_code_display.h): so Stage 7's session
// state machine can chain straight into whatever comes next.
void TradeCodeEntry_Init(struct TradeCodeBits *outBits, u32 expectedSymbols,
                          TradeCodeEntryValidator validator, enum TradeCodeEntryStatus *outStatus,
                          MainCallback callback);

#endif // GUARD_TRADE_CODE_ENTRY_H
