#ifndef GUARD_TRADE_CODE_H
#define GUARD_TRADE_CODE_H

#include "global.h"
#include "config/trade_code.h"

// Offline, code-based trading. This header is the public surface for 
// src/trade_code.c, filled in by the stages that need it:
//   Stage 1 - the bit stream + Base32 codec.
//   Stage 2 - the BoxPokemon serialiser/deserialiser (this stage).
//   Stage 3 - sealing, nonces and replay protection.

// A bit stream over a caller-owned byte buffer, MSB-first.
//
// Writing: the caller sets `data` and `capacity` (the buffer's size, in
// bits) before the first TradeCode_WriteBits call; `bitPos` then grows as
// bits are written and doubles as "how many valid bits are in `data`" once
// writing is done - pass it straight to TradeCode_Encode as `nBits`.
//
// Reading (after TradeCode_Decode): `capacity` holds the number of valid
// decoded bits, not the raw buffer size, and `bitPos` grows as bits are
// consumed via TradeCode_ReadBits.
//
// Either direction latches `error` instead of touching memory outside
// `data` - a write or read that would cross `capacity` is a no-op (writes
// leave `data` unchanged; reads return 0) other than setting the flag.
struct TradeCodeBits
{
    u8 *data;
    u32 bitPos;
    u32 capacity;
    bool8 error;
};

enum TradeCodeStatus
{
    TRADE_CODE_OK,
    TRADE_CODE_BAD_CHAR,   // a byte that isn't a symbol, a fold, or a skip char
    TRADE_CODE_TOO_LONG,   // more decoded bits than the caller's buffer holds
    TRADE_CODE_TOO_SHORT,  // no symbol characters at all
};

// Writes the low `nBits` bits of `value` (1-32) into `stream`, MSB-first.
void TradeCode_WriteBits(struct TradeCodeBits *stream, u32 value, u32 nBits);

// Reads `nBits` bits (1-32) out of `stream`, MSB-first.
u32 TradeCode_ReadBits(struct TradeCodeBits *stream, u32 nBits);

// Base32/Crockford-encodes the first `nBits` bits of `bits` (trailing bits
// of a partial final symbol are zero-padded) into `outStr` as a
// game-charmap, EOS-terminated string, hyphenated every
// TRADE_CODE_GROUP_SIZE symbols. `outStr` must be large enough: symbol
// count is ceil(nBits / 5), plus one hyphen per full group after the
// first, plus the EOS terminator.
void TradeCode_Encode(const u8 *bits, u32 nBits, u8 *outStr);

// Decodes a game-charmap, EOS-terminated string produced by (or in the
// same alphabet as) TradeCode_Encode. `out->data` and `out->capacity` (the
// buffer and its size in bits) must be set by the caller before calling;
// on TRADE_CODE_OK, `out->capacity` is narrowed to the actual decoded bit
// count and `out->bitPos` is reset to 0, ready for TradeCode_ReadBits.
enum TradeCodeStatus TradeCode_Decode(const u8 *str, struct TradeCodeBits *out);

// Result of TradeCode_DeserializeMon. Anything other than TRADE_CODE_MON_OK
// means `outBoxMon` was left untouched - a malformed or tampered code must
// never produce a bad egg (or worse, an out-of-range read).
enum TradeCodeMonStatus
{
    TRADE_CODE_MON_OK,
    TRADE_CODE_MON_TRUNCATED,          // the stream ran out before every field was read
    TRADE_CODE_MON_RESERVED_BITS_SET,  // a presence bit this format version doesn't define was set
    TRADE_CODE_MON_BAD_SPECIES,
    TRADE_CODE_MON_BAD_LEVEL,
    TRADE_CODE_MON_BAD_NATURE,
    TRADE_CODE_MON_BAD_GENDER,
    TRADE_CODE_MON_BAD_EV_TOTAL,
    TRADE_CODE_MON_BAD_MOVE,
    TRADE_CODE_MON_BAD_ITEM,
    TRADE_CODE_MON_BAD_NAME,           // nickname/OT name length or bytes don't form a valid game-charmap string
    TRADE_CODE_MON_EGG_WITH_NICKNAME,
};

// Serialises `boxMon` into `stream`: the presence bitmap, the always-present
// core fields, the OT name, then whichever optional fields differ from
// their species/level default.
// Reads only through Get(Box)MonData - never pokes the struct directly.
//
// Does NOT write formatVersion/codeKind/nonce or the seal - those live one
// layer up (Stage 3's sealing, Stage 7's session code), since they're
// properties of the trade session, not of the Pokémon.
void TradeCode_SerializeMon(const struct BoxPokemon *boxMon, struct TradeCodeBits *stream);

// Reverses TradeCode_SerializeMon. Every field is read and validated before
// anything is written to `outBoxMon`; `outBoxMon` is only touched once the
// whole payload is known to be well-formed (TRADE_CODE_MON_OK).
enum TradeCodeMonStatus TradeCode_DeserializeMon(struct TradeCodeBits *stream, struct BoxPokemon *outBoxMon);

#endif // GUARD_TRADE_CODE_H
