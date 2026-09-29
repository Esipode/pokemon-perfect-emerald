#include "global.h"
#include "healthbox.h"
#include "config/battle.h"
#include "constants/battle.h"
#include "constants/global.h"
#include "test/test.h"

static void ClearHealthboxSave(void)
{
    memset(&gSaveBlock2Ptr->healthboxOptions, 0, sizeof(gSaveBlock2Ptr->healthboxOptions));
    gSaveBlock2Ptr->optionsHpDisplayPlayer = 0;
    gSaveBlock2Ptr->optionsHpDisplayOpponent = 0;
}

TEST("(Healthbox) HP mode <-> toggles round trips for all six modes")
{
    u32 mode;
    bool32 bar;
    u32 value;

    for (mode = 0; mode < OPTIONS_HP_DISPLAY_COUNT; mode++)
    {
        HealthboxOptions_TogglesFromMode(mode, &bar, &value);
        EXPECT_EQ(HealthboxOptions_ModeFromToggles(bar, value), mode);
    }
}

TEST("(Healthbox) Pre-feature save migrates each legacy HP mode")
{
    u32 stored;
    bool32 bar;
    u32 value;

    ClearHealthboxSave();
    for (stored = 1; stored <= OPTIONS_HP_DISPLAY_COUNT; stored++)
    {
        gSaveBlock2Ptr->optionsHpDisplayPlayer = stored;
        gSaveBlock2Ptr->optionsHpDisplayOpponent = stored;
        HealthboxOptions_TogglesFromMode(stored - 1, &bar, &value);
        EXPECT_EQ(HealthboxOptions_Shows(HB_SIDE_PLAYER, HB_ELEM_HP_BAR), bar);
        EXPECT_EQ(HealthboxOptions_GetHpValue(HB_SIDE_PLAYER), value);
        EXPECT_EQ(HealthboxOptions_Shows(HB_SIDE_FOE, HB_ELEM_HP_BAR), bar);
        EXPECT_EQ(HealthboxOptions_GetHpValue(HB_SIDE_FOE), value);
    }
}

TEST("(Healthbox) Unset legacy HP fields read as each side's default")
{
    u32 foeMode = B_HP_PERCENTAGE_DISPLAY ? OPTIONS_HP_DISPLAY_BAR_PERCENT : OPTIONS_HP_DISPLAY_BAR_ONLY;
    bool32 bar;
    u32 value;

    ClearHealthboxSave();
    EXPECT_EQ(HealthboxOptions_Shows(HB_SIDE_PLAYER, HB_ELEM_HP_BAR), TRUE);
    EXPECT_EQ(HealthboxOptions_GetHpValue(HB_SIDE_PLAYER), HB_HPVAL_NUMBERS);
    HealthboxOptions_TogglesFromMode(foeMode, &bar, &value);
    EXPECT_EQ(HealthboxOptions_Shows(HB_SIDE_FOE, HB_ELEM_HP_BAR), bar);
    EXPECT_EQ(HealthboxOptions_GetHpValue(HB_SIDE_FOE), value);
}

TEST("(Healthbox) Pre-feature save shows every applicable element in the default style")
{
    ClearHealthboxSave();
    EXPECT_EQ(HealthboxOptions_GetStyle(), HEALTHBOX_STYLE_DEFAULT);
    EXPECT_EQ(HealthboxOptions_GetBackground(), HB_BG_SOLID);
    EXPECT(HealthboxOptions_Shows(HB_SIDE_PLAYER, HB_ELEM_NICK));
    EXPECT(HealthboxOptions_Shows(HB_SIDE_PLAYER, HB_ELEM_EXP));
    EXPECT(!HealthboxOptions_Shows(HB_SIDE_PLAYER, HB_ELEM_CAUGHT));
    EXPECT(HealthboxOptions_Shows(HB_SIDE_FOE, HB_ELEM_CAUGHT));
    EXPECT(!HealthboxOptions_Shows(HB_SIDE_FOE, HB_ELEM_EXP));
}

TEST("(Healthbox) SetDefaults initializes the options and ignores legacy fields")
{
    u32 foeMode = B_HP_PERCENTAGE_DISPLAY ? OPTIONS_HP_DISPLAY_BAR_PERCENT : OPTIONS_HP_DISPLAY_BAR_ONLY;

    ClearHealthboxSave();
    gSaveBlock2Ptr->optionsHpDisplayPlayer = OPTIONS_HP_DISPLAY_NONE + 1;
    gSaveBlock2Ptr->optionsHpDisplayOpponent = OPTIONS_HP_DISPLAY_NONE + 1;
    HealthboxOptions_SetDefaults();

    EXPECT(gSaveBlock2Ptr->healthboxOptions.initialized);
    EXPECT_EQ(HealthboxOptions_ModeFromToggles(HealthboxOptions_Shows(HB_SIDE_PLAYER, HB_ELEM_HP_BAR),
                                               HealthboxOptions_GetHpValue(HB_SIDE_PLAYER)),
              OPTIONS_HP_DISPLAY_BAR_NUMBERS);
    EXPECT_EQ(HealthboxOptions_ModeFromToggles(HealthboxOptions_Shows(HB_SIDE_FOE, HB_ELEM_HP_BAR),
                                               HealthboxOptions_GetHpValue(HB_SIDE_FOE)),
              foeMode);
}

