#ifndef GUARD_PLAYER_CUSTOMIZATION_H
#define GUARD_PLAYER_CUSTOMIZATION_H

#include "constants/player_customization.h" // enum PlayerColorRegion

#define PLAYER_COLOR_HUE_COUNT 16
#define PLAYER_COLOR_SHADE_MIN -3
#define PLAYER_COLOR_SHADE_MAX  3

// Per-(style, gender) slot table, one entry per storage slot in
// gSaveBlock2Ptr->playerColorSlots[]. A slot is one logical colour -- it may
// cover several palette indices that share a paint (e.g. a shading ramp),
// per Appendix A of the plan doc. name == NULL means the slot is unused for
// this (style, gender) and is hidden from the menu.
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

// Accessors over gSaveBlock2Ptr->playerColorSlots[slot]. Raw stored value
// includes PLAYER_COLOR_SET; 0 means "use the ROM colour".
u16 Player_GetColorSlot(u32 slot);
void Player_SetColorSlot(u32 slot, u16 rgb);
void Player_ClearColorSlot(u32 slot);

// Read-only lookup into the Stage P2 sPlayerColorSlots / sPlayerColorGroups
// tables, for the menu to build its row map from. slot->name / group->name
// is NULL when unused for this (style, gender).
const struct PlayerColorSlotInfo *PlayerCustomization_GetSlotInfo(u8 style, u8 gender, u8 slot);
const struct PlayerColorGroupInfo *PlayerCustomization_GetGroupInfo(u8 style, u8 gender, enum PlayerColorRegion group);

// Returns NULL unless paletteTag belongs to the player's own (style, gender)
// and at least one slot is customised; otherwise returns a static EWRAM
// u16[16] buffer holding the recoloured overworld palette. Callers must copy
// the result immediately (e.g. via LoadSpritePalette/LoadPalette).
const u16 *PlayerCustomization_GetOwPaletteOverride(u16 paletteTag);

// Same shape as PlayerCustomization_GetOwPaletteOverride, but for the
// trainer pic (front/back share one palette), gated on trainerPicId matching
// the player's (style, gender) trainer pic.
const u16 *PlayerCustomization_GetTrainerPaletteOverride(u32 trainerPicId);

// TRUE if every slot is still at its vanilla (zeroed) value.
bool32 PlayerCustomization_IsDefault(void);

// Renders a candidate overworld palette for (style, gender) from `choices`
// (a PLAYER_COLOR_SLOT_COUNT array, same raw encoding as
// gSaveBlock2Ptr->playerColorSlots) into `dest` (u16[16]), without touching
// the save block. Used by the customization menu's live preview.
void PlayerCustomization_BuildPreviewPalette(u8 style, u8 gender, const u16 *choices, u16 *dest);

// Same as PlayerCustomization_BuildPreviewPalette, but against the trainer
// front pic's base palette and trainerIndices, for Stage P8's trainer-pic
// preview toggle.
void PlayerCustomization_BuildTrainerPreviewPalette(u8 style, u8 gender, const u16 *choices, u16 *dest);

// Expected TRAINER_PIC_* id for (style, gender). Shared by
// PlayerCustomization_GetTrainerPaletteOverride and the menu's preview toggle.
u32 PlayerCustomization_GetTrainerPicId(u8 style, u8 gender);

// First owIndices entry for `slot`; lets the menu paint a swatch out of the
// buffer PlayerCustomization_BuildPreviewPalette() filled.
u8 PlayerCustomization_GetSlotSwatchIndex(u8 style, u8 gender, u8 slot);

// Same as PlayerCustomization_GetSlotSwatchIndex, but the first
// trainerIndices entry, for use against a PlayerCustomization_Build
// TrainerPreviewPalette() buffer. Falls back to the OW index when the slot
// has no trainer index (e.g. hair hidden under a cap) -- the buffer still
// holds a valid, if not slot-representative, colour there.
u8 PlayerCustomization_GetTrainerSlotSwatchIndex(u8 style, u8 gender, u8 slot);

// The battle-transition mugshot background is a plain 6-colour gradient with
// no per-index art mapping. `dest` must hold at least 6 u16s. Recoloured by
// applying the OUTFIT group's HSV delta (see PlayerCustomization_GetGroupHsvDelta).
void PlayerCustomization_GetBattleTransitionMugshotBgPalette(u8 style, u8 gender, const u16 *basePal, u16 *dest);

// HSV delta between a group's first set slot and that slot's ROM colour.
// FALSE if no slot in the group is set (dh/ds/dv left untouched). dh wraps
// mod 256; ds and dv are signed and clamped to [0, 255] by the caller.
bool32 PlayerCustomization_GetGroupHsvDelta(u8 style, u8 gender, enum PlayerColorRegion group,
                                             s16 *dh, s16 *ds, s16 *dv);

// ROM (vanilla) RGB15 colour for `slot`'s first overworld index. Used by the
// menu to seed a not-yet-set slot before the first edit, and to show its
// swatch while unset.
u16 PlayerCustomization_GetSlotRomColor(u8 style, u8 gender, u8 slot);

// Applies the old region hue/shade maths (hue step 0..PLAYER_COLOR_HUE_COUNT-1,
// shade PLAYER_COLOR_SHADE_MIN..MAX) to `slot`'s ROM colour and returns the
// resulting RGB15. Used by group-row (macro) editing to recompute every slot
// in a group from its ROM colour, not from its current stored value.
u16 PlayerCustomization_ApplyHueShadeToRomColor(u8 style, u8 gender, u8 slot, u8 hue, s8 shade);

// RGB15 <-> HSV(0-255) conversion, exported for the menu's editing model
// (Stage P4/P6) so callers stop repeating the * 255 / 31 scaling.
void PlayerCustomization_RgbToHsv(u16 color, u8 *h, u8 *s, u8 *v);
u16 PlayerCustomization_HsvToRgb(u8 h, u8 s, u8 v);

#endif // GUARD_PLAYER_CUSTOMIZATION_H
