#ifndef GUARD_AI_BATTLES_H
#define GUARD_AI_BATTLES_H

// Single source of truth for "is the AI playing this battle for the player?"
//
// FLAG_AI_BATTLES and FLAG_AI_WILD_BATTLES must never be read with FlagGet/FlagSet
// outside src/ai_battles.c, and gBattleTypeFlags must never be tested directly to
// decide AI-vs-player control. Everything else goes through the three layers below.
//
//   Layer 1 (settings)  - what the player picked, persists across battles/saves.
//   Layer 2 (session)   - is AI driving *this* battle, cached once per battle.
//   Layer 3 (behaviour) - named yes/no questions UI and battle scripts can ask.

enum AiBattlesSetting
{
    AI_BATTLES_SETTING_TRAINER, // FLAG_AI_BATTLES
    AI_BATTLES_SETTING_WILD,    // FLAG_AI_WILD_BATTLES
    AI_BATTLES_SETTING_COUNT,
};

// --- Layer 1: persistent setting. Safe to call anywhere, including the overworld. ---
bool32 AiBattles_GetSetting(enum AiBattlesSetting setting);
void AiBattles_SetSetting(enum AiBattlesSetting setting, bool32 enabled);
u32 AiBattles_BackupSettings(void); // Returns an opaque bitmask for NG+ carry-over.
void AiBattles_RestoreSettings(u32 backup);

// --- Layer 2: per-battle session state. ---
void AiBattles_BeginBattle(void); // Caches the answer for this battle. Call once, after gBattleTypeFlags is final.
bool32 AiBattles_IsActive(void); // THE predicate: is the AI controlling the player's battlers right now?
bool32 AiBattles_IsActiveTrainerBattle(void); // Trainer-battle subset of AiBattles_IsActive(). Equivalent to the old IsAiVsAiBattle().

// --- Layer 3: named behaviours. ---
bool32 AiBattles_ShouldAutoAdvanceText(void); // Auto-scroll message boxes instead of waiting for A/B.
bool32 AiBattles_ShouldAutoConfirmSendOut(void); // Skip the "Use next POKEMON?" / "opponent is about to switch" yes/no prompts.
bool32 AiBattles_ShouldAutoAdvanceLevelUpBox(void); // Auto-advance/close the level-up stat box.
bool32 AiBattles_ForcesBattleStyleSet(void); // Force battle style to SET for the duration of this battle.

#endif // GUARD_AI_BATTLES_H
