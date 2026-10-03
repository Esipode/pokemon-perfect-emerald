static const u8 sText_ActionBattle[] = _("Battle");
static const u8 sText_ActionBag[] = _("Bag");
static const u8 sText_ActionPokemon[] = _("Pokémon");
static const u8 sText_ActionRun[] = _("Run");
static const u8 sText_ActionBall[] = _("Ball");
static const u8 sText_ActionGoNear[] = _("Go Near");

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
};

// Chip rects in chip grid window pixels (inclusive); slots go left-to-right, top-to-bottom.
static const struct ActionMenuRect sActionChipRects[ACTION_MENU_SLOT_COUNT] =
{
    { 4,  3, 67, 21 },
    { 72, 3, 135, 21 },
    { 4,  25, 67, 43 },
    { 72, 25, 135, 43 },
};
