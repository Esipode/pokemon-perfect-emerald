#include "config/battle.h"
#include "constants/global.h"
#include "constants/battle.h"
#include "constants/battle_script_commands.h"
#include "constants/battle_string_ids.h"
	.include "asm/macros.inc"
	.include "asm/macros/battle_script.inc"
	.include "constants/constants.inc"

	.section script_data, "aw", %progbits

// Engine-owned shim: called via BattleScriptExecute from a non-script callback, with the
// encounter script itself invoked via BattleScriptCall. All encounter scripts end with
// `return`, which unwinds here; this is the only encounter script allowed to end2.
BattleScript_EncounterCheckpointEnd2::
	end2

EncScript_TestBattleStart::
	printstring STRINGID_EMPTYSTRING3
	waitmessage B_WAIT_TIME_LONG
	return

EncScript_TestTurnEnd::
	printstring STRINGID_EMPTYSTRING3
	waitmessage B_WAIT_TIME_LONG
	return
