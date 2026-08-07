#ifndef GUARD_ACHIEVEMENT_POPUP_H
#define GUARD_ACHIEVEMENT_POPUP_H

// Reuses src/overworld.c's ScriptShowItemDescription/ShowItemIconSprite/
// ScriptHideItemDescription box almost verbatim -- same window position/
// size, same custom frame tile/palette, same icon-left/text-right layout, no
// slide animation. Only the content differs (tier icon + achievement name/
// points/description instead of an item icon + its description), and
// show/hide is driven by a task timer instead of paired script commands.
//
// Shows immediately and ungated -- used internally by the queue below, and
// directly by the debug menu's "Test Achievement Popup" action.
void ShowAchievementPopup(u16 achievementId);

// The real entry point for actual awards.
// src/achievements.c's QueueAchievementNotification calls this rather than
// ShowAchievementPopup directly -- it pushes onto a small ring buffer that
// AchievementPopup_UpdateQueue drains one at a time, only once the previous
// popup has finished and the field is in a safe state (not mid-battle,
// mid-cutscene, or mid-transition), so simultaneous awards each get a full,
// un-truncated display instead of clobbering one another.
void AchievementPopup_Enqueue(u16 achievementId);

// Polled once per frame from CB2_Overworld (src/overworld.c) -- attempts to
// show the next queued achievement popup, if any, and if the field is
// currently in a safe state to show one. A no-op when the queue is empty.
// Deliberately a plain per-frame poll rather than a task: see the comment on
// this function's definition (src/achievement_popup.c) for why a
// self-perpetuating task doesn't survive the ResetTasks() calls scattered
// through battle/menu transitions.
void AchievementPopup_UpdateQueue(void);

#endif // GUARD_ACHIEVEMENT_POPUP_H
