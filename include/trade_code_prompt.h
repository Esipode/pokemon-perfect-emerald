#ifndef GUARD_TRADE_CODE_PROMPT_H
#define GUARD_TRADE_CODE_PROMPT_H

#include "main.h"

// A small, self-contained full-screen prompt for src/trade_code_session.c
// (Stage 7 of "Trading Codes.md"): a message (plain, already-\n-broken
// text, printed normally - not the monospace grid trade_code_display.c/
// trade_code_entry.c use for codes) plus either nothing more (ACK mode -
// press A to continue) or a YES/NO choice.
//
// Built as its own CB2_/Task_-driven screen, modelled on src/trade_code_
// display.c (Stage 5) exactly - deliberately *not* routed through
// CB2_ReturnToField/the overworld's own standard dialogue-box system the
// way an earlier draft of trade_code_session.c's native prompts were.
// That approach turned out to hang on hardware (see this file's own
// status-block entry in the plan doc) - the overworld's field-callback/
// window-0 state after returning from a full custom-BG screen isn't
// something this feature can safely assume, no matter how much extra
// re-initialization is added defensively. A fully self-contained screen,
// like every other piece of this feature, sidesteps that class of problem
// entirely rather than trying to patch around it further.
enum TradeCodePromptResult
{
    TRADE_CODE_PROMPT_ACK, // ACK-only mode: A was pressed
    TRADE_CODE_PROMPT_YES,
    TRADE_CODE_PROMPT_NO,  // also reported for B in yes/no mode
};

// `message` must be a game-charmap, EOS-terminated string with its own
// `\n` line breaks already in place (this screen prints it as-is, it
// doesn't wrap), no wider than this screen's own message window and no
// taller than what that window can show - see the .c file's own window
// layout comment for the exact budget. Copied via plain StringCopy into
// this module's own EWRAM buffer on Init (mirrors TradeCodeDisplay_Init's
// own "caller's buffer doesn't need to outlive the call" contract and its
// same StringCopy-not-StringCopyN reasoning), so callers pass a `_(...)`
// literal or an already-StringExpandPlaceholders'd buffer.
//
// `hasYesNo` FALSE: `outResult` is written TRADE_CODE_PROMPT_ACK once A is
// pressed - the only way out. TRUE: the player picks YES or NO (B counts
// as NO, same as a vanilla yes/no box), the cursor starting on
// `yesNoDefaultNo ? NO : YES`.
//
// `callback` is where SetMainCallback2 goes once the screen is done -
// taken as an explicit parameter, the same shape as TradeCodeDisplay_
// Init/TradeCodeEntry_Init, so a caller can chain straight into whatever
// comes next.
void TradeCodePrompt_Init(const u8 *message, bool8 hasYesNo, bool8 yesNoDefaultNo, enum TradeCodePromptResult *outResult, MainCallback callback);

#endif // GUARD_TRADE_CODE_PROMPT_H
