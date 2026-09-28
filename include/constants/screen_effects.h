#ifndef GUARD_CONSTANTS_SCREEN_EFFECTS_H
#define GUARD_CONSTANTS_SCREEN_EFFECTS_H

#define SCREENFX_INTENSITY_MAX  16

// Shake axis flags (param2).
#define SCREENFX_AXIS_X         (1 << 0)
#define SCREENFX_AXIS_Y         (1 << 1)
#define SCREENFX_SHAKE_MAX_AMPLITUDE    4   // pixels at SCREENFX_INTENSITY_MAX

// Wave param2: wavelength in scanlines, OR'd with flags.
#define SCREENFX_WAVE_VERTICAL          (1 << 15)
#define SCREENFX_WAVE_MAX_AMPLITUDE     4   // pixels at SCREENFX_INTENSITY_MAX

#define SCREENFX_RIPPLE_MAX_AMPLITUDE   4   // pixels at amplitude and intensity SCREENFX_INTENSITY_MAX
#define SCREENFX_RIPPLE_CENTER_AUTO     0x7FFF  // ScreenFx_TriggerRipple: centre on the anchor

#define SCREENFX_TEAR_CENTER_AUTO       0xFFFF  // tear param2: centre on the anchor

// Vignette focus presets (param1). Wider focus leaves more of the screen clear.
#define SCREENFX_VIGNETTE_WIDE          0
#define SCREENFX_VIGNETTE_MEDIUM        1
#define SCREENFX_VIGNETTE_TIGHT         2

// Particle style (param2 low byte); the high byte is the spawn radius in tiles.
enum ScreenFxParticleStyle
{
    SCREENFX_PARTICLE_SPARKLE,
    SCREENFX_PARTICLE_EMBER,
    SCREENFX_PARTICLE_BUBBLE,
    SCREENFX_PARTICLE_LEAF,
    SCREENFX_PARTICLE_MOTE,
    SCREENFX_PARTICLE_STYLE_COUNT,
};

enum ScreenFxKind
{
    SCREENFX_WAVE,
    SCREENFX_RIPPLE,
    SCREENFX_TEAR,
    SCREENFX_SHAKE,
    SCREENFX_VIGNETTE,
    SCREENFX_PARTICLES,
    SCREENFX_PILLAR,
    SCREENFX_KIND_COUNT,
};

// ScreenFx_StartPreset presets.
enum ScreenFxPreset
{
    SCREENFX_PRESET_LEGENDARY_PRESENCE, // wave + slow shake + anchored tint, proximity driven
    SCREENFX_PRESET_DIMENSIONAL,        // tear + wave + dark tint
    SCREENFX_PRESET_DIVINE_FOCUS,       // vignette + pulsing tint
    SCREENFX_PRESET_LEGENDARY_BURST,    // white flash, ripple, shake, then a settling wave
    SCREENFX_PRESET_COUNT,
};

// ScreenFx_SetProgression stages.
enum ScreenFxStage
{
    SCREENFX_STAGE_ORDINARY,
    SCREENFX_STAGE_PERCEPTIBLE,
    SCREENFX_STAGE_DRAMATIC,
    SCREENFX_STAGE_SETTLE,
    SCREENFX_STAGE_COUNT,
};

enum ScreenFxAnchor
{
    SCREENFX_ANCHOR_NONE,
    SCREENFX_ANCHOR_COORDS,
    SCREENFX_ANCHOR_OBJECT,
};

// Particle and pillar colour operand meaning "the style's own colour". Bit 15 is outside RGB15.
#define SCREENFX_COLOR_DEFAULT 0x8000

#endif // GUARD_CONSTANTS_SCREEN_EFFECTS_H
