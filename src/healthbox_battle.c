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

#define HB_NIDORAN_GENDER 100 // Suppresses the gender symbol.

// Indicator centre sits this far left of the level's right edge (fits a 2-digit level).
#define HB_INDICATOR_X_FROM_RIGHT 15

static EWRAM_DATA struct HealthboxLayout sLayouts[MAX_BATTLERS_COUNT] = {0};

static EWRAM_DATA struct Subsprite sBarSubsprites[MAX_BATTLERS_COUNT][HB_BAR_SUBSPRITES_MAX] = {0};
static EWRAM_DATA struct SubspriteTable sBarSubspriteTables[MAX_BATTLERS_COUNT] = {0};

static const struct SpritePalette sNewHealthboxPalette = { sHealthboxNewPal, TAG_HEALTHBOX_PAL };

// Right-sprite data field: x offset from the classic main.x + 64 placement.
#define hOther_XAdjust data[6]

bool32 Healthbox_IsNewStyle(void)
{
    if (gBattleTypeFlags & (BATTLE_TYPE_SAFARI | BATTLE_TYPE_FIRST_BATTLE | BATTLE_TYPE_CATCH_TUTORIAL))
        return FALSE;
    return HealthboxOptions_GetStyle() == HEALTHBOX_STYLE_NEW;
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

const struct SubspriteTable *HealthboxBattle_GetBarSubspriteTable(u32 battler)
{
    return &sBarSubspriteTables[battler];
}

bool32 HealthboxBattle_HasHpBar(u32 battler)
{
    return sLayouts[battler].rects[HB_RECT_HP_BAR].w != 0;
}

bool32 HealthboxBattle_HasCaughtIcons(u32 battler)
{
    return sLayouts[battler].rects[HB_RECT_CAUGHT].w != 0;
}

// Creates the two box-half sprites and wires them together; the caller creates the bar sprite.
u8 HealthboxBattle_CreateBoxSprites(u32 battler, void (*otherCallback)(struct Sprite *), s16 *barData6)
{
    struct HealthboxResolvedOpts opts;
    struct HealthboxLayout *layout = &sLayouts[battler];
    struct HealthboxSprites sprites;
    bool32 doubles = GetBattlerCoordsIndex(battler) == BATTLE_COORDS_DOUBLES;
    bool32 player = IsOnPlayerSide(battler);
    u32 tagIdx = doubles ? GetBattlerPosition(battler) / 2 : 0;
    struct Sprite *right;

    Healthbox_ResolveOpts(player ? HB_SIDE_PLAYER : HB_SIDE_FOE, &opts);
    Healthbox_ComputeLayout(&opts, opts.side, doubles, GetBattlerPosition(battler), layout);
    HealthboxRender_BuildBarSubsprites(layout, sBarSubsprites[battler], &sBarSubspriteTables[battler]);

    HealthboxRender_CreateBox(player, tagIdx, layout->rightSpriteW, &sprites);
    right = &gSprites[sprites.right];
    if (layout->rightSpriteW == 32)
        right->hOther_XAdjust = -16;

    right->data[5] = sprites.left;
    right->callback = otherCallback;

    HealthboxRender_Clear(&sprites, layout);

    *barData6 = HB_BAR_DATA6_NEW;
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

void HealthboxBattle_GetBoxBounds(u32 battler, s16 *left, s16 *top, s16 *right, s16 *bottom)
{
    const struct HealthboxLayout *layout = &sLayouts[battler];

    *left = layout->screenX;
    *top = layout->screenY;
    *right = layout->screenX + layout->boxW;
    *bottom = layout->screenY + layout->boxH;
}

// Classic trigger offsets assume a 32 px box centred on the main sprite.
s32 HealthboxBattle_GetCentreShift(u32 battler)
{
    return sLayouts[battler].boxH / 2 - HB_BOX_CENTER_Y;
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
    u8 text[HB_HP_TEXT_LEN];

    GetBoxSprites(healthboxSpriteId, &sprites, &layout);
    if (layout->rects[HB_RECT_HP_VALUE].w == 0)
        return;

    HealthboxRender_FormatHpValue(text, HealthboxOptions_GetHpValue(IsOnPlayerSide(battler) ? HB_SIDE_PLAYER : HB_SIDE_FOE), currHp, maxHp);

    HealthboxRender_PrintText(&sprites, layout, &layout->rects[HB_RECT_HP_VALUE], text, TRUE, HealthboxOptions_GetBackground());
}

void HealthboxBattle_DrawHpBar(u8 healthboxSpriteId, u32 fillPx, u32 trailPx, u32 colourLevel)
{
    HealthboxRender_DrawHpBar(gSprites[healthboxSpriteId].data[5], fillPx, trailPx, colourLevel);
}

void HealthboxBattle_DrawExpBar(u8 healthboxSpriteId, u32 fillPx)
{
    struct HealthboxSprites sprites;
    const struct HealthboxLayout *layout;

    GetBoxSprites(healthboxSpriteId, &sprites, &layout);
    if (layout->rects[HB_RECT_EXP].w == 0)
        return;

    HealthboxRender_DrawExpBar(&sprites, layout, &layout->rects[HB_RECT_EXP], fillPx);
}

static void DrawStatus(u8 healthboxSpriteId, struct Pokemon *mon)
{
    struct HealthboxSprites sprites;
    const struct HealthboxLayout *layout;
    enum BattlerId battler = gSprites[healthboxSpriteId].data[6];
    u32 status = GetMonData(mon, MON_DATA_STATUS);
    const struct HealthboxStatusSpec *spec = NULL;
    u32 i;

    GetBoxSprites(healthboxSpriteId, &sprites, &layout);
    if (layout->rects[HB_RECT_STATUS].w == 0)
        return;

    for (i = 0; i < ARRAY_COUNT(sHealthboxStatusSpecs); i++)
    {
        if (status & sHealthboxStatusSpecs[i].mask)
        {
            spec = &sHealthboxStatusSpecs[i];
            break;
        }
    }

    if (spec != NULL)
        LoadHealthboxStatusColor(healthboxSpriteId, battler, spec->palId);
    HealthboxRender_DrawStatusPill(&sprites, layout, &layout->rects[HB_RECT_STATUS], HB_PAL_STATUS_FIRST + battler,
                                   spec != NULL ? spec->label : NULL, HealthboxOptions_GetBackground());
}

static void DrawStatStrip(u8 healthboxSpriteId, enum BattlerId battler)
{
    struct HealthboxSprites sprites;
    const struct HealthboxLayout *layout;
    u8 stages[HB_STRIP_SLOTS];
    u32 i;

    GetBoxSprites(healthboxSpriteId, &sprites, &layout);
    if (layout->rects[HB_RECT_STRIP].w == 0)
        return;

    for (i = 0; i < HB_STRIP_SLOTS; i++)
        stages[i] = gBattleMons[battler].statStages[Healthbox_StripStat(i)];

    gBattleSpritesDataPtr->healthBoxesData[battler].statStripSig = Healthbox_PackStages(stages);
    HealthboxRender_DrawStatStrip(&sprites, layout, &layout->rects[HB_RECT_STRIP], stages, HealthboxOptions_GetBackground());
}

void HealthboxBattle_PollStatStrip(u8 healthboxSpriteId)
{
    enum BattlerId battler = gSprites[healthboxSpriteId].data[6];
    u8 stages[HB_STRIP_SLOTS];
    u32 i;

    if (sLayouts[battler].rects[HB_RECT_STRIP].w == 0)
        return;

    for (i = 0; i < HB_STRIP_SLOTS; i++)
        stages[i] = gBattleMons[battler].statStages[Healthbox_StripStat(i)];

    if (Healthbox_PackStages(stages) != gBattleSpritesDataPtr->healthBoxesData[battler].statStripSig)
        DrawStatStrip(healthboxSpriteId, battler);
}

static void UpdateExp(u8 healthboxSpriteId, struct Pokemon *mon, enum BattlerId battler)
{
    enum Species species = GetMonData(mon, MON_DATA_SPECIES);
    u32 level = GetMonData(mon, MON_DATA_LEVEL);
    u32 currLevelExp = GetExperienceAtLevel(gSpeciesInfo[species].growthRate, level);
    s32 currExp = GetMonData(mon, MON_DATA_EXP) - currLevelExp;
    s32 maxExp = GetExperienceAtLevel(gSpeciesInfo[species].growthRate, level + 1) - currLevelExp;

    SetBattleBarStruct(battler, healthboxSpriteId, maxExp, currExp, 0);
    MoveBattleBar(battler, healthboxSpriteId, EXP_BAR, 0);
}

void HealthboxBattle_Update(u8 healthboxSpriteId, struct Pokemon *mon, u8 elementId)
{
    struct HealthboxSprites sprites;
    const struct HealthboxLayout *layout;
    enum BattlerId battler = gSprites[healthboxSpriteId].data[6];
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
    if (all || elementId == HEALTHBOX_HEALTH_BAR)
    {
        SetBattleBarStruct(battler, healthboxSpriteId, GetMonData(mon, MON_DATA_MAX_HP), GetMonData(mon, MON_DATA_HP), 0);
        MoveBattleBar(battler, healthboxSpriteId, HEALTH_BAR, 0);
    }
    if ((all || elementId == HEALTHBOX_EXP_BAR) && IsOnPlayerSide(battler) && layout->rects[HB_RECT_EXP].w != 0)
        UpdateExp(healthboxSpriteId, mon, battler);
    if (all || elementId == HEALTHBOX_STATUS_ICON)
        DrawStatus(healthboxSpriteId, mon);
    if (all)
    {
        DrawStatStrip(healthboxSpriteId, battler);
        TryAddPokeballIconToHealthbox(healthboxSpriteId, TRUE);
    }
}
