#include "global.h"
#include "bg.h"
#include "decompress.h"
#include "gpu_regs.h"
#include "infinity_cave.h"
#include "main.h"
#include "malloc.h"
#include "menu.h"
#include "palette.h"
#include "scanline_effect.h"
#include "sound.h"
#include "sprite.h"
#include "string_util.h"
#include "task.h"
#include "text.h"
#include "window.h"
#include "constants/rgb.h"
#include "constants/songs.h"

// Node screen: the descent's one decision point. It shows the options
// InfCave_RollNodeOptions offers for the next depth and takes the player's pick.
//
// Skeleton copied from src/achievement_boost_menu.c (staged CB2 init, bg1 art
// layer, bg0 windows), with the list menu replaced by a hand-laid card column:
// the cards vary in number and position, which a ListMenu cannot express.
//
// bg1 carries the whole picture. Its base tilemap
// (graphics/infinity_cave/ui/node_tileset.bin) draws the header band, the cave
// backdrop and the footer panel; the cards and the branch lines are stamped over
// it at runtime from named tiles at the head of the same tileset, since their
// row and column depend on how many options were rolled.
//
// bg0 holds the header window, the footer window and one text window per card,
// all filled with PIXEL_FILL(0) so the art shows wherever there is no glyph ink.
// The card windows are added at runtime: their position follows the card layout.
//
// The room-type icons, the modifier icons and the "you are here" marker are OAM
// sprites rather than window blits. Blitting them would cost one BG palette slot
// per colour (see src/achievement_icons.c) and the sheet needs more than the
// eight the text palette leaves free.
//
// B is refused: the node is chosen before the room is generated, so there is no
// earlier state to back out to.

enum
{
    WIN_HEADER,
    WIN_FOOTER,
};

#define NODE_TAG_ICONS 7010

// Named tiles at the head of the tileset, in the order
// graphics/infinity_cave/ui/node_tileset.png stores them.
#define NODE_TILE_BAR          0  // 9-slice card bar, NW..SE
#define NODE_TILE_BAR_SELECTED 9  // the same nine slices in the selected colours
#define NODE_TILE_TRUNK        18 // branch line, vertical
#define NODE_TILE_JUNCTION     19 // vertical with a stub leaving to the right
#define NODE_TILE_STUB         20 // branch line, horizontal
#define NODE_TILE_TRUNK_TOP    21 // trunk end cap, line continuing downward
#define NODE_TILE_TRUNK_BOTTOM 22 // trunk end cap, line continuing upward

// Icon sheet frames: the room-type icons first, indexed by InfCaveRoomInfo.icon,
// then the modifier icons, indexed by InfCaveModifierInfo.icon.
#define NODE_ICON_ROOM_BASE     0
#define NODE_ICON_MODIFIER_BASE INFCAVE_ROOM_COUNT
#define NODE_ICON_MARKER        (NODE_ICON_MODIFIER_BASE + INFCAVE_MOD_COUNT - 1)
#define NODE_ICON_TILES         4 // 16x16 at 4bpp

// Card geometry, in background tiles. A card is one bar 20 tiles wide and 3 tall.
#define NODE_CARD_W       20
#define NODE_CARD_H       3
#define NODE_AREA_TOP     2  // first row below the header band
#define NODE_AREA_ROWS    15 // rows down to the footer panel
#define NODE_TRUNK_COL    4
#define NODE_CARD_COL     7
#define NODE_CARD_STAGGER 2  // odd cards sit this far right, so the column reads as a branch

// Card interior, in pixels from the bar's top-left corner.
#define NODE_ICON_X       4
#define NODE_ICON_Y       4
#define NODE_NAME_COL     3  // first text tile, clear of the room-type icon
#define NODE_NAME_W       13 // text window width; stops short of the modifier icons
#define NODE_NAME_Y       4
#define NODE_MOD_RIGHT_X  (NODE_CARD_W * 8 - 22) // rightmost modifier icon
#define NODE_MOD_PITCH    20

#define NODE_MARKER_X     20 // left of the trunk, vertically centred on it

#define NODE_HEADER_DEPTH_X  4
#define NODE_HEADER_SHARD_X  80
#define NODE_HEADER_WIDTH    (28 * 8) // WIN_HEADER, for right-aligning the boss countdown
#define NODE_HEADER_MARGIN   4
#define NODE_FOOTER_LINE1_Y  1
#define NODE_FOOTER_LINE2_Y  11
#define NODE_FOOTER_HINT_X   180
#define NODE_FOOTER_HINT_Y   3   // sits above the tag row so it clears the screen edge

