#ifndef GUARD_BATTLE_ENCOUNTER_H
#define GUARD_BATTLE_ENCOUNTER_H

#include "battle.h"
#include "constants/battle_encounter.h"

// 6 bytes, ROM-resident. A trigger's conditions pointer addresses an array terminated by an
// ENC_OP_COUNT operand, avoiding a separate count. ENC_OP_ALL/ENC_OP_ANY/ENC_OP_NOT (Stage 12)
// reuse this same struct as group nodes rather than a separate tagged union - see EvalNode in
// battle_encounter.c for how the tree is laid out prefix-encoded in one flat array.
struct EncounterCondition
{
    u8  operand;     // enum EncounterOperand
    u8  cmp;         // enum EncounterCmp; unused by a group node
    u16 arg;         // leaf: operand-specific (battler ref, var index, ENC_PACK_STAT_ARG)
                      // ENC_OP_ALL/ENC_OP_ANY: number of immediate child nodes; ENC_OP_NOT: unused
    s16 value;       // right-hand side; unused by a group node
};

struct EncounterTrigger
{
    enum EncounterCheckpoint checkpoint;
    u8 priority;                        // lower runs first; 0 = highest
    u8 flags;                           // ENC_TRIGGER_*
    const struct EncounterCondition *conditions;   // NULL = always eligible (Stage 11)
    const u8 *script;                   // battle script label
};

// Battle-start configuration, authored in a '.encounter' file's 'Properties:' block. ROM-resident
// and applied once per battle, unlike a trigger, which reacts to what happens during one. An
// all-zero properties block means "change nothing", so an encounter that omits Properties: costs
// only these bytes in ROM and no behavior at runtime.
struct EncounterProperties
{
    u64 aiFlags;         // AI_FLAG_* mask for the opponent side; 0 = whatever the battle would use
    u16 level;           // ENC_LEVEL_NONE / ENC_LEVEL_CAP / a literal level for every opponent
    u16 moves[MAX_MON_MOVES];  // the boss's moves; a MOVE_NONE slot is left as built
    u16 ability;         // ENC_ABILITY_NONE, or the ability the boss fights with
    u8 catchRate;        // ENC_CATCH_RATE_NONE, or a catch rate replacing the species' own
    u8 ballPolicy;       // enum EncounterBallPolicy
    u8 damageReduction;  // percent, applied to the boss; 0..ENC_MAX_DAMAGE_REDUCTION
    u8 immunities;       // ENC_IMMUNE_* bits, applied to the boss
    bool8 capTypeEffectiveness;  // clamps the boss's incoming type-effectiveness multiplier to 2x
    bool8 flatToxicDamage;       // disables Toxic's per-turn counter ramp on the boss
    bool8 survive;               // boss's HP can't be taken below 1 by the damage formula or a passive tick
};

struct Encounter
{
    const struct EncounterTrigger *triggers;
    u8 triggerCount;
    struct EncounterProperties properties;
};

// Test encounter scripts (data/battle_scripts_encounters.s).
extern const u8 EncScript_TestBattleStart[];
extern const u8 EncScript_TestTurnEnd[];
extern const u8 EncScript_TestGeneric[];

// Test scripts for the sENCOUNTER_VAR addressing convention (Stage 14). See test/battle/encounter/
// variables.c for what each one is used to prove.
extern const u8 EncScript_TestSetVar[];       // encsetvar var 0 to 1
extern const u8 EncScript_TestAddVar[];       // encaddvar var 0 by 1
extern const u8 EncScript_TestSeedVarHigh[];  // encsetvar var 0 to 5
extern const u8 EncScript_TestSeedVarLow[];   // encsetvar var 0 to 2
extern const u8 EncScript_TestBranch[];       // encjumpifvar on var 0 > 4; records which way into var 1
extern const u8 EncScript_TestCallSub[];      // call/return through a shared sub-script

// Stage 15 example encounters (outline Sec33/Sec34), built through the authoring path only.
extern const u8 EncScript_LegendaryBarrier_PhaseTransition[];
extern const u8 EncScript_LegendaryBarrier_Weakened[];
extern const u8 EncScript_TrainerMega_Reveal[];

// Stage 16 example encounter: three triggers chained across three checkpoints (OnBattleStart,
// OnMoveEnd, OnTurnEnd), each phase gated on the last via the same Phase var - outline Sec1's
// "multiple custom mechanics chained together" case, built through the authoring path only.
extern const u8 EncScript_StormHerald_Intro[];
extern const u8 EncScript_StormHerald_Surge[];
extern const u8 EncScript_StormHerald_Desperation[];

