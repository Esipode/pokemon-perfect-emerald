#include "global.h"
#include "overworld_overlay.h"
#include "field_weather.h"
#include "palette.h"

STATIC_ASSERT(MAX_OVERLAYS <= 8, OverlayIndexFitsHandle);
STATIC_ASSERT(sizeof(struct Overlay) <= 36, OverlaySizeBudget);

static EWRAM_DATA struct Overlay sOverlays[MAX_OVERLAYS] = {0};
static EWRAM_DATA bool8 sOverlayDirty = FALSE;   // gPlttBufferFaded must be recomposed
static EWRAM_DATA bool8 sOverlayApplied = FALSE; // gPlttBufferFaded currently holds overlay tint

static u8 NextGeneration(u8 generation)
{
    // Generation 0 is never issued, so it is skipped on wrap.
    return generation == 0xFF ? 1 : generation + 1;
}

// Clears the slot; the bumped generation makes existing handles stale.
static void ReleaseSlot(struct Overlay *overlay)
{
    u8 generation = overlay->generation;

    memset(overlay, 0, sizeof(*overlay));
    overlay->generation = NextGeneration(generation);
    sOverlayDirty = TRUE;
}

static struct Overlay *GetOverlay(OverlayId id)
{
    struct Overlay *overlay;

    if (id == OVERLAY_ID_INVALID || OVERLAY_INDEX(id) >= MAX_OVERLAYS)
        return NULL;

    overlay = &sOverlays[OVERLAY_INDEX(id)];
    if (!overlay->active || overlay->generation != OVERLAY_GENERATION(id))
        return NULL;

    return overlay;
}

void Overlay_ResetAll(void)
{
    u32 i;

    for (i = 0; i < MAX_OVERLAYS; i++)
    {
        if (sOverlays[i].active)
            ReleaseSlot(&sOverlays[i]);
    }
}

void Overlay_DestroyMapLocal(void)
{
    u32 i;

    for (i = 0; i < MAX_OVERLAYS; i++)
    {
        if (sOverlays[i].active && sOverlays[i].scope == OVERLAY_SCOPE_MAP_LOCAL)
            ReleaseSlot(&sOverlays[i]);
    }
}

static u32 LayerPaletteMask(u32 layer)
{
    // Excludes the UI palettes so text boxes are never tinted.
    switch (layer)
    {
    case OVERLAY_LAYER_WORLD:
        return PALETTES_MAP;
    case OVERLAY_LAYER_OBJECTS:
        return PALETTES_OBJECTS;
    case OVERLAY_LAYER_ALL:
        return PALETTES_MAP | PALETTES_OBJECTS;
    default:
        return 0;
    }
}

// Collects tinting overlays sorted by ascending priority; equal priorities keep pool order.
static u32 GetRenderOrder(struct Overlay *order[MAX_OVERLAYS])
{
    u32 count = 0;
    u32 i, j;

    for (i = 0; i < MAX_OVERLAYS; i++)
    {
        struct Overlay *overlay = &sOverlays[i];

        if (!overlay->active || !overlay->enabled || overlay->resolvedOpacity == 0)
            continue;

        for (j = count; j > 0 && order[j - 1]->priority > overlay->priority; j--)
            order[j] = order[j - 1];
        order[j] = overlay;
        count++;
    }

    return count;
}

static void ApplyOverlaysToPalettes(void)
{
    struct Overlay *order[MAX_OVERLAYS];
    u32 count, i;

    // The palette fade and the weather fade-in own gPlttBufferFaded and overwrite it.
    if (gPaletteFade.active || !IsWeatherNotFadingIn())
    {
        sOverlayDirty = TRUE;
        return;
    }

    if (!sOverlayDirty)
        return;

    count = GetRenderOrder(order);
    if (count != 0 || sOverlayApplied)
    {
        // Rebuilds from gPlttBufferUnfaded, which overlays never modify.
        ApplyWeatherColorMapToPals(0, 32);
    }
    sOverlayDirty = FALSE; // the rebuild invalidates through the weather hook

    for (i = 0; i < count; i++)
    {
        u32 mask = LayerPaletteMask(order[i]->layer) & ~order[i]->exemptPalettes;

        BlendPalettesFine(mask, gPlttBufferFaded, gPlttBufferFaded, order[i]->resolvedOpacity, order[i]->color);
    }

    sOverlayApplied = (count != 0);
}

static void ClearFade(struct Overlay *overlay)
{
    overlay->fadeDuration = 0;
    overlay->fadeElapsed = 0;
    overlay->destroyOnFadeOut = FALSE;
}

// Returns FALSE when the fade destroyed the overlay.
static bool32 UpdateFade(struct Overlay *overlay)
{
    if (overlay->fadeDuration == 0)
        return TRUE;

    overlay->fadeElapsed++;
    if (overlay->fadeElapsed < overlay->fadeDuration)
    {
        overlay->currentOpacity = overlay->fadeStart
            + ((s32)overlay->fadeTarget - overlay->fadeStart) * overlay->fadeElapsed / overlay->fadeDuration;
        return TRUE;
    }

    overlay->currentOpacity = overlay->fadeTarget;
    if (overlay->destroyOnFadeOut)
    {
        ReleaseSlot(overlay);
        return FALSE;
    }

    ClearFade(overlay);
    return TRUE;
}

