#ifndef GUARD_TRADE_CODE_PROMPT_H
#define GUARD_TRADE_CODE_PROMPT_H

#include "main.h"

// A small, self-contained full-screen prompt for trade_code_session.c: a
// message (plain, already-\n-broken text, not the monospace code grid) plus
// either nothing more (ACK mode - press A to continue) or a YES/NO choice.
//
// Built as its own CB2_/Task_-driven screen, modelled on trade_code_display.c,
// and deliberately not routed through CB2_ReturnToField or the overworld's
// dialogue-box system: that hung on hardware, since the field-callback/window-0
// state after returning from a full custom-BG screen cannot be assumed.
enum TradeCodePromptResult
{
    TRADE_CODE_PROMPT_ACK, // ACK-only mode: A was pressed
    TRADE_CODE_PROMPT_YES,
    TRADE_CODE_PROMPT_NO,  // also reported for B in yes/no mode
};

// `message` must be a game-charmap, EOS-terminated string with its own `\n`
// line breaks already in place (this screen does not wrap), fitting this
// screen's message window - see the .c file's window layout comment. Copied
// via plain StringCopy into this module's EWRAM buffer on Init, so callers
// pass a `_(...)` literal or an already-StringExpandPlaceholders'd buffer.
//
// `hasYesNo` FALSE: `outResult` is written TRADE_CODE_PROMPT_ACK once A is
// pressed. TRUE: the player picks YES or NO (B counts as NO), the cursor
// starting on `yesNoDefaultNo ? NO : YES`.
//
// `callback` is where SetMainCallback2 goes once the screen is done.
void TradeCodePrompt_Init(const u8 *message, bool8 hasYesNo, bool8 yesNoDefaultNo, enum TradeCodePromptResult *outResult, MainCallback callback);

#endif // GUARD_TRADE_CODE_PROMPT_H
