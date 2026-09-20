#include "global.h"
#include "screen_effects.h"

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

void ScreenFx_ResetAll(void)
{
    u32 i;

    for (i = 0; i < MAX_SCREEN_EFFECTS; i++)
    {
        if (sScreenFx[i].active)
            ReleaseSlot(&sScreenFx[i]);
    }
}

void ScreenFx_Update(void)
{
    u32 i;

    for (i = 0; i < MAX_SCREEN_EFFECTS; i++)
    {
        struct ScreenFx *effect = &sScreenFx[i];

        if (!effect->active)
            continue;

        effect->resolvedIntensity = effect->enabled ? effect->currentIntensity : 0;
    }
}

ScreenFxId ScreenFx_Start(const struct ScreenFxConfig *config)
{
    u32 i;

    if (config->kind >= SCREENFX_KIND_COUNT)
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
    effect->fadeDuration = 0;
    effect->fadeElapsed = 0;
    effect->stopOnFadeOut = FALSE;
}

u8 ScreenFx_GetIntensity(ScreenFxId id)
{
    struct ScreenFx *effect = GetScreenFx(id);

    return effect != NULL ? effect->currentIntensity : 0;
}
