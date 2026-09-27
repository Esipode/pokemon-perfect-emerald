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

// One row per enum InfCaveRoomType. The table (sInfCaveRooms) lives in
// src/data/infinity_cave_nodes.h; the node roll reads weight, minDepth and
// modifierMask, and the node screen reads name, description and icon.
struct InfCaveRoomInfo
{
    const u8 *name;
    const u8 *description;
    u8 icon;          // index into the node screen's room-type icon sheet
    u8 weight;        // relative share of one option roll; 0 is never rolled
    u8 minDepth;      // not offered before this depth
    u16 modifierMask; // modifiers the room type may carry, as INFCAVE_MOD_BIT
};

// Row for a room type, or NULL for anything out of range.
const struct InfCaveRoomInfo *InfCave_GetRoomInfo(u32 roomType);

// One node the player may descend into. Rolled by InfCave_RollNodeOptions and
// never saved: the roll is a pure function of the run seed and the target depth.
struct InfCaveNodeOption
{
    u8 roomType;                           // enum InfCaveRoomType
    u8 modifier[INFCAVE_MAX_MODIFIERS];    // enum InfCaveModifier
    u8 modifierArg[INFCAVE_MAX_MODIFIERS]; // meaning depends on the modifier
};

// Depth the offered nodes lead to: one past the depth the player stands on, so
// the lobby's options are depth 1's.
u32 InfCave_GetNodeDepth(void);

// Fills options with the nodes offered for InfCave_GetNodeDepth and returns how
// many were written, INFCAVE_MIN_OPTIONS..INFCAVE_MAX_OPTIONS, or 1 on a boss
// depth. options must hold INFCAVE_MAX_OPTIONS entries. Deterministic: the same
// run seed and depth always produce the same set, so a reload re-offers it.
u32 InfCave_RollNodeOptions(struct InfCaveNodeOption *options);

// Debug: logs the rolled option set at a spread of depths over the debug print
// handler, then restores the live depth. Empty in NDEBUG builds.
void InfCave_DebugDumpNodeOptions(void);

// TRUE while the current room's exit must refuse to descend: a boss room whose
// boss is still standing. Read by the ladder script with specialvar.
u16 InfCave_IsExitLocked(void);

// Modifier slot accessors. slot < INFCAVE_MAX_MODIFIERS.
u32 InfCave_GetModifier(u32 slot);
u32 InfCave_GetModifierArg(u32 slot);

// TRUE if any slot holds modifier.
bool32 InfCave_HasModifier(u32 modifier);

// One row per enum InfCaveModifier, INFCAVE_MOD_NONE excepted. The table
// (sInfCaveModifiers) lives in src/data/infinity_cave_modifiers.h; the node roll
// reads weight, minDepth and incompatible, the node screen reads name,
// description and icon, and the payout reads shardPercent.
struct InfCaveModifierInfo
{
    const u8 *name;
    const u8 *description;
    u8 icon;         // index into the node screen's modifier icon sheet
    u8 argKind;      // enum InfCaveModArg the slot's argument byte holds
    u8 shardPercent; // added to a cleared battle's payout share
    u8 weight;       // relative share of one modifier roll
    u8 minDepth;     // not offered before this depth
    u16 incompatible; // modifiers it never shares a room with, as INFCAVE_MOD_BIT
};

// Row for a modifier, or NULL for INFCAVE_MOD_NONE and anything out of range.
const struct InfCaveModifierInfo *InfCave_GetModifierInfo(u32 modifier);

// TRUE when two modifiers may share one room. Either row naming the other is
// enough to make the pair illegal, so the table only has to state it once.
bool32 InfCave_ModifiersCompatible(u32 a, u32 b);

// Argument byte for a modifier the node roll picked, drawn from the modifier's
// own option table at the current depth. 0 for a modifier that takes none.
u32 InfCave_RollModifierArg(u32 modifier, rng_value_t *rng);

// Payout share the room's active modifiers add, summed over the slots.
u32 InfCave_GetModifierShardPercent(void);

// Field-side modifier effects: INFCAVE_MOD_DARK's flash level and
// INFCAVE_MOD_WEATHER's field weather. Called by the room generator, so the
// reload path applies them too.
void InfCave_ApplyModifiers(void);

// Empties every modifier slot and puts the field back the way an unmodified room
// leaves it. Called when a run closes, so no dark or weather outlives the cave.
void InfCave_ClearModifiers(void);

// TRUE while INFCAVE_MOD_NO_ITEMS must refuse the Bag: a cave battle in a room
// carrying the modifier. Read by IsAllowedToUseBag.
bool32 InfCave_IsBagLocked(void);

// Arms B_FLAG_NO_WHITEOUT for the cave (Nuzlocke excepted). Called by the room
// generator, so a reload inside a room re-arms it.
void InfCave_ArmNoWhiteout(void);

void InfCave_AddShards(u32 amount);
bool32 InfCave_SpendShards(u32 amount);

