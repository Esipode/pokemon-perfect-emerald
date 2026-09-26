MeteorFalls_B1F_2R_MapScripts::
	map_script MAP_SCRIPT_ON_LOAD, MeteorFalls_B1F_2R_OnLoad
	.byte 0

MeteorFalls_B1F_2R_OnLoad:
	goto_if_unset FLAG_SYS_GAME_CLEAR, MeteorFalls_B1F_2R_HideRegidragoDoor
	special CountCaughtLegendaryFamilies
	goto_if_lt VAR_RESULT, 30, MeteorFalls_B1F_2R_HideRegidragoDoor
	goto MeteorFalls_B1F_2R_ShowRegidragoDoor

MeteorFalls_B1F_2R_HideRegidragoDoor:
	end

MeteorFalls_B1F_2R_ShowRegidragoDoor:
	setmetatilewithelevation 4, 1, 0x245, TRUE, 0
	setmetatilewithelevation 5, 1, 0x246, TRUE, 0
	setmetatilewithelevation 6, 1, 0x247, TRUE, 0
	
	setmetatilewithelevation 4, 2, 0x24D, TRUE, 0
	setmetatilewithelevation 5, 2, 0x24E, FALSE, 3
	setmetatilewithelevation 6, 2, 0x24F, TRUE, 0
	end

