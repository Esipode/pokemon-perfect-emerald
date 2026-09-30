#ifndef GUARD_CONSTANTS_OVERWORLD_OVERLAY_H
#define GUARD_CONSTANTS_OVERWORLD_OVERLAY_H

#define OVERLAY_OPACITY_MAX     16

// Hue angles are a full u8 turn: 128 is the opposite hue, 256 wraps to no shift.
#define OVERLAY_HUE_FULL_TURN   256

enum OverlayEffect
{
    OVERLAY_EFFECT_TINT,        // blends the palettes toward a colour
    OVERLAY_EFFECT_HUE_SHIFT,   // rotates the palettes' hue, keeping brightness and saturation
    OVERLAY_EFFECT_SATURATION,  // scales the palettes' distance from grey
    OVERLAY_EFFECT_INVERT,      // photo negative; color is unused
    OVERLAY_EFFECT_COUNT,
};

#define OVERLAY_SATURATION_GREY     0
#define OVERLAY_SATURATION_NEUTRAL  16  // unchanged
#define OVERLAY_SATURATION_MAX      32  // double saturation

enum OverlayPulseWave
{
    OVERLAY_PULSE_TRIANGLE,     // linear up and down
    OVERLAY_PULSE_SINE,         // eased, lingers at both ends
    OVERLAY_PULSE_SQUARE,       // min for the first half period, max for the second
    OVERLAY_PULSE_SAWTOOTH,     // linear min to max, then snaps back
    OVERLAY_PULSE_HEARTBEAT,    // beat, smaller beat, long rest
    OVERLAY_PULSE_FLICKER,      // pseudo-random levels held a few frames each
    OVERLAY_PULSE_SINE_QUARTER, // one eased hump in the first quarter period, min for the rest
    OVERLAY_PULSE_WAVE_COUNT,
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
