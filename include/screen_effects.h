#ifndef GUARD_SCREEN_EFFECTS_H
#define GUARD_SCREEN_EFFECTS_H

#include "constants/screen_effects.h"

#define MAX_SCREEN_EFFECTS      4
#define SCREENFX_ID_INVALID     0

// id = (generation << 3) | index. Generation starts at 1, so id 0 is never issued.
#define SCREENFX_INDEX(id)      ((id) & 7)
#define SCREENFX_GENERATION(id) ((id) >> 3)

typedef u16 ScreenFxId;

struct ScreenFxConfig
{
    u8 kind;                // enum ScreenFxKind
    u8 intensity;           // 0-SCREENFX_INTENSITY_MAX
    u16 param1, param2;     // kind-specific
    u16 durationFrames;     // 0 = until stopped
};

struct ScreenFx
{
    u16 fadeDuration;       // frames
    u16 fadeElapsed;
    s16 anchorX;            // map coordinates
    s16 anchorY;
    u16 lifetime;           // frames, 0 = until stopped
    u16 elapsed;
    u8 kind;                // enum ScreenFxKind
    u8 generation;          // bumped on release, invalidates stale handles
    u8 active:1;
    u8 enabled:1;
    u8 stopOnFadeOut:1;
    u8 anchorKind:2;        // enum ScreenFxAnchor
    u8 falloffEnabled:1;
    u8 baseIntensity;       // requested level
    u8 currentIntensity;    // fade output
    u8 resolvedIntensity;   // final value the renderer consumes
    u8 fadeStart;
    u8 fadeTarget;
    u8 anchorLocalId;
    u8 anchorMapNum;
    u8 anchorMapGroup;
    u8 innerRadius;         // tiles
    u8 outerRadius;
    u8 minIntensity;
    u8 maxIntensity;
    union
    {
        struct
        {
            u16 param1, param2;
        } config;
        struct              // param1 and param2 alias period and axes
        {
            u16 period;     // frames per cycle
            u16 axes;       // SCREENFX_AXIS_*
            u16 phase;      // 0x10000 = one cycle
        } shake;
        struct
        {
            u16 period;     // frames per cycle
            u16 step;       // phase advance per scanline, 0x10000 = one cycle
            u16 phase;      // 0x10000 = one cycle
            u16 vertical;   // non-zero adds the vertical component
        } wave;
        struct TearParams
        {
            s16 center;     // sixteenths of a screen line, or TEAR_CENTER_AUTO
            s16 drift;      // sixteenths of a line per frame
            u8 height;      // scanlines
            u8 roll;        // amplitude table index
            u8 timer;       // frames until the next re-roll
        } tear;
        u8 raw[8];
    } params;               // per-kind state
};

// Screen effects are transient: never saved, and cleared on every map load (ScreenFx_ResetAll).
// Colour and tint belong to the overworld overlay module; this module owns geometry and light.
// Intensity is 0-SCREENFX_INTENSITY_MAX.
//
// Intensity composition. Fade owns currentIntensity; falloff scales it:
//   resolvedIntensity = clamp((currentIntensity * distanceFactor + 8) / 16, 0, 16)
// distanceFactor is SCREENFX_INTENSITY_MAX when falloff is inactive. Effects render resolvedIntensity.
//
// A non-zero durationFrames is the lifetime: when it runs out the effect fades out over
// min(durationFrames / 4, 30) frames and stops. ScreenFx_SetIntensity or ScreenFx_FadeTo during the
// fade-out cancels the stop.

// Shake (SCREENFX_SHAKE): param1 = period in frames (24-48 reads as a slow tremor; 0 = 32),
// param2 = SCREENFX_AXIS_* flags (0 = both). Drives the camera pan from a sine table; amplitude is
// resolvedIntensity * SCREENFX_SHAKE_MAX_AMPLITUDE / SCREENFX_INTENSITY_MAX pixels. With both axes the
// pan orbits. Only one shake exists at a time: ScreenFx_Start fails while another is active.
// While a shake runs it re-asserts the camera pan every frame, overriding the camera-shake field
// effects (earthquake, Mirage Tower); those resume when the shake stops. Stopping restores the
// default pan-ahead camera.

// Scanline channel (wave, ripple and tear effects). While any of those effects exists, an HBlank DMA0
// stream rewrites BG1-3 scroll registers per scanline from gScanlineEffectRegBuffers, starting from the
// camera offset including pan, so it composes with shake. The summed per-line offset is clamped to +/-8 px.
// The channel is claimed only while gScanlineEffect.state is 0; during the flash and Battle Pyramid
// effects it stays released and is claimed again once they end. It never touches gScanlineEffect.
// With no line offsets the field renders exactly as without the channel.
//
// Wave (SCREENFX_WAVE): param1 = period in frames (0 = 90; ~60-90 reads as a presence shimmer, 180-300
// as a breathing world), param2 = wavelength in scanlines (0 = 64, minimum 8), optionally OR'd with
// SCREENFX_WAVE_VERTICAL. Each scanline is shifted horizontally by a sine of its line number, moving
// with time. Amplitude is resolvedIntensity * SCREENFX_WAVE_MAX_AMPLITUDE / SCREENFX_INTENSITY_MAX
// pixels, rounded per line, so low intensities only flicker the sine peaks. The vertical component
// uses a quarter-cycle phase shift at half the amplitude. Waves sum with each other and with other
// geometry effects; only the +/-8 px clamp limits the total. Effects started while the channel is
// unavailable (flash, Battle Pyramid) have no visible result until it is free.

