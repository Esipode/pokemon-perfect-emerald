#ifndef GUARD_ACHIEVEMENT_POPUP_H
#define GUARD_ACHIEVEMENT_POPUP_H

// Stage 4.1 (design doc §4.1, revised during implementation): reuses
// src/overworld.c's ScriptShowItemDescription/ShowItemIconSprite/
// ScriptHideItemDescription box almost verbatim -- same window position/
// size, same custom frame tile/palette, same icon-left/text-right layout, no
// slide animation. Only the content differs (tier icon + achievement name/
// points/description instead of an item icon + its description), and
// show/hide is driven by a task timer instead of paired script commands.
//
// Shows immediately and ungated -- used internally by the Stage 4.2 queue
// below, and directly by the debug menu's "Test Achievement Popup" action.
void ShowAchievementPopup(u16 achievementId);

// Stage 4.2 (design doc §4.2): the real entry point for actual awards.
// src/achievements.c's QueueAchievementNotification calls this rather than
// ShowAchievementPopup directly -- it pushes onto a small ring buffer that a
// task drains one at a time, only once the previous popup has finished and
// the field is in a safe state (not mid-battle, mid-cutscene, or mid-
// transition), so simultaneous awards each get a full, un-truncated display
// instead of clobbering one another.
void AchievementPopup_Enqueue(u16 achievementId);

#endif // GUARD_ACHIEVEMENT_POPUP_H
