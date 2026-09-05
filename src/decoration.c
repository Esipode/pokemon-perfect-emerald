#include "global.h"
#include "malloc.h"
#include "decompress.h"
#include "decoration.h"
#include "decoration_inventory.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "event_scripts.h"
#include "field_camera.h"
#include "field_player_avatar.h"
#include "field_screen_effect.h"
#include "field_weather.h"
#include "fieldmap.h"
#include "graphics.h"
#include "international_string_util.h"
#include "item_icon.h"
#include "item_menu.h"
#include "list_menu.h"
#include "main.h"
#include "menu.h"
#include "menu_helpers.h"
#include "metatile_behavior.h"
#include "overworld.h"
#include "palette.h"
#include "player_pc.h"
#include "script.h"
#include "sound.h"
#include "string_util.h"
#include "strings.h"
#include "task.h"
#include "text.h"
#include "tilesets.h"
#include "trader.h"
#include "tv.h"
#include "constants/decorations.h"
#include "constants/event_objects.h"
#include "constants/songs.h"
#include "constants/region_map_sections.h"
#include "constants/metatile_labels.h"

#define tMenuTaskId data[13]

struct DecorationItemsMenu
{
    struct ListMenuItem items[41];
    u8 names[41][24];
    u8 numMenuItems;
    u8 maxShownItems;
    u8 scrollIndicatorsTaskId;
};

// Scratch buffer used to render a decoration's map tile graphic into a sprite
// icon for decorations that don't have dedicated icon art (icon.pic == NULL).
struct PlaceDecorationGraphicsDataBuffer
{
    const struct Decoration *decoration;
    u16 tiles[0x40];
    u8 image[0x800];
    u16 palette[16];
};

enum Windows
{
    WINDOW_DECORATION_CATEGORIES,
    WINDOW_DECORATION_CATEGORY_SUMMARY,
    WINDOW_DECORATION_CATEGORY_ITEMS,
    WINDOW_COUNT
};

EWRAM_DATA u8 *gCurDecorationItems = NULL;
EWRAM_DATA static u8 sNumOwnedDecorationsInCurCategory = 0;
EWRAM_DATA static u8 sSecretBaseItemsIndicesBuffer[DECOR_MAX_SECRET_BASE] = {};
EWRAM_DATA static u8 sPlayerRoomItemsIndicesBuffer[DECOR_MAX_PLAYERS_HOUSE] = {};
EWRAM_DATA static u16 sDecorationsCursorPos = 0;
EWRAM_DATA static u16 sDecorationsScrollOffset = 0;
EWRAM_DATA u8 gCurDecorationIndex = 0;
EWRAM_DATA static u8 sCurDecorationCategory = DECORCAT_DESK;
EWRAM_DATA static u8 sDecorMenuWindowIds[WINDOW_COUNT] = {};
EWRAM_DATA static struct DecorationItemsMenu *sDecorationItemsMenu = NULL;
EWRAM_DATA static struct PlaceDecorationGraphicsDataBuffer sPlaceDecorationGraphicsDataBuffer = {};
EWRAM_DATA static struct OamData sDecorSelectorOam = {};

static u8 AddDecorationWindow(u8 windowIndex);
static void RemoveDecorationWindow(u8 windowIndex);
static void InitDecorationCategoriesWindow(u8 taskId);
static void ReinitDecorationCategoriesWindow(u8 taskId);
static void PrintDecorationCategoryMenuItems(u8 taskId);
static void PrintDecorationCategoryMenuItem(u8 winid, u8 category, u8 x, u8 y, bool8 disabled, u8 speed);
static void ColorMenuItemString(u8 *str, bool8 disabled);
static void HandleDecorationCategoriesMenuInput(u8 taskId);
static void SelectDecorationCategory(u8 taskId);
static void ReturnToDecorationCategoriesAfterInvalidSelection(u8 taskId);
static void ExitDecorationCategoriesMenu(u8 taskId);
static void ExitTraderDecorationMenu(u8 taskId);
static void CopyDecorationMenuItemName(u8 *dest, u16 decoration);
static void DecorationItemsMenu_OnCursorMove(s32 itemIndex, bool8 flag, struct ListMenu *menu);
static void DecorationItemsMenu_PrintDecorationInUse(u8 windowId, u32 itemIndex, u8 y);
static void ShowDecorationItemsWindow(u8 taskId);
static void HandleDecorationItemsMenuInput(u8 taskId);
static void PrintDecorationItemDescription(s32 itemIndex);
static void RemoveDecorationItemsOtherWindows(void);
static bool8 IsDecorationIndexInSecretBase(u8 idx);
static bool8 IsDecorationIndexInPlayersRoom(u8 idx);
static void IdentifyOwnedDecorationsCurrentlyInUseInternal(u8 taskId);
static void IdentifyOwnedDecorationsCurrentlyInUse(u8 taskId);
static void InitDecorationItemsWindow(u8 taskId);
static void ShowDecorationCategorySummaryWindow(u8 category);
static void DecorationItemsMenuAction_Cancel(u8 taskId);
static void SetDecorSelectionBoxTiles(struct PlaceDecorationGraphicsDataBuffer *data);
static void CopyPalette(u16 *dest, u16 pal);
static void CopyTile(u8 *dest, u16 tile);
static u16 GetMetatile(u16 tile);
static void SetDecorSelectionMetatiles(struct PlaceDecorationGraphicsDataBuffer *data);
static void SetDecorSelectionBoxOamAttributes(u8 decorShape);
static void ClearPlaceDecorationGraphicsDataBuffer(struct PlaceDecorationGraphicsDataBuffer *data);
static u8 AddDecorationIconObjectFromIconTable(u16 tilesTag, u16 paletteTag, u8 decor);
static const u32 *GetDecorationIconPic(u16 decor);
static const u16 *GetDecorationIconPalette(u16 decor);
static u8 AddDecorationIconObjectFromObjectEvent(u16 tilesTag, u16 paletteTag, u8 decor);

