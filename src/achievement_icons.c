#include "global.h"
#include "achievement_icons.h"
#include "palette.h"
#include "window.h"

// Icons are blitted into the window's pixel buffer rather than made OAM
// sprites, so they clear, scroll and clip with the window's text.
//
// A window renders through one 16-colour BG palette. Both menus use BG palette 1
// (graphics/interface/option_menu_text.pal): entries 0-7 are text colours,
// 8-15 are unused. Each icon has its own palette, so AchievementIcons_Load
// copies every icon's colours into the spare entries once at menu init and
// rewrites a private copy of each icon's pixels to point at them.
//
// Index 0 needs no slot: BlitBitmapToWindow passes a colorKey of 0, so
// transparent pixels stay transparent.

#define ICON_FIRST_FREE_PLTT_INDEX 8

#define ICON_BYTE_COUNT (ACHIEVEMENT_ICON_SIZE * ACHIEVEMENT_ICON_SIZE / 2) // 4bpp

// Uncompressed: the pixels are read back nibble by nibble to build the remap.
static const u32 sPointsIconGfx[] = INCGFX_U32("graphics/achievements/icons/points.png", ".4bpp");
static const u16 sPointsIconPal[] = INCGFX_U16("graphics/achievements/icons/points.png", ".gbapal");
static const u32 sLockIconGfx[]   = INCGFX_U32("graphics/achievements/icons/lock.png", ".4bpp");
static const u16 sLockIconPal[]   = INCGFX_U16("graphics/achievements/icons/lock.png", ".gbapal");

static const struct
{
    const u32 *gfx;
    const u16 *pal;
} sIconSources[ACHIEVEMENT_ICON_COUNT] =
{
    [ACHIEVEMENT_ICON_POINTS] = { sPointsIconGfx, sPointsIconPal },
    [ACHIEVEMENT_ICON_LOCK]   = { sLockIconGfx, sLockIconPal },
};

EWRAM_DATA static u8 sIconPixels[ACHIEVEMENT_ICON_COUNT][ICON_BYTE_COUNT] = {0};

// Advances *nextPlttIndex by the icon's colour count. Shared across icons so
// remaps never collide.
static void LoadOneIcon(enum AchievementIconId icon, u8 bgPaletteNum, u32 *nextPlttIndex)
{
    const u8 *src = (const u8 *)sIconSources[icon].gfx;
    const u16 *pal = sIconSources[icon].pal;
    u8 remap[16] = {0};
    u32 i, nibble;

    // Built from the pixels so only colours the art uses take up slots.
    for (i = 0; i < ICON_BYTE_COUNT; i++)
    {
        for (nibble = 0; nibble < 2; nibble++)
        {
            u32 value = (src[i] >> (nibble * 4)) & 0xF;

            // 0 is transparent and never gets a slot, so it doubles as the
            // "not assigned yet" marker in remap[].
            if (value == 0 || remap[value] != 0)
                continue;

            // Surplus colours fall through to transparent rather than
            // overwrite text colours or another icon's slots.
            if (*nextPlttIndex > 15)
                continue;

            remap[value] = *nextPlttIndex;
            LoadPalette(&pal[value], BG_PLTT_ID(bgPaletteNum) + *nextPlttIndex, PLTT_SIZEOF(1));
            (*nextPlttIndex)++;
        }
    }

    for (i = 0; i < ICON_BYTE_COUNT; i++)
        sIconPixels[icon][i] = remap[src[i] & 0xF] | (remap[(src[i] >> 4) & 0xF] << 4);
}

void AchievementIcons_Load(u8 bgPaletteNum)
{
    u32 nextPlttIndex = ICON_FIRST_FREE_PLTT_INDEX;
    enum AchievementIconId icon;

    for (icon = 0; icon < ACHIEVEMENT_ICON_COUNT; icon++)
        LoadOneIcon(icon, bgPaletteNum, &nextPlttIndex);
}

void AchievementIcons_Blit(enum AchievementIconId icon, u8 windowId, u16 x, u16 y)
{
    BlitBitmapToWindow(windowId, sIconPixels[icon], x, y, ACHIEVEMENT_ICON_SIZE, ACHIEVEMENT_ICON_SIZE);
}
