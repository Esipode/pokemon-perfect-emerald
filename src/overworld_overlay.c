#include "global.h"
#include "overworld_overlay.h"
#include "event_object_movement.h"
#include "fieldmap.h"
#include "field_weather.h"
#include "palette.h"
#include "sprite.h"
#include "constants/rgb.h"

STATIC_ASSERT(MAX_OVERLAYS <= 8, OverlayIndexFitsHandle);
STATIC_ASSERT(sizeof(struct Overlay) <= 40, OverlaySizeBudget);

static EWRAM_DATA struct Overlay sOverlays[MAX_OVERLAYS] = {0};
static EWRAM_DATA u32 sEffectiveExempt[MAX_OVERLAYS] = {0}; // exemptPalettes plus resolved player/object slots
static EWRAM_DATA u8 sDistanceFactor[MAX_OVERLAYS] = {0};
static EWRAM_DATA struct Coords16 sLastPlayerCoords = {0};
static EWRAM_DATA u8 sFalloffFrames = 0;
static EWRAM_DATA bool8 sDistanceForce = FALSE;  // recompute every distance factor on the next update
static EWRAM_DATA bool8 sOverlayDirty = FALSE;   // gPlttBufferFaded must be recomposed
static EWRAM_DATA bool8 sOverlayApplied = FALSE; // gPlttBufferFaded currently holds overlay tint

#define OVERLAY_GLOW_TILE_TAG       0x8020
#define OVERLAY_GLOW_PAL_TAG_BASE   0x8020 // + overlay index; bit 15 makes the palette weather-immune
#define GLOW_RETRY_FRAMES           30
#define GLOW_OPACITY_FORCE          0xFF

struct GlowState
{
    u16 appliedColor;
    u8 spriteId;
    u8 paletteSlot;
    u8 appliedOpacity;  // GLOW_OPACITY_FORCE rewrites the palette
    u8 retryDelay;      // frames until a failed re-creation is retried
};

static EWRAM_DATA struct GlowState sGlow[MAX_OVERLAYS] = {0};

static const u32 sGlowGfx[] = INCGFX_U32("graphics/overworld_overlay/glow.png", ".4bpp");

static const struct SpriteSheet sGlowSpriteSheet = {
    .data = sGlowGfx,
    .size = sizeof(sGlowGfx),
    .tag = OVERLAY_GLOW_TILE_TAG,
};

// The 32x32 glow is drawn at 2x in a 64x64 double-size area, which keeps the sheet at 16 tiles.
static const struct OamData sGlowOam = {
    .affineMode = ST_OAM_AFFINE_DOUBLE,
    .objMode = ST_OAM_OBJ_BLEND,
    .shape = SPRITE_SHAPE(32x32),
    .size = SPRITE_SIZE(32x32),
    .priority = 2,
};

static const union AffineAnimCmd sGlowAffineAnim[] = {
    AFFINEANIMCMD_FRAME(0x200, 0x200, 0, 0),
    AFFINEANIMCMD_END,
};

static const union AffineAnimCmd *const sGlowAffineAnims[] = {
    sGlowAffineAnim,
};

static void SpriteCB_OverlayGlow(struct Sprite *sprite);

static const struct SpriteTemplate sGlowSpriteTemplate = {
    .tileTag = OVERLAY_GLOW_TILE_TAG,
    .paletteTag = TAG_NONE,
    .oam = &sGlowOam,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = sGlowAffineAnims,
    .callback = SpriteCB_OverlayGlow,
};

// Indexed by enum OverlaySpritePosition.
static const u8 sGlowOamPriority[] = {2, 1, 1};
static const u8 sGlowSubpriority[] = {0xFF, 0xFF, 0};

static bool32 GlowSpriteExists(u32 index)
{
    struct Sprite *sprite = &gSprites[sGlow[index].spriteId];

    return sprite->inUse && sprite->callback == SpriteCB_OverlayGlow && sprite->data[0] == index;
}

static bool32 OwnsGlowPalette(u32 index)
{
    return GetSpritePaletteTagByPaletteNum(sGlow[index].paletteSlot) == OVERLAY_GLOW_PAL_TAG_BASE + index;
}

