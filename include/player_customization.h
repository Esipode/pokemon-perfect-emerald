#ifndef GUARD_PLAYER_CUSTOMIZATION_H
#define GUARD_PLAYER_CUSTOMIZATION_H

// See Customization.md for the full design. Stage 1: data model, save
// storage and palette generation only -- nothing here is wired into the
// overworld/trainer-pic palette paths yet (that's Stages 2 and 4).

#include "constants/player_customization.h" // enum PlayerColorRegion

#define PLAYER_COLOR_HUE_COUNT 16
#define PLAYER_COLOR_SHADE_MIN -3
#define PLAYER_COLOR_SHADE_MAX  3

// One entry per (gender, region), naming the region and listing which
// palette indices it recolours in the overworld sprite and in the trainer
// pic. Male's hair has no trainer-pic entry (numTrainerIndices == 0) --
// it's hidden under his cap in that pic.
struct PlayerColorRegionInfo
{
    const u8 *name;
    const u8 *owIndices;
    u8 numOwIndices;
    const u8 *trainerIndices;
    u8 numTrainerIndices;
};

// Accessors over gSaveBlock2Ptr->playerColors[region]. Each byte packs a
// hue step (low nibble, 0-15) and a signed shade offset (high nibble,
// PLAYER_COLOR_SHADE_MIN..PLAYER_COLOR_SHADE_MAX); 0x00 decodes to "no
// change" so old saves render byte-identical to vanilla.
u8 Player_GetColorHue(enum PlayerColorRegion region);
s8 Player_GetColorShade(enum PlayerColorRegion region);
void Player_SetColorHue(enum PlayerColorRegion region, u8 hue);
void Player_SetColorShade(enum PlayerColorRegion region, s8 shade);

// Returns NULL unless paletteTag belongs to the player's own gender and at
// least one region is customised; otherwise returns a static EWRAM u16[16]
// buffer holding the recoloured overworld palette. Callers must copy the
// result immediately (e.g. via LoadSpritePalette/LoadPalette).
const u16 *PlayerCustomization_GetOwPaletteOverride(u16 paletteTag);

// Same shape as PlayerCustomization_GetOwPaletteOverride, but for the
// trainer pic (front/back share one palette), gated on trainerPicId
// matching TRAINER_PIC_BRENDAN/TRAINER_PIC_MAY for the player's gender.
const u16 *PlayerCustomization_GetTrainerPaletteOverride(u32 trainerPicId);

// TRUE if every region is still at its vanilla (zeroed) value.
bool32 PlayerCustomization_IsDefault(void);

// Renders a candidate overworld palette for `gender` from `choices` (an
// array of PLAYER_COLOR_REGION_COUNT packed bytes, same encoding as
// gSaveBlock2Ptr->playerColors) into `dest` (u16[16]), without touching the
// save block. Used by the customization menu's live preview.
void PlayerCustomization_BuildPreviewPalette(u8 gender, const u8 *choices, u16 *dest);

// A single representative overworld palette index for `region` (its first
// owIndices entry) -- e.g. so the customization menu can paint a one-colour
// swatch per row out of the same 16-colour buffer PlayerCustomization_
// BuildPreviewPalette() just filled, without the menu needing its own copy
// of sPlayerColorRegions (which is private to src/player_customization.c).
u8 PlayerCustomization_GetRegionSwatchIndex(u8 gender, enum PlayerColorRegion region);

#endif // GUARD_PLAYER_CUSTOMIZATION_H
