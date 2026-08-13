#ifndef GUARD_TRADE_CODE_H
#define GUARD_TRADE_CODE_H

#include "global.h"
#include "config/trade_code.h"

// Offline, code-based trading. This header is the public surface for
// src/trade_code.c, filled in by the stages that need it:
//   Stage 1 - the bit stream + Base32 codec.
//   Stage 2 - the BoxPokemon serialiser/deserialiser.
//   Stage 3 - sealing, nonces and replay protection.
//   Stage 4 - save state (the TradeCodeState enum below; struct PendingTrade
//             itself lives in global.h - see this stage's status block).

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

// The payload spec's 2-bit `codeKind` header field (an offer code vs. a
// confirm code - see the plan doc's "Payload spec" section). Lives here,
// not in trade_code_session.c where Stage 7 first needed it, now that
// Stage 8's trade_code_receive.c needs the same two values for its own
// confirm-code validator - Stage 7's own status block anticipated this
// exact promotion ("Stage 8 will need the same two values... rather than
// this stage reaching forward to invent a shared header for a two-value
// enum with exactly one other consumer that doesn't exist yet").
enum TradeCodeKind
{
    TRADE_CODE_KIND_OFFER   = 0,
    TRADE_CODE_KIND_CONFIRM = 1,
};

// The Base32/Crockford symbol (a game-charmap byte) at `index` (0-31). The
// single source of truth for the alphabet - added for Stage 6's on-screen
// keyboard (src/trade_code_entry.c), so its 8x4 symbol grid draws exactly
// the glyphs TradeCode_Encode would produce, with no second copy of the
// alphabet in another file to drift out of sync. `index` >= 32 is a caller
// bug (asserted away, not clamped - Stage 6's grid is fixed 8x4 = 32 cells
// and never generates an out-of-range index).
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
// layer up (Stage 3's sealing, Stage 7's session code), since they're
// properties of the trade session, not of the Pokémon.
void TradeCode_SerializeMon(const struct BoxPokemon *boxMon, struct TradeCodeBits *stream);

// Reverses TradeCode_SerializeMon. Every field is read and validated before
// anything is written to `outBoxMon`; `outBoxMon` is only touched once the
// whole payload is known to be well-formed (TRADE_CODE_MON_OK).
enum TradeCodeMonStatus TradeCode_DeserializeMon(struct TradeCodeBits *stream, struct BoxPokemon *outBoxMon);

// ---------------------------------------------------------------------
// Stage 3: sealing, nonces, replay protection. No UI, no save data - the
// replay ring's *storage* is Stage 4's struct PendingTrade; these are the
// shared check/insert primitives so the eviction policy lives in one place.
// ---------------------------------------------------------------------

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
// already-assembled offer payload (header + mon fields; see the payload
// spec's "Seal" row), appended by the caller as the payload's final 32
// bits. `nBits` must not include the seal itself. The payload's core mon
// fields already carry species/otId (Stage 2), so hashing the whole
// payload already ties the seal to "this exact Pokemon" - no separate
// species/otId/personality salt is needed. Deliberately NOT salted with
// personality, unlike an earlier draft of this stage's plan: personality
// is the one field Stage 2 chose not to transmit at all, and salting with
// something the receiving cart can never reconstruct from the payload it
// just decoded would make the seal unverifiable, not more secure. `data`'s
// trailing bits past `nBits` in the final partial byte must be zero (every
// existing caller already zeroes its buffer before writing, per Stage 1/2
// convention).
u32 TradeCode_SealOffer(const u8 *payload, u32 nBits);

// The confirm code's 28-bit combined tag (see the payload spec: a confirm
// code is codeKind (2 bits) + this tag = TRADE_CODE_CONFIRM_CHARS symbols).
// Feeds both parties' full offer payloads (including each one's own seal)
// into TradeCode_Hash in a canonical order - whichever offer's `otId` is
// lower goes first, ties broken by the lower `nonce` - so both carts hash
// an identical concatenation regardless of who calls this "self". What
// makes the tag revealed to me differ from the tag I expect from my
// partner is which mon is passed as `self`: call once with (mine,
// partner's) for my own revealed code, and again with the two mons swapped
// to compute the tag I expect to receive (see Stage 7). otId/nonce are
// passed in rather than re-parsed out of the raw bit streams so this stays
// independent of Stage 2's exact field layout - by Step 3 the caller has
// already decoded and validated both mons and knows both nonces.
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
// enough that this needs no extra state in Stage 4's save struct).
void TradeCode_RecordOfferSeal(u32 ring[TRADE_CODE_REPLAY_RING], u32 seal);

// ---------------------------------------------------------------------
// Stage 4: save state. struct PendingTrade itself lives in global.h (as a
// SaveBlock2 member, it has to be declared before pokemon.h is #included
// there - see the struct's own comment), but the state machine it carries
// is this module's concern, so the enum lives here.
//
// Only COMMITTED is ever meaningfully read back after a reset (Stage 9)
// - OFFER_SHOWN and PARTNER_OFFER_ACCEPTED are session-transient and are
// never written to gSaveBlock2Ptr, so a reset during steps 1-2 simply loses
// the in-progress session, which is correct (nothing was given up yet).
// NONE is guaranteed to be zero, so a freshly-zeroed (or pre-Stage-4) save
// reads as "no pending trade" with no migration needed.
// ---------------------------------------------------------------------
enum TradeCodeState
{
    TRADE_CODE_STATE_NONE,
    TRADE_CODE_STATE_OFFER_SHOWN,
    TRADE_CODE_STATE_PARTNER_OFFER_ACCEPTED,
    TRADE_CODE_STATE_COMMITTED,
};

#endif // GUARD_TRADE_CODE_H