// Legendary Encounter Messages - see src/data/legendary_encounters/

// Articuno ("The Frozen Battlefield")
extern const u8 EncScript_Articuno_Intro[];
extern const u8 EncScript_Articuno_PhaseFrozenField[];
extern const u8 EncScript_Articuno_AbsoluteZero[];
extern const u8 EncScript_Articuno_BarrierShatter[];
extern const u8 EncScript_Articuno_IceMove[];
extern const u8 EncScript_Articuno_FireResponse[];
extern const u8 EncScript_Articuno_ShakeOff[];
extern const u8 EncScript_Articuno_PlummetTick[];
extern const u8 EncScript_Articuno_BarrierRaise[];
extern const u8 EncScript_Articuno_FrozenGround[];
extern const u8 EncScript_Articuno_Weakened[];
extern const u8 EncScript_Articuno_PlummetResolve[];
extern const u8 EncScript_Articuno_TurnReset[];

// Zapdos ("Storm Overload")
extern const u8 EncScript_Zapdos_Intro[];
extern const u8 EncScript_Zapdos_LastStand[];
extern const u8 EncScript_Zapdos_GroundHit[];
extern const u8 EncScript_Zapdos_ElectricHit[];
extern const u8 EncScript_Zapdos_BossElectric[];
extern const u8 EncScript_Zapdos_AnyHit[];
extern const u8 EncScript_Zapdos_SwitchIn[];
extern const u8 EncScript_Zapdos_TurnOpen[];
extern const u8 EncScript_Zapdos_Burnout[];
extern const u8 EncScript_Zapdos_Storm[];
extern const u8 EncScript_Zapdos_Weakened[];
extern const u8 EncScript_Zapdos_OverloadTick[];
extern const u8 EncScript_Zapdos_CrashEnd[];
extern const u8 EncScript_Zapdos_TurnClose[];

// Moltres ("The Everlasting Flame")
extern const u8 EncScript_Moltres_Intro[];
extern const u8 EncScript_Moltres_Rebirth[];
extern const u8 EncScript_Moltres_Rebirth[];
extern const u8 EncScript_Moltres_Weakened[];
extern const u8 EncScript_Moltres_Weakened[];
extern const u8 EncScript_Moltres_FireHit[];
extern const u8 EncScript_Moltres_TookDamage[];
extern const u8 EncScript_Moltres_BossFire[];
extern const u8 EncScript_Moltres_TurnOpen[];
extern const u8 EncScript_Moltres_TurnClose[];

// Mewtwo ("The Perfect Weapon")
extern const u8 EncScript_Mewtwo_Intro[];
extern const u8 EncScript_Mewtwo_PerfectAdaptation[];
extern const u8 EncScript_Mewtwo_Weakened[];
extern const u8 EncScript_Mewtwo_PlayerPhysical[];
extern const u8 EncScript_Mewtwo_PlayerStatus[];
extern const u8 EncScript_Mewtwo_PlayerSpecial[];
extern const u8 EncScript_Mewtwo_BossHit[];
extern const u8 EncScript_Mewtwo_TurnOpen[];
extern const u8 EncScript_Mewtwo_TurnClose[];
extern const u8 EncScript_Mewtwo_OverloadStart[];
extern const u8 EncScript_Mewtwo_OverloadEnd[];
extern const u8 EncScript_Mewtwo_FirstAdapt[];
extern const u8 EncScript_Mewtwo_Adapt[];

// Mew ("The Genetic Wonder")
extern const u8 EncScript_Mew_Intro[];
extern const u8 EncScript_Mew_Playtime[];
extern const u8 EncScript_Mew_Challenge[];
extern const u8 EncScript_Mew_Weakened[];
extern const u8 EncScript_Mew_PlayerPhysical[];
extern const u8 EncScript_Mew_PlayerStatus[];
extern const u8 EncScript_Mew_PlayerSpecial[];
extern const u8 EncScript_Mew_SwitchIn[];
extern const u8 EncScript_Mew_TurnOpen[];
extern const u8 EncScript_Mew_WaitStep[];
extern const u8 EncScript_Mew_Mischief[];
extern const u8 EncScript_Mew_WaitResolve[];
extern const u8 EncScript_Mew_TurnClose[];

