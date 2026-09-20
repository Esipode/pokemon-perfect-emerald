#include "global.h"
#include "screen_effects.h"
#include "event_object_movement.h"
#include "field_camera.h"
#include "fieldmap.h"
#include "overworld_overlay.h"
#include "trig.h"

#define LIFETIME_FADE_MAX       30 // frames
#define SHAKE_DEFAULT_PERIOD    32
#define SHAKE_SCALE             (256 * SCREENFX_INTENSITY_MAX)  // Q8.8 sine times intensity

STATIC_ASSERT(MAX_SCREEN_EFFECTS <= 8, ScreenFxIndexFitsHandle);
STATIC_ASSERT(sizeof(struct ScreenFx) <= 40, ScreenFxSizeBudget);

static EWRAM_DATA struct ScreenFx sScreenFx[MAX_SCREEN_EFFECTS] = {0};

static u8 NextGeneration(u8 generation)
{
    // Generation 0 is never issued, so it is skipped on wrap.
    return generation == 0xFF ? 1 : generation + 1;
}

// Clears the slot; the bumped generation makes existing handles stale.
static void ReleaseSlot(struct ScreenFx *effect)
{
    u8 generation = effect->generation;

    // Also zeroes the pan.
    if (effect->kind == SCREENFX_SHAKE)
        InstallCameraPanAheadCallback();

    memset(effect, 0, sizeof(*effect));
    effect->generation = NextGeneration(generation);
}

static struct ScreenFx *GetScreenFx(ScreenFxId id)
{
    struct ScreenFx *effect;

    if (id == SCREENFX_ID_INVALID || SCREENFX_INDEX(id) >= MAX_SCREEN_EFFECTS)
        return NULL;

    effect = &sScreenFx[SCREENFX_INDEX(id)];
    if (!effect->active || effect->generation != SCREENFX_GENERATION(id))
        return NULL;

    return effect;
}

static bool32 IsShakeActive(void)
{
    u32 i;

    for (i = 0; i < MAX_SCREEN_EFFECTS; i++)
    {
        if (sScreenFx[i].active && sScreenFx[i].kind == SCREENFX_SHAKE)
            return TRUE;
    }

    return FALSE;
}

void ScreenFx_ResetAll(void)
{
    u32 i;

    for (i = 0; i < MAX_SCREEN_EFFECTS; i++)
    {
        if (sScreenFx[i].active)
            ReleaseSlot(&sScreenFx[i]);
    }
}

// Rounds to the nearest pixel, symmetric around zero.
static s32 ScaleShake(s32 sine, s32 amplitude)
{
    s32 scaled = sine * amplitude;

    return (scaled + (scaled < 0 ? -SHAKE_SCALE / 2 : SHAKE_SCALE / 2)) / SHAKE_SCALE;
}

// The callback is cleared every frame: field effects that finish reinstall pan-ahead, which would
// zero the pan in UpdateCameraPanning.
static void UpdateShake(struct ScreenFx *effect)
{
    s32 amplitude = effect->resolvedIntensity * SCREENFX_SHAKE_MAX_AMPLITUDE;
    u32 index = effect->params.shake.phase >> 8; // gSineTable holds one cycle per 256 entries
    s16 x = 0, y = 0;

    if (effect->params.shake.axes & SCREENFX_AXIS_X)
        x = ScaleShake(gSineTable[index], amplitude);
    if (effect->params.shake.axes & SCREENFX_AXIS_Y)
        y = ScaleShake(gSineTable[index + 64], amplitude); // cosine

    SetCameraPanningCallback(NULL);
    SetCameraPanning(x, y);
    effect->params.shake.phase += 0x10000 / effect->params.shake.period;
}

static void ClearFade(struct ScreenFx *effect)
{
    effect->fadeDuration = 0;
    effect->fadeElapsed = 0;
    effect->stopOnFadeOut = FALSE;
}

// Returns FALSE when the fade stopped the effect.
static bool32 UpdateFade(struct ScreenFx *effect)
{
    if (effect->fadeDuration == 0)
        return TRUE;

    effect->fadeElapsed++;
    if (effect->fadeElapsed < effect->fadeDuration)
    {
        effect->currentIntensity = effect->fadeStart
            + ((s32)effect->fadeTarget - effect->fadeStart) * effect->fadeElapsed / effect->fadeDuration;
        return TRUE;
    }

    effect->currentIntensity = effect->fadeTarget;
    if (effect->stopOnFadeOut)
    {
        ReleaseSlot(effect);
        return FALSE;
    }

    ClearFade(effect);
    return TRUE;
}

