#ifndef GUARD_DATA_INFINITY_CAVE_TRAINERS_H
#define GUARD_DATA_INFINITY_CAVE_TRAINERS_H

// Rolled opponent data for the Infinity Cave. Included from
// src/infinity_cave_trainers.c only; the room generator's data lives in
// src/data/infinity_cave.h.

static const u8 sInfCaveName_Ardo[]   = _("ARDO");
static const u8 sInfCaveName_Brix[]   = _("BRIX");
static const u8 sInfCaveName_Corin[]  = _("CORIN");
static const u8 sInfCaveName_Delve[]  = _("DELVE");
static const u8 sInfCaveName_Ember[]  = _("EMBER");
static const u8 sInfCaveName_Fossa[]  = _("FOSSA");
static const u8 sInfCaveName_Garnet[] = _("GARNET");
static const u8 sInfCaveName_Hollis[] = _("HOLLIS");
static const u8 sInfCaveName_Ingo[]   = _("INGO");
static const u8 sInfCaveName_Jessa[]  = _("JESSA");
static const u8 sInfCaveName_Kern[]   = _("KERN");
static const u8 sInfCaveName_Lumen[]  = _("LUMEN");
static const u8 sInfCaveName_Mirel[]  = _("MIREL");
static const u8 sInfCaveName_Norv[]   = _("NORV");
static const u8 sInfCaveName_Orlan[]  = _("ORLAN");
static const u8 sInfCaveName_Pyra[]   = _("PYRA");
static const u8 sInfCaveName_Quarry[] = _("QUARRY");
static const u8 sInfCaveName_Rhodes[] = _("RHODES");
static const u8 sInfCaveName_Slate[]  = _("SLATE");
static const u8 sInfCaveName_Tunnel[] = _("TUNNEL");
static const u8 sInfCaveName_Umbra[]  = _("UMBRA");
static const u8 sInfCaveName_Vex[]    = _("VEX");
static const u8 sInfCaveName_Warden[] = _("WARDEN");
static const u8 sInfCaveName_Yarrow[] = _("YARROW");

