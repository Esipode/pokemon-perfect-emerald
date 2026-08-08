#ifndef GUARD_KEEP_STORAGE_PROMPT_H
#define GUARD_KEEP_STORAGE_PROMPT_H

// Answer to the keep-storage prompt. EWRAM, not a field on struct NewGameSettings --
// CB2_InitNewGameSettingsMenu re-initializes gPendingNewGameSettings on entry, which
// runs right after this prompt and would clobber the answer. Consumed (and cleared)
// by NewGameInitData. See also gSaveBlock2Ptr->keepStorageOnRestart, which records
// whether the *current* run's storage was itself carried over from its predecessor --
// it gates the OT-ID lock in pokemon_storage_system.c, but is deliberately NOT read
// as a prediction of this answer on the Nuzlocke-restart path (field_screen_effect.c):
// on a player's first-ever run it's unconditionally FALSE, so that would silently
// drop real PC storage the first time someone fails and restarts. That path asks
// fresh via this same prompt's message/Yes-No instead.
extern bool8 gKeepStorageOnNewGame;

void CB2_InitKeepStoragePrompt(void);

#endif // GUARD_KEEP_STORAGE_PROMPT_H