static void StartFade(struct ScreenFx *effect, u8 targetIntensity, u16 durationFrames)
{
    targetIntensity = min(targetIntensity, SCREENFX_INTENSITY_MAX);
    effect->baseIntensity = targetIntensity;

    if (durationFrames == 0 || effect->currentIntensity == targetIntensity)
    {
        effect->currentIntensity = targetIntensity;
        effect->fadeDuration = 0;
        effect->fadeElapsed = 0;
        return;
    }

    effect->fadeStart = effect->currentIntensity;
    effect->fadeTarget = targetIntensity;
    effect->fadeDuration = durationFrames;
    effect->fadeElapsed = 0;
}

static void StartFadeOutAndStop(struct ScreenFx *effect, u16 durationFrames)
{
    StartFade(effect, 0, durationFrames);
    if (effect->fadeDuration == 0)
        ReleaseSlot(effect);
    else
        effect->stopOnFadeOut = TRUE;
}

// Starts the fade-out when the lifetime runs out. Returns FALSE when that stopped the effect.
static bool32 UpdateLifetime(struct ScreenFx *effect)
{
    if (effect->lifetime == 0 || effect->elapsed >= effect->lifetime)
        return TRUE;

    effect->elapsed++;
    if (effect->elapsed < effect->lifetime)
        return TRUE;

    StartFadeOutAndStop(effect, min(effect->lifetime / 4, LIFETIME_FADE_MAX));
    return effect->active;
}

// Refreshes the anchor from the tracked object. An unresolved object keeps the last known position.
static void UpdateAnchor(struct ScreenFx *effect)
{
    if (effect->anchorKind != SCREENFX_ANCHOR_OBJECT)
        return;

    OverworldAnchor_Resolve(effect->anchorLocalId, effect->anchorMapNum, effect->anchorMapGroup,
                            &effect->anchorX, &effect->anchorY);
}

static u32 CalcDistanceFactor(const struct ScreenFx *effect, const struct Coords16 *playerCoords)
{
    u32 distance = OverworldAnchor_Distance(effect->anchorX, effect->anchorY, playerCoords);

    return OverworldAnchor_DistanceFactor(distance, effect->innerRadius, effect->outerRadius,
                                          effect->minIntensity, effect->maxIntensity);
}

// Order: fade, lifetime, anchor, falloff, final intensity, render.
void ScreenFx_Update(void)
{
    struct Coords16 playerCoords = {0};
    bool32 playerRead = FALSE;
    u32 i;

    for (i = 0; i < MAX_SCREEN_EFFECTS; i++)
    {
        struct ScreenFx *effect = &sScreenFx[i];
        u32 distanceFactor = SCREENFX_INTENSITY_MAX;

        if (!effect->active)
            continue;

        if (!UpdateFade(effect) || !UpdateLifetime(effect))
            continue;

        UpdateAnchor(effect);
        if (effect->falloffEnabled && effect->anchorKind != SCREENFX_ANCHOR_NONE)
        {
            if (!playerRead)
            {
                playerCoords = gObjectEvents[gPlayerAvatar.objectEventId].currentCoords;
                playerRead = TRUE;
            }

            distanceFactor = CalcDistanceFactor(effect, &playerCoords);
        }

        effect->resolvedIntensity = effect->enabled
            ? min((effect->currentIntensity * distanceFactor + SCREENFX_INTENSITY_MAX / 2) / SCREENFX_INTENSITY_MAX, SCREENFX_INTENSITY_MAX)
            : 0;

        if (effect->kind == SCREENFX_SHAKE)
            UpdateShake(effect);
    }
}

ScreenFxId ScreenFx_Start(const struct ScreenFxConfig *config)
{
    u32 i;

    if (config->kind >= SCREENFX_KIND_COUNT)
        return SCREENFX_ID_INVALID;

    if (config->kind == SCREENFX_SHAKE && IsShakeActive())
        return SCREENFX_ID_INVALID;

    for (i = 0; i < MAX_SCREEN_EFFECTS; i++)
    {
        struct ScreenFx *effect = &sScreenFx[i];
        u8 intensity = min(config->intensity, SCREENFX_INTENSITY_MAX);
        u8 generation = effect->generation;

        if (effect->active)
            continue;

        // A never-used slot still holds generation 0.
        memset(effect, 0, sizeof(*effect));
        effect->generation = generation != 0 ? generation : 1;
        effect->active = TRUE;
        effect->enabled = TRUE;
        effect->kind = config->kind;
        effect->lifetime = config->durationFrames;
        effect->baseIntensity = intensity;
        effect->currentIntensity = intensity;
        effect->resolvedIntensity = intensity;
        effect->params.config.param1 = config->param1;
        effect->params.config.param2 = config->param2;

        if (config->kind == SCREENFX_SHAKE)
        {
            if (effect->params.shake.period == 0)
                effect->params.shake.period = SHAKE_DEFAULT_PERIOD;
            if (!(effect->params.shake.axes & (SCREENFX_AXIS_X | SCREENFX_AXIS_Y)))
                effect->params.shake.axes = SCREENFX_AXIS_X | SCREENFX_AXIS_Y;

            SetCameraPanningCallback(NULL);
        }

        return (effect->generation << 3) | i;
    }

    return SCREENFX_ID_INVALID;
}

