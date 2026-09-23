#ifndef GUARD_PLAYER_CUSTOMIZATION_H
#define GUARD_PLAYER_CUSTOMIZATION_H

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

// Stage P2: per-(style, gender) slot table, one entry per storage slot in
// gSaveBlock2Ptr->playerColorSlots[]. A slot is one logical colour -- it may
// cover several palette indices that share a paint (e.g. a shading ramp),
// per Appendix A of the plan doc. name == NULL means the slot is unused for
// this (style, gender) and is hidden from the menu. Still unread until
// Stage P3 wires up the new render path; struct PlayerColorRegionInfo above
// and sPlayerColorRegions stay in place until then.
struct PlayerColorSlotInfo
{
    const u8 *name;
    const u8 *owIndices;
    const u8 *trainerIndices;
    u8 numOwIndices;
    u8 numTrainerIndices;
};

// A group bundles slot ids under one of the PLAYER_COLOR_REGION_* names, so
// the menu can still offer a "hue-rotate this whole group" action alongside
// per-slot editing. Slot ids index into sPlayerColorSlots[style][gender][].
struct PlayerColorGroupInfo
{
    const u8 *name;
    const u8 *slots;
    u8 numSlots;
};

// Accessors over gSaveBlock2Ptr->playerSpriteStyle. Set clamps out-of-range
// values to PLAYER_SPRITE_STYLE_EMERALD.
u8 Player_GetSpriteStyle(void);
void Player_SetSpriteStyle(u8 style);

// Accessors over gSaveBlock2Ptr->playerColors[region]. Each byte packs a
// hue step (low nibble, 0-15) and a signed shade offset (high nibble,
// PLAYER_COLOR_SHADE_MIN..PLAYER_COLOR_SHADE_MAX); 0x00 decodes to "no
// change" so old saves render byte-identical to vanilla.
// Legacy per-region storage; still used by the render path until Stage P3
// and by Stage P9's migration afterward.
u8 Player_GetColorHue(enum PlayerColorRegion region);
s8 Player_GetColorShade(enum PlayerColorRegion region);
void Player_SetColorHue(enum PlayerColorRegion region, u8 hue);
void Player_SetColorShade(enum PlayerColorRegion region, s8 shade);

// Accessors over gSaveBlock2Ptr->playerColorSlots[slot]. Raw stored value
// includes PLAYER_COLOR_SET; 0 means "use the ROM colour".
u16 Player_GetColorSlot(u32 slot);
void Player_SetColorSlot(u32 slot, u16 rgb);
void Player_ClearColorSlot(u32 slot);

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

// First owIndices entry for `region`; lets the menu paint a swatch out of the
// buffer PlayerCustomization_BuildPreviewPalette() filled.
u8 PlayerCustomization_GetRegionSwatchIndex(u8 gender, enum PlayerColorRegion region);

// The battle-transition mugshot background is a plain 6-colour gradient with no
// per-region art, so the whole gradient uses the player's OUTFIT hue/shade as a
// "theme colour". `dest` must hold at least 6 u16s.
void PlayerCustomization_GetBattleTransitionMugshotBgPalette(const u16 *basePal, u16 *dest);

#endif // GUARD_PLAYER_CUSTOMIZATION_H
