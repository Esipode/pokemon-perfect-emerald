#ifndef GUARD_TRADE_CODE_ENTRY_H
#define GUARD_TRADE_CODE_ENTRY_H

#include "main.h"
#include "trade_code.h"

// The purpose-built code entry screen. All 32 Base32/Crockford symbols on one
// page, laid out 8x4, plus BACK and OK. There is no page swapping, which is why
// this isn't naming_screen.c. Kept independent of session/protocol logic - see
// `TradeCodeEntryValidator` below.

enum TradeCodeEntryStatus
{
    TRADE_CODE_ENTRY_OK,
    TRADE_CODE_ENTRY_BAD_CHAR,      // mirrors TRADE_CODE_BAD_CHAR - not reachable through the
                                     // on-screen keyboard, kept for parity with TradeCode_Decode.
    TRADE_CODE_ENTRY_WRONG_LENGTH,  // mirrors TRADE_CODE_TOO_SHORT / TRADE_CODE_TOO_LONG
    TRADE_CODE_ENTRY_INVALID,       // validator rejected an otherwise well-formed code (tamper/seal/
                                     // "not made for you") - see TradeCodeEntryValidator
    TRADE_CODE_ENTRY_ALREADY_USED,  // validator: replay ring hit
    TRADE_CODE_ENTRY_WRONG_VERSION, // validator: formatVersion mismatch
    TRADE_CODE_ENTRY_CANCELLED,     // player backed out (B on an empty field) rather than submitting
};

// Called once a codec-level decode (TradeCode_Decode) succeeds, before this
// screen reports success. Lets the session layer add format-version/seal/
// replay-ring checks without this module (or a codec-only caller like the
// debug menu's round-trip test) knowing about sessions.
//
// Return TRADE_CODE_ENTRY_OK to accept the code. Any other status clears the
// field, shows that status's canned message, and lets the player type again.
// `decoded` is only valid for the duration of the call.
typedef enum TradeCodeEntryStatus (*TradeCodeEntryValidator)(struct TradeCodeBits *decoded);

// Opens the code entry screen.
// `outBits` is caller-owned, like TradeCode_Decode's contract: set
// `data`/`capacity` before calling. Only written when the final result
// (`*outStatus`) is TRADE_CODE_ENTRY_OK; untouched on every other status.
//
// `expectedSymbols` is 0 for a variable-length offer code (only OK/START
// submits), or an exact symbol count (TRADE_CODE_CONFIRM_CHARS for a confirm
// code) to auto-submit once that many symbols are entered and to format the
// field as one ungrouped run with no hyphens. A variable-length field is
// hyphenated every TRADE_CODE_GROUP_SIZE symbols, like TradeCodeDisplay.
//
// `validator` may be NULL for a codec-only screen; the result is then
// whatever TradeCode_Decode returned.
//
// `outStatus` is caller-owned storage written once, right before the final
// SetMainCallback2, since MainCallback takes no arguments.
//
// `callback` is where SetMainCallback2 goes once the screen is done.
void TradeCodeEntry_Init(struct TradeCodeBits *outBits, u32 expectedSymbols,
                          TradeCodeEntryValidator validator, enum TradeCodeEntryStatus *outStatus,
                          MainCallback callback);

#endif // GUARD_TRADE_CODE_ENTRY_H
