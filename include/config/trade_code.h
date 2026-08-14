#ifndef GUARD_CONFIG_TRADE_CODE_H
#define GUARD_CONFIG_TRADE_CODE_H

// Offline, code-based trading. Two players each pick a Pokémon, read a
// generated code to each other, and each cart materialises the other's
// Pokémon locally - no link hardware, no simultaneity.
//
// Defined as a literal 1, not TRUE - unlike most config flags, this one is
// tested with #if inside data/scripts/*.inc script files (preprocessed
// through the assembler's own cpp pass), not just C files. TRUE/FALSE
// (include/gba/defines.h) are never pulled into that preprocessing chain,
// so `#if TRADE_CODES` would silently evaluate as false there if this were
// defined as TRUE (an undefined identifier in a #if is treated as 0) -
// exactly the failure mode this fork's own IS_FRLG already avoids by using
// a literal 1/0 (include/constants/global.h) instead of TRUE/FALSE, for
// the same reason. Still a real boolean value in C - TRUE is itself just
// `#define TRUE 1` - so this changes nothing for any C-side #if TRADE_CODES.
#define TRADE_CODES 1

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
// field present = 374, see TradeCode_SerializeMon) + seal (32) = 428 bits.
// Stage 7 (src/trade_code_session.c) then had to insert a byte-alignment
// pad between the mon payload and the seal (TradeCode_SealOffer hashes
// whole bytes - see TradeCodeSession_BuildOffer's own comment), pushing
// the true worst case to 432 bits -> 87 Base32 symbols -> 17 group hyphens
// -> 104 displayed characters. 112 still comfortably covers this (the
// display and entry screens' buffers are both sized off this).
#define TRADE_CODE_MAX_CHARS 112

// Codes are displayed/entered in hyphen-separated groups of this many
// Base32 symbols (e.g. "M4K7Q-2WXNB-...").
#define TRADE_CODE_GROUP_SIZE 5

// Bytes needed to hold one already-built offer payload's raw bits (header +
// mon + pad + seal) verbatim, before Base32 text encoding - see TRADE_CODE_
// MAX_CHARS' own derivation above for the 432-bit worst case this is sized
// off (a few bytes of headroom over the exact ceil(432/8)=54, matching
// src/trade_code_session.c's own TRADE_CODE_SESSION_OFFER_MAX_BITS/8, which
// reuses this same constant so the two can't quietly drift apart). Shared
// here rather than left file-local because struct PendingTrade (include/
// global.h) also needs to size a buffer of this shape: post-Stage-10, a
// player's own already-built offer payload is kept around for as long as
// their trade stays COMMITTED, so the attendant's "view offer code" option
// can redisplay it verbatim without the original Pokemon still existing in
// the party to re-derive it from.
#define TRADE_CODE_OFFER_PAYLOAD_BYTES 56

// A confirm code is always exactly this many characters (codeKind, 2 bits,
// plus a 28-bit tag - see the payload spec), one group, no hyphens.
#define TRADE_CODE_CONFIRM_CHARS 6

// Size of the per-save ring buffer of recently redeemed offer seals
// (struct PendingTrade.recentOfferSeals, Stage 4). Rejects replaying the
// same offer code on the same cart.
#define TRADE_CODE_REPLAY_RING 8

#endif // GUARD_CONFIG_TRADE_CODE_H
