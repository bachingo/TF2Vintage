#include "cbase.h"
#include "tf_gamerules.h"
#include "takedamageinfo.h"
#include "vscript_shared.h"
#ifdef GAME_DLL
#include "player_voice_listener.h"
#endif


BEGIN_SCRIPTENUM( ECritType, "" )

	DEFINE_ENUMCONST( CTakeDamageInfo::CRIT_NONE, "" )
	DEFINE_ENUMCONST( CTakeDamageInfo::CRIT_MINI, "" )
	DEFINE_ENUMCONST( CTakeDamageInfo::CRIT_FULL, "" )

END_SCRIPTENUM()

//=============================================================================
//=============================================================================

BEGIN_SCRIPTENUM( EHoliday, "" )

	DEFINE_ENUMCONST( kHoliday_None, "" )
	DEFINE_ENUMCONST( kHoliday_TFBirthday, "" )
	DEFINE_ENUMCONST( kHoliday_Halloween, "" )
	DEFINE_ENUMCONST( kHoliday_Christmas, "" )
	DEFINE_ENUMCONST( kHoliday_CommunityUpdate, "" )
	DEFINE_ENUMCONST( kHoliday_EOTL, "" )
	DEFINE_ENUMCONST( kHoliday_Valentines, "" )
	DEFINE_ENUMCONST( kHoliday_MeetThePyro, "" )
	DEFINE_ENUMCONST( kHoliday_FullMoon, "" )
	DEFINE_ENUMCONST( kHoliday_HalloweenOrFullMoon, "" )
	DEFINE_ENUMCONST( kHoliday_HalloweenOrFullMoonOrValentines, "" )
	DEFINE_ENUMCONST( kHoliday_AprilFools, "" )
	DEFINE_ENUMCONST( kHoliday_Soldier, "" )
	DEFINE_ENUMCONST( kHolidayCount, "" )

END_SCRIPTENUM()

//=============================================================================
//=============================================================================

BEGIN_SCRIPTENUM( ETFTeam, "" )

	DEFINE_ENUMCONST( TEAM_ANY, "" )
	DEFINE_ENUMCONST( TEAM_INVALID, "" )
	DEFINE_ENUMCONST( TEAM_UNASSIGNED, "" )
	DEFINE_ENUMCONST( TEAM_SPECTATOR, "" )
	DEFINE_ENUMCONST( TF_TEAM_RED, "" )
	DEFINE_ENUMCONST( TF_TEAM_BLUE, "" )
	DEFINE_ENUMCONST( TF_TEAM_GREEN, "" )
	DEFINE_ENUMCONST( TF_TEAM_YELLOW, "" )
	DEFINE_ENUMCONST( TF_TEAM_PVE_DEFENDERS, "" )
	DEFINE_ENUMCONST( TF_TEAM_PVE_INVADERS, "" )
	DEFINE_ENUMCONST( TF_TEAM_PVE_INVADERS_GIANTS, "" )
	DEFINE_ENUMCONST( TF_TEAM_COUNT, "" )

END_SCRIPTENUM()

//=============================================================================
//=============================================================================

BEGIN_SCRIPTENUM( ETFClass, "" )

	DEFINE_ENUMCONST( TF_CLASS_UNDEFINED, "" )
	DEFINE_ENUMCONST( TF_CLASS_SCOUT, "" )
	DEFINE_ENUMCONST( TF_CLASS_SNIPER, "" )
	DEFINE_ENUMCONST( TF_CLASS_SOLDIER, "" )
	DEFINE_ENUMCONST( TF_CLASS_DEMOMAN, "" )
	DEFINE_ENUMCONST( TF_CLASS_MEDIC, "" )
	DEFINE_ENUMCONST( TF_CLASS_HEAVYWEAPONS, "" )
	DEFINE_ENUMCONST( TF_CLASS_PYRO, "" )
	DEFINE_ENUMCONST( TF_CLASS_SPY, "" )
	DEFINE_ENUMCONST( TF_CLASS_ENGINEER, "" )
	DEFINE_ENUMCONST( TF_CLASS_COUNT_ALL, "" )
	DEFINE_ENUMCONST( TF_CLASS_RANDOM, "" )

END_SCRIPTENUM()

