#include "global.h"
#include "battle.h"
#include "battle_action_menu.h"
#include "constants/rgb.h"

// Icon sheet: 16 px wide, see tools/battle_ui/build_action_menu_gfx.py for the layout.
const u32 gActionMenuIconsGfx[] = INCGFX_U32("graphics/battle_interface/action_menu_icons.png", ".4bpp");

// Panel base colours, indices 0-7 (indices 8-15 are filled from the menu hues).
static const u16 sActionMenuBasePalette[] = INCGFX_U16("graphics/battle_interface/action_menu.pal", ".gbapal");

#include "data/battle_action_menu.h"

const struct ActionMenuSlot *ActionMenu_GetSlot(enum ActionMenuId menuId, u32 slot)
{
    return &sActionMenus[menuId][slot];
}

// Same 2x2 rules as the classic menu; a move onto a non-navigable slot is refused.
u32 ActionMenu_GetNextSlot(enum ActionMenuId menuId, u32 slot, enum ActionMenuDirection direction)
{
    u32 next;

    switch (direction)
    {
    case ACTION_DIR_UP:
        next = (slot & 2) ? slot ^ 2 : slot;
        break;
    case ACTION_DIR_DOWN:
        next = !(slot & 2) ? slot ^ 2 : slot;
        break;
    case ACTION_DIR_LEFT:
        next = (slot & 1) ? slot ^ 1 : slot;
        break;
    default:
        next = !(slot & 1) ? slot ^ 1 : slot;
        break;
    }

    return sActionMenus[menuId][next].navigable ? next : slot;
}

struct ActionMenuRect ActionMenu_GetChipPixelRect(u32 slot)
{
    return sActionChipRects[slot];
}

struct ActionMenuRect ActionMenu_GetChipTileRect(u32 slot)
{
    struct ActionMenuRect px = sActionChipRects[slot];
    struct ActionMenuRect tiles = { px.left / 8, px.top / 8, px.right / 8, px.bottom / 8 };

    return tiles;
}

// Indices 0-6 are shared by both palettes; 7-15 carry the outline and the four slot hues.
void ActionMenu_BuildPalette(enum ActionMenuId menuId, bool32 lit, u16 *dest)
{
    u32 i;

    for (i = 0; i < ACTION_PALETTE_BASE_COLORS; i++)
        dest[i] = sActionMenuBasePalette[i];
    if (lit)
        dest[7] = sActionMenuLitOutline;

    for (i = 0; i < ACTION_MENU_SLOT_COUNT; i++)
    {
        const struct ActionMenuSlot *slot = &sActionMenus[menuId][i];
        const u16 *hue = lit ? slot->litHue : slot->idleHue;

        dest[8 + i * 2] = hue[0];
        dest[9 + i * 2] = hue[1];
    }
}
