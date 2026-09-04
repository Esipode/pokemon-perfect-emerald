@ commands
	.macro attackcanceler
	.byte B_SCR_OP_ATTACKCANCELER
	.endm

	.macro printattackstring
	.byte B_SCR_OP_PRINTATTACKSTRING
	.endm

	.macro printselectionstringfromtable ptr:req
	.byte B_SCR_OP_PRINTSELECTIONSTRINGFROMTABLE
	.4byte \ptr
	.endm

	.macro typecalc
	.byte B_SCR_OP_TYPECALC
	.endm

	.macro multihitresultmessage
	.byte B_SCR_OP_MULTIHITRESULTMESSAGE
	.endm

	.macro attackanimation
	.byte B_SCR_OP_ATTACKANIMATION
	.endm

	.macro waitanimation
	.byte B_SCR_OP_WAITANIMATION
	.endm

	.macro healthbarupdate battler:req
	.byte B_SCR_OP_HEALTHBARUPDATE
	.byte \battler
	.endm

	.macro datahpupdate battler:req assuranceDouble:req
	.byte B_SCR_OP_DATAHPUPDATE
	.byte \battler
    .byte \assuranceDouble
	.endm

	.macro critmessage
	.byte B_SCR_OP_CRITMESSAGE
	.endm

	.macro effectivenesssound
	.byte B_SCR_OP_EFFECTIVENESSSOUND
	.endm

	.macro resultmessage
	.byte B_SCR_OP_RESULTMESSAGE
	.endm

	.macro printstring id:req
	.byte B_SCR_OP_PRINTSTRING
	.2byte \id
	.endm

	.macro printsavedstring
	printstring 0
	.endm

	.macro printselectionstring id:req
	.byte B_SCR_OP_PRINTSELECTIONSTRING
	.2byte \id
	.endm

	.macro waitmessage time:req
	.byte B_SCR_OP_WAITMESSAGE
	.2byte \time
	.endm

	.macro printfromtable ptr:req
	.byte B_SCR_OP_PRINTFROMTABLE
	.4byte \ptr
	.endm

	.macro setadditionaleffects
	.byte B_SCR_OP_SETADDITIONALEFFECTS
	.endm

	.macro seteffectprimary battler:req, effectBattler:req, moveEffect=0
	.if \moveEffect != 0
		setmoveeffect \moveEffect
	.endif
	.byte B_SCR_OP_SETEFFECTPRIMARY
	.byte \battler
	.byte \effectBattler
	.endm

	.macro clearvolatile battler:req, volatile:req
	.byte B_SCR_OP_CLEARVOLATILE
	.byte \battler
	.byte \volatile
	.endm

	.macro tryfaintmon battler:req
	.byte B_SCR_OP_TRYFAINTMON
	.byte \battler
	.byte FALSE
	.4byte NULL
	.endm

	.macro tryfaintmon_spikes battler:req, instr:req
	.byte B_SCR_OP_TRYFAINTMON
	.byte \battler
	.byte TRUE
	.4byte \instr
	.endm

	.macro dofaintanimation battler:req
	.byte B_SCR_OP_DOFAINTANIMATION
	.byte \battler
	.endm

	.macro cleareffectsonfaint battler:req
	.byte B_SCR_OP_CLEAREFFECTSONFAINT
	.byte \battler
	.endm

	.macro jumpifstatus battler:req, flags:req, jumpInstr:req
	.byte B_SCR_OP_JUMPIFSTATUS
	.byte \battler
	.4byte \flags
	.4byte \jumpInstr
	.endm

	.macro jumpifvolatile battler:req, volatile:req, jumpInstr:req
	.byte B_SCR_OP_JUMPIFVOLATILE
	.byte \battler
	.byte \volatile
	.4byte \jumpInstr
	.endm

	.macro jumpifability battler:req, ability:req, jumpInstr:req
	.byte B_SCR_OP_JUMPIFABILITY
	.byte \battler
	.2byte \ability
	.4byte \jumpInstr
	.endm

	.macro jumpifsideaffecting battler:req, flags:req, jumpInstr:req
	.byte B_SCR_OP_JUMPIFSIDEAFFECTING
	.byte \battler
	.4byte \flags
	.4byte \jumpInstr
	.endm

	.macro jumpifstat battler:req, comparison:req, stat:req, value:req, jumpInstr:req
	.byte B_SCR_OP_JUMPIFSTAT
	.byte \battler
	.byte \comparison
	.byte \stat
	.byte \value
	.4byte \jumpInstr
	.endm

	.macro jumpifstatignorecontrary battler:req, comparison:req, stat:req, value:req, jumpInstr:req
	.byte B_SCR_OP_JUMPIFSTATIGNORECONTRARY
	.byte \battler
	.byte \comparison
	.byte \stat
	.byte \value
	.4byte \jumpInstr
	.endm

	.macro jumpbasedontype battler:req, type:req, jumpIfType:req, jumpInstr:req
	.byte B_SCR_OP_JUMPBASEDONTYPE
	.byte \battler
	.byte \type
	.byte \jumpIfType
	.4byte \jumpInstr
	.endm

	.macro jumpiftype battler:req, type:req, jumpInstr:req
	jumpbasedontype \battler, \type, TRUE, \jumpInstr
	.endm

	.macro jumpifnottype battler:req, type:req, jumpInstr:req
	jumpbasedontype \battler, \type, FALSE, \jumpInstr
	.endm

	.macro getexp battler:req
	.byte B_SCR_OP_GETEXP
	.byte \battler
	.endm

	.macro checkteamslost jumpInstr:req
	.byte B_SCR_OP_CHECKTEAMSLOST
	.4byte \jumpInstr
	.endm

	.macro goto instr:req
	.byte B_SCR_OP_GOTO
	.4byte \instr
	.endm

	.macro jumpifbyte comparison:req, bytePtr:req, value:req, jumpInstr:req
	.byte B_SCR_OP_JUMPIFBYTE
	.byte \comparison
	.4byte \bytePtr
	.byte \value
	.4byte \jumpInstr
	.endm

	.macro jumpifhalfword comparison:req, halfwordPtr:req, value:req, jumpInstr:req
	.byte B_SCR_OP_JUMPIFHALFWORD
	.byte \comparison
	.4byte \halfwordPtr
	.2byte \value
	.4byte \jumpInstr
	.endm

	.macro jumpifword comparison:req, wordPtr:req, value:req, jumpInstr:req
	.byte B_SCR_OP_JUMPIFWORD
	.byte \comparison
	.4byte \wordPtr
	.4byte \value
	.4byte \jumpInstr
	.endm

	.macro jumpifarrayequal array1:req, array2:req, size:req, jumpInstr:req
	.byte B_SCR_OP_JUMPIFARRAYEQUAL
	.4byte \array1
	.4byte \array2
	.byte \size
	.4byte \jumpInstr
	.endm

	.macro jumpifarraynotequal array1:req, array2:req, size:req, jumpInstr:req
	.byte B_SCR_OP_JUMPIFARRAYNOTEQUAL
	.4byte \array1
	.4byte \array2
	.byte \size
	.4byte \jumpInstr
	.endm

	.macro setbyte bytePtr:req, value:req
	.byte B_SCR_OP_SETBYTE
	.4byte \bytePtr
	.byte \value
	.endm

	.macro addbyte bytePtr:req, value:req
	.byte B_SCR_OP_ADDBYTE
	.4byte \bytePtr
	.byte \value
	.endm

	.macro subbyte bytePtr:req, value:req
	.byte B_SCR_OP_SUBBYTE
	.4byte \bytePtr
	.byte \value
	.endm