// Raikou ("The Hunting Thunder")
extern const u8 EncScript_Raikou_Intro[];
extern const u8 EncScript_Raikou_Hunt[];
extern const u8 EncScript_Raikou_Hunt[];
extern const u8 EncScript_Raikou_Incarnate[];
extern const u8 EncScript_Raikou_Incarnate[];
extern const u8 EncScript_Raikou_Weakened[];
extern const u8 EncScript_Raikou_Weakened[];
extern const u8 EncScript_Raikou_PlayerGround[];
extern const u8 EncScript_Raikou_PlayerAttack[];
extern const u8 EncScript_Raikou_PlayerStatus[];
extern const u8 EncScript_Raikou_BossMove[];
extern const u8 EncScript_Raikou_Faint[];
extern const u8 EncScript_Raikou_SwitchIn[];
extern const u8 EncScript_Raikou_TurnOpen[];
extern const u8 EncScript_Raikou_Discharge[];
extern const u8 EncScript_Raikou_TurnClose[];

// Entei ("The Walking Volcano")
extern const u8 EncScript_Entei_Intro[];
extern const u8 EncScript_Entei_Cycle[];
extern const u8 EncScript_Entei_Cycle[];
extern const u8 EncScript_Entei_Cataclysm[];
extern const u8 EncScript_Entei_Cataclysm[];
extern const u8 EncScript_Entei_Weakened[];
extern const u8 EncScript_Entei_Weakened[];
extern const u8 EncScript_Entei_PlayerWater[];
extern const u8 EncScript_Entei_PlayerAttack[];
extern const u8 EncScript_Entei_BossFire[];
extern const u8 EncScript_Entei_Faint[];
extern const u8 EncScript_Entei_SwitchIn[];
extern const u8 EncScript_Entei_TurnOpen[];
extern const u8 EncScript_Entei_Erupt[];
extern const u8 EncScript_Entei_EruptSafe[];
extern const u8 EncScript_Entei_Vent[];
extern const u8 EncScript_Entei_TurnClose[];

// Suicune ("The Purifier")
extern const u8 EncScript_Suicune_Intro[];
extern const u8 EncScript_Suicune_Cleansing[];
extern const u8 EncScript_Suicune_Cleansing[];
extern const u8 EncScript_Suicune_SacredBeast[];
extern const u8 EncScript_Suicune_SacredBeast[];
extern const u8 EncScript_Suicune_Weakened[];
extern const u8 EncScript_Suicune_Weakened[];
extern const u8 EncScript_Suicune_TurnOpen[];
extern const u8 EncScript_Suicune_Rite[];
extern const u8 EncScript_Suicune_SacredResolve[];
extern const u8 EncScript_Suicune_SacredDraw[];
extern const u8 EncScript_Suicune_ShatterType[];
extern const u8 EncScript_Suicune_ShatterCrit[];
extern const u8 EncScript_Suicune_StillWater[];
extern const u8 EncScript_Suicune_TurnClose[];

// Celebi ("The Guardian of Time")
extern const u8 EncScript_Celebi_Intro[];
extern const u8 EncScript_Celebi_FutureSight[];
extern const u8 EncScript_Celebi_Collapse[];
extern const u8 EncScript_Celebi_TimeCollapse[];
extern const u8 EncScript_Celebi_Weakened[];
extern const u8 EncScript_Celebi_TurnOpen[];
extern const u8 EncScript_Celebi_Rewind[];
extern const u8 EncScript_Celebi_OpenWindow[];
extern const u8 EncScript_Celebi_Foresee[];
extern const u8 EncScript_Celebi_Anchor[];
extern const u8 EncScript_Celebi_PredHeld[];
extern const u8 EncScript_Celebi_PredBroken[];
extern const u8 EncScript_Celebi_TurnClose[];

// Lugia ("The Storm Beneath the Sea")
extern const u8 EncScript_Lugia_Intro[];
extern const u8 EncScript_Lugia_SeaErupts[];
extern const u8 EncScript_Lugia_OceansWrath[];
extern const u8 EncScript_Lugia_Weakened[];
extern const u8 EncScript_Lugia_TurnOpen[];
extern const u8 EncScript_Lugia_DeepSea[];
extern const u8 EncScript_Lugia_Strike[];
extern const u8 EncScript_Lugia_StormAeroblast[];
extern const u8 EncScript_Lugia_StormSurge[];
extern const u8 EncScript_Lugia_Vent[];
extern const u8 EncScript_Lugia_Undertow[];
extern const u8 EncScript_Lugia_TurnClose[];

