#ifndef GUARD_ACHIEVEMENTS_H
#define GUARD_ACHIEVEMENTS_H

#include "global.h"
#include "constants/achievements.h"

#define ACHIEVEMENT_PROFILE_MAGIC   0x50454143  // 'PEAC'
#define ACHIEVEMENT_PROFILE_VERSION 1
#define MAX_ACHIEVEMENTS            512   // reserved ceiling -> 64 bytes of flags
#define MAX_BOOSTS                  32

// Cap for .name in gAchievements[] (src/data/achievements.h), enforced at
// compile time via ACHIEVEMENT_NAME() there -- same pattern as ITEM_NAME_LENGTH.
#define ACHIEVEMENT_NAME_LENGTH     24

// Cap for .name in gAchievementBoosts[] (src/data/achievement_boosts.h),
// enforced at compile time via BOOST_NAME() there.
#define BOOST_NAME_LENGTH           24

// Definition data (design doc §4, Stage 2.1/2.2) -- one const entry per
// enum AchievementId, in src/data/achievements.h. Kept strictly separate
// from struct AchievementProfile below, which is completion *state* only.
struct Achievement
{
    const u8 *name;
    const u8 *description;
    enum AchievementTier tier;
    enum AchievementScope scope;
    u16 points;
    bool8 hidden;
};

// Boost definition data (design doc §11, Stage 7) -- one const entry per
// enum BoostId, in src/data/achievement_boosts.h. Kept strictly separate
// from struct AchievementProfile below, which is completion *state* only
// (mirrors the Achievement/AchievementProfile split above): "a boost's
// definition should describe its behavior; the persistent profile should
// only store the player's state for that boost."
struct AchievementBoost
{
    const u8 *name;
    const u8 *description;
    u8 type;                  // BOOST_TYPE_LEVELED | BOOST_TYPE_BINARY
    u8 maxLevel;               // 1 for binary
    const u16 *costs;          // costs[level] -- cost to go from level to level+1
    const u16 *effects;        // effects[level]; units are per-boost, not rendered generically
};

struct AchievementProfile
{
    u32 magic;
    u16 version;
    u16 checksum;                 // over every byte after this field

    u32 totalPointsEarned;
    u32 pointsInvested;           // available = totalPointsEarned - pointsInvested

    u8  achievementFlags[MAX_ACHIEVEMENTS / 8];
    u8  boostLevels[MAX_BOOSTS];

    bool8 boostsUnlocked;
    bool8 boostsEnabled;          // defaults TRUE

    u16 playthroughsCompleted;
    u16 ngPlusCyclesCompleted;
    u8  highestNgPlusCycle;
    u16 nuzlockesCompleted;
    u16 randomizedRunsCompleted;
    u16 shiniesObtained;
    u16 boostResets;

    u8  reserved[64];             // forward compatibility
};

extern struct AchievementProfile gAchievementProfile;

// A profile read/write failure must never be able to trigger the save-failed
// screen (that's gDamagedSaveSectors/gSaveFileStatus's job, not this one).
bool8 Achievement_ProfileWriteFailed(void);

// Writes the profile to flash only if it's been marked dirty since the last
// flush (see src/achievements.c). Safe to call unconditionally from any of
// the flush points in design doc §1.3.
void Achievement_FlushProfile(void);

// Public API (design doc §24). Nothing outside src/achievements.c writes the
// profile.

// Loads the profile from flash. Call once at boot, before any save is loaded.
void  Achievement_Init(void);
bool8 Achievement_IsCompleted(u16 achievementId);

// gAchievements[] (src/data/achievements.h) is only ever included from
// src/achievements.c -- this is how the rest of the game (e.g. the
// Achievements Menu, Stage 3) reads definition data (name/description/tier/
// points) for a given ID. Returns the ACHIEVEMENT_NONE entry for an
// out-of-range ID rather than NULL, so callers never need a null check.
const struct Achievement *Achievement_GetInfo(u16 achievementId);

u32   Achievement_GetTotalPoints(void);
u32   Achievement_GetAvailablePoints(void);

bool8 Achievement_BoostsUnlocked(void);
bool8 Achievement_BoostsEnabled(void);
void  Achievement_SetBoostsEnabled(bool8 enabled);

u8    AchievementBoost_GetLevel(u16 boostId);

// gAchievementBoosts[] (src/data/achievement_boosts.h) is only ever included
// from src/achievements.c -- mirrors Achievement_GetInfo above. Returns the
// BOOST_NONE entry for an out-of-range ID rather than NULL.
const struct AchievementBoost *AchievementBoost_GetInfo(u16 boostId);

// Runs the order in design doc §4.30 (Stage 2.3): checks run eligibility
// (§1.5) and prior completion, then commits the flag and points together
// before queuing a notification. Returns FALSE without side effects if the
// run is ineligible, the ID is out of range, or it's already completed.
bool8 Achievement_TryComplete(u16 achievementId);

// design doc §5.1/§5.2 (Stage 5): the one-time first-playthrough unlock.
// Call this from GameClear() (src/post_battle_event_funcs.c) in the branch
// that only runs the first time FLAG_SYS_GAME_CLEAR is set for this save --
// that flag's own set-once semantics are what guarantee this runs exactly
// once per playthrough, so this function has no completion guard of its own.
void Achievement_OnFirstPlaythroughComplete(void);

// design doc §7 (Stage 7): validates in order -- boosts unlocked, run not
// blocked (§1.5, mirrors Achievement_TryComplete), current level < maxLevel,
// availablePoints >= costs[level] -- refusing at the first failure. Purchase
// re-runs the same checks (so it can never be called directly around a stale
// CanPurchase result) before committing pointsInvested += cost,
// boostLevels[id]++, and flushing immediately.
bool8 AchievementBoost_CanPurchase(u16 boostId);
bool8 AchievementBoost_Purchase(u16 boostId);

// Declared here to complete the API surface, but implemented once
// ACHIEVEMENT_BOOST_RESET_FEE exists (Stage 11).
bool8 AchievementBoost_Reset(void);

// Debug-only (design doc §21, Stage 1.7). src/debug.c is the only caller.
// These bypass all the validation the real Stage 2/7/11 functions above add
// (achievement completion rules, boost costs/maxLevel, reset fee) by design,
// so the rest of the system can be exercised and tested independently of
// whatever real content/rules exist at a given point in development.
void  Achievement_DebugSetCompleted(u16 achievementId, bool8 completed);
void  Achievement_DebugSetPoints(u32 amount);
void  Achievement_DebugSetBoostsUnlocked(bool8 unlocked);
void  AchievementBoost_DebugSetLevel(u16 boostId, u8 level);
void  AchievementBoost_DebugReset(void);
void  Achievement_DebugMarkPlaythroughComplete(void);

#endif // GUARD_ACHIEVEMENTS_H
