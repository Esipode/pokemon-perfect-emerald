#include "trainer_pools.h"

// Battle Emporium reward catalogue: one row per offered item, grouped by
// building so each emporium's rows are one contiguous range. Included from
// src/battle_emporium.c only; reach it through include/battle_emporium.h.
//
// aceKey is the enum Type the challenger's ace Terastallizes to for Tera rows.
// Z-Move and Mega rows leave it TYPE_NONE: their ace key is the reward item
// itself, which the ace holds, so GetEmporiumAceKey() returns .item there.
//
// Legendary and Mythical Mega Stones are intentionally absent: their ace would
// have to be the legendary itself, which the Emporium does not hand out. The
// signature Z-Crystals of legendary / mythical species (Mewnium Z, Tapunium Z,
// Solganium Z, Lunalium Z, Marshadium Z, Ultranecrozium Z) are absent for the
// same reason, and those species carry no ace row in the pools below.

static const struct EmporiumReward gEmporiumRewards[] =
{
    // --- Z-Move Emporium: every Z-Crystal, ace holds the crystal (Badge 3) ---
    { ITEM_NORMALIUM_Z, EMPORIUM_ZMOVE, TYPE_NONE, FLAG_BADGE03_GET },
    { ITEM_FIRIUM_Z, EMPORIUM_ZMOVE, TYPE_NONE, FLAG_BADGE03_GET },
    { ITEM_WATERIUM_Z, EMPORIUM_ZMOVE, TYPE_NONE, FLAG_BADGE03_GET },
    { ITEM_ELECTRIUM_Z, EMPORIUM_ZMOVE, TYPE_NONE, FLAG_BADGE03_GET },
    { ITEM_GRASSIUM_Z, EMPORIUM_ZMOVE, TYPE_NONE, FLAG_BADGE03_GET },
    { ITEM_ICIUM_Z, EMPORIUM_ZMOVE, TYPE_NONE, FLAG_BADGE03_GET },
    { ITEM_FIGHTINIUM_Z, EMPORIUM_ZMOVE, TYPE_NONE, FLAG_BADGE03_GET },
    { ITEM_POISONIUM_Z, EMPORIUM_ZMOVE, TYPE_NONE, FLAG_BADGE03_GET },
    { ITEM_GROUNDIUM_Z, EMPORIUM_ZMOVE, TYPE_NONE, FLAG_BADGE03_GET },
    { ITEM_FLYINIUM_Z, EMPORIUM_ZMOVE, TYPE_NONE, FLAG_BADGE03_GET },
    { ITEM_PSYCHIUM_Z, EMPORIUM_ZMOVE, TYPE_NONE, FLAG_BADGE03_GET },
    { ITEM_BUGINIUM_Z, EMPORIUM_ZMOVE, TYPE_NONE, FLAG_BADGE03_GET },
    { ITEM_ROCKIUM_Z, EMPORIUM_ZMOVE, TYPE_NONE, FLAG_BADGE03_GET },
    { ITEM_GHOSTIUM_Z, EMPORIUM_ZMOVE, TYPE_NONE, FLAG_BADGE03_GET },
    { ITEM_DRAGONIUM_Z, EMPORIUM_ZMOVE, TYPE_NONE, FLAG_BADGE03_GET },
    { ITEM_DARKINIUM_Z, EMPORIUM_ZMOVE, TYPE_NONE, FLAG_BADGE03_GET },
    { ITEM_STEELIUM_Z, EMPORIUM_ZMOVE, TYPE_NONE, FLAG_BADGE03_GET },
    { ITEM_FAIRIUM_Z, EMPORIUM_ZMOVE, TYPE_NONE, FLAG_BADGE03_GET },
    { ITEM_PIKANIUM_Z, EMPORIUM_ZMOVE, TYPE_NONE, FLAG_BADGE03_GET },
    { ITEM_EEVIUM_Z, EMPORIUM_ZMOVE, TYPE_NONE, FLAG_BADGE03_GET },
    { ITEM_SNORLIUM_Z, EMPORIUM_ZMOVE, TYPE_NONE, FLAG_BADGE03_GET },
    { ITEM_DECIDIUM_Z, EMPORIUM_ZMOVE, TYPE_NONE, FLAG_BADGE03_GET },
    { ITEM_INCINIUM_Z, EMPORIUM_ZMOVE, TYPE_NONE, FLAG_BADGE03_GET },
    { ITEM_PRIMARIUM_Z, EMPORIUM_ZMOVE, TYPE_NONE, FLAG_BADGE03_GET },
    { ITEM_LYCANIUM_Z, EMPORIUM_ZMOVE, TYPE_NONE, FLAG_BADGE03_GET },
    { ITEM_MIMIKIUM_Z, EMPORIUM_ZMOVE, TYPE_NONE, FLAG_BADGE03_GET },
    { ITEM_KOMMONIUM_Z, EMPORIUM_ZMOVE, TYPE_NONE, FLAG_BADGE03_GET },
    { ITEM_ALORAICHIUM_Z, EMPORIUM_ZMOVE, TYPE_NONE, FLAG_BADGE03_GET },
    { ITEM_PIKASHUNIUM_Z, EMPORIUM_ZMOVE, TYPE_NONE, FLAG_BADGE03_GET },

    // --- Mega Emporium: every non-legendary Mega Stone, ace holds it (Badge 5) ---
    { ITEM_VENUSAURITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_VENUSAUR
    { ITEM_CHARIZARDITE_X, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_CHARIZARD
    { ITEM_CHARIZARDITE_Y, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_CHARIZARD
    { ITEM_BLASTOISINITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_BLASTOISE
    { ITEM_BEEDRILLITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_BEEDRILL
    { ITEM_PIDGEOTITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_PIDGEOT
    { ITEM_RAICHUNITE_X, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_RAICHU
    { ITEM_RAICHUNITE_Y, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_RAICHU
    { ITEM_CLEFABLITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_CLEFABLE
    { ITEM_ALAKAZITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_ALAKAZAM
    { ITEM_VICTREEBELITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_VICTREEBEL
    { ITEM_SLOWBRONITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_SLOWBRO
    { ITEM_GENGARITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_GENGAR
    { ITEM_STEELIXITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_STEELIX
    { ITEM_KANGASKHANITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_KANGASKHAN
    { ITEM_STARMINITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_STARMIE
    { ITEM_SCIZORITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_SCIZOR
    { ITEM_PINSIRITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_PINSIR
    { ITEM_GYARADOSITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_GYARADOS
    { ITEM_AERODACTYLITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_AERODACTYL
    { ITEM_DRAGONINITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_DRAGONITE
    { ITEM_MEGANIUMITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_MEGANIUM
    { ITEM_FERALIGITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_FERALIGATR
    { ITEM_AMPHAROSITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_AMPHAROS
    { ITEM_HERACRONITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_HERACROSS
    { ITEM_SKARMORITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_SKARMORY
    { ITEM_HOUNDOOMINITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_HOUNDOOM
    { ITEM_TYRANITARITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_TYRANITAR
    { ITEM_SCEPTILITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_SCEPTILE
    { ITEM_BLAZIKENITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_BLAZIKEN
    { ITEM_SWAMPERTITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_SWAMPERT
    { ITEM_GARDEVOIRITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_GARDEVOIR
    { ITEM_GALLADITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_GALLADE
    { ITEM_SABLENITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_SABLEYE
    { ITEM_MAWILITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_MAWILE
    { ITEM_AGGRONITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_AGGRON
    { ITEM_MEDICHAMITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_MEDICHAM
    { ITEM_MANECTITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_MANECTRIC
    { ITEM_SHARPEDONITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_SHARPEDO
    { ITEM_CAMERUPTITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_CAMERUPT
    { ITEM_ALTARIANITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_ALTARIA
    { ITEM_BANETTITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_BANETTE
    { ITEM_CHIMECHITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_CHIMECHO
    { ITEM_ABSOLITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_ABSOL
    { ITEM_ABSOLITE_Z, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_ABSOL
    { ITEM_GLALITITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_GLALIE
    { ITEM_FROSLASSITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_FROSLASS
    { ITEM_SALAMENCITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_SALAMENCE
    { ITEM_METAGROSSITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_METAGROSS
    { ITEM_STARAPTITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_STARAPTOR
    { ITEM_LOPUNNITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_LOPUNNY
    { ITEM_GARCHOMPITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_GARCHOMP
    { ITEM_GARCHOMPITE_Z, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_GARCHOMP
    { ITEM_LUCARIONITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_LUCARIO
    { ITEM_LUCARIONITE_Z, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_LUCARIO
    { ITEM_ABOMASITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_ABOMASNOW
    { ITEM_EMBOARITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_EMBOAR
    { ITEM_EXCADRITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_EXCADRILL
    { ITEM_AUDINITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_AUDINO
    { ITEM_SCOLIPITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_SCOLIPEDE
    { ITEM_SCRAFTINITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_SCRAFTY
    { ITEM_EELEKTROSSITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_EELEKTROSS
    { ITEM_CHANDELURITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_CHANDELURE
    { ITEM_GOLURKITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_GOLURK
    { ITEM_CHESNAUGHTITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_CHESNAUGHT
    { ITEM_DELPHOXITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_DELPHOX
    { ITEM_GRENINJITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_GRENINJA
    { ITEM_PYROARITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_PYROAR
    { ITEM_FLOETTITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_FLOETTE_ETERNAL
    { ITEM_MEOWSTICITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_MEOWSTIC_M
    { ITEM_MALAMARITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_MALAMAR
    { ITEM_BARBARACITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_BARBARACLE
    { ITEM_DRAGALGITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_DRAGALGE
    { ITEM_HAWLUCHANITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_HAWLUCHA
    { ITEM_CRABOMINITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_CRABOMINABLE
    { ITEM_GOLISOPITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_GOLISOPOD
    { ITEM_DRAMPANITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_DRAMPA
    { ITEM_FALINKSITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_FALINKS
    { ITEM_SCOVILLAINITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_SCOVILLAIN
    { ITEM_GLIMMORANITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_GLIMMORA
    { ITEM_TATSUGIRINITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_TATSUGIRI_CURLY
    { ITEM_BAXCALIBRITE, EMPORIUM_MEGA, TYPE_NONE, FLAG_BADGE05_GET }, // SPECIES_BAXCALIBUR

    // --- Tera Emporium: every Tera Shard except Stellar, ace Teras to that type (Badge 7) ---
    { ITEM_BUG_TERA_SHARD, EMPORIUM_TERA, TYPE_BUG, FLAG_BADGE07_GET },
    { ITEM_DARK_TERA_SHARD, EMPORIUM_TERA, TYPE_DARK, FLAG_BADGE07_GET },
    { ITEM_DRAGON_TERA_SHARD, EMPORIUM_TERA, TYPE_DRAGON, FLAG_BADGE07_GET },
    { ITEM_ELECTRIC_TERA_SHARD, EMPORIUM_TERA, TYPE_ELECTRIC, FLAG_BADGE07_GET },
    { ITEM_FAIRY_TERA_SHARD, EMPORIUM_TERA, TYPE_FAIRY, FLAG_BADGE07_GET },
    { ITEM_FIGHTING_TERA_SHARD, EMPORIUM_TERA, TYPE_FIGHTING, FLAG_BADGE07_GET },
    { ITEM_FIRE_TERA_SHARD, EMPORIUM_TERA, TYPE_FIRE, FLAG_BADGE07_GET },
    { ITEM_FLYING_TERA_SHARD, EMPORIUM_TERA, TYPE_FLYING, FLAG_BADGE07_GET },
    { ITEM_GHOST_TERA_SHARD, EMPORIUM_TERA, TYPE_GHOST, FLAG_BADGE07_GET },
    { ITEM_GRASS_TERA_SHARD, EMPORIUM_TERA, TYPE_GRASS, FLAG_BADGE07_GET },
    { ITEM_GROUND_TERA_SHARD, EMPORIUM_TERA, TYPE_GROUND, FLAG_BADGE07_GET },
    { ITEM_ICE_TERA_SHARD, EMPORIUM_TERA, TYPE_ICE, FLAG_BADGE07_GET },
    { ITEM_NORMAL_TERA_SHARD, EMPORIUM_TERA, TYPE_NORMAL, FLAG_BADGE07_GET },
    { ITEM_POISON_TERA_SHARD, EMPORIUM_TERA, TYPE_POISON, FLAG_BADGE07_GET },
    { ITEM_PSYCHIC_TERA_SHARD, EMPORIUM_TERA, TYPE_PSYCHIC, FLAG_BADGE07_GET },
    { ITEM_ROCK_TERA_SHARD, EMPORIUM_TERA, TYPE_ROCK, FLAG_BADGE07_GET },
    { ITEM_STEEL_TERA_SHARD, EMPORIUM_TERA, TYPE_STEEL, FLAG_BADGE07_GET },
    { ITEM_WATER_TERA_SHARD, EMPORIUM_TERA, TYPE_WATER, FLAG_BADGE07_GET },
};

// Each building's rows are one contiguous block of gEmporiumRewards (start index
// plus count). The STATIC_ASSERT below fails the build if the counts drift from
// the table length.
#define EMPORIUM_ZMOVE_REWARD_START  0
#define EMPORIUM_ZMOVE_REWARD_COUNT  29
#define EMPORIUM_MEGA_REWARD_START   (EMPORIUM_ZMOVE_REWARD_START + EMPORIUM_ZMOVE_REWARD_COUNT)
#define EMPORIUM_MEGA_REWARD_COUNT   82
#define EMPORIUM_TERA_REWARD_START   (EMPORIUM_MEGA_REWARD_START + EMPORIUM_MEGA_REWARD_COUNT)
#define EMPORIUM_TERA_REWARD_COUNT   18
#define EMPORIUM_REWARD_COUNT        (EMPORIUM_TERA_REWARD_START + EMPORIUM_TERA_REWARD_COUNT)

STATIC_ASSERT(ARRAY_COUNT(gEmporiumRewards) == EMPORIUM_REWARD_COUNT, sEmporiumRewardCountMatchesTable);

// Lowest level cap at which the pool holds a legal ace for each reward: the
// smallest EmporiumSpeciesMinLevel() (highest EVO_LEVEL* threshold on the
// pre-evolution chain) across the aces whose held Z-Crystal / Mega Stone or
// .teraType matches the row. Precomputed because EmporiumSpeciesMinLevel walks
// the whole species table per call and the instructor menu tests every row.
// Recompute if the reward table, the ace pool, or an ace species' evolution
// levels change. EmporiumRewardAceAvailable hides a
// row until GetEmporiumBattleLevelForEmporium reaches this value; the battle-time
// POOL_PRUNE_EMPORIUM still rechecks the actual rolled ace species.
static const u8 sEmporiumRewardAceMinLevel[EMPORIUM_REWARD_COUNT] =
{
    // Z-Move Emporium (Badge 3)
    1, 1, 1, 1, 32, 42, 24, 35, 25, 22, 16, 10, 1, 25, 45,
    1, 42, 23, 1, 1, 1, 34, 34, 34, 25, 1, 45, 24, 1,
    // Mega Emporium (Badge 5)
    32, 36, 36, 36, 10, 36, 1, 1, 1, 16, 21, 37, 25, 1, 1,
    1, 1, 1, 20, 1, 55, 32, 30, 30, 1, 1, 24, 55, 36, 36,
    36, 30, 20, 1, 1, 42, 37, 26, 30, 33, 35, 37, 1, 1, 1,
    42, 1, 50, 45, 34, 1, 48, 48, 1, 1, 40, 36, 31, 1, 30,
    39, 39, 41, 43, 36, 36, 36, 35, 1, 25, 30, 39, 48, 1, 1,
    30, 1, 1, 1, 35, 1, 54,
    // Tera Emporium (Badge 7)
    31, 25, 20, 1, 48, 1, 1, 30, 1, 20, 1, 48, 25, 48, 1, 1, 20, 1,
};

STATIC_ASSERT(ARRAY_COUNT(sEmporiumRewardAceMinLevel) == EMPORIUM_REWARD_COUNT, sEmporiumRewardAceMinLevelCount);

// Emporium pools: fillers then aces, one contiguous array per building so the
// runtime trainer's .party / .poolSize are a single pointer and count.
// Fillers are tiered to the unlock badge and carry no held item. Every ace
// carries MON_POOL_TAG_ACE; POOL_PICK_EMPORIUM restricts the ACE slot to the
// member whose key matches the chosen reward: held Z-Crystal / Mega Stone for
// those buildings, .teraType for Tera. Type-crystal Z aces lean on their
// level-up STAB; signature crystals carry the exact species and move
// sSignatureZMoves[] (src/battle_z_move.c) requires. Mega base species are
// cross-checked against src/data/pokemon/form_change_tables.h. Each Tera ace's
// Tera type is off its own typing and answers one of its weaknesses.

static const struct TrainerMon sEmporiumZPool[] =
{
    // Fillers - Badge 3 tier, base stat total <= 400 (POOL_PRUNE_EMPORIUM enforces
    // the cap and the no-legendary rule; keep new entries under it so a lead is
    // always available).
    { .species = SPECIES_LOMBRE, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_NUZLEAF, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_SABLEYE, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_MAWILE, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_ROSELIA, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_KIRLIA, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_CARVANHA, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_NUMEL, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_ELECTRIKE, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_VIBRAVA, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_CACNEA, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_SHROOMISH, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_GULPIN, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_SPINDA, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_DELCATTY, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_DUSKULL, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_SNORUNT, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    // Fillers - Gens 1-2 and 4-9, same tier cap as the Hoenn block above.
    // Gen 1
    { .species = SPECIES_PIDGEOTTO, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_GRAVELER, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_VOLTORB, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_KRABBY, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_NIDORINO, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    // Gen 2
    { .species = SPECIES_FLAAFFY, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_HOUNDOUR, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_CHINCHOU, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_YANMA, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_PHANPY, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    // Gen 4
    { .species = SPECIES_LUXIO, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_STARAVIA, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_BUIZEL, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_DRIFLOON, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_SNOVER, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    // Gen 5
    { .species = SPECIES_KROKOROK, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_SCRAGGY, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_PAWNIARD, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_ZORUA, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_DWEBBLE, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    // Gen 6
    { .species = SPECIES_PANCHAM, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_FLETCHINDER, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_HONEDGE, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_LITLEO, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_SKRELP, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    // Gen 7
    { .species = SPECIES_CRABRAWLER, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_TRUMBEAK, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_SALANDIT, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_CHARJABUG, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_STUFFUL, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    // Gen 8
    { .species = SPECIES_CORVISQUIRE, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_MORGREM, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_DOTTLER, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_HATTREM, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_CUFANT, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    // Gen 9
    { .species = SPECIES_MASCHIFF, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_NACLSTACK, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_PAWMO, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_FRIGIBAX, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_TINKATUFF, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    // Expanded filler pool (gens 1-9), tiered to the building BST cap.
    { .species = SPECIES_MUNCHLAX, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_LEDIAN, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_WAILMER, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_KRICKETUNE, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_VANILLISH, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_TYRUNT, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_MUDBRAY, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_SILICOBRA, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_DOLLIV, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_POLIWHIRL, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_AIPOM, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_DUSTOX, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_CRANIDOS, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_BOLDORE, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_AMAURA, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_ROWLET, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_GROOKEY, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_GLIMMET, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_NIDORINA, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_MANTYKE, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_NOSEPASS, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_SHIELDON, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_GOTHORITA, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_ESPURR, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_LITTEN, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_SCORBUNNY, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_TOEDSCOOL, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_MAGBY, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_SKIPLOOM, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_LOUDRED, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_BUNEARY, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_PALPITOAD, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_SKIDDO, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_POPPLIO, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_SOBBLE, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_CETODDLE, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_ELEKID, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_TEDDIURSA, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_LILEEP, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_HIPPOPOTAS, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_SWADLOON, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_SPRITZEE, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_SANDYGAST, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_CLOBBOPUS, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_FINIZEN, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_RHYHORN, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_NATU, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_ANORITH, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_SKORUPI, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_HERDIER, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_SWIRLIX, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_MAREANIE, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_SIZZLIPEDE, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_FIDOUGH, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_BULBASAUR, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_CHIKORITA, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_CLAMPERL, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_FINNEON, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_DUOSION, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_CLAUNCHER, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_CUTIEFLY, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_SPRIGATITO, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_SLOWPOKE, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_TOTODILE, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_ARON, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_STUNKY, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_LAMPENT, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_FROAKIE, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_GRUBBIN, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_FUECOCO, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_DODUO, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_CYNDAQUIL, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_SPOINK, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_TURTWIG, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_VULLABY, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_CHESPIN, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_JANGMO_O, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_QUAXLY, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_MIME_JR, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_SNUBBULL, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_TREECKO, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_PIPLUP, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_WHIRLIPEDE, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_PHANTUMP, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_STEENEE, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_TANDEMAUS, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_MACHOP, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_REMORAID, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_TORCHIC, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_GLAMEOW, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_LARVESTA, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_FENNEKIN, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_MORELULL, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_CAPSAKID, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_SMOOCHUM, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_LARVITAR, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_MUDKIP, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_CHIMCHAR, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_TRANQUILL, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_BINACLE, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_VAROOM, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_POLIWAG, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_BONSLY, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_SWABLU, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_BRONZOR, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_TIRTOUGA, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_BERGMITE, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_TINKATINK, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_GEODUDE, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_PINECO, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_CORPHISH, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_GIBLE, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_MIENFOO, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_GOOMY, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_SHROODLE, .lvl = 30, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    // Aces
    { .species = SPECIES_SNORLAX, .lvl = 35, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_NORMALIUM_Z, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_KANGASKHAN, .lvl = 35, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_NORMALIUM_Z, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_ARCANINE, .lvl = 35, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_FIRIUM_Z, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_CHARIZARD, .lvl = 35, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_FIRIUM_Z, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_MILOTIC, .lvl = 35, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_WATERIUM_Z, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_SHARPEDO, .lvl = 35, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_WATERIUM_Z, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_MANECTRIC, .lvl = 35, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_ELECTRIUM_Z, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_JOLTEON, .lvl = 35, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_ELECTRIUM_Z, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_SCEPTILE, .lvl = 35, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_GRASSIUM_Z, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_VENUSAUR, .lvl = 35, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_GRASSIUM_Z, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_WALREIN, .lvl = 35, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_ICIUM_Z, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_GLALIE, .lvl = 35, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_ICIUM_Z, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_MACHAMP, .lvl = 35, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_FIGHTINIUM_Z, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_HARIYAMA, .lvl = 35, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_FIGHTINIUM_Z, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_MUK, .lvl = 35, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_POISONIUM_Z, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_WEEZING, .lvl = 35, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_POISONIUM_Z, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_FLYGON, .lvl = 35, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_GROUNDIUM_Z, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_DONPHAN, .lvl = 35, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_GROUNDIUM_Z, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_SWELLOW, .lvl = 35, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_FLYINIUM_Z, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_PIDGEOT, .lvl = 35, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_FLYINIUM_Z, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_ALAKAZAM, .lvl = 35, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_PSYCHIUM_Z, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_GARDEVOIR, .lvl = 35, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_PSYCHIUM_Z, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_BEEDRILL, .lvl = 35, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_BUGINIUM_Z, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_ARIADOS, .lvl = 35, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_BUGINIUM_Z, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_TYRANITAR, .lvl = 35, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_ROCKIUM_Z, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_AERODACTYL, .lvl = 35, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_ROCKIUM_Z, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_GENGAR, .lvl = 35, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_GHOSTIUM_Z, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_BANETTE, .lvl = 35, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_GHOSTIUM_Z, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_SALAMENCE, .lvl = 35, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_DRAGONIUM_Z, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_FLYGON, .lvl = 35, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_DRAGONIUM_Z, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_ABSOL, .lvl = 35, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_DARKINIUM_Z, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_HOUNDOOM, .lvl = 35, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_DARKINIUM_Z, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_METAGROSS, .lvl = 35, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_STEELIUM_Z, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_AGGRON, .lvl = 35, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_STEELIUM_Z, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_GARDEVOIR, .lvl = 35, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_FAIRIUM_Z, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_GRANBULL, .lvl = 35, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_FAIRIUM_Z, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_PIKACHU, .lvl = 35, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_PIKANIUM_Z, .moves = { MOVE_VOLT_TACKLE, MOVE_THUNDERBOLT, MOVE_QUICK_ATTACK, MOVE_IRON_TAIL }, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_EEVEE, .lvl = 35, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_EEVIUM_Z, .moves = { MOVE_LAST_RESORT, MOVE_QUICK_ATTACK, MOVE_BITE, MOVE_SWIFT }, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_SNORLAX, .lvl = 35, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_SNORLIUM_Z, .moves = { MOVE_GIGA_IMPACT, MOVE_BODY_SLAM, MOVE_CRUNCH, MOVE_EARTHQUAKE }, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_DECIDUEYE, .lvl = 35, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_DECIDIUM_Z, .moves = { MOVE_SPIRIT_SHACKLE, MOVE_LEAF_BLADE, MOVE_SHADOW_SNEAK, MOVE_BRAVE_BIRD }, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_INCINEROAR, .lvl = 35, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_INCINIUM_Z, .moves = { MOVE_DARKEST_LARIAT, MOVE_FLARE_BLITZ, MOVE_EARTHQUAKE, MOVE_U_TURN }, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_PRIMARINA, .lvl = 35, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_PRIMARIUM_Z, .moves = { MOVE_SPARKLING_ARIA, MOVE_MOONBLAST, MOVE_PSYCHIC, MOVE_ICE_BEAM }, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_LYCANROC_MIDDAY, .lvl = 35, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_LYCANIUM_Z, .moves = { MOVE_STONE_EDGE, MOVE_CRUNCH, MOVE_FIRE_FANG, MOVE_ACCELEROCK }, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_MIMIKYU_DISGUISED, .lvl = 35, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_MIMIKIUM_Z, .moves = { MOVE_PLAY_ROUGH, MOVE_SHADOW_CLAW, MOVE_SHADOW_SNEAK, MOVE_SWORDS_DANCE }, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_KOMMO_O, .lvl = 35, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_KOMMONIUM_Z, .moves = { MOVE_CLANGING_SCALES, MOVE_CLOSE_COMBAT, MOVE_POISON_JAB, MOVE_DRAGON_DANCE }, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_RAICHU_ALOLA, .lvl = 35, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_ALORAICHIUM_Z, .moves = { MOVE_THUNDERBOLT, MOVE_PSYCHIC, MOVE_SURF, MOVE_GRASS_KNOT }, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_PIKACHU_ORIGINAL, .lvl = 35, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_PIKASHUNIUM_Z, .moves = { MOVE_THUNDERBOLT, MOVE_VOLT_TACKLE, MOVE_QUICK_ATTACK, MOVE_IRON_TAIL }, .tags = MON_POOL_TAG_ACE },
};

static const struct TrainerMon sEmporiumMegaPool[] =
{
    // Fillers
    { .species = SPECIES_MIGHTYENA, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_LUDICOLO, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_SHIFTRY, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_SWELLOW, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_FLYGON, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_LAIRON, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_SEALEO, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_GRUMPIG, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_CLAYDOL, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_CACTURNE, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_CRAWDAUNT, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_WHISCASH, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_HARIYAMA, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_MILOTIC, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_TROPIUS, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_ARMALDO, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_CRADILY, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_LANTURN, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_DUSCLOPS, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    // Fillers - Gens 1-2 and 4-9, same tier cap as the Hoenn block above.
    // Gen 1
    { .species = SPECIES_PERSIAN, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_RAPIDASH, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_FEAROW, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_KINGLER, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_DRAGONAIR, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    // Gen 2
    { .species = SPECIES_GRANBULL, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_URSARING, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_MILTANK, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_OCTILLERY, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_XATU, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    // Gen 4
    { .species = SPECIES_DRIFBLIM, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_FLOATZEL, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_SKUNTANK, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_BRONZONG, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_PURUGLY, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    // Gen 5
    { .species = SPECIES_ZEBSTRIKA, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_LIEPARD, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_SWANNA, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_CINCCINO, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_GARBODOR, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    // Gen 6
    { .species = SPECIES_TALONFLAME, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_PANGORO, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_HELIOLISK, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_DIGGERSBY, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_AROMATISSE, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    // Gen 7
    { .species = SPECIES_TOUCANNON, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_SALAZZLE, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_RIBOMBEE, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_ARAQUANID, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_TOGEDEMARU, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    // Gen 8
    { .species = SPECIES_BOLTUND, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_THIEVUL, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_DUBWOOL, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_GREEDENT, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_FROSMOTH, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    // Gen 9
    { .species = SPECIES_BELLIBOLT, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_GRAFAIAI, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_KILOWATTREL, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_DACHSBUN, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_KLAWF, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    // Expanded filler pool (gens 1-9), tiered to the building BST cap.
    { .species = SPECIES_POLITOED, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_MISMAGIUS, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_WAILORD, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_DRAPION, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_SIMISAGE, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_CLAWITZER, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_VIKAVOLT, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_COPPERAJAH, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_GARGANACL, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_KLEAVOR, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_AMBIPOM, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_HUNTAIL, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_RAMPARDOS, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_SIMISEAR, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_CARBINK, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_MUDSDALE, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_CORVIKNIGHT, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_REVAVROOM, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_SLOWKING, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_FORRETRESS, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_GOREBYSS, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_BASTIODON, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_SIMIPOUR, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_SLURPUFF, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_BEWEAR, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_BARRASKEWDA, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_FLAMIGO, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_RHYDON, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_STANTLER, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_RELICANTH, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_SPIRITOMB, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_CARRACOSTA, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_TREVENANT, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_TOXAPEX, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_DREDNAW, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_PAWMOT, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_HITMONTOP, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_GIRAFARIG, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_TORKOAL, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_VESPIQUEN, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_ESCAVALIER, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_KLEFKI, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_ORANGURU, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_FLAPPLE, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_HOUNDSTONE, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_PERRSERKER, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_PILOSWINE, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_ZANGOOSE, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_LUMINEON, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_ACCELGOR, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_SLIGGOO, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_PASSIMIAN, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_APPLETUN, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_BOMBIRDIER, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_PONYTA, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_MISDREAVUS, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_SEVIPER, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_CARNIVINE, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_SIGILYPH, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_DOUBLADE, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_COMFEY, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_DIPPLIN, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_ESPATHRA, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_IVYSAUR, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_QUAGSIRE, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_NINJASK, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_GOTHITELLE, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_DEDENNE, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_TURTONATOR, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_GRAPPLOCT, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_BRAMBLEGHAST, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_MUNCHLAX, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_CLODSIRE, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_VIGOROTH, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_CHATOT, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_REUNICLUS, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_BRAIXEN, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_LURANTIS, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_CRAMORANT, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_ORTHWORM, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_POLIWHIRL, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_GLIGAR, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_LINOONE, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_BIBAREL, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_BOUFFALANT, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_QUILLADIN, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_PALOSSAND, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_STONJOURNER, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_VELUZA, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_SNEASEL, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_SHELGON, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_GABITE, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_MUSHARNA, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_FROGADIER, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_KOMALA, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_ELDEGOSS, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_RABSCA, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_SUNFLORA, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_METANG, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_GROTLE, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_BEHEEYEM, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_BRUXISH, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_PINCURCHIN, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_LOKIX, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_FURRET, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_GROVYLE, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_MONFERNO, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_DRUDDIGON, .lvl = 45, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    // Aces
    { .species = SPECIES_VENUSAUR, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_VENUSAURITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_CHARIZARD, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_CHARIZARDITE_X, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_CHARIZARD, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_CHARIZARDITE_Y, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_BLASTOISE, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_BLASTOISINITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_BEEDRILL, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_BEEDRILLITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_PIDGEOT, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_PIDGEOTITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_RAICHU, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_RAICHUNITE_X, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_RAICHU, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_RAICHUNITE_Y, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_CLEFABLE, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_CLEFABLITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_ALAKAZAM, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_ALAKAZITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_VICTREEBEL, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_VICTREEBELITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_SLOWBRO, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_SLOWBRONITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_GENGAR, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_GENGARITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_STEELIX, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_STEELIXITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_KANGASKHAN, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_KANGASKHANITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_STARMIE, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_STARMINITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_SCIZOR, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_SCIZORITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_PINSIR, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_PINSIRITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_GYARADOS, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_GYARADOSITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_AERODACTYL, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_AERODACTYLITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_DRAGONITE, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_DRAGONINITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_MEGANIUM, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_MEGANIUMITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_FERALIGATR, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_FERALIGITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_AMPHAROS, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_AMPHAROSITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_HERACROSS, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_HERACRONITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_SKARMORY, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_SKARMORITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_HOUNDOOM, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_HOUNDOOMINITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_TYRANITAR, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_TYRANITARITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_SCEPTILE, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_SCEPTILITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_BLAZIKEN, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_BLAZIKENITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_SWAMPERT, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_SWAMPERTITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_GARDEVOIR, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_GARDEVOIRITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_GALLADE, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_GALLADITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_SABLEYE, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_SABLENITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_MAWILE, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_MAWILITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_AGGRON, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_AGGRONITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_MEDICHAM, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_MEDICHAMITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_MANECTRIC, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_MANECTITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_SHARPEDO, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_SHARPEDONITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_CAMERUPT, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_CAMERUPTITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_ALTARIA, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_ALTARIANITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_BANETTE, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_BANETTITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_CHIMECHO, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_CHIMECHITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_ABSOL, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_ABSOLITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_ABSOL, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_ABSOLITE_Z, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_GLALIE, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_GLALITITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_FROSLASS, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_FROSLASSITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_SALAMENCE, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_SALAMENCITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_METAGROSS, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_METAGROSSITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_STARAPTOR, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_STARAPTITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_LOPUNNY, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_LOPUNNITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_GARCHOMP, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_GARCHOMPITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_GARCHOMP, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_GARCHOMPITE_Z, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_LUCARIO, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_LUCARIONITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_LUCARIO, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_LUCARIONITE_Z, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_ABOMASNOW, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_ABOMASITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_EMBOAR, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_EMBOARITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_EXCADRILL, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_EXCADRITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_AUDINO, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_AUDINITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_SCOLIPEDE, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_SCOLIPITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_SCRAFTY, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_SCRAFTINITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_EELEKTROSS, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_EELEKTROSSITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_CHANDELURE, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_CHANDELURITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_GOLURK, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_GOLURKITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_CHESNAUGHT, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_CHESNAUGHTITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_DELPHOX, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_DELPHOXITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_GRENINJA, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_GRENINJITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_PYROAR, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_PYROARITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_FLOETTE_ETERNAL, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_FLOETTITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_MEOWSTIC_M, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_MEOWSTICITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_MALAMAR, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_MALAMARITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_BARBARACLE, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_BARBARACITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_DRAGALGE, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_DRAGALGITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_HAWLUCHA, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_HAWLUCHANITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_CRABOMINABLE, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_CRABOMINITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_GOLISOPOD, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_GOLISOPITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_DRAMPA, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_DRAMPANITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_FALINKS, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_FALINKSITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_SCOVILLAIN, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_SCOVILLAINITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_GLIMMORA, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_GLIMMORANITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_TATSUGIRI_CURLY, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_TATSUGIRINITE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_BAXCALIBUR, .lvl = 50, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_BAXCALIBRITE, .tags = MON_POOL_TAG_ACE },
};

static const struct TrainerMon sEmporiumTeraPool[] =
{
    // Fillers
    { .species = SPECIES_MILOTIC, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_MANECTRIC, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_FLYGON, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_WALREIN, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_GARDEVOIR, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_ALTARIA, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_CRAWDAUNT, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_CACTURNE, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_ARMALDO, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_CRADILY, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_GLALIE, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_LUDICOLO, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_SWALOT, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_CAMERUPT, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_CLAYDOL, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_KECLEON, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    // Fillers - Gens 1-2 and 4-9, same tier cap as the Hoenn block above.
    // Gen 1
    { .species = SPECIES_CROBAT, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_RHYPERIOR, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_PORYGON_Z, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_EXEGGUTOR, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_UMBREON, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    // Gen 2
    { .species = SPECIES_TYPHLOSION, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_FERALIGATR, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_MAMOSWINE, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_TOGEKISS, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_HONCHKROW, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    // Gen 4
    { .species = SPECIES_LUXRAY, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_EMPOLEON, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_TORTERRA, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_INFERNAPE, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_HIPPOWDON, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    // Gen 5
    { .species = SPECIES_HAXORUS, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_CONKELDURR, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_ZOROARK, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_SAMUROTT, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_SERPERIOR, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    // Gen 6
    { .species = SPECIES_GOODRA, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_NOIVERN, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_DELPHOX, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_CHESNAUGHT, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_TYRANTRUM, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    // Gen 7
    { .species = SPECIES_DECIDUEYE, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_INCINEROAR, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_TSAREENA, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_GOLISOPOD, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_DHELMISE, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    // Gen 8
    { .species = SPECIES_DRAGAPULT, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_RILLABOOM, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_CINDERACE, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_INTELEON, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_HATTERENE, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    // Gen 9
    { .species = SPECIES_MEOWSCARADA, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_SKELEDIRGE, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_QUAQUAVAL, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_GHOLDENGO, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_TINKATON, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    // Expanded filler pool (gens 1-9), tiered to the building BST cap.
    { .species = SPECIES_BLISSEY, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_URSALUNA, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_PROBOPASS, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_ARCHEOPS, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_GOGOAT, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_VIKAVOLT, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_ARCHALUDON, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_DONDOZO, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_KINGDRA, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_WYRDEER, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_DUSKNOIR, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_KINGAMBIT, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_AURORUS, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_MUDSDALE, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_HYDRAPPLE, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_ARMAROUGE, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_ELECTIVIRE, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_FARIGIRAF, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_OBSTAGOON, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_VANILLUXE, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_AVALUGG, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_BEWEAR, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_DURALUDON, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_CERULEDGE, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_MAGMORTAR, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_YANMEGA, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_WAILORD, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_KLINKLANG, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_CLAWITZER, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_TOXAPEX, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_CENTISKORCH, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_CETITAN, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_ANNIHILAPE, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_GLISCOR, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_HUNTAIL, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_MIENSHAO, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_CARBINK, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_ORANGURU, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_COALOSSAL, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_TOEDSCRUEL, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_TANGROWTH, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_OVERQWIL, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_GOREBYSS, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_DRAPION, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_BRAVIARY, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_SLURPUFF, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_PASSIMIAN, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_SANDACONDA, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_ARBOLIVA, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_ESPEON, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_SNEASLER, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_RELICANTH, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_RAMPARDOS, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_MANDIBUZZ, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_TREVENANT, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_COMFEY, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_GRIMMSNARL, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_MABOSSTIFF, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_LEAFEON, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_CURSOLA, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_TORKOAL, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_BASTIODON, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_SIMISAGE, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_KLEFKI, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_TURTONATOR, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_ORBEETLE, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_CYCLIZAR, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_GLACEON, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_MISMAGIUS, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_SPIRITOMB, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_SIMISEAR, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_LURANTIS, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_DRACOZOLT, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_GARGANACL, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_SYLVEON, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_AMBIPOM, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_VESPIQUEN, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_SIMIPOUR, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_PALOSSAND, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_ARCTOZOLT, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_REVAVROOM, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_MR_RIME, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_FORRETRESS, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_LUMINEON, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_CARRACOSTA, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_KOMALA, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_DRACOVISH, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_FLAMIGO, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_LICKILICKY, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_STANTLER, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_ESCAVALIER, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_BRUXISH, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_ARCTOVISH, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_PAWMOT, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_PORYGON2, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_ACCELGOR, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_COPPERAJAH, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_HOUNDSTONE, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_SIRFETCHD, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_SIGILYPH, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_CORVIKNIGHT, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_BOMBIRDIER, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_POLITOED, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_GOTHITELLE, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_BARRASKEWDA, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    { .species = SPECIES_ESPATHRA, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD },
    { .species = SPECIES_KLEAVOR, .lvl = 60, .iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20), .gender = TRAINER_MON_RANDOM_GENDER, .tags = 0 },
    // Aces
    { .species = SPECIES_TYRANITAR, .lvl = 65, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_LEFTOVERS, .teraType = TYPE_BUG, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_EXCADRILL, .lvl = 65, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_LIFE_ORB, .teraType = TYPE_BUG, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_BISHARP, .lvl = 65, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_LEFTOVERS, .teraType = TYPE_BUG, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_GENGAR, .lvl = 65, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_LIFE_ORB, .teraType = TYPE_DARK, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_METAGROSS, .lvl = 65, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_LEFTOVERS, .teraType = TYPE_DARK, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_TOXICROAK, .lvl = 65, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_LIFE_ORB, .teraType = TYPE_DARK, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_CHARIZARD, .lvl = 65, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_LIFE_ORB, .teraType = TYPE_DRAGON, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_GYARADOS, .lvl = 65, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_LEFTOVERS, .teraType = TYPE_DRAGON, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_VOLCARONA, .lvl = 65, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_LIFE_ORB, .teraType = TYPE_DRAGON, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_HERACROSS, .lvl = 65, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_LIFE_ORB, .teraType = TYPE_ELECTRIC, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_BRELOOM, .lvl = 65, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_LIFE_ORB, .teraType = TYPE_ELECTRIC, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_SHIFTRY, .lvl = 65, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_LIFE_ORB, .teraType = TYPE_ELECTRIC, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_GARCHOMP, .lvl = 65, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_LIFE_ORB, .teraType = TYPE_FAIRY, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_DRAGONITE, .lvl = 65, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_LEFTOVERS, .teraType = TYPE_FAIRY, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_HYDREIGON, .lvl = 65, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_LIFE_ORB, .teraType = TYPE_FAIRY, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_CHARIZARD, .lvl = 65, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_LIFE_ORB, .teraType = TYPE_FIGHTING, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_ABSOL, .lvl = 65, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_LIFE_ORB, .teraType = TYPE_FIGHTING, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_WEAVILE, .lvl = 65, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_LIFE_ORB, .teraType = TYPE_FIGHTING, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_SCIZOR, .lvl = 65, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_LIFE_ORB, .teraType = TYPE_FIRE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_FERROTHORN, .lvl = 65, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_LEFTOVERS, .teraType = TYPE_FIRE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_ABOMASNOW, .lvl = 65, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_LIFE_ORB, .teraType = TYPE_FIRE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_EXCADRILL, .lvl = 65, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_LIFE_ORB, .teraType = TYPE_FLYING, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_TYRANITAR, .lvl = 65, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_LEFTOVERS, .teraType = TYPE_FLYING, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_MAGNEZONE, .lvl = 65, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_LEFTOVERS, .teraType = TYPE_FLYING, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_HARIYAMA, .lvl = 65, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_LEFTOVERS, .teraType = TYPE_GHOST, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_BISHARP, .lvl = 65, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_LEFTOVERS, .teraType = TYPE_GHOST, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_SNORLAX, .lvl = 65, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_LEFTOVERS, .teraType = TYPE_GHOST, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_AGGRON, .lvl = 65, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_LEFTOVERS, .teraType = TYPE_GRASS, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_GYARADOS, .lvl = 65, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_LEFTOVERS, .teraType = TYPE_GRASS, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_MAGNEZONE, .lvl = 65, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_LEFTOVERS, .teraType = TYPE_GRASS, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_GYARADOS, .lvl = 65, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_LEFTOVERS, .teraType = TYPE_GROUND, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_SKARMORY, .lvl = 65, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_LEFTOVERS, .teraType = TYPE_GROUND, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_PELIPPER, .lvl = 65, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_LEFTOVERS, .teraType = TYPE_GROUND, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_DRAGONITE, .lvl = 65, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_LEFTOVERS, .teraType = TYPE_ICE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_GARCHOMP, .lvl = 65, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_LIFE_ORB, .teraType = TYPE_ICE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_SALAMENCE, .lvl = 65, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_LIFE_ORB, .teraType = TYPE_ICE, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_GENGAR, .lvl = 65, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_LIFE_ORB, .teraType = TYPE_NORMAL, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_METAGROSS, .lvl = 65, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_LEFTOVERS, .teraType = TYPE_NORMAL, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_BANETTE, .lvl = 65, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_LIFE_ORB, .teraType = TYPE_NORMAL, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_HYDREIGON, .lvl = 65, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_LIFE_ORB, .teraType = TYPE_POISON, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_GARCHOMP, .lvl = 65, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_LIFE_ORB, .teraType = TYPE_POISON, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_DRAGONITE, .lvl = 65, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_LEFTOVERS, .teraType = TYPE_POISON, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_SNORLAX, .lvl = 65, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_LEFTOVERS, .teraType = TYPE_PSYCHIC, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_TYRANITAR, .lvl = 65, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_LEFTOVERS, .teraType = TYPE_PSYCHIC, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_BISHARP, .lvl = 65, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_LEFTOVERS, .teraType = TYPE_PSYCHIC, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_SCIZOR, .lvl = 65, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_LIFE_ORB, .teraType = TYPE_ROCK, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_HERACROSS, .lvl = 65, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_LIFE_ORB, .teraType = TYPE_ROCK, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_ABOMASNOW, .lvl = 65, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_LIFE_ORB, .teraType = TYPE_ROCK, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_SALAMENCE, .lvl = 65, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_LIFE_ORB, .teraType = TYPE_STEEL, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_VOLCARONA, .lvl = 65, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_LIFE_ORB, .teraType = TYPE_STEEL, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_GYARADOS, .lvl = 65, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_LEFTOVERS, .teraType = TYPE_STEEL, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_SCIZOR, .lvl = 65, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_LIFE_ORB, .teraType = TYPE_WATER, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_FERROTHORN, .lvl = 65, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_LEFTOVERS, .teraType = TYPE_WATER, .tags = MON_POOL_TAG_ACE },
    { .species = SPECIES_SALAMENCE, .lvl = 65, .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .gender = TRAINER_MON_RANDOM_GENDER, .heldItem = ITEM_LIFE_ORB, .teraType = TYPE_WATER, .tags = MON_POOL_TAG_ACE },
};

#define EMPORIUM_ZMOVE_POOL_SIZE  219
#define EMPORIUM_MEGA_POOL_SIZE   249
#define EMPORIUM_TERA_POOL_SIZE   217

// struct Trainer.poolSize is a u8 and POOL_SLOT_DISABLED is 0xFF, so a pool has
// to stay under 255 members as well as match its array.
STATIC_ASSERT(ARRAY_COUNT(sEmporiumZPool) == EMPORIUM_ZMOVE_POOL_SIZE, sEmporiumZPoolSize);
STATIC_ASSERT(ARRAY_COUNT(sEmporiumMegaPool) == EMPORIUM_MEGA_POOL_SIZE, sEmporiumMegaPoolSize);
STATIC_ASSERT(ARRAY_COUNT(sEmporiumTeraPool) == EMPORIUM_TERA_POOL_SIZE, sEmporiumTeraPoolSize);
STATIC_ASSERT(EMPORIUM_ZMOVE_POOL_SIZE < POOL_SLOT_DISABLED, sEmporiumZPoolFitsU8);
STATIC_ASSERT(EMPORIUM_MEGA_POOL_SIZE < POOL_SLOT_DISABLED, sEmporiumMegaPoolFitsU8);
STATIC_ASSERT(EMPORIUM_TERA_POOL_SIZE < POOL_SLOT_DISABLED, sEmporiumTeraPoolFitsU8);

// Challenger identities, rolled per attempt. Shared by all three buildings.
static const u8 sEmporiumName_Aidan[]  = _("AIDAN");
static const u8 sEmporiumName_Blake[]  = _("BLAKE");
static const u8 sEmporiumName_Carla[]  = _("CARLA");
static const u8 sEmporiumName_Dina[]   = _("DINA");
static const u8 sEmporiumName_Errol[]  = _("ERROL");
static const u8 sEmporiumName_Fern[]   = _("FERN");
static const u8 sEmporiumName_Grady[]  = _("GRADY");
static const u8 sEmporiumName_Hazel[]  = _("HAZEL");

static const struct EmporiumIdentity sEmporiumIdentities[] =
{
    { TRAINER_CLASS_YOUNGSTER, TRAINER_PIC_YOUNGSTER, sEmporiumName_Aidan, OBJ_EVENT_GFX_YOUNGSTER, TRAINER_ENCOUNTER_MUSIC_MALE, TRAINER_GENDER_MALE },
    { TRAINER_CLASS_LASS, TRAINER_PIC_LASS, sEmporiumName_Carla, OBJ_EVENT_GFX_LASS, TRAINER_ENCOUNTER_MUSIC_FEMALE, TRAINER_GENDER_FEMALE },
    { TRAINER_CLASS_HIKER, TRAINER_PIC_HIKER, sEmporiumName_Grady, OBJ_EVENT_GFX_HIKER, TRAINER_ENCOUNTER_MUSIC_HIKER, TRAINER_GENDER_MALE },
    { TRAINER_CLASS_BEAUTY, TRAINER_PIC_BEAUTY, sEmporiumName_Hazel, OBJ_EVENT_GFX_BEAUTY, TRAINER_ENCOUNTER_MUSIC_FEMALE, TRAINER_GENDER_FEMALE },
    { TRAINER_CLASS_BUG_CATCHER, TRAINER_PIC_BUG_CATCHER, sEmporiumName_Blake, OBJ_EVENT_GFX_BUG_CATCHER, TRAINER_ENCOUNTER_MUSIC_MALE, TRAINER_GENDER_MALE },
    { TRAINER_CLASS_FISHERMAN, TRAINER_PIC_FISHERMAN, sEmporiumName_Errol, OBJ_EVENT_GFX_FISHERMAN, TRAINER_ENCOUNTER_MUSIC_HIKER, TRAINER_GENDER_MALE },
    { TRAINER_CLASS_CAMPER, TRAINER_PIC_CAMPER, sEmporiumName_Aidan, OBJ_EVENT_GFX_CAMPER, TRAINER_ENCOUNTER_MUSIC_MALE, TRAINER_GENDER_MALE },
    { TRAINER_CLASS_PICNICKER, TRAINER_PIC_PICNICKER, sEmporiumName_Fern, OBJ_EVENT_GFX_PICNICKER, TRAINER_ENCOUNTER_MUSIC_GIRL, TRAINER_GENDER_FEMALE },
    { TRAINER_CLASS_COOLTRAINER, TRAINER_PIC_COOLTRAINER_M, sEmporiumName_Blake, OBJ_EVENT_GFX_MAN_3, TRAINER_ENCOUNTER_MUSIC_COOL, TRAINER_GENDER_MALE },
    { TRAINER_CLASS_COOLTRAINER, TRAINER_PIC_COOLTRAINER_F, sEmporiumName_Dina, OBJ_EVENT_GFX_WOMAN_5, TRAINER_ENCOUNTER_MUSIC_COOL, TRAINER_GENDER_FEMALE },
    { TRAINER_CLASS_PSYCHIC, TRAINER_PIC_PSYCHIC_M, sEmporiumName_Errol, OBJ_EVENT_GFX_PSYCHIC_M, TRAINER_ENCOUNTER_MUSIC_INTENSE, TRAINER_GENDER_MALE },
    { TRAINER_CLASS_POKEMANIAC, TRAINER_PIC_POKEMANIAC, sEmporiumName_Grady, OBJ_EVENT_GFX_MANIAC, TRAINER_ENCOUNTER_MUSIC_SUSPICIOUS, TRAINER_GENDER_MALE },
};

#define EMPORIUM_IDENTITY_COUNT  12

