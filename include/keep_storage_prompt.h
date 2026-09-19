#ifndef GUARD_KEEP_STORAGE_PROMPT_H
#define GUARD_KEEP_STORAGE_PROMPT_H

// Answer to the keep-storage prompt. EWRAM, not a struct NewGameSettings field:
// CB2_InitNewGameSettingsMenu re-initializes gPendingNewGameSettings right after
// this prompt and would clobber it. Consumed (and cleared) by NewGameInitData.
//
// gSaveBlock2Ptr->keepStorageOnRestart records whether the current run's storage
// was carried over; it gates the OT-ID lock in pokemon_storage_system.c. It is
// deliberately NOT used to predict this answer on the Nuzlocke-restart path
// (field_screen_effect.c): it is FALSE on a first-ever run, which would silently
// drop real PC storage. That path asks via this prompt instead.
extern bool8 gKeepStorageOnNewGame;

void CB2_InitKeepStoragePrompt(void);

#endif // GUARD_KEEP_STORAGE_PROMPT_H
