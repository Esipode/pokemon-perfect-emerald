#include "global.h"
#include "screen_effects.h"
#include "event_object_movement.h"
#include "field_camera.h"
#include "fieldmap.h"
#include "overworld_overlay.h"
#include "palette.h"
#include "scanline_effect.h"
#include "sprite.h"
#include "trig.h"
#include "constants/rgb.h"

#define LIFETIME_FADE_MAX       30 // frames
#define SHAKE_DEFAULT_PERIOD    32
#define SHAKE_SCALE             (256 * SCREENFX_INTENSITY_MAX)  // Q8.8 sine times intensity
#define WAVE_DEFAULT_PERIOD     90
#define WAVE_DEFAULT_WAVELENGTH 64
#define WAVE_MIN_WAVELENGTH     8

#define TEAR_DEFAULT_HEIGHT     24
#define TEAR_CENTER_AUTO        0x7FFF  // stored centre meaning "follow the anchor"; real values are below 160 * 16

#define VIGNETTE_TILE_TAG_BASE  0x8030  // + VIGNETTE_ROLE_*
#define VIGNETTE_PAL_TAG        0x8030  // bit 15 makes the palette weather-immune
#define VIGNETTE_NEUTRAL        9       // grey (0-31) at which the blend leaves a mid-tone scene unchanged
#define VIGNETTE_LEVELS         3       // opaque palette entries 1-3; 3 is the darkest
#define VIGNETTE_MAX_SPRITES    14
#define VIGNETTE_RETRY_FRAMES   30
#define VIGNETTE_FORCE          0xFF    // applied intensity that forces a palette rewrite
#define VIGNETTE_FLIP_H         1
#define VIGNETTE_FLIP_V         2

#define MAX_RIPPLES             3
#define RIPPLE_HALF_WIDTH       32  // scanlines each side of the front; power of two keeps the scaling a shift
#define RIPPLE_STEP             (0x10000 / RIPPLE_HALF_WIDTH) // phase per scanline: two cycles across the band
#define RIPPLE_DEFAULT_SPEED    32
#define RIPPLE_DEFAULT_DURATION 60
#define RIPPLE_SCALE            (256 * 256 * RIPPLE_HALF_WIDTH) // Q8.8 sine, Q8 amplitude, envelope in scanlines

#define PRESET_MAX_EFFECTS      3
#define PRESET_BURST_FLASH_HOLD 2   // frames the flash stays at full opacity
#define PRESET_BURST_FLASH_FADE 20
#define PRESET_BURST_SHAKE_FADE 40
#define PRESET_BURST_SETTLE     60
#define PRESET_BURST_RIPPLE_LIFETIME 64 // ripple lasts 60 frames from frame 2
#define PRESET_TINT_MAX         OVERLAY_OPACITY_MAX

#define SCANLINE_COUNT          DISPLAY_HEIGHT
#define SCANLINE_REGS           6   // BG1HOFS, BG1VOFS, BG2HOFS, BG2VOFS, BG3HOFS, BG3VOFS
#define SCANLINE_OFFSET_MAX     8   // pixels; larger offsets pull undrawn columns in at map edges
#define SCANLINE_DMA_CONTROL    (((DMA_ENABLE | DMA_START_HBLANK | DMA_REPEAT | DMA_SRC_INC | DMA_DEST_INC | DMA_16BIT | DMA_DEST_RELOAD) << 16) | SCANLINE_REGS)

STATIC_ASSERT(MAX_SCREEN_EFFECTS <= 8, ScreenFxIndexFitsHandle);
STATIC_ASSERT(MAX_SCREENFX_PRESETS <= 8, ScreenFxPresetIndexFitsHandle);
STATIC_ASSERT(sizeof(struct ScreenFx) <= 40, ScreenFxSizeBudget);
STATIC_ASSERT(SCANLINE_COUNT * SCANLINE_REGS == ARRAY_COUNT(gScanlineEffectRegBuffers[0]), ScanlineBufferFit);

struct Ripple
{
    s16 centerLine;         // screen line
    u16 speed;              // sixteenths of a scanline per frame
    u16 duration;           // frames
    u16 elapsed;
    u8 amplitude;           // 0-SCREENFX_INTENSITY_MAX
    u8 active;
};

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

// One started preset. effects[] is indexed by member role; the overlay is the tint (the flash for a burst).
struct PresetRecord
{
    ScreenFxId effects[PRESET_MAX_EFFECTS];
    OverlayId overlay;
    u16 frame;              // frames since start, stops counting once the burst timeline is done
    u8 generation;          // bumped on release, invalidates stale handles
    u8 active:1;
    u8 preset;              // enum ScreenFxPreset
    u8 stage;               // enum ScreenFxStage
    u8 intensity;           // level at SCREENFX_STAGE_DRAMATIC
};

