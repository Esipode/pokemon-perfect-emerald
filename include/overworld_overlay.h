#ifndef GUARD_OVERWORLD_OVERLAY_H
#define GUARD_OVERWORLD_OVERLAY_H

#include "constants/overworld_overlay.h"

#define MAX_OVERLAYS            4
#define OVERLAY_ID_INVALID      0
#define MAX_SPRITE_OVERLAYS     2

// id = (generation << 3) | index. Generation starts at 1, so id 0 is never issued.
#define OVERLAY_INDEX(id)       ((id) & 7)
#define OVERLAY_GENERATION(id)  ((id) >> 3)

typedef u16 OverlayId;

// Draw order of an OVERLAY_LAYER_SPRITE overlay. Only OBJ priority and subpriority are available,
// so a sprite overlay is never behind the terrain: BG1-3 are the world.
enum OverlaySpritePosition
{
    OVERLAY_SPRITE_BEHIND_OBJECTS,  // over the ground, behind NPCs and the player; under top-layer tiles
    OVERLAY_SPRITE_ABOVE_OBJECTS,   // in front of NPCs and the player, and of top-layer tiles
    OVERLAY_SPRITE_ABOVE_ALL,       // in front of every overworld sprite, including elevated ones
};

struct OverlayConfig
{
    u16 color;      // RGB15 target colour
    u8 opacity;     // 0-OVERLAY_OPACITY_MAX
    u8 layer;       // enum OverlayLayer
    u8 scope;       // enum OverlayScope
    u8 priority;    // composition order, lower applies first
    u8 spritePosition; // enum OverlaySpritePosition, sprite layer only
};

struct Overlay
{
    u32 exemptPalettes;     // one bit per palette slot, set = not tinted; bits 0-15 BG, 16-31 OBJ
    u16 color;
    u16 fadeDuration;       // frames
    u16 fadeElapsed;
    u16 pulsePeriod;        // frames
    u16 pulsePhase;
    s16 anchorX;            // map coordinates
    s16 anchorY;
    u8 generation;          // bumped on release, invalidates stale handles
    u8 active:1;
    u8 enabled:1;
    u8 destroyOnFadeOut:1;
    u8 scope:1;             // enum OverlayScope
    u8 layer:2;             // enum OverlayLayer
    u8 anchorKind:2;        // enum OverlayAnchor
    u8 baseOpacity;         // requested level
    u8 currentOpacity;      // fade output
    u8 resolvedOpacity;     // final value the renderer consumes
    u8 fadeStart;
    u8 fadeTarget;
    u8 pulseMin;
    u8 pulseMax;
    u8 anchorLocalId;
    u8 anchorMapNum;
    u8 anchorMapGroup;
    u8 innerRadius;         // tiles
    u8 outerRadius;
    u8 minIntensity;
    u8 maxIntensity;
    u8 priority;
    u8 exemptPlayer:1;
    u8 falloffEnabled:1;
    u8 spritePosition:2;    // enum OverlaySpritePosition
    u8 transient:1;         // not written to the save
    u8 exemptLocalId;      // 0 = none; resolved against exemptMapNum/exemptMapGroup
    u8 exemptMapNum;
    u8 exemptMapGroup;
};

// Saved in SaveBlock3. The magic byte rejects saves written before this struct existed.
#define OVERLAY_SAVE_MAGIC      0xA5

struct OverlaySave
{
    u8 magic;
    struct Overlay overlays[MAX_OVERLAYS];
};

// Lifecycle contract. Overlays are saved with the game and restored only when the game resumes
// on the saved map; a continue warp (Overlay_ResetAll) discards them.
//
// Event                            Map-local       Global
// Step between connected maps      destroyed       survives
// Warp / hard map load / whiteout  destroyed       destroyed (Overlay_ResetAll)
// Battle enter and return          survives        survives
// Menu open/close                  survives        survives
// Script ends                      survives        survives
//
// Ownership stays with the caller: scripts must destroy their overlays explicitly.
//
// Opacity composition. Fade owns currentOpacity; pulse and falloff scale it:
//   resolvedOpacity = (currentOpacity * pulseFactor * distanceFactor + 128) / 256
// pulseFactor and distanceFactor are OVERLAY_OPACITY_MAX when their feature is inactive.
// The result is clamped to 0-OVERLAY_OPACITY_MAX.

