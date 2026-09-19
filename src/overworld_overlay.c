#include "global.h"
#include "overworld_overlay.h"
#include "event_object_movement.h"
#include "fieldmap.h"
#include "field_weather.h"
#include "palette.h"
#include "sprite.h"

STATIC_ASSERT(MAX_OVERLAYS <= 8, OverlayIndexFitsHandle);
STATIC_ASSERT(sizeof(struct Overlay) <= 40, OverlaySizeBudget);

static EWRAM_DATA struct Overlay sOverlays[MAX_OVERLAYS] = {0};
static EWRAM_DATA u32 sEffectiveExempt[MAX_OVERLAYS] = {0}; // exemptPalettes plus resolved player/object slots
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
        u32 mask = LayerPaletteMask(order[i]->layer) & ~sEffectiveExempt[order[i] - sOverlays];

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

// Triangle wave from pulseMin to pulseMax and back over pulsePeriod frames; returns
// OVERLAY_OPACITY_MAX when no pulse is set. Phase 0 is the minimum.
static u32 UpdatePulse(struct Overlay *overlay)
{
    u32 period = overlay->pulsePeriod;
    u32 phase = overlay->pulsePhase;
    u32 half, tri;

    if (period == 0)
        return OVERLAY_OPACITY_MAX;

    half = period / 2;
    tri = (phase < half ? phase * OVERLAY_OPACITY_MAX : (period - phase) * OVERLAY_OPACITY_MAX) / half;
    tri = min(tri, OVERLAY_OPACITY_MAX); // odd periods overshoot on the turning frame

    overlay->pulsePhase = (phase + 1 >= period) ? 0 : phase + 1;
    return overlay->pulseMin + ((overlay->pulseMax - overlay->pulseMin) * tri) / OVERLAY_OPACITY_MAX;
}

static u32 ResolveOpacity(u32 currentOpacity, u32 pulseFactor, u32 distanceFactor)
{
    return min((currentOpacity * pulseFactor * distanceFactor + 128) / 256, OVERLAY_OPACITY_MAX);
}

static u32 ObjectPaletteBit(u32 objectEventId)
{
    struct ObjectEvent *objectEvent;

    if (objectEventId >= OBJECT_EVENTS_COUNT)
        return 0;

    objectEvent = &gObjectEvents[objectEventId];
    if (!objectEvent->active || objectEvent->spriteId >= MAX_SPRITES)
        return 0;

    return 1u << (16 + gSprites[objectEvent->spriteId].oam.paletteNum);
}

static u32 ResolveExemptPalettes(const struct Overlay *overlay)
{
    u32 mask = overlay->exemptPalettes;

    if (overlay->exemptPlayer)
        mask |= ObjectPaletteBit(gPlayerAvatar.objectEventId);
    if (overlay->exemptLocalId != 0)
        mask |= ObjectPaletteBit(GetObjectEventIdByLocalIdAndMap(overlay->exemptLocalId, overlay->exemptMapNum, overlay->exemptMapGroup));

    return mask;
}

// Refreshes the anchor from the tracked object. An unresolved object keeps the last known position.
static void UpdateAnchor(struct Overlay *overlay)
{
    u32 objectEventId;

    if (overlay->anchorKind != OVERLAY_ANCHOR_OBJECT)
        return;

    objectEventId = GetObjectEventIdByLocalIdAndMap(overlay->anchorLocalId, overlay->anchorMapNum, overlay->anchorMapGroup);
    if (objectEventId == OBJECT_EVENTS_COUNT)
        return;

    overlay->anchorX = gObjectEvents[objectEventId].currentCoords.x;
    overlay->anchorY = gObjectEvents[objectEventId].currentCoords.y;
}

