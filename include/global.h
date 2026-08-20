#ifndef GUARD_GLOBAL_H
#define GUARD_GLOBAL_H

#include <string.h>
#include <limits.h>
#include "config/general.h" // we need to define config before gba headers as print stuff needs the functions nulled before defines.
#include "gba/gba.h"
#include "assertf.h"
#include "gametypes.h"
#include "siirtc.h"
#include "fpmath.h"
#include "metaprogram.h"
#include "constants/global.h"
#include "constants/flags.h"
#include "constants/vars.h"
#include "constants/species.h"
#include "constants/pokedex.h"
#include "constants/apricorn_tree.h"
#include "constants/berry.h"
#include "constants/maps.h"
#include "constants/region_map_sections.h" // MAPSEC_COUNT, for NUM_NUZLOCKE_ZONE_FLAG_BYTES
#include "constants/pokemon.h"
#include "constants/easy_chat.h"
#include "constants/trainer_hill.h"
#include "constants/trainer_tower.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/player_customization.h"
#include "config/save.h"

// Prevent cross-jump optimization.
#define BLOCK_CROSS_JUMP asm("");

// to help in decompiling
#define asm_unified(x) asm(".syntax unified\n" x "\n.syntax divided")
#define NAKED __attribute__((naked))

#if MODERN
#define asm __asm__
#endif

/// IDE support
#if defined(__APPLE__) || defined(__CYGWIN__) || defined(__INTELLISENSE__)
// We define these when using certain IDEs to fool preproc
#define _(x)        {x}
#define __(x)       {x}
#define COMPOUND_STRING(x) 0
#define INCBIN(...) {0}
#define INCBIN_U8   INCBIN
#define INCBIN_U16  INCBIN
#define INCBIN_U32  INCBIN
#define INCBIN_COMP INCBIN
#define INCGFX(...) {0}
#define INCGFX_U8   INCGFX
#define INCGFX_U16  INCGFX
#define INCGFX_U32  INCGFX
#define INCGFX_COMP INCGFX
#endif // IDE support

#define ARRAY_COUNT(array) (size_t)(sizeof(array) / sizeof((array)[0]))

// GameFreak used a macro called "NELEMS", as evidenced by
// AgbAssert calls.
#define NELEMS(arr) (sizeof(arr)/sizeof(*(arr)))

#define SWAP(a, b, temp)    \
{                           \
    temp = a;               \
    a = b;                  \
    b = temp;               \
}

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) >= (b) ? (a) : (b))

#if MODERN
#define abs(x) (((x) < 0) ? -(x) : (x))
#endif

// Used in cases where division by 0 can occur in the retail version.
// Avoids invalid opcodes on some emulators, and the otherwise UB.
#ifdef UBFIX
#define SAFE_DIV(a, b) (((b) != 0) ? (a) / (b) : 0)
#else
#define SAFE_DIV(a, b) ((a) / (b))
#endif

#define IS_POW_OF_TWO(n) (((n) & ((n)-1)) == 0)

// The below macro does a%n, but (to match) will switch to a&(n-1) if n is a power of 2.
// There are cases where GF does a&(n-1) where we would really like to have a%n, because
// if n is changed to a value that isn't a power of 2 then a&(n-1) is unlikely to work as
// intended, and a%n for powers of 2 isn't always optimized to use &.
#define MOD(a, n) (((n) & ((n)-1)) ? ((a) % (n)) : ((a) & ((n)-1)))

// Increments 'a' by 1, wrapping back to 0 when it reaches 'n'. If 'n' is a power of two,
// the wrap is implemented using a bit mask: (a + 1) & (n - 1), which is slightly faster.
// This is intended to be used when 'n' is known at compile time.
#define INCREMENT_OR_WRAP(a, n) ((IS_POW_OF_TWO(n)) ? (((a) + 1) & ((n) - 1)) : (((a) + 1) >= (n) ? 0 : ((a) + 1)))

// Extracts the upper 16 bits of a 32-bit number
#define HIHALF(n) (((n) & 0xFFFF0000) >> 16)

// Extracts the lower 16 bits of a 32-bit number
#define LOHALF(n) ((n) & 0xFFFF)

// There are many quirks in the source code which have overarching behavioral differences from
// a number of other files. For example, diploma.c seems to declare rodata before each use while
// other files declare out of order and must be at the beginning. There are also a number of
// macros which differ from one file to the next due to the method of obtaining the result, such
// as these below. Because of this, there is a theory (Two Team Theory) that states that these
// programming projects had more than 1 "programming team" which utilized different macros for
// each of the files that were worked on.
#define T1_READ_8(ptr)  ((ptr)[0])
#define T1_READ_16(ptr) ((ptr)[0] | ((ptr)[1] << 8))
#define T1_READ_32(ptr) ((ptr)[0] | ((ptr)[1] << 8) | ((ptr)[2] << 16) | ((ptr)[3] << 24))
#define T1_READ_PTR(ptr) (u8 *) T1_READ_32(ptr)

// T2_READ_8 is a duplicate to remain consistent with each group.
#define T2_READ_8(ptr)  ((ptr)[0])
#define T2_READ_16(ptr) ((ptr)[0] + ((ptr)[1] << 8))
#define T2_READ_32(ptr) ((ptr)[0] + ((ptr)[1] << 8) + ((ptr)[2] << 16) + ((ptr)[3] << 24))
#define T2_READ_PTR(ptr) (void *) T2_READ_32(ptr)

#define PACK(data, shift, mask)   ( ((data) << (shift)) & (mask) )
#define UNPACK(data, shift, mask) ( ((data) & (mask)) >> (shift) )

// Macros for checking the joypad
#define TEST_BUTTON(field, button) ((field) & (button))
#define JOY_NEW(button) TEST_BUTTON(gMain.newKeys,  button)
#define JOY_HELD(button)  TEST_BUTTON(gMain.heldKeys, button)
#define JOY_HELD_RAW(button) TEST_BUTTON(gMain.heldKeysRaw, button)
#define JOY_REPEAT(button) TEST_BUTTON(gMain.newAndRepeatedKeys, button)

#define S16TOPOSFLOAT(val)   \
({                           \
    s16 v = (val);           \
    float f = (float)v;      \
    if (v < 0) f += 65536.0f;\
    f;                       \
})

#define DIV_ROUND_UP(val, roundBy) (((val) / (roundBy)) + (((val) % (roundBy)) ? 1 : 0))

#define ROUND_BITS_TO_BYTES(numBits) DIV_ROUND_UP(numBits, 8)

#define NUM_DEX_FLAG_BYTES ROUND_BITS_TO_BYTES(POKEMON_SLOTS_NUMBER)
#define NUM_FLAG_BYTES ROUND_BITS_TO_BYTES(FLAGS_COUNT)
#define NUM_TRENDY_SAYING_BYTES ROUND_BITS_TO_BYTES(NUM_TRENDY_SAYINGS)

#define NUM_APRICORN_TREE_BYTES ROUND_BITS_TO_BYTES(APRICORN_TREE_COUNT)

// This produces an error at compile-time if expr is zero.
// It looks like file.c:line: size of array `id' is negative
#define STATIC_ASSERT(expr, id) typedef char id[(expr) ? 1 : -1];

#define FEATURE_FLAG_ASSERT(flag, id) STATIC_ASSERT(flag > TEMP_FLAGS_END || flag == 0, id)

#define READ_OTID_FROM_SAVE T1_READ_32(gSaveBlock2Ptr->playerTrainerId)

// NOTE: This uses hardware timers 2 and 3; this will not work during active link connections or with the eReader
static inline void CycleCountStart()
{
    REG_TM2CNT_H = 0;
    REG_TM3CNT_H = 0;

    REG_TM2CNT_L = 0;
    REG_TM3CNT_L = 0;

    // init timers (tim3 count up mode, tim2 every clock cycle)
    REG_TM3CNT_H = TIMER_ENABLE | TIMER_COUNTUP;
    REG_TM2CNT_H = TIMER_1CLK | TIMER_ENABLE;
}

static inline u32 CycleCountEnd()
{
    // stop timers
    REG_TM2CNT_H = 0;
    REG_TM3CNT_H = 0;

    // return result
    return REG_TM2CNT_L | (REG_TM3CNT_L << 16u);
}

struct Coords8
{
    s8 x;
    s8 y;
};

struct UCoords8
{
    u8 x;
    u8 y;
};

struct Coords16
{
    s16 x;
    s16 y;
};

struct UCoords16
{
    u16 x;
    u16 y;
};

struct Coords32
{
    s32 x;
    s32 y;
};

struct UCoords32
{
    u32 x;
    u32 y;
};

struct Time
{
    /*0x00*/ s16 days;
    /*0x02*/ s8 hours;
    /*0x03*/ s8 minutes;
    /*0x04*/ s8 seconds;
};

struct NPCFollowerPadding
{
    u8 padding1;
    u8 padding2;
    u8 padding3;
};

struct NPCFollower
{
    u8 inProgress:1;
    u8 warpEnd:1;
    u8 createSurfBlob:2;
    u8 comeOutDoorStairs:2;
    u8 forcedMovement:2;
    u8 objId;
    u8 currentSprite;
    u8 delayedState;
    struct NPCFollowerPadding padding;
    struct Coords16 log;
    const u8 *script;
    u16 flag;
    u16 graphicsId;
    u16 flags;
    u8 battlePartner; // If you have more than 255 total battle partners defined, change this to a u16
};

#include "constants/items.h"
#define ITEM_FLAGS_COUNT ((ITEMS_COUNT / 8) + ((ITEMS_COUNT % 8) ? 1 : 0))