// Ho-Oh ("The Rainbow Above the Ashes")
extern const u8 EncScript_HoOh_Intro[];
extern const u8 EncScript_HoOh_TheRainbow[];
extern const u8 EncScript_HoOh_RebirthGate[];
extern const u8 EncScript_HoOh_Weakened[];
extern const u8 EncScript_HoOh_TurnOpen[];
extern const u8 EncScript_HoOh_TurnClose[];
extern const u8 EncScript_HoOh_FlameGain[];
extern const u8 EncScript_HoOh_Fallen[];
extern const u8 EncScript_HoOh_Flight[];
extern const u8 EncScript_HoOh_JudgeImpure[];
extern const u8 EncScript_HoOh_JudgeDefiant[];
extern const u8 EncScript_HoOh_JudgeMerciful[];
extern const u8 EncScript_HoOh_JudgeWorthy[];

// Deoxys ("The Alien Organism")
extern const u8 EncScript_Deoxys_Intro[];
extern const u8 EncScript_Deoxys_TurnOpen[];
extern const u8 EncScript_Deoxys_TurnClose[];
extern const u8 EncScript_Deoxys_RebuildBegin[];
extern const u8 EncScript_Deoxys_RebuildResolve[];
extern const u8 EncScript_Deoxys_Instability[];
extern const u8 EncScript_Deoxys_RapidMutation[];
extern const u8 EncScript_Deoxys_PerfectAdaptation[];
extern const u8 EncScript_Deoxys_HybridOne[];
extern const u8 EncScript_Deoxys_HybridTwo[];
extern const u8 EncScript_Deoxys_Collapse[];
extern const u8 EncScript_Deoxys_HitBug[];
extern const u8 EncScript_Deoxys_HitGhost[];
extern const u8 EncScript_Deoxys_HitDark[];
extern const u8 EncScript_Deoxys_HitOtherPhysical[];
extern const u8 EncScript_Deoxys_HitOtherSpecial[];

// Jirachi ("The Wish Pokemon")
extern const u8 EncScript_Jirachi_Intro[];
extern const u8 EncScript_Jirachi_TagsBlaze[];
extern const u8 EncScript_Jirachi_TagsBlaze[];
extern const u8 EncScript_Jirachi_WishGranted[];
extern const u8 EncScript_Jirachi_WishGranted[];
extern const u8 EncScript_Jirachi_TurnOpen[];
extern const u8 EncScript_Jirachi_Miracle[];
extern const u8 EncScript_Jirachi_Wish[];
extern const u8 EncScript_Jirachi_HeartForce[];
extern const u8 EncScript_Jirachi_HeartWill[];
extern const u8 EncScript_Jirachi_TurnClose[];

// Rayquaza ("The Sky Guardian")
extern const u8 EncScript_Rayquaza_Intro[];
extern const u8 EncScript_Rayquaza_DeltaAscension[];
extern const u8 EncScript_Rayquaza_DeltaAscension[];
extern const u8 EncScript_Rayquaza_AtmosphereBreaks[];
extern const u8 EncScript_Rayquaza_AtmosphereBreaks[];
extern const u8 EncScript_Rayquaza_Weakened[];
extern const u8 EncScript_Rayquaza_Weakened[];
extern const u8 EncScript_Rayquaza_TurnOpen[];
extern const u8 EncScript_Rayquaza_SkyfallLand[];
extern const u8 EncScript_Rayquaza_SkyfallBegin[];
extern const u8 EncScript_Rayquaza_Climb[];
extern const u8 EncScript_Rayquaza_SkyGuard[];
extern const u8 EncScript_Rayquaza_WeatherChaos[];
extern const u8 EncScript_Rayquaza_VentPrimal[];
extern const u8 EncScript_Rayquaza_Vent[];
extern const u8 EncScript_Rayquaza_ShootDown[];
extern const u8 EncScript_Rayquaza_TurnClose[];

// Kyogre ("The Endless Ocean")
extern const u8 EncScript_Kyogre_Intro[];
extern const u8 EncScript_Kyogre_PrimalReversion[];
extern const u8 EncScript_Kyogre_PrimalReversion[];
extern const u8 EncScript_Kyogre_GreatDeluge[];
extern const u8 EncScript_Kyogre_GreatDeluge[];
extern const u8 EncScript_Kyogre_Weakened[];
extern const u8 EncScript_Kyogre_Weakened[];
extern const u8 EncScript_Kyogre_WillNotBeStilled[];
extern const u8 EncScript_Kyogre_TurnOpen[];
extern const u8 EncScript_Kyogre_DelugeTick[];
extern const u8 EncScript_Kyogre_DelugeBegin[];
extern const u8 EncScript_Kyogre_Ebb[];
extern const u8 EncScript_Kyogre_SurgeFire[];
extern const u8 EncScript_Kyogre_SurgeWater[];
extern const u8 EncScript_Kyogre_UndertowBail[];
extern const u8 EncScript_Kyogre_DelugeResolve[];
extern const u8 EncScript_Kyogre_TurnClose[];

