#ifndef GUARD_RANDOMIZATION_H
#define GUARD_RANDOMIZATION_H

// Single resolver layer for move/type randomization.
//
// Every caller that needs to know a mon's "effective" type or move (summary
// screen, relearner, battle setup, party menu, etc.) should go through this
// module instead of checking FLAG_RANDOMIZE_TYPE / FLAG_RANDOMIZE_MOVES and
// calling the RNG helpers inline. Resolving always starts from the mon's
// original (unrandomized) data and is deterministic for a given save, so
// calling these functions repeatedly on the same input is always safe and
// never compounds randomization on top of itself.

#include "global.h"

// The data contract for "effective" mon data: everything a UI, relearner,
// battle-setup, or summary-screen caller needs after randomization has been
// applied. Every field is derived from the mon's original stored data —
// nothing in here is ever built from a previously-resolved value, so a
// caller can freely re-resolve without compounding randomization.
struct ResolvedMonData
{
    u8 type1;
    u8 type2;
    u16 moves[MAX_MON_MOVES];
};

// Resolves the effective type 1/2 for a species, applying FLAG_RANDOMIZE_TYPE.
// When randomization is off, this simply mirrors the species' real types.
// When on, a single-typed species stays single-typed (type2 == type1) and a
// dual-typed species gets two independently resolved random types.
void GetResolvedTypePair(u16 species, u8 *outType1, u8 *outType2);

// Resolves the effective move for a species from its original move slot,
// applying FLAG_RANDOMIZE_MOVES. Returns originalMove unchanged if the flag
// is off or originalMove is MOVE_NONE. Always pass the ORIGINAL move (the
// one from level-up data / the trainer party table / the saved mon) — never
// feed an already-resolved move back in, or it will be re-randomized.
u16 GetResolvedMove(u16 species, u16 originalMove);

// Resolves the effective type of a move for display/battle purposes. Pass in
// the type the caller would otherwise use (base move type, or a dynamic type
// override such as Hidden Power/Weather Ball already applied) as baseType;
// this returns it unchanged unless FLAG_RANDOMIZE_TYPE is on, in which case
// it overrides with the resolved random move type.
u8 GetResolvedMoveType(u16 move, u8 baseType);

// Resolves an entire moveset in one pass (MAX_MON_MOVES slots), strictly
// slot-for-slot: outMoves[i] is always the resolved counterpart of
// originalMoves[i], and a MOVE_NONE slot stays MOVE_NONE. Slots are never
// reordered or packed, so every caller keeps the mon's real slot layout.
// Safe to call with outMoves == originalMoves.
void ResolveMonMoves(u16 species, const u16 *originalMoves, u16 *outMoves);

// Max PP of a stored move slot. A slot's stored PP always tracks the RESOLVED
// move (that's the move the player sees and actually spends), so anything that
// refills or caps a slot has to size it against the resolved move rather than
// the original stored one.
u8 GetResolvedMovePP(u16 species, u16 originalMove, u8 ppBonuses, u8 slot);

// Resolves a mon's full effective data (types + moveset) in one call from its
// original species and original stored moves. This is the entry point
// display code (summary screen) and battle setup should share, so the two
// paths can never independently drift from each other.
void ResolveMonData(u16 species, const u16 *originalMoves, struct ResolvedMonData *out);

#endif // GUARD_RANDOMIZATION_H
