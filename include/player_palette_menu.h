#ifndef GUARD_PLAYER_PALETTE_MENU_H
#define GUARD_PLAYER_PALETTE_MENU_H

// Full-screen menu for picking the player's Hair/Hat/Outfit/Accent hue and shade.
// Reads/writes gSaveBlock2Ptr-> playerColors (through Stage 1's src/player_customization.c)
// and reads gSaveBlock2Ptr->playerGender to pick which region table and avatar
// graphics to preview -- both must already be set by the time this runs.
// Follows the CB2_InitOptionMenu convention: the caller sets
// gMain.savedCallback to wherever the screen should return to (B, or A on
// CONFIRM) *before* calling SetMainCallback2(CB2_InitPlayerPaletteMenu).
void CB2_InitPlayerPaletteMenu(void);

#endif // GUARD_PLAYER_PALETTE_MENU_H