struct SaveBlock3
{
#if OW_USE_FAKE_RTC
    struct SiiRtcInfo fakeRTC;
#endif
#if FNPC_ENABLE_NPC_FOLLOWERS
    struct NPCFollower NPCfollower;
#endif
#if OW_SHOW_ITEM_DESCRIPTIONS == OW_ITEM_DESCRIPTIONS_FIRST_TIME
    u8 itemFlags[ITEM_FLAGS_COUNT];
#endif
#if USE_DEXNAV_SEARCH_LEVELS == TRUE
    u8 dexNavSearchLevels[NUM_SPECIES];
#endif
    u8 dexNavChain;
#if APRICORN_TREE_COUNT > 0
    u8 apricornTrees[NUM_APRICORN_TREE_BYTES];
#endif
}; /* max size 1624 bytes */

extern struct SaveBlock3 *gSaveBlock3Ptr;

struct Pokedex
{
    /*0x00*/ u8 order;
    /*0x01*/ u8 mode;
    /*0x02*/ u8 nationalMagic; // must equal 0xDA in order to have National mode
    /*0x03*/ u8 unknown2;
    /*0x04*/ u32 unownPersonality; // set when you first see Unown
    /*0x08*/ u32 spindaPersonality; // set when you first see Spinda
    /*0x0C*/ u32 unknown3;
#if FREE_EXTRA_SEEN_FLAGS_SAVEBLOCK2 == FALSE
    /*0x10*/ u8 filler[0x68]; // Previously Dex Flags, feel free to remove.
#endif //FREE_EXTRA_SEEN_FLAGS_SAVEBLOCK2
};

struct PokemonJumpRecords
{
    u16 jumpsInRow;
    u16 unused1; // Set to 0, never read
    u16 excellentsInRow;
    u16 gamesWithMaxPlayers;
    u32 unused2; // Set to 0, never read
    u32 bestJumpScore;
};

struct BerryPickingResults
{
    u32 bestScore;
    u16 berriesPicked;
    u16 berriesPickedInRow;
    u8 field_8;
    u8 field_9;
    u8 field_A;
    u8 field_B;
    u8 field_C;
    u8 field_D;
    u8 field_E;
    u8 field_F;
};

struct PyramidBag
{
    enum Item itemId[FRONTIER_LVL_MODE_COUNT][PYRAMID_BAG_ITEMS_COUNT];
#if MAX_PYRAMID_BAG_ITEM_CAPACITY > 255
    u16 quantity[FRONTIER_LVL_MODE_COUNT][PYRAMID_BAG_ITEMS_COUNT];
#else
    u8 quantity[FRONTIER_LVL_MODE_COUNT][PYRAMID_BAG_ITEMS_COUNT];
#endif
};

struct BerryCrush
{
    u16 pressingSpeeds[4]; // For the record with each possible group size, 2-5 players
    u32 berryPowderAmount;
    u32 unk;
};

struct ApprenticeMon
{
    enum Species species;
    enum Move moves[MAX_MON_MOVES];
    enum Item item;
};

// This is for past players Apprentices or Apprentices received via Record Mix.
// For the current Apprentice, see struct PlayersApprentice
struct Apprentice
{
    u8 id:5;
    u8 lvlMode:2;
    //u8 padding1:1;
    u8 numQuestions;
    u8 number;
    //u8 padding2;
    struct ApprenticeMon party[MULTI_PARTY_SIZE];
    u16 speechWon[EASY_CHAT_BATTLE_WORDS_COUNT];
    u8 playerId[TRAINER_ID_LENGTH];
    u8 playerName[PLAYER_NAME_LENGTH];
    u8 language;
    u32 checksum;
};

struct BattleTowerPokemon
{
    enum Species species;
    enum Item heldItem;
    enum Move moves[MAX_MON_MOVES];
    u16 level;
    u8 ppBonuses;
    u8 hpEV;
    u8 attackEV;
    u8 defenseEV;
    u8 speedEV;
    u8 spAttackEV;
    u8 spDefenseEV;
    u32 otId;
    u32 hpIV:5;
    u32 attackIV:5;
    u32 defenseIV:5;
    u32 speedIV:5;
    u32 spAttackIV:5;
    u32 spDefenseIV:5;
    u32 gap:1;
    u32 abilityNum:1;
    u32 personality;
    u8 nickname[VANILLA_POKEMON_NAME_LENGTH + 1];
    u8 friendship;
};

struct EmeraldBattleTowerRecord
{
    /*0x00*/ u16 lvlMode; // 0 = level 50, 1 = level 100
    /*0x01*/ u8 facilityClass;
    /*0x02*/ u16 winStreak;
    /*0x04*/ u8 name[PLAYER_NAME_LENGTH + 1];
    /*0x0C*/ u8 trainerId[TRAINER_ID_LENGTH];
    /*0x10*/ u16 greeting[EASY_CHAT_BATTLE_WORDS_COUNT];
    /*0x1C*/ u16 speechWon[EASY_CHAT_BATTLE_WORDS_COUNT];
    /*0x28*/ u16 speechLost[EASY_CHAT_BATTLE_WORDS_COUNT];
    /*0x34*/ struct BattleTowerPokemon party[MAX_FRONTIER_PARTY_SIZE];
    /*0xE4*/ u8 language;
    /*0xE7*/ //u8 padding[3];
    /*0xE8*/ u32 checksum;
};

struct BattleTowerInterview
{
    enum Species playerSpecies;
    enum Species opponentSpecies;
    u8 opponentName[PLAYER_NAME_LENGTH + 1];
    u8 opponentMonNickname[VANILLA_POKEMON_NAME_LENGTH + 1];
    u8 opponentLanguage;
};

struct BattleTowerEReaderTrainer
{
    /*0x00*/ u8 unk0;
    /*0x01*/ u8 facilityClass;
    /*0x02*/ u16 winStreak;
    /*0x04*/ u8 name[PLAYER_NAME_LENGTH + 1];
    /*0x0C*/ u8 trainerId[TRAINER_ID_LENGTH];
    /*0x10*/ u16 greeting[EASY_CHAT_BATTLE_WORDS_COUNT];
    /*0x1C*/ u16 farewellPlayerLost[EASY_CHAT_BATTLE_WORDS_COUNT];
    /*0x28*/ u16 farewellPlayerWon[EASY_CHAT_BATTLE_WORDS_COUNT];
    /*0x34*/ struct BattleTowerPokemon party[FRONTIER_PARTY_SIZE];
    /*0xB8*/ u32 checksum;
};

// For displaying party information on the player's Battle Dome tourney page
struct DomeMonData
{
    enum Move moves[MAX_MON_MOVES];
    u8 evs[NUM_STATS];
    u8 nature;
    //u8 padding;
};

struct RentalMon
{
    u16 monId;
    //u8 padding1[2];
    u32 personality;
    u8 ivs;
    u8 abilityNum;
    //u8 padding2[2];
};

struct BattleDomeTrainer
{
    u16 trainerId:10;
    u16 isEliminated:1;
    u16 eliminatedAt:2;
    u16 forfeited:3;
};

#define DOME_TOURNAMENT_TRAINERS_COUNT 16
#define BATTLE_TOWER_RECORD_COUNT 5

