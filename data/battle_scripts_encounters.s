#include "config/battle.h"
#include "constants/global.h"
#include "constants/battle.h"
#include "constants/battle_script_commands.h"
#include "constants/battle_string_ids.h"
	.include "asm/macros.inc"
	.include "asm/macros/battle_script.inc"
	.include "constants/constants.inc"

	.section script_data, "aw", %progbits

EncScript_TestBattleStart::
	printstring STRINGID_EMPTYSTRING3
	waitmessage B_WAIT_TIME_LONG
	end2