static EWRAM_DATA struct ScreenFx sScreenFx[MAX_SCREEN_EFFECTS] = {0};
static EWRAM_DATA struct PresetRecord sPresets[MAX_SCREENFX_PRESETS] = {0};
static EWRAM_DATA struct ScanlineChannel sChannel = {0};
static EWRAM_DATA struct Ripple sRipples[MAX_RIPPLES] = {0}; // owned by the one SCREENFX_RIPPLE effect
// Summed per-line offsets of every geometry effect; clamped when the buffer is built.
static EWRAM_DATA s16 sLineDx[SCANLINE_COUNT] = {0};
static EWRAM_DATA s16 sLineDy[SCANLINE_COUNT] = {0};

// Vignette: 14 screen-fixed blend-mode sprites built from three sheets. The art holds the darkness
// gradient (entry 1-3 = light to dark, dithered); the palette holds the intensity.

enum VignetteRole
{
    VIGNETTE_ROLE_CORNER,   // 64x32, top-left of the screen
    VIGNETTE_ROLE_EDGE,     // 64x32, top edge away from the corners
    VIGNETTE_ROLE_SIDE,     // 32x64, left edge away from the corners
    VIGNETTE_ROLE_COUNT,
};

struct VignetteSpriteSpec
{
    s16 x, y;               // top-left corner
    u8 role;
    u8 flip;                // VIGNETTE_FLIP_*
};

struct VignetteState
{
    u8 spriteIds[VIGNETTE_MAX_SPRITES];
    u8 spriteCount;         // 0 = nothing created
    u8 paletteSlot;
    u8 appliedIntensity;
    u8 retryDelay;          // frames until a failed re-creation is retried
};

static EWRAM_DATA struct VignetteState sVignette = {0}; // owned by the one SCREENFX_VIGNETTE effect

static const u32 sVignetteCornerGfx[] = INCGFX_U32("graphics/screen_effects/vignette_corner.png", ".4bpp");
static const u32 sVignetteEdgeGfx[] = INCGFX_U32("graphics/screen_effects/vignette_edge.png", ".4bpp");
static const u32 sVignetteSideGfx[] = INCGFX_U32("graphics/screen_effects/vignette_side.png", ".4bpp");

static const struct SpriteSheet sVignetteSheets[VIGNETTE_ROLE_COUNT] = {
    [VIGNETTE_ROLE_CORNER] = { .data = sVignetteCornerGfx, .size = sizeof(sVignetteCornerGfx), .tag = VIGNETTE_TILE_TAG_BASE + VIGNETTE_ROLE_CORNER },
    [VIGNETTE_ROLE_EDGE]   = { .data = sVignetteEdgeGfx,   .size = sizeof(sVignetteEdgeGfx),   .tag = VIGNETTE_TILE_TAG_BASE + VIGNETTE_ROLE_EDGE },
    [VIGNETTE_ROLE_SIDE]   = { .data = sVignetteSideGfx,   .size = sizeof(sVignetteSideGfx),   .tag = VIGNETTE_TILE_TAG_BASE + VIGNETTE_ROLE_SIDE },
};

// OBJ priority 1 keeps the sprites under BG0, which is not a blend target.
static const struct OamData sVignetteOam[VIGNETTE_ROLE_COUNT] = {
    [VIGNETTE_ROLE_CORNER] = { .objMode = ST_OAM_OBJ_BLEND, .shape = SPRITE_SHAPE(64x32), .size = SPRITE_SIZE(64x32), .priority = 1 },
    [VIGNETTE_ROLE_EDGE]   = { .objMode = ST_OAM_OBJ_BLEND, .shape = SPRITE_SHAPE(64x32), .size = SPRITE_SIZE(64x32), .priority = 1 },
    [VIGNETTE_ROLE_SIDE]   = { .objMode = ST_OAM_OBJ_BLEND, .shape = SPRITE_SHAPE(32x64), .size = SPRITE_SIZE(32x64), .priority = 1 },
};

static void SpriteCB_Vignette(struct Sprite *sprite)
{
}

#define VIGNETTE_TEMPLATE(role) {                       \
    .tileTag = VIGNETTE_TILE_TAG_BASE + (role),         \
    .paletteTag = TAG_NONE,                             \
    .oam = &sVignetteOam[role],                         \
    .anims = gDummySpriteAnimTable,                     \
    .images = NULL,                                     \
    .affineAnims = gDummySpriteAffineAnimTable,         \
    .callback = SpriteCB_Vignette,                      \
}

static const struct SpriteTemplate sVignetteTemplates[VIGNETTE_ROLE_COUNT] = {
    [VIGNETTE_ROLE_CORNER] = VIGNETTE_TEMPLATE(VIGNETTE_ROLE_CORNER),
    [VIGNETTE_ROLE_EDGE]   = VIGNETTE_TEMPLATE(VIGNETTE_ROLE_EDGE),
    [VIGNETTE_ROLE_SIDE]   = VIGNETTE_TEMPLATE(VIGNETTE_ROLE_SIDE),
};

// Pixels the sprites sit outside the screen, per SCREENFX_VIGNETTE_* preset. At most 16, beyond which
// the side sprites no longer meet the top and bottom strips.
static const u8 sVignetteOutset[] = {
    [SCREENFX_VIGNETTE_WIDE] = 16,
    [SCREENFX_VIGNETTE_MEDIUM] = 8,
    [SCREENFX_VIGNETTE_TIGHT] = 0,
};