// Groudon ("The Living Continent")
extern const u8 EncScript_Groudon_Intro[];
extern const u8 EncScript_Groudon_PrimalReversion[];
extern const u8 EncScript_Groudon_ContinentalCollapse[];
extern const u8 EncScript_Groudon_Weakened[];
extern const u8 EncScript_Groudon_WillNotBeShaken[];
extern const u8 EncScript_Groudon_LandWillNotBeWatered[];
extern const u8 EncScript_Groudon_TurnOpen[];
extern const u8 EncScript_Groudon_Erosion[];
extern const u8 EncScript_Groudon_FeedFlame[];
extern const u8 EncScript_Groudon_FeedBoss[];
extern const u8 EncScript_Groudon_QuakeResolve[];
extern const u8 EncScript_Groudon_CollapseTick[];
extern const u8 EncScript_Groudon_TurnClose[];

// Regice ("The Frozen Clock")
extern const u8 EncScript_Regice_Intro[];
extern const u8 EncScript_Regice_DeepFreeze[];
extern const u8 EncScript_Regice_DeepFreeze[];
extern const u8 EncScript_Regice_FrozenTomb[];
extern const u8 EncScript_Regice_FrozenTomb[];
extern const u8 EncScript_Regice_Weakened[];
extern const u8 EncScript_Regice_Weakened[];
extern const u8 EncScript_Regice_TurnOpen[];
extern const u8 EncScript_Regice_Refreeze[];
extern const u8 EncScript_Regice_AbsoluteZero[];
extern const u8 EncScript_Regice_FrozenAction[];
extern const u8 EncScript_Regice_ThawFire[];
extern const u8 EncScript_Regice_ThawStrike[];
extern const u8 EncScript_Regice_ColdLetsGo[];
extern const u8 EncScript_Regice_TurnClose[];
extern const u8 EncScript_Regice_TombSeal[];
extern const u8 EncScript_Regice_TombRelease[];

extern const u8 EncScript_Regirock_Intro[];
extern const u8 EncScript_Regirock_MountainsWrath[];
extern const u8 EncScript_Regirock_MountainsWrath[];
extern const u8 EncScript_Regirock_Collapse[];
extern const u8 EncScript_Regirock_Collapse[];
extern const u8 EncScript_Regirock_Weakened[];
extern const u8 EncScript_Regirock_Weakened[];
extern const u8 EncScript_Regirock_WillNotSleep[];
extern const u8 EncScript_Regirock_TurnOpen[];
extern const u8 EncScript_Regirock_Break[];
extern const u8 EncScript_Regirock_Build[];
extern const u8 EncScript_Regirock_BossBuild[];
extern const u8 EncScript_Regirock_RebuildResolve[];
extern const u8 EncScript_Regirock_RockfallResolve[];
extern const u8 EncScript_Regirock_CollapseTick[];
extern const u8 EncScript_Regirock_TurnClose[];

extern const u8 EncScript_Registeel_Intro[];
extern const u8 EncScript_Registeel_Optimization[];
extern const u8 EncScript_Registeel_Optimization[];
extern const u8 EncScript_Registeel_PerfectConfig[];
extern const u8 EncScript_Registeel_PerfectConfig[];
extern const u8 EncScript_Registeel_Weakened[];
extern const u8 EncScript_Registeel_Weakened[];
extern const u8 EncScript_Registeel_TurnOpen[];
extern const u8 EncScript_Registeel_Analyse[];
extern const u8 EncScript_Registeel_SystemsOnline[];
extern const u8 EncScript_Registeel_CriticalOverheat[];
extern const u8 EncScript_Registeel_Decay[];
extern const u8 EncScript_Registeel_ForceReconfig[];
extern const u8 EncScript_Registeel_StatusCleared[];
extern const u8 EncScript_Registeel_TurnClose[];

