@ Hands out the items queued by RoamerCatch_PrepareItems (src/roamer.c) after a roamer catch.

Roamer_EventScript_GiveCatchItems::
	lockall
Roamer_EventScript_NextCatchItem:
	callnative RoamerCatch_PopPendingItem
	goto_if_eq VAR_RESULT, ITEM_NONE, Roamer_EventScript_CatchItemsDone
	giveitem VAR_RESULT
	goto Roamer_EventScript_NextCatchItem

Roamer_EventScript_CatchItemsDone:
	releaseall
	end
