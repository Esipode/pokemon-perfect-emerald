#ifndef GUARD_TYPE_ICONS_H
#define GUARD_TYPE_ICONS_H

void LoadTypeIcons(enum BattlerId battler);
// Settings preview: creates a static icon in its slid-out position beside a box. Returns the sprite id.
u32 TypeIcons_CreatePreviewIcon(enum Type type, s32 boxLeft, s32 boxRight, s32 boxTop, s32 boxBottom, bool32 rightEdge, u32 typeNum);

// Copies a type's 8x16 battle icon (4bpp, linear rows; 0 transparent, 1 border, 2 glyph, other = type fill) for blitting onto a window.
#define TYPE_ICON_PIXEL_BYTES 64
void TypeIcons_GetPixels(enum Type type, u8 *dest);

#define TYPE_ICON_TAG 0x2720
#define TYPE_ICON_TAG_2 0x2721
#define NUM_FRAMES_HIDE_TYPE_ICON 10
#define TYPE_ICON_EDGE_INSET 5 // New style: icon centre offset from the box edge.

#define tMonPosition      data[0]
#define tBattlerId        data[1]
#define tHideIconTimer    data[2]
#define tVerticalPosition data[3]
#define tStartX           data[4]

#define TYPE_ICON_1_FRAME(monType) ((monType - 1) * 2)
#define TYPE_ICON_2_FRAME(monType) ((monType - 11) * 2)

#endif // GUARD_TYPE_ICONS_H
