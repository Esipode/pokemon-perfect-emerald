#ifndef GUARD_ACHIEVEMENT_POPUP_H
#define GUARD_ACHIEVEMENT_POPUP_H

// Shows the popup immediately, ungated. Used by the queue below and by the
// debug menu's "Test Achievement Popup" action.
void ShowAchievementPopup(u16 achievementId);

// Same box as ShowAchievementPopup, showing a level cap increase. Shows
// immediately and ungated; real increases go through LevelCapPopup_Enqueue.
void ShowLevelCapPopup(u32 newLevelCap);

// Entry point for real awards. Pushes onto a ring buffer that
// AchievementPopup_UpdateQueue drains one at a time, once the previous popup
// has finished and the field is in a safe state.
void AchievementPopup_Enqueue(u16 achievementId);

// Same as AchievementPopup_Enqueue for the level cap increase notification.
// Shares the queue, so popups still display one at a time.
void LevelCapPopup_Enqueue(u32 newLevelCap);

// Polled once per frame from CB2_Overworld. Shows the next queued popup if the
// field is in a safe state. A plain poll rather than a task because
// ResetTasks() during battle/menu transitions would destroy the task.
void AchievementPopup_UpdateQueue(void);

#endif // GUARD_ACHIEVEMENT_POPUP_H
