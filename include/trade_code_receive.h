#ifndef GUARD_TRADE_CODE_RECEIVE_H
#define GUARD_TRADE_CODE_RECEIVE_H

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
// Parameterless for the same reason TradeCodeSession_Start (Stage 7) is -
// everything this needs lives in gSaveBlock2Ptr->pendingTrade already. For
// now (Stage 9's boot-time reset-resistant re-entry and Stage 10's real
// script wiring don't exist yet), this is reachable only from the debug
// menu (src/debug.c), exactly like every other stage's screen was before
// its own real entry point existed.
void TradeCodeReceive_Start(void);

#endif // GUARD_TRADE_CODE_RECEIVE_H
