#ifndef GUARD_RANDOMIZATION_H
#define GUARD_RANDOMIZATION_H

// Single resolver layer for move/type randomization. Every caller needing a
// mon's effective type or move (summary screen, relearner, battle setup, party
// menu) goes through here instead of checking FLAG_RANDOMIZE_TYPE /
// FLAG_RANDOMIZE_MOVES and calling the RNG helpers inline. Resolving starts from
// the original (unrandomized) data and is deterministic per save, so repeated
// calls never compound randomization.

#include "global.h"

// Effective mon data after randomization. Every field derives from the mon's
// original stored data, never a previously-resolved value.
struct ResolvedMonData
{
    u8 type1;
    u8 type2;
    u16 moves[MAX_MON_MOVES];
};

// Resolves the effective type 1/2 for a species (FLAG_RANDOMIZE_TYPE). Off:
// the species' real types. On: a single-typed species stays single-typed
// (type2 == type1); a dual-typed one gets two independently resolved types.
void GetResolvedTypePair(u16 species, u8 *outType1, u8 *outType2);

// Resolves the effective move for an original move slot (FLAG_RANDOMIZE_MOVES).
// Returns originalMove unchanged if off or MOVE_NONE. Always pass the ORIGINAL
// move (level-up data / trainer party / saved mon), never a resolved one.
u16 GetResolvedMove(u16 species, u16 originalMove);

// Resolves a move's effective type for display/battle. Pass the type the
// caller would otherwise use (including dynamic overrides like Hidden Power) as
// baseType; it is returned unchanged unless FLAG_RANDOMIZE_TYPE is on.
u8 GetResolvedMoveType(u16 move, u8 baseType);

// Resolves a whole moveset (MAX_MON_MOVES slots) strictly slot-for-slot:
// MOVE_NONE stays MOVE_NONE, slots are never reordered or packed. Safe with
// outMoves == originalMoves.
void ResolveMonMoves(u16 species, const u16 *originalMoves, u16 *outMoves);

// Max PP of a stored move slot. Stored PP tracks the RESOLVED move, so size
// refills and caps against the resolved move, not the original.
u8 GetResolvedMovePP(u16 species, u16 originalMove, u8 ppBonuses, u8 slot);

// Resolves a mon's full effective data (types + moveset) from its original
// species and moves. Shared by display code and battle setup so they cannot drift.
void ResolveMonData(u16 species, const u16 *originalMoves, struct ResolvedMonData *out);

#endif // GUARD_RANDOMIZATION_H
