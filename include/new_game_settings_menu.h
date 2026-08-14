#ifndef GUARD_NEW_GAME_SETTINGS_MENU_H
#define GUARD_NEW_GAME_SETTINGS_MENU_H

struct NewGameSettings
{
    u8 difficulty;          // DIFFICULTY_EASY/NORMAL/HARD
    bool8 nuzlockeEnabled;
    bool8 draftMode;        // Draft challenge mode: mutually exclusive with nuzlockeEnabled; see draft_mode.h
    bool8 randomizeSpecies;
    bool8 randomizeTypes;
    bool8 randomizeMoves;
    bool8 allowStatEditor;
    bool8 levelCapOff;      // Same polarity as FLAG_LEVEL_CAP_OFF (flag ON means the level cap is disabled)
    bool8 debugMode;
    u8 monoType;            // TYPE_NONE ("OFF") or one of the 18 real types; see mono_type.h
    u8 monoGen;             // 0 ("OFF") or a generation 1-9; see mono_gen.h
    bool8 limitedParty;     // Limited Party challenge: caps the party below PARTY_SIZE; see limited_party.h
};

extern struct NewGameSettings gPendingNewGameSettings;

void CB2_InitNewGameSettingsMenu(void);
void ApplyPendingNewGameSettings(void);
void CaptureCurrentSaveIntoPendingNewGameSettings(void);

#endif // GUARD_NEW_GAME_SETTINGS_MENU_H