static void FreeGlowSheetIfUnused(void)
{
    u32 i;

    for (i = 0; i < MAX_OVERLAYS; i++)
    {
        if (GlowSpriteExists(i))
            return;
    }

    FreeSpriteTilesByTag(OVERLAY_GLOW_TILE_TAG);
}

static void FreeGlow(u32 index)
{
    if (GlowSpriteExists(index))
        DestroySprite(&gSprites[sGlow[index].spriteId]);

    FreeSpritePaletteByTag(OVERLAY_GLOW_PAL_TAG_BASE + index);
    FreeGlowSheetIfUnused();
    memset(&sGlow[index], 0, sizeof(sGlow[index]));
}

static u16 ScaleColor(u16 color, u32 level)
{
    u32 r = (color & 0x1F) * level / 15;
    u32 g = ((color >> 5) & 0x1F) * level / 15;
    u32 b = ((color >> 10) & 0x1F) * level / 15;

    return RGB(r, g, b);
}

// Palette entry n is the glow colour at n/15 intensity, faded toward black by the opacity.
// Written to the unfaded buffer as well, so screen fades and weather rebuilds keep it.
static void WriteGlowPalette(u32 index, const struct Overlay *overlay)
{
    u16 palette[16];
    u32 offset = OBJ_PLTT_ID(sGlow[index].paletteSlot);
    u32 level;

    palette[0] = 0;
    for (level = 1; level < 16; level++)
        palette[level] = ScaleColor(overlay->color, level);

    BlendPalettesFine(1, palette, &gPlttBufferUnfaded[offset], OVERLAY_OPACITY_MAX - overlay->resolvedOpacity, 0);
    if (!gPaletteFade.active)
        CpuCopy16(&gPlttBufferUnfaded[offset], &gPlttBufferFaded[offset], PLTT_SIZE_4BPP);

    sGlow[index].appliedColor = overlay->color;
    sGlow[index].appliedOpacity = overlay->resolvedOpacity;
}

static bool32 CreateGlow(u32 index, const struct Overlay *overlay)
{
    struct SpritePalette spritePalette;
    u16 palette[16] = {0};
    u32 slot, spriteId;
    struct Sprite *sprite;

    if (GetSpriteTileStartByTag(OVERLAY_GLOW_TILE_TAG) == TAG_NONE)
        LoadSpriteSheet(&sGlowSpriteSheet);
    if (GetSpriteTileStartByTag(OVERLAY_GLOW_TILE_TAG) == TAG_NONE)
        return FALSE;

    spritePalette.data = palette;
    spritePalette.tag = OVERLAY_GLOW_PAL_TAG_BASE + index;
    slot = LoadSpritePalette(&spritePalette);
    if (slot == 0xFF)
    {
        FreeGlowSheetIfUnused();
        return FALSE;
    }

    spriteId = CreateSprite(&sGlowSpriteTemplate, 0, 0, sGlowSubpriority[overlay->spritePosition]);
    if (spriteId == MAX_SPRITES)
    {
        FreeSpritePaletteByTag(spritePalette.tag);
        FreeGlowSheetIfUnused();
        return FALSE;
    }

    sprite = &gSprites[spriteId];
    sprite->oam.paletteNum = slot;
    sprite->coordOffsetEnabled = TRUE;
    sprite->invisible = TRUE; // shown by the callback once it has a position
    sprite->data[0] = index;

    sGlow[index].spriteId = spriteId;
    sGlow[index].paletteSlot = slot;
    sGlow[index].appliedOpacity = GLOW_OPACITY_FORCE;
    sGlow[index].retryDelay = 0;
    return TRUE;
}

