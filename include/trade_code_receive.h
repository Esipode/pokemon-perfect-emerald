#ifndef GUARD_TRADE_CODE_RECEIVE_H
#define GUARD_TRADE_CODE_RECEIVE_H

#include "main.h"

// Stage 8 of "Trading Codes.md": Step 4, the commit and swap - entering
// the partner's confirm code, materialising gSaveBlock2Ptr->pendingTrade.
// incoming into the party (or the PC, if the party is full), trade
// evolution, and the second force-save.
//
// Deliberately a separate file/entry point from Stage 7's trade_code_
// session.c, not an extension of it. Step 4 shares no live state with
// Steps 1-3 - that session's own struct TradeCodeSessionState is already
// freed by the time Step 3's commit finishes (see trade_code_session.c's
// TradeCodeSession_DoCommit) - other than gSaveBlock2Ptr->pendingTrade
// itself, and Step 4 can genuinely happen much later: after a reset, or
// after the cart's been switched off for a day waiting on the partner to
// read back their own confirm code.
//
// This stage does NOT reuse CB2_InitInGameTrade (src/trade.c) the way the
// plan doc's own Stage 8 bullet list originally sketched - see this
// stage's status block in the plan doc for the full reasoning (in short:
// by Step 4 there is no real "outgoing" mon left to animate - Step 3
// already escrowed it away - and CB2_InitInGameTrade's own text-buffering
// path reads OT name/nickname from the static sIngameTrades[] NPC-trade
// table via gSpecialVar_0x8005, not from the actual partner mon, which
// would show the wrong trainer/Pokemon names). Trade evolution instead
// reuses evolution_scene.h's own general-purpose BeginEvolutionScene, the
// same self-contained "your Pokemon is evolving!" cutscene any other
// evolution trigger in this codebase already uses, which needs no
// trade-animation-specific setup at all.
//
// `returnCallback` is where SetMainCallback2 goes once this screen is
// entirely done with - Step 4 completing successfully, the player giving
// up on the trade (see below), or a corrupted pendingTrade being cleared
// on resume - the same explicit-parameter shape Stage 5/6's own Init
// functions use (include/trade_code_display.h/trade_code_entry.h), and for
// the same reason: this module doesn't need to know what should happen
// next. Two real callers need two different answers here. Stage 8's own
// debug-menu entry point (src/debug.c) and Stage 10's eventual mid-game
// script wiring both want CB2_ReturnToField - the field is already loaded
// and running. Stage 9's own boot-time reset-resistant re-entry
// (CB2_ContinueSavedGame, src/overworld.c) wants CB2_ContinueSavedGame
// itself instead - at that point in a Continue, the field/map hasn't been
// loaded for this session yet at all, and re-invoking that same function
// (this time with pendingTrade.state left at TRADE_CODE_STATE_NONE) is
// what actually runs that loading and hands control to the player for the
// first time.
//
// This screen never routes through CB2_ReturnToField/gFieldCallback
// itself, no matter what `returnCallback` is - see trade_code_session.c's
// own top-of-file comment for why that machinery isn't safe to depend on
// mid-flow. `returnCallback` is only ever invoked once, right at the very
// end, exactly like every other screen in this feature already does.
//
// Guards its own "is a trade actually pending, and is it safe to
// materialise" preconditions internally rather than trusting the caller:
// gSaveBlock2Ptr->pendingTrade.state != TRADE_CODE_STATE_COMMITTED shows a
// plain "nothing waiting" message (the debug menu can call this with
// nothing pending; Stage 9's own boot hook only calls this after checking
// the same state itself, so this is a second, defensive check on that
// path, not the only one), and a pendingTrade.incoming that fails
// TradeCode_ValidatePendingBoxMon (a corrupted or hand-tampered save) is
// never materialised - see that function's own comment (trade_code.h) for
// exactly what it checks. Either way, gSaveBlock2Ptr->pendingTrade.state
// ends up back at TRADE_CODE_STATE_NONE (force-saved) before
// `returnCallback` is ever invoked, so a caller like the Stage 9 boot hook
// that re-invokes itself as `returnCallback` never loops.
//
// Also the only place in this feature the player can walk away from a
// COMMITTED trade without completing it - "give up on this trade?", reached
// by pressing B on an empty confirm-code field, permanently forfeits the
// incoming Pokemon (there is no partial undo of Step 3's escrow - the
// partner's own copy is already gone regardless of what happens here). This
// isn't in the plan doc's own Stage 9 bullet list, which only says "no
// cancel and no walk-away" - added because of that same stage's own dev
// note: without *some* way out, a partner who never sends back a valid
// confirm code leaves this player stuck re-entering this exact screen every
// single boot, forever, with no way to just go play the game. Entering the
// correct confirm code is still the only way to actually *receive* the
// Pokemon; giving up just stops blocking the rest of the game. Deliberately
// not tracked or surfaced anywhere (see src/trade_code_receive.c's own
// header comment on pendingTrade.abandonedCount) - a save file is trivially
// duplicable outside the game, so this can't actually stop a determined
// duper, and a player reaching this path is at least as likely to have been
// ghosted as to be the one ghosting.
void TradeCodeReceive_Start(MainCallback returnCallback);

#endif // GUARD_TRADE_CODE_RECEIVE_H
