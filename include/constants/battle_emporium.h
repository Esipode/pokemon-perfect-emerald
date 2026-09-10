#ifndef GUARD_CONSTANTS_BATTLE_EMPORIUM_H
#define GUARD_CONSTANTS_BATTLE_EMPORIUM_H

// Battle Emporiums: three reward facilities that replace the cut Contest Halls.
// The player picks a reward item, fights a runtime-generated trainer whose ace
// uses that item's mechanic, and wins the item.
//   Z-Move Emporium  - Z-Crystals, unlocked by Badge 3, 4-mon enemy team
//   Mega Emporium    - Mega Stones, unlocked by Badge 5, 5-mon enemy team
//   Tera Emporium    - Tera Shards, unlocked by Badge 7, 6-mon enemy team

enum EmporiumId
{
    EMPORIUM_NONE,
    EMPORIUM_ZMOVE,
    EMPORIUM_MEGA,
    EMPORIUM_TERA,
    EMPORIUM_COUNT,
};

// VAR_EMPORIUM_RESULT: how the last back-room battle ended. Set by the battle
// room as the player leaves, read by the lobby ON_FRAME script to run the reward
// payout or the recovery scene, then cleared back to EMPORIUM_RESULT_NONE.
enum EmporiumResult
{
    EMPORIUM_RESULT_NONE,
    EMPORIUM_RESULT_WON,
    EMPORIUM_RESULT_LOST,
};

#define EMPORIUM_PARTY_SIZE_ZMOVE   4
#define EMPORIUM_PARTY_SIZE_MEGA    5
#define EMPORIUM_PARTY_SIZE_TERA    6

// Rows shown before the reward menu starts scrolling (dynmultichoice maxBeforeScroll).
#define EMPORIUM_MENU_PAGE_SIZE     6

// Poke money charged per challenge attempt.
#define EMPORIUM_ENTRY_FEE          2500

// Sentinel for struct EmporiumReward.requiredFlag when a row has no unlock gate.
#define EMPORIUM_FLAG_NONE          0xFFFF

#endif // GUARD_CONSTANTS_BATTLE_EMPORIUM_H
