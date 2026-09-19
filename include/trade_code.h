#ifndef GUARD_TRADE_CODE_H
#define GUARD_TRADE_CODE_H

#include "global.h"
#include "config/trade_code.h"

// Offline, code-based trading. Public surface for src/trade_code.c: bit stream,
// Base32 codec, BoxPokemon serialiser, sealing and replay protection.

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

// The payload's 2-bit `codeKind` header field: an offer code vs. a confirm
// code. Shared by trade_code_session.c and trade_code_receive.c.
enum TradeCodeKind
{
    TRADE_CODE_KIND_OFFER   = 0,
    TRADE_CODE_KIND_CONFIRM = 1,
};

// The Base32/Crockford symbol (a game-charmap byte) at `index` (0-31). Single
// source of truth for the alphabet; the entry screen's 8x4 grid draws from it.
// `index` >= 32 is a caller bug (asserted, not clamped).
u8 TradeCode_AlphabetSymbol(u32 index);

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
// layer up (sealing, session code), since they're properties of the trade
// session, not of the Pokémon.
void TradeCode_SerializeMon(const struct BoxPokemon *boxMon, struct TradeCodeBits *stream);

// Reverses TradeCode_SerializeMon. Every field is read and validated before
// anything is written to `outBoxMon`; `outBoxMon` is only touched once the
// whole payload is known to be well-formed (TRADE_CODE_MON_OK).
enum TradeCodeMonStatus TradeCode_DeserializeMon(struct TradeCodeBits *stream, struct BoxPokemon *outBoxMon);

// Sealing, nonces and replay protection. The replay ring's storage is
// struct PendingTrade; these primitives keep the eviction policy in one place.

// FNV-1a over `data[0..len)`, keyed with TRADE_CODE_SECRET ^ salt, then a
// murmur3-style avalanche finalizer (xor-shift + multiply, twice, plus a
// final xor-shift) so a single flipped input bit changes roughly half the
// output bits. Deliberately NOT Crc32B (src/random.c) - CRC32 is linear, so
// a forger can patch a payload and keep a CRC valid; this can't be
// cancelled out the same way. Still not real cryptography - see the plan
// doc's "Honest note on cryptographic strength" - the secret is a
// compile-time constant baked into a public ROM.
u32 TradeCode_Hash(const u8 *data, u32 len, u32 salt);

// The offer code's anti-tamper seal: a keyed hash over every bit of the
// assembled offer payload (header + mon fields), appended by the caller as the
// payload's final 32 bits. `nBits` must not include the seal itself. The core
// mon fields already carry species/otId, so hashing the whole payload ties the
// seal to this exact Pokemon. Deliberately NOT salted with personality: it is
// never transmitted, so the receiving cart could not reconstruct the salt and
// the seal would be unverifiable. Trailing bits past `nBits` in the final
// partial byte must be zero.
u32 TradeCode_SealOffer(const u8 *payload, u32 nBits);

// The confirm code's 28-bit combined tag (a confirm code is codeKind
// (2 bits) + this tag = TRADE_CODE_CONFIRM_CHARS symbols).
// Feeds both parties' full offer payloads (including each one's own seal)
// into TradeCode_Hash in a canonical order - whichever offer's `otId` is
// lower goes first, ties broken by the lower `nonce` - so both carts hash
// an identical concatenation regardless of who calls this "self". What
// makes the tag revealed to me differ from the tag I expect from my
// partner is which mon is passed as `self`: call once with (mine,
// partner's) for my own revealed code, and again with the two mons swapped
// to compute the tag I expect to receive. otId/nonce are passed in rather than
// re-parsed from the bit streams so this stays independent of the field layout.
u32 TradeCode_ConfirmTag(const u8 *offerSelf, u32 lenSelfBits, u32 otIdSelf, u16 nonceSelf,
                          const u8 *offerPartner, u32 lenPartnerBits, u32 otIdPartner, u16 noncePartner);

// TRUE if `seal` already appears in `ring` - this exact offer code has
// already been redeemed on this cart. A zero entry means "unused slot" (a
// freshly-zeroed save never false-positives); a genuinely redeemed code
// whose seal happens to hash to exactly 0 would be indistinguishable from
// an empty slot, but that's a 1-in-2^32 event, in the same spirit as the
// otId collision handled in TradeCode_DeserializeMon.
bool32 TradeCode_IsOfferSealUsed(const u32 ring[TRADE_CODE_REPLAY_RING], u32 seal);

// Inserts `seal` at the front of `ring`, dropping the oldest entry (a
// simple shift, not a cursor-indexed ring - TRADE_CODE_REPLAY_RING is small
// enough that this needs no extra save state).
void TradeCode_RecordOfferSeal(u32 ring[TRADE_CODE_REPLAY_RING], u32 seal);

// State machine carried by struct PendingTrade (declared in global.h).
//
// Only COMMITTED is read back after a reset. OFFER_SHOWN and
// PARTNER_OFFER_ACCEPTED are session-transient and never written to
// gSaveBlock2Ptr, so a reset before commit loses the session with nothing
// given up. NONE must stay zero so a zeroed save reads as "no pending trade".
enum TradeCodeState
{
    TRADE_CODE_STATE_NONE,
    TRADE_CODE_STATE_OFFER_SHOWN,
    TRADE_CODE_STATE_PARTNER_OFFER_ACCEPTED,
    TRADE_CODE_STATE_COMMITTED,
};

// TRUE if `boxMon` looks safe to materialise as the incoming half of a
// resumed COMMITTED trade. Not a re-run of TradeCode_DeserializeMon's
// validation: a corrupted or hand-edited save sector can hold bytes it never
// produced. Checks:
//   - the checksum still matches the encrypted data (MON_DATA_SANITY_IS_BAD_EGG);
//   - the species is inside the range TradeCode_DeserializeMon enforces.
// Works on a copy, since GetBoxMonData lazily mutates isBadEgg; `boxMon` is
// left untouched.
bool32 TradeCode_ValidatePendingBoxMon(const struct BoxPokemon *boxMon);

#endif // GUARD_TRADE_CODE_H
