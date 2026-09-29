#ifndef GUARD_HEALTHBOX_H
#define GUARD_HEALTHBOX_H

#include "global.h"
#include "constants/healthbox.h"

// TRUE forces the New style everywhere it is allowed, ignoring the saved option.
#define HEALTHBOX_DEV_FORCE_NEW FALSE

struct Pokemon;
struct Sprite;
struct SubspriteTable;

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

// Stat strip: stages[] is in strip order (Atk, Def, SpA, SpD, Spe). HB_GLYPH_NONE means nothing is drawn.
u32 Healthbox_StageToGlyph(u8 stage);
u32 Healthbox_PackStages(const u8 stages[5]);
u32 Healthbox_StripStat(u32 slot); // STAT_* shown in a strip slot

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
// Prints str inside rect after clearing it, shrinking the font to fit rect->w. Text crossing the
// left/right sprite seam is handled.
void HealthboxRender_PrintText(const struct HealthboxSprites *sprites, const struct HealthboxLayout *layout,
                               const struct HealthboxRect *rect, const u8 *str, bool32 rightAlign, u32 background);
void HealthboxRender_DrawFrame(const struct HealthboxSprites *sprites, const struct HealthboxLayout *layout,
                               u32 background);
void HealthboxRender_DrawExpBar(const struct HealthboxSprites *sprites, const struct HealthboxLayout *layout,
                                const struct HealthboxRect *rect, u32 fillPx);
// A NULL label clears the pill. palIndex is the box palette slot holding the status colour.
void HealthboxRender_DrawStatusPill(const struct HealthboxSprites *sprites, const struct HealthboxLayout *layout,
                                    const struct HealthboxRect *rect, u32 palIndex, const u8 *label, u32 background);
void HealthboxRender_DrawStatStrip(const struct HealthboxSprites *sprites, const struct HealthboxLayout *layout,
                                   const struct HealthboxRect *rect, const u8 stages[5], u32 background);
// colourLevel: 0 green, 1 yellow, 2 red.
void HealthboxRender_DrawHpBar(u8 barSpriteId, u32 fillPx, u32 colourLevel);

// Battle glue.
bool32 Healthbox_IsNewStyle(void);
void HealthboxBattle_LoadPalette(void);
bool32 HealthboxBattle_LoadBoxSheet(u8 state);
u8 HealthboxBattle_CreateBoxSprites(u32 battler, void (*otherCallback)(struct Sprite *), s16 *barData6);
void HealthboxBattle_GetCoords(u32 battler, s16 *x, s16 *y);
void HealthboxBattle_DrawNick(u8 healthboxSpriteId, struct Pokemon *mon);
void HealthboxBattle_DrawHpValue(u8 healthboxSpriteId, s16 currHp, s16 maxHp);
// Gimmick indicator offsets from the main sprite centre.
void HealthboxBattle_GetIndicatorPos(u32 battler, s16 *x, s16 *y);
bool32 HealthboxBattle_HasHpBar(u32 battler);
bool32 HealthboxBattle_HasCaughtIcons(u32 battler);
const struct SubspriteTable *HealthboxBattle_GetBarSubspriteTable(u32 battler);
void HealthboxBattle_DrawHpBar(u8 healthboxSpriteId, u32 fillPx, u32 colourLevel);
void HealthboxBattle_DrawExpBar(u8 healthboxSpriteId, u32 fillPx);
// Redraws the stat strip when the battler's stages changed; called every frame by the bar sprite.
void HealthboxBattle_PollStatStrip(u8 healthboxSpriteId);
void HealthboxBattle_Update(u8 healthboxSpriteId, struct Pokemon *mon, u8 elementId);

#endif // GUARD_HEALTHBOX_H
