#include "global.h"
#include "limited_party.h"
#include "badge_mart.h"
#include "pokemon.h"

// Shared rules for the Limited Party challenge. See include/limited_party.h.

bool32 LimitedParty_IsEnabled(void)
{
    return gSaveBlock2Ptr->limitedPartySetting != 0;
}

// Badges required to unlock the 4th, 5th and 6th party slot.
static const u8 sLimitedPartyBadgeUnlocks[] = { 2, 4, 7 };

STATIC_ASSERT(ARRAY_COUNT(sLimitedPartyBadgeUnlocks) == PARTY_SIZE - LIMITED_PARTY_BASE_SIZE, LimitedPartyBadgeUnlocksMustCoverEveryRemainingSlot);

u8 LimitedParty_GetMaxPartySize(void)
{
    u8 badges, size;
    u32 i;

    if (!LimitedParty_IsEnabled())
        return PARTY_SIZE;

    badges = CountPlayerBadges();
    size = LIMITED_PARTY_BASE_SIZE;
    for (i = 0; i < ARRAY_COUNT(sLimitedPartyBadgeUnlocks); i++)
    {
        if (badges >= sLimitedPartyBadgeUnlocks[i])
            size++;
    }

    // Defensive only - matters if PARTY_SIZE is ever lowered or a badge is
    // added without updating the table above.
    return min(size, PARTY_SIZE);
}

bool32 LimitedParty_IsPartyFull(void)
{
    return CalculatePlayerPartyCount() >= LimitedParty_GetMaxPartySize();
}

u16 IsPlayerPartyFull(void)
{
    return LimitedParty_IsPartyFull();
}

u16 IsLimitedPartyEnabled(void)
{
    return LimitedParty_IsEnabled();
}