static bool32 VignetteSpriteExists(u32 index)
{
    struct Sprite *sprite = &gSprites[sVignette.spriteIds[index]];

    return sprite->inUse && sprite->callback == SpriteCB_Vignette;
}

static bool32 OwnsVignettePalette(void)
{
    return GetSpritePaletteTagByPaletteNum(sVignette.paletteSlot) == VIGNETTE_PAL_TAG;
}

static bool32 IsVignetteIntact(void)
{
    u32 i;

    if (sVignette.spriteCount == 0 || !OwnsVignettePalette())
        return FALSE;

    for (i = 0; i < sVignette.spriteCount; i++)
    {
        if (!VignetteSpriteExists(i))
            return FALSE;
    }

    return TRUE;
}

static void FreeVignette(void)
{
    u32 i;

    for (i = 0; i < sVignette.spriteCount; i++)
    {
        if (VignetteSpriteExists(i))
            DestroySprite(&gSprites[sVignette.spriteIds[i]]);
    }

    FreeSpritePaletteByTag(VIGNETTE_PAL_TAG);
    for (i = 0; i < VIGNETTE_ROLE_COUNT; i++)
        FreeSpriteTilesByTag(VIGNETTE_TILE_TAG_BASE + i);

    memset(&sVignette, 0, sizeof(sVignette));
}

// The top and bottom strips are a corner sprite at each end and three edge sprites between, which
// overlap by identical art; the side sprites overlap the strips the same way.
static u32 BuildVignetteLayout(u32 preset, struct VignetteSpriteSpec *specs)
{
    s32 outset = sVignetteOutset[min(preset, SCREENFX_VIGNETTE_TIGHT)];
    s32 left = -outset;
    s32 right = DISPLAY_WIDTH + outset;
    s32 top = -outset;
    s32 bottom = DISPLAY_HEIGHT + outset;
    s32 edgeFirst = 64 - outset;
    s32 edgeLast = right - 128;
    u32 count = 0;
    u32 strip, i;

    for (strip = 0; strip < 2; strip++)
    {
        s32 y = strip == 0 ? top : bottom - 32;
        u8 flipV = strip == 0 ? 0 : VIGNETTE_FLIP_V;
        s32 edgeX[] = {edgeFirst, (edgeFirst + edgeLast) / 2, edgeLast};

        specs[count++] = (struct VignetteSpriteSpec){left, y, VIGNETTE_ROLE_CORNER, flipV};
        specs[count++] = (struct VignetteSpriteSpec){right - 64, y, VIGNETTE_ROLE_CORNER, flipV | VIGNETTE_FLIP_H};
        for (i = 0; i < ARRAY_COUNT(edgeX); i++)
            specs[count++] = (struct VignetteSpriteSpec){edgeX[i], y, VIGNETTE_ROLE_EDGE, flipV};
    }

    for (i = 0; i < 2; i++)
    {
        s32 y = i == 0 ? top + 32 : bottom - 96;

        specs[count++] = (struct VignetteSpriteSpec){left, y, VIGNETTE_ROLE_SIDE, 0};
        specs[count++] = (struct VignetteSpriteSpec){right - 32, y, VIGNETTE_ROLE_SIDE, VIGNETTE_FLIP_H};
    }

    return count;
}

static bool32 CreateVignette(u32 preset)
{
    struct VignetteSpriteSpec specs[VIGNETTE_MAX_SPRITES];
    struct SpritePalette spritePalette;
    u16 palette[16] = {0};
    u32 slot, count, i;

    for (i = 0; i < VIGNETTE_ROLE_COUNT; i++)
    {
        if (GetSpriteTileStartByTag(VIGNETTE_TILE_TAG_BASE + i) == TAG_NONE)
            LoadSpriteSheet(&sVignetteSheets[i]);
        if (GetSpriteTileStartByTag(VIGNETTE_TILE_TAG_BASE + i) == TAG_NONE)
        {
            FreeVignette();
            return FALSE;
        }
    }

    spritePalette.data = palette;
    spritePalette.tag = VIGNETTE_PAL_TAG;
    slot = LoadSpritePalette(&spritePalette);
    if (slot == 0xFF)
    {
        FreeVignette();
        return FALSE;
    }

    sVignette.paletteSlot = slot;
    sVignette.appliedIntensity = VIGNETTE_FORCE;

    count = BuildVignetteLayout(preset, specs);
    for (i = 0; i < count; i++)
    {
        const struct VignetteSpriteSpec *spec = &specs[i];
        u32 width = spec->role == VIGNETTE_ROLE_SIDE ? 32 : 64;
        u32 height = spec->role == VIGNETTE_ROLE_SIDE ? 64 : 32;
        u32 spriteId = CreateSprite(&sVignetteTemplates[spec->role], spec->x + width / 2, spec->y + height / 2, 0);
        struct Sprite *sprite;

        if (spriteId == MAX_SPRITES)
        {
            FreeVignette();
            return FALSE;
        }

        sprite = &gSprites[spriteId];
        sprite->oam.paletteNum = slot;
        sprite->coordOffsetEnabled = FALSE;
        sprite->invisible = TRUE; // shown by UpdateVignette once the palette is written
        sprite->hFlip = (spec->flip & VIGNETTE_FLIP_H) != 0;
        sprite->vFlip = (spec->flip & VIGNETTE_FLIP_V) != 0;
        SetSpriteOamFlipBits(sprite, 0, 0);

        sVignette.spriteIds[i] = spriteId;
        sVignette.spriteCount = i + 1;
    }

    sVignette.retryDelay = 0;
    return TRUE;
}

