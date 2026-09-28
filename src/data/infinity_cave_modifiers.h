#ifndef GUARD_DATA_INFINITY_CAVE_MODIFIERS_H
#define GUARD_DATA_INFINITY_CAVE_MODIFIERS_H

// Room modifier data. Included from src/infinity_cave.c only; everything outside
// it reads a row through InfCave_GetModifierInfo.

static const u8 sInfCaveModName_Monotype[] = _("MONOTYPE");
static const u8 sInfCaveModName_Weather[] = _("WEATHER");
static const u8 sInfCaveModName_Terrain[] = _("TERRAIN");
static const u8 sInfCaveModName_Doubles[] = _("DOUBLES");
static const u8 sInfCaveModName_Swarm[] = _("SWARM");
static const u8 sInfCaveModName_Surge[] = _("SURGE");
static const u8 sInfCaveModName_Gimmick[] = _("GIMMICK");
static const u8 sInfCaveModName_NoItems[] = _("NO ITEMS");
static const u8 sInfCaveModName_Bounty[] = _("BOUNTY");
static const u8 sInfCaveModName_Dark[] = _("DARK");
static const u8 sInfCaveModName_Cramped[] = _("CRAMPED");
static const u8 sInfCaveModName_Treasured[] = _("TREASURED");

static const u8 sInfCaveModDesc_Monotype[] = _("Every foe fields one type.");
static const u8 sInfCaveModDesc_Weather[] = _("Battles open in weather.");
static const u8 sInfCaveModDesc_Terrain[] = _("Battles open on terrain.");
static const u8 sInfCaveModDesc_Doubles[] = _("Every battle is a double.");
static const u8 sInfCaveModDesc_Swarm[] = _("Two more trainers wait.");
static const u8 sInfCaveModDesc_Surge[] = _("Stronger foes, more shards.");
static const u8 sInfCaveModDesc_Gimmick[] = _("Aces carry a gimmick.");
static const u8 sInfCaveModDesc_NoItems[] = _("No BAG. More shards.");
static const u8 sInfCaveModDesc_Bounty[] = _("Far richer payouts.");
static const u8 sInfCaveModDesc_Dark[] = _("An unlit maze.");
static const u8 sInfCaveModDesc_Cramped[] = _("Tight rooms and corridors.");
static const u8 sInfCaveModDesc_Treasured[] = _("Two item balls are hidden.");

// One row per enum InfCaveModifier. INFCAVE_MOD_NONE has no row: a slot holding
// it is an empty slot.
//
// incompatible is read symmetrically (InfCave_ModifiersCompatible ORs both rows),
// so a pair only has to be named on one side of it.
static const struct InfCaveModifierInfo sInfCaveModifiers[INFCAVE_MOD_COUNT] =
{
    [INFCAVE_MOD_MONOTYPE] =
    {
        .name = sInfCaveModName_Monotype,
        .description = sInfCaveModDesc_Monotype,
        .icon = 0,
        .argKind = INFCAVE_MOD_ARG_TYPE,
        .weight = 20,
        .minDepth = 2,
    },
    [INFCAVE_MOD_WEATHER] =
    {
        .name = sInfCaveModName_Weather,
        .description = sInfCaveModDesc_Weather,
        .icon = 1,
        .argKind = INFCAVE_MOD_ARG_WEATHER,
        .weight = 6,
        .minDepth = 3,
    },
    [INFCAVE_MOD_TERRAIN] =
    {
        .name = sInfCaveModName_Terrain,
        .description = sInfCaveModDesc_Terrain,
        .icon = 2,
        .argKind = INFCAVE_MOD_ARG_TERRAIN,
        .weight = 10,
        .minDepth = 6,
        // One field rule per room: two at once makes a node card that cannot be
        // read at a glance, and the pair rarely changes a battle twice over.
        .incompatible = INFCAVE_MOD_BIT(INFCAVE_MOD_WEATHER),
    },
    [INFCAVE_MOD_DOUBLES] =
    {
        .name = sInfCaveModName_Doubles,
        .description = sInfCaveModDesc_Doubles,
        .icon = 3,
        .weight = 12,
        .minDepth = 4,
    },
    [INFCAVE_MOD_SWARM] =
    {
        .name = sInfCaveModName_Swarm,
        .description = sInfCaveModDesc_Swarm,
        .icon = 4,
        .weight = 8,
        .minDepth = 6,
        // Eight double battles in one room is a floor that outlasts the run it
        // belongs to.
        .incompatible = INFCAVE_MOD_BIT(INFCAVE_MOD_DOUBLES),
    },
    [INFCAVE_MOD_SURGE] =
    {
        .name = sInfCaveModName_Surge,
        .description = sInfCaveModDesc_Surge,
        .icon = 5,
        .shardPercent = 20,
        .weight = 8,
        .minDepth = 3,
    },
    [INFCAVE_MOD_GIMMICK] =
    {
        .name = sInfCaveModName_Gimmick,
        .description = sInfCaveModDesc_Gimmick,
        .icon = 6,
        .weight = 8,
        .minDepth = 6,
    },
    [INFCAVE_MOD_NO_ITEMS] =
    {
        .name = sInfCaveModName_NoItems,
        .description = sInfCaveModDesc_NoItems,
        .icon = 7,
        .shardPercent = 20,
        .weight = 6,
        .minDepth = 5,
    },
    [INFCAVE_MOD_BOUNTY] =
    {
        .name = sInfCaveModName_Bounty,
        .description = sInfCaveModDesc_Bounty,
        .icon = 8,
        .shardPercent = 30,
        .weight = 8,
        .minDepth = 3,
    },
    [INFCAVE_MOD_DARK] =
    {
        .name = sInfCaveModName_Dark,
        .description = sInfCaveModDesc_Dark,
        .icon = 9,
        .weight = 6,
        .minDepth = 8,
    },
    [INFCAVE_MOD_CRAMPED] =
    {
        .name = sInfCaveModName_Cramped,
        .description = sInfCaveModDesc_Cramped,
        .icon = 10,
        .weight = 12,
        .minDepth = 4,
        // Both bias the layout the same way; stacked, they leave a room that is
        // all corridor and no floor for the trainers to stand on.
        .incompatible = INFCAVE_MOD_BIT(INFCAVE_MOD_DARK),
    },
    [INFCAVE_MOD_TREASURED] =
    {
        .name = sInfCaveModName_Treasured,
        .description = sInfCaveModDesc_Treasured,
        .icon = 11,
        .weight = 3,
        .minDepth = 2,
    },
};