#include "data/decoration/tiles.h"
#include "data/decoration/header.h"

static const u8 *const sDecorationCategoryNames[] =
{
    gText_Desk,
    gText_Chair,
    gText_Plant,
    gText_Ornament,
    gText_Mat,
    gText_Poster,
    gText_Doll,
    gText_Cushion
};

static const struct WindowTemplate sDecorationWindowTemplates[WINDOW_COUNT] =
{
    {
        .bg = 0,
        .tilemapLeft = 1,
        .tilemapTop = 1,
        .width = 13,
        .height = 18,
        .paletteNum = 13,
        .baseBlock = 0x0091
    },
    {
        .bg = 0,
        .tilemapLeft = 17,
        .tilemapTop = 1,
        .width = 12,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0x017b
    },
    {
        .bg = 0,
        .tilemapLeft = 16,
        .tilemapTop = 13,
        .width = 13,
        .height = 6,
        .paletteNum = 15,
        .baseBlock = 0x0193
    }
};

static const u16 sDecorationMenuPalette[] = INCGFX_U16("graphics/decorations/decoration_menu.pal", ".gbapal");

static const struct ListMenuTemplate sDecorationItemsListMenuTemplate =
{
    .items = NULL,
    .moveCursorFunc = DecorationItemsMenu_OnCursorMove,
    .itemPrintFunc = DecorationItemsMenu_PrintDecorationInUse,
    .totalItems = 0,
    .maxShowed = 0,
    .windowId = 0,
    .header_X = 0,
    .item_X = 8,
    .cursor_X = 0,
    .upText_Y = 9,
    .cursorPal = 2,
    .fillValue = 1,
    .cursorShadowPal = 3,
    .lettersSpacing = FALSE,
    .itemVerticalPadding = 0,
    .scrollMultiple = LIST_NO_MULTIPLE_SCROLL,
    .fontId = FONT_NARROW,
    .cursorKind = CURSOR_BLACK_ARROW,
};

#include "data/decoration/tilemaps.h"

struct DecorShape {
    u16 size;
    u16 width;
    u16 height;
    u8 spriteShape;
    u8 spriteSize;
    u8 cameraX;
    u8 cameraY;
};

static const struct DecorShape sDecorShapes[] = {
    [DECORSHAPE_1x1] = {
        .size = 4,
        .width = 1,
        .height = 1,
        .spriteShape = SPRITE_SHAPE(16x16),
        .spriteSize = SPRITE_SIZE(16x16),
        .cameraX = 120,
        .cameraY= 78,
    },

    [DECORSHAPE_2x1] = {
        .size = 8,
        .width = 2,
        .height = 1,
        .spriteShape = SPRITE_SHAPE(32x16),
        .spriteSize = SPRITE_SIZE(32x16),
        .cameraX = 128,
        .cameraY= 78,
    },

    [DECORSHAPE_3x1] = {
        .size = 16,
        .width = 3,
        .height = 1,
        .spriteShape = SPRITE_SHAPE(64x32),
        .spriteSize = SPRITE_SIZE(64x32),
        .cameraX = 144,
        .cameraY= 86,
    },

    [DECORSHAPE_4x2] = {
        .size = 32,
        .width = 4,
        .height = 2,
        .spriteShape = SPRITE_SHAPE(64x32),
        .spriteSize = SPRITE_SIZE(64x32),
        .cameraX = 144,
        .cameraY= 70,
    },

    [DECORSHAPE_2x2] = {
        .size = 16,
        .width = 2,
        .height = 2,
        .spriteShape = SPRITE_SHAPE(32x32),
        .spriteSize = SPRITE_SIZE(32x32),
        .cameraX = 128,
        .cameraY= 70,
    },

    [DECORSHAPE_1x2] = {
        .size = 8,
        .width = 1,
        .height = 2,
        .spriteShape = SPRITE_SHAPE(16x32),
        .spriteSize = SPRITE_SIZE(16x32),
        .cameraX = 120,
        .cameraY= 70,
    },

    [DECORSHAPE_1x3] = {
        .size = 16,
        .width = 1,
        .height = 3,
        .spriteShape = SPRITE_SHAPE(32x64),
        .spriteSize = SPRITE_SIZE(32x64),
        .cameraX = 128,
        .cameraY= 86,
    },

    [DECORSHAPE_2x4] = {
        .size = 32,
        .width = 2,
        .height = 4,
        .spriteShape = SPRITE_SHAPE(32x64),
        .spriteSize = SPRITE_SIZE(32x64),
        .cameraX = 128,
        .cameraY= 54,
    },

    [DECORSHAPE_3x3] = {
        .size = 64,
        .width = 3,
        .height = 3,
        .spriteShape = SPRITE_SHAPE(64x64),
        .spriteSize = SPRITE_SIZE(64x64),
        .cameraX = 144,
        .cameraY= 70,
    },

    [DECORSHAPE_3x2] = {
        .size = 32,
        .width = 3,
        .height = 2,
        .spriteShape = SPRITE_SHAPE(64x32),
        .spriteSize = SPRITE_SIZE(64x32),
        .cameraX = 144,
        .cameraY= 70,
    },
};