// Rest, treasure and shop rooms (src/infinity_cave.c). Consumed state - an
// emptied item ball, a used shrine, a bought one-off service - lives in the run
// struct's roomFlags rather than in the generator, since the generator rebuilds
// the room from its seed on every load.

// Item balls a treasure room stands, 0 in every other room type. Deterministic
// per room seed.
u32 InfCave_RollItemBallCount(void);

// TRUE when the player has already emptied this room's ball in slot, so the
// generator must leave it out.
bool32 InfCave_IsItemBallTaken(u32 slot);

// Item ball script specials. Both read the ball's slot from VAR_0x800A;
// InfCave_SetItemBallItem writes the rolled item into the vars STD_FIND_ITEM
// reads.
void InfCave_SetItemBallItem(void);
void InfCave_MarkItemBallTaken(void);

// Rest room shrine. One heal per room: the script checks InfCave_IsShrineUsed
// with specialvar, heals with HealPlayerParty, then calls InfCave_UseShrine.
u16 InfCave_IsShrineUsed(void);
void InfCave_UseShrine(void);

// Merchant specials. BuildList fills the dynamic multichoice stack and returns
// the row count in VAR_RESULT; Buy consumes the menu's pick from VAR_RESULT and
// replaces it with an enum InfCaveBuyResult.
void InfCaveShop_BuildList(void);
void InfCaveShop_Buy(void);

// Every generator and roller downstream draws from one of these. Seeding is
// explicit so the same seed always reproduces the same room.
rng_value_t InfCave_SeedRoomRng(u32 salt);

// Same, off the run's own seed, for the rolls that must hold across every room
// of a run rather than be rebuilt per room (the boss order).
rng_value_t InfCave_SeedRunRng(u32 salt);
u32 InfCave_Rand(rng_value_t *rng);
u32 InfCave_RandRange(rng_value_t *rng, u32 lo, u32 hi);

// src/infinity_cave_map.c. Writes the current room into backupMapData and,
// unless the position came back from a save, places the player.
void InfCave_GenerateRoom(u16 *backupMapData, bool8 setPlayerPosition);

// Creates the current depth's hue-shift overlay over the map tiles. Called by
// InfCave_GenerateRoom; the previous floor's overlay is already gone, since the
// descent is a warp.
void InfCave_ApplyDepthHue(void);

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

// Object event templates the generated room owns. The room map's header declares
// none, so the spawner reads this instead of
// gMapHeader.events->objectEventCount.
u32 InfCave_GetObjectCount(void);

// Trainers the placement pass actually stood in the current room, which can be
// fewer than the roll asked for when no legal tile was left.
u32 InfCave_GetRoomTrainerCount(void);

// TRUE on the generated room map, where the object count above applies.
bool32 InfCave_InGeneratedRoom(void);

// TRUE when (x, y), in layout coordinates, is the current room's exit pad. The
// step trigger that descends reads this: the pad is a generated metatile, so the
// map header can hold neither an object nor a coord event for it.
bool32 InfCave_IsExitTile(u32 x, u32 y);

// TRUE when (x, y), in layout coordinates, is a rest room's shrine pad. Like the
// exit pad the shrine is a generated metatile, so its trigger cannot be a coord
// event in the room map's header.
bool32 InfCave_IsShrineTile(u32 x, u32 y);

// Trace probe: logs every active object event's local id, graphics id and tile.
// A no-op unless INFCAVE_TRACE is on.
void InfCave_DebugDumpObjects(void);

// Reassigns the generated objects' scripts on the continue-from-save path, where
// LoadSaveblockObjEventScripts cannot be used: it reads one script per template
// slot out of the map header, which declares no objects.
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

// One boss identity. Table (sInfCaveBosses) lives in
// src/data/infinity_cave_trainers.h. partyTrainer is the trainer whose authored
// team the boss borrows, used only when that team is already a full boss party;
// TRAINER_NONE, or a shorter team, rolls a pooled one at the boss tier instead.
struct InfCaveBoss
{
    u16 trainerPic;
    const u8 *name;
    u16 objectGfxId;
    u16 partyTrainer;
    u8 encounterMusic;
    u8 mugshotColor;
    u8 gender;
};

// Which boss identity the current depth fields. Bosses come in a permutation of
// the table fixed by the run's seed and indexed by how many boss rooms the run
// has reached, so none repeats until the table is exhausted and a reload picks
// the same one again.
u32 InfCave_GetBossIndex(void);

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

// Battle-side modifier effects on one rolled opponent: INFCAVE_MOD_DOUBLES's
// battle type and the INFCAVE_MOD_WEATHER / INFCAVE_MOD_TERRAIN starting status.
// Called by the trainer roll once the slot's own fields are written.
void InfCave_ApplyModifiersToTrainer(struct Trainer *trainer);

// Debug: rolls a trainer into slot 0 off the global RNG and arms the redirect.
void InfCave_DebugFillTrainer(void);

#endif // GUARD_INFINITY_CAVE_H
