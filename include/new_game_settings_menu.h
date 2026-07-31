#ifndef GUARD_NEW_GAME_SETTINGS_MENU_H
#define GUARD_NEW_GAME_SETTINGS_MENU_H

struct NewGameSettings
{
    u8 difficulty;          // DIFFICULTY_EASY/NORMAL/HARD
    bool8 nuzlockeEnabled;
    bool8 randomizeSpecies;
    bool8 randomizeTypes;
    bool8 randomizeMoves;
    bool8 allowStatEditor;
    bool8 debugMode;
    bool8 levelCapOff;      // Same polarity as FLAG_LEVEL_CAP_OFF (flag ON means the level cap is disabled)
};

extern struct NewGameSettings gPendingNewGameSettings;

void CB2_InitNewGameSettingsMenu(void);
void ApplyPendingNewGameSettings(void);

#endif // GUARD_NEW_GAME_SETTINGS_MENU_H