void Overlay_Update(void)
{
    u32 i;

    for (i = 0; i < MAX_OVERLAYS; i++)
    {
        struct Overlay *overlay = &sOverlays[i];
        u32 pulseFactor, distanceFactor, resolved, exempt;

        if (!overlay->active)
            continue;

        // The falloff step runs after the anchor and before the fold.
        if (!UpdateFade(overlay))
            continue;

        pulseFactor = UpdatePulse(overlay);
        UpdateAnchor(overlay);
        distanceFactor = OVERLAY_OPACITY_MAX;

        resolved = ResolveOpacity(overlay->currentOpacity, pulseFactor, distanceFactor);
        if (overlay->resolvedOpacity != resolved)
        {
            overlay->resolvedOpacity = resolved;
            sOverlayDirty = TRUE;
        }

        // Sprite palette slots are reallocated on map load, so they are re-read every frame.
        exempt = ResolveExemptPalettes(overlay);
        if (sEffectiveExempt[i] != exempt)
        {
            sEffectiveExempt[i] = exempt;
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

void Overlay_Pulse(OverlayId id, u8 minOpacity, u8 maxOpacity, u16 periodFrames)
{
    struct Overlay *overlay = GetOverlay(id);

    if (overlay == NULL)
        return;

    // A period under 2 frames has no half-cycle, so it clears the pulse.
    if (periodFrames < 2)
    {
        Overlay_StopAnimation(id);
        return;
    }

    minOpacity = min(minOpacity, OVERLAY_OPACITY_MAX);
    maxOpacity = min(maxOpacity, OVERLAY_OPACITY_MAX);
    overlay->pulseMin = min(minOpacity, maxOpacity);
    overlay->pulseMax = max(minOpacity, maxOpacity);
    overlay->pulsePeriod = periodFrames;
    overlay->pulsePhase = 0;
}

// Clears the pulse only; the fade and currentOpacity are untouched.
void Overlay_StopAnimation(OverlayId id)
{
    struct Overlay *overlay = GetOverlay(id);

    if (overlay == NULL)
        return;

    overlay->pulseMin = 0;
    overlay->pulseMax = 0;
    overlay->pulsePeriod = 0;
    overlay->pulsePhase = 0;
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

void Overlay_SetAnchorToPosition(OverlayId id, s16 x, s16 y)
{
    struct Overlay *overlay = GetOverlay(id);

    if (overlay == NULL)
        return;

    overlay->anchorKind = OVERLAY_ANCHOR_COORDS;
    overlay->anchorX = x + MAP_OFFSET;
    overlay->anchorY = y + MAP_OFFSET;
}

void Overlay_SetAnchorToObject(OverlayId id, u8 localId, u8 mapNum, u8 mapGroup)
{
    struct Overlay *overlay = GetOverlay(id);

    if (overlay == NULL)
        return;

    overlay->anchorKind = OVERLAY_ANCHOR_OBJECT;
    overlay->anchorLocalId = localId;
    overlay->anchorMapNum = mapNum;
    overlay->anchorMapGroup = mapGroup;
    UpdateAnchor(overlay);
}

void Overlay_ClearAnchor(OverlayId id)
{
    struct Overlay *overlay = GetOverlay(id);

    if (overlay == NULL)
        return;

    overlay->anchorKind = OVERLAY_ANCHOR_NONE;
    overlay->anchorX = 0;
    overlay->anchorY = 0;
    overlay->anchorLocalId = 0;
    overlay->anchorMapNum = 0;
    overlay->anchorMapGroup = 0;
}

void Overlay_ExemptPalette(OverlayId id, u8 paletteIndex)
{
    struct Overlay *overlay = GetOverlay(id);

    if (overlay == NULL || paletteIndex >= 32)
        return;

    overlay->exemptPalettes |= 1u << paletteIndex;
}

void Overlay_UnexemptPalette(OverlayId id, u8 paletteIndex)
{
    struct Overlay *overlay = GetOverlay(id);

    if (overlay == NULL || paletteIndex >= 32)
        return;

    overlay->exemptPalettes &= ~(1u << paletteIndex);
}

void Overlay_ExemptPlayer(OverlayId id, bool32 exempt)
{
    struct Overlay *overlay = GetOverlay(id);

    if (overlay == NULL)
        return;

    overlay->exemptPlayer = (exempt != FALSE);
}

void Overlay_ExemptObject(OverlayId id, u8 localId, bool32 exempt)
{
    struct Overlay *overlay = GetOverlay(id);

    if (overlay == NULL || localId == 0)
        return;

    if (exempt)
    {
        overlay->exemptLocalId = localId;
        overlay->exemptMapNum = gSaveBlock1Ptr->location.mapNum;
        overlay->exemptMapGroup = gSaveBlock1Ptr->location.mapGroup;
    }
    else if (overlay->exemptLocalId == localId)
    {
        overlay->exemptLocalId = 0;
    }
}

void Overlay_SetRenderLayer(OverlayId id, u8 layer)
{
    struct Overlay *overlay = GetOverlay(id);

    if (overlay == NULL || overlay->layer == layer)
        return;

    overlay->layer = layer;
    sOverlayDirty = TRUE;
}