static const union AnimCmd sDecorSelectorAnimCmd0[] =
{
    ANIMCMD_FRAME(0, 0, FALSE, FALSE),
    ANIMCMD_END
};

static const union AnimCmd *const sDecorSelectorAnimCmds[] = { sDecorSelectorAnimCmd0 };

// Generic sprite template used to display a decoration icon rendered into
// sPlaceDecorationGraphicsDataBuffer (see AddDecorationIconObjectFromObjectEvent).
static const struct SpriteTemplate sDecorWhilePlacingSpriteTemplate =
{
    0x0000,
    0x0000,
    &sDecorSelectorOam,
    sDecorSelectorAnimCmds,
    NULL,
    gDummySpriteAffineAnimTable,
    SpriteCallbackDummy
};

void InitDecorationContextItems(void)
{
    if (sCurDecorationCategory < DECORCAT_COUNT)
        gCurDecorationItems = gDecorationInventories[sCurDecorationCategory].items;
}

static u8 AddDecorationWindow(u8 windowIndex)
{
    u8 *windowId = &sDecorMenuWindowIds[windowIndex];
    *windowId = AddWindow(&sDecorationWindowTemplates[windowIndex]);
    DrawStdFrameWithCustomTileAndPalette(*windowId, FALSE, 0x214, 14);
    ScheduleBgCopyTilemapToVram(0);
    return *windowId;
}

static void RemoveDecorationWindow(u8 windowIndex)
{
    ClearStdWindowAndFrameToTransparent(sDecorMenuWindowIds[windowIndex], FALSE);
    ClearWindowTilemap(sDecorMenuWindowIds[windowIndex]);
    RemoveWindow(sDecorMenuWindowIds[windowIndex]);
    ScheduleBgCopyTilemapToVram(0);
}

static void InitDecorationCategoriesWindow(u8 taskId)
{
    u8 windowId = AddDecorationWindow(WINDOW_DECORATION_CATEGORIES);
    PrintDecorationCategoryMenuItems(taskId);
    InitMenuInUpperLeftCornerNormal(windowId, DECORCAT_COUNT + 1, sCurDecorationCategory);
    gTasks[taskId].func = HandleDecorationCategoriesMenuInput;
}

static void ReinitDecorationCategoriesWindow(u8 taskId)
{
    FillWindowPixelBuffer(sDecorMenuWindowIds[WINDOW_DECORATION_CATEGORIES], PIXEL_FILL(1));
    PrintDecorationCategoryMenuItems(taskId);
    InitMenuInUpperLeftCornerNormal(sDecorMenuWindowIds[WINDOW_DECORATION_CATEGORIES], DECORCAT_COUNT + 1, sCurDecorationCategory);
    gTasks[taskId].func = HandleDecorationCategoriesMenuInput;
}

static void PrintDecorationCategoryMenuItems(u8 taskId)
{
    u8 i;
    u8 windowId = sDecorMenuWindowIds[WINDOW_DECORATION_CATEGORIES];

    for (i = 0; i < DECORCAT_COUNT; i++)
        PrintDecorationCategoryMenuItem(windowId, i, 8, i * 16, FALSE, TEXT_SKIP_DRAW);

    AddTextPrinterParameterized(windowId, FONT_NORMAL, gText_Exit, 8, i * 16 + 1, 0, NULL);
    ScheduleBgCopyTilemapToVram(0);
}

static void PrintDecorationCategoryMenuItem(u8 winid, u8 category, u8 x, u8 y, bool8 disabled, u8 speed)
{
    u8 width;
    u8 *str;

    width = x == 8 ? 104 : 96;
    y++;
    ColorMenuItemString(gStringVar4, disabled);
    str = StringLength(gStringVar4) + gStringVar4;
    StringCopy(str, sDecorationCategoryNames[category]);
    AddTextPrinterParameterized(winid, FONT_NORMAL, gStringVar4, x, y, speed, NULL);
    str = ConvertIntToDecimalStringN(str, GetNumOwnedDecorationsInCategory(category), STR_CONV_MODE_RIGHT_ALIGN, 2);
    *(str++) = CHAR_SLASH;
    ConvertIntToDecimalStringN(str, gDecorationInventories[category].size, STR_CONV_MODE_RIGHT_ALIGN, 2);
    x = GetStringRightAlignXOffset(FONT_NORMAL, gStringVar4, width);
    AddTextPrinterParameterized(winid, FONT_NORMAL, gStringVar4, x, y, speed, NULL);
}