// Classes that read as cave-dwellers. objectGfxId is the sprite the placer gives
// the NPC (Stage 15), so every row needs one that exists.
static const struct InfCaveIdentity sInfCaveIdentities[] =
{
    { TRAINER_CLASS_HIKER,        TRAINER_PIC_HIKER,          sInfCaveName_Ardo,   OBJ_EVENT_GFX_HIKER,       TRAINER_ENCOUNTER_MUSIC_HIKER,      TRAINER_GENDER_MALE },
    { TRAINER_CLASS_HIKER,        TRAINER_PIC_HIKER,          sInfCaveName_Brix,   OBJ_EVENT_GFX_HIKER,       TRAINER_ENCOUNTER_MUSIC_HIKER,      TRAINER_GENDER_MALE },
    { TRAINER_CLASS_RUIN_MANIAC,  TRAINER_PIC_RUIN_MANIAC,    sInfCaveName_Delve,  OBJ_EVENT_GFX_MANIAC,      TRAINER_ENCOUNTER_MUSIC_HIKER,      TRAINER_GENDER_MALE },
    { TRAINER_CLASS_RUIN_MANIAC,  TRAINER_PIC_RUIN_MANIAC,    sInfCaveName_Quarry, OBJ_EVENT_GFX_MANIAC,      TRAINER_ENCOUNTER_MUSIC_HIKER,      TRAINER_GENDER_MALE },
    { TRAINER_CLASS_POKEMANIAC,   TRAINER_PIC_POKEMANIAC,     sInfCaveName_Kern,   OBJ_EVENT_GFX_MANIAC,      TRAINER_ENCOUNTER_MUSIC_SUSPICIOUS, TRAINER_GENDER_MALE },
    { TRAINER_CLASS_HEX_MANIAC,   TRAINER_PIC_HEX_MANIAC,     sInfCaveName_Umbra,  OBJ_EVENT_GFX_HEX_MANIAC,  TRAINER_ENCOUNTER_MUSIC_SUSPICIOUS, TRAINER_GENDER_FEMALE },
    { TRAINER_CLASS_HEX_MANIAC,   TRAINER_PIC_HEX_MANIAC,     sInfCaveName_Mirel,  OBJ_EVENT_GFX_HEX_MANIAC,  TRAINER_ENCOUNTER_MUSIC_SUSPICIOUS, TRAINER_GENDER_FEMALE },
    { TRAINER_CLASS_BLACK_BELT,   TRAINER_PIC_BLACK_BELT,     sInfCaveName_Norv,   OBJ_EVENT_GFX_BLACK_BELT,  TRAINER_ENCOUNTER_MUSIC_INTENSE,    TRAINER_GENDER_MALE },
    { TRAINER_CLASS_BATTLE_GIRL,  TRAINER_PIC_BATTLE_GIRL,    sInfCaveName_Pyra,   OBJ_EVENT_GFX_BLACK_BELT,  TRAINER_ENCOUNTER_MUSIC_INTENSE,    TRAINER_GENDER_FEMALE },
    { TRAINER_CLASS_KINDLER,      TRAINER_PIC_KINDLER,        sInfCaveName_Ember,  OBJ_EVENT_GFX_MAN_3,       TRAINER_ENCOUNTER_MUSIC_HIKER,      TRAINER_GENDER_MALE },
    { TRAINER_CLASS_PSYCHIC,      TRAINER_PIC_PSYCHIC_M,      sInfCaveName_Lumen,  OBJ_EVENT_GFX_PSYCHIC_M,   TRAINER_ENCOUNTER_MUSIC_INTENSE,    TRAINER_GENDER_MALE },
    { TRAINER_CLASS_PSYCHIC,      TRAINER_PIC_PSYCHIC_F,      sInfCaveName_Jessa,  OBJ_EVENT_GFX_BEAUTY,      TRAINER_ENCOUNTER_MUSIC_INTENSE,    TRAINER_GENDER_FEMALE },
    { TRAINER_CLASS_EXPERT,       TRAINER_PIC_EXPERT_M,       sInfCaveName_Ingo,   OBJ_EVENT_GFX_EXPERT_M,    TRAINER_ENCOUNTER_MUSIC_INTENSE,    TRAINER_GENDER_MALE },
    { TRAINER_CLASS_EXPERT,       TRAINER_PIC_EXPERT_F,       sInfCaveName_Fossa,  OBJ_EVENT_GFX_EXPERT_F,    TRAINER_ENCOUNTER_MUSIC_INTENSE,    TRAINER_GENDER_FEMALE },
    { TRAINER_CLASS_COOLTRAINER,  TRAINER_PIC_COOLTRAINER_M,  sInfCaveName_Rhodes, OBJ_EVENT_GFX_MAN_3,       TRAINER_ENCOUNTER_MUSIC_COOL,       TRAINER_GENDER_MALE },
    { TRAINER_CLASS_COOLTRAINER,  TRAINER_PIC_COOLTRAINER_F,  sInfCaveName_Garnet, OBJ_EVENT_GFX_WOMAN_5,     TRAINER_ENCOUNTER_MUSIC_COOL,       TRAINER_GENDER_FEMALE },
    { TRAINER_CLASS_COLLECTOR,    TRAINER_PIC_COLLECTOR,      sInfCaveName_Slate,  OBJ_EVENT_GFX_MANIAC,      TRAINER_ENCOUNTER_MUSIC_SUSPICIOUS, TRAINER_GENDER_MALE },
    { TRAINER_CLASS_BUG_MANIAC,   TRAINER_PIC_BUG_MANIAC,     sInfCaveName_Vex,    OBJ_EVENT_GFX_BUG_CATCHER, TRAINER_ENCOUNTER_MUSIC_SUSPICIOUS, TRAINER_GENDER_MALE },
    { TRAINER_CLASS_NINJA_BOY,    TRAINER_PIC_NINJA_BOY,      sInfCaveName_Corin,  OBJ_EVENT_GFX_NINJA_BOY,   TRAINER_ENCOUNTER_MUSIC_SUSPICIOUS, TRAINER_GENDER_MALE },
    { TRAINER_CLASS_DRAGON_TAMER, TRAINER_PIC_DRAGON_TAMER,   sInfCaveName_Orlan,  OBJ_EVENT_GFX_MAN_5,       TRAINER_ENCOUNTER_MUSIC_INTENSE,    TRAINER_GENDER_MALE },
    { TRAINER_CLASS_AROMA_LADY,   TRAINER_PIC_AROMA_LADY,     sInfCaveName_Yarrow, OBJ_EVENT_GFX_WOMAN_3,     TRAINER_ENCOUNTER_MUSIC_FEMALE,     TRAINER_GENDER_FEMALE },
    { TRAINER_CLASS_POKEFAN,      TRAINER_PIC_POKEFAN_F,      sInfCaveName_Hollis, OBJ_EVENT_GFX_POKEFAN_F,   TRAINER_ENCOUNTER_MUSIC_TWINS,      TRAINER_GENDER_FEMALE },
    { TRAINER_CLASS_GENTLEMAN,    TRAINER_PIC_GENTLEMAN,      sInfCaveName_Warden, OBJ_EVENT_GFX_GENTLEMAN,   TRAINER_ENCOUNTER_MUSIC_RICH,       TRAINER_GENDER_MALE },
    { TRAINER_CLASS_SAILOR,       TRAINER_PIC_SAILOR,         sInfCaveName_Tunnel, OBJ_EVENT_GFX_SAILOR,      TRAINER_ENCOUNTER_MUSIC_MALE,       TRAINER_GENDER_MALE },
};

