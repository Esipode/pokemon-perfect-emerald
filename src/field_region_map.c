#include "global.h"
#include "bg.h"
#include "decompress.h"
#include "event_data.h"
#include "field_effect.h"
#include "gpu_regs.h"
#include "international_string_util.h"
#include "landmark.h"
#include "main.h"
#include "malloc.h"
#include "menu.h"
#include "overworld.h"
#include "palette.h"
#include "region_map.h"
#include "sound.h"
#include "sprite.h"
#include "string_util.h"
#include "strings.h"
#include "text.h"
#include "text_window.h"
#include "window.h"
#include "constants/region_map_sections.h"
#include "constants/rgb.h"
#include "constants/songs.h"

/*
 *  This is the type of map shown when interacting with the metatiles for
 *  a wall-mounted Region Map (on the wall of the Pokemon Centers near the PC)
 *  A zooms in/out (showing a city map or landmark list), B zooms out or closes the map,
 *  R flies to the selected city
 *
 *  For the region map in the pokedex, see pokdex_area_screen.c/pokedex_area_region_map.c
 *  For the fly map, and utility functions all of the maps use, see region_map.c
 */

enum {
    WIN_MAPSEC_NAME,
    WIN_TITLE,
};

enum {
    TAG_PLAYER_ICON,
    TAG_CURSOR,
    TAG_CITY_ZOOM = 6,
    TAG_CITY_ZOOM_PAL = 11,
};

#define NUM_CITY_MAPS 22

// BG1 offset of the info panel while it is off screen
#define INFO_PANEL_HIDDEN_Y (-0xA000)
#define INFO_PANEL_SCROLL_SPEED 0xA00

struct CityMapEntry
{
    mapsec_u16_t mapSecId;
    u16 index;
    const u32 *tilemap;
};

static EWRAM_DATA struct {
    MainCallback callback;
    u32 unused;
    struct RegionMap regionMap;
    u16 state;
    bool8 choseFlyDestination;
    bool8 zoomingIn;
    bool8 panelScrolling;
    bool8 cityTextHidden;
    u8 infoWindowId;
    struct Sprite *cityZoomTextSprites[3];
    u8 ALIGNED(2) tilemapBuffer[BG_SCREEN_SIZE];
    u8 ALIGNED(2) cityMapBuffer[200];
} *sFieldRegionMapHandler = NULL;

static void MCB2_InitRegionMapRegisters(void);
static void VBCB_FieldUpdateRegionMap(void);
static void MCB2_FieldUpdateRegionMap(void);
static void FieldUpdateRegionMap(void);
static void PrintRegionMapSecName();
static void PrintTitleWindowText();
static void LoadInfoPanelGfx(void);
static void FreeCityZoomViewGfx(void);
static void CreateCityZoomTextSprites(void);
static void SpriteCB_CityZoomText(struct Sprite *sprite);
static void UpdateMapSecInfoWindow(void);
static void StartZoom(void);
static bool32 ScrollInfoPanel(void);

static const u16 sMapSecInfoWindow_Pal[] = INCGFX_U16("graphics/region_map/info_window.pal", ".gbapal");
static const u16 sCityZoomTiles_Pal[] = INCGFX_U16("graphics/region_map/zoom_tiles.png", ".gbapal");
static const u32 sCityZoomTiles_Gfx[] = INCGFX_U32("graphics/region_map/zoom_tiles.png", ".4bpp.smol");
static const u32 sCityZoomText_Gfx[] = INCGFX_U32("graphics/region_map/city_zoom_text.png", ".4bpp.smol");

#include "data/region_map/city_map_tilemaps.h"
#include "data/region_map/city_map_entries.h"

static const struct CompressedSpriteSheet sCityZoomTextSpriteSheet =
{
    sCityZoomText_Gfx, 0x800, TAG_CITY_ZOOM
};

static const struct SpritePalette sCityZoomSpritePalette = {sCityZoomTiles_Pal, TAG_CITY_ZOOM_PAL};

