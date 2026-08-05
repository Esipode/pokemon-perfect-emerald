#include "global.h"
#include "randomization.h"
#include "event_data.h"
#include "pokemon.h"
#include "ui_birch_case.h"
#include "constants/flags.h"
#include "constants/moves.h"
#include "constants/pokemon.h"

// This module is the single place that decides WHETHER a mon's type or move
// is randomized and HOW the pieces (dual types, per-slot movesets) combine.
// The underlying RNG primitives (GetRandomType/GetRandomMove/GetRandomMoveType)
// still live in ui_birch_case.c for now; callers should migrate to the
// functions below instead of checking FLAG_RANDOMIZE_TYPE / FLAG_RANDOMIZE_MOVES
// and calling those primitives directly.

void GetResolvedTypePair(u16 species, u8 *outType1, u8 *outType2)
{
    u8 originalType1 = gSpeciesInfo[species].types[0];
    u8 originalType2 = gSpeciesInfo[species].types[1];
    bool8 isOriginalDualType = (originalType2 != TYPE_NONE && originalType2 != originalType1);

    if (FlagGet(FLAG_RANDOMIZE_TYPE))
    {
        *outType1 = GetRandomType(species, 0);
        // Only randomize type2 if the original species had a dual type, so a
        // naturally single-typed species stays single-typed after resolving.
        if (isOriginalDualType)
            *outType2 = GetRandomType(species, 1);
        else
            *outType2 = *outType1;
    }
    else
    {
        *outType1 = originalType1;
        *outType2 = originalType2;
    }
}

u16 GetResolvedMove(u16 species, u16 originalMove)
{
    if (originalMove == MOVE_NONE)
        return originalMove;

    return GetEffectiveMove(originalMove, species);
}

u8 GetResolvedMoveType(u16 move, u8 baseType)
{
    if (FlagGet(FLAG_RANDOMIZE_TYPE))
        return GetRandomMoveType(move);

    return baseType;
}

void ResolveMonMoves(u16 species, const u16 *originalMoves, u16 *outMoves)
{
    u16 resolvedMoves[MAX_MON_MOVES];
    u32 moveIdx;
    u32 outCount = 0;

    for (moveIdx = 0; moveIdx < MAX_MON_MOVES; moveIdx++)
        resolvedMoves[moveIdx] = MOVE_NONE;

    for (moveIdx = 0; moveIdx < MAX_MON_MOVES; moveIdx++)
    {
        u16 originalMove = originalMoves[moveIdx];
        u16 resolvedMove;
        u32 dupIdx;
        bool8 isDuplicate = FALSE;

        if (originalMove == MOVE_NONE)
            continue;

        resolvedMove = GetResolvedMove(species, originalMove);

        // Two different original moves can resolve to the same randomized
        // move. Skip duplicates rather than wasting a move slot on a repeat,
        // matching the dedup behavior trainer-party building used to do
        // inline before it was centralized here.
        for (dupIdx = 0; dupIdx < outCount; dupIdx++)
        {
            if (resolvedMoves[dupIdx] == resolvedMove)
            {
                isDuplicate = TRUE;
                break;
            }
        }

        if (!isDuplicate)
            resolvedMoves[outCount++] = resolvedMove;
    }

    // Copy from a local buffer (not directly into outMoves) so this remains
    // safe to call with outMoves == originalMoves.
    for (moveIdx = 0; moveIdx < MAX_MON_MOVES; moveIdx++)
        outMoves[moveIdx] = resolvedMoves[moveIdx];
}

void ResolveMonData(u16 species, const u16 *originalMoves, struct ResolvedMonData *out)
{
    GetResolvedTypePair(species, &out->type1, &out->type2);
    ResolveMonMoves(species, originalMoves, out->moves);
}