static void ColorMenuItemString(u8 *str, bool8 disabled)
{
    StringCopy(str, gText_Color161Shadow161);
    if (disabled == TRUE)
    {
        str[2] = 4;
        str[5] = 5;
    }
    else
    {
        str[2] = 2;
        str[5] = 3;
    }
}

static void HandleDecorationCategoriesMenuInput(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        s8 input = Menu_ProcessInput();
        switch (input)
        {
        case MENU_B_PRESSED:
        case DECORCAT_COUNT: // CANCEL
            PlaySE(SE_SELECT);
            ExitDecorationCategoriesMenu(taskId);
            break;
        case MENU_NOTHING_CHOSEN:
            break;
        default:
            PlaySE(SE_SELECT);
            sCurDecorationCategory = input;
            SelectDecorationCategory(taskId);
            break;
        }
    }
}

static void SelectDecorationCategory(u8 taskId)
{
    sNumOwnedDecorationsInCurCategory = GetNumOwnedDecorationsInCategory(sCurDecorationCategory);
    if (sNumOwnedDecorationsInCurCategory != 0)
    {
        CondenseDecorationsInCategory(sCurDecorationCategory);
        gCurDecorationItems = gDecorationInventories[sCurDecorationCategory].items;
        IdentifyOwnedDecorationsCurrentlyInUse(taskId);
        sDecorationsScrollOffset = 0;
        sDecorationsCursorPos = 0;
        gTasks[taskId].func = ShowDecorationItemsWindow;
    }
    else
    {
        RemoveDecorationWindow(WINDOW_DECORATION_CATEGORIES);
        StringExpandPlaceholders(gStringVar4, gText_NoDecorations);
        DisplayItemMessageOnField(taskId, gStringVar4, ReturnToDecorationCategoriesAfterInvalidSelection);
    }
}

static void ReturnToDecorationCategoriesAfterInvalidSelection(u8 taskId)
{
    ClearDialogWindowAndFrame(0, FALSE);
    InitDecorationCategoriesWindow(taskId);
}

static void ExitDecorationCategoriesMenu(u8 taskId)
{
    ExitTraderDecorationMenu(taskId);
}

void ShowDecorationCategoriesWindow(u8 taskId)
{
    LoadPalette(sDecorationMenuPalette, BG_PLTT_ID(13), sizeof(sDecorationMenuPalette));
    ClearDialogWindowAndFrame(0, FALSE);
    sCurDecorationCategory = DECORCAT_DESK;
    InitDecorationCategoriesWindow(taskId);
}

void CopyDecorationCategoryName(u8 *dest, u8 category)
{
    StringCopy(dest, sDecorationCategoryNames[category]);
}

static void ExitTraderDecorationMenu(u8 taskId)
{
    RemoveDecorationWindow(WINDOW_DECORATION_CATEGORIES);
    ExitTraderMenu(taskId);
}

static void InitDecorationItemsMenuLimits(void)
{
    sDecorationItemsMenu->numMenuItems = sNumOwnedDecorationsInCurCategory + 1;
    if (sDecorationItemsMenu->numMenuItems > 8)
        sDecorationItemsMenu->maxShownItems = 8;
    else
        sDecorationItemsMenu->maxShownItems = sDecorationItemsMenu->numMenuItems;
}

static void InitDecorationItemsMenuScrollAndCursor(void)
{
    SetCursorWithinListBounds(&sDecorationsScrollOffset, &sDecorationsCursorPos, sDecorationItemsMenu->maxShownItems, sDecorationItemsMenu->numMenuItems);
}

static void InitDecorationItemsMenuScrollAndCursor2(void)
{
    SetCursorScrollWithinListBounds(&sDecorationsScrollOffset, &sDecorationsCursorPos, sDecorationItemsMenu->maxShownItems, sDecorationItemsMenu->numMenuItems, 8);
}

static void PrintDecorationItemMenuItems(u8 taskId)
{
    u16 i;

    ColorMenuItemString(gStringVar1, FALSE);

    for (i = 0; i < sDecorationItemsMenu->numMenuItems - 1; i++)
    {
        CopyDecorationMenuItemName(sDecorationItemsMenu->names[i], gCurDecorationItems[i]);
        sDecorationItemsMenu->items[i].name = sDecorationItemsMenu->names[i];
        sDecorationItemsMenu->items[i].id = i;
    }

    StringCopy(sDecorationItemsMenu->names[i], gText_Cancel);
    sDecorationItemsMenu->items[i].name = sDecorationItemsMenu->names[i];
    sDecorationItemsMenu->items[i].id = LIST_CANCEL;
    gMultiuseListMenuTemplate = sDecorationItemsListMenuTemplate;
    gMultiuseListMenuTemplate.windowId = sDecorMenuWindowIds[WINDOW_DECORATION_CATEGORIES];
    gMultiuseListMenuTemplate.totalItems = sDecorationItemsMenu->numMenuItems;
    gMultiuseListMenuTemplate.items = sDecorationItemsMenu->items;
    gMultiuseListMenuTemplate.maxShowed = sDecorationItemsMenu->maxShownItems;
}