struct BattleFrontier
{
    /*0x64C*/ struct EmeraldBattleTowerRecord towerPlayer;
    /*0x738*/ struct EmeraldBattleTowerRecord towerRecords[BATTLE_TOWER_RECORD_COUNT]; // From record mixing.
    /*0xBEB*/ struct BattleTowerInterview towerInterview;
#if FREE_BATTLE_TOWER_E_READER == FALSE
    /*0xBEC*/ struct BattleTowerEReaderTrainer ereaderTrainer;  // 200 bytes (see include/config/save.h)
#endif //FREE_BATTLE_TOWER_E_READER
    /*0xCA8*/ u8 challengeStatus;
    /*0xCA9*/ u8 challengePaused:1;
              //u8 padding1:7; // lvlMode and disableRecordBattle used to live in this byte; both relocated to top-level SaveBlock2 fields (see gSaveBlock2Ptr->lvlMode / disableRecordBattle) since generic battle/link/recorded-battle code needs them regardless of FREE_BATTLE_FRONTIER.
    /*0xCAA*/ u16 selectedPartyMons[MAX_FRONTIER_PARTY_SIZE];
    /*0xCB2*/ u16 curChallengeBattleNum; // Battle number / room number (Pike) / floor number (Pyramid)
    /*0xCB4*/ u16 trainerIds[20];
    /*0xCDC*/ u32 winStreakActiveFlags;
    /*0xCE0*/ u16 towerWinStreaks[4][FRONTIER_LVL_MODE_COUNT];
    /*0xCF0*/ u16 towerRecordWinStreaks[4][FRONTIER_LVL_MODE_COUNT];
    /*0xD00*/ u16 battledBrainFlags;
    /*0xD02*/ u16 towerSinglesStreak; // Never read
    /*0xD04*/ u16 towerNumWins; // Increments to MAX_STREAK but never read otherwise
    /*0xD06*/ u8 towerBattleOutcome;
    /*0xD07*/ u8 towerLvlMode;
    /*0xD08*/ u8 domeAttemptedSingles50:1;
    /*0xD08*/ u8 domeAttemptedSinglesOpen:1;
    /*0xD08*/ u8 domeHasWonSingles50:1;
    /*0xD08*/ u8 domeHasWonSinglesOpen:1;
    /*0xD08*/ u8 domeAttemptedDoubles50:1;
    /*0xD08*/ u8 domeAttemptedDoublesOpen:1;
    /*0xD08*/ u8 domeHasWonDoubles50:1;
    /*0xD08*/ u8 domeHasWonDoublesOpen:1;
    /*0xD09*/ u8 domeUnused;
    /*0xD0A*/ u8 domeLvlMode;
    /*0xD0B*/ u8 domeBattleMode;
    /*0xD0C*/ u16 domeWinStreaks[2][FRONTIER_LVL_MODE_COUNT];
    /*0xD14*/ u16 domeRecordWinStreaks[2][FRONTIER_LVL_MODE_COUNT];
    /*0xD1C*/ u16 domeTotalChampionships[2][FRONTIER_LVL_MODE_COUNT];
    /*0xD24*/ struct BattleDomeTrainer domeTrainers[DOME_TOURNAMENT_TRAINERS_COUNT];
    /*0xD64*/ u16 domeMonIds[DOME_TOURNAMENT_TRAINERS_COUNT][FRONTIER_PARTY_SIZE];
    /*0xDC4*/ u16 unused_DC4;
    /*0xDC6*/ u16 palacePrize;
    /*0xDC8*/ u16 palaceWinStreaks[2][FRONTIER_LVL_MODE_COUNT];
    /*0xDD0*/ u16 palaceRecordWinStreaks[2][FRONTIER_LVL_MODE_COUNT];
    /*0xDD8*/ u16 arenaPrize;
    /*0xDDA*/ u16 arenaWinStreaks[FRONTIER_LVL_MODE_COUNT];
    /*0xDDE*/ u16 arenaRecordStreaks[FRONTIER_LVL_MODE_COUNT];
    /*0xDE2*/ u16 factoryWinStreaks[2][FRONTIER_LVL_MODE_COUNT];
    /*0xDEA*/ u16 factoryRecordWinStreaks[2][FRONTIER_LVL_MODE_COUNT];
    /*0xDF6*/ u16 factoryRentsCount[2][FRONTIER_LVL_MODE_COUNT];
    /*0xDFA*/ u16 factoryRecordRentsCount[2][FRONTIER_LVL_MODE_COUNT];
    /*0xE02*/ u16 pikePrize;
    /*0xE04*/ u16 pikeWinStreaks[FRONTIER_LVL_MODE_COUNT];
    /*0xE08*/ u16 pikeRecordStreaks[FRONTIER_LVL_MODE_COUNT];
    /*0xE0C*/ u16 pikeTotalStreaks[FRONTIER_LVL_MODE_COUNT];
    /*0xE10*/ u8 pikeHintedRoomIndex:3;
              u8 pikeHintedRoomType:4;
              u8 pikeHealingRoomsDisabled:1;
    /*0xE11*/ //u8 padding2;
    /*0xE12*/ u16 pikeHeldItemsBackup[FRONTIER_PARTY_SIZE];
    /*0xE18*/ u16 pyramidPrize;
    /*0xE1A*/ u16 pyramidWinStreaks[FRONTIER_LVL_MODE_COUNT];
    /*0xE1E*/ u16 pyramidRecordStreaks[FRONTIER_LVL_MODE_COUNT];
    /*0xE22*/ u16 pyramidRandoms[4];
    /*0xE2A*/ u8 pyramidTrainerFlags; // 1 bit for each trainer (MAX_PYRAMID_TRAINERS)
    /*0xE2B*/ //u8 padding3;
    /*0xE2C*/ struct PyramidBag pyramidBag;
    /*0xE68*/ u8 pyramidLightRadius;
    /*0xE69*/ //u8 padding4;
    /*0xE6A*/ u16 verdanturfTentPrize;
    /*0xE6C*/ u16 fallarborTentPrize;
    /*0xE6E*/ u16 slateportTentPrize;
    /*0xE70*/ struct RentalMon rentalMons[FRONTIER_PARTY_SIZE * 2];
    /*0xEB8*/ u16 battlePoints;
    /*0xEBA*/ u16 cardBattlePoints;
    /*0xEBC*/ u32 battlesCount;
    /*0xEC0*/ u16 domeWinningMoves[DOME_TOURNAMENT_TRAINERS_COUNT];
    /*0xEE0*/ u8 trainerFlags;
    /*0xEE1*/ u8 opponentNames[FRONTIER_LVL_MODE_COUNT][PLAYER_NAME_LENGTH + 1];
    /*0xEF1*/ u8 opponentTrainerIds[FRONTIER_LVL_MODE_COUNT][TRAINER_ID_LENGTH];
    /*0xEF9*/ u8 unk_EF9:7; // Never read
    /*0xEF9*/ u8 savedGame:1;
    /*0xEFA*/ u8 unused_EFA;
    /*0xEFB*/ u8 unused_EFB;
    /*0xEFC*/ struct DomeMonData domePlayerPartyData[FRONTIER_PARTY_SIZE];
};

struct ApprenticeQuestion
{
    u8 questionId:2;
    u8 monId:2;
    u8 moveSlot:2;
    u8 suggestedChange:2; // TRUE if told to use held item or second move, FALSE if told to use no item or first move
    //u8 padding;
    u16 data; // used both as an itemId and a move
};

struct PlayersApprentice
{
    /*0xB0*/ u8 id;
    /*0xB1*/ u8 lvlMode:2;  //0: Unassigned, 1: Lv 50, 2: Open Lv
    /*0xB1*/ u8 questionsAnswered:4;
    /*0xB1*/ u8 leadMonId:2;
    /*0xB2*/ u8 party:3;
             u8 saveId:2;
             //u8 padding1:3;
    /*0xB3*/ u8 unused;
    /*0xB4*/ u8 speciesIds[MULTI_PARTY_SIZE];
    /*0xB7*/ //u8 padding2;
    /*0xB8*/ struct ApprenticeQuestion questions[APPRENTICE_MAX_QUESTIONS];
};

struct RankingHall1P
{
    u8 id[TRAINER_ID_LENGTH];
    u16 winStreak;
    u8 name[PLAYER_NAME_LENGTH + 1];
    u8 language;
    //u8 padding;
};

struct RankingHall2P
{
    u8 id1[TRAINER_ID_LENGTH];
    u8 id2[TRAINER_ID_LENGTH];
    u16 winStreak;
    u8 name1[PLAYER_NAME_LENGTH + 1];
    u8 name2[PLAYER_NAME_LENGTH + 1];
    u8 language;
    //u8 padding;
};

// Run-scoped data for the exploration/economy
// entries, kept in SaveBlock2 rather than AchievementRunData (SaveBlock1) --
// SaveBlock1 only had 12 bytes of slack left by the time these fields were
// added (confirmed via temporary compiler-error probes added to src/save.c
// after a real build failed on the SaveBlock1FreeSpace STATIC_ASSERT), and
// these 163 bytes didn't fit. SaveBlock2 had 1304 bytes free, comfortably
// enough. Kept as its own struct/field, not merged into SaveBlock2's
// existing fields or into AchievementRunData, so this relocation only
// touches this code (src/achievements.c) and not the rest of it.
struct AchievementRunDataExt
{
    // Distinct (mapGroup, mapNum) pairs entered this run, for
    // Cartographer/etc. NOT a raw-mapNum bitfield -- an earlier
    // sketch proposed indexing 128 bits by mapNum alone, but mapNum resets
    // per map GROUP (MAP_GROUPS_COUNT == 75), so two unrelated maps in
    // different groups routinely share a mapNum, and this tracker has no
    // scoping precondition that would make raw-mapNum indexing safe (see
    // NUM_NUZLOCKE_ZONE_FLAG_BYTES for a related indexing pitfall). Each
    // entry is (mapGroup << 8) | (u8)mapNum, deduplicated by linear scan on
    // write -- same idiom as AchievementRunData.majorBattleSpecies. Capped
    // at 80 (the top achievement threshold): once full, additional distinct
    // maps just stop being recorded, which is harmless since no entry needs
    // more.
    u16 mapsVisited[80];
    u8  mapsVisitedCount;

    // "Since the last Gym" shopping window, the same temporal shape Fresh
    // Start's ring buffer (AchievementRunData.recentlyObtainedPersonality)
    // uses. shoppedSinceLastGym is set by the shop hook and read/reset by
    // Achievement_CheckGymEconomyMilestones (HandleEndTurn_BattleWon, same
    // call site as category L). consecutiveGymsNoShopping counts unbroken
    // Gym-clears-without-shopping streaks for No Shopping.
    bool8 shoppedSinceLastGym;
    u8  consecutiveGymsNoShopping;

    // Randomizer & New Game+ fields: SaveBlock1 has zero
    // bytes of slack left, so every run-scoped field these entries need
    // lands here instead -- the same detour the fields above took, for the
    // same reason.
    //
    // Two different reset cadences now share this struct. mapsVisited/etc.
    // above are cleared only by a genuine new game (Sav2_ClearSetDefault) and
    // deliberately span every NG+ cycle on the save, matching
    // ACHIEVEMENT_SCOPE_NG_PLUS. trainersDefeatedThisCycle below is "within a
    // single NG+ cycle" instead, so it's explicitly zeroed by
    // Achievement_OnNewGamePlusStarted -- ClearSav1 can't do it for us here,
    // since that only ever touches SaveBlock1.
    u16 trainersDefeatedThisCycle;             // Fresh Faces (NGP-006)
    // gymSpeciesUsedThisCycle/Count, reinventionBroken,
    // majorBossClassesDefeatedThisCycle, and previousCyclePartySpecies/Set
    // below are all unused now -- they backed Complete Reinvention
    // (ACHIEVEMENT_NG_PLUS_COMPLETE_REINVENTION), Boss Gauntlet
    // (ACHIEVEMENT_NG_PLUS_BOSS_GAUNTLET), and No Nostalgia
    // (ACHIEVEMENT_NG_PLUS_NO_NOSTALGIA), all removed -- see
    // src/data/achievements.h's own comment. Left in place rather than
    // removed, to avoid reshuffling every later field's offset.
    u16 gymSpeciesUsedThisCycle[NUM_BADGES * PARTY_SIZE];
    u8  gymSpeciesUsedThisCycleCount;
    bool8 reinventionBroken;
    u8  majorBossClassesDefeatedThisCycle;
    u16 previousCyclePartySpecies[PARTY_SIZE];
    bool8 previousCyclePartySpeciesSet;