static const struct OamData sCityZoomTextSprite_OamData =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(32x8),
    .x = 0,
    .size = SPRITE_SIZE(32x8),
    .tileNum = 0,
    .priority = 1,
    .paletteNum = 0,
};

static const struct SpriteTemplate sCityZoomTextSpriteTemplate =
{
    .tileTag = TAG_CITY_ZOOM,
    .paletteTag = TAG_CITY_ZOOM_PAL,
    .oam = &sCityZoomTextSprite_OamData,
    .callback = SpriteCB_CityZoomText,
};

static const struct WindowTemplate sMapSecInfoWindowTemplate =
{
    .bg = 1,
    .tilemapLeft = 17,
    .tilemapTop = 4,
    .width = 12,
    .height = 13,
    .paletteNum = 1,
    .baseBlock = 0x4C
};

static const struct BgTemplate sFieldRegionMapBgTemplates[] = {
    {
        .bg = 0,
        .charBaseIndex = 0,
        .mapBaseIndex = 31,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0
    }, {
        .bg = 1,
        .charBaseIndex = 1,
        .mapBaseIndex = 30,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 1,
        .baseTile = 0
    }, {
        .bg = 2,
        .charBaseIndex = 2,
        .mapBaseIndex = 28,
        .screenSize = 2,
        .paletteMode = 1,
        .priority = 2,
        .baseTile = 0
    }
};

static const struct WindowTemplate sFieldRegionMapWindowTemplates[] =
{
    [WIN_MAPSEC_NAME] = {
        .bg = 0,
        .tilemapLeft = 17,
        .tilemapTop = 17,
        .width = 12,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 1
    },
    [WIN_TITLE] = {
        .bg = 0,
        .tilemapLeft = 22,
        .tilemapTop = 1,
        .width = 7,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 25
    },
    DUMMY_WIN_TEMPLATE
};

void FieldInitRegionMap(MainCallback callback)
{
    SetVBlankCallback(NULL);
    sFieldRegionMapHandler = Alloc(sizeof(*sFieldRegionMapHandler));
    sFieldRegionMapHandler->state = 0;
    sFieldRegionMapHandler->callback = callback;
    sFieldRegionMapHandler->choseFlyDestination = FALSE;
    sFieldRegionMapHandler->zoomingIn = FALSE;
    sFieldRegionMapHandler->panelScrolling = FALSE;
    sFieldRegionMapHandler->cityTextHidden = TRUE;
    SetMainCallback2(MCB2_InitRegionMapRegisters);
}

static void MCB2_InitRegionMapRegisters(void)
{
    SetGpuReg(REG_OFFSET_DISPCNT, 0);
    SetGpuReg(REG_OFFSET_BG0HOFS, 0);
    SetGpuReg(REG_OFFSET_BG0VOFS, 0);
    SetGpuReg(REG_OFFSET_BG1HOFS, 0);
    SetGpuReg(REG_OFFSET_BG1VOFS, 0);
    SetGpuReg(REG_OFFSET_BG2HOFS, 0);
    SetGpuReg(REG_OFFSET_BG2VOFS, 0);
    SetGpuReg(REG_OFFSET_BG3HOFS, 0);
    SetGpuReg(REG_OFFSET_BG3VOFS, 0);
    ResetSpriteData();
    FreeAllSpritePalettes();
    ResetBgsAndClearDma3BusyFlags(0);
    InitBgsFromTemplates(1, sFieldRegionMapBgTemplates, ARRAY_COUNT(sFieldRegionMapBgTemplates));
    InitWindows(sFieldRegionMapWindowTemplates);
    DeactivateAllTextPrinters();
    LoadUserWindowBorderGfx(0, 0x27, BG_PLTT_ID(13));
    ClearScheduledBgCopiesToVram();
    SetMainCallback2(MCB2_FieldUpdateRegionMap);
    SetVBlankCallback(VBCB_FieldUpdateRegionMap);
}

