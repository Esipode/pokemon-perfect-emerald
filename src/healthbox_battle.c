#include "global.h"
#include "battle.h"
#include "battle_gimmick.h"
#include "battle_interface.h"
#include "battle_main.h"
#include "battle_util.h"
#include "decompress.h"
#include "graphics.h"
#include "healthbox.h"
#include "pokemon.h"
#include "strings.h"
#include "string_util.h"
#include "sprite.h"
#include "constants/battle.h"
#include "constants/characters.h"
#include "constants/healthbox.h"
#include "constants/rgb.h"
#include "data/healthbox.h"

#define HB_BOX_TILES_LEFT 32
#define HB_BOX_CENTER_X   32
#define HB_BOX_CENTER_Y   16
#define HB_HP_TEXT_LEN    12
#define HB_NIDORAN_GENDER 100 // Suppresses the gender symbol.

// Indicator centre sits this far left of the level's right edge (fits a 2-digit level).
#define HB_INDICATOR_X_FROM_RIGHT 15

static EWRAM_DATA struct HealthboxLayout sLayouts[MAX_BATTLERS_COUNT] = {0};

static const struct OamData sOamData_HealthboxBox =
{
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(64x32),
    .size = SPRITE_SIZE(64x32),
    .priority = 1,
};

static const struct SpriteTemplate sPlayerBoxTemplates[2] =
{
    { .tileTag = TAG_HEALTHBOX_PLAYER1_TILE, .paletteTag = TAG_HEALTHBOX_PAL, .oam = &sOamData_HealthboxBox },
    { .tileTag = TAG_HEALTHBOX_PLAYER2_TILE, .paletteTag = TAG_HEALTHBOX_PAL, .oam = &sOamData_HealthboxBox },
};

static const struct SpriteTemplate sOpponentBoxTemplates[2] =
{
    { .tileTag = TAG_HEALTHBOX_OPPONENT1_TILE, .paletteTag = TAG_HEALTHBOX_PAL, .oam = &sOamData_HealthboxBox },
    { .tileTag = TAG_HEALTHBOX_OPPONENT2_TILE, .paletteTag = TAG_HEALTHBOX_PAL, .oam = &sOamData_HealthboxBox },
};

static const struct SpritePalette sNewHealthboxPalette = { sHealthboxNewPal, TAG_HEALTHBOX_PAL };

// Right-sprite data field: x offset from the classic main.x + 64 placement.
#define hOther_XAdjust data[6]

bool32 Healthbox_IsNewStyle(void)
{
    if (gBattleTypeFlags & (BATTLE_TYPE_SAFARI | BATTLE_TYPE_FIRST_BATTLE | BATTLE_TYPE_CATCH_TUTORIAL))
        return FALSE;
    return HEALTHBOX_DEV_FORCE_NEW || HealthboxOptions_GetStyle() == HEALTHBOX_STYLE_NEW;
}

void HealthboxBattle_LoadPalette(void)
{
    LoadSpritePalette(&sNewHealthboxPalette);
}

static u32 GetBoxTileCount(enum BattlerId battler)
{
    return GetBattlerCoordsIndex(battler) == BATTLE_COORDS_SINGLES ? 64 : 48;
}

// Loads the blank box sheet for a box-sheet state of BattleLoadAllHealthBoxesGfx; FALSE for other states.
bool32 HealthboxBattle_LoadBoxSheet(u8 state)
{
    static const u16 sSingles[] = { TAG_HEALTHBOX_PLAYER1_TILE, TAG_HEALTHBOX_OPPONENT1_TILE };
    static const u16 sDoubles[] = { TAG_HEALTHBOX_PLAYER1_TILE, TAG_HEALTHBOX_PLAYER2_TILE,
                                    TAG_HEALTHBOX_OPPONENT1_TILE, TAG_HEALTHBOX_OPPONENT2_TILE };
    static const u8 sDoublesPositions[] = { B_POSITION_PLAYER_LEFT, B_POSITION_PLAYER_RIGHT,
                                            B_POSITION_OPPONENT_LEFT, B_POSITION_OPPONENT_RIGHT };
    struct CompressedSpriteSheet sheet;
    u32 idx;

    if (state < 2)
        return FALSE;
    idx = state - 2;

    if (!IsDoubleBattle())
    {
        if (idx >= ARRAY_COUNT(sSingles))
            return FALSE;
        sheet.tag = sSingles[idx];
        sheet.size = 64 * TILE_SIZE_4BPP;
    }
    else
    {
        if (idx >= ARRAY_COUNT(sDoubles))
            return FALSE;
        sheet.tag = sDoubles[idx];
        sheet.size = GetBoxTileCount(GetBattlerAtPosition(sDoublesPositions[idx])) * TILE_SIZE_4BPP;
    }
    sheet.data = gBlankGfxCompressed;
    LoadCompressedSpriteSheet(&sheet);
    return TRUE;
}