// Sprite-space centre of the glow. Object and player anchors use the target's sprite, which
// already includes step progress; a tile anchor is converted from map coordinates.
static bool32 GetGlowCenter(const struct Overlay *overlay, s16 *x, s16 *y)
{
    u32 objectEventId = OBJECT_EVENTS_COUNT;

    if (overlay->anchorKind == OVERLAY_ANCHOR_NONE)
        objectEventId = gPlayerAvatar.objectEventId;
    else if (overlay->anchorKind == OVERLAY_ANCHOR_OBJECT)
        objectEventId = GetObjectEventIdByLocalIdAndMap(overlay->anchorLocalId, overlay->anchorMapNum, overlay->anchorMapGroup);

    if (objectEventId < OBJECT_EVENTS_COUNT && gObjectEvents[objectEventId].active
     && gObjectEvents[objectEventId].spriteId < MAX_SPRITES)
    {
        struct Sprite *target = &gSprites[gObjectEvents[objectEventId].spriteId];

        // Object sprites are placed with their bottom edge on the tile's bottom edge.
        *x = target->x + target->x2;
        *y = target->y + target->y2 - target->centerToCornerVecY - 8;
        return TRUE;
    }

    if (overlay->anchorKind == OVERLAY_ANCHOR_NONE)
        return FALSE;

    SetSpritePosToMapCoords(overlay->anchorX, overlay->anchorY, x, y);
    *x += 8;
    *y += 8;
    return TRUE;
}

static void SpriteCB_OverlayGlow(struct Sprite *sprite)
{
    const struct Overlay *overlay = &sOverlays[sprite->data[0]];
    s16 x, y;

    if (!overlay->active || !overlay->enabled || overlay->resolvedOpacity == 0 || !GetGlowCenter(overlay, &x, &y))
    {
        sprite->invisible = TRUE;
        return;
    }

    sprite->x = x;
    sprite->y = y;
    sprite->oam.priority = sGlowOamPriority[overlay->spritePosition];
    sprite->subpriority = sGlowSubpriority[overlay->spritePosition];
    sprite->invisible = FALSE;
}

// Palette slots that hold glow sprites; palette overlays must not tint them.
static u32 GetGlowPaletteMask(void)
{
    u32 mask = 0;
    u32 i;

    for (i = 0; i < MAX_OVERLAYS; i++)
    {
        if (sOverlays[i].active && sOverlays[i].layer == OVERLAY_LAYER_SPRITE && OwnsGlowPalette(i))
            mask |= 1u << (16 + sGlow[i].paletteSlot);
    }

    return mask;
}

// Re-creates a glow whose sprite or palette was reset (battle, map load), then keeps its palette current.
static void UpdateGlow(u32 index, const struct Overlay *overlay)
{
    if (!GlowSpriteExists(index) || !OwnsGlowPalette(index))
    {
        if (sGlow[index].retryDelay != 0)
        {
            sGlow[index].retryDelay--;
            return;
        }

        FreeGlow(index);
        if (!CreateGlow(index, overlay))
        {
            sGlow[index].retryDelay = GLOW_RETRY_FRAMES;
            return;
        }
    }

    if (sGlow[index].appliedColor != overlay->color || sGlow[index].appliedOpacity != overlay->resolvedOpacity)
        WriteGlowPalette(index, overlay);
}

static u8 NextGeneration(u8 generation)
{
    // Generation 0 is never issued, so it is skipped on wrap.
    return generation == 0xFF ? 1 : generation + 1;
}

