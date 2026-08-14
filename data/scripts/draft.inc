@ Draft mode's offer flow. One script tail serves both a route's own draft
@ (Draft_EventScript_RouteDraft, triggered by src/field_control_avatar.c)
@ and a gift/egg that got queued into the pending buffer instead of the PC
@ (Draft_EventScript_ResolvePending, entered directly - see §6/src/pokemon.c).
@ See src/draft_mode.c for the natives called here and include/draft_mode.h
@ for the pending-buffer contract.

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

@ Draft_IsAreaDraftable() (the field hook's gate) only confirms this area has
@ *some* wild encounters - it's deliberately unfiltered by Mono Type/Mono
@ Gen, so an organically-empty pool still reaches here rather than the field
@ hook silently never firing. Draft_Text_AreaIntro would be misleading in
@ that case (there's nothing this run could legally draft), so this is its
@ own terminal node rather than falling into the normal offer.
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

@ Mirrors the starter case UI's own "Give your Pokémon a Nickname? A: Yes
@ B: No" bottom-bar prompt (src/ui_birch_case.c's Task_BirchCaseRecievedMon) -
@ a Draft pick should get the same choice, not a mandatory naming screen.
@ Draft_Text_AskNickname stays species-generic rather than naming the mon
@ (unlike gText_NicknameThisPokemon) because VAR_0x8004 already holds the
@ right party slot coming from either path into here, but gStringVar1 does
@ not: the join path (Draft_BufferPendingNickname) leaves the incoming mon's
@ name there, while the replace path (Draft_BufferReplacementNames) leaves
@ the *outgoing* mon's name there instead.
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
