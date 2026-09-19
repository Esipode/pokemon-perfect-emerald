#include "global.h"
#include "overworld_overlay.h"

STATIC_ASSERT(MAX_OVERLAYS <= 8, OverlayIndexFitsHandle);
STATIC_ASSERT(sizeof(struct Overlay) <= 36, OverlaySizeBudget);

static EWRAM_DATA struct Overlay sOverlays[MAX_OVERLAYS] = {0};

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