@ encounter script variables (constants/battle_encounter.h) - thin wrappers over the opcodes
@ above, addressed into gEncounterVars so authors write a var index instead of a raw address.
@ NOTE: this uses `gEncounterVars + \var` directly rather than the sENCOUNTER_VAR() cpp macro -
@ .include (unlike #include) is never seen by the C preprocessor, so a #define referenced inside
@ a macro body here would reach the assembler unexpanded. sENCOUNTER_VAR is still fine for a
@ script to use directly, since a top-level .s file's own text *is* cpp-preprocessed.
	.macro encsetvar var:req, value:req
	setbyte gEncounterVars + \var, \value
	.endm

	.macro encaddvar var:req, value:req
	addbyte gEncounterVars + \var, \value
	.endm

	.macro encsubvar var:req, value:req
	subbyte gEncounterVars + \var, \value
	.endm

	.macro encjumpifvar comparison:req, var:req, value:req, jumpInstr:req
	jumpifbyte \comparison, gEncounterVars + \var, \value, \jumpInstr
	.endm

	@ Copies var \src into var \dst. For a script that needs to consume a counter without losing it -
	@ copy it to a scratch var and decrement that.
	.macro enccopyvar dst:req, src:req
	copybyte gEncounterVars + \dst, gEncounterVars + \src
	.endm

	.macro copyarray dest:req, src:req, size:req
	.byte B_SCR_OP_COPYARRAY
	.4byte \dest
	.4byte \src
	.byte \size
	.endm

	.macro copyarraywithindex dest:req, src:req, indexPtr:req, size:req
	.byte B_SCR_OP_COPYARRAYWITHINDEX
	.4byte \dest
	.4byte \src
	.4byte \indexPtr
	.byte \size
	.endm

	.macro orbyte bytePtr:req, value:req
	.byte B_SCR_OP_ORBYTE
	.4byte \bytePtr
	.byte \value
	.endm

	.macro orhalfword halfwordPtr:req, value:req
	.byte B_SCR_OP_ORHALFWORD
	.4byte \halfwordPtr
	.2byte \value
	.endm

	.macro orword wordPtr:req, value:req
	.byte B_SCR_OP_ORWORD
	.4byte \wordPtr
	.4byte \value
	.endm

	.macro bicbyte bytePtr:req, value:req
	.byte B_SCR_OP_BICBYTE
	.4byte \bytePtr
	.byte \value
	.endm

	.macro bichalfword halfwordPtr:req, value:req
	.byte B_SCR_OP_BICHALFWORD
	.4byte \halfwordPtr
	.2byte \value
	.endm

	.macro bicword wordPtr:req, value:req
	.byte B_SCR_OP_BICWORD
	.4byte \wordPtr
	.4byte \value
	.endm

	.macro pause frames:req
	.byte B_SCR_OP_PAUSE
	.2byte \frames
	.endm

	.macro setchargingturn
	.byte B_SCR_OP_SETCHARGINGTURN
	.endm

	.macro waitstate
	.byte B_SCR_OP_WAITSTATE
	.endm

	.macro tryselfconfusiondmgformchange
	.byte B_SCR_OP_TRYSELFCONFUSIONDMGFORMCHANGE
	.endm

	.macro return
	.byte B_SCR_OP_RETURN
	.endm

	.macro end
	.byte B_SCR_OP_END
	.endm

	.macro end3
	.byte B_SCR_OP_END3
	.endm

	.macro call instr:req
	.byte B_SCR_OP_CALL
	.4byte \instr
	.endm

	.macro setroost
	.byte B_SCR_OP_SETROOST
	.endm

	.macro jumpifabilitypresent ability:req, jumpInstr:req
	.byte B_SCR_OP_JUMPIFABILITYPRESENT
	.2byte \ability
	.4byte \jumpInstr
	.endm

	.macro endselectionscript
	.byte B_SCR_OP_ENDSELECTIONSCRIPT
	.endm

	.macro playanimation battler:req, animId:req, argPtr=NULL
	.byte B_SCR_OP_PLAYANIMATION
	.byte \battler
	.byte \animId
	.4byte \argPtr
	.endm

	.macro playanimation_var battler:req, animIdPtr:req, argPtr=NULL
	.byte B_SCR_OP_PLAYANIMATION_VAR
	.byte \battler
	.4byte \animIdPtr
	.4byte \argPtr
	.endm

	.macro jumpfifsemiinvulnerable battler:req, state:req, jumpInstr:req
	.byte B_SCR_OP_JUMPFIFSEMIINVULNERABLE
	.byte \battler
	.byte \state
	.4byte \jumpInstr
	.endm

	.macro trainerslidein position:req
	.byte B_SCR_OP_TRAINERSLIDEIN
	.byte \position
	.endm

	.macro moveend endMode:req, endState:req
	.byte B_SCR_OP_MOVEEND
	.byte \endMode
	.byte \endState
	.endm

	@ Help macros for 5 uses of moveend command

	@ All cases
	.macro moveendall
	setbyte sMOVEEND_STATE, 0
	moveend 0, 0
	.endm

	@ Chosen case
	.macro moveendcase case:req
	setbyte sMOVEEND_STATE, \case
	moveend 1, 0
	.endm

	@ All cases from (inclusive)
	.macro moveendfrom from:req
	setbyte sMOVEEND_STATE, \from
	moveend 0, 0
	.endm

	@ All cases from 0 to (not inclusive)
	.macro moveendto to:req
	setbyte sMOVEEND_STATE, 0
	moveend 2, \to
	.endm

	@ Cases from (inclusive) to (not inclusive)
	.macro moveendfromto from:req, to:req
	setbyte sMOVEEND_STATE, \from
	moveend 2, \to
	.endm

	.macro returnatktoball
	.byte B_SCR_OP_RETURNATKTOBALL
	.endm

	.macro getswitchedmondata battler:req
	.byte B_SCR_OP_GETSWITCHEDMONDATA
	.byte \battler
	.endm

	.macro switchindataupdate battler:req
	.byte B_SCR_OP_SWITCHINDATAUPDATE
	.byte \battler
	.endm

	.macro switchinanim battler:req, dontClearTransform:req, dontClearSubstitute:req
	.byte B_SCR_OP_SWITCHINANIM
	.byte \battler
	.byte \dontClearTransform
	.byte \dontClearSubstitute
	.endm

	.macro jumpifcantswitch battler:req, jumpInstr:req
	.byte B_SCR_OP_JUMPIFCANTSWITCH
	.byte \battler
	.4byte \jumpInstr
	.endm

	.macro openpartyscreen battler:req, failInstr:req
	.byte B_SCR_OP_OPENPARTYSCREEN
	.byte \battler
	.4byte \failInstr
	.endm

	.macro switchhandleorder battler:req, state:req
	.byte B_SCR_OP_SWITCHHANDLEORDER
	.byte \battler
	.byte \state
	.endm

	.macro switchineffects battler:req
	.byte B_SCR_OP_SWITCHINEFFECTS
	.byte \battler
	.endm

	.macro switchinevents
	.byte B_SCR_OP_SWITCHINEVENTS
	.endm

	.macro playse song:req
	.byte B_SCR_OP_PLAYSE
	.2byte \song
	.endm

	.macro fanfare song:req
	.byte B_SCR_OP_FANFARE
	.2byte \song
	.endm

	.macro playfaintcry battler:req
	.byte B_SCR_OP_PLAYFAINTCRY
	.byte \battler
	.endm

	.macro endlinkbattle
	.byte B_SCR_OP_ENDLINKBATTLE
	.endm

	.macro returntoball battler:req, changingForm:req
	.byte B_SCR_OP_RETURNTOBALL
	.byte \battler
	.byte \changingForm
	.endm

	.macro handlelearnnewmove learnedMoveInstr:req, nothingToLearnInstr:req, isFirstMove:req
	.byte B_SCR_OP_HANDLELEARNNEWMOVE
	.4byte \learnedMoveInstr
	.4byte \nothingToLearnInstr
	.byte \isFirstMove
	.endm

	.macro yesnoboxlearnmove forgotMoveInstr:req
	.byte B_SCR_OP_YESNOBOXLEARNMOVE
	.4byte \forgotMoveInstr
	.endm

	.macro yesnoboxstoplearningmove noInstr:req
	.byte B_SCR_OP_YESNOBOXSTOPLEARNINGMOVE
	.4byte \noInstr
	.endm

	.macro hitanimation battler:req
	.byte B_SCR_OP_HITANIMATION
	.byte \battler
	.endm

	.macro getmoneyreward
	.byte B_SCR_OP_GETMONEYREWARD
	.endm

	.macro updatebattlermoves battler:req
	.byte B_SCR_OP_UPDATEBATTLERMOVES
	.byte \battler
	.endm

	.macro swapattackerwithtarget
	.byte B_SCR_OP_SWAPATTACKERWITHTARGET
	.endm

	.macro incrementgamestat stat:req
	.byte B_SCR_OP_INCREMENTGAMESTAT
	.byte \stat
	.endm

	.macro drawpartystatussummary battler:req
	.byte B_SCR_OP_DRAWPARTYSTATUSSUMMARY
	.byte \battler
	.endm

	.macro hidepartystatussummary battler:req
	.byte B_SCR_OP_HIDEPARTYSTATUSSUMMARY
	.byte \battler
	.endm

	.macro jumptocalledmove notChosenMove:req
	.byte B_SCR_OP_JUMPTOCALLEDMOVE
	.byte \notChosenMove
	.endm

	.macro statusanimation battler:req, status=0, isVolatile=FALSE
	.byte B_SCR_OP_STATUSANIMATION
	.byte \battler
	.4byte \status
	.byte \isVolatile
	.endm

	.macro volatileanimation battler:req, volatile:req
	statusanimation \battler, \volatile, TRUE
	.endm

	.macro futuresighttargetfailure failInstr:req
	.byte B_SCR_OP_FUTURESIGHTTARGETFAILURE
	.4byte \failInstr
	.endm

	.macro getpossiblenexttarget jumpInstr:req
	.byte B_SCR_OP_GETPOSSIBLENEXTTARGET
	.4byte \jumpInstr
	.endm

	.macro yesnobox
	.byte B_SCR_OP_YESNOBOX
	.endm

	.macro cancelallactions
	.byte B_SCR_OP_CANCELALLACTIONS
	.endm

	.macro setgravity failInstr:req
	.byte B_SCR_OP_SETGRAVITY
	.4byte \failInstr
	.endm

	.macro removeitem battler:req
	.byte B_SCR_OP_REMOVEITEM
	.byte \battler
	.endm

	.macro atknameinbuff1
	.byte B_SCR_OP_ATKNAMEINBUFF1
	.endm

	.macro drawlvlupbox
	.byte B_SCR_OP_DRAWLVLUPBOX
	.endm

	.macro resetsentmonsvalue
	.byte B_SCR_OP_RESETSENTMONSVALUE
	.endm

	.macro setatktoplayer0
	.byte B_SCR_OP_SETATKTOPLAYER0
	.endm

	.macro makevisible battler:req
	.byte B_SCR_OP_MAKEVISIBLE
	.byte \battler
	.endm

	.macro recordability battler:req
	.byte B_SCR_OP_RECORDABILITY
	.byte \battler
	.endm

	.macro buffermovetolearn
	.byte B_SCR_OP_BUFFERMOVETOLEARN
	.endm

	.macro jumpifplayerran jumpInstr:req
	.byte B_SCR_OP_JUMPIFPLAYERRAN
	.4byte \jumpInstr
	.endm

	.macro hpthresholds battler:req
	.byte B_SCR_OP_HPTHRESHOLDS
	.byte \battler
	.endm

	.macro hpthresholds2 battler:req
	.byte B_SCR_OP_HPTHRESHOLDS2
	.byte \battler
	.endm

	.macro useitemonopponent
	.byte B_SCR_OP_USEITEMONOPPONENT
	.endm

	.macro setprotectlike
	.byte B_SCR_OP_SETPROTECTLIKE
	.endm

	.macro tryexplosion
	.byte B_SCR_OP_TRYEXPLOSION
	.endm

	.macro setatkhptozero
	.byte B_SCR_OP_SETATKHPTOZERO
	.endm

	.macro jumpifnexttargetvalid jumpInstr:req
	.byte B_SCR_OP_JUMPIFNEXTTARGETVALID
	.4byte \jumpInstr
	.endm

	.macro tryhealhalfhealth battler:req, failInstr:req
	.byte B_SCR_OP_TRYHEALHALFHEALTH
	.byte \battler
	.4byte \failInstr
	.endm

	.macro setfieldweather
	.byte B_SCR_OP_SETFIELDWEATHER
	.endm

	.macro setreflect
	.byte B_SCR_OP_SETREFLECT
	.endm

	.macro setseeded
	.byte B_SCR_OP_SETSEEDED
	.endm

	.macro manipulatedamage mode:req
	.byte B_SCR_OP_MANIPULATEDAMAGE
	.byte \mode
	.endm

	.macro trysetrest
	.byte B_SCR_OP_TRYSETREST
	.endm

	.macro jumpifuproarwakes jumpInstr:req
	.byte B_SCR_OP_JUMPIFUPROARWAKES
	.4byte \jumpInstr
	.endm

	.macro stockpiletohpheal failInstr:req
	.byte B_SCR_OP_STOCKPILETOHPHEAL
	.4byte \failInstr
	.endm

	.macro normalisebuffs
	.byte B_SCR_OP_NORMALISEBUFFS
	.endm

	.macro twoturnmoveschargestringandanimation
	.byte B_SCR_OP_TWOTURNMOVESCHARGESTRINGANDANIMATION
	.4byte 1f @animation then attack string
	@default - attack string then animation
	printsavedstring
	waitmessage B_WAIT_TIME_LONG
	attackanimation
	waitanimation
	goto 2f
	1:
	attackanimation
	waitanimation
	printsavedstring
	waitmessage B_WAIT_TIME_LONG
	2:
	.endm

	.macro trynonvolatilestatus
	.byte B_SCR_OP_TRYNONVOLATILESTATUS
	.endm

	.macro initmultihitstring
	.byte B_SCR_OP_INITMULTIHITSTRING
	.endm

	.macro forcerandomswitch failInstr:req
	.byte B_SCR_OP_FORCERANDOMSWITCH
	.4byte \failInstr
	.endm

	.macro tryconversiontypechange failInstr:req
	.byte B_SCR_OP_TRYCONVERSIONTYPECHANGE
	.4byte \failInstr
	.endm

	.macro givepaydaymoney
	.byte B_SCR_OP_GIVEPAYDAYMONEY
	.endm

	.macro setlightscreen
	.byte B_SCR_OP_SETLIGHTSCREEN
	.endm

	.macro tryKO failInstr:req
	.byte B_SCR_OP_TRYKO
	.4byte \failInstr
	.endm

	.macro checknonvolatiletrigger nonVolatile:req, failInstr:req
	.byte B_SCR_OP_CHECKNONVOLATILETRIGGER
	.2byte \nonVolatile
	.4byte \failInstr
	.endm

	.macro animatewildpokemonafterfailedpokeball battler:req
	.byte B_SCR_OP_ANIMATEWILDPOKEMONAFTERFAILEDPOKEBALL
	.byte \battler
	.endm

	.macro tryinfatuating failInstr:req
	.byte B_SCR_OP_TRYINFATUATING
	.4byte \failInstr
	.endm

	.macro updatestatusicon battler:req
	.byte B_SCR_OP_UPDATESTATUSICON
	.byte \battler
	.endm

	.macro setmist
	.byte B_SCR_OP_SETMIST
	.endm

	.macro setfocusenergy battler:req
	.byte B_SCR_OP_SETFOCUSENERGY
	.byte \battler
	.endm

	.macro transformdataexecution
	.byte B_SCR_OP_TRANSFORMDATAEXECUTION
	.endm

	.macro setsubstitute
	.byte B_SCR_OP_SETSUBSTITUTE
	.endm

	.macro mimicattackcopy failInstr:req
	.byte B_SCR_OP_MIMICATTACKCOPY
	.4byte \failInstr
	.endm

	.macro setcalledmove
	.byte B_SCR_OP_SETCALLEDMOVE
	.endm

	.macro disablelastusedattack failInstr:req
	.byte B_SCR_OP_DISABLELASTUSEDATTACK
	.4byte \failInstr
	.endm

	.macro trysetencore failInstr:req
	.byte B_SCR_OP_TRYSETENCORE
	.4byte \failInstr
	.endm

	.macro painsplitdmgcalc failInstr:req
	.byte B_SCR_OP_PAINSPLITDMGCALC
	.4byte \failInstr
	.endm

	.macro settypetorandomresistance failInstr:req
	.byte B_SCR_OP_SETTYPETORANDOMRESISTANCE
	.4byte \failInstr
	.endm

	.macro setalwayshitflag
	.byte B_SCR_OP_SETALWAYSHITFLAG
	.endm

	.macro copymovepermanently failInstr:req
	.byte B_SCR_OP_COPYMOVEPERMANENTLY
	.4byte \failInstr
	.endm

	.macro settailwind failInstr:req
	.byte B_SCR_OP_SETTAILWIND
	.4byte \failInstr
	.endm

	.macro tryspiteppreduce failInstr:req
	.byte B_SCR_OP_TRYSPITEPPREDUCE
	.4byte \failInstr
	.endm

	.macro healpartystatus
	.byte B_SCR_OP_HEALPARTYSTATUS
	.endm

	.macro cursetarget failInstr:req
	.byte B_SCR_OP_CURSETARGET
	.4byte \failInstr
	.endm

	.macro trysetspikes failInstr:req
	.byte B_SCR_OP_TRYSETSPIKES
	.4byte \failInstr
	.endm

	.macro setvolatile battler:req, volatile:req, value=TRUE
	.byte B_SCR_OP_SETVOLATILE
	.byte \battler
	.byte \volatile
	.byte \value
	.endm

	.macro trysetperishsong failInstr:req
	.byte B_SCR_OP_TRYSETPERISHSONG
	.4byte \failInstr
	.endm

	.macro jumpifconfusedandstatmaxed stat:req, jumpInstr:req
	.byte B_SCR_OP_JUMPIFCONFUSEDANDSTATMAXED
	.byte \stat
	.4byte \jumpInstr
	.endm

	.macro setembargo failInstr:req
	.byte B_SCR_OP_SETEMBARGO
	.4byte \failInstr
	.endm

	.macro setsafeguard
	.byte B_SCR_OP_SETSAFEGUARD
	.endm

	.macro jumpifnopursuitswitchdmg jumpInstr:req
	.byte B_SCR_OP_JUMPIFNOPURSUITSWITCHDMG
	.4byte \jumpInstr
	.endm

	.macro tryactivateitem battler:req, flag:req
	.byte B_SCR_OP_TRYACTIVATEITEM
	.byte \battler
	.byte \flag
	.endm

	.macro copyfoestats
	.byte B_SCR_OP_COPYFOESTATS
	.endm

	.macro rapidspinfree
	.byte B_SCR_OP_RAPIDSPINFREE
	.endm

	.macro recoverbasedonsunlight failInstr:req
	.byte B_SCR_OP_RECOVERBASEDONSUNLIGHT
	.4byte \failInstr
	.endm

	.macro setstickyweb failInstr:req
	.byte B_SCR_OP_SETSTICKYWEB
	.4byte \failInstr
	.endm

	.macro selectfirstvalidtarget
	.byte B_SCR_OP_SELECTFIRSTVALIDTARGET
	.endm

	.macro setsemiinvulnerablebit clear=FALSE
	.byte B_SCR_OP_SETSEMIINVULNERABLEBIT
	.byte \clear
	.endm

	.macro clearsemiinvulnerablebit
	setsemiinvulnerablebit TRUE
	.endm

	.macro trymemento failInstr:req
	.byte B_SCR_OP_TRYMEMENTO
	.4byte \failInstr
	.endm

	.macro setforcedtarget
	.byte B_SCR_OP_SETFORCEDTARGET
	.endm

	.macro curestatuswithmove failInstr:req
	.byte B_SCR_OP_CURESTATUSWITHMOVE
	.4byte \failInstr
	.endm

	.macro settorment failInstr:req
	.byte B_SCR_OP_SETTORMENT
	.4byte \failInstr
	.endm

	.macro settaunt failInstr:req
	.byte B_SCR_OP_SETTAUNT
	.4byte \failInstr
	.endm

	.macro trysethelpinghand failInstr:req
	.byte B_SCR_OP_TRYSETHELPINGHAND
	.4byte \failInstr
	.endm

	.macro tryswapitems failInstr:req
	.byte B_SCR_OP_TRYSWAPITEMS
	.4byte \failInstr
	.endm

	.macro trycopyability battler:req, failInstr:req
	.byte B_SCR_OP_TRYCOPYABILITY
	.byte \battler
	.4byte \failInstr
	.endm

	.macro trywish failInstr:req
	.byte B_SCR_OP_TRYWISH
	.4byte \failInstr
	.endm

	.macro settoxicspikes failInstr:req
	.byte B_SCR_OP_SETTOXICSPIKES
	.4byte \failInstr
	.endm

	.macro setgastroacid failInstr:req
	.byte B_SCR_OP_SETGASTROACID
	.4byte \failInstr
	.endm

	.macro setyawn failInstr:req
	.byte B_SCR_OP_SETYAWN
	.4byte \failInstr
	.endm

	.macro setroom
	.byte B_SCR_OP_SETROOM
	.endm

	.macro tryswapabilities failInstr:req
	.byte B_SCR_OP_TRYSWAPABILITIES
	.4byte \failInstr
	.endm

	.macro tryimprison failInstr:req
	.byte B_SCR_OP_TRYIMPRISON
	.4byte \failInstr
	.endm

	.macro setstealthrock failInstr:req
	.byte B_SCR_OP_SETSTEALTHROCK
	.4byte \failInstr
	.endm

	.macro trysetvolatile battler:req, _volatile:req, failInstr:req
	.byte B_SCR_OP_TRYSETVOLATILE
	.byte \battler
	.byte \_volatile
	.4byte \failInstr
	.endm

	.macro trysetmagiccoat failInstr:req
	.byte B_SCR_OP_TRYSETMAGICCOAT
	.4byte \failInstr
	.endm

	.macro trysetsnatch failInstr:req
	.byte B_SCR_OP_TRYSETSNATCH
	.4byte \failInstr
	.endm

	.macro switchoutabilities battler:req
	.byte B_SCR_OP_SWITCHOUTABILITIES
	.byte \battler
	.endm

	.macro jumpifhasnohp battler:req, jumpInstr:req
	.byte B_SCR_OP_JUMPIFHASNOHP
	.byte \battler
	.4byte \jumpInstr
	.endm

	.macro pickup
	.byte B_SCR_OP_PICKUP
	.endm

	.macro settypebasedhalvers failInstr:req
	.byte B_SCR_OP_SETTYPEBASEDHALVERS
	.4byte \failInstr
	.endm

	.macro tryrecycleitem failInstr:req
	.byte B_SCR_OP_TRYRECYCLEITEM
	.4byte \failInstr
	.endm

	.macro settypetoenvironment failInstr:req
	.byte B_SCR_OP_SETTYPETOENVIRONMENT
	.4byte \failInstr
	.endm

	.macro snatchsetbattlers
	.byte B_SCR_OP_SNATCHSETBATTLERS
	.endm

	.macro handleballthrow
	.byte B_SCR_OP_HANDLEBALLTHROW
	.endm

	.macro givecaughtmon passInstr:req
	.byte B_SCR_OP_GIVECAUGHTMON
	.4byte \passInstr
	.endm

	.macro trysetcaughtmondexflags failInstr:req
	.byte B_SCR_OP_TRYSETCAUGHTMONDEXFLAGS
	.4byte \failInstr
	.endm

	.macro displaydexinfo
	.byte B_SCR_OP_DISPLAYDEXINFO
	.endm

	.macro trygivecaughtmonnick
	.byte B_SCR_OP_TRYGIVECAUGHTMONNICK
	.endm

	.macro sortbattlers
	.byte B_SCR_OP_SORTBATTLERS
	.endm

	.macro removeattackerstatus1
	.byte B_SCR_OP_REMOVEATTACKERSTATUS1
	.endm

	.macro finishaction
	.byte B_SCR_OP_FINISHACTION
	.endm

	.macro finishturn
	.byte B_SCR_OP_FINISHTURN
	.endm

	.macro trainerslideout position:req
	.byte B_SCR_OP_TRAINERSLIDEOUT
	.byte \position
	.endm

	.macro settelekinesis failInstr:req
	.byte B_SCR_OP_SETTELEKINESIS
	.4byte \failInstr
	.endm

	.macro swapstatstages stat:req
	.byte B_SCR_OP_SWAPSTATSTAGES
	.byte \stat
	.endm

	.macro averagestats stat:req
	.byte B_SCR_OP_AVERAGESTATS
	.byte \stat
	.endm

	.macro setnonvolatilestatus trigger:req
	.byte B_SCR_OP_SETNONVOLATILESTATUS
	.byte \trigger
	.endm

	.macro tryoverwriteability failInstr:req
	.byte B_SCR_OP_TRYOVERWRITEABILITY
	.4byte \failInstr
	.endm

	.macro trysynchronize
	.byte B_SCR_OP_TRY_SYNCHRONIZE
	.endm

	.macro tryconfusionafterskydrop battler:req
	.byte B_SCR_OP_TRY_CONFUSION_AFTER_SKY_DROP
    .byte \battler
	.endm

    .macro trymovestatchanges
    .byte B_SCR_OP_TRYMOVESTATCHANGES
    .endm

    .macro trystatchanges battler:req statChangeFlags:req
    .byte B_SCR_OP_TRYSTATCHANGES
    .byte \battler
    .2byte \statChangeFlags
    .endm

    .macro trybattlerstatchange battler:req statChangeFlags:req
    .byte B_SCR_OP_TRYBATTLERSTATCHANGE
    .byte \battler
    .2byte \statChangeFlags
    .endm

	.macro callnative func:req
	.byte B_SCR_OP_CALLNATIVE
	.4byte \func
	.endm

@ Encounter command vocabulary (constants/battle_encounter.h's enum EncounterTarget).
@ callnative-first - none of these are real opcodes.

	@ Changes HP for every battler \target resolves to by \amount (positive = heal, negative =
	@ damage), reusing the same PASSIVE_HP_UPDATE healthbarupdate/datahpupdate opcodes Leftovers
	@ and status damage use, once per targeted battler so the health bar still animates for each.
	@ \mode is ENC_AMOUNT_FIXED (raw HP, the default) or ENC_AMOUNT_PERCENT (percent of that
	@ battler's own max HP - the portable choice when the boss's level isn't fixed).
	@ Scripted damage ignores the target's encounter damage reduction: the number here is the
	@ encounter's own, not something for another encounter rule to rescale.
	@ A battler taken to 0 HP faints in place, mirroring status/weather chip damage
	@ (BattleScript_DoTurnDmg); checkteamslost ends the battle if that empties a side.
	.macro encchangehp target:req, amount:req, mode=ENC_AMOUNT_FIXED
	callnative BS_EncounterChangeHpBegin
	.byte \target
	.2byte \amount
	.byte \mode
1:
	callnative BS_EncounterChangeHpStep
	.4byte 2f
	healthbarupdate BS_SCRIPTING, PASSIVE_HP_UPDATE
	datahpupdate BS_SCRIPTING, PASSIVE_HP_UPDATE
	tryfaintmon BS_SCRIPTING
	checkteamslost 2f
	goto 1b
2:
	.endm

	@ Records \target's current HP as a percentage of its max HP (0-100) into author variable \var.
	@ \target must resolve to exactly one battler - a percentage is one byte, so there is no sensible
	@ reading of "snapshot a whole set into it". A living battler never records 0.
	@ \mode selects what gets written:
	@   ENC_SNAP_SET      (default) the current percentage, overwriting whatever \var held
	@   ENC_SNAP_LOWEST   the current percentage, but only if it is LOWER than what \var holds
	@   ENC_SNAP_RECOVERY max(0, currentPct - \var) - how far \target has recovered since the mark
	@ \failInstr (optional) is jumped to when nothing was written: for LOWEST, the value was not
	@ lower; for RECOVERY, there was no recovery; for SET, never. So one command is both the test
	@ and the record, the same way encclearscreens is both the test and the clear.
	.macro encsnapshothp target:req, var:req, mode=ENC_SNAP_SET, failInstr=0
	callnative BS_EncSnapshotHp
	.byte \target
	.byte \var
	.byte \mode
	.4byte \failInstr
	.endm

	@ Moves \target's HP TO the percentage held in author variable \var, healing or damaging as
	@ needed, with the same animated health bar encchangehp uses. The counterpart to encsnapshothp:
	@ record a percentage into a var, then snap the battler back to it later. Like encchangehp this
	@ bypasses the target's damage reduction, and a battler taken to 0 HP faints in place.
	.macro encrewindhp target:req, var:req
	callnative BS_EncounterChangeHpBeginVar
	.byte \target
	.byte \var
1:
	callnative BS_EncounterChangeHpStep
	.4byte 2f
	healthbarupdate BS_SCRIPTING, PASSIVE_HP_UPDATE
	datahpupdate BS_SCRIPTING, PASSIVE_HP_UPDATE
	tryfaintmon BS_SCRIPTING
	checkteamslost 2f
	goto 1b
2:
	.endm

	@ Writes the AI's predicted move CATEGORY for \target into author variable \var:
	@   0 no prediction this turn   1 physical   2 special   3 status
	@ \target must resolve to exactly one battler. This is the real AI_FLAG_PREDICT_MOVE prediction
	@ (gAiLogicData->predictedMove), computed during action selection - so it is only meaningful at
	@ OnTurnStart, and only in an encounter whose AiFlags: include AI_FLAG_PREDICT_MOVE.
	@ The category rather than the move id: a move id doesn't fit in a byte, and three outcomes are
	@ what a player can actually plan around.
	.macro encstoreprediction target:req, var:req
	callnative BS_EncStorePrediction
	.byte \target
	.byte \var
	.endm

	@ Files the type of the move that just landed as a type-keyed damage resistance on \target, at
	@ \percent (capped at ENC_MAX_ADAPT_PERCENT), in a FIFO \slots deep (capped at
	@ ENC_MAX_ADAPTATIONS). Re-filing a type already held raises it to \percent instead of taking a
	@ second slot, and a \slots narrower than the board drops the overflow first.
	@ \countVar takes the resulting adaptation count - the authoritative board size, so a script that
	@ mirrors it can never drift. \resultVar takes an ENC_ADAPT_RESULT_* outcome:
	@   ENC_ADAPT_RESULT_HARDENED   the type was already held; its percent was raised
	@   ENC_ADAPT_RESULT_FILED      filed into a free slot
	@   ENC_ADAPT_RESULT_EVICTED    filed, and the OLDEST adaptation was pushed out
	@ The new type is buffered into B_BUFF1 and any evicted type into B_BUFF2.
	@ OnMoveEnd only - it reads the event's move. A move that missed, was blocked or had no effect is
	@ ignored outright and leaves both vars untouched, so a branch on \resultVar can't act on a whiff.
	.macro encadapt target:req, percent:req, slots:req, countVar:req, resultVar:req
	callnative BS_EncAdapt
	.byte \target
	.byte \percent
	.byte \slots
	.byte \countVar
	.byte \resultVar
	.endm

	@ Drops one type adaptation from \target - ENC_ADAPT_OLDEST, ENC_ADAPT_NEWEST or ENC_ADAPT_ALL -
	@ and writes the remaining count into \countVar. The dropped type is buffered into B_BUFF1
	@ (ENC_ADAPT_ALL buffers the newest of what it wiped). Silent, and a no-op on an empty board, so
	@ it is safe to call unconditionally - test \countVar afterwards to know whether anything fell.
	.macro encpurgeadapt target:req, which:req, countVar:req
	callnative BS_EncPurgeAdapt
	.byte \target
	.byte \which
	.byte \countVar
	.endm

	@ Writes the result of comparing \targetA's live \stat against \targetB's into author variable
	@ \var: 0 = A is lower, 1 = equal, 2 = A is higher. Both targets must resolve to exactly one
	@ live battler. Conditions and encjumpifvar only compare a var against a literal, so a rule that
	@ weighs one battler's stat against another's needs a command that parks the answer in a var.
	@ STAT_SPEED reads the full turn-order speed (Tailwind, Choice Scarf, paralysis, stages); the
	@ other stats read the plain stat-with-stages value. STAT_ACC/STAT_EVASION have no battle stat.
	.macro enccomparestat targetA:req, targetB:req, stat:req, var:req
	callnative BS_EncCompareStat
	.byte \targetA
	.byte \targetB
	.byte \stat
	.byte \var
	.endm

	@ Adds \stages (signed) to \stat for every battler \target resolves to, clamped to the normal
	@ stat stage range. Silent - no message, no animation (outline Sec31); pair with playanimation
	@ or a literal trybattlerstatchange call if presentation is wanted.
	.macro encchangestat target:req, stat:req, stages:req
	callnative BS_EncounterChangeStat
	.byte \target
	.byte \stat
	.byte \stages
	.endm

	@ Moves the raw battle stat (not the stage) of \stat by \amount for every battler \target
	@ resolves to. \mode is ENC_AMOUNT_FIXED (raw points, the default) or ENC_AMOUNT_PERCENT
	@ (percent of the battler's current value for that stat). \stat must be STAT_ATK, STAT_DEF,
	@ STAT_SPATK, STAT_SPDEF or STAT_SPEED - there is no battle stat behind STAT_ACC/STAT_EVASION.
	@ Silent, like encchangestat; unlike a stage change, Haze and switching out don't undo it.
	.macro encchangestatvalue target:req, stat:req, amount:req, mode=ENC_AMOUNT_FIXED
	callnative BS_EncounterChangeStatValue
	.byte \target
	.byte \stat
	.2byte \amount
	.byte \mode
	.endm

	@ Sets how much damage every battler \target resolves to takes, as a percentage reduction:
	@ 70 means it takes 70% less from every source that goes through the damage formula or a
	@ passive HP tick. Replaces the current value; 0 clears it. Max ENC_MAX_DAMAGE_REDUCTION.
	.macro encsetdamagereduction target:req, percent:req
	callnative BS_EncounterSetDamageReduction
	.byte \target
	.byte \percent
	.endm

	@ Replaces the ENC_IMMUNE_* mask on every battler \target resolves to (0 clears it). See
	@ constants/battle_encounter.h for what each bit covers; ENC_IMMUNE_ALL sets every one.
	.macro encsetimmunity target:req, immunities:req
	callnative BS_EncounterSetImmunity
	.byte \target
	.byte \immunities
	.endm

	@ Clamps every battler \target resolves to at 2x incoming type effectiveness (TRUE) or removes
	@ the clamp (FALSE). Unlike encsetimmunity, this never blocks a move - a double weakness still
	@ lands, it just can't stack into a quadruple one.
	.macro encsetcaptypeeffectiveness target:req, cap:req
	callnative BS_EncounterSetCapTypeEffectiveness
	.byte \target
	.byte \cap
	.endm

	@ TRUE stops Toxic's counter from scaling every battler \target resolves to up each turn - it
	@ takes the same flat 1/16 max HP regular Poison would, every turn, instead of that growing
	@ turn over turn. FALSE restores the normal ramp.
	.macro encsetflattoxicdamage target:req, flat:req
	callnative BS_EncounterSetFlatToxicDamage
	.byte \target
	.byte \flat
	.endm

	@ TRUE guards every battler \target resolves to against dropping below 1 HP from anything that
	@ goes through the damage formula or a passive HP tick (weather, Toxic, recoil, confusion). It
	@ does NOT cover fixed-damage moves, Perish Song or Destiny Bond - pair with encsetimmunity.
	@ FALSE removes the guard. For scripted last-stand / rebirth beats and guaranteed catch windows.
	.macro encsetsurvive target:req, survive:req
	callnative BS_EncounterSetSurvive
	.byte \target
	.byte \survive
	.endm

	@ Gives every battler \target resolves to the same protection Protect itself grants, so Feint,
	@ never-miss moves and contact punishment all behave exactly as they do against a real Protect.
	@ For a telegraphed evade/brace turn. Only useful at OnTurnStart: gProtectStructs is cleared
	@ after end-of-turn effects, so a protect set there covers the whole turn and expires on its own.
	.macro encsetprotect target:req
	callnative BS_EncSetProtect
	.byte \target
	.endm

	@ Sets the same escape-prevention volatile Mean Look sets on every battler \target resolves to,
	@ with the boss as the trapper - so switching and fleeing are both refused until a script clears
	@ it again, or the boss faints and the engine's own cleanup releases everything it held.
	@ Ghost-types are exempt (B_GHOSTS_ESCAPE >= GEN_6), the same way they are exempt from Mean Look.
	@ Unlike encsetprotect this is NOT wiped at end of turn: it persists until cleared, which is the
	@ point. The boss is skipped even when \target includes it.
	.macro encsettrapped target:req, trapped:req
	callnative BS_EncSetTrapped
	.byte \target
	.byte \trapped
	.endm

	@ Puts every battler \target resolves to on the engine's Hyper Beam recharge timer, so it loses
	@ an action and prints the stock "must recharge!" line when its move tries to resolve. The
	@ decrement lives at the very end of the turn, AFTER the OnTurnEnd checkpoint, so \turns means
	@ different things depending on where it is set: 1 at OnTurnStart costs the battler THIS turn's
	@ action, while 2 at OnTurnEnd/OnMoveEnd costs it NEXT turn's. For an overexerted boss, a
	@ staggered beat, or any telegraphed free turn.
	.macro encsetrecharge target:req, turns=1
	callnative BS_EncSetRecharge
	.byte \target
	.byte \turns
	.endm

	@ Asks the player a yes/no question mid-battle. Prints \stringId, opens the standard YES/NO
	@ window, and jumps \yesInstr on YES; falls through on NO (and on B, which the box treats as NO).
	@ The leading printstring is what makes this safe at OnTurnStart - it resets gBattle_BG0_X/Y,
	@ scrolling the still-open action menu back out of the way before the window is drawn.
	@ A decided battle skips the prompt entirely and takes the NO path, so scripted damage earlier in
	@ the same script can't leave the player answering a question about a fight that is already over -
	@ the same gBattleOutcome guard vanilla puts in front of its own yes/no box.
	@ Single-player only: a link battle would desync on the input. Encounters are single-player.
	.macro encaskyesno stringId:req, yesInstr:req
	jumpifbyte CMP_NOT_EQUAL, gBattleOutcome, 0, 8f
	printstring \stringId
	setbyte gBattleCommunication, 0
	yesnobox
	jumpifbyte CMP_EQUAL, gBattleCommunication + 1, 0, \yesInstr
8:
	.endm

	@ Strips every screen, Safeguard, Mist, Tailwind and Lucky Chant from the side \target resolves
	@ to, along with the matching gSideTimers entries. Jumps \failInstr when that side had none, so
	@ one command serves as both the test and the clear. Silent - the caller supplies its own
	@ dialogue. Nothing else removes these generically: trydefog only ever clears the side opposite
	@ gBattlerAttacker, which is stale outside a move's own resolution.
	.macro encclearscreens target:req, failInstr:req
	callnative BS_EncClearScreens
	.byte \target
	.4byte \failInstr
	.endm

	@ Strips every entry hazard (Spikes, Toxic Spikes, Stealth Rock, Sticky Web, Steelsurge) from
	@ each side \target resolves to, and jumps \failInstr when there was nothing on any of them - so
	@ one command is both the test and the clear, the same as encclearscreens. Silent: it takes the
	@ side bare in one pass rather than printing Defog's per-hazard lines, so the caller supplies one
	@ line for the whole scouring. Nothing else could remove hazards from a checkpoint script.
	.macro encclearhazards target:req, failInstr:req
	callnative BS_EncClearHazards
	.byte \target
	.4byte \failInstr
	.endm

	@ Raises one side-wide status (an ENC_SIDE_* selector) on every side \target resolves to, for
	@ \turns turns; 0 (the default) is permanent, the same convention encsetweather uses. The
	@ single-status counterpart to encclearscreens, which is all-or-nothing and deliberately skips
	@ the Pledge statuses - this one can hand out a lone Safeguard, a Tailwind phase, a Lucky Chant
	@ "no crits" stance, or raise a Rainbow / Sea of Fire / Swamp no Pledge combo produced. Silent.
	.macro encsetsidestatus target:req, status:req, turns=0
	callnative BS_EncSetSideStatus
	.byte \target
	.byte \status
	.byte \turns
	.byte TRUE
	.endm

	@ Drops one side-wide status from every side \target resolves to, timer included. The precise
	@ inverse of encsetsidestatus, and the only way to remove a Pledge status (encclearscreens does
	@ not touch them) or to strip one screen without the rest. Silent, and a no-op if it wasn't up.
	.macro encclearsidestatus target:req, status:req
	callnative BS_EncSetSideStatus
	.byte \target
	.byte \status
	.byte 0
	.byte FALSE
	.endm

	@ Revives the first fainted party member on every side \target resolves to, at \percent of its
	@ max HP. Bench Pokemon only - a fainted battler still has replacement bookkeeping pending, so
	@ it is skipped rather than pulled back mid-faint. Silent, and a no-op when a side has nobody to
	@ revive, so it is safe to call unconditionally; supply your own dialogue.
	.macro encrevive target:req, percent:req
	callnative BS_EncRevive
	.byte \target
	.byte \percent
	.endm

	@ Whether the player may throw a Poke Ball: ENC_BALLS_DEFAULT, ENC_BALLS_BLOCKED or
	@ ENC_BALLS_ALLOWED. ALLOWED only lifts this encounter's own block - it never makes a battle
	@ catchable that wouldn't be otherwise (trainer battle, Ghost without a Silph Scope, ...).
	.macro encsetballs policy:req
	callnative BS_EncounterSetBallPolicy
	.byte \policy
	.endm

	@ Replaces the catch rate used for this battle's wild Pokemon; ENC_CATCH_RATE_NONE restores
	@ the species' own. Higher is easier to catch, on the same 0-255 scale as species data.
	.macro encsetcatchrate catchRate:req
	callnative BS_EncounterSetCatchRate
	.byte \catchRate
	.endm

	@ Sets the battle weather to \weather (an enum BattleWeather value, e.g. BATTLE_WEATHER_SNOW).
	@ setfieldweather can't do this from an encounter script - it reads the weather type off the
	@ current move, and a checkpoint script has no current move. \turns is how many turns it lasts;
	@ 0 (the default) means permanent, the same convention primal weather uses. Silent, like the
	@ other enc* commands; pair with playanimation for presentation. Does nothing if that weather is
	@ already up or primal weather is in force - losing a weather race is a legitimate outcome.
	@ Use the existing removeweather to clear it again.
	.macro encsetweather weather:req, turns=0
	callnative BS_EncSetWeather
	.byte \weather
	.byte \turns
	.endm

	@ Sets the field terrain to \terrain (an ENC_TERRAIN_* selector) for \turns turns; 0 (the
	@ default) means permanent. The mirror of encsetweather one field over, and needed for the same
	@ reason: the stock setterrain opcode reads its terrain type off the current move, and a
	@ checkpoint script has no current move. Does nothing if that terrain is already up or the battle
	@ is a sky battle. Silent - pair with playmoveanimation for presentation, and use the existing
	@ removeterrain to clear it.
	.macro encsetterrain terrain:req, turns=0
	callnative BS_EncSetTerrain
	.byte \terrain
	.byte \turns
	.endm

	@ Shorts out the held item of every battler \target resolves to for \turns turns, by setting the
	@ same volatile Embargo itself sets - so every existing item check behaves unchanged, and the
	@ engine's own end-turn handler ticks the timer and prints the expiry line. \turns of 0 clears it
	@ outright. The boss is skipped even when \target includes it, the same way encsettrapped skips
	@ it. Silent; supply your own dialogue.
	.macro encsetembargo target:req, turns:req
	callnative BS_EncSetEmbargo
	.byte \target
	.byte \turns
	.endm

	@ Disables the move \target (must resolve to exactly one battler) used last, for \turns turns,
	@ writing the same volatiles Disable does - so the engine already ticks the timer, already drops
	@ the lock if the move leaves the moveset, and already prints its own "no longer disabled" line.
	@ The move's name is buffered into {B_BUFF1} for the caller's string.
	@ Jumps \failInstr when there is nothing to take: no move used yet (turn 1), the move is gone
	@ from the moveset or out of PP, something is already disabled, or the target is not there. All of
	@ those are ordinary battle states, so they branch rather than assert. Silent otherwise.
	.macro encdisablemove target:req, turns:req, failInstr:req
	callnative BS_EncDisableMove
	.byte \target
	.byte \turns
	.4byte \failInstr
	.endm

	@ Forces \target (must resolve to exactly one battler) to Mega Evolve outside the normal
	@ gimmick-selection flow, then plays the same presentation BattleScript_MegaEvolution
	@ (data/battle_scripts_1.s) uses. Jumps to \failInstr if the target can't mega evolve right now
	@ (wrong shape, fainted, already mega evolved, or another gimmick active).
	.macro encmegaevolve target:req, failInstr:req
	callnative BS_EncounterMegaEvolve
	.byte \target
	.4byte \failInstr
	handleformchange BS_SCRIPTING, 0
	playanimation BS_SCRIPTING, B_ANIM_MEGA_EVOLUTION
	waitanimation
	handleformchange BS_SCRIPTING, 1
	printstring STRINGID_MEGAEVOEVOLVED
	waitmessage B_WAIT_TIME_LONG
	switchinabilities BS_SCRIPTING
	.endm

	@ Changes \target (must resolve to exactly one battler) into \species outright, outside any
	@ form-change table, then plays \anim with the same presentation opcodes
	@ BattleScript_MegaEvolution uses. The general form of encmegaevolve: the destination is named
	@ here instead of coming from a held Mega Stone, so a boss can change shape repeatedly, into a
	@ form it has no stone for, and back again. Stats, types and ability come from the new species;
	@ HP and the moveset are kept. Prints nothing - the caller supplies its own dialogue, since no
	@ one string fits both an arrival and a revert. Jumps to \failInstr if the target does not
	@ resolve to exactly one live battler.
	@ Leads with flushtextbox because this animates before the caller's dialogue: the action/move
	@ selection menu is shown by scrolling BG0 (gBattle_BG0_Y, battle_controller_player.c) and only
	@ a printstring scrolls it back (BtlController_HandlePrintString). A script dispatched at
	@ OnTurnStart runs while that scroll is still in place, so animating first would render over the
	@ open menu. BattleScript_QuickClawActivation opens the same way for the same reason.
	.macro encformchange target:req, species:req, failInstr:req, anim=B_ANIM_MEGA_EVOLUTION
	callnative BS_EncounterFormChange
	.byte \target
	.2byte \species
	.4byte \failInstr
	flushtextbox
	handleformchange BS_SCRIPTING, 0
	playanimation BS_SCRIPTING, \anim
	waitanimation
	handleformchange BS_SCRIPTING, 1
	switchinabilities BS_SCRIPTING
	.endm

	@ Writes \move into slot \slot (0-3) of \target's BATTLE mon, with that move's full PP. \target
	@ must resolve to exactly one live battler. The party Pokemon is never touched, so a boss caught
	@ afterwards keeps the moveset it was built with.
	@ Moves: is a battle-start property, so this is the only way a boss gains a move partway through
	@ a fight - the signature move a form unlocks when it transforms. Silent; the caller supplies its
	@ own dialogue.
	.macro encsetmove target:req, slot:req, move:req
	callnative BS_EncSetMove
	.byte \target
	.byte \slot
	.2byte \move
	.endm

	@ Turns \target (must resolve to exactly one live battler) into \source (likewise) outright:
	@ species, stats, stat stages, types, ability and moveset are copied, exactly as the Transform
	@ move does. \target keeps its own HP, level, held item and status, and its party Pokemon is
	@ never written - so a boss transformed this way is still its own species if caught. Re-copying
	@ preserves the first original species so encuntransform can revert. Jumps \failInstr if \source
	@ is semi-invulnerable, is itself transformed, or is behind Illusion. Prints nothing - the caller
	@ supplies dialogue. Leads with flushtextbox for the same OnTurnStart BG0 reason as encformchange.
	@ Uses B_ANIM_ENCOUNTER_TRANSFORM, not the encformchange animation pair - that one repaints from
	@ the party Pokemon and so requires writing it.
	.macro enctransform target:req, source:req, failInstr:req
	callnative BS_EncounterTransform
	.byte \target
	.byte \source
	.4byte \failInstr
	flushtextbox
	playanimation BS_SCRIPTING, B_ANIM_ENCOUNTER_TRANSFORM
	waitanimation
	handleformchange BS_SCRIPTING, 1
	.endm

	@ Reverts \target (must resolve to exactly one battler) from an enctransform/Transform copy back
	@ to its own party species, rebuilding stats, types, ability and moves from the party Pokemon.
	@ Silent no-op if \target isn't transformed, so it can be called unconditionally. Stat stages are
	@ reset to neutral. Prints nothing - the caller supplies dialogue. Shares enctransform's animation,
	@ which repaints from the battle mon and so needs no party write.
	.macro encuntransform target:req
	callnative BS_EncounterUntransform
	.byte \target
	flushtextbox
	playanimation BS_SCRIPTING, B_ANIM_ENCOUNTER_TRANSFORM
	waitanimation
	handleformchange BS_SCRIPTING, 1
	.endm

	@ Branches to \jumpInstr with \percent probability (0-100), otherwise falls through. The roll is
	@ tagged RNG_ENCOUNTER_SCRIPT, so it is seedable/observable like every other tagged battle roll.
	.macro encjumpifchance percent:req, jumpInstr:req
	callnative BS_EncJumpIfChance
	.byte \percent
	.4byte \jumpInstr
	.endm

	.macro savetarget
	callnative BS_SaveTarget
	.endm

	.macro restoretarget
	callnative BS_RestoreTarget
	.endm

	.macro saveattacker
	callnative BS_SaveAttacker
	.endm

	.macro restoreattacker
	callnative BS_RestoreAttacker
	.endm

	.macro allyswitchswapbattlers
	callnative BS_AllySwitchSwapBattler
	.endm

	.macro tryallyswitch jumpInstr:req
	callnative BS_TryAllySwitch
	.4byte \jumpInstr
	.endm

	.macro jumpifmorethanhalfHP battler:req, jumpInstr:req
	callnative BS_JumpIfMoreThanHalfHP
	.byte \battler
	.4byte \jumpInstr
	.endm

	.macro itemrestorehp jumpInstr:req, restoreBattlerInstr:req
	callnative BS_ItemRestoreHP
	.4byte \jumpInstr
	.4byte \restoreBattlerInstr
	.endm

	.macro itemcurestatus jumpInstr:req, restoreBattlerInstr:req
	callnative BS_ItemCureStatus
	.4byte \jumpInstr
	.4byte \restoreBattlerInstr
	.endm

	.macro itemincreasestat
	callnative BS_ItemIncreaseStat
	.endm

	.macro itemcornnberry
	callnative BS_ItemCornnBerry
	.endm

	.macro itemmagostberry
	callnative BS_ItemMagostBerry
	.endm

	.macro itemrabutaberry
	callnative BS_ItemRabutaBerry
	.endm

	.macro itemrazzberry
	callnative BS_ItemRazzBerry
	.endm

	.macro itembelueberry
	callnative BS_ItemBelueBerry
	.endm

	.macro itemrestorepp
	callnative BS_ItemRestorePP
	.endm

	.macro tryrevertweatherform
	callnative BS_TryRevertWeatherForm
	.endm

	.macro trysetoctolock failInstr:req
	callnative BS_TrySetOctolock
	.4byte \failInstr
	.endm

	.macro setzeffect
	callnative BS_SetZEffect
	.endm

	@ Used by effects that may proc Symbiosis but do not call removeitem.
	.macro trysymbiosis battler:req
	callnative BS_TrySymbiosis
	.byte \battler
	.endm

	@ returns B_SIDE_x to gBattleCommunication[0]
	.macro getbattlerside battler:req
	callnative BS_GetBattlerSide
	.byte \battler
	.endm

	.macro checkparentalbondcounter counter:req, ptr:req
	callnative BS_CheckParentalBondCounter
	.byte \counter
	.4byte \ptr
	.endm

	.macro jumpifcantloseitem ptr:req
	callnative BS_JumpIfCantLoseItem
	.4byte \ptr
	.endm

	.macro setterrain failInstr:req
	callnative BS_SetTerrain
	.4byte \failInstr
	.endm

	.macro jumpifterrainaffected battler:req, terrainFlags:req, jumpInstr:req
	callnative BS_JumpIfTerrainAffected
	.byte \battler
	.byte \terrainFlags
	.4byte \jumpInstr
	.endm

	.macro tryreflecttype failInstr:req
	callnative BS_TryReflectType
	.4byte \failInstr
	.endm

	@ Used to active a different Max Move effects.
	.macro setmaxmoveeffect
	callnative BS_SetMaxMoveEffect
	.endm

	.macro setsteelsurge, failInstr:req
	callnative BS_SetSteelsurge
	.4byte \failInstr
	.endm

	.macro trysetparalysis, ptr:req
	callnative BS_TrySetParalysis
	.4byte \ptr
	.endm

	.macro trysetpoison, ptr:req
	callnative BS_TrySetPoison
	.4byte \ptr
	.endm

	.macro trysetpoisonparalysis, ptr:req
	callnative BS_TrySetPoisonParalyzis
	.4byte \ptr
	.endm

	.macro tryseteffectspore, ptr:req
	callnative BS_TrySetEffectSpore
	.4byte \ptr
	.endm

	.macro trysetconfusion, ptr:req
	callnative BS_TrySetConfusion
	.4byte \ptr
	.endm

	.macro trysetinfatuation, ptr:req
	callnative BS_TrySetInfatuation
	.4byte \ptr
	.endm

	.macro trysetescapeprevention, ptr:req
	callnative BS_TrySetEscapePrevention
	.4byte \ptr
	.endm

	.macro trysettorment, ptr:req
	callnative BS_TrySetTorment
	.4byte \ptr
	.endm

	.macro tryhealsixthhealth, ptr:req
	callnative BS_HealOneSixth
	.4byte \ptr
	.endm

	.macro tryrecycleberry, ptr:req
	callnative BS_TryRecycleBerry
	.4byte \ptr
	.endm

	.macro updatedynamax
	callnative BS_UpdateDynamax
	.endm

	.macro jumpiftargetdynamaxed, ptr:req
	callnative BS_JumpIfDynamaxed
	.4byte \ptr
	.endm

	.macro undodynamax battler:req
	callnative BS_UndoDynamax
	.byte \battler
	.endm

	.macro trytrainerslidezmovemsg
	callnative BS_TryTrainerSlideZMoveMsg
	.endm

	.macro trytrainerslidemegaevolutionmsg
	callnative BS_TryTrainerSlideMegaEvolutionMsg
	.endm

	.macro trytrainerslidedynamaxmsg
	callnative BS_TryTrainerSlideDynamaxMsg
	.endm

	.macro trytrainerslideteramsg
	callnative BS_TryTrainerSlideTeraMsg
	.endm

	.macro tryhealpulse failInstr:req
	callnative BS_TryHealPulse
	.4byte \failInstr
	.endm

	.macro trydefog clear:req, failInstr:req
	callnative BS_TryDefog
	.byte \clear
	.4byte \failInstr
	.endm

	.macro trytriggerstatusform
	callnative BS_TryTriggerStatusForm
	.endm

	.macro tryupperhand failInstr:req
	callnative BS_TryUpperHand
	.4byte \failInstr
	.endm

	.macro trytidyup clear:req, jumpInstr:req
	callnative BS_TryTidyUp
	.byte \clear
	.4byte \jumpInstr
	.endm

	.macro tryquash failInstr:req
	callnative BS_TryQuash
	.4byte \failInstr
	.endm

	.macro removeweather
	callnative BS_RemoveWeather
	.endm

	.macro applyterastallization
	callnative BS_ApplyTerastallization
	.endm

	.macro jumpifsleepclause jumpInstr:req
	callnative BS_JumpIfSleepClause
	.4byte \jumpInstr
	.endm

	.macro jumpifnoberry battler:req, jumpInstr:req
	callnative BS_JumpIfNoBerry
	.byte \battler
	.4byte \jumpInstr
	.endm

	.macro checkteatimetargets failInstr:req
	callnative BS_CheckTeaTimeTargets
	.4byte \failInstr
	.endm

	.macro trywindriderpower battler:req, failInstr:req
	callnative BS_TryWindRiderPower
	.byte \battler
	.4byte \failInstr
	.endm

	.macro activateweatherchangeabilities battler:req
	callnative BS_ActivateWeatherChangeAbilities
	.byte \battler
	.endm

	.macro activateterrainchangeabilities battler:req
	callnative BS_ActivateTerrainChangeAbilities
	.byte \battler
	.endm

	.macro resetterrainabilityflags
	callnative BS_ResetTerrainAbilityFlags
	.endm

	@ Stores Healing Wish effect.
	.macro storehealingwish battler:req
	callnative BS_StoreHealingWish
	.byte \battler
	.endm

	.macro tryrevivalblessing, failInstr:req
	callnative BS_TryRevivalBlessing
	.4byte \failInstr
	.endm

	.macro jumpifblockedbysoundproof battler:req, failInstr:req
	callnative BS_JumpIfBlockedBySoundproof
	.byte \battler
	.4byte \failInstr
	.endm

	.macro jumpifcommanderactive battler:req, jumpInstr:req
	callnative BS_JumpIfCommanderActive
	.byte \battler
	.4byte \jumpInstr
	.endm

	.macro checkpokeflute
	callnative BS_CheckPokeFlute
	.endm

	.macro waitfanfare
	callnative BS_WaitFanfare
	.endm

	.macro removeterrain
	callnative BS_RemoveTerrain
	.endm

	.macro setmoveresultflags flags:req
	callnative BS_SetMoveResultFlags
	.4byte \flags
	.endm

	.macro clearspecialstatuses
	callnative BS_ClearSpecialStatuses
	.endm

	.macro clearmoveresultflags flags:req
	callnative BS_ClearMoveResultFlags
	.4byte \flags
	.endm

	.macro jumpifmoveresultflags flags:req failInstr:req
	callnative BS_JumpIfMoveResultFlags
	.4byte \flags
	.4byte \failInstr
	.endm

	.macro jumpifnotcriticalhit failInstr:req
	callnative BS_JumpIfNotCriticalHit
	.4byte \failInstr
	.endm

	.macro jumpifcangigantamax battler:req, jumpInstr:req
	callnative BS_JumpIfCanGigantamax
	.byte \battler
	.4byte \jumpInstr
	.endm

	.macro swapsidestatuses
	callnative BS_CourtChangeSwapSideStatuses
	.endm

	.macro swapstats stat:req
	callnative BS_SwapStats
	.byte \stat
	.endm

	.macro restoresavedmove
	callnative BS_RestoreSavedMove
	.endm

    .macro trydefiantrattled
    callnative BS_TryDefiantRattled
    .endm

    .macro tryadrenalineorb
    callnative BS_TryAdrenalineOrb
    .endm

    .macro raisecritstatchangeanim battler:req
    callnative BS_RaiseCritStatChangeAnim
    .byte \battler
    .endm

    .macro restorestatchangequeue
    callnative BS_RestoreStatChangeQueue
    .endm

@ helpful macros
	.macro setmoveeffect effect:req
	sethword sMOVE_EFFECT, \effect
	.endm

	.macro sethword dst:req, value:req
	setbyte \dst, (\value) & 0xFF
	setbyte \dst + 1, ((\value) >> 8) & 0xFF
	.endm

	.macro setword dst:req, value:req
	setbyte \dst, (\value) & 0xFF
	setbyte \dst + 1, ((\value) >> 8) & 0xFF
	setbyte \dst + 2, ((\value) >> 16) & 0xFF
	setbyte \dst + 3, ((\value) >> 24) & 0xFF
	.endm

	.macro copybyte dst:req, src:req
	copyarray \dst, \src, 0x1
	.endm

	.macro copyhword dst:req, src:req
	copyarray \dst, \src, 0x2
	.endm

	.macro copyword dst:req, src:req
	copyarray \dst, \src, 0x4
	.endm

	.macro jumpifbytenotequal byte1:req, byte2:req, jumpInstr:req
	jumpifarraynotequal \byte1, \byte2, 0x1, \jumpInstr
	.endm

	.macro jumpifbyteequal byte1:req, byte2:req, jumpInstr:req
	jumpifarrayequal \byte1, \byte2, 0x1, \jumpInstr
	.endm

	.macro jumpifmove move:req, jumpInstr:req
	jumpifhalfword CMP_EQUAL, gCurrentMove, \move, \jumpInstr
	.endm

	.macro jumpifnotmove move:req, jumpInstr:req
	jumpifhalfword CMP_NOT_EQUAL, gCurrentMove, \move, \jumpInstr
	.endm

	.macro jumpifnotchosenmove move:req, jumpInstr:req
	jumpifhalfword CMP_NOT_EQUAL, gChosenMove, \move, \jumpInstr
	.endm

	.macro jumpifmovehadnoeffect jumpInstr:req
	jumpifmoveresultflags MOVE_RESULT_NO_EFFECT, \jumpInstr
	.endm

	.macro jumpifside battler:req, side:req, equalJumpInstr:req
	getbattlerside \battler
	jumpifbyte CMP_EQUAL, gBattleCommunication, \side, \equalJumpInstr
	.endm

	.macro jumpifbattletype flags:req, jumpInstr:req
	jumpifword CMP_COMMON_BITS, gBattleTypeFlags, \flags, \jumpInstr
	.endm

	.macro jumpifnotbattletype flags:req, jumpInstr:req
	jumpifword CMP_NO_COMMON_BITS, gBattleTypeFlags, \flags, \jumpInstr
	.endm

	.macro dmg_1_8_targethp
	manipulatedamage DMG_1_8_TARGET_HP
	.endm

	.macro jumpifflowerveil jumpInstr:req
	jumpifnottype BS_TARGET, TYPE_GRASS, 1f
	jumpifability BS_TARGET_SIDE, ABILITY_FLOWER_VEIL, \jumpInstr
	1:
	.endm

	.macro jumpifflowerveilattacker jumpInstr:req
	jumpifnottype BS_ATTACKER, TYPE_GRASS, 1f
	jumpifability BS_ATTACKER_SIDE, ABILITY_FLOWER_VEIL, \jumpInstr
	1:
	.endm

	.macro jumpifabilitycantbereactivated battler:req, jumpInstr:req
	callnative BS_JumpIfAbilityCantBeReactivated
	.byte \battler
	.4byte \jumpInstr
	.endm

	.macro tryactivateabilityshield battler:req
	callnative BS_TryActivateAbilityShield
	.byte \battler
	.endm

	.macro jumpifroarfails jumpInstr:req
	callnative BS_JumpIfRoarFails
	.4byte \jumpInstr
	.endm

	.macro jumpifabsent battler:req, jumpInstr:req
	callnative BS_JumpIfAbsent
	.byte \battler
	.4byte \jumpInstr
	.endm

	.macro jumpifnoholdeffect battler:req, holdEffect:req, jumpInstr:req
	jumpifholdeffect \battler, \holdEffect, \jumpInstr, FALSE
	.endm

	.macro jumpifholdeffect battler:req, holdEffect:req, jumpInstr:req, equal:req
	callnative BS_JumpIfHoldEffect
	.byte \battler
	.byte \holdEffect
	.4byte \jumpInstr
	.byte \equal
	.endm

	.macro jumpifnoally battler:req, jumpInstr:req
	callnative BS_JumpIfNoAlly
	.byte \battler
	.4byte \jumpInstr
	.endm

	.macro setlastuseditem battler:req
	callnative BS_SetLastUsedItem
	.byte \battler
	.endm

	.macro trysetfairylock failInstr:req
	callnative BS_TrySetFairyLock
	.4byte \failInstr
	.endm

	.macro jumpiffullhp battler:req, jumpInstr:req
	callnative BS_JumpIfFullHp
	.byte \battler
	.4byte \jumpInstr
	.endm

	.macro tryfriskmessage
	callnative BS_TryFriskMessage
	.endm

	.macro settracedability battler:req
	callnative BS_SetTracedAbility
	.byte \battler
	.endm

	.macro tryillusionoff battler:req
	callnative BS_TryIllusionOff
	.byte \battler
	.endm

	.macro updatenick
	callnative BS_UpdateNick
	.endm

	.macro gravityonairbornemons
	callnative BS_GravityOnAirborneMons
	.endm

	.macro cancelmultiturnmoves
	callnative BS_CancelMultiTurnMoves
	.endm

	.macro isrunningimpossible
	callnative BS_IsRunningImpossible
	.endm

	.macro getmovetarget
	callnative BS_GetMoveTarget
	.endm

	@ Will jump to script pointer if the specified battler has or has not fainted.
	.macro jumpiffainted battler:req, value:req, ptr:req
	getbattlerfainted \battler
	jumpifbyte CMP_EQUAL, gBattleCommunication, \value, \ptr
	.endm

	.macro getbattlerfainted battler:req
	callnative BS_GetBattlerFainted
	.byte \battler
	.endm

	.macro resetswitchinabilitybits
	callnative BS_ResetSwitchInAbilityBits
	.endm

	.macro updatechoicemoveonlvlup
	callnative BS_UpdateChoiceMoveOnLvlUp
	.endm

	.macro resetplayerfainted
	callnative BS_ResetPlayerFainted
	.endm

	.macro palaceflavortext
	callnative BS_PalaceFlavorText
	.endm

	.macro arenajudgmentwindow
	callnative BS_ArenaJudgmentWindow
	.endm

	.macro arenaopponentmonlost
	callnative BS_ArenaOpponentMonLost
	.endm

	.macro arenaplayermonlost
	callnative BS_ArenaPlayerMonLost
	.endm

	.macro arenabothmonslost
	callnative BS_ArenaBothMonsLost
	.endm

	.macro forfeityesnobox
	callnative BS_ForfeitYesNoBox
	.endm

	.macro drawarenareftextbox
	callnative BS_DrawArenaRefTextBox
	.endm

	.macro erasearenareftextbox
	callnative BS_EraseArenaRefTextBox
	.endm

	.macro arenajudgmentstring id:req
	callnative BS_ArenaJudgmentString
	.byte \id
	.endm

	.macro arenawaitmessage
	callnative BS_ArenaWaitMessage
	.endm

	.macro waitcry
	callnative BS_WaitCry
	.endm

	.macro returnopponentmon1toball
	callnative BS_ReturnOpponentMon1ToBall
	.endm

	.macro returnopponentmon2toball
	callnative BS_ReturnOpponentMon2ToBall
	.endm

	.macro volumedown
	callnative BS_VolumeDown
	.endm

	.macro volumeup
	callnative BS_VolumeUp
	.endm

	.macro setalreadystatusedmoveattempt
	callnative BS_SetAlreadyStatusedMoveAttempt
	.endm

	.macro palacetryescapestatus
	callnative BS_PalaceTryEscapeStatus
	.endm

	.macro setteleportoutcome battler:req
	callnative BS_SetTeleportOutcome
	.byte \battler
	.endm

	.macro playtrainerdefeatedmusic
	callnative BS_PlayTrainerDefeatedMusic
	.endm

	.macro stattextbuffer
	callnative BS_StatTextBuffer
	.endm

	.macro switchinabilities battler:req
	callnative BS_SwitchinAbilities
	.byte \battler
	.endm

	.macro effectsafterformchange
	callnative BS_EffectsAfterFormChange
	.endm

	.macro abilityonformchange battler:req
	callnative BS_AbilityOnFormChange
	.byte \battler
	.endm

	.macro instanthpdrop
	callnative BS_InstantHpDrop
	.endm

	.macro clearstatus battler:req
	callnative BS_ClearStatus
	.byte \battler
	.endm

	.macro restoremovepp battler:req
	callnative BS_RestoreMovePp
	.byte \battler
	.endm

	.macro tryactivatereceiver battler:req
	callnative BS_TryActivateReceiver
	.byte \battler
	.endm

	.macro tryactivatesoulheart
	callnative BS_TryActivateSoulheart
	.endm

	.macro playmoveanimation move:req
	callnative BS_PlayMoveAnimation
	.2byte \move
	.endm

	.macro setluckychant failInstr:req
	callnative BS_SetLuckyChant
	.4byte \failInstr
	.endm

	.macro tryentrainment failInstr:req
	callnative BS_TryEntrainment
	.4byte \failInstr
	.endm

	.macro setlastusedability
	callnative BS_SetLastUsedAbility
	.endm

	.macro invertstatstages
	callnative BS_InvertStatStages
	.endm

	.macro tryelectrify failInstr:req
	callnative BS_TryElectrify
	.4byte \failInstr
	.endm

	.macro trysoak failInstr:req
	callnative BS_TrySoak
	.4byte \failInstr
	.endm

	.macro handleformchange battler:req, caseId:req, bufferSpeciesName=TRUE
	callnative BS_HandleFormChange
	.byte \battler
	.byte \caseId
	.byte \bufferSpeciesName
	.endm

	.macro tryinstruct failInstr:req
	callnative BS_TryInstruct
	.4byte \failInstr
	.endm

	.macro showabilitypopup
	callnative BS_ShowAbilityPopup
	.endm

	.macro updateabilitypopup
	callnative BS_UpdateAbilityPopup
	.endm

	.macro waitabilitypopup
	callnative BS_WaitAbilityPopup
	.endm

	.macro jumpiftargetally jumpInstr:req
	callnative BS_JumpIfTargetAlly
	.4byte \jumpInstr
	.endm

	.macro trypsychoshift failInstr:req sleepClauseFailInstr:req
	callnative BS_TryPsychoShift
	.4byte \failInstr
	.4byte \sleepClauseFailInstr
	.endm

	.macro curestatus battler:req
	callnative BS_CureStatus
	.byte \battler
	.endm

	.macro powertrick
	callnative BS_PowerTrick
	.endm

	.macro tryafteryou failInstr:req
	callnative BS_TryAfterYou
	.4byte \failInstr
	.endm

	.macro trybestow failInstr:req
	callnative BS_TryBestow
	.4byte \failInstr
	.endm

	.macro handletrainerslidemsg battler:req, case_:req
	callnative BS_HandleTrainerSlideMsg
	.byte \battler
	.byte \case_
	.endm

	.macro trytrainerslidemsgfirstoff battler:req
	callnative BS_TryTrainerSlideMsgFirstOff
	.byte \battler
	.endm

	.macro trytrainerslidemsglaston battler:req
	callnative BS_TryTrainerSlideMsgLastOn
	.byte \battler
	.endm

	.macro setauroraveil
	callnative BS_SetAuroraVeil
	.endm

	.macro trythirdtype failInstr:req
	callnative BS_TryThirdType
	.4byte \failInstr
	.endm

	.macro destroyabilitypopup
	callnative BS_DestroyAbilityPopup
	.endm

	.macro activateitemeffects
	callnative BS_ActivateItemEffects
	.endm

	.macro tryroomservice battler:req, failInstr:req
	callnative BS_TryRoomService
	.byte \battler
	.4byte \failInstr
	.endm

	.macro tryterrainseed battler:req, failInstr:req
	callnative BS_TryTerrainSeed
	.byte \battler
	.4byte \failInstr
	.endm

	.macro makeinvisible battler:req
	callnative BS_MakeInvisible
	.byte \battler
	.endm

	.macro jumpifteamhealthy jumpInstr:req
	callnative BS_JumpIfTeamHealthy
	.4byte \jumpInstr
	.endm

	.macro tryhealquarterhealth battler:req, failInstr:req
	callnative BS_TryHealQuarterHealth
	.byte \battler
	.4byte \failInstr
	.endm

	.macro jumpifpranksterblocked jumpInstr:req
	callnative BS_JumpIfPranksterBlocked
	.4byte \jumpInstr
	.endm

	.macro trytoclearprimalweather
	callnative BS_TryToClearPrimalWeather
	.endm

	.macro tryendneutralizinggas
	callnative BS_TryEndNeutralizingGas
	.endm

	.macro consumeberry battler:req, fromBattler:req
	callnative BS_ConsumeBerry
	.byte \battler
	.byte \fromBattler
	.endm

	.macro jumpifweatheraffected flags:req, jumpInstr:req
	callnative BS_JumpIfWeatherAffected
	.2byte \flags
	.4byte \jumpInstr
	.endm

	.macro jumpifspecies species:req, jumpInstr:req
	callnative BS_JumpIfSpecies
	.2byte \species
	.4byte \jumpInstr
	.endm

	.macro jumpifabilitypreventsrest battler:req, jumpInstr:req
	callnative BS_JumpIfAbilityPreventsRest
	.byte \battler
	.4byte \jumpInstr
	.endm

	.macro cutonethirdhpandraisestats failInstr:req
	callnative BS_CutOneThirdHpAndRaiseStats
	.4byte \failInstr
	.endm

	.macro tryresetnegativestatstages
	callnative BS_TryResetNegativeStatStages
	.endm

	.macro savebattleritem
	callnative BS_SaveBattlerItem
	.endm

	.macro restorebattleritem
	callnative BS_RestoreBattlerItem
	.endm

	.macro battleritemtolastuseditem
	callnative BS_BattlerItemToLastUsedItem
	.endm

	.macro setallytonexttarget jumpInstr:req
	jumpifbyte CMP_GREATER_THAN, gBattlerTarget, 0x1, 1f
	addbyte gBattlerTarget, 0x2
	goto \jumpInstr
	1:
	subbyte gBattlerTarget, 0x2
	goto \jumpInstr
	.endm

	.macro setallytonextattacker jumpInstr:req
	jumpifbyte CMP_GREATER_THAN, gBattlerAttacker, 0x1, 1f
	addbyte gBattlerAttacker, 0x2
	goto \jumpInstr
	1:
	subbyte gBattlerAttacker, 0x2
	goto \jumpInstr
	.endm

	.macro jumpifsafeguard jumpInstr:req
	jumpiftargetally 2f
	jumpifability BS_ATTACKER, ABILITY_INFILTRATOR, 1f
	2:
	jumpifsideaffecting BS_TARGET, SIDE_STATUS_SAFEGUARD, \jumpInstr
	1:
	.endm

	.macro flushtextbox
	printstring STRINGID_EMPTYSTRING3
	waitmessage 1
	.endm

	.macro jumpifnowhiteout jumpInstr:req
	callnative BS_JumpIfNoWhiteOut
	.4byte \jumpInstr
	.endm

	.macro tryboosterenergy onFieldStatus:req
	callnative BS_TryBoosterEnergy
	.byte \onFieldStatus
	.endm

	.macro jumpifgenconfiglowerthan tag:req, gen:req, jumpInstr:req
	callnative BS_JumpIfGenConfigLowerThan
	.2byte \tag
	.byte \gen
	.4byte \jumpInstr
	.endm

	.macro tryactivateabilitywithabilityshield battler:req, switchedItems=FALSE
	callnative BS_TryActivateAbilityWithAbilityShield
	.byte \battler
	.byte \switchedItems
	.endm

	.macro endturnevents
	callnative BS_EndTurnEvents
	.endm

	.macro trywakebattlersuproar
	callnative BS_TryWakeBattlersUproar
	.endm

	.macro catchaftervictory
	callnative BS_CatchAfterVictory
	.endm

	.macro jumpifnoballs jumpInstr:req
	callnative BS_JumpIfNoBalls
	.4byte \jumpInstr
	.endm

	.macro handlefailedvictorycatch
	callnative BS_HandleFailedVictoryCatch
	.endm

	.macro catchornot
	callnative BS_CatchOrNot
	.endm

	.macro showitempopup battler:req
	callnative BS_ShowItemPopup
	.byte \battler
	.endm

	.macro destroyitempopup
	callnative BS_DestroyItemPopup
	.endm

	.macro multihitplurality
	callnative BS_MultiHitPlurality
	.endm

	.macro jumpifterrain comparison:req, terrain:req, jumpInstr:req
	.byte B_SCR_OP_JUMPIFTERRAIN
	.byte \comparison
	.byte \terrain
	.4byte \jumpInstr
	.endm

	.macro trydomoveeffectsbeforemoves
	callnative BS_TryDoMoveEffectsBeforeMoves
	.endm
