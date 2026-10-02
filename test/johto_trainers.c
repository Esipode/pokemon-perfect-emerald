#include "global.h"
#include "battle_setup.h"
#include "data.h"
#include "constants/opponents.h"
#include "test/test.h"

TEST("(Johto trainers) Johto ids map to the Johto trainer flag range")
{
    EXPECT_EQ(GetTrainerFlagId(JOHTO_TRAINERS_START), JOHTO_TRAINER_FLAGS_START);
    EXPECT_EQ(GetTrainerFlagId(TRAINER_FALKNER_1_JOHTO), JOHTO_TRAINER_FLAGS_START + (TRAINER_FALKNER_1_JOHTO - JOHTO_TRAINERS_START));
    EXPECT_EQ(GetTrainerFlagId(JOHTO_TRAINERS_START + MAX_JOHTO_TRAINERS_COUNT - 1), JOHTO_TRAINER_FLAGS_START + MAX_JOHTO_TRAINERS_COUNT - 1);
    EXPECT_EQ(GetTrainerFlagId(JOHTO_TRAINERS_START + MAX_JOHTO_TRAINERS_COUNT), 0);
}

TEST("(Johto trainers) Hoenn and Kanto trainer flags are unchanged")
{
    EXPECT_EQ(GetTrainerFlagId(TRAINER_ROXANNE_1), TRAINER_FLAGS_START + TRAINER_ROXANNE_1);
    EXPECT_EQ(GetTrainerFlagId(TRAINER_YOUNGSTER_BEN), KANTO_TRAINER_FLAGS_START + 1);
    EXPECT_EQ(GetTrainerFlagId(TRAINER_CUE_BALL_PAXTON), KANTO_TRAINER_FLAGS_START + 623);
}

TEST("(Johto trainers) trainer blocks are disjoint and ordered")
{
    EXPECT(!IsKantoTrainerId(JOHTO_TRAINERS_START));
    EXPECT(IsKantoTrainerId(JOHTO_TRAINERS_START - 1));
    EXPECT(!IsJohtoTrainerId(JOHTO_TRAINERS_START - 1));
    EXPECT(IsJohtoTrainerId(TRAINER_RIVAL_TOTODILE_1_JOHTO));
    EXPECT(TRAINER_PARTNER(1) > JOHTO_TRAINERS_START + MAX_JOHTO_TRAINERS_COUNT - 1);
    EXPECT_EQ(KANTO_TRAINERS_START, 864);
    EXPECT_EQ(JOHTO_TRAINERS_START, 1508);
}