// Clears the slot; the bumped generation makes existing handles stale.
static void ReleaseSlot(struct Overlay *overlay)
{
    u8 generation = overlay->generation;

    if (overlay->layer == OVERLAY_LAYER_SPRITE)
        FreeGlow(overlay - sOverlays);

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

// Written to SaveBlock3 so overlays resume with the map they were saved on. Generations are kept,
// so script variables holding a handle stay valid.
void Overlay_SaveToBlock(void)
{
    gSaveBlock3Ptr->overlaySave.magic = OVERLAY_SAVE_MAGIC;
    memcpy(gSaveBlock3Ptr->overlaySave.overlays, sOverlays, sizeof(sOverlays));
}

static bool32 IsSavedOverlayValid(const struct Overlay *saved)
{
    return saved->active && saved->generation != 0
        && saved->baseOpacity <= OVERLAY_OPACITY_MAX
        && saved->currentOpacity <= OVERLAY_OPACITY_MAX
        && saved->resolvedOpacity <= OVERLAY_OPACITY_MAX
        && saved->pulseMax <= OVERLAY_OPACITY_MAX
        && saved->minIntensity <= OVERLAY_OPACITY_MAX
        && saved->maxIntensity <= OVERLAY_OPACITY_MAX
        && saved->spritePosition <= OVERLAY_SPRITE_ABOVE_ALL;
}

// Sprite glows are re-created by Overlay_Update.
void Overlay_LoadFromBlock(void)
{
    const struct OverlaySave *save = &gSaveBlock3Ptr->overlaySave;
    u32 spriteOverlays = 0;
    u32 i;

    Overlay_ResetAll();
    if (save->magic != OVERLAY_SAVE_MAGIC)
        return;

    for (i = 0; i < MAX_OVERLAYS; i++)
    {
        const struct Overlay *saved = &save->overlays[i];

        sOverlays[i].generation = saved->generation;
        if (!IsSavedOverlayValid(saved))
            continue;

        if (saved->layer == OVERLAY_LAYER_SPRITE && ++spriteOverlays > MAX_SPRITE_OVERLAYS)
            continue;

        sOverlays[i] = *saved;
    }

    sDistanceForce = TRUE;
    sOverlayDirty = TRUE;
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

        if (!overlay->active || !overlay->enabled || overlay->resolvedOpacity == 0 || overlay->layer == OVERLAY_LAYER_SPRITE)
            continue;

        for (j = count; j > 0 && order[j - 1]->priority > overlay->priority; j--)
            order[j] = order[j - 1];
        order[j] = overlay;
        count++;
    }

    return count;
}

static bool32 HasRenderedOverlay(void)
{
    struct Overlay *order[MAX_OVERLAYS];

    return GetRenderOrder(order) != 0;
}

// Blends each tinting overlay into the given palettes of gPlttBufferFaded, scaled by
// progress (0-OVERLAY_OPACITY_MAX). Returns the number of overlays considered.
static u32 TintPalettes(u32 palettes, u32 progress)
{
    struct Overlay *order[MAX_OVERLAYS];
    u32 count = GetRenderOrder(order);
    u32 glowMask = GetGlowPaletteMask();
    u32 i;

    for (i = 0; i < count; i++)
    {
        u32 mask = LayerPaletteMask(order[i]->layer) & palettes & ~sEffectiveExempt[order[i] - sOverlays] & ~glowMask;
        u32 opacity = (order[i]->resolvedOpacity * progress + OVERLAY_OPACITY_MAX / 2) / OVERLAY_OPACITY_MAX;

        if (opacity != 0)
            BlendPalettesFine(mask, gPlttBufferFaded, gPlttBufferFaded, opacity, order[i]->color);
    }

    return count;
}

// The palette fade and the weather fade-in own gPlttBufferFaded and overwrite it. A finished
// fade-out leaves the buffer black until the next fade-in, so it must not be recomposed either.
static bool32 IsPaletteBufferBusy(void)
{
    return gPaletteFade.active || !IsWeatherNotFadingIn() || IsWeatherFadingOut();
}

void Overlay_OnPalettesRebuilt(u32 palettes)
{
    u32 count;

    if (IsPaletteBufferBusy())
    {
        sOverlayDirty = TRUE;
        return;
    }

    count = TintPalettes(palettes, OVERLAY_OPACITY_MAX);
    if (palettes == PALETTES_ALL)
    {
        sOverlayDirty = FALSE;
        sOverlayApplied = (count != 0);
    }
}

void Overlay_ApplyFadeInStep(u32 palettes, u32 y)
{
    if (IsWeatherNotFadingIn())
        return;

    TintPalettes(palettes, OVERLAY_OPACITY_MAX - min(y, OVERLAY_OPACITY_MAX));
    sOverlayDirty = TRUE; // recomposed once the fade ends
}

