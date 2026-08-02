#include "global.h"
#include "achievement_icons.h"
#include "palette.h"
#include "window.h"

// The achievements and boost menus show point values with this icon in place
// of the words "Points"/"pts". It's blitted straight into the window's pixel
// buffer rather than created as an OAM sprite, so it behaves like the text it
// sits next to: it lands wherever GetStringWidth says the string before it
// ended, the same FillWindowPixelBuffer that clears the text clears it too,
// and it scrolls and clips with the window instead of floating over the
// screen in sprite coordinates (which is what src/achievement_popup.c's tier
// icon has to do, since it isn't part of any window's text).
//
// The catch is palette space. A window renders through one 16-colour BG
// palette -- both menus use BG palette 1, graphics/interface/option_menu_text.pal,
// whose entries 0-7 are its text colours and whose entries 8-15 are unused
// padding. The icon has a palette of its own, so its pixel indices can't be
// blitted as they are; they'd come out in whatever the text colours happen to
// be at those indices. AchievementIcons_LoadPointsIcon therefore copies the
// icon's colours into those spare high entries once, at menu init, and
// rewrites a private copy of its pixels to point at them.
//
// Index 0 is the one index that needs no slot: BlitBitmapToWindow (src/window.c)
// hands BlitBitmapRect4Bit a colorKey of 0, which skips source pixels of value
// 0 outright, so the icon's transparent index stays transparent over whatever
// the window already had underneath.

// Entries 0-7 belong to option_menu_text.pal's text colours; the icon's get
// appended from here up.
#define POINTS_ICON_FIRST_FREE_PLTT_INDEX 8

#define POINTS_ICON_BYTE_COUNT (ACHIEVEMENT_POINTS_ICON_SIZE * ACHIEVEMENT_POINTS_ICON_SIZE / 2) // 4bpp

// Uncompressed, unlike the popup's tier icons: this gets read back a nibble at
// a time to build the remap below, so it has to be plain pixels rather than a
// .smol stream that only LoadCompressedSpriteSheet knows how to unpack.
static const u32 sPointsIconGfx[] = INCGFX_U32("graphics/achievements/icons/points.png", ".4bpp");
static const u16 sPointsIconPal[] = INCGFX_U16("graphics/achievements/icons/points.png", ".gbapal");

EWRAM_DATA static u8 sPointsIconPixels[POINTS_ICON_BYTE_COUNT] = {0};

void AchievementIcons_LoadPointsIcon(u8 bgPaletteNum)
{
    const u8 *src = (const u8 *)sPointsIconGfx;
    u8 remap[16] = {0};
    u32 nextPlttIndex = POINTS_ICON_FIRST_FREE_PLTT_INDEX;
    u32 i, nibble;

    // Built from the pixels rather than from the palette so only the colours
    // the art actually uses take up slots, and so re-exporting points.png with
    // its palette in a different order stays a pure asset change.
    for (i = 0; i < POINTS_ICON_BYTE_COUNT; i++)
    {
        for (nibble = 0; nibble < 2; nibble++)
        {
            u32 value = (src[i] >> (nibble * 4)) & 0xF;

            // 0 is transparent (see the file comment) and never gets a slot,
            // which is also what makes it a safe "not assigned yet" marker in
            // remap[] -- no real assignment can produce 0.
            if (value == 0 || remap[value] != 0)
                continue;

            // More distinct colours than there are spare entries would be an
            // art problem rather than a code one; the surplus falls through to
            // transparent instead of overwriting the text colours below
            // POINTS_ICON_FIRST_FREE_PLTT_INDEX.
            if (nextPlttIndex > 15)
                continue;

            remap[value] = nextPlttIndex;
            LoadPalette(&sPointsIconPal[value], BG_PLTT_ID(bgPaletteNum) + nextPlttIndex, PLTT_SIZEOF(1));
            nextPlttIndex++;
        }
    }

    for (i = 0; i < POINTS_ICON_BYTE_COUNT; i++)
        sPointsIconPixels[i] = remap[src[i] & 0xF] | (remap[(src[i] >> 4) & 0xF] << 4);
}

void AchievementIcons_BlitPointsIcon(u8 windowId, u16 x, u16 y)
{
    BlitBitmapToWindow(windowId, sPointsIconPixels, x, y, ACHIEVEMENT_POINTS_ICON_SIZE, ACHIEVEMENT_POINTS_ICON_SIZE);
}
