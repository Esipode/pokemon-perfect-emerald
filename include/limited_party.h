#ifndef GUARD_LIMITED_PARTY_H
#define GUARD_LIMITED_PARTY_H

#include "global.h"

// Limited Party challenge mode. The party is capped below PARTY_SIZE and the
// missing slots are earned back from Gym Badges. Sibling of Mono Type / Mono Gen
// (mono_type.h / mono_gen.h) but restricts only how many Pokémon are carried;
// over-cap Pokémon go to the PC, as they already do at 6.
//
// Enabled state lives in gSaveBlock2Ptr->limitedPartySetting; 0 (also what old
// saves read back) means off.
//
// The cap is derived, never stored: CountPlayerBadges() is read live, so there
// is no counter to sync and New Game+ (which clears badges) resets it.

// Party size with 0-1 badges, before any slots are earned back.
#define LIMITED_PARTY_BASE_SIZE 3

bool32 LimitedParty_IsEnabled(void);

// PARTY_SIZE when the mode is off, otherwise LIMITED_PARTY_BASE_SIZE (3)
// plus one slot per badge-count threshold in sLimitedPartyBadgeUnlocks that
// the player has met, clamped to PARTY_SIZE.
u8 LimitedParty_GetMaxPartySize(void);

// TRUE when the party count has reached the current cap (PARTY_SIZE when off).
bool32 LimitedParty_IsPartyFull(void);

// Script special wrapper around LimitedParty_IsPartyFull (data/specials.inc).
u16 IsPlayerPartyFull(void);

// Script special wrapper around LimitedParty_IsEnabled, for Gym Leader defeat
// scripts guarding the "slot unlocked" message (data/specials.inc).
u16 IsLimitedPartyEnabled(void);

#endif // GUARD_LIMITED_PARTY_H
