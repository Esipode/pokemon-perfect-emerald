#ifndef GUARD_TRADE_CODE_DISPLAY_H
#define GUARD_TRADE_CODE_DISPLAY_H

#include "main.h"
#include "config/trade_code.h"

// Shared code-grid layout constants, public so trade_code_entry.c lays its
// typed-code field out on the identical grid.
//
// The code is laid out on a strict monospace grid rather than relying on
// proportional glyph widths: with codes up to ~105 characters, proportional
// printing could clip on real hardware, while a fixed grid cannot clip
// mid-glyph.
#define TRADE_CODE_DISPLAY_GROUPS_PER_ROW 5
#define TRADE_CODE_DISPLAY_SYMBOLS_PER_ROW (TRADE_CODE_DISPLAY_GROUPS_PER_ROW * TRADE_CODE_GROUP_SIZE)
#define TRADE_CODE_DISPLAY_MAX_ROWS 4
// 25 symbols + 4 internal (mid-row) hyphens = 29 monospace cells/row.
#define TRADE_CODE_DISPLAY_ROW_CAPACITY (TRADE_CODE_DISPLAY_SYMBOLS_PER_ROW + TRADE_CODE_DISPLAY_GROUPS_PER_ROW - 1)
// FONT_SHORT_NARROW's own maxLetterWidth/maxLetterHeight (src/text.c) are
// 5px/14px - an 8x16 cell gives every glyph clear margin on all sides.
#define CODE_CELL_WIDTH  8
#define CODE_CELL_HEIGHT 16

// A read-only, full-screen display for a generated trade code (offer or
// confirm). Modelled on ui_stat_editor.c's CB2_/Task_-driven full-screen
// pattern. No entry, decoding or session logic here.
//
// `codeStr` must be a game-charmap, EOS-terminated, hyphen-grouped string as
// produced by TradeCode_Encode, no longer than TRADE_CODE_MAX_CHARS. It is
// re-wrapped into up to TRADE_CODE_DISPLAY_MAX_ROWS rows at a
// TRADE_CODE_GROUP_SIZE-group boundary and copied via plain StringCopy into
// this module's EWRAM buffer, so the caller's buffer need not outlive the
// call. The copy relies on the length bound rather than truncating, so do not
// pass a longer or non-EOS-terminated buffer.
//
// `species` selects whether an offered-mon icon + name is shown, so the player
// can check they are reading out the code for the right Pokemon. Pass
// SPECIES_NONE to omit it (a confirm code is not tied to a specific Pokemon).
//
// `nickname` is the name printed next to the icon; NULL falls back to the
// species name. Ignored when `species` is SPECIES_NONE. Same length bound and
// StringCopy caveat as `codeStr` (POKEMON_NAME_LENGTH).
//
// `isConfirmCode` selects the header text: "YOUR TRADE CODE" when FALSE,
// "YOUR CONFIRM CODE" when TRUE.
//
// `callback` is invoked via SetMainCallback2 once the player presses A, so the
// session state machine can chain into whatever screen comes next.
void TradeCodeDisplay_Init(const u8 *codeStr, u16 species, const u8 *nickname, bool8 isConfirmCode, MainCallback callback);

#endif // GUARD_TRADE_CODE_DISPLAY_H