// Creates the two box-half sprites and wires them together; the caller creates the bar sprite.
u8 HealthboxBattle_CreateBoxSprites(u32 battler, void (*otherCallback)(struct Sprite *), s16 *barData6)
{
    struct HealthboxResolvedOpts opts;
    struct HealthboxLayout *layout = &sLayouts[battler];
    struct HealthboxSprites sprites;
    const struct SpriteTemplate *templates;
    bool32 doubles = GetBattlerCoordsIndex(battler) == BATTLE_COORDS_DOUBLES;
    bool32 player = IsOnPlayerSide(battler);
    u32 tagIdx = doubles ? GetBattlerPosition(battler) / 2 : 0;
    struct Sprite *right;

    Healthbox_ResolveOpts(player ? HB_SIDE_PLAYER : HB_SIDE_FOE, &opts);
    Healthbox_ComputeLayout(&opts, opts.side, doubles, GetBattlerPosition(battler), layout);

    templates = player ? sPlayerBoxTemplates : sOpponentBoxTemplates;
    sprites.left = CreateSprite(&templates[tagIdx], DISPLAY_WIDTH, DISPLAY_HEIGHT, 1);
    sprites.right = CreateSpriteAtEnd(&templates[tagIdx], DISPLAY_WIDTH, DISPLAY_HEIGHT, 1);

    right = &gSprites[sprites.right];
    right->oam.tileNum += HB_BOX_TILES_LEFT;
    if (layout->rightSpriteW == 32)
    {
        right->oam.shape = SPRITE_SHAPE(32x32);
        right->oam.size = SPRITE_SIZE(32x32);
        right->hOther_XAdjust = -16;
    }

    gSprites[sprites.left].oam.affineParam = sprites.right;
    right->data[5] = sprites.left;
    right->callback = otherCallback;

    HealthboxRender_Clear(&sprites, layout);

    *barData6 = player ? (doubles ? 1 : 0) : 2;
    return sprites.left;
}

void HealthboxBattle_GetCoords(u32 battler, s16 *x, s16 *y)
{
    // Main sprite origin is the centre of its 64x32 frame.
    *x = sLayouts[battler].screenX + HB_BOX_CENTER_X;
    *y = sLayouts[battler].screenY + HB_BOX_CENTER_Y;
}

static void GetBoxSprites(u8 healthboxSpriteId, struct HealthboxSprites *sprites, const struct HealthboxLayout **layout)
{
    sprites->left = healthboxSpriteId;
    sprites->right = gSprites[healthboxSpriteId].oam.affineParam;
    sprites->bar = gSprites[healthboxSpriteId].data[5];
    *layout = &sLayouts[gSprites[healthboxSpriteId].data[6]];
}

void HealthboxBattle_GetIndicatorPos(u32 battler, s16 *x, s16 *y)
{
    const struct HealthboxRect *level = &sLayouts[battler].rects[HB_RECT_LEVEL];

    *x = level->x + level->w - HB_BOX_CENTER_X - HB_INDICATOR_X_FROM_RIGHT;
    *y = level->y + 1 - HB_BOX_CENTER_Y;
}

void HealthboxBattle_DrawNick(u8 healthboxSpriteId, struct Pokemon *mon)
{
    struct HealthboxSprites sprites;
    const struct HealthboxLayout *layout;
    enum BattlerId battler = gSprites[healthboxSpriteId].data[6];
    struct Pokemon *illusionMon = GetIllusionMonPtr(battler);
    u8 nickname[POKEMON_NAME_LENGTH + 1];
    u8 *ptr;
    u32 gender;
    enum Species species;

    GetBoxSprites(healthboxSpriteId, &sprites, &layout);
    if (layout->rects[HB_RECT_NICK].w == 0)
        return;

    if (illusionMon != NULL)
        mon = illusionMon;

    GetMonData(mon, MON_DATA_NICKNAME, nickname);
    StringGet_Nickname(nickname);
    ptr = StringCopy(gDisplayedStringBattle, nickname);

    gender = GetMonGender(mon);
    species = GetMonData(mon, MON_DATA_SPECIES);
    if ((species == SPECIES_NIDORAN_F || species == SPECIES_NIDORAN_M) && StringCompare(nickname, GetSpeciesName(species)) == 0)
        gender = HB_NIDORAN_GENDER;
    if (GetBattlerSide(battler) == B_SIDE_OPPONENT && IsGhostBattleWithoutScope())
        gender = HB_NIDORAN_GENDER;

    switch (gender)
    {
    default:
        StringCopy(ptr, gText_HealthboxGender_None);
        break;
    case MON_MALE:
        StringCopy(ptr, gText_HealthboxGender_Male);
        break;
    case MON_FEMALE:
        StringCopy(ptr, gText_HealthboxGender_Female);
        break;
    }

    HealthboxRender_PrintText(&sprites, layout, &layout->rects[HB_RECT_NICK], gDisplayedStringBattle, FALSE, HealthboxOptions_GetBackground());
}