// Anchor and falloff helpers shared with the screen effects module. Positions are in object-event
// coordinate space.
// Looks up the object by local id and map. On success stores its position in *x, *y and returns TRUE
// if that differs from the previous value. An unresolved object leaves *x, *y untouched and returns FALSE.
bool32 OverworldAnchor_Resolve(u8 localId, u8 mapNum, u8 mapGroup, s16 *x, s16 *y);
// Octagonal distance in tiles: max(dx, dy) + min(dx, dy) / 2.
u32 OverworldAnchor_Distance(s16 anchorX, s16 anchorY, const struct Coords16 *playerCoords);
// Linear between maxIntensity at or inside innerRadius and minIntensity at or beyond outerRadius.
u32 OverworldAnchor_DistanceFactor(u32 distance, u8 innerRadius, u8 outerRadius, u8 minIntensity, u8 maxIntensity);

// Invalidates every outstanding handle and clears the pool.
void Overlay_ResetAll(void);
// Copies the pool into SaveBlock3. Called on every save.
void Overlay_SaveToBlock(void);
// Replaces the pool with the one in SaveBlock3. Called when the game resumes on the saved map.
void Overlay_LoadFromBlock(void);
// Destroys every OVERLAY_SCOPE_MAP_LOCAL overlay.
void Overlay_DestroyMapLocal(void);
// Per-frame update. Order: fade, pulse, anchor, falloff, final opacity, render.
void Overlay_Update(void);
// Marks the palette buffer as rewritten behind the overlays' back; they are re-applied
// on the next update.
void Overlay_Invalidate(void);
// Tints palettes that were just rebuilt in gPlttBufferFaded without overlays (weather colour
// map, time of day). PALETTES_ALL also clears the pending recomposition. While a screen fade
// owns the buffer this only marks it for recomposition.
void Overlay_OnPalettesRebuilt(u32 palettes);
// Tints palettes that a screen fade-in just wrote. y is the fade coefficient (16 = fully faded);
// each overlay applies at resolvedOpacity * (16 - y) / 16, so the tint fades in with the screen.
// Ignored outside the field's fade-in.
void Overlay_ApplyFadeInStep(u32 palettes, u32 y);

void Overlay_Enable(OverlayId id);
void Overlay_Disable(OverlayId id);
void Overlay_SetColor(OverlayId id, u16 color);
// Direct request, 0-OVERLAY_OPACITY_MAX.
void Overlay_SetOpacity(OverlayId id, u8 opacity);
u8 Overlay_GetOpacity(OverlayId id);
// Linear fade over durationFrames; 0 snaps. Replaces any running fade.
void Overlay_FadeTo(OverlayId id, u8 targetOpacity, u16 durationFrames);
// Fades to 0, then destroys the overlay. Overlay_SetOpacity or Overlay_FadeTo cancels the destroy.
void Overlay_FadeOutAndDisable(OverlayId id, u16 durationFrames);
// Triangle-wave pulse between minOpacity and maxOpacity, starting at the minimum.
// Multiplies the fade output; it does not replace it. Periods under 2 frames clear the pulse.
void Overlay_Pulse(OverlayId id, u8 minOpacity, u8 maxOpacity, u16 periodFrames);
// Clears the pulse only; any running fade continues.
void Overlay_StopAnimation(OverlayId id);

// Anchors store gameplay identity, never a sprite pointer. Positions are held in object-event
// coordinate space (map coordinates + MAP_OFFSET), the same space as the player's currentCoords.
// x, y are map coordinates as used in scripts and Porymap; they are relative to the map loaded
// when set, so a global coordinate anchor is stale after a connected-map step.
// Object anchors are looked up by local id and map every frame:
// - hidden, invisible and frozen objects resolve normally;
// - an object that is not spawned, was removed, or is on another map keeps the last known
//   position, and the overlay stays alive;
// - a recreated object re-attaches by local id.
// If the object cannot be resolved when the anchor is set, the previous position is kept.
void Overlay_SetAnchorToPosition(OverlayId id, s16 x, s16 y);
void Overlay_SetAnchorToObject(OverlayId id, u8 localId, u8 mapNum, u8 mapGroup);
void Overlay_ClearAnchor(OverlayId id);

