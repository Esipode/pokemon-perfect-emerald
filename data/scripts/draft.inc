@ Draft mode's offer flow. One script tail serves both a route's own draft
@ (Draft_EventScript_RouteDraft, from src/field_control_avatar.c) and a gift/egg
@ queued into the pending buffer instead of the PC (Draft_EventScript_ResolvePending,
@ entered from src/pokemon.c). Natives: src/draft_mode.c; buffer contract:
@ include/draft_mode.h.

Draft_EventScript_RouteDraft::
	lockall
	callnative Draft_CheckPoolEligible
	goto_if_eq VAR_RESULT, FALSE, Draft_EventScript_NoEligiblePool
	msgbox Draft_Text_AreaIntro, MSGBOX_DEFAULT
	fadescreen FADE_TO_BLACK
	callnative Draft_StartCaseUI
	waitstate
	goto_if_eq VAR_0x8004, 1, Draft_EventScript_ConfirmDecline
	goto Draft_EventScript_ResolvePending

@ Draft_IsAreaDraftable() only confirms the area has some wild encounters,
@ unfiltered by Mono Type/Mono Gen. A pool the filters empty lands here, a
@ terminal node, since Draft_Text_AreaIntro would offer nothing draftable.
Draft_EventScript_NoEligiblePool::
	msgbox Draft_Text_NoEligiblePool, MSGBOX_DEFAULT
	callnative Draft_MarkAreaSpentNoOffer
	releaseall
	end

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

@ Reached by declining the case (VAR_0x8004 == 1, no pending mon yet) or by
@ cancelling the replace screen (VAR_0x8004 == PARTY_NOTHING_CHOSEN, a pending
@ mon waits). Draft_Text_ConfirmDecline stays generic to read correctly in both,
@ and "NO" routes back using the differing VAR_0x8004 value.
Draft_EventScript_ConfirmDecline::
	msgbox Draft_Text_ConfirmDecline, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, YES, Draft_EventScript_ConfirmDecline_Accept
	goto_if_eq VAR_0x8004, 1, Draft_EventScript_RouteDraft
	goto Draft_EventScript_PartyFull

Draft_EventScript_ConfirmDecline_Accept::
	callnative Draft_DiscardPending
	goto Draft_EventScript_Finish

@ Mirrors the starter case's "Give your Pokémon a Nickname?" prompt
@ (Task_BirchCaseRecievedMon, src/ui_birch_case.c). Draft_Text_AskNickname is
@ species-generic because gStringVar1 differs by path: the join path leaves the
@ incoming mon's name, the replace path the outgoing mon's. VAR_0x8004 already
@ holds the right slot either way.
Draft_EventScript_OfferNickname::
	msgbox Draft_Text_AskNickname, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Draft_EventScript_Finish
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

Draft_Text_AskNickname:
	.string "Would you like to give your new\n"
	.string "Pokémon a nickname?$"

Draft_Text_NoEligiblePool:
	.string "None of the Pokémon native to this\n"
	.string "area fit your current rules. There's\l"
	.string "nothing to draft here.$"
