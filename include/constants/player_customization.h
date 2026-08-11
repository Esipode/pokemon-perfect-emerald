#ifndef GUARD_CONSTANTS_PLAYER_CUSTOMIZATION_H
#define GUARD_CONSTANTS_PLAYER_CUSTOMIZATION_H

// Split out from include/player_customization.h so PLAYER_COLOR_REGION_COUNT
// can size gSaveBlock2Ptr->playerColors[] in include/global.h (which is
// included well before player_customization.h could be) -- same split as
// constants/achievements.h vs include/achievements.h.
enum PlayerColorRegion
{
    PLAYER_COLOR_REGION_HAIR,
    PLAYER_COLOR_REGION_HAT,
    PLAYER_COLOR_REGION_OUTFIT,
    PLAYER_COLOR_REGION_ACCENT,
    PLAYER_COLOR_REGION_COUNT,
};

#endif // GUARD_CONSTANTS_PLAYER_CUSTOMIZATION_H
