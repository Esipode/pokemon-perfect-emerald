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

// FALSE when the run struct cannot describe the room the player is standing in
// (no run, lobby depth, zeroed room seed, out-of-range room type or modifier).
bool32 InfCave_IsRunConsistent(void);

// InfinityCave_Room's ON_LOAD special. Sets VAR_TEMP_1 to 1 when the run is
// inconsistent; the map's ON_FRAME script then ejects the player to the lobby.
void InfCave_ValidateRoom(void);

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

// Arms B_FLAG_NO_WHITEOUT for the cave (Nuzlocke excepted). Called by the room
// generator, so a reload inside a room re-arms it.
void InfCave_ArmNoWhiteout(void);

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

// Object event templates the generated room owns, the exit crystal included. The
// room map's header describes the crystal alone, so the spawner reads this
// instead of gMapHeader.events->objectEventCount.
u32 InfCave_GetObjectCount(void);

// Trainers the placement pass actually stood in the current room, which can be
// fewer than the roll asked for when no legal tile was left.
u32 InfCave_GetRoomTrainerCount(void);

// TRUE on the generated room map, where the object count above applies.
bool32 InfCave_InGeneratedRoom(void);

// Reassigns the generated objects' scripts on the continue-from-save path, where
// LoadSaveblockObjEventScripts cannot be used: it reads one script per template
// slot out of the map header, which holds only the crystal.
void LoadInfinityCaveObjectEventScripts(void);

// Legality harness for the placement rules. Generates count rooms from baseSeed
// with the trainer count pinned to trainers, and returns how many broke a rule;
// the first failure's seed and fault id land in firstBadSeed and firstFault,
// either of which may be NULL. Restores the live room's seed.
u32 InfCave_DebugValidatePlacement(u32 baseSeed, u32 count, u32 trainers, u32 *firstBadSeed, u32 *firstFault);

// Name of a fault id reported by InfCave_DebugValidatePlacement.
const u8 *InfCave_GetPlacementFaultName(u32 fault);

// src/infinity_cave_trainers.c. One runtime struct Trainer per room trainer slot,
// swapped in for the stub ids TRAINER_INFCAVE_0..7 while gInfCaveBattleActive is
// set (redirect in GetTrainerStructFromId, data.h). Nothing here is saved: a
// reload rebuilds the room's trainers from the run's seeds.
struct Trainer;
extern bool8 gInfCaveBattleActive;

// One rolled opponent identity. Table (sInfCaveIdentities) lives in
// src/data/infinity_cave_trainers.h; InfCave_BuildTrainer copies a row into a
// runtime struct Trainer and returns objectGfxId for the NPC's sprite.
struct InfCaveIdentity
{
    u16 trainerClass;
    u16 trainerPic;
    const u8 *name;
    u16 objectGfxId;
    u8 encounterMusic;
    u8 gender;
};

// Stub trainer id for a slot, or TRAINER_NONE when slot >= INFCAVE_MAX_TRAINERS.
u16 InfCave_GetTrainerId(u32 slot);

// Runtime opponent behind a stub id. Used by the data.h redirect.
const struct Trainer *InfCave_GetTrainer(u16 trainerId);

// Writable slot for the trainer roll. NULL when slot >= INFCAVE_MAX_TRAINERS.
struct Trainer *InfCave_GetTrainerSlot(u32 slot);

// Arms the redirect. Call once the slots hold built trainers.
void InfCave_ArmTrainers(void);

// Disarms the redirect, blanks every slot and clears the stub ids' defeat flags.
void InfCave_ClearTrainers(void);

// Rolls the current room's opponent for one slot and arms the redirect. Returns
// the identity's overworld gfx id. Deterministic per (room seed, slot).
u16 InfCave_BuildTrainer(u32 slot);

// Level every rolled cave mon is generated at: the progression level cap plus a
// depth bonus, plus INFCAVE_MOD_SURGE. Read by CreateNPCTrainerPartyFromTrainer
// while gInfCaveBattleActive is set.
u32 InfCave_GetBattleLevel(void);

// Trainers the current room places: the room type's rolled count plus
// INFCAVE_MOD_SWARM, clamped to INFCAVE_MAX_TRAINERS. Deterministic per room seed.
u32 InfCave_RollTrainerCount(void);

// Depth tier the slot behind trainer rolled on, 0 for anything that is not a
// cave trainer slot.
u32 InfCave_GetTrainerTier(const struct Trainer *trainer);

// Pool rules for the cave (POOL_PRUNE_INFCAVE / POOL_PICK_INFCAVE,
// src/trainer_pools.c). Filler legality is the tier's base-stat-total band plus
// the INFCAVE_MOD_MONOTYPE type when applyMonotype is set; gimmick legality is
// the tier's gimmick kind at the room's battle level.
struct TrainerMon;
bool32 InfCave_MonAllowedAsFiller(const struct Trainer *trainer, const struct TrainerMon *mon, bool32 applyMonotype);
bool32 InfCave_MonMatchesGimmick(const struct Trainer *trainer, const struct TrainerMon *mon);

// Shards one cleared battle in the current room pays: the room type's base
// reward, grown by depth and scaled by INFCAVE_MOD_SURGE / NO_ITEMS / BOUNTY.
// 0 when no run is in progress.
u32 InfCave_GetBattleShards(void);

// Post-battle script special. Adds the battle's shards silently; the start menu's
// route tracker box shows the run total.
void InfCave_PayBattleShards(void);

// Debug: rolls a trainer into slot 0 off the global RNG and arms the redirect.
void InfCave_DebugFillTrainer(void);

#endif // GUARD_INFINITY_CAVE_H