static void VBCB_FieldUpdateRegionMap(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
    UpdateRegionMapVideoRegs();
}

static void MCB2_FieldUpdateRegionMap(void)
{
    FieldUpdateRegionMap();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
    DoScheduledBgTilemapCopiesToVram();
}

static void FieldUpdateRegionMap(void)
{
    switch (sFieldRegionMapHandler->state)
    {
    case 0:
        InitRegionMap(&sFieldRegionMapHandler->regionMap, FALSE);
        CreateRegionMapPlayerIcon(TAG_PLAYER_ICON, TAG_PLAYER_ICON);
        CreateRegionMapCursor(TAG_CURSOR, TAG_CURSOR);
        LoadCompressedSpriteSheet(&sCityZoomTextSpriteSheet);
        LoadSpritePalette(&sCityZoomSpritePalette);
        CreateCityZoomTextSprites();
        LoadInfoPanelGfx();
        sFieldRegionMapHandler->state++;
        break;
    case 1:
        DrawStdFrameWithCustomTileAndPalette(WIN_TITLE, FALSE, 0x27, 0xd);
        FillWindowPixelBuffer(WIN_TITLE, PIXEL_FILL(1));
        PrintTitleWindowText();
        ScheduleBgCopyTilemapToVram(0);
        DrawStdFrameWithCustomTileAndPalette(WIN_MAPSEC_NAME, FALSE, 0x27, 0xd);
        PrintRegionMapSecName();
        BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
        sFieldRegionMapHandler->state++;
        break;
    case 2:
        SetGpuRegBits(REG_OFFSET_DISPCNT, DISPCNT_OBJ_1D_MAP | DISPCNT_OBJ_ON);
        ShowBg(0);
        ShowBg(2);
        sFieldRegionMapHandler->state++;
        break;
    case 3:
        if (!gPaletteFade.active && !FreeTempTileDataBuffersIfPossible())
        {
            sFieldRegionMapHandler->state++;
        }
        break;
    case 4:
        switch (DoRegionMapInputCallback())
        {
        case MAP_INPUT_MOVE_END:
                PrintRegionMapSecName();
                PrintTitleWindowText();
                if (IsRegionMapZoomed())
                    UpdateMapSecInfoWindow();
                break;
        case MAP_INPUT_A_BUTTON:
                if (!IsEventIslandMapSecId(gMapHeader.regionMapSectionId))
                    StartZoom();
                break;
        case MAP_INPUT_B_BUTTON:
                if (IsRegionMapZoomed())
                    StartZoom();
                else
                {
                    sFieldRegionMapHandler->state++;
                }
                break;
        case MAP_INPUT_R_BUTTON:
                if (sFieldRegionMapHandler->regionMap.mapSecType == MAPSECTYPE_CITY_CANFLY
                    && FlagGet(FLAG_FLY_FROM_TOWN_MAP) && Overworld_MapTypeAllowsTeleportAndFly(gMapHeader.mapType) == TRUE)
                {
                    PlaySE(SE_SELECT);
                    SetFlyDestination(&sFieldRegionMapHandler->regionMap);
                    gSkipShowMonAnim = TRUE;
                    // Fly starts from the shared exit states so the map's
                    // resources are released and the screen is faded out first.
                    sFieldRegionMapHandler->choseFlyDestination = TRUE;
                    sFieldRegionMapHandler->state++;
                }
                break;
        }
        break;
    case 7:
        // Both must run every frame, so no short-circuit
        if (!(UpdateRegionMapZoom() | ScrollInfoPanel()))
        {
            PrintRegionMapSecName();
            PrintTitleWindowText();
            sFieldRegionMapHandler->state = 4;
        }
        break;
    case 5:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        sFieldRegionMapHandler->state++;
        break;
    case 6:
        if (!gPaletteFade.active)
        {
            FreeRegionMapIconResources();
            FreeCityZoomViewGfx();
            if (sFieldRegionMapHandler->choseFlyDestination)
                ReturnToFieldFromFlyMapSelect();
            else
                SetMainCallback2(sFieldRegionMapHandler->callback);
            TRY_FREE_AND_SET_NULL(sFieldRegionMapHandler);
            FreeAllWindowBuffers();
        }
        break;
    }
}

