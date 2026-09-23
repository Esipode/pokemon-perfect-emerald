#include "global.h"
#include "data.h"
#include "graphics.h"
#include "player_customization.h"
#include "constants/event_objects.h"
#include "constants/rgb.h"

// Per-step brightness/saturation nudge; +/-3 steps must not crush a colour to black/white.
#define SHADE_STEP_V 24
#define SHADE_STEP_S 8

#include "data/player_customization.h"

static EWRAM_DATA u16 sOwPaletteBuffer[16] = {0};
static EWRAM_DATA u16 sTrainerPaletteBuffer[16] = {0};

static const u8 sBattleTransitionBgIndices[] = {0, 1, 2, 3, 4, 5};

// 0x00 must decode to (hue = 0, shade = 0) so old saves stay vanilla.
static void UnpackColorByte(u8 raw, u8 *hue, s8 *shade)
{
    s8 s = (raw >> 4) & 0xF;
    if (s > 7)
        s -= 16;
    *hue = raw & 0xF;
    *shade = s;
}

static u8 PackColorByte(u8 hue, s8 shade)
{
    return (hue & 0xF) | ((shade & 0xF) << 4);
}

static void GetRegionChoice(enum PlayerColorRegion region, u8 *hue, s8 *shade)
{
    UnpackColorByte(gSaveBlock2Ptr->playerColors[region], hue, shade);
}

u8 Player_GetColorHue(enum PlayerColorRegion region)
{
    u8 hue;
    s8 shade;
    GetRegionChoice(region, &hue, &shade);
    return hue;
}

s8 Player_GetColorShade(enum PlayerColorRegion region)
{
    u8 hue;
    s8 shade;
    GetRegionChoice(region, &hue, &shade);
    return shade;
}

void Player_SetColorHue(enum PlayerColorRegion region, u8 hue)
{
    u8 curHue;
    s8 curShade;
    GetRegionChoice(region, &curHue, &curShade);
    gSaveBlock2Ptr->playerColors[region] = PackColorByte(hue, curShade);
}

void Player_SetColorShade(enum PlayerColorRegion region, s8 shade)
{
    u8 curHue;
    s8 curShade;
    GetRegionChoice(region, &curHue, &curShade);
    gSaveBlock2Ptr->playerColors[region] = PackColorByte(curHue, shade);
}

u16 Player_GetColorSlot(u32 slot)
{
    return gSaveBlock2Ptr->playerColorSlots[slot];
}

void Player_SetColorSlot(u32 slot, u16 rgb)
{
    gSaveBlock2Ptr->playerColorSlots[slot] = PLAYER_COLOR_SET | rgb;
}

void Player_ClearColorSlot(u32 slot)
{
    gSaveBlock2Ptr->playerColorSlots[slot] = 0;
}

u8 Player_GetSpriteStyle(void)
{
    return gSaveBlock2Ptr->playerSpriteStyle;
}

void Player_SetSpriteStyle(u8 style)
{
    if (style >= PLAYER_SPRITE_STYLE_COUNT)
        style = PLAYER_SPRITE_STYLE_EMERALD;
    gSaveBlock2Ptr->playerSpriteStyle = style;
}

bool32 PlayerCustomization_IsDefault(void)
{
    u32 i;
    for (i = 0; i < PLAYER_COLOR_REGION_COUNT; i++)
    {
        if (gSaveBlock2Ptr->playerColors[i] != 0)
            return FALSE;
    }
    return TRUE;
}

// Integer RGB(8-bit)<->HSV(all 0-255) helpers.
static void RgbToHsv(u8 r, u8 g, u8 b, u8 *h, u8 *s, u8 *v)
{
    u8 max = r;
    u8 min = r;
    s16 delta;

    if (g > max)
        max = g;
    if (b > max)
        max = b;
    if (g < min)
        min = g;
    if (b < min)
        min = b;

    *v = max;
    delta = max - min;

    if (max == 0 || delta == 0)
    {
        *s = 0;
        *h = 0;
        return;
    }

    *s = (delta * 255) / max;

    if (max == r)
        *h = (u8)((43 * ((s16)g - b)) / delta);
    else if (max == g)
        *h = (u8)(85 + (43 * ((s16)b - r)) / delta);
    else
        *h = (u8)(171 + (43 * ((s16)r - g)) / delta);
}

static void HsvToRgb(u8 h, u8 s, u8 v, u8 *r, u8 *g, u8 *b)
{
    u8 region;
    u8 remainder;
    u8 p, q, t;

    if (s == 0)
    {
        *r = *g = *b = v;
        return;
    }

    region = h / 43;
    remainder = (h - region * 43) * 6;

    p = (v * (255 - s)) >> 8;
    q = (v * (255 - ((s * remainder) >> 8))) >> 8;
    t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

    switch (region)
    {
    case 0:
        *r = v, *g = t, *b = p;
        break;
    case 1:
        *r = q, *g = v, *b = p;
        break;
    case 2:
        *r = p, *g = v, *b = t;
        break;
    case 3:
        *r = p, *g = q, *b = v;
        break;
    case 4:
        *r = t, *g = p, *b = v;
        break;
    default:
        *r = v, *g = p, *b = q;
        break;
    }
}

