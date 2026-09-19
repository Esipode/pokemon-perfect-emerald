#ifndef GUARD_OVERWORLD_OVERLAY_H
#define GUARD_OVERWORLD_OVERLAY_H

#define MAX_OVERLAYS            4
#define OVERLAY_OPACITY_MAX     16
#define OVERLAY_ID_INVALID      0

// id = (generation << 3) | index. Generation starts at 1, so id 0 is never issued.
#define OVERLAY_INDEX(id)       ((id) & 7)
#define OVERLAY_GENERATION(id)  ((id) >> 3)

typedef u16 OverlayId;

enum OverlayLayer
{
    OVERLAY_LAYER_WORLD,    // BG palettes only
    OVERLAY_LAYER_OBJECTS,  // OBJ palettes only
    OVERLAY_LAYER_ALL,      // both
    OVERLAY_LAYER_SPRITE,   // spatial sprite backend
};

enum OverlayScope
{
    OVERLAY_SCOPE_MAP_LOCAL,
    OVERLAY_SCOPE_GLOBAL,
};

enum OverlayAnchor
{
    OVERLAY_ANCHOR_NONE,
    OVERLAY_ANCHOR_COORDS,
    OVERLAY_ANCHOR_OBJECT,
};

struct OverlayConfig
{
    u16 color;      // RGB15 target colour
    u8 opacity;     // 0-OVERLAY_OPACITY_MAX
    u8 layer;       // enum OverlayLayer
    u8 scope;       // enum OverlayScope
    u8 priority;    // composition order, lower applies first
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
    u8 exemptLocalId;       // 0 = none; resolved against exemptMapNum/exemptMapGroup
    u8 exemptMapNum;
    u8 exemptMapGroup;
};

// Lifecycle contract. Overlays are never saved.
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

// Invalidates every outstanding handle and clears the pool.
void Overlay_ResetAll(void);
// Destroys every OVERLAY_SCOPE_MAP_LOCAL overlay.
void Overlay_DestroyMapLocal(void);
// Per-frame update. Order: fade, pulse, anchor, falloff, final opacity, render.
void Overlay_Update(void);
// Marks the palette buffer as rewritten behind the overlays' back; they are re-applied
// on the next update.
void Overlay_Invalidate(void);

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
void Overlay_SetRenderLayer(OverlayId id, u8 layer);
// New overlays start enabled. Returns OVERLAY_ID_INVALID when the pool is full.
OverlayId Overlay_Create(const struct OverlayConfig *config);
void Overlay_Destroy(OverlayId id);
bool32 Overlay_IsValid(OverlayId id);

#endif // GUARD_OVERWORLD_OVERLAY_H