static void ApplyOverlaysToPalettes(void)
{
    bool8 transferDisabled;

    if (IsPaletteBufferBusy())
    {
        sOverlayDirty = TRUE;
        return;
    }

    if (!sOverlayDirty)
        return;

    if (!sOverlayApplied && !HasRenderedOverlay())
    {
        sOverlayDirty = FALSE;
        return;
    }

    // Rebuilds from gPlttBufferUnfaded, which overlays never modify, and tints through
    // Overlay_OnPalettesRebuilt. VBlank must not copy the buffer while it is untinted.
    transferDisabled = gPaletteFade.bufferTransferDisabled;
    gPaletteFade.bufferTransferDisabled = TRUE;
    ApplyWeatherColorMapToPals(0, 32);
    gPaletteFade.bufferTransferDisabled = transferDisabled;
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

bool32 OverworldAnchor_Resolve(u8 localId, u8 mapNum, u8 mapGroup, s16 *x, s16 *y)
{
    u32 objectEventId = GetObjectEventIdByLocalIdAndMap(localId, mapNum, mapGroup);
    s16 objectX, objectY;

    if (objectEventId == OBJECT_EVENTS_COUNT)
        return FALSE;

    objectX = gObjectEvents[objectEventId].currentCoords.x;
    objectY = gObjectEvents[objectEventId].currentCoords.y;
    if (*x == objectX && *y == objectY)
        return FALSE;

    *x = objectX;
    *y = objectY;
    return TRUE;
}

// Octagonal distance in tiles, no square root.
u32 OverworldAnchor_Distance(s16 anchorX, s16 anchorY, const struct Coords16 *playerCoords)
{
    s32 dx = playerCoords->x - anchorX;
    s32 dy = playerCoords->y - anchorY;

    if (dx < 0)
        dx = -dx;
    if (dy < 0)
        dy = -dy;

    return max(dx, dy) + min(dx, dy) / 2;
}

u32 OverworldAnchor_DistanceFactor(u32 distance, u8 innerRadius, u8 outerRadius, u8 minIntensity, u8 maxIntensity)
{
    if (distance <= innerRadius)
        return maxIntensity;
    if (distance >= outerRadius)
        return minIntensity;

    return minIntensity + ((s32)maxIntensity - minIntensity) * (s32)(outerRadius - distance) / (s32)(outerRadius - innerRadius);
}

// Refreshes the anchor from the tracked object. An unresolved object keeps the last known position.
// Returns TRUE when the anchor moved.
static bool32 UpdateAnchor(struct Overlay *overlay)
{
    if (overlay->anchorKind != OVERLAY_ANCHOR_OBJECT)
        return FALSE;

    return OverworldAnchor_Resolve(overlay->anchorLocalId, overlay->anchorMapNum, overlay->anchorMapGroup,
                                   &overlay->anchorX, &overlay->anchorY);
}

static u32 CalcDistanceFactor(const struct Overlay *overlay, const struct Coords16 *playerCoords)
{
    u32 distance = OverworldAnchor_Distance(overlay->anchorX, overlay->anchorY, playerCoords);

    return OverworldAnchor_DistanceFactor(distance, overlay->innerRadius, overlay->outerRadius,
                                          overlay->minIntensity, overlay->maxIntensity);
}

void Overlay_Update(void)
{
    struct Coords16 playerCoords = {0};
    bool32 playerRead = FALSE;
    bool32 refreshDistance = FALSE;
    u32 i;

    for (i = 0; i < MAX_OVERLAYS; i++)
    {
        struct Overlay *overlay = &sOverlays[i];
        u32 pulseFactor, distanceFactor, resolved, exempt;
        bool32 anchorMoved;

        if (!overlay->active)
            continue;

        if (!UpdateFade(overlay))
            continue;

        pulseFactor = UpdatePulse(overlay);
        anchorMoved = UpdateAnchor(overlay);

        distanceFactor = OVERLAY_OPACITY_MAX;
        if (overlay->falloffEnabled && overlay->anchorKind != OVERLAY_ANCHOR_NONE)
        {
            // The player is read once per frame; distance is recomputed on a player step,
            // an anchor move, a falloff or anchor change, or every 8th frame.
            if (!playerRead)
            {
                struct Coords16 lastCoords = sLastPlayerCoords;

                playerCoords = gObjectEvents[gPlayerAvatar.objectEventId].currentCoords;
                sLastPlayerCoords = playerCoords;
                sFalloffFrames = (sFalloffFrames + 1) & 7;
                refreshDistance = sDistanceForce || sFalloffFrames == 0
                    || lastCoords.x != playerCoords.x || lastCoords.y != playerCoords.y;
                playerRead = TRUE;
            }

            if (refreshDistance || anchorMoved)
                sDistanceFactor[i] = CalcDistanceFactor(overlay, &playerCoords);
            distanceFactor = sDistanceFactor[i];
        }

        resolved = ResolveOpacity(overlay->currentOpacity, pulseFactor, distanceFactor);
        if (overlay->resolvedOpacity != resolved)
        {
            overlay->resolvedOpacity = resolved;
            sOverlayDirty = TRUE;
        }

        if (overlay->layer == OVERLAY_LAYER_SPRITE)
            UpdateGlow(i, overlay);

        // Sprite palette slots are reallocated on map load, so they are re-read every frame.
        exempt = ResolveExemptPalettes(overlay);
        if (sEffectiveExempt[i] != exempt)
        {
            sEffectiveExempt[i] = exempt;
            sOverlayDirty = TRUE;
        }
    }

    sDistanceForce = FALSE;
    ApplyOverlaysToPalettes();
}

void Overlay_Invalidate(void)
{
    sOverlayDirty = TRUE;
}

OverlayId Overlay_Create(const struct OverlayConfig *config)
{
    u32 i, spriteOverlays = 0;

    if (config->layer == OVERLAY_LAYER_SPRITE)
    {
        for (i = 0; i < MAX_OVERLAYS; i++)
        {
            if (sOverlays[i].active && sOverlays[i].layer == OVERLAY_LAYER_SPRITE)
                spriteOverlays++;
        }

        if (spriteOverlays >= MAX_SPRITE_OVERLAYS)
            return OVERLAY_ID_INVALID;
    }

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
        overlay->spritePosition = min(config->spritePosition, OVERLAY_SPRITE_ABOVE_ALL);
        overlay->baseOpacity = opacity;
        overlay->currentOpacity = opacity;
        overlay->resolvedOpacity = opacity;
        sOverlayDirty = TRUE;

        if (config->layer == OVERLAY_LAYER_SPRITE && !CreateGlow(i, overlay))
        {
            ReleaseSlot(overlay);
            return OVERLAY_ID_INVALID;
        }

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
    sDistanceForce = TRUE;
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
    sDistanceForce = TRUE;
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

void Overlay_SetFalloff(OverlayId id, u8 innerRadius, u8 outerRadius, u8 minIntensity, u8 maxIntensity)
{
    struct Overlay *overlay = GetOverlay(id);

    if (overlay == NULL)
        return;

    overlay->innerRadius = innerRadius;
    overlay->outerRadius = outerRadius;
    overlay->minIntensity = min(minIntensity, OVERLAY_OPACITY_MAX);
    overlay->maxIntensity = min(maxIntensity, OVERLAY_OPACITY_MAX);
    overlay->falloffEnabled = TRUE;
    sDistanceForce = TRUE;
}

void Overlay_ClearFalloff(OverlayId id)
{
    struct Overlay *overlay = GetOverlay(id);

    if (overlay == NULL)
        return;

    overlay->innerRadius = 0;
    overlay->outerRadius = 0;
    overlay->minIntensity = 0;
    overlay->maxIntensity = 0;
    overlay->falloffEnabled = FALSE;
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

    // The sprite layer owns resources that are only set up at creation.
    if ((layer == OVERLAY_LAYER_SPRITE) != (overlay->layer == OVERLAY_LAYER_SPRITE))
        return;

    overlay->layer = layer;
    sOverlayDirty = TRUE;
}

void Overlay_SetSpritePosition(OverlayId id, u8 position)
{
    struct Overlay *overlay = GetOverlay(id);

    if (overlay == NULL)
        return;

    overlay->spritePosition = min(position, OVERLAY_SPRITE_ABOVE_ALL);
}
