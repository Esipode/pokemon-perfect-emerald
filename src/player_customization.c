#include "global.h"
#include "data.h"
#include "graphics.h"
#include "player_customization.h"
#include "constants/event_objects.h"
#include "constants/rgb.h"
#include "constants/trainers.h"

#include "data/player_customization.h"

static EWRAM_DATA u16 sOwPaletteBuffer[16] = {0};
static EWRAM_DATA u16 sTrainerPaletteBuffer[16] = {0};

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
    for (i = 0; i < PLAYER_COLOR_SLOT_COUNT; i++)
    {
        if (gSaveBlock2Ptr->playerColorSlots[i] != 0)
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

void PlayerCustomization_RgbToHsv(u16 color, u8 *h, u8 *s, u8 *v)
{
    u8 r = GET_R(color) * 255 / 31;
    u8 g = GET_G(color) * 255 / 31;
    u8 b = GET_B(color) * 255 / 31;
    RgbToHsv(r, g, b, h, s, v);
}

u16 PlayerCustomization_HsvToRgb(u8 h, u8 s, u8 v)
{
    u8 r, g, b;
    HsvToRgb(h, s, v, &r, &g, &b);
    return RGB(r * 31 / 255, g * 31 / 255, b * 31 / 255);
}

enum PlayerPaletteAsset { PLAYER_PALETTE_ASSET_OW, PLAYER_PALETTE_ASSET_TRAINER };

// For each slot with a nonzero stored value, writes it into every index that slot owns for `asset`.
// No HSV maths -- that only runs in the menu's editing model now.
static void ApplySlotsToPalette(u16 *pal, u8 style, u8 gender, enum PlayerPaletteAsset asset)
{
    u32 slot;

    for (slot = 0; slot < PLAYER_COLOR_SLOT_COUNT; slot++)
    {
        const struct PlayerColorSlotInfo *info = &sPlayerColorSlots[style][gender][slot];
        u16 value = gSaveBlock2Ptr->playerColorSlots[slot];
        const u8 *indices = (asset == PLAYER_PALETTE_ASSET_TRAINER) ? info->trainerIndices : info->owIndices;
        u8 count = (asset == PLAYER_PALETTE_ASSET_TRAINER) ? info->numTrainerIndices : info->numOwIndices;
        u32 i;

        if (info->name == NULL || value == 0)
            continue;

        for (i = 0; i < count; i++)
            pal[indices[i]] = value & ~PLAYER_COLOR_SET;
    }
}

static const u16 *GetOwBasePalette(u8 style, u8 gender)
{
    if (style == PLAYER_SPRITE_STYLE_FRLG)
        return gObjectEventPal_PlayerFrlg; // both FRLG genders share one base palette
    return (gender == MALE) ? gObjectEventPal_Brendan : gObjectEventPal_May;
}

const u16 *PlayerCustomization_GetOwPaletteOverride(u16 paletteTag)
{
    u8 style = Player_GetSpriteStyle();
    u8 gender = gSaveBlock2Ptr->playerGender;
    u16 expectedTag;
    const u16 *basePal;
    u32 i;

    if (style == PLAYER_SPRITE_STYLE_FRLG)
        expectedTag = (gender == MALE) ? OBJ_EVENT_PAL_TAG_PLAYER_RED : OBJ_EVENT_PAL_TAG_PLAYER_GREEN;
    else
        expectedTag = (gender == MALE) ? OBJ_EVENT_PAL_TAG_BRENDAN : OBJ_EVENT_PAL_TAG_MAY;

    if (paletteTag != expectedTag || PlayerCustomization_IsDefault())
        return NULL;

    basePal = GetOwBasePalette(style, gender);
    for (i = 0; i < 16; i++)
        sOwPaletteBuffer[i] = basePal[i];

    ApplySlotsToPalette(sOwPaletteBuffer, style, gender, PLAYER_PALETTE_ASSET_OW);
    return sOwPaletteBuffer;
}

const u16 *PlayerCustomization_GetTrainerPaletteOverride(u32 trainerPicId)
{
    u8 style = Player_GetSpriteStyle();
    u8 gender = gSaveBlock2Ptr->playerGender;
    u32 expectedPicId;
    const u16 *basePal;
    u32 i;

    if (style == PLAYER_SPRITE_STYLE_FRLG)
        expectedPicId = (gender == MALE) ? TRAINER_PIC_RED : TRAINER_PIC_LEAF;
    else
        expectedPicId = (gender == MALE) ? TRAINER_PIC_BRENDAN : TRAINER_PIC_MAY;

    if (trainerPicId != expectedPicId || PlayerCustomization_IsDefault())
        return NULL;

    // Read gTrainerPicInfo directly; GetTrainerFrontPicPalette/GetTrainerBackPicPalette
    // route through this function and would recurse.
    basePal = gTrainerPicInfo[expectedPicId].frontPic->paletteData;
    for (i = 0; i < 16; i++)
        sTrainerPaletteBuffer[i] = basePal[i];

    ApplySlotsToPalette(sTrainerPaletteBuffer, style, gender, PLAYER_PALETTE_ASSET_TRAINER);
    return sTrainerPaletteBuffer;
}

u8 PlayerCustomization_GetSlotSwatchIndex(u8 style, u8 gender, u8 slot)
{
    return sPlayerColorSlots[style][gender][slot].owIndices[0];
}

// TODO(Stage P4): apply the OUTFIT group's HSV delta to this gradient instead of leaving it vanilla.
void PlayerCustomization_GetBattleTransitionMugshotBgPalette(const u16 *basePal, u16 *dest)
{
    u32 i;

    for (i = 0; i < 6; i++)
        dest[i] = basePal[i];
}

void PlayerCustomization_BuildPreviewPalette(u8 style, u8 gender, const u16 *choices, u16 *dest)
{
    const u16 *basePal = GetOwBasePalette(style, gender);
    u32 i;

    for (i = 0; i < 16; i++)
        dest[i] = basePal[i];

    for (i = 0; i < PLAYER_COLOR_SLOT_COUNT; i++)
    {
        const struct PlayerColorSlotInfo *info = &sPlayerColorSlots[style][gender][i];
        u16 value = choices[i];
        u32 j;

        if (info->name == NULL || value == 0)
            continue;

        for (j = 0; j < info->numOwIndices; j++)
            dest[info->owIndices[j]] = value & ~PLAYER_COLOR_SET;
    }
}
