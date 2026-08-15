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
        partnerPartyId = gBattlerPartyIndexes[BATTLE_PARTNER(battler)];

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
