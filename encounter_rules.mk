# encounter files are run through encounterproc, which converts the authored '.encounter' format
# into the struct EncounterCondition / EncounterTrigger / Encounter initializers battle_encounter.c
# expects (see include/battle_encounter.h).
# Every source is parsed into the one gEncounters[], so each legendary keeps its definition in its
# own file next to its battle scripts in data/legendary_encounters/.

ENCOUNTER_SRCS := src/data/battle_encounters.encounter $(sort $(wildcard src/data/legendary_encounters/*.encounter))

AUTO_GEN_TARGETS += src/data/battle_encounters.h

src/data/battle_encounters.h: $(ENCOUNTER_SRCS) $(ENCOUNTERPROC)
	$(ENCOUNTERPROC) -o $@ $(ENCOUNTER_SRCS)
