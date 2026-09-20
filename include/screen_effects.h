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
        u8 raw[8];
    } params;               // per-kind state
};

// Screen effects are transient: never saved, and cleared on every map load (ScreenFx_ResetAll).
// Colour and tint belong to the overworld overlay module; this module owns geometry and light.
// Intensity is 0-SCREENFX_INTENSITY_MAX.

// Invalidates every outstanding handle and clears the pool.
void ScreenFx_ResetAll(void);
// Per-frame update. Runs before UpdateCameraPanning so camera-pan effects apply the same frame.
void ScreenFx_Update(void);

// New effects start enabled. Returns SCREENFX_ID_INVALID when the pool is full or the kind is unknown.
ScreenFxId ScreenFx_Start(const struct ScreenFxConfig *config);
void ScreenFx_Stop(ScreenFxId id);
void ScreenFx_StopAll(void);
bool32 ScreenFx_IsValid(ScreenFxId id);
// Direct request, 0-SCREENFX_INTENSITY_MAX. Cancels any running fade.
void ScreenFx_SetIntensity(ScreenFxId id, u8 intensity);
u8 ScreenFx_GetIntensity(ScreenFxId id);

#endif // GUARD_SCREEN_EFFECTS_H
