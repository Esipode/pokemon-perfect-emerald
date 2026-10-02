MtSilver_3F_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, MtSilver_3F_OnTransition
	map_script MAP_SCRIPT_ON_RESUME, MtSilver_3F_OnResume
	.byte 0

MtSilver_3F_OnResume::
	setescapewarp MAP_MT_SILVER_OUTSIDE, 17, 8
	end

MtSilver_3F_OnTransition: 
	setescapewarp MAP_MT_SILVER_OUTSIDE, 17, 8
	end
