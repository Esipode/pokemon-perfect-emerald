#ifndef GUARD_TRADE_CODE_DISPLAY_H
#define GUARD_TRADE_CODE_DISPLAY_H

#include "main.h"

// Stage 5 of "Trading Codes.md": a read-only, full-screen display for a
// generated trade code (offer or confirm). Modelled on ui_stat_editor.c's
// CB2_/Task_-driven full-screen pattern (see that file's "Begin Generic UI
// Initialization Code" section) - same malloc'd-EWRAM-BG, gMain.state gfx
// setup, VBlank/main callback split. No entry, no decoding, no session
// logic here - Stage 6 builds the paired entry screen, Stage 7 the state
// machine that decides what "A" leads to next.
//
// `codeStr` must be a game-charmap, EOS-terminated, hyphen-grouped string
// as produced by TradeCode_Encode (include/trade_code.h), no longer than
// TRADE_CODE_MAX_CHARS. This module only re-wraps that single string into
// up to TRADE_CODE_DISPLAY_MAX_ROWS display rows at a TRADE_CODE_GROUP_SIZE-
// group boundary - it never touches the codec itself. Copied via plain
// StringCopy into this module's own EWRAM buffer on Init (so the caller's
// buffer doesn't need to outlive the call) - the copy relies on that length
// bound rather than truncating defensively, so don't hand this a longer or
// non-EOS-terminated buffer.
//
// `species` selects whether an offered-mon icon + name is shown alongside
// the code, so the player can sanity-check they're reading out the code for
// the right Pokemon (see the plan doc's Stage 5 bullet list). Pass
// SPECIES_NONE to omit it entirely - the confirm code screen has nothing to
// show here, since a confirm code is a proof-of-escrow tag, not tied to
// display of a specific Pokemon.
//
// `nickname` is the name printed next to the icon; pass NULL to fall back
// to the species name (mirrors TradeCode_SerializeMon's own "nickname
// absent" default). Ignored when `species` is SPECIES_NONE. Same length
// bound as `codeStr` (POKEMON_NAME_LENGTH), same plain-StringCopy caveat.
//
// `isConfirmCode` selects the header text: "YOUR TRADE CODE" when FALSE,
// "YOUR CONFIRM CODE" when TRUE.
//
// `callback` is invoked via SetMainCallback2 once the player presses A -
// mirrors StatEditor_Init's callback-in-as-parameter shape (not a fixed
// CB2_ReturnToField read back out of gMain.savedCallback), so Stage 7's
// session state machine can chain straight into whatever screen comes next
// without this module needing to know about it.
void TradeCodeDisplay_Init(const u8 *codeStr, u16 species, const u8 *nickname, bool8 isConfirmCode, MainCallback callback);

#endif // GUARD_TRADE_CODE_DISPLAY_H
