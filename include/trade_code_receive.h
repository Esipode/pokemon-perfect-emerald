#ifndef GUARD_TRADE_CODE_RECEIVE_H
#define GUARD_TRADE_CODE_RECEIVE_H

#include "main.h"

// Step 4, the commit and swap: entering the partner's confirm code,
// materialising gSaveBlock2Ptr->pendingTrade.incoming into the party (or the
// PC, if the party is full), trade evolution, and the second force-save.
//
// A separate entry point from trade_code_session.c. Step 4 shares no live
// state with Steps 1-3 (the session's struct is freed once the commit
// finishes) other than gSaveBlock2Ptr->pendingTrade, and can happen much
// later: after a reset, or after the cart has been off for a day waiting on
// the partner's confirm code.
//
// Does NOT reuse CB2_InitInGameTrade (src/trade.c): by Step 4 there is no
// outgoing mon left to animate (Step 3 escrowed it), and that function reads
// OT name/nickname from the static sIngameTrades[] table via
// gSpecialVar_0x8005, not from the partner mon. Trade evolution uses
// BeginEvolutionScene, which needs no trade-animation setup.
//
// `returnCallback` is where SetMainCallback2 goes once this screen is done
// (success, giving up, or a corrupted pendingTrade cleared on resume). Callers
// differ: the debug menu and mid-game scripts want CB2_ReturnToField, while the
// boot-time re-entry (CB2_ContinueSavedGame, src/overworld.c) passes
// CB2_ContinueSavedGame itself, since the map is not loaded yet and re-invoking
// it (with pendingTrade.state now NONE) is what loads it.
//
// This screen never routes through CB2_ReturnToField/gFieldCallback itself
// (see trade_code_session.c's top-of-file comment); `returnCallback` is
// invoked once, at the very end.
//
// Guards its own preconditions rather than trusting the caller: a state other
// than TRADE_CODE_STATE_COMMITTED shows a plain "nothing waiting" message, and
// a pendingTrade.incoming failing TradeCode_ValidatePendingBoxMon (corrupted
// or hand-tampered save) is never materialised. Either way, the state is back
// at TRADE_CODE_STATE_NONE (force-saved) before `returnCallback` runs, so a
// caller that re-invokes itself as `returnCallback` never loops.
//
// Also the only place the player can walk away from a COMMITTED trade: "give
// up on this trade?", reached by pressing B on an empty confirm-code field,
// permanently forfeits the incoming Pokemon (Step 3's escrow has no undo; the
// partner's copy is already gone). Without some way out, a partner who never
// sends a valid confirm code would trap the player in this screen on every
// boot. Giving up is deliberately not tracked or surfaced (see the
// pendingTrade.abandonedCount note in trade_code_receive.c): a save file is
// trivially duplicable, so tracking cannot stop a determined duper, and a
// player reaching this path is as likely to have been ghosted as to be
// ghosting.
void TradeCodeReceive_Start(MainCallback returnCallback);

#endif // GUARD_TRADE_CODE_RECEIVE_H
