#ifndef GUARD_CONSTANTS_OVERWORLD_OVERLAY_H
#define GUARD_CONSTANTS_OVERWORLD_OVERLAY_H

#define OVERLAY_OPACITY_MAX     16

// Hue angles are a full u8 turn: 128 is the opposite hue, 256 wraps to no shift.
#define OVERLAY_HUE_FULL_TURN   256

enum OverlayEffect
{
    OVERLAY_EFFECT_TINT,        // blends the palettes toward a colour
    OVERLAY_EFFECT_HUE_SHIFT,   // rotates the palettes' hue, keeping brightness and saturation
};

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

#endif // GUARD_CONSTANTS_OVERWORLD_OVERLAY_H
