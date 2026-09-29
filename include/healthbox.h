#ifndef GUARD_HEALTHBOX_H
#define GUARD_HEALTHBOX_H

#include "global.h"
#include "constants/healthbox.h"

// Effective (default-resolved) options; see struct HealthboxOptions in global.h.
void HealthboxOptions_Get(struct HealthboxOptions *out);
void HealthboxOptions_Commit(const struct HealthboxOptions *options);
void HealthboxOptions_SetDefaults(void);

u32 HealthboxOptions_GetStyle(void);
u32 HealthboxOptions_GetBackground(void);
bool32 HealthboxOptions_Shows(u32 side, enum HealthboxElement elem);
u32 HealthboxOptions_GetHpValue(u32 side);

// Classic HP display mapping (OPTIONS_HP_DISPLAY_*) <-> per-side bar/value toggles.
u32 HealthboxOptions_ModeFromToggles(bool32 bar, u32 value);
void HealthboxOptions_TogglesFromMode(u32 mode, bool32 *bar, u32 *value);
void HealthboxOptions_SetHpToggles(struct HealthboxOptions *options, u32 side, bool32 bar, u32 value);

// Box-local pixel rect. w == 0 means the element is absent.
struct HealthboxRect
{
    u8 x, y, w, h;
};

// Flat per-side copy of the toggles the layout engine reads.
struct HealthboxResolvedOpts
{
    u8 side;       // HB_SIDE_*
    u8 background; // HB_BG_*
    u8 nick;
    u8 level;
    u8 hpBar;
    u8 hpValue;    // HB_HPVAL_*
    u8 exp;        // player only
    u8 status;
    u8 types;
    u8 statStages;
    u8 caught;     // foe only
};

struct HealthboxLayout
{
    u8 boxW, boxH;
    u8 rightSpriteW;   // 64 (singles) or 32 (doubles); the left sprite is always 64 wide
    struct HealthboxRect rects[HB_RECT_COUNT];
    s16 anchorX;       // screen edge the box is pinned to
    s16 anchorY;       // screen top
    bool8 rightAnchored;
    s16 screenX;       // resolved top-left
    s16 screenY;
};

void Healthbox_ResolveOpts(u32 side, struct HealthboxResolvedOpts *out);
void Healthbox_ComputeLayout(const struct HealthboxResolvedOpts *opts, u32 side, bool32 doubles,
                             u32 position, struct HealthboxLayout *out);

#endif // GUARD_HEALTHBOX_H