//=============================================================================
//=============================================================================

BEGIN_SCRIPTENUM( ETFCond, "" )

	DEFINE_ENUMCONST( TF_COND_INVALID, "" )
	DEFINE_ENUMCONST( TF_COND_AIMING, "" )
	DEFINE_ENUMCONST( TF_COND_ZOOMED, "" )
	DEFINE_ENUMCONST( TF_COND_DISGUISING, "" )
	DEFINE_ENUMCONST( TF_COND_DISGUISED, "" )
	DEFINE_ENUMCONST( TF_COND_STEALTHED, "" )
	DEFINE_ENUMCONST( TF_COND_INVULNERABLE, "" )
	DEFINE_ENUMCONST( TF_COND_TELEPORTED, "" )
	DEFINE_ENUMCONST( TF_COND_TAUNTING, "" )
	DEFINE_ENUMCONST( TF_COND_INVULNERABLE_WEARINGOFF, "" )
	DEFINE_ENUMCONST( TF_COND_STEALTHED_BLINK, "" )
	DEFINE_ENUMCONST( TF_COND_SELECTED_TO_TELEPORT, "" )
	DEFINE_ENUMCONST( TF_COND_CRITBOOSTED, "" )
	DEFINE_ENUMCONST( TF_COND_TMPDAMAGEBONUS, "" )
	DEFINE_ENUMCONST( TF_COND_FEIGN_DEATH, "" )
	DEFINE_ENUMCONST( TF_COND_PHASE, "" )
	DEFINE_ENUMCONST( TF_COND_STUNNED, "" )
	DEFINE_ENUMCONST( TF_COND_OFFENSEBUFF, "" )
	DEFINE_ENUMCONST( TF_COND_SHIELD_CHARGE, "" )
	DEFINE_ENUMCONST( TF_COND_DEMO_BUFF, "" )
	DEFINE_ENUMCONST( TF_COND_ENERGY_BUFF, "" )
	DEFINE_ENUMCONST( TF_COND_RADIUSHEAL, "" )
	DEFINE_ENUMCONST( TF_COND_HEALTH_BUFF, "" )
	DEFINE_ENUMCONST( TF_COND_BURNING, "" )
	DEFINE_ENUMCONST( TF_COND_HEALTH_OVERHEALED, "" )
	DEFINE_ENUMCONST( TF_COND_URINE, "" )
	DEFINE_ENUMCONST( TF_COND_BLEEDING, "" )
	DEFINE_ENUMCONST( TF_COND_DEFENSEBUFF, "" )
	DEFINE_ENUMCONST( TF_COND_MAD_MILK, "" )
	DEFINE_ENUMCONST( TF_COND_MEGAHEAL, "" )
	DEFINE_ENUMCONST( TF_COND_REGENONDAMAGEBUFF, "" )
	DEFINE_ENUMCONST( TF_COND_MARKEDFORDEATH, "" )
	DEFINE_ENUMCONST( TF_COND_NOHEALINGDAMAGEBUFF, "" )
	DEFINE_ENUMCONST( TF_COND_SPEED_BOOST, "" )
	DEFINE_ENUMCONST( TF_COND_CRITBOOSTED_PUMPKIN, "" )
	DEFINE_ENUMCONST( TF_COND_CRITBOOSTED_USER_BUFF, "" )
	DEFINE_ENUMCONST( TF_COND_SODAPOPPER_HYPE, "" )
	DEFINE_ENUMCONST( TF_COND_CRITBOOSTED_FIRST_BLOOD, "" )
	DEFINE_ENUMCONST( TF_COND_CRITBOOSTED_BONUS_TIME, "" )
	DEFINE_ENUMCONST( TF_COND_CRITBOOSTED_CTF_CAPTURE, "" )
	DEFINE_ENUMCONST( TF_COND_CRITBOOSTED_ON_KILL, "" )
	DEFINE_ENUMCONST( TF_COND_CANNOT_SWITCH_FROM_MELEE, "" )
	DEFINE_ENUMCONST( TF_COND_DEFENSEBUFF_NO_CRIT_BLOCK, "" )
	DEFINE_ENUMCONST( TF_COND_REPROGRAMMED, "" )
	DEFINE_ENUMCONST( TF_COND_CRITBOOSTED_RAGE_BUFF, "" )
	DEFINE_ENUMCONST( TF_COND_DEFENSEBUFF_HIGH, "" )
	DEFINE_ENUMCONST( TF_COND_SNIPERCHARGE_RAGE_BUFF, "" )
	DEFINE_ENUMCONST( TF_COND_DISGUISE_WEARINGOFF, "" )
	DEFINE_ENUMCONST( TF_COND_MARKEDFORDEATH_SILENT, "" )
	DEFINE_ENUMCONST( TF_COND_DISGUISED_AS_DISPENSER, "" )
	DEFINE_ENUMCONST( TF_COND_SAPPED, "" )
	DEFINE_ENUMCONST( TF_COND_INVULNERABLE_HIDE_UNLESS_DAMAGED, "" )
	DEFINE_ENUMCONST( TF_COND_INVULNERABLE_USER_BUFF, "" )
	DEFINE_ENUMCONST( TF_COND_HALLOWEEN_BOMB_HEAD, "" )
	DEFINE_ENUMCONST( TF_COND_RADIUSHEAL_ON_DAMAGE, "" )
	DEFINE_ENUMCONST( TF_COND_CRITBOOSTED_CARD_EFFECT, "" )
	DEFINE_ENUMCONST( TF_COND_INVULNERABLE_CARD_EFFECT, "" )
	DEFINE_ENUMCONST( TF_COND_MEDIGUN_UBER_BULLET_RESIST, "" )
	DEFINE_ENUMCONST( TF_COND_MEDIGUN_UBER_BLAST_RESIST, "" )
	DEFINE_ENUMCONST( TF_COND_MEDIGUN_UBER_FIRE_RESIST, "" )
	DEFINE_ENUMCONST( TF_COND_MEDIGUN_SMALL_BULLET_RESIST, "" )
	DEFINE_ENUMCONST( TF_COND_MEDIGUN_SMALL_BLAST_RESIST, "" )
	DEFINE_ENUMCONST( TF_COND_STEALTHED_USER_BUFF_FADING, "" )
	DEFINE_ENUMCONST( TF_COND_BULLET_IMMUNE, "" )
	DEFINE_ENUMCONST( TF_COND_BLAST_IMMUNE, "" )
	DEFINE_ENUMCONST( TF_COND_FIRE_IMMUNE, "" )
	DEFINE_ENUMCONST( TF_COND_PREVENT_DEATH, "" )
	DEFINE_ENUMCONST( TF_COND_MVM_BOT_STUN_RADIOWAVE, "" )
	DEFINE_ENUMCONST( TF_COND_HALLOWEEN_SPEED_BOOST, "" )
	DEFINE_ENUMCONST( TF_COND_HALLOWEEN_QUICK_HEAL, "" )
	DEFINE_ENUMCONST( TF_COND_HALLOWEEN_GIANT, "" )
	DEFINE_ENUMCONST( TF_COND_HALLOWEEN_TINY, "" )
	DEFINE_ENUMCONST( TF_COND_HALLOWEEN_IN_HELL, "" )
	DEFINE_ENUMCONST( TF_COND_HALLOWEEN_GHOST_MODE, "" )
	DEFINE_ENUMCONST( TF_COND_MINICRITBOOSTED_ON_KILL, "" )
	DEFINE_ENUMCONST( TF_COND_OBSCURED_SMOKE, "" )
	DEFINE_ENUMCONST( TF_COND_PARACHUTE_ACTIVE, "" )
	DEFINE_ENUMCONST( TF_COND_BLASTJUMPING, "" )
	DEFINE_ENUMCONST( TF_COND_HALLOWEEN_KART, "" )
	DEFINE_ENUMCONST( TF_COND_HALLOWEEN_KART_DASH, "" )
	DEFINE_ENUMCONST( TF_COND_BALLOON_HEAD, "" )
	DEFINE_ENUMCONST( TF_COND_MELEE_ONLY, "" )
	DEFINE_ENUMCONST( TF_COND_SWIMMING_CURSE, "" )
	DEFINE_ENUMCONST( TF_COND_FREEZE_INPUT, "" )
	DEFINE_ENUMCONST( TF_COND_HALLOWEEN_KART_CAGE, "" )
	DEFINE_ENUMCONST( TF_COND_DONOTUSE_0, "" )
	DEFINE_ENUMCONST( TF_COND_RUNE_STRENGTH, "" )
	DEFINE_ENUMCONST( TF_COND_RUNE_HASTE, "" )
	DEFINE_ENUMCONST( TF_COND_RUNE_REGEN, "" )
	DEFINE_ENUMCONST( TF_COND_RUNE_RESIST, "" )
	DEFINE_ENUMCONST( TF_COND_RUNE_VAMPIRE, "" )
	DEFINE_ENUMCONST( TF_COND_RUNE_REFLECT, "" )
	DEFINE_ENUMCONST( TF_COND_RUNE_PRECISION, "" )
	DEFINE_ENUMCONST( TF_COND_RUNE_AGILITY, "" )
	DEFINE_ENUMCONST( TF_COND_GRAPPLINGHOOK, "" )
	DEFINE_ENUMCONST( TF_COND_GRAPPLINGHOOK_SAFEFALL, "" )
	DEFINE_ENUMCONST( TF_COND_GRAPPLINGHOOK_LATCHED, "" )
	DEFINE_ENUMCONST( TF_COND_GRAPPLINGHOOK_BLEEDING, "" )
	DEFINE_ENUMCONST( TF_COND_AFTERBURN_IMMUNE, "" )
	DEFINE_ENUMCONST( TF_COND_RUNE_KNOCKOUT, "" )
	DEFINE_ENUMCONST( TF_COND_RUNE_IMBALANCE, "" )
	DEFINE_ENUMCONST( TF_COND_CRITBOOSTED_RUNE_TEMP, "" )
	DEFINE_ENUMCONST( TF_COND_PASSTIME_INTERCEPTION, "" )
	DEFINE_ENUMCONST( TF_COND_SWIMMING_NO_EFFECTS, "" )
	DEFINE_ENUMCONST( TF_COND_PURGATORY, "" )
	DEFINE_ENUMCONST( TF_COND_RUNE_KING, "" )
	DEFINE_ENUMCONST( TF_COND_RUNE_PLAGUE, "" )
	DEFINE_ENUMCONST( TF_COND_RUNE_SUPERNOVA, "" )
	DEFINE_ENUMCONST( TF_COND_PLAGUE, "" )
	DEFINE_ENUMCONST( TF_COND_KING_BUFFED, "" )
	DEFINE_ENUMCONST( TF_COND_TEAM_GLOWS, "" )
	DEFINE_ENUMCONST( TF_COND_KNOCKED_INTO_AIR, "" )
	DEFINE_ENUMCONST( TF_COND_COMPETITIVE_WINNER, "" )
	DEFINE_ENUMCONST( TF_COND_COMPETITIVE_LOSER, "" )
	DEFINE_ENUMCONST( TF_COND_HEALING_DEBUFF, "" )
	DEFINE_ENUMCONST( TF_COND_PASSTIME_PENALTY_DEBUFF, "" )
	DEFINE_ENUMCONST( TF_COND_GRAPPLED_TO_PLAYER, "" )
	DEFINE_ENUMCONST( TF_COND_GRAPPLED_BY_PLAYER, "" )
	DEFINE_ENUMCONST( TF_COND_PARACHUTE_DEPLOYED, "" )
	DEFINE_ENUMCONST( TF_COND_GAS, "" )
	DEFINE_ENUMCONST( TF_COND_BURNING_PYRO, "" )
	DEFINE_ENUMCONST( TF_COND_ROCKETPACK, "" )
	DEFINE_ENUMCONST( TF_COND_LOST_FOOTING, "" )
	DEFINE_ENUMCONST( TF_COND_AIR_CURRENT, "" )
	DEFINE_ENUMCONST( TF_COND_HALLOWEEN_HELL_HEAL, "" )
	DEFINE_ENUMCONST( TF_COND_POWERUPMODE_DOMINANT, "" )