// Ripple (SCREENFX_RIPPLE): an effect that holds up to 3 concurrent ripples fired with
// ScreenFx_TriggerRipple; it does nothing until one is triggered and stays in the pool until stopped.
// Only one ripple effect exists at a time: ScreenFx_Start fails while another is active. Its intensity
// is a master gain applied on top of each ripple's amplitude. The hardware displaces whole scanlines,
// so a ripple is a horizontally displaced band travelling vertically away from the centre line, not a
// circular wave; it reads as radial when paired with a vignette or glow overlay at the same centre.
// A ripple ends when its duration runs out or its front has left the screen.

// Tear (SCREENFX_TEAR): a band of scanlines pushed sideways by a hard alternating +/- displacement,
// odd lines one way and even lines the other, so it reads as torn space. param1 = band height in
// scanlines (0 = 24), param2 = band centre as a screen line (0-159; there is no default),
// SCREENFX_TEAR_CENTER_AUTO follows the anchor's screen position each frame (the screen centre without
// an anchor). The displacement is re-rolled from a small fixed table every 4 + (16 - resolvedIntensity)
// frames, up to 8 px at SCREENFX_INTENSITY_MAX. Only the band is affected. ScreenFx_SetTearDrift moves a
// fixed-centre band vertically, wrapping at the screen edges; it has no effect on an anchored band.

// Invalidates every outstanding handle and clears the pool. Stops the scanline DMA at once.
void ScreenFx_ResetAll(void);
// Per-frame update. Runs before UpdateCameraPanning so camera-pan effects apply the same frame.
void ScreenFx_Update(void);
// Builds the scanline buffer. Runs after UpdateCameraPanning.
void ScreenFx_Render(void);
// Installs the scanline DMA. Runs in VBlankCB_Field after FieldUpdateBgTilemapScroll.
void ScreenFx_VBlank(void);

// New effects start enabled. Returns SCREENFX_ID_INVALID when the pool is full or the kind is unknown.
ScreenFxId ScreenFx_Start(const struct ScreenFxConfig *config);
void ScreenFx_Stop(ScreenFxId id);
void ScreenFx_StopAll(void);
bool32 ScreenFx_IsValid(ScreenFxId id);
// Direct request, 0-SCREENFX_INTENSITY_MAX. Cancels any running fade.
void ScreenFx_SetIntensity(ScreenFxId id, u8 intensity);
u8 ScreenFx_GetIntensity(ScreenFxId id);
// Linear fade over durationFrames; 0 snaps. Replaces any running fade.
void ScreenFx_FadeTo(ScreenFxId id, u8 targetIntensity, u16 durationFrames);
// Fades to 0, then stops the effect. ScreenFx_SetIntensity or ScreenFx_FadeTo cancels the stop.
void ScreenFx_FadeOutAndStop(ScreenFxId id, u16 durationFrames);

// Anchors and falloff follow the overlay module: anchors store gameplay identity, never a sprite
// pointer; an object that cannot be resolved keeps its last known position and the effect stays alive;
// distance is max(dx, dy) + min(dx, dy) / 2 in tiles. x, y are map coordinates as used in scripts.
// Falloff has no effect while the effect has no anchor. maxIntensity applies at or inside
// innerRadius, minIntensity at or beyond outerRadius, linear in between.
void ScreenFx_SetAnchorToPosition(ScreenFxId id, s16 x, s16 y);
void ScreenFx_SetAnchorToObject(ScreenFxId id, u8 localId, u8 mapNum, u8 mapGroup);
void ScreenFx_ClearAnchor(ScreenFxId id);
void ScreenFx_SetFalloff(ScreenFxId id, u8 innerRadius, u8 outerRadius, u8 minIntensity, u8 maxIntensity);
void ScreenFx_ClearFalloff(ScreenFxId id);

// Vertical drift of a SCREENFX_TEAR band in sixteenths of a scanline per frame (negative = up).
void ScreenFx_SetTearDrift(ScreenFxId id, s16 drift);

// Fires a ripple on a SCREENFX_RIPPLE effect. screenCenterY is a screen line, or
// SCREENFX_RIPPLE_CENTER_AUTO for the anchor's screen position (the screen centre without an anchor);
// the centre is fixed when triggered. amplitude is 0-SCREENFX_INTENSITY_MAX. speed is the front's
// travel in sixteenths of a scanline per frame (0 = 32). durationFrames is the ripple's lifetime; the
// amplitude decays linearly over it (0 = 60). With all 3 ripples busy the oldest is replaced.
// Returns FALSE when id is not a valid ripple effect.
bool32 ScreenFx_TriggerRipple(ScreenFxId id, s16 screenCenterY, u8 amplitude, u16 speed, u16 durationFrames);

#endif // GUARD_SCREEN_EFFECTS_H