#define INFCAVE_IDENTITY_COUNT ARRAY_COUNT(sInfCaveIdentities)

// Boss identities. A depth divisible by INFCAVE_BOSS_INTERVAL stands one of
// these on its arena instead of a rolled cave trainer.
//
// partyTrainer names the trainer whose authored team the boss fights with. The
// team is only borrowed when it already holds a full INFCAVE_ROOM_BOSS party
// (the gym-leader rematch entries and every Elite Four / Champion entry do); a
// shorter one, or TRAINER_NONE, rolls a pooled team at the boss tier instead, so
// every boss fields six regardless. Tate & Liza are left out: their entry is a
// double battle behind a two-trainer pic, which the cave's single-battle arena
// and one overworld sprite cannot represent.
//
// The class is not stored: a boss holds no office in the cave, so every row
// battles as TRAINER_CLASS_CHALLENGER.
static const u8 sInfCaveBossName_Roxanne[]  = _("ROXANNE");
static const u8 sInfCaveBossName_Brawly[]   = _("BRAWLY");
static const u8 sInfCaveBossName_Wattson[]  = _("WATTSON");
static const u8 sInfCaveBossName_Flannery[] = _("FLANNERY");
static const u8 sInfCaveBossName_Norman[]   = _("NORMAN");
static const u8 sInfCaveBossName_Winona[]   = _("WINONA");
static const u8 sInfCaveBossName_Juan[]     = _("JUAN");
static const u8 sInfCaveBossName_Sidney[]   = _("SIDNEY");
static const u8 sInfCaveBossName_Phoebe[]   = _("PHOEBE");
static const u8 sInfCaveBossName_Glacia[]   = _("GLACIA");
static const u8 sInfCaveBossName_Drake[]    = _("DRAKE");
static const u8 sInfCaveBossName_Wallace[]  = _("WALLACE");
static const u8 sInfCaveBossName_Steven[]   = _("STEVEN");
static const u8 sInfCaveBossName_Brendan[]  = _("BRENDAN");
static const u8 sInfCaveBossName_May[]      = _("MAY");

