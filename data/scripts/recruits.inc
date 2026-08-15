@ Recruits mode's retirement flow, triggered by
@ Recruits_TryStartFieldScript (src/field_control_avatar.c). See
@ src/recruits_mode.c for the natives called here.

@ The message runs before Recruits_DoRetirement so the mon's name is still
@ readable - the native zeroes and compacts the slot it names. Compacting can
@ move a different mon into slot 0, so UpdateFollowingPokemon (src/event_
@ object_movement.c) refreshes the overworld follower sprite right after,
@ same as every other party-changing flow (e.g. data/scripts/day_care.inc).
Recruits_EventScript_Retire::
	lockall
	msgbox Recruits_Text_ServiceEnded, MSGBOX_DEFAULT
	callnative Recruits_DoRetirement
	callnative UpdateFollowingPokemon
	callnative Recruits_IsRunFailed
	goto_if_eq VAR_RESULT, TRUE, Recruits_EventScript_RunFailed
	releaseall
	end

@ Never routes through CB2_WhiteOut - see the design note on this in
@ src/recruits_mode.c above Recruits_StartRunFailedScreen.
Recruits_EventScript_RunFailed::
	fadescreen FADE_TO_BLACK
	callnative Recruits_StartRunFailedScreen
	end

Recruits_Text_ServiceEnded:
	.string "{STR_VAR_1}'s service has ended!$"