// Entry n fades from neutral grey toward black as intensity rises, more so for darker entries.
// Written to the unfaded buffer as well, so screen fades and weather rebuilds keep it.
static void WriteVignettePalette(u32 intensity)
{
    u16 palette[16] = {0};
    u32 offset = OBJ_PLTT_ID(sVignette.paletteSlot);
    u32 level;

    for (level = 1; level <= VIGNETTE_LEVELS; level++)
    {
        u32 grey = VIGNETTE_NEUTRAL * (VIGNETTE_LEVELS * SCREENFX_INTENSITY_MAX - level * intensity)
                 / (VIGNETTE_LEVELS * SCREENFX_INTENSITY_MAX);

        palette[level] = RGB(grey, grey, grey);
    }

    CpuCopy16(palette, &gPlttBufferUnfaded[offset], PLTT_SIZE_4BPP);
    if (!gPaletteFade.active)
        CpuCopy16(palette, &gPlttBufferFaded[offset], PLTT_SIZE_4BPP);

    sVignette.appliedIntensity = intensity;
}

// Rebuilds a vignette whose sprites or palette were reset (battle, full-screen menu), then keeps the
// palette and visibility current.
static void UpdateVignette(const struct ScreenFx *effect)
{
    u32 i;

    if (!IsVignetteIntact())
    {
        if (sVignette.retryDelay != 0)
        {
            sVignette.retryDelay--;
            return;
        }

        FreeVignette();
        if (!CreateVignette(effect->params.config.param1))
        {
            sVignette.retryDelay = VIGNETTE_RETRY_FRAMES;
            return;
        }
    }

    if (sVignette.appliedIntensity != effect->resolvedIntensity)
        WriteVignettePalette(effect->resolvedIntensity);

    for (i = 0; i < sVignette.spriteCount; i++)
        gSprites[sVignette.spriteIds[i]].invisible = effect->resolvedIntensity == 0;
}

u32 ScreenFx_GetVignettePaletteMask(void)
{
    if (sVignette.spriteCount == 0 || !OwnsVignettePalette())
        return 0;

    return 1u << (16 + sVignette.paletteSlot);
}

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
    else if (effect->kind == SCREENFX_RIPPLE)
        memset(sRipples, 0, sizeof(sRipples));
    else if (effect->kind == SCREENFX_VIGNETTE)
        FreeVignette();

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

