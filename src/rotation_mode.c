#include "global.h"
#include "rotation_mode.h"
#include "battle.h"
#include "pokemon.h"
#include "random.h"

// Shared rules for Rotation Mode. See include/rotation_mode.h.

bool32 RotationMode_IsEnabled(void)
{
    return gSaveBlock2Ptr->rotationModeSetting != 0;
}

// Eligibility loop modelled on Cmd_forcerandomswitch (src/battle_script_commands.c).
u32 RotationMode_PickReplacement(enum BattlerId battler)
{
    struct Pokemon *party = GetBattlerParty(battler);
    u8 validMons[PARTY_SIZE];
    u32 validMonsCount = 0;
    u32 ownPartyId = gBattlerPartyIndexes[battler];
    u32 partnerPartyId = ownPartyId;

    if (IsDoubleBattle())
        partnerPartyId = gBattlerPartyIndexes[GetPartnerBattler(battler)];

    for (u32 i = 0; i < PARTY_SIZE; i++)
    {
        if (GetMonData(&party[i], MON_DATA_SPECIES) != SPECIES_NONE
         && !GetMonData(&party[i], MON_DATA_IS_EGG)
         && GetMonData(&party[i], MON_DATA_HP) != 0
         && i != ownPartyId
         && i != partnerPartyId)
        {
            validMons[validMonsCount++] = i;
        }
    }

    if (validMonsCount == 0)
        return PARTY_SIZE;

    return validMons[RandomUniform(RNG_ROTATION_MODE, 0, validMonsCount - 1)];
}

// Battle types and battler states where an engine-initiated switch would desync,
// break a scripted flow, or isn't implemented yet.
bool32 RotationMode_IsBattleEligible(enum BattlerId battler)
{
    // Trainer battles only -- wild battles never rotate.
    if (!(gBattleTypeFlags & BATTLE_TYPE_TRAINER))
        return FALSE;

    // BATTLE_TYPE_FRONTIER already covers Arena, Battle Tower, Dome, Palace,
    // Factory, Pike and Pyramid.
    if (gBattleTypeFlags & (BATTLE_TYPE_LINK
                           | BATTLE_TYPE_RECORDED_LINK
                           | BATTLE_TYPE_RECORDED
                           | BATTLE_TYPE_MULTI
                           | BATTLE_TYPE_INGAME_PARTNER
                           | BATTLE_TYPE_FRONTIER
                           | BATTLE_TYPE_TRAINER_HILL))
        return FALSE;

    if (gBattleStruct->battlerState[battler].commanderSpecies != SPECIES_NONE)
        return FALSE;
    if (gBattleMons[battler].volatiles.semiInvulnerable == STATE_SKY_DROP_TARGET)
        return FALSE;

    return TRUE;
}

// Doubles: pick exactly one of the two player battlers to rotate this turn,
// at random between whichever are currently eligible, and cache the pick so
// both battlers' end-turn handler calls agree on it.
bool32 RotationMode_ShouldRotate(enum BattlerId battler)
{
    if (!IsDoubleBattle())
        return TRUE;

    if (!gBattleStruct->rotationModeResolvedThisTurn)
    {
        enum BattlerId partner = GetPartnerBattler(battler);
        u8 candidates[2];
        u32 candidateCount = 0;

        candidates[candidateCount++] = battler; // Already known alive and eligible by the caller.
        if (IsBattlerAlive(partner) && RotationMode_IsBattleEligible(partner))
            candidates[candidateCount++] = partner;

        gBattleStruct->rotationModeChosenBattler = candidates[RandomUniform(RNG_ROTATION_MODE_DOUBLES, 0, candidateCount - 1)];
        gBattleStruct->rotationModeResolvedThisTurn = TRUE;
    }

    return gBattleStruct->rotationModeChosenBattler == battler;
}
