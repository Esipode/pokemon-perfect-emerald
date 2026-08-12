#ifndef GUARD_LIMITED_PARTY_H
#define GUARD_LIMITED_PARTY_H

#include "global.h"

// Limited Party challenge mode. The player's party is capped below
// PARTY_SIZE, and the missing slots are earned back from Gym Badges. It is
// the sibling of Mono Type / Mono Gen (see mono_type.h / mono_gen.h) and
// reuses their plumbing wherever possible, but it restricts nothing about
// which Pokémon may be obtained - only how many may be carried. Every
// over-cap Pokémon simply goes to the PC, which is exactly what already
// happens at 6 today.
//
// Whether the mode is on lives in gSaveBlock2Ptr->limitedPartySetting. 0
// means the mode is off, which is also what old saves read back.
//
// The cap is derived, never stored: CountPlayerBadges() is read live every
// time LimitedParty_GetMaxPartySize() is called. There is no "slots
// unlocked" counter to keep in sync, and New Game+ (which clears badges)
// resets the cap for free.

// Party size with 0-1 badges, before any slots are earned back.
#define LIMITED_PARTY_BASE_SIZE 3

bool32 LimitedParty_IsEnabled(void);

// PARTY_SIZE when the mode is off, otherwise LIMITED_PARTY_BASE_SIZE (3)
// plus one slot per badge-count threshold in sLimitedPartyBadgeUnlocks that
// the player has met, clamped to PARTY_SIZE.
u8 LimitedParty_GetMaxPartySize(void);

// TRUE when the live party count has reached the current cap (PARTY_SIZE
// when the mode is off). Convenience wrapper around
// CalculatePlayerPartyCount() vs LimitedParty_GetMaxPartySize() for C
// callers.
bool32 LimitedParty_IsPartyFull(void);

// Script special wrapper around LimitedParty_IsPartyFull - scripts can't
// call C functions and compare against a derived cap, so this gives them a
// plain TRUE/FALSE special instead. See data/specials.inc.
u16 IsPlayerPartyFull(void);

// Script special wrapper around LimitedParty_IsEnabled - lets Gym Leader
// defeat scripts guard the "a party slot just unlocked" message. See
// data/specials.inc.
u16 IsLimitedPartyEnabled(void);

#endif // GUARD_LIMITED_PARTY_H
