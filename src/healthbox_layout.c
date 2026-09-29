#include "global.h"
#include "healthbox.h"
#include "constants/battle.h"
#include "constants/healthbox.h"
#include "constants/rgb.h"
#include "data/healthbox.h"

#define HB_MIN_BOX_W 64
#define HB_ROUND_UP_8(n) (((n) + 7) & ~7)

void Healthbox_ResolveOpts(u32 side, struct HealthboxResolvedOpts *out)
{
    struct HealthboxOptions o;

    HealthboxOptions_Get(&o);
    *out = (struct HealthboxResolvedOpts){0};
    out->side = side;
    out->background = o.background;
    if (side == HB_SIDE_PLAYER)
    {
        out->nick = o.playerNick;
        out->level = o.playerLevel;
        out->hpBar = o.playerHpBar;
        out->hpValue = o.playerHpValue;
        out->exp = o.playerExp;
        out->status = o.playerStatus;
        out->types = o.playerTypes;
        out->statStages = o.playerStatStages;
    }
    else
    {
        out->nick = o.foeNick;
        out->level = o.foeLevel;
        out->hpBar = o.foeHpBar;
        out->hpValue = o.foeHpValue;
        out->status = o.foeStatus;
        out->types = o.foeTypes;
        out->statStages = o.foeStatStages;
        out->caught = o.foeCaught;
    }
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
    for (i = 0; i < HB_RECT_COUNT; i++)
    {
        const struct HealthboxElemSpec *elem = &spec->elems[i];

        if (!shown[i])
            continue;
        out->rects[i] = (struct HealthboxRect){ elem->x, bandTop[elem->band] + elem->yOff, elem->w, elem->h };
        if (elem->x + elem->w > right)
            right = elem->x + elem->w;
    }

    out->boxH = y ? HB_ROUND_UP_8(y) : 8;
    out->boxW = right ? HB_ROUND_UP_8(right + spec->rightMargin) : HB_MIN_BOX_W;
    if (out->boxW < HB_MIN_BOX_W)
        out->boxW = HB_MIN_BOX_W;

    anchor = doubles ? &sHealthboxDoublesAnchors[position] : &sHealthboxSinglesAnchors[position & 1];
    out->anchorX = anchor->x;
    out->anchorY = anchor->y;
    out->rightAnchored = anchor->rightAnchored;
    out->screenX = anchor->rightAnchored ? anchor->x - out->boxW : anchor->x;
    out->screenY = anchor->y;
}
