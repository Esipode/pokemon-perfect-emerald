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

// Draft_IsEnabled() plus FLAG_SYS_POKEDEX_GET - Draft doesn't engage until
// the player has their Pokédex from Birch, after the first rival battle, not
// merely their starter. This is the same script node that sets
// FLAG_NUZLOCKE_CATCH_MODE, so it mirrors when Nuzlocke's own restrictions
// actually start too - see the comment on this function in draft_mode.c.
bool32 Draft_IsActive(void);

// TRUE when the current map's MAPSEC is a real, eligible, unspent area with
// at least one wild encounter defined - NOT necessarily one Draft_BuildPool
// will actually offer, since this check is intentionally unfiltered by Mono
// Type/Mono Gen (see Draft_BuildPool). That's what lets the field hook still
// engage - and Draft_EventScript_RouteDraft show a "nothing eligible here"
// message via Draft_CheckPoolEligible - on an area whose every species a
// Mono restriction happens to filter out. Excludes MAPSEC_NONE and
// MAPSEC_DYNAMIC (the latter shared by 36 unrelated maps, including the
// battle facilities, which fall out for free since
// GetCurrentMapWildMonHeaderId only scans gWildMonHeaders).
bool32 Draft_IsAreaDraftable(void);

// Builds the current map's draft pool (land + water encounters, unioned
// across all four times of day, deduped by species and keeping the highest
// maxLevel seen) into `out`, which must have room for DRAFT_MAX_CHOICES
// entries. Returns the number of entries written, 0..DRAFT_MAX_CHOICES.
//
// Filtered by MonoType_IsSpeciesAllowed / MonoGen_IsSpeciesAllowed when
// those modes are enabled, so a species this run isn't allowed to own never
// appears in the case UI - this is what actually enforces those
// restrictions on a draft pick. It's legal, if unlikely, for every species
// an area has to be filtered out; see Draft_CheckPoolEligible.
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

// `fromDraft` is TRUE for a route's own draft pick (src/ui_birch_case.c) and
// FALSE for a gift/egg funnelled in instead of going to the PC
// (src/pokemon.c). It's remembered until Draft_MarkAreaSpent runs, which
// must only ever spend the area for the former - accepting Steven's Beldum
// in Granite Cave should never burn that area's own draft.
void Draft_QueuePendingMon(struct Pokemon *mon, bool32 fromDraft);

// Script natives for the offer flow (data/scripts/draft.inc). All of them
// are callnative entry points - see the header comment above each
// definition in draft_mode.c for what they actually do.

// TRUE if this area's Mono-filtered draft pool (Draft_BuildPool) has at
// least one legal pick; gSpecialVar_Result carries the result. Lets
// Draft_EventScript_RouteDraft pick between the normal offer and
// Draft_EventScript_NoEligiblePool once Draft_IsAreaDraftable() has already
// confirmed the area has *some* (unfiltered) wild encounters.
void Draft_CheckPoolEligible(void);

// Places the pending mon in the first empty party slot below
// LimitedParty_GetMaxPartySize(). gSpecialVar_Result: 0 = joined, 1 = no
// room (the caller should fall back to the replace screen).
void Draft_TryGiveToEmptySlot(void);

// gStringVar1 = the pending mon's display name (nickname-safe - see the
// GetMonNickname comment on this function in draft_mode.c). Also sets
// gSpecialVar_0x8004 to the party slot it just joined (last party index),
// which Draft_EventScript_OfferNickname passes straight to
// ChangePokemonNickname.
void Draft_BufferPendingNickname(void);

// gStringVar1 = the outgoing party mon at gSpecialVar_0x8004 (as chosen by
// `special ChoosePartyMon`), gStringVar2 = the pending mon's display name.
void Draft_BufferReplacementNames(void);

// Replaces the party mon at gSpecialVar_0x8004 with the pending mon.
void Draft_DoReplacement(void);

// Discards the pending mon without placing it anywhere.
void Draft_DiscardPending(void);

// Marks the current area's draft as spent. The single terminal node of the
// offer flow (Draft_EventScript_Finish) is the only caller, so this must
// never run anywhere else in the flow. No-ops when the pending mon that just
// resolved was a gift/egg rather than a draft pick - see the fromDraft note
// on Draft_QueuePendingMon.
void Draft_MarkAreaSpent(void);

// Marks the current area's draft as spent with no offer ever having been
// made - Draft_EventScript_NoEligiblePool (data/scripts/draft.inc) is the
// only caller, when Mono Type/Mono Gen filter every species this area has
// out of Draft_BuildPool. Unlike Draft_MarkAreaSpent, unconditional: there's
// no pending mon to key off of here, since the case UI never opened.
void Draft_MarkAreaSpentNoOffer(void);

// Field hook for src/field_control_avatar.c's ProcessPlayerFieldInput, called
// every frame the player has field control. Returns TRUE (having started the
// relevant script with ScriptContext_SetupScript) when either a gift/egg is
// waiting to be resolved or the current area has an unspent draft available;
// the former is checked first so a gift handed over on a fresh route
// resolves before that route's own draft does. Returns FALSE, touching
// nothing, when neither applies (including whenever Draft_IsEnabled() is
// false).
bool32 Draft_TryStartFieldScript(void);

#endif // GUARD_DRAFT_MODE_H