END_SCRIPTENUM()

//=============================================================================
//=============================================================================

BEGIN_SCRIPTENUM( ETFDmgCustom, "" )

	DEFINE_ENUMCONST( TF_DMG_CUSTOM_NONE, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_HEADSHOT, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_BACKSTAB, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_BURNING, "" )
	DEFINE_ENUMCONST( TF_DMG_WRENCH_FIX, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_MINIGUN, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_SUICIDE, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_TAUNTATK_HADOUKEN, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_BURNING_FLARE, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_TAUNTATK_HIGH_NOON, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_TAUNTATK_GRAND_SLAM, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_PENETRATE_MY_TEAM, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_PENETRATE_ALL_PLAYERS, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_TAUNTATK_FENCING, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_PENETRATE_NONBURNING_TEAMMATE, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_TAUNTATK_ARROW_STAB, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_TELEFRAG, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_BURNING_ARROW, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_FLYINGBURN, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_PUMPKIN_BOMB, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_DECAPITATION, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_TAUNTATK_GRENADE, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_BASEBALL, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_CHARGE_IMPACT, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_TAUNTATK_BARBARIAN_SWING, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_AIR_STICKY_BURST, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_DEFENSIVE_STICKY, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_PICKAXE, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_ROCKET_DIRECTHIT, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_TAUNTATK_UBERSLICE, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_PLAYER_SENTRY, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_STANDARD_STICKY, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_SHOTGUN_REVENGE_CRIT, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_TAUNTATK_ENGINEER_GUITAR_SMASH, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_BLEEDING, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_GOLD_WRENCH, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_CARRIED_BUILDING, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_COMBO_PUNCH, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_TAUNTATK_ENGINEER_ARM_KILL, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_FISH_KILL, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_TRIGGER_HURT, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_DECAPITATION_BOSS, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_STICKBOMB_EXPLOSION, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_AEGIS_ROUND, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_FLARE_EXPLOSION, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_BOOTS_STOMP, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_PLASMA, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_PLASMA_CHARGED, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_PLASMA_GIB, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_PRACTICE_STICKY, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_EYEBALL_ROCKET, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_HEADSHOT_DECAPITATION, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_TAUNTATK_ARMAGEDDON, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_FLARE_PELLET, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_CLEAVER, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_CLEAVER_CRIT, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_SAPPER_RECORDER_DEATH, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_MERASMUS_PLAYER_BOMB, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_MERASMUS_GRENADE, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_MERASMUS_ZAP, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_MERASMUS_DECAPITATION, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_CANNONBALL_PUSH, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_TAUNTATK_ALLCLASS_GUITAR_RIFF, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_THROWABLE, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_THROWABLE_KILL, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_SPELL_TELEPORT, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_SPELL_SKELETON, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_SPELL_MIRV, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_SPELL_METEOR, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_SPELL_LIGHTNING, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_SPELL_FIREBALL, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_SPELL_MONOCULUS, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_SPELL_BLASTJUMP, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_SPELL_BATS, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_SPELL_TINY, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_KART, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_GIANT_HAMMER, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_RUNE_REFLECT, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_DRAGONS_FURY_IGNITE, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_DRAGONS_FURY_BONUS_BURNING, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_SLAP_KILL, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_CROC, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_TAUNTATK_GASBLAST, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_AXTINGUISHER_BOOSTED, "" )
	DEFINE_ENUMCONST( TF_DMG_CUSTOM_END, "" )

