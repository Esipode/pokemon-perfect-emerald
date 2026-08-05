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

// Resolves an entire moveset in one pass (MAX_MON_MOVES slots), preserving
// MOVE_NONE slots as-is. Safe to call with outMoves == originalMoves.
void ResolveMonMoves(u16 species, const u16 *originalMoves, u16 *outMoves);

#endif // GUARD_RANDOMIZATION_H