    // Streaks, Records & Collection Remainder fields: same "SaveBlock1 has
    // zero slack left" detour the fields above already took -- see those
    // fields above. Most fields below span the whole save the same way
    // mapsVisited (top of this struct) does: cleared only by
    // Sav2_ClearSetDefault, never reset per NG+ cycle. A win streak that
    // happens to straddle an NG+ boundary is exactly what "since the last
    // party wipe" should mean, not an artificial reset at the cycle line.
    // gymLeadersSinceWipe is the one exception (alongside koCountPerSlot/
    // majorKoCountPerSlot further down): Three/Eight Gym Streak are
    // CURRENT_PLAYTHROUGH scoped, and gym counts are a per-cycle notion the
    // same way KOs are -- carrying it over let a couple of Gym wins into a
    // fresh cycle complete Three Gym Streak immediately, so it's zeroed by
    // Achievement_OnNewGamePlusStarted instead.
    u16 currentTrainerWinStreak;         // Hot Streak..Untouchable Streak (REC-001..004)
    u16 bestTrainerWinStreakThisRun;     // high-water mark; mirrored into gAchievementProfile.bestTrainerWinStreakEver on every party wipe
    u8  gymLeadersSinceWipe;             // Three/Eight Gym Streak (REC-005/006) -- per-NG+-cycle reset, see comment above
    u8  leagueWinsSinceWipe;             // Elite Four/Champion wins since the last party wipe, for League Streak (REC-007)
    u16 koCountPerSlot[PARTY_SIZE];      // cumulative opposing KOs credited to whatever's in this party slot, any battle -- Veteran Team (REC-008)
    u16 majorKoCountPerSlot[PARTY_SIZE]; // same, major battles only -- Old Reliable (REC-009)
    u8  presentAtEveryMajorBattleSlots;  // unused -- replaced by legendCandidatePersonalities/legendCandidateCount below. This bitmask tracked occupied party SLOTS rather than individual Pokemon, and slot 0 is never empty while you're able to battle at all, so it trivially always kept bit 0 set -- Legend of the Run fired on essentially every completed run. Left in place rather than reflowing this struct's fields.
    bool8 anyMajorBattleThisRun;         // legendCandidatePersonalities is meaningless until this is set (still used by the fix below)
    u8  comebackWinsThisRun;             // unused -- backed Comeback Count (ACHIEVEMENT_RECORD_COMEBACK_COUNT), removed. Left in place rather than reflowing this struct's fields.
    u8  tmsTaughtThisRun;                // Move Tutor (backfill)

    // Legend of the Run (REC-010), fixed. Tracks actual
    // Pokemon (by personality, survives evolution) rather than party
    // slots -- the set of candidates still present in every major battle
    // so far this run, shrunk by intersection each major battle win. Empty
    // once no single Pokemon has made it into every major battle.
    u32 legendCandidatePersonalities[PARTY_SIZE];
    u8  legendCandidateCount;

    // Recruits/Limited Party/Draft/Rotation/Mono Type/Mono Gen challenge-mode
    // achievements -- same SaveBlock1-has-no-slack detour as every field
    // above. Per-NG+-cycle scope, zeroed by Achievement_OnNewGamePlusStarted
    // alongside trainersDefeatedThisCycle etc.
    u8 recruitsRetiredThisCycle;      // Revolving Door/Full Turnover
    bool8 recruitsRunFailedThisCycle; // unused -- backed Never Understaffed (ACHIEVEMENT_RECRUITS_NEVER_UNDERSTAFFED), removed. Left in place rather than reflowing this struct's fields.
    u8 limitedPartyWinsAtCap;         // No Room to Spare
    u8 draftsCompletedThisCycle;      // The Case is Closed/Full Case Clear
    u8 rotationTrainerWinsThisCycle;  // On a Rotation
    u8 monoTypeObtainedThisCycle;     // Perfect Fit
    u8 monoGenObtainedThisCycle;      // Gotta Catch Some of Them
};

// Offline, code-based trading (see trade_code.h) -- a decoded but not-yet-
// materialised partner Pokemon, held between the Step 3 escrow commit and
// the Step 4 swap, plus the bookkeeping the session state machine needs to
// survive a reset. See enum TradeCodeState (trade_code.h) for what `state`
// holds; TRADE_CODE_STATE_NONE is guaranteed to be 0, so a save from before
// this struct existed (or a freshly zeroed one) reads as "no pending trade"
// with no migration needed.
//
// `incoming` is raw storage for a struct BoxPokemon (pinned to 80 bytes as
// of Saveblock-Shrinking Stage 5; see the STATIC_ASSERT tying the two
// together in pokemon.h), not a `struct BoxPokemon incoming;` member as
// trade_code.h's own Stage 4 planning sketch has it -- pokemon.h isn't
// #included until after struct SaveBlock2 below, so its full definition
// (and therefore its size) isn't visible at this point in the header yet.
// struct SecretBaseParty, a little further down, hits the same ordering
// constraint for struct Pokemon and works around it the same way (raw
// fields instead of an embedded struct). Moved in/out via memcpy from
// src/trade_code.c, which does have pokemon.h visible.
// myConfirmTag/myOfferBits/myOfferSpecies/myOfferBytes/myOfferNickname
// (post-Stage-10 follow-up) are a player's own already-built Step 1 offer
// and Step 3 confirm tag, kept verbatim so the Cable Club attendant's "view
// offer code"/"view confirm code" options can redisplay either one exactly
// as first shown, any number of times, without the original Pokemon still
// existing in the party to re-derive them from (Step 3 already escrowed it
// away). Populated once, at the same commit point expectedConfirmTag is
// (src/trade_code_session.c's TradeCodeSession_DoCommit) -- everything
// needed is already in that function's own EWRAM session state at that
// point, right before it's freed. Left populated (not cleared) once the
// trade actually completes or is given up, same as every other pendingTrade
// field the doc's own "keeping the replay ring and abandonedCount" wording
// already established isn't worth zeroing defensively -- harmless stale
// bytes with `state` back at TRADE_CODE_STATE_NONE, since both view options
// refuse to show anything unless `state == TRADE_CODE_STATE_COMMITTED`.
struct PendingTrade
{
    u8 incoming[80];                               // struct BoxPokemon, by value -- see above
    u32 recentOfferSeals[TRADE_CODE_REPLAY_RING];  // Stage 3's replay ring
    u32 expectedConfirmTag;                        // Stage 3's TradeCode_ConfirmTag, masked to 28 bits
    u32 myConfirmTag;                              // my own confirm tag, shown to my partner (see comment above)
    u16 nonce;
    u16 myOfferBits;                               // exact bit length within myOfferBytes below
    u16 myOfferSpecies;                            // offered mon's species, for the redisplay icon (SPECIES_NONE if never set)
    u8 state;                                      // enum TradeCodeState (trade_code.h)
    u8 partySlot;                                  // the vacated party slot, for Step 4
    u8 abandonedCount;                             // Stage 11's soft fair-exchange deterrent
    u8 myOfferBytes[TRADE_CODE_OFFER_PAYLOAD_BYTES]; // my own built Step 1 offer payload, raw bits
    u8 myOfferNickname[POKEMON_NAME_LENGTH + 1];   // offered mon's nickname, for the redisplay icon
    u8 padding[2];
}; // sizeof == 200: 80 + 32 + 4 + 4 + 2 + 2 + 2 + 1 + 1 + 1 + 56 + 13 + 2,
   // no compiler-inserted padding between members (every u32-then-narrower
   // transition already lands on a natural boundary, same discipline as
   // the original 124-byte layout) -- see this stage's status block for
   // the full accounting.

// Size of the nuzlocke per-zone bitfields in struct SaveBlock2. Indexed by
// GetCurrentRegionMapSectionId() -- the map's MAPSEC, i.e. the name shown on
// the region map -- NOT by raw mapNum like these fields used to be (that
// scheme, and the fields using it, have been removed). Keying off mapNum
// meant every floor of a multi-floor location (e.g.
// MAP_GRANITE_CAVE_1F/B1F/B2F/STEVENS_ROOM are four different mapNums in the
// same map group) got its own "already caught here" bucket, so a Nuzlocke
// run could catch one mon per floor of the same cave instead of one per
// cave. MAPSEC is shared by every floor of a given named area, so keying off
// it collapses them into a single bucket, matching what "one catch per area"
// actually means. Sized off MAPSEC_COUNT (rather than a hardcoded number) so
// it can't quietly fall out of sync the way the mapNum version did as new
// maps were added.
#define NUM_NUZLOCKE_ZONE_FLAG_BYTES ((MAPSEC_COUNT + 7) / 8)
#define NUM_NUZLOCKE_ZONE_FLAGS      (NUM_NUZLOCKE_ZONE_FLAG_BYTES * 8)