END_SCRIPTENUM()

//=============================================================================
//=============================================================================

#ifdef GAME_DLL
int ScriptGetRoundState()
{
	return TFGameRules()->State_Get();
}

bool ScriptIsInWaitingForPlayers()
{
	return TFGameRules()->IsInWaitingForPlayers();
}

int ScriptGetWinningTeam()
{
	return TFGameRules()->GetWinningTeam();
}

bool ScriptInOvertime()
{
	return TFGameRules()->InOvertime();
}

bool ScriptIsBirthday()
{
	return TFGameRules()->IsBirthday();
}

bool ScriptIsHolidayActive( int eHoliday )
{
	return TFGameRules()->IsHolidayActive( eHoliday );
}

bool ScriptPointsMayBeCaptured()
{
	return TFGameRules()->PointsMayBeCaptured();
}

int ScriptGetClassLimit( int iClass )
{
	return TFGameRules()->GetClassLimit( iClass );
}

bool ScriptFlagsMayBeCapped()
{
	return TFGameRules()->FlagsMayBeCapped();
}

int ScriptGetStopWatchState()
{
	return TFGameRules()->GetStopWatchState();
}

bool ScriptIsInArenaMode()
{
	return TFGameRules()->IsInArenaMode();
}

bool ScriptIsInKothMode()
{
	return TFGameRules()->IsInKothMode();
}

