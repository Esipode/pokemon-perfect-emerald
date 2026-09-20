#include "global.h"
#include "screen_effects.h"
#include "event_object_movement.h"
#include "field_camera.h"
#include "fieldmap.h"
#include "overworld_overlay.h"
#include "scanline_effect.h"
#include "trig.h"

#define LIFETIME_FADE_MAX       30 // frames
#define SHAKE_DEFAULT_PERIOD    32
#define SHAKE_SCALE             (256 * SCREENFX_INTENSITY_MAX)  // Q8.8 sine times intensity

#define SCANLINE_COUNT          DISPLAY_HEIGHT
#define SCANLINE_REGS           6   // BG1HOFS, BG1VOFS, BG2HOFS, BG2VOFS, BG3HOFS, BG3VOFS
#define SCANLINE_OFFSET_MAX     8   // pixels; larger offsets pull undrawn columns in at map edges
#define SCANLINE_DMA_CONTROL    (((DMA_ENABLE | DMA_START_HBLANK | DMA_REPEAT | DMA_SRC_INC | DMA_DEST_INC | DMA_16BIT | DMA_DEST_RELOAD) << 16) | SCANLINE_REGS)

STATIC_ASSERT(MAX_SCREEN_EFFECTS <= 8, ScreenFxIndexFitsHandle);
STATIC_ASSERT(sizeof(struct ScreenFx) <= 40, ScreenFxSizeBudget);
STATIC_ASSERT(SCANLINE_COUNT * SCANLINE_REGS == ARRAY_COUNT(gScanlineEffectRegBuffers[0]), ScanlineBufferFit);

// What one gScanlineEffectRegBuffers half currently holds, so unchanged data is not rewritten.
struct ScanlineBufferState
{
    s16 baseX;
    s16 baseY;
    u8 valid:1;
    u8 hasHorizontal:1;     // holds non-zero horizontal line offsets
    u8 hasVertical:1;
};

struct ScanlineChannel
{
    struct ScanlineBufferState buffers[2];
    u8 acquired:1;          // owns gScanlineEffectRegBuffers
    u8 dmaRunning:1;        // DMA0 was installed by this channel
    u8 ready:1;             // write buffer built since the last VBlank
    u8 readValid:1;         // read buffer holds a complete frame
    u8 hasHorizontal:1;     // this frame's offsets
    u8 hasVertical:1;
    u8 writeBuffer;
    u8 readBuffer;
};

static EWRAM_DATA struct ScreenFx sScreenFx[MAX_SCREEN_EFFECTS] = {0};
static EWRAM_DATA struct ScanlineChannel sChannel = {0};
// Summed per-line offsets of every geometry effect; clamped when the buffer is built.
static EWRAM_DATA s16 sLineDx[SCANLINE_COUNT] = {0};
static EWRAM_DATA s16 sLineDy[SCANLINE_COUNT] = {0};

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

static bool32 IsGeometryKind(u32 kind)
{
    return kind == SCREENFX_WAVE || kind == SCREENFX_RIPPLE || kind == SCREENFX_TEAR;
}

static bool32 HasGeometryEffect(void)
{
    u32 i;

    for (i = 0; i < MAX_SCREEN_EFFECTS; i++)
    {
        if (sScreenFx[i].active && IsGeometryKind(sScreenFx[i].kind))
            return TRUE;
    }

    return FALSE;
}

// Scanline channel: one HBlank DMA0 stream of six BG scroll registers per line, built from the
// camera offset plus the summed per-line offsets. Only claimed while gScanlineEffect is idle.

static void ScanlineChannel_ClearOffsets(void)
{
    memset(sLineDx, 0, sizeof(sLineDx));
    memset(sLineDy, 0, sizeof(sLineDy));
    sChannel.hasHorizontal = FALSE;
    sChannel.hasVertical = FALSE;
}

static bool32 ScanlineChannel_Acquire(void)
{
    if (sChannel.acquired)
        return TRUE;

    // The flash and Battle Pyramid effects own the buffers and DMA0 while their state is set.
    if (gScanlineEffect.state != 0)
        return FALSE;

    // Keeps dmaRunning so a stop still pending from a recent release is honoured.
    memset(sChannel.buffers, 0, sizeof(sChannel.buffers));
    sChannel.ready = FALSE;
    sChannel.readValid = FALSE;
    sChannel.writeBuffer = 0;
    sChannel.readBuffer = 0;
    sChannel.acquired = TRUE;
    ScanlineChannel_ClearOffsets();
    return TRUE;
}

// The DMA is stopped at the next VBlank so the frame in progress is not cut short.
static void ScanlineChannel_Release(void)
{
    sChannel.acquired = FALSE;
    sChannel.readValid = FALSE;
    sChannel.ready = FALSE;
}