struct SaveBlock2
{
    /*0x00*/ u8 playerName[PLAYER_NAME_LENGTH + 1];
    /*0x08*/ u8 playerGender; // MALE, FEMALE
    /*0x09*/ u8 specialSaveWarpFlags;
    /*0x0A*/ u8 playerTrainerId[TRAINER_ID_LENGTH];
    /*0x0E*/ u16 playTimeHours;
    /*0x10*/ u8 playTimeMinutes;
    /*0x11*/ u8 playTimeSeconds;
    /*0x12*/ u8 playTimeVBlanks;
    /*0x13*/ u8 optionsButtonMode;  // OPTIONS_BUTTON_MODE_[NORMAL/LR/L_EQUALS_A]
    /*0x14*/ u16 optionsTextSpeed:3; // OPTIONS_TEXT_SPEED_[SLOW/MID/FAST]
             u16 optionsWindowFrameType:5; // Specifies one of the 20 decorative borders for text boxes
             u16 optionsSound:1; // OPTIONS_SOUND_[MONO/STEREO]
             u16 optionsBattleStyle:1; // OPTIONS_BATTLE_STYLE_[SHIFT/SET]
             u16 optionsBattleSceneOff:1; // whether battle animations are disabled
             u16 regionMapZoom:1; // whether the map is zoomed in
             u16 optionsRouteTracker:1; // show the route completion box on the start menu; 0 (off) on existing saves
             u16 optionsExpShare:1; // Gen 6 style exp share (whole party gains exp); 0 (off) on existing saves, defaulted on for new ones in SetDefaultOptions
             //u16 padding1:2;
    /*0x16*/ u8 limitedPartySetting; // Limited Party challenge: nonzero means on. 0 means off, so old saves read as OFF.
    /*0x17*/ u8 rotationModeSetting; // Rotation Mode: nonzero means on. 0 means off, so old saves read as OFF.
    /*0x18*/ struct Pokedex pokedex;
    /*0x90*/ u8 monoGenSetting; // Mono Gen challenge: the one generation the player may obtain.
    /*0x91*/ u8 monoTypeSetting; // Mono Type challenge: the one type the player may obtain. TYPE_NONE (0) means off, so old saves read as OFF.
    /*0x92*/ u8 playerColors[PLAYER_COLOR_REGION_COUNT]; // see player_customization.h; 0 means vanilla, so old saves render unchanged
    /*0x96*/ u8 keepStorageOnRestart; // this playthrough carried its PC over from the previous one; gates the OT-ID lock in pokemon_storage_system.c
    /*0x97*/ u8 newGamePlus; // New Game+ counter (0-255)
    /*0x98*/ struct Time localTimeOffset;
    /*0xA0*/ struct Time lastBerryTreeUpdate;
    /*0xA8*/ u32 gcnLinkFlags; // Read by Pokémon Colosseum/XD
    /*0xAC*/ u32 encryptionKey;
    // Relocated out of struct BattleFrontier so generic (non-frontier) battle/link/recorded-battle
    // code still has somewhere to read/write these regardless of FREE_BATTLE_FRONTIER.
    u8 disableRecordBattle:1;
    u8 lvlMode:2;
             //u8 padding:5;
    // Debug-menu scratch: which party mons were picked for a debug in-game-partner test battle
    // (src/debug.c writes it, src/battle_setup.c's CB2_EndDebugBattle reads it back). This was
    // squatting on struct BattleFrontier's selectedPartyMons purely for storage convenience, not
    // as a frontier feature, so it gets its own always-present field instead of being gated away.
    u16 selectedPartyMons[MAX_FRONTIER_PARTY_SIZE];
#if FREE_BATTLE_FRONTIER == FALSE
    /*0xB0*/ struct PlayersApprentice playerApprentice;
    /*0xDC*/ struct Apprentice apprentices[APPRENTICE_COUNT];
#endif //FREE_BATTLE_FRONTIER
    /*0x1EC*/ struct BerryCrush berryCrush;
#if FREE_POKEMON_JUMP == FALSE
    /*0x1FC*/ struct PokemonJumpRecords pokeJump;
#endif //FREE_POKEMON_JUMP
    /*0x20C*/ struct BerryPickingResults berryPick;
#if FREE_RECORD_MIXING_HALL_RECORDS == FALSE
    /*0x21C*/ struct RankingHall1P hallRecords1P[HALL_FACILITIES_COUNT][FRONTIER_LVL_MODE_COUNT][HALL_RECORDS_COUNT]; // From record mixing.
    /*0x57C*/ struct RankingHall2P hallRecords2P[FRONTIER_LVL_MODE_COUNT][HALL_RECORDS_COUNT]; // From record mixing.
#endif //FREE_RECORD_MIXING_HALL_RECORDS
#if FREE_CONTESTS == FALSE
    /*0x624*/ u16 contestLinkResults[CONTEST_CATEGORIES_COUNT][CONTESTANT_COUNT];
#endif //FREE_CONTESTS
#if FREE_BATTLE_FRONTIER == FALSE
    /*0x64C*/ struct BattleFrontier frontier;
#endif //FREE_BATTLE_FRONTIER
    struct AchievementRunDataExt achievementRunDataExt; // see that struct's comment
    // Nuzlocke "one catch per area" tracking, MAPSEC-indexed -- see
    // NUM_NUZLOCKE_ZONE_FLAG_BYTES above. Lives here rather than SaveBlock1
    // for the same reason achievementRunDataExt does: SaveBlock1 didn't have
    // the room. Replaces the old raw-mapNum-indexed fields of the same
    // purpose that used to live in SaveBlock1 (removed outright, along with
    // the mapNum indexing bug that motivated this move -- see git history --
    // save compatibility with pre-existing Nuzlocke runs was not a concern).
    u8 nuzlockeZoneCaughtFlags[NUM_NUZLOCKE_ZONE_FLAG_BYTES];
    u8 nuzlockeZoneExtraEncounterFlags[NUM_NUZLOCKE_ZONE_FLAG_BYTES];
    // Offline, code-based trading (trade_code.h) -- see struct PendingTrade's
    // own comment above. A genuine new field, not filler reuse (see this
    // stage's status block for why).
    struct PendingTrade pendingTrade;
}; // sizeof=0xF2C - Pretty sure this size is no longer accurate

extern struct SaveBlock2 *gSaveBlock2Ptr;

extern u8 UpdateSpritePaletteWithTime(u8);

struct SecretBaseParty
{
    u32 personality[PARTY_SIZE];
    enum Move moves[PARTY_SIZE * MAX_MON_MOVES];
    enum Species species[PARTY_SIZE];
    enum Item heldItems[PARTY_SIZE];
    u16 levels[PARTY_SIZE];
    u8 EVs[PARTY_SIZE];
};

struct SecretBase
{
    /*0x1A9C*/ u8 secretBaseId;
    /*0x1A9D*/ bool8 toRegister:4;
    /*0x1A9D*/ u8 gender:1;
    /*0x1A9D*/ u8 battledOwnerToday:1;
    /*0x1A9D*/ u8 registryStatus:2;
    /*0x1A9E*/ u8 trainerName[PLAYER_NAME_LENGTH];
    /*0x1AA5*/ u8 trainerId[TRAINER_ID_LENGTH]; // byte 0 is used for determining trainer class
    /*0x1AA9*/ u8 language;
    /*0x1AAA*/ u16 numSecretBasesReceived;
    /*0x1AAC*/ u8 numTimesEntered;
    /*0x1AAD*/ u8 unused;
    /*0x1AAE*/ u8 decorations[DECOR_MAX_SECRET_BASE];
    /*0x1ABE*/ u8 decorationPositions[DECOR_MAX_SECRET_BASE];
    /*0x1ACE*/ //u8 padding[2];
    /*0x1AD0*/ struct SecretBaseParty party;
};

#include "constants/game_stat.h"
#include "global.fieldmap.h"
#include "global.berry.h"
#include "global.tv.h"
#include "pokemon.h"

struct WarpData
{
    s8 mapGroup;
    s8 mapNum;
    s8 warpId;
    //u8 padding;
    s16 x, y;
};

struct ItemSlot
{
    enum Item itemId;
    u16 quantity;
};

struct Pokeblock
{
    u8 color;
    u8 spicy;
    u8 dry;
    u8 sweet;
    u8 bitter;
    u8 sour;
    u8 feel;
};

struct Roamer
{
    /*0x00*/ u32 ivs;
    /*0x04*/ u32 personality;
    /*0x08*/ enum Species species;
    /*0x0A*/ u16 hp;
    /*0x0C*/ u16 level;
    /*0x0E*/ u8 statusA;
    /*0x0F*/ u8 cool;
    /*0x10*/ u8 beauty;
    /*0x11*/ u8 cute;
    /*0x12*/ u8 smart;
    /*0x13*/ u8 tough;
    /*0x14*/ bool8 active;
    /*0x15*/ u8 statusB; // Stores frostbite
    /*0x16*/ bool8 shiny;
    /*0x17*/ u8 filler[0x5];
};

struct RamScriptData
{
    u8 magic;
    u8 mapGroup;
    u8 mapNum;
    u8 localId;
    u8 script[995];
    //u8 padding;
};

struct RamScript
{
    u32 checksum;
    struct RamScriptData data;
};

// See dewford_trend.c
struct DewfordTrend
{
    u16 trendiness:7;
    u16 maxTrendiness:7;
    u16 gainingTrendiness:1;
    //u16 padding:1;
    u16 rand;
    u16 words[2];
}; /*size = 0x8*/

struct MauvilleManCommon
{
    u8 id;
};