bool ScriptIsInMedievalMode()
{
	return TFGameRules()->IsInMedievalMode();
}

bool ScriptIsHolidayMap( int eHoliday )
{
	return TFGameRules()->IsHolidayMap( eHoliday );
}

bool ScriptIsMannVsMachineMode()
{
	return TFGameRules()->IsMannVsMachineMode();
}

bool ScriptGetMannVsMachineAlarmStatus()
{
	return TFGameRules()->GetMannVsMachineAlarmStatus();
}


void ScriptSetMannVsMachineAlarmStatus( bool bActive )
{
	TFGameRules()->SetMannVsMachineAlarmStatus( bActive );
}

bool ScriptIsQuickBuildTime()
{
	return TFGameRules()->IsQuickBuildTime();
}

bool ScriptGameModeUsesUpgrades()
{
	return TFGameRules()->GameModeUsesUpgrades();
}

bool ScriptGameModeUsesCurrency()
{
	return TFGameRules()->GameModeUsesCurrency();
}

bool ScriptGameModeUsesMiniBosses()
{
	return TFGameRules()->GameModeUsesMiniBosses();
}

bool ScriptIsPasstimeMode()
{
	return TFGameRules()->IsPasstimeMode();
}

bool ScriptIsMannVsMachineRespecEnabled()
{
	return TFGameRules()->IsMannVsMachineRespecEnabled();
}

