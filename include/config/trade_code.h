#ifndef GUARD_CONFIG_TRADE_CODE_H
#define GUARD_CONFIG_TRADE_CODE_H

// Offline, code-based trading. Two players each pick a Pokémon, read a
// generated code to each other, and each cart materialises the other's
// Pokémon locally - no link hardware, no simultaneity.
#define TRADE_CODES TRUE

// Bumped whenever the payload layout in trade_code.c changes. A code
// carrying a different version is rejected outright rather than
// misparsed - see the `formatVersion` header field.
#define TRADE_CODE_FORMAT_VERSION 1

// Keyed-hash seed for the anti-theft seal (Stage 3). Baked into the ROM,
// so this deters casual code sharing - it is not real cryptography, since
// anyone willing to disassemble the ROM can recover it. Deliberately not a
// round number.
#define TRADE_CODE_SECRET 0x5C3A9F17

// Upper bound on a displayed/entered code's length in characters, including
// hyphens. The original 80 (headroom over the plan doc's published 78-char
// worst case) was flagged as stale by Stage 1's status block: Stage 2 had
// to switch both name fields from the doc's spec'd 7 bits/character to 8
// (this fork's charmap puts every real character above 0x7F - see Stage 2's
// status block), and met data/hyper-training were dropped instead, so the
// true worst case is no longer 78. Recomputed bit-for-bit against the
// actual Stage 2/3 implementation for Stage 5 (the code display screen,
// whose buffer this sizes): header-minus-presence (formatVersion 4 +
// codeKind 2 + nonce 16 = 22, Stage 7) + worst-case mon payload (presence 8
// + 93 bits of always-present core/OT-name + 214 bits of every optional
// field present = 374, see TradeCode_SerializeMon) + seal (32) = 428 bits
// -> 86 Base32 symbols -> 17 group hyphens -> 103 displayed characters.
// 112 leaves a little headroom without wasting much EWRAM (the display and
// entry screens' buffers are both sized off this).
#define TRADE_CODE_MAX_CHARS 112

// Codes are displayed/entered in hyphen-separated groups of this many
// Base32 symbols (e.g. "M4K7Q-2WXNB-...").
#define TRADE_CODE_GROUP_SIZE 5

// A confirm code is always exactly this many characters (codeKind, 2 bits,
// plus a 28-bit tag - see the payload spec), one group, no hyphens.
#define TRADE_CODE_CONFIRM_CHARS 6

// Size of the per-save ring buffer of recently redeemed offer seals
// (struct PendingTrade.recentOfferSeals, Stage 4). Rejects replaying the
// same offer code on the same cart.
#define TRADE_CODE_REPLAY_RING 8

#endif // GUARD_CONFIG_TRADE_CODE_H
