#ifndef GUARD_HEALTHBOX_H
#define GUARD_HEALTHBOX_H

#include "global.h"
#include "constants/healthbox.h"

// Effective (default-resolved) options; see struct HealthboxOptions in global.h.
void HealthboxOptions_Get(struct HealthboxOptions *out);
void HealthboxOptions_Commit(const struct HealthboxOptions *options);
void HealthboxOptions_SetDefaults(void);

u32 HealthboxOptions_GetStyle(void);
u32 HealthboxOptions_GetBackground(void);
bool32 HealthboxOptions_Shows(u32 side, enum HealthboxElement elem);
u32 HealthboxOptions_GetHpValue(u32 side);

// Classic HP display mapping (OPTIONS_HP_DISPLAY_*) <-> per-side bar/value toggles.
u32 HealthboxOptions_ModeFromToggles(bool32 bar, u32 value);
void HealthboxOptions_TogglesFromMode(u32 mode, bool32 *bar, u32 *value);
void HealthboxOptions_SetHpToggles(struct HealthboxOptions *options, u32 side, bool32 bar, u32 value);

#endif // GUARD_HEALTHBOX_H
