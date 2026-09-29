#include "global.h"
#include "healthbox.h"
#include "config/battle.h"
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
