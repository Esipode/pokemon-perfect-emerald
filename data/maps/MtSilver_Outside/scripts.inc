MtSilver_Outside_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, MtSilver_Outside_OnTransition
	.byte 0

MtSilver_Outside_OnTransition: 
	setflag FLAG_VISITED_MT_SILVER
	end

MtSilver_Outside_EventScript_Sign::
	msgbox MtSilver_Outside_Text_Sign, MSGBOX_SIGN
	end

MtSilver_Outside_Text_Sign:
	.string "MT. SILVER$"
