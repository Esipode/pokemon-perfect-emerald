@ Draft mode's offer flow. One script tail serves both a route's own draft
@ (Draft_EventScript_RouteDraft, triggered by src/field_control_avatar.c)
@ and a gift/egg that got queued into the pending buffer instead of the PC
@ (Draft_EventScript_ResolvePending, entered directly - see §6/src/pokemon.c).
@ See src/draft_mode.c for the natives called here and include/draft_mode.h
@ for the pending-buffer contract.

Draft_EventScript_RouteDraft::
	lockall
	msgbox Draft_Text_AreaIntro, MSGBOX_DEFAULT
	fadescreen FADE_TO_BLACK
	callnative Draft_StartCaseUI
	waitstate
	goto_if_eq VAR_0x8004, 1, Draft_EventScript_ConfirmDecline
	goto Draft_EventScript_ResolvePending

@ Entry point for a gift/egg queued by the pending-buffer funnels in
@ src/pokemon.c, and the tail of the draft pick above.
Draft_EventScript_ResolvePending::
	callnative Draft_TryGiveToEmptySlot
	goto_if_eq VAR_RESULT, 1, Draft_EventScript_PartyFull
	callnative Draft_BufferPendingNickname
	msgbox Draft_Text_Joined, MSGBOX_DEFAULT
	goto Draft_EventScript_OfferNickname

Draft_EventScript_PartyFull::
	msgbox Draft_Text_PartyFullWarning, MSGBOX_DEFAULT
	special ChoosePartyMon
	goto_if_eq VAR_0x8004, PARTY_NOTHING_CHOSEN, Draft_EventScript_ConfirmDecline
	callnative Draft_BufferReplacementNames
	msgbox Draft_Text_ConfirmReplace, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Draft_EventScript_PartyFull
	callnative Draft_DoReplacement
	msgbox Draft_Text_Replaced, MSGBOX_DEFAULT
	goto Draft_EventScript_OfferNickname

@ Reached either by declining the case outright (VAR_0x8004 == 1, no pending
@ mon exists yet - Task_BirchCaseMain's B-button path) or by cancelling out
@ of the replace screen above (VAR_0x8004 == PARTY_NOTHING_CHOSEN, a real
@ pending mon is waiting for a slot). Draft_Text_ConfirmDecline stays
@ area-level generic rather than naming a species, since it has to read
@ correctly in both cases - and since both cases leave a different value in
@ VAR_0x8004, "NO" can route back to wherever the player actually came from
@ without a native needed to remember it.
Draft_EventScript_ConfirmDecline::
	msgbox Draft_Text_ConfirmDecline, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, YES, Draft_EventScript_ConfirmDecline_Accept
	goto_if_eq VAR_0x8004, 1, Draft_EventScript_RouteDraft
	goto Draft_EventScript_PartyFull

Draft_EventScript_ConfirmDecline_Accept::
	callnative Draft_DiscardPending
	goto Draft_EventScript_Finish

Draft_EventScript_OfferNickname::
	fadescreen FADE_TO_BLACK
	special ChangePokemonNickname
	goto Draft_EventScript_Finish

@ The offer flow's single terminal node. Draft_MarkAreaSpent must only ever
@ run here - see the comment on it in src/draft_mode.c.
Draft_EventScript_Finish::
	callnative Draft_MarkAreaSpent
	releaseall
	end

Draft_Text_AreaIntro:
	.string "Pokémon native to this area can be\n"
	.string "drafted! Choose one to join your team.$"

Draft_Text_Joined:
	.string "{STR_VAR_1} joined your team!$"

Draft_Text_PartyFullWarning:
	.string "Your party is full. Choose a Pokémon\n"
	.string "to release - this can't be undone!$"

Draft_Text_ConfirmReplace:
	.string "Release {STR_VAR_1} and welcome\n"
	.string "{STR_VAR_2} to your team?\l"
	.string "This can't be undone!$"

Draft_Text_Replaced:
	.string "{STR_VAR_2} joined your team!$"

Draft_Text_ConfirmDecline:
	.string "If you decline, this area's Pokémon\n"
	.string "will be gone forever.\l"
	.string "This can't be undone!$"