static void PrintRegionMapSecName(void)
{
    if (sFieldRegionMapHandler->regionMap.mapSecType != MAPSECTYPE_NONE)
    {
        FillWindowPixelBuffer(WIN_MAPSEC_NAME, PIXEL_FILL(1));
        AddTextPrinterParameterized(WIN_MAPSEC_NAME, FONT_NORMAL, sFieldRegionMapHandler->regionMap.mapSecName, 0, 1, 0, NULL);
        ScheduleBgCopyTilemapToVram(WIN_MAPSEC_NAME);
    }
    else
    {
        FillWindowPixelBuffer(WIN_MAPSEC_NAME, PIXEL_FILL(1));
        CopyWindowToVram(WIN_MAPSEC_NAME, COPYWIN_FULL);
    }
}

static void PrintTitleWindowText(void)
{
    static const u8 FlyPromptText[] = _("{R_BUTTON} FLY");
    const u8 *region;
    if (IS_FRLG)
        region = gText_Kanto;
    else
        region = gText_Hoenn;
    u32 hoennOffset = GetStringCenterAlignXOffset(FONT_NORMAL, region, 0x38);
    u32 flyOffset = GetStringCenterAlignXOffset(FONT_NORMAL, FlyPromptText, 0x38);

    FillWindowPixelBuffer(WIN_TITLE, PIXEL_FILL(1));

    if (sFieldRegionMapHandler->regionMap.mapSecType == MAPSECTYPE_CITY_CANFLY
        && FlagGet(FLAG_FLY_FROM_TOWN_MAP) && Overworld_MapTypeAllowsTeleportAndFly(gMapHeader.mapType) == TRUE)
    {
        AddTextPrinterParameterized(WIN_TITLE, FONT_NORMAL, FlyPromptText, flyOffset, 1, 0, NULL);
        ScheduleBgCopyTilemapToVram(WIN_TITLE);
    }
    else
    {
        AddTextPrinterParameterized(WIN_TITLE, FONT_NORMAL, region, hoennOffset, 1, 0, NULL);
        CopyWindowToVram(WIN_TITLE, COPYWIN_FULL);
    }
}

static void StartZoom(void)
{
    PlaySE(SE_SELECT);
    sFieldRegionMapHandler->zoomingIn = !IsRegionMapZoomed();
    if (sFieldRegionMapHandler->zoomingIn)
    {
        UpdateMapSecInfoWindow();
        ShowBg(1);
    }
    sFieldRegionMapHandler->panelScrolling = TRUE;
    SetRegionMapDataForZoom();
    sFieldRegionMapHandler->state = 7;
}

static void LoadInfoPanelGfx(void)
{
    u8 windowId;

    BgDmaFill(1, PIXEL_FILL(0), 0x40, 1);
    BgDmaFill(1, PIXEL_FILL(1), 0x41, 1);
    CpuFill16(0x1040, sFieldRegionMapHandler->tilemapBuffer, BG_SCREEN_SIZE);
    SetBgTilemapBuffer(1, sFieldRegionMapHandler->tilemapBuffer);
    windowId = AddWindow(&sMapSecInfoWindowTemplate);
    sFieldRegionMapHandler->infoWindowId = windowId;
    LoadUserWindowBorderGfx_(windowId, 0x42, BG_PLTT_ID(4));
    DrawTextBorderOuter(windowId, 0x42, 4);
    DecompressAndCopyTileDataToVram(1, sCityZoomTiles_Gfx, 0, 0, 0);
    FillWindowPixelBuffer(windowId, PIXEL_FILL(1));
    PutWindowTilemap(windowId);
    CopyWindowToVram(windowId, COPYWIN_FULL);
    LoadPalette(sMapSecInfoWindow_Pal, BG_PLTT_ID(1), sizeof(sMapSecInfoWindow_Pal));
    LoadPalette(sCityZoomTiles_Pal, BG_PLTT_ID(3), PLTT_SIZE_4BPP);
    ChangeBgY(1, INFO_PANEL_HIDDEN_Y, BG_COORD_SET);
    ChangeBgX(1, 0, BG_COORD_SET);
}

