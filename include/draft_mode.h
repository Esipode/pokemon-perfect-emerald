#ifndef GUARD_DRAFT_MODE_H
#define GUARD_DRAFT_MODE_H

#include "global.h"

// Draft challenge mode. You never catch anything - each new area offers a
// one-time pick from the Pokémon that naturally live there, taken into an
// empty party slot or traded against a party member who is released
// forever. Either way the area is spent.
//
// Draft is mutually exclusive with Nuzlocke (see src/new_game_settings_menu.c)
// and reuses Nuzlocke's per-area bookkeeping wholesale:
// gSaveBlock2Ptr->nuzlockeZoneCaughtFlags[] / GET_NUZLOCKE_ZONE_FLAG /
// SET_NUZLOCKE_ZONE_FLAG (include/global.h) double as "this area's draft is
// spent" in a Draft run, at zero extra save cost. The area key is MAPSEC,
// matching Nuzlocke - see the comment above NUM_NUZLOCKE_ZONE_FLAG_BYTES in
// include/global.h before touching this.
//
// Whether the mode is on lives in gSaveBlock1Ptr->draftModeEnabled. 0 means
// the mode is off, which is also what old saves read back.

// graphics/ui_birch_case/case_tiles.png draws a physical 9-ball case (4/3/2),
// so a route's draftable species pool is capped here. Overflow pools are
// reduced to this many with a deterministic, trainer-seeded pick - see
// Draft_BuildPool.
#define DRAFT_MAX_CHOICES 9

struct DraftChoice
{
    u16 species;
    u8 level;
};

// TRUE when the player has Draft mode turned on for this save, independent
// of whether a draft is currently obtainable anywhere.
bool32 Draft_IsEnabled(void);

// Draft_IsEnabled() plus FLAG_SYS_POKEMON_GET - mirrors the Nuzlocke/starter
// convention that per-area challenge mechanics don't engage before the
// player has their first Pokémon.
bool32 Draft_IsActive(void);

// TRUE when the current map's MAPSEC is a real, eligible, unspent area with
// at least one species to offer. Excludes MAPSEC_NONE and MAPSEC_DYNAMIC
// (the latter shared by 36 unrelated maps, including the battle facilities,
// which fall out for free since GetCurrentMapWildMonHeaderId only scans
// gWildMonHeaders).
bool32 Draft_IsAreaDraftable(void);

// Builds the current map's draft pool (land + water encounters, unioned
// across all four times of day, deduped by species and keeping the highest
// maxLevel seen) into `out`, which must have room for DRAFT_MAX_CHOICES
// entries. Returns the number of entries written, 0..DRAFT_MAX_CHOICES.
//
// Pools larger than DRAFT_MAX_CHOICES are reduced to DRAFT_MAX_CHOICES with
// a partial Fisher-Yates shuffle seeded from the trainer ID and the area's
// MAPSEC, so the chosen subset is stable across soft-resets but differs by
// playthrough (and re-rolls on a New Game+ cycle). The result is re-sorted
// into encounter-table order before it's copied out, so display stays
// predictable.
u32 Draft_BuildPool(struct DraftChoice *out);

// Pending-offer buffer. Holds a drafted-but-not-yet-placed Pokémon (or a
// gift/egg queued into the Draft offer flow) from the moment it's chosen
// until the party screen resolves it. Lives in EWRAM, never in the save: a
// soft-reset mid-offer loses the pending mon but leaves the area
// re-draftable, which is the safe direction to fail.
bool32 Draft_HasPendingMon(void);
void Draft_QueuePendingMon(struct Pokemon *mon);

#endif // GUARD_DRAFT_MODE_H