static void CopyDecorationMenuItemName(u8 *dest, u16 decoration)
{
    StringCopy(dest, gStringVar1);
    StringAppend(dest, gDecorations[decoration].name);
}

static void DecorationItemsMenu_OnCursorMove(s32 itemIndex, bool8 flag, struct ListMenu *menu)
{
    if (flag != TRUE)
        PlaySE(SE_SELECT);

    PrintDecorationItemDescription(itemIndex);
}

static void DecorationItemsMenu_PrintDecorationInUse(u8 windowId, u32 itemIndex, u8 y)
{
    if (itemIndex != LIST_CANCEL)
    {
        if (IsDecorationIndexInSecretBase(itemIndex + 1) == TRUE)
            BlitMenuInfoIcon(windowId, MENU_INFO_ICON_BALL_RED, 92, y + 2);
        else if (IsDecorationIndexInPlayersRoom(itemIndex + 1) == TRUE)
            BlitMenuInfoIcon(windowId, MENU_INFO_ICON_BALL_BLUE, 92, y + 2);
    }
}

static void AddDecorationItemsScrollIndicators(void)
{
    if (sDecorationItemsMenu->scrollIndicatorsTaskId == TASK_NONE)
    {
        sDecorationItemsMenu->scrollIndicatorsTaskId = AddScrollIndicatorArrowPairParameterized(
            SCROLL_ARROW_UP,
            0x3c,
            0x0c,
            0x94,
            sDecorationItemsMenu->numMenuItems - sDecorationItemsMenu->maxShownItems,
            0x6e,
            0x6e,
            &sDecorationsScrollOffset);
    }
}

static void RemoveDecorationItemsScrollIndicators(void)
{
    if (sDecorationItemsMenu->scrollIndicatorsTaskId != TASK_NONE)
    {
        RemoveScrollIndicatorArrowPair(sDecorationItemsMenu->scrollIndicatorsTaskId);
        sDecorationItemsMenu->scrollIndicatorsTaskId = TASK_NONE;
    }
}

static void InitDecorationItemsWindow(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    AddDecorationWindow(WINDOW_DECORATION_CATEGORY_ITEMS);
    ShowDecorationCategorySummaryWindow(sCurDecorationCategory);
    sDecorationItemsMenu = AllocZeroed(sizeof(*sDecorationItemsMenu));
    sDecorationItemsMenu->scrollIndicatorsTaskId = TASK_NONE;
    InitDecorationItemsMenuLimits();
    InitDecorationItemsMenuScrollAndCursor();
    InitDecorationItemsMenuScrollAndCursor2();
    PrintDecorationItemMenuItems(taskId);
    tMenuTaskId = ListMenuInit(&gMultiuseListMenuTemplate, sDecorationsScrollOffset, sDecorationsCursorPos);
    AddDecorationItemsScrollIndicators();
}

static void ShowDecorationItemsWindow(u8 taskId)
{
    InitDecorationItemsWindow(taskId);
    gTasks[taskId].func = HandleDecorationItemsMenuInput;
}

static void HandleDecorationItemsMenuInput(u8 taskId)
{
    s16 *data;
    s32 input;

    data = gTasks[taskId].data;
    if (!gPaletteFade.active)
    {
        input = ListMenu_ProcessInput(tMenuTaskId);
        ListMenuGetScrollAndRow(tMenuTaskId, &sDecorationsScrollOffset, &sDecorationsCursorPos);
        switch (input)
        {
        case LIST_NOTHING_CHOSEN:
            break;
        case LIST_CANCEL:
            PlaySE(SE_SELECT);
            DecorationItemsMenuAction_Cancel(taskId);
            break;
        default:
            PlaySE(SE_SELECT);
            gCurDecorationIndex = input;
            RemoveDecorationItemsScrollIndicators();
            DestroyListMenuTask(tMenuTaskId, &sDecorationsScrollOffset, &sDecorationsCursorPos);
            RemoveDecorationWindow(WINDOW_DECORATION_CATEGORIES);
            RemoveDecorationItemsOtherWindows();
            Free(sDecorationItemsMenu);
            DecorationItemsMenuAction_Trade(taskId);
            break;
        }
    }
}

static void ShowDecorationCategorySummaryWindow(u8 category)
{
    PrintDecorationCategoryMenuItem(AddDecorationWindow(WINDOW_DECORATION_CATEGORY_SUMMARY), category, 0, 0, 0, 0);
}