struct MauvilleManBard
{
    /*0x00*/ u8 id;
    /*0x01*/ //u8 padding1;
    /*0x02*/ u16 songLyrics[NUM_BARD_SONG_WORDS];
    /*0x0E*/ u16 newSongLyrics[NUM_BARD_SONG_WORDS];
    /*0x1A*/ u8 playerName[PLAYER_NAME_LENGTH + 1];
    /*0x22*/ u8 filler_2DB6[0x3];
    /*0x25*/ u8 playerTrainerId[TRAINER_ID_LENGTH];
    /*0x29*/ bool8 hasChangedSong;
    /*0x2A*/ u8 language;
    /*0x2B*/ //u8 padding2;
}; /*size = 0x2C*/

struct MauvilleManStoryteller
{
    u8 id;
    bool8 alreadyRecorded;
    u8 filler2[2];
    u8 gameStatIDs[NUM_STORYTELLER_TALES];
    u8 trainerNames[NUM_STORYTELLER_TALES][PLAYER_NAME_LENGTH];
    u8 statValues[NUM_STORYTELLER_TALES][4];
    u8 language[NUM_STORYTELLER_TALES];
};

struct MauvilleManGiddy
{
    /*0x00*/ u8 id;
    /*0x01*/ u8 taleCounter;
    /*0x02*/ u8 questionNum;
    /*0x03*/ //u8 padding1;
    /*0x04*/ u16 randomWords[GIDDY_MAX_TALES];
    /*0x18*/ u8 questionList[GIDDY_MAX_QUESTIONS];
    /*0x20*/ u8 language;
    /*0x21*/ //u8 padding2;
}; /*size = 0x2C*/

struct MauvilleManHipster
{
    u8 id;
    bool8 taughtWord;
    u8 language;
};

struct MauvilleOldManTrader
{
    u8 id;
    u8 decorations[NUM_TRADER_ITEMS];
    u8 playerNames[NUM_TRADER_ITEMS][11];
    u8 alreadyTraded;
    u8 language[NUM_TRADER_ITEMS];
};

typedef union OldMan
{
    struct MauvilleManCommon common;
    struct MauvilleManBard bard;
    struct MauvilleManGiddy giddy;
    struct MauvilleManHipster hipster;
    struct MauvilleOldManTrader trader;
    struct MauvilleManStoryteller storyteller;
    u8 filler[0x40];
} OldMan;

#define LINK_B_RECORDS_COUNT 5

struct LinkBattleRecord
{
    u8 name[PLAYER_NAME_LENGTH + 1];
    u16 trainerId;
    u16 wins;
    u16 losses;
    u16 draws;
};

struct LinkBattleRecords
{
    struct LinkBattleRecord entries[LINK_B_RECORDS_COUNT];
    u8 languages[LINK_B_RECORDS_COUNT];
    //u8 padding;
};

struct RecordMixingGiftData
{
    u8 unk0;
    u8 quantity;
    enum Item itemId;
    u8 filler4[8];
};

struct RecordMixingGift
{
    int checksum;
    struct RecordMixingGiftData data;
};

struct ContestWinner
{
    u32 personality;
    u32 trainerId;
    enum Species species;
    u8 contestCategory;
    u8 monName[VANILLA_POKEMON_NAME_LENGTH + 1];
    u8 trainerName[PLAYER_NAME_LENGTH + 1];
    u8 contestRank:7;
    bool8 isShiny:1;
    //u8 padding;
};

struct Mail
{
    /*0x00*/ u16 words[MAIL_WORDS_COUNT];
    /*0x12*/ u8 playerName[PLAYER_NAME_LENGTH + 1];
    /*0x1A*/ u8 trainerId[TRAINER_ID_LENGTH];
    /*0x1E*/ enum Species species;
    /*0x20*/ enum Item itemId;
};

struct DaycareMail
{
#if FREE_MAIL == FALSE
    struct Mail message;
#endif //FREE_MAIL
    u8 otName[PLAYER_NAME_LENGTH + 1];
    u8 monName[VANILLA_POKEMON_NAME_LENGTH + 1];
    u8 gameLanguage:4;
    u8 monLanguage:4;
};

struct DaycareMon
{
    struct BoxPokemon mon;
    struct DaycareMail mail;
    u32 steps;
};

struct DayCare
{
    struct DaycareMon mons[DAYCARE_MON_COUNT];
    u32 offspringPersonality;
    u32 stepCounter;
};

struct LilycoveLadyQuiz
{
    /*0x000*/ u8 id;
    /*0x001*/ u8 state;
    /*0x002*/ u16 question[QUIZ_QUESTION_LEN];
    /*0x014*/ u16 correctAnswer;
    /*0x016*/ u16 playerAnswer;
    /*0x018*/ u8 playerName[PLAYER_NAME_LENGTH + 1];
    /*0x020*/ u16 playerTrainerId[TRAINER_ID_LENGTH];
    /*0x028*/ u16 prize;
    /*0x02A*/ bool8 waitingForChallenger;
    /*0x02B*/ u8 questionId;
    /*0x02C*/ u8 prevQuestionId;
    /*0x02D*/ u8 language;
};

struct LilycoveLadyFavor
{
    /*0x000*/ u8 id;
    /*0x001*/ u8 state;
    /*0x002*/ bool8 likedItem;
    /*0x003*/ u8 numItemsGiven;
    /*0x004*/ u8 playerName[PLAYER_NAME_LENGTH + 1];
    /*0x00C*/ u8 favorId;
    /*0x00D*/ //u8 padding1;
    /*0x00E*/ enum Item itemId;
    /*0x010*/ u16 bestItem;
    /*0x012*/ u8 language;
    /*0x013*/ //u8 padding2;
};

struct LilycoveLadyContest
{
    /*0x000*/ u8 id;
    /*0x001*/ bool8 givenPokeblock;
    /*0x002*/ u8 numGoodPokeblocksGiven;
    /*0x003*/ u8 numOtherPokeblocksGiven;
    /*0x004*/ u8 playerName[PLAYER_NAME_LENGTH + 1];
    /*0x00C*/ u8 maxSheen;
    /*0x00D*/ u8 category;
    /*0x00E*/ u8 language;
};

typedef union // 3b58
{
    struct LilycoveLadyQuiz quiz;
    struct LilycoveLadyFavor favor;
    struct LilycoveLadyContest contest;
    u8 id;
    u8 filler[0x40];
} LilycoveLady;

struct WaldaPhrase
{
    u16 colors[2]; // Background, foreground.
    u8 text[16];
    u8 iconId;
    u8 patternId;
    bool8 patternUnlocked;
    //u8 padding;
};

struct TrainerNameRecord
{
    u32 trainerId;
    u8 ALIGNED(2) trainerName[PLAYER_NAME_LENGTH + 1];
};

struct TrainerHillSave
{
    /*0x3D64*/ u32 timer;
    /*0x3D68*/ u32 bestTime;
    /*0x3D6C*/ u8 unk_3D6C;
    /*0x3D6D*/ u8 unused;
    /*0x3D6E*/ u16 receivedPrize:1;
               u16 checkedFinalTime:1;
               u16 spokeToOwner:1;
               u16 hasLost:1;
               u16 maybeECardScanDuringChallenge:1;
               u16 field_3D6E_0f:1;
               u16 mode:2; // HILL_MODE_*
               //u16 padding:8;
};

struct TrainerTower
{
    u32 timer;
    u32 bestTime;
    u8 floorsCleared;
    u8 unk9;
    bool8 receivedPrize:1;
    bool8 checkedFinalTime:1;
    bool8 spokeToOwner:1;
    bool8 hasLost:1;
    bool8 unkA_4:1;
    bool8 validated:1;
};

struct WonderNewsMetadata
{
    u8 newsType:2;
    u8 sentRewardCounter:3;
    u8 rewardCounter:3;
    u8 berry;
    //u8 padding[2];
};

struct WonderNews
{
    u16 id;
    u8 sendType; // SEND_TYPE_*
    u8 bgType;
    u8 titleText[WONDER_NEWS_TEXT_LENGTH];
    u8 bodyText[WONDER_NEWS_BODY_TEXT_LINES][WONDER_NEWS_TEXT_LENGTH];
};

struct WonderCard
{
    u16 flagId; // Event flag (sReceivedGiftFlags) + WONDER_CARD_FLAG_OFFSET
    enum Species iconSpecies;
    u32 idNumber;
    u8 type:2; // CARD_TYPE_*
    u8 bgType:4;
    u8 sendType:2; // SEND_TYPE_*
    u8 maxStamps;
    u8 titleText[WONDER_CARD_TEXT_LENGTH];
    u8 subtitleText[WONDER_CARD_TEXT_LENGTH];
    u8 bodyText[WONDER_CARD_BODY_TEXT_LINES][WONDER_CARD_TEXT_LENGTH];
    u8 footerLine1Text[WONDER_CARD_TEXT_LENGTH];
    u8 footerLine2Text[WONDER_CARD_TEXT_LENGTH];
    //u8 padding[2];
};

struct WonderCardMetadata
{
    u16 battlesWon;
    u16 battlesLost;
    u16 numTrades;
    enum Species iconSpecies;
    u16 stampData[2][MAX_STAMP_CARD_STAMPS]; // First element is STAMP_SPECIES, second is STAMP_ID
};

struct MysteryGiftSave
{
    u32 newsCrc;
    struct WonderNews news;
    u32 cardCrc;
    struct WonderCard card;
    u32 cardMetadataCrc;
    struct WonderCardMetadata cardMetadata;
    u16 questionnaireWords[NUM_QUESTIONNAIRE_WORDS];
    struct WonderNewsMetadata newsMetadata;
    u32 trainerIds[2][5]; // Saved ids for 10 trainers, 5 each for battles and trades
}; // 0x36C 0x3598