bool ScriptIsPowerupMode()
{
	return TFGameRules()->IsPowerupMode();
}

bool ScriptIsCompetitiveMode()
{
	return TFGameRules()->IsCompetitiveMode();
}

bool ScriptIsMatchTypeCasual()
{
	return TFGameRules()->IsMatchTypeCasual();
}

bool ScriptIsMatchTypeCompetitive()
{
	return TFGameRules()->IsMatchTypeCompetitive();
}

bool ScriptInMatchStartCountdown()
{
	return TFGameRules()->BInMatchStartCountdown();
}

bool ScriptMatchmakingShouldUseStopwatchMode()
{
	return TFGameRules()->IsAttackDefenseMode();
}

bool ScriptIsAttackDefenseMode()
{
	return TFGameRules()->IsAttackDefenseMode();
}

bool ScriptUsePlayerReadyStatusMode()
{
	return TFGameRules()->UsePlayerReadyStatusMode();
}

bool ScriptPlayerReadyStatus_HaveMinPlayersToEnable()
{
	return TFGameRules()->PlayerReadyStatus_HaveMinPlayersToEnable();
}

bool ScriptPlayerReadyStatus_ArePlayersOnTeamReady( int iTeam )
{
	return TFGameRules()->PlayerReadyStatus_ArePlayersOnTeamReady( iTeam );
}

void ScriptPlayerReadyStatus_ResetState()
{
	TFGameRules()->PlayerReadyStatus_ResetState();
}

bool ScriptIsDefaultGameMode()
{
	return TFGameRules()->IsDefaultGameMode();
}

bool ScriptIsPVEModeActive()
{
	return TFGameRules()->IsPVEModeActive();
}

bool ScriptAllowThirdPersonCamera()
{
	return TFGameRules()->AllowThirdPersonCamera();
}

void ScriptSetGravityMultiplier( float flMultiplier )
{
	TFGameRules()->SetGravityMultiplier( flMultiplier );
}

float ScriptGetGravityMultiplier( float flMultiplier )
{
	return TFGameRules()->GetGravityMultiplier();
}

void ScriptSetPlayersInHell( bool bInHell )
{
	TFGameRules()->SetPlayersInHell( bInHell );
}

bool ScriptArePlayersInHell()
{
	return TFGameRules()->ArePlayersInHell();
}

void ScriptSetUsingSpells( bool bUseSpells )
{
	TFGameRules()->SetUsingSpells( bUseSpells );
}

bool ScriptIsUsingSpells()
{
	return TFGameRules()->IsUsingSpells();
}