TEST("(Healthbox) Commit stores the options and marks them initialized")
{
    struct HealthboxOptions options;

    ClearHealthboxSave();
    HealthboxOptions_Get(&options);
    options.style = HEALTHBOX_STYLE_NEW;
    options.background = HB_BG_NONE;
    options.foeNick = FALSE;
    HealthboxOptions_SetHpToggles(&options, HB_SIDE_FOE, FALSE, HB_HPVAL_PERCENT);
    HealthboxOptions_Commit(&options);

    EXPECT(gSaveBlock2Ptr->healthboxOptions.initialized);
    EXPECT_EQ(HealthboxOptions_GetStyle(), HEALTHBOX_STYLE_NEW);
    EXPECT_EQ(HealthboxOptions_GetBackground(), HB_BG_NONE);
    EXPECT(!HealthboxOptions_Shows(HB_SIDE_FOE, HB_ELEM_NICK));
    EXPECT(!HealthboxOptions_Shows(HB_SIDE_FOE, HB_ELEM_HP_BAR));
    EXPECT_EQ(HealthboxOptions_GetHpValue(HB_SIDE_FOE), HB_HPVAL_PERCENT);
    EXPECT(HealthboxOptions_Shows(HB_SIDE_PLAYER, HB_ELEM_NICK));
}

static void AllOnOpts(struct HealthboxResolvedOpts *o, u32 side)
{
    *o = (struct HealthboxResolvedOpts){0};
    o->side = side;
    o->nick = o->level = o->hpBar = o->status = o->types = o->statStages = TRUE;
    o->hpValue = HB_HPVAL_NUMBERS;
    if (side == HB_SIDE_PLAYER)
        o->exp = TRUE;
    else
        o->caught = TRUE;
}

TEST("(Healthbox) Layout: all-on singles is 104x32, doubles is 96x24")
{
    struct HealthboxResolvedOpts o;
    struct HealthboxLayout l;

    AllOnOpts(&o, HB_SIDE_PLAYER);
    Healthbox_ComputeLayout(&o, HB_SIDE_PLAYER, FALSE, B_POSITION_PLAYER_LEFT, &l);
    EXPECT_EQ(l.boxW, 104);
    EXPECT_EQ(l.boxH, 32);
    EXPECT_EQ(l.rightSpriteW, 64);
    EXPECT_EQ(l.screenX, 127);
    EXPECT_EQ(l.screenY, 58);

    Healthbox_ComputeLayout(&o, HB_SIDE_PLAYER, TRUE, B_POSITION_PLAYER_LEFT, &l);
    EXPECT_EQ(l.boxW, 96);
    EXPECT_EQ(l.boxH, 24);
    EXPECT_EQ(l.rightSpriteW, 32);
}

TEST("(Healthbox) Layout: foe is left-anchored and has no EXP rect")
{
    struct HealthboxResolvedOpts o;
    struct HealthboxLayout l;

    AllOnOpts(&o, HB_SIDE_FOE);
    o.exp = TRUE;
    Healthbox_ComputeLayout(&o, HB_SIDE_FOE, FALSE, B_POSITION_OPPONENT_LEFT, &l);
    EXPECT_EQ(l.rects[HB_RECT_EXP].w, 0);
    EXPECT_NE(l.rects[HB_RECT_CAUGHT].w, 0);
    EXPECT(!l.rightAnchored);
    EXPECT_EQ(l.screenX, 13);
    EXPECT_EQ(l.screenY, 16);
}

TEST("(Healthbox) Layout: empty bands are dropped")
{
    struct HealthboxResolvedOpts o;
    struct HealthboxLayout l;

    AllOnOpts(&o, HB_SIDE_PLAYER);
    o.status = o.statStages = FALSE;
    Healthbox_ComputeLayout(&o, HB_SIDE_PLAYER, FALSE, B_POSITION_PLAYER_LEFT, &l);
    EXPECT_EQ(l.boxH, 24);
    EXPECT_EQ(l.rects[HB_RECT_STATUS].w, 0);
    EXPECT_EQ(l.rects[HB_RECT_STRIP].w, 0);

    AllOnOpts(&o, HB_SIDE_FOE);
    o.nick = o.level = FALSE;
    Healthbox_ComputeLayout(&o, HB_SIDE_FOE, FALSE, B_POSITION_OPPONENT_LEFT, &l);
    EXPECT_EQ(l.rects[HB_RECT_NICK].w, 0);
    EXPECT_EQ(l.rects[HB_RECT_LEVEL].w, 0);
    EXPECT_EQ(l.rects[HB_RECT_HP_BAR].y, 2);
}

