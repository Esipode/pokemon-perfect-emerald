#ifndef GUARD_CONFIG_TRADE_CODE_H
#define GUARD_CONFIG_TRADE_CODE_H

// Offline, code-based trading. Two players each pick a Pokémon, read a
// generated code to each other, and each cart materialises the other's
// Pokémon locally - no link hardware, no simultaneity. See
// "Trading Codes.md" for the full protocol and payload spec.
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

// Upper bound on a displayed/entered code's length in characters,
// including hyphens - the longest spec'd payload (a fully custom
// competitive mon) is 78. Chosen with headroom for Stage 11's optional
// fields.
#define TRADE_CODE_MAX_CHARS 80

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