void Overlay_Update(void)
{
    u32 i;

    for (i = 0; i < MAX_OVERLAYS; i++)
    {
        struct Overlay *overlay = &sOverlays[i];

        if (!overlay->active)
            continue;

        // Pulse, anchor and falloff steps run here, in that order, before the fold.
        if (!UpdateFade(overlay))
            continue;

        if (overlay->resolvedOpacity != overlay->currentOpacity)
        {
            overlay->resolvedOpacity = overlay->currentOpacity;
            sOverlayDirty = TRUE;
        }
    }

    ApplyOverlaysToPalettes();
}

void Overlay_Invalidate(void)
{
    sOverlayDirty = TRUE;
}

OverlayId Overlay_Create(const struct OverlayConfig *config)
{
    u32 i;

    for (i = 0; i < MAX_OVERLAYS; i++)
    {
        struct Overlay *overlay = &sOverlays[i];
        u8 opacity = min(config->opacity, OVERLAY_OPACITY_MAX);

        if (overlay->active)
            continue;

        // A never-used slot still holds generation 0.
        if (overlay->generation == 0)
            overlay->generation = 1;

        overlay->active = TRUE;
        overlay->enabled = TRUE;
        overlay->color = config->color;
        overlay->layer = config->layer;
        overlay->scope = config->scope;
        overlay->priority = config->priority;
        overlay->baseOpacity = opacity;
        overlay->currentOpacity = opacity;
        overlay->resolvedOpacity = opacity;
        sOverlayDirty = TRUE;
        return (overlay->generation << 3) | i;
    }

    return OVERLAY_ID_INVALID;
}

void Overlay_Destroy(OverlayId id)
{
    struct Overlay *overlay = GetOverlay(id);

    if (overlay == NULL)
        return;

    ReleaseSlot(overlay);
}

bool32 Overlay_IsValid(OverlayId id)
{
    return GetOverlay(id) != NULL;
}

void Overlay_Enable(OverlayId id)
{
    struct Overlay *overlay = GetOverlay(id);

    if (overlay == NULL || overlay->enabled)
        return;

    overlay->enabled = TRUE;
    sOverlayDirty = TRUE;
}

void Overlay_Disable(OverlayId id)
{
    struct Overlay *overlay = GetOverlay(id);

    if (overlay == NULL || !overlay->enabled)
        return;

    overlay->enabled = FALSE;
    sOverlayDirty = TRUE;
}

void Overlay_SetColor(OverlayId id, u16 color)
{
    struct Overlay *overlay = GetOverlay(id);

    if (overlay == NULL || overlay->color == color)
        return;

    overlay->color = color;
    sOverlayDirty = TRUE;
}

void Overlay_SetOpacity(OverlayId id, u8 opacity)
{
    struct Overlay *overlay = GetOverlay(id);

    if (overlay == NULL)
        return;

    opacity = min(opacity, OVERLAY_OPACITY_MAX);
    ClearFade(overlay);
    overlay->baseOpacity = opacity;
    overlay->currentOpacity = opacity;
}

static void StartFade(struct Overlay *overlay, u8 targetOpacity, u16 durationFrames)
{
    targetOpacity = min(targetOpacity, OVERLAY_OPACITY_MAX);
    overlay->baseOpacity = targetOpacity;

    if (durationFrames == 0 || overlay->currentOpacity == targetOpacity)
    {
        overlay->currentOpacity = targetOpacity;
        overlay->fadeDuration = 0;
        overlay->fadeElapsed = 0;
        return;
    }

    overlay->fadeStart = overlay->currentOpacity;
    overlay->fadeTarget = targetOpacity;
    overlay->fadeDuration = durationFrames;
    overlay->fadeElapsed = 0;
}

void Overlay_FadeTo(OverlayId id, u8 targetOpacity, u16 durationFrames)
{
    struct Overlay *overlay = GetOverlay(id);

    if (overlay == NULL)
        return;

    overlay->destroyOnFadeOut = FALSE;
    StartFade(overlay, targetOpacity, durationFrames);
}

void Overlay_FadeOutAndDisable(OverlayId id, u16 durationFrames)
{
    struct Overlay *overlay = GetOverlay(id);

    if (overlay == NULL)
        return;

    StartFade(overlay, 0, durationFrames);
    if (overlay->fadeDuration == 0)
        ReleaseSlot(overlay);
    else
        overlay->destroyOnFadeOut = TRUE;
}

u8 Overlay_GetOpacity(OverlayId id)
{
    struct Overlay *overlay = GetOverlay(id);

    return overlay != NULL ? overlay->currentOpacity : 0;
}

void Overlay_SetRenderLayer(OverlayId id, u8 layer)
{
    struct Overlay *overlay = GetOverlay(id);

    if (overlay == NULL || overlay->layer == layer)
        return;

    overlay->layer = layer;
    sOverlayDirty = TRUE;
}