// Stage 15 command tests (test/battle/encounter/commands.c).
extern const u8 EncScript_TestChangeHpDamage[];
extern const u8 EncScript_TestChangeHpHeal[];
extern const u8 EncScript_TestChangeHpAllFoes[];
extern const u8 EncScript_TestChangeStat[];
extern const u8 EncScript_TestChangeStatAllFoes[];
extern const u8 EncScript_TestMegaEvolve[];

// Engine-owned shim: all encounter scripts end with `return`; checkpoints dispatched
// from a non-script engine callback (e.g. BATTLE_START) call the encounter script from
// this shim so the callback stack still unwinds via `end2`.
extern const u8 BattleScript_EncounterCheckpointEnd2[];

const struct Encounter *GetEncounter(enum EncounterId id);

// Author-defined variables, addressed by battle scripts via sENCOUNTER_VAR (constants/
// battle_encounter.h) and by conditions via ENC_OP_VAR. Fixed EWRAM array outside gBattleStruct -
// see the struct EncounterRuntime comment (battle.h) for why.
extern u8 gEncounterVars[MAX_ENCOUNTER_VARS];

// Zeroes gEncounterVars. Called once per battle (AllocateBattleResources, battle_util2.c) so a
// var an earlier battle left set can't leak into an unrelated one.
void ResetEncounterVars(void);

// Set by the overworld script that starts the battle; consumed once at battle start.
void SetPendingBattleEncounter(enum EncounterId id);
enum EncounterId TakePendingBattleEncounter(void);

// --- Encounter properties (struct EncounterProperties) -------------------------------------------

// Rebuilds every opponent Pokemon at the encounter's Level: property. Called from
// CB2_InitBattleInternal (battle_main.c) after the opponent parties exist but before any
// gBattleMons are built from them - a level change has to happen while the party is still the only
// copy of the data, since stats, HP and the battler's own struct are all derived from it here.
// No-op when the encounter has no Level: property.
void ApplyEncounterLevelOverride(void);

// Replaces the boss's moves with the encounter's Moves: property. Called from CB2_InitBattleInternal
// (battle_main.c) right after ApplyEncounterLevelOverride, in the same pre-build window - the level
// override rebuilds stats but not moves, so at a high level cap the boss would otherwise roll
// whatever its learnset hands it.
//
// The boss and nobody else, unlike the level override: a move list is inherently per-mon, so
// applying one list to every opponent would be meaningless. A MOVE_NONE slot is left alone, so a
// list shorter than MAX_MON_MOVES overrides only the slots it names.
void ApplyEncounterMoveOverride(void);

// Gives the boss the encounter's Ability: property. Called from CB2_InitBattleInternal
// (battle_main.c) alongside the level and move overrides, in the same pre-build window: when the
// ability is one of the species' own slots this only has to move MON_DATA_ABILITY_NUM, and the
// battler, the AI and the party UI all derive the rest for free.
//
// An ability the species doesn't have can't be stored on a party mon at all, so it is left to
// ApplyEncounterBattlerAbilityOverride instead. The boss and nobody else, like Moves:.
void ApplyEncounterAbilityOverride(void);

// The off-list half of the Ability: property: writes the ability straight onto a battler once
// gBattleMons exist. Called for every battler as it is built (DoBattleIntro) and as it switches in
// (SwitchInClearSetData), and applies only to the mon ApplyEncounterAbilityOverride targeted, so a
// trainer encounter's other Pokemon keep their own abilities. No-op when the party's ability slot
// already carries the property, which is the common case.
//
// Both call sites sit before switch-in abilities activate, so an Intimidate or a Drought granted
// this way still fires on entry.
void ApplyEncounterBattlerAbilityOverride(enum BattlerId battler);

// The encounter's AiFlags: property, or 0 when it has none. A wild battle gets no AI scoring at all
// by default (GetWildAiFlags is gated behind WE_SMART_WILD_AI_FLAG), so this is what gives a wild
// boss a real AI - see BattleAI_SetupFlags/IsSmartBattle (battle_ai_main.c) and
// OpponentHandleChooseMove (battle_controller_opponent.c), the three places that consult it.
u64 GetEncounterAiFlags(void);

// Seeds the per-battler modifiers (damage reduction, immunities) from the encounter's properties.
// Called once from TryRunEncounterCheckpoint's first ENC_ON_BATTLE_START pass, which is the
// earliest point every battler exists and can be resolved through ENC_BOSS.
void ApplyEncounterBattlerProperties(void);

// Whether the player may throw a Poke Ball right now (Cmd_handleballthrow, battle_script_commands.c).
// Only ever blocks - an encounter can't make a battle catchable that wouldn't be otherwise.
bool32 IsEncounterBlockingBalls(void);

