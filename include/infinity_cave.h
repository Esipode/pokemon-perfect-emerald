#ifndef GUARD_INFINITY_CAVE_H
#define GUARD_INFINITY_CAVE_H

#include "global.h"
#include "constants/infinity_cave.h"
#include "random.h"

// Run state lives in gSaveBlock1Ptr->infinityCaveRun; 0 (also what old saves
// read back) means no run is in progress. Room layouts, trainers and node
// options are never saved: everything is a pure function of the stored seeds,
// so a reload inside a room rebuilds it identically.

bool32 InfCave_IsInRun(void);
u32 InfCave_GetDepth(void);
u32 InfCave_GetShards(void);

// Clears the run struct, seeds runSeed from the global RNG and puts the player
// at depth 0. The first descent picks depth 1's node.
void InfCave_StartRun(void);

// Closes the run out. Rewards are paid by the caller before this runs, since
// this zeroes the shard count.
void InfCave_EndRun(enum InfCaveEndReason reason);

// Script-facing wrapper for INFCAVE_END_QUIT. The lobby calls this on entry, so
// walking back out of the cave always closes the descent.
void InfCave_EndRunQuit(void);

// Advances depth and derives the new room's seed from runSeed and the new
// depth, so a room's contents never depend on which nodes were taken.
void InfCave_AdvanceDepth(void);

// Records the node the player chose for the current depth.
void InfCave_SetRoom(u32 roomType, const u8 *modifiers, const u8 *modifierArgs);

u32 InfCave_GetRoomType(void);

// Modifier slot accessors. slot < INFCAVE_MAX_MODIFIERS.
u32 InfCave_GetModifier(u32 slot);
u32 InfCave_GetModifierArg(u32 slot);

// TRUE if any slot holds modifier.
bool32 InfCave_HasModifier(u32 modifier);

void InfCave_AddShards(u32 amount);
bool32 InfCave_SpendShards(u32 amount);

// Every generator and roller downstream draws from one of these. Seeding is
// explicit so the same seed always reproduces the same room.
rng_value_t InfCave_SeedRoomRng(u32 salt);
u32 InfCave_Rand(rng_value_t *rng);
u32 InfCave_RandRange(rng_value_t *rng, u32 lo, u32 hi);

// src/infinity_cave_map.c. Writes the current room into backupMapData and,
// unless the position came back from a save, places the player.
void InfCave_GenerateRoom(u16 *backupMapData, bool8 setPlayerPosition);

// Reads LAYOUT_INFINITY_CAVE_TILEKEY into the role table. Called by
// InfCave_GenerateRoom; exposed for set-piece and decoration passes.
void InfCave_LoadTileRoles(void);

// Authored block word for a role. role < INFCAVE_ROLE_COUNT.
u16 InfCave_GetRoleBlock(u32 role);

// Legality harness. Generates count masks from baseSeed and returns how many
// broke a rule; the first failure's seed and fault id land in firstBadSeed and
// firstFault, either of which may be NULL. Restores the live room's seed.
u32 InfCave_DebugValidateMask(u32 baseSeed, u32 count, u32 *firstBadSeed, u32 *firstFault);

// Name of a fault id reported by InfCave_DebugValidateMask.
const u8 *InfCave_GetMaskFaultName(u32 fault);

#endif // GUARD_INFINITY_CAVE_H
