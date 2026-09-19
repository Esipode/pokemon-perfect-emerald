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

// Invalidates every outstanding handle and clears the pool.
void Overlay_ResetAll(void);
// New overlays start enabled. Returns OVERLAY_ID_INVALID when the pool is full.
OverlayId Overlay_Create(const struct OverlayConfig *config);
void Overlay_Destroy(OverlayId id);
bool32 Overlay_IsValid(OverlayId id);

#endif // GUARD_OVERWORLD_OVERLAY_H