// The catch rate to use instead of the species' own, or ENC_CATCH_RATE_NONE for no override
// (ComputeCaptureOdds, battle_script_commands.c).
u32 GetEncounterCatchRate(void);

// TRUE if battler currently has every ENC_IMMUNE_* bit in immunity. Callers pass one bit; the
// engine hooks that consult this are the OHKO accuracy check (battle_util.c), fixed-damage
// calculation (battle_util.c), Pain Split (battle_script_commands.c), Destiny Bond
// (battle_move_resolution.c) and Perish Song (battle_end_turn.c).
bool32 DoesEncounterGrantImmunity(enum BattlerId battler, u32 immunity);

// Setters behind encsetdamagereduction / encsetimmunity. Both replace the battler's current value
// outright (0 clears it) and assert on an out-of-range argument rather than silently truncating.
void SetEncounterDamageReduction(enum BattlerId battler, u32 percent);
void SetEncounterImmunities(enum BattlerId battler, u32 immunities);

// Setter behind encsetcaptypeeffectiveness; replaces the battler's current value outright. TRUE
// clamps every type-effectiveness multiplier the battler takes (CalcTypeEffectivenessMultiplier,
// battle_util.c) to 2x, so a double weakness stacked onto a double weakness can't spike to 4x.
// Unlike ENC_IMMUNE_*, this only softens a matchup - it never blocks a move outright.
void SetEncounterCapTypeEffectiveness(enum BattlerId battler, bool32 cap);

// TRUE if battler's incoming type effectiveness should be clamped to 2x (see
// SetEncounterCapTypeEffectiveness above). FALSE with no encounter active.
bool32 DoesEncounterCapTypeEffectiveness(enum BattlerId battler);

// Setter behind encsetflattoxicdamage; replaces the battler's current value outright. TRUE stops
// Toxic's counter (HandleEndTurnPoison, battle_end_turn.c) from scaling its damage up each turn -
// the battler still takes the same 1/16 max HP every turn regular Poison would, instead of that
// amount growing every turn up to a full max-HP hit by turn 16. An encounter can run for far more
// turns than a normal battle, so the ramp that's balanced for a ~16-turn fight elsewhere isn't here.
void SetEncounterFlatToxicDamage(enum BattlerId battler, bool32 flat);

// TRUE if battler's Toxic damage should stay flat instead of ramping (see
// SetEncounterFlatToxicDamage above). FALSE with no encounter active.
bool32 DoesEncounterFlattenToxicDamage(enum BattlerId battler);

// Setter behind encsetsurvive; replaces the battler's current value outright. While TRUE the
// battler's HP cannot be taken below 1 by anything that passes through ApplyEncounterDamageReduction
// - move damage and every passive HP tick (weather, Toxic, recoil, confusion self-hits). Fixed-damage
// moves, Perish Song and Destiny Bond bypass that path and are not covered; pair with Immunities:.
// Intended for scripted last-stand / rebirth sequences and for guaranteeing the catch window opens.
void SetEncounterSurvive(enum BattlerId battler, bool32 survive);

// TRUE if battler's HP is currently guarded against dropping below 1 (see SetEncounterSurvive
// above). FALSE with no encounter active.
bool32 DoesEncounterSurvive(enum BattlerId battler);

#if TESTING
// Overrides GetEncounter's id-indexed lookup so tests can supply their own trigger
// tables without touching real encounter data. NULL restores normal lookup.
void TestSetEncounter(const struct Encounter *encounter);
#endif