// "MONOTYPE" plus a parenthesised type name plus the separator, twice over.
#define NODE_TAG_BUFFER_SIZE 64

EWRAM_DATA static u8 *sNodeBg1Tilemap = NULL;
EWRAM_DATA static struct InfCaveNodeOption sNodeOptions[INFCAVE_MAX_OPTIONS] = {0};
EWRAM_DATA static u8 sNodeCardWindows[INFCAVE_MAX_OPTIONS] = {0};
// The marker plus, per card, one room icon and one icon per filled modifier slot.
EWRAM_DATA static u8 sNodeSprites[1 + INFCAVE_MAX_OPTIONS * (1 + INFCAVE_MAX_MODIFIERS)] = {0};
EWRAM_DATA static u8 sNodeSpriteCount = 0;
EWRAM_DATA static u8 sNodeOptionCount = 0;
EWRAM_DATA static u8 sNodeSelected = 0;
EWRAM_DATA static u8 sNodeChosen = 0;
EWRAM_DATA static u8 sNodeTagBuffer[NODE_TAG_BUFFER_SIZE] = {0};

static void Task_NodeScreenFadeIn(u8 taskId);
static void Task_NodeScreenInput(u8 taskId);
static void Task_NodeScreenClose(u8 taskId);
static void NodeCardSlot(u32 index, u32 *row, u32 *col);
static void StampCards(void);
static void StampBranch(void);
static void CreateNodeSprites(void);
static void AddCardWindows(void);
static void DrawHeader(void);
static void DrawCardNames(void);
static void DrawFooter(void);

static const u8 sText_NodeDepth[]  = _("DEPTH {STR_VAR_1}");
static const u8 sText_NodeShards[] = _("SHARDS {STR_VAR_1}");
static const u8 sText_NodeBossIn[] = _("BOSS IN {STR_VAR_1}");
static const u8 sText_NodeBossNow[] = _("BOSS FLOOR");
static const u8 sText_NodeHint[]   = _("{A_BUTTON} GO");
static const u8 sText_NodeTagSeparator[] = _("  ");
static const u8 sText_NodeArgOpen[]  = _(" (");
static const u8 sText_NodeArgClose[] = _(")");

static const struct WindowTemplate sNodeWinTemplates[] =
{
    // The header band is the art's top two rows.
    [WIN_HEADER] = {
        .bg = 0,
        .tilemapLeft = 1,
        .tilemapTop = 0,
        .width = 28,
        .height = 2,
        .paletteNum = 1,
        .baseBlock = 1
    },
    // baseBlock follows WIN_HEADER's (1 + 28*2 tiles).
    [WIN_FOOTER] = {
        .bg = 0,
        .tilemapLeft = 1,
        .tilemapTop = NODE_AREA_TOP + NODE_AREA_ROWS,
        .width = 28,
        .height = 3,
        .paletteNum = 1,
        .baseBlock = 57
    },
    DUMMY_WIN_TEMPLATE
};

// Card windows are added after the two static ones, so their tiles start past
// WIN_FOOTER's (57 + 28*3).
#define NODE_CARD_WIN_BASE_BLOCK 141

static const struct BgTemplate sNodeBgTemplates[] =
{
    {
        // Art layer -- its own charBaseIndex, never touched by window text.
        .bg = 1,
        .charBaseIndex = 3,
        .mapBaseIndex = 30,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 1,
        .baseTile = 0
    },
    {
        // Windows only. Lower priority number than bg1 so text draws in front of
        // the art.
        .bg = 0,
        .charBaseIndex = 1,
        .mapBaseIndex = 31,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0
    }
};

static const u16 sNodeText_Pal[] = INCGFX_U16("graphics/interface/option_menu_text.pal", ".gbapal");

// {background, foreground, shadow}, as src/achievement_boost_menu.c: index 6 is
// the dark gray the TEXT_COLOR_* names do not reach in this palette.
static const u8 sNodeTextColors[3] = { TEXT_COLOR_TRANSPARENT, TEXT_COLOR_WHITE, 6 };

static const u32 sNodeScreenTiles[]   = INCBIN_U32("graphics/infinity_cave/ui/node_tileset.4bpp.smol");
static const u32 sNodeScreenTilemap[] = INCBIN_U32("graphics/infinity_cave/ui/node_tileset.bin.smolTM");
static const u16 sNodeScreenPal[]     = INCBIN_U16("graphics/infinity_cave/ui/node_tileset.gbapal");