static void DrawLevel(u8 healthboxSpriteId, u32 level)
{
    struct HealthboxSprites sprites;
    const struct HealthboxLayout *layout;
    u8 text[8];

    GetBoxSprites(healthboxSpriteId, &sprites, &layout);
    if (layout->rects[HB_RECT_LEVEL].w == 0)
    {
        UpdateIndicatorVisibilityAndType(healthboxSpriteId, TRUE);
        return;
    }

    // A gimmick indicator replaces the "Lv" label, as in Classic.
    if (GetIndicatorPalTag(gSprites[healthboxSpriteId].data[6]) != TAG_NONE)
    {
        ConvertIntToDecimalStringN(text, level, STR_CONV_MODE_LEFT_ALIGN, 4);
        UpdateIndicatorLevelData(healthboxSpriteId, level);
        UpdateIndicatorVisibilityAndType(healthboxSpriteId, FALSE);
    }
    else
    {
        text[0] = CHAR_EXTRA_SYMBOL;
        text[1] = CHAR_LV_2;
        ConvertIntToDecimalStringN(text + 2, level, STR_CONV_MODE_LEFT_ALIGN, 4);
        UpdateIndicatorVisibilityAndType(healthboxSpriteId, TRUE);
    }

    HealthboxRender_PrintText(&sprites, layout, &layout->rects[HB_RECT_LEVEL], text, TRUE, HealthboxOptions_GetBackground());
}

void HealthboxBattle_DrawHpValue(u8 healthboxSpriteId, s16 currHp, s16 maxHp)
{
    struct HealthboxSprites sprites;
    const struct HealthboxLayout *layout;
    enum BattlerId battler = gSprites[healthboxSpriteId].data[6];
    u8 text[HB_HP_TEXT_LEN], *ptr;

    GetBoxSprites(healthboxSpriteId, &sprites, &layout);
    if (layout->rects[HB_RECT_HP_VALUE].w == 0)
        return;

    if (HealthboxOptions_GetHpValue(IsOnPlayerSide(battler) ? HB_SIDE_PLAYER : HB_SIDE_FOE) == HB_HPVAL_PERCENT)
    {
        s32 percent = currHp == 0 || maxHp <= 0 ? 0 : max((currHp * 100) / maxHp, 1);

        ptr = ConvertIntToDecimalStringN(text, percent, STR_CONV_MODE_LEFT_ALIGN, 3);
        *ptr++ = CHAR_PERCENT;
        *ptr = EOS;
    }
    else
    {
        ptr = ConvertIntToDecimalStringN(text, currHp, STR_CONV_MODE_LEFT_ALIGN, 4);
        *ptr++ = CHAR_SLASH;
        ConvertIntToDecimalStringN(ptr, maxHp, STR_CONV_MODE_LEFT_ALIGN, 4);
    }

    HealthboxRender_PrintText(&sprites, layout, &layout->rects[HB_RECT_HP_VALUE], text, TRUE, HealthboxOptions_GetBackground());
}

void HealthboxBattle_Update(u8 healthboxSpriteId, struct Pokemon *mon, u8 elementId)
{
    struct HealthboxSprites sprites;
    const struct HealthboxLayout *layout;
    bool32 all = elementId == HEALTHBOX_ALL;

    GetBoxSprites(healthboxSpriteId, &sprites, &layout);

    if (all)
    {
        HealthboxRender_Clear(&sprites, layout);
        HealthboxRender_DrawFrame(&sprites, layout, HealthboxOptions_GetBackground());
    }
    if (all || elementId == HEALTHBOX_NICK)
        HealthboxBattle_DrawNick(healthboxSpriteId, mon);
    if (all || elementId == HEALTHBOX_LEVEL)
        DrawLevel(healthboxSpriteId, GetMonData(mon, MON_DATA_LEVEL));
    if (all || elementId == HEALTHBOX_CURRENT_HP || elementId == HEALTHBOX_MAX_HP)
        HealthboxBattle_DrawHpValue(healthboxSpriteId, GetMonData(mon, MON_DATA_HP), GetMonData(mon, MON_DATA_MAX_HP));
}
