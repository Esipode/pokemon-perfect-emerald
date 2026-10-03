static const u8 sText_ActionBattle[] = _("Battle");
static const u8 sText_ActionBag[] = _("Bag");
static const u8 sText_ActionPokemon[] = _("Pokémon");
static const u8 sText_ActionRun[] = _("Run");
static const u8 sText_ActionBall[] = _("Ball");
static const u8 sText_ActionGoNear[] = _("Go Near");
static const u8 sText_ActionBallPrefix[] = _("Ball ×");

static const u16 sActionMenuLitOutline = RGB(31, 29, 20);

#define ACTION_HUE_RED(lit)   { RGB(lit ? 27 : 21, lit ? 7 : 6, lit ? 6 : 5), RGB(lit ? 16 : 11, 2, 2) }
#define ACTION_HUE_AMBER(lit) { RGB(lit ? 29 : 23, lit ? 20 : 15, lit ? 4 : 3), RGB(lit ? 18 : 13, lit ? 11 : 8, 1) }
#define ACTION_HUE_GREEN(lit) { RGB(lit ? 7 : 6, lit ? 23 : 17, lit ? 9 : 7), RGB(2, lit ? 13 : 9, 3) }
#define ACTION_HUE_BLUE(lit)  { RGB(lit ? 7 : 6, lit ? 14 : 10, lit ? 29 : 22), RGB(2, lit ? 7 : 5, lit ? 18 : 13) }

#define ACTION_SLOT(_label, _icon, _action, _hue) \
{ \
    .label = _label, \
    .icon = _icon, \
    .action = _action, \
    .navigable = TRUE, \
    .idleHue = _hue(FALSE), \
    .litHue = _hue(TRUE), \
}

static const struct ActionMenuSlot sActionMenus[ACTION_MENU_COUNT][ACTION_MENU_SLOT_COUNT] =
{
    [ACTION_MENU_STANDARD] =
    {
        ACTION_SLOT(sText_ActionBattle, ACTION_ICON_BATTLE, B_ACTION_USE_MOVE, ACTION_HUE_RED),
        ACTION_SLOT(sText_ActionBag, ACTION_ICON_BAG, B_ACTION_USE_ITEM, ACTION_HUE_AMBER),
        ACTION_SLOT(sText_ActionPokemon, ACTION_ICON_POKEMON, B_ACTION_SWITCH, ACTION_HUE_GREEN),
        ACTION_SLOT(sText_ActionRun, ACTION_ICON_RUN, B_ACTION_RUN, ACTION_HUE_BLUE),
    },
    [ACTION_MENU_SAFARI] =
    {
        ACTION_SLOT(sText_ActionBall, ACTION_ICON_SAFARI_BALL, B_ACTION_SAFARI_BALL, ACTION_HUE_RED),
        ACTION_SLOT(sText_ActionGoNear, ACTION_ICON_GO_NEAR, B_ACTION_SAFARI_GO_NEAR, ACTION_HUE_GREEN),
        ACTION_SLOT(sText_ActionRun, ACTION_ICON_RUN, B_ACTION_SAFARI_RUN, ACTION_HUE_BLUE),
        { .navigable = FALSE },
    },
    [ACTION_MENU_TUTORIAL] =
    {
        ACTION_SLOT(sText_ActionBattle, ACTION_ICON_BATTLE, B_ACTION_USE_MOVE, ACTION_HUE_RED),
        ACTION_SLOT(sText_ActionBag, ACTION_ICON_BAG, B_ACTION_USE_ITEM, ACTION_HUE_AMBER),
        ACTION_SLOT(sText_ActionPokemon, ACTION_ICON_POKEMON, B_ACTION_SWITCH, ACTION_HUE_GREEN),
        ACTION_SLOT(sText_ActionRun, ACTION_ICON_RUN, B_ACTION_RUN, ACTION_HUE_BLUE),
    },
};

// Chip rects in chip grid window pixels (inclusive); slots go left-to-right, top-to-bottom.
static const struct ActionMenuRect sActionChipRects[ACTION_MENU_SLOT_COUNT] =
{
    { 4,  5, 67, 23 },
    { 72, 5, 135, 23 },
    { 4,  27, 67, 45 },
    { 72, 27, 135, 45 },
};

// Move cells in grid window pixels (inclusive). Each spans whole 8x8 tiles per column and keeps one tile row
// band per cell row, so a cell's palette swap never touches a neighbour.
static const struct ActionMenuRect sMoveCellRects[MOVE_MENU_SLOT_COUNT] =
{
    { 0,  2,  71,  23 },
    { 72, 2,  143, 23 },
    { 0,  24, 71,  45 },
    { 72, 24, 143, 45 },
};

// Move type colours as 5-bit RGB; idle and dark shades derive from these in MoveMenu_BuildPalette.
static const u8 sMoveTypeColors[NUMBER_OF_MON_TYPES][3] =
{
    [TYPE_NONE]     = { 10, 10, 12 },
    [TYPE_NORMAL]   = { 21, 21, 16 },
    [TYPE_FIGHTING] = { 24, 9, 6 },
    [TYPE_FLYING]   = { 17, 17, 28 },
    [TYPE_POISON]   = { 20, 9, 22 },
    [TYPE_GROUND]   = { 24, 20, 10 },
    [TYPE_ROCK]     = { 19, 17, 9 },
    [TYPE_BUG]      = { 17, 21, 5 },
    [TYPE_GHOST]    = { 13, 11, 19 },
    [TYPE_STEEL]    = { 18, 19, 22 },
    [TYPE_MYSTERY]  = { 11, 18, 17 },
    [TYPE_FIRE]     = { 28, 15, 5 },
    [TYPE_WATER]    = { 9, 15, 28 },
    [TYPE_GRASS]    = { 10, 23, 8 },
    [TYPE_ELECTRIC] = { 29, 24, 4 },
    [TYPE_PSYCHIC]  = { 29, 11, 18 },
    [TYPE_ICE]      = { 13, 25, 25 },
    [TYPE_DRAGON]   = { 11, 7, 28 },
    [TYPE_DARK]     = { 12, 9, 8 },
    [TYPE_FAIRY]    = { 29, 17, 24 },
    [TYPE_STELLAR]  = { 17, 25, 29 },
};
