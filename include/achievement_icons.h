#ifndef GUARD_ACHIEVEMENT_ICONS_H
#define GUARD_ACHIEVEMENT_ICONS_H

#define ACHIEVEMENT_ICON_SIZE 16

// Text printed at a window-local y sits a pixel or so below its line's top,
// and the icon art is inset by a pixel of its own, so passing a line's text y
// through this lines the icon's body up with the glyphs beside it.
#define ACHIEVEMENT_ICON_Y(textY) ((textY) - 1)

enum AchievementIconId
{
    ACHIEVEMENT_ICON_POINTS, // replaces the word "Points"/"pts" next to a point value
    ACHIEVEMENT_ICON_LOCK,   // replaces the word "LOCKED" on a not-yet-purchased binary boost
    ACHIEVEMENT_ICON_COUNT,
};

// Call once per menu init, after that menu has loaded its own text palette
// into bgPaletteNum -- this appends every icon's colours to that palette's
// unused high entries, so it must not be undone by a later LoadPalette over
// the same slots. See src/achievement_icons.c for why the icons can't just be
// blitted with their own palettes.
void AchievementIcons_Load(u8 bgPaletteNum);

// Draws the given icon into a window at window-local pixel coordinates.
// Cleared by the same FillWindowPixelBuffer that clears the window's text.
void AchievementIcons_Blit(enum AchievementIconId icon, u8 windowId, u16 x, u16 y);

#endif // GUARD_ACHIEVEMENT_ICONS_H
