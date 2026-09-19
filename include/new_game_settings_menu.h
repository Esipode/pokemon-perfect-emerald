#ifndef GUARD_NEW_GAME_SETTINGS_MENU_H
#define GUARD_NEW_GAME_SETTINGS_MENU_H

// The four challenge modes are mutually exclusive, so the menu exposes them as one
// cycling row. ApplyPendingNewGameSettings fans it out to the separate save
// bytes (nuzlockeModeEnabled, draftModeEnabled, recruitsModeEnabled); the save
// format is unchanged.
enum GameMode
{
    GAME_MODE_NORMAL,
    GAME_MODE_NUZLOCKE,
    GAME_MODE_DRAFT,
    GAME_MODE_RECRUITS,
    GAME_MODE_COUNT,
};

struct NewGameSettings
{
    u8 difficulty;          // DIFFICULTY_EASY/NORMAL/HARD
    u8 gameMode;            // GAME_MODE_NORMAL/NUZLOCKE/DRAFT/RECRUITS
    bool8 randomizeSpecies;
    bool8 randomizeTypes;
    bool8 randomizeMoves;
    bool8 allowStatEditor;
    bool8 levelCapOff;      // Same polarity as FLAG_LEVEL_CAP_OFF (flag ON means the level cap is disabled)
    bool8 debugMode;
    u8 monoType;            // TYPE_NONE ("OFF") or one of the 18 real types; see mono_type.h
    u8 monoGen;             // 0 ("OFF") or a generation 1-9; see mono_gen.h
    bool8 limitedParty;     // Limited Party challenge: caps the party below PARTY_SIZE; see limited_party.h
    bool8 rotationMode;     // Rotation Mode: free random party switch each turn; see rotation_mode.h
};

extern struct NewGameSettings gPendingNewGameSettings;

void CB2_InitNewGameSettingsMenu(void);
void ApplyPendingNewGameSettings(void);
void CaptureCurrentSaveIntoPendingNewGameSettings(void);

#endif // GUARD_NEW_GAME_SETTINGS_MENU_H