// For external event data storage. The majority of these may have never been used.
// In Emerald, the only known used fields are the PokeCoupon and BoxRS ones, but hacking the distribution discs allows Emerald to receive events and set the others
struct ExternalEventData
{
    u8 unknownExternalDataFields1[7]; // if actually used, may be broken up into different fields.
    u32 unknownExternalDataFields2:8;
    u32 currentPokeCoupons:24; // PokéCoupons stored by Pokémon Colosseum and XD from Mt. Battle runs. Earned PokéCoupons are also added to totalEarnedPokeCoupons. Colosseum/XD caps this at 9,999,999, but will read up to 16,777,215.
    u32 gotGoldPokeCouponTitleReward:1; // Master Ball from JP Colosseum Bonus Disc; for reaching 30,000 totalEarnedPokeCoupons
    u32 gotSilverPokeCouponTitleReward:1; // Light Ball Pikachu from JP Colosseum Bonus Disc; for reaching 5000 totalEarnedPokeCoupons
    u32 gotBronzePokeCouponTitleReward:1; // PP Max from JP Colosseum Bonus Disc; for reaching 2500 totalEarnedPokeCoupons
    u32 receivedAgetoCelebi:1; // from JP Colosseum Bonus Disc
    u32 unknownExternalDataFields3:4;
    u32 totalEarnedPokeCoupons:24; // Used by the JP Colosseum bonus disc. Determines PokéCoupon rank to distribute rewards. Unread in International games. Colosseum/XD caps this at 9,999,999.
    u8 unknownExternalDataFields4[5]; // if actually used, may be broken up into different fields.
} __attribute__((packed)); /*size = 0x14*/

// For external event flags. The majority of these may have never been used.
// In Emerald, Jirachi cannot normally be received, but hacking the distribution discs allows Emerald to receive Jirachi and set the flag
struct ExternalEventFlags
{
    u8 usedBoxRS:1; // Set by Pokémon Box: Ruby & Sapphire; denotes whether this save has connected to it and triggered the free False Swipe Swablu Egg giveaway.
    u8 boxRSEggsUnlocked:2; // Set by Pokémon Box: Ruby & Sapphire; denotes the number of Eggs unlocked from deposits; 1 for ExtremeSpeed Zigzagoon (at 100 deposited), 2 for Pay Day Skitty (at 500 deposited), 3 for Surf Pichu (at 1499 deposited)
    //u8 padding:5;
    u8 unknownFlag1;
    u8 receivedGCNJirachi; // Both the US Colosseum Bonus Disc and PAL/AUS Pokémon Channel use this field. One cannot receive a WISHMKR Jirachi and CHANNEL Jirachi with the same savefile.
    u8 unknownFlag3;
    u8 unknownFlag4;
    u8 unknownFlag5;
    u8 unknownFlag6;
    u8 unknownFlag7;
    u8 unknownFlag8;
    u8 unknownFlag9;
    u8 unknownFlag10;
    u8 unknownFlag11;
    u8 unknownFlag12;
    u8 unknownFlag13;
    u8 unknownFlag14;
    u8 unknownFlag15;
    u8 unknownFlag16;
    u8 unknownFlag17;
    u8 unknownFlag18;
    u8 unknownFlag19;
    u8 unknownFlag20;

} __attribute__((packed));/*size = 0x15*/

#define NUM_WILD_ENCOUNTER_MAPS 116

struct Bag
{
    struct ItemSlot items[BAG_ITEMS_COUNT];
    struct ItemSlot keyItems[BAG_KEYITEMS_COUNT];
    struct ItemSlot pokeBalls[BAG_POKEBALLS_COUNT];
    struct ItemSlot TMsHMs[BAG_TMHM_COUNT];
    struct ItemSlot berries[BAG_BERRIES_COUNT];
};

// Per-run counters that achievement conditions read from.
// Reset to zero every new game because ClearSav1 zeroes the whole SaveBlock1.
// Named fields get added here as achievements need run-scoped
// tracking that isn't already available elsewhere in the save block.
//
// Category L was the first real user. Species sets
// are tracked by species ID, not by individual (personality/OT), matching
// the granularity struct AchievementBattleData already tracks party
// members at (slot/species, never full identity) -- see src/achievements.c
// for how each field is populated and consumed.
struct AchievementRunData
{
    u16 majorBattleSpecies[32];      // distinct species that have acted in a major battle this run
    u8  majorBattleSpeciesCount;
    u8  monoTypeType;                // NUMBER_OF_MON_TYPES == not yet locked in / discipline broken
    bool8 monoTypeBroken;
    u8  monoTypeGymsCleared;         // Gym clears where the active party happened to be mono-type
    u8  prevMajorBattleSlots;        // bitmask over party slots, for Benchwarmer
    u32 prevGymTypeComposition;      // bitmask over enum Type, for Type Roulette
    bool8 typeRouletteBroken;
    u16 firstGymPartySpecies[PARTY_SIZE]; // baseline snapshot at Gym 1, for Same Six
    bool8 sameSixBaselineSet;
    bool8 sameSixBroken;
    u16 prevGymPartySpecies[PARTY_SIZE];  // snapshot at the previous Gym, for Rebuild
    bool8 prevGymSnapshotSet;
    bool8 rebuildAchieved;
    u16 gym4PartySpecies[PARTY_SIZE];     // snapshot at Gym 4, for Radical Rebuild
    bool8 gym4SnapshotSet;
    bool8 levelCapEverExceeded;      // unused -- backed Capped Out (ACHIEVEMENT_TEAM_CAPPED_OUT) and Perfectly Capped (ACHIEVEMENT_CHALLENGE_PERFECTLY_CAPPED), both removed. Left in place rather than reflowing this struct's fields.
    bool8 bstEverExceeded450;        // for Underdog Run
    bool8 nobodyBenchedBroken;
    u8  gymBattlesWon;               // Gym wins this run -- NOT the same as the badge flags,
                                      // which aren't set until after HandleEndTurn_BattleWon returns
    u16 gymFinalKoSpecies[NUM_BADGES]; // the species that landed the final KO in each Gym battle
    u8  gymFinalKoCount;              // how many of the slots above are filled in, for Ace Rotation
    u32 recentlyObtainedPersonality[8]; // ring buffer of mons obtained since the last Gym, for Fresh Start
    u8  recentlyObtainedCount;

    // The exploration/economy category's own run-scoped fields (maps
    // visited, shop-since-last-Gym tracking) do NOT live here -- SaveBlock1
    // only had 12 bytes of slack left by the time the fields above were
    // added (verified via temporary compiler-error probes in src/save.c),
    // and those fields needed 163 more. They live in struct
    // AchievementRunDataExt (SaveBlock2) instead; see that struct's comment
    // for why.

    // Challenge Runs & Nuzlocke: unlike the exploration/economy fields
    // above, these additions are small enough (12 bytes) to fit the slack
    // left behind here directly -- no SaveBlock2 detour needed. An earlier
    // infra sketch for this category ("a party-wipe flag") didn't
    // survive contact with the actual roster: every entry that sounded like
    // it needed one turned out to be covered by nuzlockeMonsLost, revives
    // used, or a route-skipped flag instead -- a full party wipe is already
    // captured at the moment it happens via Achievement_RecordPartyWipe()
    // (src/overworld.c's RemoveFaintedMonsFromParty), so a flag here
    // observing the same event after the fact would be redundant. See
    // src/achievements.c for the per-field hook-site breakdown.
    u32 starterPersonality;          // the run's starter, by personality (survives evolution) -- for No Freebies; 0 == not yet recorded
    u16 nuzlockePendingRoute;        // unused -- backed Full Encounter (ACHIEVEMENT_NUZLOCKE_FULL_ENCOUNTER), now removed; left in place rather than reflowing this struct's fields
    u8  highestPartySizeThisRun;     // high-water mark for Three-Pokemon Challenge/Solo Journey
    u8  nuzlockeMonsLost;            // for Perfect Nuzlocke/The Graveyard
    u8  nuzlockeRevivesUsed;         // unused -- backed No Second Chances (ACHIEVEMENT_NUZLOCKE_NO_REVIVES), now removed; left in place rather than reflowing this struct's fields
    bool8 starterActedInMajorBattle; // for No Freebies (sticky, same "Broken" idiom used elsewhere)
    bool8 boughtConsumableItem;      // for No Shopping Run (sticky)
    bool8 nuzlockeRouteSkipped;      // unused -- backed Full Encounter (ACHIEVEMENT_NUZLOCKE_FULL_ENCOUNTER), now removed; left in place rather than reflowing this struct's fields
};

