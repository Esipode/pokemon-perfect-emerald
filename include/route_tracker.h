#ifndef GUARD_ROUTE_TRACKER_H
#define GUARD_ROUTE_TRACKER_H

struct RouteProgress
{
    u8 speciesCaught;
    u8 speciesTotal;
    u8 itemsCollected;
    u8 itemsTotal;
    u8 trainersDefeated;
    u8 trainersTotal;
};

// Fills progress with the current map's completion counts. Returns FALSE when the map tracks nothing.
bool32 GetCurrentRouteProgress(struct RouteProgress *progress);

#endif // GUARD_ROUTE_TRACKER_H
