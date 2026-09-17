#include "global.h"
#include "battle.h"
#include "battle_anim.h"
#include "battle_ai_record.h"
#include "battle_controllers.h"
#include "battle_message.h"
#include "battle_setup.h"
#include "battle_special.h"
#include "battle_z_move.h"
#include "data.h"
#include "event_data.h"
#include "frontier_util.h"
#include "graphics.h"
#include "international_string_util.h"
#include "item.h"
#include "link.h"
#include "menu.h"
#include "palette.h"
#include "random.h"
#include "recorded_battle.h"
#include "string_util.h"
#include "strings.h"
#include "test_runner.h"
#include "text.h"
#include "trainer_hill.h"
#include "trainer_slide.h"
#include "trainer_tower.h"
#include "window.h"
#include "line_break.h"
#include "constants/abilities.h"
#include "constants/battle_dome.h"
#include "constants/battle_string_ids.h"
#include "constants/frontier_util.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/opponents.h"
#include "constants/species.h"
#include "constants/trainers.h"
#include "constants/trainer_hill.h"
#include "constants/weather.h"

struct BattleWindowText
{
    u8 fillValue;
    u8 fontId;
    u8 x;
    u8 y;
    union {
        struct {
            DEPRECATED("Use color.background instead") u8 bgColor;
            DEPRECATED("Use color.foreground instead") u8 fgColor;
            DEPRECATED("Use color.shadow instead") u8 shadowColor;
            DEPRECATED("Use color.accent instead") u8 accentColor;
        };
        union TextColor color;
    };
    u8 letterSpacing;
    u8 lineSpacing;
    u8 speed;
};

#if TESTING
EWRAM_DATA u16 sBattlerAbilities[MAX_BATTLERS_COUNT] = {0};
#else
static EWRAM_DATA u16 sBattlerAbilities[MAX_BATTLERS_COUNT] = {0};
#endif
EWRAM_DATA struct BattleMsgData *gBattleMsgDataPtr = NULL;

// todo: make some of those names less vague: attacker/target vs pkmn, etc.

static const u8 sText_EmptyString4[] = _("");

const u8 gText_PkmnShroudedInMist[] = _("{B_ATK_NAME_WITH_PREFIX} surrounds itself with a protective mist!");
const u8 gText_PkmnGettingPumped[] = _("{B_DEF_NAME_WITH_PREFIX} is getting pumped!");
const u8 gText_PkmnsXPreventsSwitching[] = _("{B_BUFF1} is preventing switching out with its {B_LAST_ABILITY} Ability!\p");
const u8 gText_StatSharply[] = _(" sharply");
const u8 gText_StatRose[] = _("rose!");
const u8 gText_StatFell[] = _("fell!");
const u8 gText_DefendersStatRose[] = _("{B_DEF_NAME_WITH_PREFIX}'s {B_BUFF1} rose{B_BUFF2}!");
const u8 gText_NuzlockeNoCatch[] = _("You can't catch more than one Pokémon\nper area in Nuzlocke mode!");
const u8 gText_DraftNoCatch[] = _("You can't catch Pokémon in a\nDraft run!");
const u8 gText_MonoTypeNoCatch[] = _("Only {B_BUFF1}-type Pokémon can be\ncaught in this game!");
const u8 gText_MonoGenNoCatch[] = _("Only Gen {B_BUFF1} Pokémon can be\ncaught in this game!");
static const u8 sText_GotAwaySafely[] = _("{PLAY_SE SE_FLEE}You got away safely!\p");
static const u8 sText_PlayerDefeatedLinkTrainer[] = _("You defeated {B_LINK_OPPONENT1_NAME}!");
static const u8 sText_TwoLinkTrainersDefeated[] = _("You defeated {B_LINK_OPPONENT1_NAME} and {B_LINK_OPPONENT2_NAME}!");
static const u8 sText_PlayerLostAgainstLinkTrainer[] = _("You lost against {B_LINK_OPPONENT1_NAME}!");
static const u8 sText_PlayerLostToTwo[] = _("You lost to {B_LINK_OPPONENT1_NAME} and {B_LINK_OPPONENT2_NAME}!");
static const u8 sText_PlayerBattledToDrawLinkTrainer[] = _("You battled to a draw against {B_LINK_OPPONENT1_NAME}!");
static const u8 sText_PlayerBattledToDrawVsTwo[] = _("You battled to a draw against {B_LINK_OPPONENT1_NAME} and {B_LINK_OPPONENT2_NAME}!");
static const u8 sText_WildFled[] = _("{PLAY_SE SE_FLEE}{B_LINK_OPPONENT1_NAME} fled!"); //not in gen 5+, replaced with match was forfeited text
static const u8 sText_TwoWildFled[] = _("{PLAY_SE SE_FLEE}{B_LINK_OPPONENT1_NAME} and {B_LINK_OPPONENT2_NAME} fled!"); //not in gen 5+, replaced with match was forfeited text
static const u8 sText_PlayerDefeatedLinkTrainerTrainer1[] = _("You defeated {B_TRAINER1_NAME_WITH_CLASS}!\p");
static const u8 sText_OpponentMon1Appeared[] = _("{B_OPPONENT_MON1_NAME} appeared!\p");
static const u8 sText_WildPkmnAppeared[] = _("You encountered a wild {B_OPPONENT_MON1_NAME}!\p");
static const u8 sText_LegendaryPkmnAppeared[] = _("You encountered a wild {B_OPPONENT_MON1_NAME}!\p");
static const u8 sText_WildPkmnAppearedPause[] = _("You encountered a wild {B_OPPONENT_MON1_NAME}!{PAUSE 127}");
static const u8 sText_TwoWildPkmnAppeared[] = _("Oh! A wild {B_OPPONENT_MON1_NAME} and {B_OPPONENT_MON2_NAME} appeared!\p");
static const u8 sText_GhostAppearedCantId[] = _("The GHOST appeared!\pDarn!\nThe GHOST can't be ID'd!\p");
static const u8 sText_TheGhostAppeared[] = _("The GHOST appeared!\p");
static const u8 sText_Trainer1WantsToBattle[] = _("You are challenged by {B_TRAINER1_NAME_WITH_CLASS}!\p");
static const u8 sText_LinkTrainerWantsToBattle[] = _("You are challenged by {B_LINK_OPPONENT1_NAME}!");
static const u8 sText_TwoLinkTrainersWantToBattle[] = _("You are challenged by {B_LINK_OPPONENT1_NAME} and {B_LINK_OPPONENT2_NAME}!");
static const u8 sText_Trainer1SentOutPkmn[] = _("{B_TRAINER1_NAME_WITH_CLASS} sent out {B_OPPONENT_MON1_NAME}!");
static const u8 sText_Trainer1SentOutTwoPkmn[] = _("{B_TRAINER1_NAME_WITH_CLASS} sent out {B_OPPONENT_MON1_NAME} and {B_OPPONENT_MON2_NAME}!");
static const u8 sText_Trainer1SentOutPkmn2[] = _("{B_TRAINER1_NAME_WITH_CLASS} sent out {B_BUFF1}!");
static const u8 sText_LinkTrainerIntroSendOutPkmn[] = _("{B_LINK_OPPONENT1_NAME} sent out {B_LINK_OPPONENT_MON1_NAME}!");
static const u8 sText_LinkTrainerSentOutPkmn[] = _("{B_LINK_OPPONENT1_NAME} sent out {B_BUFF1}!");
static const u8 sText_LinkTrainer2SentOutPkmn2[] = _("{B_LINK_OPPONENT2_NAME} sent out {B_BUFF1}!");
static const u8 sText_LinkTrainerSentOutTwoPkmn[] = _("{B_LINK_OPPONENT1_NAME} sent out {B_OPPONENT_MON1_NAME} and {B_OPPONENT_MON2_NAME}!");
static const u8 sText_TwoLinkTrainersIntroSendOutPkmn[] = _("{B_LINK_OPPONENT1_NAME} sent out {B_LINK_OPPONENT_MON1_NAME}! {B_LINK_OPPONENT2_NAME} sent out {B_LINK_OPPONENT_MON2_NAME}!");
static const u8 sText_LinkTrainerSentOutPkmn2[] = _("{B_LINK_OPPONENT1_NAME} sent out {B_LINK_OPPONENT_MON2_NAME}!");
static const u8 sText_LinkTrainerMultiSentOutPkmn[] = _("{B_LINK_SCR_TRAINER_NAME} sent out {B_BUFF1}!");
static const u8 sText_GoPkmn[] = _("Go! {B_PLAYER_MON1_NAME}!");
static const u8 sText_GoTwoPkmn[] = _("Go! {B_PLAYER_MON1_NAME} and {B_PLAYER_MON2_NAME}!");
static const u8 sText_GoPkmn2[] = _("Go! {B_BUFF1}!");
static const u8 sText_DoItPkmn[] = _("You're in charge, {B_BUFF1}!");
static const u8 sText_GoForItPkmn[] = _("Go for it, {B_BUFF1}!");
static const u8 sText_BeCarefulPkmn[] = _("Be careful, {B_PLAYER_MON1_NAME}!");
static const u8 sText_JustALittleMorePkmn[] = _("Just a little more! Hang in there, {B_BUFF1}!"); //currently unused, will require code changes
static const u8 sText_YourFoesWeakGetEmPkmn[] = _("Your opponent's weak! Get 'em, {B_BUFF1}!");
static const u8 sText_LinkPartnerSentOutPkmn1GoPkmn[] = _("{B_LINK_PARTNER_NAME} sent out {B_LINK_PLAYER_MON1_NAME}! Go, {B_LINK_PLAYER_MON2_NAME}!");
static const u8 sText_LinkPartnerSentOutPkmn2GoPkmn[] = _("{B_LINK_PARTNER_NAME} sent out {B_LINK_PLAYER_MON2_NAME}! Go, {B_LINK_PLAYER_MON1_NAME}!");
static const u8 sText_LinkPartnerSentOutPkmn1[] = _("{B_LINK_PARTNER_NAME} sent out {B_BUFF1}!");
static const u8 sText_LinkPartnerSentOutPkmn2[] = _("{B_LINK_PARTNER_NAME} sent out {B_BUFF1}!");
static const u8 sText_LinkPartnerWithdrewPkmn1[] = _("{B_LINK_PARTNER_NAME} withdrew {B_LINK_PLAYER_MON1_NAME}!");
static const u8 sText_LinkPartnerWithdrewPkmn2[] = _("{B_LINK_PARTNER_NAME} withdrew {B_LINK_PLAYER_MON2_NAME}!");
static const u8 sText_PkmnSwitchOut[] = _("{B_BUFF1}, switch out! Come back!"); //currently unused, I believe its used for when you switch on a Pokémon in shift mode
static const u8 sText_PkmnThatsEnough[] = _("{B_BUFF1}, that's enough! Come back!");
static const u8 sText_PkmnComeBack[] = _("{B_BUFF1}, come back!");
static const u8 sText_PkmnOkComeBack[] = _("OK, {B_BUFF1}! Come back!");
static const u8 sText_PkmnGoodComeBack[] = _("Good job, {B_BUFF1}! Come back!");
static const u8 sText_Trainer1WithdrewPkmn[] = _("{B_TRAINER1_NAME_WITH_CLASS} withdrew {B_BUFF1}!");
static const u8 sText_Trainer2WithdrewPkmn[] = _("{B_TRAINER2_NAME_WITH_CLASS} withdrew {B_BUFF1}!");
static const u8 sText_LinkTrainer1WithdrewPkmn[] = _("{B_LINK_OPPONENT1_NAME} withdrew {B_BUFF1}!");
static const u8 sText_LinkTrainer2WithdrewPkmn[] = _("{B_LINK_OPPONENT2_NAME} withdrew {B_BUFF1}!");
static const u8 sText_WildPkmnPrefix[] = _("The wild ");
static const u8 sText_FoePkmnPrefix[] = _("The opposing ");
static const u8 sText_WildPkmnPrefixLower[] = _("the wild ");
static const u8 sText_FoePkmnPrefixLower[] = _("the opposing ");
static const u8 sText_EmptyString8[] = _("");
static const u8 sText_FoePkmnPrefix2[] = _("Opposing");
static const u8 sText_AllyPkmnPrefix[] = _("Ally");
static const u8 sText_FoePkmnPrefix3[] = _("Opposing");
static const u8 sText_AllyPkmnPrefix2[] = _("Ally");
static const u8 sText_FoePkmnPrefix4[] = _("Opposing");
static const u8 sText_AllyPkmnPrefix3[] = _("Ally");
static const u8 sText_AttackerUsedX[] = _("{B_ATK_NAME_WITH_PREFIX} used {B_BUFF3}!");
static const u8 sText_AttackerUsedX2[] = _("{B_ATK_NAME_WITH_PREFIX} goes for {B_BUFF3}!");
static const u8 sText_AttackerUsedX3[] = _("{B_ATK_NAME_WITH_PREFIX} unleashes {B_BUFF3}!");
static const u8 sText_AttackerUsedX4[] = _("{B_ATK_NAME_WITH_PREFIX} lets loose {B_BUFF3}!");
static const u8 sText_AttackerUsedX5[] = _("{B_ATK_NAME_WITH_PREFIX} attacks with {B_BUFF3}!");
static const u8 sText_AttackerUsedX6[] = _("{B_ATK_NAME_WITH_PREFIX} fires off {B_BUFF3}!");
static const u8 sText_AttackerUsedX7[] = _("{B_ATK_NAME_WITH_PREFIX} breaks out {B_BUFF3}!");
static const u8 sText_AttackerUsedX8[] = _("{B_ATK_NAME_WITH_PREFIX} whips out {B_BUFF3}!");
static const u8 sText_ExclamationMark[] = _("!");
static const u8 sText_ExclamationMark2[] = _("!");
static const u8 sText_ExclamationMark3[] = _("!");
static const u8 sText_ExclamationMark4[] = _("!");
static const u8 sText_ExclamationMark5[] = _("!");
static const u8 sText_HP[] = _("HP");
static const u8 sText_Attack[] = _("Attack");
static const u8 sText_Defense[] = _("Defense");
static const u8 sText_Speed[] = _("Speed");
static const u8 sText_SpAttack[] = _("Sp. Atk");
static const u8 sText_SpDefense[] = _("Sp. Def");
static const u8 sText_Accuracy[] = _("accuracy");
static const u8 sText_Evasiveness[] = _("evasiveness");
static const u8 sText_NuzlockeNoCatch[] = _("You can't catch more than one Pokémon\nper area in Nuzlocke mode!");

const u8 *const gStatNamesTable[NUM_BATTLE_STATS] =
{
    [STAT_HP]      = sText_HP,
    [STAT_ATK]     = sText_Attack,
    [STAT_DEF]     = sText_Defense,
    [STAT_SPEED]   = sText_Speed,
    [STAT_SPATK]   = sText_SpAttack,
    [STAT_SPDEF]   = sText_SpDefense,
    [STAT_ACC]     = sText_Accuracy,
    [STAT_EVASION] = sText_Evasiveness,
};
const u8 *const gPokeblockWasTooXStringTable[FLAVOR_COUNT] =
{
    [FLAVOR_SPICY]  = COMPOUND_STRING("was too spicy!"),
    [FLAVOR_DRY]    = COMPOUND_STRING("was too dry!"),
    [FLAVOR_SWEET]  = COMPOUND_STRING("was too sweet!"),
    [FLAVOR_BITTER] = COMPOUND_STRING("was too bitter!"),
    [FLAVOR_SOUR]   = COMPOUND_STRING("was too sour!"),
};

static const u8 sText_Someones[] = _("someone's");
static const u8 sText_Lanettes[] = _("LANETTE's"); //no decapitalize until it is everywhere
static const u8 sText_Bills[] = _("BILL's");
static const u8 sText_EnigmaBerry[] = _("ENIGMA BERRY"); //no decapitalize until it is everywhere
static const u8 sText_BerrySuffix[] = _(" BERRY"); //no decapitalize until it is everywhere
const u8 gText_EmptyString3[] = _("");

static const u8 sText_TwoInGameTrainersDefeated[] = _("You defeated {B_TRAINER1_NAME_WITH_CLASS} and {B_TRAINER2_NAME_WITH_CLASS}!\p");

// New battle strings.
const u8 gText_drastically[] = _(" drastically");
const u8 gText_severely[] = _("severely ");
static const u8 sText_TerrainReturnedToNormal[] = _("The terrain returned to normal!"); // Unused

const u8 *const gBattleStringsTable[STRINGID_COUNT] =
{
    [STRINGID_TRAINER1LOSETEXT]                     = COMPOUND_STRING("{B_TRAINER1_LOSE_TEXT}"),
    [STRINGID_PKMNGAINEDEXP]                        = COMPOUND_STRING("{B_BUFF1} gained{B_BUFF2} {B_BUFF3} Exp. Points!\p"),
    [STRINGID_PKMNGREWTOLV]                         = COMPOUND_STRING("{B_BUFF1} grew to Lv. {B_BUFF2}!{WAIT_SE}\p"),
    [STRINGID_PKMNLEARNEDMOVE]                      = COMPOUND_STRING("{B_BUFF1} learned {B_BUFF2}!{WAIT_SE}\p"),
    [STRINGID_TRYTOLEARNMOVE1]                      = COMPOUND_STRING("{B_BUFF1} wants to learn the move {B_BUFF2}.\p"),
    [STRINGID_TRYTOLEARNMOVE2]                      = COMPOUND_STRING("However, {B_BUFF1} already knows four moves.\p"),
    [STRINGID_TRYTOLEARNMOVE3]                      = COMPOUND_STRING("Should another move be forgotten and replaced with {B_BUFF2}?"),
    [STRINGID_PKMNFORGOTMOVE]                       = COMPOUND_STRING("{B_BUFF1} forgot {B_BUFF2}…\p"),
    [STRINGID_STOPLEARNINGMOVE]                     = COMPOUND_STRING("{PAUSE 32}Do you want to give up on having {B_BUFF1} learn {B_BUFF2}?"),
    [STRINGID_DIDNOTLEARNMOVE]                      = COMPOUND_STRING("{B_BUFF1} did not learn {B_BUFF2}.\p"),
    [STRINGID_PKMNLEARNEDMOVE2]                     = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} learned {B_BUFF1}!"),
    [STRINGID_PKMNPROTECTEDITSELF]                  = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} protected itself!"),
    [STRINGID_ITDOESNTAFFECT]                       = COMPOUND_STRING("It doesn't affect {B_DEF_NAME_WITH_PREFIX2}…"),
    [STRINGID_ITDOESNTAFFECTSCR]                    = COMPOUND_STRING("It doesn't affect {B_SCR_NAME_WITH_PREFIX2}…"),
    [STRINGID_BATTLERFAINTED]                       = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} fainted!\p"),
    [STRINGID_BATTLERFAINTED_2]                     = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} went down!\p"),
    [STRINGID_BATTLERFAINTED_3]                     = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} is unable to battle!\p"),
    [STRINGID_BATTLERFAINTED_4]                     = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} collapsed!\p"),
    [STRINGID_BATTLERFAINTED_5]                     = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} was knocked out!\p"),
    [STRINGID_BATTLERFAINTED_6]                     = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} couldn't go on!\p"),
    [STRINGID_BATTLERFAINTED_7]                     = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} hit the ground!\p"),
    [STRINGID_BATTLERFAINTED_8]                     = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} dropped!\p"),
    [STRINGID_PLAYERGOTMONEY]                       = COMPOUND_STRING("You got ¥{B_BUFF1} for winning!\p"),
    [STRINGID_PLAYERWHITEOUT]                       = COMPOUND_STRING("You have no more Pokémon that can fight!\p"),
    [STRINGID_PLAYERWHITEOUT2_WILD]                 = COMPOUND_STRING("You panicked and dropped ¥{B_BUFF1}…"),
    [STRINGID_PLAYERWHITEOUT2_TRAINER]              = COMPOUND_STRING("You gave ¥{B_BUFF1} to the winner…"),
    [STRINGID_PLAYERWHITEOUT3]                      = COMPOUND_STRING("You were overwhelmed by your defeat!"),
    [STRINGID_PREVENTSESCAPE]                       = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} prevents escape with {B_SCR_ABILITY}!\p"),
    [STRINGID_HITXTIMES]                            = COMPOUND_STRING("The Pokémon was hit {B_BUFF1} time{B_BUFF2}!"),
    [STRINGID_PKMNFELLASLEEP]                       = COMPOUND_STRING("{B_EFF_NAME_WITH_PREFIX} fell asleep!"),
    [STRINGID_PKMNMADESLEEP]                        = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX}'s {B_BUFF1} made {B_EFF_NAME_WITH_PREFIX2} sleep!"), //not in gen 5+, ability popup
    [STRINGID_PKMNALREADYASLEEP]                    = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} is already asleep!"),
    [STRINGID_PKMNALREADYASLEEP2]                   = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} is already asleep!"),
    [STRINGID_PKMNWASPOISONED]                      = COMPOUND_STRING("{B_EFF_NAME_WITH_PREFIX} was poisoned!"),
    [STRINGID_PKMNPOISONEDBY]                       = COMPOUND_STRING("{B_EFF_NAME_WITH_PREFIX} was poisoned by {B_SCR_NAME_WITH_PREFIX2}'s {B_BUFF1}!"), //not in gen 5+, ability popup
    [STRINGID_PKMNHURTBYPOISON]                     = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} was hurt by its poisoning!"),
    [STRINGID_PKMNALREADYPOISONED]                  = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} is already poisoned!"),
    [STRINGID_PKMNBADLYPOISONED]                    = COMPOUND_STRING("{B_EFF_NAME_WITH_PREFIX} was badly poisoned!"),
    [STRINGID_PKMNENERGYDRAINED]                    = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} had its energy drained!"),
    [STRINGID_PKMNWASBURNED]                        = COMPOUND_STRING("{B_EFF_NAME_WITH_PREFIX} was burned!"),
    [STRINGID_PKMNBURNEDBY]                         = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX}'s {B_BUFF1} burned {B_EFF_NAME_WITH_PREFIX2}!"), //not in gen 5+, ability popup
    [STRINGID_PKMNHURTBYBURN]                       = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} was hurt by its burn!"),
    [STRINGID_PKMNWASFROZEN]                        = COMPOUND_STRING("{B_EFF_NAME_WITH_PREFIX} was frozen solid!"),
    [STRINGID_PKMNFROZENBY]                         = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX}'s {B_BUFF1} froze {B_EFF_NAME_WITH_PREFIX2} solid!"), //not in gen 5+, ability popup
    [STRINGID_PKMNISFROZEN]                         = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} is frozen solid!"),
    [STRINGID_PKMNWASDEFROSTED]                     = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} thawed out!"),
    [STRINGID_PKMNWASDEFROSTEDBY]                   = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX}'s {B_CURRENT_MOVE} melted the ice!"),
    [STRINGID_PKMNWASPARALYZED]                     = COMPOUND_STRING("{B_EFF_NAME_WITH_PREFIX} is paralyzed, so it may be unable to move!"),
    [STRINGID_PKMNWASPARALYZEDBY]                   = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX}'s {B_BUFF1} paralyzed {B_EFF_NAME_WITH_PREFIX2}, so it may be unable to move!"), //not in gen 5+, ability popup
    [STRINGID_PKMNISPARALYZED]                      = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} couldn't move because it's paralyzed!"),
    [STRINGID_PKMNISALREADYPARALYZED]               = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} is already paralyzed!"),
    [STRINGID_PKMNHEALEDPARALYSIS]                  = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX} was cured of paralysis!"),
    [STRINGID_STATSWONTINCREASE]                    = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX}'s {B_BUFF1} won't go any higher!"),
    [STRINGID_STATSWONTDECREASE]                    = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX}'s {B_BUFF1} won't go any lower!"),
    [STRINGID_PKMNISCONFUSED]                       = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} is confused!"),
    [STRINGID_PKMNHEALEDCONFUSION]                  = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} snapped out of its confusion!"),
    [STRINGID_PKMNWASCONFUSED]                      = COMPOUND_STRING("{B_EFF_NAME_WITH_PREFIX} became confused!"),
    [STRINGID_PKMNALREADYCONFUSED]                  = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX} is already confused!"),
    [STRINGID_PKMNFELLINLOVE]                       = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX} fell in love!"),
    [STRINGID_PKMNINLOVE]                           = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} is in love with {B_SCR_NAME_WITH_PREFIX2}!"),
    [STRINGID_PKMNIMMOBILIZEDBYLOVE]                = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} is immobilized by love!"),
    [STRINGID_PKMNCHANGEDTYPE]                      = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} transformed into the {B_BUFF1} type!"),
    [STRINGID_PKMNFLINCHED]                         = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} flinched and couldn't move!"),
    [STRINGID_PKMNREGAINEDHEALTH]                   = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX}'s HP was restored."),
    [STRINGID_PKMNHPFULL]                           = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX}'s HP is full!"),
    [STRINGID_PKMNRAISEDSPDEF]                      = COMPOUND_STRING("Light Screen made {B_ATK_TEAM2} side stronger against special moves!"),
    [STRINGID_PKMNRAISEDDEF]                        = COMPOUND_STRING("Reflect made {B_ATK_TEAM2} side stronger against physical moves!"),
    [STRINGID_PKMNAURORAVEIL]                       = COMPOUND_STRING("Aurora Veil made {B_ATK_TEAM2} side stronger against physical and special moves!"),
    [STRINGID_PKMNCOVEREDBYVEIL]                    = COMPOUND_STRING("{B_ATK_TEAM1} side became cloaked in a mystical veil!"),
    [STRINGID_PKMNUSEDSAFEGUARD]                    = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} is protected by Safeguard!"),
    [STRINGID_PKMNSAFEGUARDEXPIRED]                 = COMPOUND_STRING("{B_ATK_TEAM1} side is no longer protected by the mystical veil!"),
    [STRINGID_PKMNWENTTOSLEEP]                      = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} went to sleep!"), //not in gen 5+
    [STRINGID_PKMNSLEPTHEALTHY]                     = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} slept and restored its HP!"),
    [STRINGID_PKMNWHIPPEDWHIRLWIND]                 = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} whipped up a whirlwind!"),
    [STRINGID_PKMNTOOKSUNLIGHT]                     = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} absorbed light!"),
    [STRINGID_PKMNLOWEREDHEAD]                      = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} tucked in its head!"),
    [STRINGID_PKMNFLEWHIGH]                         = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} flew up high!"),
    [STRINGID_PKMNDUGHOLE]                          = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} burrowed its way under the ground!"),
    [STRINGID_PKMNSQUEEZEDBYBIND]                   = COMPOUND_STRING("{B_EFF_NAME_WITH_PREFIX} was squeezed by {B_SCR_NAME_WITH_PREFIX2}!"),
    [STRINGID_PKMNTRAPPEDINVORTEX]                  = COMPOUND_STRING("{B_EFF_NAME_WITH_PREFIX} became trapped in the vortex!"),
    [STRINGID_PKMNWRAPPEDBY]                        = COMPOUND_STRING("{B_EFF_NAME_WITH_PREFIX} was wrapped by {B_SCR_NAME_WITH_PREFIX2}!"),
    [STRINGID_PKMNCLAMPED]                          = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} clamped down on {B_EFF_NAME_WITH_PREFIX2}!"),
    [STRINGID_PKMNHURTBY]                           = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} is hurt by {B_BUFF1}!"),
    [STRINGID_PKMNFREEDFROM]                        = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} was freed from {B_BUFF1}!"),
    [STRINGID_PKMNCRASHED]                          = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} kept going and crashed!"),
    [STRINGID_PKMNSHROUDEDINMIST]                   = gText_PkmnShroudedInMist,
    [STRINGID_PKMNPROTECTEDBYMIST]                  = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} is protected by the mist!"),
    [STRINGID_PKMNGETTINGPUMPED]                    = gText_PkmnGettingPumped,
    [STRINGID_PKMNHITWITHRECOIL]                    = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} was damaged by the recoil!"),
    [STRINGID_PKMNPROTECTEDITSELF2]                 = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} protected itself!"),
    [STRINGID_PKMNBUFFETEDBYSANDSTORM]              = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} is buffeted by the sandstorm!"),
    [STRINGID_PKMNPELTEDBYHAIL]                     = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} is buffeted by the hail!"),
    [STRINGID_PKMNSEEDED]                           = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX} was seeded!"),
    [STRINGID_PKMNAVOIDEDATTACK]                    = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX} avoided the attack!"),
    [STRINGID_PKMNAVOIDEDATTACK_2]                  = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX} dodged the attack!"),
    [STRINGID_PKMNAVOIDEDATTACK_3]                  = COMPOUND_STRING("The attack missed {B_DEF_NAME_WITH_PREFIX2}!"),
    [STRINGID_PKMNAVOIDEDATTACK_4]                  = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX} slipped away from the attack!"),
    [STRINGID_PKMNAVOIDEDATTACK_5]                  = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX} evaded the attack!"),
    [STRINGID_PKMNAVOIDEDATTACK_6]                  = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX} sidestepped the attack!"),
    [STRINGID_PKMNAVOIDEDATTACK_7]                  = COMPOUND_STRING("The attack couldn't touch {B_DEF_NAME_WITH_PREFIX2}!"),
    [STRINGID_PKMNAVOIDEDATTACK_8]                  = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX} got out of the way!"),
    [STRINGID_BATTLERAVOIDEDATTACK]                 = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} avoided the attack!"),
    [STRINGID_PKMNSAPPEDBYLEECHSEED]                = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX}'s health is sapped by Leech Seed!"),
    [STRINGID_PKMNFASTASLEEP]                       = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} is fast asleep."),
    [STRINGID_PKMNWOKEUP]                           = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} woke up!"),
    [STRINGID_PKMNWOKEUPINUPROAR]                   = COMPOUND_STRING("The uproar woke {B_EFF_NAME_WITH_PREFIX2}!"),
    [STRINGID_PKMNCAUSEDUPROAR]                     = COMPOUND_STRING("{B_EFF_NAME_WITH_PREFIX} caused an uproar!"),
    [STRINGID_PKMNMAKINGUPROAR]                     = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} is making an uproar!"),
    [STRINGID_PKMNCALMEDDOWN]                       = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} calmed down."),
    [STRINGID_PKMNSTOCKPILED]                       = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} stockpiled {B_BUFF1}!"),
    [STRINGID_PKMNCANTSLEEPINUPROAR2]               = COMPOUND_STRING("The uproar prevented {B_DEF_NAME_WITH_PREFIX2} from falling asleep!"),
    [STRINGID_UPROARKEPTPKMNAWAKE]                  = COMPOUND_STRING("But the uproar kept {B_DEF_NAME_WITH_PREFIX2} awake!"),
    [STRINGID_PKMNSTAYEDAWAKEUSING]                 = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX} stayed awake!"),
    [STRINGID_PKMNSTORINGENERGY]                    = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} is storing energy!"),
    [STRINGID_PKMNUNLEASHEDENERGY]                  = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} unleashed its energy!"),
    [STRINGID_PKMNFATIGUECONFUSION]                 = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} became confused due to fatigue!"),
    [STRINGID_PLAYERPICKEDUPMONEY]                  = COMPOUND_STRING("You picked up ¥{B_BUFF1}!\p"),
    [STRINGID_PKMNUNAFFECTED]                       = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX} is unaffected!"),
    [STRINGID_PKMNTRANSFORMEDINTO]                  = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} transformed into {B_BUFF1}!"),
    [STRINGID_PKMNMADESUBSTITUTE]                   = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} put in a substitute!"),
    [STRINGID_PKMNHASSUBSTITUTE]                    = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} already has a substitute!"),
    [STRINGID_SUBSTITUTEDAMAGED]                    = COMPOUND_STRING("The substitute took damage for {B_DEF_NAME_WITH_PREFIX2}!\p"),
    [STRINGID_PKMNSUBSTITUTEFADED]                  = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX}'s substitute faded!\p"),
    [STRINGID_PKMNMUSTRECHARGE]                     = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} must recharge!"),
    [STRINGID_PKMNRAGEBUILDING]                     = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX}'s rage is building!"),
    [STRINGID_PKMNMOVEWASDISABLED]                  = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX}'s {B_BUFF1} was disabled!"),
    [STRINGID_PKMNMOVEISDISABLED]                   = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX}'s {B_CURRENT_MOVE} is disabled!\p"),
    [STRINGID_PKMNMOVEDISABLEDNOMORE]               = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX}'s move is no longer disabled!"),
    [STRINGID_PKMNGOTENCORE]                        = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX} must do an encore!"),
    [STRINGID_PKMNGOTENCOREDMOVE]                   = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} can only use {B_CURRENT_MOVE}!\p"),
    [STRINGID_PKMNENCOREENDED]                      = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} ended its encore!"),
    [STRINGID_PKMNTOOKAIM]                          = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} took aim at {B_DEF_NAME_WITH_PREFIX2}!"),
    [STRINGID_PKMNSKETCHEDMOVE]                     = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} sketched {B_BUFF1}!"),
    [STRINGID_PKMNTRYINGTOTAKEFOE]                  = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} is hoping to take its attacker down with it!"),
    [STRINGID_PKMNTOOKFOE]                          = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX} took its attacker down with it!"),
    [STRINGID_PKMNREDUCEDPP]                        = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX} lost {B_BUFF2} PP from {B_BUFF1}!"),
    [STRINGID_PKMNSTOLEITEM]                        = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} stole {B_BUFF2}'s {B_LAST_ITEM}!"),
    [STRINGID_TARGETCANTESCAPENOW]                  = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX} can no longer escape!"),
    [STRINGID_PKMNFELLINTONIGHTMARE]                = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX} began having a nightmare!"),
    [STRINGID_PKMNLOCKEDINNIGHTMARE]                = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} is locked in a nightmare!"),
    [STRINGID_PKMNLAIDCURSE]                        = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} cut its own HP and put a curse on {B_DEF_NAME_WITH_PREFIX2}!"),
    [STRINGID_PKMNAFFLICTEDBYCURSE]                 = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} is afflicted by the curse!"),
    [STRINGID_SPIKESSCATTERED]                      = COMPOUND_STRING("Spikes were scattered on the ground all around {B_DEF_TEAM2} side!"),
    [STRINGID_PKMNHURTBYSPIKES]                     = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} was hurt by the spikes!"),
    [STRINGID_PKMNIDENTIFIED]                       = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX} was identified!"),
    [STRINGID_PKMNPERISHCOUNTFELL]                  = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX}'s perish count fell to {B_BUFF1}!"),
    [STRINGID_PKMNBRACEDITSELF]                     = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} braced itself!"),
    [STRINGID_PKMNENDUREDHIT]                       = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX} endured the hit!"),
    [STRINGID_MAGNITUDESTRENGTH]                    = COMPOUND_STRING("Magnitude {B_BUFF1}!"),
    [STRINGID_PKMNCUTHPMAXEDATTACK]                 = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} cut its own HP and maximized its Attack!"),
    [STRINGID_PKMNCOPIEDSTATCHANGES]                = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} copied {B_EFF_NAME_WITH_PREFIX2}'s stat changes!"),
    [STRINGID_PKMNGOTFREE]                          = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} was freed from {B_BUFF1}!"),
    [STRINGID_PKMNSHEDLEECHSEED]                    = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} was freed from Leech Seed!"),
    [STRINGID_PKMNBLEWAWAYSPIKES]                   = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} blew away Spikes!"), // Not in gen 5+
    [STRINGID_PKMNFLEDFROMBATTLE]                   = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} fled from battle!"),
    [STRINGID_PKMNFORESAWATTACK]                    = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} foresaw an attack!"),
    [STRINGID_PKMNTOOKATTACK]                       = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX} took the {B_BUFF1} attack!"),
    [STRINGID_PKMNATTACK]                           = COMPOUND_STRING("{B_BUFF1}'s attack!"), // Not in gen 5+
    [STRINGID_PKMNCENTERATTENTION]                  = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX} became the center of attention!"),
    [STRINGID_PKMNCHARGINGPOWER]                    = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} began charging power!"),
    [STRINGID_NATUREPOWERTURNEDINTO]                = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX}'s Nature Power turned into {B_CURRENT_MOVE}!"),
    [STRINGID_PKMNSTATUSNORMAL]                     = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX}'s status returned to normal!"),
    [STRINGID_PKMNHASNOMOVESLEFT]                   = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} has no moves left that it can use!\p"),
    [STRINGID_PKMNSUBJECTEDTOTORMENT]               = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX} was subjected to torment!"),
    [STRINGID_PKMNCANTUSEMOVETORMENT]               = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} can't use the same move twice in a row due to the torment!\p"),
    [STRINGID_PKMNTIGHTENINGFOCUS]                  = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} is tightening its focus!"),
    [STRINGID_PKMNFELLFORTAUNT]                     = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX} fell for the taunt!"),
    [STRINGID_PKMNCANTUSEMOVETAUNT]                 = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} can't use {B_CURRENT_MOVE} after the taunt!\p"),
    [STRINGID_PKMNREADYTOHELP]                      = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} is ready to help {B_DEF_NAME_WITH_PREFIX2}!"),
    [STRINGID_PKMNSWITCHEDITEMS]                    = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} switched items with its target!"),
    [STRINGID_PKMNCOPIEDFOE]                        = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} copied {B_DEF_NAME_WITH_PREFIX2}'s Ability!"),
    [STRINGID_PKMNWISHCAMETRUE]                     = COMPOUND_STRING("{B_BUFF1}'s wish came true!"),
    [STRINGID_PKMNPLANTEDROOTS]                     = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} planted its roots!"),
    [STRINGID_PKMNABSORBEDNUTRIENTS]                = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} absorbed nutrients with its roots!"),
    [STRINGID_PKMNANCHOREDITSELF]                   = COMPOUND_STRING("{B_EFF_NAME_WITH_PREFIX} is anchored in place with its roots!"),
    [STRINGID_PKMNWASMADEDROWSY]                    = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX} grew drowsy!"),
    [STRINGID_PKMNKNOCKEDOFF]                       = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} knocked off {B_EFF_NAME_WITH_PREFIX2}'s {B_LAST_ITEM}!"),
    [STRINGID_PKMNSWAPPEDABILITIES]                 = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} swapped Abilities with its target!"),
    [STRINGID_PKMNSEALEDOPPONENTMOVE]               = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} sealed any moves its target shares with it!"),
    [STRINGID_PKMNCANTUSEMOVESEALED]                = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} can't use its sealed {B_CURRENT_MOVE}!\p"),
    [STRINGID_PKMNWANTSGRUDGE]                      = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} wants its target to bear a grudge!"),
    [STRINGID_PKMNLOSTPPGRUDGE]                     = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} lost all of {B_BUFF1}'s PP due to the grudge!"),
    [STRINGID_PKMNSHROUDEDITSELF]                   = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} shrouded itself with Magic Coat!"),
    [STRINGID_PKMNMOVEBOUNCED]                      = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} bounced the {B_CURRENT_MOVE} back!"),
    [STRINGID_PKMNWAITSFORTARGET]                   = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} is waiting for a target to make a move!"),
    [STRINGID_PKMNSNATCHEDMOVE]                     = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} snatched {B_SCR_NAME_WITH_PREFIX2}'s move!"),
    [STRINGID_PKMNMADEITRAIN]                       = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX}'s {B_SCR_ABILITY} made it rain!"), //not in gen 5+, ability popup
    [STRINGID_PKMNPROTECTEDBY]                      = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX} was protected by {B_DEF_ABILITY}!"), //not in gen 5+, ability popup
    [STRINGID_PKMNPREVENTSUSAGE]                    = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX}'s {B_DEF_ABILITY} prevents {B_ATK_NAME_WITH_PREFIX2} from using {B_CURRENT_MOVE}!"), //not in gen 5+, ability popup
    [STRINGID_PKMNRESTOREDHPUSING]                  = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} had its HP restored."),
    [STRINGID_PKMNCHANGEDTYPEWITH]                  = COMPOUND_STRING("{B_EFF_NAME_WITH_PREFIX}'s type changed to {B_BUFF1}!"),
    [STRINGID_PKMNPREVENTSROMANCEWITH]              = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX}'s {B_DEF_ABILITY} prevents romance!"), //not in gen 5+, ability popup
    [STRINGID_PKMNPREVENTSCONFUSIONWITH]            = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} cannot be confused!"),
    [STRINGID_PKMNRAISEDFIREPOWERWITH]              = COMPOUND_STRING("The power of {B_SCR_NAME_WITH_PREFIX}'s Fire-type moves rose!"),
    [STRINGID_PKMNANCHORSITSELFWITH]                = COMPOUND_STRING("{B_EFF_NAME_WITH_PREFIX} is anchored in place with its suction cups!"),
    [STRINGID_PKMNPREVENTSSTATLOSSWITH]             = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX}'s stats were not lowered!"),
    [STRINGID_PKMNHURTSWITH]                        = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} was hurt by {B_DEF_NAME_WITH_PREFIX2}'s {B_BUFF1}!"),
    [STRINGID_PKMNTRACED]                           = COMPOUND_STRING("It traced {B_BUFF1}'s {B_BUFF2}!"),
    [STRINGID_STATSHARPLY]                          = gText_StatSharply,
    [STRINGID_STATHARSHLY]                          = COMPOUND_STRING("harshly "),
    [STRINGID_STATROSE]                             = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX}'s {B_BUFF1} rose{B_BUFF2}!"),
    [STRINGID_STATFELL]                             = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX}'s {B_BUFF1} {B_BUFF2}fell!"),
    [STRINGID_CRITICALHIT]                          = COMPOUND_STRING("A critical hit!"),
    [STRINGID_CRITICALHIT_2]                        = COMPOUND_STRING("Critical hit!"),
    [STRINGID_CRITICALHIT_3]                        = COMPOUND_STRING("A critical hit connects!"),
    [STRINGID_CRITICALHIT_4]                        = COMPOUND_STRING("A crushing critical hit!"),
    [STRINGID_CRITICALHIT_5]                        = COMPOUND_STRING("Dead on target! A critical hit!"),
    [STRINGID_CRITICALHIT_6]                        = COMPOUND_STRING("A critical strike lands!"),
    [STRINGID_CRITICALHIT_7]                        = COMPOUND_STRING("A telling critical hit!"),
    [STRINGID_CRITICALHIT_8]                        = COMPOUND_STRING("A perfectly placed hit!"),
    [STRINGID_ONEHITKO]                             = COMPOUND_STRING("It's a one-hit KO!"),
    [STRINGID_123POOF]                              = COMPOUND_STRING("One…{PAUSE 10}two…{PAUSE 10}and…{PAUSE 10}{PAUSE 20}{PLAY_SE SE_BALL_BOUNCE_1}ta-da!\p"),
    [STRINGID_ANDELLIPSIS]                          = COMPOUND_STRING("And…\p"),
    [STRINGID_NOTVERYEFFECTIVE]                     = COMPOUND_STRING("It's not very effective…"),
    [STRINGID_NOTVERYEFFECTIVE_2]                   = COMPOUND_STRING("It didn't do much…"),
    [STRINGID_NOTVERYEFFECTIVE_3]                   = COMPOUND_STRING("That barely worked…"),
    [STRINGID_NOTVERYEFFECTIVE_4]                   = COMPOUND_STRING("It's not doing much…"),
    [STRINGID_NOTVERYEFFECTIVE_5]                   = COMPOUND_STRING("That had little effect…"),
    [STRINGID_NOTVERYEFFECTIVE_6]                   = COMPOUND_STRING("It hardly did anything…"),
    [STRINGID_NOTVERYEFFECTIVE_7]                   = COMPOUND_STRING("It's a weak hit…"),
    [STRINGID_NOTVERYEFFECTIVE_8]                   = COMPOUND_STRING("That didn't land well…"),
    [STRINGID_SUPEREFFECTIVE]                       = COMPOUND_STRING("It's super effective!"),
    [STRINGID_SUPEREFFECTIVE_2]                     = COMPOUND_STRING("A devastating hit!"),
    [STRINGID_SUPEREFFECTIVE_3]                     = COMPOUND_STRING("That did tremendous damage!"),
    [STRINGID_SUPEREFFECTIVE_4]                     = COMPOUND_STRING("A tremendously effective hit!"),
    [STRINGID_SUPEREFFECTIVE_5]                     = COMPOUND_STRING("It's extremely effective!"),
    [STRINGID_SUPEREFFECTIVE_6]                     = COMPOUND_STRING("That hit a weak spot!"),
    [STRINGID_SUPEREFFECTIVE_7]                     = COMPOUND_STRING("It's brutally effective!"),
    [STRINGID_SUPEREFFECTIVE_8]                     = COMPOUND_STRING("That struck hard!"),
    [STRINGID_GOTAWAYSAFELY]                        = sText_GotAwaySafely,
    [STRINGID_WILDPKMNFLED]                         = COMPOUND_STRING("{PLAY_SE SE_FLEE}The wild {B_BUFF1} fled!"),
    [STRINGID_NORUNNINGFROMTRAINERS]                = COMPOUND_STRING("No! There's no running from a Trainer battle!\p"),
    [STRINGID_CANTESCAPE]                           = COMPOUND_STRING("You can't escape!\p"),
    [STRINGID_DONTLEAVEBIRCH]                       = COMPOUND_STRING("PROF. BIRCH: Don't leave me like this!\p"), //no decapitalize until it is everywhere
    [STRINGID_BUTNOTHINGHAPPENED]                   = COMPOUND_STRING("But nothing happened!"),
    [STRINGID_BUTNOTHINGHAPPENED_2]                 = COMPOUND_STRING("Nothing happened!"),
    [STRINGID_BUTNOTHINGHAPPENED_3]                 = COMPOUND_STRING("But nothing came of it!"),
    [STRINGID_BUTNOTHINGHAPPENED_4]                 = COMPOUND_STRING("But there was no effect!"),
    [STRINGID_BUTNOTHINGHAPPENED_5]                 = COMPOUND_STRING("But nothing changed!"),
    [STRINGID_BUTNOTHINGHAPPENED_6]                 = COMPOUND_STRING("But it did nothing!"),
    [STRINGID_BUTNOTHINGHAPPENED_7]                 = COMPOUND_STRING("But not a thing happened!"),
    [STRINGID_BUTNOTHINGHAPPENED_8]                 = COMPOUND_STRING("But there was no result!"),
    [STRINGID_BUTITFAILED]                          = COMPOUND_STRING("But it failed!"),
    [STRINGID_BUTITFAILED_2]                        = COMPOUND_STRING("It failed!"),
    [STRINGID_BUTITFAILED_3]                        = COMPOUND_STRING("But it didn't work!"),
    [STRINGID_BUTITFAILED_4]                        = COMPOUND_STRING("But it had no effect!"),
    [STRINGID_BUTITFAILED_5]                        = COMPOUND_STRING("But it didn't succeed!"),
    [STRINGID_BUTITFAILED_6]                        = COMPOUND_STRING("But it fell flat!"),
    [STRINGID_BUTITFAILED_7]                        = COMPOUND_STRING("But it came to nothing!"),
    [STRINGID_BUTITFAILED_8]                        = COMPOUND_STRING("But it was no use!"),
    [STRINGID_ITHURTCONFUSION]                      = COMPOUND_STRING("It hurt itself in its confusion!"),
    [STRINGID_STARTEDTORAIN]                        = COMPOUND_STRING("It started to rain!"),
    [STRINGID_DOWNPOURSTARTED]                      = COMPOUND_STRING("A downpour started!"), // corresponds to DownpourText in pokegold and pokecrystal and is used by Rain Dance in GSC
    [STRINGID_RAINCONTINUES]                        = COMPOUND_STRING("Rain continues to fall."), //not in gen 5+
    [STRINGID_DOWNPOURCONTINUES]                    = COMPOUND_STRING("The downpour continues."), // unused
    [STRINGID_RAINSTOPPED]                          = COMPOUND_STRING("The rain stopped."),
    [STRINGID_SANDSTORMBREWED]                      = COMPOUND_STRING("A sandstorm kicked up!"),
    [STRINGID_SANDSTORMRAGES]                       = COMPOUND_STRING("The sandstorm is raging."),
    [STRINGID_SANDSTORMSUBSIDED]                    = COMPOUND_STRING("The sandstorm subsided."),
    [STRINGID_SUNLIGHTGOTBRIGHT]                    = COMPOUND_STRING("The sunlight turned harsh!"),
    [STRINGID_SUNLIGHTSTRONG]                       = COMPOUND_STRING("The sunlight is strong."), //not in gen 5+
    [STRINGID_SUNLIGHTFADED]                        = COMPOUND_STRING("The sunlight faded."),
    [STRINGID_STARTEDHAIL]                          = COMPOUND_STRING("It started to hail!"),
    [STRINGID_HAILCONTINUES]                        = COMPOUND_STRING("The hail is crashing down."),
    [STRINGID_HAILSTOPPED]                          = COMPOUND_STRING("The hail stopped."),
    [STRINGID_STATCHANGESGONE]                      = COMPOUND_STRING("All stat changes were eliminated!"),
    [STRINGID_COINSSCATTERED]                       = COMPOUND_STRING("Coins were scattered everywhere!"),
    [STRINGID_TOOWEAKFORSUBSTITUTE]                 = COMPOUND_STRING("But it does not have enough HP left to make a substitute!"),
    [STRINGID_SHAREDPAIN]                           = COMPOUND_STRING("The battlers shared their pain!"),
    [STRINGID_BELLCHIMED]                           = COMPOUND_STRING("A bell chimed!"),
    [STRINGID_FAINTINTHREE]                         = COMPOUND_STRING("All Pokémon that heard the song will faint in three turns!"),
    [STRINGID_NOPPLEFT]                             = COMPOUND_STRING("There's no PP left for this move!\p"), //not in gen 5+
    [STRINGID_BUTNOPPLEFT]                          = COMPOUND_STRING("But there was no PP left for the move!"),
    [STRINGID_PLAYERUSEDITEM]                       = COMPOUND_STRING("You used {B_LAST_ITEM}!"),
    [STRINGID_TRAINERBLOCKEDBALL]                   = COMPOUND_STRING("The Trainer blocked your Poké Ball!"),
    [STRINGID_DONTBEATHIEF]                         = COMPOUND_STRING("Don't be a thief!"),
    [STRINGID_ITDODGEDBALL]                         = COMPOUND_STRING("It dodged your thrown Poké Ball! This Pokémon can't be caught!"),
    [STRINGID_PKMNBROKEFREE]                        = COMPOUND_STRING("Oh no! The Pokémon broke free!"),
    [STRINGID_ITAPPEAREDCAUGHT]                     = COMPOUND_STRING("Aww! It appeared to be caught!"),
    [STRINGID_AARGHALMOSTHADIT]                     = COMPOUND_STRING("Aargh! Almost had it!"),
    [STRINGID_SHOOTSOCLOSE]                         = COMPOUND_STRING("Gah! It was so close, too!"),
    [STRINGID_GOTCHAPKMNCAUGHTPLAYER]               = COMPOUND_STRING("Gotcha! {B_DEF_NAME} was caught!{WAIT_SE}{PLAY_BGM MUS_CAUGHT}\p"),
    [STRINGID_GOTCHAPKMNCAUGHTWALLY]                = COMPOUND_STRING("Gotcha! {B_DEF_NAME} was caught!{WAIT_SE}{PLAY_BGM MUS_CAUGHT}{PAUSE 127}"),
    [STRINGID_GIVENICKNAMECAPTURED]                 = COMPOUND_STRING("Would you like to give {B_DEF_NAME} a nickname?"),
    [STRINGID_PKMNDATAADDEDTODEX]                   = COMPOUND_STRING("{B_DEF_NAME}'s data has been added to the Pokédex!\p"),
    [STRINGID_ITISRAINING]                          = COMPOUND_STRING("It's raining!"),
    [STRINGID_SANDSTORMISRAGING]                    = COMPOUND_STRING("The sandstorm is raging!"),
    [STRINGID_CANTESCAPE2]                          = COMPOUND_STRING("You couldn't get away!\p"),
    [STRINGID_PKMNIGNORESASLEEP]                    = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} ignored orders and kept sleeping!"),
    [STRINGID_PKMNIGNOREDORDERS]                    = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} ignored orders!"),
    [STRINGID_PKMNBEGANTONAP]                       = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} began to nap!"),
    [STRINGID_PKMNLOAFING]                          = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} is loafing around!"),
    [STRINGID_PKMNWONTOBEY]                         = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} won't obey!"),
    [STRINGID_PKMNTURNEDAWAY]                       = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} turned away!"),
    [STRINGID_PKMNPRETENDNOTNOTICE]                 = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} pretended not to notice!"),
    [STRINGID_ENEMYABOUTTOSWITCHPKMN]               = COMPOUND_STRING("{B_TRAINER1_NAME_WITH_CLASS} is about to send out {B_BUFF2}. Will you switch your Pokémon?"),
    [STRINGID_CREPTCLOSER]                          = COMPOUND_STRING("{B_PLAYER_NAME} crept closer to {B_OPPONENT_MON1_NAME}!"), //safari
    [STRINGID_CANTGETCLOSER]                        = COMPOUND_STRING("{B_PLAYER_NAME} can't get any closer!"), //safari
    [STRINGID_PKMNWATCHINGCAREFULLY]                = COMPOUND_STRING("{B_OPPONENT_MON1_NAME} is watching carefully!"), //safari
    [STRINGID_PKMNCURIOUSABOUTX]                    = COMPOUND_STRING("{B_OPPONENT_MON1_NAME} is curious about the {B_BUFF1}!"), //safari
    [STRINGID_PKMNENTHRALLEDBYX]                    = COMPOUND_STRING("{B_OPPONENT_MON1_NAME} is enthralled by the {B_BUFF1}!"), //safari
    [STRINGID_PKMNIGNOREDX]                         = COMPOUND_STRING("{B_OPPONENT_MON1_NAME} completely ignored the {B_BUFF1}!"), //safari
    [STRINGID_THREWPOKEBLOCKATPKMN]                 = COMPOUND_STRING("{B_PLAYER_NAME} threw a {POKEBLOCK} at the {B_OPPONENT_MON1_NAME}!"), //safari
    [STRINGID_OUTOFSAFARIBALLS]                     = COMPOUND_STRING("{PLAY_SE SE_DING_DONG}ANNOUNCER: You're out of Safari Balls! Game over!\p"), //safari
    [STRINGID_PKMNSITEMCUREDPARALYSIS]              = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX}'s {B_LAST_ITEM} cured its paralysis!"),
    [STRINGID_PKMNSITEMCUREDPOISON]                 = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX}'s {B_LAST_ITEM} cured its poison!"),
    [STRINGID_PKMNSITEMHEALEDBURN]                  = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX}'s {B_LAST_ITEM} cured its burn!"),
    [STRINGID_PKMNSITEMDEFROSTEDIT]                 = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX}'s {B_LAST_ITEM} defrosted it!"),
    [STRINGID_PKMNSITEMWOKEIT]                      = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX}'s {B_LAST_ITEM} woke it up!"),
    [STRINGID_PKMNSITEMSNAPPEDOUT]                  = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX}'s {B_LAST_ITEM} snapped it out of its confusion!"),
    [STRINGID_PKMNSITEMCUREDPROBLEM]                = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX}'s {B_LAST_ITEM} cured its {B_BUFF1} problem!"), // Not in Gen 5+
    [STRINGID_PKMNSITEMRESTOREDHEALTH]              = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} restored its health using its {B_LAST_ITEM}!"),
    [STRINGID_PKMNSITEMRESTOREDPP]                  = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} restored PP to its move {B_BUFF1} using its {B_LAST_ITEM}!"),
    [STRINGID_PKMNSITEMRESTOREDSTATUS]              = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} returned its stats to normal using its {B_LAST_ITEM}!"),
    [STRINGID_PKMNSITEMRESTOREDHPALITTLE]           = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} restored a little HP using its {B_LAST_ITEM}!"),
    [STRINGID_ITEMALLOWSONLYYMOVE]                  = COMPOUND_STRING("{B_LAST_ITEM} only allows the use of {B_CURRENT_MOVE}!\p"),
    [STRINGID_PKMNHUNGONWITHX]                      = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX} hung on using its {B_LAST_ITEM}!"),
    [STRINGID_EMPTYSTRING3]                         = gText_EmptyString3,
    [STRINGID_PKMNSXRESTOREDHPALITTLE2]             = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} had its HP restored."),
    [STRINGID_PKMNSXWHIPPEDUPSANDSTORM]             = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX}'s {B_SCR_ABILITY} whipped up a sandstorm!"), //not in gen 5+, ability popup
    [STRINGID_PKMNSXPREVENTSYLOSS]                  = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX}'s {B_BUFF1} was not lowered!"),
    [STRINGID_PKMNSXINFATUATEDY]                    = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} fell in love!"),
    [STRINGID_PKMNSXMADEYINEFFECTIVE]               = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX}'s item cannot be removed!"),
    [STRINGID_ITSUCKEDLIQUIDOOZE]                   = COMPOUND_STRING("{B_EFF_NAME_WITH_PREFIX} sucked up the liquid ooze!"),
    [STRINGID_PKMNTRANSFORMED]                      = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} transformed!"),
    [STRINGID_ELECTRICITYWEAKENED]                  = COMPOUND_STRING("Electricity's power was weakened!"),
    [STRINGID_FIREWEAKENED]                         = COMPOUND_STRING("Fire's power was weakened!"),
    [STRINGID_PKMNHIDUNDERWATER]                    = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} hid underwater!"),
    [STRINGID_PKMNSPRANGUP]                         = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} sprang up!"),
    [STRINGID_HMMOVESCANTBEFORGOTTEN]               = COMPOUND_STRING("HM moves can't be forgotten now.\p"),
    [STRINGID_XFOUNDONEY]                           = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} found one {B_LAST_ITEM}!"),
    [STRINGID_PLAYERDEFEATEDTRAINER1]               = sText_PlayerDefeatedLinkTrainerTrainer1,
    [STRINGID_SOOTHINGAROMA]                        = COMPOUND_STRING("A soothing aroma wafted through the area!"),
    [STRINGID_ITEMSCANTBEUSEDNOW]                   = COMPOUND_STRING("Items can't be used now.{PAUSE 64}"), // Not present in Gen 5+
    [STRINGID_USINGITEMSTATOFPKMNROSE]              = COMPOUND_STRING("The {B_LAST_ITEM}{B_BUFF2} boosted {B_SCR_NAME_WITH_PREFIX2}'s {B_BUFF1}!"),
    [STRINGID_USINGITEMSTATOFPKMNFELL]              = COMPOUND_STRING("The {B_LAST_ITEM}{B_BUFF2} lowered {B_SCR_NAME_WITH_PREFIX2}'s {B_BUFF1}!"), // This string does not exist in Gen 5+. Used to print more info that's otherwise obscured such as using Room Service
    [STRINGID_PKMNUSEDXTOGETPUMPED]                 = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} used the {B_LAST_ITEM} to get pumped!"),
    [STRINGID_PKMNSXMADEYUSELESS]                   = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX}'s {B_SCR_ABILITY} made {B_CURRENT_MOVE} useless!"), //not in gen 5+, ability popup
    [STRINGID_PKMNTRAPPEDBYSANDTOMB]                = COMPOUND_STRING("{B_EFF_NAME_WITH_PREFIX} became trapped by the quicksand!"),
    [STRINGID_EMPTYSTRING4]                         = COMPOUND_STRING(""),
    [STRINGID_ABOOSTED]                             = COMPOUND_STRING(" a boosted"),
    [STRINGID_PKMNSXINTENSIFIEDSUN]                 = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX}'s {B_SCR_ABILITY} intensified the sun's rays!"), //not in gen 5+, ability popup
    [STRINGID_YOUTHROWABALLNOWRIGHT]                = COMPOUND_STRING("You throw a Ball now, right? I… I'll do my best!"),
    [STRINGID_PKMNSXTOOKATTACK]                     = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX} took the attack!"),
    [STRINGID_PKMNCHOSEXASDESTINY]                  = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} chose Doom Desire as its destiny!"),
    [STRINGID_PKMNLOSTFOCUS]                        = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} lost its focus and couldn't move!"),
    [STRINGID_USENEXTPKMN]                          = COMPOUND_STRING("Use next Pokémon?"),
    [STRINGID_PKMNFLEDUSINGITS]                     = COMPOUND_STRING("{PLAY_SE SE_FLEE}{B_ATK_NAME_WITH_PREFIX} fled using its {B_LAST_ITEM}!\p"),
    [STRINGID_PKMNFLEDUSING]                        = COMPOUND_STRING("{PLAY_SE SE_FLEE}{B_ATK_NAME_WITH_PREFIX} fled using {B_ATK_ABILITY}!\p"), //not in gen 5+
    [STRINGID_PKMNWASDRAGGEDOUT]                    = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX} was dragged out!\p"),
    [STRINGID_PKMNSITEMNORMALIZEDSTATUS]            = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX}'s {B_LAST_ITEM} normalized its status!"), // Not in Gen 5+
    [STRINGID_TRAINER1USEDITEM]                     = COMPOUND_STRING("{B_ATK_TRAINER_NAME_WITH_CLASS} used {B_LAST_ITEM}!"),
    [STRINGID_BOXISFULL]                            = COMPOUND_STRING("The Box is full! You can't catch any more!\p"),
    [STRINGID_PKMNSXMADEITINEFFECTIVE]              = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} made it ineffective!"),
    [STRINGID_PKMNSXPREVENTSFLINCHING]              = COMPOUND_STRING("{B_EFF_NAME_WITH_PREFIX}'s {B_EFF_ABILITY} prevents flinching!"), //not in gen 5+, ability popup
    [STRINGID_PKMNALREADYHASBURN]                   = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX} is already burned!"),
    [STRINGID_PKMNSXBLOCKSY]                        = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX}'s {B_SCR_ABILITY} blocks {B_CURRENT_MOVE}!"), //not in gen 5+, ability popup
    [STRINGID_PKMNSXWOREOFF]                        = COMPOUND_STRING("{B_ATK_TEAM1} side's {B_BUFF1} wore off!"),
    [STRINGID_THEWALLSHATTERED]                     = COMPOUND_STRING("The wall shattered!"), //not in gen5+, uses "your teams light screen wore off!" etc instead
    [STRINGID_PKMNSXCUREDITSYPROBLEM]               = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX}'s {B_SCR_ABILITY} cured its {B_BUFF1} problem!"), //not in gen 5+, ability popup
    [STRINGID_ATTACKERCANTESCAPE]                   = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} can't escape!"),
    [STRINGID_PKMNOBTAINEDX]                        = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} obtained {B_BUFF1}."),
    [STRINGID_PKMNOBTAINEDX2]                       = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX} obtained {B_BUFF2}."),
    [STRINGID_PKMNOBTAINEDXYOBTAINEDZ]              = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} obtained {B_BUFF1}.\p{B_DEF_NAME_WITH_PREFIX} obtained {B_BUFF2}."),
    [STRINGID_BUTNOEFFECT]                          = COMPOUND_STRING("But it had no effect!"),
    [STRINGID_TWOENEMIESDEFEATED]                   = sText_TwoInGameTrainersDefeated,
    [STRINGID_TRAINER2LOSETEXT]                     = COMPOUND_STRING("{B_TRAINER2_LOSE_TEXT}"),
    [STRINGID_PKMNINCAPABLEOFPOWER]                 = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} appears incapable of using its power!"),
    [STRINGID_GLINTAPPEARSINEYE]                    = COMPOUND_STRING("A glint appears in {B_SCR_NAME_WITH_PREFIX2}'s eyes!"),
    [STRINGID_PKMNGETTINGINTOPOSITION]              = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} is getting into position!"),
    [STRINGID_PKMNBEGANGROWLINGDEEPLY]              = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} began growling deeply!"),
    [STRINGID_PKMNEAGERFORMORE]                     = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} is eager for more!"),
    [STRINGID_DEFEATEDOPPONENTBYREFEREE]            = COMPOUND_STRING("{B_PLAYER_MON1_NAME} defeated the opponent {B_OPPONENT_MON1_NAME} in a REFEREE's decision!"),
    [STRINGID_LOSTTOOPPONENTBYREFEREE]              = COMPOUND_STRING("{B_PLAYER_MON1_NAME} lost to the opponent {B_OPPONENT_MON1_NAME} in a REFEREE's decision!"),
    [STRINGID_TIEDOPPONENTBYREFEREE]                = COMPOUND_STRING("{B_PLAYER_MON1_NAME} tied the opponent {B_OPPONENT_MON1_NAME} in a REFEREE's decision!"),
    [STRINGID_QUESTIONFORFEITMATCH]                 = COMPOUND_STRING("Would you like to forfeit the match and quit now?"),
    [STRINGID_FORFEITEDMATCH]                       = COMPOUND_STRING("The match was forfeited."),
    [STRINGID_PKMNTRANSFERREDSOMEONESPC]            = gText_PkmnTransferredSomeonesPC,
    [STRINGID_PKMNTRANSFERREDLANETTESPC]            = gText_PkmnTransferredLanettesPC,
    [STRINGID_PKMNBOXSOMEONESPCFULL]                = gText_PkmnTransferredSomeonesPCBoxFull,
    [STRINGID_PKMNBOXLANETTESPCFULL]                = gText_PkmnTransferredLanettesPCBoxFull,
    [STRINGID_TRAINER1WINTEXT]                      = COMPOUND_STRING("{B_TRAINER1_WIN_TEXT}"),
    [STRINGID_TRAINER2WINTEXT]                      = COMPOUND_STRING("{B_TRAINER2_WIN_TEXT}"),
    [STRINGID_ENDUREDSTURDY]                        = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX} endured the hit using {B_DEF_ABILITY}!"),
    [STRINGID_POWERHERB]                            = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} became fully charged due to its {B_LAST_ITEM}!"),
    [STRINGID_HURTBYITEM]                           = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} was hurt by the {B_LAST_ITEM}!"),
    [STRINGID_GRAVITYINTENSIFIED]                   = COMPOUND_STRING("Gravity intensified!"),
    [STRINGID_TARGETWOKEUP]                         = COMPOUND_STRING("{B_EFF_NAME_WITH_PREFIX} woke up!"),
    [STRINGID_TAILWINDBLEW]                         = COMPOUND_STRING("A tailwind started blowing on {B_ATK_TEAM2} side!"),
    [STRINGID_PKMNWENTBACK]                         = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} went back to {B_ATK_TRAINER_NAME}!"),
    [STRINGID_PKMNCANTUSEITEMSANYMORE]              = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX} can't use items anymore!"),
    [STRINGID_PKMNFLUNG]                            = COMPOUND_STRING("{B_EFF_NAME_WITH_PREFIX} flung its {B_LAST_ITEM}!"),
    [STRINGID_PKMNPREVENTEDFROMHEALING]             = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX} was prevented from healing!"),
    [STRINGID_PKMNSWITCHEDATKANDDEF]                = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} switched its Attack and Defense!"),
    [STRINGID_PKMNSABILITYSUPPRESSED]               = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX}'s Ability was suppressed!"),
    [STRINGID_SHIELDEDFROMCRITICALHITS]             = COMPOUND_STRING("Lucky Chant shielded {B_ATK_TEAM2} side from critical hits!"), // Currently not present in Champions
    [STRINGID_PKMNACQUIREDABILITY]                  = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX} acquired {B_DEF_ABILITY}!"),
    [STRINGID_POISONSPIKESSCATTERED]                = COMPOUND_STRING("Toxic spikes were scattered on the ground all around {B_DEF_TEAM2} side!"),
    [STRINGID_PKMNSWITCHEDSTATCHANGES]              = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} switched stat changes with its target!"),
    [STRINGID_PKMNSURROUNDEDWITHVEILOFWATER]        = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} surrounded itself with a veil of water!"),
    [STRINGID_PKMNLEVITATEDONELECTROMAGNETISM]      = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} levitated with electromagnetism!"),
    [STRINGID_PKMNTWISTEDDIMENSIONS]                = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} twisted the dimensions!"),
    [STRINGID_POINTEDSTONESFLOAT]                   = COMPOUND_STRING("Pointed stones float in the air on {B_DEF_TEAM2} side!"),
    [STRINGID_TRAPPEDBYSWIRLINGMAGMA]               = COMPOUND_STRING("{B_EFF_NAME_WITH_PREFIX} became trapped by swirling magma!"),
    [STRINGID_VANISHEDINSTANTLY]                    = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} vanished instantly!"),
    [STRINGID_PROTECTEDTEAM]                        = COMPOUND_STRING("{B_CURRENT_MOVE} now protects {B_ATK_TEAM2} side!"),
    [STRINGID_SHAREDITSGUARD]                       = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} shared its guard with the target!"),
    [STRINGID_SHAREDITSPOWER]                       = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} shared its power with the target!"),
    [STRINGID_SWAPSDEFANDSPDEFOFALLPOKEMON]         = COMPOUND_STRING("It created a bizarre area in which Defense and Sp. Def stats are swapped!"),
    [STRINGID_BECAMENIMBLE]                         = COMPOUND_STRING("{B_EFF_NAME_WITH_PREFIX} became nimble!"),
    [STRINGID_HURLEDINTOTHEAIR]                     = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX} was hurled into the air!"),
    [STRINGID_HELDITEMSLOSEEFFECTS]                 = COMPOUND_STRING("It created a bizarre area in which Pokémon's held items lose their effects!"),
    [STRINGID_FELLSTRAIGHTDOWN]                     = COMPOUND_STRING("{B_EFF_NAME_WITH_PREFIX} fell straight down!"),
    [STRINGID_TARGETCHANGEDTYPE]                    = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX} transformed into the {B_BUFF1} type!"),
    [STRINGID_KINDOFFER]                            = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX} took the kind offer!"),
    [STRINGID_RESETSTARGETSSTATLEVELS]              = COMPOUND_STRING("{B_EFF_NAME_WITH_PREFIX}'s stat changes were removed!"),
    [STRINGID_ALLYSWITCHPOSITION]                   = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} and {B_SCR_NAME_WITH_PREFIX2} switched places!"),
    [STRINGID_REFLECTTARGETSTYPE]                   = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} became the same type as {B_DEF_NAME_WITH_PREFIX2}!"),
    [STRINGID_EMBARGOENDS]                          = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} can use items again!"),
    [STRINGID_ELECTROMAGNETISM]                     = COMPOUND_STRING("electromagnetism"),
    [STRINGID_BUFFERENDS]                           = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX}'s {B_BUFF1} wore off!"),
    [STRINGID_TELEKINESISENDS]                      = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} was freed from the telekinesis!"),
    [STRINGID_TAILWINDENDS]                         = COMPOUND_STRING("{B_ATK_TEAM1} side's tailwind petered out!"),
    [STRINGID_LUCKYCHANTENDS]                       = COMPOUND_STRING("{B_ATK_TEAM1} side's Lucky Chant wore off!"),
    [STRINGID_TRICKROOMENDS]                        = COMPOUND_STRING("The twisted dimensions returned to normal!"),
    [STRINGID_WONDERROOMENDS]                       = COMPOUND_STRING("Wonder Room wore off, and Defense and Sp. Def stats returned to normal!"),
    [STRINGID_MAGICROOMENDS]                        = COMPOUND_STRING("Magic Room wore off, and held items' effects returned to normal!"),
    [STRINGID_MUDSPORTENDS]                         = COMPOUND_STRING("The effects of Mud Sport have faded."),
    [STRINGID_WATERSPORTENDS]                       = COMPOUND_STRING("The effects of Water Sport have faded."),
    [STRINGID_GRAVITYENDS]                          = COMPOUND_STRING("Gravity returned to normal!"),
    [STRINGID_AQUARINGHEAL]                         = COMPOUND_STRING("A veil of water restored {B_ATK_NAME_WITH_PREFIX2}'s HP!"),
    [STRINGID_ELECTRICTERRAINENDS]                  = COMPOUND_STRING("The electricity disappeared from the battlefield."),
    [STRINGID_MISTYTERRAINENDS]                     = COMPOUND_STRING("The mist disappeared from the battlefield."),
    [STRINGID_PSYCHICTERRAINENDS]                   = COMPOUND_STRING("The weirdness disappeared from the battlefield!"),
    [STRINGID_GRASSYTERRAINENDS]                    = COMPOUND_STRING("The grass disappeared from the battlefield."),
    [STRINGID_TARGETABILITYSTATRAISE]               = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX}'s {B_DEF_ABILITY} {B_BUFF2}raised its {B_BUFF1}!"), // Not in Gen 5+
    [STRINGID_STATWASMAXEDOUT]                      = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} maxed its {B_BUFF1}!"),
    [STRINGID_ATTACKERABILITYSTATRAISE]             = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX}'s {B_ATK_ABILITY} {B_BUFF2}raised its {B_BUFF1}!"), // Not in Gen 5+
    [STRINGID_POISONHEALHPUP]                       = COMPOUND_STRING("The poisoning healed {B_ATK_NAME_WITH_PREFIX2} a little bit!"), // Not in Gen 5+
    [STRINGID_BADDREAMSDMG]                         = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX} is tormented!"),
    [STRINGID_MOLDBREAKERENTERS]                    = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} breaks the mold!"),
    [STRINGID_TERAVOLTENTERS]                       = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} is radiating a bursting aura!"),
    [STRINGID_TURBOBLAZEENTERS]                     = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} is radiating a blazing aura!"),
    [STRINGID_SLOWSTARTENTERS]                      = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} is slow to get going!"),
    [STRINGID_SLOWSTARTEND]                         = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} finally got its act together!"),
    [STRINGID_SOLARPOWERHPDROP]                     = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX}'s {B_ATK_ABILITY} takes its toll!"), // Not in Gen 5+
    [STRINGID_PKMNWASHURT]                          = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} was hurt!"),
    [STRINGID_ANTICIPATIONACTIVATES]                = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} shuddered!"),
    [STRINGID_FOREWARNACTIVATES]                    = COMPOUND_STRING("{B_SCR_ABILITY} alerted {B_SCR_NAME_WITH_PREFIX2} to {B_EFF_NAME_WITH_PREFIX2}'s {B_BUFF1}!"),
    [STRINGID_ICEBODYHPGAIN]                        = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX}'s {B_ATK_ABILITY} healed it a little bit!"), // Not in Gen 5+
    [STRINGID_SNOWWARNINGHAIL]                      = COMPOUND_STRING("It started to hail!"),
    [STRINGID_FRISKACTIVATES]                       = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} was frisked, revealing its {B_LAST_ITEM}!"),
    [STRINGID_UNNERVEENTERS]                        = COMPOUND_STRING("{B_EFF_TEAM1} side is too nervous to eat Berries!"),
    [STRINGID_HARVESTBERRY]                         = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} harvested its {B_LAST_ITEM}!"),
    [STRINGID_PROTEANTYPECHANGE]                    = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX}'s {B_ATK_ABILITY} transformed it into the {B_BUFF1} type!"),
    [STRINGID_SYMBIOSISITEMPASS]                    = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} shared its {B_LAST_ITEM} with {B_EFF_NAME_WITH_PREFIX2}!"),
    [STRINGID_STEALTHROCKDMG]                       = COMPOUND_STRING("Pointed stones dug into {B_SCR_NAME_WITH_PREFIX2}!"),
    [STRINGID_TOXICSPIKESABSORBED]                  = COMPOUND_STRING("The toxic spikes disappeared from the ground around {B_EFF_TEAM2} side!"),
    [STRINGID_TOXICSPIKESPOISONED]                  = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} was poisoned!"),
    [STRINGID_TOXICSPIKESBADLYPOISONED]             = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} was badly poisoned!"),
    [STRINGID_STICKYWEBSWITCHIN]                    = COMPOUND_STRING("{B_EFF_NAME_WITH_PREFIX} was caught in a sticky web!"),
    [STRINGID_HEALINGWISHCAMETRUE]                  = COMPOUND_STRING("The healing wish came true for {B_SCR_NAME_WITH_PREFIX2}!"),
    [STRINGID_HEALINGWISHHEALED]                    = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} regained health!"),
    [STRINGID_LUNARDANCECAMETRUE]                   = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} became cloaked in mystical moonlight!"),
    [STRINGID_CURSEDBODYDISABLED]                   = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX}'s {B_BUFF1} was disabled!"),
    [STRINGID_ATTACKERACQUIREDABILITY]              = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} acquired {B_ATK_ABILITY}!"), // Not in Gen 5+
    [STRINGID_TARGETABILITYSTATLOWER]               = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX}'s {B_DEF_ABILITY} {B_BUFF2}lowered its {B_BUFF1}!"), // Not in Gen 5+
    [STRINGID_TARGETSTATWONTGOHIGHER]               = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX}'s {B_BUFF1} won't go any higher!"),
    [STRINGID_PKMNMOVEBOUNCEDABILITY]               = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX}'s {B_CURRENT_MOVE} was bounced back!"),
    [STRINGID_IMPOSTERTRANSFORM]                    = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} transformed into {B_DEF_NAME_WITH_PREFIX2}!"),
    [STRINGID_ASSAULTVESTDOESNTALLOW]               = COMPOUND_STRING("The effects of the {B_LAST_ITEM} prevent status moves from being used!\p"),
    [STRINGID_GRAVITYPREVENTSUSAGE]                 = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} can't use {B_CURRENT_MOVE} because of gravity!\p"),
    [STRINGID_HEALBLOCKPREVENTSUSAGE]               = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} was prevented from healing!\p"),
    [STRINGID_NOTDONEYET]                           = COMPOUND_STRING("This move effect is not done yet!\p"),
    [STRINGID_STICKYWEBUSED]                        = COMPOUND_STRING("A sticky web has been laid out on the ground on {B_DEF_TEAM2} side!"),
    [STRINGID_QUASHSUCCESS]                         = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX}'s move was postponed!"),
    [STRINGID_PKMNBLEWAWAYTOXICSPIKES]              = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} blew away Toxic Spikes!"),
    [STRINGID_PKMNBLEWAWAYSTICKYWEB]                = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} blew away Sticky Web!"),
    [STRINGID_PKMNBLEWAWAYSTEALTHROCK]              = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} blew away Stealth Rock!"),
    [STRINGID_IONDELUGEON]                          = COMPOUND_STRING("A deluge of ions showers the battlefield!"),
    [STRINGID_TOPSYTURVYSWITCHEDSTATS]              = COMPOUND_STRING("All stat changes on {B_DEF_NAME_WITH_PREFIX2} were inverted!"),
    [STRINGID_TERRAINBECOMESMISTY]                  = COMPOUND_STRING("Mist swirled around the battlefield!"),
    [STRINGID_TERRAINBECOMESGRASSY]                 = COMPOUND_STRING("Grass grew to cover the battlefield!"),
    [STRINGID_TERRAINBECOMESELECTRIC]               = COMPOUND_STRING("An electric current ran across the battlefield!"),
    [STRINGID_TERRAINBECOMESPSYCHIC]                = COMPOUND_STRING("The battlefield got weird!"),
    [STRINGID_TARGETELECTRIFIED]                    = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX}'s moves have been electrified!"),
    [STRINGID_MEGAEVOREACTING]                      = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX}'s {B_LAST_ITEM} is reacting to {B_ATK_TRAINER_NAME}'s Mega Ring!"), //actually displays the type of mega ring in inventory, but we didnt implement them :(
    [STRINGID_MEGAEVOEVOLVED]                       = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} has Mega Evolved into Mega {B_BUFF1}!"),
    [STRINGID_DRASTICALLY]                          = gText_drastically,
    [STRINGID_SEVERELY]                             = gText_severely,
    [STRINGID_INFESTATION]                          = COMPOUND_STRING("{B_EFF_NAME_WITH_PREFIX} has been afflicted with an infestation by {B_SCR_NAME_WITH_PREFIX2}!"),
    [STRINGID_NOEFFECTONTARGET]                     = COMPOUND_STRING("It won't have any effect on {B_DEF_NAME_WITH_PREFIX2}!"),
    [STRINGID_BURSTINGFLAMESHIT]                    = COMPOUND_STRING("The bursting flames hit {B_EFF_NAME_WITH_PREFIX2}!"),
    [STRINGID_BESTOWITEMGIVING]                     = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX} received {B_LAST_ITEM} from {B_ATK_NAME_WITH_PREFIX2}!"),
    [STRINGID_THIRDTYPEADDED]                       = COMPOUND_STRING("{B_BUFF1} type was added to {B_DEF_NAME_WITH_PREFIX2}!"),
    [STRINGID_FELLFORFEINT]                         = COMPOUND_STRING("{B_EFF_NAME_WITH_PREFIX} fell for the feint!"),
    [STRINGID_POKEMONCANNOTUSEMOVE]                 = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} cannot use {B_CURRENT_MOVE}!"),
    [STRINGID_COVEREDINPOWDER]                      = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX} is covered in powder!"),
    [STRINGID_POWDEREXPLODES]                       = COMPOUND_STRING("When the flame touched the powder on the Pokémon, it exploded!"),
    [STRINGID_BELCHCANTSELECT]                      = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} hasn't eaten any held Berries, so it can't possibly belch!\p"),
    [STRINGID_SPECTRALTHIEFSTEAL]                   = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} stole the target's boosted stats!"),
    [STRINGID_GRAVITYGROUNDING]                     = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX} fell from the sky due to the gravity!"),
    [STRINGID_MISTYTERRAINPREVENTS]                 = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX} surrounds itself with a protective mist!"),
    [STRINGID_GRASSYTERRAINHEALS]                   = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} is healed by the grassy terrain!"),
    [STRINGID_ELECTRICTERRAINPREVENTS]              = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX} surrounds itself with electrified terrain!"),
    [STRINGID_PSYCHICTERRAINPREVENTS]               = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} is protected by the Psychic Terrain!"),
    [STRINGID_SAFETYGOGGLESPROTECTED]               = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} is not affected thanks to its {B_LAST_ITEM}!"),
    [STRINGID_FLOWERVEILPROTECTED]                  = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} surrounded itself with a veil of petals!"),
    [STRINGID_FLOWERVEILPROTECTEDTARGET]            = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX} surrounded itself with a veil of petals!"),
    [STRINGID_AROMAVEILPROTECTED]                   = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX} is protected by an aromatic veil!"),
    [STRINGID_CELEBRATEMESSAGE]                     = COMPOUND_STRING("Congratulations, {B_PLAYER_NAME}!"),
    [STRINGID_USEDINSTRUCTEDMOVE]                   = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX} followed {B_ATK_NAME_WITH_PREFIX2}'s instructions!"),
    [STRINGID_THROATCHOPENDS]                       = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} can use sound-based moves again!"),
    [STRINGID_PKMNCANTUSEMOVETHROATCHOP]            = COMPOUND_STRING("The effects of Throat Chop prevent {B_ATK_NAME_WITH_PREFIX2} from using certain moves!\p"),
    [STRINGID_LASERFOCUS]                           = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} concentrated intensely!"),
    [STRINGID_GEMACTIVATES]                         = COMPOUND_STRING("The {B_LAST_ITEM} strengthened {B_ATK_NAME_WITH_PREFIX2}'s power!"),
    [STRINGID_BERRYDMGREDUCES]                      = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX}'s {B_LAST_ITEM} lessened the damage it took!"),
    [STRINGID_AIRBALLOONFLOAT]                      = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} floats in the air with its Air Balloon!"),
    [STRINGID_AIRBALLOONPOP]                        = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX}'s Air Balloon popped!"),
    [STRINGID_INCINERATEBURN]                       = COMPOUND_STRING("{B_EFF_NAME_WITH_PREFIX}'s {B_LAST_ITEM} was burnt up!"),
    [STRINGID_BUGBITE]                              = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} stole and ate its target's {B_LAST_ITEM}!"),
    [STRINGID_ILLUSIONWOREOFF]                      = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX}'s illusion wore off!"),
    [STRINGID_ATTACKERCUREDTARGETSTATUS]            = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} cured {B_DEF_NAME_WITH_PREFIX2}'s problem!"),
    [STRINGID_ATTACKERLOSTFIRETYPE]                 = COMPOUND_STRING("{B_EFF_NAME_WITH_PREFIX} burned itself out!"),
    [STRINGID_HEALERCURE]                           = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} was cured of {B_SCR_NAME_WITH_PREFIX2}!"),
    [STRINGID_SCRIPTINGABILITYSTATRAISE]            = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX}'s {B_SCR_ABILITY} {B_BUFF2}raised its {B_BUFF1}!"),
    [STRINGID_RECEIVERABILITYTAKEOVER]              = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX}'s {B_SCR_ABILITY} was taken over!"),
    [STRINGID_PKNMABSORBINGPOWER]                   = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} is absorbing power!"),
    [STRINGID_NOONEWILLBEABLETORUNAWAY]             = COMPOUND_STRING("No one will be able to leave the battlefield during the next turn!"),
    [STRINGID_DESTINYKNOTACTIVATES]                 = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX} fell in love because of the {B_LAST_ITEM}!"),
    [STRINGID_CLOAKEDINAFREEZINGLIGHT]              = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} became cloaked in a freezing light!"),
    [STRINGID_CLEARAMULETWONTLOWERSTATS]            = COMPOUND_STRING("The effects of the {B_LAST_ITEM} held by {B_SCR_NAME_WITH_PREFIX2} prevents its stats from being lowered!"),
    [STRINGID_FERVENTWISHREACHED]                   = COMPOUND_STRING("{B_ATK_TRAINER_NAME}'s fervent wish has reached {B_ATK_NAME_WITH_PREFIX2}!"),
    [STRINGID_AIRLOCKACTIVATES]                     = COMPOUND_STRING("The effects of the weather disappeared."),
    [STRINGID_PRESSUREENTERS]                       = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} is exerting its pressure!"),
    [STRINGID_DARKAURAENTERS]                       = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} is radiating a dark aura!"),
    [STRINGID_FAIRYAURAENTERS]                      = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} is radiating a fairy aura!"),
    [STRINGID_AURABREAKENTERS]                      = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} reversed all other Pokémon's auras!"),
    [STRINGID_COMATOSEENTERS]                       = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} is drowsing!"),
    [STRINGID_SCREENCLEANERENTERS]                  = COMPOUND_STRING("All screens on the field were cleansed!"),
    [STRINGID_FETCHEDPOKEBALL]                      = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} found a {B_LAST_ITEM}!"),
    [STRINGID_ASANDSTORMKICKEDUP]                   = COMPOUND_STRING("A sandstorm kicked up!"),
    [STRINGID_PKMNSWILLPERISHIN3TURNS]              = COMPOUND_STRING("Both Pokémon will faint in three turns!"),
    [STRINGID_AURAFLAREDTOLIFE]                     = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX}'s aura flared to life!"),
    [STRINGID_ASONEENTERS]                          = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} has two Abilities!"),
    [STRINGID_CURIOUSMEDICINEENTERS]                = COMPOUND_STRING("{B_EFF_NAME_WITH_PREFIX}'s stat changes were removed!"),
    [STRINGID_CANACTFASTERTHANKSTO]                 = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} can act faster than normal, thanks to its {B_BUFF1}!"),
    [STRINGID_MICLEBERRYACTIVATES]                  = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} boosted the accuracy of its next move using {B_LAST_ITEM}!"),
    [STRINGID_PINAPBERRYACTIVATES]                  = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} used {B_LAST_ITEM} to power up its next move!"),
    [STRINGID_DURINBERRYACTIVATES]                  = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} was revived by its {B_LAST_ITEM}!"),
    [STRINGID_CORNNBERRYUSED]                       = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} used the {B_LAST_ITEM}!\nIt will gain more experience from this battle!"),
    [STRINGID_MAGOSTBERRYUSED]                      = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} used the {B_LAST_ITEM}!\nMore prize money is on the line next battle!"),
    [STRINGID_RABUTABERRYUSED]                      = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} used the {B_LAST_ITEM} and became friendlier!"),
    [STRINGID_RAZZBERRYUSED]                        = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} used the {B_LAST_ITEM} and honed its aim!"),
    [STRINGID_BELUEBERRYUSED]                       = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} used the {B_LAST_ITEM}!\nIt's braced against critical hits!"),
    [STRINGID_PKMNSHOOKOFFTHETAUNT]                 = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} shook off the taunt!"),
    [STRINGID_PKMNGOTOVERITSINFATUATION]            = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} got over its infatuation!"),
    [STRINGID_ITEMCANNOTBEREMOVED]                  = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX}'s item cannot be removed!"),
    [STRINGID_STICKYBARBTRANSFER]                   = COMPOUND_STRING("The {B_LAST_ITEM} attached itself to {B_ATK_NAME_WITH_PREFIX2}!"),
    [STRINGID_PKMNBURNHEALED]                       = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX}'s burn was cured!"),
    [STRINGID_REDCARDACTIVATE]                      = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} held up its Red Card against {B_ATK_NAME_WITH_PREFIX2}!"),
    [STRINGID_EJECTBUTTONACTIVATE]                  = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} is switched out with the {B_LAST_ITEM}!"),
    [STRINGID_ATKGOTOVERINFATUATION]                = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} cured its infatuation status using its {B_LAST_ITEM}!"),
    [STRINGID_TORMENTEDNOMORE]                      = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} is no longer tormented!"),
    [STRINGID_HEALBLOCKEDNOMORE]                    = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} is no longer prevented from healing!"),
    [STRINGID_ATTACKERBECAMEFULLYCHARGED]           = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} became fully charged due to its bond with its trainer!\p"),
    [STRINGID_ATTACKERBECAMEASHSPECIES]             = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} became Ash-Greninja!\p"),
    [STRINGID_EXTREMELYHARSHSUNLIGHT]               = COMPOUND_STRING("The sunlight turned extremely harsh!"),
    [STRINGID_EXTREMESUNLIGHTFADED]                 = COMPOUND_STRING("The extremely harsh sunlight faded!"),
    [STRINGID_MOVEEVAPORATEDINTHEHARSHSUNLIGHT]     = COMPOUND_STRING("The Water-type attack evaporated in the extremely harsh sunlight!"),
    [STRINGID_EXTREMELYHARSHSUNLIGHTWASNOTLESSENED] = COMPOUND_STRING("The extremely harsh sunlight was not lessened at all!"),
    [STRINGID_HEAVYRAIN]                            = COMPOUND_STRING("A heavy rain began to fall!"),
    [STRINGID_HEAVYRAINLIFTED]                      = COMPOUND_STRING("The heavy rain has lifted!"),
    [STRINGID_MOVEFIZZLEDOUTINTHEHEAVYRAIN]         = COMPOUND_STRING("The Fire-type attack fizzled out in the heavy rain!"),
    [STRINGID_NORELIEFROMHEAVYRAIN]                 = COMPOUND_STRING("There is no relief from this heavy rain!"),
    [STRINGID_MYSTERIOUSAIRCURRENT]                 = COMPOUND_STRING("Mysterious strong winds are protecting Flying-type Pokémon!"),
    [STRINGID_STRONGWINDSDISSIPATED]                = COMPOUND_STRING("The mysterious strong winds have dissipated!"),
    [STRINGID_MYSTERIOUSAIRCURRENTBLOWSON]          = COMPOUND_STRING("The mysterious strong winds blow on regardless!"),
    [STRINGID_ATTACKWEAKENEDBSTRONGWINDS]           = COMPOUND_STRING("The mysterious strong winds weakened the attack!"),
    [STRINGID_STUFFCHEEKSCANTSELECT]                = COMPOUND_STRING("It can't use the move because it doesn't have a Berry!\p"),
    [STRINGID_PKMNREVERTEDTOPRIMAL]                 = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX}'s Primal Reversion! It reverted to its primal state!"),
    [STRINGID_BUTPOKEMONCANTUSETHEMOVE]             = COMPOUND_STRING("But {B_ATK_NAME_WITH_PREFIX2} can't use the move!"),
    [STRINGID_BUTHOOPACANTUSEIT]                    = COMPOUND_STRING("But {B_ATK_NAME_WITH_PREFIX2} can't use it the way it is now!"),
    [STRINGID_BROKETHROUGHPROTECTION]               = COMPOUND_STRING("It broke through {B_EFF_NAME_WITH_PREFIX2}'s protection!"),
    [STRINGID_ABILITYALLOWSONLYMOVE]                = COMPOUND_STRING("{B_ATK_ABILITY} only allows the use of {B_CURRENT_MOVE}!\p"),
    [STRINGID_SWAPPEDABILITIES]                     = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX} swapped Abilities with its target!"),
    [STRINGID_PKMNHEALEDPOISON]                     = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX} was cured of its poisoning!"),
    [STRINGID_BATTLERTYPECHANGEDTO]                 = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX}'s type changed to {B_BUFF1}!"),
    [STRINGID_BOTHCANNOLONGERESCAPE]                = COMPOUND_STRING("Neither Pokémon can run away!"),
    [STRINGID_CANTESCAPEDUETOUSEDMOVE]              = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} can no longer escape because it used No Retreat!"),
    [STRINGID_PKMNBECAMEWEAKERTOFIRE]               = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX} became weaker to fire!"),
    [STRINGID_ABOUTTOUSEPOLTERGEIST]                = COMPOUND_STRING("{B_EFF_NAME_WITH_PREFIX} is about to be attacked by its {B_LAST_ITEM}!"),
    [STRINGID_CANTESCAPEBECAUSEOFCURRENTMOVE]       = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX} can no longer escape because of Octolock!"),
    [STRINGID_NEUTRALIZINGGASENTERS]                = COMPOUND_STRING("Neutralizing gas filled the area!"),
    [STRINGID_NEUTRALIZINGGASOVER]                  = COMPOUND_STRING("The effects of the neutralizing gas wore off!"),
    [STRINGID_TARGETTOOHEAVY]                       = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX} is too heavy to be lifted!"),
    [STRINGID_PKMNTOOKTARGETHIGH]                   = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} took {B_DEF_NAME_WITH_PREFIX2} into the sky!"),
    [STRINGID_PKMNINSNAPTRAP]                       = COMPOUND_STRING("{B_EFF_NAME_WITH_PREFIX} got trapped by a snap trap!"),
    [STRINGID_METEORBEAMCHARGING]                   = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} is overflowing with space power!"),
    [STRINGID_HEATUPBEAK]                           = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} started heating up its beak!"),
    [STRINGID_COURTCHANGE]                          = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} swapped the battle effects affecting each side of the field!"),
    [STRINGID_ZPOWERSURROUNDS]                      = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} surrounded itself with its Z-Power!"),
    [STRINGID_ZMOVEUNLEASHED]                       = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} unleashes its full-force Z-Move!"),
    [STRINGID_ZMOVERESETSSTATS]                     = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} returned its decreased stats to normal using its Z-Power!"),
    [STRINGID_ZMOVEALLSTATSUP]                      = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} boosted its stats using its Z-Power!"),
    [STRINGID_ZMOVEZBOOSTCRIT]                      = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} boosted its critical-hit ratio using its Z-Power!"),
    [STRINGID_ZMOVERESTOREHP]                       = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} restored its HP using its Z-Power!"),
    [STRINGID_ZMOVESTATUP]                          = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} boosted its stats using its Z-Power!"),
    [STRINGID_ZMOVEHPTRAP]                          = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX}'s HP was restored by the Z-Power!"),
    [STRINGID_ATTACKEREXPELLEDTHEPOISON]            = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} managed to expel the poison so you wouldn't worry!"),
    [STRINGID_ATTACKERSHOOKITSELFAWAKE]             = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} shook itself awake so you wouldn't worry!"),
    [STRINGID_ATTACKERBROKETHROUGHPARALYSIS]        = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} gathered all its energy to break through its paralysis so you wouldn't worry!"),
    [STRINGID_ATTACKERHEALEDITSBURN]                = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} cured its burn through sheer determination so you wouldn't worry!"),
    [STRINGID_ATTACKERMELTEDTHEICE]                 = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} melted the ice with its fiery determination so you wouldn't worry!"),
    [STRINGID_TARGETTOUGHEDITOUT]                   = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX} toughed it out so you wouldn't feel sad!"),
    [STRINGID_ATTACKERLOSTELECTRICTYPE]             = COMPOUND_STRING("{B_EFF_NAME_WITH_PREFIX} used up all its electricity!"),
    [STRINGID_ATTACKERSWITCHEDSTATWITHTARGET]       = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} switched {B_BUFF1} with its target!"),
    [STRINGID_BEINGHITCHARGEDPKMNWITHPOWER]         = COMPOUND_STRING("Being hit by {B_CURRENT_MOVE} charged {B_EFF_NAME_WITH_PREFIX2} with power!"),
    [STRINGID_SUNLIGHTACTIVATEDABILITY]             = COMPOUND_STRING("The harsh sunlight activated {B_SCR_NAME_WITH_PREFIX2}'s Protosynthesis!"),
    [STRINGID_ORICHALCUMPULSEACTIVATES]             = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} turned the sunlight harsh, sending its ancient pulse into a frenzy!"),
    [STRINGID_ORICHALCUMPULSEACTIVATESINSUN]        = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} basked in the sunlight, sending its ancient pulse into a frenzy!"),
    [STRINGID_STATWASHEIGHTENED]                    = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX}'s {B_BUFF1} was heightened!"),
    [STRINGID_ELECTRICTERRAINACTIVATEDABILITY]      = COMPOUND_STRING("The Electric Terrain activated {B_SCR_NAME_WITH_PREFIX2}'s Quark Drive!"),
    [STRINGID_HADRONENGINEACTIVATES]                = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} turned the ground into Electric Terrain, energizing its futuristic engine!"),
    [STRINGID_HADRONENGINEACTIVATESINTERRAIN]       = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} used the Electric Terrain to energize its futuristic engine!"),
    [STRINGID_ABILITYWEAKENEDSURROUNDINGMONSSTAT]   = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX}'s {B_SCR_ABILITY} weakened the {B_BUFF1} of all surrounding Pokémon!\p"),
    [STRINGID_ATTACKERGAINEDSTRENGTHFROMTHEFALLEN]  = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} gained strength from the fallen!"),
    [STRINGID_PKMNSABILITYPREVENTSABILITY]          = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX}'s {B_SCR_ABILITY} prevents {B_DEF_NAME_WITH_PREFIX2}'s {B_DEF_ABILITY} from working!"), //not in gen 5+, ability popup
    [STRINGID_PREPARESHELLTRAP]                     = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} set a shell trap!"),
    [STRINGID_SHELLTRAPDIDNTWORK]                   = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX}'s shell trap didn't work!"),
    [STRINGID_SPIKESDISAPPEAREDFROMTEAM]            = COMPOUND_STRING("The spikes disappeared from the ground around {B_ATK_TEAM2} side!"),
    [STRINGID_TOXICSPIKESDISAPPEAREDFROMTEAM]       = COMPOUND_STRING("The toxic spikes disappeared from the ground around {B_ATK_TEAM2} side!"),
    [STRINGID_STICKYWEBDISAPPEAREDFROMTEAM]         = COMPOUND_STRING("The sticky web has disappeared from the ground on {B_ATK_TEAM2} side!"),
    [STRINGID_STEALTHROCKDISAPPEAREDFROMTEAM]       = COMPOUND_STRING("The pointed stones disappeared from {B_ATK_TEAM2} side!"),
    [STRINGID_COULDNTFULLYPROTECT]                  = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX} couldn't fully protect itself and got hurt!"),
    [STRINGID_STOCKPILEDEFFECTWOREOFF]              = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX}'s stockpiled effect wore off!"),
    [STRINGID_PKMNREVIVEDREADYTOFIGHT]              = COMPOUND_STRING("{B_BUFF1} was revived and is ready to fight again!"),
    [STRINGID_ITEMRESTOREDSPECIESHEALTH]            = COMPOUND_STRING("{B_BUFF1} had its HP restored."),
    [STRINGID_ITEMCUREDSPECIESSTATUS]               = COMPOUND_STRING("{B_BUFF1} had its status healed!"), // Not in Gen 5+
    [STRINGID_ITEMRESTOREDSPECIESPP]                = COMPOUND_STRING("The PP of {B_BUFF1}'s {B_BUFF2} was restored!"),
    [STRINGID_THUNDERCAGETRAPPED]                   = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} trapped {B_EFF_NAME_WITH_PREFIX2}!"),
    [STRINGID_PKMNHURTBYFROSTBITE]                  = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} was hurt by its frostbite!"),
    [STRINGID_PKMNGOTFROSTBITE]                     = COMPOUND_STRING("{B_EFF_NAME_WITH_PREFIX} got frostbite!"),
    [STRINGID_PKMNSITEMHEALEDFROSTBITE]             = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX}'s {B_LAST_ITEM} cured its frostbite!"),
    [STRINGID_ATTACKERHEALEDITSFROSTBITE]           = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} cured its frostbite through sheer determination so you wouldn't worry!"),
    [STRINGID_PKMNFROSTBITEHEALED]                  = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX}'s frostbite was cured!"),
    [STRINGID_PKMNFROSTBITEHEALEDBY]                = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX}'s {B_CURRENT_MOVE} cured its frostbite!"),
    [STRINGID_MIRRORHERBCOPIED]                     = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} used its Mirror Herb to mirror its opponent's stat changes!"),
    [STRINGID_STARTEDSNOW]                          = COMPOUND_STRING("It started to snow!"),
    [STRINGID_SNOWCONTINUES]                        = COMPOUND_STRING("Snow continues to fall."), //not in gen 5+ (lol)
    [STRINGID_SNOWSTOPPED]                          = COMPOUND_STRING("The snow stopped."),
    [STRINGID_SNOWWARNINGSNOW]                      = COMPOUND_STRING("It started to snow!"),
    [STRINGID_PKMNITEMMELTED]                       = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} corroded {B_DEF_NAME_WITH_PREFIX2}'s {B_LAST_ITEM}!"),
    [STRINGID_ULTRABURSTREACTING]                   = COMPOUND_STRING("Bright light is about to burst out of {B_ATK_NAME_WITH_PREFIX2}!"),
    [STRINGID_ULTRABURSTCOMPLETED]                  = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} regained its true power through Ultra Burst!"),
    [STRINGID_TEAMGAINEDEXP]                        = COMPOUND_STRING("The rest of your team gained Exp. Points!\p"),
    [STRINGID_CURRENTMOVECANTSELECT]                = COMPOUND_STRING("{B_BUFF1} cannot be used!\p"),
    [STRINGID_TARGETISBEINGSALTCURED]               = COMPOUND_STRING("{B_EFF_NAME_WITH_PREFIX} is being salt cured!"),
    [STRINGID_TARGETISHURTBYSALTCURE]               = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} is hurt by {B_BUFF1}!"),
    [STRINGID_TARGETCOVEREDINSTICKYCANDYSYRUP]      = COMPOUND_STRING("{B_EFF_NAME_WITH_PREFIX} got covered in sticky candy syrup!"),
    [STRINGID_SHARPSTEELFLOATS]                     = COMPOUND_STRING("Sharp-pointed pieces of steel started floating around {B_DEF_TEAM2} Pokémon!"),
    [STRINGID_SHARPSTEELDMG]                        = COMPOUND_STRING("The sharp steel bit into {B_DEF_NAME_WITH_PREFIX2}!"),
    [STRINGID_PKMNBLEWAWAYSHARPSTEEL]               = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} blew away sharp steel!"),
    [STRINGID_SHARPSTEELDISAPPEAREDFROMTEAM]        = COMPOUND_STRING("The pieces of steel surrounding {B_ATK_TEAM2} Pokémon disappeared!"),
    [STRINGID_TEAMTRAPPEDWITHVINES]                 = COMPOUND_STRING("{B_EFF_TEAM1} Pokémon got trapped with vines!"),
    [STRINGID_PKMNHURTBYVINES]                      = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} is hurt by G-Max Vine Lash's ferocious beating!"),
    [STRINGID_TEAMCAUGHTINVORTEX]                   = COMPOUND_STRING("{B_EFF_TEAM1} Pokémon got caught in a vortex of water!"),
    [STRINGID_PKMNHURTBYVORTEX]                     = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} is hurt by G-Max Cannonade's vortex!"),
    [STRINGID_TEAMSURROUNDEDBYFIRE]                 = COMPOUND_STRING("{B_EFF_TEAM1} Pokémon were surrounded by fire!"),
    [STRINGID_PKMNBURNINGUP]                        = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} is burning up within G-Max Wildfire's flames!"),
    [STRINGID_TEAMSURROUNDEDBYROCKS]                = COMPOUND_STRING("{B_EFF_TEAM1} Pokémon became surrounded by rocks!"),
    [STRINGID_PKMNHURTBYROCKSTHROWN]                = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} is hurt by rocks thrown out by G-Max Volcalith!"),
    [STRINGID_MOVEBLOCKEDBYDYNAMAX]                 = COMPOUND_STRING("The move was blocked by the power of Dynamax!"),
    [STRINGID_ZEROTOHEROTRANSFORMATION]             = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} underwent a heroic transformation!"),
    [STRINGID_THETWOMOVESBECOMEONE]                 = COMPOUND_STRING("The two moves have become one! It's a combined move!{PAUSE 16}"),
    [STRINGID_ARAINBOWAPPEAREDONSIDE]               = COMPOUND_STRING("A rainbow appeared in the sky on {B_EFF_TEAM2} side!"),
    [STRINGID_THERAINBOWDISAPPEARED]                = COMPOUND_STRING("The rainbow on {B_ATK_TEAM2} side disappeared!"),
    [STRINGID_WAITINGFORPARTNERSMOVE]               = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} is waiting for {B_ATK_PARTNER_NAME}'s move…{PAUSE 16}"),
    [STRINGID_SEAOFFIREENVELOPEDSIDE]               = COMPOUND_STRING("A sea of fire enveloped {B_EFF_TEAM2} side!"),
    [STRINGID_HURTBYTHESEAOFFIRE]                   = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} was hurt by the sea of fire!"),
    [STRINGID_THESEAOFFIREDISAPPEARED]              = COMPOUND_STRING("The sea of fire around {B_ATK_TEAM2} side disappeared!"),
    [STRINGID_SWAMPENVELOPEDSIDE]                   = COMPOUND_STRING("A swamp enveloped {B_EFF_TEAM2} side!"),
    [STRINGID_THESWAMPDISAPPEARED]                  = COMPOUND_STRING("The swamp around {B_ATK_TEAM2} side disappeared!"),
    [STRINGID_PKMNTELLCHILLINGRECEPTIONJOKE]        = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} is preparing to tell a chillingly bad joke!"),
    [STRINGID_HOSPITALITYRESTORATION]               = COMPOUND_STRING("{B_EFF_NAME_WITH_PREFIX} drank down all the matcha that {B_SCR_NAME_WITH_PREFIX2} made!"),
    [STRINGID_ELECTROSHOTCHARGING]                  = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} absorbed electricity!"),
    [STRINGID_ITEMWASUSEDUP]                        = COMPOUND_STRING("The {B_LAST_ITEM} was used up…"),
    [STRINGID_ATTACKERLOSTITSTYPE]                  = COMPOUND_STRING("{B_EFF_NAME_WITH_PREFIX} lost its {B_BUFF1} type!"),
    [STRINGID_SHEDITSTAIL]                          = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} shed its tail to create a decoy!"),
    [STRINGID_CLOAKEDINAHARSHLIGHT]                 = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} became cloaked in a harsh light!"),
    [STRINGID_SUPERSWEETAROMAWAFTS]                 = COMPOUND_STRING("A supersweet aroma is wafting from the syrup covering {B_EFF_NAME_WITH_PREFIX2}!"),
    [STRINGID_DIMENSIONSWERETWISTED]                = COMPOUND_STRING("The dimensions were twisted!"),
    [STRINGID_BIZARREARENACREATED]                  = COMPOUND_STRING("A bizarre area was created in which Pokémon's held items lose their effects!"),
    [STRINGID_BIZARREAREACREATED]                   = COMPOUND_STRING("A bizarre area was created in which Defense and Sp. Def stats are swapped!"),
    [STRINGID_TIDYINGUPCOMPLETE]                    = COMPOUND_STRING("Tidying up complete!"),
    [STRINGID_PKMNTERASTALLIZEDINTO]                = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} terastallized into the {B_BUFF1} type!"), // Does not exist, meant to mimic form change strings
    [STRINGID_BOOSTERENERGYACTIVATES]               = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} used its {B_LAST_ITEM} to activate {B_SCR_ABILITY}!"),
    [STRINGID_FOGCREPTUP]                           = COMPOUND_STRING("Fog crept up as thick as soup!"),
    [STRINGID_FOGISDEEP]                            = COMPOUND_STRING("The fog is deep…"),
    [STRINGID_FOGLIFTED]                            = COMPOUND_STRING("The fog lifted."),
    [STRINGID_PKMNMADESHELLGLEAM]                   = COMPOUND_STRING("{B_DEF_NAME_WITH_PREFIX} made its shell gleam! It's distorting type matchups!"),
    [STRINGID_FICKLEBEAMDOUBLED]                    = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} is going all out for this attack!"),
    [STRINGID_COMMANDERACTIVATES]                   = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} was swallowed by {B_BUFF1} and became {B_BUFF1}'s commander!"),
    [STRINGID_POKEFLUTECATCHY]                      = COMPOUND_STRING("{B_PLAYER_NAME} played the {B_LAST_ITEM}.\pNow, that's a catchy tune!"),
    [STRINGID_POKEFLUTE]                            = COMPOUND_STRING("{B_PLAYER_NAME} played the {B_LAST_ITEM}."),
    [STRINGID_MONHEARINGFLUTEAWOKE]                 = COMPOUND_STRING("The Pokémon hearing the flute awoke!"),
    [STRINGID_SUNLIGHTISHARSH]                      = COMPOUND_STRING("The sunlight is harsh!"),
    [STRINGID_ITISHAILING]                          = COMPOUND_STRING("It's hailing!"),
    [STRINGID_ITISSNOWING]                          = COMPOUND_STRING("It's snowing!"),
    [STRINGID_ISCOVEREDWITHGRASS]                   = COMPOUND_STRING("The battlefield is covered with grass!"),
    [STRINGID_MISTSWIRLSAROUND]                     = COMPOUND_STRING("Mist swirls around the battlefield!"),
    [STRINGID_ELECTRICCURRENTISRUNNING]             = COMPOUND_STRING("An electric current is running across the battlefield!"),
    [STRINGID_SEEMSWEIRD]                           = COMPOUND_STRING("The battlefield seems weird!"),
    [STRINGID_WAGGLINGAFINGER]                      = COMPOUND_STRING("Waggling a finger let it use {B_CURRENT_MOVE}!"),
    [STRINGID_BLOCKEDBYSLEEPCLAUSE]                 = COMPOUND_STRING("Sleep Clause kept {B_DEF_NAME_WITH_PREFIX2} awake!"),
    [STRINGID_SUPEREFFECTIVETWOFOES]                = COMPOUND_STRING("It's super effective on {B_DEF_NAME_WITH_PREFIX2} and {B_DEF_PARTNER_NAME}!"),
    [STRINGID_NOTVERYEFFECTIVETWOFOES]              = COMPOUND_STRING("It's not very effective on {B_DEF_NAME_WITH_PREFIX2} and {B_DEF_PARTNER_NAME}."),
    [STRINGID_ITDOESNTAFFECTTWOFOES]                = COMPOUND_STRING("It doesn't affect {B_DEF_NAME_WITH_PREFIX2} and {B_DEF_PARTNER_NAME}…"),
    [STRINGID_SENDCAUGHTMONPARTYORBOX]              = COMPOUND_STRING("Add {B_DEF_NAME} to your party?"),
    [STRINGID_PKMNSENTTOPCAFTERCATCH]               = gText_PkmnSentToPCAfterCatch,
    [STRINGID_NUZLOCKECANTCATCH]                    = gText_NuzlockeNoCatch,
    [STRINGID_DRAFTCANTCATCH]                       = gText_DraftNoCatch,
    [STRINGID_MONOTYPECANTCATCH]                    = gText_MonoTypeNoCatch,
    [STRINGID_MONOGENCANTCATCH]                     = gText_MonoGenNoCatch,
    [STRINGID_PKMNDYNAMAXED]                        = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} grew huge into its Dynamax form!"),
    [STRINGID_PKMNGIGANTAMAXED]                     = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} grew huge into its Gigantamax form!"),
    [STRINGID_TIMETODYNAMAX]                        = COMPOUND_STRING("Time to Dynamax!"),
    [STRINGID_TIMETOGIGANTAMAX]                     = COMPOUND_STRING("Time to Gigantamax!"),
    [STRINGID_QUESTIONFORFEITBATTLE]                = COMPOUND_STRING("Would you like to give up on this battle and quit now? Quitting the battle is the same as losing the battle."),
    [STRINGID_POWERCONSTRUCTPRESENCEOFMANY]         = COMPOUND_STRING("You sense the presence of many!"),
    [STRINGID_POWERCONSTRUCTTRANSFORM]              = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} transformed into its Complete Forme!"),
    [STRINGID_ABILITYSHIELDPROTECTS]                = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX}'s Ability is protected by the effects of its {B_LAST_ITEM}!"),
    [STRINGID_MONTOOSCAREDTOMOVE]                   = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} is too scared to move!"),
    [STRINGID_GHOSTGETOUTGETOUT]                    = COMPOUND_STRING("GHOST: Get out…… Get out……"),
    [STRINGID_SILPHSCOPEUNVEILED]                   = COMPOUND_STRING("SILPH SCOPE unveiled the GHOST's\nidentity!"),
    [STRINGID_GHOSTWASMAROWAK]                      = COMPOUND_STRING("The GHOST was MAROWAK!\p"),
    [STRINGID_TRAINER1MON1COMEBACK]                 = COMPOUND_STRING("{B_TRAINER1_NAME}: {B_OPPONENT_MON1_NAME}, come back!"),
    [STRINGID_THREWROCK]                            = COMPOUND_STRING("{B_PLAYER_NAME} threw a ROCK\nat the {B_OPPONENT_MON1_NAME}!"),
    [STRINGID_THREWBAIT]                            = COMPOUND_STRING("{B_PLAYER_NAME} threw some BAIT\nat the {B_OPPONENT_MON1_NAME}!"),
    [STRINGID_PKMNANGRY]                            = COMPOUND_STRING("{B_OPPONENT_MON1_NAME} is angry!"),
    [STRINGID_PKMNEATING]                           = COMPOUND_STRING("{B_OPPONENT_MON1_NAME} is eating!"),
    [STRINGID_PKMNDISGUISEWASBUSTED]                = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX}'s disguise was busted!"),
    [STRINGID_ZENMODETRIGGERED]                     = COMPOUND_STRING("{B_SCR_ABILITY} triggered!"),
    [STRINGID_ZENMODEENDED]                         = COMPOUND_STRING("{B_SCR_ABILITY} ended!"),
    [STRINGID_SCRCUREDPARALYSIS]                    = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} was cured of paralysis!"),
    [STRINGID_SCRCUREDPOISON]                       = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} was cured of its poisoning!"),
    [STRINGID_SCRCUREDBURN]                         = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX}'s burn was cured!"),
    [STRINGID_SCRCUREDSLEEP]                        = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} woke up!"),
    [STRINGID_SCRCUREDCONFUSION]                    = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX} snapped out of its confusion!"),
    [STRINGID_PARTYCUREDPARALYSIS]                  = COMPOUND_STRING("{B_BUFF1} was cured of paralysis!"),
    [STRINGID_PARTYCUREDPOISON]                     = COMPOUND_STRING("{B_BUFF1} was cured of its poisoning!"),
    [STRINGID_PARTYCUREDBURN]                       = COMPOUND_STRING("{B_BUFF1}'s burn was cured!"),
    [STRINGID_PARTYCUREDSLEEP]                      = COMPOUND_STRING("{B_BUFF1} woke up!"),
    [STRINGID_PARTYCUREDFREEZE]                     = COMPOUND_STRING("{B_BUFF1} thawed out!"),
    [STRINGID_PARTYCUREDFROSTBITE]                  = COMPOUND_STRING("{B_BUFF1}'s frostbite was cured!"),
    [STRINGID_PKMNATKNOTLOWERED]                    = COMPOUND_STRING("{B_SCR_NAME_WITH_PREFIX}'s Attack was not lowered!"),
    [STRINGID_VICTORYCATCH]                         = COMPOUND_STRING("{B_DEF_NAME} is weak!\nThrow a Poké Ball now!"),
    [STRINGID_CANTUSEMOVE]                          = COMPOUND_STRING("This move can't be used!\p"),
    [STRINGID_REFLECTWOREOFF]                       = COMPOUND_STRING("{B_DEF_TEAM1} side's Reflect wore off!"),
    [STRINGID_LIGHTSCREENWOREOFF]                   = COMPOUND_STRING("{B_DEF_TEAM1} side's Light Screen wore off!"),
    [STRINGID_AURORAVEILWOREOFF]                    = COMPOUND_STRING("{B_DEF_TEAM1} side's Aurora Veil wore off!"),
    [STRINGID_MOSTLYINEFFECTIVE]                    = COMPOUND_STRING("It's mostly ineffective…"),
    [STRINGID_EXTREMELYEFFECTIVE]                   = COMPOUND_STRING("It's extremely effective!"),
    [STRINGID_NOTVERYEFFECTIVEONDEF]                = COMPOUND_STRING("It's not very effective on {B_DEF_NAME_WITH_PREFIX2}."),
    [STRINGID_SUPEREFFECTIVEONDEF]                  = COMPOUND_STRING("It's super effective on {B_DEF_NAME_WITH_PREFIX2}!"),
    [STRINGID_MOSTLYINEFFECTIVEONDEF]               = COMPOUND_STRING("It's mostly ineffective on {B_DEF_NAME_WITH_PREFIX2}."),
    [STRINGID_EXTREMELYEFFECTIVEONDEF]              = COMPOUND_STRING("It's extremely effective on {B_DEF_NAME_WITH_PREFIX2}!"),
    [STRINGID_EXTREMELYEFFECTIVETWOFOES]            = COMPOUND_STRING("It's extremely effective on {B_DEF_NAME_WITH_PREFIX2} and {B_DEF_PARTNER_NAME}!"),
    [STRINGID_MOSTLYINEFFECTIVETWOFOES]             = COMPOUND_STRING("It's mostly ineffective on {B_DEF_NAME_WITH_PREFIX2} and {B_DEF_PARTNER_NAME}."),
    [STRINGID_CRITICALHITONDEF]                     = COMPOUND_STRING("A critical hit on {B_DEF_NAME_WITH_PREFIX2}!"),
    [STRINGID_S]                                    = COMPOUND_STRING("s"),
    [STRINGID_LOSTSOMEOFITSHP]                      = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} lost some of its HP!"),
    [STRINGID_BELCHCANTUSE]                         = COMPOUND_STRING("{B_ATK_NAME_WITH_PREFIX} hasn't eaten any held Berries, so it can't possibly belch!\p"),

    // Encounter properties.
    [STRINGID_ENCCANTCATCHYET]                      = COMPOUND_STRING("It's far too strong to be caught\nright now!"),

    // Stage 15 example encounters (outline Sec33/Sec34).
    [STRINGID_ENCLEGENDARYGATHERSSTRENGTH]          = COMPOUND_STRING("The legendary gathers its\nstrength!"),
    [STRINGID_ENCMYSTERIOUSBARRIERSURROUNDS]        = COMPOUND_STRING("A mysterious barrier surrounds\nit!"),
    [STRINGID_ENCLEGENDARYWEAKENED]                 = COMPOUND_STRING("The barrier shatters! It's weak\nenough to catch!"),
    [STRINGID_ENCTRAINERPUSHEDTHISFAR]              = COMPOUND_STRING("You've pushed me this far..."),
    [STRINGID_ENCTRAINERSHOWTRUEPOWER]              = COMPOUND_STRING("Then I'll show you its true\npower!"),

    // Stage 16 example encounter (Storm_Herald).
    [STRINGID_ENCSTORMHERALDINTRO]                  = COMPOUND_STRING("The sky churns as it senses a\nchallenger!"),
    [STRINGID_ENCSTORMHERALDSURGE]                  = COMPOUND_STRING("It calls forth the storm's fury!"),
    [STRINGID_ENCSTORMHERALDDESPERATION]            = COMPOUND_STRING("Cornered, it lashes out with\neverything it has!"),

    // Articuno ("The Frozen Battlefield").
    [STRINGID_ENCARTICUNOINTRO]                     = COMPOUND_STRING("Articuno's wings scatter a\nfreezing gale!"),
    [STRINGID_ENCARTICUNOFROST1]                    = COMPOUND_STRING("Snow begins to fall across the\nbattlefield!"),
    [STRINGID_ENCARTICUNOFROST2]                    = COMPOUND_STRING("The deepening frost drags at\nyour team's footing!"),
    [STRINGID_ENCARTICUNOFROST3]                    = COMPOUND_STRING("The ground has turned to sheet\nice. Retreat is treacherous!"),
    [STRINGID_ENCARTICUNOFROZENDOMAIN]              = COMPOUND_STRING("Articuno's FROZEN DOMAIN\ntakes hold! The cold can\pno longer be driven back!"),
    [STRINGID_ENCARTICUNOFLAMESPUSHBACK]            = COMPOUND_STRING("The flames drive the frost back!"),
    [STRINGID_ENCARTICUNOCHILLLIFTS]                = COMPOUND_STRING("Your team shakes off the chill!"),
    [STRINGID_ENCARTICUNOSNOWCLEARS]                = COMPOUND_STRING("The snow clears from the\nbattlefield!"),
    [STRINGID_ENCARTICUNOBARRIERUP]                 = COMPOUND_STRING("The swirling frost hardens into\nan ICE BARRIER around Articuno!"),
    [STRINGID_ENCARTICUNOBARRIERTHIN]               = COMPOUND_STRING("The frost knits into an\nICE BARRIER, but the heat\pleaves it thin and brittle!"),
    [STRINGID_ENCARTICUNOBARRIERSEALEDSOLID]        = COMPOUND_STRING("The ICE BARRIER seals at full\nforce!\pThe locked cold will rebuild it\nthat strong every turn."),
    [STRINGID_ENCARTICUNOBARRIERSEALEDTHIN]         = COMPOUND_STRING("The ICE BARRIER seals thin and\nbrittle.\pThe locked cold can't reinforce\nit now."),
    [STRINGID_ENCARTICUNOBARRIERSEALEDNONE]         = COMPOUND_STRING("The cold locks in place with\ntoo little frost left to\praise any barrier at all!"),
    [STRINGID_ENCARTICUNOBARRIERBLUNTED]            = COMPOUND_STRING("Articuno's ICE BARRIER swallowed\nmost of that hit!\pThe frost in the air is already\nknitting it back together…"),
    [STRINGID_ENCARTICUNOBARRIERSHATTERS]           = COMPOUND_STRING("The ICE BARRIER shatters!"),
    [STRINGID_ENCARTICUNOSHOCKWAVE]                 = COMPOUND_STRING("An icy shockwave erupts from the\nbroken ice!"),
    [STRINGID_ENCARTICUNOFROZENGROUND]              = COMPOUND_STRING("The frozen ground bites at the\nnewcomer!"),
    [STRINGID_ENCARTICUNOSHRUGSOFFSLEEP]            = COMPOUND_STRING("Articuno's frozen aura shatters\nits slumber!"),
    [STRINGID_ENCARTICUNOPHASE1]                    = COMPOUND_STRING("Articuno rises above the storm.\nThe air itself turns to ice!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCARTICUNOABSOLUTEZERO]              = COMPOUND_STRING("Articuno unleashes\nABSOLUTE ZERO! The\pbattlefield freezes solid!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCARTICUNOTEMPPLUMMETS]              = COMPOUND_STRING("The temperature plummets! Only\nflame can hold it back!"),
    [STRINGID_ENCARTICUNOCOLDDEEPENS]               = COMPOUND_STRING("The cold deepens…"),
    [STRINGID_ENCARTICUNOFLAMESHOLD]                = COMPOUND_STRING("The flames hold the killing cold\nat bay!"),
    [STRINGID_ENCARTICUNOFIELDFREEZES]              = COMPOUND_STRING("The battlefield freezes\nover! Your team is\pbattered by the cold!"),
    [STRINGID_ENCARTICUNOWEAKENED]                  = COMPOUND_STRING("Articuno's wings falter and the\nfrost thins around it.\pNow is the moment to catch it!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCZAPDOSINTRO]                       = COMPOUND_STRING("Zapdos descends in a shroud of\ncrackling static!"),
    [STRINGID_ENCZAPDOSSPARKS]                      = COMPOUND_STRING("Sparks dance across Zapdos's\nfeathers."),
    [STRINGID_ENCZAPDOSCHARGE2]                     = COMPOUND_STRING("The static in the air is\nbuilding!"),
    [STRINGID_ENCZAPDOSSEVERESTORM]                 = COMPOUND_STRING("The sky splits open! A storm\nbreaks over the battlefield!"),
    [STRINGID_ENCZAPDOSOVERLOAD]                    = COMPOUND_STRING("Zapdos OVERLOADS! Raw current\npours off its wings!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCZAPDOSUNSTABLE]                    = COMPOUND_STRING("It's pouring everything\ninto the storm. Its body\pcan't hold together!"),
    [STRINGID_ENCZAPDOSGUARDHARDENS]                = COMPOUND_STRING("The charged air closes\naround Zapdos. Your\pattacks barely reach it!"),
    [STRINGID_ENCZAPDOSGUARDSLACKENS]               = COMPOUND_STRING("The air around Zapdos\nthins. Your attacks\pare getting through!"),
    [STRINGID_ENCZAPDOSDRINKSITIN]                  = COMPOUND_STRING("Zapdos drinks the current in!"),
    [STRINGID_ENCZAPDOSFEEDSONIMPACT]               = COMPOUND_STRING("The impact feeds the storm!"),
    [STRINGID_ENCZAPDOSDISCHARGES]                  = COMPOUND_STRING("The charge earths itself into\nthe ground!"),
    [STRINGID_ENCZAPDOSNOTHINGTOEARTH]              = COMPOUND_STRING("There's no charge left to draw\noff."),
    [STRINGID_ENCZAPDOSEARTHSOUT]                   = COMPOUND_STRING("The overload earths out all at\nonce! Zapdos is left reeling!"),
    [STRINGID_ENCZAPDOSSHAKESITOFF]                 = COMPOUND_STRING("Zapdos jolts itself awake, and\nthe storm dims for it!"),
    [STRINGID_ENCZAPDOSSTORMBUILDS]                 = COMPOUND_STRING("The storm gathers strength on\nits own…"),
    [STRINGID_ENCZAPDOSMARKS]                       = COMPOUND_STRING("Zapdos calls down the sky! Your\nPokémon is marked!"),
    [STRINGID_ENCZAPDOSAIRCRACKLES]                 = COMPOUND_STRING("The air above your\nPokémon crackles! The\pbolt falls next turn!"),
    [STRINGID_ENCZAPDOSLIGHTNINGSTRIKE]             = COMPOUND_STRING("LIGHTNING STRIKE!"),
    [STRINGID_ENCZAPDOSBOLTGROUNDED]                = COMPOUND_STRING("The bolt loses its hold and\nscatters harmlessly!"),
    [STRINGID_ENCZAPDOSLOSESITSTARGET]              = COMPOUND_STRING("Zapdos loses its target!"),
    [STRINGID_ENCZAPDOSCHARGEDAIR]                  = COMPOUND_STRING("The charged air bites at the\nnewcomer!"),
    [STRINGID_ENCZAPDOSSTRIKE]                      = COMPOUND_STRING("The storm hurls another bolt at\nyour team!"),
    [STRINGID_ENCZAPDOSBURNSOUT]                    = COMPOUND_STRING("Zapdos burns out! The storm\ncollapses!"),
    [STRINGID_ENCZAPDOSREELING]                     = COMPOUND_STRING("Zapdos is grounded and reeling!\nIts guard is gone!"),
    [STRINGID_ENCZAPDOSSTEADIES]                    = COMPOUND_STRING("Zapdos steadies itself\nand begins to gather\pthe storm again."),
    [STRINGID_ENCZAPDOSLASTSTAND]                   = COMPOUND_STRING("Zapdos pours its life into one\nendless storm!\pIt won't burn out again!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCZAPDOSWEAKENED]                    = COMPOUND_STRING("The storm gutters out.\pZapdos is spent. Now is the\nmoment to catch it!{PAUSE_UNTIL_PRESS}"),

    // Moltres ("The Everlasting Flame").
    [STRINGID_ENCMOLTRESINTRO]                      = COMPOUND_STRING("Moltres descends in a pillar of\nliving flame!"),
    [STRINGID_ENCMOLTRESEMBERS]                     = COMPOUND_STRING("Embers scatter from Moltres's\nfeathers."),
    [STRINGID_ENCMOLTRESROARS]                      = COMPOUND_STRING("Moltres's flames roar. Its\nattacks are burning hotter!"),
    [STRINGID_ENCMOLTRESAURA]                       = COMPOUND_STRING("An aura of fire wraps\nMoltres. Anything that\pstrikes it will burn!"),
    [STRINGID_ENCMOLTRESENGULFED]                   = COMPOUND_STRING("The battlefield ignites! The sky\nturns white with heat!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCMOLTRESBURNSITSELF]                = COMPOUND_STRING("Moltres is burning itself away\nto keep the fire alive!"),
    [STRINGID_ENCMOLTRESEVERLASTING]                = COMPOUND_STRING("MOLTRES BECOMES THE EVERLASTING\nFLAME!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCMOLTRESGUARDSLACKENS]             = COMPOUND_STRING("The heat swells, and Moltres's\nbody glows thin and open!"),
    [STRINGID_ENCMOLTRESGUARDHARDENS]              = COMPOUND_STRING("The flames bank low and Moltres\ndraws itself in tight."),
    [STRINGID_ENCMOLTRESBANKS]                      = COMPOUND_STRING("Without fuel, the fire dies\ndown…"),
    [STRINGID_ENCMOLTRESDRINKSITIN]                = COMPOUND_STRING("Moltres drinks the fire in!"),
    [STRINGID_ENCMOLTRESAURABURNS]                 = COMPOUND_STRING("The flame aura lashes back!"),
    [STRINGID_ENCMOLTRESFIELDBURNS]                = COMPOUND_STRING("The burning ground scorches your\nPokémon!"),
    [STRINGID_ENCMOLTRESCONSUMESITSELF]            = COMPOUND_STRING("Moltres's own fire eats away at\nit!"),
    [STRINGID_ENCMOLTRESBLAZESAWAKE]              = COMPOUND_STRING("Moltres blazes awake, and the\nfire leaps higher for it!"),
    [STRINGID_ENCMOLTRESSTILLBURNING]             = COMPOUND_STRING("The heat has not let up…"),
    [STRINGID_ENCMOLTRESFALLS]                      = COMPOUND_STRING("Moltres's fire gutters out. It\nfalls…"),
    [STRINGID_ENCMOLTRESEMBER]                      = COMPOUND_STRING("…but in the ashes, one ember\nstill burns."),
    [STRINGID_ENCMOLTRESREBIRTH]                    = COMPOUND_STRING("MOLTRES RISES FROM ITS OWN\nASHES!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCMOLTRESSCORCHED]                  = COMPOUND_STRING("The ground is scorched\nblack. The flames will\pnever leave it now."),
    [STRINGID_ENCMOLTRESWEAKENED]                  = COMPOUND_STRING("The everlasting flame finally\ndims.\pMoltres is spent. Now is the\nmoment to catch it!{PAUSE_UNTIL_PRESS}"),

    // Mewtwo ("The Perfect Weapon").
    [STRINGID_ENCMEWTWOINTRO]                       = COMPOUND_STRING("Mewtwo's gaze follows your every\nmovement."),
    [STRINGID_ENCMEWTWOANALYZED]                    = COMPOUND_STRING("Mewtwo has finished analyzing\nyou.\pIts body begins to change!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCMEWTWOBECOMESX]                    = COMPOUND_STRING("Mewtwo's body swells with raw\npower!"),
    [STRINGID_ENCMEWTWOBECOMESY]                    = COMPOUND_STRING("Mewtwo's mind expands beyond its\nbody!"),
    [STRINGID_ENCMEWTWOISEE]                        = COMPOUND_STRING("Mewtwo narrows its eyes, as if\nit can't decide something."),
    [STRINGID_ENCMEWTWOWATCHES]                     = COMPOUND_STRING("Mewtwo studies your every\nmovement…"),
    [STRINGID_ENCMEWTWOSTRAINS]                     = COMPOUND_STRING("Mewtwo's body shudders as it\nholds its shape…"),
    [STRINGID_ENCMEWTWOFLICKER]                     = COMPOUND_STRING("Mewtwo's outline flickers."),
    [STRINGID_ENCMEWTWORIPPLES]                     = COMPOUND_STRING("Mewtwo's form ripples\nviolently. It is straining\pto hold its shape!"),
    [STRINGID_ENCMEWTWOERUPTS]                      = COMPOUND_STRING("MEWTWO'S GENETIC STRUCTURE\nERUPTS!\pIts body won't hold!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCMEWTWOREASSEMBLES]                 = COMPOUND_STRING("Mewtwo reassembles itself. Its\nguard is whole again."),
    [STRINGID_ENCMEWTWOSHRUGS]                      = COMPOUND_STRING("Mewtwo's mind tears\nfree of the effect, but\psomething in it slips."),
    [STRINGID_ENCMEWTWOFORCE]                       = COMPOUND_STRING("Mewtwo's strikes are building on\neach other!"),
    [STRINGID_ENCMEWTWOFOCUS]                       = COMPOUND_STRING("Mewtwo's focus closes around\nyour Pokémon!"),
    [STRINGID_ENCMEWTWOPERFECT]                     = COMPOUND_STRING("Mewtwo abandons the line between\nits forms.\pIt won't settle again!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCMEWTWOWEAKENED]                    = COMPOUND_STRING("Mewtwo's power finally gutters\nout.\pNow is the moment to catch it!{PAUSE_UNTIL_PRESS}"),

    // Mew ("The Genetic Wonder").
    [STRINGID_ENCMEWINTRO]                          = COMPOUND_STRING("Mew circles you, watching with\ndelighted curiosity."),
    [STRINGID_ENCMEWWATCHES]                        = COMPOUND_STRING("Mew is studying everything you\ndo."),
    [STRINGID_ENCMEWENTRANCED]                      = COMPOUND_STRING("Mew is so absorbed in\nyou it keeps forgetting\pto defend itself!"),
    [STRINGID_ENCMEWCLOSER]                         = COMPOUND_STRING("Mew floats in closer. It wants a\nbetter look!"),
    [STRINGID_ENCMEWUNSHIELDED]                     = COMPOUND_STRING("Mew has stopped\nshielding itself. It's\ptoo busy watching you!"),
    [STRINGID_ENCMEWABSORBED]                       = COMPOUND_STRING("MEW IS COMPLETELY ABSORBED IN\nYOU!\pNothing stands between you and\nit!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCMEWRETREATS]                       = COMPOUND_STRING("Mew drifts back out of reach,\nwatching politely."),
    [STRINGID_ENCMEWEYESLIGHTUP]                    = COMPOUND_STRING("Mew's eyes light up!"),
    [STRINGID_ENCMEWDRIFTS]                         = COMPOUND_STRING("Mew's attention starts to drift…"),
    [STRINGID_ENCMEWBORED]                          = COMPOUND_STRING("Mew is bored!"),
    [STRINGID_ENCMEWFASCINATED]                     = COMPOUND_STRING("Mew is fascinated by your\ntechnique!"),
    [STRINGID_ENCMEWSTUDIES]                        = COMPOUND_STRING("Mew studies the energy you used!"),
    [STRINGID_ENCMEWCOPIESTECHNIQUE]                = COMPOUND_STRING("Mew copies the technique!"),
    [STRINGID_ENCMEWFLITSBACK]                      = COMPOUND_STRING("Mew imitates you! it flits away\nand comes right back!"),
    [STRINGID_ENCMEWCOPYCAT]                        = COMPOUND_STRING("Mew copies your fighting spirit!"),
    [STRINGID_ENCMEWSKY]                            = COMPOUND_STRING("Mew rearranges the sky, just to\nsee what happens!"),
    [STRINGID_ENCMEWRECOVERS]                       = COMPOUND_STRING("Mew imitates your recovery\ntechnique!"),
    [STRINGID_ENCMEWTAG]                            = COMPOUND_STRING("Mew darts in and tags your\nPokémon! it's playing tag!"),
    [STRINGID_ENCMEWCLEARSFIELD]                    = COMPOUND_STRING("Mew happily cleared the\nbattlefield!"),
    [STRINGID_ENCMEWCOPIES]                         = COMPOUND_STRING("Mew studies your Pokémon\nintently…"),
    [STRINGID_ENCMEWWEARSYOURSHAPE]                 = COMPOUND_STRING("Mew took on the shape of your\nPokémon!"),
    [STRINGID_ENCMEWCANTQUITEGETIT]                 = COMPOUND_STRING("Mew tried to copy your Pokémon,\nbut couldn't quite get it!"),
    [STRINGID_ENCMEWPLAYTIME]                       = COMPOUND_STRING("Mew seems to have decided this\nis a game!"),
    [STRINGID_ENCMEWDEMAND]                         = COMPOUND_STRING("Mew doesn't want to be hit.\pIt wants to be shown something!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCMEWWAITING]                        = COMPOUND_STRING("Mew is waiting…"),
    [STRINGID_ENCMEWDELIGHTED]                      = COMPOUND_STRING("Mew is delighted!"),
    [STRINGID_ENCMEWDISAPPOINTED]                   = COMPOUND_STRING("Mew looks disappointed."),
    [STRINGID_ENCMEWGENESIS]                        = COMPOUND_STRING("Mew's genetic energy begins to\noverflow!"),
    [STRINGID_ENCMEWWEAKENED]                       = COMPOUND_STRING("Mew has worn itself out playing\nwith you.\pNow is the moment to catch it!{PAUSE_UNTIL_PRESS}"),

    [STRINGID_ENCRAIKOUINTRO]                       = COMPOUND_STRING("Raikou lands in a crack of\nthunder!\pThe ground still hasn't stopped\nshaking!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCRAIKOUCIRCLES]                     = COMPOUND_STRING("It begins to circle, and it\nhasn't taken its eyes off you."),
    [STRINGID_ENCRAIKOUQUICKENS]                    = COMPOUND_STRING("Raikou quickens its pace."),
    [STRINGID_ENCRAIKOUBECOMESABLUR]                = COMPOUND_STRING("Raikou is moving too fast to\nfollow!"),
    [STRINGID_ENCRAIKOUFULLSTRIDE]                  = COMPOUND_STRING("Raikou hits full stride - it's\neverywhere at once!"),
    [STRINGID_ENCRAIKOUBLURS]                       = COMPOUND_STRING("Raikou blurs - your attacks are\nbarely finding it!"),
    [STRINGID_ENCRAIKOUSLOWS]                       = COMPOUND_STRING("Raikou's stride breaks - you can\nsee it again!"),
    [STRINGID_ENCRAIKOUOUTPACES]                    = COMPOUND_STRING("Raikou got there first!"),
    [STRINGID_ENCRAIKOURUNSDOWN]                    = COMPOUND_STRING("Raikou ran its quarry down, and\nit isn't slowing!"),
    [STRINGID_ENCRAIKOUEARTHED]                     = COMPOUND_STRING("The strike earths Raikou's\ncharge - it stumbles!"),
    [STRINGID_ENCRAIKOUCANTFINDFOOTING]             = COMPOUND_STRING("Raikou can't find its footing!"),
    [STRINGID_ENCRAIKOUTEARSFREE]                   = COMPOUND_STRING("Raikou tears itself awake - but\nit's lost its stride!"),
    [STRINGID_ENCRAIKOUDENIED]                      = COMPOUND_STRING("Raikou never got moving!"),
    [STRINGID_ENCRAIKOUMARKS]                       = COMPOUND_STRING("Raikou's eyes lock onto your\nPOKéMON!"),
    [STRINGID_ENCRAIKOUPRESSES]                     = COMPOUND_STRING("Raikou bears down on its quarry!"),
    [STRINGID_ENCRAIKOUGIVESCHASE]                  = COMPOUND_STRING("Raikou gives chase!"),
    [STRINGID_ENCRAIKOUQUARRYSTOOD]                 = COMPOUND_STRING("Its quarry refused to break!"),
    [STRINGID_ENCRAIKOUOVERSHOOTS]                  = COMPOUND_STRING("Raikou overshoots - for a moment\nit's wide open!"),
    [STRINGID_ENCRAIKOUCOILS]                       = COMPOUND_STRING("Raikou coils, and the air behind\nit goes still…"),
    [STRINGID_ENCRAIKOUVANISHES]                    = COMPOUND_STRING("Raikou vanished in a flash!"),
    [STRINGID_ENCRAIKOUREAD]                        = COMPOUND_STRING("Raikou's charge found nothing to\nstrike!"),
    [STRINGID_ENCRAIKOUSTORMROLLS]                  = COMPOUND_STRING("Thunder rolls across the field…"),
    [STRINGID_ENCRAIKOUBOLT]                        = COMPOUND_STRING("A bolt falls out of a clear sky!"),
    [STRINGID_ENCRAIKOUSTORMFEEDS]                  = COMPOUND_STRING("The storm feeds Raikou's stride!"),
    [STRINGID_ENCRAIKOURAINLASHES]                  = COMPOUND_STRING("Rain lashes sideways across the\nfield!"),
    [STRINGID_ENCRAIKOUSTATICHANGS]                 = COMPOUND_STRING("Static hangs in the air - Raikou\nfinds another gear."),
    [STRINGID_ENCRAIKOUROAR]                        = COMPOUND_STRING("Raikou lets out a thunderous\nroar!"),
    [STRINGID_ENCRAIKOUHUNTBEGINS]                  = COMPOUND_STRING("It has stopped circling.\pNow it's hunting.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCRAIKOUINCARNATE]                   = COMPOUND_STRING("Raikou's body crackles with\nuncontrollable electricity!"),
    [STRINGID_ENCRAIKOUUNSTABLE]                    = COMPOUND_STRING("It's carrying more current than\nit can hold."),
    [STRINGID_ENCRAIKOUARCS]                        = COMPOUND_STRING("Loose current arcs across the\nbattlefield!"),
    [STRINGID_ENCRAIKOUSTRAINING]                   = COMPOUND_STRING("Raikou's coat stands on end -\nsomething is about to give!"),
    [STRINGID_ENCRAIKOUDISCHARGES]                  = COMPOUND_STRING("Raikou's lightning discharges\nwildly!"),
    [STRINGID_ENCRAIKOUOPENING]                     = COMPOUND_STRING("It's blown itself off its feet -\nnow!"),
    [STRINGID_ENCRAIKOUWEAKENED]                    = COMPOUND_STRING("The thunder dies in its throat.\pRaikou is spent - now is the\nmoment to catch it!{PAUSE_UNTIL_PRESS}"),

    [STRINGID_ENCENTEIINTRO]                        = COMPOUND_STRING("Entei's roar splits the air like\na mountain cracking open!"),
    [STRINGID_ENCENTEIGROUNDHOT]                    = COMPOUND_STRING("The ground beneath your feet is\nalready too hot to stand on."),
    [STRINGID_ENCENTEIGROUNDWARMS]                  = COMPOUND_STRING("The heat under the battlefield\nclimbs."),
    [STRINGID_ENCENTEIHEATPOURS]                    = COMPOUND_STRING("Heat is pouring off Entei's back\nin waves!"),
    [STRINGID_ENCENTEITREMBLES]                     = COMPOUND_STRING("The earth begins to tremble\nbeneath you!"),
    [STRINGID_ENCENTEIMAGMA]                        = COMPOUND_STRING("Magma is bubbling up through the\ncracks!"),
    [STRINGID_ENCENTEISPLITS]                       = COMPOUND_STRING("The ground splits open -\nsomething is coming!"),
    [STRINGID_ENCENTEIERUPTS]                       = COMPOUND_STRING("VOLCANIC ERUPTION!"),
    [STRINGID_ENCENTEIFIELDSCORCHED]                = COMPOUND_STRING("The battlefield is left scorched\nand smoking!"),
    [STRINGID_ENCENTEISPENT]                        = COMPOUND_STRING("Entei is spent from the blast -\nit's wide open!"),
    [STRINGID_ENCENTEISTILLOPEN]                    = COMPOUND_STRING("Entei still hasn't recovered its\nfooting!"),
    [STRINGID_ENCENTEIRECOVERS]                     = COMPOUND_STRING("Entei plants its feet - the\nmoment is gone."),
    [STRINGID_ENCENTEIRECOIL]                       = COMPOUND_STRING("The eruption tore through Entei\ntoo!"),
    [STRINGID_ENCENTEIVENTS]                        = COMPOUND_STRING("Entei releases a wave of\nvolcanic heat!"),
    [STRINGID_ENCENTEISCOURED]                      = COMPOUND_STRING("The blast scours the battlefield\nclean!"),
    [STRINGID_ENCENTEIOFFBALANCE]                   = COMPOUND_STRING("Entei is off balance for a\nmoment!"),
    [STRINGID_ENCENTEISCORCHED]                     = COMPOUND_STRING("More of the ground blackens and\ncracks."),
    [STRINGID_ENCENTEIGROUNDGLOWS]                  = COMPOUND_STRING("The scorched ground is still\nglowing."),
    [STRINGID_ENCENTEICOOLS]                        = COMPOUND_STRING("Steam roars up - the scorched\nground cools!"),
    [STRINGID_ENCENTEIDRINKSHEAT]                   = COMPOUND_STRING("Entei drinks the heat rising\nfrom the burning ground!"),
    [STRINGID_ENCENTEISEARS]                        = COMPOUND_STRING("The scorched ground sears your\nPOKéMON!"),
    [STRINGID_ENCENTEINOWHERETOGO]                  = COMPOUND_STRING("Entei can't move, and the\npressure has nowhere to go!"),
    [STRINGID_ENCENTEIUNMOVED]                      = COMPOUND_STRING("Entei didn't even slow down."),
    [STRINGID_ENCENTEIHARDENS]                      = COMPOUND_STRING("The heat haze thickens - Entei\nis harder to reach!"),
    [STRINGID_ENCENTEICYCLE]                        = COMPOUND_STRING("Entei stops circling and plants\nitself.\pThe mountain has started to\nmove.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCENTEICATACLYSM]                    = COMPOUND_STRING("Entei's roar shakes the whole\nbattlefield!\pThere is nowhere left that isn't\nburning.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCENTEIWEAKENED]                     = COMPOUND_STRING("The fire under Entei finally\ngutters out.\pIt's spent - now is the moment\nto catch it!{PAUSE_UNTIL_PRESS}"),

    [STRINGID_ENCSUICUNEINTRO]                      = COMPOUND_STRING("Suicune's cry rings out, clear\nand cold as falling water."),
    [STRINGID_ENCSUICUNEWATCHES]                    = COMPOUND_STRING("The north wind goes still.\pSuicune isn't watching you -\nit's watching the battlefield.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCSUICUNEPURIFICATION]               = COMPOUND_STRING("PURIFICATION!"),
    [STRINGID_ENCSUICUNEWASHESWEATHER]              = COMPOUND_STRING("The waters carry away the sky."),
    [STRINGID_ENCSUICUNEWASHESTERRAIN]              = COMPOUND_STRING("The waters wash the ground\nclean."),
    [STRINGID_ENCSUICUNEWASHESSCREENS]              = COMPOUND_STRING("The waters break through the\nbarriers!"),
    [STRINGID_ENCSUICUNEWASHESSTATUS]               = COMPOUND_STRING("The waters run clear through\nSuicune."),
    [STRINGID_ENCSUICUNEFLOWRISES]                  = COMPOUND_STRING("Suicune moves more surely now."),
    [STRINGID_ENCSUICUNEFLOWSURGES]                 = COMPOUND_STRING("The water around Suicune runs\nfast and deep!"),
    [STRINGID_ENCSUICUNEFLOWEBBS]                   = COMPOUND_STRING("The water around Suicune slows."),
    [STRINGID_ENCSUICUNESTILL]                      = COMPOUND_STRING("The water around Suicune is\nperfectly still."),
    [STRINGID_ENCSUICUNETHICKENS]                   = COMPOUND_STRING("The current thickens - Suicune\nis harder to reach!"),
    [STRINGID_ENCSUICUNESLACKENS]                   = COMPOUND_STRING("The current slackens around\nSuicune."),
    [STRINGID_ENCSUICUNEMENDS]                      = COMPOUND_STRING("Clear water closes over\nSuicune's wounds."),
    [STRINGID_ENCSUICUNEDRAWS]                      = COMPOUND_STRING("Suicune draws upon the purest\nwater!"),
    [STRINGID_ENCSUICUNESACREDWATER]                = COMPOUND_STRING("The sacred water washes through\nSuicune!"),
    [STRINGID_ENCSUICUNEBROKEN]                     = COMPOUND_STRING("The water clouds\nover - Suicune's\pconcentration is broken!"),
    [STRINGID_ENCSUICUNESHATTERED]                  = COMPOUND_STRING("Suicune's purity has been\ndisrupted!"),
    [STRINGID_ENCSUICUNEUNSTEADY]                   = COMPOUND_STRING("Suicune still hasn't found its\nrhythm again."),
    [STRINGID_ENCSUICUNESHEDS]                      = COMPOUND_STRING("Suicune lets the water carry the\naffliction away."),
    [STRINGID_ENCSUICUNEUNDERTOW]                   = COMPOUND_STRING("The undertow drags at your\nPOKéMON!"),
    [STRINGID_ENCSUICUNECLEANSING]                  = COMPOUND_STRING("Suicune's aura floods the\nbattlefield.\pThe cleansing has begun.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCSUICUNESACREDBEAST]                = COMPOUND_STRING("Suicune stops, and stands\nperfectly still.\pEverything is calm. Suicune has\nreached perfect purity.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCSUICUNEWEAKENED]                   = COMPOUND_STRING("The water falls away from\nSuicune at last.\pIt's spent - now is the moment\nto catch it!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCCELEBIINTRO]                       = COMPOUND_STRING("A ripple runs through the air,\nand Celebi steps out of it."),
    [STRINGID_ENCCELEBIWATCHES]                     = COMPOUND_STRING("Celebi isn't watching your\nPOKéMON.\pIt's watching the battle\nitself - and it seems\pto be keeping notes.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCCELEBIRECORDS]                     = COMPOUND_STRING("Celebi begins to record this\nmoment..."),
    [STRINGID_ENCCELEBIANCHOR]                      = COMPOUND_STRING("You struck as the moment set -\nthis instant is anchored!"),
    [STRINGID_ENCCELEBIREWIND]                      = COMPOUND_STRING("TIME REWIND!"),
    [STRINGID_ENCCELEBIRESTORED]                    = COMPOUND_STRING("Celebi's injuries unhappen."),
    [STRINGID_ENCCELEBIPARADOX]                     = COMPOUND_STRING("Celebi distorts the flow of\ntime!"),
    [STRINGID_ENCCELEBIPARADOXHEAL]                 = COMPOUND_STRING("The moment of your recovery\nrepeats - for Celebi."),
    [STRINGID_ENCCELEBIPARADOXSTAT]                 = COMPOUND_STRING("The moment of your ascent\nrepeats - for Celebi."),
    [STRINGID_ENCCELEBIPARADOXBLOW]                 = COMPOUND_STRING("Your own blow returns out of the\npast!"),
    [STRINGID_ENCCELEBIFORESEEPHYS]                 = COMPOUND_STRING("Celebi has seen a fierce blow\ncoming..."),
    [STRINGID_ENCCELEBIFORESEESPEC]                 = COMPOUND_STRING("Celebi has seen a gathering of\nstrange power..."),
    [STRINGID_ENCCELEBIFORESEESTAT]                 = COMPOUND_STRING("Celebi has seen a moment of\nstillness coming..."),
    [STRINGID_ENCCELEBIFORESEENONE]                 = COMPOUND_STRING("Celebi looks ahead, and finds\nthe future clouded."),
    [STRINGID_ENCCELEBIPREDHELD]                    = COMPOUND_STRING("Celebi was already there."),
    [STRINGID_ENCCELEBIPREDBROKEN]                  = COMPOUND_STRING("The future Celebi saw did not\ncome to pass!"),
    [STRINGID_ENCCELEBITHICKENS]                    = COMPOUND_STRING("Time closes around Celebi - it\nis harder to reach!"),
    [STRINGID_ENCCELEBITHINS]                       = COMPOUND_STRING("Time thins around Celebi."),
    [STRINGID_ENCCELEBIUNSETTLED]                   = COMPOUND_STRING("The moment around Celebi still\nhasn't settled back into place."),
    [STRINGID_ENCCELEBISHEDS]                       = COMPOUND_STRING("Celebi lets the affliction slip\nback into the past."),
    [STRINGID_ENCCELEBIFUTURESIGHT]                 = COMPOUND_STRING("Celebi's eyes go strange and\ndistant.\pIt has stopped reacting to you.\nIt is reading ahead.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCCELEBICOLLAPSE]                    = COMPOUND_STRING("The air splits into a dozen\noverlapping days.\pTime itself is fracturing around\nCelebi!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCCELEBIWEATHER]                     = COMPOUND_STRING("The rain of a day long past\nfalls across the field."),
    [STRINGID_ENCCELEBITIMECOLLAPSE]                = COMPOUND_STRING("TIME COLLAPSE!"),
    [STRINGID_ENCCELEBICOLLAPSEFULL]                = COMPOUND_STRING("The battle unwinds all the way\nback!"),
    [STRINGID_ENCCELEBICOLLAPSEPART]                = COMPOUND_STRING("The battle unwinds - but not as\nfar as Celebi wanted."),
    [STRINGID_ENCCELEBICOLLAPSEHELD]                = COMPOUND_STRING("The anchored moments hold -\nCelebi cannot reach past them!"),
    [STRINGID_ENCCELEBIWEAKENED]                    = COMPOUND_STRING("Celebi slips out of the flow of\ntime, and stays where it lands.\pIt's spent - now is the moment\nto catch it!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCLUGIAINTRO]                        = COMPOUND_STRING("The sea heaves, and Lugia rises\nout of it."),
    [STRINGID_ENCLUGIASKY]                          = COMPOUND_STRING("The clouds are already turning\nabove it.\pWhatever the sky is about to do,\nLugia is the one doing it.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCLUGIACALM]                         = COMPOUND_STRING("The sky goes quiet again."),
    [STRINGID_ENCLUGIABREEZE]                       = COMPOUND_STRING("A cold wind starts to pick up."),
    [STRINGID_ENCLUGIARAIN]                         = COMPOUND_STRING("The clouds break, and the rain\ncomes down hard!"),
    [STRINGID_ENCLUGIASTORM]                        = COMPOUND_STRING("The rain turns to a howling\nstorm!"),
    [STRINGID_ENCLUGIATEMPEST]                      = COMPOUND_STRING("A TEMPEST tears across the\nfield!\pThe wind is pushing everything\naway from Lugia.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCLUGIASEAWEATHER]                   = COMPOUND_STRING("The sea itself climbs into the\nsky!"),
    [STRINGID_ENCLUGIATHICKENS]                     = COMPOUND_STRING("The sea closes around Lugia - it\nis harder to reach!"),
    [STRINGID_ENCLUGIASLACKENS]                     = COMPOUND_STRING("Lugia's guard slips."),
    [STRINGID_ENCLUGIACHIP]                         = COMPOUND_STRING("The churning water batters your\nPOKéMON!"),
    [STRINGID_ENCLUGIACHIPFLYING]                   = COMPOUND_STRING("The wind tears your POKéMON out\nof the air!"),
    [STRINGID_ENCLUGIACHIPSPARED]                   = COMPOUND_STRING("The sea does not touch its own."),
    [STRINGID_ENCLUGIAERODES]                       = COMPOUND_STRING("Spray and wind blind your\nPOKéMON!"),
    [STRINGID_ENCLUGIAUNDERTOW]                     = COMPOUND_STRING("The undertow drags at your\nPOKéMON!"),
    [STRINGID_ENCLUGIASTRAIN]                       = COMPOUND_STRING("Lugia's wings falter against the\nsea it raised."),
    [STRINGID_ENCLUGIASTRAINHIGH]                   = COMPOUND_STRING("Lugia is fighting the storm as\nhard as it is fighting you!"),
    [STRINGID_ENCLUGIACOLLAPSE]                     = COMPOUND_STRING("Lugia struggles against the\nraging sea it summoned!\pIt has lost hold of the storm -\nand of itself.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCLUGIAUNSTEADY]                     = COMPOUND_STRING("Lugia still hasn't recovered its\nrhythm."),
    [STRINGID_ENCLUGIAMAELSTROM]                    = COMPOUND_STRING("LUGIA SUMMONED A MAELSTROM!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCLUGIADIVE]                         = COMPOUND_STRING("Lugia vanished beneath the\nwaves!"),
    [STRINGID_ENCLUGIACHURN]                        = COMPOUND_STRING("The ocean begins to churn\nviolently!"),
    [STRINGID_ENCLUGIASTRIKE]                       = COMPOUND_STRING("The sea erupts under your\nPOKéMON!"),
    [STRINGID_ENCLUGIAEYEOPENS]                     = COMPOUND_STRING("The storm goes eerily calm..."),
    [STRINGID_ENCLUGIAEYECALM]                      = COMPOUND_STRING("Lugia rests in the quiet it\nmade."),
    [STRINGID_ENCLUGIAEYEMENDS]                     = COMPOUND_STRING("The still water knits Lugia back\ntogether."),
    [STRINGID_ENCLUGIAEYECLOSES]                    = COMPOUND_STRING("The eye begins to close!"),
    [STRINGID_ENCLUGIAVENT]                         = COMPOUND_STRING("The sky tears open, and the sea\nloses its rhythm!\pEverything the weather was doing\nis gone - yours as well.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCLUGIASHEDS]                        = COMPOUND_STRING("The raging sea will not let\nLugia rest!"),
    [STRINGID_ENCLUGIASEAERUPTS]                    = COMPOUND_STRING("Lugia throws its wings wide, and\nthe ocean answers.\pThe sea and the sky\nare one thing now, and\pit is carrying both.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCLUGIAOCEANSWRATH]                  = COMPOUND_STRING("Lugia stops holding back.\pIt has stopped holding on, too.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCLUGIAWEAKENED]                     = COMPOUND_STRING("The storm falls out of the sky\nall at once, and Lugia with it.\pIt's spent - now is the moment\nto catch it!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCHOOHINTRO]                         = COMPOUND_STRING("Ho-Oh descends on a pillar of\nfire, and the air turns gold."),
    [STRINGID_ENCHOOHFIRE]                          = COMPOUND_STRING("The flame it carries isn't only\nfor burning.\pIt gives that fire away - and it\nhas only so much of it.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCHOOHGUARDRISE]                     = COMPOUND_STRING("The sacred flames close around\nHo-Oh - it is harder to reach!"),
    [STRINGID_ENCHOOHGUARDFALL]                     = COMPOUND_STRING("Ho-Oh's light dims, and its\nguard with it."),
    [STRINGID_ENCHOOHFLAMEOUT]                      = COMPOUND_STRING("Ho-Oh's fire has burned down to\nnothing."),
    [STRINGID_ENCHOOHFLAMELOW]                      = COMPOUND_STRING("Ho-Oh's fire is running thin."),
    [STRINGID_ENCHOOHFLAMESTEADY]                   = COMPOUND_STRING("Ho-Oh's fire burns steady again."),
    [STRINGID_ENCHOOHFLAMEHIGH]                     = COMPOUND_STRING("Ho-Oh's fire climbs higher than\nbefore!"),
    [STRINGID_ENCHOOHEMBER]                         = COMPOUND_STRING("Only embers are left in Ho-Oh's\nplumage."),
    [STRINGID_ENCHOOHBLAZE]                         = COMPOUND_STRING("Ho-Oh is carrying more fire than\nit arrived with."),
    [STRINGID_ENCHOOHTRIALNEAR]                     = COMPOUND_STRING("Ho-Oh's gaze settles on you..."),
    [STRINGID_ENCHOOHJUDGES]                        = COMPOUND_STRING("HO-OH JUDGES YOUR ACTIONS."),
    [STRINGID_ENCHOOHIMPURE]                        = COMPOUND_STRING("IMPURE.\pHo-Oh will not be judged on\na soiled field - it burns the\pfield clean, and takes the fire\nback.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCHOOHDEFIANT]                       = COMPOUND_STRING("DEFIANT.\pIf you will climb, Ho-Oh will\nclimb higher.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCHOOHMERCIFUL]                      = COMPOUND_STRING("MERCIFUL."),
    [STRINGID_ENCHOOHWORTHY]                        = COMPOUND_STRING("WORTHY."),
    [STRINGID_ENCHOOHMEND]                          = COMPOUND_STRING("Ho-Oh bathes itself in sacred\nfire!"),
    [STRINGID_ENCHOOHPURIFY]                        = COMPOUND_STRING("The sacred flame burns the\nsickness away!"),
    [STRINGID_ENCHOOHREKINDLE]                      = COMPOUND_STRING("Ho-Oh spreads its wings and\ncalls the sunlight back!"),
    [STRINGID_ENCHOOHREVIVE]                        = COMPOUND_STRING("Sacred fire pours into one of\nyour fallen POKéMON.\pIt stands up again.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCHOOHFALLEN]                        = COMPOUND_STRING("Ho-Oh gathers up the fire of the\nfallen."),
    [STRINGID_ENCHOOHBLESSSWIFT]                    = COMPOUND_STRING("BLESSING OF SWIFTNESS.\pHo-Oh's flames wrap your POKéMON\nand lift it.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCHOOHBLESSVALOR]                    = COMPOUND_STRING("BLESSING OF VALOR.\pHo-Oh's flames wrap your POKéMON\nand sharpen it.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCHOOHBLESSAEGIS]                    = COMPOUND_STRING("BLESSING OF THE AEGIS.\pHo-Oh's flames wrap your POKéMON\nand harden around it.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCHOOHBLESSPURITY]                   = COMPOUND_STRING("BLESSING OF PURITY.\pHo-Oh's flames wrap your\nPOKéMON, and nothing\pfoul can reach it.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCHOOHBLESSPRICE]                    = COMPOUND_STRING("The flame that blesses is also\nconsuming!"),
    [STRINGID_ENCHOOHBLESSENDS]                     = COMPOUND_STRING("The blessing burns out."),
    [STRINGID_ENCHOOHBLESSSHED]                     = COMPOUND_STRING("The sacred flame gutters as your\nPOKéMON leaves the field."),
    [STRINGID_ENCHOOHRAINBOW]                       = COMPOUND_STRING("A brilliant rainbow arcs across\nthe battlefield!"),
    [STRINGID_ENCHOOHRAINBOWRULE]                   = COMPOUND_STRING("The sun will not set while Ho-Oh\nholds the sky.\pEverything it does lands\nharder under that light -\pand the light is feeding it.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCHOOHGUTTERS]                       = COMPOUND_STRING("Ho-Oh's flames go out.\pThe field goes completely\nsilent...{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCHOOHREBIRTH]                       = COMPOUND_STRING("A rainbow opens where Ho-Oh\nfell.\pHO-OH IS REBORN!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCHOOHRITEBEGINS]                    = COMPOUND_STRING("It has nothing left to give\naway.\pWhat comes now is the\nlast of its fire, spent\pfour beats at a time.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCHOOHASHES]                         = COMPOUND_STRING("Ho-Oh reaches for a fire that\nisn't there any more.\pIt sinks into its own ashes.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCHOOHRITEFLAME]                     = COMPOUND_STRING("THE RITE: SACRED FLAME.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCHOOHRITEPURIFY]                    = COMPOUND_STRING("THE RITE: PURIFICATION.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCHOOHRITEJUDGMENT]                  = COMPOUND_STRING("THE RITE: JUDGMENT.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCHOOHRITEPHOENIX]                   = COMPOUND_STRING("THE RITE: PHOENIX.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCHOOHWEAKENED]                      = COMPOUND_STRING("The last of the sacred\nflame goes out, and Ho-Oh\psettles to the ground.\pIt's spent - now is the moment\nto catch it!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCDEOXYSINTRO]                       = COMPOUND_STRING("The virus unfolds out of\nthe meteorite, and its cells\pbegin to arrange themselves."),
    [STRINGID_ENCDEOXYSADAPTS]                      = COMPOUND_STRING("It isn't studying your moves.\pIt is rebuilding its body to\nanswer them, and each rebuild\pleaves it a little less whole.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCDEOXYSGUARDRISE]                   = COMPOUND_STRING("Deoxys' cells knit tighter - it\nis harder to hurt!"),
    [STRINGID_ENCDEOXYSGUARDFALL]                   = COMPOUND_STRING("Deoxys' structure thins, and its\nguard with it."),
    [STRINGID_ENCDEOXYSSTRESSSTABLE]                = COMPOUND_STRING("Deoxys' cells are holding their\nshape."),
    [STRINGID_ENCDEOXYSSTRESSSTRAIN]                = COMPOUND_STRING("Deoxys' cells are straining\nto hold together - and\phitting harder for it."),
    [STRINGID_ENCDEOXYSSTRESSCRITICAL]              = COMPOUND_STRING("Deoxys' body is barely\nholding its shape, and\peverything it does is violent!"),
    [STRINGID_ENCDEOXYSRECONSTRUCT]                 = COMPOUND_STRING("Deoxys is changing its cellular\nstructure!"),
    [STRINGID_ENCDEOXYSRECONSTRUCTING]              = COMPOUND_STRING("RECONSTRUCTING...{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCDEOXYSNOCHANGE]                    = COMPOUND_STRING("Deoxys finds nothing worth\nchanging."),
    [STRINGID_ENCDEOXYSFORMNORMAL]                  = COMPOUND_STRING("NORMAL FORME.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCDEOXYSFORMATTACK]                  = COMPOUND_STRING("ATTACK FORME.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCDEOXYSFORMDEFENSE]                 = COMPOUND_STRING("DEFENSE FORME.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCDEOXYSFORMSPEED]                   = COMPOUND_STRING("SPEED FORME.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCDEOXYSHARDEN]                      = COMPOUND_STRING("Deoxys' cells harden against\nthat attack!"),
    [STRINGID_ENCDEOXYSBARRIERBREAK]                = COMPOUND_STRING("Deoxys' cellular defenses\ncollapse!"),
    [STRINGID_ENCDEOXYSASSAULTCHARGE]               = COMPOUND_STRING("Deoxys' offensive cells are\nmassing."),
    [STRINGID_ENCDEOXYSASSAULTFIRE]                 = COMPOUND_STRING("Deoxys' assault reaches critical\nmass!"),
    [STRINGID_ENCDEOXYSBLITZ]                       = COMPOUND_STRING("Deoxys strikes before you can\neven move!"),
    [STRINGID_ENCDEOXYSEVADE]                       = COMPOUND_STRING("Deoxys is moving too fast to be\nhit!"),
    [STRINGID_ENCDEOXYSSHRUG]                       = COMPOUND_STRING("Deoxys' cells re-form around the\neffect!"),
    [STRINGID_ENCDEOXYSINSTABILITY]                 = COMPOUND_STRING("CELLULAR INSTABILITY!\pDeoxys collapses out of its\nforme, and cannot rebuild!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCDEOXYSUNSTABLE]                    = COMPOUND_STRING("Deoxys' cells are still coming\napart - it can't hold a shape."),
    [STRINGID_ENCDEOXYSSTABILIZED]                  = COMPOUND_STRING("Deoxys' cells have stabilized."),
    [STRINGID_ENCDEOXYSSTUDIES]                     = COMPOUND_STRING("Deoxys studies the battle,\nwaiting."),
    [STRINGID_ENCDEOXYSRAPID]                       = COMPOUND_STRING("Deoxys begins mutating at an\nincredible rate!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCDEOXYSPERFECT]                     = COMPOUND_STRING("Deoxys' cells are changing\nfaster than ever!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCDEOXYSHYBRID]                      = COMPOUND_STRING("Deoxys combines two formes at\nonce!"),
    [STRINGID_ENCDEOXYSHYBRIDONE]                   = COMPOUND_STRING("ASSAULT VELOCITY!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCDEOXYSHYBRIDTWO]                   = COMPOUND_STRING("ARMORED RETALIATION!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCDEOXYSDESTABILIZE]                 = COMPOUND_STRING("Deoxys' cells begin to\ndestabilize!"),
    [STRINGID_ENCDEOXYSNOADAPT]                     = COMPOUND_STRING("DEOXYS' CELLS CAN NO LONGER\nADAPT!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCDEOXYSWEAKENED]                    = COMPOUND_STRING("Deoxys settles back\ninto its first shape,\punable to build another.\pIt's spent - now is the moment\nto catch it!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCJIRACHIAWAKENS]                    = COMPOUND_STRING("Jirachi's eyes open. Its tags\nbegin to sway."),
    [STRINGID_ENCJIRACHIHEARTFORCE]                 = COMPOUND_STRING("Jirachi's tags glow a fierce\nred."),
    [STRINGID_ENCJIRACHIHEARTBALANCE]               = COMPOUND_STRING("Jirachi's tags shimmer, silver\nand unsettled."),
    [STRINGID_ENCJIRACHIHEARTWILL]                  = COMPOUND_STRING("Jirachi's tags glow a still,\ndeep blue."),
    [STRINGID_ENCJIRACHITAGSCHIME]                  = COMPOUND_STRING("Jirachi's tags begin to chime\nsoftly…"),
    [STRINGID_ENCJIRACHITAGSRING]                   = COMPOUND_STRING("Jirachi's tags are ringing!"),
    [STRINGID_ENCJIRACHIDREAMS]                     = COMPOUND_STRING("Jirachi sleeps… and its dreams\ngrow stronger."),
    [STRINGID_ENCJIRACHITAGSBLAZE]                  = COMPOUND_STRING("Jirachi's tags blaze with\ngathered light!"),
    [STRINGID_ENCJIRACHIWISHONE]                    = COMPOUND_STRING("JIRACHI'S FIRST WISH{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCJIRACHIWISHTWO]                    = COMPOUND_STRING("JIRACHI'S SECOND WISH{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCJIRACHIFINALWISH]                  = COMPOUND_STRING("JIRACHI'S FINAL WISH{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCJIRACHIWISHPOWER]                  = COMPOUND_STRING("Jirachi wished for overwhelming\npower!"),
    [STRINGID_ENCJIRACHIWISHPEACE]                  = COMPOUND_STRING("Jirachi wished for safety."),
    [STRINGID_ENCJIRACHIWISHWONDER]                 = COMPOUND_STRING("Jirachi wished for something it\ncould not name…"),
    [STRINGID_ENCJIRACHILISTENING]                  = COMPOUND_STRING("Jirachi is listening to your\nheart…\pWill you make a wish too?"),
    [STRINGID_ENCJIRACHISHAREDPOWER]                = COMPOUND_STRING("Jirachi's wish shines on you\nboth!"),
    [STRINGID_ENCJIRACHISHAREDPEACE]                = COMPOUND_STRING("Jirachi's wish shelters you\nboth."),
    [STRINGID_ENCJIRACHISHAREDWONDER]               = COMPOUND_STRING("Jirachi's wish spills over onto\nyou!"),
    [STRINGID_ENCJIRACHIPOWERCOST]                  = COMPOUND_STRING("Jirachi's wish burns through its\nown guard!"),
    [STRINGID_ENCJIRACHIPEACECOST]                  = COMPOUND_STRING("Jirachi drifts, lost inside its\nown wish…"),
    [STRINGID_ENCJIRACHIWONDERCOST]                 = COMPOUND_STRING("Jirachi drifts away, dreaming of\nits own wish."),
    [STRINGID_ENCJIRACHIWONDERSTRENGTH]             = COMPOUND_STRING("Jirachi wished for renewed\nstrength!"),
    [STRINGID_ENCJIRACHIWONDERSTAR]                 = COMPOUND_STRING("Jirachi wished upon a falling\nstar!"),
    [STRINGID_ENCJIRACHIWONDERSTILL]                = COMPOUND_STRING("Jirachi wished for stillness."),
    [STRINGID_ENCJIRACHIWONDERPLAY]                 = COMPOUND_STRING("Jirachi wished to play!"),
    [STRINGID_ENCJIRACHIGUARDRISE]                  = COMPOUND_STRING("Jirachi feels further away than\nbefore."),
    [STRINGID_ENCJIRACHIGUARDFALL]                  = COMPOUND_STRING("Jirachi's light has thinned - it\ncan be reached!"),
    [STRINGID_ENCJIRACHIMIRACLEPOWER]               = COMPOUND_STRING("Jirachi wished for\noverwhelming power - and\pthe wish was granted!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCJIRACHIMIRACLELIFE]                = COMPOUND_STRING("Jirachi wished to live - and the\nwish was granted!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCJIRACHIMIRACLESTARS]               = COMPOUND_STRING("Jirachi wished upon every star\nat once!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCJIRACHIMIRACLEBURNS]               = COMPOUND_STRING("Jirachi's miracle is still\nburning."),
    [STRINGID_ENCJIRACHISTARFALLS]                  = COMPOUND_STRING("A fallen star comes down!"),
    [STRINGID_ENCJIRACHICLINGS]                     = COMPOUND_STRING("Jirachi holds on - its last wish\nis not yet spent."),
    [STRINGID_ENCJIRACHIWEAKENED]                   = COMPOUND_STRING("Jirachi's tags have gone dark.\nIts wishes are spent.\pIt's worn out - now is the\nmoment to catch it!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCRAYQUAZAAWAKENS]                   = COMPOUND_STRING("Rayquaza descends out of the\nozone, coiling on the wind!"),
    [STRINGID_ENCRAYQUAZAABSORBS]                   = COMPOUND_STRING("Rayquaza absorbs the energy\nsurrounding the battlefield!"),
    [STRINGID_ENCRAYQUAZADELTAASCENSION]            = COMPOUND_STRING("DELTA ASCENSION{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCRAYQUAZACONTROLFAILING]            = COMPOUND_STRING("Rayquaza's hold on the sky is\ncoming apart!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCRAYQUAZAGIVESOUT]                  = COMPOUND_STRING("Rayquaza's ascent gives out. It\ncannot stay aloft."),
    [STRINGID_ENCRAYQUAZACLIMBSLOW]                 = COMPOUND_STRING("Rayquaza lifts away on a rising\ncurrent."),
    [STRINGID_ENCRAYQUAZACLIMBSHIGH]                = COMPOUND_STRING("Rayquaza climbs higher - it's\nhard to even see!"),
    [STRINGID_ENCRAYQUAZAVANISHED]                  = COMPOUND_STRING("Rayquaza has disappeared into\nthe heavens!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCRAYQUAZAGROUNDED]                  = COMPOUND_STRING("Rayquaza is dragged down to the\nfield - it's within reach!"),
    [STRINGID_ENCRAYQUAZAPRESSUREONE]               = COMPOUND_STRING("The air around Rayquaza is\ngrowing heavy."),
    [STRINGID_ENCRAYQUAZAPRESSURETWO]               = COMPOUND_STRING("The sky is screaming. Something\nis about to give!"),
    [STRINGID_ENCRAYQUAZAOUTOFREACH]                = COMPOUND_STRING("Rayquaza circles far beyond your\nreach."),
    [STRINGID_ENCRAYQUAZASKYFALLWARN]               = COMPOUND_STRING("The atmosphere itself buckles!"),
    [STRINGID_ENCRAYQUAZASKYFALLCOMING]             = COMPOUND_STRING("Something massive is coming\ndown…"),
    [STRINGID_ENCRAYQUAZASKYFALL]                   = COMPOUND_STRING("RAYQUAZA FALLS OUT OF THE SKY!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCRAYQUAZATEARS]                     = COMPOUND_STRING("Rayquaza dives to tear the\nweather apart!"),
    [STRINGID_ENCRAYQUAZAENOUGH]                    = COMPOUND_STRING("Rayquaza has had enough!"),
    [STRINGID_ENCRAYQUAZACONFLICT]                  = COMPOUND_STRING("The sky answers to Rayquaza\nalone!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCRAYQUAZASHOTDOWN]                  = COMPOUND_STRING("That blow knocked Rayquaza out\nof the air!"),
    [STRINGID_ENCRAYQUAZAREADSYOU]                  = COMPOUND_STRING("Rayquaza reads your intent and\nrolls away on the wind!"),
    [STRINGID_ENCRAYQUAZACHAOSRAIN]                 = COMPOUND_STRING("The broken sky spills rain\nacross the field."),
    [STRINGID_ENCRAYQUAZACHAOSSUN]                  = COMPOUND_STRING("The clouds tear open and\nsunlight pours through."),
    [STRINGID_ENCRAYQUAZACHAOSSAND]                 = COMPOUND_STRING("The wind drags a wall of sand\nover the field."),
    [STRINGID_ENCRAYQUAZACHAOSHAIL]                 = COMPOUND_STRING("The upper air comes down as\nhail."),
    [STRINGID_ENCRAYQUAZACHAOSCLEAR]                = COMPOUND_STRING("The sky falls still and empty."),
    [STRINGID_ENCRAYQUAZADRIFTS]                    = COMPOUND_STRING("Rayquaza drifts on the wind… but\nthe sky still turns."),
    [STRINGID_ENCRAYQUAZAWEAKENED]                  = COMPOUND_STRING("Rayquaza has fallen back\nto its true shape, too\pspent to rise again.\pIt's worn out - now is the\nmoment to catch it!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCKYOGREAWAKENS]                     = COMPOUND_STRING("Kyogre rises from the trench,\nand the sea rises with it!"),
    [STRINGID_ENCKYOGRETIDEONE]                     = COMPOUND_STRING("The water is creeping higher\naround your POKéMON."),
    [STRINGID_ENCKYOGRETIDETWO]                     = COMPOUND_STRING("HIGH TIDE. Kyogre is harder to\nreach through the swell."),
    [STRINGID_ENCKYOGRETIDETHREE]                   = COMPOUND_STRING("FLOOD. The battlefield is going\nunder!"),
    [STRINGID_ENCKYOGRETIDEFOUR]                    = COMPOUND_STRING("DELUGE. Everything is slowing\ndown in the water!"),
    [STRINGID_ENCKYOGRETIDEFIVE]                    = COMPOUND_STRING("OCEAN'S WRATH. Nothing can move\nagainst this current!"),
    [STRINGID_ENCKYOGRETIDERISES]                   = COMPOUND_STRING("The tide rises!"),
    [STRINGID_ENCKYOGRETIDEFALLS]                   = COMPOUND_STRING("The tide is driven back!"),
    [STRINGID_ENCKYOGREANSWERSFLAME]                = COMPOUND_STRING("Kyogre answers the flame. The\nwater surges up to meet it!"),
    [STRINGID_ENCKYOGREDROWNING]                    = COMPOUND_STRING("The rising water drags at your\nPOKéMON!"),
    [STRINGID_ENCKYOGRENOTSTILLED]                  = COMPOUND_STRING("The ocean will not be\nstilled! Kyogre shakes it\poff and the water climbs!"),
    [STRINGID_ENCKYOGREUNDERTOWMARK]                = COMPOUND_STRING("The current begins pulling your\nPOKéMON beneath the waves!"),
    [STRINGID_ENCKYOGREUNDERTOWPULL]                = COMPOUND_STRING("The undertow drags it deeper!"),
    [STRINGID_ENCKYOGREUNDERTOWHELD]                = COMPOUND_STRING("It held its ground against the\ncurrent, and the water gave way!"),
    [STRINGID_ENCKYOGREUNDERTOWFLED]                = COMPOUND_STRING("It tore free of the undertow -\nand the sea rushed in behind it!"),
    [STRINGID_ENCKYOGREPRIMAL]                      = COMPOUND_STRING("PRIMAL REVERSION{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCKYOGREANCIENTPOWER]                = COMPOUND_STRING("Kyogre's ancient power awakens!\nThe sea answers faster now!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCKYOGREBEYONDCONTROL]               = COMPOUND_STRING("The sea begins to rise beyond\nall control!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCKYOGREDELUGEBEGINS]                = COMPOUND_STRING("Kyogre is gathering the whole\nocean above the field!"),
    [STRINGID_ENCKYOGREDELUGECOUNT]                 = COMPOUND_STRING("The wave is still building…"),
    [STRINGID_ENCKYOGREDELUGENEAR]                  = COMPOUND_STRING("The wave is about to break!"),
    [STRINGID_ENCKYOGREDELUGEHITS]                  = COMPOUND_STRING("THE BATTLEFIELD IS SWALLOWED BY\nTHE OCEAN!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCKYOGREDELUGEFAILS]                 = COMPOUND_STRING("The raging sea subsides! Kyogre\nis spent from holding it up!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCKYOGREREELING]                     = COMPOUND_STRING("Kyogre is still reeling. Its\nguard is down!"),
    [STRINGID_ENCKYOGREWEAKENED]                    = COMPOUND_STRING("The endless sea drains\naway, and Kyogre sinks\pback into its true shape.\pIt's worn out - now is the\nmoment to catch it!{PAUSE_UNTIL_PRESS}"),

    [STRINGID_ENCGROUDONAWAKENS]                    = COMPOUND_STRING("Groudon heaves itself\nup out of the magma, and\pthe land rises with it!"),
    [STRINGID_ENCGROUDONLANDONE]                    = COMPOUND_STRING("The ground is pushing upward\naround your POKéMON."),
    [STRINGID_ENCGROUDONLANDTWO]                    = COMPOUND_STRING("HIGHLANDS. Groudon is harder to\nreach across the risen rock."),
    [STRINGID_ENCGROUDONLANDTHREE]                  = COMPOUND_STRING("SCORCHED PLATEAU. The ground is\ntoo hot to stand on!"),
    [STRINGID_ENCGROUDONLANDFOUR]                   = COMPOUND_STRING("CONTINENTAL RISE. The land is\nclosing in around you!"),
    [STRINGID_ENCGROUDONLANDFIVE]                   = COMPOUND_STRING("PRIMAL LAND. The battlefield has\nbecome part of Groudon!"),
    [STRINGID_ENCGROUDONLANDRISES]                  = COMPOUND_STRING("The land swells higher!"),
    [STRINGID_ENCGROUDONLANDFALLS]                  = COMPOUND_STRING("The land around Groudon erodes\naway!"),
    [STRINGID_ENCGROUDONFEEDSFLAME]                 = COMPOUND_STRING("Groudon drinks in the flame. The\nground rises to meet it!"),
    [STRINGID_ENCGROUDONSCORCH]                     = COMPOUND_STRING("The scorching ground sears your\nPOKéMON!"),
    [STRINGID_ENCGROUDONNOTSHAKEN]                  = COMPOUND_STRING("Groudon will not be slowed!\nIt shrugs the numbness\poff and the land climbs!"),
    [STRINGID_ENCGROUDONSUNRETURNS]                 = COMPOUND_STRING("The land will not be watered.\nThe sky burns clear again!"),
    [STRINGID_ENCGROUDONTREMBLE]                    = COMPOUND_STRING("The ground trembles beneath you."),
    [STRINGID_ENCGROUDONCRACK]                      = COMPOUND_STRING("The earth begins to crack!"),
    [STRINGID_ENCGROUDONPREPARING]                  = COMPOUND_STRING("Groudon is preparing a\ndevastating earthquake!"),
    [STRINGID_ENCGROUDONTREMOR]                     = COMPOUND_STRING("TREMOR! The field shudders\nunderfoot!"),
    [STRINGID_ENCGROUDONQUAKE]                      = COMPOUND_STRING("QUAKE! The ground bucks and\nthrows your POKéMON down!"),
    [STRINGID_ENCGROUDONMAJORQUAKE]                 = COMPOUND_STRING("MAJOR QUAKE! The shockwave tears\neverything loose!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCGROUDONBREAK]                      = COMPOUND_STRING("CONTINENTAL BREAK! The whole\nlandmass comes apart at once!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCGROUDONSTRIPPED]                   = COMPOUND_STRING("The upheaval tore your side's\ndefenses apart!"),
    [STRINGID_ENCGROUDONPRIMAL]                     = COMPOUND_STRING("PRIMAL REVERSION{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCGROUDONANCIENTPOWER]               = COMPOUND_STRING("Groudon's ancient power\nerupts! The land answers\pit now without being asked!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCGROUDONCOLLAPSEBEGINS]             = COMPOUND_STRING("Groudon's power begins tearing\nits own continent apart!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCGROUDONCOLLAPSE]                   = COMPOUND_STRING("CONTINENTAL COLLAPSE!\nThe ground caves in under\peverything standing on it!"),
    [STRINGID_ENCGROUDONUNSTABLE]                   = COMPOUND_STRING("The land will not hold.\nGroudon is being crushed\punder its own weight!"),
    [STRINGID_ENCGROUDONWEAKENED]                   = COMPOUND_STRING("The continent sinks away,\nand Groudon folds back\pinto its true shape.\pIt's spent - now is the moment\nto catch it!{PAUSE_UNTIL_PRESS}"),

    [STRINGID_ENCREGICEAWAKENS]                     = COMPOUND_STRING("Regice's core pulses once, and\nthe air goes still."),
    [STRINGID_ENCREGICEHAILFIELD]                   = COMPOUND_STRING("A freezing gale settles over the\nfield."),
    [STRINGID_ENCREGICEDEEPENS]                     = COMPOUND_STRING("The cold deepens."),
    [STRINGID_ENCREGICECHILLONE]                    = COMPOUND_STRING("The air is thickening.\nEverything is getting slower."),
    [STRINGID_ENCREGICECHILLTWO]                    = COMPOUND_STRING("The battlefield has almost\nstopped moving."),
    [STRINGID_ENCREGICECHILLRECUR]                  = COMPOUND_STRING("The cold presses in from every\nside."),
    [STRINGID_ENCREGICESHEDS]                       = COMPOUND_STRING("The cold no longer troubles\nRegice."),
    [STRINGID_ENCREGICEFROZENYOU]                   = COMPOUND_STRING("The air locks solid. Your\nPOKéMON can't move!"),
    [STRINGID_ENCREGICEFROZENBOSS]                  = COMPOUND_STRING("The air locks solid. Regice\ncan't move!"),
    [STRINGID_ENCREGICETHAWFIRE]                    = COMPOUND_STRING("The flames drive the cold back!"),
    [STRINGID_ENCREGICETHAWSTRIKE]                  = COMPOUND_STRING("The ice cracks under the weight\nof that blow!"),
    [STRINGID_ENCREGICETHAWSUN]                     = COMPOUND_STRING("The sunlight burns the frost\naway."),
    [STRINGID_ENCREGICEREFREEZE]                    = COMPOUND_STRING("Regice smothers the sky, and the\nfrost comes back."),
    [STRINGID_ENCREGICESLEEPS]                      = COMPOUND_STRING("Regice sleeps, but the cold\nkeeps its own time."),
    [STRINGID_ENCREGICEABSOLUTEZERO]                = COMPOUND_STRING("ABSOLUTE ZERO.\pEverything stops - except\nRegice.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCREGICEDEEPFREEZE]                  = COMPOUND_STRING("Regice stops moving entirely.\pThe temperature falls past\nanything the field can hold.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCREGICEFROZENTOMBPHASE]             = COMPOUND_STRING("The ice reaches for you. Regice\nwon't let this end.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCREGICETOMBSEAL]                    = COMPOUND_STRING("The ice closes in. There's\nnowhere to go!"),
    [STRINGID_ENCREGICETOMBBREAK]                   = COMPOUND_STRING("The ice splits apart! You can\nmove again."),
    [STRINGID_ENCREGICEWEAKENED]                    = COMPOUND_STRING("Regice's core has gone dark, and\nthe cold with it.\pIt's worn out - now is the\nmoment to catch it!{PAUSE_UNTIL_PRESS}"),

    [STRINGID_ENCREGIROCKAWAKENS]                   = COMPOUND_STRING("Regirock grinds awake, and the\ncavern's stone answers it."),
    [STRINGID_ENCREGIROCKFORTUP]                    = COMPOUND_STRING("Stone drags itself onto\nRegirock's body!"),
    [STRINGID_ENCREGIROCKFORTDOWN]                  = COMPOUND_STRING("The rock around Regirock\ncrumbles away!"),
    [STRINGID_ENCREGIROCKTIERONE]                   = COMPOUND_STRING("Regirock's skin has set like\nstone."),
    [STRINGID_ENCREGIROCKTIERTWO]                   = COMPOUND_STRING("Regirock's core is packed too\ntight to find a weak point."),
    [STRINGID_ENCREGIROCKTIERTHREE]                 = COMPOUND_STRING("A shell of rock closes over\nRegirock. Blows land soft!"),
    [STRINGID_ENCREGIROCKTIERFOUR]                  = COMPOUND_STRING("Regirock is a fortress now. It\nwon't be worn down!"),
    [STRINGID_ENCREGIROCKTIERFIVE]                  = COMPOUND_STRING("Regirock has become a MOUNTAIN.\pThere is almost nothing left to\nhit.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCREGIROCKGRIND]                     = COMPOUND_STRING("Regirock's plating grinds and\nsettles."),
    [STRINGID_ENCREGIROCKQUIET]                     = COMPOUND_STRING("The dust settles, and Regirock's\nstone knits together."),
    [STRINGID_ENCREGIROCKREBUILD]                   = COMPOUND_STRING("Regirock stops attacking and\nbegins reconstructing its body!"),
    [STRINGID_ENCREGIROCKREBUILDDONE]               = COMPOUND_STRING("The reconstruction finished.\nRegirock is whole again!"),
    [STRINGID_ENCREGIROCKREBUILDBROKE]              = COMPOUND_STRING("The battering shattered the\nreconstruction!"),
    [STRINGID_ENCREGIROCKBRACE]                     = COMPOUND_STRING("Regirock braces behind its\nshell!"),
    [STRINGID_ENCREGIROCKWRATH]                     = COMPOUND_STRING("Regirock's body is breaking\napart.\pIt stops defending, and starts\nthrowing the pieces!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCREGIROCKROCKFALL]                  = COMPOUND_STRING("Regirock tears off a slab and\nhurls it!"),
    [STRINGID_ENCREGIROCKNOTHINGLEFT]               = COMPOUND_STRING("Regirock reaches for a slab to\nthrow - there's nothing left!"),
    [STRINGID_ENCREGIROCKCOLLAPSING]                = COMPOUND_STRING("Regirock can barely hold itself\ntogether.\pThe whole cavern starts coming\ndown.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCREGIROCKBREAKAWAY]                 = COMPOUND_STRING("Another piece of Regirock breaks\naway!"),
    [STRINGID_ENCREGIROCKMOUNTAINFALLS]             = COMPOUND_STRING("MOUNTAIN'S COLLAPSE!\pRegirock brings everything down\nat once!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCREGIROCKNOSLEEP]                   = COMPOUND_STRING("Regirock doesn't sleep. The\nstone only settles harder."),
    [STRINGID_ENCREGIROCKWEAKENED]                  = COMPOUND_STRING("The rubble settles, and Regirock\nlies exposed.\pIt's worn out - now is the\nmoment to catch it!{PAUSE_UNTIL_PRESS}"),

    [STRINGID_ENCREGISTEELAWAKENS]                  = COMPOUND_STRING("Registeel's eyes light in\nsequence. It is measuring you."),
    [STRINGID_ENCREGISTEELFILED]                    = COMPOUND_STRING("Registeel's plating\nreshapes itself against\p{B_BUFF1}-type attacks!"),
    [STRINGID_ENCREGISTEELHARDENED]                 = COMPOUND_STRING("Registeel has already solved\n{B_BUFF1}.\pIts plating thickens further."),
    [STRINGID_ENCREGISTEELEVICTED]                  = COMPOUND_STRING("Registeel discards its\n{B_BUFF2} configuration to\pmake room for {B_BUFF1}."),
    [STRINGID_ENCREGISTEELDEGRADED]                 = COMPOUND_STRING("Registeel's {B_BUFF1}\nconfiguration has degraded."),
    [STRINGID_ENCREGISTEELCONTRADICTION]            = COMPOUND_STRING("Registeel's readings contradict\neach other!\pIts {B_BUFF1} configuration\nfails."),
    [STRINGID_ENCREGISTEELASSAULT]                  = COMPOUND_STRING("Registeel routes everything into\nits weapons!"),
    [STRINGID_ENCREGISTEELBALANCED]                 = COMPOUND_STRING("Registeel rebalances itself."),
    [STRINGID_ENCREGISTEELFORTRESS]                 = COMPOUND_STRING("Registeel routes everything into\nits plating."),
    [STRINGID_ENCREGISTEELLOADONE]                  = COMPOUND_STRING("Registeel is holding a\nconfiguration against you."),
    [STRINGID_ENCREGISTEELLOADTWO]                  = COMPOUND_STRING("Registeel has an answer\nfor almost everything\pyou've shown it."),
    [STRINGID_ENCREGISTEELOPTIMIZATION]             = COMPOUND_STRING("Registeel's processing\naccelerates.\pIt's filing far more than it\nwas.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCREGISTEELPERFECTCONFIG]            = COMPOUND_STRING("PERFECT CONFIGURATION!\pRegisteel stops choosing between\nits plating and its weapons.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCREGISTEELOVERHEAT]                 = COMPOUND_STRING("CRITICAL OVERHEAT!\pRegisteel's configurations\nburn away, and it can't\pfile anything new.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCREGISTEELCOOLING]                  = COMPOUND_STRING("Registeel's systems are still\ncooling."),
    [STRINGID_ENCREGISTEELONLINE]                   = COMPOUND_STRING("Registeel's systems come back\nonline."),
    [STRINGID_ENCREGISTEELWEAKENED]                 = COMPOUND_STRING("Registeel's core dims, and its\nplating goes slack.\pIt's worn out - now is the\nmoment to catch it!{PAUSE_UNTIL_PRESS}"),

    [STRINGID_ENCARCEUSGAZES]               = COMPOUND_STRING("Arceus gazes upon you."),
    [STRINGID_ENCARCEUSSILENCE]             = COMPOUND_STRING("The world falls completely\nsilent.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCARCEUSAUTHORITYZERO]       = COMPOUND_STRING("Arceus's presence thins.\pThe air is only air."),
    [STRINGID_ENCARCEUSAUTHORITYONE]        = COMPOUND_STRING("Arceus's will settles over the\nfield."),
    [STRINGID_ENCARCEUSAUTHORITYTWO]        = COMPOUND_STRING("The world leans toward Arceus."),
    [STRINGID_ENCARCEUSAUTHORITYTHREE]      = COMPOUND_STRING("ARCEUS'S AUTHORITY IS ABSOLUTE.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCARCEUSAUTHORITYWAVERS]     = COMPOUND_STRING("Arceus reaches for something,\nand finds nothing there.\pIts authority wavers."),
    [STRINGID_ENCARCEUSPLATESAWAKEN]        = COMPOUND_STRING("Arceus's ring begins to glow.\pThe Plates surrounding its body\nresonate.\pTHE PLATES AWAKEN.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCARCEUSPLATETURNS]          = COMPOUND_STRING("Arceus's ring turns."),
    [STRINGID_ENCARCEUSNEXTFLAME]           = COMPOUND_STRING("The FLAME PLATE rises to the\nfore."),
    [STRINGID_ENCARCEUSNEXTSPLASH]          = COMPOUND_STRING("The SPLASH PLATE rises to the\nfore."),
    [STRINGID_ENCARCEUSNEXTZAP]             = COMPOUND_STRING("The ZAP PLATE rises to the fore."),
    [STRINGID_ENCARCEUSNEXTMEADOW]          = COMPOUND_STRING("The MEADOW PLATE rises to the\nfore."),
    [STRINGID_ENCARCEUSNEXTICICLE]          = COMPOUND_STRING("The ICICLE PLATE rises to the\nfore."),
    [STRINGID_ENCARCEUSNEXTFIST]            = COMPOUND_STRING("The FIST PLATE rises to the\nfore."),
    [STRINGID_ENCARCEUSLAWFLAME]            = COMPOUND_STRING("THE LAW OF FLAME.\pArceus decrees that the world\nshall burn.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCARCEUSLAWWATER]            = COMPOUND_STRING("THE LAW OF WATER.\pArceus decrees that the world\nshall drown.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCARCEUSLAWLIGHTNING]        = COMPOUND_STRING("THE LAW OF LIGHTNING.\pArceus decrees that nothing\nshall outrun it.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCARCEUSLAWLIFE]             = COMPOUND_STRING("THE LAW OF LIFE.\pArceus decrees that the world\nshall take what it needs.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCARCEUSLAWFROST]            = COMPOUND_STRING("THE LAW OF FROST.\pArceus decrees that all things\nshall be stilled.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCARCEUSLAWFORCE]            = COMPOUND_STRING("THE LAW OF FORCE.\pArceus decrees that strength\nshall answer to nothing.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCARCEUSLAWHOLDS]            = COMPOUND_STRING("Arceus's law still holds over\nthe field."),
    [STRINGID_ENCARCEUSPLATEBREAKS]         = COMPOUND_STRING("The Plate cracks and goes dark!\pArceus's law collapses with it.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCARCEUSDECREESTRENGTH]      = COMPOUND_STRING("ARCEUS HAS DECLARED A DECREE.\pLet strength be exalted.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCARCEUSDECREEMIND]          = COMPOUND_STRING("ARCEUS HAS DECLARED A DECREE.\pLet the mind be exalted.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCARCEUSDECREEBINDING]       = COMPOUND_STRING("ARCEUS HAS DECLARED A DECREE.\pNone may leave the Creator's\npresence.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCARCEUSDECREEBALANCE]       = COMPOUND_STRING("ARCEUS HAS DECLARED A DECREE.\pLet all things be equal.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCARCEUSDECREETOLL]          = COMPOUND_STRING("ARCEUS HAS DECLARED A DECREE.\pExistence has a price.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCARCEUSHOLDSSTRENGTH]       = COMPOUND_STRING("The Decree of Strength still\nholds."),
    [STRINGID_ENCARCEUSHOLDSMIND]           = COMPOUND_STRING("The Decree of Mind still holds."),
    [STRINGID_ENCARCEUSHOLDSBINDING]        = COMPOUND_STRING("The Decree of Binding still\nholds."),
    [STRINGID_ENCARCEUSHOLDSBALANCE]        = COMPOUND_STRING("The Decree of Balance still\nholds."),
    [STRINGID_ENCARCEUSHOLDSTOLL]           = COMPOUND_STRING("The Decree of Toll still holds."),
    [STRINGID_ENCARCEUSDECREEENDS]          = COMPOUND_STRING("Arceus lets the Decree fall\naway."),
    [STRINGID_ENCARCEUSDECREECRACKS]        = COMPOUND_STRING("The Decree cracks under the\npressure!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCARCEUSDECIDESTOJUDGE]      = COMPOUND_STRING("Arceus raises its head.\pThe Plates begin to orbit\nfaster.\pARCEUS HAS DECIDED TO JUDGE YOU.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCARCEUSRECALLSCREATION]     = COMPOUND_STRING("ARCEUS RECALLS THE POWERS OF\nCREATION.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCARCEUSJUDGES]              = COMPOUND_STRING("ARCEUS HAS PASSED JUDGMENT ON\nYOUR POKéMON.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCARCEUSMARKED]              = COMPOUND_STRING("Arceus's gaze has not left your\nPOKéMON."),
    [STRINGID_ENCARCEUSCONDEMNED]           = COMPOUND_STRING("Arceus's attacks begin to\nconverge.\pJUDGMENT NEARS.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCARCEUSMARKTRANSFERS]       = COMPOUND_STRING("The Judgment settles onto the\nPOKéMON that took the field."),
    [STRINGID_ENCARCEUSCONCENTRATIONBROKEN] = COMPOUND_STRING("You broke its concentration!\pThe Judgment scatters.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCARCEUSJUDGMENT]            = COMPOUND_STRING("JUDGMENT.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCARCEUSECHOTEMPORAL]        = COMPOUND_STRING("Time folds around Arceus."),
    [STRINGID_ENCARCEUSECHOSPATIAL]         = COMPOUND_STRING("Space closes.\pThere is nowhere to go."),
    [STRINGID_ENCARCEUSECHODISTORTION]      = COMPOUND_STRING("The world turns inside out."),
    [STRINGID_ENCARCEUSCREATIONBEGINS]      = COMPOUND_STRING("Arceus looks toward the heavens."),
    [STRINGID_ENCARCEUSTHEREISNOTHING]      = COMPOUND_STRING("For a moment, there is nothing.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCARCEUSASPARK]              = COMPOUND_STRING("A spark.\pThen light."),
    [STRINGID_ENCARCEUSWORLDREMADE]         = COMPOUND_STRING("Arceus has recreated the world.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCARCEUSSEENENOUGH]          = COMPOUND_STRING("ARCEUS HAS SEEN ENOUGH.\pIt shapes itself against what\nyou have shown it.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCARCEUSPLATESSHATTER]       = COMPOUND_STRING("Arceus's body begins to radiate\nan impossible light.\pTHE PLATES SHATTER.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCARCEUSORIGINALONE]         = COMPOUND_STRING("THE ORIGINAL ONE.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCARCEUSPREPARESJUDGMENT]    = COMPOUND_STRING("ARCEUS PREPARES TO PASS JUDGMENT\nUPON THE WORLD.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCARCEUSWORLDAWAITS]         = COMPOUND_STRING("THE WORLD AWAITS YOUR RESPONSE."),
    [STRINGID_ENCARCEUSREFUSESTOYIELD]      = COMPOUND_STRING("Your POKéMON refuses to yield.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCARCEUSABSOLUTE]            = COMPOUND_STRING("Arceus's authority is absolute.\pThere is nothing to do but\nendure.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCARCEUSCHARGEDEEPENS]       = COMPOUND_STRING("The gathering light deepens."),
    [STRINGID_ENCARCEUSLADDEREIGHTEEN]      = COMPOUND_STRING("Eighteen Plates orbit the\ngathering light."),
    [STRINGID_ENCARCEUSLADDERTWELVE]        = COMPOUND_STRING("Twelve Plates orbit the\ngathering light."),
    [STRINGID_ENCARCEUSLADDEREIGHT]         = COMPOUND_STRING("Eight Plates orbit the gathering\nlight."),
    [STRINGID_ENCARCEUSLADDERFOUR]          = COMPOUND_STRING("Four Plates orbit the gathering\nlight."),
    [STRINGID_ENCARCEUSLADDERONE]           = COMPOUND_STRING("One Plate orbits the gathering\nlight."),
    [STRINGID_ENCARCEUSCHARGECRACKS]        = COMPOUND_STRING("The gathering light cracks!"),
    [STRINGID_ENCARCEUSFINALPLATE]          = COMPOUND_STRING("THE FINAL PLATE SHATTERS.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCARCEUSREELS]               = COMPOUND_STRING("Arceus reels, and the light\ncomes apart in its hands.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCARCEUSJUDGMENTCREATION]    = COMPOUND_STRING("JUDGMENT: CREATION.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCARCEUSFINALMOMENT]         = COMPOUND_STRING("Arceus stands silently.\pThe last blow is yours to\nstrike.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCARCEUSNOTSTILLED]          = COMPOUND_STRING("The Original One does not sleep,\nand is not stilled.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCARCEUSWEAKENED]            = COMPOUND_STRING("The light around Arceus fades to\nalmost nothing.\pIt's weakened - now is the\nmoment to catch it!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCAZELFAWAKENS]      = COMPOUND_STRING("Azelf's eyes snap open.\pIt does not flee. It does not\nhesitate.\pIt comes straight for you.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCAZELFFALTERS]      = COMPOUND_STRING("That one got through!\pAzelf's resolve falters!"),
    [STRINGID_ENCAZELFSTEELS]       = COMPOUND_STRING("Your blows only steel its will!"),
    [STRINGID_ENCAZELFENDURED]      = COMPOUND_STRING("You weathered everything it had.\pAzelf's resolve falters!"),
    [STRINGID_ENCAZELFSHRUGS]       = COMPOUND_STRING("Azelf throws the affliction off!\pIts will does not waver - it\nonly sharpens."),
    [STRINGID_ENCAZELFSNAPBACK]     = COMPOUND_STRING("Azelf refuses to be diminished!"),
    [STRINGID_ENCAZELFEMBOLDENED]   = COMPOUND_STRING("Azelf's eyes blaze.\pYour struggle only steels it!"),
    [STRINGID_ENCAZELFSTEADY]       = COMPOUND_STRING("Azelf's stance loosens."),
    [STRINGID_ENCAZELFUNYIELDING]   = COMPOUND_STRING("AZELF IS UNYIELDING.\pNothing you put on it will stay."),
    [STRINGID_ENCAZELFUNDIMINISHED] = COMPOUND_STRING("AZELF IS UNDIMINISHED.\pIts strength will not be dulled."),
    [STRINGID_ENCAZELFUNBOWED]      = COMPOUND_STRING("AZELF IS UNBOWED.\pIt is holding something back."),
    [STRINGID_ENCAZELFUNMATCHED]    = COMPOUND_STRING("AZELF IS UNMATCHED.\pIt moves before you finish\ndeciding."),
    [STRINGID_ENCAZELFUNBREAKABLE]  = COMPOUND_STRING("AZELF'S WILL IS UNBREAKABLE.\pEverything you do to it, it\nreturns.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCAZELFBURNS]        = COMPOUND_STRING("Azelf's will still burns\nwhite-hot."),
    [STRINGID_ENCAZELFNARROWS]      = COMPOUND_STRING("Azelf's eyes narrow.\pIt is only getting started."),
    [STRINGID_ENCAZELFRESOLUTE]     = COMPOUND_STRING("AZELF IS RESOLUTE.\pHalf measures will not reach it\nnow.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCAZELFREFUSES]      = COMPOUND_STRING("AZELF REFUSES TO FALL!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCAZELFFLOODSBACK]   = COMPOUND_STRING("Its will floods back, brighter\nthan before."),
    [STRINGID_ENCAZELFHOLLOW]       = COMPOUND_STRING("Azelf reaches for its will - and\nfinds nothing there.\pIt has spent everything it had.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCAZELFLASTSTAND]    = COMPOUND_STRING("Azelf plants itself between you\nand the way out.\pLAST STAND.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCAZELFHOLDS]        = COMPOUND_STRING("Azelf will not fall while it\nstill has a will to fight!"),
    [STRINGID_ENCAZELFLASH]         = COMPOUND_STRING("Azelf's will lashes back!"),
    [STRINGID_ENCAZELFBREAKS]       = COMPOUND_STRING("Azelf's will finally breaks.\pIts body slumps. Whatever was\nholding it up is gone.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCAZELFWEAKENED]     = COMPOUND_STRING("Azelf's eyes dim, and it settles\nto the ground.\pIt's worn out - now is the\nmoment to catch it!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCUXIEAWAKENS]          = COMPOUND_STRING("Uxie opens its eyes.\pIt does not look at your\nPOKéMON.\pIt looks at you.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCUXIEREADPHYS]         = COMPOUND_STRING("Uxie foresees a blow struck with\nforce."),
    [STRINGID_ENCUXIEREADSPEC]         = COMPOUND_STRING("Uxie foresees a blow struck with\nwill."),
    [STRINGID_ENCUXIEREADSTAT]         = COMPOUND_STRING("Uxie foresees that you will not\nstrike at all."),
    [STRINGID_ENCUXIENOREAD]           = COMPOUND_STRING("Uxie's gaze turns inward.\pIt makes no reading this turn."),
    [STRINGID_ENCUXIEFORESEEN]         = COMPOUND_STRING("Just as Uxie foresaw."),
    [STRINGID_ENCUXIEFORESEENSTAT]     = COMPOUND_STRING("Uxie foresaw even your\nhesitation."),
    [STRINGID_ENCUXIEDEFIED]           = COMPOUND_STRING("Uxie's prediction was wrong!"),
    [STRINGID_ENCUXIEFILED]            = COMPOUND_STRING("Uxie files away the shape of\n{B_BUFF1}.\pIt will not be caught by that\nagain."),
    [STRINGID_ENCUXIEHARDENED]         = COMPOUND_STRING("Uxie already knows {B_BUFF1}.\pIts understanding of it deepens."),
    [STRINGID_ENCUXIEEVICTED]          = COMPOUND_STRING("Uxie sets aside what\nit knew of {B_BUFF2} to\pmake room for {B_BUFF1}."),
    [STRINGID_ENCUXIEFORGETS]          = COMPOUND_STRING("Uxie loses its grasp on\n{B_BUFF1}!"),
    [STRINGID_ENCUXIEUNSEEN]           = COMPOUND_STRING("Uxie has never seen this from\nyou!"),
    [STRINGID_ENCUXIETESTPHYS]         = COMPOUND_STRING("Uxie challenges your knowledge!\pIt foresees a blow struck with\nforce.\pWill you prove it right?"),
    [STRINGID_ENCUXIETESTSPEC]         = COMPOUND_STRING("Uxie challenges your knowledge!\pIt foresees a blow struck with\nwill.\pWill you prove it right?"),
    [STRINGID_ENCUXIETESTSTAT]         = COMPOUND_STRING("Uxie challenges your knowledge!\pIt foresees that you will not\nstrike at all.\pWill you prove it right?"),
    [STRINGID_ENCUXIEVOWYES]           = COMPOUND_STRING("You meet its gaze and agree."),
    [STRINGID_ENCUXIEVOWNO]            = COMPOUND_STRING("You meet its gaze and deny it."),
    [STRINGID_ENCUXIEVOWKEPT]          = COMPOUND_STRING("Your word and your deed agree."),
    [STRINGID_ENCUXIEVOWBROKEN]        = COMPOUND_STRING("Uxie cannot reconcile your words\nwith your actions!"),
    [STRINGID_ENCUXIEBACKLASH]         = COMPOUND_STRING("The contradiction reels through\nyour POKéMON's mind!"),
    [STRINGID_ENCUXIEK0]               = COMPOUND_STRING("Uxie is reconsidering what it\nknows."),
    [STRINGID_ENCUXIEK1]               = COMPOUND_STRING("Uxie is beginning to understand\nyou."),
    [STRINGID_ENCUXIEK2]               = COMPOUND_STRING("Uxie has grasped the shape of\nyour fighting."),
    [STRINGID_ENCUXIEK3]               = COMPOUND_STRING("Uxie anticipates you now."),
    [STRINGID_ENCUXIEK4]               = COMPOUND_STRING("Uxie knows your intentions\nbefore you form them."),
    [STRINGID_ENCUXIEK5]               = COMPOUND_STRING("OMNISCIENCE.\pUxie knows everything.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCUXIEUNDERSTANDING]    = COMPOUND_STRING("Uxie has learned almost\neverything about you.\pIt is no longer studying. It is\nconfirming.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCUXIEOMNISCIENCE]      = COMPOUND_STRING("Uxie closes its eyes.\pIt no longer needs to watch you\nto know what you will do.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCUXIEWEAKENED]         = COMPOUND_STRING("Uxie's eyes lose their focus,\nand it sinks toward the ground.\pIt's weakened - now is the\nmoment to catch it!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCUXIEREELING]          = COMPOUND_STRING("Uxie is still reeling. It is not\nreading you."),
    [STRINGID_ENCUXIECANNOTCOMPREHEND] = COMPOUND_STRING("Uxie cannot comprehend what just\nhappened!\pEverything it had gathered comes\napart at once.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCUXIEREFORMS]          = COMPOUND_STRING("Uxie gathers itself.\pIt knows you again.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCUXIEREFUSESSLEEP]     = COMPOUND_STRING("Uxie's mind cannot be dimmed.\pIt has already learned from the\nattempt."),
    [STRINGID_ENCUXIEENLIGHTENED]      = COMPOUND_STRING("Your loss has taught it\nsomething."),

    [STRINGID_ENCMESPRITAWAKENS]        = COMPOUND_STRING("Mesprit's eyes open, and they\nare already searching yours.\pIt does not know yet what it\nwill feel about you.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCMESPRITCALM]           = COMPOUND_STRING("Mesprit's face empties.\pCALM."),
    [STRINGID_ENCMESPRITJOY]            = COMPOUND_STRING("Mesprit's eyes brighten.\pJOY."),
    [STRINGID_ENCMESPRITANGER]          = COMPOUND_STRING("Mesprit's expression darkens.\pANGER."),
    [STRINGID_ENCMESPRITFEAR]           = COMPOUND_STRING("Mesprit shrinks back from you.\pFEAR."),
    [STRINGID_ENCMESPRITSADNESS]        = COMPOUND_STRING("Mesprit's gaze falls away from\nyours.\pSADNESS."),
    [STRINGID_ENCMESPRITOVERJOYED]      = COMPOUND_STRING("Its delight has nowhere left to\ngo.\pOVERJOYED."),
    [STRINGID_ENCMESPRITRAGE]           = COMPOUND_STRING("It has stopped protecting itself\nentirely.\pRAGE."),
    [STRINGID_ENCMESPRITPANIC]          = COMPOUND_STRING("It will not let anything reach\nit now.\pPANIC."),
    [STRINGID_ENCMESPRITDESPAIR]        = COMPOUND_STRING("Something in it has quietly\ngiven up.\pDESPAIR."),
    [STRINGID_ENCMESPRITOVERWHELMING]   = COMPOUND_STRING("MESPRIT'S EMOTIONS BECOME\nOVERWHELMING!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCMESPRITSTIRS]          = COMPOUND_STRING("Mesprit has watched you long\nenough.\pSomething stirs in it."),
    [STRINGID_ENCMESPRITCALMHOLDS]      = COMPOUND_STRING("Mesprit is still waiting for you\nto do something."),
    [STRINGID_ENCMESPRITJOYHOLDS]       = COMPOUND_STRING("Mesprit is still delighted with\nyou."),
    [STRINGID_ENCMESPRITANGERHOLDS]     = COMPOUND_STRING("Mesprit's anger has not cooled."),
    [STRINGID_ENCMESPRITFEARHOLDS]      = COMPOUND_STRING("Mesprit is still braced against\nyou."),
    [STRINGID_ENCMESPRITSADNESSHOLDS]   = COMPOUND_STRING("Mesprit's sorrow has not lifted."),
    [STRINGID_ENCMESPRITTHRIVES]        = COMPOUND_STRING("Mesprit's joy spills over, and\nits hurts close up."),
    [STRINGID_ENCMESPRITLASHES]         = COMPOUND_STRING("Mesprit's fury lashes out at\nyour POKéMON!"),
    [STRINGID_ENCMESPRITCOWERS]         = COMPOUND_STRING("Mesprit cannot stop flinching."),
    [STRINGID_ENCMESPRITHIDES]          = COMPOUND_STRING("Mesprit hides behind its own\nterror!"),
    [STRINGID_ENCMESPRITDRAINS]         = COMPOUND_STRING("Mesprit's sorrow pulls at your\nPOKéMON!"),
    [STRINGID_ENCMESPRITWASHESOVER]     = COMPOUND_STRING("Mesprit's emotions wash over\nyour POKéMON!"),
    [STRINGID_ENCMESPRITCONTJOY]        = COMPOUND_STRING("It is swept up in the delight."),
    [STRINGID_ENCMESPRITCONTANGER]      = COMPOUND_STRING("It burns to strike, and sees\nless for it."),
    [STRINGID_ENCMESPRITCONTFEAR]       = COMPOUND_STRING("A cold weight settles on it."),
    [STRINGID_ENCMESPRITCONTSADNESS]    = COMPOUND_STRING("The strength goes out of it."),
    [STRINGID_ENCMESPRITCONTFADES]      = COMPOUND_STRING("The borrowed feeling drains out\nof your POKéMON."),
    [STRINGID_ENCMESPRITSHIFTSOFF]      = COMPOUND_STRING("Mesprit's mood shifts, and the\naffliction slips away."),
    [STRINGID_ENCMESPRITSORROWCLEARS]   = COMPOUND_STRING("Mesprit's sorrow washes\neverything else away."),
    [STRINGID_ENCMESPRITUNSTABLE]       = COMPOUND_STRING("Mesprit can no longer hold onto\na feeling.\pIts moods begin to turn on a\nsingle moment.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCMESPRITEMPATHY]        = COMPOUND_STRING("Mesprit reaches into your\nPOKéMON's heart.\pIt has stopped reacting to you.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCMESPRITEMPATHYANGER]   = COMPOUND_STRING("It has been carrying your fury\nthis whole time.\pNow it gives it back."),
    [STRINGID_ENCMESPRITEMPATHYFEAR]    = COMPOUND_STRING("It has been carrying your\ncaution this whole time.\pNow it hides behind it."),
    [STRINGID_ENCMESPRITEMPATHYSADNESS] = COMPOUND_STRING("It has been carrying your doubt\nthis whole time.\pNow it has nothing else left."),
    [STRINGID_ENCMESPRITWEAKENED]       = COMPOUND_STRING("Mesprit's eyes go quiet, and it\ndrifts down to you.\pIt's worn out - now is the\nmoment to catch it!{PAUSE_UNTIL_PRESS}"),

    [STRINGID_ENCREGIGIGASSLUMBERS]        = COMPOUND_STRING("A colossus fills the chamber,\nand it does not look at you.\pRegigigas is asleep.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCREGIGIGASRUNG0]           = COMPOUND_STRING("Regigigas does not move.\pIts hide turns your blow aside\nlike weather."),
    [STRINGID_ENCREGIGIGASRUNG1]           = COMPOUND_STRING("Something behind Regigigas' eyes\nhas begun to move."),
    [STRINGID_ENCREGIGIGASRUNG2]           = COMPOUND_STRING("The floor shakes.\pRegigigas is pulling itself\nupright."),
    [STRINGID_ENCREGIGIGASRUNG3]           = COMPOUND_STRING("Regigigas is awake.\pIts hide no longer shrugs you\noff the way it did.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCREGIGIGASRUNG4]           = COMPOUND_STRING("Regigigas remembers what it was\nbuilt to do."),
    [STRINGID_ENCREGIGIGASRUNG5]           = COMPOUND_STRING("REGIGIGAS HAS AWAKENED.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCREGIGIGASREMEMBERSWEIGHT] = COMPOUND_STRING("Regigigas remembers its weight."),
    [STRINGID_ENCREGIGIGASREMEMBERSGRIP]   = COMPOUND_STRING("Regigigas remembers how to close\nits hands."),
    [STRINGID_ENCREGIGIGASDOESNOTSTIR]     = COMPOUND_STRING("Regigigas does not stir."),
    [STRINGID_ENCREGIGIGASFALTERS]         = COMPOUND_STRING("Regigigas' awakening falters."),
    [STRINGID_ENCREGIGIGASKNITS]           = COMPOUND_STRING("Its wounds knit closed in the\nstillness."),
    [STRINGID_ENCREGIGIGASGRUDGESETUP]     = COMPOUND_STRING("Regigigas remembers how you\ngrew."),
    [STRINGID_ENCREGIGIGASGRUDGESWITCH]    = COMPOUND_STRING("Regigigas remembers the one that\nran."),
    [STRINGID_ENCREGIGIGASGRUDGESTATUS]    = COMPOUND_STRING("Regigigas remembers the poison\nin its joints."),
    [STRINGID_ENCREGIGIGASCRUSHESBOOSTS]   = COMPOUND_STRING("Regigigas shakes the ground out\nfrom under you!"),
    [STRINGID_ENCREGIGIGASTITANMOVES]      = COMPOUND_STRING("Regigigas sets one hand on the\nfloor and stands.\pTHE TITAN MOVES.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCREGIGIGASTEARSSCREENS]    = COMPOUND_STRING("The air you were hiding behind\ntears apart."),
    [STRINGID_ENCREGIGIGASFULLAWAKENING]   = COMPOUND_STRING("Regigigas' seals give way all at\nonce.\pFULL AWAKENING.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCREGIGIGASWEAKENED]        = COMPOUND_STRING("The titan sinks back to one\nknee, and does not rise.\pNow - now is the moment to catch\nit!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCREGIGIGASSECONDSTRIKE]    = COMPOUND_STRING("Regigigas moves before you can!"),
    [STRINGID_ENCREGIGIGASFORCECHARGE]     = COMPOUND_STRING("Regigigas plants its feet.\pThe ground begins to split."),
    [STRINGID_ENCREGIGIGASFORCEFIRE]       = COMPOUND_STRING("CONTINENTAL FORCE!"),
    [STRINGID_ENCREGIGIGASFORCEREELING]    = COMPOUND_STRING("Regigigas' own strength has left\nit reeling."),
    [STRINGID_ENCREGIGIGASSLOWING]         = COMPOUND_STRING("Regigigas' movements are\nslowing."),
    [STRINGID_ENCREGIGIGASFALTERING]       = COMPOUND_STRING("Regigigas' breathing has turned\nragged."),
    [STRINGID_ENCREGIGIGASCOLLAPSE]        = COMPOUND_STRING("Regigigas' colossal body finally\ngives out beneath it!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCREGIGIGASSTILLDOWN]       = COMPOUND_STRING("Regigigas' body still will not\nanswer it."),
    [STRINGID_ENCREGIGIGASUPRIGHT]         = COMPOUND_STRING("Regigigas hauls itself upright."),

    [STRINGID_ENCROTOMINTRO1]           = COMPOUND_STRING("The appliance in the corner\ncomes on by itself.\pIts screen fills with a face.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCROTOMINTRO2]           = COMPOUND_STRING("Rotom is running something it\nwas not built to run."),
    [STRINGID_ENCROTOMPROTOCOLHEAT]     = COMPOUND_STRING("OVERHEAT PROTOCOL.\pThe element glows white, and\nstays glowing."),
    [STRINGID_ENCROTOMPROTOCOLWASH]     = COMPOUND_STRING("FLOOD PROTOCOL.\pThe drum starts to spin."),
    [STRINGID_ENCROTOMPROTOCOLFROST]    = COMPOUND_STRING("CRYO PROTOCOL.\pThe air in front of the vents\ngoes white."),
    [STRINGID_ENCROTOMPROTOCOLFAN]      = COMPOUND_STRING("SPEED PROTOCOL.\pThe blades come up past hearing."),
    [STRINGID_ENCROTOMPROTOCOLMOW]      = COMPOUND_STRING("HARVEST PROTOCOL.\pThe floor answers with grass."),
    [STRINGID_ENCROTOMFLUSH]            = COMPOUND_STRING("The old machine powers down, and\ntakes everything on it with it."),
    [STRINGID_ENCROTOMWASHSCOURS]       = COMPOUND_STRING("The flood scours the field bare."),
    [STRINGID_ENCROTOMWASHREGEN]        = COMPOUND_STRING("Coolant runs through Rotom, and\nthe damage goes quiet."),
    [STRINGID_ENCROTOMFANSTRIKE]        = COMPOUND_STRING("Rotom is already moving when you\nlook up!"),
    [STRINGID_ENCROTOMOVERCLOCK1]       = COMPOUND_STRING("Something in Rotom starts to\nhum."),
    [STRINGID_ENCROTOMOVERCLOCK2]       = COMPOUND_STRING("The hum climbs a note."),
    [STRINGID_ENCROTOMOVERCLOCK3]       = COMPOUND_STRING("Rotom is running past what the\ncasing was rated for."),
    [STRINGID_ENCROTOMOVERCLOCK4]       = COMPOUND_STRING("The casing has begun to blister."),
    [STRINGID_ENCROTOMOVERCLOCK5]       = COMPOUND_STRING("EVERYTHING IN ROTOM IS\nSCREAMING.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCROTOMHARDENS]          = COMPOUND_STRING("Rotom pulls the heat inward and\nsets it against you.\pYour attacks are barely reaching\nit now.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCROTOMSTRAINING]        = COMPOUND_STRING("Rotom is holding itself\ntogether, and holding\pyou off with it."),
    [STRINGID_ENCROTOMFEDTHEMOTOR]      = COMPOUND_STRING("The current pours into Rotom and\nthe motor takes it."),
    [STRINGID_ENCROTOMSYSTEMOVERLOAD]   = COMPOUND_STRING("SYSTEM OVERLOAD!\pRotom is thrown out of\nthe machine, and lands\pwith nothing left running.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCROTOMCRITICALFAILURE]  = COMPOUND_STRING("CRITICAL SYSTEM FAILURE!\pEvery possession fails at once.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCROTOMSTILLDOWN]        = COMPOUND_STRING("Rotom is still trying to bring\nitself back up."),
    [STRINGID_ENCROTOMREBOOT]           = COMPOUND_STRING("Rotom finds a socket and slides\nback in."),
    [STRINGID_ENCROTOMHIJACKCONTROL]    = COMPOUND_STRING("Rotom reaches into your\ncontrols.\p{B_BUFF1} no longer answers!"),
    [STRINGID_ENCROTOMHIJACKPOWER]      = COMPOUND_STRING("Rotom cuts the power to the item\nyour Pokémon was holding!"),
    [STRINGID_ENCROTOMHIJACKFIELDFIRE]  = COMPOUND_STRING("Rotom vents the burners across\nyour side of the field!"),
    [STRINGID_ENCROTOMHIJACKFIELDSWAMP] = COMPOUND_STRING("Rotom floods your side of the\nfield and leaves it thick!"),
    [STRINGID_ENCROTOMROGUEPROGRAM]     = COMPOUND_STRING("Rotom stops pretending to be an\nappliance.\pROGUE PROGRAM.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCROTOMTEARSSCREENS]     = COMPOUND_STRING("The field you were hiding behind\nis wiped."),
    [STRINGID_ENCROTOMTAKEOVER]         = COMPOUND_STRING("Every machine in the room turns\nto face you at once.\pTOTAL SYSTEM TAKEOVER.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCROTOMWEAKENED]         = COMPOUND_STRING("Rotom flickers, and cannot hold\na shape any more.\pNow - now is the moment to catch\nit!{PAUSE_UNTIL_PRESS}"),

    [STRINGID_ENCSHAYMININTRO1]           = COMPOUND_STRING("The clearing is deep in\ngracidea, and every flower\pin it has turned to face you.\pSomething in the middle of them\nlifts its head.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCSHAYMININTRO2]           = COMPOUND_STRING("Shaymin does not step back."),
    [STRINGID_ENCSHAYMINBLOOM3]           = COMPOUND_STRING("THE WHOLE CLEARING OPENS AT\nONCE.\pShaymin stands in the middle\nof it with nothing held\pback, and nothing held up.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCSHAYMINBLOOM2]           = COMPOUND_STRING("The clearing is thick and\ngreen now, and the air\pover it has gone sweet.\pShaymin has stopped bracing.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCSHAYMINBLOOM1]           = COMPOUND_STRING("Shoots come up through the dirt\nwhere Shaymin is standing."),
    [STRINGID_ENCSHAYMINNEUTRAL]          = COMPOUND_STRING("The clearing goes quiet, and\nShaymin watches you across it."),
    [STRINGID_ENCSHAYMINBLIGHT1]          = COMPOUND_STRING("The flowers nearest Shaymin have\nstarted to curl."),
    [STRINGID_ENCSHAYMINBLIGHT2]          = COMPOUND_STRING("The gracidea is going brown from\nthe edges in.\pShaymin will not settle.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCSHAYMINBLIGHT3]          = COMPOUND_STRING("NOTHING IS LEFT GROWING HERE.\pShaymin turns on the bare\nground, and the ground catches.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCSHAYMINSTANDINGBLOOM]    = COMPOUND_STRING("The clearing is still growing\naround Shaymin."),
    [STRINGID_ENCSHAYMINSTANDINGBLIGHT]   = COMPOUND_STRING("The clearing is still dying\naround Shaymin."),
    [STRINGID_ENCSHAYMINTOSKY]            = COMPOUND_STRING("Shaymin cannot stand on ground\nlike this.\pIt folds itself into something\nbuilt to leave.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCSHAYMINTOLAND]           = COMPOUND_STRING("Shaymin comes down, and lets the\ngrass take its weight again."),
    [STRINGID_ENCSHAYMINGRATITUDE]        = COMPOUND_STRING("Shaymin looks at what the two of\nyou have made of this clearing.\pIt is grateful.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCSHAYMINGRATITUDEGIFT]    = COMPOUND_STRING("The gratitude is not kept. It is\nhanded back."),
    [STRINGID_ENCSHAYMININDIFFERENT]      = COMPOUND_STRING("Shaymin is still deciding what\nthis battle is."),
    [STRINGID_ENCSHAYMINDISTRESS]         = COMPOUND_STRING("Shaymin cries out, and\nthe sound does not stop\pwhere the clearing does."),
    [STRINGID_ENCSHAYMINDISTRESSFIELD]    = COMPOUND_STRING("The dead growth on your side of\nthe field goes up!"),
    [STRINGID_ENCSHAYMINFLAREARRIVES]     = COMPOUND_STRING("Shaymin gathers in everything\nthe clearing has left to give.\pSEED FLARE.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCSHAYMINFLARE]            = COMPOUND_STRING("Shaymin releases the flower on\nits back!"),
    [STRINGID_ENCSHAYMINFLAREBLOOM]       = COMPOUND_STRING("The burst passes over your\nPokémon like weather, and\pcloses over Shaymin's wounds."),
    [STRINGID_ENCSHAYMINFLAREBLIGHTSTRIP] = COMPOUND_STRING("Everything you had raised over\nyour side is stripped away!"),
    [STRINGID_ENCSHAYMINFLAREBLIGHT]      = COMPOUND_STRING("The burst is all thorn and no\nbloom!"),
    [STRINGID_ENCSHAYMINWEAKENEDBLOOM]    = COMPOUND_STRING("Shaymin settles into the grass\nin front of you, and waits.\pNow - now is the moment to catch\nit!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCSHAYMINWEAKENEDNEUTRAL]  = COMPOUND_STRING("Shaymin is spent, and watching\nyou.\pNow - now is the moment to catch\nit!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCSHAYMINWEAKENEDBLIGHT]   = COMPOUND_STRING("Shaymin will not come down, and\nit will not stop watching you.\pNow - now is the moment to catch\nit!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCHEATRANINTRO1]           = COMPOUND_STRING("The heat comes off the\nstone in sheets, and the\pwhole dome is breathing.\pHeatran is standing on\nthe ceiling of it, upside\pdown, watching you.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCHEATRANINTRO2]           = COMPOUND_STRING("The mountain is awake too."),
    [STRINGID_ENCHEATRANPRESSURE0]        = COMPOUND_STRING("The mountain settles, and the\nstone underfoot stops humming."),
    [STRINGID_ENCHEATRANPRESSURE1]        = COMPOUND_STRING("Something turns over\nfar below, and the air\pover the dome goes dry."),
    [STRINGID_ENCHEATRANPRESSURE2]        = COMPOUND_STRING("The floor of the dome is glowing\nthrough its cracks."),
    [STRINGID_ENCHEATRANPRESSURE3]        = COMPOUND_STRING("THE BASIN IS RUNNING OVER.\pLava comes across the\nfloor, and the light off\pit fills the whole dome.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCHEATRANPRESSURE4]        = COMPOUND_STRING("The mountain is boiling,\nand Heatran has stopped\pgetting out of its way."),
    [STRINGID_ENCHEATRANPRESSURE5]        = COMPOUND_STRING("EVERYTHING GOES STILL.\pThe mountain draws breath.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCHEATRANSTANDING]         = COMPOUND_STRING("The lava is still moving across\nyour side of the field."),
    [STRINGID_ENCHEATRANLAVAFLOW]         = COMPOUND_STRING("The flow comes down over your\nside of the field!"),
    [STRINGID_ENCHEATRANSCOURED]          = COMPOUND_STRING("It takes everything laid on the\nground with it!"),
    [STRINGID_ENCHEATRANTHERMALSHOCK]     = COMPOUND_STRING("The rock around Heatran\ncontracts with a crack, and\psettles harder than it was."),
    [STRINGID_ENCHEATRANSHOCKSTANDING]    = COMPOUND_STRING("Heatran's shell is still drawn\ntight."),
    [STRINGID_ENCHEATRANSHOCKENDS]        = COMPOUND_STRING("The stone over Heatran works\nitself loose again."),
    [STRINGID_ENCHEATRANCOOLWATER]        = COMPOUND_STRING("The water hits glowing rock and\ngoes straight up as steam!"),
    [STRINGID_ENCHEATRANCOOLICE]          = COMPOUND_STRING("The cold rolls in low across the\nfloor of the dome."),
    [STRINGID_ENCHEATRANSTOKED]           = COMPOUND_STRING("Heatran feeds its own fire back\ninto the mountain!"),
    [STRINGID_ENCHEATRANWEATHERCOOL]      = COMPOUND_STRING("The sky over the dome is working\nagainst the heat."),
    [STRINGID_ENCHEATRANPLUGGED]          = COMPOUND_STRING("Heatran has gone still - and the\nheat has nowhere to go."),
    [STRINGID_ENCHEATRANPOCKETARMED]      = COMPOUND_STRING("The ground beneath {B_PLAYER_MON1_NAME}\nbegins to glow!"),
    [STRINGID_ENCHEATRANPOCKETSTANDING]   = COMPOUND_STRING("The glow under {B_PLAYER_MON1_NAME} is\ngetting brighter!"),
    [STRINGID_ENCHEATRANPOCKETBURST]      = COMPOUND_STRING("The ground opens under\n{B_PLAYER_MON1_NAME}!"),
    [STRINGID_ENCHEATRANPOCKETBLOCKED]    = COMPOUND_STRING("The burst breaks against\n{B_PLAYER_MON1_NAME}'s guard!"),
    [STRINGID_ENCHEATRANPOCKETSWITCHED]   = COMPOUND_STRING("The magma comes up through\nground nobody is standing on."),
    [STRINGID_ENCHEATRANPOCKETCOOLED]     = COMPOUND_STRING("The glow under {B_PLAYER_MON1_NAME} dims,\nand goes out."),
    [STRINGID_ENCHEATRANFIELDBURN]        = COMPOUND_STRING("The heat coming up through the\nfloor scalds your side!"),
    [STRINGID_ENCHEATRANGROUNDED]         = COMPOUND_STRING("{B_PLAYER_MON1_NAME} has nowhere to stand\nthat is not burning!"),
    [STRINGID_ENCHEATRANCOREVENT]         = COMPOUND_STRING("THE DOME VENTS.\pEverything the mountain was\nholding comes out at once - and\pHeatran is in the middle of it.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCHEATRANVENTSTANDING]     = COMPOUND_STRING("Heatran is still shaking off the\nblast."),
    [STRINGID_ENCHEATRANVENTRECOVER]      = COMPOUND_STRING("Heatran plants itself again, and\nthe heat begins to gather."),
    [STRINGID_ENCHEATRANMAGMACORE]        = COMPOUND_STRING("Heatran drops through the crust\ninto the chamber under it.\pWhat climbs back out is burning\nfrom the inside.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCHEATRANMEGA]             = COMPOUND_STRING("Heatran has taken the mountain\ninto itself.\pMEGA HEATRAN.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCHEATRANDRINKS]           = COMPOUND_STRING("Heatran is drinking from the\nflow."),
    [STRINGID_ENCHEATRANERUPTION]         = COMPOUND_STRING("THE ENTIRE MOUNTAIN BEGINS TO\nSHAKE!\pWhatever is coming, it is not\ngoing to wait for either of you.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCHEATRANSUPERNOVA]        = COMPOUND_STRING("THE MOUNTAIN ERUPTS!"),
    [STRINGID_ENCHEATRANSUBSIDED]         = COMPOUND_STRING("The pressure under the\ndome lets go, and nothing\pcomes up after it."),
    [STRINGID_ENCHEATRANSPENT]            = COMPOUND_STRING("Heatran stands in the ruin\nof its own mountain with\pnothing left to draw on."),
    [STRINGID_ENCHEATRANWEAKENED]         = COMPOUND_STRING("Heatran's plating has gone dull,\nand the light behind it is out.\pNow - now is the moment to catch\nit!{PAUSE_UNTIL_PRESS}"),

    // Manaphy, "The Prince of the Sea". The fight never names the Bond meter - every line describes
    // the bond itself, and the loudest lines in the encounter are the Heart Swap pair, which are
    // deliberately the most misleading.
    [STRINGID_ENCMANAPHYINTRO1]           = COMPOUND_STRING("The water here is warm, and it\nis moving on its own.\pManaphy rises through it without\na sound and looks straight\ppast you, at your Pokémon.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCMANAPHYINTRO2]           = COMPOUND_STRING("It has already decided to love\nit."),
    [STRINGID_ENCMANAPHYBAND1]            = COMPOUND_STRING("Manaphy drifts a little closer\nto your Pokémon."),
    [STRINGID_ENCMANAPHYBAND2]            = COMPOUND_STRING("Manaphy will not take its eyes\noff your Pokémon."),
    [STRINGID_ENCMANAPHYBAND3]            = COMPOUND_STRING("Manaphy moves when your Pokémon\nmoves.\pThe water between them has gone\nstill."),
    [STRINGID_ENCMANAPHYBAND4]            = COMPOUND_STRING("Manaphy and your Pokémon are\nbreathing in time.\pIt is getting hard to tell where\none of them ends."),
    [STRINGID_ENCMANAPHYBAND5]            = COMPOUND_STRING("MANAPHY IS AS ONE WITH YOUR\nPOKÉMON.\pNothing you throw at it seems to\nreach it any more.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCMANAPHYBANDBROKEN]       = COMPOUND_STRING("Manaphy is thrashing at the\nwater where your Pokémon was!\pIt has stopped defending itself."),
    [STRINGID_ENCMANAPHYBANDNETWORK]      = COMPOUND_STRING("Manaphy stops holding on\nto your Pokémon and holds\pon to the sea instead."),
    [STRINGID_ENCMANAPHYBANDSHATTERED]    = COMPOUND_STRING("Every thread goes slack at once,\nand Manaphy comes apart."),
    [STRINGID_ENCMANAPHYSTANDINGBOND]     = COMPOUND_STRING("Manaphy has not looked away from\nyour Pokémon once."),
    [STRINGID_ENCMANAPHYSTANDINGBROKEN]   = COMPOUND_STRING("Manaphy is still searching the\nwater for something that left."),
    [STRINGID_ENCMANAPHYSTANDINGNETWORK]  = COMPOUND_STRING("The water is carrying everything\nboth ways now."),
    [STRINGID_ENCMANAPHYSEVER]            = COMPOUND_STRING("Manaphy reaches for a Pokémon\nthat is not there any more.\pTHE BOND BREAKS.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCMANAPHYECHOSOFT]         = COMPOUND_STRING("The broken bond washes over your\nPokémon."),
    [STRINGID_ENCMANAPHYECHOHARD]         = COMPOUND_STRING("The broken bond crashes over\nyour Pokémon!"),
    [STRINGID_ENCMANAPHYREBOND]           = COMPOUND_STRING("Manaphy finds your Pokémon in\nthe water, and settles."),
    [STRINGID_ENCMANAPHYSWAPGIVE]         = COMPOUND_STRING("Manaphy shared its strength with\nyour Pokémon!"),
    [STRINGID_ENCMANAPHYSWAPTAKE]         = COMPOUND_STRING("Manaphy took on its burden!"),
    [STRINGID_ENCMANAPHYPHASE1]           = COMPOUND_STRING("Manaphy sinks to the floor\nof the water, and the whole\psea leans in after it.\pHEART OF THE OCEAN.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCMANAPHYTIDALBOND]        = COMPOUND_STRING("TIDAL BOND.\pThe rain will not stop for as\nlong as Manaphy wants it.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCMANAPHYDRAWS]            = COMPOUND_STRING("Manaphy draws the rain into\nitself."),
    [STRINGID_ENCMANAPHYTIDEBROKEN]       = COMPOUND_STRING("The rain thins, and Manaphy\nfalters."),
    [STRINGID_ENCMANAPHYTIDERETURN]       = COMPOUND_STRING("Manaphy calls the rain back\ndown."),
    [STRINGID_ENCMANAPHYOCEANSHEART]      = COMPOUND_STRING("OCEAN'S HEART.\pManaphy opens every bond it has\never made, all at once.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCMANAPHYSHARE]            = COMPOUND_STRING("The water carries the blow back\nto your Pokémon!"),
    [STRINGID_ENCMANAPHYCOLLAPSE1]        = COMPOUND_STRING("Every bond Manaphy made comes\napart at once."),
    [STRINGID_ENCMANAPHYCOLLAPSE2]        = COMPOUND_STRING("There is nothing holding it up\nany more."),
    [STRINGID_ENCMANAPHYWEAKENED]         = COMPOUND_STRING("Manaphy's light has gone\nout of the water, and\pit is barely moving.\pNow - now is the moment to catch\nit!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCMANAPHYCATCHCLOSE]       = COMPOUND_STRING("It has not let go of you, even\nnow."),
    [STRINGID_ENCMANAPHYCATCHWATCHING]    = COMPOUND_STRING("It is watching you, and\ndeciding."),
    [STRINGID_ENCMANAPHYCATCHDISTANT]     = COMPOUND_STRING("It has been sent away too many\ntimes to come willingly."),

    // Darkrai, "The Pitch-Black Pokemon". Half of these lines are lies, and none of them say so.
    // Every hallucination is introduced by STRINGID_ENCDARKRAIWHISPER and nothing else in the
    // encounter prints it, so the ripple is the one fixed tell the player can learn to read.
    [STRINGID_ENCDARKRAIINTRO1]           = COMPOUND_STRING("Darkrai does not appear\nso much as the light stops\preaching where it is.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCDARKRAIINTRO2]           = COMPOUND_STRING("Everything after this happened\nwhile you were asleep."),
    [STRINGID_ENCDARKRAINM0]              = COMPOUND_STRING("The air clears. The field is\njust a field again."),
    [STRINGID_ENCDARKRAINM1]              = COMPOUND_STRING("Something feels wrong."),
    [STRINGID_ENCDARKRAINM2]              = COMPOUND_STRING("The colours on the field have\ngone slightly wrong."),
    [STRINGID_ENCDARKRAINM3]              = COMPOUND_STRING("You are no longer certain\nthe battle in front of you\pis the battle you are in."),
    [STRINGID_ENCDARKRAINM4]              = COMPOUND_STRING("Shapes move at the edge\nof the field that have\pno business being there."),
    [STRINGID_ENCDARKRAINM5]              = COMPOUND_STRING("THE DARK IS COMPLETE.\pThere is no field here any more\n- only the dream.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCDARKRAIPRESSES]          = COMPOUND_STRING("The dark presses in a little\ncloser."),
    [STRINGID_ENCDARKRAIWHISPER]          = COMPOUND_STRING("The dark ripples."),
    [STRINGID_ENCDARKRAICOLLAPSE]         = COMPOUND_STRING("Darkrai's strength is failing!"),
    [STRINGID_ENCDARKRAIREWIND]           = COMPOUND_STRING("Nothing was ever wrong with it."),
    [STRINGID_ENCDARKRAIFALSESTATUS]      = COMPOUND_STRING("{B_PLAYER_MON1_NAME} was badly poisoned!"),
    [STRINGID_ENCDARKRAIFALSEBOOST]       = COMPOUND_STRING("{B_PLAYER_MON1_NAME}'s Sp. Atk rose!"),
    [STRINGID_ENCDARKRAIFALSEVANISH]      = COMPOUND_STRING("Darkrai sank into its own\nshadow."),
    [STRINGID_ENCDARKRAICOPY]             = COMPOUND_STRING("A nightmare wearing {B_PLAYER_MON1_NAME}'s\nface rises out of the dark.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCDARKRAICOPYENDS]         = COMPOUND_STRING("The borrowed face slides off,\nand Darkrai is behind it."),
    [STRINGID_ENCDARKRAIWAKEGAIN]         = COMPOUND_STRING("{B_PLAYER_MON1_NAME}'s eyes clear for a\nmoment."),
    [STRINGID_ENCDARKRAIWAKESPEND]        = COMPOUND_STRING("{B_PLAYER_MON1_NAME} shakes the\ndream off, and some of\pthe dark goes with it."),
    [STRINGID_ENCDARKRAIREAD]             = COMPOUND_STRING("Darkrai has already watched you\ndo that."),
    [STRINGID_ENCDARKRAIFEEDS]            = COMPOUND_STRING("Darkrai takes the affliction the\nway it takes everything else.\pThe dark thickens."),
    [STRINGID_ENCDARKRAINOSLEEP]          = COMPOUND_STRING("Darkrai does not sleep.\pIt is what sleeps in you."),
    [STRINGID_ENCDARKRAITHEYFELL]         = COMPOUND_STRING("Something goes out of the\nfield, and the dark moves\pinto the space it left."),
    [STRINGID_ENCDARKRAIPHASE1]           = COMPOUND_STRING("Darkrai's dream stops being\nsomething you are watching.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCDARKRAIPHASE2]           = COMPOUND_STRING("ABSOLUTE NIGHTMARE.\pDarkrai stops hiding what\nit is and stands up out of\pthe dark wearing all of it.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCDARKRAIHEAVY]            = COMPOUND_STRING("It is heavier now.\pWhatever else this thing has\nbecome, it is slower."),
    [STRINGID_ENCDARKRAIWAKEUP]           = COMPOUND_STRING("You are almost sure none of this\nis happening.\pWAKE UP?"),
    [STRINGID_ENCDARKRAINOTYET]           = COMPOUND_STRING("You decide to see where the\ndream goes."),
    [STRINGID_ENCDARKRAISHATTER]          = COMPOUND_STRING("THE NIGHTMARE COMES APART.\pDarkrai is caught standing in a\nroom with the lights on.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCDARKRAIWINDOWCLOSES]     = COMPOUND_STRING("The dark closes over you again."),
    [STRINGID_ENCDARKRAIWEAKENED]         = COMPOUND_STRING("The dark thins, and what is left\nof Darkrai is a small thing that\phas been holding all of this up\non its own.\pNow - now is the moment to catch\nit!{PAUSE_UNTIL_PRESS}"),

    // Cresselia, "The Lunar Guardian". Every kindness in this fight - a quiet turn, a status move, a
    // real heal, a protected construct, a cured status - deepens the dream and makes Cresselia harder
    // to touch. Nothing ever says so; the fight only ever describes the moonlight.
    [STRINGID_ENCCRESSELIAINTRO1]         = COMPOUND_STRING("Cresselia does not wake\nso much as the moonlight\paround it thickens, and you\nbecome sleepy that has no\pbusiness feeling sleepy.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCCRESSELIAINTRO2]         = COMPOUND_STRING("It looks at you the way\nsomething looks at a guest it\phas already decided to keep."),
    [STRINGID_ENCCRESSELIADREAM0]         = COMPOUND_STRING("The air clears. The dream stops\nasking anything of you."),
    [STRINGID_ENCCRESSELIADREAM1]         = COMPOUND_STRING("Moonlight gathers over the\nfield, and everything feels\pa little more forgiving."),
    [STRINGID_ENCCRESSELIADREAM2]         = COMPOUND_STRING("LUCID.\pThe battlefield goes soft\nunderfoot, and nothing\phere means you harm.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCCRESSELIADREAM3]         = COMPOUND_STRING("The dream settles deeper.\nIt is getting harder to\premember why you came."),
    [STRINGID_ENCCRESSELIADREAM4]         = COMPOUND_STRING("REVERIE.\pThe moonlight is healing\nyou now, too - and you are\pstarting to understand why.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCCRESSELIADREAM5]         = COMPOUND_STRING("PERFECT DREAM.\pThere is nothing here any more\nthat wants to let you go.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCCRESSELIASTANDINGSHALLOW] = COMPOUND_STRING("The moonlight is still thin\nhere."),
    [STRINGID_ENCCRESSELIASTANDINGDEEP]   = COMPOUND_STRING("The dream is not finished with\nyou yet."),
    [STRINGID_ENCCRESSELIAOFFER]          = COMPOUND_STRING("Cresselia offers you something\ngentle, and asks nothing in\preturn but that you take it.\pACCEPT THE DREAM?"),
    [STRINGID_ENCCRESSELIARESIST]         = COMPOUND_STRING("You hold on to what is real."),
    [STRINGID_ENCCRESSELIABOONREST]       = COMPOUND_STRING("Something in the dream\nfolds itself around your\pPokémon, and it rests."),
    [STRINGID_ENCCRESSELIABOONCOURAGE]    = COMPOUND_STRING("The dream lends your Pokémon its\nown quiet certainty."),
    [STRINGID_ENCCRESSELIABOONSHELTER]    = COMPOUND_STRING("A soft light gathers in front of\nyour Pokémon and does not fade."),
    [STRINGID_ENCCRESSELIACONSTRUCT]      = COMPOUND_STRING("A second Cresselia peels away\nfrom the moonlight, and stands\pbetween you and the first.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCCRESSELIAREAD]           = COMPOUND_STRING("The dream has already learned to\nexpect that."),
    [STRINGID_ENCCRESSELIAREPRISAL]       = COMPOUND_STRING("The construct will not be struck\ntwice for nothing."),
    [STRINGID_ENCCRESSELIACLEANSE]        = COMPOUND_STRING("The dream will not let\nanything stay wrong\pwith Cresselia for long."),
    [STRINGID_ENCCRESSELIALURCHWARM]      = COMPOUND_STRING("The moonlight grows warm,\nunexpectedly, and lingers."),
    [STRINGID_ENCCRESSELIALURCHDISTORT]   = COMPOUND_STRING("The dream distorts, and\nfor a moment nothing\phere is being protected."),
    [STRINGID_ENCCRESSELIALURCHRESTORE]   = COMPOUND_STRING("Cresselia gathers the dream back\naround itself."),
    [STRINGID_ENCCRESSELIAPHASE1]         = COMPOUND_STRING("Cresselia's dream begins to\nfracture.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCCRESSELIAPHASE2]         = COMPOUND_STRING("Cresselia's eyes begin\nto glow. It desperately\pclings to its dream.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCCRESSELIAWAKES]          = COMPOUND_STRING("Your Pokémon awakens from the\ndream!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCCRESSELIAWINDOWCLOSES]   = COMPOUND_STRING("The dream closes back over it."),
    [STRINGID_ENCCRESSELIAWEAKENED]       = COMPOUND_STRING("The moonlight is thin, and what\nis left of Cresselia is barely\pholding the dream up at all.\pNow - now is the moment to catch\nit!{PAUSE_UNTIL_PRESS}"),

    // Giratina, "The Renegade Pokemon". The Distortion level does not touch the damage guard - it
    // controls how many of the battle's rules are currently inverted, and how fast they rotate.
    // Every inversion is a real field status the engine already animates; every rotation re-announces
    // whatever is live, so a player is never more than one rotation away from being told outright
    // what reality is doing.
    [STRINGID_ENCGIRATINAINTRO1]          = COMPOUND_STRING("Giratina is not standing\non the field. The field\pis standing on Giratina.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCGIRATINAINTRO2]          = COMPOUND_STRING("It watches you without\nmoving, and waits to\psee whether you notice."),
    [STRINGID_ENCGIRATINADIST0]           = COMPOUND_STRING("The field is holding its shape."),
    [STRINGID_ENCGIRATINADIST1]           = COMPOUND_STRING("REALITY BENDS. Something\nabout the ground is no\plonger agreeing with itself."),
    [STRINGID_ENCGIRATINADIST2]           = COMPOUND_STRING("UNSTABLE REALITY. The\nrules here have stopped\pstaying where you put them."),
    [STRINGID_ENCGIRATINADIST3]           = COMPOUND_STRING("THE DISTORTION WORLD.\nYou are not fighting\pin your world any more."),
    [STRINGID_ENCGIRATINADIST4]           = COMPOUND_STRING("REALITY COLLAPSE. Two\nof this world's laws\pare wrong at once now."),
    [STRINGID_ENCGIRATINADIST5]           = COMPOUND_STRING("DISTORTION WORLD MASTERY.\pGiratina has stopped bending the\nrules and started writing them.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCGIRATINAROTATE]          = COMPOUND_STRING("THE WORLD TURNS OVER."),
    [STRINGID_ENCGIRATINAROTATESOON]      = COMPOUND_STRING("Something in the air is about to\ngive."),
    [STRINGID_ENCGIRATINASHED]            = COMPOUND_STRING("The affliction slides off a\nGiratina that is no longer\pquite the one you gave it to."),
    [STRINGID_ENCGIRATINAINVSPEED]        = COMPOUND_STRING("DISTORTION: SPEED INVERTED.\pThe slower Pokémon will move\nfirst."),
    [STRINGID_ENCGIRATINAINVGRAVITY]      = COMPOUND_STRING("DISTORTION: GRAVITY COLLAPSED.\pEverything on the field is being\ndragged down."),
    [STRINGID_ENCGIRATINAINVDEFENCE]      = COMPOUND_STRING("DISTORTION: DEFENCES INVERTED.\pWhat was armoured is bare, and\nwhat was bare is armoured."),
    [STRINGID_ENCGIRATINAINVITEMS]        = COMPOUND_STRING("DISTORTION: POSSESSIONS\nINVERTED.\pNothing carried into this place\nstill works here."),
    [STRINGID_ENCGIRATINAINVHEALING]      = COMPOUND_STRING("DISTORTION: HEALING INVERTED.\pNothing you mend will stay\nmended."),
    [STRINGID_ENCGIRATINASTABREADY]       = COMPOUND_STRING("{B_PLAYER_MON1_NAME} is standing on\nsomething solid."),
    [STRINGID_ENCGIRATINASTABGONE]        = COMPOUND_STRING("{B_PLAYER_MON1_NAME} let go of the ground\nit was standing on."),
    [STRINGID_ENCGIRATINAANCHORASK]       = COMPOUND_STRING("You can feel where the real\nworld is from here.\pANCHOR REALITY?"),
    [STRINGID_ENCGIRATINAANCHORNO]        = COMPOUND_STRING("You decide to let the world stay\nas it is."),
    [STRINGID_ENCGIRATINAANCHORYES]       = COMPOUND_STRING("REALITY ANCHORED.\pFor a moment the field is\njust a field, and Giratina\pis just standing in it.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCGIRATINAANCHORCLOSE]     = COMPOUND_STRING("The distortion closes back over\nthe field."),
    [STRINGID_ENCGIRATINAGATE]            = COMPOUND_STRING("A tear opens in the air beside\nGiratina."),
    [STRINGID_ENCGIRATINAPHASE1]          = COMPOUND_STRING("Giratina steps out of its own\nshadow, and what comes out is\plonger, and has more of it.\pORIGIN FORME.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCGIRATINAPHASE1GROUND]    = COMPOUND_STRING("Whatever holds Giratina up now,\nit is not the ground."),
    [STRINGID_ENCGIRATINAFLED]            = COMPOUND_STRING("Giratina watches something\nleave, and the distortion\pwidens behind it."),
    [STRINGID_ENCGIRATINAFELL]            = COMPOUND_STRING("Something goes down, and the\nworld leans further over."),
    [STRINGID_ENCGIRATINAPHASE2]          = COMPOUND_STRING("REALITY BREAK.\pGiratina stops holding the world\ntogether on purpose.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCGIRATINABREAKSOON]       = COMPOUND_STRING("The field will not hold much\nlonger."),
    [STRINGID_ENCGIRATINABREAK]           = COMPOUND_STRING("THE WORLD COMES APART.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCGIRATINARETURN1]         = COMPOUND_STRING("The Distortion World is folding\nitself back up."),
    [STRINGID_ENCGIRATINARETURN2]         = COMPOUND_STRING("Giratina is pulled back down\ninto the shape it started in."),
    [STRINGID_ENCGIRATINAWEAKENED]        = COMPOUND_STRING("Giratina has nothing left\nholding the world open, and\pit can barely hold itself.\pNow - now is the moment to catch\nit!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCGIRATINACATCHHELD]       = COMPOUND_STRING("You are the only thing in\nthis place still standing\pwhere you put yourself."),
    [STRINGID_ENCGIRATINACATCHSLIPPED]    = COMPOUND_STRING("It has been thrown around as\nmuch as you have."),
    [STRINGID_ENCGIRATINACATCHLOST]       = COMPOUND_STRING("Neither of you is quite sure\nwhere the ground went."),

    [STRINGID_ENCDIALGAINTRO1]            = COMPOUND_STRING("Dialga is not waiting for you.\nIt is deciding when you arrived.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCDIALGAINTRO2]            = COMPOUND_STRING("The air ticks like something\nenormous keeping time."),
    [STRINGID_ENCDIALGAMARK]              = COMPOUND_STRING("Dialga has marked this moment in\ntime."),
    [STRINGID_ENCDIALGASHED]              = COMPOUND_STRING("Dialga returns to the moment\nbefore it was harmed!"),
    [STRINGID_ENCDIALGAQUOTA0]            = COMPOUND_STRING("It gives you four\nturns, and asks for a\pwound it will remember."),
    [STRINGID_ENCDIALGAQUOTA1]            = COMPOUND_STRING("Four turns. It expects less of\nyou than before."),
    [STRINGID_ENCDIALGAQUOTA2]            = COMPOUND_STRING("Three turns. The mark is set\ncloser now."),
    [STRINGID_ENCDIALGAQUOTA3]            = COMPOUND_STRING("Three turns, and it has already\ndecided how they end."),
    [STRINGID_ENCDIALGAQUOTA4]            = COMPOUND_STRING("Two turns. There is barely a\nmoment to fill."),
    [STRINGID_ENCDIALGAQUOTA5]            = COMPOUND_STRING("Two turns. It is no longer\nreally asking."),
    [STRINGID_ENCDIALGACOUNT3]            = COMPOUND_STRING("The moment is still far off."),
    [STRINGID_ENCDIALGACOUNT2]            = COMPOUND_STRING("Two turns remain before the\nmark."),
    [STRINGID_ENCDIALGACOUNT1]            = COMPOUND_STRING("One turn remains before the\nmark."),
    [STRINGID_ENCDIALGAHOLD]              = COMPOUND_STRING("THE TIMELINE HOLDS!\pDialga's grip on the moment\nslips."),
    [STRINGID_ENCDIALGASTAGGER]           = COMPOUND_STRING("Dialga is caught out of step\nwith itself!"),
    [STRINGID_ENCDIALGACOLLAPSE]          = COMPOUND_STRING("THE INTERVAL COLLAPSES!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCDIALGAUNDONE]            = COMPOUND_STRING("Those turns did not happen."),
    [STRINGID_ENCDIALGAECHO]              = COMPOUND_STRING("A temporal echo tears out of the\ncollapse!"),
    [STRINGID_ENCDIALGAECHOBLOCKED]       = COMPOUND_STRING("Most of it breaks against\nsomething {B_PLAYER_MON1_NAME}\pwas already holding."),
    [STRINGID_ENCDIALGACHARGE0]           = COMPOUND_STRING("Time is running the way it\nshould."),
    [STRINGID_ENCDIALGACHARGE1]           = COMPOUND_STRING("TEMPORAL CHARGE: TICKING.\pSomething has started counting."),
    [STRINGID_ENCDIALGACHARGE2]           = COMPOUND_STRING("TEMPORAL CHARGE: FLOWING.\pThe moments are running\ntogether."),
    [STRINGID_ENCDIALGACHARGE3]           = COMPOUND_STRING("TEMPORAL CHARGE: ACCELERATED.\pDialga is moving through a\nfaster hour than you are."),
    [STRINGID_ENCDIALGACHARGE4]           = COMPOUND_STRING("TEMPORAL CHARGE: UNMOORED.\pYour turns are arriving before\nyou have taken them."),
    [STRINGID_ENCDIALGACHARGE5]           = COMPOUND_STRING("TIME MASTERY.\pDialga owns every moment on this\nfield.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCDIALGAACCEL1]            = COMPOUND_STRING("Dialga accelerates the flow of\ntime!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCDIALGAACCEL2]            = COMPOUND_STRING("Everything you set down begins\nto wear out."),
    [STRINGID_ENCDIALGAAGE]               = COMPOUND_STRING("Time accelerates around\n{B_PLAYER_MON1_NAME}!"),
    [STRINGID_ENCDIALGAAGEWEARS]          = COMPOUND_STRING("What you built here has already\nworn away."),
    [STRINGID_ENCDIALGAAGEDEEP]           = COMPOUND_STRING("{B_PLAYER_MON1_NAME} is fighting through\nyears it has not lived."),
    [STRINGID_ENCDIALGAORIGIN1]           = COMPOUND_STRING("Dialga's temporal power reaches\nits peak!"),
    [STRINGID_ENCDIALGAORIGIN2]           = COMPOUND_STRING("ORIGIN FORME.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCDIALGAFRACTURE]          = COMPOUND_STRING("The future splits into countless\npossibilities..."),
    [STRINGID_ENCDIALGAFUTRECOIL]         = COMPOUND_STRING("In every future you can\nsee, you strike first -\pand something strikes back.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCDIALGAFUTLOCK]           = COMPOUND_STRING("In every future you can\nsee, you are running.\pDialga closes them all.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCDIALGAFUTCLOSING]        = COMPOUND_STRING("In every future you can\nsee, you are waiting. Dialga\pstops giving you the time.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCDIALGARECOIL]            = COMPOUND_STRING("The moment you struck comes back\naround!"),
    [STRINGID_ENCDIALGALOCK]              = COMPOUND_STRING("There is no longer a moment in\nwhich you leave."),
    [STRINGID_ENCDIALGACLOSING]           = COMPOUND_STRING("The hours you were counting on\nare already spent!"),
    [STRINGID_ENCDIALGACONV1]             = COMPOUND_STRING("The timelines begin collapsing\ninto one!"),
    [STRINGID_ENCDIALGACONV2]             = COMPOUND_STRING("Only this moment is left - and\nDialga is barely holding it.\pNow - now is the moment to catch\nit!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCPALKIAINTRO1]            = COMPOUND_STRING("Palkia does not step onto\nthe field. The field arranges\pitself around Palkia.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCPALKIAINTRO2]            = COMPOUND_STRING("It decides how far away\nit is standing, and\pthe distance obeys."),
    [STRINGID_ENCPALKIADIST0]             = COMPOUND_STRING("The space between you is holding\nstill."),
    [STRINGID_ENCPALKIADIST1]             = COMPOUND_STRING("SPACE RIPPLES. The\ndistance between you\pwill not sit straight."),
    [STRINGID_ENCPALKIADIST2]             = COMPOUND_STRING("SPACE WARPS. What reaches Palkia\nis no longer what you aimed."),
    [STRINGID_ENCPALKIADIST3]             = COMPOUND_STRING("SPACE FRACTURES. There\nare seams in the air where\pthere should be none."),
    [STRINGID_ENCPALKIADIST4]             = COMPOUND_STRING("SPACE COMES UNMOORED.\nNothing here is where\pit looks like it is."),
    [STRINGID_ENCPALKIADIST5]             = COMPOUND_STRING("SHATTERED SPACE.\pPalkia is no longer standing\nanywhere you can point at.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCPALKIAZONENEAR]          = COMPOUND_STRING("SPACE COMPRESSES. Palkia is\nsuddenly close enough to touch\pand there is nowhere to step\nback to."),
    [STRINGID_ENCPALKIAZONEFAR]           = COMPOUND_STRING("SPACE SEPARATES. Palkia pulls\nthe distance out long, and only\pwhat is sent across it arrives."),
    [STRINGID_ENCPALKIAZONEWARPED]        = COMPOUND_STRING("SPACE FOLDS. Palkia is\nstanding in a direction\pyou cannot aim at."),
    [STRINGID_ENCPALKIAREMINDNEAR]        = COMPOUND_STRING("The distance is still\ncrushed short. Only a\pbody crossing it lands."),
    [STRINGID_ENCPALKIAREMINDFAR]         = COMPOUND_STRING("The distance is still\ndrawn long. Only what is\psent across it arrives."),
    [STRINGID_ENCPALKIAREMINDWARPED]      = COMPOUND_STRING("The fold is still holding.\nNothing thrown at Palkia is\pgoing where you throw it."),
    [STRINGID_ENCPALKIALANDED]            = COMPOUND_STRING("The attack crosses the distance\ncleanly!"),
    [STRINGID_ENCPALKIARIFTSWALLOWS]      = COMPOUND_STRING("The blow slips into the torn\nspace and is gone..."),
    [STRINGID_ENCPALKIARIFTRETURNS]       = COMPOUND_STRING("...and comes back out behind\n{B_PLAYER_MON1_NAME}!"),
    [STRINGID_ENCPALKIAANCHOR0]           = COMPOUND_STRING("{B_PLAYER_MON1_NAME} has lost its footing\nin the distortion."),
    [STRINGID_ENCPALKIAANCHOR1]           = COMPOUND_STRING("{B_PLAYER_MON1_NAME} is beginning to feel\nwhere it really is."),
    [STRINGID_ENCPALKIAANCHOR2]           = COMPOUND_STRING("{B_PLAYER_MON1_NAME} is holding a fixed\npoint in the warped space."),
    [STRINGID_ENCPALKIAANCHOR3]           = COMPOUND_STRING("{B_PLAYER_MON1_NAME} is anchored. The\ndistortion cannot move it now."),
    [STRINGID_ENCPALKIAANCHORASK]         = COMPOUND_STRING("{B_PLAYER_MON1_NAME} can feel\nthe rift pulling on the\ppoint it is holding.\pDRIVE THE ANCHOR IN?"),
    [STRINGID_ENCPALKIAANCHORHELD]        = COMPOUND_STRING("{B_PLAYER_MON1_NAME} keeps its footing and\nwaits."),
    [STRINGID_ENCPALKIAANCHORDRIVE]       = COMPOUND_STRING("{B_PLAYER_MON1_NAME} drives itself into a\nfixed point in space!"),
    [STRINGID_ENCPALKIARIFTCOLLAPSES]     = COMPOUND_STRING("The rift folds shut on\nnothing, and Palkia is dragged\pback to where it stands!"),
    [STRINGID_ENCPALKIAWORMOPENS]         = COMPOUND_STRING("A wormhole opens beside Palkia."),
    [STRINGID_ENCPALKIAWORMWAITS]         = COMPOUND_STRING("The wormhole is waiting for\nsomething to be thrown into it."),
    [STRINGID_ENCPALKIAWORMPUNISH]        = COMPOUND_STRING("The attack goes into\nthe wormhole - and the\pwormhole gives it back!"),
    [STRINGID_ENCPALKIAWORMEMPTY]         = COMPOUND_STRING("The wormhole closes on\nnothing. Palkia has to\pcome back to where it was."),
    [STRINGID_ENCPALKIATEARSSPACE]        = COMPOUND_STRING("Palkia tears space open beneath\n{B_PLAYER_MON1_NAME}!"),
    [STRINGID_ENCPALKIADISPLACED]         = COMPOUND_STRING("{B_PLAYER_MON1_NAME} is pulled somewhere\nelse entirely!"),
    [STRINGID_ENCPALKIADISPLACEDAGAIN]    = COMPOUND_STRING("The field folds again, and there\nis nothing to brace against!"),
    [STRINGID_ENCPALKIANOWHERETOSEND]     = COMPOUND_STRING("...but there was nowhere\nleft to send it, and the tear\pcloses on your Pokemon instead!"),
    [STRINGID_ENCPALKIAANCHORHOLDS]       = COMPOUND_STRING("The space beneath\n{B_PLAYER_MON1_NAME} tears open\p- and it does not move."),
    [STRINGID_ENCPALKIACOLLAPSE1]         = COMPOUND_STRING("Palkia begins tearing space\napart in earnest!"),
    [STRINGID_ENCPALKIACOLLAPSE2]         = COMPOUND_STRING("The distance itself has stopped\nbehaving.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCPALKIAORIGIN1]           = COMPOUND_STRING("Palkia's hold on space reaches\nits limit!"),
    [STRINGID_ENCPALKIAORIGIN2]           = COMPOUND_STRING("ORIGIN FORME.\pPalkia has stopped standing in\nthe field at all.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCPALKIAREND1]             = COMPOUND_STRING("A rift tears open behind Palkia."),
    [STRINGID_ENCPALKIAREND2]             = COMPOUND_STRING("The rift widens. The air is\nbeing pulled into it."),
    [STRINGID_ENCPALKIAREND3]             = COMPOUND_STRING("Space is collapsing inward, and\nPalkia is at the centre of it!"),
    [STRINGID_ENCPALKIARENDFIRE]          = COMPOUND_STRING("SPATIAL REND! The fold snaps\nshut across the whole field!"),
    [STRINGID_ENCPALKIARENDSPENT]         = COMPOUND_STRING("Palkia is spent from tearing the\nfield apart."),
    [STRINGID_ENCPALKIASHED]              = COMPOUND_STRING("The affliction slides off\na Palkia that is no longer\pquite where you left it."),
    [STRINGID_ENCPALKIANOWHERETOHIDE]     = COMPOUND_STRING("Palkia folds space around the\nentire battlefield!"),
    [STRINGID_ENCPALKIACOLLAPSEALL]       = COMPOUND_STRING("...and then every fold in it\ncollapses at once.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCPALKIAEXHAUSTED]         = COMPOUND_STRING("Palkia is exhausted. Its edges\nare unsteady.\pNow - now is the moment to catch\nit!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCCOBALIONINTRO1]          = COMPOUND_STRING("Cobalion meets your eyes and\nwaits for you to move first."),
    [STRINGID_ENCCOBALIONINTRO2]          = COMPOUND_STRING("It has fought before.\pIt is already reading\nhow you fight now."),
    [STRINGID_ENCCOBALIONGUARD]           = COMPOUND_STRING("GUARD STANCE. Cobalion sets\nitself against the blow\pit has just seen."),
    [STRINGID_ENCCOBALIONASSAULT]         = COMPOUND_STRING("ASSAULT STANCE. Cobalion\ndrops its guard and comes\pforward - you gave it room."),
    [STRINGID_ENCCOBALIONCOMMAND]         = COMPOUND_STRING("COMMAND STANCE. Cobalion\nstops fighting, and starts\pwatching you instead."),
    [STRINGID_ENCCOBALIONSHEDS]           = COMPOUND_STRING("Cobalion shrugs off its\naffliction without breaking\pstance."),
    [STRINGID_ENCCOBALIONDISC0]           = COMPOUND_STRING("Cobalion's eyes lose your\nrhythm."),
    [STRINGID_ENCCOBALIONDISC1]           = COMPOUND_STRING("Cobalion is beginning to\nfollow your movements."),
    [STRINGID_ENCCOBALIONDISC2]           = COMPOUND_STRING("Cobalion has taken your\nmeasure."),
    [STRINGID_ENCCOBALIONDISC3]           = COMPOUND_STRING("Cobalion answers your attack\nbefore you have finished it."),
    [STRINGID_ENCCOBALIONDISC4]           = COMPOUND_STRING("Cobalion is standing a step\nahead of you now."),
    [STRINGID_ENCCOBALIONDISC5]           = COMPOUND_STRING("Cobalion knows what you\nare about to do.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCCOBALIONDISCREMIND]      = COMPOUND_STRING("Cobalion is still watching\nthe shape of your attacks."),
    [STRINGID_ENCCOBALIONCOUNTERSTRIKE]   = COMPOUND_STRING("Cobalion turns your own\nattack straight back at you!"),
    [STRINGID_ENCCOBALIONCONTEMPT]        = COMPOUND_STRING("Cobalion will not wait for a\nfight you are not having!"),
    [STRINGID_ENCCOBALIONDECLAREPHYSICAL] = COMPOUND_STRING("Cobalion sets its stance\nagainst a head-on blow."),
    [STRINGID_ENCCOBALIONDECLARESPECIAL]  = COMPOUND_STRING("Cobalion braces for something\nsent from a distance."),
    [STRINGID_ENCCOBALIONDECLARESTATUS]   = COMPOUND_STRING("Cobalion watches your\nPokemon's footing, not\pits attack."),
    [STRINGID_ENCCOBALIONREADLANDS]       = COMPOUND_STRING("Cobalion was already waiting\nexactly there!"),
    [STRINGID_ENCCOBALIONREADMISPLACED]   = COMPOUND_STRING("Cobalion committed to the\nwrong answer, and its guard\phangs open!"),
    [STRINGID_ENCCOBALIONTHEREAD]         = COMPOUND_STRING("Cobalion has learned your\nbattle rhythm."),
    [STRINGID_ENCCOBALIONLOCKED]          = COMPOUND_STRING("It will not let {B_BUFF1}\nthrough again!"),
    [STRINGID_ENCCOBALIONREPOSITION]      = COMPOUND_STRING("Cobalion moves to drive\n{B_PLAYER_MON1_NAME} out of position!"),
    [STRINGID_ENCCOBALIONSEESTHROUGH]     = COMPOUND_STRING("Cobalion's eyes follow\n{B_PLAYER_MON1_NAME} straight\pthrough the feints!"),
    [STRINGID_ENCCOBALIONLEADER1]         = COMPOUND_STRING("Cobalion's resolve becomes\nunshakable!"),
    [STRINGID_ENCCOBALIONLEADER2]         = COMPOUND_STRING("It has stopped answering your\nlast move.\pIt is answering all of\nthem.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCCOBALIONRESOLVE1]        = COMPOUND_STRING("Cobalion refuses to fall!"),
    [STRINGID_ENCCOBALIONRESOLVE2]        = COMPOUND_STRING("It will not hold one stance\nlong enough for you to\puse it.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCCOBALIONSTANDSFIRM]      = COMPOUND_STRING("Cobalion stands firm!"),
    [STRINGID_ENCCOBALIONFINALASSAULT]    = COMPOUND_STRING("Cobalion gathers everything\nit has left..."),
    [STRINGID_ENCCOBALIONWEAKENED]        = COMPOUND_STRING("Cobalion is spent. Its stance\nhas finally broken.\pNow - now is the moment to\ncatch it!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCTERRAKIONINTRO1]         = COMPOUND_STRING("Terrakion plants its hooves.\nThe ground has not stopped\pshaking since before you\narrived."),
    [STRINGID_ENCTERRAKIONINTRO2]         = COMPOUND_STRING("Every blow it lands only makes\nthe next one harder to stop.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCTERRAKIONIMPACT0]        = COMPOUND_STRING("The tremor under Terrakion\nfades. It has to set itself\pagain."),
    [STRINGID_ENCTERRAKIONIMPACT1]        = COMPOUND_STRING("The ground around Terrakion\nbegins to shudder."),
    [STRINGID_ENCTERRAKIONIMPACT2]        = COMPOUND_STRING("Terrakion's footing sends\ncracks racing outward."),
    [STRINGID_ENCTERRAKIONIMPACT3]        = COMPOUND_STRING("The whole field pitches with\nTerrakion's weight now."),
    [STRINGID_ENCTERRAKIONIMPACT4]        = COMPOUND_STRING("Terrakion is coming through\nyour defenses, not around\pthem."),
    [STRINGID_ENCTERRAKIONIMPACT5]        = COMPOUND_STRING("Terrakion hits like the\nmountain coming down."),
    [STRINGID_ENCTERRAKIONIMPACT6]        = COMPOUND_STRING("Terrakion can barely hold\nthe force it is carrying.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCTERRAKIONIMPACT7]        = COMPOUND_STRING("Terrakion's strength is about\nto tear loose!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCTERRAKIONIMPACTREMIND]   = COMPOUND_STRING("The rock under Terrakion is\nstill ringing from the last\pblow."),
    [STRINGID_ENCTERRAKIONSHOCKWAVE]      = COMPOUND_STRING("The force rolls off Terrakion\nand across the whole field!"),
    [STRINGID_ENCTERRAKIONGROUNDSPLITS]   = COMPOUND_STRING("The ground splits open, and\nthe traps set in it are gone!"),
    [STRINGID_ENCTERRAKIONAIRCRACKS]      = COMPOUND_STRING("The air itself cracks, and the\nscreens break apart with it!"),
    [STRINGID_ENCTERRAKIONFIELDTEARS]     = COMPOUND_STRING("The field tears, and what was\nlaid over it is thrown clear!"),
    [STRINGID_ENCTERRAKIONTHROUGH]        = COMPOUND_STRING("Terrakion's force drives\nstraight through the guard!"),
    [STRINGID_ENCTERRAKIONSTRIPS]         = COMPOUND_STRING("The shock wrenches\n{B_PLAYER_MON1_NAME} out of every\pstance it had built!"),
    [STRINGID_ENCTERRAKIONSHATTERS]       = COMPOUND_STRING("Terrakion's force shatters\nthe barriers around\p{B_PLAYER_MON1_NAME}!"),
    [STRINGID_ENCTERRAKIONSEESTHROUGH]    = COMPOUND_STRING("Terrakion's eyes lock onto\n{B_PLAYER_MON1_NAME}, straight\pthrough the feints!"),
    [STRINGID_ENCTERRAKIONENRAGED1]       = COMPOUND_STRING("Terrakion stops holding\nanything back!"),
    [STRINGID_ENCTERRAKIONABANDONS]       = COMPOUND_STRING("It has quit defending itself.\nIt only means to hit you now."),
    [STRINGID_ENCTERRAKIONUNSTABLE]       = COMPOUND_STRING("And nothing is capping how far\nits force can build anymore.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCTERRAKIONREFUSES]        = COMPOUND_STRING("Terrakion refuses to yield!"),
    [STRINGID_ENCTERRAKIONUNSTOPPABLE]    = COMPOUND_STRING("It will not be slowed and it\nwill not be worn down. Only\poutlasted.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCTERRAKIONOVERWHELMS]     = COMPOUND_STRING("Terrakion's own strength\noverwhelms it!"),
    [STRINGID_ENCTERRAKIONCHARGE]         = COMPOUND_STRING("EARTH-SPLITTING CHARGE!"),
    [STRINGID_ENCTERRAKIONSTAGGERS]       = COMPOUND_STRING("The charge took everything\nTerrakion had. It staggers!"),
    [STRINGID_ENCTERRAKIONEXPOSED]        = COMPOUND_STRING("Terrakion is wide open.\nNow - hit it now!"),
    [STRINGID_ENCTERRAKIONGIVESOUT]       = COMPOUND_STRING("Terrakion's strength finally\ngives out."),
    [STRINGID_ENCTERRAKIONSPENT]          = COMPOUND_STRING("It has nothing left to draw\non."),
    [STRINGID_ENCTERRAKIONWEAKENED]       = COMPOUND_STRING("Terrakion is spent, and barely\nstanding.\pNow - now is the moment to\ncatch it!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCVIRIZIONINTRO1]          = COMPOUND_STRING("Virizion moves as though the\nfight is already flowing, and\pyou have only just stepped in."),
    [STRINGID_ENCVIRIZIONINTRO2]          = COMPOUND_STRING("Every step it takes lands where\nyour last one did not.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCVIRIZIONMOMENTUM0]       = COMPOUND_STRING("Virizion's flow is gone. It\nhas to find its footing again."),
    [STRINGID_ENCVIRIZIONMOMENTUM1]       = COMPOUND_STRING("Virizion begins to settle into\na rhythm."),
    [STRINGID_ENCVIRIZIONMOMENTUM2]       = COMPOUND_STRING("Virizion's footwork is\nflowing more freely now."),
    [STRINGID_ENCVIRIZIONMOMENTUM3]       = COMPOUND_STRING("Virizion moves with real speed.\nYour blows keep catching air."),
    [STRINGID_ENCVIRIZIONMOMENTUM4]       = COMPOUND_STRING("Virizion flows from one strike\nto the next without a pause."),
    [STRINGID_ENCVIRIZIONMOMENTUM5]       = COMPOUND_STRING("You can barely keep Virizion\nin your sight."),
    [STRINGID_ENCVIRIZIONMOMENTUM6]       = COMPOUND_STRING("Virizion is a blur. Nothing\nyou throw is landing clean.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCVIRIZIONMOMENTUM7]       = COMPOUND_STRING("Virizion's rhythm is perfect.\nIt cannot be touched.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCVIRIZIONMOMENTUMREMIND]  = COMPOUND_STRING("Virizion's flow still has not\nbroken. Your hits glance off."),
    [STRINGID_ENCVIRIZIONBLADEDANCE]      = COMPOUND_STRING("BLADE DANCE! Virizion's speed\nfolds one strike into two."),
    [STRINGID_ENCVIRIZIONUNTOUCHABLE]     = COMPOUND_STRING("Virizion has slipped past what\nyour eyes can follow."),
    [STRINGID_ENCVIRIZIONBROKEN]          = COMPOUND_STRING("Virizion's momentum has been\nbroken!"),
    [STRINGID_ENCVIRIZIONEXHAUSTED]       = COMPOUND_STRING("Virizion's charge collapses,\nand it is left wide open!"),
    [STRINGID_ENCVIRIZIONBREAKTYPE]       = COMPOUND_STRING("That struck something Virizion\ncould not flow around!"),
    [STRINGID_ENCVIRIZIONBREAKSLOWED]     = COMPOUND_STRING("Virizion cannot find its\nrhythm at this pace!"),
    [STRINGID_ENCVIRIZIONBREAKSTATUSED]   = COMPOUND_STRING("Virizion's step falters - it\ncannot hold the flow!"),
    [STRINGID_ENCVIRIZIONDANCE]           = COMPOUND_STRING("Virizion darts back in for a\nsecond cut!"),
    [STRINGID_ENCVIRIZIONBLUR]            = COMPOUND_STRING("Virizion blurs - it is set to\nslip the next blow!"),
    [STRINGID_ENCVIRIZIONFASTER]          = COMPOUND_STRING("Virizion becomes almost\nimpossible to follow!"),
    [STRINGID_ENCVIRIZIONCEILING]         = COMPOUND_STRING("Nothing caps its flow now. It\ncan build further than before.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCVIRIZIONSHAKESOFF]       = COMPOUND_STRING("And it shrugs off whatever\nslows it almost at once.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCVIRIZIONSHEDS]           = COMPOUND_STRING("Virizion shakes off the\naffliction and flows on."),
    [STRINGID_ENCVIRIZIONFOOTING]         = COMPOUND_STRING("Virizion recovers its footing\na step at a time."),
    [STRINGID_ENCVIRIZIONGATHERS]         = COMPOUND_STRING("Virizion gathers itself for\none final effort."),
    [STRINGID_ENCVIRIZIONCHARGING]        = COMPOUND_STRING("Its rhythm is climbing on its\nown. Denying it a hit will\pnot stop that now.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCVIRIZIONSACREDBLADE]     = COMPOUND_STRING("SACRED BLADE!"),
    [STRINGID_ENCVIRIZIONAGAIN]           = COMPOUND_STRING("Virizion falls back, and\nbegins to gather itself again."),
    [STRINGID_ENCVIRIZIONAGAIN2]          = COMPOUND_STRING("Again Virizion draws itself up.\nIt will not stop coming."),
    [STRINGID_ENCVIRIZIONFALLS]           = COMPOUND_STRING("Virizion's blade finally\nfalls."),
    [STRINGID_ENCVIRIZIONSTILL]           = COMPOUND_STRING("Virizion goes still. It has\nnothing left to draw on."),
    [STRINGID_ENCVIRIZIONWEAKENED]        = COMPOUND_STRING("Virizion is spent, and barely\nstanding.\pNow - now is the moment to\ncatch it!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCGENESECTINTRO1]         = COMPOUND_STRING("TARGET ACQUIRED.\pGenesect's systems come\nonline one by one."),
    [STRINGID_ENCGENESECTINTRO2]         = COMPOUND_STRING("It is already reading\neverything you do.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCGENESECTFILED]          = COMPOUND_STRING("ANALYSIS COMPLETE.\pGenesect reconfigures its\nplating against {B_BUFF1}!"),
    [STRINGID_ENCGENESECTHARDENED]       = COMPOUND_STRING("Genesect has already filed\n{B_BUFF1}.\pIts plating hardens further."),
    [STRINGID_ENCGENESECTEVICTED]        = COMPOUND_STRING("Genesect discards its\n{B_BUFF2} plating to make\proom for {B_BUFF1}!"),
    [STRINGID_ENCGENESECTDOUSE]          = COMPOUND_STRING("DOUSE DRIVE. A downpour\nrolls in around it."),
    [STRINGID_ENCGENESECTCHILL]          = COMPOUND_STRING("CHILL DRIVE. The cold bites -\nyour Pokemon slows down!"),
    [STRINGID_ENCGENESECTSHOCK]          = COMPOUND_STRING("SHOCK DRIVE. Current crawls\nacross the battlefield."),
    [STRINGID_ENCGENESECTBURN]           = COMPOUND_STRING("BURN DRIVE. Genesect's vents\nglow white-hot."),
    [STRINGID_ENCGENESECTDUALDRIVE]      = COMPOUND_STRING("DUAL DRIVE! Genesect is\nrunning two loadouts at once!"),
    [STRINGID_ENCGENESECTSTRAIN1]        = COMPOUND_STRING("Something in Genesect's\nframe is protesting."),
    [STRINGID_ENCGENESECTSTRAIN2]        = COMPOUND_STRING("Genesect's systems are\nstruggling to keep up!"),
    [STRINGID_ENCGENESECTSTRAIN3]        = COMPOUND_STRING("SYSTEM STRAIN CRITICAL!"),
    [STRINGID_ENCGENESECTSTRAINEASE]     = COMPOUND_STRING("Genesect's systems settle\nback down."),
    [STRINGID_ENCGENESECTOVERLOAD]       = COMPOUND_STRING("SYSTEM OVERLOAD!"),
    [STRINGID_ENCGENESECTEXPOSED]        = COMPOUND_STRING("Its Drive is ejected -\nGenesect's core is exposed!"),
    [STRINGID_ENCGENESECTEXPOSEDHOLD]    = COMPOUND_STRING("Genesect's plating is still\nhanging open."),
    [STRINGID_ENCGENESECTONLINE]         = COMPOUND_STRING("Genesect's systems come back\nonline."),
    [STRINGID_ENCGENESECTCHARGE]         = COMPOUND_STRING("Genesect's cannon begins to\nglow."),
    [STRINGID_ENCGENESECTCHARGEHEAVY]    = COMPOUND_STRING("That cost you dearly - and\nthe cannon drank it in!"),
    [STRINGID_ENCGENESECTCHARGEFULL]     = COMPOUND_STRING("The cannon is fully charged!"),
    [STRINGID_ENCGENESECTCANNONREMIND]   = COMPOUND_STRING("Genesect's cannon is still\nglowing."),
    [STRINGID_ENCGENESECTCANNONFIRES]    = COMPOUND_STRING("TECHNO BLAST!"),
    [STRINGID_ENCGENESECTLOCKEDSHOT]     = COMPOUND_STRING("The shot finds its locked\ntarget dead on!"),
    [STRINGID_ENCGENESECTVENTED]         = COMPOUND_STRING("Genesect vents its {B_BUFF1}\nplating to shed the heat!"),
    [STRINGID_ENCGENESECTOVERHEATS]      = COMPOUND_STRING("The weapon has overheated!\pGenesect cannot fire again\nyet."),
    [STRINGID_ENCGENESECTSCANNING]       = COMPOUND_STRING("Nothing reached Genesect to\nread.\pIt configures for attack\ninstead!"),
    [STRINGID_ENCGENESECTFLAMECHIP]      = COMPOUND_STRING("Genesect sprays burning\nvapour across the field!"),
    [STRINGID_ENCGENESECTTARGETLOCKED]   = COMPOUND_STRING("TARGET LOCKED.\pGenesect will not let your\nPokemon leave the field!"),
    [STRINGID_ENCGENESECTLOCKHOLD]       = COMPOUND_STRING("The lock is still holding\nyour Pokemon in place."),
    [STRINGID_ENCGENESECTLOCKEXPIRES]    = COMPOUND_STRING("The target lock releases."),
    [STRINGID_ENCGENESECTLOCKBROKEN]     = COMPOUND_STRING("Genesect re-targets, and the\nlock breaks!"),
    [STRINGID_ENCGENESECTPURGE]          = COMPOUND_STRING("SYSTEM PURGE.\pGenesect vents the\ninterference."),
    [STRINGID_ENCGENESECTPLATINGHUM]     = COMPOUND_STRING("Genesect's plating hums\nagainst your attacks."),
    [STRINGID_ENCGENESECTOVERCLOCK]      = COMPOUND_STRING("WARNING - WEAPON OVERLOAD.\pThe cannon is building past\nits own limits!"),
    [STRINGID_ENCGENESECTOVERCLOCK3]     = COMPOUND_STRING("The overload is still\nclimbing."),
    [STRINGID_ENCGENESECTOVERCLOCK2]     = COMPOUND_STRING("Genesect's whole frame is\nshaking now!"),
    [STRINGID_ENCGENESECTOVERCLOCK1]     = COMPOUND_STRING("The cannon is about to let\ngo!"),
    [STRINGID_ENCGENESECTOVERCLOCKFIRE]  = COMPOUND_STRING("TECHNO BLAST: OVERCLOCK!"),
    [STRINGID_ENCGENESECTOVERCLOCKBREAK] = COMPOUND_STRING("Genesect's weapon system has\ncatastrophically overheated!"),
    [STRINGID_ENCGENESECTPROTOCOL]       = COMPOUND_STRING("Genesect activates its\nadvanced combat protocol!"),
    [STRINGID_ENCGENESECTPROTOCOL2]      = COMPOUND_STRING("It reads harder, answers\nfaster, and will not carry\pwhat you put on it.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCGENESECTWEAPONIZED]     = COMPOUND_STRING("Genesect's systems are\noperating beyond their\pintended parameters!"),
    [STRINGID_ENCGENESECTWEAPONIZED2]    = COMPOUND_STRING("It keeps every loadout it\nequips now.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCGENESECTFALLS]          = COMPOUND_STRING("Genesect fires one last\nshot."),
    [STRINGID_ENCGENESECTSTILL]          = COMPOUND_STRING("Genesect's frame goes still.\nIts systems are failing."),
    [STRINGID_ENCGENESECTWEAKENED]       = COMPOUND_STRING("Genesect is barely holding\ntogether.\pNow - now is the moment to\ncatch it!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCKELDEOINTRO1]           = COMPOUND_STRING("Keldeo squares up to you the\nway it would to a mentor -\phead high, guard honest."),
    [STRINGID_ENCKELDEOINTRO2]           = COMPOUND_STRING("It did not come here to win.\nIt came here to be tested.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCKELDEORESOLVE0]         = COMPOUND_STRING("Keldeo's resolve falters. It\nis back to first principles."),
    [STRINGID_ENCKELDEORESOLVE1]         = COMPOUND_STRING("Keldeo finds its feet. Its\nsteps come quicker now."),
    [STRINGID_ENCKELDEORESOLVE2]         = COMPOUND_STRING("Keldeo's blade work sharpens.\nIt is finding the openings."),
    [STRINGID_ENCKELDEORESOLVE3]         = COMPOUND_STRING("Keldeo fights like an adept\nnow. Strike it off its guard\pand it strikes straight back.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCKELDEORESOLVE4]         = COMPOUND_STRING("Keldeo moves like a master.\nEvery duel it opens now will\pdisarm you first.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCKELDEORESOLVE5]         = COMPOUND_STRING("Keldeo's resolve is complete.\nIt has nothing left to learn\pfrom you.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCKELDEOCHALLENGE]        = COMPOUND_STRING("Keldeo locks eyes with\n{B_PLAYER_MON1_NAME} - it wants\pa clean duel, one on one!"),
    [STRINGID_ENCKELDEODUELGUARD]        = COMPOUND_STRING("Keldeo abandons its guard to\nfight you head-on!"),
    [STRINGID_ENCKELDEODUELREMIND]       = COMPOUND_STRING("Keldeo is still locked in the\nduel, its guard wide open."),
    [STRINGID_ENCKELDEODUELEND]          = COMPOUND_STRING("The duel breaks off, and\nKeldeo raises its guard again."),
    [STRINGID_ENCKELDEODISARM]           = COMPOUND_STRING("Keldeo's blade sweeps first -\n{B_PLAYER_MON1_NAME} is disarmed!"),
    [STRINGID_ENCKELDEOTEMPERED]         = COMPOUND_STRING("That blow should have told.\nKeldeo takes it and stands -\pit is only tempered by it.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCKELDEODUELWON]          = COMPOUND_STRING("Keldeo won the duel on its\nown terms. Its resolve\phardens!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCKELDEODUELSURVIVED]     = COMPOUND_STRING("Time ran out with your\nPokemon still standing.\pKeldeo's resolve falters!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCKELDEORIPOSTE]          = COMPOUND_STRING("You struck a swordsman on\nguard - Keldeo turns the\pblade straight back!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCKELDEORESOLUTE1]        = COMPOUND_STRING("Keldeo draws itself up to its\nfull height. This is the form\pit fought to earn."),
    [STRINGID_ENCKELDEORESOLUTE2]        = COMPOUND_STRING("It will not duel again. From\nhere it means to end it with\pone stroke.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCKELDEOBLADERAISED]      = COMPOUND_STRING("Keldeo raises its blade for\nSACRED SWORD!"),
    [STRINGID_ENCKELDEOSWORDBROKEN]      = COMPOUND_STRING("You broke the stroke before\nit fell! Keldeo reels back!"),
    [STRINGID_ENCKELDEOSWORDOPEN]        = COMPOUND_STRING("Keldeo is wide open - hit it\nnow!"),
    [STRINGID_ENCKELDEOSACREDSWORD]      = COMPOUND_STRING("SACRED SWORD!"),
    [STRINGID_ENCKELDEOAGAIN1]           = COMPOUND_STRING("Keldeo sets its feet and\nraises the blade again."),
    [STRINGID_ENCKELDEOAGAIN2]           = COMPOUND_STRING("The strokes are coming harder\neach time. Keldeo will not\pstop.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCKELDEOTEACHINGS1]       = COMPOUND_STRING("Keldeo remembers the three\nwho trained it..."),
    [STRINGID_ENCKELDEOTEACHINGS2]       = COMPOUND_STRING("Cobalion's guard. Terrakion's\nweight. Virizion's speed. It\pfights with all of them now.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCKELDEOSHEDCUE]          = COMPOUND_STRING("Keldeo will not stay wounded\nnow - it shrugs the affliction\poff and fights on.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCKELDEOSHEDS]            = COMPOUND_STRING("Keldeo throws off the\naffliction and steadies\pitself."),
    [STRINGID_ENCKELDEOTRUERESOLVE1]     = COMPOUND_STRING("Keldeo stops holding back.\nThere is nothing left to save\pit for."),
    [STRINGID_ENCKELDEOTRUERESOLVE2]     = COMPOUND_STRING("It will not fall until it has\nlanded the stroke it came\pfor.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCKELDEOFALLS]            = COMPOUND_STRING("Keldeo's legs give out\nbeneath it..."),
    [STRINGID_ENCKELDEOSTILLSTANDING]    = COMPOUND_STRING("...but it drags itself up for\none last strike."),
    [STRINGID_ENCKELDEOWEAKENED]         = COMPOUND_STRING("Keldeo is spent, and barely\nstanding.\pNow - now is the moment to\ncatch it!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCKYUREMINTRO1]       = COMPOUND_STRING("Kyurem stands where the air\nitself has given up."),
    [STRINGID_ENCKYUREMINTRO2]       = COMPOUND_STRING("Its body is hollow. Something\nwas torn out of it, and never\preplaced."),
    [STRINGID_ENCKYUREMINTRO3]       = COMPOUND_STRING("The cold here does not blow.\nIt simply takes.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCKYUREMBAND0]        = COMPOUND_STRING("The air around Kyurem thins.\nIt is holding less of the\pbattle inside itself."),
    [STRINGID_ENCKYUREMBAND1]        = COMPOUND_STRING("Kyurem draws the battle in.\nIts frost sets harder."),
    [STRINGID_ENCKYUREMBAND2]        = COMPOUND_STRING("The cold runs deep now. Blows\nreach Kyurem and find nothing\pto land on."),
    [STRINGID_ENCKYUREMBAND3]        = COMPOUND_STRING("Everything the battle spent\nis inside Kyurem now.\pAlmost nothing you do reaches\nit.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCKYUREMDRAIN]        = COMPOUND_STRING("Kyurem swallows what was set\nup on the field."),
    [STRINGID_ENCKYUREMDRAINEMPTY]   = COMPOUND_STRING("Kyurem reaches for the field\nand finds nothing left to\ptake."),
    [STRINGID_ENCKYUREMFROZE1]       = COMPOUND_STRING("The air around your Pokemon\nsets solid."),
    [STRINGID_ENCKYUREMFROZE2]       = COMPOUND_STRING("{B_BUFF1} freezes in your\nPokemon's mind!"),
    [STRINGID_ENCKYUREMFROZE3]       = COMPOUND_STRING("The sky and the ground stop\nanswering."),
    [STRINGID_ENCKYUREMFROZE4]       = COMPOUND_STRING("Your Pokemon's momentum\nfreezes over."),
    [STRINGID_ENCKYUREMHOLD1]        = COMPOUND_STRING("The frozen air still holds\nyour Pokemon fast."),
    [STRINGID_ENCKYUREMHOLD2]        = COMPOUND_STRING("That move is still locked\nunder the ice."),
    [STRINGID_ENCKYUREMHOLD3]        = COMPOUND_STRING("The sky and the ground are\nstill frozen out."),
    [STRINGID_ENCKYUREMHOLD4]        = COMPOUND_STRING("The ice still smothers every\nadvantage you take."),
    [STRINGID_ENCKYUREMTHAWLAW]      = COMPOUND_STRING("The law Kyurem set over the\nbattle cracks and lifts."),
    [STRINGID_ENCKYUREMCHARGE]       = COMPOUND_STRING("Kyurem draws the heat out of\nthe air..."),
    [STRINGID_ENCKYUREMCHARGEGUARD]  = COMPOUND_STRING("The cold is gathering\nsomewhere above, and Kyurem\pis harder to reach."),
    [STRINGID_ENCKYUREMCHARGEREMIND] = COMPOUND_STRING("The blade of cold overhead\ngrows heavier."),
    [STRINGID_ENCKYUREMBROKEN]       = COMPOUND_STRING("The gathering cold comes\napart!"),
    [STRINGID_ENCKYUREMOPENGUARD]    = COMPOUND_STRING("Kyurem is wide open - hit it\nnow!"),
    [STRINGID_ENCKYUREMZERO]         = COMPOUND_STRING("ABSOLUTE ZERO."),
    [STRINGID_ENCKYUREMGLACIATE]     = COMPOUND_STRING("GLACIATE - the whole field\nfreezes where it stands!"),
    [STRINGID_ENCKYUREMAGAIN1]       = COMPOUND_STRING("The cold starts gathering\nagain, faster this time."),
    [STRINGID_ENCKYUREMAGAIN2]       = COMPOUND_STRING("Each freeze bites deeper than\nthe last. Kyurem will not\pstop.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCKYUREMHEAT]         = COMPOUND_STRING("Heat reaches the hollow in\nKyurem's chest. The frost\pgives ground!"),
    [STRINGID_ENCKYUREMCONSUME]      = COMPOUND_STRING("Kyurem swallows what is left\nof it."),
    [STRINGID_ENCKYUREMHEALFED]      = COMPOUND_STRING("The health you put back is\ndrawn straight into Kyurem."),
    [STRINGID_ENCKYUREMCRACKS1]      = COMPOUND_STRING("Kyurem's body cracks open\nalong the frost."),
    [STRINGID_ENCKYUREMCRACKS2]      = COMPOUND_STRING("There is nothing inside it at\nall.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCKYUREMVOIDCUE]      = COMPOUND_STRING("The hollow in its chest\nstarts pulling at the\pbattlefield.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCKYUREMVOIDBITES]    = COMPOUND_STRING("The hollow drinks your side's\nhealing before it lands."),
    [STRINGID_ENCKYUREMREACH1]       = COMPOUND_STRING("Kyurem reaches for something\nthat is not there."),
    [STRINGID_ENCKYUREMREACH2]       = COMPOUND_STRING("It remembers being whole, and\nfights like it.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCKYUREMRESREMEMBER]  = COMPOUND_STRING("Kyurem remembers a dragon you\nhave met, and the memory\panswers it!"),
    [STRINGID_ENCKYUREMRESFAINT]     = COMPOUND_STRING("Kyurem reaches for a dragon\nyou have never met. Only a\pfaint echo answers."),
    [STRINGID_ENCKYUREMRESFIRE]      = COMPOUND_STRING("A white flame gathers in the\nhollow of its chest!"),
    [STRINGID_ENCKYUREMRESBOLT]      = COMPOUND_STRING("Black lightning gathers in\nthe hollow of its chest!"),
    [STRINGID_ENCKYUREMABSORB]       = COMPOUND_STRING("Kyurem is pulling that power\ninto the hollow in its chest!"),
    [STRINGID_ENCKYUREMFUSED]        = COMPOUND_STRING("The power takes hold. Kyurem\nis not alone in that body\pany more!"),
    [STRINGID_ENCKYUREMFUSIONFAILS]  = COMPOUND_STRING("The resonance tears loose!"),
    [STRINGID_ENCKYUREMFUSIONTEARS]  = COMPOUND_STRING("The fusion tears itself\napart! Kyurem is torn with\pit."),
    [STRINGID_ENCKYUREMLOCK1]        = COMPOUND_STRING("Everything stops."),
    [STRINGID_ENCKYUREMLOCK2]        = COMPOUND_STRING("The battle, the field, the\nair - all of it belongs to\pthe cold now.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCKYUREMTHAWCUE]      = COMPOUND_STRING("Only heat will move anything\nnow.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCKYUREMENERGY]       = COMPOUND_STRING("The ice over the battle\ngroans. Something is giving."),
    [STRINGID_ENCKYUREMSHATTER1]     = COMPOUND_STRING("The ice screams, and splits."),
    [STRINGID_ENCKYUREMSHATTER2]     = COMPOUND_STRING("Kyurem falls out of its own\ncold, spent.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCKYUREMWEAKENED]     = COMPOUND_STRING("Kyurem is hollow and still.\pNow - now is the moment to\ncatch it!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCLANDORUSINTRO1]        = COMPOUND_STRING("The ground under your feet\nanswers to Landorus."),
    [STRINGID_ENCLANDORUSINTRO2]        = COMPOUND_STRING("It does not brace itself. The\nland will do that for it.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCLANDORUSSTAB0]         = COMPOUND_STRING("The land lies barren. Nothing\nrises to shield Landorus."),
    [STRINGID_ENCLANDORUSSTAB1]         = COMPOUND_STRING("The soil stirs. Landorus\nstands firmer on it."),
    [STRINGID_ENCLANDORUSSTAB2]         = COMPOUND_STRING("Grass breaks through the\nbarren ground."),
    [STRINGID_ENCLANDORUSSTAB3]         = COMPOUND_STRING("The land is flourishing.\nStone begins to answer it."),
    [STRINGID_ENCLANDORUSSTAB4]         = COMPOUND_STRING("The ground is abundant, and\nit feeds Landorus."),
    [STRINGID_ENCLANDORUSSTAB5]         = COMPOUND_STRING("CONTINENTAL. The whole\nlandmass moves with it.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCLANDORUSLANDCUE]       = COMPOUND_STRING("The land is restless beneath\nyou."),
    [STRINGID_ENCLANDORUSPILLARUP]      = COMPOUND_STRING("A pillar of earth grinds up\nout of the ground!"),
    [STRINGID_ENCLANDORUSPILLARSTANDS]  = COMPOUND_STRING("The earth pillar takes the\nblow for Landorus!"),
    [STRINGID_ENCLANDORUSPILLARFALLS]   = COMPOUND_STRING("An earth pillar crumbles\naway."),
    [STRINGID_ENCLANDORUSPILLARCRACKS]  = COMPOUND_STRING("The force of that splits a\npillar apart!"),
    [STRINGID_ENCLANDORUSDESOLATION]    = COMPOUND_STRING("The land recoils from the\nheat and the cold!"),
    [STRINGID_ENCLANDORUSDESOREFUSED]   = COMPOUND_STRING("The land does not yield this\ntime. Landorus will not let\pit."),
    [STRINGID_ENCLANDORUSSHIFTCOLLAPSE] = COMPOUND_STRING("The gathering shift comes\napart under it!"),
    [STRINGID_ENCLANDORUSSHIFT1]        = COMPOUND_STRING("Landorus drives its power\ndown into the ground..."),
    [STRINGID_ENCLANDORUSSHIFT2]        = COMPOUND_STRING("Something enormous is about\nto move."),
    [STRINGID_ENCLANDORUSSHIFTLANDS]    = COMPOUND_STRING("TECTONIC SHIFT - the ground\nheaves and turns over!"),
    [STRINGID_ENCLANDORUSSHIFTSCOUR]    = COMPOUND_STRING("The upheaval scours the\nfield clean."),
    [STRINGID_ENCLANDORUSFERTILE]       = COMPOUND_STRING("The barren ground turns\nfertile underfoot."),
    [STRINGID_ENCLANDORUSABUNDANCE]     = COMPOUND_STRING("The fertile land feeds\nLandorus."),
    [STRINGID_ENCLANDORUSSMOTHER]       = COMPOUND_STRING("The sky is smothered. No\nweather holds here."),
    [STRINGID_ENCLANDORUSGROUNDSTIRS]   = COMPOUND_STRING("The ground drinks that in.\nLandorus stands taller."),
    [STRINGID_ENCLANDORUSTHEYFELL]      = COMPOUND_STRING("The land takes what fell on\nit."),
    [STRINGID_ENCLANDORUSRUMBLE]        = COMPOUND_STRING("CONTINENTAL RUMBLE - the\nground will not stop moving!"),
    [STRINGID_ENCLANDORUSRUMBLEHARDER]  = COMPOUND_STRING("The rumbling drives deeper\nthan before!"),
    [STRINGID_ENCLANDORUSTHERIAN1]      = COMPOUND_STRING("Landorus sheds the shape it\nwas holding."),
    [STRINGID_ENCLANDORUSTHERIAN2]      = COMPOUND_STRING("The Abundance Pokemon takes\nits true form!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCLANDORUSGRAVITY]       = COMPOUND_STRING("The air itself grows heavy.\nNothing leaves the ground\pnow."),
    [STRINGID_ENCLANDORUSSTORM]         = COMPOUND_STRING("Landorus gathers a searing\nsandstorm!"),
    [STRINGID_ENCLANDORUSWRATH1]        = COMPOUND_STRING("Landorus stops cultivating\nand starts spending."),
    [STRINGID_ENCLANDORUSWRATH2]        = COMPOUND_STRING("WRATH OF THE LAND. The\nground itself is its rage.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCLANDORUSSETTLES1]      = COMPOUND_STRING("The land has nothing left to\ngive..."),
    [STRINGID_ENCLANDORUSSETTLES2]      = COMPOUND_STRING("The earth settles, and\nLandorus settles with it.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCLANDORUSSETTLED]       = COMPOUND_STRING("Landorus drops out of the\nair, exhausted."),
    [STRINGID_ENCLANDORUSWEAKENED]      = COMPOUND_STRING("Landorus is spent, and the\nland is quiet.\pNow - now is the moment to\ncatch it!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCTHUNDURUSINTRO1]        = COMPOUND_STRING("The air over the field is\nalready humming."),
    [STRINGID_ENCTHUNDURUSINTRO2]        = COMPOUND_STRING("Thundurus does not gather\nitself. It gathers charge.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCTHUNDURUSCHARGE0]       = COMPOUND_STRING("The air goes still. Nothing\ncrackles around Thundurus."),
    [STRINGID_ENCTHUNDURUSCHARGE1]       = COMPOUND_STRING("A low hum builds around\nThundurus."),
    [STRINGID_ENCTHUNDURUSCHARGE2]       = COMPOUND_STRING("The air crackles, and the\nhair on your arms lifts."),
    [STRINGID_ENCTHUNDURUSCHARGE3]       = COMPOUND_STRING("Arcs jump off Thundurus.\nThe strikes will bite deeper."),
    [STRINGID_ENCTHUNDURUSCHARGE4]       = COMPOUND_STRING("The light is blinding. The\nstorm is nearly full."),
    [STRINGID_ENCTHUNDURUSCHARGE5]       = COMPOUND_STRING("OVERCHARGED. Thundurus holds\nmore than it can stand.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCTHUNDURUSCHARGE6]       = COMPOUND_STRING("The charge has nowhere left\nto go!"),
    [STRINGID_ENCTHUNDURUSSTATICCUE]     = COMPOUND_STRING("Static claws at your skin."),
    [STRINGID_ENCTHUNDURUSROD]           = COMPOUND_STRING("A rod of lightning spears\ndown beside your POKéMON!"),
    [STRINGID_ENCTHUNDURUSCOUNT3]        = COMPOUND_STRING("The rod hums.\nLIGHTNING IN 3 TURNS."),
    [STRINGID_ENCTHUNDURUSCOUNT2]        = COMPOUND_STRING("The rod hums louder.\nLIGHTNING IN 2 TURNS."),
    [STRINGID_ENCTHUNDURUSCOUNT1]        = COMPOUND_STRING("The air tears open.\nLIGHTNING NEXT TURN!"),
    [STRINGID_ENCTHUNDURUSRODGROUNDED]   = COMPOUND_STRING("The rod loses its ground and\ngutters out."),
    [STRINGID_ENCTHUNDURUSRODEARTHED]    = COMPOUND_STRING("The bolt earths itself\nharmlessly!"),
    [STRINGID_ENCTHUNDURUSSTRIKE]        = COMPOUND_STRING("The bolt comes down on your\nPOKéMON!"),
    [STRINGID_ENCTHUNDURUSSTRIKEHARD]    = COMPOUND_STRING("The bolt comes down harder\nthan the last one!"),
    [STRINGID_ENCTHUNDURUSSTRIKESLOW]    = COMPOUND_STRING("The shock leaves your POKéMON\nsluggish."),
    [STRINGID_ENCTHUNDURUSBREAK1]        = COMPOUND_STRING("The gathering charge comes\napart under that!"),
    [STRINGID_ENCTHUNDURUSBREAK2]        = COMPOUND_STRING("Thundurus reels, its rhythm\nbroken."),
    [STRINGID_ENCTHUNDURUSOVERLOAD1]     = COMPOUND_STRING("Thundurus cannot hold it any\nlonger..."),
    [STRINGID_ENCTHUNDURUSOVERLOAD2]     = COMPOUND_STRING("OVERLOAD - the whole storm\ndischarges at once!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCTHUNDURUSOVERLOAD3]     = COMPOUND_STRING("Thundurus is empty, and its\nguard is gone with it."),
    [STRINGID_ENCTHUNDURUSIDLE]          = COMPOUND_STRING("Lightning falls out of the\nstorm on its own!"),
    [STRINGID_ENCTHUNDURUSSTORM1]        = COMPOUND_STRING("The clouds close over the\nfield."),
    [STRINGID_ENCTHUNDURUSSTORM2]        = COMPOUND_STRING("THUNDERSTORM. The rain will\nnot stop now.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCTHUNDURUSSHAKESOFF]     = COMPOUND_STRING("The storm shakes Thundurus\nout of it."),
    [STRINGID_ENCTHUNDURUSWRATH1]        = COMPOUND_STRING("Thundurus sheds the shape it\nwas holding."),
    [STRINGID_ENCTHUNDURUSWRATH2]        = COMPOUND_STRING("WRATH OF THE STORM - the\nBolt Strike POKéMON is loose!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCTHUNDURUSTAILWIND]      = COMPOUND_STRING("A tailwind screams in behind\nThundurus."),
    [STRINGID_ENCTHUNDURUSSILENT1]       = COMPOUND_STRING("The storm suddenly goes\nsilent..."),
    [STRINGID_ENCTHUNDURUSSILENT2]       = COMPOUND_STRING("The rain stops. Thundurus\nhangs there, exhausted.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCTHUNDURUSRESUME]        = COMPOUND_STRING("The clouds turn over, and\nthe storm picks up again!"),
    [STRINGID_ENCTHUNDURUSCONDUCT]       = COMPOUND_STRING("The charged air drinks that\nin."),
    [STRINGID_ENCTHUNDURUSEARTH]         = COMPOUND_STRING("The charge earths itself out\nof Thundurus!"),
    [STRINGID_ENCTHUNDURUSTHEYFELL]      = COMPOUND_STRING("The storm feeds on what it\nfelled."),
    [STRINGID_ENCTHUNDURUSWEAKENED]      = COMPOUND_STRING("Thundurus is grounded and\nspent.\pNow - now is the moment to\ncatch it!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCMELOETTAINTRO1]        = COMPOUND_STRING("Meloetta turns, and the air\ntakes the shape of a note."),
    [STRINGID_ENCMELOETTAINTRO2]        = COMPOUND_STRING("It is not fighting you.\nIt is performing for you."),
    [STRINGID_ENCMELOETTAINTRO3]        = COMPOUND_STRING("Listen to the bar. The song is\nnot soft all the way through.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCMELOETTAVERSE]         = COMPOUND_STRING("The verse plays softly, and\nMeloetta sings with you."),
    [STRINGID_ENCMELOETTACHORUS]        = COMPOUND_STRING("The chorus swells - Meloetta\nis singing over your attacks!"),
    [STRINGID_ENCMELOETTAGATHER]        = COMPOUND_STRING("Meloetta gathers every note\nit has sung so far..."),
    [STRINGID_ENCMELOETTAQUIET]         = COMPOUND_STRING("Only a quiet melody remains.\nThe bar has stopped counting."),
    [STRINGID_ENCMELOETTARECUR]         = COMPOUND_STRING("The song goes on, beat after\nbeat."),
    [STRINGID_ENCMELOETTAMETER1]        = COMPOUND_STRING("Meloetta finds its place in\nthe bar."),
    [STRINGID_ENCMELOETTAMETER2]        = COMPOUND_STRING("The soft part of the bar is\ngetting shorter!"),
    [STRINGID_ENCMELOETTAMETER3]        = COMPOUND_STRING("A refrain starts ringing out\nbetween the beats!"),
    [STRINGID_ENCMELOETTAMETER4]        = COMPOUND_STRING("Only one soft beat is left in\nthe bar!"),
    [STRINGID_ENCMELOETTAMETER5]        = COMPOUND_STRING("The whole bar is chorus now.\nNo soft beat is left!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCMELOETTAMETERFALL]     = COMPOUND_STRING("The song loses its place, and\nthe soft beats come back."),
    [STRINGID_ENCMELOETTABREAK]         = COMPOUND_STRING("RHYTHM BREAK! The song falls\nback to its opening bar."),
    [STRINGID_ENCMELOETTATOPIROUETTE]   = COMPOUND_STRING("Meloetta stops singing, and\nstarts to dance!"),
    [STRINGID_ENCMELOETTATOARIA]        = COMPOUND_STRING("The dance collapses, and the\nsinging begins again!"),
    [STRINGID_ENCMELOETTACYCLE]         = COMPOUND_STRING("The performance turns over on\nits own!"),
    [STRINGID_ENCMELOETTACHIP]          = COMPOUND_STRING("The refrain rings straight\nthrough your POKéMON!"),
    [STRINGID_ENCMELOETTAFOLLOWUP]      = COMPOUND_STRING("The dance snaps back for a\nsecond strike!"),
    [STRINGID_ENCMELOETTACRESC1]        = COMPOUND_STRING("Meloetta draws the whole song\nin around itself..."),
    [STRINGID_ENCMELOETTACRESC2]        = COMPOUND_STRING("CRESCENDO! Everything you\nbuilt is swept away!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCMELOETTAFINALE1]       = COMPOUND_STRING("Meloetta lets the finale go!"),
    [STRINGID_ENCMELOETTAFINALE2]       = COMPOUND_STRING("The song starts over from the\nbeginning, quieter."),
    [STRINGID_ENCMELOETTAINT1]          = COMPOUND_STRING("The performance is\ninterrupted!"),
    [STRINGID_ENCMELOETTAINT2]          = COMPOUND_STRING("Meloetta is left wide open,\nand it cannot act next turn!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCMELOETTAGRAND1]        = COMPOUND_STRING("Meloetta will not be hurried.\nThe second act begins."),
    [STRINGID_ENCMELOETTAGRAND2]        = COMPOUND_STRING("GRAND PERFORMANCE! Both of\nits forms are stronger now!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCMELOETTAGRAND3]        = COMPOUND_STRING("It will change shape on its\nown from here!"),
    [STRINGID_ENCMELOETTAFINAL1]        = COMPOUND_STRING("Meloetta holds one long note,\nand the hall goes quiet..."),
    [STRINGID_ENCMELOETTAFINAL2]        = COMPOUND_STRING("It draws a little of itself\nback together."),
    [STRINGID_ENCMELOETTAFINAL3]        = COMPOUND_STRING("THE FINAL NOTE. The song has\nstopped, and so has the bar.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCMELOETTASPENT1]        = COMPOUND_STRING("Meloetta has nothing left to\nsing with."),
    [STRINGID_ENCMELOETTAWEAKENED]      = COMPOUND_STRING("Meloetta is spent, and the\nhall is silent.\pNow - now is the moment to\ncatch it!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCMELOETTASILENCE]       = COMPOUND_STRING("The hall falls silent, and\nMeloetta fills it."),
    [STRINGID_ENCTORNADUSINTRO1]           = COMPOUND_STRING("The air around Tornadus is\nalready moving."),
    [STRINGID_ENCTORNADUSINTRO2]           = COMPOUND_STRING("It does not look at you. It\nlooks at where you will be.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCTORNADUSWIND0]            = COMPOUND_STRING("The wind drops away, and the\nair goes calm."),
    [STRINGID_ENCTORNADUSWIND1]            = COMPOUND_STRING("A wind begins to rise."),
    [STRINGID_ENCTORNADUSWIND2]            = COMPOUND_STRING("A gale sweeps across the\nfield now!"),
    [STRINGID_ENCTORNADUSWIND3]            = COMPOUND_STRING("The wind turns tempestuous!"),
    [STRINGID_ENCTORNADUSWIND4]            = COMPOUND_STRING("The wind is ROARING - it is\nmoving everything!"),
    [STRINGID_ENCTORNADUSWIND5]            = COMPOUND_STRING("A CYCLONE is about to tear\nopen!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCTORNADUSSTORMFRONT]       = COMPOUND_STRING("Tornadus turns, and a STORM\nFRONT rolls in behind it!\pThe wind is feeding itself\nnow!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCTORNADUSTEMPEST]          = COMPOUND_STRING("Tornadus sheds its shape and\nbecomes the TEMPEST!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCTORNADUSWEAKENED]         = COMPOUND_STRING("Tornadus is grounded, and the\nair is still.\pNow - now is the moment to\ncatch it!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCTORNADUSEYEFORMS]         = COMPOUND_STRING("A calm centre opens - the EYE\nOF THE STORM!\pStone and cold are lost\nsomewhere inside it.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCTORNADUSEYEHOLDS]         = COMPOUND_STRING("The eye holds, and Tornadus\nis hard to find in it."),
    [STRINGID_ENCTORNADUSEYEPASSES]        = COMPOUND_STRING("The eye passes, and the wind\ncloses over it again."),
    [STRINGID_ENCTORNADUSEYETORN]          = COMPOUND_STRING("The sky is already taken -\nthe eye is torn open!"),
    [STRINGID_ENCTORNADUSEYEBROKEN]        = COMPOUND_STRING("The jolt scatters the eye!"),
    [STRINGID_ENCTORNADUSEYESWALLOWS]      = COMPOUND_STRING("The stone is swallowed by the\ncalm, and nothing changes."),
    [STRINGID_ENCTORNADUSKEYROCK]          = COMPOUND_STRING("The stone breaks the wind\napart!"),
    [STRINGID_ENCTORNADUSKEYICE]           = COMPOUND_STRING("The cold seizes Tornadus -\nit staggers badly!"),
    [STRINGID_ENCTORNADUSKEYELECTRIC]      = COMPOUND_STRING("The jolt tears through the\nwind and disrupts it!"),
    [STRINGID_ENCTORNADUSGATHERS]          = COMPOUND_STRING("Tornadus begins to gather a\nHURRICANE..."),
    [STRINGID_ENCTORNADUSGATHER3]          = COMPOUND_STRING("Three turns until the sky\nbreaks open!"),
    [STRINGID_ENCTORNADUSGATHER2]          = COMPOUND_STRING("Two turns until the sky\nbreaks open!"),
    [STRINGID_ENCTORNADUSGATHER1]          = COMPOUND_STRING("One turn until the sky\nbreaks open!"),
    [STRINGID_ENCTORNADUSGATHERBREAKS]     = COMPOUND_STRING("The sky is about to break!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCTORNADUSHURRICANE]        = COMPOUND_STRING("THE HURRICANE LANDS!"),
    [STRINGID_ENCTORNADUSGATHERBROKEN]     = COMPOUND_STRING("The jolt rips the gathering\nhurricane apart!"),
    [STRINGID_ENCTORNADUSGATHERDISPERSED]  = COMPOUND_STRING("The hurricane finds the sky\ntaken, and disperses!"),
    [STRINGID_ENCTORNADUSGUST]             = COMPOUND_STRING("Gusts sweep across the whole\nfield!"),
    [STRINGID_ENCTORNADUSCYCLONE1]         = COMPOUND_STRING("THE CYCLONE! Everything on\nthe field is thrown clear!"),
    [STRINGID_ENCTORNADUSCYCLONE2]         = COMPOUND_STRING("ANOTHER CYCLONE tears the\nfield open!"),
    [STRINGID_ENCTORNADUSCYCLONE3]         = COMPOUND_STRING("The cyclones will not stop\ncoming!"),
    [STRINGID_ENCTORNADUSDISPLACE]         = COMPOUND_STRING("The wind decides who stands\nin front of it!"),
    [STRINGID_ENCTORNADUSDISPLACEFAIL]     = COMPOUND_STRING("There is nothing left for the\nwind to move."),
    [STRINGID_ENCTORNADUSTRAPPED]          = COMPOUND_STRING("The tempest closes in - there\nis nowhere to step back to!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCTORNADUSTRAPPEDCUE]       = COMPOUND_STRING("The tempest holds you where\nyou are."),
    [STRINGID_ENCTORNADUSSUBSIDE]          = COMPOUND_STRING("The winds subside all at\nonce, and Tornadus sags!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCTORNADUSOPENSKY]          = COMPOUND_STRING("The sky is open. Tornadus\ncannot cover itself!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCTORNADUSGATHERAGAIN]      = COMPOUND_STRING("The winds gather again."),
    [STRINGID_ENCTORNADUSTHEYFELL]         = COMPOUND_STRING("The wind takes what it\nfelled, and rises."),
    [STRINGID_ENCTORNADUSTHEYENTERED]      = COMPOUND_STRING("The wind does not care who\nis standing there."),
    [STRINGID_ENCTORNADUSWEATHERBITES]     = COMPOUND_STRING("The sky is already taken, and\nthe wind loses its hold."),
    [STRINGID_ENCRESHIRAMINTRO1]            = COMPOUND_STRING("Reshiram's eyes settle on you,\nand do not move."),
    [STRINGID_ENCRESHIRAMINTRO2]            = COMPOUND_STRING("It is not waiting to attack.\nIt is waiting to understand.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCRESHIRAMBANDUNREAD]        = COMPOUND_STRING("Reshiram has nothing on you\nyet."),
    [STRINGID_ENCRESHIRAMBANDSTUDIED]       = COMPOUND_STRING("Reshiram is studying the way\nyou fight."),
    [STRINGID_ENCRESHIRAMBANDUNDERSTOOD]    = COMPOUND_STRING("Reshiram understands what you\nare trying to do."),
    [STRINGID_ENCRESHIRAMBANDKNOWN]         = COMPOUND_STRING("Reshiram knows exactly what\nyou will do.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCRESHIRAMSTILLWATCHING]     = COMPOUND_STRING("Reshiram has not looked away."),
    [STRINGID_ENCRESHIRAMNOTHINGHIDDEN]     = COMPOUND_STRING("Nothing you do reaches it\nunseen."),
    [STRINGID_ENCRESHIRAMREADPHYSICAL]      = COMPOUND_STRING("It reads the tension in your\nPokemon's stance!"),
    [STRINGID_ENCRESHIRAMREADSPECIAL]       = COMPOUND_STRING("It reads the energy gathering\naround your Pokemon!"),
    [STRINGID_ENCRESHIRAMREADSTATUS]        = COMPOUND_STRING("It sees your Pokemon is not\ngoing to attack!"),
    [STRINGID_ENCRESHIRAMSEENTHATBEFORE]    = COMPOUND_STRING("It has seen that move before."),
    [STRINGID_ENCRESHIRAMNOTWHATITEXPECTED] = COMPOUND_STRING("That was not what Reshiram\nexpected!"),
    [STRINGID_ENCRESHIRAMEXPOSE]            = COMPOUND_STRING("The white flame lays your\nPokemon bare!"),
    [STRINGID_ENCRESHIRAMTRUTHMARK]         = COMPOUND_STRING("Reshiram marks your Pokemon\nwith white fire!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCRESHIRAMMARKDEEPENS]       = COMPOUND_STRING("The mark burns deeper into\nyour Pokemon!"),
    [STRINGID_ENCRESHIRAMATTENTIONSHIFTS]   = COMPOUND_STRING("Reshiram's attention shifts!"),
    [STRINGID_ENCRESHIRAMFLAMEGATHERS]      = COMPOUND_STRING("White flame gathers along\nReshiram's tail..."),
    [STRINGID_ENCRESHIRAMTRUTHFLAME]        = COMPOUND_STRING("TRUTH FLAME sweeps the field\nclean!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCRESHIRAMHEALINGDENIED]     = COMPOUND_STRING("The white flame sears shut\nevery way back!"),
    [STRINGID_ENCRESHIRAMIDENTIFIEDTARGET]  = COMPOUND_STRING("Reshiram has identified its\ntarget.\pBlue fire begins to gather!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCRESHIRAMCHARGECUE]         = COMPOUND_STRING("The blue fire grows brighter."),
    [STRINGID_ENCRESHIRAMRETARGET]          = COMPOUND_STRING("The blue fire follows your\nPokemon!"),
    [STRINGID_ENCRESHIRAMBLUEFLARE]         = COMPOUND_STRING("BLUE FLARE erupts across the\nbattlefield!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCRESHIRAMLOSTTHETHREAD]     = COMPOUND_STRING("The blue fire scatters -\nReshiram lost the thread!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCRESHIRAMSEENTHETRUTH]      = COMPOUND_STRING("Reshiram has seen the truth\nof your Pokemon!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCRESHIRAMMAKEITDOUBT]       = COMPOUND_STRING("Reshiram is certain of you\nnow.\pMake it doubt!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCRESHIRAMWHITELIGHT]        = COMPOUND_STRING("Reshiram sheds a white light\nover everything!\pThere is nowhere left to\nhide!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCRESHIRAMJUDGMENT]          = COMPOUND_STRING("Reshiram gathers TRUTH'S\nJUDGMENT!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCRESHIRAMJUDGMENTCUE]       = COMPOUND_STRING("The white light draws inward."),
    [STRINGID_ENCRESHIRAMJUDGMENTFIRES]     = COMPOUND_STRING("TRUTH'S JUDGMENT falls!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCRESHIRAMFLAMESFADE]        = COMPOUND_STRING("The white flames fade, and\nReshiram sags.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCRESHIRAMWEAKENED]          = COMPOUND_STRING("Reshiram is spent, and the\nwhite fire is out.\pNow - now is the moment to\ncatch it!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCZEKROMINTRO1]             = COMPOUND_STRING("Zekrom does not look at your\nPokemon. It looks at you."),
    [STRINGID_ENCZEKROMINTRO2]             = COMPOUND_STRING("Something in it is waiting to\nsee what you believe.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCZEKROMKNOWSTHEWHITE]      = COMPOUND_STRING("It knows the white one's scent\non you.\pIt has been waiting for this.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCZEKROMBANDDOUBT]          = COMPOUND_STRING("Zekrom is unsure of you."),
    [STRINGID_ENCZEKROMBANDCONVICTION]     = COMPOUND_STRING("Zekrom is settling into its\nchoice."),
    [STRINGID_ENCZEKROMBANDCERTAINTY]      = COMPOUND_STRING("Zekrom is certain of its\ncourse."),
    [STRINGID_ENCZEKROMBANDABSOLUTE]       = COMPOUND_STRING("Zekrom believes absolutely.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCZEKROMCHOSENPATH]         = COMPOUND_STRING("ZEKROM HAS CHOSEN ITS PATH.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCZEKROMIDEALPOWER]         = COMPOUND_STRING("It believes in overwhelming\nforce."),
    [STRINGID_ENCZEKROMIDEALENDURANCE]     = COMPOUND_STRING("It braces to outlast you."),
    [STRINGID_ENCZEKROMIDEALSPEED]         = COMPOUND_STRING("It believes nothing can be\nfast enough."),
    [STRINGID_ENCZEKROMIDEALRESOLVE]       = COMPOUND_STRING("It will not be moved from its\ncourse."),
    [STRINGID_ENCZEKROMPATHWAVERS]         = COMPOUND_STRING("Zekrom's chosen path wavers..."),
    [STRINGID_ENCZEKROMTRAPPED]            = COMPOUND_STRING("Zekrom's will pins your\nPokemon in place!"),
    [STRINGID_ENCZEKROMSHAKEN]             = COMPOUND_STRING("ZEKROM'S IDEALS HAVE BEEN\nSHAKEN!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCZEKROMSHAKEOUTLAST]       = COMPOUND_STRING("Your Pokemon is still standing\nstrong."),
    [STRINGID_ENCZEKROMSHAKEOVERWHELM]     = COMPOUND_STRING("That blow went right through\nZekrom's guard!"),
    [STRINGID_ENCZEKROMSHAKESLOWED]        = COMPOUND_STRING("Zekrom could not get ahead of\nyour Pokemon."),
    [STRINGID_ENCZEKROMSHAKEDISRUPT]       = COMPOUND_STRING("Zekrom has been knocked off\nits course!"),
    [STRINGID_ENCZEKROMREFUSESTOYIELD]     = COMPOUND_STRING("It sees your Pokemon refuse\nto yield."),
    [STRINGID_ENCZEKROMWATCHESRETREAT]     = COMPOUND_STRING("Zekrom watches your Pokemon\nretreat."),
    [STRINGID_ENCZEKROMPATHISSET]          = COMPOUND_STRING("Zekrom's path is set."),
    [STRINGID_ENCZEKROMWILLNOTCHANGE]      = COMPOUND_STRING("Nothing you do has changed\nits mind."),
    [STRINGID_ENCZEKROMBOLTGATHERS]        = COMPOUND_STRING("Black lightning gathers around\nZekrom's tail!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCZEKROMBOLTCUE]            = COMPOUND_STRING("The lightning grows louder."),
    [STRINGID_ENCZEKROMBOLTPOWER]          = COMPOUND_STRING("It is winding up to break\nsomething."),
    [STRINGID_ENCZEKROMBOLTENDURANCE]      = COMPOUND_STRING("It is drawing the strike back\ninto itself."),
    [STRINGID_ENCZEKROMBOLTSPEED]          = COMPOUND_STRING("It will not wait its turn."),
    [STRINGID_ENCZEKROMBOLTRESOLVE]        = COMPOUND_STRING("Nothing will interrupt this\none."),
    [STRINGID_ENCZEKROMBOLTSTRIKE]         = COMPOUND_STRING("BOLT STRIKE tears across the\nbattlefield!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCZEKROMFOCUSBREAKS]        = COMPOUND_STRING("ZEKROM'S FOCUS BREAKS!\pThe lightning scatters!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCZEKROMABSOLUTECONVICTION] = COMPOUND_STRING("Zekrom has seen enough of you\nto decide!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCZEKROMWILLNOTCHANGEEVER]  = COMPOUND_STRING("It will not change.\pNot now. Not for anything.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCZEKROMIDEALAWAKENING]     = COMPOUND_STRING("Zekrom's ideal awakens, and\nthe air turns to current!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCZEKROMBOLTIDEAL]          = COMPOUND_STRING("It gathers BOLT STRIKE:\nIDEAL!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCZEKROMBOLTIDEALCUE]       = COMPOUND_STRING("The whole field is humming."),
    [STRINGID_ENCZEKROMBOLTIDEALFIRES]     = COMPOUND_STRING("BOLT STRIKE: IDEAL falls!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCZEKROMCONVICTIONFALTERS]  = COMPOUND_STRING("Zekrom's conviction finally\nfalters.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCZEKROMWEAKENED]           = COMPOUND_STRING("Zekrom is spent, and the\nlightning is gone.\pNow - now is the moment to\ncatch it!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCDIANCIEAWAKENS]           = COMPOUND_STRING("Diancie blooms with cold light!\nA crystal lattice takes shape!"),
    [STRINGID_ENCDIANCIEFACETFORMS]        = COMPOUND_STRING("A new facet grows on Diancie!\nThe lattice spreads wider."),
    [STRINGID_ENCDIANCIELATTICETHICKENS]   = COMPOUND_STRING("The lattice thickens!\nAttacks glance off the facets."),
    [STRINGID_ENCDIANCIELATTICETHINS]      = COMPOUND_STRING("The lattice thins!\nDiancie's guard is weaker now."),
    [STRINGID_ENCDIANCIESTANDINGLATTICE]   = COMPOUND_STRING("Crystal walls ring Diancie.\nOnly a hard blow can crack it."),
    [STRINGID_ENCDIANCIESHATTER]           = COMPOUND_STRING("The crystal shatters!\nShards tear into the party!"),
    [STRINGID_ENCDIANCIEENCASE]            = COMPOUND_STRING("{B_PLAYER_MON1_NAME} has been\ncrystallized!\pIt cannot flee or be healed!"),
    [STRINGID_ENCDIANCIEENCASEHOLD]        = COMPOUND_STRING("The crystal shell holds tight."),
    [STRINGID_ENCDIANCIEENCASEBREAK]       = COMPOUND_STRING("The crystal shell breaks apart!"),
    [STRINGID_ENCDIANCIESTORMBEGINS]       = COMPOUND_STRING("Diancie draws the facets inward!\nDIAMOND STORM is forming!"),
    [STRINGID_ENCDIANCIESTORMCHARGE]       = COMPOUND_STRING("The crystals gather more light...\nDiancie's guard is open!"),
    [STRINGID_ENCDIANCIESTORMFIRES]        = COMPOUND_STRING("DIAMOND STORM!\nDiamonds rip across the field!"),
    [STRINGID_ENCDIANCIESTORMCOLLAPSE]     = COMPOUND_STRING("THE CRYSTAL STORM COLLAPSES!\nDiancie reels from the backlash!"),
    [STRINGID_ENCDIANCIEJEWELOFLIFE]       = COMPOUND_STRING("Diancie sinks into the ground!\nThe field turns to crystal!\pIts facets feed it now."),
    [STRINGID_ENCDIANCIEGARDENHEAL]        = COMPOUND_STRING("The crystal field feeds Diancie!"),
    [STRINGID_ENCDIANCIEMEGA]              = COMPOUND_STRING("Diancie's ring blazes!\nMEGA DIANCIE!"),
    [STRINGID_ENCDIANCIECATACLYSM]         = COMPOUND_STRING("The field reflects the light!\nDIAMOND STORM: CATACLYSM!"),
    [STRINGID_ENCDIANCIEFADE]              = COMPOUND_STRING("The lattice goes dark.\nDiancie is spent."),
    [STRINGID_ENCDIANCIEWEAKENED]          = COMPOUND_STRING("Diancie's glow is nearly gone!\nNow is the moment!{PAUSE_UNTIL_PRESS}"),

    [STRINGID_ENCHOOPAINTRO]               = COMPOUND_STRING("Hoopa's rings hang in the air!\nIt's already smiling.{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCHOOPABANDLOW]             = COMPOUND_STRING("Hoopa's rings gutter and thin!\nIts guard is at its weakest!"),
    [STRINGID_ENCHOOPABANDMID]             = COMPOUND_STRING("Hoopa's rings turn slowly.\nIt's fairly well guarded."),
    [STRINGID_ENCHOOPABANDHIGH]            = COMPOUND_STRING("Hoopa's rings crowd the air!\nAlmost nothing gets through!"),
    [STRINGID_ENCHOOPARINGSTURN]           = COMPOUND_STRING("Hoopa's rings drift and turn."),
    [STRINGID_ENCHOOPATELEGRAPHHANDS]      = COMPOUND_STRING("A ring of grasping hands opens!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCHOOPATELEGRAPHTHIEVES]    = COMPOUND_STRING("A ring opens over your side!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCHOOPATELEGRAPHTEETH]      = COMPOUND_STRING("A ring turns to face Hoopa!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCHOOPATELEGRAPHCHAINS]     = COMPOUND_STRING("Heavy chains glint in a ring!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCHOOPARINGWIDENS]          = COMPOUND_STRING("The glowing ring widens!"),
    [STRINGID_ENCHOOPAHANDSHIT]            = COMPOUND_STRING("Hands drag {B_PLAYER_MON1_NAME}\nhalfway into the ring!"),
    [STRINGID_ENCHOOPATHIEVESHIT]          = COMPOUND_STRING("Hoopa takes everything it can!"),
    [STRINGID_ENCHOOPATHIEVESNONE]         = COMPOUND_STRING("There was nothing worth taking."),
    [STRINGID_ENCHOOPATHIEVESRETURN]       = COMPOUND_STRING("Stolen effects return as fire!"),
    [STRINGID_ENCHOOPATEETHARM]            = COMPOUND_STRING("The ring closes over Hoopa!"),
    [STRINGID_ENCHOOPATEETHHIT]            = COMPOUND_STRING("A blow lands from the ring\nbehind {B_PLAYER_MON1_NAME}!"),
    [STRINGID_ENCHOOPACHAINSHIT]           = COMPOUND_STRING("Chains coil around\n{B_PLAYER_MON1_NAME}!"),
    [STRINGID_ENCHOOPACHAINSTICK]          = COMPOUND_STRING("The chains tighten!"),
    [STRINGID_ENCHOOPACHAINSBREAK]         = COMPOUND_STRING("The ring snaps shut and\nreleases {B_PLAYER_MON1_NAME}!"),
    [STRINGID_ENCHOOPASHATTER]             = COMPOUND_STRING("The ring breaks on the guard!\nHoopa reels!"),
    [STRINGID_ENCHOOPAUNBOUND]             = COMPOUND_STRING("Six rings tear open!\nHoopa Unbound!"),
    [STRINGID_ENCHOOPAHYPERSPACE]          = COMPOUND_STRING("Hyperspace swallows the field!"),
    [STRINGID_ENCHOOPAHYPERTURN]           = COMPOUND_STRING("The distortion holds!"),
    [STRINGID_ENCHOOPACOLLAPSE]            = COMPOUND_STRING("The distortion collapses!"),
    [STRINGID_ENCHOOPASIXRINGS]            = COMPOUND_STRING("Six portals surround the field!\nHyperspace Fury!"),
    [STRINGID_ENCHOOPAPORTALARM]           = COMPOUND_STRING("A portal begins to glow!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCHOOPAPORTALFIRE]          = COMPOUND_STRING("An attack erupts from the portal!"),
    [STRINGID_ENCHOOPAPIERCE]              = COMPOUND_STRING("A ring opened inside the guard!"),
    [STRINGID_ENCHOOPAFAINT]               = COMPOUND_STRING("Hoopa pockets the loss and grins."),
    [STRINGID_ENCHOOPAFADE]                = COMPOUND_STRING("The portals collapse.\nHoopa is spent."),
    [STRINGID_ENCHOOPAWEAKENED]            = COMPOUND_STRING("Hoopa has no rings left to hide\nbehind! Now is the moment!{PAUSE_UNTIL_PRESS}"),

    [STRINGID_ENCVOLCANIONINTRO]            = COMPOUND_STRING("Volcanion's vents scream white!\pSteam pressure is building..."),
    [STRINGID_ENCVOLCANIONPRESSURELOW]      = COMPOUND_STRING("Volcanion's vents ease open.\nSTEAM PRESSURE: LOW"),
    [STRINGID_ENCVOLCANIONPRESSUREHIGH]     = COMPOUND_STRING("Volcanion's hull begins to hum.\nSTEAM PRESSURE: HIGH"),
    [STRINGID_ENCVOLCANIONPRESSURECRITICAL] = COMPOUND_STRING("Volcanion's plating glows red!\nSTEAM PRESSURE: CRITICAL"),
    [STRINGID_ENCVOLCANIONHISS]             = COMPOUND_STRING("Steam shrieks from the vents!"),
    [STRINGID_ENCVOLCANIONVENT]             = COMPOUND_STRING("Volcanion blasts its vents wide!\pA STEAM FIELD floods in!"),
    [STRINGID_ENCVOLCANIONSUPERHEATEDSTEAM] = COMPOUND_STRING("The steam thickens and glows!\pSUPERHEATED STEAM!"),
    [STRINGID_ENCVOLCANIONFIELDFADE]        = COMPOUND_STRING("The steam field breaks apart."),
    [STRINGID_ENCVOLCANIONREASSERT]         = COMPOUND_STRING("Volcanion's vents roar again.\nThe steam boils back in!"),
    [STRINGID_ENCVOLCANIONDRINK]            = COMPOUND_STRING("Volcanion drinks the water in!\nThe pressure jumps!"),
    [STRINGID_ENCVOLCANIONRELEASE]          = COMPOUND_STRING("Volcanion's pressure gives way!\pPRESSURE RELEASE!"),
    [STRINGID_ENCVOLCANIONSPENT]            = COMPOUND_STRING("Volcanion sags, vents gaping.\nIts guard is wide open!"),
    [STRINGID_ENCVOLCANIONSPENTEND]         = COMPOUND_STRING("Volcanion's vents clamp shut.\nIts guard is back up."),
    [STRINGID_ENCVOLCANIONPHASE2]           = COMPOUND_STRING("Volcanion floods its own core!\pSUPERHEATED! The pressure\nwill not settle now!"),
    [STRINGID_ENCVOLCANIONINJECT]           = COMPOUND_STRING("Volcanion draws in water fast!\pWATER INJECTION!"),
    [STRINGID_ENCVOLCANIONINJECTSPIKE]      = COMPOUND_STRING("The water flashes to steam!"),
    [STRINGID_ENCVOLCANIONPHASE3]           = COMPOUND_STRING("Volcanion's core runs away!\pMELTDOWN! It cannot vent\nanymore!"),
    [STRINGID_ENCVOLCANIONMELTDOWNWARN]     = COMPOUND_STRING("The hull screams. Not long now!"),
    [STRINGID_ENCVOLCANIONSTEAMERUPTION]    = COMPOUND_STRING("Volcanion's vents lock forward!\pIt readies STEAM ERUPTION!"),
    [STRINGID_ENCVOLCANIONERUPTION]         = COMPOUND_STRING("Volcanion lets go of it all!\pERUPTION!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCVOLCANIONFADE]             = COMPOUND_STRING("The steam thins to nothing.\nVolcanion is spent."),
    [STRINGID_ENCVOLCANIONWEAKENED]         = COMPOUND_STRING("Volcanion's vents hang open.\nNow is the moment!"),
    [STRINGID_ENCZYGARDEAWAKEN]             = COMPOUND_STRING("Zygarde surveys the field.\nAll imbalance will be corrected.\pWhoever pulls ahead is next.\nEven Zygarde itself."),
    [STRINGID_ENCZYGARDEBALANCETIP]         = COMPOUND_STRING("The scales tip your way.\nZygarde takes note."),
    [STRINGID_ENCZYGARDEBALANCETIPBOSS]     = COMPOUND_STRING("Zygarde has pulled ahead,\nand it will not allow that."),
    [STRINGID_ENCZYGARDEBANDGUARD]          = COMPOUND_STRING("Zygarde's order tightens.\nYour blows land shallower."),
    [STRINGID_ENCZYGARDEBANDYIELD]          = COMPOUND_STRING("Zygarde sheds its excess.\nIts guard hangs looser."),
    [STRINGID_ENCZYGARDECELLFORM]           = COMPOUND_STRING("A Zygarde Cell drifts in!\nStrike hard or it is absorbed."),
    [STRINGID_ENCZYGARDECELLBREAK]          = COMPOUND_STRING("The Cell is scattered!"),
    [STRINGID_ENCZYGARDECELLABSORB]         = COMPOUND_STRING("Zygarde absorbed the Cell!\nIts enforcement grows."),
    [STRINGID_ENCZYGARDEEXCESS]             = COMPOUND_STRING("EXCESS! Zygarde levels\nwhat you piled up."),
    [STRINGID_ENCZYGARDEEXCESSSELF]         = COMPOUND_STRING("EXCESS! Zygarde strips its\nown gains away."),
    [STRINGID_ENCZYGARDESCARCITY]           = COMPOUND_STRING("SCARCITY! Zygarde lifts\nthe weak back up."),
    [STRINGID_ENCZYGARDELIFEDRAW]           = COMPOUND_STRING("Zygarde closes the gap\nbetween the two of you."),
    [STRINGID_ENCZYGARDECHAOS]              = COMPOUND_STRING("CHAOS! Zygarde sweeps the\nfield clean."),
    [STRINGID_ENCZYGARDESTAGNATION]         = COMPOUND_STRING("STAGNATION! Zygarde forces\nthe field to change."),
    [STRINGID_ENCZYGARDEENFORCEBOOST]       = COMPOUND_STRING("Three times you climbed.\nZygarde ends the climb."),
    [STRINGID_ENCZYGARDEENFORCEHEAL]        = COMPOUND_STRING("You lean on healing.\nZygarde seals it shut."),
    [STRINGID_ENCZYGARDEENFORCESWITCH]      = COMPOUND_STRING("You keep fleeing the field.\nZygarde closes the way out."),
    [STRINGID_ENCZYGARDEENFORCESETUP]       = COMPOUND_STRING("You keep laying traps.\nZygarde unmakes them all."),
    [STRINGID_ENCZYGARDECOMPLETE]           = COMPOUND_STRING("Zygarde's cells swarm\ntogether!\pCOMPLETE FORME! Order will\nbe absolute."),
    [STRINGID_ENCZYGARDECOREENFORCER]       = COMPOUND_STRING("CORE ENFORCER! Zygarde\nstrikes the imbalance."),
    [STRINGID_ENCZYGARDELANDSWRATH]         = COMPOUND_STRING("LAND'S WRATH! Everything\nreturns to zero.\pZygarde's hoarded power is\ngone with it."),
    [STRINGID_ENCZYGARDEFADE]               = COMPOUND_STRING("Zygarde's cells come loose.\nThe pattern is breaking."),
    [STRINGID_ENCZYGARDEWEAKENED]           = COMPOUND_STRING("Zygarde is spent.\nNow is the moment!{PAUSE_UNTIL_PRESS}"),

    [STRINGID_ENCXERNEASINTRO1]             = COMPOUND_STRING("The forest holds its breath.\nEverything here is alive."),
    [STRINGID_ENCXERNEASINTRO2]             = COMPOUND_STRING("XERNEAS gives life freely-\nand takes it back as it needs."),
    [STRINGID_ENCXERNEASBANDWITHERING]      = COMPOUND_STRING("LIFE ENERGY is low.\nThe garden is thin."),
    [STRINGID_ENCXERNEASBANDBLOOMING]       = COMPOUND_STRING("LIFE ENERGY is BLOOMING!"),
    [STRINGID_ENCXERNEASBANDFLOURISHING]    = COMPOUND_STRING("LIFE ENERGY is FLOURISHING!\nXERNEAS is harder to wound."),
    [STRINGID_ENCXERNEASBANDETERNAL]        = COMPOUND_STRING("LIFE ENERGY is at its peak!\nNothing here can be hurt."),
    [STRINGID_ENCXERNEASBLOOM1]             = COMPOUND_STRING("A LIFE BLOOM opens!\nXERNEAS's petals will not wilt."),
    [STRINGID_ENCXERNEASBLOOM2]             = COMPOUND_STRING("A LIFE BLOOM opens!\nNo strike can find its heart."),
    [STRINGID_ENCXERNEASBLOOM3]             = COMPOUND_STRING("A LIFE BLOOM opens!\nNothing withers in the garden."),
    [STRINGID_ENCXERNEASBLOOM4]             = COMPOUND_STRING("A LIFE BLOOM opens!\nThe garden turns light aside."),
    [STRINGID_ENCXERNEASBLOOMSHATTERS]      = COMPOUND_STRING("The LIFE BLOOM shatters!\nIts energy returns to XERNEAS."),
    [STRINGID_ENCXERNEASLIFESURGE]          = COMPOUND_STRING("Life stirs on the field!\nXERNEAS drinks it in."),
    [STRINGID_ENCXERNEASBLESSING]           = COMPOUND_STRING("XERNEAS blesses your POKéMON!\nLife pours into it."),
    [STRINGID_ENCXERNEASBLESSINGFADES]      = COMPOUND_STRING("The LIFE BLESSING fades."),
    [STRINGID_ENCXERNEASBLESSINGRETURNS]    = COMPOUND_STRING("The blessed life returns\nto XERNEAS!"),
    [STRINGID_ENCXERNEASBLESSINGLEAVES]     = COMPOUND_STRING("The LIFE BLESSING is left\nbehind."),
    [STRINGID_ENCXERNEASGEOMANCYBEGINS]     = COMPOUND_STRING("XERNEAS is absorbing\nthe life around it!"),
    [STRINGID_ENCXERNEASGEOMANCYCUE]        = COMPOUND_STRING("The battlefield glows brighter!"),
    [STRINGID_ENCXERNEASGEOMANCYFIRES]      = COMPOUND_STRING("GEOMANCY! Life erupts from\nthe ground!"),
    [STRINGID_ENCXERNEASGEOMANCYBREAKS]     = COMPOUND_STRING("XERNEAS loses its hold on\nthe gathered life!"),
    [STRINGID_ENCXERNEASACTIVEMODE]         = COMPOUND_STRING("XERNEAS blazes with colour!\nACTIVE MODE!"),
    [STRINGID_ENCXERNEASFORESTAWAKENS]      = COMPOUND_STRING("The forest awakens!\nLife surges through the field!"),
    [STRINGID_ENCXERNEASVERDANT]            = COMPOUND_STRING("VERDANT STATE!\nThe garden grows faster now."),
    [STRINGID_ENCXERNEASLIFEETERNAL]        = COMPOUND_STRING("ETERNAL LIFE!\nXERNEAS cannot be brought down."),
    [STRINGID_ENCXERNEASNOTHINGDIES]        = COMPOUND_STRING("Nothing dies in this garden.\nNot even your POKéMON."),
    [STRINGID_ENCXERNEASHOLDS]              = COMPOUND_STRING("XERNEAS spends its life\nto stay standing!"),
    [STRINGID_ENCXERNEASPRESSURE]           = COMPOUND_STRING("XERNEAS spends life to\nmend the wound."),
    [STRINGID_ENCXERNEASCUEA]               = COMPOUND_STRING("The garden is still growing."),
    [STRINGID_ENCXERNEASCUEB]               = COMPOUND_STRING("Life gathers around XERNEAS."),
    [STRINGID_ENCXERNEASSTARVING]           = COMPOUND_STRING("The light around XERNEAS\nis thinning!"),
    [STRINGID_ENCXERNEASNATURESGIFT]        = COMPOUND_STRING("NATURE'S GIFT!\nXERNEAS gives it all away!"),
    [STRINGID_ENCXERNEASGIFTHEALS]          = COMPOUND_STRING("Your team is bathed in light!"),
    [STRINGID_ENCXERNEASSILENT]             = COMPOUND_STRING("The forest falls silent."),
    [STRINGID_ENCXERNEASWEAKENED]           = COMPOUND_STRING("XERNEAS's light is spent.\nNow is the moment!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCXERNEASGUARDDROPS]         = COMPOUND_STRING("The garden's guard drops!\nLife can be lost again."),
    [STRINGID_ENCXERNEASLIFERETURNS]        = COMPOUND_STRING("Nothing dies in this garden\nagain, for now."),
    [STRINGID_ENCYVELTALINTRO1]             = COMPOUND_STRING("A crushing aura descends -"),
    [STRINGID_ENCYVELTALINTRO2]             = COMPOUND_STRING("YVELTAL awakens to destroy!"),
    [STRINGID_ENCYVELTALMARK]               = COMPOUND_STRING("YVELTAL marks\n{B_PLAYER_MON1_NAME} for doom!"),
    [STRINGID_ENCYVELTALDOOMED]             = COMPOUND_STRING("MARK DETONATES!\nYVELTAL claims its due."),
    [STRINGID_ENCYVELTALPULSE]              = COMPOUND_STRING("DESTRUCTION PULSE!\nYour barriers are gone."),
    [STRINGID_ENCYVELTALLIFEDRAIN]          = COMPOUND_STRING("LIFE DRAIN siphons\nyour strength away!"),
    [STRINGID_ENCYVELTALPHASE1]             = COMPOUND_STRING("THE LIGHT BEGINS\nTO FADE."),
    [STRINGID_ENCYVELTALBANDLOW]            = COMPOUND_STRING("YVELTAL's aura is\nquiet, for now."),
    [STRINGID_ENCYVELTALBANDMED]            = COMPOUND_STRING("Destructive energy\nchurns within it!"),
    [STRINGID_ENCYVELTALBANDHIGH]           = COMPOUND_STRING("Its aura roars with\nutter destruction!"),
    [STRINGID_ENCYVELTALCUE]                = COMPOUND_STRING("Destruction stirs."),
    [STRINGID_ENCYVELTALWINGBEGIN]          = COMPOUND_STRING("YVELTAL's wings\nglow with dark power!"),
    [STRINGID_ENCYVELTALWINGWARN]           = COMPOUND_STRING("OBLIVION WING is\nabout to fall!"),
    [STRINGID_ENCYVELTALWINGHIT]            = COMPOUND_STRING("OBLIVION WING tears\ndown - and heals it!"),
    [STRINGID_ENCYVELTALPHASE2]             = COMPOUND_STRING("DESTRUCTION\nINCARNATE"),
    [STRINGID_ENCYVELTALINCARNATEBEGIN]     = COMPOUND_STRING("It gathers all its\nremaining power!"),
    [STRINGID_ENCYVELTALINCARNATEWARN]      = COMPOUND_STRING("THE DESTRUCTION OF ALL\ndraws nearer!"),
    [STRINGID_ENCYVELTALINCARNATEHIT]       = COMPOUND_STRING("THE DESTRUCTION OF ALL\nis unleashed!"),
    [STRINGID_ENCYVELTALINTERRUPTED]        = COMPOUND_STRING("Your assault breaks\nits concentration!"),
    [STRINGID_ENCYVELTALFADE]               = COMPOUND_STRING("The destruction\nfades from it."),
    [STRINGID_ENCYVELTALWEAKENED]           = COMPOUND_STRING("Now is the moment -\nthrow the Ball!{PAUSE_UNTIL_PRESS}"),
    [STRINGID_ENCNECROZMAFRAGMENTSPAWN]     = COMPOUND_STRING("A shard of light breaks\nfree! Hit it before it fades!"),
    [STRINGID_ENCNECROZMAFRAGMENTCLAIMED]   = COMPOUND_STRING("You claimed the light\nshard! Necrozma reels!"),
    [STRINGID_ENCNECROZMAFRAGMENTABSORBED]  = COMPOUND_STRING("Necrozma devours the\ndrifting shard of light!"),
    [STRINGID_ENCNECROZMAFEEDING]           = COMPOUND_STRING("Necrozma's light grows\nbrighter by the moment!"),
    [STRINGID_ENCNECROZMASATED]             = COMPOUND_STRING("Sated, Necrozma's eyes\ngleam with lethal focus!"),
    [STRINGID_ENCNECROZMAOVERCHARGED]       = COMPOUND_STRING("Overcharged with light,\nNecrozma's power surges!"),
    [STRINGID_ENCNECROZMAFUSION]            = COMPOUND_STRING("Necrozma's prism form\nbegins to fuse and shift!"),
    [STRINGID_ENCNECROZMADUSKMANE]          = COMPOUND_STRING("Necrozma fuses with\nsteel! Dusk Mane awakens!"),
    [STRINGID_ENCNECROZMADAWNWINGS]         = COMPOUND_STRING("Necrozma fuses with\nshadow! Dawn Wings stirs!"),
    [STRINGID_ENCNECROZMAARMORBREAK]        = COMPOUND_STRING("A plate of solar armor\nshatters off Necrozma!"),
    [STRINGID_ENCNECROZMAECLIPSE]           = COMPOUND_STRING("The eclipse deepens,\nswallowing the light!"),
    [STRINGID_ENCNECROZMAULTRA]             = COMPOUND_STRING("Necrozma unleashes its\nfull power - ULTRA NECROZMA!"),
    [STRINGID_ENCNECROZMAGEYSER]            = COMPOUND_STRING("Light spikes within it -\nbrace for Photon Geyser!"),
    [STRINGID_ENCNECROZMACATCH]             = COMPOUND_STRING("Necrozma's light is\nfading - now is the moment!{PAUSE_UNTIL_PRESS}"),

    [STRINGID_ENCZERAORAOVERDRIVE]          = COMPOUND_STRING("Zeraora's fur crackles with\nstatic - it shifts gears!"),
    [STRINGID_ENCZERAORAMAXVELOCITY]        = COMPOUND_STRING("Static roars off Zeraora -\nit's about to outrun sight!"),
    [STRINGID_ENCZERAORANETWORKCOMPLETE]    = COMPOUND_STRING("The scattered arcs snap\ntogether - the Network locks in!"),
    [STRINGID_ENCZERAORATHUNDERCLAP]        = COMPOUND_STRING("Zeraora vanishes in a flash -\nit's already gone!"),
    [STRINGID_ENCZERAORAPLASMAARM]          = COMPOUND_STRING("Electricity floods outward,\npooling at Zeraora's fists!"),
    [STRINGID_ENCZERAORAPLASMARELEASE]      = COMPOUND_STRING("The charge discharges at\nonce - Zeraora looks spent!"),
    [STRINGID_ENCZERAORASPEEDCHAIN]         = COMPOUND_STRING("Each strike feeds the last -\nZeraora is picking up speed!"),
    [STRINGID_ENCZERAORACATCH]              = COMPOUND_STRING("Zeraora's speed gives out -\nnow is the moment!{PAUSE_UNTIL_PRESS}"),
};

const u16 gOneHitKOStringIds[] =
{
    STRINGID_ONEHITKO,
    STRINGID_ONEHITKO_2,
    STRINGID_ONEHITKO_3,
};

const u16 gTrainerUsedItemStringIds[] =
{
    STRINGID_PLAYERUSEDITEM, STRINGID_TRAINER1USEDITEM
};

const u16 gZEffectStringIds[] =
{
    [B_MSG_Z_RESET_STATS] = STRINGID_ZMOVERESETSSTATS,
    [B_MSG_Z_ALL_STATS_UP]= STRINGID_ZMOVEALLSTATSUP,
    [B_MSG_Z_BOOST_CRITS] = STRINGID_ZMOVEZBOOSTCRIT,
    [B_MSG_Z_FOLLOW_ME]   = STRINGID_PKMNCENTERATTENTION,
    [B_MSG_Z_RECOVER_HP]  = STRINGID_ZMOVERESTOREHP,
    [B_MSG_Z_STAT_UP]     = STRINGID_ZMOVESTATUP,
    [B_MSG_Z_HP_TRAP]     = STRINGID_ZMOVEHPTRAP,
};

const u16 gMentalHerbCureStringIds[] =
{
    [B_MSG_MENTALHERBCURE_INFATUATION] = STRINGID_ATKGOTOVERINFATUATION,
    [B_MSG_MENTALHERBCURE_TORMENT]     = STRINGID_TORMENTEDNOMORE,
    [B_MSG_MENTALHERBCURE_DISABLE]     = STRINGID_PKMNMOVEDISABLEDNOMORE,
    [B_MSG_MENTALHERBCURE_HEALBLOCK]   = STRINGID_HEALBLOCKEDNOMORE,
    [B_MSG_MENTALHERBCURE_ENCORE]      = STRINGID_PKMNENCOREENDED,
    [B_MSG_MENTALHERBCURE_TAUNT]       = STRINGID_PKMNSHOOKOFFTHETAUNT,
};

const u16 gStartingStatusStringIds[B_MSG_STARTING_STATUS_COUNT] =
{
    [B_MSG_TERRAIN_SET_MISTY]    = STRINGID_TERRAINBECOMESMISTY,
    [B_MSG_TERRAIN_SET_ELECTRIC] = STRINGID_TERRAINBECOMESELECTRIC,
    [B_MSG_TERRAIN_SET_PSYCHIC]  = STRINGID_TERRAINBECOMESPSYCHIC,
    [B_MSG_TERRAIN_SET_GRASSY]   = STRINGID_TERRAINBECOMESGRASSY,
    [B_MSG_SET_TRICK_ROOM]       = STRINGID_DIMENSIONSWERETWISTED,
    [B_MSG_SET_MAGIC_ROOM]       = STRINGID_BIZARREARENACREATED,
    [B_MSG_SET_WONDER_ROOM]      = STRINGID_BIZARREAREACREATED,
    [B_MSG_SET_TAILWIND]         = STRINGID_TAILWINDBLEW,
    [B_MSG_SET_RAINBOW]          = STRINGID_ARAINBOWAPPEAREDONSIDE,
    [B_MSG_SET_SEA_OF_FIRE]      = STRINGID_SEAOFFIREENVELOPEDSIDE,
    [B_MSG_SET_SWAMP]            = STRINGID_SWAMPENVELOPEDSIDE,
    [B_MSG_SET_SPIKES]           = STRINGID_SPIKESSCATTERED,
    [B_MSG_SET_POISON_SPIKES]    = STRINGID_POISONSPIKESSCATTERED,
    [B_MSG_SET_STICKY_WEB]       = STRINGID_STICKYWEBUSED,
    [B_MSG_SET_STEALTH_ROCK]     = STRINGID_POINTEDSTONESFLOAT,
    [B_MSG_SET_SHARP_STEEL]      = STRINGID_SHARPSTEELFLOATS,
};

const u16 gTerrainStringIds[B_MSG_TERRAIN_COUNT] =
{
    [B_MSG_TERRAIN_SET_MISTY] = STRINGID_TERRAINBECOMESMISTY,
    [B_MSG_TERRAIN_SET_ELECTRIC] = STRINGID_TERRAINBECOMESELECTRIC,
    [B_MSG_TERRAIN_SET_PSYCHIC] = STRINGID_TERRAINBECOMESPSYCHIC,
    [B_MSG_TERRAIN_SET_GRASSY] = STRINGID_TERRAINBECOMESGRASSY,
    [B_MSG_TERRAIN_END_MISTY] = STRINGID_MISTYTERRAINENDS,
    [B_MSG_TERRAIN_END_ELECTRIC] = STRINGID_ELECTRICTERRAINENDS,
    [B_MSG_TERRAIN_END_PSYCHIC] = STRINGID_PSYCHICTERRAINENDS,
    [B_MSG_TERRAIN_END_GRASSY] = STRINGID_GRASSYTERRAINENDS,
};

const u16 gTerrainPreventsStringIds[] =
{
    [B_MSG_TERRAINPREVENTS_MISTY]    = STRINGID_MISTYTERRAINPREVENTS,
    [B_MSG_TERRAINPREVENTS_ELECTRIC] = STRINGID_ELECTRICTERRAINPREVENTS,
    [B_MSG_TERRAINPREVENTS_PSYCHIC]  = STRINGID_PSYCHICTERRAINPREVENTS
};

const u16 gHealingWishStringIds[] =
{
    STRINGID_HEALINGWISHCAMETRUE,
    STRINGID_LUNARDANCECAMETRUE
};

const u16 gDmgHazardsStringIds[] =
{
    [B_MSG_PKMNHURTBYSPIKES]   = STRINGID_PKMNHURTBYSPIKES,
    [B_MSG_STEALTHROCKDMG]     = STRINGID_STEALTHROCKDMG,
    [B_MSG_SHARPSTEELDMG]      = STRINGID_SHARPSTEELDMG,
    [B_MSG_POINTEDSTONESFLOAT] = STRINGID_POINTEDSTONESFLOAT,
    [B_MSG_SPIKESSCATTERED]    = STRINGID_SPIKESSCATTERED,
    [B_MSG_SHARPSTEELFLOATS]   = STRINGID_SHARPSTEELFLOATS,
};

const u16 gSwitchInAbilityStringIds[] =
{
    [B_MSG_SWITCHIN_MOLDBREAKER] = STRINGID_MOLDBREAKERENTERS,
    [B_MSG_SWITCHIN_TERAVOLT] = STRINGID_TERAVOLTENTERS,
    [B_MSG_SWITCHIN_TURBOBLAZE] = STRINGID_TURBOBLAZEENTERS,
    [B_MSG_SWITCHIN_SLOWSTART] = STRINGID_SLOWSTARTENTERS,
    [B_MSG_SWITCHIN_UNNERVE] = STRINGID_UNNERVEENTERS,
    [B_MSG_SWITCHIN_ANTICIPATION] = STRINGID_ANTICIPATIONACTIVATES,
    [B_MSG_SWITCHIN_FOREWARN] = STRINGID_FOREWARNACTIVATES,
    [B_MSG_SWITCHIN_PRESSURE] = STRINGID_PRESSUREENTERS,
    [B_MSG_SWITCHIN_DARKAURA] = STRINGID_DARKAURAENTERS,
    [B_MSG_SWITCHIN_FAIRYAURA] = STRINGID_FAIRYAURAENTERS,
    [B_MSG_SWITCHIN_AURABREAK] = STRINGID_AURABREAKENTERS,
    [B_MSG_SWITCHIN_COMATOSE] = STRINGID_COMATOSEENTERS,
    [B_MSG_SWITCHIN_SCREENCLEANER] = STRINGID_SCREENCLEANERENTERS,
    [B_MSG_SWITCHIN_ASONE] = STRINGID_ASONEENTERS,
    [B_MSG_SWITCHIN_CURIOUS_MEDICINE] = STRINGID_CURIOUSMEDICINEENTERS,
    [B_MSG_SWITCHIN_PASTEL_VEIL] = STRINGID_PKMNHEALEDPOISON,
    [B_MSG_SWITCHIN_NEUTRALIZING_GAS] = STRINGID_NEUTRALIZINGGASENTERS,
};

const u16 gNoEscapeStringIds[] =
{
    [B_MSG_CANT_ESCAPE]          = STRINGID_CANTESCAPE,
    [B_MSG_DONT_LEAVE_BIRCH]     = STRINGID_DONTLEAVEBIRCH,
    [B_MSG_PREVENTS_ESCAPE]      = STRINGID_PREVENTSESCAPE,
    [B_MSG_CANT_ESCAPE_2]        = STRINGID_CANTESCAPE2,
    [B_MSG_ATTACKER_CANT_ESCAPE] = STRINGID_ATTACKERCANTESCAPE
};

const u16 gMoveWeatherChangeStringIds[] =
{
    [B_MSG_STARTED_RAIN]      = STRINGID_STARTEDTORAIN,
    [B_MSG_STARTED_DOWNPOUR]  = STRINGID_DOWNPOURSTARTED, // Unused
    [B_MSG_WEATHER_FAILED]    = STRINGID_BUTITFAILED,
    [B_MSG_STARTED_SANDSTORM] = STRINGID_SANDSTORMBREWED,
    [B_MSG_STARTED_SUNLIGHT]  = STRINGID_SUNLIGHTGOTBRIGHT,
    [B_MSG_STARTED_HAIL]      = STRINGID_STARTEDHAIL,
    [B_MSG_STARTED_SNOW]      = STRINGID_STARTEDSNOW,
    [B_MSG_STARTED_FOG]       = STRINGID_FOGCREPTUP, // Unused, can use for custom moves that set fog
};

const u16 gAbilityWeatherChangeStringId[] =
{
    [B_MSG_STARTED_DRIZZLE]        = STRINGID_STARTEDTORAIN,
    [B_MSG_STARTED_SAND_STREAM]    = STRINGID_SANDSTORMBREWED,
    [B_MSG_STARTED_DROUGHT]        = STRINGID_SUNLIGHTGOTBRIGHT,
    [B_MSG_STARTED_HAIL_WARNING]   = STRINGID_STARTEDHAIL,
    [B_MSG_STARTED_SNOW_WARNING]   = STRINGID_STARTEDSNOW,
    [B_MSG_STARTED_DESOLATE_LAND]  = STRINGID_EXTREMELYHARSHSUNLIGHT,
    [B_MSG_STARTED_PRIMORDIAL_SEA] = STRINGID_HEAVYRAIN,
    [B_MSG_STARTED_STRONG_WINDS]   = STRINGID_MYSTERIOUSAIRCURRENT,
};

const u16 gWeatherEndsStringIds[B_MSG_WEATHER_END_COUNT] =
{
    [B_MSG_WEATHER_END_RAIN]                       = STRINGID_RAINSTOPPED,
    [B_MSG_WEATHER_END_SUN]                        = STRINGID_SUNLIGHTFADED,
    [B_MSG_WEATHER_END_SANDSTORM]                  = STRINGID_SANDSTORMSUBSIDED,
    [B_MSG_WEATHER_END_HAIL]                       = STRINGID_HAILSTOPPED,
    [B_MSG_WEATHER_END_SNOW]                       = STRINGID_SNOWSTOPPED,
    [B_MSG_WEATHER_END_FOG]                        = STRINGID_FOGLIFTED,
    [B_MSG_WEATHER_END_EXTREMELY_HARSH_SUNLIGHT]   = STRINGID_EXTREMESUNLIGHTFADED,
    [B_MSG_WEATHER_END_HEAVY_RAIN]                 = STRINGID_HEAVYRAINLIFTED,
    [B_MSG_WEATHER_END_STRONG_WINDS]               = STRINGID_STRONGWINDSDISSIPATED,
};

const u16 gWeatherTurnStringIds[] =
{
    [B_MSG_WEATHER_TURN_RAIN]         = STRINGID_RAINCONTINUES,
    [B_MSG_WEATHER_TURN_DOWNPOUR]     = STRINGID_DOWNPOURCONTINUES,
    [B_MSG_WEATHER_TURN_SUN]          = STRINGID_SUNLIGHTSTRONG,
    [B_MSG_WEATHER_TURN_SANDSTORM]    = STRINGID_SANDSTORMRAGES,
    [B_MSG_WEATHER_TURN_HAIL]         = STRINGID_HAILCONTINUES,
    [B_MSG_WEATHER_TURN_SNOW]         = STRINGID_SNOWCONTINUES,
    [B_MSG_WEATHER_TURN_FOG]          = STRINGID_FOGISDEEP,
    [B_MSG_WEATHER_TURN_STRONG_WINDS] = STRINGID_MYSTERIOUSAIRCURRENTBLOWSON,
};

const u16 gSandStormHailDmgStringIds[] =
{
    [B_MSG_SANDSTORM] = STRINGID_PKMNBUFFETEDBYSANDSTORM,
    [B_MSG_HAIL]      = STRINGID_PKMNPELTEDBYHAIL
};

const u16 gProtectLikeUsedStringIds[] =
{
    [B_MSG_PROTECTED_ITSELF] = STRINGID_PKMNPROTECTEDITSELF2,
    [B_MSG_BRACED_ITSELF]    = STRINGID_PKMNBRACEDITSELF,
    [B_MSG_PROTECTED_TEAM]   = STRINGID_PROTECTEDTEAM,
};

const u16 gBrokeProtectionStringIds[] =
{
    [B_MSG_FEINT]                 = STRINGID_FELLFORFEINT,
    [B_MSG_BROKE_THROUGH_PROTECT] = STRINGID_BROKETHROUGHPROTECTION,
};

const u16 gReflectLightScreenSafeguardStringIds[] =
{
    [B_MSG_SIDE_STATUS_FAILED]     = STRINGID_BUTITFAILED,
    [B_MSG_SET_REFLECT_SINGLE]     = STRINGID_PKMNRAISEDDEF,
    [B_MSG_SET_REFLECT_DOUBLE]     = STRINGID_PKMNRAISEDDEF,
    [B_MSG_SET_LIGHTSCREEN_SINGLE] = STRINGID_PKMNRAISEDSPDEF,
    [B_MSG_SET_LIGHTSCREEN_DOUBLE] = STRINGID_PKMNRAISEDSPDEF,
    [B_MSG_SET_SAFEGUARD]          = STRINGID_PKMNCOVEREDBYVEIL,
    [B_MSG_SET_AURORA_VEIL]        = STRINGID_PKMNAURORAVEIL,
};

const u16 gLeechSeedStringIds[] =
{
    [B_MSG_LEECH_SEED_SET]   = STRINGID_PKMNSEEDED,
    [B_MSG_LEECH_SEED_MISS]  = STRINGID_PKMNAVOIDEDATTACK,
    [B_MSG_LEECH_SEED_FAIL]  = STRINGID_ITDOESNTAFFECT,
    [B_MSG_LEECH_SEED_DRAIN] = STRINGID_PKMNSAPPEDBYLEECHSEED,
    [B_MSG_LEECH_SEED_OOZE]  = STRINGID_ITSUCKEDLIQUIDOOZE,
};

const u16 gRestUsedStringIds[] =
{
    [B_MSG_REST]          = STRINGID_PKMNWENTTOSLEEP,
    [B_MSG_REST_STATUSED] = STRINGID_PKMNSLEPTHEALTHY
};

const u16 gUproarOverTurnStringIds[] =
{
    [B_MSG_UPROAR_CONTINUES] = STRINGID_PKMNMAKINGUPROAR,
    [B_MSG_UPROAR_ENDS]      = STRINGID_PKMNCALMEDDOWN
};

const u16 gWokeUpStringIds[] =
{
    [B_MSG_WOKE_UP]        = STRINGID_PKMNWOKEUP,
    [B_MSG_WOKE_UP_UPROAR] = STRINGID_PKMNWOKEUPINUPROAR
};

const u16 gUproarAwakeStringIds[] =
{
    [B_MSG_CANT_SLEEP_UPROAR]  = STRINGID_PKMNCANTSLEEPINUPROAR2,
    [B_MSG_UPROAR_KEPT_AWAKE]  = STRINGID_UPROARKEPTPKMNAWAKE,
};

const u16 gStatUpStringIds[] =
{
    [B_MSG_STAT_CHANGED]            = STRINGID_STATROSE,
    [B_MSG_STAT_WONT_CHANGE]        = STRINGID_STATSWONTINCREASE,
    [B_MSG_STAT_MAXED]              = STRINGID_STATWASMAXEDOUT,
    [B_MSG_STAT_CHANGE_EMPTY]       = STRINGID_EMPTYSTRING3,
    [B_MSG_STAT_CHANGED_ITEM]       = STRINGID_USINGITEMSTATOFPKMNROSE,
    [B_MSG_STAT_CHANGED_BELLY_DRUM] = STRINGID_PKMNCUTHPMAXEDATTACK,
    [B_MSG_USED_DIRE_HIT]           = STRINGID_PKMNUSEDXTOGETPUMPED,
};

// Mostly redundant, combine with above in a future pr
const u16 gStatDownStringIds[] =
{
    [B_MSG_STAT_CHANGED]            = STRINGID_STATFELL,
    [B_MSG_STAT_WONT_CHANGE]        = STRINGID_STATSWONTDECREASE,
    [B_MSG_STAT_CHANGE_EMPTY]       = STRINGID_EMPTYSTRING3,
    [B_MSG_STAT_CHANGED_ITEM]       = STRINGID_USINGITEMSTATOFPKMNFELL,
    [B_MSG_STAT_CHANGED_BELLY_DRUM] = STRINGID_PKMNCUTHPMAXEDATTACK, // Message for contrary is still printed
};

// Index copied from move's index in sTrappingMoves
const u16 gWrappedStringIds[NUM_TRAPPING_MOVES] =
{
    [B_MSG_WRAPPED_BIND]        = STRINGID_PKMNSQUEEZEDBYBIND,     // MOVE_BIND
    [B_MSG_WRAPPED_WRAP]        = STRINGID_PKMNWRAPPEDBY,          // MOVE_WRAP
    [B_MSG_WRAPPED_FIRE_SPIN]   = STRINGID_PKMNTRAPPEDINVORTEX,    // MOVE_FIRE_SPIN
    [B_MSG_WRAPPED_CLAMP]       = STRINGID_PKMNCLAMPED,            // MOVE_CLAMP
    [B_MSG_WRAPPED_WHIRLPOOL]   = STRINGID_PKMNTRAPPEDINVORTEX,    // MOVE_WHIRLPOOL
    [B_MSG_WRAPPED_SAND_TOMB]   = STRINGID_PKMNTRAPPEDBYSANDTOMB,  // MOVE_SAND_TOMB
    [B_MSG_WRAPPED_MAGMA_STORM] = STRINGID_TRAPPEDBYSWIRLINGMAGMA, // MOVE_MAGMA_STORM
    [B_MSG_WRAPPED_INFESTATION] = STRINGID_INFESTATION,            // MOVE_INFESTATION
    [B_MSG_WRAPPED_SNAP_TRAP]   = STRINGID_PKMNINSNAPTRAP,         // MOVE_SNAP_TRAP
    [B_MSG_WRAPPED_THUNDER_CAGE]= STRINGID_THUNDERCAGETRAPPED,     // MOVE_THUNDER_CAGE
};

const u16 gMistUsedStringIds[] =
{
    [B_MSG_SET_MIST]    = STRINGID_PKMNSHROUDEDINMIST,
    [B_MSG_MIST_FAILED] = STRINGID_BUTITFAILED
};

const u16 gFocusEnergyUsedStringIds[] =
{
    [B_MSG_GETTING_PUMPED]      = STRINGID_PKMNGETTINGPUMPED,
    [B_MSG_FOCUS_ENERGY_FAILED] = STRINGID_BUTITFAILED
};

const u16 gTransformUsedStringIds[] =
{
    [B_MSG_TRANSFORMED]      = STRINGID_PKMNTRANSFORMEDINTO,
    [B_MSG_TRANSFORM_FAILED] = STRINGID_BUTITFAILED
};

const u16 gSubstituteUsedStringIds[] =
{
    [B_MSG_SET_SUBSTITUTE]    = STRINGID_PKMNMADESUBSTITUTE,
    [B_MSG_SUBSTITUTE_FAILED] = STRINGID_TOOWEAKFORSUBSTITUTE
};

const u16 gGotPoisonedStringIds[] =
{
    [B_MSG_STATUSED]            = STRINGID_PKMNWASPOISONED,
    [B_MSG_STATUSED_BY_ABILITY] = STRINGID_PKMNPOISONEDBY
};

const u16 gGotParalyzedStringIds[] =
{
    [B_MSG_STATUSED]            = STRINGID_PKMNWASPARALYZED,
    [B_MSG_STATUSED_BY_ABILITY] = STRINGID_PKMNWASPARALYZEDBY
};

const u16 gFellAsleepStringIds[] =
{
    [B_MSG_STATUSED]            = STRINGID_PKMNFELLASLEEP,
    [B_MSG_STATUSED_BY_ABILITY] = STRINGID_PKMNMADESLEEP,
};

const u16 gGotBurnedStringIds[] =
{
    [B_MSG_STATUSED]            = STRINGID_PKMNWASBURNED,
    [B_MSG_STATUSED_BY_ABILITY] = STRINGID_PKMNBURNEDBY
};

const u16 gGotFrostbiteStringIds[] =
{
    [B_MSG_STATUSED]            = STRINGID_PKMNGOTFROSTBITE,
    [B_MSG_STATUSED_BY_ABILITY] = STRINGID_PKMNGOTFROSTBITE,
};

const u16 gFrostbiteHealedStringIds[] =
{
    [B_MSG_FROSTBITE_HEALED]         = STRINGID_PKMNFROSTBITEHEALED,
    [B_MSG_FROSTBITE_HEALED_BY_MOVE] = STRINGID_PKMNFROSTBITEHEALEDBY
};

const u16 gGotFrozenStringIds[] =
{
    [B_MSG_STATUSED]            = STRINGID_PKMNWASFROZEN,
    [B_MSG_STATUSED_BY_ABILITY] = STRINGID_PKMNFROZENBY
};

const u16 gGotDefrostedStringIds[] =
{
    [B_MSG_DEFROSTED]         = STRINGID_PKMNWASDEFROSTED,
    [B_MSG_DEFROSTED_BY_MOVE] = STRINGID_PKMNWASDEFROSTEDBY
};

const u16 gKOFailedStringIds[] =
{
    [B_MSG_KO_MISS]       = STRINGID_PKMNAVOIDEDATTACK,
    [B_MSG_KO_UNAFFECTED] = STRINGID_PKMNUNAFFECTED
};

const u16 gAttractUsedStringIds[] =
{
    [B_MSG_STATUSED]            = STRINGID_PKMNFELLINLOVE,
    [B_MSG_STATUSED_BY_ABILITY] = STRINGID_PKMNSXINFATUATEDY
};

const u16 gAbsorbDrainStringIds[] =
{
    [B_MSG_ABSORB]      = STRINGID_PKMNENERGYDRAINED,
    [B_MSG_ABSORB_OOZE] = STRINGID_ITSUCKEDLIQUIDOOZE
};

const u16 gSportsUsedStringIds[] =
{
    [B_MSG_WEAKEN_ELECTRIC] = STRINGID_ELECTRICITYWEAKENED,
    [B_MSG_WEAKEN_FIRE]     = STRINGID_FIREWEAKENED
};

const u16 gPartyStatusHealStringIds[] =
{
    [B_MSG_BELL]                     = STRINGID_BELLCHIMED,
    [B_MSG_BELL_SOUNDPROOF_ATTACKER] = STRINGID_BELLCHIMED,
    [B_MSG_BELL_SOUNDPROOF_PARTNER]  = STRINGID_BELLCHIMED,
    [B_MSG_BELL_BOTH_SOUNDPROOF]     = STRINGID_BELLCHIMED,
    [B_MSG_SOOTHING_AROMA]           = STRINGID_SOOTHINGAROMA
};

const u16 gFutureMoveUsedStringIds[] =
{
    [B_MSG_FUTURE_SIGHT] = STRINGID_PKMNFORESAWATTACK,
    [B_MSG_DOOM_DESIRE]  = STRINGID_PKMNCHOSEXASDESTINY
};

const u16 gBallEscapeStringIds[] =
{
    [BALL_NO_SHAKES]     = STRINGID_PKMNBROKEFREE,
    [BALL_1_SHAKE]       = STRINGID_ITAPPEAREDCAUGHT,
    [BALL_2_SHAKES]      = STRINGID_AARGHALMOSTHADIT,
    [BALL_3_SHAKES_FAIL] = STRINGID_SHOOTSOCLOSE
};

// Overworld weathers that don't have an associated battle weather default to "It is raining."
const u16 gWeatherStartsStringIds[] =
{
    [WEATHER_NONE]               = STRINGID_ITISRAINING,
    [WEATHER_SUNNY_CLOUDS]       = STRINGID_ITISRAINING,
    [WEATHER_SUNNY]              = STRINGID_ITISRAINING,
    [WEATHER_RAIN]               = STRINGID_ITISRAINING,
    [WEATHER_SNOW]               = (B_OVERWORLD_SNOW >= GEN_9 ? STRINGID_ITISSNOWING : STRINGID_ITISHAILING),
    [WEATHER_RAIN_THUNDERSTORM]  = STRINGID_ITISRAINING,
    [WEATHER_FOG_HORIZONTAL]     = STRINGID_FOGISDEEP,
    [WEATHER_VOLCANIC_ASH]       = STRINGID_ITISRAINING,
    [WEATHER_SANDSTORM]          = STRINGID_SANDSTORMISRAGING,
    [WEATHER_FOG_DIAGONAL]       = STRINGID_FOGISDEEP,
    [WEATHER_UNDERWATER]         = STRINGID_ITISRAINING,
    [WEATHER_SHADE]              = STRINGID_ITISRAINING,
    [WEATHER_DROUGHT]            = STRINGID_SUNLIGHTISHARSH,
    [WEATHER_DOWNPOUR]           = STRINGID_ITISRAINING,
    [WEATHER_UNDERWATER_BUBBLES] = STRINGID_ITISRAINING,
    [WEATHER_ABNORMAL]           = STRINGID_ITISRAINING
};

const u16 gTerrainStartsStringIds[] =
{
    [B_MSG_TERRAIN_SET_MISTY]    = STRINGID_MISTSWIRLSAROUND,
    [B_MSG_TERRAIN_SET_ELECTRIC] = STRINGID_ELECTRICCURRENTISRUNNING,
    [B_MSG_TERRAIN_SET_PSYCHIC]  = STRINGID_SEEMSWEIRD,
    [B_MSG_TERRAIN_SET_GRASSY]   = STRINGID_ISCOVEREDWITHGRASS,
};

const u16 gPrimalWeatherBlocksStringIds[] =
{
    [B_MSG_PRIMAL_WEATHER_FIZZLED_BY_RAIN]      = STRINGID_MOVEFIZZLEDOUTINTHEHEAVYRAIN,
    [B_MSG_PRIMAL_WEATHER_EVAPORATED_IN_SUN]    = STRINGID_MOVEEVAPORATEDINTHEHARSHSUNLIGHT,
};

const u16 gInobedientStringIds[] =
{
    [B_MSG_LOAFING]            = STRINGID_PKMNLOAFING,
    [B_MSG_WONT_OBEY]          = STRINGID_PKMNWONTOBEY,
    [B_MSG_TURNED_AWAY]        = STRINGID_PKMNTURNEDAWAY,
    [B_MSG_PRETEND_NOT_NOTICE] = STRINGID_PKMNPRETENDNOTNOTICE,
    [B_MSG_INCAPABLE_OF_POWER] = STRINGID_PKMNINCAPABLEOFPOWER
};

const u16 gSafariReactionStringIds[NUM_SAFARI_REACTIONS] =
{
    [B_MSG_MON_WATCHING] = STRINGID_PKMNWATCHINGCAREFULLY,
    [B_MSG_MON_ANGRY]    = STRINGID_PKMNANGRY,
    [B_MSG_MON_EATING]   = STRINGID_PKMNEATING
};

const u16 gSafariGetNearStringIds[] =
{
    [B_MSG_CREPT_CLOSER]    = STRINGID_CREPTCLOSER,
    [B_MSG_CANT_GET_CLOSER] = STRINGID_CANTGETCLOSER
};

const u16 gSafariPokeblockResultStringIds[] =
{
    [B_MSG_MON_CURIOUS]    = STRINGID_PKMNCURIOUSABOUTX,
    [B_MSG_MON_ENTHRALLED] = STRINGID_PKMNENTHRALLEDBYX,
    [B_MSG_MON_IGNORED]    = STRINGID_PKMNIGNOREDX
};

const u16 CureStatusBerryEffectStringID[] =
{
    [B_MSG_CURED_PARALYSIS] = STRINGID_PKMNSITEMCUREDPARALYSIS,
    [B_MSG_CURED_POISON] = STRINGID_PKMNSITEMCUREDPOISON,
    [B_MSG_CURED_BURN] = STRINGID_PKMNSITEMHEALEDBURN,
    [B_MSG_CURED_FREEZE] = STRINGID_PKMNSITEMDEFROSTEDIT,
    [B_MSG_CURED_FROSTBITE] = STRINGID_PKMNSITEMHEALEDFROSTBITE,
    [B_MSG_CURED_SLEEP] = STRINGID_PKMNSITEMWOKEIT,
    [B_MSG_CURED_CONFUSION] = STRINGID_PKMNSITEMSNAPPEDOUT,
};

const u16 gItemSwapStringIds[] =
{
    [B_MSG_ITEM_SWAP_TAKEN] = STRINGID_PKMNOBTAINEDX,
    [B_MSG_ITEM_SWAP_GIVEN] = STRINGID_PKMNOBTAINEDX2,
    [B_MSG_ITEM_SWAP_BOTH]  = STRINGID_PKMNOBTAINEDXYOBTAINEDZ
};

const u16 gFlashFireStringIds[] =
{
    [B_MSG_FLASH_FIRE_BOOST]    = STRINGID_PKMNRAISEDFIREPOWERWITH,
    [B_MSG_FLASH_FIRE_NO_BOOST] = STRINGID_PKMNSXMADEITINEFFECTIVE
};

const u16 gCaughtMonStringIds[] =
{
    [B_MSG_SENT_SOMEONES_PC]   = STRINGID_PKMNTRANSFERREDSOMEONESPC,
    [B_MSG_SENT_LANETTES_PC]   = STRINGID_PKMNTRANSFERREDLANETTESPC,
    [B_MSG_SOMEONES_BOX_FULL]  = STRINGID_PKMNBOXSOMEONESPCFULL,
    [B_MSG_LANETTES_BOX_FULL]  = STRINGID_PKMNBOXLANETTESPCFULL,
    [B_MSG_SWAPPED_INTO_PARTY] = STRINGID_PKMNSENTTOPCAFTERCATCH,
};

const u16 gRoomsStringIds[] =
{
    STRINGID_PKMNTWISTEDDIMENSIONS, STRINGID_TRICKROOMENDS,
    STRINGID_SWAPSDEFANDSPDEFOFALLPOKEMON, STRINGID_WONDERROOMENDS,
    STRINGID_HELDITEMSLOSEEFFECTS, STRINGID_MAGICROOMENDS,
    STRINGID_EMPTYSTRING3
};

const u16 gStatusConditionsStringIds[] =
{
    STRINGID_PKMNWASPOISONED, STRINGID_PKMNBADLYPOISONED, STRINGID_PKMNWASBURNED, STRINGID_PKMNWASPARALYZED, STRINGID_PKMNFELLASLEEP, STRINGID_PKMNGOTFROSTBITE
};

const u16 gDamageNonTypesStartStringIds[] =
{
    [B_MSG_TRAPPED_WITH_VINES]  = STRINGID_TEAMTRAPPEDWITHVINES,
    [B_MSG_CAUGHT_IN_VORTEX]    = STRINGID_TEAMCAUGHTINVORTEX,
    [B_MSG_SURROUNDED_BY_FIRE]  = STRINGID_TEAMSURROUNDEDBYFIRE,
    [B_MSG_SURROUNDED_BY_ROCKS] = STRINGID_TEAMSURROUNDEDBYROCKS,
};

const u16 gDamageNonTypesDmgStringIds[] =
{
    [B_MSG_HURT_BY_VINES]        = STRINGID_PKMNHURTBYVINES,
    [B_MSG_HURT_BY_VORTEX]       = STRINGID_PKMNHURTBYVORTEX,
    [B_MSG_BURNING_UP]           = STRINGID_PKMNBURNINGUP,
    [B_MSG_HURT_BY_ROCKS_THROWN] = STRINGID_PKMNHURTBYROCKSTHROWN,
};

const u16 gRemoveHazardsStringIds[] =
{
    [HAZARDS_SPIKES] = STRINGID_SPIKESDISAPPEAREDFROMTEAM,
    [HAZARDS_STICKY_WEB] = STRINGID_STICKYWEBDISAPPEAREDFROMTEAM,
    [HAZARDS_TOXIC_SPIKES] = STRINGID_TOXICSPIKESDISAPPEAREDFROMTEAM,
    [HAZARDS_STEALTH_ROCK] = STRINGID_STEALTHROCKDISAPPEAREDFROMTEAM,
    [HAZARDS_STEELSURGE] = STRINGID_SHARPSTEELDISAPPEAREDFROMTEAM,
};

const u16 gZenModeStringIds[] =
{
    [B_MSG_ZEN_MODE_TRIGGERED] = STRINGID_ZENMODETRIGGERED,
    [B_MSG_ZEN_MODE_ENDED] = STRINGID_ZENMODEENDED
};

const u16 gCureStatusStringIds[] =
{
    [B_MSG_CURED_PARALYSIS] = STRINGID_SCRCUREDPARALYSIS,
    [B_MSG_CURED_POISON] = STRINGID_SCRCUREDPOISON,
    [B_MSG_CURED_BURN] = STRINGID_SCRCUREDBURN,
    [B_MSG_CURED_SLEEP] = STRINGID_SCRCUREDSLEEP,
    [B_MSG_CURED_FREEZE] = STRINGID_PKMNWASDEFROSTED,
    [B_MSG_CURED_FROSTBITE] = STRINGID_PKMNFROSTBITEHEALED,
    [B_MSG_CURED_CONFUSION] = STRINGID_SCRCUREDCONFUSION,
    [B_MSG_CURED_INFATUATION] = STRINGID_PKMNGOTOVERITSINFATUATION,
    [B_MSG_CURED_TAUNT] = STRINGID_PKMNSHOOKOFFTHETAUNT,
};

const u16 gPartyCureStatusStringIds[] =
{
    [B_MSG_CURED_PARALYSIS] = STRINGID_PARTYCUREDPARALYSIS,
    [B_MSG_CURED_POISON] = STRINGID_PARTYCUREDPOISON,
    [B_MSG_CURED_BURN] = STRINGID_PARTYCUREDBURN,
    [B_MSG_CURED_SLEEP] = STRINGID_PARTYCUREDSLEEP,
    [B_MSG_CURED_FREEZE] = STRINGID_PARTYCUREDFREEZE,
    [B_MSG_CURED_FROSTBITE] = STRINGID_PARTYCUREDFROSTBITE,
    [B_MSG_CURED_CONFUSION] = STRINGID_SCRCUREDCONFUSION,
    [B_MSG_CURED_INFATUATION] = STRINGID_PKMNGOTOVERITSINFATUATION,
    [B_MSG_CURED_TAUNT] = STRINGID_PKMNSHOOKOFFTHETAUNT,
};

const u16 gHurtByStringIds[] =
{
    [B_MSG_HURT] = STRINGID_PKMNWASHURT,
    [B_MSG_HURT_BY_ITEM] = STRINGID_PKMNHURTSWITH,
};

const u16 gBreakScreensStringIds[] =
{
    [B_MSG_BREAK_REFLECT] = STRINGID_REFLECTWOREOFF,
    [B_MSG_BREAK_LIGHT_SCREEN] = STRINGID_LIGHTSCREENWOREOFF,
    [B_MSG_BREAK_AURORA_VEIL] = STRINGID_AURORAVEILWOREOFF,
};

const u8 gText_PkmnIsEvolving[] = _("What?\n{STR_VAR_1} is evolving!");
const u8 gText_CongratsPkmnEvolved[] = _("Congratulations! Your {STR_VAR_1}\nevolved into {STR_VAR_2}!{WAIT_SE}\p");
const u8 gText_PkmnStoppedEvolving[] = _("Huh? {STR_VAR_1}\nstopped evolving!\p");
const u8 gText_EllipsisQuestionMark[] = _("……?\p");
const u8 gText_WhatWillPkmnDo[] = _("What will\n{B_BUFF1} do?");
const u8 gText_WhatWillPkmnDo2[] = _("What will\n{B_PLAYER_NAME} do?");
const u8 gText_WhatWillWallyDo[] = _("What will\nWALLY do?");
const u8 gText_LinkStandby[] = _("{PAUSE 16}Link standby…");
const u8 gText_BattleMenu[] = _("Battle{CLEAR_TO 56}Bag\nPokémon{CLEAR_TO 56}Run");
const u8 gText_SafariZoneMenu[] = _("Ball{CLEAR_TO 56}{POKEBLOCK}\nGo Near{CLEAR_TO 56}Run");
const u8 gText_SafariZoneMenuFrlg[] = _("{PALETTE 5}{COLOR_HIGHLIGHT_SHADOW 13 14 15}BALL{CLEAR_TO 56}BAIT\nROCK{CLEAR_TO 56}RUN");
const u8 gText_MoveInterfacePP[] = _("PP ");
const u8 gText_MoveInterfaceType[] = _("TYPE/");
const u8 gText_MoveInterfacePPType[] = _("{PALETTE 5}{BACKGROUND DYNAMIC_COLOR5}{TEXT_COLORS DYNAMIC_COLOR4 DYNAMIC_COLOR6 DYNAMIC_COLOR5}PP\nTYPE/");
const u8 gText_MoveInterfaceDynamicColors[] = _("{PALETTE 5}{BACKGROUND DYNAMIC_COLOR5}{TEXT_COLORS DYNAMIC_COLOR4 DYNAMIC_COLOR6 DYNAMIC_COLOR5}");
const u8 gText_WhichMoveToForget4[] = _("{PALETTE 5}{BACKGROUND DYNAMIC_COLOR5}{TEXT_COLORS DYNAMIC_COLOR4 DYNAMIC_COLOR6 DYNAMIC_COLOR5}Which move should\nbe forgotten?");
const u8 gText_BattleCatchOrNot[] = _("{PALETTE 5}{BACKGROUND DYNAMIC_COLOR5}{TEXT_COLORS DYNAMIC_COLOR4 DYNAMIC_COLOR6 DYNAMIC_COLOR5}Catch\nDon't catch");
const u8 gText_BattleYesNoChoice[] = _("{PALETTE 5}{BACKGROUND DYNAMIC_COLOR5}{TEXT_COLORS DYNAMIC_COLOR4 DYNAMIC_COLOR6 DYNAMIC_COLOR5}Yes\nNo");
const u8 gText_BattleSwitchWhich[] = _("{PALETTE 5}{BACKGROUND DYNAMIC_COLOR5}{TEXT_COLORS DYNAMIC_COLOR4 DYNAMIC_COLOR6 DYNAMIC_COLOR5}Switch\nwhich?");
const u8 gText_BattleSwitchWhich2[] = _("{PALETTE 5}{BACKGROUND DYNAMIC_COLOR5}{TEXT_COLORS DYNAMIC_COLOR4 DYNAMIC_COLOR6 DYNAMIC_COLOR5}");
const u8 gText_BattleSwitchWhich3[] = _("{UP_ARROW}");
const u8 gText_BattleSwitchWhich4[] = _("{ESCAPE 4}");
const u8 gText_BattleSwitchWhich5[] = _("-");
const u8 gText_SafariBalls[] = _("Safari Balls");
const u8 gText_SafariBallLeft[] = _("Left: $");
const u8 gText_Sleep[] = _("sleep");
const u8 gText_Poison[] = _("poison");
const u8 gText_Burn[] = _("burn");
const u8 gText_Paralysis[] = _("paralysis");
const u8 gText_Ice[] = _("ice");
const u8 gText_Confusion[] = _("confusion");
const u8 gText_Love[] = _("love");
const u8 gText_SpaceAndSpace[] = _(" and ");
const u8 gText_CommaSpace[] = _(", ");
const u8 gText_Space2[] = _(" ");
const u8 gText_LineBreak[] = _("\l");
const u8 gText_NewLine[] = _("\n");
const u8 gText_Are[] = _("are");
const u8 gText_Are2[] = _("are");
const u8 gText_BadEgg[] = _("Bad Egg");
const u8 gText_BattleWallyName[] = _("WALLY");
const u8 gText_Win[] = _("{BACKGROUND TRANSPARENT}{ACCENT TRANSPARENT}Win");
const u8 gText_Loss[] = _("{BACKGROUND TRANSPARENT}{ACCENT TRANSPARENT}Loss");
const u8 gText_Draw[] = _("{BACKGROUND TRANSPARENT}{ACCENT TRANSPARENT}Draw");
static const u8 sText_SpaceIs[] = _(" is");
static const u8 sText_ApostropheS[] = _("'s");
const u8 gText_BattleTourney[] = _("BATTLE TOURNEY");

const u8 *const gRoundsStringTable[DOME_ROUNDS_COUNT] =
{
    [DOME_ROUND1]    = COMPOUND_STRING("Round 1"),
    [DOME_ROUND2]    = COMPOUND_STRING("Round 2"),
    [DOME_SEMIFINAL] = COMPOUND_STRING("Semifinal"),
    [DOME_FINAL]     = COMPOUND_STRING("Final"),
};

const u8 gText_TheGreatNewHope[] = _("The great new hope!\p");
const u8 gText_WillChampionshipDreamComeTrue[] = _("Will the championship dream come true?!\p");
const u8 gText_AFormerChampion[] = _("A former champion!\p");
const u8 gText_ThePreviousChampion[] = _("The previous champion!\p");
const u8 gText_TheUnbeatenChampion[] = _("The unbeaten champion!\p");
const u8 gText_PlayerMon1Name[] = _("{B_PLAYER_MON1_NAME}");
const u8 gText_Vs[] = _("VS");
const u8 gText_OpponentMon1Name[] = _("{B_OPPONENT_MON1_NAME}");
const u8 gText_Mind[] = _("Mind");
const u8 gText_Skill[] = _("Skill");
const u8 gText_Body[] = _("Body");
const u8 gText_Judgment[] = _("{B_BUFF1}{CLEAR 13}Judgment{CLEAR 13}{B_BUFF2}");
static const u8 sText_TwoTrainersSentPkmn[] = _("{B_TRAINER1_NAME_WITH_CLASS} sent out {B_OPPONENT_MON1_NAME}!\p{B_TRAINER2_NAME_WITH_CLASS} sent out {B_OPPONENT_MON2_NAME}!");
static const u8 sText_Trainer2SentOutPkmn[] = _("{B_TRAINER2_NAME_WITH_CLASS} sent out {B_BUFF1}!");
static const u8 sText_TwoTrainersWantToBattle[] = _("You are challenged by {B_TRAINER1_NAME_WITH_CLASS} and {B_TRAINER2_NAME_WITH_CLASS}!\p");
static const u8 sText_InGamePartnerSentOutZGoN[] = _("{B_PARTNER_NAME_WITH_CLASS} sent out {B_PLAYER_MON2_NAME}! Go, {B_PLAYER_MON1_NAME}!");
static const u8 sText_InGamePartnerSentOutNGoZ[] = _("{B_PARTNER_NAME_WITH_CLASS} sent out {B_PLAYER_MON1_NAME}! Go, {B_PLAYER_MON2_NAME}!");
static const u8 sText_InGamePartnerSentOutPkmn1[] = _("{B_PARTNER_NAME_WITH_CLASS} sent out {B_PLAYER_MON1_NAME}!");
static const u8 sText_InGamePartnerSentOutPkmn2[] = _("{B_PARTNER_NAME_WITH_CLASS} sent out {B_PLAYER_MON2_NAME}!");
static const u8 sText_InGamePartnerWithdrewPkmn1[] = _("{B_PARTNER_NAME_WITH_CLASS} withdrew {B_PLAYER_MON1_NAME}!");
static const u8 sText_InGamePartnerWithdrewPkmn2[] = _("{B_PARTNER_NAME_WITH_CLASS} withdrew {B_PLAYER_MON2_NAME}!");

const u16 gBattlePalaceFlavorTextTable[] =
{
    [B_MSG_GLINT_IN_EYE]   = STRINGID_GLINTAPPEARSINEYE,
    [B_MSG_GETTING_IN_POS] = STRINGID_PKMNGETTINGINTOPOSITION,
    [B_MSG_GROWL_DEEPLY]   = STRINGID_PKMNBEGANGROWLINGDEEPLY,
    [B_MSG_EAGER_FOR_MORE] = STRINGID_PKMNEAGERFORMORE,
};

const u8 *const gRefereeStringsTable[] =
{
    [B_MSG_REF_NOTHING_IS_DECIDED] = COMPOUND_STRING("REFEREE: If nothing is decided in 3 turns, we will go to judging!"),
    [B_MSG_REF_THATS_IT]           = COMPOUND_STRING("REFEREE: That's it! We will now go to judging to determine the winner!"),
    [B_MSG_REF_JUDGE_MIND]         = COMPOUND_STRING("REFEREE: Judging category 1, Mind! The POKéMON showing the most guts!\p"),
    [B_MSG_REF_JUDGE_SKILL]        = COMPOUND_STRING("REFEREE: Judging category 2, Skill! The POKéMON using moves the best!\p"),
    [B_MSG_REF_JUDGE_BODY]         = COMPOUND_STRING("REFEREE: Judging category 3, Body! The POKéMON with the most vitality!\p"),
    [B_MSG_REF_PLAYER_WON]         = COMPOUND_STRING("REFEREE: Judgment: {B_BUFF1} to {B_BUFF2}! The winner is {B_PLAYER_NAME}'s {B_PLAYER_MON1_NAME}!\p"),
    [B_MSG_REF_OPPONENT_WON]       = COMPOUND_STRING("REFEREE: Judgment: {B_BUFF1} to {B_BUFF2}! The winner is {B_TRAINER1_NAME}'s {B_OPPONENT_MON1_NAME}!\p"),
    [B_MSG_REF_DRAW]               = COMPOUND_STRING("REFEREE: Judgment: 3 to 3! We have a draw!\p"),
    [B_MSG_REF_COMMENCE_BATTLE]    = COMPOUND_STRING("REFEREE: {B_PLAYER_MON1_NAME} VS {B_OPPONENT_MON1_NAME}! Commence battling!"),
};

static const u8 sText_Trainer1Fled[] = _( "{PLAY_SE SE_FLEE}{B_TRAINER1_NAME_WITH_CLASS} fled!");
static const u8 sText_PlayerLostAgainstTrainer1[] = _("You lost to {B_TRAINER1_NAME_WITH_CLASS}!");
static const u8 sText_PlayerBattledToDrawTrainer1[] = _("You battled to a draw against {B_TRAINER1_NAME_WITH_CLASS}!");
const u8 gText_RecordBattleToPass[] = _("Would you like to record your battle\non your Frontier Pass?");
const u8 gText_BattleRecordedOnPass[] = _("{B_PLAYER_NAME}'s battle result was recorded\non the Frontier Pass.");
static const u8 sText_LinkTrainerWantsToBattlePause[] = _("You are challenged by {B_LINK_OPPONENT1_NAME}!\p");
static const u8 sText_TwoLinkTrainersWantToBattlePause[] = _("You are challenged by {B_LINK_OPPONENT1_NAME} and {B_LINK_OPPONENT2_NAME}!\p");
static const u8 sText_Your1[] = _("Your");
static const u8 sText_Opposing1[] = _("The opposing");
static const u8 sText_Your2[] = _("your");
static const u8 sText_Opposing2[] = _("the opposing");
static const u8 sText_EmptyStatus[] = _("$$$$$$$");

static const struct BattleWindowText sTextOnWindowsInfo_Normal[] =
{
    [B_WIN_MSG] = {
        .fillValue = PIXEL_FILL(0xF),
        .fontId = FONT_NORMAL,
        .x = 0,
        .y = 1,
        .speed = 1,
        .color.foreground = 1,
        .color.background = 15,
        .color.accent = 15,
        .color.shadow = 6,
    },
    [B_WIN_ACTION_PROMPT] = {
        .fillValue = PIXEL_FILL(0xF),
        .fontId = FONT_NORMAL,
        .x = 1,
        .y = 1,
        .speed = 0,
        .color.foreground = 1,
        .color.background = 15,
        .color.accent = 15,
        .color.shadow = 6,
    },
    [B_WIN_ACTION_MENU] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NORMAL,
        .x = 0,
        .y = 1,
        .speed = 0,
        .color.foreground = 13,
        .color.background = 14,
        .color.accent = 14,
        .color.shadow = 15,
    },
    [B_WIN_MOVE_NAME_1] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NARROW,
        .x = 0,
        .y = 1,
        .speed = 0,
        .color.foreground = 13,
        .color.background = 14,
        .color.accent = 14,
        .color.shadow = 15,
    },
    [B_WIN_MOVE_NAME_2] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NARROW,
        .x = 0,
        .y = 1,
        .speed = 0,
        .color.foreground = 13,
        .color.background = 14,
        .color.accent = 14,
        .color.shadow = 15,
    },
    [B_WIN_MOVE_NAME_3] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NARROW,
        .x = 0,
        .y = 1,
        .speed = 0,
        .color.foreground = 13,
        .color.background = 14,
        .color.accent = 14,
        .color.shadow = 15,
    },
    [B_WIN_MOVE_NAME_4] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NARROW,
        .x = 0,
        .y = 1,
        .speed = 0,
        .color.foreground = 13,
        .color.background = 14,
        .color.accent = 14,
        .color.shadow = 15,
    },
    [B_WIN_PP] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NARROW,
        .x = 0,
        .y = 1,
        .speed = 0,
        .color.foreground = B_SHOW_EFFECTIVENESS != SHOW_EFFECTIVENESS_NEVER ? 13 : 12,
        .color.background = 14,
        .color.accent = 14,
        .color.shadow = B_SHOW_EFFECTIVENESS != SHOW_EFFECTIVENESS_NEVER ? 15 : 11,
    },
    [B_WIN_DUMMY] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NORMAL,
        .x = 0,
        .y = 1,
        .speed = 0,
        .color.foreground = 13,
        .color.background = 14,
        .color.accent = 14,
        .color.shadow = 15,
    },
    [B_WIN_PP_REMAINING] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NORMAL,
        .x = 2,
        .y = 1,
        .speed = 0,
        .color.foreground = 12,
        .color.background = 14,
        .color.accent = 14,
        .color.shadow = 11,
    },
    [B_WIN_MOVE_TYPE] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NARROW,
        .x = 0,
        .y = 1,
        .speed = 0,
        .color.foreground = 13,
        .color.background = 14,
        .color.accent = 14,
        .color.shadow = 15,
    },
    [B_WIN_SWITCH_PROMPT] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NARROW,
        .x = 0,
        .y = 1,
        .speed = 0,
        .color.foreground = 13,
        .color.background = 14,
        .color.accent = 14,
        .color.shadow = 15,
    },
    [B_WIN_YESNO] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NORMAL,
        .x = 0,
        .y = 1,
        .speed = 0,
        .color.foreground = 13,
        .color.background = 14,
        .color.accent = 14,
        .color.shadow = 15,
    },
    [B_WIN_LEVEL_UP_BOX] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NORMAL,
        .x = 0,
        .y = 1,
        .speed = 0,
        .color.foreground = 13,
        .color.background = 14,
        .color.accent = 14,
        .color.shadow = 15,
    },
    [B_WIN_LEVEL_UP_BANNER] = {
        .fillValue = PIXEL_FILL(0),
        .fontId = FONT_NORMAL,
        .x = 32,
        .y = 1,
        .speed = 0,
        .color.foreground = 1,
        .color.shadow = 2,
    },
    [B_WIN_VS_PLAYER] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NORMAL,
        .x = -1,
        .y = 1,
        .speed = 0,
        .color.foreground = 13,
        .color.background = 14,
        .color.accent = 14,
        .color.shadow = 15,
    },
    [B_WIN_VS_OPPONENT] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NORMAL,
        .x = -1,
        .y = 1,
        .speed = 0,
        .color.foreground = 13,
        .color.background = 14,
        .color.accent = 14,
        .color.shadow = 15,
    },
    [B_WIN_VS_MULTI_PLAYER_1] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NORMAL,
        .x = -1,
        .y = 1,
        .speed = 0,
        .color.foreground = 13,
        .color.background = 14,
        .color.accent = 14,
        .color.shadow = 15,
    },
    [B_WIN_VS_MULTI_PLAYER_2] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NORMAL,
        .x = -1,
        .y = 1,
        .speed = 0,
        .color.foreground = 13,
        .color.background = 14,
        .color.accent = 14,
        .color.shadow = 15,
    },
    [B_WIN_VS_MULTI_PLAYER_3] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NORMAL,
        .x = -1,
        .y = 1,
        .speed = 0,
        .color.foreground = 13,
        .color.background = 14,
        .color.accent = 14,
        .color.shadow = 15,
    },
    [B_WIN_VS_MULTI_PLAYER_4] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NORMAL,
        .x = -1,
        .y = 1,
        .speed = 0,
        .color.foreground = 13,
        .color.background = 14,
        .color.accent = 14,
        .color.shadow = 15,
    },
    [B_WIN_VS_OUTCOME_DRAW] = {
        .fillValue = PIXEL_FILL(0),
        .fontId = FONT_NORMAL,
        .x = -1,
        .y = 1,
        .speed = 0,
        .color.foreground = 1,
        .color.shadow = 6,
    },
    [B_WIN_VS_OUTCOME_LEFT] = {
        .fillValue = PIXEL_FILL(0),
        .fontId = FONT_NORMAL,
        .x = -1,
        .y = 1,
        .speed = 0,
        .color.foreground = 1,
        .color.shadow = 6,
    },
    [B_WIN_VS_OUTCOME_RIGHT] = {
        .fillValue = PIXEL_FILL(0x0),
        .fontId = FONT_NORMAL,
        .x = -1,
        .y = 1,
        .speed = 0,
        .color.foreground = 1,
        .color.shadow = 6,
    },
    [B_WIN_MOVE_DESCRIPTION] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NARROW,
        .x = 0,
        .y = 1,
        .letterSpacing = 0,
        .lineSpacing = 0,
        .speed = 0,
        .color.foreground = TEXT_DYNAMIC_COLOR_4,
        .color.background = TEXT_DYNAMIC_COLOR_5,
        .color.accent = TEXT_DYNAMIC_COLOR_5,
        .color.shadow = TEXT_DYNAMIC_COLOR_6,
    },
    [B_CATCH_OR_NOT] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NORMAL,
        .x = 0,
        .y = 1,
        .speed = 0,
        .color.foreground = 13,
        .color.background = 14,
        .color.accent = 14,
        .color.shadow = 15,
    },
};

static const struct BattleWindowText sTextOnWindowsInfo_KantoTutorial[] =
{
    [B_WIN_MSG] = {
        .fillValue = PIXEL_FILL(0xF),
        .fontId = FONT_NORMAL,
        .x = 0,
        .y = 1,
        .speed = 1,
        .color.foreground = 1,
        .color.background = 15,
        .color.accent = 15,
        .color.shadow = 6,
    },
    [B_WIN_ACTION_PROMPT] = {
        .fillValue = PIXEL_FILL(0xF),
        .fontId = FONT_NORMAL,
        .x = 1,
        .y = 1,
        .speed = 0,
        .color.foreground = 1,
        .color.background = 15,
        .color.accent = 15,
        .color.shadow = 6,
    },
    [B_WIN_ACTION_MENU] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NORMAL,
        .x = 0,
        .y = 1,
        .speed = 0,
        .color.foreground = 13,
        .color.background = 14,
        .color.accent = 14,
        .color.shadow = 15,
    },
    [B_WIN_MOVE_NAME_1] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NARROW,
        .x = 0,
        .y = 1,
        .speed = 0,
        .color.foreground = 13,
        .color.background = 14,
        .color.accent = 14,
        .color.shadow = 15,
    },
    [B_WIN_MOVE_NAME_2] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NARROW,
        .x = 0,
        .y = 1,
        .speed = 0,
        .color.foreground = 13,
        .color.background = 14,
        .color.accent = 14,
        .color.shadow = 15,
    },
    [B_WIN_MOVE_NAME_3] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NARROW,
        .x = 0,
        .y = 1,
        .speed = 0,
        .color.foreground = 13,
        .color.background = 14,
        .color.accent = 14,
        .color.shadow = 15,
    },
    [B_WIN_MOVE_NAME_4] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NARROW,
        .x = 0,
        .y = 1,
        .speed = 0,
        .color.foreground = 13,
        .color.background = 14,
        .color.accent = 14,
        .color.shadow = 15,
    },
    [B_WIN_PP] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NARROW,
        .x = 0,
        .y = 1,
        .speed = 0,
        .color.foreground = B_SHOW_EFFECTIVENESS != SHOW_EFFECTIVENESS_NEVER ? 13 : 12,
        .color.background = 14,
        .color.accent = 14,
        .color.shadow = B_SHOW_EFFECTIVENESS != SHOW_EFFECTIVENESS_NEVER ? 15 : 11,
    },
    [B_WIN_DUMMY] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NORMAL,
        .x = 0,
        .y = 1,
        .speed = 0,
        .color.foreground = 13,
        .color.background = 14,
        .color.accent = 14,
        .color.shadow = 15,
    },
    [B_WIN_PP_REMAINING] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NORMAL,
        .x = 2,
        .y = 1,
        .speed = 0,
        .color.foreground = 12,
        .color.background = 14,
        .color.accent = 14,
        .color.shadow = 11,
    },
    [B_WIN_MOVE_TYPE] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NARROW,
        .x = 0,
        .y = 1,
        .speed = 0,
        .color.foreground = 13,
        .color.background = 14,
        .color.accent = 14,
        .color.shadow = 15,
    },
    [B_WIN_SWITCH_PROMPT] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NARROW,
        .x = 0,
        .y = 1,
        .speed = 0,
        .color.foreground = 13,
        .color.background = 14,
        .color.accent = 14,
        .color.shadow = 15,
    },
    [B_WIN_YESNO] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NORMAL,
        .x = 0,
        .y = 1,
        .speed = 0,
        .color.foreground = 13,
        .color.background = 14,
        .color.accent = 14,
        .color.shadow = 15,
    },
    [B_WIN_LEVEL_UP_BOX] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NORMAL,
        .x = 0,
        .y = 1,
        .speed = 0,
        .color.foreground = 13,
        .color.background = 14,
        .color.accent = 14,
        .color.shadow = 15,
    },
    [B_WIN_LEVEL_UP_BANNER] = {
        .fillValue = PIXEL_FILL(0),
        .fontId = FONT_NORMAL,
        .x = 32,
        .y = 1,
        .speed = 0,
        .color.foreground = 1,
        .color.shadow = 2,
    },
    [B_WIN_VS_PLAYER] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NORMAL,
        .x = -1,
        .y = 1,
        .speed = 0,
        .color.foreground = 13,
        .color.background = 14,
        .color.accent = 14,
        .color.shadow = 15,
    },
    [B_WIN_VS_OPPONENT] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NORMAL,
        .x = -1,
        .y = 1,
        .speed = 0,
        .color.foreground = 13,
        .color.background = 14,
        .color.accent = 14,
        .color.shadow = 15,
    },
    [B_WIN_VS_MULTI_PLAYER_1] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NORMAL,
        .x = -1,
        .y = 1,
        .speed = 0,
        .color.foreground = 13,
        .color.background = 14,
        .color.accent = 14,
        .color.shadow = 15,
    },
    [B_WIN_VS_MULTI_PLAYER_2] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NORMAL,
        .x = -1,
        .y = 1,
        .speed = 0,
        .color.foreground = 13,
        .color.background = 14,
        .color.accent = 14,
        .color.shadow = 15,
    },
    [B_WIN_VS_MULTI_PLAYER_3] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NORMAL,
        .x = -1,
        .y = 1,
        .speed = 0,
        .color.foreground = 13,
        .color.background = 14,
        .color.accent = 14,
        .color.shadow = 15,
    },
    [B_WIN_VS_MULTI_PLAYER_4] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NORMAL,
        .x = -1,
        .y = 1,
        .speed = 0,
        .color.foreground = 13,
        .color.background = 14,
        .color.accent = 14,
        .color.shadow = 15,
    },
    [B_WIN_VS_OUTCOME_DRAW] = {
        .fillValue = PIXEL_FILL(0),
        .fontId = FONT_NORMAL,
        .x = -1,
        .y = 1,
        .speed = 0,
        .color.foreground = 1,
        .color.shadow = 6,
    },
    [B_WIN_VS_OUTCOME_LEFT] = {
        .fillValue = PIXEL_FILL(0),
        .fontId = FONT_NORMAL,
        .x = -1,
        .y = 1,
        .speed = 0,
        .color.foreground = 1,
        .color.shadow = 6,
    },
    [B_WIN_VS_OUTCOME_RIGHT] = {
        .fillValue = PIXEL_FILL(0x0),
        .fontId = FONT_NORMAL,
        .x = -1,
        .y = 1,
        .speed = 0,
        .color.foreground = 1,
        .color.shadow = 6,
    },
    [B_WIN_MOVE_DESCRIPTION] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NARROW,
        .x = 0,
        .y = 1,
        .letterSpacing = 0,
        .lineSpacing = 0,
        .speed = 0,
        .color.foreground = TEXT_DYNAMIC_COLOR_4,
        .color.background = TEXT_DYNAMIC_COLOR_5,
        .color.accent = TEXT_DYNAMIC_COLOR_5,
        .color.shadow = TEXT_DYNAMIC_COLOR_6,
    },
    [B_WIN_OAK_OLD_MAN] = {
        .fillValue = PIXEL_FILL(0x1),
        .fontId = FONT_NORMAL,
        .x = 0,
        .y = 1,
        .letterSpacing = 0,
        .lineSpacing = 1,
        .speed = 1,
        .color.foreground = 2,
        .color.background = 1,
        .color.accent = 1,
        .color.shadow = 3,
    },
};

static const struct BattleWindowText sTextOnWindowsInfo_Arena[] =
{
    [B_WIN_MSG] = {
        .fillValue = PIXEL_FILL(0xF),
        .fontId = FONT_NORMAL,
        .x = 0,
        .y = 1,
        .speed = 1,
        .color.foreground = 1,
        .color.background = 15,
        .color.accent = 15,
        .color.shadow = 6,
    },
    [B_WIN_ACTION_PROMPT] = {
        .fillValue = PIXEL_FILL(0xF),
        .fontId = FONT_NORMAL,
        .x = 1,
        .y = 1,
        .speed = 0,
        .color.foreground = 1,
        .color.background = 15,
        .color.accent = 15,
        .color.shadow = 6,
    },
    [B_WIN_ACTION_MENU] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NORMAL,
        .x = 0,
        .y = 1,
        .speed = 0,
        .color.foreground = 13,
        .color.background = 14,
        .color.accent = 14,
        .color.shadow = 15,
    },
    [B_WIN_MOVE_NAME_1] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NARROW,
        .x = 0,
        .y = 1,
        .speed = 0,
        .color.foreground = 13,
        .color.background = 14,
        .color.accent = 14,
        .color.shadow = 15,
    },
    [B_WIN_MOVE_NAME_2] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NARROW,
        .x = 0,
        .y = 1,
        .speed = 0,
        .color.foreground = 13,
        .color.background = 14,
        .color.accent = 14,
        .color.shadow = 15,
    },
    [B_WIN_MOVE_NAME_3] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NARROW,
        .x = 0,
        .y = 1,
        .speed = 0,
        .color.foreground = 13,
        .color.background = 14,
        .color.accent = 14,
        .color.shadow = 15,
    },
    [B_WIN_MOVE_NAME_4] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NARROW,
        .x = 0,
        .y = 1,
        .speed = 0,
        .color.foreground = 13,
        .color.background = 14,
        .color.accent = 14,
        .color.shadow = 15,
    },
    [B_WIN_PP] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NARROW,
        .x = 0,
        .y = 1,
        .speed = 0,
        .color.foreground = B_SHOW_EFFECTIVENESS != SHOW_EFFECTIVENESS_NEVER ? 13 : 12,
        .color.background = 14,
        .color.accent = 14,
        .color.shadow = B_SHOW_EFFECTIVENESS != SHOW_EFFECTIVENESS_NEVER ? 15 : 11,
    },
    [B_WIN_DUMMY] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NORMAL,
        .x = 0,
        .y = 1,
        .speed = 0,
        .color.foreground = 13,
        .color.background = 14,
        .color.accent = 14,
        .color.shadow = 15,
    },
    [B_WIN_PP_REMAINING] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NORMAL,
        .x = 2,
        .y = 1,
        .speed = 0,
        .color.foreground = 12,
        .color.background = 14,
        .color.accent = 14,
        .color.shadow = 11,
    },
    [B_WIN_MOVE_TYPE] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NARROW,
        .x = 0,
        .y = 1,
        .speed = 0,
        .color.foreground = 13,
        .color.background = 14,
        .color.accent = 14,
        .color.shadow = 15,
    },
    [B_WIN_SWITCH_PROMPT] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NARROW,
        .x = 0,
        .y = 1,
        .speed = 0,
        .color.foreground = 13,
        .color.background = 14,
        .color.accent = 14,
        .color.shadow = 15,
    },
    [B_WIN_YESNO] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NORMAL,
        .x = 0,
        .y = 1,
        .speed = 0,
        .color.foreground = 13,
        .color.background = 14,
        .color.accent = 14,
        .color.shadow = 15,
    },
    [B_WIN_LEVEL_UP_BOX] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NORMAL,
        .x = 0,
        .y = 1,
        .speed = 0,
        .color.foreground = 13,
        .color.background = 14,
        .color.accent = 14,
        .color.shadow = 15,
    },
    [B_WIN_LEVEL_UP_BANNER] = {
        .fillValue = PIXEL_FILL(0),
        .fontId = FONT_NORMAL,
        .x = 32,
        .y = 1,
        .speed = 0,
        .color.foreground = 1,
        .color.shadow = 2,
    },
    [ARENA_WIN_PLAYER_NAME] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NORMAL,
        .x = -1,
        .y = 1,
        .speed = 0,
        .color.foreground = 1,
        .color.background = 14,
        .color.accent = 14,
        .color.shadow = 15,
    },
    [ARENA_WIN_VS] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NORMAL,
        .x = -1,
        .y = 1,
        .speed = 0,
        .color.foreground = 13,
        .color.background = 14,
        .color.accent = 14,
        .color.shadow = 15,
    },
    [ARENA_WIN_OPPONENT_NAME] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NORMAL,
        .x = -1,
        .y = 1,
        .speed = 0,
        .color.foreground = 13,
        .color.background = 14,
        .color.accent = 14,
        .color.shadow = 15,
    },
    [ARENA_WIN_MIND] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NORMAL,
        .x = -1,
        .y = 1,
        .speed = 0,
        .color.foreground = 13,
        .color.background = 14,
        .color.accent = 14,
        .color.shadow = 15,
    },
    [ARENA_WIN_SKILL] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NORMAL,
        .x = -1,
        .y = 1,
        .speed = 0,
        .color.foreground = 13,
        .color.background = 14,
        .color.accent = 14,
        .color.shadow = 15,
    },
    [ARENA_WIN_BODY] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NORMAL,
        .x = -1,
        .y = 1,
        .speed = 0,
        .color.foreground = 13,
        .color.background = 14,
        .color.accent = 14,
        .color.shadow = 15,
    },
    [ARENA_WIN_JUDGMENT_TITLE] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NORMAL,
        .x = -1,
        .y = 1,
        .speed = 0,
        .color.foreground = 13,
        .color.background = 14,
        .color.accent = 14,
        .color.shadow = 15,
    },
    [ARENA_WIN_JUDGMENT_TEXT] = {
        .fillValue = PIXEL_FILL(0x1),
        .fontId = FONT_NORMAL,
        .x = 0,
        .y = 1,
        .speed = 1,
        .color.foreground = 2,
        .color.background = 1,
        .color.accent = 1,
        .color.shadow = 3,
    },
    [B_WIN_MOVE_DESCRIPTION] = {
        .fillValue = PIXEL_FILL(0xE),
        .fontId = FONT_NARROW,
        .x = 0,
        .y = 1,
        .letterSpacing = 0,
        .lineSpacing = 0,
        .speed = 0,
        .color.foreground = TEXT_DYNAMIC_COLOR_4,
        .color.background = TEXT_DYNAMIC_COLOR_5,
        .color.accent = TEXT_DYNAMIC_COLOR_5,
        .color.shadow = TEXT_DYNAMIC_COLOR_6,
    },
};

static const struct BattleWindowText *const sBattleTextOnWindowsInfo[] =
{
    [B_WIN_TYPE_NORMAL] = sTextOnWindowsInfo_Normal,
    [B_WIN_TYPE_ARENA]  = sTextOnWindowsInfo_Arena,
    [B_WIN_TYPE_KANTO_TUTORIAL] = sTextOnWindowsInfo_KantoTutorial,
};

static const u8 sRecordedBattleTextSpeeds[] = {8, 4, 1, 0};

static const u8 *const sUsedMoveStringVariants[] = {
    sText_AttackerUsedX, sText_AttackerUsedX2, sText_AttackerUsedX3, sText_AttackerUsedX4,
    sText_AttackerUsedX5, sText_AttackerUsedX6, sText_AttackerUsedX7, sText_AttackerUsedX8,
};

// Common, non-move-specific battle messages that are reworded at random for variety.
static const u16 sCriticalHitStringVariants[] = {
    STRINGID_CRITICALHIT, STRINGID_CRITICALHIT_2, STRINGID_CRITICALHIT_3, STRINGID_CRITICALHIT_4,
    STRINGID_CRITICALHIT_5, STRINGID_CRITICALHIT_6, STRINGID_CRITICALHIT_7, STRINGID_CRITICALHIT_8,
};
static const u16 sSuperEffectiveStringVariants[] = {
    STRINGID_SUPEREFFECTIVE, STRINGID_SUPEREFFECTIVE_2, STRINGID_SUPEREFFECTIVE_3, STRINGID_SUPEREFFECTIVE_4,
    STRINGID_SUPEREFFECTIVE_5, STRINGID_SUPEREFFECTIVE_6, STRINGID_SUPEREFFECTIVE_7, STRINGID_SUPEREFFECTIVE_8,
};
static const u16 sNotVeryEffectiveStringVariants[] = {
    STRINGID_NOTVERYEFFECTIVE, STRINGID_NOTVERYEFFECTIVE_2, STRINGID_NOTVERYEFFECTIVE_3, STRINGID_NOTVERYEFFECTIVE_4,
    STRINGID_NOTVERYEFFECTIVE_5, STRINGID_NOTVERYEFFECTIVE_6, STRINGID_NOTVERYEFFECTIVE_7, STRINGID_NOTVERYEFFECTIVE_8,
};
static const u16 sFaintedStringVariants[] = {
    STRINGID_BATTLERFAINTED, STRINGID_BATTLERFAINTED_2, STRINGID_BATTLERFAINTED_3, STRINGID_BATTLERFAINTED_4,
    STRINGID_BATTLERFAINTED_5, STRINGID_BATTLERFAINTED_6, STRINGID_BATTLERFAINTED_7, STRINGID_BATTLERFAINTED_8,
};
static const u16 sAvoidedAttackStringVariants[] = {
    STRINGID_PKMNAVOIDEDATTACK, STRINGID_PKMNAVOIDEDATTACK_2, STRINGID_PKMNAVOIDEDATTACK_3, STRINGID_PKMNAVOIDEDATTACK_4,
    STRINGID_PKMNAVOIDEDATTACK_5, STRINGID_PKMNAVOIDEDATTACK_6, STRINGID_PKMNAVOIDEDATTACK_7, STRINGID_PKMNAVOIDEDATTACK_8,
};
static const u16 sMoveFailedStringVariants[] = {
    STRINGID_BUTITFAILED, STRINGID_BUTITFAILED_2, STRINGID_BUTITFAILED_3, STRINGID_BUTITFAILED_4,
    STRINGID_BUTITFAILED_5, STRINGID_BUTITFAILED_6, STRINGID_BUTITFAILED_7, STRINGID_BUTITFAILED_8,
};
static const u16 sNothingHappenedStringVariants[] = {
    STRINGID_BUTNOTHINGHAPPENED, STRINGID_BUTNOTHINGHAPPENED_2, STRINGID_BUTNOTHINGHAPPENED_3, STRINGID_BUTNOTHINGHAPPENED_4,
    STRINGID_BUTNOTHINGHAPPENED_5, STRINGID_BUTNOTHINGHAPPENED_6, STRINGID_BUTNOTHINGHAPPENED_7, STRINGID_BUTNOTHINGHAPPENED_8,
};

// Picks a random reworded variant for stringID if it has any, otherwise returns it unchanged.
static enum StringID GetVariedStringId(enum StringID stringID)
{
    // Keep messages deterministic for the test runner, which expects exact text.
    if (gTestRunnerEnabled)
        return stringID;

    switch (stringID)
    {
    case STRINGID_CRITICALHIT:
        return sCriticalHitStringVariants[Random() % ARRAY_COUNT(sCriticalHitStringVariants)];
    case STRINGID_SUPEREFFECTIVE:
        return sSuperEffectiveStringVariants[Random() % ARRAY_COUNT(sSuperEffectiveStringVariants)];
    case STRINGID_NOTVERYEFFECTIVE:
        return sNotVeryEffectiveStringVariants[Random() % ARRAY_COUNT(sNotVeryEffectiveStringVariants)];
    case STRINGID_BATTLERFAINTED:
        return sFaintedStringVariants[Random() % ARRAY_COUNT(sFaintedStringVariants)];
    case STRINGID_PKMNAVOIDEDATTACK:
        return sAvoidedAttackStringVariants[Random() % ARRAY_COUNT(sAvoidedAttackStringVariants)];
    case STRINGID_BUTITFAILED:
        return sMoveFailedStringVariants[Random() % ARRAY_COUNT(sMoveFailedStringVariants)];
    case STRINGID_BUTNOTHINGHAPPENED:
        return sNothingHappenedStringVariants[Random() % ARRAY_COUNT(sNothingHappenedStringVariants)];
    default:
        return stringID;
    }
}

void BufferStringBattle(enum StringID stringID, enum BattlerId battler)
{
    s32 i;
    const u8 *stringPtr = NULL;

    gBattleMsgDataPtr = (struct BattleMsgData *)(&gBattleResources->bufferA[battler][4]);
    gLastUsedItem = gBattleMsgDataPtr->lastItem;
    gLastUsedAbility = gBattleMsgDataPtr->lastAbility;
    gBattleScripting.battler = gBattleMsgDataPtr->scrActive;
    gBattleStruct->scriptPartyIdx = gBattleMsgDataPtr->bakScriptPartyIdx;
    gBattleStruct->hpScale = gBattleMsgDataPtr->hpScale;
    gPotentialItemEffectBattler = gBattleMsgDataPtr->itemEffectBattler;
    gBattleStruct->stringMoveType = gBattleMsgDataPtr->moveType;

    for (i = 0; i < MAX_BATTLERS_COUNT; i++)
    {
        sBattlerAbilities[i] = gBattleMsgDataPtr->abilities[i];
    }
    for (i = 0; i < TEXT_BUFF_ARRAY_COUNT; i++)
    {
        gBattleTextBuff1[i] = gBattleMsgDataPtr->textBuffs[0][i];
        gBattleTextBuff2[i] = gBattleMsgDataPtr->textBuffs[1][i];
        gBattleTextBuff3[i] = gBattleMsgDataPtr->textBuffs[2][i];
    }

    switch (stringID)
    {
    case STRINGID_INTROMSG: // first battle msg
        if (gBattleTypeFlags & BATTLE_TYPE_TRAINER)
        {
            if (gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_RECORDED_LINK))
            {
                if (gBattleTypeFlags & BATTLE_TYPE_TOWER_LINK_MULTI)
                {
                    stringPtr = sText_TwoTrainersWantToBattle;
                }
                else if (gBattleTypeFlags & BATTLE_TYPE_TWO_OPPONENTS)
                {
                    stringPtr = sText_TwoLinkTrainersWantToBattle;
                }
                else if (gBattleTypeFlags & BATTLE_TYPE_MULTI)
                {
                    if (gBattleTypeFlags & BATTLE_TYPE_RECORDED)
                    {
                        if (gBattleTypeFlags & BATTLE_TYPE_RECORDED_LINK)
                            stringPtr = sText_LinkTrainerWantsToBattle;
                        else if (gBattleTypeFlags & BATTLE_TYPE_TWO_OPPONENTS)
                            stringPtr = sText_TwoTrainersWantToBattle;
                        else if (!(gBattleTypeFlags & BATTLE_TYPE_TWO_OPPONENTS))
                            stringPtr = sText_LinkTrainerWantsToBattlePause;
                        else
                            stringPtr = sText_TwoLinkTrainersWantToBattlePause;
                    }
                    else
                    {
                        stringPtr = sText_TwoLinkTrainersWantToBattle;
                    }
                }
                else
                {
                    if (TRAINER_BATTLE_PARAM.opponentA == TRAINER_UNION_ROOM)
                        stringPtr = sText_Trainer1WantsToBattle;
                    else if (gBattleTypeFlags & BATTLE_TYPE_RECORDED)
                        stringPtr = sText_LinkTrainerWantsToBattlePause;
                    else
                        stringPtr = sText_LinkTrainerWantsToBattle;
                }
            }
            else
            {
                if (BATTLE_TWO_VS_ONE_OPPONENT)
                    stringPtr = sText_Trainer1WantsToBattle;
                else if (gBattleTypeFlags & (BATTLE_TYPE_MULTI | BATTLE_TYPE_INGAME_PARTNER))
                    stringPtr = sText_TwoTrainersWantToBattle;
                else if (gBattleTypeFlags & BATTLE_TYPE_TWO_OPPONENTS)
                    stringPtr = sText_TwoTrainersWantToBattle;
                else
                    stringPtr = sText_Trainer1WantsToBattle;
            }
        }
        else
        {
            if (IsGhostBattleWithoutScope())
                stringPtr = sText_GhostAppearedCantId;
            else if (gBattleTypeFlags & BATTLE_TYPE_GHOST)
                stringPtr = sText_TheGhostAppeared;
            else if (gBattleTypeFlags & BATTLE_TYPE_LEGENDARY)
                stringPtr = sText_LegendaryPkmnAppeared;
            else if (IsDoubleBattle() && IsValidForBattle(GetBattlerMon(GetBattlerAtPosition(B_POSITION_OPPONENT_RIGHT))))
                stringPtr = sText_TwoWildPkmnAppeared;
            else if (gBattleTypeFlags & BATTLE_TYPE_CATCH_TUTORIAL)
                stringPtr = sText_WildPkmnAppearedPause;
            else
                stringPtr = sText_WildPkmnAppeared;
        }
        break;
    case STRINGID_INTROSENDOUT: // poke first send-out
        if (BattlerIsPlayer(battler) || BattlerIsPlayer(GetPartnerBattler(battler))
         || BattlerIsWally(battler) || BattlerIsWally(GetPartnerBattler(battler)))
        {
            if (IsDoubleBattle() && IsValidForBattle(GetBattlerMon(GetPartnerBattler(battler))))
            {
                if (gBattleTypeFlags & BATTLE_TYPE_MULTI && (gBattleTypeFlags & BATTLE_TYPE_RECORDED_LINK || gBattleTypeFlags & BATTLE_TYPE_LINK))
                {
                    if (BattlerIsPlayer(battler))
                        stringPtr = sText_LinkPartnerSentOutPkmn2GoPkmn; // Player is on left
                    else
                        stringPtr = sText_LinkPartnerSentOutPkmn1GoPkmn; // Link Partner on left
                }
                else if (gBattleTypeFlags & BATTLE_TYPE_INGAME_PARTNER)
                {
                    if (BattlerIsPlayer(battler) && (battler & BIT_FLANK) == B_FLANK_LEFT)
                        stringPtr = sText_InGamePartnerSentOutZGoN; // Player is on left
                    else
                        stringPtr = sText_InGamePartnerSentOutNGoZ; // Partner on left
                }
                else if (gBattleTypeFlags & BATTLE_TYPE_RAID)
                {
                    stringPtr = sText_BeCarefulPkmn;
                }
                else
                {
                    stringPtr = sText_GoTwoPkmn;
                }
            }
            else
            {
                stringPtr = sText_GoPkmn;
            }
        }
        else
        {
            if (IsDoubleBattle() && IsValidForBattle(GetBattlerMon(GetPartnerBattler(battler))))
            {
                if (gBattleTypeFlags & BATTLE_TYPE_RECORDED_LINK && BATTLE_TWO_VS_ONE_OPPONENT)
                    stringPtr = sText_LinkTrainerSentOutTwoPkmn;
                else if (BATTLE_TWO_VS_ONE_OPPONENT)
                    stringPtr = sText_Trainer1SentOutTwoPkmn;
                else if ((gBattleTypeFlags & BATTLE_TYPE_RECORDED_LINK && gBattleTypeFlags & BATTLE_TYPE_TWO_OPPONENTS) || (gBattleTypeFlags & BATTLE_TYPE_MULTI && (gBattleTypeFlags & BATTLE_TYPE_RECORDED_LINK || gBattleTypeFlags & BATTLE_TYPE_LINK)))
                    stringPtr = sText_TwoLinkTrainersIntroSendOutPkmn;
                else if (gBattleTypeFlags & BATTLE_TYPE_TWO_OPPONENTS || gBattleTypeFlags & BATTLE_TYPE_TOWER_LINK_MULTI)
                    stringPtr = sText_TwoTrainersSentPkmn;
                else if (BattlerIsLink(battler) || (BattlerIsRecorded(battler) && BattlerIsOpponent(battler))) // Link Opponent doubles and test opponent
                    stringPtr = sText_LinkTrainerSentOutTwoPkmn;
                else
                    stringPtr = sText_Trainer1SentOutTwoPkmn;
            }
            else if (BattlerIsLink(battler) || (BattlerIsRecorded(battler) && BattlerIsOpponent(battler)))
            {
                if (TRAINER_BATTLE_PARAM.opponentA == TRAINER_UNION_ROOM)
                    stringPtr = sText_Trainer1SentOutPkmn;
                else
                    stringPtr = sText_LinkTrainerIntroSendOutPkmn;
            }
            else
            {
                stringPtr = sText_Trainer1SentOutPkmn;
            }
        }
        break;
    case STRINGID_RETURNMON: // sending poke to ball msg
        if ((GetBattlerPosition(battler) & BIT_FLANK) == B_FLANK_LEFT) // battler 0 and 1
        {
            // A partner-controller battler that isn't a real Link or in-game partner is just
            // the AI controlling the player's own second mon (see IsPlayerAiControlled) - treat
            // it as the player for message purposes rather than a distinct partner trainer.
            if (BattlerIsPlayer(battler) || BattlerIsWally(battler)
             || (BattlerIsPartner(battler) && !(gBattleTypeFlags & (BATTLE_TYPE_INGAME_PARTNER | BATTLE_TYPE_LINK | BATTLE_TYPE_RECORDED_LINK)))) // Player
            {
                if (*(&gBattleStruct->hpScale) == 0)
                    stringPtr = sText_PkmnThatsEnough;
                else if (*(&gBattleStruct->hpScale) == 1 || IsDoubleBattle())
                    stringPtr = sText_PkmnComeBack;
                else if (*(&gBattleStruct->hpScale) == 2)
                    stringPtr = sText_PkmnOkComeBack;
                else
                    stringPtr = sText_PkmnGoodComeBack;
            }
            else if (BattlerIsPartner(battler)) // Link or Ingame Partner
            {
                if (BattlerIsLink(battler) || gBattleTypeFlags & BATTLE_TYPE_RECORDED_LINK)
                    stringPtr = sText_LinkPartnerWithdrewPkmn1;
                else
                    stringPtr = sText_InGamePartnerWithdrewPkmn1;
            }
            else if (TRAINER_BATTLE_PARAM.opponentA == TRAINER_LINK_OPPONENT
             || gBattleTypeFlags & BATTLE_TYPE_RECORDED_LINK) // Link Opponent A and test opponent
            {
                stringPtr = sText_LinkTrainer1WithdrewPkmn;
            }
            else // Opponent A
            {
                stringPtr = sText_Trainer1WithdrewPkmn;
            }
        }
        else // battler 2 and 3
        {
            // See the battler 0/1 branch above: a partner-controller battler that isn't a
            // real Link or in-game partner is just the AI controlling the player's own mon.
            if (BattlerIsPlayer(battler)
             || (BattlerIsPartner(battler) && !(gBattleTypeFlags & (BATTLE_TYPE_INGAME_PARTNER | BATTLE_TYPE_LINK | BATTLE_TYPE_RECORDED_LINK)))) // Player
            {
                if (*(&gBattleStruct->hpScale) == 0)
                    stringPtr = sText_PkmnThatsEnough;
                else if (*(&gBattleStruct->hpScale) == 1 || IsDoubleBattle())
                    stringPtr = sText_PkmnComeBack;
                else if (*(&gBattleStruct->hpScale) == 2)
                    stringPtr = sText_PkmnOkComeBack;
                else
                    stringPtr = sText_PkmnGoodComeBack;
            }
            else if (BattlerIsPartner(battler)) // Link or Ingame Partner
            {
                if (BattlerIsLink(battler) || gBattleTypeFlags & BATTLE_TYPE_RECORDED_LINK)
                    stringPtr = sText_LinkPartnerWithdrewPkmn2;
                else
                    stringPtr = sText_InGamePartnerWithdrewPkmn2;
            }
            else if (TRAINER_BATTLE_PARAM.opponentA == TRAINER_LINK_OPPONENT
             || gBattleTypeFlags & BATTLE_TYPE_RECORDED_LINK) // Link Opponent B and test opponent
            {
                if (BattleSideHasTwoTrainers(B_SIDE_OPPONENT))
                    stringPtr = sText_LinkTrainer2WithdrewPkmn;
                else
                    stringPtr = sText_LinkTrainer1WithdrewPkmn;
            }
            else if (gBattleTypeFlags & BATTLE_TYPE_TWO_OPPONENTS) // Opponent B
            {
                stringPtr = sText_Trainer2WithdrewPkmn;
            }
            else // Opponent A
            {
                stringPtr = sText_Trainer1WithdrewPkmn;
            }
        }
        break;
    case STRINGID_SWITCHINMON: // switch-in msg
        if ((GetBattlerPosition(gBattleScripting.battler) & BIT_FLANK) == B_FLANK_LEFT) // battler 0 and 1
        {
            // A partner-controller battler that isn't a real Link or in-game partner is just
            // the AI controlling the player's own second mon (see IsPlayerAiControlled) - treat
            // it as the player for message purposes rather than a distinct partner trainer.
            if (BattlerIsPlayer(gBattleScripting.battler)
             || (BattlerIsPartner(gBattleScripting.battler) && !(gBattleTypeFlags & (BATTLE_TYPE_INGAME_PARTNER | BATTLE_TYPE_LINK | BATTLE_TYPE_RECORDED_LINK)))) // Player
            {
                if (*(&gBattleStruct->hpScale) == 0)
                    stringPtr = sText_GoPkmn2;
                else if (*(&gBattleStruct->hpScale) == 1 || IsDoubleBattle())
                    stringPtr = sText_DoItPkmn;
                else if (*(&gBattleStruct->hpScale) == 2)
                    stringPtr = sText_GoForItPkmn;
                else
                    stringPtr = sText_YourFoesWeakGetEmPkmn;
            }
            else if (BattlerIsPartner(gBattleScripting.battler)) // Link or Ingame Partner
            {
                if (BattlerIsLink(gBattleScripting.battler) || gBattleTypeFlags & BATTLE_TYPE_RECORDED_LINK)
                    stringPtr = sText_LinkPartnerSentOutPkmn1;
                else
                    stringPtr = sText_InGamePartnerSentOutPkmn1;
            }
            else if (TRAINER_BATTLE_PARAM.opponentA == TRAINER_LINK_OPPONENT
             || gBattleTypeFlags & BATTLE_TYPE_RECORDED_LINK) // Link Opponent A and test opponent
            {
                stringPtr = sText_LinkTrainerSentOutPkmn;
            }
            else // Opponent A
            {
                stringPtr = sText_Trainer1SentOutPkmn;
            }
        }
        else // battler 2 and 3
        {
            // See the battler 0/1 branch above: a partner-controller battler that isn't a
            // real Link or in-game partner is just the AI controlling the player's own mon.
            if (BattlerIsPlayer(gBattleScripting.battler)
             || (BattlerIsPartner(gBattleScripting.battler) && !(gBattleTypeFlags & (BATTLE_TYPE_INGAME_PARTNER | BATTLE_TYPE_LINK | BATTLE_TYPE_RECORDED_LINK)))) // Player
            {
                if (*(&gBattleStruct->hpScale) == 0)
                    stringPtr = sText_GoPkmn2;
                else if (*(&gBattleStruct->hpScale) == 1 || IsDoubleBattle())
                    stringPtr = sText_DoItPkmn;
                else if (*(&gBattleStruct->hpScale) == 2)
                    stringPtr = sText_GoForItPkmn;
                else
                    stringPtr = sText_YourFoesWeakGetEmPkmn;
            }
            else if (BattlerIsPartner(gBattleScripting.battler)) // Link or Ingame Partner
            {
                if (BattlerIsLink(gBattleScripting.battler) || gBattleTypeFlags & BATTLE_TYPE_RECORDED_LINK)
                    stringPtr = sText_LinkPartnerSentOutPkmn2;
                else
                    stringPtr = sText_InGamePartnerSentOutPkmn2;
            }
            else if (TRAINER_BATTLE_PARAM.opponentA == TRAINER_LINK_OPPONENT
             || gBattleTypeFlags & BATTLE_TYPE_RECORDED_LINK) // Link Opponent B and test opponent
            {
                if (BattleSideHasTwoTrainers(B_SIDE_OPPONENT))
                    stringPtr = sText_LinkTrainer2SentOutPkmn2;
                else
                    stringPtr = sText_LinkTrainerSentOutPkmn2;
            }
            else if (gBattleTypeFlags & BATTLE_TYPE_TWO_OPPONENTS) // Opponent B
            {
                stringPtr = sText_Trainer2SentOutPkmn;
            }
            else // Single trainer double Opponent A
            {
                stringPtr = sText_Trainer1SentOutPkmn2;
            }
        }
        break;
    case STRINGID_USEDMOVE: // Pokémon used a move msg
        if (gBattleMsgDataPtr->currentMove >= MOVES_COUNT
         && !IsZMove(gBattleMsgDataPtr->currentMove)
         && !IsMaxMove(gBattleMsgDataPtr->currentMove))
            StringCopy(gBattleTextBuff3, gTypesInfo[*(&gBattleStruct->stringMoveType)].generic);
        else
            StringCopy(gBattleTextBuff3, GetMoveName(gBattleMsgDataPtr->currentMove));
        // Keep messages deterministic for the test runner, which expects exact text.
        if (gTestRunnerEnabled)
            stringPtr = sText_AttackerUsedX;
        else
            stringPtr = sUsedMoveStringVariants[Random() % ARRAY_COUNT(sUsedMoveStringVariants)];
        break;
    case STRINGID_BATTLEEND: // battle end
        if (gBattleTextBuff1[0] & B_OUTCOME_LINK_BATTLE_RAN)
        {
            gBattleTextBuff1[0] &= ~(B_OUTCOME_LINK_BATTLE_RAN);
            if (!(BattlerIsPlayer(battler) || BattlerIsPlayer(GetPartnerBattler(battler))) && gBattleTextBuff1[0] != B_OUTCOME_DREW)
                gBattleTextBuff1[0] ^= (B_OUTCOME_LOST | B_OUTCOME_WON);

            if (gBattleTextBuff1[0] == B_OUTCOME_LOST || gBattleTextBuff1[0] == B_OUTCOME_DREW)
                stringPtr = sText_GotAwaySafely;
            else if (gBattleTypeFlags & BATTLE_TYPE_MULTI)
                stringPtr = sText_TwoWildFled;
            else
                stringPtr = sText_WildFled;
        }
        else
        {
            if (!(BattlerIsPlayer(battler) || BattlerIsPlayer(GetPartnerBattler(battler))) && gBattleTextBuff1[0] != B_OUTCOME_DREW)
                gBattleTextBuff1[0] ^= (B_OUTCOME_LOST | B_OUTCOME_WON);

            if (gBattleTypeFlags & BATTLE_TYPE_MULTI)
            {
                switch (gBattleTextBuff1[0])
                {
                case B_OUTCOME_WON:
                    if (gBattleTypeFlags & BATTLE_TYPE_TOWER_LINK_MULTI)
                        stringPtr = sText_TwoInGameTrainersDefeated;
                    else
                        stringPtr = sText_TwoLinkTrainersDefeated;
                    break;
                case B_OUTCOME_LOST:
                    stringPtr = sText_PlayerLostToTwo;
                    break;
                case B_OUTCOME_DREW:
                    stringPtr = sText_PlayerBattledToDrawVsTwo;
                    break;
                }
            }
            else if (TRAINER_BATTLE_PARAM.opponentA == TRAINER_UNION_ROOM)
            {
                switch (gBattleTextBuff1[0])
                {
                case B_OUTCOME_WON:
                    stringPtr = sText_PlayerDefeatedLinkTrainerTrainer1;
                    break;
                case B_OUTCOME_LOST:
                    stringPtr = sText_PlayerLostAgainstTrainer1;
                    break;
                case B_OUTCOME_DREW:
                    stringPtr = sText_PlayerBattledToDrawTrainer1;
                    break;
                }
            }
            else
            {
                switch (gBattleTextBuff1[0])
                {
                case B_OUTCOME_WON:
                    stringPtr = sText_PlayerDefeatedLinkTrainer;
                    break;
                case B_OUTCOME_LOST:
                    stringPtr = sText_PlayerLostAgainstLinkTrainer;
                    break;
                case B_OUTCOME_DREW:
                    stringPtr = sText_PlayerBattledToDrawLinkTrainer;
                    break;
                }
            }
        }
        break;
    case STRINGID_TRAINERSLIDE:
        stringPtr = gBattleStruct->trainerSlideMsg;
        break;
    default: // load a string from the table
        stringID = GetVariedStringId(stringID);
        // Also guard against string IDs that exist in enum StringID but have no
        // gBattleStringsTable entry, otherwise the NULL would be expanded as text.
        if (stringID >= STRINGID_COUNT || gBattleStringsTable[stringID] == NULL)
        {
            gDisplayedStringBattle[0] = EOS;
            return;
        }
        else
        {
            stringPtr = gBattleStringsTable[stringID];
        }
        break;
    }

    BattleStringExpandPlaceholdersToDisplayedString(stringPtr);
}

u32 BattleStringExpandPlaceholdersToDisplayedString(const u8 *src)
{
#ifndef NDEBUG
    u32 j, strWidth;
    u32 dstID = BattleStringExpandPlaceholders(src, gDisplayedStringBattle, sizeof(gDisplayedStringBattle));
    for (j = 1;; j++)
    {
        strWidth = GetStringLineWidth(0, gDisplayedStringBattle, 0, j, sizeof(gDisplayedStringBattle));
        if (strWidth == 0)
            break;
    }
    return dstID;
#else
    return BattleStringExpandPlaceholders(src, gDisplayedStringBattle, sizeof(gDisplayedStringBattle));
#endif
}

static const u8 *TryGetStatusString(u8 *src)
{
    u32 i;
    u8 status[8];
    u32 chars1, chars2;
    u8 *statusPtr;

    memcpy(status, sText_EmptyStatus, min(ARRAY_COUNT(status), ARRAY_COUNT(sText_EmptyStatus)));

    statusPtr = status;
    for (i = 0; i < ARRAY_COUNT(status); i++)
    {
        if (*src == EOS) break; // one line required to match -g
        *statusPtr = *src;
        src++;
        statusPtr++;
    }

    chars1 = *(u32 *)(&status[0]);
    chars2 = *(u32 *)(&status[4]);

    for (i = 0; i < ARRAY_COUNT(gStatusConditionStringsTable); i++)
    {
        if (chars1 == *(u32 *)(&gStatusConditionStringsTable[i][0][0])
            && chars2 == *(u32 *)(&gStatusConditionStringsTable[i][0][4]))
            return gStatusConditionStringsTable[i][1];
    }
    return NULL;
}

static void GetBattlerNick(enum BattlerId battler, u8 *dst)
{
    struct Pokemon *illusionMon = GetIllusionMonPtr(battler);
    struct Pokemon *mon = GetBattlerMon(battler);

    if (illusionMon != NULL)
        mon = illusionMon;
    GetMonData(mon, MON_DATA_NICKNAME, dst);
    StringGet_Nickname(dst);
}

#define HANDLE_NICKNAME_STRING_CASE(battler)                            \
    if (!IsOnPlayerSide(battler))                                       \
    {                                                                   \
        if (gBattleTypeFlags & BATTLE_TYPE_TRAINER)                     \
            toCpy = sText_FoePkmnPrefix;                                \
        else                                                            \
            toCpy = sText_WildPkmnPrefix;                               \
        while (*toCpy != EOS)                                           \
        {                                                               \
            dst[dstID] = *toCpy;                                        \
            dstID++;                                                    \
            toCpy++;                                                    \
        }                                                               \
    }                                                                   \
    GetBattlerNick(battler, text);                                      \
    toCpy = text;

#define HANDLE_NICKNAME_STRING_LOWERCASE(battler)                       \
    if (!IsOnPlayerSide(battler))                       \
    {                                                                   \
        if (gBattleTypeFlags & BATTLE_TYPE_TRAINER)                     \
            toCpy = sText_FoePkmnPrefixLower;                           \
        else                                                            \
            toCpy = sText_WildPkmnPrefixLower;                          \
        while (*toCpy != EOS)                                           \
        {                                                               \
            dst[dstID] = *toCpy;                                        \
            dstID++;                                                    \
            toCpy++;                                                    \
        }                                                               \
    }                                                                   \
    GetBattlerNick(battler, text);                                      \
    toCpy = text;

static const u8 *BattleStringGetOpponentNameByTrainerId(u16 trainerId, u8 *text, u8 multiplayerId, enum BattlerId battler)
{
    const u8 *toCpy = NULL;

    if (gBattleTypeFlags & BATTLE_TYPE_SECRET_BASE)
    {
        u32 i;
        for (i = 0; i < ARRAY_COUNT(gBattleResources->secretBase->trainerName); i++)
            text[i] = gBattleResources->secretBase->trainerName[i];
        text[i] = EOS;
        ConvertInternationalString(text, gBattleResources->secretBase->language);
        toCpy = text;
    }
    else if (trainerId == TRAINER_UNION_ROOM)
    {
        toCpy = gLinkPlayers[multiplayerId ^ BIT_SIDE].name;
    }
    else if (trainerId == TRAINER_LINK_OPPONENT)
    {
        if (gBattleTypeFlags & BATTLE_TYPE_MULTI)
            toCpy = gLinkPlayers[GetBattlerMultiplayerId(battler)].name;
        else
            toCpy = gLinkPlayers[GetBattlerMultiplayerId(battler) & BIT_SIDE].name;
    }
    else if (trainerId == TRAINER_FRONTIER_BRAIN)
    {
        CopyFrontierBrainTrainerName(text);
        toCpy = text;
    }
    else if (gBattleTypeFlags & BATTLE_TYPE_FRONTIER)
    {
        GetFrontierTrainerName(text, trainerId);
        toCpy = text;
    }
    else if (gBattleTypeFlags & BATTLE_TYPE_TRAINER_TOWER && gMapHeader.regionMapSectionId == MAPSEC_TRAINER_TOWER_2)
    {
        GetTrainerTowerOpponentName(text);
        toCpy = text;
    }
    else if (gBattleTypeFlags & BATTLE_TYPE_TRAINER_HILL)
    {
        GetTrainerHillTrainerName(text, trainerId);
        toCpy = text;
    }
    else if (gBattleTypeFlags & BATTLE_TYPE_EREADER_TRAINER)
    {
        GetEreaderTrainerName(text);
        toCpy = text;
    }
    else
    {
        enum TrainerClassID trainerClass = GetTrainerClassFromId(TRAINER_BATTLE_PARAM.opponentA);

        if (trainerClass == TRAINER_CLASS_RIVAL_EARLY_FRLG || trainerClass == TRAINER_CLASS_RIVAL_LATE_FRLG || trainerClass == TRAINER_CLASS_CHAMPION_FRLG)
            toCpy = GetExpandedPlaceholder(PLACEHOLDER_ID_RIVAL);
        else
            toCpy = GetTrainerNameFromId(trainerId);
    }

    assertf(DoesStringProperlyTerminate(toCpy, TRAINER_NAME_LENGTH + 1),"Opponent needs a valid name")
    {
        return sText_EmptyString4;
    }

    return toCpy;
}

static const u8 *BattleStringGetOpponentName(u8 *text, u8 multiplayerId, enum BattlerId battler)
{
    const u8 *toCpy = NULL;

    switch (GetBattlerPosition(battler))
    {
    case B_POSITION_OPPONENT_LEFT:
        toCpy = BattleStringGetOpponentNameByTrainerId(TRAINER_BATTLE_PARAM.opponentA, text, multiplayerId, battler);
        break;
    case B_POSITION_OPPONENT_RIGHT:
        if (gBattleTypeFlags & (BATTLE_TYPE_TWO_OPPONENTS | BATTLE_TYPE_MULTI) && !BATTLE_TWO_VS_ONE_OPPONENT)
            toCpy = BattleStringGetOpponentNameByTrainerId(TRAINER_BATTLE_PARAM.opponentB, text, multiplayerId, battler);
        else
            toCpy = BattleStringGetOpponentNameByTrainerId(TRAINER_BATTLE_PARAM.opponentA, text, multiplayerId, battler);
        break;
    default:
        break;
    }

    return toCpy;
}

static const u8 *BattleStringGetPlayerName(u8 *text, enum BattlerId battler)
{
    const u8 *toCpy = NULL;

    switch (GetBattlerPosition(battler))
    {
    case B_POSITION_PLAYER_LEFT:
        if (gBattleTypeFlags & BATTLE_TYPE_RECORDED)
            toCpy = gLinkPlayers[0].name;
        else
            toCpy = gSaveBlock2Ptr->playerName;
        break;
    case B_POSITION_PLAYER_RIGHT:
        if (((gBattleTypeFlags & BATTLE_TYPE_RECORDED) && !(gBattleTypeFlags & (BATTLE_TYPE_MULTI | BATTLE_TYPE_INGAME_PARTNER)))
            || gTestRunnerEnabled)
        {
            toCpy = gLinkPlayers[0].name;
        }
        else if ((gBattleTypeFlags & BATTLE_TYPE_LINK) && gBattleTypeFlags & (BATTLE_TYPE_RECORDED | BATTLE_TYPE_MULTI))
        {
            toCpy = gLinkPlayers[2].name;
        }
        else if (gBattleTypeFlags & BATTLE_TYPE_INGAME_PARTNER)
        {
            GetFrontierTrainerName(text, gPartnerTrainerId);
            toCpy = text;
        }
        else
        {
            toCpy = gSaveBlock2Ptr->playerName;
        }
        break;
    default:
        break;
    }

    return toCpy;
}

static const u8 *BattleStringGetTrainerName(u8 *text, u8 multiplayerId, enum BattlerId battler)
{
    if (IsOnPlayerSide(battler))
        return BattleStringGetPlayerName(text, battler);
    else
        return BattleStringGetOpponentName(text, multiplayerId, battler);
}

static const u8 *BattleStringGetOpponentClassByTrainerId(u16 trainerId)
{
    const u8 *toCpy;

    if (gBattleTypeFlags & BATTLE_TYPE_SECRET_BASE)
        toCpy = gTrainerClasses[GetSecretBaseTrainerClass()].name;
    else if (trainerId == TRAINER_UNION_ROOM)
        toCpy = gTrainerClasses[GetUnionRoomTrainerClass()].name;
    else if (trainerId == TRAINER_FRONTIER_BRAIN)
        toCpy = gTrainerClasses[GetFrontierBrainTrainerClass()].name;
    else if (gBattleTypeFlags & BATTLE_TYPE_FRONTIER)
        toCpy = gTrainerClasses[GetFrontierOpponentClass(trainerId)].name;
    else if (gBattleTypeFlags & BATTLE_TYPE_TRAINER_TOWER && gMapHeader.regionMapSectionId == MAPSEC_TRAINER_TOWER_2)
        toCpy = gTrainerClasses[GetTrainerTowerOpponentClass()].name;
    else if (gBattleTypeFlags & BATTLE_TYPE_TRAINER_HILL)
        toCpy = gTrainerClasses[GetTrainerHillOpponentClass(trainerId)].name;
    else if (gBattleTypeFlags & BATTLE_TYPE_EREADER_TRAINER)
        toCpy = gTrainerClasses[GetEreaderTrainerClassId()].name;
    else if (trainerId == TRAINER_LINK_OPPONENT)
        toCpy = gTrainerClasses[TRAINER_NONE].name;
    else
        toCpy = gTrainerClasses[GetTrainerClassFromId(trainerId)].name;

    return toCpy;
}

// Ensure the defined length for an item name can contain the full defined length of a berry name.
// This ensures that custom Enigma Berry names will fit in the text buffer at the top of BattleStringExpandPlaceholders.
STATIC_ASSERT(BERRY_NAME_LENGTH + ARRAY_COUNT(sText_BerrySuffix) <= ITEM_NAME_LENGTH, BerryNameTooLong);

u32 BattleStringExpandPlaceholders(const u8 *src, u8 *dst, u32 dstSize)
{
    u32 dstID = 0; // if they used dstID, why not use srcID as well?
    const u8 *toCpy = NULL;
    u8 text[max(max(max(32, TRAINER_NAME_LENGTH + 1), POKEMON_NAME_LENGTH + 1), ITEM_NAME_LENGTH)];
    u8 *textStart = &text[0];
    u8 multiplayerId;
    u8 fontId = FONT_NORMAL;

    if (gBattleTypeFlags & BATTLE_TYPE_RECORDED_LINK)
        multiplayerId = gRecordedBattleMultiplayerId;
    else
        multiplayerId = GetMultiplayerId();

    // Clear destination first
    while (dstID < dstSize)
    {
        dst[dstID] = EOS;
        dstID++;
    }

    dstID = 0;
    while (*src != EOS)
    {
        toCpy = NULL;

        if (*src == PLACEHOLDER_BEGIN)
        {
            src++;
            u32 classLength = 0;
            u32 nameLength = 0;
            const u8 *classString;
            const u8 *nameString;
            switch (*src)
            {
            case B_TXT_BUFF1:
                if (gBattleTextBuff1[0] == B_BUFF_PLACEHOLDER_BEGIN)
                {
                    ExpandBattleTextBuffPlaceholders(gBattleTextBuff1, gStringVar1);
                    toCpy = gStringVar1;
                }
                else
                {
                    toCpy = TryGetStatusString(gBattleTextBuff1);
                    if (toCpy == NULL)
                        toCpy = gBattleTextBuff1;
                }
                break;
            case B_TXT_BUFF2:
                if (gBattleTextBuff2[0] == B_BUFF_PLACEHOLDER_BEGIN)
                {
                    ExpandBattleTextBuffPlaceholders(gBattleTextBuff2, gStringVar2);
                    toCpy = gStringVar2;
                }
                else
                {
                    toCpy = gBattleTextBuff2;
                }
                break;
            case B_TXT_BUFF3:
                if (gBattleTextBuff3[0] == B_BUFF_PLACEHOLDER_BEGIN)
                {
                    ExpandBattleTextBuffPlaceholders(gBattleTextBuff3, gStringVar3);
                    toCpy = gStringVar3;
                }
                else
                {
                    toCpy = gBattleTextBuff3;
                }
                break;
            case B_TXT_COPY_VAR_1:
                toCpy = gStringVar1;
                break;
            case B_TXT_COPY_VAR_2:
                toCpy = gStringVar2;
                break;
            case B_TXT_COPY_VAR_3:
                toCpy = gStringVar3;
                break;
            case B_TXT_PLAYER_MON1_NAME: // first player poke name
                GetBattlerNick(GetBattlerAtPosition(B_POSITION_PLAYER_LEFT), text);
                toCpy = text;
                break;
            case B_TXT_OPPONENT_MON1_NAME: // first enemy poke name
                GetBattlerNick(GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT), text);
                toCpy = text;
                break;
            case B_TXT_PLAYER_MON2_NAME: // second player poke name
                GetBattlerNick(GetBattlerAtPosition(B_POSITION_PLAYER_RIGHT), text);
                toCpy = text;
                break;
            case B_TXT_OPPONENT_MON2_NAME: // second enemy poke name
                GetBattlerNick(GetBattlerAtPosition(B_POSITION_OPPONENT_RIGHT), text);
                toCpy = text;
                break;
            case B_TXT_LINK_PLAYER_MON1_NAME: // link first player poke name
                GetBattlerNick(GetBattlerAtPosition(B_POSITION_PLAYER_LEFT), text);
                toCpy = text;
                break;
            case B_TXT_LINK_OPPONENT_MON1_NAME: // link first opponent poke name
                GetBattlerNick(GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT), text);
                toCpy = text;
                break;
            case B_TXT_LINK_PLAYER_MON2_NAME: // link second player poke name
                GetBattlerNick(GetBattlerAtPosition(B_POSITION_PLAYER_RIGHT), text);
                toCpy = text;
                break;
            case B_TXT_LINK_OPPONENT_MON2_NAME: // link second opponent poke name
                GetBattlerNick(GetBattlerAtPosition(B_POSITION_OPPONENT_RIGHT), text);
                toCpy = text;
                break;
            case B_TXT_ATK_NAME_WITH_PREFIX_MON1: // Unused, to change into sth else.
                break;
            case B_TXT_ATK_PARTNER_NAME: // attacker partner name
                GetBattlerNick(GetPartnerBattler(gBattlerAttacker), text);
                toCpy = text;
                break;
            case B_TXT_ATK_NAME_WITH_PREFIX: // attacker name with prefix
                HANDLE_NICKNAME_STRING_CASE(gBattlerAttacker)
                break;
            case B_TXT_DEF_NAME_WITH_PREFIX: // target name with prefix
                HANDLE_NICKNAME_STRING_CASE(gBattlerTarget)
                break;
            case B_TXT_DEF_NAME: // target name
                GetBattlerNick(gBattlerTarget, text);
                toCpy = text;
                break;
            case B_TXT_DEF_PARTNER_NAME: // partner target name
                GetBattlerNick(GetPartnerBattler(gBattlerTarget), text);
                toCpy = text;
                break;
            case B_TXT_EFF_NAME_WITH_PREFIX: // effect battler name with prefix
                HANDLE_NICKNAME_STRING_CASE(gEffectBattler)
                break;
            case B_TXT_SCR_ACTIVE_NAME_WITH_PREFIX: // scripting active battler name with prefix
                HANDLE_NICKNAME_STRING_CASE(gBattleScripting.battler)
                break;
            case B_TXT_CURRENT_MOVE: // current move name
                if (gBattleMsgDataPtr->currentMove >= MOVES_COUNT
                 && !IsZMove(gBattleMsgDataPtr->currentMove)
                 && !IsMaxMove(gBattleMsgDataPtr->currentMove))
                    toCpy = gTypesInfo[gBattleStruct->stringMoveType].generic;
                else
                    toCpy = GetMoveName(gBattleMsgDataPtr->currentMove);
                break;
            case B_TXT_LAST_MOVE: // originally used move name
                if (gBattleMsgDataPtr->originallyUsedMove >= MOVES_COUNT
                 && !IsZMove(gBattleMsgDataPtr->currentMove)
                 && !IsMaxMove(gBattleMsgDataPtr->currentMove))
                    toCpy = gTypesInfo[gBattleStruct->stringMoveType].generic;
                else
                    toCpy = GetMoveName(gBattleMsgDataPtr->originallyUsedMove);
                break;
            case B_TXT_LAST_ITEM: // last used item
                if (gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_RECORDED_LINK))
                {
                    if (gLastUsedItem == ITEM_ENIGMA_BERRY_E_READER)
                    {
                        if (!(gBattleTypeFlags & BATTLE_TYPE_MULTI))
                        {
                            if ((gBattleScripting.multiplayerId != 0 && (gPotentialItemEffectBattler & BIT_SIDE))
                                || (gBattleScripting.multiplayerId == 0 && !(gPotentialItemEffectBattler & BIT_SIDE)))
                            {
                                StringCopy(text, gEnigmaBerries[gPotentialItemEffectBattler].name);
                                StringAppend(text, sText_BerrySuffix);
                                toCpy = text;
                            }
                            else
                            {
                                toCpy = sText_EnigmaBerry;
                            }
                        }
                        else
                        {
                            if (gLinkPlayers[gBattleScripting.multiplayerId].id == gPotentialItemEffectBattler)
                            {
                                StringCopy(text, gEnigmaBerries[gPotentialItemEffectBattler].name);
                                StringAppend(text, sText_BerrySuffix);
                                toCpy = text;
                            }
                            else
                            {
                                toCpy = sText_EnigmaBerry;
                            }
                        }
                    }
                    else
                    {
                        CopyItemName(gLastUsedItem, text);
                        toCpy = text;
                    }
                }
                else
                {
                    CopyItemName(gLastUsedItem, text);
                    toCpy = text;
                }
                break;
            case B_TXT_LAST_ABILITY: // last used ability
                toCpy = gAbilitiesInfo[gLastUsedAbility].name;
                break;
            case B_TXT_ATK_ABILITY: // attacker ability
                toCpy = gAbilitiesInfo[sBattlerAbilities[gBattlerAttacker]].name;
                break;
            case B_TXT_DEF_ABILITY: // target ability
                toCpy = gAbilitiesInfo[sBattlerAbilities[gBattlerTarget]].name;
                break;
            case B_TXT_SCR_ACTIVE_ABILITY: // scripting active ability
                toCpy = gAbilitiesInfo[sBattlerAbilities[gBattleScripting.battler]].name;
                break;
            case B_TXT_EFF_ABILITY: // effect battler ability
                toCpy = gAbilitiesInfo[sBattlerAbilities[gEffectBattler]].name;
                break;
            case B_TXT_TRAINER1_CLASS: // trainer class name
                toCpy = BattleStringGetOpponentClassByTrainerId(TRAINER_BATTLE_PARAM.opponentA);
                break;
            case B_TXT_TRAINER1_NAME: // trainer1 name
                toCpy = BattleStringGetOpponentNameByTrainerId(TRAINER_BATTLE_PARAM.opponentA, text, multiplayerId, GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT));
                break;
            case B_TXT_TRAINER1_NAME_WITH_CLASS: // trainer1 name with trainer class
                toCpy = textStart;
                classString = BattleStringGetOpponentClassByTrainerId(TRAINER_BATTLE_PARAM.opponentA);
                while (classString[classLength] != EOS)
                {
                    textStart[classLength] = classString[classLength];
                    classLength++;
                }
                textStart[classLength] = CHAR_SPACE;
                textStart += classLength + 1;
                nameString = BattleStringGetOpponentNameByTrainerId(TRAINER_BATTLE_PARAM.opponentA, textStart, multiplayerId, GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT));
                if (nameString != textStart)
                {
                    while (nameString[nameLength] != EOS)
                    {
                        textStart[nameLength] = nameString[nameLength];
                        nameLength++;
                    }
                    textStart[nameLength] = EOS;
                }
                break;
            case B_TXT_LINK_PLAYER_NAME: // link player name
                toCpy = gLinkPlayers[multiplayerId].name;
                break;
            case B_TXT_LINK_PARTNER_NAME: // link partner name
                toCpy = gLinkPlayers[GetBattlerMultiplayerId(GetPartnerPosition(gLinkPlayers[multiplayerId].id))].name;
                break;
            case B_TXT_LINK_OPPONENT1_NAME: // link opponent 1 name
                toCpy = gLinkPlayers[GetBattlerMultiplayerId(GetBattlerLeftFoe(gLinkPlayers[multiplayerId].id))].name;
                break;
            case B_TXT_LINK_OPPONENT2_NAME: // link opponent 2 name
                toCpy = gLinkPlayers[GetBattlerMultiplayerId(GetBattlerRightFoe(gLinkPlayers[multiplayerId].id))].name;
                break;
            case B_TXT_LINK_SCR_TRAINER_NAME: // link scripting active name
                toCpy = gLinkPlayers[GetBattlerMultiplayerId(gBattleScripting.battler)].name;
                break;
            case B_TXT_PLAYER_NAME: // player name
                toCpy = BattleStringGetPlayerName(text, GetBattlerAtPosition(B_POSITION_PLAYER_LEFT));
                break;
            case B_TXT_TRAINER1_LOSE_TEXT: // trainerA lose text
                if (gBattleTypeFlags & BATTLE_TYPE_FRONTIER)
                {
                    CopyFrontierTrainerText(FRONTIER_PLAYER_WON_TEXT, TRAINER_BATTLE_PARAM.opponentA);
                    toCpy = gStringVar4;
                }
                else if (gBattleTypeFlags & BATTLE_TYPE_TRAINER_TOWER && gMapHeader.regionMapSectionId == MAPSEC_TRAINER_TOWER_2)
                {
                    GetTrainerTowerOpponentLoseText(gStringVar4, 0);
                    toCpy = gStringVar4;
                }
                else if (gBattleTypeFlags & BATTLE_TYPE_TRAINER_HILL)
                {
                    CopyTrainerHillTrainerText(TRAINER_HILL_TEXT_PLAYER_WON, TRAINER_BATTLE_PARAM.opponentA);
                    toCpy = gStringVar4;
                }
                else
                {
                    toCpy = GetTrainerALoseText();
                }
                break;
            case B_TXT_TRAINER1_WIN_TEXT: // trainerA win text
                if (gBattleTypeFlags & BATTLE_TYPE_FRONTIER)
                {
                    CopyFrontierTrainerText(FRONTIER_PLAYER_LOST_TEXT, TRAINER_BATTLE_PARAM.opponentA);
                    toCpy = gStringVar4;
                }
                else if (gBattleTypeFlags & BATTLE_TYPE_TRAINER_TOWER && gMapHeader.regionMapSectionId == MAPSEC_TRAINER_TOWER_2)
                {
                    GetTrainerTowerOpponentWinText(gStringVar4, 0);
                    toCpy = gStringVar4;
                }
                else if (gBattleTypeFlags & BATTLE_TYPE_TRAINER_HILL)
                {
                    CopyTrainerHillTrainerText(TRAINER_HILL_TEXT_PLAYER_LOST, TRAINER_BATTLE_PARAM.opponentA);
                    toCpy = gStringVar4;
                }
                else
                {
                    toCpy = GetTrainerWonSpeech();
                }
                break;
            case B_TXT_26: // ?
                if (!IsOnPlayerSide(gBattleScripting.battler))
                {
                    if (gBattleTypeFlags & BATTLE_TYPE_TRAINER)
                        toCpy = sText_FoePkmnPrefix;
                    else
                        toCpy = sText_WildPkmnPrefix;
                    while (*toCpy != EOS)
                    {
                        dst[dstID] = *toCpy;
                        dstID++;
                        toCpy++;
                    }
                }
                GetMonData(&GetBattlerParty(gBattleScripting.battler)[gBattleStruct->scriptPartyIdx], MON_DATA_NICKNAME, text);
                StringGet_Nickname(text);
                toCpy = text;
                break;
            case B_TXT_PC_CREATOR_NAME: // lanette pc
                if (FlagGet(FLAG_SYS_PC_LANETTE))
                    toCpy = IS_FRLG ? sText_Bills : sText_Lanettes;
                else
                    toCpy = sText_Someones;
                break;
            case B_TXT_ATK_PREFIX2:
                if (IsOnPlayerSide(gBattlerAttacker))
                    toCpy = sText_AllyPkmnPrefix2;
                else
                    toCpy = sText_FoePkmnPrefix3;
                break;
            case B_TXT_DEF_PREFIX2:
                if (IsOnPlayerSide(gBattlerTarget))
                    toCpy = sText_AllyPkmnPrefix2;
                else
                    toCpy = sText_FoePkmnPrefix3;
                break;
            case B_TXT_ATK_PREFIX1:
                if (IsOnPlayerSide(gBattlerAttacker))
                    toCpy = sText_AllyPkmnPrefix;
                else
                    toCpy = sText_FoePkmnPrefix2;
                break;
            case B_TXT_DEF_PREFIX1:
                if (IsOnPlayerSide(gBattlerTarget))
                    toCpy = sText_AllyPkmnPrefix;
                else
                    toCpy = sText_FoePkmnPrefix2;
                break;
            case B_TXT_ATK_PREFIX3:
                if (IsOnPlayerSide(gBattlerAttacker))
                    toCpy = sText_AllyPkmnPrefix3;
                else
                    toCpy = sText_FoePkmnPrefix4;
                break;
            case B_TXT_DEF_PREFIX3:
                if (IsOnPlayerSide(gBattlerTarget))
                    toCpy = sText_AllyPkmnPrefix3;
                else
                    toCpy = sText_FoePkmnPrefix4;
                break;
            case B_TXT_TRAINER2_CLASS:
                toCpy = BattleStringGetOpponentClassByTrainerId(TRAINER_BATTLE_PARAM.opponentB);
                break;
            case B_TXT_TRAINER2_NAME:
                toCpy = BattleStringGetOpponentNameByTrainerId(TRAINER_BATTLE_PARAM.opponentB, text, multiplayerId, GetBattlerAtPosition(B_POSITION_OPPONENT_RIGHT));
                break;
            case B_TXT_TRAINER2_NAME_WITH_CLASS:
                toCpy = textStart;
                classString = BattleStringGetOpponentClassByTrainerId(TRAINER_BATTLE_PARAM.opponentB);
                while (classString[classLength] != EOS)
                {
                    textStart[classLength] = classString[classLength];
                    classLength++;
                }
                textStart[classLength] = CHAR_SPACE;
                textStart += classLength + 1;
                nameString = BattleStringGetOpponentNameByTrainerId(TRAINER_BATTLE_PARAM.opponentB, textStart, multiplayerId, GetBattlerAtPosition(B_POSITION_OPPONENT_RIGHT));
                if (nameString != textStart)
                {
                    while (nameString[nameLength] != EOS)
                    {
                        textStart[nameLength] = nameString[nameLength];
                        nameLength++;
                    }
                    textStart[nameLength] = EOS;
                }
                break;
            case B_TXT_TRAINER2_LOSE_TEXT:
                if (gBattleTypeFlags & BATTLE_TYPE_FRONTIER)
                {
                    CopyFrontierTrainerText(FRONTIER_PLAYER_WON_TEXT, TRAINER_BATTLE_PARAM.opponentB);
                    toCpy = gStringVar4;
                }
                else if (gBattleTypeFlags & BATTLE_TYPE_TRAINER_TOWER && gMapHeader.regionMapSectionId == MAPSEC_TRAINER_TOWER_2)
                {
                    GetTrainerTowerOpponentLoseText(gStringVar4, 1);
                    toCpy = gStringVar4;
                }
                else if (gBattleTypeFlags & BATTLE_TYPE_TRAINER_HILL)
                {
                    CopyTrainerHillTrainerText(TRAINER_HILL_TEXT_PLAYER_WON, TRAINER_BATTLE_PARAM.opponentB);
                    toCpy = gStringVar4;
                }
                else
                {
                    toCpy = GetTrainerBLoseText();
                }
                break;
            case B_TXT_TRAINER2_WIN_TEXT:
                if (gBattleTypeFlags & BATTLE_TYPE_FRONTIER)
                {
                    CopyFrontierTrainerText(FRONTIER_PLAYER_LOST_TEXT, TRAINER_BATTLE_PARAM.opponentB);
                    toCpy = gStringVar4;
                }
                else if (gBattleTypeFlags & BATTLE_TYPE_TRAINER_TOWER && gMapHeader.regionMapSectionId == MAPSEC_TRAINER_TOWER_2)
                {
                    GetTrainerTowerOpponentWinText(gStringVar4, 1);
                    toCpy = gStringVar4;
                }
                else if (gBattleTypeFlags & BATTLE_TYPE_TRAINER_HILL)
                {
                    CopyTrainerHillTrainerText(TRAINER_HILL_TEXT_PLAYER_LOST, TRAINER_BATTLE_PARAM.opponentB);
                    toCpy = gStringVar4;
                }
                break;
            case B_TXT_PARTNER_CLASS:
                toCpy = gTrainerClasses[GetFrontierOpponentClass(gPartnerTrainerId)].name;
                break;
            case B_TXT_PARTNER_NAME:
                toCpy = BattleStringGetPlayerName(text, GetBattlerAtPosition(B_POSITION_PLAYER_RIGHT));
                break;
            case B_TXT_PARTNER_NAME_WITH_CLASS:
                toCpy = textStart;
                classString = gTrainerClasses[GetFrontierOpponentClass(gPartnerTrainerId)].name;
                while (classString[classLength] != EOS)
                {
                    textStart[classLength] = classString[classLength];
                    classLength++;
                }
                textStart[classLength] = CHAR_SPACE;
                textStart += classLength + 1;
                nameString = BattleStringGetPlayerName(textStart, GetBattlerAtPosition(B_POSITION_PLAYER_RIGHT));
                if (nameString != textStart)
                {
                    while (nameString[nameLength] != EOS)
                    {
                        textStart[nameLength] = nameString[nameLength];
                        nameLength++;
                    }
                    textStart[nameLength] = EOS;
                }
                break;
            case B_TXT_ATK_TRAINER_NAME:
                toCpy = BattleStringGetTrainerName(text, multiplayerId, gBattlerAttacker);
                break;
            case B_TXT_ATK_TRAINER_CLASS:
                switch (GetBattlerPosition(gBattlerAttacker))
                {
                case B_POSITION_PLAYER_RIGHT:
                    if (gBattleTypeFlags & BATTLE_TYPE_INGAME_PARTNER)
                        toCpy = gTrainerClasses[GetFrontierOpponentClass(gPartnerTrainerId)].name;
                    break;
                case B_POSITION_OPPONENT_LEFT:
                    toCpy = BattleStringGetOpponentClassByTrainerId(TRAINER_BATTLE_PARAM.opponentA);
                    break;
                case B_POSITION_OPPONENT_RIGHT:
                    if (gBattleTypeFlags & BATTLE_TYPE_TWO_OPPONENTS && !BATTLE_TWO_VS_ONE_OPPONENT)
                        toCpy = BattleStringGetOpponentClassByTrainerId(TRAINER_BATTLE_PARAM.opponentB);
                    else
                        toCpy = BattleStringGetOpponentClassByTrainerId(TRAINER_BATTLE_PARAM.opponentA);
                    break;
                default:
                    break;
                }
                break;
            case B_TXT_ATK_TRAINER_NAME_WITH_CLASS:
                toCpy = textStart;
                if (gBattleTypeFlags & BATTLE_TYPE_CATCH_TUTORIAL)
                {
                    if (IS_FRLG)
                        textStart = StringCopy(textStart, COMPOUND_STRING("The old man"));
                    else
                        textStart = StringCopy(textStart, COMPOUND_STRING("WALLY"));
                }
                else if (GetBattlerPosition(gBattlerAttacker) == B_POSITION_PLAYER_LEFT)
                {
                    textStart = StringCopy(textStart, BattleStringGetTrainerName(textStart, multiplayerId, gBattlerAttacker));
                }
                else
                {
                    classString = NULL;
                    switch (GetBattlerPosition(gBattlerAttacker))
                    {
                    case B_POSITION_PLAYER_RIGHT:
                        if (gBattleTypeFlags & BATTLE_TYPE_INGAME_PARTNER)
                            classString = gTrainerClasses[GetFrontierOpponentClass(gPartnerTrainerId)].name;
                        break;
                    case B_POSITION_OPPONENT_LEFT:
                        classString = BattleStringGetOpponentClassByTrainerId(TRAINER_BATTLE_PARAM.opponentA);
                        break;
                    case B_POSITION_OPPONENT_RIGHT:
                        if (gBattleTypeFlags & BATTLE_TYPE_TWO_OPPONENTS && !BATTLE_TWO_VS_ONE_OPPONENT)
                            classString = BattleStringGetOpponentClassByTrainerId(TRAINER_BATTLE_PARAM.opponentB);
                        else
                            classString = BattleStringGetOpponentClassByTrainerId(TRAINER_BATTLE_PARAM.opponentA);
                        break;
                    default:
                        break;
                    }
                    classLength = 0;
                    nameLength = 0;
                    while (classString[classLength] != EOS)
                    {
                        textStart[classLength] = classString[classLength];
                        classLength++;
                    }
                    textStart[classLength] = CHAR_SPACE;
                    textStart += 1 + classLength;
                    nameString = BattleStringGetTrainerName(textStart, multiplayerId, gBattlerAttacker);
                    if (nameString != textStart)
                    {
                        while (nameString[nameLength] != EOS)
                        {
                            textStart[nameLength] = nameString[nameLength];
                            nameLength++;
                        }
                        textStart[nameLength] = EOS;
                    }
                }
                break;
            case B_TXT_ATK_TEAM1:
                if (IsOnPlayerSide(gBattlerAttacker))
                    toCpy = sText_Your1;
                else
                    toCpy = sText_Opposing1;
                break;
            case B_TXT_ATK_TEAM2:
                if (IsOnPlayerSide(gBattlerAttacker))
                    toCpy = sText_Your2;
                else
                    toCpy = sText_Opposing2;
                break;
            case B_TXT_DEF_TEAM1:
                if (IsOnPlayerSide(gBattlerTarget))
                    toCpy = sText_Your1;
                else
                    toCpy = sText_Opposing1;
                break;
            case B_TXT_DEF_TEAM2:
                if (IsOnPlayerSide(gBattlerTarget))
                    toCpy = sText_Your2;
                else
                    toCpy = sText_Opposing2;
                break;
            case B_TXT_EFF_TEAM1:
                if (IsOnPlayerSide(gEffectBattler))
                    toCpy = sText_Your1;
                else
                    toCpy = sText_Opposing1;
                break;
            case B_TXT_EFF_TEAM2:
                if (IsOnPlayerSide(gEffectBattler))
                    toCpy = sText_Your2;
                else
                    toCpy = sText_Opposing2;
                break;
            case B_TXT_ATK_NAME_WITH_PREFIX2:
                HANDLE_NICKNAME_STRING_LOWERCASE(gBattlerAttacker)
                break;
            case B_TXT_DEF_NAME_WITH_PREFIX2:
                HANDLE_NICKNAME_STRING_LOWERCASE(gBattlerTarget)
                break;
            case B_TXT_EFF_NAME_WITH_PREFIX2:
                HANDLE_NICKNAME_STRING_LOWERCASE(gEffectBattler)
                break;
            case B_TXT_SCR_ACTIVE_NAME_WITH_PREFIX2:
                HANDLE_NICKNAME_STRING_LOWERCASE(gBattleScripting.battler)
                break;
            }

            if (toCpy != NULL)
            {
                while (*toCpy != EOS)
                {
                    if (*toCpy == CHAR_SPACE)
                        dst[dstID] = CHAR_NBSP;
                    else
                        dst[dstID] = *toCpy;
                    dstID++;
                    toCpy++;
                }
            }

            if (*src == B_TXT_TRAINER1_LOSE_TEXT || *src == B_TXT_TRAINER2_LOSE_TEXT
                || *src == B_TXT_TRAINER1_WIN_TEXT || *src == B_TXT_TRAINER2_WIN_TEXT)
            {
                dst[dstID] = EXT_CTRL_CODE_BEGIN;
                dstID++;
                dst[dstID] = EXT_CTRL_CODE_PAUSE_UNTIL_PRESS;
                dstID++;
            }
        }
        else
        {
            dst[dstID] = *src;
            dstID++;
        }
        src++;
    }

    dst[dstID] = *src;
    dstID++;

    BreakStringAutomatic(dst, BATTLE_MSG_MAX_WIDTH, BATTLE_MSG_MAX_LINES, fontId, TRUE);

    return dstID;
}

static void IllusionNickHack(enum BattlerId battler, u32 partyId, u8 *dst)
{
    u32 id = PARTY_SIZE;
    struct Pokemon *party = GetBattlerParty(battler);
    struct Pokemon *mon = &party[partyId], *partnerMon;

    if (GetMonAbility(mon) == ABILITY_ILLUSION)
    {
        if (IsBattlerAlive(GetPartnerBattler(battler)))
            partnerMon = GetBattlerMon(GetPartnerBattler(battler));
        else
            partnerMon = mon;

        id = GetIllusionMonPartyId(party, mon, partnerMon, battler);
    }

    if (id != PARTY_SIZE)
        GetMonData(&party[id], MON_DATA_NICKNAME, dst);
    else
        GetMonData(mon, MON_DATA_NICKNAME, dst);
}

void ExpandBattleTextBuffPlaceholders(const u8 *src, u8 *dst)
{
    u32 srcID = 1;
    u32 value = 0;
    u8 nickname[POKEMON_NAME_LENGTH + 1];
    u16 hword;

    *dst = EOS;
    while (src[srcID] != B_BUFF_EOS)
    {
        switch (src[srcID])
        {
        case B_BUFF_STRING: // battle string
            hword = T1_READ_16(&src[srcID + 1]);
            StringAppend(dst, gBattleStringsTable[hword]);
            srcID += 3;
            break;
        case B_BUFF_NUMBER: // int to string
            switch (src[srcID + 1])
            {
            case 1:
                value = src[srcID + 3];
                break;
            case 2:
                value = T1_READ_16(&src[srcID + 3]);
                break;
            case 4:
                value = T1_READ_32(&src[srcID + 3]);
                break;
            }
            ConvertIntToDecimalStringN(dst, value, STR_CONV_MODE_LEFT_ALIGN, src[srcID + 2]);
            srcID += src[srcID + 1] + 3;
            break;
        case B_BUFF_MOVE: // move name
            StringAppend(dst, GetMoveName(T1_READ_16(&src[srcID + 1])));
            srcID += 3;
            break;
        case B_BUFF_TYPE: // type name
            StringAppend(dst, gTypesInfo[src[srcID + 1]].name);
            srcID += 2;
            break;
        case B_BUFF_MON_NICK_WITH_PREFIX: // poke nick with prefix
        case B_BUFF_MON_NICK_WITH_PREFIX_LOWER: // poke nick with lowercase prefix
            if (!IsOnPlayerSide(src[srcID + 1]))
            {
                if (src[srcID] == B_BUFF_MON_NICK_WITH_PREFIX_LOWER)
                {
                    if (gBattleTypeFlags & BATTLE_TYPE_TRAINER)
                        StringAppend(dst, sText_FoePkmnPrefixLower);
                    else
                        StringAppend(dst, sText_WildPkmnPrefixLower);
                }
                else
                {
                    if (gBattleTypeFlags & BATTLE_TYPE_TRAINER)
                        StringAppend(dst, sText_FoePkmnPrefix);
                    else
                        StringAppend(dst, sText_WildPkmnPrefix);
                }
            }
            GetMonData(&GetBattlerParty(src[srcID + 1])[src[srcID + 2]], MON_DATA_NICKNAME, nickname);
            StringGet_Nickname(nickname);
            StringAppend(dst, nickname);
            srcID += 3;
            break;
        case B_BUFF_STAT: // stats
            StringAppend(dst, gStatNamesTable[src[srcID + 1]]);
            srcID += 2;
            break;
        case B_BUFF_SPECIES: // species name
            StringCopy(dst, GetSpeciesName(T1_READ_16(&src[srcID + 1])));
            srcID += 3;
            break;
        case B_BUFF_MON_NICK: // poke nick without prefix
            if (src[srcID + 2] == gBattlerPartyIndexes[src[srcID + 1]])
            {
                GetBattlerNick(src[srcID + 1], dst);
            }
            else if (gBattleScripting.illusionNickHack) // for STRINGID_ENEMYABOUTTOSWITCHPKMN
            {
                gBattleScripting.illusionNickHack = 0;
                IllusionNickHack(src[srcID + 1], src[srcID + 2], dst);
                StringGet_Nickname(dst);
            }
            else
            {
                GetMonData(&GetBattlerParty(src[srcID + 1])[src[srcID + 2]], MON_DATA_NICKNAME, dst);
                StringGet_Nickname(dst);
            }
            srcID += 3;
            break;
        case B_BUFF_NEGATIVE_FLAVOR: // flavor table
            StringAppend(dst, gPokeblockWasTooXStringTable[src[srcID + 1]]);
            srcID += 2;
            break;
        case B_BUFF_ABILITY: // ability names
            StringAppend(dst, gAbilitiesInfo[T1_READ_16(&src[srcID + 1])].name);
            srcID += 3;
            break;
        case B_BUFF_ITEM: // item name
            hword = T1_READ_16(&src[srcID + 1]);
            if (gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_RECORDED_LINK))
            {
                if (hword == ITEM_ENIGMA_BERRY_E_READER)
                {
                    if (gLinkPlayers[gBattleScripting.multiplayerId].id == gPotentialItemEffectBattler)
                    {
                        StringCopy(dst, gEnigmaBerries[gPotentialItemEffectBattler].name);
                        StringAppend(dst, sText_BerrySuffix);
                    }
                    else
                    {
                        StringAppend(dst, sText_EnigmaBerry);
                    }
                }
                else
                {
                    CopyItemName(hword, dst);
                }
            }
            else
            {
                CopyItemName(hword, dst);
            }
            srcID += 3;
            break;
        }
    }
}

void BattlePutTextOnWindow(const u8 *text, u8 windowId)
{
    const struct BattleWindowText *textInfo = sBattleTextOnWindowsInfo[gBattleScripting.windowsType];
    bool32 copyToVram;
    struct TextPrinterTemplate printerTemplate;
    u8 speed;

    if (windowId & B_WIN_COPYTOVRAM)
    {
        windowId &= ~B_WIN_COPYTOVRAM;
        copyToVram = FALSE;
    }
    else
    {
        FillWindowPixelBuffer(windowId, textInfo[windowId].fillValue);
        copyToVram = TRUE;
    }

    printerTemplate.currentChar = text;
    printerTemplate.type = WINDOW_TEXT_PRINTER;
    printerTemplate.windowId = windowId;
    printerTemplate.fontId = textInfo[windowId].fontId;
    printerTemplate.x = textInfo[windowId].x;
    printerTemplate.y = textInfo[windowId].y;
    printerTemplate.currentX = printerTemplate.x;
    printerTemplate.currentY = printerTemplate.y;
    printerTemplate.letterSpacing = textInfo[windowId].letterSpacing;
    printerTemplate.lineSpacing = textInfo[windowId].lineSpacing;
    printerTemplate.color = textInfo[windowId].color;

    if (B_WIN_MOVE_NAME_1 <= windowId && windowId <= B_WIN_MOVE_NAME_4)
    {
        // We cannot check the actual width of the window because
        // B_WIN_MOVE_NAME_1 and B_WIN_MOVE_NAME_3 are 16 wide for
        // Z-move details.
        if (gBattleStruct->zmove.viewing && windowId == B_WIN_MOVE_NAME_1)
            printerTemplate.fontId = GetFontIdToFit(text, printerTemplate.fontId, printerTemplate.letterSpacing, 16 * TILE_WIDTH);
        else
            printerTemplate.fontId = GetFontIdToFit(text, printerTemplate.fontId, printerTemplate.letterSpacing, 8 * TILE_WIDTH);
    }

    if (printerTemplate.x == 0xFF)
    {
        u32 width = GetBattleWindowTemplatePixelWidth(gBattleScripting.windowsType, windowId);
        s32 alignX = GetStringCenterAlignXOffsetWithLetterSpacing(printerTemplate.fontId, printerTemplate.currentChar, width, printerTemplate.letterSpacing);
        printerTemplate.x = printerTemplate.currentX = alignX;
    }

    if (windowId == ARENA_WIN_JUDGMENT_TEXT || windowId == B_WIN_OAK_OLD_MAN)
        gTextFlags.useAlternateDownArrow = FALSE;
    else
        gTextFlags.useAlternateDownArrow = TRUE;

    if ((gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_RECORDED)) || gTestRunnerEnabled || ((gBattleTypeFlags & BATTLE_TYPE_POKEDUDE) && windowId != B_WIN_OAK_OLD_MAN))
        gTextFlags.autoScroll = TRUE;
    else
        gTextFlags.autoScroll = FALSE;

    if (windowId == B_WIN_MSG || windowId == ARENA_WIN_JUDGMENT_TEXT || windowId == B_WIN_OAK_OLD_MAN)
    {
        if (gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_RECORDED_LINK))
            speed = 1;
        else if (gBattleTypeFlags & BATTLE_TYPE_RECORDED)
            speed = sRecordedBattleTextSpeeds[GetTextSpeedInRecordedBattle()];
        else
            speed = GetPlayerTextSpeedDelay();

        gTextFlags.canABSpeedUpPrint = 1;
    }
    else
    {
        speed = textInfo[windowId].speed;
        gTextFlags.canABSpeedUpPrint = 0;
    }

    AddTextPrinter(&printerTemplate, speed, NULL);

    if (copyToVram)
    {
        PutWindowTilemap(windowId);
        CopyWindowToVram(windowId, COPYWIN_FULL);
    }
}

void SetPPNumbersPaletteInMoveSelection(enum BattlerId battler)
{
    struct ChooseMoveStruct *chooseMoveStruct = (struct ChooseMoveStruct *)(&gBattleResources->bufferA[battler][4]);
    const u16 *palPtr = gPPTextPalette;
    u8 var;

    if (!gBattleStruct->zmove.viewing)
        var = GetCurrentPPToMaxPPState(chooseMoveStruct->currentPP[gMoveSelectionCursor[battler]],
                         chooseMoveStruct->maxPP[gMoveSelectionCursor[battler]]);
    else
        var = 3;

    gPlttBufferUnfaded[BG_PLTT_ID(5) + 12] = palPtr[(var * 2) + 0];
    gPlttBufferUnfaded[BG_PLTT_ID(5) + 11] = palPtr[(var * 2) + 1];

    CpuCopy16(&gPlttBufferUnfaded[BG_PLTT_ID(5) + 12], &gPlttBufferFaded[BG_PLTT_ID(5) + 12], PLTT_SIZEOF(1));
    CpuCopy16(&gPlttBufferUnfaded[BG_PLTT_ID(5) + 11], &gPlttBufferFaded[BG_PLTT_ID(5) + 11], PLTT_SIZEOF(1));
}

u8 GetCurrentPPToMaxPPState(u8 currentPP, u8 maxPP)
{
    if (maxPP == currentPP)
    {
        return 3;
    }
    else if (maxPP <= 2)
    {
        if (currentPP > 1)
            return 3;
        else
            return 2 - currentPP;
    }
    else if (maxPP <= 7)
    {
        if (currentPP > 2)
            return 3;
        else
            return 2 - currentPP;
    }
    else
    {
        if (currentPP == 0)
            return 2;
        if (currentPP <= maxPP / 4)
            return 1;
        if (currentPP > maxPP / 2)
            return 3;
    }

    return 0;
}