bool ScriptIsUsingGrapplingHook()
{
	return TFGameRules()->IsUsingGrapplingHook();
}

bool ScriptIsTruceActive()
{
	return TFGameRules()->IsTruceActive();
}

bool ScriptMapHasMatchSummaryStage()
{
	return TFGameRules()->MapHasMatchSummaryStage();
}

bool ScriptPlayersAreOnMatchSummaryStage()
{
	return TFGameRules()->PlayersAreOnMatchSummaryStage();
}

bool ScriptHaveStopWatchWinner()
{
	return TFGameRules()->HaveStopWatchWinner();
}

bool ScriptGetOvertimeAllowedForCTF()
{
	return TFGameRules()->GetOvertimeAllowedForCTF();
}

void ScriptSetOvertimeAllowedForCTF( bool bAllowed )
{
	TFGameRules()->SetOvertimeAllowedForCTF( bAllowed );
}

void ScriptForceEnableUpgrades( int iForce )
{
	TFGameRules()->ForceEnableUpgrades( iForce );
}

void ScriptForceEscortPushLogic( int iForce )
{
	TFGameRules()->ForceEscortPushLogic( iForce );
}
#endif
//=============================================================================
//=============================================================================

BEGIN_SCRIPTDESC( CTFGameRules, CGameRules, SCRIPT_SINGLETON "The Team Fortress game rules" )
END_SCRIPTDESC()