struct SaveBlock1
{
    struct Coords16 pos;
    struct WarpData location;
    struct WarpData continueGameWarp;
    struct WarpData dynamicWarp;
    struct WarpData lastHealLocation; // used by white-out and teleport
    struct WarpData escapeWarp; // used by Dig and Escape Rope
    u16 savedMusic;
    u8 weather;
    u8 weatherCycleStage;
    u8 flashLevel;
    //u8 padding1;
    u16 mapLayoutId;
    u16 mapView[0x100];
    u8 playerPartyCount;
    //u8 padding2[3];
    struct Pokemon playerParty[PARTY_SIZE];
    u32 money;
    u16 coins;
    u16 registeredItem; // registered for use with SELECT button
    struct ItemSlot pcItems[PC_ITEMS_COUNT];
    struct Bag bag;
#if FREE_POKEBLOCKS == FALSE
    struct Pokeblock pokeblocks[POKEBLOCKS_COUNT];
#endif //FREE_POKEBLOCKS
#if FREE_EXTRA_SEEN_FLAGS_SAVEBLOCK1 == FALSE
    u8 filler1[0x34]; // Previously Dex Flags, feel free to remove.
#endif //FREE_EXTRA_SEEN_FLAGS_SAVEBLOCK1
    u16 berryBlenderRecords[3];
    u8 nuzlockeModeEnabled;
    u8 autosaveModeEnabled;
    u8 difficulty;
    u8 achievementsBlocked; // set once debug mode is used, this playthrough can never earn achievements
    struct AchievementRunData achievementRunData;
    u16 registeredLongItem; // Registered for long press of SELECT button
    u8 draftModeEnabled; // Draft challenge mode - see include/draft_mode.h. Was unused_9C2[0]; 0 reads as OFF on old saves.
    u8 recruitsModeEnabled; // Recruits challenge mode - see include/recruits_mode.h. Was unused_9C3[0]; 0 reads as OFF on old saves.
    u32 dailySeed;
#if FREE_MATCH_CALL == FALSE
    u16 trainerRematchStepCounter;
    u8 trainerRematches[MAX_REMATCH_ENTRIES];
#endif //FREE_MATCH_CALL
    //u8 padding3[2];
    struct ObjectEvent objectEvents[OBJECT_EVENTS_COUNT];
    struct ObjectEventTemplate objectEventTemplates[OBJECT_EVENT_TEMPLATES_COUNT];
    u8 flags[NUM_FLAG_BYTES];
    u16 vars[VARS_COUNT];
    u32 gameStats[NUM_GAME_STATS];
    struct BerryTree berryTrees[BERRY_TREES_COUNT];
#if FREE_SECRET_BASES == FALSE
    struct SecretBase secretBases[SECRET_BASES_COUNT];
#endif //FREE_SECRET_BASES
#if FREE_DECORATIONS == FALSE
    u8 playerRoomDecorations[DECOR_MAX_PLAYERS_HOUSE];
    u8 playerRoomDecorationPositions[DECOR_MAX_PLAYERS_HOUSE];
    u8 decorationDesks[10];
    u8 decorationChairs[10];
    u8 decorationPlants[10];
    u8 decorationOrnaments[15]; // ORIGINALLY 30
    u8 decorationMats[15]; // ORIGINALLY 30
    u8 decorationPosters[10];
    u8 decorationDolls[20]; // ORIGINALLY 40
    u8 decorationCushions[10];
#endif //FREE_DECORATIONS
    TVShow tvShows[TV_SHOWS_COUNT];
    //u8 padding4[2];
    PokeNews pokeNews[POKE_NEWS_COUNT];
    enum Species outbreakPokemonSpecies;
    u8 outbreakLocationMapNum;
    u8 outbreakLocationMapGroup;
    u16 outbreakPokemonLevel;
    u8 outbreakUnused1;
    u16 outbreakUnused2;
    u16 outbreakPokemonMoves[MAX_MON_MOVES];
    u8 outbreakUnused3;
    u8 outbreakPokemonProbability;
    u16 outbreakDaysLeft;
#if FREE_GABBY_AND_TY == FALSE
    struct GabbyAndTyData gabbyAndTyData;
#endif //FREE_GABBY_AND_TY
#if FREE_EASY_CHAT_PROFILE == FALSE
    u16 easyChatProfile[EASY_CHAT_BATTLE_WORDS_COUNT];
    u16 easyChatBattleStart[EASY_CHAT_BATTLE_WORDS_COUNT];
    u16 easyChatBattleWon[EASY_CHAT_BATTLE_WORDS_COUNT];
    u16 easyChatBattleLost[EASY_CHAT_BATTLE_WORDS_COUNT];
#endif //FREE_EASY_CHAT_PROFILE
#if FREE_MAIL == FALSE
    struct Mail mail[MAIL_COUNT];
#endif //FREE_MAIL
    u8 unlockedTrendySayings[NUM_TRENDY_SAYING_BYTES]; // Bitfield for unlockable Easy Chat words in EC_GROUP_TRENDY_SAYING
    //u8 padding5[3];
#if FREE_OLD_MAN == FALSE
    OldMan oldMan;
#endif //FREE_OLD_MAN
#if FREE_DEWFORD_TRENDS == FALSE
    struct DewfordTrend dewfordTrends[SAVED_TRENDS_COUNT];
#endif //FREE_DEWFORD_TRENDS
#if FREE_CONTESTS == FALSE
    struct ContestWinner contestWinners[NUM_CONTEST_WINNERS]; // see CONTEST_WINNER_*
#endif //FREE_CONTESTS
    struct DayCare daycare;
#if FREE_LINK_BATTLE_RECORDS == FALSE
    struct LinkBattleRecords linkBattleRecords;
#endif //FREE_LINK_BATTLE_RECORDS
    u8 giftRibbons[NUM_GIFT_RIBBONS];
    u8 padding[4];
#if FREE_EXTERNAL_EVENT_DATA == FALSE
    struct ExternalEventData externalEventData;
    struct ExternalEventFlags externalEventFlags;
#endif //FREE_EXTERNAL_EVENT_DATA
    struct Roamer roamer[ROAMER_COUNT];
#if FREE_ENIGMA_BERRY == FALSE
    struct EnigmaBerry enigmaBerry;
#endif //FREE_ENIGMA_BERRY
#if FREE_MYSTERY_GIFT == FALSE
    struct MysteryGiftSave mysteryGift;
#endif //FREE_MYSTERY_GIFT
    u8 dexSeen[NUM_DEX_FLAG_BYTES];
    u8 dexCaught[NUM_DEX_FLAG_BYTES];
#if FREE_TRAINER_HILL == FALSE
    u32 trainerHillTimes[NUM_TRAINER_HILL_MODES];
#endif //FREE_TRAINER_HILL
#if FREE_MYSTERY_EVENT_BUFFERS == FALSE
    struct RamScript ramScript;
#endif //FREE_MYSTERY_EVENT_BUFFERS
#if FREE_RECORD_MIXING_GIFT == FALSE
    struct RecordMixingGift recordMixingGift;
#endif //FREE_RECORD_MIXING_GIFT
#if FREE_LILYCOVE_LADY == FALSE
    LilycoveLady lilycoveLady;
#endif //FREE_LILYCOVE_LADY
    struct TrainerNameRecord trainerNameRecords[4]; // ORIGINALLY 20
#if FREE_UNION_ROOM_CHAT == FALSE
    u8 registeredTexts[UNION_ROOM_KB_ROW_COUNT][21];
#endif //FREE_UNION_ROOM_CHAT
#if FREE_TRAINER_HILL == FALSE
    struct TrainerHillSave trainerHill;
#endif //FREE_TRAINER_HILL
    struct WaldaPhrase waldaPhrase;
#if FREE_TRAINER_TOWER == FALSE && IS_FRLG
    u32 towerChallengeId;
    struct TrainerTower trainerTower[NUM_TOWER_CHALLENGE_TYPES];
#endif //FREE_TRAINER_TOWER
#if IS_FRLG
    u8 rivalName[PLAYER_NAME_LENGTH + 1];
    struct DaycareMon route5DayCareMon;
#endif
    // Actual size: see T_SAVEBLOCK1_SIZE in test/save.c (kept in sync by the
    // "SaveBlock1 is backwards compatible" test) or the in-game debug readout
    // (CheckSaveBlock1Size, src/debug.c). Per-field offsets above are not tracked
};

extern struct SaveBlock1 *gSaveBlock1Ptr;

struct MapPosition
{
    s16 x;
    s16 y;
    s8 elevation;
};

// Helper macros
// The (zone) < NUM_NUZLOCKE_ZONE_FLAGS bounds check is not decoration: zone
// is a MAPSEC id (see NUM_NUZLOCKE_ZONE_FLAG_BYTES's comment), and while it's
// sized off MAPSEC_COUNT, an unchecked SET_ would write into whatever follows
// the array in SaveBlock2 if that were ever to drift. gSaveBlock2Ptr's fields
// are what actually gate catching -- see GetCurrentRegionMapSectionId().
#define GET_NUZLOCKE_ZONE_FLAG(zone) ((zone) < NUM_NUZLOCKE_ZONE_FLAGS && (gSaveBlock2Ptr->nuzlockeZoneCaughtFlags[(zone) / 8] & (1 << ((zone) % 8))))
#define SET_NUZLOCKE_ZONE_FLAG(zone) do { if ((zone) < NUM_NUZLOCKE_ZONE_FLAGS) gSaveBlock2Ptr->nuzlockeZoneCaughtFlags[(zone) / 8] |= (1 << ((zone) % 8)); } while (0)

// For BOOST_NUZLOCKE_SECOND_CHANCE: "this zone's one-time free pass
// has been spent." Only read/written by CB2_EndWildBattle (src/battle_setup.c).
#define GET_NUZLOCKE_ZONE_EXTRA_FLAG(zone) ((zone) < NUM_NUZLOCKE_ZONE_FLAGS && (gSaveBlock2Ptr->nuzlockeZoneExtraEncounterFlags[(zone) / 8] & (1 << ((zone) % 8))))
#define SET_NUZLOCKE_ZONE_EXTRA_FLAG(zone) do { if ((zone) < NUM_NUZLOCKE_ZONE_FLAGS) gSaveBlock2Ptr->nuzlockeZoneExtraEncounterFlags[(zone) / 8] |= (1 << ((zone) % 8)); } while (0)

#if TESTING
extern bool32 gLoadFail;
extern bool32 gCountAllocs;
extern s32 gSpriteAllocs;
#endif // TESTING

#endif // GUARD_GLOBAL_H