void ScreenFx_Stop(ScreenFxId id)
{
    struct ScreenFx *effect = GetScreenFx(id);

    if (effect == NULL)
        return;

    ReleaseSlot(effect);
}

void ScreenFx_StopAll(void)
{
    ScreenFx_ResetAll();
}

bool32 ScreenFx_IsValid(ScreenFxId id)
{
    return GetScreenFx(id) != NULL;
}

void ScreenFx_SetIntensity(ScreenFxId id, u8 intensity)
{
    struct ScreenFx *effect = GetScreenFx(id);

    if (effect == NULL)
        return;

    intensity = min(intensity, SCREENFX_INTENSITY_MAX);
    effect->baseIntensity = intensity;
    effect->currentIntensity = intensity;
    ClearFade(effect);
}

u8 ScreenFx_GetIntensity(ScreenFxId id)
{
    struct ScreenFx *effect = GetScreenFx(id);

    return effect != NULL ? effect->currentIntensity : 0;
}

void ScreenFx_FadeTo(ScreenFxId id, u8 targetIntensity, u16 durationFrames)
{
    struct ScreenFx *effect = GetScreenFx(id);

    if (effect == NULL)
        return;

    effect->stopOnFadeOut = FALSE;
    StartFade(effect, targetIntensity, durationFrames);
}

void ScreenFx_FadeOutAndStop(ScreenFxId id, u16 durationFrames)
{
    struct ScreenFx *effect = GetScreenFx(id);

    if (effect == NULL)
        return;

    StartFadeOutAndStop(effect, durationFrames);
}

void ScreenFx_SetAnchorToPosition(ScreenFxId id, s16 x, s16 y)
{
    struct ScreenFx *effect = GetScreenFx(id);

    if (effect == NULL)
        return;

    effect->anchorKind = SCREENFX_ANCHOR_COORDS;
    effect->anchorX = x + MAP_OFFSET;
    effect->anchorY = y + MAP_OFFSET;
}

void ScreenFx_SetAnchorToObject(ScreenFxId id, u8 localId, u8 mapNum, u8 mapGroup)
{
    struct ScreenFx *effect = GetScreenFx(id);

    if (effect == NULL)
        return;

    effect->anchorKind = SCREENFX_ANCHOR_OBJECT;
    effect->anchorLocalId = localId;
    effect->anchorMapNum = mapNum;
    effect->anchorMapGroup = mapGroup;
    UpdateAnchor(effect);
}

void ScreenFx_ClearAnchor(ScreenFxId id)
{
    struct ScreenFx *effect = GetScreenFx(id);

    if (effect == NULL)
        return;

    effect->anchorKind = SCREENFX_ANCHOR_NONE;
    effect->anchorX = 0;
    effect->anchorY = 0;
    effect->anchorLocalId = 0;
    effect->anchorMapNum = 0;
    effect->anchorMapGroup = 0;
}

void ScreenFx_SetFalloff(ScreenFxId id, u8 innerRadius, u8 outerRadius, u8 minIntensity, u8 maxIntensity)
{
    struct ScreenFx *effect = GetScreenFx(id);

    if (effect == NULL)
        return;

    effect->innerRadius = innerRadius;
    effect->outerRadius = outerRadius;
    effect->minIntensity = min(minIntensity, SCREENFX_INTENSITY_MAX);
    effect->maxIntensity = min(maxIntensity, SCREENFX_INTENSITY_MAX);
    effect->falloffEnabled = TRUE;
}

void ScreenFx_ClearFalloff(ScreenFxId id)
{
    struct ScreenFx *effect = GetScreenFx(id);

    if (effect == NULL)
        return;

    effect->innerRadius = 0;
    effect->outerRadius = 0;
    effect->minIntensity = 0;
    effect->maxIntensity = 0;
    effect->falloffEnabled = FALSE;
}