// Stops DMA0 at once. Used where the screen is being rebuilt anyway.
static void ScanlineChannel_Reset(void)
{
    if (sChannel.dmaRunning && gScanlineEffect.state == 0)
        DmaStop(0);

    memset(&sChannel, 0, sizeof(sChannel));
}

// Adds a pixel offset to one scanline. Only meaningful between ScreenFx_Update and ScreenFx_Render.
static void UNUSED ScanlineChannel_Line(u32 line, s16 dx, s16 dy)
{
    if (!sChannel.acquired || line >= SCANLINE_COUNT)
        return;

    if (dx != 0)
    {
        sLineDx[line] += dx;
        sChannel.hasHorizontal = TRUE;
    }
    if (dy != 0)
    {
        sLineDy[line] += dy;
        sChannel.hasVertical = TRUE;
    }
}

static s32 ClampLineOffset(s32 offset)
{
    return offset < -SCANLINE_OFFSET_MAX ? -SCANLINE_OFFSET_MAX
         : offset > SCANLINE_OFFSET_MAX ? SCANLINE_OFFSET_MAX
         : offset;
}

// A full rewrite happens only when the camera moved or vertical offsets are involved; horizontal-only
// changes patch the three H fields per line.
static void ScanlineChannel_Build(void)
{
    struct ScanlineBufferState *state = &sChannel.buffers[sChannel.writeBuffer];
    u16 *buffer = gScanlineEffectRegBuffers[sChannel.writeBuffer];
    bool32 vertical = sChannel.hasVertical || state->hasVertical;
    bool32 horizontal = sChannel.hasHorizontal || state->hasHorizontal;
    s16 baseX, baseY;
    u32 line;

    GetCameraOffsetWithPan(&baseX, &baseY);

    if (!state->valid || baseX != state->baseX || baseY != state->baseY || vertical)
    {
        for (line = 0; line < SCANLINE_COUNT; line++)
        {
            u16 *regs = &buffer[line * SCANLINE_REGS];
            u16 x = baseX + ClampLineOffset(sLineDx[line]);
            u16 y = baseY + ClampLineOffset(sLineDy[line]);

            regs[0] = regs[2] = regs[4] = x;
            regs[1] = regs[3] = regs[5] = y;
        }
    }
    else if (horizontal)
    {
        for (line = 0; line < SCANLINE_COUNT; line++)
        {
            u16 *regs = &buffer[line * SCANLINE_REGS];

            regs[0] = regs[2] = regs[4] = baseX + ClampLineOffset(sLineDx[line]);
        }
    }

    state->valid = TRUE;
    state->baseX = baseX;
    state->baseY = baseY;
    state->hasHorizontal = sChannel.hasHorizontal;
    state->hasVertical = sChannel.hasVertical;
    sChannel.ready = TRUE;
}

static void ReleaseAllEffects(void)
{
    u32 i;

    for (i = 0; i < MAX_SCREEN_EFFECTS; i++)
    {
        if (sScreenFx[i].active)
            ReleaseSlot(&sScreenFx[i]);
    }
}

void ScreenFx_ResetAll(void)
{
    ReleaseAllEffects();
    ScanlineChannel_Reset();
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

    if (sChannel.acquired)
        ScanlineChannel_ClearOffsets();

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

// Runs after UpdateCameraPanning so the buffer holds the camera offset VBlank will apply.
void ScreenFx_Render(void)
{
    bool32 wanted = HasGeometryEffect();

    if (sChannel.acquired && (!wanted || gScanlineEffect.state != 0))
        ScanlineChannel_Release();

    if (wanted && !sChannel.acquired)
        ScanlineChannel_Acquire();

    if (sChannel.acquired)
        ScanlineChannel_Build();
}

// Runs after FieldUpdateBgTilemapScroll. Line 0 is drawn before the first HBlank transfer, so it keeps
// the base offsets and the stream starts at line 1. The final transfer reads six halfwords past the
// buffer; it lands after the last visible line.
void ScreenFx_VBlank(void)
{
    if (gScanlineEffect.state != 0)
    {
        // The stock effect has already reprogrammed DMA0.
        sChannel.dmaRunning = FALSE;
        return;
    }

    if (!sChannel.acquired)
    {
        if (sChannel.dmaRunning)
        {
            DmaStop(0);
            sChannel.dmaRunning = FALSE;
        }
        return;
    }

    if (sChannel.ready)
    {
        sChannel.readBuffer = sChannel.writeBuffer;
        sChannel.writeBuffer ^= 1;
        sChannel.ready = FALSE;
        sChannel.readValid = TRUE;
    }

    if (!sChannel.readValid)
        return;

    DmaStop(0);
    DmaSet(0, &gScanlineEffectRegBuffers[sChannel.readBuffer][SCANLINE_REGS], &REG_BG1HOFS, SCANLINE_DMA_CONTROL);
    sChannel.dmaRunning = TRUE;
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
    ReleaseAllEffects();
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
