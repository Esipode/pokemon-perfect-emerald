#ifndef GUARD_FIELD_CHANSEY_H
#define GUARD_FIELD_CHANSEY_H

// Field Chansey: a heal/PC NPC placed near major battles. Every Chansey object
// shares FLAG_HIDE_FIELD_CHANSEY, so a map holds at most one.
struct FieldChanseySite
{
    u16 map;          // MAP_* constant
    u16 requiredFlag; // must be set; 0 = no requirement
    u16 blockingFlag; // must be clear; 0 = no requirement
    u16 var;          // 0 = no var check
    u16 varMin;       // inclusive
    u16 varMax;       // inclusive
};

// Recomputes FLAG_HIDE_FIELD_CHANSEY for the current map. If several rows
// share a map, the first match wins.
void FieldChansey_RefreshVisibility(void);

// Script hook: a Chansey heal counts as a Pokémon Center visit.
void FieldChansey_RecordHeal(void);

#endif // GUARD_FIELD_CHANSEY_H
