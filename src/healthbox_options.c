#include "global.h"
#include "healthbox.h"
#include "config/battle.h"
#include "constants/global.h"

// Legacy optionsHpDisplay* fields store the OPTIONS_HP_DISPLAY_* mode + 1; 0 is unset.
static u32 LegacyHpMode(u32 side)
{
    u32 stored = (side == HB_SIDE_PLAYER) ? gSaveBlock2Ptr->optionsHpDisplayPlayer : gSaveBlock2Ptr->optionsHpDisplayOpponent;

    if (stored == 0 || stored - 1 >= OPTIONS_HP_DISPLAY_COUNT)
    {
        if (side == HB_SIDE_PLAYER)
            return OPTIONS_HP_DISPLAY_BAR_NUMBERS;
        return B_HP_PERCENTAGE_DISPLAY ? OPTIONS_HP_DISPLAY_BAR_PERCENT : OPTIONS_HP_DISPLAY_BAR_ONLY;
    }
    return stored - 1;
}

u32 HealthboxOptions_ModeFromToggles(bool32 bar, u32 value)
{
    if (bar)
    {
        switch (value)
        {
        case HB_HPVAL_NUMBERS: return OPTIONS_HP_DISPLAY_BAR_NUMBERS;
        case HB_HPVAL_PERCENT: return OPTIONS_HP_DISPLAY_BAR_PERCENT;
        default:               return OPTIONS_HP_DISPLAY_BAR_ONLY;
        }
    }
    switch (value)
    {
    case HB_HPVAL_NUMBERS: return OPTIONS_HP_DISPLAY_NUMBERS;
    case HB_HPVAL_PERCENT: return OPTIONS_HP_DISPLAY_PERCENT;
    default:               return OPTIONS_HP_DISPLAY_NONE;
    }
}

void HealthboxOptions_TogglesFromMode(u32 mode, bool32 *bar, u32 *value)
{
    switch (mode)
    {
    case OPTIONS_HP_DISPLAY_BAR_NUMBERS: *bar = TRUE;  *value = HB_HPVAL_NUMBERS; break;
    case OPTIONS_HP_DISPLAY_BAR_PERCENT: *bar = TRUE;  *value = HB_HPVAL_PERCENT; break;
    case OPTIONS_HP_DISPLAY_BAR_ONLY:    *bar = TRUE;  *value = HB_HPVAL_NONE;    break;
    case OPTIONS_HP_DISPLAY_NUMBERS:     *bar = FALSE; *value = HB_HPVAL_NUMBERS; break;
    case OPTIONS_HP_DISPLAY_PERCENT:     *bar = FALSE; *value = HB_HPVAL_PERCENT; break;
    default:                             *bar = FALSE; *value = HB_HPVAL_NONE;    break;
    }
}

void HealthboxOptions_SetHpToggles(struct HealthboxOptions *options, u32 side, bool32 bar, u32 value)
{
    if (side == HB_SIDE_PLAYER)
    {
        options->playerHpBar = bar;
        options->playerHpValue = value;
    }
    else
    {
        options->foeHpBar = bar;
        options->foeHpValue = value;
    }
}

static void FillDefaults(struct HealthboxOptions *o, u32 playerHpMode, u32 foeHpMode)
{
    bool32 bar;
    u32 value;

    *o = (struct HealthboxOptions){0};
    o->style = HEALTHBOX_STYLE_DEFAULT;
    o->background = HB_BG_SOLID;

    o->playerNick = o->playerLevel = o->playerStatus = o->playerTypes = o->playerStatStages = o->playerExp = TRUE;
    o->foeNick = o->foeLevel = o->foeStatus = o->foeTypes = o->foeStatStages = o->foeCaught = TRUE;

    HealthboxOptions_TogglesFromMode(playerHpMode, &bar, &value);
    HealthboxOptions_SetHpToggles(o, HB_SIDE_PLAYER, bar, value);
    HealthboxOptions_TogglesFromMode(foeHpMode, &bar, &value);
    HealthboxOptions_SetHpToggles(o, HB_SIDE_FOE, bar, value);
}

void HealthboxOptions_Get(struct HealthboxOptions *out)
{
    if (gSaveBlock2Ptr->healthboxOptions.initialized)
        *out = gSaveBlock2Ptr->healthboxOptions;
    else
        FillDefaults(out, LegacyHpMode(HB_SIDE_PLAYER), LegacyHpMode(HB_SIDE_FOE));
}

void HealthboxOptions_Commit(const struct HealthboxOptions *options)
{
    gSaveBlock2Ptr->healthboxOptions = *options;
    gSaveBlock2Ptr->healthboxOptions.initialized = TRUE;
}

void HealthboxOptions_SetDefaults(void)
{
    struct HealthboxOptions o;

    FillDefaults(&o, OPTIONS_HP_DISPLAY_BAR_NUMBERS,
                 B_HP_PERCENTAGE_DISPLAY ? OPTIONS_HP_DISPLAY_BAR_PERCENT : OPTIONS_HP_DISPLAY_BAR_ONLY);
    HealthboxOptions_Commit(&o);
}

u32 HealthboxOptions_GetStyle(void)
{
    struct HealthboxOptions o;

    HealthboxOptions_Get(&o);
    return o.style;
}

u32 HealthboxOptions_GetBackground(void)
{
    struct HealthboxOptions o;

    HealthboxOptions_Get(&o);
    return o.background;
}

bool32 HealthboxOptions_Shows(u32 side, enum HealthboxElement elem)
{
    struct HealthboxOptions o;

    HealthboxOptions_Get(&o);
    if (side == HB_SIDE_PLAYER)
    {
        switch (elem)
        {
        case HB_ELEM_NICK:        return o.playerNick;
        case HB_ELEM_LEVEL:       return o.playerLevel;
        case HB_ELEM_HP_BAR:      return o.playerHpBar;
        case HB_ELEM_EXP:         return o.playerExp;
        case HB_ELEM_STATUS:      return o.playerStatus;
        case HB_ELEM_TYPES:       return o.playerTypes;
        case HB_ELEM_STAT_STAGES: return o.playerStatStages;
        default:                  return FALSE;
        }
    }
    switch (elem)
    {
    case HB_ELEM_NICK:        return o.foeNick;
    case HB_ELEM_LEVEL:       return o.foeLevel;
    case HB_ELEM_HP_BAR:      return o.foeHpBar;
    case HB_ELEM_STATUS:      return o.foeStatus;
    case HB_ELEM_TYPES:       return o.foeTypes;
    case HB_ELEM_CAUGHT:      return o.foeCaught;
    case HB_ELEM_STAT_STAGES: return o.foeStatStages;
    default:                  return FALSE;
    }
}

u32 HealthboxOptions_GetHpValue(u32 side)
{
    struct HealthboxOptions o;

    HealthboxOptions_Get(&o);
    return (side == HB_SIDE_PLAYER) ? o.playerHpValue : o.foeHpValue;
}