static void PrintDecorationItemDescription(s32 itemIndex)
{
    u8 windowId;
    const u8 *str;

    windowId = sDecorMenuWindowIds[WINDOW_DECORATION_CATEGORY_ITEMS];
    FillWindowPixelBuffer(windowId, PIXEL_FILL(1));
    if ((u32)itemIndex >= sNumOwnedDecorationsInCurCategory)
        str = gText_GoBackPrevMenu;
    else
        str = gDecorations[gCurDecorationItems[itemIndex]].description;

    AddTextPrinterParameterized(windowId, FONT_NORMAL, str, 0, 1, 0, 0);
}

static void RemoveDecorationItemsOtherWindows(void)
{
    RemoveDecorationWindow(WINDOW_DECORATION_CATEGORY_ITEMS);
    RemoveDecorationWindow(WINDOW_DECORATION_CATEGORY_SUMMARY);
}

static bool8 IsDecorationIndexInSecretBase(u8 idx)
{
    u8 i;
    for (i = 0; i < ARRAY_COUNT(sSecretBaseItemsIndicesBuffer); i++)
    {
        if (sSecretBaseItemsIndicesBuffer[i] == idx)
            return TRUE;
    }

    return FALSE;
}

static bool8 IsDecorationIndexInPlayersRoom(u8 idx)
{
    u8 i;
    for (i = 0; i < ARRAY_COUNT(sPlayerRoomItemsIndicesBuffer); i++)
    {
        if (sPlayerRoomItemsIndicesBuffer[i] == idx)
            return TRUE;
    }

    return FALSE;
}

static void IdentifyOwnedDecorationsCurrentlyInUseInternal(u8 taskId)
{
    // UNUSED: if both FREE_SECRET_BASES and FREE_DECORATIONS are TRUE (the
    // default), neither loop below compiles and these go entirely unread.
    u16 UNUSED i;
    u16 UNUSED j;
    u16 UNUSED k;
    u16 UNUSED count;

    count = 0;
    memset(sSecretBaseItemsIndicesBuffer, 0, sizeof(sSecretBaseItemsIndicesBuffer));
    memset(sPlayerRoomItemsIndicesBuffer, 0, sizeof(sPlayerRoomItemsIndicesBuffer));

#if FREE_SECRET_BASES == FALSE
    for (i = 0; i < ARRAY_COUNT(sSecretBaseItemsIndicesBuffer); i++)
    {
        if (gSaveBlock1Ptr->secretBases[0].decorations[i] != DECOR_NONE)
        {
            for (j = 0; j < gDecorationInventories[sCurDecorationCategory].size; j++)
            {
                if (gCurDecorationItems[j] == gSaveBlock1Ptr->secretBases[0].decorations[i])
                {
                    for (k = 0; k < count && sSecretBaseItemsIndicesBuffer[k] != j + 1; k++)
                        ;

                    if (k == count)
                    {
                        sSecretBaseItemsIndicesBuffer[count] = j + 1;
                        count++;
                        break;
                    }
                }
            }
        }
    }
#endif //FREE_SECRET_BASES

    count = 0;
#if FREE_DECORATIONS == FALSE
    for (i = 0; i < ARRAY_COUNT(sPlayerRoomItemsIndicesBuffer); i++)
    {
        if (gSaveBlock1Ptr->playerRoomDecorations[i] != DECOR_NONE)
        {
            for (j = 0; j < gDecorationInventories[sCurDecorationCategory].size; j++)
            {
                if (gCurDecorationItems[j] == gSaveBlock1Ptr->playerRoomDecorations[i] && IsDecorationIndexInSecretBase(j + 1) != TRUE)
                {
                    for (k = 0; k < count && sPlayerRoomItemsIndicesBuffer[k] != j + 1; k++);
                    if (k == count)
                    {
                        sPlayerRoomItemsIndicesBuffer[count] = j + 1;
                        count++;
                        break;
                    }
                }
            }
        }
    }
#endif //FREE_DECORATIONS
}

static void IdentifyOwnedDecorationsCurrentlyInUse(u8 taskId)
{
    IdentifyOwnedDecorationsCurrentlyInUseInternal(taskId);
}

bool8 IsSelectedDecorInThePC(void)
{
    u16 i;
    for (i = 0; i < ARRAY_COUNT(sSecretBaseItemsIndicesBuffer); i++)
    {
        if (sSecretBaseItemsIndicesBuffer[i] == sDecorationsScrollOffset + sDecorationsCursorPos + 1)
            return FALSE;

        if (i < ARRAY_COUNT(sPlayerRoomItemsIndicesBuffer)
         && sPlayerRoomItemsIndicesBuffer[i] == sDecorationsScrollOffset + sDecorationsCursorPos + 1)
        {
            return FALSE;
        }
    }

    return TRUE;
}

static void DecorationItemsMenuAction_Cancel(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    RemoveDecorationItemsScrollIndicators();
    RemoveDecorationItemsOtherWindows();
    DestroyListMenuTask(tMenuTaskId, NULL, NULL);
    Free(sDecorationItemsMenu);
    ReinitDecorationCategoriesWindow(taskId);
}

