#include "global.h"
#include "event_data.h"
#include "test/overworld_script.h"
#include "test/test.h"

TEST("(Kanto progression) Misc Kanto Rockets stay while Hideout Giovanni is undefeated")
{
    FlagSet(FLAG_DEFEATED_LEADER_GIOVANNI);

    RUN_OVERWORLD_SCRIPT(
        call EventScript_TryHideMiscKantoRockets;
    );

    EXPECT(!FlagGet(FLAG_HIDE_MISC_KANTO_ROCKETS));
}

TEST("(Kanto progression) Misc Kanto Rockets leave once both Giovanni fights are done")
{
    FlagSet(FLAG_DEFEATED_LEADER_GIOVANNI);

    RUN_OVERWORLD_SCRIPT(
        settrainerflag TRAINER_BOSS_GIOVANNI;
        call EventScript_TryHideMiscKantoRockets;
    );

    EXPECT(FlagGet(FLAG_HIDE_MISC_KANTO_ROCKETS));
}

TEST("(Kanto progression) Misc Kanto Rockets stay while the Earth Badge is missing")
{
    RUN_OVERWORLD_SCRIPT(
        settrainerflag TRAINER_BOSS_GIOVANNI;
        call EventScript_TryHideMiscKantoRockets;
    );

    EXPECT(!FlagGet(FLAG_HIDE_MISC_KANTO_ROCKETS));
}

TEST("(Kanto progression) Lavender Town transition does not start the S.S. Anne scene")
{
    VarSet(VAR_MAP_SCENE_S_S_ANNE_2F_CORRIDOR, 0);

    RUN_OVERWORLD_SCRIPT(
        call LavenderTown_OnTransition;
    );

    EXPECT_EQ(VarGet(VAR_MAP_SCENE_S_S_ANNE_2F_CORRIDOR), 0);
    EXPECT(FlagGet(FLAG_WORLD_MAP_LAVENDER_TOWN));
}

TEST("(Kanto progression) Route 22 late rival needs the early fight and the Earth Badge")
{
    u32 scene, badge, expected;
    PARAMETRIZE { scene = 0; badge = TRUE;  expected = 0; }
    PARAMETRIZE { scene = 2; badge = TRUE;  expected = 3; }
    PARAMETRIZE { scene = 2; badge = FALSE; expected = 2; }

    VarSet(VAR_MAP_SCENE_ROUTE22, scene);
    if (badge)
        FlagSet(FLAG_DEFEATED_LEADER_GIOVANNI);

    RUN_OVERWORLD_SCRIPT(
        call EventScript_TryReadyRoute22LateRival;
    );

    EXPECT_EQ(VarGet(VAR_MAP_SCENE_ROUTE22), expected);
}

TEST("(Kanto progression) Daddy's Meteorite only advances Bill's One Island detour")
{
    u32 start, expected;
    PARAMETRIZE { start = 5; expected = 5; }
    PARAMETRIZE { start = 1; expected = 2; }

    VarSet(VAR_MAP_SCENE_ONE_ISLAND_POKEMON_CENTER_1F, start);

    RUN_OVERWORLD_SCRIPT(
        call TwoIsland_JoyfulGameCorner_EventScript_TryReadyLeaveOneIsland;
    );

    EXPECT_EQ(VarGet(VAR_MAP_SCENE_ONE_ISLAND_POKEMON_CENTER_1F), expected);
}