TEST("(Healthbox) Layout: every toggle combination stays inside the size limits")
{
    u32 mask, hpValue, config;
    struct HealthboxResolvedOpts o;
    struct HealthboxLayout l;
    u32 i;

    for (config = 0; config < 4; config++)
    {
        u32 side = config & 1;
        bool32 doubles = config >> 1;
        u32 maxRight = doubles ? 96 : 128;
        u32 maxH = doubles ? 24 : 32;

        for (mask = 0; mask < 128; mask++)
        {
            for (hpValue = HB_HPVAL_NONE; hpValue <= HB_HPVAL_PERCENT; hpValue++)
            {
                o = (struct HealthboxResolvedOpts){0};
                o.side = side;
                o.nick = mask & 1;
                o.level = (mask >> 1) & 1;
                o.hpBar = (mask >> 2) & 1;
                o.exp = (mask >> 3) & 1;
                o.status = (mask >> 4) & 1;
                o.statStages = (mask >> 5) & 1;
                o.caught = (mask >> 6) & 1;
                o.hpValue = hpValue;
                Healthbox_ComputeLayout(&o, side, doubles, doubles ? B_POSITION_PLAYER_RIGHT : B_POSITION_PLAYER_LEFT, &l);

                EXPECT_GE(l.boxW, 64);
                EXPECT_LE(l.boxW, maxRight);
                EXPECT_LE(l.boxH, maxH);
                for (i = 0; i < HB_RECT_COUNT; i++)
                {
                    if (l.rects[i].w == 0)
                        continue;
                    EXPECT_LE(l.rects[i].x + l.rects[i].w, l.boxW);
                    EXPECT_LE(l.rects[i].y + l.rects[i].h, l.boxH);
                }
            }
        }
    }
}

TEST("(Healthbox) ResolveOpts keeps the player-only and foe-only toggles on their own side")
{
    struct HealthboxResolvedOpts o;

    ClearHealthboxSave();
    Healthbox_ResolveOpts(HB_SIDE_PLAYER, &o);
    EXPECT(o.exp);
    EXPECT(!o.caught);
    Healthbox_ResolveOpts(HB_SIDE_FOE, &o);
    EXPECT(!o.exp);
    EXPECT(o.caught);
}

TEST("(Healthbox) Stat stage maps to the right glyph")
{
    EXPECT_EQ(Healthbox_StageToGlyph(0), HB_GLYPH_DOWN3);
    EXPECT_EQ(Healthbox_StageToGlyph(3), HB_GLYPH_DOWN3);
    EXPECT_EQ(Healthbox_StageToGlyph(4), HB_GLYPH_DOWN2);
    EXPECT_EQ(Healthbox_StageToGlyph(5), HB_GLYPH_DOWN1);
    EXPECT_EQ(Healthbox_StageToGlyph(6), HB_GLYPH_NONE);
    EXPECT_EQ(Healthbox_StageToGlyph(7), HB_GLYPH_UP1);
    EXPECT_EQ(Healthbox_StageToGlyph(8), HB_GLYPH_UP2);
    EXPECT_EQ(Healthbox_StageToGlyph(9), HB_GLYPH_UP3);
    EXPECT_EQ(Healthbox_StageToGlyph(12), HB_GLYPH_UP3);
}

TEST("(Healthbox) Stat strip order is Atk, Def, SpA, SpD, Spe")
{
    EXPECT_EQ(Healthbox_StripStat(0), STAT_ATK);
    EXPECT_EQ(Healthbox_StripStat(1), STAT_DEF);
    EXPECT_EQ(Healthbox_StripStat(2), STAT_SPATK);
    EXPECT_EQ(Healthbox_StripStat(3), STAT_SPDEF);
    EXPECT_EQ(Healthbox_StripStat(4), STAT_SPEED);
}

TEST("(Healthbox) Stat signature differs per slot and per stage")
{
    u8 a[5] = { 6, 6, 6, 6, 6 };
    u8 b[5] = { 6, 6, 6, 6, 7 };
    u8 c[5] = { 7, 6, 6, 6, 6 };

    EXPECT_NE(Healthbox_PackStages(a), Healthbox_PackStages(b));
    EXPECT_NE(Healthbox_PackStages(b), Healthbox_PackStages(c));
    EXPECT_EQ(Healthbox_PackStages(a), Healthbox_PackStages(a));
}
