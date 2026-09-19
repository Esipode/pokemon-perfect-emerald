#ifndef GUARD_ACHIEVEMENT_ICONS_H
#define GUARD_ACHIEVEMENT_ICONS_H

#define ACHIEVEMENT_ICON_SIZE 16

// Aligns an icon's body with the glyphs of a text line printed at textY.
#define ACHIEVEMENT_ICON_Y(textY) ((textY) - 1)

enum AchievementIconId
{
    ACHIEVEMENT_ICON_POINTS, // replaces the word "Points"/"pts" next to a point value
    ACHIEVEMENT_ICON_LOCK,   // replaces the word "LOCKED" on a not-yet-purchased binary boost
    ACHIEVEMENT_ICON_COUNT,
};

// Call once per menu init, after the menu has loaded its text palette into
// bgPaletteNum. Appends every icon's colours to that palette's unused high
// entries, so a later LoadPalette over those slots would undo it.
void AchievementIcons_Load(u8 bgPaletteNum);

// Draws the icon at window-local pixel coordinates.
void AchievementIcons_Blit(enum AchievementIconId icon, u8 windowId, u16 x, u16 y);

#endif // GUARD_ACHIEVEMENT_ICONS_H
