#ifndef GUARD_HEALTHBOX_H
#define GUARD_HEALTHBOX_H

#include "global.h"
#include "constants/healthbox.h"

// TRUE forces the New style everywhere it is allowed, ignoring the saved option.
#define HEALTHBOX_DEV_FORCE_NEW FALSE

struct Pokemon;
struct Sprite;

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

// The three sprites of one healthbox: left box half, right box half, HP bar.
struct HealthboxSprites
{
    u8 left;
    u8 right;
    u8 bar;
};

// Box-local pixel coordinates; pixels outside the box sprites are ignored.
void HealthboxRender_PutPixel(const struct HealthboxSprites *sprites, const struct HealthboxLayout *layout,
                              s32 x, s32 y, u8 palIndex);
void HealthboxRender_FillRect(const struct HealthboxSprites *sprites, const struct HealthboxLayout *layout,
                              s32 x, s32 y, s32 w, s32 h, u8 palIndex);
void HealthboxRender_Clear(const struct HealthboxSprites *sprites, const struct HealthboxLayout *layout);
void HealthboxRender_DrawFrame(const struct HealthboxSprites *sprites, const struct HealthboxLayout *layout,
                               u32 background);

// Battle glue.
bool32 Healthbox_IsNewStyle(void);
void HealthboxBattle_LoadPalette(void);
bool32 HealthboxBattle_LoadBoxSheet(u8 state);
u8 HealthboxBattle_CreateBoxSprites(u32 battler, void (*otherCallback)(struct Sprite *), s16 *barData6);
void HealthboxBattle_GetCoords(u32 battler, s16 *x, s16 *y);
void HealthboxBattle_Update(u8 healthboxSpriteId, struct Pokemon *mon, u8 elementId);

#endif // GUARD_HEALTHBOX_H
