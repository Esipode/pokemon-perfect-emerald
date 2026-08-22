# encounter files are run through encounterproc, which converts the authored '.encounter' format
# into the struct EncounterCondition / EncounterTrigger / Encounter initializers battle_encounter.c
# expects (see include/battle_encounter.h).
# Strip `#` comments before CPP sees them, because CPP otherwise reads them as directives.

AUTO_GEN_TARGETS += src/data/battle_encounters.h

%.h: %.encounter $(ENCOUNTERPROC)
	sed 's/#.*//' $< | $(CPP) $(CPPFLAGS) -traditional-cpp - | $(ENCOUNTERPROC) -o $@ -i $< -
