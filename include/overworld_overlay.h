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
    u32 exemptPalettes;     // one bit per palette slot, set = not tinted
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
void Overlay_SetRenderLayer(OverlayId id, u8 layer);
// New overlays start enabled. Returns OVERLAY_ID_INVALID when the pool is full.
OverlayId Overlay_Create(const struct OverlayConfig *config);
void Overlay_Destroy(OverlayId id);
bool32 Overlay_IsValid(OverlayId id);

#endif // GUARD_OVERWORLD_OVERLAY_H