static const u32 sNodeIconGfx[] = INCBIN_U32("graphics/infinity_cave/ui/node_icons.4bpp");
static const u16 sNodeIconPal[] = INCBIN_U16("graphics/infinity_cave/ui/node_icons.gbapal");

static const struct SpriteSheet sNodeIconSheet =
{
    .data = (const void *)sNodeIconGfx,
    .size = sizeof(sNodeIconGfx),
    .tag = NODE_TAG_ICONS,
};

static const struct SpritePalette sNodeIconPalette =
{
    .data = sNodeIconPal,
    .tag = NODE_TAG_ICONS,
};

static const struct OamData sNodeIconOam =
{
    .shape = SPRITE_SHAPE(16x16),
    .size = SPRITE_SIZE(16x16),
    .priority = 0,
};

// Every icon comes out of one sheet; the frame is picked by writing tileNum, so
// the template needs no anim table.
static const struct SpriteTemplate sNodeIconSpriteTemplate =
{
    .tileTag = NODE_TAG_ICONS,
    .paletteTag = NODE_TAG_ICONS,
    .oam = &sNodeIconOam,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy,
};

static void MainCB2(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    // Flushes bg1's art tilemap, which the stamping passes only schedule.
    DoScheduledBgTilemapCopiesToVram();
    UpdatePaletteFade();
}