static bool32 IsKindActive(u32 kind)
{
    u32 i;

    for (i = 0; i < MAX_SCREEN_EFFECTS; i++)
    {
        if (sScreenFx[i].active && sScreenFx[i].kind == kind)
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
static void ScanlineChannel_Line(u32 line, s16 dx, s16 dy)
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

static void ReleaseAllPresets(void);

static void ReleaseAllEffects(void)
{
    u32 i;

    ReleaseAllPresets();

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
static s32 ScaleSine(s32 sine, s32 amplitude)
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
        x = ScaleSine(gSineTable[index], amplitude);
    if (effect->params.shake.axes & SCREENFX_AXIS_Y)
        y = ScaleSine(gSineTable[index + 64], amplitude); // cosine

    SetCameraPanningCallback(NULL);
    SetCameraPanning(x, y);
    effect->params.shake.phase += 0x10000 / effect->params.shake.period;
}

// Adds this wave's per-line offsets to the scanline accumulator.
static void UpdateWave(struct ScreenFx *effect)
{
    s32 amplitude = effect->resolvedIntensity * SCREENFX_WAVE_MAX_AMPLITUDE;
    u16 linePhase = effect->params.wave.phase;
    u32 line;

    effect->params.wave.phase += 0x10000 / effect->params.wave.period;

    if (amplitude == 0 || !sChannel.acquired)
        return;

    for (line = 0; line < SCANLINE_COUNT; line++)
    {
        u32 index = linePhase >> 8; // gSineTable holds one cycle per 256 entries
        s16 dy = 0;

        if (effect->params.wave.vertical)
            dy = ScaleSine(gSineTable[(index + 64) & 0xFF], amplitude / 2);

        ScanlineChannel_Line(line, ScaleSine(gSineTable[index], amplitude), dy);
        linePhase += effect->params.wave.step;
    }
}

// Screen line of the anchor tile's centre, or the middle of the screen without an anchor.
static s16 GetAnchorScreenLine(const struct ScreenFx *effect)
{
    s16 x, y;

    if (effect->anchorKind == SCREENFX_ANCHOR_NONE)
        return DISPLAY_HEIGHT / 2;

    SetSpritePosToMapCoords(effect->anchorX, effect->anchorY, &x, &y);
    return y + 8 + gSpriteCoordOffsetY;
}

// Rounds to the nearest pixel, symmetric around zero.
static s32 ScaleRipple(s32 sine, s32 amplitude, s32 envelope)
{
    s32 scaled = sine * amplitude * envelope;

    return (scaled + (scaled < 0 ? -RIPPLE_SCALE / 2 : RIPPLE_SCALE / 2)) / RIPPLE_SCALE;
}

// Adds the band of every live ripple to the scanline accumulator. For each distance d from the centre
// line the offset applies to both line centre - d and centre + d.
static void UpdateRipple(const struct ScreenFx *effect)
{
    u32 i;

    for (i = 0; i < MAX_RIPPLES; i++)
    {
        struct Ripple *ripple = &sRipples[i];
        s32 radius, amplitude, d, last;
        s32 farthest;

        if (!ripple->active)
            continue;

        radius = (s32)ripple->speed * ripple->elapsed / 16;
        farthest = max(abs(ripple->centerLine), abs(DISPLAY_HEIGHT - 1 - ripple->centerLine));
        if (ripple->elapsed >= ripple->duration || radius - RIPPLE_HALF_WIDTH >= farthest)
        {
            ripple->active = FALSE;
            continue;
        }

        amplitude = ripple->amplitude * effect->resolvedIntensity * SCREENFX_RIPPLE_MAX_AMPLITUDE
                  * (ripple->duration - ripple->elapsed) / ripple->duration;
        ripple->elapsed++;
        if (amplitude == 0 || !sChannel.acquired)
            continue;

        d = radius - RIPPLE_HALF_WIDTH + 1;
        if (d < 0)
            d = 0;
        last = radius + RIPPLE_HALF_WIDTH - 1;

        for (; d <= last; d++)
        {
            s32 offset = d - radius;
            s16 dx = ScaleRipple(gSineTable[((offset * RIPPLE_STEP) & 0xFFFF) >> 8],
                                 amplitude, RIPPLE_HALF_WIDTH - abs(offset));
            s32 above = ripple->centerLine - d;
            s32 below = ripple->centerLine + d;

            if (dx == 0)
                continue;

            if (above >= 0 && above < SCANLINE_COUNT)
                ScanlineChannel_Line(above, dx, 0);
            if (d != 0 && below >= 0 && below < SCANLINE_COUNT)
                ScanlineChannel_Line(below, dx, 0);
        }
    }
}

// Offsets a band of scanlines by an alternating +/- amplitude. The amplitude is re-rolled from a
// fixed table at an interval that shortens as intensity rises.
static void UpdateTear(struct ScreenFx *effect)
{
    static const u8 sTearAmplitudes[] = {5, 8, 3, 7, 4, 8, 6, 2}; // pixels at SCREENFX_INTENSITY_MAX
    struct TearParams *tear = &effect->params.tear;
    s32 amplitude, first, last, line;

    if (tear->timer != 0)
    {
        tear->timer--;
    }
    else
    {
        tear->roll = (tear->roll + 3) & (ARRAY_COUNT(sTearAmplitudes) - 1); // 3 is coprime with the table size
        tear->timer = 4 + SCREENFX_INTENSITY_MAX - effect->resolvedIntensity - 1;
    }

    if (tear->center == TEAR_CENTER_AUTO)
    {
        first = GetAnchorScreenLine(effect);
    }
    else
    {
        if (tear->drift != 0)
        {
            tear->center += tear->drift;
            if (tear->center < 0)
                tear->center += DISPLAY_HEIGHT * 16;
            else if (tear->center >= DISPLAY_HEIGHT * 16)
                tear->center -= DISPLAY_HEIGHT * 16;
        }
        first = tear->center >> 4;
    }

    amplitude = (sTearAmplitudes[tear->roll] * effect->resolvedIntensity + SCREENFX_INTENSITY_MAX / 2) / SCREENFX_INTENSITY_MAX;
    if (amplitude == 0 || !sChannel.acquired)
        return;

    first -= tear->height / 2;
    last = first + tear->height - 1;
    first = max(first, 0);
    last = min(last, DISPLAY_HEIGHT - 1);

    for (line = first; line <= last; line++)
        ScanlineChannel_Line(line, (line & 1) ? amplitude : -amplitude, 0);
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

static void UpdatePresets(void);

// Order: presets, then per effect: fade, lifetime, anchor, falloff, final intensity, render.
void ScreenFx_Update(void)
{
    struct Coords16 playerCoords = {0};
    bool32 playerRead = FALSE;
    u32 i;

    UpdatePresets();

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
        else if (effect->kind == SCREENFX_WAVE)
            UpdateWave(effect);
        else if (effect->kind == SCREENFX_RIPPLE)
            UpdateRipple(effect);
        else if (effect->kind == SCREENFX_TEAR)
            UpdateTear(effect);
        else if (effect->kind == SCREENFX_VIGNETTE)
            UpdateVignette(effect);
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

    // Shake, ripple and vignette each have one shared backing store.
    if ((config->kind == SCREENFX_SHAKE || config->kind == SCREENFX_RIPPLE || config->kind == SCREENFX_VIGNETTE)
     && IsKindActive(config->kind))
        return SCREENFX_ID_INVALID;

    for (i = 0; i < MAX_SCREEN_EFFECTS; i++)
    {
        struct ScreenFx *effect = &sScreenFx[i];
        u8 intensity = min(config->intensity, SCREENFX_INTENSITY_MAX);
        u8 generation = effect->generation;

        if (effect->active)
            continue;

        if (config->kind == SCREENFX_VIGNETTE && !CreateVignette(config->param1))
            return SCREENFX_ID_INVALID;

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
        else if (config->kind == SCREENFX_WAVE)
        {
            u16 wavelength = config->param2 & ~SCREENFX_WAVE_VERTICAL;

            if (wavelength == 0)
                wavelength = WAVE_DEFAULT_WAVELENGTH;

            effect->params.wave.period = config->param1 != 0 ? config->param1 : WAVE_DEFAULT_PERIOD;
            effect->params.wave.step = 0x10000 / max(wavelength, WAVE_MIN_WAVELENGTH);
            effect->params.wave.phase = 0;
            effect->params.wave.vertical = (config->param2 & SCREENFX_WAVE_VERTICAL) != 0;
        }
        else if (config->kind == SCREENFX_TEAR)
        {
            effect->params.tear.height = config->param1 != 0 ? min(config->param1, DISPLAY_HEIGHT) : TEAR_DEFAULT_HEIGHT;
            effect->params.tear.center = config->param2 == SCREENFX_TEAR_CENTER_AUTO
                                       ? TEAR_CENTER_AUTO
                                       : min(config->param2, DISPLAY_HEIGHT - 1) * 16;
            effect->params.tear.drift = 0;
            effect->params.tear.roll = 0;
            effect->params.tear.timer = 0;
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

void ScreenFx_SetTearDrift(ScreenFxId id, s16 drift)
{
    struct ScreenFx *effect = GetScreenFx(id);

    if (effect == NULL || effect->kind != SCREENFX_TEAR)
        return;

    effect->params.tear.drift = drift;
}

bool32 ScreenFx_TriggerRipple(ScreenFxId id, s16 screenCenterY, u8 amplitude, u16 speed, u16 durationFrames)
{
    struct ScreenFx *effect = GetScreenFx(id);
    struct Ripple *ripple = &sRipples[0];
    u32 i;

    if (effect == NULL || effect->kind != SCREENFX_RIPPLE)
        return FALSE;

    // A free entry, else the one furthest along.
    for (i = 0; i < MAX_RIPPLES; i++)
    {
        if (!sRipples[i].active)
        {
            ripple = &sRipples[i];
            break;
        }
        if (sRipples[i].elapsed > ripple->elapsed)
            ripple = &sRipples[i];
    }

    ripple->centerLine = screenCenterY == SCREENFX_RIPPLE_CENTER_AUTO ? GetAnchorScreenLine(effect) : screenCenterY;
    ripple->amplitude = min(amplitude, SCREENFX_INTENSITY_MAX);
    ripple->speed = speed != 0 ? speed : RIPPLE_DEFAULT_SPEED;
    ripple->duration = durationFrames != 0 ? durationFrames : RIPPLE_DEFAULT_DURATION;
    ripple->elapsed = 0;
    ripple->active = TRUE;
    return TRUE;
}

// Presets

struct PresetSpec
{
    u16 tint;               // overlay colour
    u8 effectCount;         // members in effects[]
    u8 falloffInner;        // tiles; falloffOuter 0 = no proximity falloff
    u8 falloffOuter;
    u8 weights[PRESET_MAX_EFFECTS + 1]; // 0-16 share of the stage level per member; last is the tint. 0 = not stage driven
};

struct StageSpec
{
    u8 scale;               // 0-16 share of the preset intensity
    u8 fadeFrames;
};

// Member roles by preset:
//   PRESENCE: wave, shake       DIMENSIONAL: tear, wave
//   DIVINE_FOCUS: vignette      BURST: wave, shake, ripple
static const struct PresetSpec sPresetSpecs[SCREENFX_PRESET_COUNT] = {
    [SCREENFX_PRESET_LEGENDARY_PRESENCE] = { RGB(22, 14, 31), 2, 2, 10, {16, 4, 0, 6} },
    [SCREENFX_PRESET_DIMENSIONAL]        = { RGB(6, 0, 10),   2, 3, 12, {16, 8, 0, 8} },
    [SCREENFX_PRESET_DIVINE_FOCUS]       = { RGB(31, 29, 20), 1, 3, 12, {16, 0, 0, 5} },
    [SCREENFX_PRESET_LEGENDARY_BURST]    = { RGB(31, 31, 31), 3, 0, 0,  {16, 0, 0, 0} },
};

static const struct StageSpec sStageSpecs[SCREENFX_STAGE_COUNT] = {
    [SCREENFX_STAGE_ORDINARY]    = {  6, 60 },
    [SCREENFX_STAGE_PERCEPTIBLE] = { 10, 60 },
    [SCREENFX_STAGE_DRAMATIC]    = { 16, 30 },
    [SCREENFX_STAGE_SETTLE]      = {  4, 90 },
};

static struct PresetRecord *GetPreset(ScreenFxPresetId id)
{
    struct PresetRecord *record;

    if (id == SCREENFX_PRESET_ID_INVALID || SCREENFX_INDEX(id) >= MAX_SCREENFX_PRESETS)
        return NULL;

    record = &sPresets[SCREENFX_INDEX(id)];
    if (!record->active || record->generation != SCREENFX_GENERATION(id))
        return NULL;

    return record;
}

// Stops the members at once and clears the record.
static void ReleasePreset(struct PresetRecord *record)
{
    u8 generation = record->generation;
    u32 i;

    for (i = 0; i < PRESET_MAX_EFFECTS; i++)
        ScreenFx_Stop(record->effects[i]);
    Overlay_Destroy(record->overlay);

    memset(record, 0, sizeof(*record));
    record->generation = NextGeneration(generation);
}

static void ReleaseAllPresets(void)
{
    u32 i;

    for (i = 0; i < MAX_SCREENFX_PRESETS; i++)
    {
        if (sPresets[i].active)
            ReleasePreset(&sPresets[i]);
    }
}

static u8 GetMemberLevel(const struct PresetRecord *record, u32 weight)
{
    return (record->intensity * sStageSpecs[record->stage].scale * weight + 128) / 256;
}

// Fades every stage-driven member to the current stage's level.
static void ApplyStage(const struct PresetRecord *record)
{
    const struct PresetSpec *spec = &sPresetSpecs[record->preset];
    u16 frames = sStageSpecs[record->stage].fadeFrames;
    u32 i;

    for (i = 0; i < spec->effectCount; i++)
    {
        if (spec->weights[i] != 0)
            ScreenFx_FadeTo(record->effects[i], GetMemberLevel(record, spec->weights[i]), frames);
    }

    if (spec->weights[PRESET_MAX_EFFECTS] != 0)
        Overlay_FadeTo(record->overlay, GetMemberLevel(record, spec->weights[PRESET_MAX_EFFECTS]), frames);
}

static ScreenFxId StartMember(u8 kind, u8 intensity, u16 param1, u16 param2, u16 durationFrames)
{
    struct ScreenFxConfig config = {0};

    config.kind = kind;
    config.intensity = intensity;
    config.param1 = param1;
    config.param2 = param2;
    config.durationFrames = durationFrames;
    return ScreenFx_Start(&config);
}

static bool32 CreatePresetMembers(struct PresetRecord *record)
{
    const struct PresetSpec *spec = &sPresetSpecs[record->preset];
    struct OverlayConfig overlay = {0};
    u32 i;

    switch (record->preset)
    {
    case SCREENFX_PRESET_LEGENDARY_PRESENCE:
        record->effects[0] = StartMember(SCREENFX_WAVE, 0, 75, 64, 0);
        record->effects[1] = StartMember(SCREENFX_SHAKE, 0, 36, 0, 0);
        break;
    case SCREENFX_PRESET_DIMENSIONAL:
        record->effects[0] = StartMember(SCREENFX_TEAR, 0, 0, SCREENFX_TEAR_CENTER_AUTO, 0);
        record->effects[1] = StartMember(SCREENFX_WAVE, 0, 50, 40 | SCREENFX_WAVE_VERTICAL, 0);
        break;
    case SCREENFX_PRESET_DIVINE_FOCUS:
        record->effects[0] = StartMember(SCREENFX_VIGNETTE, 0, SCREENFX_VIGNETTE_MEDIUM, 0, 0);
        break;
    case SCREENFX_PRESET_LEGENDARY_BURST:
        // The shake stays silent and the ripple idle until the sequence reaches them.
        record->effects[0] = StartMember(SCREENFX_WAVE, record->intensity, 75, 64, 0);
        record->effects[1] = StartMember(SCREENFX_SHAKE, 0, 36, 0, 0);
        record->effects[2] = StartMember(SCREENFX_RIPPLE, SCREENFX_INTENSITY_MAX, 0, 0, PRESET_BURST_RIPPLE_LIFETIME);
        break;
    }

    for (i = 0; i < spec->effectCount; i++)
    {
        if (record->effects[i] == SCREENFX_ID_INVALID)
            return FALSE;
    }

    overlay.color = spec->tint;
    overlay.opacity = record->preset == SCREENFX_PRESET_LEGENDARY_BURST ? PRESET_TINT_MAX : 0;
    overlay.layer = OVERLAY_LAYER_ALL;
    overlay.scope = OVERLAY_SCOPE_GLOBAL;
    record->overlay = Overlay_Create(&overlay);
    if (record->overlay == OVERLAY_ID_INVALID)
        return FALSE;

    Overlay_SetTransient(record->overlay);
    if (record->preset == SCREENFX_PRESET_DIVINE_FOCUS)
        Overlay_Pulse(record->overlay, 6, PRESET_TINT_MAX, 120);

    return TRUE;
}

static void AnchorPreset(const struct PresetRecord *record, u8 localId)
{
    const struct PresetSpec *spec = &sPresetSpecs[record->preset];
    u8 mapNum = gSaveBlock1Ptr->location.mapNum;
    u8 mapGroup = gSaveBlock1Ptr->location.mapGroup;
    u32 i;

    for (i = 0; i < spec->effectCount; i++)
    {
        ScreenFx_SetAnchorToObject(record->effects[i], localId, mapNum, mapGroup);
        if (spec->falloffOuter != 0)
            ScreenFx_SetFalloff(record->effects[i], spec->falloffInner, spec->falloffOuter, 0, SCREENFX_INTENSITY_MAX);
    }

    if (spec->falloffOuter != 0)
    {
        Overlay_SetAnchorToObject(record->overlay, localId, mapNum, mapGroup);
        Overlay_SetFalloff(record->overlay, spec->falloffInner, spec->falloffOuter, 0, OVERLAY_OPACITY_MAX);
    }
}

ScreenFxPresetId ScreenFx_StartPreset(u8 preset, u8 intensity, u8 anchorLocalId)
{
    u32 i;

    if (preset >= SCREENFX_PRESET_COUNT)
        return SCREENFX_PRESET_ID_INVALID;

    for (i = 0; i < MAX_SCREENFX_PRESETS; i++)
    {
        struct PresetRecord *record = &sPresets[i];
        u8 generation = record->generation;

        if (record->active)
            continue;

        // A never-used slot still holds generation 0.
        memset(record, 0, sizeof(*record));
        record->generation = generation != 0 ? generation : 1;
        record->active = TRUE;
        record->preset = preset;
        record->intensity = min(intensity, SCREENFX_INTENSITY_MAX);

        if (!CreatePresetMembers(record))
        {
            ReleasePreset(record);
            return SCREENFX_PRESET_ID_INVALID;
        }

        if (anchorLocalId != 0)
            AnchorPreset(record, anchorLocalId);

        // The burst wave already runs at full intensity and settles from frame 2.
        record->stage = preset == SCREENFX_PRESET_LEGENDARY_BURST ? SCREENFX_STAGE_SETTLE : SCREENFX_STAGE_PERCEPTIBLE;
        if (preset != SCREENFX_PRESET_LEGENDARY_BURST)
            ApplyStage(record);

        return (record->generation << 3) | i;
    }

    return SCREENFX_PRESET_ID_INVALID;
}

void ScreenFx_StopPreset(ScreenFxPresetId id, u16 fadeFrames)
{
    struct PresetRecord *record = GetPreset(id);
    u8 generation;
    u32 i;

    if (record == NULL)
        return;

    for (i = 0; i < PRESET_MAX_EFFECTS; i++)
        ScreenFx_FadeOutAndStop(record->effects[i], fadeFrames);
    Overlay_FadeOutAndDisable(record->overlay, fadeFrames);

    // The members finish their fade-outs on their own.
    generation = record->generation;
    memset(record, 0, sizeof(*record));
    record->generation = NextGeneration(generation);
}

bool32 ScreenFx_IsPresetValid(ScreenFxPresetId id)
{
    return GetPreset(id) != NULL;
}

void ScreenFx_SetProgression(ScreenFxPresetId id, u8 stage)
{
    struct PresetRecord *record = GetPreset(id);

    if (record == NULL || stage >= SCREENFX_STAGE_COUNT)
        return;

    record->stage = stage;
    ApplyStage(record);
}

// Burst timeline: flash at frame 0, the rest from frame PRESET_BURST_FLASH_HOLD.
static void UpdateBurst(struct PresetRecord *record)
{
    if (record->frame > PRESET_BURST_FLASH_HOLD)
        return;

    if (record->frame == PRESET_BURST_FLASH_HOLD)
    {
        Overlay_FadeOutAndDisable(record->overlay, PRESET_BURST_FLASH_FADE);
        ScreenFx_TriggerRipple(record->effects[2], SCREENFX_RIPPLE_CENTER_AUTO, record->intensity, 0, 0);
        ScreenFx_SetIntensity(record->effects[1], record->intensity);
        ScreenFx_FadeOutAndStop(record->effects[1], PRESET_BURST_SHAKE_FADE);
        ScreenFx_FadeTo(record->effects[0], GetMemberLevel(record, sPresetSpecs[record->preset].weights[0]), PRESET_BURST_SETTLE);
    }

    record->frame++;
}

static void UpdatePresets(void)
{
    u32 i;

    for (i = 0; i < MAX_SCREENFX_PRESETS; i++)
    {
        if (sPresets[i].active && sPresets[i].preset == SCREENFX_PRESET_LEGENDARY_BURST)
            UpdateBurst(&sPresets[i]);
    }
}
