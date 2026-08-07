#include "global.h"
#include "main.h"
#include "battle.h"
#include "event_data.h"
#include "ai_battles.h"
#include "constants/battle.h"
#include "constants/flags.h"

// See include/ai_battles.h for the layer contract this file implements.

static const u16 sSettingFlags[AI_BATTLES_SETTING_COUNT] =
{
    [AI_BATTLES_SETTING_TRAINER] = FLAG_AI_BATTLES,
    [AI_BATTLES_SETTING_WILD] = FLAG_AI_WILD_BATTLES,
};

// Layer 2 cache. Only valid while gMain.inBattle is set; AiBattles_IsActive() guards on that
// directly so nothing outside this file needs to know the cache exists.
static bool8 sSessionActive;
static bool8 sSessionTrainerBattle;

// --- Layer 1: persistent setting ---

bool32 AiBattles_GetSetting(enum AiBattlesSetting setting)
{
    return FlagGet(sSettingFlags[setting]);
}

void AiBattles_SetSetting(enum AiBattlesSetting setting, bool32 enabled)
{
    if (enabled)
        FlagSet(sSettingFlags[setting]);
    else
        FlagClear(sSettingFlags[setting]);
}

u32 AiBattles_BackupSettings(void)
{
    u32 backup = 0;
    enum AiBattlesSetting setting;

    for (setting = 0; setting < AI_BATTLES_SETTING_COUNT; setting++)
    {
        if (AiBattles_GetSetting(setting))
            backup |= 1 << setting;
    }
    return backup;
}

void AiBattles_RestoreSettings(u32 backup)
{
    enum AiBattlesSetting setting;

    for (setting = 0; setting < AI_BATTLES_SETTING_COUNT; setting++)
        AiBattles_SetSetting(setting, (backup & (1 << setting)) != 0);
}

// --- Layer 2: per-battle session state ---

static bool32 IsBattleTypeEligible(u32 battleType)
{
    // Battle types the player cannot be AI-driven through: they either have no
    // B_ACTION_USE_MOVE (Safari, Ghost), must replay recorded inputs exactly
    // (Recorded, Recorded Link), are refereed rather than played (Palace), or
    // are network battles where the local player must stay in control (Link).
    if (battleType & (BATTLE_TYPE_SAFARI
                    | BATTLE_TYPE_GHOST
                    | BATTLE_TYPE_CATCH_TUTORIAL
                    | BATTLE_TYPE_LINK
                    | BATTLE_TYPE_RECORDED
                    | BATTLE_TYPE_RECORDED_LINK
                    | BATTLE_TYPE_PALACE))
        return FALSE;
    return TRUE;
}

void AiBattles_BeginBattle(void)
{
    bool32 isTrainer = (gBattleTypeFlags & BATTLE_TYPE_TRAINER) != 0;

    sSessionTrainerBattle = isTrainer;
    sSessionActive = IsBattleTypeEligible(gBattleTypeFlags)
                  && AiBattles_GetSetting(isTrainer ? AI_BATTLES_SETTING_TRAINER : AI_BATTLES_SETTING_WILD);
}

bool32 AiBattles_IsActive(void)
{
    return gMain.inBattle && sSessionActive;
}

bool32 AiBattles_IsActiveTrainerBattle(void)
{
    return AiBattles_IsActive() && sSessionTrainerBattle;
}

// --- Layer 3: named behaviours ---
// All currently derive straight from AiBattles_IsActive(). Keeping them as separate,
// named functions is the point: a future behaviour that needs to diverge (e.g. auto-
// advance text but still prompt on send-out) becomes a one-line change here instead of
// a new ad-hoc condition at the call site.

bool32 AiBattles_ShouldAutoAdvanceText(void)
{
    return AiBattles_IsActive();
}

bool32 AiBattles_ShouldAutoConfirmSendOut(void)
{
    return AiBattles_IsActive();
}

bool32 AiBattles_ShouldAutoAdvanceLevelUpBox(void)
{
    return AiBattles_IsActive();
}

bool32 AiBattles_ForcesBattleStyleSet(void)
{
    return AiBattles_IsActive();
}