// Distance falloff scales the opacity by the player's distance to the anchor. Radii are in tiles;
// distance is max(dx, dy) + min(dx, dy) / 2. Intensities are 0-OVERLAY_OPACITY_MAX: maxIntensity
// applies at or inside innerRadius, minIntensity at or beyond outerRadius, linear in between.
// It has no effect while the overlay has no anchor. The palette backend scales its global
// intensity; it does not draw a spatial gradient.
void Overlay_SetFalloff(OverlayId id, u8 innerRadius, u8 outerRadius, u8 minIntensity, u8 maxIntensity);
void Overlay_ClearFalloff(OverlayId id);

// Palette filtering. Constraints of the palette backend:
// - Filtering granularity is one palette slot, not one entity. Two NPCs that share an
//   object-event palette cannot be filtered independently.
// - Overlay_ExemptPlayer exempts the player's overworld palette, which in this fork is shared
//   with the surf Wailmer, the bike, and the pail. Those vehicles are exempted with the player.
// - "Entity rendered above the overlay" and "entity exempt from the overlay" are the same thing
//   here. True per-pixel ordering against a global tint needs the sprite backend.
// - The number of independently filterable entities is bounded by the distinct palette slots in
//   use: 16 OBJ slots at the hardware ceiling, fewer in practice.
// - A palette preserved from weather (PreservePaletteInWeather) is still tinted unless the
//   overlay also exempts it.
//
// paletteIndex is a palette slot 0-31: 0-15 BG, 16-31 OBJ.
void Overlay_ExemptPalette(OverlayId id, u8 paletteIndex);
void Overlay_UnexemptPalette(OverlayId id, u8 paletteIndex);
// The player and object exemptions follow the sprite's palette slot as it is reallocated.
void Overlay_ExemptPlayer(OverlayId id, bool32 exempt);
// Tracks one object per overlay, looked up by local id on the current map when this is called;
// a later call replaces it. Passing exempt = FALSE clears it only if localId matches.
void Overlay_ExemptObject(OverlayId id, u8 localId, bool32 exempt);
// Not valid for OVERLAY_LAYER_SPRITE: the layer is fixed at creation, and a call that would
// switch to or from it is ignored.
void Overlay_SetRenderLayer(OverlayId id, u8 layer);

// Sprite backend (OVERLAY_LAYER_SPRITE). A radial glow sprite, alpha-blended over the world.
// - At most MAX_SPRITE_OVERLAYS exist at once; Overlay_Create fails when the limit, a free OBJ
//   palette slot, or a sprite is unavailable.
// - Each glow owns one OBJ palette slot. Opacity is emulated by fading that palette toward black;
//   BLDALPHA is shared with shadows and light sprites and is never written.
// - The glow is centred on the anchor tile, or on the player when there is no anchor. An object
//   anchor follows the object's sprite, so it moves smoothly while the object walks.
// - The blend coefficients are global, so terrain under the glow is darkened by the shadow
//   intensity even at low opacity. Opacity 0 hides the sprite.
// - The sprite and palette are re-created after a battle or map load that resets them.
// - Palette-backend filtering (exemptions, layer masks) does not apply, and palette overlays
//   never tint the glow's palette.
void Overlay_SetSpritePosition(OverlayId id, u8 position);
// A transient overlay is left out of the save: its owner is not saved either, so a restored copy
// would have nothing to stop it. The slot's generation is still saved.
void Overlay_SetTransient(OverlayId id);
// New overlays start enabled. Returns OVERLAY_ID_INVALID when the pool is full.
OverlayId Overlay_Create(const struct OverlayConfig *config);
void Overlay_Destroy(OverlayId id);
bool32 Overlay_IsValid(OverlayId id);

#endif // GUARD_OVERWORLD_OVERLAY_H