static const struct InfCaveBoss sInfCaveBosses[] =
{
    { TRAINER_PIC_LEADER_ROXANNE,  sInfCaveBossName_Roxanne,  OBJ_EVENT_GFX_ROXANNE,  TRAINER_ROXANNE_1,  TRAINER_ENCOUNTER_MUSIC_FEMALE,     MUGSHOT_COLOR_YELLOW, TRAINER_GENDER_FEMALE },
    { TRAINER_PIC_LEADER_BRAWLY,   sInfCaveBossName_Brawly,   OBJ_EVENT_GFX_BRAWLY,   TRAINER_BRAWLY_1,   TRAINER_ENCOUNTER_MUSIC_MALE,       MUGSHOT_COLOR_BLUE,   TRAINER_GENDER_MALE },
    { TRAINER_PIC_LEADER_WATTSON,  sInfCaveBossName_Wattson,  OBJ_EVENT_GFX_WATTSON,  TRAINER_WATTSON_5,  TRAINER_ENCOUNTER_MUSIC_MALE,       MUGSHOT_COLOR_YELLOW, TRAINER_GENDER_MALE },
    { TRAINER_PIC_LEADER_FLANNERY, sInfCaveBossName_Flannery, OBJ_EVENT_GFX_FLANNERY, TRAINER_FLANNERY_5, TRAINER_ENCOUNTER_MUSIC_FEMALE,     MUGSHOT_COLOR_PINK,   TRAINER_GENDER_FEMALE },
    { TRAINER_PIC_LEADER_NORMAN,   sInfCaveBossName_Norman,   OBJ_EVENT_GFX_NORMAN,   TRAINER_NORMAN_5,   TRAINER_ENCOUNTER_MUSIC_MALE,       MUGSHOT_COLOR_PURPLE, TRAINER_GENDER_MALE },
    { TRAINER_PIC_LEADER_WINONA,   sInfCaveBossName_Winona,   OBJ_EVENT_GFX_WINONA,   TRAINER_WINONA_5,   TRAINER_ENCOUNTER_MUSIC_FEMALE,     MUGSHOT_COLOR_GREEN,  TRAINER_GENDER_FEMALE },
    { TRAINER_PIC_LEADER_JUAN,     sInfCaveBossName_Juan,     OBJ_EVENT_GFX_JUAN,     TRAINER_JUAN_5,     TRAINER_ENCOUNTER_MUSIC_MALE,       MUGSHOT_COLOR_BLUE,   TRAINER_GENDER_MALE },
    { TRAINER_PIC_ELITE_FOUR_SIDNEY, sInfCaveBossName_Sidney, OBJ_EVENT_GFX_SIDNEY,   TRAINER_SIDNEY,     TRAINER_ENCOUNTER_MUSIC_ELITE_FOUR, MUGSHOT_COLOR_PURPLE, TRAINER_GENDER_MALE },
    { TRAINER_PIC_ELITE_FOUR_PHOEBE, sInfCaveBossName_Phoebe, OBJ_EVENT_GFX_PHOEBE,   TRAINER_PHOEBE,     TRAINER_ENCOUNTER_MUSIC_ELITE_FOUR, MUGSHOT_COLOR_GREEN,  TRAINER_GENDER_FEMALE },
    { TRAINER_PIC_ELITE_FOUR_GLACIA, sInfCaveBossName_Glacia, OBJ_EVENT_GFX_GLACIA,   TRAINER_GLACIA,     TRAINER_ENCOUNTER_MUSIC_ELITE_FOUR, MUGSHOT_COLOR_PINK,   TRAINER_GENDER_FEMALE },
    { TRAINER_PIC_ELITE_FOUR_DRAKE,  sInfCaveBossName_Drake,  OBJ_EVENT_GFX_DRAKE,    TRAINER_DRAKE,      TRAINER_ENCOUNTER_MUSIC_ELITE_FOUR, MUGSHOT_COLOR_BLUE,   TRAINER_GENDER_MALE },
    { TRAINER_PIC_CHAMPION_WALLACE,  sInfCaveBossName_Wallace, OBJ_EVENT_GFX_WALLACE, TRAINER_WALLACE,    TRAINER_ENCOUNTER_MUSIC_MALE,       MUGSHOT_COLOR_YELLOW, TRAINER_GENDER_MALE },
    { TRAINER_PIC_STEVEN,            sInfCaveBossName_Steven, OBJ_EVENT_GFX_STEVEN,   TRAINER_STEVEN,     TRAINER_ENCOUNTER_MUSIC_MALE,       MUGSHOT_COLOR_BLUE,   TRAINER_GENDER_MALE },
    { TRAINER_PIC_BRENDAN,           sInfCaveBossName_Brendan, OBJ_EVENT_GFX_BRENDAN_NORMAL, TRAINER_NONE, TRAINER_ENCOUNTER_MUSIC_MALE,      MUGSHOT_COLOR_GREEN,  TRAINER_GENDER_MALE },
    { TRAINER_PIC_MAY,               sInfCaveBossName_May,    OBJ_EVENT_GFX_MAY_NORMAL, TRAINER_NONE,     TRAINER_ENCOUNTER_MUSIC_FEMALE,     MUGSHOT_COLOR_PINK,   TRAINER_GENDER_FEMALE },
};

#define INFCAVE_BOSS_COUNT ARRAY_COUNT(sInfCaveBosses)