void CTFGameRules::RegisterScriptFunctions( void )
{
#ifdef GAME_DLL
#define TF_GAMERULES_SCRIPT_FUNC( function, desc ) \
		ScriptRegisterFunctionNamed( g_pScriptVM, Script##function, #function, desc )

	TF_GAMERULES_SCRIPT_FUNC( GetRoundState,							"Get current round state. See Constants.ERoundState" );
	TF_GAMERULES_SCRIPT_FUNC( IsInWaitingForPlayers,					"Are we waiting for some stragglers?" );
	TF_GAMERULES_SCRIPT_FUNC( GetWinningTeam,							"Who won!" );
	TF_GAMERULES_SCRIPT_FUNC( InOvertime,								"Currently in overtime?" );

	TF_GAMERULES_SCRIPT_FUNC( IsBirthday,								"Are we in birthday mode?" );
	TF_GAMERULES_SCRIPT_FUNC( IsHolidayActive,							"Is the given holiday active? See Constants.EHoliday" );
	TF_GAMERULES_SCRIPT_FUNC( PointsMayBeCaptured,						"Are points able to be captured?" );
	TF_GAMERULES_SCRIPT_FUNC( GetClassLimit,							"Get class limit for class. See Constants.ETFClass" );
	TF_GAMERULES_SCRIPT_FUNC( FlagsMayBeCapped,							"May a flag be captured?" );
	TF_GAMERULES_SCRIPT_FUNC( GetStopWatchState,						"Get the current stopwatch state. See Constants.EStopwatchState" );
	TF_GAMERULES_SCRIPT_FUNC( IsInArenaMode,							"Playing arena mode?" );
	TF_GAMERULES_SCRIPT_FUNC( IsInKothMode,								"Playing king of the hill mode?" );
	TF_GAMERULES_SCRIPT_FUNC( IsInMedievalMode,							"Playing medieval mode?" );
	TF_GAMERULES_SCRIPT_FUNC( IsHolidayMap,								"Playing a holiday map? See Constants.EHoliday" );
	TF_GAMERULES_SCRIPT_FUNC( IsMannVsMachineMode,						"Playing MvM? Beep boop" );
	TF_GAMERULES_SCRIPT_FUNC( GetMannVsMachineAlarmStatus,				"" );
	TF_GAMERULES_SCRIPT_FUNC( SetMannVsMachineAlarmStatus,				"" );
	TF_GAMERULES_SCRIPT_FUNC( IsQuickBuildTime,							"If an engie places a building, will it immediately upgrade? Eg. MvM pre-round etc." );
	TF_GAMERULES_SCRIPT_FUNC( GameModeUsesUpgrades,						"Does the current gamemode have upgrades?" );
	TF_GAMERULES_SCRIPT_FUNC( GameModeUsesCurrency,						"Does the current gamemode have currency?" );
	TF_GAMERULES_SCRIPT_FUNC( GameModeUsesMiniBosses,					"Does the current gamemode have minibosses?" );
	TF_GAMERULES_SCRIPT_FUNC( IsPasstimeMode,							"No ball games." );
	TF_GAMERULES_SCRIPT_FUNC( IsMannVsMachineRespecEnabled,				"Are players allowed to refund their upgrades?" );
	TF_GAMERULES_SCRIPT_FUNC( IsPowerupMode,							"Playing powerup mode? Not compatible with MvM" );
	TF_GAMERULES_SCRIPT_FUNC( IsCompetitiveMode,						"Playing competitive?" );
	TF_GAMERULES_SCRIPT_FUNC( IsMatchTypeCasual,						"Playing casual?" );
	TF_GAMERULES_SCRIPT_FUNC( IsMatchTypeCompetitive,					"Playing competitive?" );
	TF_GAMERULES_SCRIPT_FUNC( InMatchStartCountdown,					"Are we in the pre-match state?" );
	TF_GAMERULES_SCRIPT_FUNC( MatchmakingShouldUseStopwatchMode,		"" );
	TF_GAMERULES_SCRIPT_FUNC( IsAttackDefenseMode,						"" );
	TF_GAMERULES_SCRIPT_FUNC( UsePlayerReadyStatusMode,					"" );
	TF_GAMERULES_SCRIPT_FUNC( PlayerReadyStatus_HaveMinPlayersToEnable,	"" );
	TF_GAMERULES_SCRIPT_FUNC( PlayerReadyStatus_ArePlayersOnTeamReady,	"" );
	TF_GAMERULES_SCRIPT_FUNC( PlayerReadyStatus_ResetState,				"" );
	TF_GAMERULES_SCRIPT_FUNC( IsDefaultGameMode,						"The absence of arena, mvm, tournament mode, etc" );
	TF_GAMERULES_SCRIPT_FUNC( IsPVEModeActive,							"" );
	TF_GAMERULES_SCRIPT_FUNC( AllowThirdPersonCamera,					"" );
	TF_GAMERULES_SCRIPT_FUNC( SetGravityMultiplier,						"" );
	TF_GAMERULES_SCRIPT_FUNC( GetGravityMultiplier,						"" );
	TF_GAMERULES_SCRIPT_FUNC( SetPlayersInHell,							"" );
	TF_GAMERULES_SCRIPT_FUNC( ArePlayersInHell,							"" );
	TF_GAMERULES_SCRIPT_FUNC( SetUsingSpells,							"" );
	TF_GAMERULES_SCRIPT_FUNC( IsUsingSpells,							"" );
	TF_GAMERULES_SCRIPT_FUNC( IsUsingGrapplingHook,						"" );
	TF_GAMERULES_SCRIPT_FUNC( IsTruceActive,							"" );
	TF_GAMERULES_SCRIPT_FUNC( MapHasMatchSummaryStage,					"" );
	TF_GAMERULES_SCRIPT_FUNC( PlayersAreOnMatchSummaryStage,			"" );
	TF_GAMERULES_SCRIPT_FUNC( HaveStopWatchWinner,						"" );
	TF_GAMERULES_SCRIPT_FUNC( GetOvertimeAllowedForCTF,					"" );
	TF_GAMERULES_SCRIPT_FUNC( SetOvertimeAllowedForCTF,					"" );

	TF_GAMERULES_SCRIPT_FUNC( ForceEnableUpgrades,						"Whether to force on MvM-styled upgrades on/off. 0 -> default, 1 -> force off, 2 -> force on" );
	TF_GAMERULES_SCRIPT_FUNC( ForceEscortPushLogic,						"Forces payload pushing logic. 0 -> default, 1 -> force off, 2 -> force on" );

	g_pScriptVM->RegisterInstance( &PlayerVoiceListener(), "PlayerVoiceListener" );
#endif

	if ( GetItemSchema() )
	{
		GetItemSchema()->RegisterScriptFunctions();
	}
}

void CTFItemSchema::RegisterScriptFunctions( void )
{
#ifdef GAME_DLL
#define TF_ECON_SCRIPT_FUNC( function, desc ) \
		ScriptRegisterFunctionNamed( g_pScriptVM, Script##function, #function, desc )
#endif
}