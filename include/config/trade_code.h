#ifndef GUARD_CONFIG_TRADE_CODE_H
#define GUARD_CONFIG_TRADE_CODE_H

// Offline, code-based trading. Two players each pick a Pokémon, read a
// generated code to each other, and each cart materialises the other's
// Pokémon locally - no link hardware, no simultaneity.
//
// Defined as a literal 1, not TRUE: this is tested with #if inside
// data/scripts/*.inc, whose cpp pass never sees include/gba/defines.h, so
// `#if TRADE_CODES` would evaluate as 0 if defined as TRUE.
#define TRADE_CODES 1

// Bumped whenever the payload layout in trade_code.c changes. A code
// carrying a different version is rejected outright rather than
// misparsed - see the `formatVersion` header field.
#define TRADE_CODE_FORMAT_VERSION 1

// Keyed-hash seed for the anti-theft seal. Baked into the ROM, so this deters
// casual code sharing - it is not real cryptography, since anyone willing to
// disassemble the ROM can recover it.
#define TRADE_CODE_SECRET 0x5C3A9F17

// Upper bound on a displayed/entered code's length in characters, including
// hyphens. Worst case is 432 bits (22 header + 374 mon payload with every
// optional field + 32 seal, plus a byte-alignment pad before the seal), i.e.
// 87 Base32 symbols + 17 group hyphens = 104 characters. The display and entry buffers
// are sized off this.
#define TRADE_CODE_MAX_CHARS 112

// Codes are displayed/entered in hyphen-separated groups of this many
// Base32 symbols (e.g. "M4K7Q-2WXNB-...").
#define TRADE_CODE_GROUP_SIZE 5

// Bytes needed to hold one built offer payload (header + mon + pad + seal)
// before Base32 encoding: the 432-bit worst case (see TRADE_CODE_MAX_CHARS) is
// 54 bytes, plus headroom. Shared with TRADE_CODE_SESSION_OFFER_MAX_BITS/8 and
// struct PendingTrade, which keeps the player's own offer payload while
// COMMITTED so "view offer code" can redisplay it without the original mon.
#define TRADE_CODE_OFFER_PAYLOAD_BYTES 56

// A confirm code is always exactly this many characters (codeKind, 2 bits,
// plus a 28-bit tag), one group, no hyphens.
#define TRADE_CODE_CONFIRM_CHARS 6

// Size of the per-save ring of recently redeemed offer seals
// (struct PendingTrade.recentOfferSeals). Rejects replaying the same offer code
// on the same cart.
#define TRADE_CODE_REPLAY_RING 8

#endif // GUARD_CONFIG_TRADE_CODE_H
