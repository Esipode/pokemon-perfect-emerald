#ifndef GUARD_ACHIEVEMENT_BOOST_MENU_H
#define GUARD_ACHIEVEMENT_BOOST_MENU_H

// Stage 7 (design doc Stage 7 / plan §7): a flat, scrollable list of every
// boost (no tier grouping -- boosts aren't tiered like achievements are)
// with [A] to purchase the highlighted boost's next level and [B] to go
// back. Reached from the achievements menu's TIER SELECT screen
// (src/achievements_menu.c) once boosts are both unlocked and enabled;
// also reachable directly from the debug menu for testing.
void CB2_InitAchievementBoostMenu(void);

#endif // GUARD_ACHIEVEMENT_BOOST_MENU_H
