#ifndef GUARD_BATTLE_EMPORIUM_H
#define GUARD_BATTLE_EMPORIUM_H

#include "constants/battle_emporium.h"
#include "constants/items.h"    // enum Item
#include "constants/pokemon.h"  // enum Type

// One catalogue row per reward item. Table lives in src/data/battle_emporium.h,
// reached only through the accessors below.
struct EmporiumReward
{
    enum Item item;      // the reward handed to the player on victory
    u8 emporium;         // enum EmporiumId this reward belongs to
    u16 aceKey;          // Tera: enum Type the ace Terastallizes to. Z/Mega: unused (see GetEmporiumAceKey)
    u16 requiredFlag;    // badge flag gating the reward, or EMPORIUM_FLAG_NONE
};

// One rolled challenger identity. Table (sEmporiumIdentities) lives in
// src/data/battle_emporium.h; BuildEmporiumTrainer() (Stage 3) copies a row
// into the runtime struct Trainer and returns objectGfxId for VAR_OBJ_GFX_ID_0.
struct EmporiumIdentity
{
    u16 trainerClass;
    u16 trainerPic;      // enum TrainerPicID
    const u8 *name;
    u16 objectGfxId;     // OBJ_EVENT_GFX_* for the back-room challenger
    u8 encounterMusic;   // TRAINER_ENCOUNTER_MUSIC_*
    u8 gender;           // TRAINER_GENDER_*
};

u32 GetEmporiumRewardCount(u32 emporium);
u32 GetEmporiumRewardStart(u32 emporium);
enum Item GetEmporiumRewardItem(u32 rewardIndex);
u32 GetEmporiumRewardEmporium(u32 rewardIndex);
u16 GetEmporiumRewardRequiredFlag(u32 rewardIndex);

// Reads VAR_EMPORIUM_REWARD and returns the value POOL_PICK_EMPORIUM matches the
// ace slot on: the reward item for Z-Move/Mega, the ace's Tera type for Tera.
u16 GetEmporiumAceKey(void);

// TRUE if mon is the ace the current VAR_EMPORIUM_REWARD selection calls for:
// holds the chosen Z-Crystal / Mega Stone, or Teras to the chosen shard's type.
// POOL_PICK_EMPORIUM (src/trainer_pools.c) uses this to lock the ace slot.
struct TrainerMon;
bool32 EmporiumMonMatchesReward(const struct TrainerMon *mon);

// Runtime challenger. sEmporiumTrainer is swapped in for TRAINER_EMPORIUM while
// gEmporiumBattleActive is set (redirect in GetTrainerStructFromId, data.h).
struct Trainer;
extern bool8 gEmporiumBattleActive;
const struct Trainer *GetEmporiumTrainer(void);
u16 BuildEmporiumTrainer(u32 emporium);
void ClearEmporiumBattle(void);

// Level every Emporium challenger mon is generated at: the progression level cap
// plus a per-building offset. Read by CreateNPCTrainerPartyFromTrainer while
// gEmporiumBattleActive is set.
u32 GetEmporiumBattleLevel(void);

// Script specials for the instructor reward menu (see data/scripts/battle_emporium.inc).
// EmporiumMenu_BuildList / _CommitReward read the building id from VAR_0x8004.
void EmporiumMenu_BuildList(void);
void EmporiumMenu_CommitReward(void);
void EmporiumMenu_BufferConfirm(void);
void EmporiumRollChallenger(void);
void EmporiumShowChallenger(void);

#endif // GUARD_BATTLE_EMPORIUM_H