// Species pool per tier. The cave draws on the Battle Emporium's pools rather
// than a second copy of them: their filler bands (base stat total <= 400 / 500 /
// 600) are exactly the cave's tiers, and their ace rows already hold the
// Z-Crystals, Mega Stones and Tera types INFCAVE_MOD_GIMMICK needs.
static const u8 sInfCaveTierPool[INFCAVE_TIER_COUNT] =
{
    EMPORIUM_ZMOVE,
    EMPORIUM_TERA,
    EMPORIUM_MEGA,
};

// Base-stat-total band per tier. The maxima match the Emporium filler caps the
// pools are authored to; the minima keep a deep room from fielding the early
// tiers' weakest mons when a pool is shared with a shallower tier.
// POOL_PRUNE_INFCAVE drops everything outside the band, and relaxes the minimum
// if that would leave fewer members than the party needs.
struct InfCaveBstBand
{
    u16 minBst;
    u16 maxBst;
};

static const struct InfCaveBstBand sInfCaveTierBst[INFCAVE_TIER_COUNT] =
{
    { .minBst =   0, .maxBst = 400 },
    { .minBst = 330, .maxBst = 500 },
    { .minBst = 400, .maxBst = 600 },
};

// Lowest depth that reads each tier, ascending. Depth 0 (the lobby) reads tier 0.
static const u8 sInfCaveTierMinDepth[INFCAVE_TIER_COUNT] =
{
    5,
    15,
    25,
};

// Per-room-type opponent shape. tierOffset shifts the depth's tier, so a
// gauntlet's many trainers are individually weaker and an elite is stronger than
// the depth alone would give. partySize 0 means the room rolls no trainers.
// minCount/maxCount are how many trainers the room places before
// INFCAVE_MOD_SWARM; the total is clamped to INFCAVE_MAX_TRAINERS, which the
// widest case (a swarming gauntlet) reaches exactly.
// shardReward is what one of the room's trainers pays when beaten, before the
// depth growth and the modifier scaling in InfCave_GetBattleShards. A gauntlet
// trainer pays less than a battle-room trainer but the room fields more of them,
// so clearing a gauntlet is still the better haul.
// partySize is the room type's party size before any boss has fallen; the ramp
// in src/infinity_cave_trainers.c grows it with every boss beaten. fullParty
// rooms field that size exactly, where the rest roll between the ramp's floor
// and it.
struct InfCaveTrainerSpec
{
    u8 partySize;
    s8 tierOffset;
    u8 minCount;
    u8 maxCount;
    u8 shardReward;
    bool8 fullParty;
    u64 aiFlags;
};

static const struct InfCaveTrainerSpec sInfCaveTrainerSpec[INFCAVE_ROOM_COUNT] =
{
    [INFCAVE_ROOM_BATTLE]   = { .partySize = 3, .tierOffset =  0, .minCount = 2, .maxCount = 3, .shardReward = 10, .aiFlags = AI_FLAG_SMART_TRAINER },
    [INFCAVE_ROOM_GAUNTLET] = { .partySize = 2, .tierOffset = -1, .minCount = 4, .maxCount = 6, .shardReward =  8, .aiFlags = AI_FLAG_SMART_TRAINER },
    [INFCAVE_ROOM_ELITE]    = { .partySize = 3, .tierOffset =  1, .minCount = 1, .maxCount = 1, .shardReward = 25, .fullParty = TRUE, .aiFlags = AI_FLAG_SMART_TRAINER | AI_FLAG_ACE_POKEMON },
    [INFCAVE_ROOM_BOSS]     = { .partySize = 3, .tierOffset =  1, .minCount = 1, .maxCount = 1, .shardReward = 40, .fullParty = TRUE, .aiFlags = AI_FLAG_UNFAIR_TRAINER | AI_FLAG_ACE_POKEMON },
    [INFCAVE_ROOM_REST]     = { .partySize = 0, .tierOffset =  0, .minCount = 0, .maxCount = 0, .shardReward =  0, .aiFlags = 0 },
    [INFCAVE_ROOM_TREASURE] = { .partySize = 0, .tierOffset =  0, .minCount = 0, .maxCount = 0, .shardReward =  0, .aiFlags = 0 },
    [INFCAVE_ROOM_SHOP]     = { .partySize = 0, .tierOffset =  0, .minCount = 0, .maxCount = 0, .shardReward =  0, .aiFlags = 0 },
};

#endif // GUARD_DATA_INFINITY_CAVE_TRAINERS_H