static void VBlankCB(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

void CB2_InitInfCaveNodeScreen(void)
{
    switch (gMain.state)
    {
    default:
    case 0:
        SetVBlankCallback(NULL);
        sNodeOptionCount = InfCave_RollNodeOptions(sNodeOptions);
        sNodeSelected = 0;
        sNodeChosen = 0;
        gMain.state++;
        break;
    case 1:
        DmaClearLarge16(3, (void *)(VRAM), VRAM_SIZE, 0x1000);
        DmaClear32(3, OAM, OAM_SIZE);
        DmaClear16(3, PLTT, PLTT_SIZE);
        SetGpuReg(REG_OFFSET_DISPCNT, 0);
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(0, sNodeBgTemplates, ARRAY_COUNT(sNodeBgTemplates));
        sNodeBg1Tilemap = Alloc(0x800);
        memset(sNodeBg1Tilemap, 0, 0x800);
        SetBgTilemapBuffer(1, sNodeBg1Tilemap);
        ChangeBgX(0, 0, BG_COORD_SET);
        ChangeBgY(0, 0, BG_COORD_SET);
        ChangeBgX(1, 0, BG_COORD_SET);
        ChangeBgY(1, 0, BG_COORD_SET);
        InitWindows(sNodeWinTemplates);
        DeactivateAllTextPrinters();
        SetGpuReg(REG_OFFSET_WIN0H, 0);
        SetGpuReg(REG_OFFSET_WIN0V, 0);
        SetGpuReg(REG_OFFSET_WININ, 0);
        SetGpuReg(REG_OFFSET_WINOUT, 0);
        SetGpuReg(REG_OFFSET_BLDCNT, 0);
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
        ShowBg(0);
        ShowBg(1);
        gMain.state++;
        break;
    case 2:
        ResetPaletteFade();
        ScanlineEffect_Stop();
        ResetTasks();
        ResetSpriteData();
        FreeAllSpritePalettes();
        gMain.state++;
        break;
    case 3:
        DecompressAndCopyTileDataToVram(1, sNodeScreenTiles, 0, 0, 0);
        gMain.state++;
        break;
    case 4:
        FreeTempTileDataBuffersIfPossible();
        DecompressDataWithHeaderWram(sNodeScreenTilemap, sNodeBg1Tilemap);
        StampBranch();
        StampCards();
        ScheduleBgCopyTilemapToVram(1);
        LoadPalette(sNodeScreenPal, BG_PLTT_ID(0), PLTT_SIZE_4BPP);
        LoadPalette(sNodeText_Pal, BG_PLTT_ID(1), sizeof(sNodeText_Pal));
        gMain.state++;
        break;
    case 5:
        LoadSpriteSheet(&sNodeIconSheet);
        LoadSpritePalette(&sNodeIconPalette);
        CreateNodeSprites();
        gMain.state++;
        break;
    case 6:
        AddCardWindows();
        PutWindowTilemap(WIN_HEADER);
        PutWindowTilemap(WIN_FOOTER);
        DrawHeader();
        DrawCardNames();
        DrawFooter();
        CopyBgTilemapBufferToVram(0);
        gMain.state++;
        break;
    case 7:
        CreateTask(Task_NodeScreenFadeIn, 0);
        BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
        SetVBlankCallback(VBlankCB);
        SetMainCallback2(MainCB2);
        return;
    }
}

static void Task_NodeScreenFadeIn(u8 taskId)
{
    if (!gPaletteFade.active)
        gTasks[taskId].func = Task_NodeScreenInput;
}

static void MoveSelection(s32 delta)
{
    s32 next = sNodeSelected + delta;

    if (next < 0 || next >= sNodeOptionCount)
        return;

    sNodeSelected = next;
    PlaySE(SE_SELECT);
    StampCards();
    ScheduleBgCopyTilemapToVram(1);
    DrawCardNames();
    DrawFooter();
}

static void Task_NodeScreenInput(u8 taskId)
{
    if (JOY_NEW(DPAD_UP))
    {
        MoveSelection(-1);
    }
    else if (JOY_NEW(DPAD_DOWN))
    {
        MoveSelection(1);
    }
    else if (JOY_NEW(A_BUTTON))
    {
        sNodeChosen = sNodeSelected;
        PlaySE(SE_SELECT);
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        gTasks[taskId].func = Task_NodeScreenClose;
    }
    else if (JOY_NEW(B_BUTTON))
    {
        // Refused: a node has to be picked before the room behind it exists.
        PlaySE(SE_FAILURE);
    }
}

static void Task_NodeScreenClose(u8 taskId)
{
    if (gPaletteFade.active)
        return;

    while (sNodeSpriteCount > 0)
        DestroySprite(&gSprites[sNodeSprites[--sNodeSpriteCount]]);

    DestroyTask(taskId);
    FreeAllWindowBuffers();
    FreeSpriteTilesByTag(NODE_TAG_ICONS);
    FreeSpritePaletteByTag(NODE_TAG_ICONS);
    Free(sNodeBg1Tilemap);
    sNodeBg1Tilemap = NULL;
    SetMainCallback2(gMain.savedCallback);
}

// Row and column of a card's top-left tile. The pitch widens when fewer options
// were rolled, so three cards spread over the same band five fill.
static void NodeCardSlot(u32 index, u32 *row, u32 *col)
{
    u32 pitch = (sNodeOptionCount >= 5) ? 3 : (sNodeOptionCount == 4 ? 4 : 5);
    u32 span = (sNodeOptionCount - 1) * pitch + NODE_CARD_H;

    *row = NODE_AREA_TOP + (NODE_AREA_ROWS - span) / 2 + index * pitch;
    *col = NODE_CARD_COL + ((index & 1) ? NODE_CARD_STAGGER : 0);
}

// The card's middle row, which the branch line meets.
static u32 NodeCardMidRow(u32 index)
{
    u32 row, col;

    NodeCardSlot(index, &row, &col);
    return row + 1;
}

static void SetTile(u32 col, u32 row, u32 tile)
{
    ((u16 *)sNodeBg1Tilemap)[row * 32 + col] = tile;
}

// The trunk down the left edge plus one stub per card. Drawn once: neither
// depends on which card is selected.
static void StampBranch(void)
{
    u32 top = NodeCardMidRow(0);
    u32 bottom = NodeCardMidRow(sNodeOptionCount - 1);
    u32 i, row, col, x;

    for (row = top; row <= bottom; row++)
        SetTile(NODE_TRUNK_COL, row, NODE_TILE_TRUNK);

    for (i = 0; i < sNodeOptionCount; i++)
    {
        NodeCardSlot(i, &row, &col);
        row = NodeCardMidRow(i);

        for (x = NODE_TRUNK_COL + 1; x < col; x++)
            SetTile(x, row, NODE_TILE_STUB);

        // A single option leaves no trunk to cap, so its stub starts at the
        // junction tile and the marker sits directly beside it.
        if (sNodeOptionCount == 1)
            SetTile(NODE_TRUNK_COL, row, NODE_TILE_STUB);
        else if (i == 0)
            SetTile(NODE_TRUNK_COL, row, NODE_TILE_TRUNK_TOP);
        else if (i == sNodeOptionCount - 1)
            SetTile(NODE_TRUNK_COL, row, NODE_TILE_TRUNK_BOTTOM);
        else
            SetTile(NODE_TRUNK_COL, row, NODE_TILE_JUNCTION);
    }
}

// One bar per card, in the selected colours for the highlighted one. Idempotent,
// so a selection change re-stamps without restoring the base tilemap first.
static void StampCards(void)
{
    u32 i, row, col, dx, dy;

    for (i = 0; i < sNodeOptionCount; i++)
    {
        u32 base = (i == sNodeSelected) ? NODE_TILE_BAR_SELECTED : NODE_TILE_BAR;

        NodeCardSlot(i, &row, &col);
        for (dy = 0; dy < NODE_CARD_H; dy++)
        {
            for (dx = 0; dx < NODE_CARD_W; dx++)
            {
                u32 slice = (dx == 0) ? 0 : (dx == NODE_CARD_W - 1 ? 2 : 1);

                SetTile(col + dx, row + dy, base + dy * 3 + slice);
            }
        }
    }
}

// x and y are the icon's top-left corner; a 16x16 sprite is positioned by its
// centre. Every created sprite is tracked, since the screen tears them all down
// on close.
static void CreateIconSprite(u32 frame, s16 x, s16 y)
{
    u8 spriteId = CreateSprite(&sNodeIconSpriteTemplate, x + 8, y + 8, 0);

    if (spriteId == MAX_SPRITES)
        return;

    gSprites[spriteId].oam.tileNum = GetSpriteTileStartByTag(NODE_TAG_ICONS) + frame * NODE_ICON_TILES;
    sNodeSprites[sNodeSpriteCount++] = spriteId;
}

static void CreateNodeSprites(void)
{
    u32 i, slot, row, col;

    sNodeSpriteCount = 0;

    for (i = 0; i < sNodeOptionCount; i++)
    {
        const struct InfCaveRoomInfo *room = InfCave_GetRoomInfo(sNodeOptions[i].roomType);
        u32 iconCount = 0;

        NodeCardSlot(i, &row, &col);
        CreateIconSprite(NODE_ICON_ROOM_BASE + (room != NULL ? room->icon : 0),
                         col * 8 + NODE_ICON_X, row * 8 + NODE_ICON_Y);

        // Tags run right to left, so a one-modifier card puts its tag where a
        // two-modifier card puts its second.
        for (slot = 0; slot < INFCAVE_MAX_MODIFIERS; slot++)
        {
            const struct InfCaveModifierInfo *mod = InfCave_GetModifierInfo(sNodeOptions[i].modifier[slot]);

            if (mod == NULL)
                continue;

            CreateIconSprite(NODE_ICON_MODIFIER_BASE + mod->icon,
                col * 8 + NODE_MOD_RIGHT_X - iconCount * NODE_MOD_PITCH, row * 8 + NODE_ICON_Y);
            iconCount++;
        }
    }

    CreateIconSprite(NODE_ICON_MARKER, NODE_MARKER_X,
        (NodeCardMidRow(0) + NodeCardMidRow(sNodeOptionCount - 1)) * 4);
}

static void AddCardWindows(void)
{
    struct WindowTemplate template = sNodeWinTemplates[WIN_HEADER];
    u32 i, row, col;

    template.width = NODE_NAME_W;
    template.height = NODE_CARD_H;
    template.baseBlock = NODE_CARD_WIN_BASE_BLOCK;

    for (i = 0; i < sNodeOptionCount; i++)
    {
        NodeCardSlot(i, &row, &col);
        template.tilemapLeft = col + NODE_NAME_COL;
        template.tilemapTop = row;
        sNodeCardWindows[i] = AddWindow(&template);
        PutWindowTilemap(sNodeCardWindows[i]);
        template.baseBlock += NODE_NAME_W * NODE_CARD_H;
    }
}

static void PrintNodeText(u8 windowId, u32 fontId, const u8 *text, u8 x, u8 y)
{
    AddTextPrinterParameterized3(windowId, fontId, x, y, sNodeTextColors, TEXT_SKIP_DRAW, text);
}

static void DrawHeader(void)
{
    u32 depth = InfCave_GetNodeDepth();
    u32 toBoss = INFCAVE_BOSS_INTERVAL - (depth % INFCAVE_BOSS_INTERVAL);

    FillWindowPixelBuffer(WIN_HEADER, PIXEL_FILL(0));

    ConvertIntToDecimalStringN(gStringVar1, depth, STR_CONV_MODE_LEFT_ALIGN, 3);
    StringExpandPlaceholders(gStringVar4, sText_NodeDepth);
    PrintNodeText(WIN_HEADER, FONT_NORMAL, gStringVar4, NODE_HEADER_DEPTH_X, 0);

    ConvertIntToDecimalStringN(gStringVar1, InfCave_GetShards(), STR_CONV_MODE_LEFT_ALIGN, 5);
    StringExpandPlaceholders(gStringVar4, sText_NodeShards);
    PrintNodeText(WIN_HEADER, FONT_NORMAL, gStringVar4, NODE_HEADER_SHARD_X, 0);

    if (toBoss == INFCAVE_BOSS_INTERVAL)
    {
        StringCopy(gStringVar4, sText_NodeBossNow);
    }
    else
    {
        ConvertIntToDecimalStringN(gStringVar1, toBoss, STR_CONV_MODE_LEFT_ALIGN, 2);
        StringExpandPlaceholders(gStringVar4, sText_NodeBossIn);
    }

    // Right-aligned: "BOSS FLOOR" and "BOSS IN 9" are different widths, and a
    // fixed x would leave one of them either clipped or adrift.
    PrintNodeText(WIN_HEADER, FONT_NORMAL, gStringVar4,
                  NODE_HEADER_WIDTH - NODE_HEADER_MARGIN - GetStringWidth(FONT_NORMAL, gStringVar4, 0), 0);

    CopyWindowToVram(WIN_HEADER, COPYWIN_GFX);
}

static void DrawCardNames(void)
{
    u32 i;

    for (i = 0; i < sNodeOptionCount; i++)
    {
        const struct InfCaveRoomInfo *room = InfCave_GetRoomInfo(sNodeOptions[i].roomType);

        FillWindowPixelBuffer(sNodeCardWindows[i], PIXEL_FILL(0));
        if (room != NULL)
            PrintNodeText(sNodeCardWindows[i], FONT_NORMAL, room->name, 0, NODE_NAME_Y);
        CopyWindowToVram(sNodeCardWindows[i], COPYWIN_GFX);
    }
}

// "MONOTYPE (FIRE)  SURGE" for the selected card, or an empty string when it
// carries no modifiers.
static void BuildModifierTags(void)
{
    const struct InfCaveNodeOption *option = &sNodeOptions[sNodeSelected];
    u8 *end = sNodeTagBuffer;
    u32 slot;

    sNodeTagBuffer[0] = EOS;

    for (slot = 0; slot < INFCAVE_MAX_MODIFIERS; slot++)
    {
        const struct InfCaveModifierInfo *mod = InfCave_GetModifierInfo(option->modifier[slot]);
        const u8 *argName;

        if (mod == NULL)
            continue;

        if (end != sNodeTagBuffer)
            end = StringAppend(end, sText_NodeTagSeparator);

        end = StringAppend(end, mod->name);

        argName = InfCave_GetModifierArgName(option->modifier[slot], option->modifierArg[slot]);
        if (argName != NULL)
        {
            end = StringAppend(end, sText_NodeArgOpen);
            end = StringAppend(end, argName);
            end = StringAppend(end, sText_NodeArgClose);
        }
    }
}

static void DrawFooter(void)
{
    const struct InfCaveRoomInfo *room = InfCave_GetRoomInfo(sNodeOptions[sNodeSelected].roomType);

    FillWindowPixelBuffer(WIN_FOOTER, PIXEL_FILL(0));

    if (room != NULL)
        PrintNodeText(WIN_FOOTER, FONT_SMALL, room->description, 2, NODE_FOOTER_LINE1_Y);

    BuildModifierTags();
    PrintNodeText(WIN_FOOTER, FONT_SMALL, sNodeTagBuffer, 2, NODE_FOOTER_LINE2_Y);
    PrintNodeText(WIN_FOOTER, FONT_SMALL, sText_NodeHint, NODE_FOOTER_HINT_X, NODE_FOOTER_HINT_Y);

    CopyWindowToVram(WIN_FOOTER, COPYWIN_GFX);
}

u32 InfCave_GetChosenNodeIndex(void)
{
    return sNodeChosen;
}

const struct InfCaveNodeOption *InfCave_GetChosenNode(void)
{
    if (sNodeChosen >= sNodeOptionCount)
        return NULL;

    return &sNodeOptions[sNodeChosen];
}
