Common_EventScript_CheckLevelCapIncrease::
	callnative Achievement_CheckStoryMilestones
	specialvar VAR_TEMP_1, GetProgressionLevelCapSpecial
@ VAR_PREV_LEVEL_CAP is the last cap this script announced, not a value the
@ caller has to stage: VAR_RESULT is clobbered by any trainerbattle/msgbox
@ between a caller's pre-capture and the milestone that raises the cap.
	compare VAR_PREV_LEVEL_CAP, VAR_TEMP_1
	goto_if_lt Common_EventScript_LevelCapIncreased
	return

Common_EventScript_LevelCapIncreased::
	copyvar VAR_PREV_LEVEL_CAP, VAR_TEMP_1
	copyvar VAR_RESULT, VAR_TEMP_1
	callnative EnqueueLevelCapIncreasePopup
	return