// Near-white/near-black entries have S close to 0, so hue rotation leaves outlines and whites alone.
static void ApplyRegionToPalette(u16 *pal, const u8 *indices, u8 count, u8 hue, s8 shade)
{
    u32 i;

    if (hue == 0 && shade == 0)
        return;

    for (i = 0; i < count; i++)
    {
        u16 color = pal[indices[i]];
        u8 r = GET_R(color) * 255 / 31;
        u8 g = GET_G(color) * 255 / 31;
        u8 b = GET_B(color) * 255 / 31;
        u8 h, s, v;

        RgbToHsv(r, g, b, &h, &s, &v);

        h += hue * (256 / PLAYER_COLOR_HUE_COUNT); // u8 add wraps mod 256, matching hue's circular range
        v = min(max(v + shade * SHADE_STEP_V, 0), 255);
        s = min(max(s + shade * SHADE_STEP_S, 0), 255);

        HsvToRgb(h, s, v, &r, &g, &b);
        pal[indices[i]] = RGB(r * 31 / 255, g * 31 / 255, b * 31 / 255);
    }
}

static void ApplyAllRegions(u16 *pal, u8 gender, bool32 useTrainerIndices)
{
    u32 i;

    for (i = 0; i < PLAYER_COLOR_REGION_COUNT; i++)
    {
        const struct PlayerColorRegionInfo *info = &sPlayerColorRegions[gender][i];
        u8 hue;
        s8 shade;

        GetRegionChoice(i, &hue, &shade);
        if (useTrainerIndices)
            ApplyRegionToPalette(pal, info->trainerIndices, info->numTrainerIndices, hue, shade);
        else
            ApplyRegionToPalette(pal, info->owIndices, info->numOwIndices, hue, shade);
    }
}

const u16 *PlayerCustomization_GetOwPaletteOverride(u16 paletteTag)
{
    u8 gender = gSaveBlock2Ptr->playerGender;
    u16 expectedTag = (gender == MALE) ? OBJ_EVENT_PAL_TAG_BRENDAN : OBJ_EVENT_PAL_TAG_MAY;
    const u16 *basePal;
    u32 i;

    if (paletteTag != expectedTag || PlayerCustomization_IsDefault())
        return NULL;

    basePal = (gender == MALE) ? gObjectEventPal_Brendan : gObjectEventPal_May;
    for (i = 0; i < 16; i++)
        sOwPaletteBuffer[i] = basePal[i];

    ApplyAllRegions(sOwPaletteBuffer, gender, FALSE);
    return sOwPaletteBuffer;
}

const u16 *PlayerCustomization_GetTrainerPaletteOverride(u32 trainerPicId)
{
    u8 gender = gSaveBlock2Ptr->playerGender;
    u32 expectedPicId = (gender == MALE) ? TRAINER_PIC_BRENDAN : TRAINER_PIC_MAY;
    const u16 *basePal;
    u32 i;

    if (trainerPicId != expectedPicId || PlayerCustomization_IsDefault())
        return NULL;

    // Read gTrainerPicInfo directly; GetTrainerFrontPicPalette/GetTrainerBackPicPalette
    // route through this function and would recurse.
    basePal = gTrainerPicInfo[expectedPicId].frontPic->paletteData;
    for (i = 0; i < 16; i++)
        sTrainerPaletteBuffer[i] = basePal[i];

    ApplyAllRegions(sTrainerPaletteBuffer, gender, TRUE);
    return sTrainerPaletteBuffer;
}

u8 PlayerCustomization_GetRegionSwatchIndex(u8 gender, enum PlayerColorRegion region)
{
    return sPlayerColorRegions[gender][region].owIndices[0];
}

void PlayerCustomization_GetBattleTransitionMugshotBgPalette(const u16 *basePal, u16 *dest)
{
    u8 hue;
    s8 shade;
    u32 i;

    for (i = 0; i < 6; i++)
        dest[i] = basePal[i];

    if (PlayerCustomization_IsDefault())
        return;

    GetRegionChoice(PLAYER_COLOR_REGION_OUTFIT, &hue, &shade);
    ApplyRegionToPalette(dest, sBattleTransitionBgIndices, ARRAY_COUNT(sBattleTransitionBgIndices), hue, shade);
}

void PlayerCustomization_BuildPreviewPalette(u8 gender, const u8 *choices, u16 *dest)
{
    const u16 *basePal = (gender == MALE) ? gObjectEventPal_Brendan : gObjectEventPal_May;
    u32 i;

    for (i = 0; i < 16; i++)
        dest[i] = basePal[i];

    for (i = 0; i < PLAYER_COLOR_REGION_COUNT; i++)
    {
        const struct PlayerColorRegionInfo *info = &sPlayerColorRegions[gender][i];
        u8 hue;
        s8 shade;

        UnpackColorByte(choices[i], &hue, &shade);
        ApplyRegionToPalette(dest, info->owIndices, info->numOwIndices, hue, shade);
    }
}