// Input
// gSpecialVar_0x8004: Current iteration.
//
// Output
// gSpecialVar_Result: TRUE if all iterations complete.
// gSpecialVar_0x8005: flagId of decoration (if any).
void GetObjectEventLocalIdByFlag(void)
{
    u8 i;

    for (i = 0; i < gMapHeader.events->objectEventCount; i++)
    {
        if (gMapHeader.events->objectEvents[i].flagId == gSpecialVar_0x8004)
        {
            gSpecialVar_0x8005 = gMapHeader.events->objectEvents[i].localId;
            break;
        }
    }
}

static void SetDecorSelectionBoxTiles(struct PlaceDecorationGraphicsDataBuffer *data)
{
    u16 i;
    for (i = 0; i < 64; i++)
        CopyTile(&data->image[i * TILE_SIZE_4BPP], data->tiles[i]);
}

static void CopyPalette(u16 *dest, u16 pal)
{
    CpuFastCopy(&gTilesetPointer_SecretBase->palettes[pal], dest, PLTT_SIZE_4BPP);
}

static void CopyTile(u8 *dest, u16 tile)
{
    u8 ALIGNED(4) buffer[TILE_SIZE_4BPP];
    u16 mode;
    u16 i;

    mode = tile >> 10;
    if (tile != 0)
        tile &= 0x03FF;

    CpuFastCopy(&gTilesetPointer_SecretBase->tiles[tile * TILE_SIZE_4BPP / sizeof(u32)], buffer, TILE_SIZE_4BPP);
    switch (mode)
    {
    case 0:
        CpuFastCopy(buffer, dest, TILE_SIZE_4BPP);
        break;
    case BG_TILE_H_FLIP(0) >> 10:
        for (i = 0; i < 8; i++)
        {
            dest[4 * i + 0] = (buffer[4 * (i + 1) - 1] >> 4) + ((buffer[4 * (i + 1) - 1] & 0x0F) << 4);
            dest[4 * i + 1] = (buffer[4 * (i + 1) - 2] >> 4) + ((buffer[4 * (i + 1) - 2] & 0x0F) << 4);
            dest[4 * i + 2] = (buffer[4 * (i + 1) - 3] >> 4) + ((buffer[4 * (i + 1) - 3] & 0x0F) << 4);
            dest[4 * i + 3] = (buffer[4 * (i + 1) - 4] >> 4) + ((buffer[4 * (i + 1) - 4] & 0x0F) << 4);
        }
        break;
    case BG_TILE_V_FLIP(0) >> 10:
        for (i = 0; i < 8; i++)
        {
            dest[4 * i + 0] = buffer[4 * (7 - i) + 0];
            dest[4 * i + 1] = buffer[4 * (7 - i) + 1];
            dest[4 * i + 2] = buffer[4 * (7 - i) + 2];
            dest[4 * i + 3] = buffer[4 * (7 - i) + 3];
        }
        break;
    case BG_TILE_H_FLIP(BG_TILE_V_FLIP(0)) >> 10:
        for (i = 0; i < 32; i++)
        {
            dest[i] = (buffer[31 - i] >> 4) + ((buffer[31 - i] & 0x0F) << 4);
        }
        break;
    }
}

static u16 GetMetatile(u16 tile)
{
    return gTilesetPointer_SecretBaseRedCave->metatiles[tile] & 0xFFF;
}

static void SetDecorSelectionMetatiles(struct PlaceDecorationGraphicsDataBuffer *data)
{
    u8 i;
    u8 shape;

    shape = data->decoration->shape;
    for (i = 0; i < sDecorTilemaps[shape].size; i++)
    {
        data->tiles[sDecorTilemaps[shape].tiles[i]] = GetMetatile(data->decoration->tiles[sDecorTilemaps[shape].y[i]] * NUM_TILES_PER_METATILE + sDecorTilemaps[shape].x[i]);
    }
}

static void SetDecorSelectionBoxOamAttributes(u8 decorShape)
{
    sDecorSelectorOam.y = 0;
    sDecorSelectorOam.affineMode = ST_OAM_AFFINE_OFF;
    sDecorSelectorOam.objMode = ST_OAM_OBJ_NORMAL;
    sDecorSelectorOam.mosaic = FALSE;
    sDecorSelectorOam.bpp = ST_OAM_4BPP;
    sDecorSelectorOam.shape = sDecorShapes[decorShape].spriteShape;
    sDecorSelectorOam.x = 0;
    sDecorSelectorOam.matrixNum = 0;
    sDecorSelectorOam.size = sDecorShapes[decorShape].spriteSize;
    sDecorSelectorOam.tileNum = 0;
    sDecorSelectorOam.priority = 0;
    sDecorSelectorOam.paletteNum = 0;
}

static void ClearPlaceDecorationGraphicsDataBuffer(struct PlaceDecorationGraphicsDataBuffer *data)
{
    CpuFill16(0, data, sizeof(*data));
}

