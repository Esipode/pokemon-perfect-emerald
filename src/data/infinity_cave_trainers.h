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

// Species pool per tier. The cave draws on the Battle Emporium's pools rather
// than a second copy of them: their filler bands (base stat total <= 400 / 500 /
// 600) are exactly the cave's tiers, and their ace rows already hold the
// Z-Crystals, Mega Stones and Tera types INFCAVE_MOD_GIMMICK needs.
static const u8 sInfCaveTierPool[INFCAVE_TIER_COUNT] =
{
    EMPORIUM_ZMOVE,
    EMPORIUM_MEGA,
    EMPORIUM_TERA,
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
    0,
    9,
    20,
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
struct InfCaveTrainerSpec
{
    u8 partySize;
    s8 tierOffset;
    u8 minCount;
    u8 maxCount;
    u8 shardReward;
    u64 aiFlags;
};

static const struct InfCaveTrainerSpec sInfCaveTrainerSpec[INFCAVE_ROOM_COUNT] =
{
    [INFCAVE_ROOM_BATTLE]   = { .partySize = 3, .tierOffset =  0, .minCount = 2, .maxCount = 3, .shardReward = 10, .aiFlags = AI_FLAG_SMART_TRAINER },
    [INFCAVE_ROOM_GAUNTLET] = { .partySize = 2, .tierOffset = -1, .minCount = 4, .maxCount = 6, .shardReward =  8, .aiFlags = AI_FLAG_SMART_TRAINER },
    [INFCAVE_ROOM_ELITE]    = { .partySize = 6, .tierOffset =  1, .minCount = 1, .maxCount = 1, .shardReward = 25, .aiFlags = AI_FLAG_SMART_TRAINER | AI_FLAG_ACE_POKEMON },
    [INFCAVE_ROOM_BOSS]     = { .partySize = 6, .tierOffset =  1, .minCount = 1, .maxCount = 1, .shardReward = 40, .aiFlags = AI_FLAG_SMART_TRAINER | AI_FLAG_ACE_POKEMON },
    [INFCAVE_ROOM_REST]     = { .partySize = 0, .tierOffset =  0, .minCount = 0, .maxCount = 0, .shardReward =  0, .aiFlags = 0 },
    [INFCAVE_ROOM_TREASURE] = { .partySize = 0, .tierOffset =  0, .minCount = 0, .maxCount = 0, .shardReward =  0, .aiFlags = 0 },
    [INFCAVE_ROOM_SHOP]     = { .partySize = 0, .tierOffset =  0, .minCount = 0, .maxCount = 0, .shardReward =  0, .aiFlags = 0 },
};

#endif // GUARD_DATA_INFINITY_CAVE_TRAINERS_H
