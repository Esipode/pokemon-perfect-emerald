#ifndef GUARD_CONSTANTS_PLAYER_CUSTOMIZATION_H
#define GUARD_CONSTANTS_PLAYER_CUSTOMIZATION_H

// Split out from include/player_customization.h so PLAYER_COLOR_REGION_COUNT
// can size gSaveBlock2Ptr->playerColorSlots[] in include/global.h (which is
// included well before player_customization.h could be) -- same split as
// constants/achievements.h vs include/achievements.h.
// No longer a storage index (see PLAYER_COLOR_SLOT_COUNT / playerColorSlots
// below) -- now a *group* id, naming a bundle of slots the menu can
// hue-rotate in bulk.
enum PlayerColorRegion
{
    PLAYER_COLOR_REGION_HAIR,
    PLAYER_COLOR_REGION_HAT,
    PLAYER_COLOR_REGION_OUTFIT,
    PLAYER_COLOR_REGION_ACCENT,
    PLAYER_COLOR_REGION_COUNT,
};

// Palette indices 1..15 are colourable; index 0 is transparency.
#define PLAYER_COLOR_SLOT_COUNT 15
// Bit 15 is unused by GBA BGR555, so it doubles as an is-set flag: 0 always
// means "use the ROM colour", with no separate bitmask needed.
#define PLAYER_COLOR_SET (1 << 15)

// Which protagonist sprite set the save renders the player with. Stored in
// gSaveBlock2Ptr->playerSpriteStyle, so EMERALD must stay 0.
enum PlayerSpriteStyle
{
    PLAYER_SPRITE_STYLE_EMERALD,
    PLAYER_SPRITE_STYLE_FRLG,
    PLAYER_SPRITE_STYLE_COUNT,
};

#endif // GUARD_CONSTANTS_PLAYER_CUSTOMIZATION_H
