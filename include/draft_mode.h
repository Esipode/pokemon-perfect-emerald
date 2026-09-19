#ifndef GUARD_DRAFT_MODE_H
#define GUARD_DRAFT_MODE_H

#include "global.h"

// Draft challenge mode. Nothing is caught: each new area offers a one-time
// pick from the Pokémon that live there, taken into an empty party slot or
// swapped for a party member who is released forever. Either way the area is
// spent.
//
// Mutually exclusive with Nuzlocke (see src/new_game_settings_menu.c). Reuses
// Nuzlocke's per-area flags (GET/SET_NUZLOCKE_ZONE_FLAG, keyed by MAPSEC) as
// "this area's draft is spent"; see the comment above
// NUM_NUZLOCKE_ZONE_FLAG_BYTES in include/global.h before touching this.
//
// Enabled state lives in gSaveBlock1Ptr->draftModeEnabled; 0 (also what old
// saves read back) means off.

// The Birch case UI draws a 9-ball case (4/3/2), so a route's pool is capped.
// Overflow is reduced deterministically; see Draft_BuildPool.
#define DRAFT_MAX_CHOICES 9

struct DraftChoice
{
    u16 species;
    u16 minLevel;
    u16 maxLevel;
    u16 level; // Resolved by Draft_BuildPool: a random level within [minLevel, min(maxLevel, level cap)]
};

// TRUE when Draft is turned on for this save.
bool32 Draft_IsEnabled(void);

// Draft_IsEnabled() plus FLAG_SYS_POKEDEX_GET. Engages at the same script node
// as FLAG_NUZLOCKE_CATCH_MODE, i.e. when Nuzlocke's restrictions start.
bool32 Draft_IsActive(void);

// TRUE when the current map's MAPSEC is an eligible, unspent area with at
// least one wild encounter. Deliberately not filtered by Mono Type/Mono Gen, so
// the field hook still fires and Draft_EventScript_RouteDraft can report
// "nothing eligible" via Draft_CheckPoolEligible. Excludes MAPSEC_NONE and
// MAPSEC_DYNAMIC (shared by 36 maps; battle facilities drop out because
// GetCurrentMapWildMonHeaderId only scans gWildMonHeaders).
bool32 Draft_IsAreaDraftable(void);

// Builds the current map's draft pool into `out` (room for DRAFT_MAX_CHOICES).
// Land + water encounters are unioned across all four times of day and deduped
// by species, keeping the widest [minLevel, maxLevel]. Returns the entry count.
// Each `level` is a random roll within its range clamped to the level cap, so
// a high-level rare slot (rock smash, fishing) doesn't leak its level.
//
// Filtered by MonoType_IsSpeciesAllowed / MonoGen_IsSpeciesAllowed when enabled;
// every species may be filtered out (see Draft_CheckPoolEligible).
//
// Pools over DRAFT_MAX_CHOICES are cut with a partial Fisher-Yates shuffle
// seeded from trainer ID and MAPSEC (stable across soft-resets, differs per
// playthrough and New Game+ cycle), then re-sorted into encounter-table order.
u32 Draft_BuildPool(struct DraftChoice *out);

// Pending-offer buffer for a drafted (or gift/egg) Pokémon until the party
// screen resolves it. EWRAM only: a soft-reset mid-offer loses the mon but
// leaves the area re-draftable.
bool32 Draft_HasPendingMon(void);

// `fromDraft` is TRUE for a route's own pick (src/ui_birch_case.c), FALSE for a
// gift/egg diverted from the PC (src/pokemon.c). Draft_MarkAreaSpent only
// spends the area for the former, so e.g. Steven's Beldum doesn't burn
// Granite Cave's draft.
void Draft_QueuePendingMon(struct Pokemon *mon, bool32 fromDraft);

// Script natives for the offer flow (data/scripts/draft.inc), used via callnative.

// gSpecialVar_Result: TRUE if the Mono-filtered pool has at least one legal
// pick. Lets Draft_EventScript_RouteDraft choose between the normal offer and
// Draft_EventScript_NoEligiblePool.
void Draft_CheckPoolEligible(void);

// Places the pending mon in the first empty slot below
// LimitedParty_GetMaxPartySize(). gSpecialVar_Result: 0 = joined, 1 = no room.
void Draft_TryGiveToEmptySlot(void);

// gStringVar1 = the pending mon's nickname-safe display name. Sets
// gSpecialVar_0x8004 to the joined party slot for ChangePokemonNickname.
void Draft_BufferPendingNickname(void);

// gStringVar1 = outgoing party mon at gSpecialVar_0x8004, gStringVar2 = the
// pending mon's display name.
void Draft_BufferReplacementNames(void);

// Replaces the party mon at gSpecialVar_0x8004 with the pending mon.
void Draft_DoReplacement(void);

// Discards the pending mon without placing it anywhere.
void Draft_DiscardPending(void);

// Marks the current area's draft as spent. Only Draft_EventScript_Finish may
// call it. No-ops when the resolved mon was a gift/egg (see fromDraft above).
void Draft_MarkAreaSpent(void);

// Marks the area spent with no offer made. Only called by
// Draft_EventScript_NoEligiblePool; unconditional, as no pending mon exists.
void Draft_MarkAreaSpentNoOffer(void);

// Field hook for ProcessPlayerFieldInput, run every frame the player has field
// control. Returns TRUE after starting a script when a gift/egg is pending
// (checked first) or the area has an unspent draft. Returns FALSE otherwise,
// including when Draft_IsEnabled() is false.
bool32 Draft_TryStartFieldScript(void);

#endif // GUARD_DRAFT_MODE_H
