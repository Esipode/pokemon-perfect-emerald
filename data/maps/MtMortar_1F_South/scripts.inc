MtMortar_1F_South_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, MtMortar_1F_South_OnTransition
	.byte 0

MtMortar_1F_South_OnTransition: 
	end



MtMortar_1F_South_EventScript_Item_Aerial_Ace::
    finditem ITEM_TM_RAPID_SPIN
    end

MtMortar_1F_South_EventScript_Item_Elixir::
    finditem ITEM_ELIXIR
    end