// priority 0 = highest, runs first; priority 255 = lowest, runs last.
// Selects the highest-priority eligible trigger for checkpoint and returns its script,
// or NULL if none is eligible. Does not run the script.
//
// Ordering guarantees (call sites loop on this until it returns NULL):
// 1. At a checkpoint, the eligible trigger with the lowest priority runs first.
// 2. Ties break by trigger table order, lowest index first.
// 3. After a script completes, all triggers are re-evaluated against the new state.
// 4. A trigger that lost on priority stays eligible and can run in a later pass.
// 5. ENC_TRIGGER_ONCE triggers are marked fired on selection, so they cannot run twice
//    even within one checkpoint.
// 6. At most MAX_ENCOUNTER_SCRIPTS_PER_CHECKPOINT scripts run per checkpoint.
// 7. runtime->prevHp is captured before ENC_ON_BATTLE_START's first pass and re-captured once a
//    checkpoint has no more eligible triggers - the "previous" state ENC_TRIGGER_ON_ENTER compares
//    against is always "as of the last checkpoint", never mid-checkpoint.
//
// A returned script reaches the interpreter via BattleScriptCall, which shares gBattleResources->
// battleScriptsStack (8 entries, include/battle.h) with the rest of the engine; `call`/`return`
// inside an encounter script (Cmd_call/Cmd_return - meaningfully usable now that scripts can
// address their own state, Stage 14) spend from the same budget. Static reading of the checkpoint
// call sites (battle_main.c, battle_move_resolution.c, battle_util.c, battle_switch_in.c) suggests
// the engine's own script chains have already unwound by the time a checkpoint dispatches, leaving
// ample headroom - but this hasn't been confirmed by measuring the stack's actual size in a running
// build. BattleScriptPush already asserts loudly on overflow either way (src/battle_util.c), so a
// wrong assumption here fails safe.
const u8 *TryRunEncounterCheckpoint(enum EncounterCheckpoint checkpoint);

// Populates the current checkpoint's event context. Call sites only need to pass the fields their
// checkpoint actually has data for (see sCheckpointEventFields in battle_encounter.c) - unset
// fields are left zeroed and reading them via GetEncounterEventField asserts.
//
// Call this AFTER TryRunEncounterCheckpoint, not before: TryRunEncounterCheckpoint clears the
// event context on checkpoint entry (the first pass of a checkpoint only), so calling this first
// would have that clear wipe it straight back out on pass 1.
void SetEncounterEvent(u8 battler, u8 target, u16 move, enum EncounterEventCause cause, s16 oldValue, s16 newValue);

// Reads one field of the current checkpoint's event context. Asserts (recovery: return FALSE) if
// field is not valid for the checkpoint currently dispatching, e.g. reading ENC_EVENT_MOVE at
// ENC_ON_BATTLE_START - that would otherwise silently return a stale value from an earlier
// checkpoint. field is one of the ENC_EVENT_* constants in constants/battle_encounter.h.
bool32 GetEncounterEventField(u32 field, s32 *out);

// Resolves a battler reference (enum EncounterBattlerRef) to a concrete battler id. Shared with
// Stage 15's command targeting so both use one vocabulary. Returns FALSE for a _RIGHT ref in a
// singles battle (recovery: *battlerOut left untouched) rather than resolving to an inactive
// battler's stale gBattleMons entry - never read that.
bool32 ResolveEncounterBattlerRef(u32 ref, u8 *battlerOut);

// Resolves an EncounterTarget (constants/battle_encounter.h) to a battler bitmask, for commands.
// Never fails outright - a single-slot target invalid for the current battle format just
// contributes no bit, same as ResolveEncounterBattlerRef's _RIGHT-in-singles case. A
// state-changing command must assert on a fainted/absent battler found in the returned mask
// itself; that isn't checked here.
u32 ResolveEncounterTarget(enum EncounterTarget target);

// TRUE for ENC_TARGET_ALL_FOES / ALL_ALLIES / ALL_BATTLERS. A state-changing command uses this to
// decide whether a fainted/absent battler in the resolved set is tolerable (group: skip it) or an
// authoring mistake (single slot: assert).
bool32 IsEncounterGroupTarget(enum EncounterTarget target);

// Reads one operand of live battle state or event context, for comparison against a condition's
// value. useSnapshot (Stage 10) redirects ENC_OP_HP / ENC_OP_HP_PERCENT to runtime->prevHp instead
// of live HP; every other operand ignores it.
s32 GetEncounterOperand(enum EncounterOperand operand, u32 arg, bool32 useSnapshot);

// Walks conds - an array terminated by an ENC_OP_COUNT operand - ANDing every top-level node,
// short-circuiting on the first failure. A node is either a leaf comparison or an ENC_OP_ALL /
// ENC_OP_ANY / ENC_OP_NOT group (Stage 12) that consumes its child nodes; nesting is bounded by
// MAX_ENCOUNTER_COND_DEPTH, past which the offending subtree asserts and evaluates FALSE. NULL is
// vacuously TRUE: a trigger with no conditions is always eligible.
bool32 EvaluateConditions(const struct EncounterCondition *conds, bool32 useSnapshot);

static inline bool32 IsEncounterActive(void)
{
#if B_ENCOUNTER_SCRIPTING
    return gBattleStruct->encounter.id != ENCOUNTER_NONE;
#else
    return FALSE;
#endif
}

#endif // GUARD_BATTLE_ENCOUNTER_H
