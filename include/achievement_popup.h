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
// Still not here:
//   - a real ring buffer for back-to-back awards (Stage 4.2). A second call
//     while one's showing currently just swaps the displayed content and
//     restarts the display timer.
//   - PlayFanfare(MUS_OBTAIN_SYMBOL) and suppressing the popup during
//     battles/cutscenes (Stage 4.2).
//   - QueueAchievementNotification in src/achievements.c does not call this
//     yet -- that hookup is Stage 4.2's, once the real queue exists.
void ShowAchievementPopup(u16 achievementId);

#endif // GUARD_ACHIEVEMENT_POPUP_H
