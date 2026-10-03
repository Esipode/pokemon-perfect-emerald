#include "global.h"
#include "event_data.h"
#include "new_game.h"
#include "pokemon.h"
#include "script.h"
#include "strings.h"
#include "string_util.h"
#include "test/test.h"
#include "trade.h"
#include "ui_birch_case.h"
#include "constants/flags.h"
#include "constants/trade.h"

static bool32 IsCanonicalStarterName(const u8 *name)
{
    u8 gen, slot;

    for (gen = 1; gen <= 9; gen++)
    {
        for (slot = 0; slot < 3; slot++)
        {
            if (StringCompare(name, GetSpeciesName(GetCanonicalStarterSpecies(gen, slot))) == 0)
                return TRUE;
        }
    }

    return FALSE;
}

TEST("Canonical starter lookup covers all generations")
{
    u8 gen, slot;

    for (gen = 1; gen <= 9; gen++)
    {
        for (slot = 0; slot < 3; slot++)
            EXPECT(IsCanonicalStarterSpecies(GetCanonicalStarterSpecies(gen, slot)));
    }

    EXPECT(!IsCanonicalStarterSpecies(SPECIES_PIKACHU));
}

TEST("Random in-game trades avoid starters unless species randomization is enabled")
{
    u32 savedTrainerId = GetTrainerId(gSaveBlock2Ptr->playerTrainerId);
    u16 savedTradeId = gSpecialVar_0x8005;
    bool32 randomizeWasEnabled = FlagGet(FLAG_RANDOMIZE_MON);
    u32 trainerId, tradeId;

    FlagClear(FLAG_RANDOMIZE_MON);
    for (u8 gen = 1; gen <= 9; gen++)
    {
        for (u8 slot = 0; slot < 3; slot++)
                EXPECT(!IsRandomInGameTradeSpeciesAllowed(GetCanonicalStarterSpecies(gen, slot), FALSE));
    }
            EXPECT(IsRandomInGameTradeSpeciesAllowed(SPECIES_PIKACHU, FALSE));

    for (u8 gen = 1; gen <= 9; gen++)
    {
        for (u8 slot = 0; slot < 3; slot++)
                EXPECT(IsRandomInGameTradeSpeciesAllowed(GetCanonicalStarterSpecies(gen, slot), TRUE));
    }
            EXPECT(IsRandomInGameTradeSpeciesAllowed(SPECIES_PIKACHU, TRUE));

    FlagClear(FLAG_RANDOMIZE_MON);
    for (trainerId = 0; trainerId < 64; trainerId++)
    {
        SetTrainerId(trainerId, gSaveBlock2Ptr->playerTrainerId);
        for (tradeId = INGAME_TRADE_RANDOM1; tradeId <= INGAME_TRADE_RANDOM18; tradeId++)
        {
            u16 requested;

            gSpecialVar_0x8005 = tradeId;
            requested = GetInGameTradeSpeciesInfo();
            EXPECT(!IsCanonicalStarterSpecies(requested));
            EXPECT(!IsCanonicalStarterName(gStringVar2));
        }
    }

    if (randomizeWasEnabled)
        FlagSet(FLAG_RANDOMIZE_MON);
    else
        FlagClear(FLAG_RANDOMIZE_MON);
    SetTrainerId(savedTrainerId, gSaveBlock2Ptr->playerTrainerId);
    gSpecialVar_0x8005 = savedTradeId;
}