// INFCAVE_MOD_WEATHER's argument. startingStatus is what every trainer in the
// room opens its battles with; fieldWeather is what the room itself runs, so the
// overworld says what the battle will do before the player walks into it.
// name is what the node screen prints beside the modifier's own name, so the
// card says which weather the room runs rather than just that it runs one.
struct InfCaveWeatherOption
{
    const u8 *name;
    u8 startingStatus; // enum StartingStatus
    u8 fieldWeather;   // enum OverworldWeather
    u8 weight;
    u8 minDepth;
};

static const struct InfCaveWeatherOption sInfCaveWeathers[] =
{
    { COMPOUND_STRING("SAND"), STARTING_STATUS_WEATHER_SANDSTORM, WEATHER_SANDSTORM,      .weight = 22, .minDepth = 3 },
    { COMPOUND_STRING("RAIN"), STARTING_STATUS_WEATHER_RAIN,      WEATHER_RAIN,           .weight = 20, .minDepth = 6 },
    { COMPOUND_STRING("SUN"),  STARTING_STATUS_WEATHER_SUN,       WEATHER_DROUGHT,        .weight = 20, .minDepth = 10 },
    { COMPOUND_STRING("SNOW"), STARTING_STATUS_WEATHER_SNOW,      WEATHER_SNOW,           .weight = 18, .minDepth = 14 },
    { COMPOUND_STRING("FOG"),  STARTING_STATUS_WEATHER_FOG,       WEATHER_FOG_HORIZONTAL, .weight = 14, .minDepth = 20 },
};

// INFCAVE_MOD_TERRAIN's argument. The terrains are permanent rather than the
// five-turn variants: a room rule the player plans around must outlast the turn
// it is noticed on.
struct InfCaveTerrainOption
{
    const u8 *name;
    u8 startingStatus; // enum StartingStatus
    u8 weight;
    u8 minDepth;
};

static const struct InfCaveTerrainOption sInfCaveTerrains[] =
{
    { COMPOUND_STRING("GRASSY"),   STARTING_STATUS_GRASSY_TERRAIN,   .weight = 20, .minDepth = 3 },
    { COMPOUND_STRING("ELECTRIC"), STARTING_STATUS_ELECTRIC_TERRAIN, .weight = 20, .minDepth = 6 },
    { COMPOUND_STRING("MISTY"),    STARTING_STATUS_MISTY_TERRAIN,    .weight = 18, .minDepth = 12 },
    { COMPOUND_STRING("PSYCHIC"),  STARTING_STATUS_PSYCHIC_TERRAIN,  .weight = 16, .minDepth = 18 },
};

// INFCAVE_MOD_MONOTYPE's argument. TYPE_NONE, TYPE_MYSTERY and TYPE_STELLAR are
// left out: no filler carries them, so the room would prune to nothing and fall
// back to an unrestricted pool. A type whose band the tier's pool cannot fill
// falls back the same way (InfCavePrune), so no row needs a depth gate.
static const u8 sInfCaveMonotypes[] =
{
    TYPE_NORMAL, TYPE_FIGHTING, TYPE_FLYING, TYPE_POISON, TYPE_GROUND,
    TYPE_ROCK, TYPE_BUG, TYPE_GHOST, TYPE_STEEL, TYPE_FIRE, TYPE_WATER,
    TYPE_GRASS, TYPE_ELECTRIC, TYPE_PSYCHIC, TYPE_ICE, TYPE_DRAGON,
    TYPE_DARK, TYPE_FAIRY,
};

#endif // GUARD_DATA_INFINITY_CAVE_MODIFIERS_H