static void FreeCityZoomViewGfx(void)
{
    u32 i;

    FreeSpriteTilesByTag(TAG_CITY_ZOOM);
    FreeSpritePaletteByTag(TAG_CITY_ZOOM_PAL);
    for (i = 0; i < ARRAY_COUNT(sFieldRegionMapHandler->cityZoomTextSprites); i++)
        DestroySprite(sFieldRegionMapHandler->cityZoomTextSprites[i]);
}

static void SetCityZoomTextPosition(void)
{
    u32 i;
    s32 y = 132 - (GetBgY(1) >> 8);

    for (i = 0; i < ARRAY_COUNT(sFieldRegionMapHandler->cityZoomTextSprites); i++)
    {
        struct Sprite *sprite = sFieldRegionMapHandler->cityZoomTextSprites[i];

        sprite->y = y;
        sprite->invisible = sFieldRegionMapHandler->cityTextHidden || y > 160;
    }
}

static void CreateCityZoomTextSprites(void)
{
    u32 i;

    for (i = 0; i < ARRAY_COUNT(sFieldRegionMapHandler->cityZoomTextSprites); i++)
    {
        u8 spriteId = CreateSprite(&sCityZoomTextSpriteTemplate, 152 + i * 32, 228, 8);
        struct Sprite *sprite = &gSprites[spriteId];

        sprite->invisible = TRUE;
        sprite->data[0] = 0;
        sprite->data[1] = i * 4;
        sprite->data[2] = sprite->oam.tileNum;
        sprite->data[3] = 150;
        sprite->data[4] = i * 4;
        sprite->oam.tileNum += i * 4;
        sFieldRegionMapHandler->cityZoomTextSprites[i] = sprite;
    }
}

// Slide and cycle through the text key showing what the features on the zoomed city map are
static void SpriteCB_CityZoomText(struct Sprite *sprite)
{
    if (sprite->data[3])
    {
        sprite->data[3]--;
        return;
    }

    if (++sprite->data[0] > 11)
        sprite->data[0] = 0;

    if (++sprite->data[1] > 60)
        sprite->data[1] = 0;

    sprite->oam.tileNum = sprite->data[2] + sprite->data[1];
    if (sprite->data[5] < 4)
    {
        if (sprite->data[0] == 0)
        {
            sprite->data[5]++;
            sprite->data[3] = 120;
        }
    }
    else
    {
        if (sprite->data[1] == sprite->data[4])
        {
            sprite->data[5] = 0;
            sprite->data[0] = 0;
            sprite->data[3] = 120;
        }
    }
}

// Slides the info panel in or out alongside the map zoom. Returns TRUE while scrolling.
static bool32 ScrollInfoPanel(void)
{
    if (!sFieldRegionMapHandler->panelScrolling)
        return FALSE;

    if (sFieldRegionMapHandler->zoomingIn)
    {
        if (ChangeBgY(1, INFO_PANEL_SCROLL_SPEED, BG_COORD_ADD) >= 0)
        {
            ChangeBgY(1, 0, BG_COORD_SET);
            sFieldRegionMapHandler->panelScrolling = FALSE;
        }
    }
    else
    {
        if (ChangeBgY(1, INFO_PANEL_SCROLL_SPEED, BG_COORD_SUB) <= INFO_PANEL_HIDDEN_Y)
        {
            ChangeBgY(1, INFO_PANEL_HIDDEN_Y, BG_COORD_SET);
            HideBg(1);
            sFieldRegionMapHandler->panelScrolling = FALSE;
        }
    }
    SetCityZoomTextPosition();
    return TRUE;
}

