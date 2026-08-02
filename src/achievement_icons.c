#include "global.h"
#include "achievement_icons.h"
#include "palette.h"
#include "window.h"

// The achievements and boost menus show point values and a locked binary
// boost with these icons in place of words ("Points"/"pts", "LOCKED"). Each
// is blitted straight into the window's pixel buffer rather than created as
// an OAM sprite, so it behaves like the text it sits next to: it lands
// wherever GetStringWidth says the string before it ended, the same
// FillWindowPixelBuffer that clears the text clears it too, and it scrolls
// and clips with the window instead of floating over the screen in sprite
// coordinates (which is what src/achievement_popup.c's tier icon has to do,
// since it isn't part of any window's text).
//
// The catch is palette space. A window renders through one 16-colour BG
// palette -- both menus use BG palette 1, graphics/interface/option_menu_text.pal,
// whose entries 0-7 are its text colours and whose entries 8-15 are unused
// padding. Each icon has a palette of its own, so its pixel indices can't be
// blitted as they are; they'd come out in whatever the text colours happen to
// be at those indices. AchievementIcons_Load therefore copies every icon's
// colours into those spare high entries once, at menu init, and rewrites a
// private copy of each icon's pixels to point at them. The two icons here use
// 3 and 2 distinct non-transparent colours respectively, well inside the 8
// free slots.
//
// Index 0 is the one index that needs no slot: BlitBitmapToWindow (src/window.c)
// hands BlitBitmapRect4Bit a colorKey of 0, which skips source pixels of value
// 0 outright, so each icon's transparent index stays transparent over
// whatever the window already had underneath.

// Entries 0-7 belong to option_menu_text.pal's text colours; the icons' get
// appended from here up, points first then lock.
#define ICON_FIRST_FREE_PLTT_INDEX 8

#define ICON_BYTE_COUNT (ACHIEVEMENT_ICON_SIZE * ACHIEVEMENT_ICON_SIZE / 2) // 4bpp

// Uncompressed, unlike the popup's tier icons: these get read back a nibble
// at a time to build the remap below, so they have to be plain pixels rather
// than a .smol stream that only LoadCompressedSpriteSheet knows how to
// unpack.
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

// Builds one icon's remap and rewritten pixels, advancing *nextPlttIndex by
// however many new colours that icon actually needed -- shared across every
// icon so their remaps never collide.
static void LoadOneIcon(enum AchievementIconId icon, u8 bgPaletteNum, u32 *nextPlttIndex)
{
    const u8 *src = (const u8 *)sIconSources[icon].gfx;
    const u16 *pal = sIconSources[icon].pal;
    u8 remap[16] = {0};
    u32 i, nibble;

    // Built from the pixels rather than from the palette so only the colours
    // the art actually uses take up slots, and so re-exporting an icon with
    // its palette in a different order stays a pure asset change.
    for (i = 0; i < ICON_BYTE_COUNT; i++)
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
            // art problem rather than a code one; the surplus falls through
            // to transparent instead of overwriting the text colours below
            // ICON_FIRST_FREE_PLTT_INDEX or another icon's already-assigned
            // slots.
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
