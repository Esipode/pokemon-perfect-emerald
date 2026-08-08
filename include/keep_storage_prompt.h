#ifndef GUARD_KEEP_STORAGE_PROMPT_H
#define GUARD_KEEP_STORAGE_PROMPT_H

// Answer to the keep-storage prompt. EWRAM, not a field on struct NewGameSettings --
// CB2_InitNewGameSettingsMenu re-initializes gPendingNewGameSettings on entry, which
// runs right after this prompt and would clobber the answer. Consumed (and cleared)
// by NewGameInitData; see also gSaveBlock2Ptr->keepStorageOnRestart, which is what
// actually persists the choice across a session boundary.
extern bool8 gKeepStorageOnNewGame;

void CB2_InitKeepStoragePrompt(void);

#endif // GUARD_KEEP_STORAGE_PROMPT_H