static void DrawCityMap(mapsec_u16_t mapSecId, u16 pos)
{
    u32 i;

    for (i = 0; i < NUM_CITY_MAPS && (sCityMaps[i].mapSecId != mapSecId || sCityMaps[i].index != pos); i++)
        ;

    if (i == NUM_CITY_MAPS)
        return;

    DecompressDataWithHeaderWram(sCityMaps[i].tilemap, sFieldRegionMapHandler->cityMapBuffer);
    FillBgTilemapBufferRect_Palette0(1, 0x1041, 17, 6, 12, 11);
    CopyToBgTilemapBufferRect(1, sFieldRegionMapHandler->cityMapBuffer, 18, 6, 10, 10);
}

static void PrintLandmarkNames(mapsec_u16_t mapSecId, u16 pos)
{
    u32 i = 0;

    while (1)
    {
        const u8 *landmarkName = GetLandmarkName(mapSecId, pos, i);
        if (!landmarkName)
            break;

        StringCopyPadded(gStringVar1, landmarkName, CHAR_SPACE, 12);
        AddTextPrinterParameterized(sFieldRegionMapHandler->infoWindowId, FONT_NARROW, gStringVar1, 0, i * 16 + 17, TEXT_SKIP_DRAW, NULL);
        i++;
    }
}

// Redraws the zoomed view's info panel: name, then a city map or landmark list for the selection
static void UpdateMapSecInfoWindow(void)
{
    struct RegionMap *regionMap = &sFieldRegionMapHandler->regionMap;
    u8 windowId = sFieldRegionMapHandler->infoWindowId;

    switch (regionMap->mapSecType)
    {
    case MAPSECTYPE_CITY_CANFLY:
        FillWindowPixelBuffer(windowId, PIXEL_FILL(1));
        PutWindowRectTilemap(windowId, 0, 0, 12, 2);
        AddTextPrinterParameterized(windowId, FONT_NARROW, regionMap->mapSecName, 0, 1, TEXT_SKIP_DRAW, NULL);
        DrawCityMap(regionMap->mapSecId, regionMap->posWithinMapSec);
        CopyWindowToVram(windowId, COPYWIN_FULL);
        sFieldRegionMapHandler->cityTextHidden = FALSE;
        break;
    case MAPSECTYPE_CITY_CANTFLY:
        FillWindowPixelBuffer(windowId, PIXEL_FILL(1));
        PutWindowRectTilemap(windowId, 0, 0, 12, 2);
        AddTextPrinterParameterized(windowId, FONT_NARROW, regionMap->mapSecName, 0, 1, TEXT_SKIP_DRAW, NULL);
        FillBgTilemapBufferRect(1, 0x1041, 17, 6, 12, 11, 17);
        CopyWindowToVram(windowId, COPYWIN_FULL);
        sFieldRegionMapHandler->cityTextHidden = TRUE;
        break;
    case MAPSECTYPE_ROUTE:
    case MAPSECTYPE_BATTLE_FRONTIER:
        FillWindowPixelBuffer(windowId, PIXEL_FILL(1));
        PutWindowTilemap(windowId);
        AddTextPrinterParameterized(windowId, FONT_NARROW, regionMap->mapSecName, 0, 1, TEXT_SKIP_DRAW, NULL);
        PrintLandmarkNames(regionMap->mapSecId, regionMap->posWithinMapSec);
        CopyWindowToVram(windowId, COPYWIN_FULL);
        sFieldRegionMapHandler->cityTextHidden = TRUE;
        break;
    case MAPSECTYPE_NONE:
        FillBgTilemapBufferRect(1, 0x1041, 17, 4, 12, 13, 17);
        CopyBgTilemapBufferToVram(1);
        sFieldRegionMapHandler->cityTextHidden = TRUE;
        break;
    }
    SetCityZoomTextPosition();
}
