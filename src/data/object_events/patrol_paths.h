// Waypoints in map-local tile coordinates (no MAP_OFFSET). Consecutive points share an x or y.
// The object starts at the first point and walks to the last, then back, in a loop.
static const struct Coords16 sPatrolPoints_Koraidon[] = {
    {26, 7}, {26, 30}, {11, 30}, {11, 56}, {16, 56}, {16, 77}, {28, 77}, {28, 91},
};

static const struct Coords16 sPatrolPoints_Miraidon[] = {
    {29, 91}, {29, 78}, {18, 78}, {18, 52}, {13, 52}, {13, 31}, {28, 31}, {28, 7},
};

static const struct PatrolPath sPatrolPaths[] = {
    { OBJ_EVENT_GFX_SPECIES(KORAIDON), ARRAY_COUNT(sPatrolPoints_Koraidon), sPatrolPoints_Koraidon },
    { OBJ_EVENT_GFX_SPECIES(MIRAIDON), ARRAY_COUNT(sPatrolPoints_Miraidon), sPatrolPoints_Miraidon },
};