static u8 AddDecorationIconObjectFromIconTable(u16 tilesTag, u16 paletteTag, u8 decor)
{
    struct SpriteSheet sheet;
    struct SpritePalette palette;
    struct SpriteTemplate *template;
    u8 spriteId;

    if (!AllocItemIconTemporaryBuffers())
        return MAX_SPRITES;

    DecompressDataWithHeaderWram(GetDecorationIconPic(decor), gItemIconDecompressionBuffer);
    CopyItemIconPicTo4x4Buffer(gItemIconDecompressionBuffer, gItemIcon4x4Buffer);
    sheet.data = gItemIcon4x4Buffer;
    sheet.size = 0x200;
    sheet.tag = tilesTag;
    LoadSpriteSheet(&sheet);
    palette.data = GetDecorationIconPalette(decor);
    palette.tag = paletteTag;
    LoadSpritePalette(&palette);
    template = Alloc(sizeof(struct SpriteTemplate));
    *template = gItemIconSpriteTemplate;
    template->tileTag = tilesTag;
    template->paletteTag = paletteTag;
    spriteId = CreateSpriteUnchecked(template, 0, 0, 0);
    FreeItemIconTemporaryBuffers();
    Free(template);
    return spriteId;
}

static const u32 *GetDecorationIconPic(u16 decor)
{
    if (decor > NUM_DECORATIONS)
        decor = DECOR_NONE;

    return gDecorations[decor].icon.pic;
}

static const u16 *GetDecorationIconPalette(u16 decor)
{
    if (decor > NUM_DECORATIONS)
        decor = DECOR_NONE;

    return gDecorations[decor].icon.pal;
}

static u8 AddDecorationIconObjectFromObjectEvent(u16 tilesTag, u16 paletteTag, u8 decor)
{
    u8 spriteId;
    struct SpriteSheet sheet;
    struct SpritePalette palette;
    struct SpriteTemplate *template;

    ClearPlaceDecorationGraphicsDataBuffer(&sPlaceDecorationGraphicsDataBuffer);
    sPlaceDecorationGraphicsDataBuffer.decoration = &gDecorations[decor];
    if (sPlaceDecorationGraphicsDataBuffer.decoration->permission != DECORPERM_SPRITE)
    {
        SetDecorSelectionMetatiles(&sPlaceDecorationGraphicsDataBuffer);
        SetDecorSelectionBoxOamAttributes(sPlaceDecorationGraphicsDataBuffer.decoration->shape);
        SetDecorSelectionBoxTiles(&sPlaceDecorationGraphicsDataBuffer);
        CopyPalette(sPlaceDecorationGraphicsDataBuffer.palette, gTilesetPointer_SecretBaseRedCave->metatiles[(sPlaceDecorationGraphicsDataBuffer.decoration->tiles[0] * NUM_TILES_PER_METATILE) + 7] >> 12);
        sheet.data = sPlaceDecorationGraphicsDataBuffer.image;
        sheet.size = sDecorShapes[sPlaceDecorationGraphicsDataBuffer.decoration->shape].size * TILE_SIZE_4BPP;
        sheet.tag = tilesTag;
        LoadSpriteSheet(&sheet);
        palette.data = sPlaceDecorationGraphicsDataBuffer.palette;
        palette.tag = paletteTag;
        LoadSpritePalette(&palette);
        template = Alloc(sizeof(struct SpriteTemplate));
        *template = sDecorWhilePlacingSpriteTemplate;
        template->tileTag = tilesTag;
        template->paletteTag = paletteTag;
        spriteId = CreateSpriteUnchecked(template, 0, 0, 0);
        Free(template);
    }
    else
    {
        spriteId = CreateObjectGraphicsSpriteWithTag(sPlaceDecorationGraphicsDataBuffer.decoration->tiles[0], SpriteCallbackDummy, 0, 0, 1, paletteTag);
    }
    return spriteId;
}

u8 AddDecorationIconObject(u8 decor, s16 x, s16 y, u8 priority, u16 tilesTag, u16 paletteTag)
{
    u8 spriteId;

    if (decor > NUM_DECORATIONS)
    {
        spriteId = AddDecorationIconObjectFromIconTable(tilesTag, paletteTag, DECOR_NONE);
        if (spriteId == MAX_SPRITES)
            return MAX_SPRITES;

        gSprites[spriteId].x2 = x + 4;
        gSprites[spriteId].y2 = y + 4;
    }
    else if (gDecorations[decor].icon.pic == NULL)
    {
        spriteId = AddDecorationIconObjectFromObjectEvent(tilesTag, paletteTag, decor);
        if (spriteId == MAX_SPRITES)
            return MAX_SPRITES;

        gSprites[spriteId].x2 = x;
        if (decor == DECOR_SILVER_SHIELD || decor == DECOR_GOLD_SHIELD)
            gSprites[spriteId].y2 = y - 4;
        else
            gSprites[spriteId].y2 = y;
    }
    else
    {
        spriteId = AddDecorationIconObjectFromIconTable(tilesTag, paletteTag, decor);
        if (spriteId == MAX_SPRITES)
            return MAX_SPRITES;

        gSprites[spriteId].x2 = x + 4;
        gSprites[spriteId].y2 = y + 4;
    }

    gSprites[spriteId].oam.priority = priority;
    return spriteId;
}
