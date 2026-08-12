#ifndef GUARD_TRADE_CODE_H
#define GUARD_TRADE_CODE_H

#include "global.h"
#include "config/trade_code.h"

// Offline, code-based trading (see "Trading Codes.md"). This header is the
// public surface for src/trade_code.c; it is deliberately left empty here
// in Stage 0 (guard rails only, no behaviour) and filled in by the stages
// that follow:
//   Stage 1 - the bit stream + Base32 codec (TradeCode_Encode/Decode).
//   Stage 2 - the BoxPokemon serialiser/deserialiser.
//   Stage 3 - sealing, nonces and replay protection.

#endif // GUARD_TRADE_CODE_H
