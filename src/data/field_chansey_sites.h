// One row per map; the first match wins. Visible while requiredFlag is set,
// blockingFlag is clear and var is within [varMin, varMax].
static const struct FieldChanseySite sFieldChanseySites[] =
{
    // Story
    { .map = MAP_ROUTE103,                    .blockingFlag = FLAG_DEFEATED_RIVAL_ROUTE103 },
    { .map = MAP_PETALBURG_WOODS,             .blockingFlag = FLAG_BEAT_FIRST_GRUNT },
    { .map = MAP_ROUTE104,                    .blockingFlag = FLAG_DEFEATED_RIVAL_ROUTE_104 },
    { .map = MAP_ROUTE110,                    .var = VAR_ROUTE110_STATE, .varMin = 0, .varMax = 0 },
    { .map = MAP_MT_CHIMNEY,                  .blockingFlag = FLAG_DEFEATED_EVIL_TEAM_MT_CHIMNEY },
    { .map = MAP_ROUTE119,                    .var = VAR_ROUTE119_STATE, .varMin = 0, .varMax = 0 },
    { .map = MAP_AQUA_HIDEOUT_B2F,            .blockingFlag = FLAG_TEAM_AQUA_ESCAPED_IN_SUBMARINE },
    { .map = MAP_MAGMA_HIDEOUT_4F,            .blockingFlag = FLAG_GROUDON_AWAKENED_MAGMA_HIDEOUT },
    { .map = MAP_MOSSDEEP_CITY_SPACE_CENTER_2F, .blockingFlag = FLAG_DEFEATED_MAGMA_SPACE_CENTER, .var = VAR_MOSSDEEP_CITY_STATE, .varMin = 2, .varMax = 2 },
    { .map = MAP_SEAFLOOR_CAVERN_ROOM9,       .blockingFlag = FLAG_KYOGRE_ESCAPED_SEAFLOOR_CAVERN },
    { .map = MAP_VICTORY_ROAD_1F,             .blockingFlag = FLAG_DEFEATED_WALLY_VICTORY_ROAD, .var = VAR_VICTORY_ROAD_1F_STATE, .varMin = 0, .varMax = 0 },

    // Gyms
    { .map = MAP_RUSTBORO_CITY_GYM,           .blockingFlag = FLAG_DEFEATED_RUSTBORO_GYM },
    { .map = MAP_DEWFORD_TOWN_GYM,            .blockingFlag = FLAG_DEFEATED_DEWFORD_GYM },
    { .map = MAP_MAUVILLE_CITY_GYM,           .blockingFlag = FLAG_DEFEATED_MAUVILLE_GYM },
    { .map = MAP_LAVARIDGE_TOWN_GYM_1F,       .blockingFlag = FLAG_DEFEATED_LAVARIDGE_GYM },
    { .map = MAP_PETALBURG_CITY_GYM,          .blockingFlag = FLAG_DEFEATED_PETALBURG_GYM },
    { .map = MAP_FORTREE_CITY_GYM,            .blockingFlag = FLAG_DEFEATED_FORTREE_GYM },
    { .map = MAP_MOSSDEEP_CITY_GYM,           .blockingFlag = FLAG_DEFEATED_MOSSDEEP_GYM },
    { .map = MAP_SOOTOPOLIS_CITY_GYM_1F,      .blockingFlag = FLAG_DEFEATED_SOOTOPOLIS_GYM },

    // Post-game
    { .map = MAP_METEOR_FALLS_STEVENS_CAVE,   .requiredFlag = FLAG_SYS_GAME_CLEAR, .blockingFlag = FLAG_DEFEATED_METEOR_FALLS_STEVEN },
};
