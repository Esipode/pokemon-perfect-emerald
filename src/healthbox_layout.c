#include "global.h"
#include "healthbox.h"
#include "constants/battle.h"
#include "constants/healthbox.h"
#include "constants/rgb.h"
#include "data/healthbox.h"

#define HB_MIN_BOX_W 64
#define HB_ROUND_UP_8(n) (((n) + 7) & ~7)
// The height is only rounded to an even number; the doubles box has no spare rows for tile alignment.
#define HB_ROUND_UP_2(n) (((n) + 1) & ~1)

void Healthbox_ResolveOptsFrom(const struct HealthboxOptions *o, u32 side, struct HealthboxResolvedOpts *out)
{
    *out = (struct HealthboxResolvedOpts){0};
    out->side = side;
    out->background = o->background;
    if (side == HB_SIDE_PLAYER)
    {
        out->nick = o->playerNick;
        out->level = o->playerLevel;
        out->hpBar = o->playerHpBar;
        out->hpValue = o->playerHpValue;
        out->exp = o->playerExp;
        out->status = o->playerStatus;
        out->types = o->playerTypes;
        out->statStages = o->playerStatStages;
    }
    else
    {
        out->nick = o->foeNick;
        out->level = o->foeLevel;
        out->hpBar = o->foeHpBar;
        out->hpValue = o->foeHpValue;
        out->status = o->foeStatus;
        out->types = o->foeTypes;
        out->statStages = o->foeStatStages;
        out->caught = o->foeCaught;
        out->catchable = o->foeCatchable;
    }
}

void Healthbox_ResolveOpts(u32 side, struct HealthboxResolvedOpts *out)
{
    struct HealthboxOptions o;

    HealthboxOptions_Get(&o);
    Healthbox_ResolveOptsFrom(&o, side, out);
}

void Healthbox_ComputeLayout(const struct HealthboxResolvedOpts *opts, u32 side, bool32 doubles,
                             u32 position, struct HealthboxLayout *out)
{
    const struct HealthboxLayoutSpec *spec = doubles ? &sHealthboxDoublesSpec : &sHealthboxSinglesSpec;
    const struct HealthboxAnchor *anchor;
    bool8 shown[HB_RECT_COUNT];
    bool8 bandUsed[HB_BAND_COUNT] = {0};
    u8 bandTop[HB_BAND_COUNT] = {0};
    u32 i, y, right = 0;

    // Doubles show the HP value only in place of the bar.
    shown[HB_RECT_NICK]     = opts->nick;
    shown[HB_RECT_LEVEL]    = opts->level;
    shown[HB_RECT_HP_BAR]   = opts->hpBar;
    shown[HB_RECT_HP_VALUE] = opts->hpValue != HB_HPVAL_NONE && (!doubles || !opts->hpBar);
    shown[HB_RECT_EXP]      = !doubles && side == HB_SIDE_PLAYER && opts->exp;
    shown[HB_RECT_STATUS]   = opts->status;
    shown[HB_RECT_STRIP]    = opts->statStages;
    shown[HB_RECT_CAUGHT]   = side == HB_SIDE_FOE && opts->caught;
    shown[HB_RECT_CATCHABLE] = side == HB_SIDE_FOE && opts->catchable;

    for (i = 0; i < HB_RECT_COUNT; i++)
    {
        if (shown[i])
            bandUsed[spec->elems[i].band] = TRUE;
    }

    y = spec->topPad;
    for (i = 0; i < HB_BAND_COUNT; i++)
    {
        if (!bandUsed[i])
            continue;
        bandTop[i] = y;
        y += spec->bandH[i];
    }

    *out = (struct HealthboxLayout){0};
    out->rightSpriteW = spec->rightSpriteW;
    out->compactHpValue = doubles;
    for (i = 0; i < HB_RECT_COUNT; i++)
    {
        const struct HealthboxElemSpec *elem = &spec->elems[i];

        if (!shown[i])
            continue;
        out->rects[i] = (struct HealthboxRect){ elem->x, bandTop[elem->band] + elem->yOff, elem->w, elem->h };
        if (elem->x + elem->w > right)
            right = elem->x + elem->w;
    }

    out->boxH = y ? HB_ROUND_UP_2(y) : 8;
    out->boxW = right ? HB_ROUND_UP_8(right + spec->rightMargin) : HB_MIN_BOX_W;
    if (out->boxW < HB_MIN_BOX_W)
        out->boxW = HB_MIN_BOX_W;

    // A lone HP bar is centred on the box.
    if (out->rects[HB_RECT_HP_BAR].w != 0)
    {
        bool32 alone = TRUE;

        for (i = 0; i < HB_RECT_COUNT; i++)
        {
            if (i != HB_RECT_HP_BAR && out->rects[i].w != 0)
                alone = FALSE;
        }
        if (alone)
        {
            out->rects[HB_RECT_HP_BAR].x = (out->boxW - out->rects[HB_RECT_HP_BAR].w) / 2;
            out->rects[HB_RECT_HP_BAR].y = (out->boxH - out->rects[HB_RECT_HP_BAR].h) / 2;
        }
    }

    anchor = doubles ? &sHealthboxDoublesAnchors[position] : &sHealthboxSinglesAnchors[position & 1];
    out->anchorX = anchor->x;
    out->anchorY = anchor->y;
    out->rightAnchored = anchor->rightAnchored;
    out->screenX = anchor->rightAnchored ? anchor->x - out->boxW : anchor->x;
    out->screenY = anchor->bottomAnchored ? anchor->y - out->boxH : anchor->y;
}

u32 Healthbox_StageToGlyph(u8 stage)
{
    if (stage == DEFAULT_STAT_STAGE)
        return HB_GLYPH_NONE;
    if (stage > DEFAULT_STAT_STAGE)
        return min(stage - DEFAULT_STAT_STAGE, 3) - 1 + HB_GLYPH_UP1;
    return min(DEFAULT_STAT_STAGE - stage, 3) - 1 + HB_GLYPH_DOWN1;
}

u32 Healthbox_PackStages(const u8 stages[HB_STRIP_SLOTS])
{
    u32 sig = 0, i;

    for (i = 0; i < HB_STRIP_SLOTS; i++)
        sig |= (stages[i] & 0xF) << (i * 4);
    return sig;
}

u32 Healthbox_StripStat(u32 slot)
{
    return sStatStripStats[slot];
}
