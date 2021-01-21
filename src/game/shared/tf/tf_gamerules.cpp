//========= Copyright © 1996-2004, Valve LLC, All rights reserved. ============
//
// Purpose: The TF Game rules 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "tf_gamerules.h"
#include "ammodef.h"
#include "KeyValues.h"
#include "tf_weaponbase.h"
#include "filesystem.h"
#include "time.h"
#include "viewport_panel_names.h"
#include "tf_halloween_boss.h"
#include "tf_zombie.h"
#include "entity_bossresource.h"
#include "vscript_shared.h"
#ifdef CLIENT_DLL
	#include <game/client/iviewport.h>
	#include "c_tf_player.h"
	#include "c_tf_objective_resource.h"
	#include "c_user_message_register.h"
	#include "tf_autorp.h"
#else
	#include "basemultiplayerplayer.h"
	#include "voice_gamemgr.h"
	#include "items.h"
	#include "team.h"
	#include "tf_bot_temp.h"
	#include "tf_player.h"
	#include "tf_team.h"
	#include "player_resource.h"
	#include "entity_tfstart.h"
	#include "filesystem.h"
	#include "tf_obj.h"
	#include "tf_objective_resource.h"
	#include "tf_player_resource.h"
	#include "playerclass_info_parse.h"
	#include "team_control_point_master.h"
	#include "coordsize.h"
	#include "entity_healthkit.h"
	#include "tf_gamestats.h"
	#include "entity_capture_flag.h"
	#include "tf_player_resource.h"
	#include "tf_obj_sentrygun.h"
	#include "tier0/icommandline.h"
	#include "activitylist.h"
	#include "AI_ResponseSystem.h"
	#include "hl2orange.spa.h"
	#include "hltvdirector.h"
	#include "team_train_watcher.h"
	#include "vote_controller.h"
	#include "tf_voteissues.h"
	#include "tf_weaponbase_grenadeproj.h"
	#include "eventqueue.h"
	#include "nav_mesh.h"
	#include "tf_logic_entities.h"
	#include "bot/tf_bot_manager.h"
	#include "entity_wheelofdoom.h"
	#include "player_vs_environment/merasmus.h"
	#include "player_vs_environment/ghost.h"
	#include "map_entities/tf_bot_roster.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define ITEM_RESPAWN_TIME	10.0f

//=============================================================================
void HalloweenChanged( IConVar *var, const char *pOldValue, float flOldValue )
{
	ConVarRef cvar( var );
	if ( cvar.IsValid() )
	{
		if( cvar.GetBool() )
		{
		#if defined( CLIENT_DLL )
			C_BasePlayer *pLocal = C_BasePlayer::GetLocalPlayer();
			if ( pLocal == nullptr )
				return;

			if ( RandomInt( 0, 100 ) <= 15 )
			{
				pLocal->EmitSound( "Halloween.MerasmusHalloweenModeRare" );
			}
			else
			{
				pLocal->EmitSound( "Halloween.MerasmusHalloweenModeCommon" );
			}
		#endif
		}
	}
}

void ForcedHolidayChanged( IConVar *var, const char *oldValue, float flOldValue )
{
	IGameEvent *event = gameeventmanager->CreateEvent( "recalculate_holidays" );
	if ( event )
	{
		gameeventmanager->FireEvent( event );
	}
}

void MedievalModeChanged( IConVar *pConVar, const char *pOldString, float flOldValue )
{
	ConVarRef var( pConVar );
	bool bOldValue = flOldValue > 0;
	if ( var.IsValid() && ( bOldValue != var.GetBool() ) )
	{
		Msg( "Medieval mode changes take effect after the next map change.\n" );
	}
}

void ValidateCapturesPerRound( IConVar *pConVar, const char *oldValue, float flOldValue )
{
#ifdef GAME_DLL
	ConVarRef var( pConVar );

	if ( var.GetInt() <= 0 )
	{
		// reset the flag captures being played in the current round
		int nTeamCount = TFTeamMgr()->GetTeamCount();
		for ( int iTeam = FIRST_GAME_TEAM; iTeam < nTeamCount; ++iTeam )
		{
			CTFTeam *pTeam = GetGlobalTFTeam( iTeam );
			if ( !pTeam )
				continue;

			pTeam->SetFlagCaptures( 0 );
		}
	}
#endif
}

void StopwatchChanged( IConVar *pConVar, const char *pOldString, float flOldValue )
{
	IGameEvent *event = gameeventmanager->CreateEvent( "stop_watch_changed" );
	if ( event )
	{
		gameeventmanager->FireEvent( event );
	}
}
//=============================================================================

static int g_TauntCamAchievements[] =
{
	0,		// TF_CLASS_UNDEFINED

	0,		// TF_CLASS_SCOUT,	
	0,		// TF_CLASS_SNIPER,
	0,		// TF_CLASS_SOLDIER,
	0,		// TF_CLASS_DEMOMAN,
	0,		// TF_CLASS_MEDIC,
	0,		// TF_CLASS_HEAVYWEAPONS,
	0,		// TF_CLASS_PYRO,
	0,		// TF_CLASS_SPY,
	0,		// TF_CLASS_ENGINEER,

	0,		// TF_CLASS_COUNT_ALL,
};

extern ConVar mp_capstyle;
extern ConVar sv_turbophysics;
extern ConVar mp_chattime;
extern ConVar tf_arena_max_streak;
extern ConVar mp_tournament;

ConVar tf_caplinear( "tf_caplinear", "1", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY, "If set to 1, teams must capture control points linearly." );
ConVar tf_stalematechangeclasstime( "tf_stalematechangeclasstime", "20", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY, "Amount of time that players are allowed to change class in stalemates." );
ConVar tf_spells_enabled( "tf_spells_enabled", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Enable to Allow Halloween Spells to be dropped and used by players" );
ConVar tf_birthday( "tf_birthday", "0", FCVAR_NOTIFY | FCVAR_REPLICATED );
ConVar tf_forced_holiday( "tf_forced_holiday", "0", FCVAR_REPLICATED, "Forced holiday, \n   Birthday = 1\n   Halloween = 2\n" //  Christmas = 3\n   Valentines = 4\n   MeetThePyro = 5\n   FullMoon=6
					  #if defined( GAME_DLL )
						  , ForcedHolidayChanged
					  #endif
);
ConVar tf_item_based_forced_holiday( "tf_item_based_forced_holiday", "0", FCVAR_REPLICATED, "" 	// like a clone of tf_forced_holiday, but controlled by client consumable item use
								 #if defined( GAME_DLL )
									 , ForcedHolidayChanged
								 #endif
);
ConVar tf_force_holidays_off( "tf_force_holidays_off", "0", FCVAR_NOTIFY | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY, ""
						  #if defined( GAME_DLL )
							  , ForcedHolidayChanged
						  #endif
);
ConVar tf_medieval( "tf_medieval", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "Enable Medieval Mode.\n", true, 0, true, 1
				#ifdef GAME_DLL
					, MedievalModeChanged
				#endif 
);
ConVar tf_medieval_autorp( "tf_medieval_autorp", "1", FCVAR_NOTIFY | FCVAR_REPLICATED, "Enable Medieval Mode auto-roleplaying." );
ConVar tf_flag_caps_per_round( "tf_flag_caps_per_round", "3", FCVAR_REPLICATED, "Number of flag captures per round on CTF maps. Set to 0 to disable.", true, 0, true, 9
						   #if defined( GAME_DLL )
							   , ValidateCapturesPerRound
						   #endif
);
#ifdef CLIENT_DLL
ConVar tf_particles_disable_weather( "tf_particles_disable_weather", "0", FCVAR_ARCHIVE, "Disable particles related to weather effects." );
#endif

// Tournament mode
ConVar mp_tournament_redteamname( "mp_tournament_redteamname", "RED", FCVAR_REPLICATED | FCVAR_HIDDEN );
ConVar mp_tournament_blueteamname( "mp_tournament_blueteamname", "BLU", FCVAR_REPLICATED | FCVAR_HIDDEN );
ConVar mp_tournament_stopwatch( "mp_tournament_stopwatch", "1", FCVAR_REPLICATED | FCVAR_NOTIFY, "Use Stopwatch mode while using Tournament mode (mp_tournament)"
							#ifdef GAME_DLL
								, StopwatchChanged 
							#endif
);
ConVar mp_tournament_readymode( "mp_tournament_readymode", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "Enable per-player ready status for tournament mode." );
ConVar mp_tournament_readymode_min( "mp_tournament_readymode_min", "2", FCVAR_REPLICATED | FCVAR_NOTIFY, "Minimum number of players required on the server before players can toggle ready status." );
ConVar mp_tournament_readymode_team_size( "mp_tournament_readymode_team_size", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "Minimum number of players required to be ready per-team before the game can begin." );
ConVar mp_tournament_readymode_countdown( "mp_tournament_readymode_countdown", "10", FCVAR_REPLICATED | FCVAR_NOTIFY, "The number of seconds before a match begins when both teams are ready." );

ConVar tf_tournament_classlimit_scout( "tf_tournament_classlimit_scout", "-1", FCVAR_REPLICATED, "Tournament mode per-team class limit for Scouts.\n" );
ConVar tf_tournament_classlimit_sniper( "tf_tournament_classlimit_sniper", "-1", FCVAR_REPLICATED, "Tournament mode per-team class limit for Snipers.\n" );
ConVar tf_tournament_classlimit_soldier( "tf_tournament_classlimit_soldier", "-1", FCVAR_REPLICATED, "Tournament mode per-team class limit for Soldiers.\n" );
ConVar tf_tournament_classlimit_demoman( "tf_tournament_classlimit_demoman", "-1", FCVAR_REPLICATED, "Tournament mode per-team class limit for Demomen.\n" );
ConVar tf_tournament_classlimit_medic( "tf_tournament_classlimit_medic", "-1", FCVAR_REPLICATED, "Tournament mode per-team class limit for Medics.\n" );
ConVar tf_tournament_classlimit_heavy( "tf_tournament_classlimit_heavy", "-1", FCVAR_REPLICATED, "Tournament mode per-team class limit for Heavies.\n" );
ConVar tf_tournament_classlimit_pyro( "tf_tournament_classlimit_pyro", "-1", FCVAR_REPLICATED, "Tournament mode per-team class limit for Pyros.\n" );
ConVar tf_tournament_classlimit_spy( "tf_tournament_classlimit_spy", "-1", FCVAR_REPLICATED, "Tournament mode per-team class limit for Spies.\n" );
ConVar tf_tournament_classlimit_engineer( "tf_tournament_classlimit_engineer", "-1", FCVAR_REPLICATED, "Tournament mode per-team class limit for Engineers.\n" );
ConVar tf_tournament_classchange_allowed( "tf_tournament_classchange_allowed", "1", FCVAR_REPLICATED, "Allow players to change class while the game is active?.\n" );
ConVar tf_tournament_classchange_ready_allowed( "tf_tournament_classchange_ready_allowed", "1", FCVAR_REPLICATED, "Allow players to change class after they are READY?.\n" );
ConVar tf_classlimit( "tf_classlimit", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "Limit on how many players can be any class (i.e. tf_class_limit 2 would limit 2 players per class).\n" );

// tf2v specific cvars.
ConVar tf2v_falldamage_disablespread( "tf2v_falldamage_disablespread", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "Toggles random 20% fall damage spread." );
ConVar tf2v_allow_thirdperson( "tf2v_allow_thirdperson", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Allow players to switch to third person mode." );
ConVar tf2v_classlimit( "tf2v_classlimit", "1", FCVAR_NOTIFY | FCVAR_REPLICATED, "Enable classlimits, even when tournament mode is disabled." );
ConVar tf2v_critchance( "tf2v_critchance", "2.0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Percent chance for regular critical hits.");
ConVar tf2v_critchance_rapid( "tf2v_critchance_rapid", "2.0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Percent chance for rapid fire critical hits.");
ConVar tf2v_critchance_melee( "tf2v_critchance_melee", "2.0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Percent chance of melee critical hits.");
ConVar tf2v_crit_duration_rapid( "tf2v_crit_duration_rapid", "2.0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Length in seconds for rapid fire critical hit duration.");
ConVar tf2v_ctf_capcrits( "tf2v_ctf_capcrits", "1", FCVAR_NOTIFY | FCVAR_REPLICATED, "Enable critical hits on flag capture." );
ConVar tf2v_minicrits_on_deflect( "tf2v_minicrits_on_deflect", "1", FCVAR_NOTIFY | FCVAR_REPLICATED, "Deflected projectiles get minicrits." );

ConVar tf2v_allow_objective_glow_ctf( "tf2v_allow_objective_glow_ctf", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Enable glow for CTF flags." );
ConVar tf2v_allow_objective_glow_pl( "tf2v_allow_objective_glow_pl", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Enable glow for Payload carts." );

ConVar tf2v_console_grenadelauncher_damage("tf2v_console_grenadelauncher_damage", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Changes the grenade launcher damage to reflect console values.", true, 0, true, 1 );
ConVar tf2v_console_grenadelauncher_magazine("tf2v_console_grenadelauncher_magazine", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Changes the grenade launcher magazaine capacity to reflect console values.", true, 0, true, 1 );

ConVar tf2v_remove_loser_disguise("tf2v_remove_loser_disguise", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Forces spies on a losing team to undisguise.", true, 0, true, 1 );

#ifdef GAME_DLL
ConVar hide_server( "hide_server", "0", FCVAR_GAMEDLL, "Whether the server should be hidden from the master server" );

// TF overrides the default value of this convar
ConVar mp_waitingforplayers_time( "mp_waitingforplayers_time", "30", FCVAR_GAMEDLL | FCVAR_DEVELOPMENTONLY, "WaitingForPlayers time length in seconds" );

ConVar mp_humans_must_join_team( "mp_humans_must_join_team", "any", FCVAR_GAMEDLL | FCVAR_REPLICATED, "Restricts human players to a single team {any, blue, red, spectator}" );

ConVar tf_arena_force_class( "tf_arena_force_class", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Force random classes in arena." );
ConVar tf_arena_first_blood( "tf_arena_first_blood", "1", FCVAR_NOTIFY | FCVAR_REPLICATED, "Toggles first blood criticals" );
ConVar tf_arena_first_blood_length( "tf_arena_first_blood_length", "5.0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Duration of first blood criticals" );
ConVar tf_arena_override_cap_enable_time( "tf_arena_override_cap_enable_time", "-1", FCVAR_REPLICATED | FCVAR_NOTIFY, "Overrides the time (in seconds) it takes for the capture point to become enable, -1 uses the level designer specified time." );

ConVar tf_gamemode_arena( "tf_gamemode_arena", "0", FCVAR_NOTIFY | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );
ConVar tf_gamemode_cp( "tf_gamemode_cp", "0", FCVAR_NOTIFY | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );
ConVar tf_gamemode_ctf( "tf_gamemode_ctf", "0", FCVAR_NOTIFY | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );
ConVar tf_gamemode_sd( "tf_gamemode_sd", "0", FCVAR_NOTIFY | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );
ConVar tf_gamemode_rd( "tf_gamemode_rd", "0", FCVAR_NOTIFY | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );
ConVar tf_gamemode_tc( "tf_gamemode_tc", "0", FCVAR_NOTIFY | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );
ConVar tf_gamemode_payload( "tf_gamemode_payload", "0", FCVAR_NOTIFY | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );
ConVar tf_gamemode_plr( "tf_gamemode_plr", "0", FCVAR_NOTIFY | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );
ConVar tf_gamemode_mvm( "tf_gamemode_mvm", "0", FCVAR_NOTIFY | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );
ConVar tf_gamemode_passtime( "tf_gamemode_passtime", "0", FCVAR_NOTIFY | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );
ConVar tf_gamemode_medieval( "tf_gamemode_medieval", "0", FCVAR_NOTIFY | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );
ConVar tf_gamemode_koth( "tf_gamemode_koth", "0", FCVAR_NOTIFY | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );
ConVar tf_gamemode_vsh( "tf_gamemode_vsh", "0", FCVAR_NOTIFY | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );
ConVar tf_gamemode_dr( "tf_gamemode_dr", "0", FCVAR_NOTIFY | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );
ConVar tf_gamemode_pd( "tf_gamemode_pd", "0", FCVAR_NOTIFY | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );
ConVar tf_gamemode_misc( "tf_gamemode_misc", "0", FCVAR_NOTIFY | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );

ConVar tf_teamtalk( "tf_teamtalk", "1", FCVAR_NOTIFY, "Teammates can always chat with each other whether alive or dead." );
ConVar tf_gravetalk( "tf_gravetalk", "1", FCVAR_NOTIFY, "Allows living players to hear dead players using text/voice chat.", true, 0, true, 1 );
ConVar tf_ctf_bonus_time( "tf_ctf_bonus_time", "10", FCVAR_NOTIFY, "Length of team crit time for CTF capture." );

extern ConVar tf_halloween_bot_min_player_count;

ConVar tf_halloween_boss_spawn_interval( "tf_halloween_boss_spawn_interval", "480", FCVAR_CHEAT, "Average interval between boss spawns, in seconds" );
ConVar tf_halloween_boss_spawn_interval_variation( "tf_halloween_boss_spawn_interval_variation", "60", FCVAR_CHEAT, "Variation of spawn interval +/-" );

ConVar tf_halloween_eyeball_boss_spawn_interval( "tf_halloween_eyeball_boss_spawn_interval", "180", FCVAR_CHEAT, "Average interval between boss spawns, in seconds" );
ConVar tf_halloween_eyeball_boss_spawn_interval_variation( "tf_halloween_eyeball_boss_spawn_interval_variation", "30", FCVAR_CHEAT, "Variation of spawn interval +/-" );

ConVar tf_merasmus_spawn_interval( "tf_merasmus_spawn_interval", "180", FCVAR_CHEAT, "Average interval between boss spawns, in seconds" );
ConVar tf_merasmus_spawn_interval_variation( "tf_merasmus_spawn_interval_variation", "30", FCVAR_CHEAT, "Variation of spawn interval +/-" );

ConVar tf_halloween_zombie_mob_enabled( "tf_halloween_zombie_mob_enabled", "0", FCVAR_CHEAT, "If set to 1, spawn zombie mobs on non-Halloween Valve maps" );
ConVar tf_halloween_zombie_mob_spawn_interval( "tf_halloween_zombie_mob_spawn_interval", "180", FCVAR_CHEAT, "Average interval between zombie mob spawns, in seconds" );
ConVar tf_halloween_zombie_mob_spawn_count( "tf_halloween_zombie_mob_spawn_count", "20", FCVAR_CHEAT, "How many zombies to spawn" );

static bool isBossForceSpawning = false;
CON_COMMAND_F( tf_halloween_force_boss_spawn, "For testing.", FCVAR_DEVELOPMENTONLY )
{
	isBossForceSpawning = true;
}

static bool isZombieMobForceSpawning = false;
CON_COMMAND_F( tf_halloween_force_mob_spawn, "For testing.", FCVAR_DEVELOPMENTONLY )
{
	isZombieMobForceSpawning = true;
}
#endif

static bool BIsCvarIndicatingHolidayIsActive( int iCvarValue, /*EHoliday*/ int eHoliday )
{
	if ( iCvarValue == 0 )
		return false;

	// Some values can equal multiple things
	switch ( eHoliday )
	{
		case kHoliday_Halloween:
			return iCvarValue == kHoliday_Halloween || iCvarValue == kHoliday_HalloweenOrFullMoon || iCvarValue == kHoliday_HalloweenOrFullMoonOrValentines;
		case kHoliday_ValentinesDay:
			return iCvarValue == kHoliday_ValentinesDay || iCvarValue == kHoliday_HalloweenOrFullMoonOrValentines;
		case kHoliday_FullMoon:
			return iCvarValue == kHoliday_FullMoon || iCvarValue == kHoliday_HalloweenOrFullMoon || iCvarValue == kHoliday_HalloweenOrFullMoonOrValentines;
		case kHoliday_HalloweenOrFullMoon:
			return iCvarValue == kHoliday_Halloween || iCvarValue == kHoliday_FullMoon || iCvarValue == kHoliday_HalloweenOrFullMoon || iCvarValue == kHoliday_HalloweenOrFullMoonOrValentines;
		case kHoliday_HalloweenOrFullMoonOrValentines:
			return iCvarValue == kHoliday_Halloween || iCvarValue == kHoliday_FullMoon || iCvarValue == kHoliday_ValentinesDay || iCvarValue == kHoliday_HalloweenOrFullMoon || iCvarValue == kHoliday_HalloweenOrFullMoonOrValentines;
	}

	return iCvarValue == eHoliday;
}

bool TF_IsHolidayActive( /*EHoliday*/ int eHoliday )
{
	if ( tf_force_holidays_off.GetBool() )
		return false;

	if ( BIsCvarIndicatingHolidayIsActive( tf_forced_holiday.GetInt(), eHoliday ) )
		return true;

	if ( BIsCvarIndicatingHolidayIsActive( tf_item_based_forced_holiday.GetInt(), eHoliday ) )
		return true;

	if ( ( eHoliday == kHoliday_TF2Birthday ) && tf_birthday.GetBool() )
		return true;

	if ( TFGameRules() )
	{
		if ( eHoliday == kHoliday_HalloweenOrFullMoon )
		{
			if ( TFGameRules()->IsHolidayMap( kHoliday_Halloween ) )
				return true;
			if ( TFGameRules()->IsHolidayMap( kHoliday_FullMoon ) )
				return true;
		}

		if ( TFGameRules()->IsHolidayMap( eHoliday ) )
			return true;
	}

	return UTIL_IsHolidayActive( eHoliday );
}

struct StatueInfo_t
{
	char const *pszMapName;
	Vector vecOrigin;
	QAngle vecAngles;
};

static StatueInfo_t s_StatueMaps[] ={
	{"ctf_2fort",			{483, 613, 0},			{0, 180, 0}},
	{"cp_dustbowl",			{-596, 2650, -256},		{0, 180, 0}},
	{"cp_granary",			{-544, -510, -416},		{0, 180, 0}},
	{"cp_well",				{1255, 515, -512},		{0, 180, 0}},
	{"cp_foundry",			{-85, 912, 0},			{0, -90, 0}},
	{"cp_gravelpit",		{-4624, 660, -512},		{0, 0, 0}},
	{"ctf_well",			{1000, -240, -512},		{0, 180, 0}},
	{"cp_badlands",			{808, -1079, 64},		{0, 135, 0}},
	{"pl_goldrush",			{-2780, -650, 0},		{0, 90, 0}},
	{"pl_badwater",			{2690, -416, 131},		{0, -90, 0}},
	{"plr_pipeline",		{220, -2527, 128},		{0, 90, 0}},
	{"cp_gorge",			{-6970, 5920, -42},		{0, 0, 0}},
	{"ctf_doublecross",		{1304, -206, 8},		{0, 180, 0}},
	{"pl_thundermountain",	{-720, -1058, 128},		{0, -90, 0}},
	{"cp_mountainlab",		{-2930, 1606, -1069},	{0, 90, 0}},
	{"cp_degrootkeep",		{-1000, 4580, -255},	{0, -25, 0}},
	{"pl_barnblitz",		{3415, -2144, -54},		{0, 90, 0}},
	{"pl_upwward",			{-736, -2275, 63},		{0, 90, 0}},
	{"plr_hightower",		{5632, 7747, 8},		{0, 0, 0}},
	{"koth_viaduct",		{-979, 0, 240},			{0, 180, 0}},
	{"koth_king",			{715, -395, -224},		{0, 135, 0}},
	{"sd_doomsday",			{-1025, 675, 128},		{0, 90, 0}},
	{"cp_mercenarypark",	{-2800, -775, -40},		{0, 0, 0}},
	{"ctf_turbine",			{718, 0, -256},			{0, 180, 0}},
	{"koth_harvest_final",	{-1428, 220, -15},		{0, 0, 0}},
	{"pl_swiftwater_final", {706, -2785, -934},		{0, 0, 0}},
	{"pl_frontier_final",	{3070, -3013, -193},	{0, -90, 0}},
	{"cp_process_final",	{650, -980, 535},		{0, 90, 0}},
	{"cp_gullywash_final",  {200, 83, 47},			{0, -102, 0}},
	{"cp_sunshine",			{-4725, 5860, 65},		{0, 180, 0}},
};

/**
 * Player hull & eye position for standing, ducking, etc.  This version has a taller
 * player height, but goldsrc-compatible collision bounds.
 */
static CViewVectors g_TFViewVectors(
	Vector( 0, 0, 72 ),		//VEC_VIEW (m_vView) eye position

	Vector( -24, -24, 0 ),	//VEC_HULL_MIN (m_vHullMin) hull min
	Vector( 24, 24, 82 ),	//VEC_HULL_MAX (m_vHullMax) hull max

	Vector( -24, -24, 0 ),	//VEC_DUCK_HULL_MIN (m_vDuckHullMin) duck hull min
	Vector( 24, 24, 55 ),	//VEC_DUCK_HULL_MAX	(m_vDuckHullMax) duck hull max
	Vector( 0, 0, 45 ),		//VEC_DUCK_VIEW		(m_vDuckView) duck view

	Vector( -10, -10, -10 ),	//VEC_OBS_HULL_MIN	(m_vObsHullMin) observer hull min
	Vector( 10, 10, 10 ),	//VEC_OBS_HULL_MAX	(m_vObsHullMax) observer hull max

	Vector( 0, 0, 14 )		//VEC_DEAD_VIEWHEIGHT (m_vDeadViewHeight) dead view height
);

Vector g_TFClassViewVectors[TF_CLASS_COUNT_ALL] =
{
	Vector( 0, 0, 72 ),		// TF_CLASS_UNDEFINED

	Vector( 0, 0, 65 ),		// TF_CLASS_SCOUT,			// TF_FIRST_NORMAL_CLASS
	Vector( 0, 0, 75 ),		// TF_CLASS_SNIPER,
	Vector( 0, 0, 68 ),		// TF_CLASS_SOLDIER,
	Vector( 0, 0, 68 ),		// TF_CLASS_DEMOMAN,
	Vector( 0, 0, 75 ),		// TF_CLASS_MEDIC,
	Vector( 0, 0, 75 ),		// TF_CLASS_HEAVYWEAPONS,
	Vector( 0, 0, 68 ),		// TF_CLASS_PYRO,
	Vector( 0, 0, 75 ),		// TF_CLASS_SPY,
	Vector( 0, 0, 68 ),		// TF_CLASS_ENGINEER,
	Vector( 0, 0, 68 ),		// TF_CLASS_SAXTON,
};

const CViewVectors *CTFGameRules::GetViewVectors() const
{
	return &g_TFViewVectors;
}

REGISTER_GAMERULES_CLASS( CTFGameRules );

BEGIN_NETWORK_TABLE_NOBASE( CTFGameRules, DT_TFGameRules )
#ifdef CLIENT_DLL

	RecvPropInt( RECVINFO( m_nGameType ) ),
	RecvPropString( RECVINFO( m_pszTeamGoalStringRed ) ),
	RecvPropString( RECVINFO( m_pszTeamGoalStringBlue ) ),
	RecvPropString( RECVINFO( m_pszTeamGoalStringGreen ) ),
	RecvPropString( RECVINFO( m_pszTeamGoalStringYellow ) ),
	RecvPropTime( RECVINFO( m_flCapturePointEnableTime ) ),
	RecvPropInt( RECVINFO( m_nHudType ) ),
	RecvPropBool( RECVINFO( m_bPlayingKoth ) ),
	RecvPropBool( RECVINFO( m_bPlayingVSH ) ),
	RecvPropBool( RECVINFO( m_bPlayingDR ) ),
	RecvPropBool( RECVINFO( m_bPlayingMedieval ) ),
	RecvPropBool( RECVINFO( m_bPlayingHybrid_CTF_CP ) ),
	RecvPropBool( RECVINFO( m_bPlayingSpecialDeliveryMode ) ),
	RecvPropBool( RECVINFO( m_bPlayingRobotDestructionMode ) ),
	RecvPropBool( RECVINFO( m_bPlayingMannVsMachine ) ),
	RecvPropBool( RECVINFO( m_bCompetitiveMode ) ),
	RecvPropBool( RECVINFO( m_bPowerupMode ) ),
	RecvPropBool( RECVINFO( m_bFourTeamMode ) ),
	RecvPropEHandle( RECVINFO( m_hRedKothTimer ) ),
	RecvPropEHandle( RECVINFO( m_hBlueKothTimer ) ),
	RecvPropEHandle( RECVINFO( m_hGreenKothTimer ) ), 
	RecvPropEHandle( RECVINFO( m_hYellowKothTimer ) ),
	RecvPropInt( RECVINFO( m_nMapHolidayType ) ),
	RecvPropEHandle( RECVINFO( m_itHandle ) ),
	RecvPropInt( RECVINFO( m_halloweenScenario ) ),
	RecvPropString( RECVINFO( m_pszCustomUpgradesFile ) ),
	RecvPropBool( RECVINFO( m_bMannVsMachineAlarmStatus ) ),
	RecvPropBool( RECVINFO( m_bHaveMinPlayersToEnableReady ) ),
#else
	SendPropInt( SENDINFO( m_nGameType ), 4, SPROP_UNSIGNED ),
	SendPropString( SENDINFO( m_pszTeamGoalStringRed ) ),
	SendPropString( SENDINFO( m_pszTeamGoalStringBlue ) ),
	SendPropString( SENDINFO( m_pszTeamGoalStringGreen ) ),
	SendPropString( SENDINFO( m_pszTeamGoalStringYellow ) ),
	SendPropTime( SENDINFO( m_flCapturePointEnableTime ) ),
	SendPropInt( SENDINFO( m_nHudType ) ),
	SendPropBool( SENDINFO( m_bPlayingKoth ) ),
	SendPropBool( SENDINFO( m_bPlayingVSH ) ),
	SendPropBool( SENDINFO( m_bPlayingDR ) ),
	SendPropBool( SENDINFO( m_bPlayingMedieval ) ),
	SendPropBool( SENDINFO( m_bPlayingHybrid_CTF_CP ) ),
	SendPropBool( SENDINFO( m_bPlayingSpecialDeliveryMode ) ),
	SendPropBool( SENDINFO( m_bPlayingRobotDestructionMode ) ),
	SendPropBool( SENDINFO( m_bPlayingMannVsMachine ) ),
	SendPropBool( SENDINFO( m_bCompetitiveMode ) ),
	SendPropBool( SENDINFO( m_bPowerupMode ) ),
	SendPropBool( SENDINFO( m_bFourTeamMode ) ),
	SendPropEHandle( SENDINFO( m_hRedKothTimer ) ),
	SendPropEHandle( SENDINFO( m_hBlueKothTimer ) ),
	SendPropEHandle( SENDINFO( m_hGreenKothTimer ) ), 
	SendPropEHandle( SENDINFO( m_hYellowKothTimer ) ),
	SendPropInt( SENDINFO( m_nMapHolidayType ), 3, SPROP_UNSIGNED ),
	SendPropEHandle( SENDINFO( m_itHandle ) ),
	SendPropInt( SENDINFO( m_halloweenScenario ) ),
	SendPropString( SENDINFO( m_pszCustomUpgradesFile ) ),
	SendPropBool( SENDINFO( m_bMannVsMachineAlarmStatus ) ),
	SendPropBool( SENDINFO( m_bHaveMinPlayersToEnableReady ) ),
#endif
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tf_gamerules, CTFGameRulesProxy );
IMPLEMENT_NETWORKCLASS_ALIASED( TFGameRulesProxy, DT_TFGameRulesProxy )

#ifdef CLIENT_DLL
void RecvProxy_TFGameRules( const RecvProp *pProp, void **pOut, void *pData, int objectID )
{
	CTFGameRules *pRules = TFGameRules();
	Assert( pRules );
	*pOut = pRules;
}

BEGIN_RECV_TABLE( CTFGameRulesProxy, DT_TFGameRulesProxy )
	RecvPropDataTable( "tf_gamerules_data", 0, 0, &REFERENCE_RECV_TABLE( DT_TFGameRules ), RecvProxy_TFGameRules )
END_RECV_TABLE()
#else
void *SendProxy_TFGameRules( const SendProp *pProp, const void *pStructBase, const void *pData, CSendProxyRecipients *pRecipients, int objectID )
{
	CTFGameRules *pRules = TFGameRules();
	Assert( pRules );
	pRecipients->SetAllRecipients();
	return pRules;
}

BEGIN_SEND_TABLE( CTFGameRulesProxy, DT_TFGameRulesProxy )
	SendPropDataTable( "tf_gamerules_data", 0, &REFERENCE_SEND_TABLE( DT_TFGameRules ), SendProxy_TFGameRules )
END_SEND_TABLE()
#endif

#ifdef GAME_DLL
BEGIN_DATADESC( CTFGameRulesProxy )
	DEFINE_KEYFIELD( m_iHud_Type, FIELD_INTEGER, "hud_type" ),
	DEFINE_KEYFIELD( m_bFourTeamMode, FIELD_BOOLEAN, "fourteammode"),
	//DEFINE_KEYFIELD( m_bCTF_Overtime, FIELD_BOOLEAN, "ctf_overtime" ),

	// Inputs.
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetRedTeamRespawnWaveTime", InputSetRedTeamRespawnWaveTime ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetBlueTeamRespawnWaveTime", InputSetBlueTeamRespawnWaveTime ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetGreenTeamRespawnWaveTime", InputSetGreenTeamRespawnWaveTime ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetYellowTeamRespawnWaveTime", InputSetYellowTeamRespawnWaveTime ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "AddRedTeamRespawnWaveTime", InputAddRedTeamRespawnWaveTime ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "AddBlueTeamRespawnWaveTime", InputAddBlueTeamRespawnWaveTime ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "AddGreenTeamRespawnWaveTime", InputAddGreenTeamRespawnWaveTime ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "AddYelloTeamRespawnWaveTime", InputAddYellowTeamRespawnWaveTime ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetRedTeamGoalString", InputSetRedTeamGoalString ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetBlueTeamGoalString", InputSetBlueTeamGoalString ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetGreenTeamGoalString", InputSetGreenTeamGoalString ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetYellowTeamGoalString", InputSetYellowTeamGoalString ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetRedTeamRole", InputSetRedTeamRole ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetBlueTeamRole", InputSetBlueTeamRole ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetGreenTeamRole", InputSetGreenTeamRole ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetYellowTeamRole", InputSetYellowTeamRole ),
	//DEFINE_INPUTFUNC( FIELD_STRING, "SetRequiredObserverTarget", InputSetRequiredObserverTarget ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "AddRedTeamScore", InputAddRedTeamScore ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "AddBlueTeamScore", InputAddBlueTeamScore ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "AddGreenTeamScore", InputAddGreenTeamScore ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "AddYellowTeamScore", InputAddYellowTeamScore) ,

	DEFINE_INPUTFUNC( FIELD_VOID, "SetRedKothClockActive", InputSetRedKothClockActive ),
	DEFINE_INPUTFUNC( FIELD_VOID, "SetBlueKothClockActive", InputSetBlueKothClockActive ),
	DEFINE_INPUTFUNC( FIELD_VOID, "SetGreenKothClockActive", InputSetGreenKothClockActive ),
	DEFINE_INPUTFUNC( FIELD_VOID, "SetYellowKothClockActive", InputSetYellowKothClockActive ),

	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetCTFCaptureBonusTime", InputSetCTFCaptureBonusTime ),

	DEFINE_INPUTFUNC( FIELD_STRING, "PlayVORed", InputPlayVORed ),
	DEFINE_INPUTFUNC( FIELD_STRING, "PlayVOBlue", InputPlayVOBlue ),
	DEFINE_INPUTFUNC( FIELD_STRING, "PlayVOGreen", InputPlayVOGreen ),
	DEFINE_INPUTFUNC( FIELD_STRING, "PlayVOYellow", InputPlayVOYellow ),
	DEFINE_INPUTFUNC( FIELD_STRING, "PlayVO", InputPlayVO ),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetRedTeamRespawnWaveTime( inputdata_t &inputdata )
{
	TFGameRules()->SetTeamRespawnWaveTime( TF_TEAM_RED, inputdata.value.Float() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetBlueTeamRespawnWaveTime( inputdata_t &inputdata )
{
	TFGameRules()->SetTeamRespawnWaveTime( TF_TEAM_BLUE, inputdata.value.Float() );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetGreenTeamRespawnWaveTime(inputdata_t &inputdata)
{
	TFGameRules()->SetTeamRespawnWaveTime(TF_TEAM_GREEN, inputdata.value.Float());
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetYellowTeamRespawnWaveTime(inputdata_t &inputdata)
{
	TFGameRules()->SetTeamRespawnWaveTime(TF_TEAM_YELLOW, inputdata.value.Float());
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputAddRedTeamRespawnWaveTime( inputdata_t &inputdata )
{
	TFGameRules()->AddTeamRespawnWaveTime( TF_TEAM_RED, inputdata.value.Float() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputAddBlueTeamRespawnWaveTime( inputdata_t &inputdata )
{
	TFGameRules()->AddTeamRespawnWaveTime( TF_TEAM_BLUE, inputdata.value.Float() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputAddGreenTeamRespawnWaveTime(inputdata_t &inputdata)
{
	TFGameRules()->AddTeamRespawnWaveTime(TF_TEAM_GREEN, inputdata.value.Float());
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputAddYellowTeamRespawnWaveTime(inputdata_t &inputdata)
{
	TFGameRules()->AddTeamRespawnWaveTime(TF_TEAM_YELLOW, inputdata.value.Float());
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetRedTeamGoalString( inputdata_t &inputdata )
{
	TFGameRules()->SetTeamGoalString( TF_TEAM_RED, inputdata.value.String() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetBlueTeamGoalString( inputdata_t &inputdata )
{
	TFGameRules()->SetTeamGoalString( TF_TEAM_BLUE, inputdata.value.String() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetGreenTeamGoalString(inputdata_t &inputdata)
{
	TFGameRules()->SetTeamGoalString(TF_TEAM_GREEN, inputdata.value.String());
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetYellowTeamGoalString(inputdata_t &inputdata)
{
	TFGameRules()->SetTeamGoalString(TF_TEAM_YELLOW, inputdata.value.String());
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetRedTeamRole( inputdata_t &inputdata )
{
	CTFTeam *pTeam = TFTeamMgr()->GetTeam( TF_TEAM_RED );
	if ( pTeam )
	{
		pTeam->SetRole( inputdata.value.Int() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetBlueTeamRole( inputdata_t &inputdata )
{
	CTFTeam *pTeam = TFTeamMgr()->GetTeam( TF_TEAM_BLUE );
	if ( pTeam )
	{
		pTeam->SetRole( inputdata.value.Int() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetGreenTeamRole( inputdata_t &inputdata )
{
	CTFTeam *pTeam = TFTeamMgr()->GetTeam( TF_TEAM_GREEN );
	if ( pTeam )
	{
		pTeam->SetRole( inputdata.value.Int() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetYellowTeamRole( inputdata_t &inputdata )
{
	CTFTeam *pTeam = TFTeamMgr()->GetTeam( TF_TEAM_YELLOW );
	if ( pTeam )
	{
		pTeam->SetRole( inputdata.value.Int() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputAddRedTeamScore( inputdata_t &inputdata )
{
	CTFTeam *pTeam = TFTeamMgr()->GetTeam( TF_TEAM_RED );
	if ( pTeam )
	{
		pTeam->AddScore( inputdata.value.Int() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputAddBlueTeamScore( inputdata_t &inputdata )
{
	CTFTeam *pTeam = TFTeamMgr()->GetTeam( TF_TEAM_BLUE );
	if ( pTeam )
	{
		pTeam->AddScore( inputdata.value.Int() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputAddGreenTeamScore( inputdata_t &inputdata )
{
	CTFTeam *pTeam = TFTeamMgr()->GetTeam( TF_TEAM_GREEN );
	if ( pTeam )
	{
		pTeam->AddScore( inputdata.value.Int() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputAddYellowTeamScore( inputdata_t &inputdata )
{
	CTFTeam *pTeam = TFTeamMgr()->GetTeam( TF_TEAM_YELLOW );
	if ( pTeam )
	{
		pTeam->AddScore( inputdata.value.Int() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetRedKothClockActive( inputdata_t &inputdata )
{
	if ( TFGameRules() && TFGameRules()->GetRedKothRoundTimer() )
	{
		TFGameRules()->GetRedKothRoundTimer()->InputEnable( inputdata );

		if ( TFGameRules()->GetBlueKothRoundTimer() )
			TFGameRules()->GetBlueKothRoundTimer()->InputDisable( inputdata );
		
		if ( TFGameRules()->IsFourTeamGame() )
		{
			if ( TFGameRules()->GetGreenKothRoundTimer() )
				TFGameRules()->GetGreenKothRoundTimer()->InputDisable( inputdata );

			if ( TFGameRules()->GetYellowKothRoundTimer() )
				TFGameRules()->GetYellowKothRoundTimer()->InputDisable( inputdata );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetBlueKothClockActive( inputdata_t &inputdata )
{
	if ( TFGameRules() && TFGameRules()->GetBlueKothRoundTimer() )
	{
		TFGameRules()->GetBlueKothRoundTimer()->InputEnable( inputdata );

		if ( TFGameRules()->GetRedKothRoundTimer() )
			TFGameRules()->GetRedKothRoundTimer()->InputDisable( inputdata );
		
		if ( TFGameRules()->IsFourTeamGame() )
		{
			if ( TFGameRules()->GetGreenKothRoundTimer() )
				TFGameRules()->GetGreenKothRoundTimer()->InputDisable( inputdata );

			if ( TFGameRules()->GetYellowKothRoundTimer() )
				TFGameRules()->GetYellowKothRoundTimer()->InputDisable( inputdata );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetGreenKothClockActive(inputdata_t &inputdata)
{
	if ( TFGameRules() && !TFGameRules()->IsFourTeamGame() )
	{
		Warning( "SetGreenKothClockActive called, but 4 team mode isn't on!\n" );
		return;
	}

	if ( TFGameRules() && TFGameRules()->GetGreenKothRoundTimer() )
	{
		TFGameRules()->GetGreenKothRoundTimer()->InputEnable( inputdata );

		if ( TFGameRules()->GetRedKothRoundTimer() )
			TFGameRules()->GetRedKothRoundTimer()->InputDisable( inputdata );

		if ( TFGameRules()->GetBlueKothRoundTimer() )
			TFGameRules()->GetBlueKothRoundTimer()->InputDisable( inputdata );

		if ( TFGameRules()->GetYellowKothRoundTimer() )
			TFGameRules()->GetYellowKothRoundTimer()->InputDisable( inputdata );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetYellowKothClockActive( inputdata_t &inputdata )
{
	if ( TFGameRules() && !TFGameRules()->IsFourTeamGame() )
	{
		Warning( "SetYellowKothClockActive called, but 4 team mode isn't on!\n" );
		return;
	}

	if ( TFGameRules() && TFGameRules()->GetYellowKothRoundTimer() )
	{
		TFGameRules()->GetYellowKothRoundTimer()->InputEnable( inputdata );

		if ( TFGameRules()->GetRedKothRoundTimer() )
			TFGameRules()->GetRedKothRoundTimer()->InputDisable( inputdata );

		if ( TFGameRules()->GetBlueKothRoundTimer() )
			TFGameRules()->GetBlueKothRoundTimer()->InputDisable( inputdata );

		if ( TFGameRules()->GetGreenKothRoundTimer() )
			TFGameRules()->GetGreenKothRoundTimer()->InputDisable( inputdata );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetCTFCaptureBonusTime( inputdata_t &inputdata )
{
	if ( TFGameRules() )
	{
		TFGameRules()->m_flCTFBonusTime = inputdata.value.Float();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputPlayVO( inputdata_t &inputdata )
{
	if ( TFGameRules() )
	{
		TFGameRules()->BroadcastSound( 255, inputdata.value.String() );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputPlayVORed( inputdata_t &inputdata )
{
	if ( TFGameRules() )
	{
		TFGameRules()->BroadcastSound( TF_TEAM_RED, inputdata.value.String() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputPlayVOBlue( inputdata_t &inputdata )
{
	if ( TFGameRules() )
	{
		TFGameRules()->BroadcastSound( TF_TEAM_BLUE, inputdata.value.String() );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputPlayVOGreen( inputdata_t &inputdata )
{
	if ( TFGameRules() && TFGameRules()->IsFourTeamGame() )
	{
		TFGameRules()->BroadcastSound( TF_TEAM_GREEN, inputdata.value.String() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputPlayVOYellow( inputdata_t &inputdata )
{
	if ( TFGameRules() && TFGameRules()->IsFourTeamGame() )
	{
		TFGameRules()->BroadcastSound( TF_TEAM_YELLOW, inputdata.value.String() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::Activate()
{
	TFGameRules()->m_bFourTeamMode = m_bFourTeamMode;
		
	TFGameRules()->Activate();

	TFGameRules()->SetHudType( m_iHud_Type );

	BaseClass::Activate();
}

#endif

// (We clamp ammo ourselves elsewhere).
ConVar ammo_max( "ammo_max", "5000", FCVAR_REPLICATED );

#ifndef CLIENT_DLL
ConVar sk_plr_dmg_grenade( "sk_plr_dmg_grenade", "0" );		// Very lame that the base code needs this defined
#endif

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iDmgType - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTFGameRules::Damage_IsTimeBased( int iDmgType )
{
	// Damage types that are time-based.
	return ( ( iDmgType & ( DMG_PARALYZE | DMG_NERVEGAS | DMG_DROWNRECOVER ) ) != 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iDmgType - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTFGameRules::Damage_ShowOnHUD( int iDmgType )
{
	// Damage types that have client HUD art.
	return ( ( iDmgType & ( DMG_DROWN | DMG_BURN | DMG_NERVEGAS | DMG_SHOCK ) ) != 0 );
}
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iDmgType - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTFGameRules::Damage_ShouldNotBleed( int iDmgType )
{
	// Should always bleed currently.
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGameRules::Damage_GetTimeBased( void )
{
	int iDamage = ( DMG_PARALYZE | DMG_NERVEGAS | DMG_DROWNRECOVER );
	return iDamage;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGameRules::Damage_GetShowOnHud( void )
{
	int iDamage = ( DMG_DROWN | DMG_BURN | DMG_NERVEGAS | DMG_SHOCK );
	return iDamage;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFGameRules::Damage_GetShouldNotBleed( void )
{
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::HaveSavedConvar( ConVarRef const& cvar )
{
	Assert( cvar.IsValid() );

	UtlSymId_t iSymbol = m_SavedConvars.Find( cvar.GetName() );
	if ( iSymbol == m_SavedConvars.InvalidIndex() )
		return false;

	return m_SavedConvars[ iSymbol ] != NULL_STRING;
}

#if defined( CLIENT_DLL )
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void __MsgFunc_SavedConvar( bf_read &msg )
{
	Assert( TFGameRules() );
	if ( !TFGameRules() )
		return;

	char szKey[64];
	bool bReadKey = msg.ReadString( szKey, sizeof( szKey ) );
	Assert( bReadKey );

	char szValue[256];
	bool bReadValue = msg.ReadString( szValue, sizeof( szValue ) );
	Assert( bReadValue );

	if ( bReadKey && bReadValue )
	{
		ConVarRef cvar( szKey );

		if ( cvar.IsValid() && cvar.IsFlagSet( FCVAR_REPLICATED ) )
			TFGameRules()->m_SavedConvars[ szKey ] = AllocPooledString( szValue );
	}
}
USER_MESSAGE_REGISTER( SavedConvar );
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::SaveConvar( ConVarRef const& cvar )
{
	Assert( cvar.IsValid() );

	if ( HaveSavedConvar( cvar ) )
	{
		// already saved, don't override.
		return;
	}

#if defined( GAME_DLL )
	// BenLubar: Send saved replicated convars to the client so that it can reset them if the player disconnects.
	if ( cvar.IsFlagSet( FCVAR_REPLICATED ) )
	{
		CReliableBroadcastRecipientFilter filter;
		UserMessageBegin( filter, "SavedConvar" );
			WRITE_STRING( cvar.GetName() );
			WRITE_STRING( cvar.GetString() );
		MessageEnd();
	}
#endif
	m_SavedConvars[ cvar.GetName() ] = AllocPooledString( cvar.GetString() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::RevertSingleConvar( ConVarRef &cvar )
{
	Assert( cvar.IsValid() );

	if ( !HaveSavedConvar( cvar ) )
	{
		// don't have a saved value
		return;
	}

	string_t &saved = m_SavedConvars[ cvar.GetName() ];
	cvar.SetValue( STRING( saved ) );
	saved = NULL_STRING;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::RevertSavedConvars()
{
	// revert saved convars
	for ( int i = 0; i < m_SavedConvars.GetNumStrings(); i++ )
	{
		const char *pszName = m_SavedConvars.String( i );
		string_t iszValue = m_SavedConvars[i];

		ConVarRef cvar( pszName );
		Assert( cvar.IsValid() );

		if ( iszValue != NULL_STRING )
			cvar.SetValue( STRING( iszValue ) );
	}

	m_SavedConvars.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFGameRules::CTFGameRules()
{
#if defined( GAME_DLL )
	// Create teams.
	TFTeamMgr()->Init();

	ResetMapTime();

	// Create the team managers
//	for ( int i = 0; i < ARRAYSIZE( teamnames ); i++ )
//	{
//		CTeam *pTeam = static_cast<CTeam*>(CreateEntityByName( "tf_team" ));
//		pTeam->Init( sTeamNames[i], i );
//
//		g_Teams.AddToTail( pTeam );
//	}

	m_flIntermissionEndTime = 0.0f;
	m_flNextPeriodicThink = 0.0f;

	ListenForGameEvent( "teamplay_point_captured" );
	ListenForGameEvent( "teamplay_capture_blocked" );
	ListenForGameEvent( "teamplay_round_win" );
	ListenForGameEvent( "teamplay_flag_event" );
	ListenForGameEvent( "teamplay_point_unlocked" );

	Q_memset( m_vecPlayerPositions, 0, sizeof( m_vecPlayerPositions ) );

	m_iPrevRoundState = -1;
	m_iCurrentRoundState = -1;
	m_iCurrentMiniRoundMask = 0;
	m_flTimerMayExpireAt = -1.0f;

	m_bFirstBlood = false;
	m_iArenaTeamCount = 0;
	m_flCTFBonusTime = -1;

	// Lets execute a map specific cfg file
	// ** execute this after server.cfg!
	char szCommand[32];
	Q_snprintf( szCommand, sizeof( szCommand ), "exec %s.cfg\n", STRING( gpGlobals->mapname ) );
	engine->ServerCommand( szCommand );

#else // GAME_DLL

	ListenForGameEvent( "game_newmap" );
	ListenForGameEvent( "recalculate_holidays" );

	SetUpVisionFilterKeyValues();

#endif

	m_flCapturePointEnableTime = 0;

	// Initialize the game type
	m_nGameType.Set( TF_GAMETYPE_UNDEFINED );

	// Initialize the classes here.
	InitPlayerClasses();

	// Set turbo physics on.  Do it here for now.
	sv_turbophysics.SetValue( 1 );

	// Initialize the team manager here, etc...

	// If you hit these asserts its because you added or removed a weapon type 
	// and didn't also add or remove the weapon name or damage type from the
	// arrays defined in tf_shareddefs.cpp
	Assert( g_aWeaponDamageTypes[TF_WEAPON_COUNT] == TF_DMG_SENTINEL_VALUE );
	Assert( FStrEq( g_aWeaponNames[TF_WEAPON_COUNT], "TF_WEAPON_COUNT" ) );

	m_flGravityScale = 1.0;

	m_iPreviousRoundWinners = TEAM_UNASSIGNED;

	m_pszTeamGoalStringRed.GetForModify()[0] = '\0';
	m_pszTeamGoalStringBlue.GetForModify()[0] = '\0';
	m_pszTeamGoalStringGreen.GetForModify()[0] = '\0';
	m_pszTeamGoalStringYellow.GetForModify()[0] = '\0';

#ifdef GAME_DLL
	const char *szMapname = STRING( gpGlobals->mapname );
	if ( !Q_strncmp( szMapname, "cp_manor_event", MAX_MAP_NAME ) )
		m_halloweenScenario = HALLOWEEN_SCENARIO_MANOR;
	else if ( !Q_strncmp( szMapname, "koth_viaduct_event", MAX_MAP_NAME ) )
		m_halloweenScenario = HALLOWEEN_SCENARIO_VIADUCT;
	else if ( !Q_strncmp( szMapname, "koth_lakeside_event", MAX_MAP_NAME ) )
		m_halloweenScenario = HALLOWEEN_SCENARIO_LAKESIDE;
	else if ( !Q_strncmp( szMapname, "plr_hightower_event", MAX_MAP_NAME ) )
		m_halloweenScenario = HALLOWEEN_SCENARIO_HIGHTOWER;
	else if ( !Q_strncmp( szMapname, "sd_doomsday_event", MAX_MAP_NAME ) )
		m_halloweenScenario = HALLOWEEN_SCENARIO_DOOMSDAY;
#endif
	m_iGlobalAttributeCacheVersion = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::FlagsMayBeCapped( void )
{
	if ( State_Get() != GR_STATE_TEAM_WIN )
		return true;

	return false;
}

#ifdef GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: Determines whether we should allow mp_timelimit to trigger a map change
//-----------------------------------------------------------------------------
bool CTFGameRules::CanChangelevelBecauseOfTimeLimit( void )
{
	CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;

	// we only want to deny a map change triggered by mp_timelimit if we're not forcing a map reset,
	// we're playing mini-rounds, and the master says we need to play all of them before changing (for maps like Dustbowl)
	if ( !m_bForceMapReset && pMaster && pMaster->PlayingMiniRounds() && pMaster->ShouldPlayAllControlPointRounds() )
	{
		if ( pMaster->NumPlayableControlPointRounds() )
		{
			return false;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::CanGoToStalemate( void )
{
	// In CTF, don't go to stalemate if one of the flags isn't at home
	if ( m_nGameType == TF_GAMETYPE_CTF )
	{
		CCaptureFlag *pFlag = dynamic_cast<CCaptureFlag *> ( gEntList.FindEntityByClassname( NULL, "item_teamflag" ) );
		while ( pFlag )
		{
			if ( pFlag->IsDropped() || pFlag->IsStolen() )
				return false;

			pFlag = dynamic_cast<CCaptureFlag *> ( gEntList.FindEntityByClassname( pFlag, "item_teamflag" ) );
		}

		// check that one team hasn't won by capping
		if ( CheckCapsPerRound() )
			return false;
	}

	if ( m_nGameType == TF_GAMETYPE_ESCORT )
		return false;

	return BaseClass::CanGoToStalemate();
}

// Classnames of entities that are preserved across round restarts
static const char *s_PreserveEnts[] =
{
	"tf_gamerules",
	"tf_team_manager",
	"tf_player_manager",
	"tf_team",
	"tf_objective_resource",
	"keyframe_rope",
	"move_rope",
	"tf_viewmodel",
	"tf_logic_training",
	"tf_logic_training_mode",
	"tf_powerup_bottle",
	"tf_mann_vs_machine_stats",
	"tf_wearable",
	"tf_wearable_demoshield",
	"tf_wearable_robot_arm",
	"tf_wearable_vm",
	"tf_logic_bonusround",
	"vote_controller",
	"monster_resource",
	"tf_logic_medieval",
	"tf_logic_cp_timer",
	"tf_logic_tower_defense",
	"tf_logic_mann_vs_machine",
	"func_upgradestation"
	"entity_rocket",
	"entity_carrier",
	"entity_sign",
	"entity_suacer",
	"info_ladder",
	"prop_vehicle_jeep",
	"", // END Marker
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::Activate()
{
	m_nGameType.Set( TF_GAMETYPE_UNDEFINED );

	tf_gamemode_arena.SetValue( 0 );
	tf_gamemode_cp.SetValue( 0 );
	tf_gamemode_ctf.SetValue( 0 );
	tf_gamemode_sd.SetValue( 0 );
	tf_gamemode_payload.SetValue( 0 );
	tf_gamemode_plr.SetValue( 0 );
	tf_gamemode_mvm.SetValue( 0 );
	tf_gamemode_rd.SetValue( 0 );
	tf_gamemode_passtime.SetValue( 0 );
	tf_gamemode_koth.SetValue( 0 );
	tf_gamemode_medieval.SetValue( 0 );
	tf_gamemode_vsh.SetValue( 0 );
	tf_gamemode_dr.SetValue( 0 );
	tf_gamemode_pd.SetValue( 0 );
	tf_gamemode_tc.SetValue( 0 );

	m_bPlayingKoth.Set( false );
	m_bPlayingMedieval.Set( false );
	m_bPlayingHybrid_CTF_CP.Set( false );
	m_bPlayingSpecialDeliveryMode.Set( false );
	m_bPlayingMannVsMachine.Set( false );
	m_bMannVsMachineAlarmStatus.Set( false );
	m_bPlayingRobotDestructionMode.Set( false );
	m_bPowerupMode.Set( false );

	TeamplayRoundBasedRules()->SetMultipleTrains( false );

	m_hRedAttackTrain = NULL;
	m_hBlueAttackTrain = NULL;
	m_hRedDefendTrain = NULL;
	m_hBlueDefendTrain = NULL;

	m_hBlueBotRoster = NULL;
	m_hRedBotRoster = NULL;

	m_nMapHolidayType.Set( kHoliday_None );

	if ( !Q_strncmp( STRING( gpGlobals->mapname ), "tc_", 3 )  )
	{
		tf_gamemode_tc.SetValue( 1 );
	}

	CMedievalLogic *pMedieval = dynamic_cast<CMedievalLogic *>( gEntList.FindEntityByClassname( NULL, "tf_logic_medieval" ) );
	if ( pMedieval || tf_medieval.GetBool() )
	{
		m_nGameType.Set( TF_GAMETYPE_MEDIEVAL );
		tf_gamemode_medieval.SetValue( 1 );
		m_bPlayingMedieval = true;
	}
	
	CArenaLogic *pArena = dynamic_cast<CArenaLogic *>( gEntList.FindEntityByClassname( NULL, "tf_logic_arena" ) );
	if ( pArena )
	{
		m_hArenaLogic = pArena;

		m_nGameType.Set( TF_GAMETYPE_ARENA );
		tf_gamemode_arena.SetValue( 1 );

		Msg( "Executing server arena config file\n" );
		engine->ServerCommand( "exec config_arena.cfg \n" );
	}

	/*if ( CTFRobotDestructionLogic::GetRobotDestructionLogic() )
	{
		m_bPlayingRobotDestructionMode.Set( true );
		if ( CTFRobotDestructionLogic::GetRobotDestructionLogic()->GetType() == CTFRobotDestructionLogic::TYPE_ROBOT_DESTRUCTION )
		{
			tf_gamemode_rd.SetValue( 1 );
			m_nGameType.Set( TF_GAMETYPE_RD );
		}
		else
		{
			tf_gamemode_pd.SetValue( 1 );
			m_nGameType.Set( TF_GAMETYPE_PD );
		}
	}
	else if ( dynamic_cast<CMannVsMachineLogic *>( gEntList.FindEntityByClassname( NULL, "tf_logic_mann_vs_machine" ) ) )
	{
		m_nGameType.Set( TF_GAMETYPE_MVM );
		tf_gamemode_mvm.SetValue( 1 );
		m_bPlayingMannVsMachine = true;
	} 
	else*/ if ( !Q_strncmp( STRING( gpGlobals->mapname ), "sd_", 3 )  )
	{
		tf_gamemode_sd.SetValue( 1 );
		m_bPlayingSpecialDeliveryMode = true;
	}
	else if ( ICaptureFlagAutoList::AutoList().Count() > 0 )
	{
		m_nGameType.Set( TF_GAMETYPE_CTF );
		tf_gamemode_ctf.SetValue( 1 );
	}
	else if ( dynamic_cast<CTeamTrainWatcher *>( gEntList.FindEntityByClassname( NULL, "team_train_watcher" ) ) )
	{
		m_nGameType.Set( TF_GAMETYPE_ESCORT );
		tf_gamemode_payload.SetValue( 1 );

		if ( dynamic_cast<CMultipleEscortLogic *>( gEntList.FindEntityByClassname( NULL, "tf_logic_multiple_escort" ) ) )
		{
			tf_gamemode_plr.SetValue( 1 );
			TeamplayRoundBasedRules()->SetMultipleTrains( true );
		}
	}
	else if ( g_hControlPointMasters.Count() && m_nGameType != TF_GAMETYPE_ARENA )
	{
		m_nGameType.Set( TF_GAMETYPE_CP );
		tf_gamemode_cp.SetValue( 1 );
	}

	if ( dynamic_cast<CKothLogic *>( gEntList.FindEntityByClassname( NULL, "tf_logic_koth" ) ) )
	{
		m_nGameType.Set( TF_GAMETYPE_CP );
		tf_gamemode_koth.SetValue( 1 );
		m_bPlayingKoth = true;
	}

	if ( dynamic_cast<CHybridMap_CTF_CP *>( gEntList.FindEntityByClassname( NULL, "tf_logic_hybrid_ctf_cp" ) ) )
	{
		m_nGameType.Set( TF_GAMETYPE_CP );
		m_bPlayingHybrid_CTF_CP = true;
	}

	CTFHolidayEntity *pHolidayEntity = dynamic_cast<CTFHolidayEntity*> ( gEntList.FindEntityByClassname( NULL, "tf_logic_holiday" ) );
	if ( pHolidayEntity )
	{
		m_nMapHolidayType = pHolidayEntity->GetHolidayType();
	}

	CTFBotRoster *pRoster = dynamic_cast<CTFBotRoster *>( gEntList.FindEntityByClassname( NULL, "bot_roster" ) );
	while ( pRoster )
	{
		if ( FStrEq( pRoster->GetTeamName(), "blue" ) )
		{
			m_hBlueBotRoster = pRoster;
		}
		else if ( FStrEq( pRoster->GetTeamName(), "red" ) )
		{
			m_hRedBotRoster = pRoster;
		}
		else
		{
			if ( !m_hBlueBotRoster )
				m_hBlueBotRoster = pRoster;

			if ( !m_hRedBotRoster )
				m_hRedBotRoster = pRoster;
		}

		pRoster = dynamic_cast<CTFBotRoster *>( gEntList.FindEntityByClassname( pRoster, "bot_roster" ) );
	}

	CreateSoldierStatue();

	if ( IsInTraining() || TheTFBots().IsInOfflinePractice() || IsInItemTestingMode() )
	{
		hide_server.SetValue( true );
	}

	if ( tf_gamemode_tc.GetBool() || tf_gamemode_sd.GetBool() || tf_gamemode_pd.GetBool() || tf_gamemode_medieval.GetBool() )
	{
		tf_gamemode_misc.SetValue( 1 );
	}

	/*if ( IsPVEModeActive() && g_pPopulationManager == NULL )
	{
		CreateEntityByName( "info_populator" );
	}*/
}

void CTFGameRules::OnNavMeshLoad( void )
{
	TheNavMesh->SetPlayerSpawnName( "info_player_teamspawn" );
}

void CTFGameRules::LevelShutdown( void )
{
	TheTFBots().OnLevelShutdown();
}

int CTFGameRules::GetClassLimit( int iDesiredClassIndex )
{
	int result;

	if ( IsInTournamentMode() || tf2v_classlimit.GetInt() == 1 /*||  *((_DWORD *)this + 462) == 7 */ )
	{
		if ( iDesiredClassIndex <= TF_LAST_NORMAL_CLASS )
		{
			switch ( iDesiredClassIndex )
			{
				default:
					result = -1;
				case TF_CLASS_ENGINEER:
					result = tf_tournament_classlimit_engineer.GetInt();
					break;
				case TF_CLASS_SPY:
					result = tf_tournament_classlimit_spy.GetInt();
					break;
				case TF_CLASS_PYRO:
					result = tf_tournament_classlimit_pyro.GetInt();
					break;
				case TF_CLASS_HEAVYWEAPONS:
					result = tf_tournament_classlimit_heavy.GetInt();
					break;
				case TF_CLASS_MEDIC:
					result = tf_tournament_classlimit_medic.GetInt();
					break;
				case TF_CLASS_DEMOMAN:
					result = tf_tournament_classlimit_demoman.GetInt();
					break;
				case TF_CLASS_SOLDIER:
					result = tf_tournament_classlimit_soldier.GetInt();
					break;
				case TF_CLASS_SNIPER:
					result = tf_tournament_classlimit_sniper.GetInt();
					break;
				case TF_CLASS_SCOUT:
					result = tf_tournament_classlimit_scout.GetInt();
					break;
			}
		}
		else
		{
			result = 1;
		}
	}
	else if ( IsInHighlanderMode() )
	{
		result = 1;
	}
	else if ( tf_classlimit.GetBool() )
	{
		result = tf_classlimit.GetInt();
	}
	else
	{
		result = -1;
	}

	return result;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::CanPlayerChooseClass( CBasePlayer *pPlayer, int iDesiredClassIndex )
{
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
	CTFTeam *pTFTeam = pTFPlayer->GetTFTeam();
	int iClassLimit = 0;
	int iClassCount = 0;

	if ( iDesiredClassIndex <= TF_LAST_NORMAL_CLASS ) 
	{
		iClassLimit = GetClassLimit( iDesiredClassIndex );

		if ( iClassLimit != -1 && pTFTeam && pTFPlayer->GetTeamNumber() >= TF_TEAM_RED )
		{
			for ( int i = 0; i < pTFTeam->GetNumPlayers(); i++ )
			{
				if ( pTFTeam->GetPlayer( i ) && pTFTeam->GetPlayer( i ) != pPlayer )
					iClassCount += iDesiredClassIndex == ToTFPlayer( pTFTeam->GetPlayer( i ) )->GetPlayerClass()->GetClassIndex();
			}

			return iClassLimit > iClassCount;
		}
		else
			return true;
	}
	else if ( !IsInVSHMode() && ( iDesiredClassIndex >= TF_FIRST_BOSS_CLASS ) )
		return false;
	else if ( IsInVSHMode() && ( iDesiredClassIndex >= TF_FIRST_BOSS_CLASS ) )
	{
		if ( pTFPlayer->GetTeamNumber() == TF_TEAM_PLAYER_BOSS )
			return true;
		else
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::CanBotChooseClass( CBasePlayer *pBot, int iDesiredClassIndex )
{
	switch ( pBot->GetTeamNumber() )
	{
		case TF_TEAM_RED:
		{
			if ( m_hRedBotRoster && !m_hRedBotRoster->IsClassAllowed( iDesiredClassIndex ) )
				return false;

			break;
		}
		case TF_TEAM_BLUE:
		{
			if ( m_hBlueBotRoster && !m_hBlueBotRoster->IsClassAllowed( iDesiredClassIndex ) )
				return false;

			break;
		}
	}

	return CanPlayerChooseClass( pBot, iDesiredClassIndex );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::CanBotChangeClass( CBasePlayer *pBot )
{
	CTFPlayer *pPlayer = ToTFPlayer( pBot );
	if ( !pPlayer || pPlayer->GetPlayerClass()->GetClassIndex() == TF_CLASS_UNDEFINED )
		return true;

	if ( IsMannVsMachineMode() || IsInVSHMode() )
		return false;

	switch ( pPlayer->GetTeamNumber() )
	{
		case TF_TEAM_RED:
		{
			if ( m_hRedBotRoster && !m_hRedBotRoster->IsClassChangeAllowed() )
				return false;

			break;
		}
		case TF_TEAM_BLUE:
		{
			if ( m_hBlueBotRoster && !m_hBlueBotRoster->IsClassChangeAllowed() )
				return false;

			break;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::AllowDamage( CBaseEntity *pVictim, const CTakeDamageInfo &info )
{
	bool bRetVal = true;

	if ( ( State_Get() == GR_STATE_TEAM_WIN ) && pVictim )
	{
		if ( pVictim->GetTeamNumber() == GetWinningTeam() )
		{
			CBaseTrigger *pTrigger = dynamic_cast<CBaseTrigger *>( info.GetInflictor() );

			// we don't want players on the winning team to be
			// hurt by team-specific trigger_hurt entities during the bonus time
			if ( pTrigger && pTrigger->UsesFilter() )
			{
				bRetVal = false;
			}
		}
	}

	return bRetVal;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::SetTeamGoalString( int iTeam, const char *pszGoal )
{
	if ( iTeam == TF_TEAM_RED )
	{
		if ( !pszGoal || !pszGoal[0] )
		{
			m_pszTeamGoalStringRed.GetForModify()[0] = '\0';
		}
		else
		{
			if ( Q_stricmp( m_pszTeamGoalStringRed.Get(), pszGoal ) )
			{
				Q_strncpy( m_pszTeamGoalStringRed.GetForModify(), pszGoal, MAX_TEAMGOAL_STRING );
			}
		}
	}
	else if ( iTeam == TF_TEAM_BLUE )
	{
		if ( !pszGoal || !pszGoal[0] )
		{
			m_pszTeamGoalStringBlue.GetForModify()[0] = '\0';
		}
		else
		{
			if ( Q_stricmp( m_pszTeamGoalStringBlue.Get(), pszGoal ) )
			{
				Q_strncpy( m_pszTeamGoalStringBlue.GetForModify(), pszGoal, MAX_TEAMGOAL_STRING );
			}
		}
	}
	else if ( iTeam == TF_TEAM_GREEN )
	{
		if ( !pszGoal || !pszGoal[0] )
		{
			m_pszTeamGoalStringGreen.GetForModify()[0] = '\0';
		}
		else
		{
			if ( Q_stricmp( m_pszTeamGoalStringGreen.Get(), pszGoal ) )
			{
				Q_strncpy( m_pszTeamGoalStringGreen.GetForModify(), pszGoal, MAX_TEAMGOAL_STRING );
			}
		}
	}
	else if ( iTeam == TF_TEAM_YELLOW )
	{
		if ( !pszGoal || !pszGoal[0] )
		{
			m_pszTeamGoalStringYellow.GetForModify()[0] = '\0';
		}
		else
		{
			if (Q_stricmp( m_pszTeamGoalStringYellow.Get(), pszGoal ) )
			{
				Q_strncpy( m_pszTeamGoalStringYellow.GetForModify(), pszGoal, MAX_TEAMGOAL_STRING );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::RoundCleanupShouldIgnore( CBaseEntity *pEnt )
{
	if ( FindInList( s_PreserveEnts, pEnt->GetClassname() ) )
		return true;

	//There has got to be a better way of doing this.
	if ( Q_strstr( pEnt->GetClassname(), "tf_weapon_" ) )
		return true;

	return BaseClass::RoundCleanupShouldIgnore( pEnt );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::ShouldCreateEntity( const char *pszClassName )
{
	if ( FindInList( s_PreserveEnts, pszClassName ) )
		return false;

	return BaseClass::ShouldCreateEntity( pszClassName );
}

void CTFGameRules::CleanUpMap( void )
{
	BaseClass::CleanUpMap();

	if ( HLTVDirector() )
	{
		HLTVDirector()->BuildCameraList();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::RecalculateControlPointState( void )
{
	Assert( ObjectiveResource() );

	if ( !g_hControlPointMasters.Count() )
		return;

	if ( g_pObjectiveResource && g_pObjectiveResource->PlayingMiniRounds() )
		return;

	for ( int iTeam = LAST_SHARED_TEAM+1; iTeam < GetNumberOfTeams(); iTeam++ )
	{
		int iFarthestPoint = GetFarthestOwnedControlPoint( iTeam, true );
		if ( iFarthestPoint == -1 )
			continue;

		// Now enable all spawn points for that spawn point
		CBaseEntity *pSpot = gEntList.FindEntityByClassname( NULL, "info_player_teamspawn" );
		while ( pSpot )
		{
			CTFTeamSpawn *pTFSpawn = assert_cast<CTFTeamSpawn *>( pSpot );
			if ( pTFSpawn->GetControlPoint() )
			{
				if ( pTFSpawn->GetTeamNumber() == iTeam )
				{
					if ( pTFSpawn->GetControlPoint()->GetPointIndex() == iFarthestPoint )
					{
						pTFSpawn->SetDisabled( false );
					}
					else
					{
						pTFSpawn->SetDisabled( true );
					}
				}
			}

			pSpot = gEntList.FindEntityByClassname( pSpot, "info_player_teamspawn" );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called when a new round is being initialized
//-----------------------------------------------------------------------------
void CTFGameRules::SetupOnRoundStart( void )
{
	for ( int i = 0; i < MAX_TEAMS; i++ )
	{
		ObjectiveResource()->SetBaseCP( -1, i );
	}

	for ( int i = 0; i < TF_TEAM_COUNT; i++ )
	{
		m_iNumCaps[i] = 0;
	}

	SetIT( NULL );

	m_hRedAttackTrain = NULL;
	m_hBlueAttackTrain = NULL;
	m_hRedDefendTrain = NULL;
	m_hBlueDefendTrain = NULL;

	m_hAmmoEntities.RemoveAll();
	m_hHealthEntities.RemoveAll();

	// Let all entities know that a new round is starting
	CBaseEntity *pEnt = gEntList.FirstEnt();
	while ( pEnt )
	{
		variant_t emptyVariant;
		pEnt->AcceptInput( "RoundSpawn", NULL, NULL, emptyVariant, 0 );

		if ( pEnt->ClassMatches( "func_regenerate" ) || pEnt->ClassMatches( "item_ammopack*" ) )
		{
			EHANDLE hndl( pEnt );
			m_hAmmoEntities.AddToTail( hndl );
		}

		if ( pEnt->ClassMatches( "func_regenerate" ) || pEnt->ClassMatches( "item_healthkit*" ) )
		{
			EHANDLE hndl( pEnt );
			m_hHealthEntities.AddToTail( hndl );
		}

		pEnt = gEntList.NextEnt( pEnt );
	}

	// All entities have been spawned, now activate them
	pEnt = gEntList.FirstEnt();
	while ( pEnt )
	{
		variant_t emptyVariant;
		pEnt->AcceptInput( "RoundActivate", NULL, NULL, emptyVariant, 0 );

		pEnt = gEntList.NextEnt( pEnt );
	}

	IGameEvent *event = gameeventmanager->CreateEvent( "recalculate_holidays" );
	if ( event )
	{
		gameeventmanager->FireEvent( event );
	}

	UTIL_CalculateHolidays();

	if ( g_pObjectiveResource && !g_pObjectiveResource->PlayingMiniRounds() )
	{
		// Find all the control points with associated spawnpoints
		Q_memset( m_bControlSpawnsPerTeam, 0, sizeof( bool ) * MAX_TEAMS * MAX_CONTROL_POINTS );
		CBaseEntity *pSpot = gEntList.FindEntityByClassname( NULL, "info_player_teamspawn" );
		while ( pSpot )
		{
			CTFTeamSpawn *pTFSpawn = assert_cast<CTFTeamSpawn *>( pSpot );
			if ( pTFSpawn->GetControlPoint() )
			{
				m_bControlSpawnsPerTeam[pTFSpawn->GetTeamNumber()][pTFSpawn->GetControlPoint()->GetPointIndex()] = true;
				pTFSpawn->SetDisabled( true );
			}

			pSpot = gEntList.FindEntityByClassname( pSpot, "info_player_teamspawn" );
		}

		RecalculateControlPointState();

		SetRoundOverlayDetails();
	}

	m_szMostRecentCappers[0] = 0;

	m_bossSpawnTimer.Invalidate();

	m_hGhosts.RemoveAll();

	m_mobSpawnTimer.Invalidate();
	m_nZombiesToSpawn = 0;

	if ( g_pMonsterResource )
	{
		g_pMonsterResource->HideBossHealthMeter();
	}

	if ( IsHolidayActive( kHoliday_EOTL ) )
	{
		for ( int i = 0; i < IPhysicsPropAutoList::AutoList().Count(); i++ )
		{
			CPhysicsProp *pPhysicsProp = static_cast<CPhysicsProp *>( IPhysicsPropAutoList::AutoList()[i] );
			const char *pszModel = pPhysicsProp->GetModelName().ToCStr();

			if ( FStrEq( pszModel, "models/props_trainyard/bomb_cart.mdl" ) )
			{
				pPhysicsProp->SetModel( "models/props_trainyard/bomb_eotl_blue.mdl" );
			}
			else if ( FStrEq( pszModel, "models/props_trainyard/bomb_cart_red.mdl" ) )
			{
				pPhysicsProp->SetModel( "models/props_trainyard/bomb_eotl_red.mdl" );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called when a new round is off and running
//-----------------------------------------------------------------------------
void CTFGameRules::SetupOnRoundRunning( void )
{
	// Let out control point masters know that the round has started
	for ( int i = 0; i < g_hControlPointMasters.Count(); i++ )
	{
		variant_t emptyVariant;
		if ( g_hControlPointMasters[i] )
		{
			g_hControlPointMasters[i]->AcceptInput( "RoundStart", NULL, NULL, emptyVariant, 0 );
		}
	}

	// Reset player speeds after preround lock
	CTFPlayer *pPlayer;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

		if ( !pPlayer )
			continue;

		pPlayer->TeamFortress_SetSpeed();
		pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_ROUND_START );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called before a new round is started (so the previous round can end)
//-----------------------------------------------------------------------------
void CTFGameRules::PreviousRoundEnd( void )
{
	// before we enter a new round, fire the "end output" for the previous round
	if ( g_hControlPointMasters.Count() && g_hControlPointMasters[0] )
	{
		g_hControlPointMasters[0]->FireRoundEndOutput();
	}

	m_iPreviousRoundWinners = GetWinningTeam();
}

//-----------------------------------------------------------------------------
// Purpose: Called when a round has entered stalemate mode (timer has run out)
//-----------------------------------------------------------------------------
void CTFGameRules::SetupOnStalemateStart( void )
{
	// Remove everyone's objects
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( pPlayer )
		{
			pPlayer->RemoveAllOwnedEntitiesFromWorld();
		}
	}

	if ( IsInArenaMode() )
	{
		if ( m_hArenaLogic.IsValid() )
		{
			m_hArenaLogic->m_OnArenaRoundStart.FireOutput( m_hArenaLogic.Get(), m_hArenaLogic.Get() );

			IGameEvent *event = gameeventmanager->CreateEvent( "arena_round_start" );
			if ( event )
			{
				gameeventmanager->FireEvent( event );
			}

			if ( tf_arena_override_cap_enable_time.GetFloat() > 0 )
				m_flCapturePointEnableTime = gpGlobals->curtime + tf_arena_override_cap_enable_time.GetFloat();
			else
				m_flCapturePointEnableTime = gpGlobals->curtime + m_hArenaLogic->m_flTimeToEnableCapPoint;

			BroadcastSound( 255, "Announcer.AM_RoundStartRandom" );
			BroadcastSound( 255, "Ambient.Siren" );
		}
	}
	else
	{
		// Respawn all the players
		RespawnPlayers( true );

		// Disable all the active health packs in the world
		m_hDisabledHealthKits.Purge();
		CHealthKit *pHealthPack = gEntList.NextEntByClass( (CHealthKit *)NULL );
		while ( pHealthPack )
		{
			if ( !pHealthPack->IsDisabled() )
			{
				pHealthPack->SetDisabled( true );
				m_hDisabledHealthKits.AddToTail( pHealthPack );
			}
			pHealthPack = gEntList.NextEntByClass( pHealthPack );
		}
	}

	CTFPlayer *pPlayer;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

		if ( !pPlayer )
			continue;

		if ( IsInArenaMode() )
		{
			pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_ROUND_START );
			pPlayer->TeamFortress_SetSpeed();
		}
		else
		{
			pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_SUDDENDEATH_START );
		}
	}

	m_flStalemateStartTime = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::SetupOnStalemateEnd( void )
{
	// Reenable all the health packs we disabled
	for ( int i = 0; i < m_hDisabledHealthKits.Count(); i++ )
	{
		if ( m_hDisabledHealthKits[i] )
		{
			m_hDisabledHealthKits[i]->SetDisabled( false );
		}
	}

	m_hDisabledHealthKits.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::InitTeams( void )
{
	BaseClass::InitTeams();

	// clear the player class data
	ResetFilePlayerClassInfoDatabase();
}

// Skips players except for the specified one.
class CTraceFilterHitPlayer : public CTraceFilterSimple
{
public:
	DECLARE_CLASS( CTraceFilterIgnorePlayers, CTraceFilterSimple );

	CTraceFilterHitPlayer( const IHandleEntity *passentity, IHandleEntity *pHitEntity, int collisionGroup )
		: CTraceFilterSimple( passentity, collisionGroup )
	{
		m_pHitEntity = pHitEntity;
	}

	virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
	{
		CBaseEntity *pEntity = EntityFromEntityHandle( pServerEntity );

		if ( !pEntity )
			return false;

		if ( pEntity->IsPlayer() && pEntity != m_pHitEntity )
			return false;

		return BaseClass::ShouldHitEntity( pServerEntity, contentsMask );
	}

private:
	const IHandleEntity *m_pHitEntity;
};

ConVar tf_fixedup_damage_radius( "tf_fixedup_damage_radius", "1", FCVAR_DEVELOPMENTONLY );

bool CTFRadiusDamageInfo::ApplyToEntity( CBaseEntity *pEntity )
{
	const int MASK_RADIUS_DAMAGE = MASK_SHOT&( ~CONTENTS_HITBOX );
	trace_t		tr;
	float		falloff;
	Vector		vecSpot;

	if ( info->GetDamageType() & DMG_RADIUS_MAX )
		falloff = 0.0;
	else if ( info->GetDamageType() & DMG_HALF_FALLOFF )
		falloff = 0.5;
	else if ( m_flRadius )
		falloff = info->GetDamage() / m_flRadius;
	else
		falloff = 1.0;

	float flFalloffMult = 1.0f;
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( info->GetWeapon(), flFalloffMult, mult_dmg_falloff );
	if ( flFalloffMult != 1.0f )
		falloff += flFalloffMult;

	CBaseEntity *pInflictor = info->GetInflictor();

	//	float flHalfRadiusSqr = Square( flRadius / 2.0f );

	// This value is used to scale damage when the explosion is blocked by some other object.
	float flBlockedDamagePercent = 0.0f;

	// Check that the explosion can 'see' this entity, trace through players.
	vecSpot = pEntity->BodyTarget( m_vecSrc, false );
	CTraceFilterIgnorePlayers filterPlayers( pInflictor, COLLISION_GROUP_PROJECTILE );
	CTraceFilterIgnoreFriendlyCombatItems filterItems( pInflictor, COLLISION_GROUP_PROJECTILE, pInflictor->GetTeamNumber() );
	CTraceFilterChain filter( &filterPlayers, &filterItems );
	UTIL_TraceLine( m_vecSrc, vecSpot, MASK_RADIUS_DAMAGE, &filter, &tr );

	if ( tr.startsolid && tr.m_pEnt )
	{
		if ( tr.m_pEnt->IsCombatItem() )
		{
			if ( pEntity->InSameTeam( tr.m_pEnt ) && pEntity != tr.m_pEnt )
				return false;
		}

		filterPlayers.SetPassEntity( tr.m_pEnt );
		UTIL_TraceLine( m_vecSrc, vecSpot, MASK_RADIUS_DAMAGE, &filter, &tr );
	}

	if ( tr.fraction != 1.0f && tr.m_pEnt != pEntity )
		return false;

	// Adjust the damage - apply falloff.
	float flAdjustedDamage = 0.0f;
	float flDistanceToEntity;

	// Rockets store the ent they hit as the enemy and have already
	// dealt full damage to them by this time
	if ( pInflictor && ( pEntity == pInflictor->GetEnemy() ) )
	{
		// Full damage, we hit this entity directly
		flDistanceToEntity = 0;
	}
	else if ( pEntity->IsPlayer() )
	{
		// Use whichever is closer, absorigin or worldspacecenter
		float flToWorldSpaceCenter = ( m_vecSrc - pEntity->WorldSpaceCenter() ).Length();
		float flToOrigin = ( m_vecSrc - pEntity->GetAbsOrigin() ).Length();

		flDistanceToEntity = Min( flToWorldSpaceCenter, flToOrigin );
	}
	else
	{
		flDistanceToEntity = ( m_vecSrc - tr.endpos ).Length();
	}

	if ( tf_fixedup_damage_radius.GetBool() )
	{
		flAdjustedDamage = RemapValClamped( flDistanceToEntity, 0, m_flRadius, info->GetDamage(), info->GetDamage() * falloff );
	}
	else
	{
		flAdjustedDamage = flDistanceToEntity * falloff;
		flAdjustedDamage = info->GetDamage() - flAdjustedDamage;
	}

	CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase *>( info->GetWeapon() );

	// Grenades & Pipebombs do less damage to ourselves.
	if ( pEntity == info->GetAttacker() && pWeapon )
	{
		switch( pWeapon->GetWeaponID() )
		{
			case TF_WEAPON_PIPEBOMBLAUNCHER :
			case TF_WEAPON_GRENADELAUNCHER :
			case TF_WEAPON_CANNON :
			case TF_WEAPON_STICKBOMB :
				flAdjustedDamage *= 0.75f;
				break;
		}
	}

	if ( flAdjustedDamage <= 0 )
		return false;

	// the explosion can 'see' this entity, so hurt them!
	if ( tr.startsolid )
	{
		// if we're stuck inside them, fixup the position and distance
		tr.endpos = m_vecSrc;
		tr.fraction = 0.0;
	}

	CTakeDamageInfo adjustedInfo = *info;
	//Msg("%s: Blocked damage: %f percent (in:%f  out:%f)\n", pEntity->GetClassname(), flBlockedDamagePercent * 100, flAdjustedDamage, flAdjustedDamage - (flAdjustedDamage * flBlockedDamagePercent) );
	adjustedInfo.SetDamage( flAdjustedDamage - ( flAdjustedDamage * flBlockedDamagePercent ) );

	// Now make a consideration for skill level!
	if ( info->GetAttacker() && info->GetAttacker()->IsPlayer() && pEntity->IsNPC() )
	{
		// An explosion set off by the player is harming an NPC. Adjust damage accordingly.
		adjustedInfo.AdjustPlayerDamageInflictedForSkillLevel();
	}

	Vector dir = vecSpot - m_vecSrc;
	VectorNormalize( dir );

	// If we don't have a damage force, manufacture one
	if ( adjustedInfo.GetDamagePosition() == vec3_origin || adjustedInfo.GetDamageForce() == vec3_origin )
	{
		CalculateExplosiveDamageForce( &adjustedInfo, dir, m_vecSrc );
	}
	else
	{
		// Assume the force passed in is the maximum force. Decay it based on falloff.
		float flForce = adjustedInfo.GetDamageForce().Length() * falloff;
		adjustedInfo.SetDamageForce( dir * flForce );
		adjustedInfo.SetDamagePosition( m_vecSrc );
	}

	adjustedInfo.ScaleDamageForce( m_flPushbackScale );

	if ( tr.fraction != 1.0 && pEntity == tr.m_pEnt )
	{
		ClearMultiDamage();
		pEntity->DispatchTraceAttack( adjustedInfo, dir, &tr );
		ApplyMultiDamage();
	}
	else
	{
		pEntity->TakeDamage( adjustedInfo );
	}

	// Now hit all triggers along the way that respond to damage... 
	pEntity->TraceAttackToTriggers( adjustedInfo, m_vecSrc, tr.endpos, dir );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Logic for jar-based throwable object collisions
//-----------------------------------------------------------------------------
bool CTFGameRules::RadiusJarEffect( CTFRadiusDamageInfo &radiusInfo, int iCond )
{
	bool bExtinguished = false;
	CBaseEntity *pAttacker = radiusInfo.info->GetAttacker();

	CBaseEntity *pEntity = NULL;
	for ( CEntitySphereQuery sphere( radiusInfo.m_vecSrc, radiusInfo.m_flRadius ); ( pEntity = sphere.GetCurrentEntity() ) != NULL; sphere.NextEntity() )
	{
		if ( pEntity == radiusInfo.m_pEntityIgnore )
			continue;

		if ( pEntity->m_takedamage == DAMAGE_NO )
			continue;

		// UNDONE: this should check a damage mask, not an ignore
		if ( radiusInfo.m_iClassIgnore != CLASS_NONE && pEntity->Classify() == radiusInfo.m_iClassIgnore )
		{
			continue;
		}

		// Checking distance from source because Valve were apparently too lazy to fix the engine function.
		Vector vecHitPoint;
		pEntity->CollisionProp()->CalcNearestPoint( radiusInfo.m_vecSrc, &vecHitPoint );
		Vector vecDir = vecHitPoint - radiusInfo.m_vecSrc;

		if ( vecDir.LengthSqr() > ( radiusInfo.m_flRadius * radiusInfo.m_flRadius ) )
			continue;

		CTFPlayer *pTFPlayer = ToTFPlayer( pEntity );
		if ( pTFPlayer )
		{
			if ( !pTFPlayer->InSameTeam( pAttacker ) )
			{
				pTFPlayer->m_Shared.AddCond( iCond, 10.0f );
				switch ( iCond )
				{
					case TF_COND_URINE:
						pTFPlayer->m_Shared.m_hUrineAttacker.Set( pAttacker );
						break;
					case TF_COND_MAD_MILK:
						pTFPlayer->m_Shared.m_hMilkAttacker.Set( pAttacker );
						break;
					case TF_COND_GAS:
						pTFPlayer->m_Shared.m_hGasAttacker.Set( pAttacker );
						break;
					default:
						break;
				}
			}
			else
			{
				if ( pTFPlayer->m_Shared.InCond( TF_COND_BURNING ) )
				{
					pTFPlayer->m_Shared.RemoveCond( TF_COND_BURNING );
					if (pTFPlayer->m_Shared.InCond(TF_COND_BURNING_PYRO))
						pTFPlayer->m_Shared.RemoveCond(TF_COND_BURNING_PYRO);
					
					pTFPlayer->EmitSound( "TFPlayer.FlameOut" );

					if ( pEntity != pAttacker )
					{
						bExtinguished = true;

						CTFPlayer *pTFAttacker = ToTFPlayer( pAttacker );
						if ( pTFAttacker )
						{
							// Bonus points.
							IGameEvent *event_bonus = gameeventmanager->CreateEvent( "player_bonuspoints" );
							if ( event_bonus )
							{
								event_bonus->SetInt( "player_entindex", pEntity->entindex() );
								event_bonus->SetInt( "source_entindex", pAttacker->entindex() );
								event_bonus->SetInt( "points", 1 );

								gameeventmanager->FireEvent( event_bonus );
							}
							CTF_GameStats.Event_PlayerAwardBonusPoints( pTFAttacker, pEntity, 1 );
						}
					}
				}
			}
		}
	}

	return bExtinguished;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::RadiusDamage( CTFRadiusDamageInfo &radiusInfo )
{
	CTakeDamageInfo *info = (CTakeDamageInfo *)radiusInfo.info;
	CBaseEntity *pAttacker = info->GetAttacker();
	int iPlayersDamaged = 0;

	CBaseEntity *pEntity = NULL;
	for ( CEntitySphereQuery sphere( radiusInfo.m_vecSrc, radiusInfo.m_flRadius ); ( pEntity = sphere.GetCurrentEntity() ) != NULL; sphere.NextEntity() )
	{
		if ( pEntity == radiusInfo.m_pEntityIgnore )
			continue;

		if ( pEntity->m_takedamage == DAMAGE_NO )
			continue;

		// UNDONE: this should check a damage mask, not an ignore
		if ( radiusInfo.m_iClassIgnore != CLASS_NONE && pEntity->Classify() == radiusInfo.m_iClassIgnore )
		{
			continue;
		}

		// Skip the attacker as we'll handle him separately.
		if ( pEntity == pAttacker )
			continue;

		// Checking distance from source because Valve were apparently too lazy to fix the engine function.
		Vector vecHitPoint;
		pEntity->CollisionProp()->CalcNearestPoint( radiusInfo.m_vecSrc, &vecHitPoint );
		Vector vecDir = vecHitPoint - radiusInfo.m_vecSrc;

		if ( vecDir.LengthSqr() > Square(radiusInfo.m_flRadius) )
			continue;

		if ( radiusInfo.ApplyToEntity( pEntity ) )
		{
			if ( pEntity->IsPlayer() && !pEntity->InSameTeam( pAttacker ) )
			{
				iPlayersDamaged++;
			}
		}
	}

	info->SetDamagedOtherPlayers( iPlayersDamaged );

	// For attacker, radius and damage need to be consistent so custom weapons don't screw up rocket jumping.
	if ( radiusInfo.m_flSelfDamageRadius != 0.0f )
	{
		if ( pAttacker )
		{
			// Get stock damage.
			CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase *>( info->GetWeapon() );
			if ( pWeapon )
			{
				info->SetDamage( (float)pWeapon->GetTFWpnData().GetWeaponData( TF_WEAPON_PRIMARY_MODE ).m_nDamage );
				info->CopyDamageToBaseDamage();
			}

			// Use stock radius.
			radiusInfo.m_flRadius = radiusInfo.m_flSelfDamageRadius;

			Vector vecHitPoint;
			pAttacker->CollisionProp()->CalcNearestPoint( radiusInfo.m_vecSrc, &vecHitPoint );
			Vector vecDir = vecHitPoint - radiusInfo.m_vecSrc;

			if ( vecDir.LengthSqr() <= Square(radiusInfo.m_flRadius) )
			{
				radiusInfo.ApplyToEntity( pAttacker );
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
//			&vecSrcIn - 
//			flRadius - 
//			iClassIgnore - 
//			*pEntityIgnore - 
//-----------------------------------------------------------------------------
void CTFGameRules::RadiusDamage( const CTakeDamageInfo &info, const Vector &vecSrcIn, float flRadius, int iClassIgnore, CBaseEntity *pEntityIgnore )
{
	Assert( false );

	CTFRadiusDamageInfo radiusInfo( &info, vecSrcIn, flRadius, pEntityIgnore );
	RadiusDamage( radiusInfo );
}

// --------------------------------------------------------------------------------------------------- //
// Voice helper
// --------------------------------------------------------------------------------------------------- //

class CVoiceGameMgrHelper : public IVoiceGameMgrHelper
{
public:
	virtual bool		CanPlayerHearPlayer( CBasePlayer *pListener, CBasePlayer *pTalker, bool &bProximity )
	{
		// Dead players can only be heard by other dead team mates but only if a match is in progress
		if ( !tf_gravetalk.GetBool() )
		{
			if ( TFGameRules()->State_Get() != GR_STATE_TEAM_WIN && TFGameRules()->State_Get() != GR_STATE_GAME_OVER )
			{
				if ( pTalker->IsAlive() == false )
				{
					if ( pListener->IsAlive() == false || tf_teamtalk.GetBool() )
						return ( pListener->InSameTeam( pTalker ) );

					return false;
				}
			}
		}

		return ( pListener->InSameTeam( pTalker ) );
	}
};
CVoiceGameMgrHelper g_VoiceGameMgrHelper;
IVoiceGameMgrHelper *g_pVoiceGameMgrHelper = &g_VoiceGameMgrHelper;

// Load the objects.txt file.
class CObjectsFileLoad : public CAutoGameSystem
{
public:
	virtual bool Init()
	{
		LoadObjectInfos( filesystem );
		return true;
	}
} g_ObjectsFileLoad;

// --------------------------------------------------------------------------------------------------- //
// Globals.
// --------------------------------------------------------------------------------------------------- //
/*
	// NOTE: the indices here must match TEAM_UNASSIGNED, TEAM_SPECTATOR, TF_TEAM_RED, TF_TEAM_BLUE, etc.
	char *sTeamNames[] =
	{
		"Unassigned",
		"Spectator",
		"Red",
		"Blue"
	};
*/
// --------------------------------------------------------------------------------------------------- //
// Global helper functions.
// --------------------------------------------------------------------------------------------------- //

// World.cpp calls this but we don't use it in TF.
void InitBodyQue()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFGameRules::~CTFGameRules()
{
	// Note, don't delete each team since they are in the gEntList and will 
	// automatically be deleted from there, instead.
	TFTeamMgr()->Shutdown();
	ShutdownCustomResponseRulesDicts();
}

//-----------------------------------------------------------------------------
// Purpose: TF2 Specific Client Commands
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CTFGameRules::ClientCommand( CBaseEntity *pEdict, const CCommand &args )
{
	CTFPlayer *pPlayer = ToTFPlayer( pEdict );

	const char *pcmd = args[0];
	if ( FStrEq( pcmd, "objcmd" ) )
	{
		if ( args.ArgC() < 3 )
			return true;

		int entindex = atoi( args[1] );
		edict_t *pEdict = INDEXENT( entindex );
		if ( pEdict )
		{
			CBaseEntity *pBaseEntity = GetContainingEntity( pEdict );
			CBaseObject *pObject = dynamic_cast<CBaseObject *>( pBaseEntity );

			if ( pObject )
			{
				// We have to be relatively close to the object too...

				// BUG! Some commands need to get sent without the player being near the object, 
				// eg delayed dismantle commands. Come up with a better way to ensure players aren't
				// entering these commands in the console.

				//float flDistSq = pObject->GetAbsOrigin().DistToSqr( pPlayer->GetAbsOrigin() );
				//if (flDistSq <= (MAX_OBJECT_SCREEN_INPUT_DISTANCE * MAX_OBJECT_SCREEN_INPUT_DISTANCE))
				{
					// Strip off the 1st two arguments and make a new argument string
					CCommand objectArgs( args.ArgC() - 2, &args.ArgV()[2] );
					pObject->ClientCommand( pPlayer, objectArgs );
				}
			}
		}

		return true;
	}

	// Handle some player commands here as they relate more directly to gamerules state
	if ( FStrEq( pcmd, "nextmap" ) )
	{
		if ( pPlayer->m_flNextTimeCheck < gpGlobals->curtime )
		{
			char szNextMap[32];

			if ( nextlevel.GetString() && *nextlevel.GetString() && engine->IsMapValid( nextlevel.GetString() ) )
			{
				Q_strncpy( szNextMap, nextlevel.GetString(), sizeof( szNextMap ) );
			}
			else
			{
				GetNextLevelName( szNextMap, sizeof( szNextMap ) );
			}

			ClientPrint( pPlayer, HUD_PRINTTALK, "#TF_nextmap", szNextMap );

			pPlayer->m_flNextTimeCheck = gpGlobals->curtime + 1;
		}

		return true;
	}
	else if ( FStrEq( pcmd, "timeleft" ) )
	{
		if ( pPlayer->m_flNextTimeCheck < gpGlobals->curtime )
		{
			if ( mp_timelimit.GetInt() > 0 )
			{
				int iTimeLeft = GetTimeLeft();

				char szMinutes[5];
				char szSeconds[3];

				if ( iTimeLeft <= 0 )
				{
					Q_snprintf( szMinutes, sizeof( szMinutes ), "0" );
					Q_snprintf( szSeconds, sizeof( szSeconds ), "00" );
				}
				else
				{
					Q_snprintf( szMinutes, sizeof( szMinutes ), "%d", iTimeLeft / 60 );
					Q_snprintf( szSeconds, sizeof( szSeconds ), "%02d", iTimeLeft % 60 );
				}

				ClientPrint( pPlayer, HUD_PRINTTALK, "#TF_timeleft", szMinutes, szSeconds );
			}
			else
			{
				ClientPrint( pPlayer, HUD_PRINTTALK, "#TF_timeleft_nolimit" );
			}

			pPlayer->m_flNextTimeCheck = gpGlobals->curtime + 1;
		}
		return true;
	}
	else if ( FStrEq( pcmd, "freezecam_taunt" ) )
	{
		// let's check this came from the client .dll and not the console
	//	int iCmdPlayerID = pPlayer->GetUserID();
	//	unsigned short mask = UTIL_GetAchievementEventMask();

	//	int iAchieverIndex = atoi( args[1] ) ^ mask;
	//	int code = ( iCmdPlayerID ^ iAchieverIndex ) ^ mask;
	//	if ( code == atoi( args[2] ) )
	//	{
	//		CTFPlayer *pAchiever = ToTFPlayer( UTIL_PlayerByIndex( iAchieverIndex ) );
	//		if ( pAchiever && ( pAchiever->GetUserID() != iCmdPlayerID ) )
	//		{
	//			int iClass = pAchiever->GetPlayerClass()->GetClassIndex();
	//			pAchiever->AwardAchievement( g_TauntCamAchievements[ iClass ] );
	//		}
	//	}

		return true;
	}
	else if ( pPlayer->ClientCommand( args ) )
	{
		return true;
	}

	return BaseClass::ClientCommand( pEdict, args );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::ClientCommandKeyValues( edict_t *pEntity, KeyValues *pKeyValues )
{
	CTFPlayer *pTFPlayer = ToTFPlayer( CBaseEntity::Instance( pEntity ) );

	if ( !pTFPlayer )
		return;

	char const *pcmd = pKeyValues->GetName();
	if ( FStrEq( pcmd, "+inspect_server" ) )
	{
		pTFPlayer->InspectButtonPressed();
	}
	else if ( FStrEq( pcmd, "-inspect_server" ) )
	{
		pTFPlayer->InspectButtonReleased();
	}
	else if ( FStrEq( pcmd, "MVM_Upgrade" ) )
	{
		if ( IsMannVsMachineMode() )
		{
		}
	}
	else if ( FStrEq( pcmd, "MvM_UpgradesBegin" ) )
	{
		//pTFPlayer->BeginPurchasableUpgrades();
	}
	else if ( FStrEq( pcmd, "MvM_UpgradesDone" ) )
	{
		//pTFPlayer->EndPurchasableUpgrades();

		if ( IsMannVsMachineMode() && pKeyValues->GetInt( "num_upgrades", 0 ) > 0 )
		{
			pTFPlayer->SpeakConceptIfAllowed( MP_CONCEPT_MVM_UPGRADE_COMPLETE );
		}
	}
	else if ( FStrEq( pcmd, "MVM_Revive_Response" ) )
	{
		
	}
	else if ( FStrEq( pcmd, "MVM_Respec" ) )
	{

	}
}

// Add the ability to ignore the world trace
void CTFGameRules::Think()
{
	if ( !g_fGameOver )
	{
		if ( gpGlobals->curtime > m_flNextPeriodicThink )
		{
			if ( State_Get() != GR_STATE_TEAM_WIN )
			{
				if ( CheckCapsPerRound() )
					return;
			}
		}
	}

	if ( IsInArenaMode() && m_flArenaNotificationSend > 0.0 && gpGlobals->curtime >= m_flArenaNotificationSend )
	{
		Arena_SendPlayerNotifications();
	}

	SpawnHalloweenBoss();

	BaseClass::Think();
}

//Runs think for all player's conditions
//Need to do this here instead of the player so players that crash still run their important thinks
void CTFGameRules::RunPlayerConditionThink( void )
{
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

		if ( pPlayer )
		{
			pPlayer->m_Shared.ConditionGameRulesThink();
		}
	}
}

void CTFGameRules::FrameUpdatePostEntityThink()
{
	BaseClass::FrameUpdatePostEntityThink();

	RunPlayerConditionThink();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::BeginHaunting( int nDesiredCount, float flMinLifetime, float flMaxLifetime )
{
	if ( !IsHolidayActive( kHoliday_Halloween ) )
		return;

	if ( !IsHalloweenScenario( HALLOWEEN_SCENARIO_VIADUCT ) && !IsHalloweenScenario( HALLOWEEN_SCENARIO_LAKESIDE ) )
	{
		CTFHolidayEntity *pHolidayEntity = dynamic_cast<CTFHolidayEntity *>( gEntList.FindEntityByClassname( NULL, "tf_logic_holiday" ) );
		if ( !pHolidayEntity || !pHolidayEntity->ShouldAllowHaunting() )
			return;
	}

	m_hGhosts.RemoveAll();

	// Just update existing ghosts
	FOR_EACH_VEC( IGhostAutoList::AutoList(), i )
	{
		CGhost *pGhost = (CGhost *)IGhostAutoList::AutoList()[i];
		pGhost->SetLifetime( RandomFloat( flMinLifetime, flMaxLifetime ) );
		m_hGhosts.AddToTail( pGhost );
	}

	// If there was already too many existing
	if ( m_hGhosts.Count() >= nDesiredCount )
		return;

	CUtlVector<CTFPlayer *> players;
	CollectPlayers( &players, TF_TEAM_RED, true );
	CollectPlayers( &players, TF_TEAM_BLUE, true, true );

	auto IsPlayerNearby = [&]( Vector const &vecSpot ) -> bool {
		FOR_EACH_VEC( players, i )
		{
			if ( vecSpot.DistTo( players[i]->GetAbsOrigin() ) < 240.0f )
				return true;
		}

		return false;
	};

	CUtlVector<Vector> spawnPoints;
	FOR_EACH_VEC( TheNavAreas, i )
	{
		CTFNavArea *area = assert_cast<CTFNavArea *>( TheNavAreas[i] );
		if ( area->HasTFAttributes( RED_SPAWN_ROOM|BLUE_SPAWN_ROOM ) )
		{
			// keep out of spawn rooms
			continue;
		}

		const Vector vecSpot = area->GetRandomPoint();
		if ( IsPlayerNearby( vecSpot ) )
			continue;

		spawnPoints.AddToTail( vecSpot );
	}

	if ( spawnPoints.IsEmpty() )
		return;

	const int nTotalGhosts = nDesiredCount - m_hGhosts.Count();
	for ( int i=0; i < nTotalGhosts; ++i )
	{
		const int nSpawnPoint = RandomInt( 0, spawnPoints.Count()-1 );
		const float flLifetime = RandomFloat( flMinLifetime, flMaxLifetime );

		CGhost *pGhost = CGhost::Create( spawnPoints[nSpawnPoint], vec3_angle, flLifetime );
		m_hGhosts.AddToTail( pGhost );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::SpawnHalloweenBoss( void )
{
	VPROF_BUDGET( __FUNCTION__, "NextBotSpiky" );

	if ( !IsHolidayActive( kHoliday_Halloween ) )
		return SpawnZombieMob();

	float fSpawnInterval;
	float fSpawnVariation;
	CHalloweenBaseBoss::HalloweenBossType eBossType;

	if ( IsHalloweenScenario( HALLOWEEN_SCENARIO_MANOR ) )
	{
		eBossType = CHalloweenBaseBoss::HEADLESS_HATMAN;
		fSpawnInterval = tf_halloween_boss_spawn_interval.GetFloat();
		fSpawnVariation = tf_halloween_boss_spawn_interval_variation.GetFloat();
	}
	else if ( IsHalloweenScenario( HALLOWEEN_SCENARIO_VIADUCT ) )
	{
		eBossType = CHalloweenBaseBoss::EYEBALL_BOSS;
		fSpawnInterval = tf_halloween_eyeball_boss_spawn_interval.GetFloat();
		fSpawnVariation = tf_halloween_eyeball_boss_spawn_interval_variation.GetFloat();
	}
	else if ( IsHalloweenScenario( HALLOWEEN_SCENARIO_LAKESIDE ) )
	{
		CWheelOfDoom *pWheelOfDoom = (CWheelOfDoom *)gEntList.FindEntityByClassname( NULL, "wheel_of_doom" );
		if ( pWheelOfDoom && !pWheelOfDoom->IsDoneBroadcastingEffectSound() )
			return;

		if (CMerasmus::m_level > 3)
		{
			fSpawnInterval = 60.0f;
			fSpawnVariation = 0.0f;
		}
		else
		{
			fSpawnInterval = tf_merasmus_spawn_interval.GetFloat();
			fSpawnVariation = tf_merasmus_spawn_interval_variation.GetFloat();
		}
		eBossType = CHalloweenBaseBoss::MERASMUS;
	}
	else
	{
		SpawnZombieMob();
		return;
	}

	if ( !m_hBosses.IsEmpty() )
	{
		if ( m_hBosses[0] )
		{
			isBossForceSpawning = false;
			StartBossTimer( RandomFloat( fSpawnInterval - fSpawnVariation, fSpawnInterval + fSpawnVariation ) );
			return;
		}
	}

	if ( !m_bossSpawnTimer.HasStarted() && !isBossForceSpawning )
	{
		if ( IsHalloweenScenario( HALLOWEEN_SCENARIO_LAKESIDE ) )
			StartBossTimer( RandomFloat( fSpawnInterval - fSpawnVariation, fSpawnInterval + fSpawnVariation ) );
		else
			StartBossTimer( RandomFloat( 0, fSpawnInterval + fSpawnVariation ) * 0.5 );

		return;
	}

	if ( m_bossSpawnTimer.IsElapsed() || isBossForceSpawning )
	{
		if ( !isBossForceSpawning )
		{
			if ( InSetup() || IsInWaitingForPlayers() )
				return;

			CUtlVector<CTFPlayer *> players;
			CollectPlayers( &players, TF_TEAM_RED, true );
			CollectPlayers( &players, TF_TEAM_BLUE, true, true );

			int nNumHumans = 0;
			for ( int i=0; i<players.Count(); ++i )
			{
				if ( !players[i]->IsBot() )
					nNumHumans++;
			}

			if ( tf_halloween_bot_min_player_count.GetInt() > nNumHumans )
				return;
		}

		Vector vecSpawnLoc = vec3_origin;
		if ( !g_hControlPointMasters.IsEmpty() && g_hControlPointMasters[0] )
		{
			CTeamControlPointMaster *pMaster = g_hControlPointMasters[0];
			for ( int i=0; i<pMaster->GetNumPoints(); ++i )
			{
				CTeamControlPoint *pPoint = pMaster->GetControlPoint( i );
				if ( !pMaster->IsInRound( pPoint ) )
					continue;

				if ( ObjectiveResource()->GetOwningTeam( pPoint->GetPointIndex() ) == TF_TEAM_BLUE )
					continue;

				if ( !TFGameRules()->TeamMayCapturePoint( TF_TEAM_BLUE, pPoint->GetPointIndex() ) )
					continue;

				vecSpawnLoc = pPoint->GetAbsOrigin();
				if ( eBossType > CHalloweenBaseBoss::HEADLESS_HATMAN )
				{
					pPoint->ForceOwner( TEAM_UNASSIGNED );

					if ( TFGameRules()->IsInKothMode() )
					{
						CTeamRoundTimer *pRedTimer = TFGameRules()->GetRedKothRoundTimer();
						CTeamRoundTimer *pBluTimer = TFGameRules()->GetBlueKothRoundTimer();

						if ( pRedTimer )
						{
							variant_t emptyVar;
							pRedTimer->AcceptInput( "Pause", NULL, NULL, emptyVar, 0 );
						}
						if ( pBluTimer )
						{
							variant_t emptyVar;
							pBluTimer->AcceptInput( "Pause", NULL, NULL, emptyVar, 0 );
						}
					}
				}
			}

			CBaseEntity *pSpawnPoint = gEntList.FindEntityByClassname( NULL, "spawn_boss" );
			if ( pSpawnPoint )
			{
				vecSpawnLoc = pSpawnPoint->GetAbsOrigin();
			}
		}
		else
		{
			CBaseEntity *pSpawnPoint = gEntList.FindEntityByClassname( NULL, "spawn_boss" );
			if ( pSpawnPoint )
			{
				vecSpawnLoc = pSpawnPoint->GetAbsOrigin();
			}
			else
			{
				CUtlVector<CNavArea *> candidates;
				for ( int i=0; i<TheNavAreas.Count(); ++i )
				{
					CNavArea *area = TheNavAreas[i];
					if ( area->GetSizeX() >= 100.0f && area->GetSizeY() >= 100.0f )
						candidates.AddToTail( area );
				}

				if ( !candidates.IsEmpty() )
				{
					CNavArea *area = candidates.Random();
					vecSpawnLoc = area->GetCenter();
				}
			}
		}

		if( !vecSpawnLoc.IsZero() )
		{
			CHalloweenBaseBoss::SpawnBossAtPos( eBossType, vecSpawnLoc );
			isBossForceSpawning = false;
			StartBossTimer( RandomFloat( fSpawnInterval - fSpawnVariation, fSpawnInterval + fSpawnVariation ) );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::SpawnZombieMob( void )
{
	VPROF_BUDGET( __FUNCTION__, "NextBotSpiky" );

	// can we even spawn some?
	if ( !tf_halloween_zombie_mob_enabled.GetBool() )
		return;

	// do nothing when the game hasn't started yet
	if ( InSetup() || IsInWaitingForPlayers() )
	{
		m_mobSpawnTimer.Start( tf_halloween_zombie_mob_spawn_interval.GetFloat() );
		return;
	}

	if ( isZombieMobForceSpawning )
	{
		isZombieMobForceSpawning = false;
		m_mobSpawnTimer.Invalidate();
	}

	if ( m_nZombiesToSpawn > 0 && IsSpaceToSpawnHere( m_vecMobSpawnLocation ) )
	{
		if ( CZombie::SpawnAtPos( m_vecMobSpawnLocation, 0 ) )
			--m_nZombiesToSpawn;
	}

	CUtlVector<CTFPlayer *> players;
	CollectPlayers( &players, TF_TEAM_RED, true );
	CollectPlayers( &players, TF_TEAM_BLUE, true, true );

	int nHumans = 0;
	FOR_EACH_VEC( players, i )
	{
		if ( !players[i]->IsBot() )
			++nHumans;
	}

	if ( nHumans <= 0 || !m_mobSpawnTimer.IsElapsed() )
		return;

	m_mobSpawnTimer.Start( tf_halloween_zombie_mob_spawn_interval.GetFloat() );

	CUtlVector<CTFNavArea *> validAreas;
	const float flSearchRange = 2000.0f;

	// populate a vector of valid spawn locations
	FOR_EACH_VEC( players, i )
	{
		CUtlVector<CTFNavArea *> nearby;
		// ignore bots
		if ( players[i]->IsBot() )
			continue;
		// are they on mesh?
		if ( players[i]->GetLastKnownArea() == nullptr )
			continue;

		CollectSurroundingAreas( &nearby, players[i]->GetLastKnownArea(), flSearchRange );
		FOR_EACH_VEC( nearby, j )
		{
			if ( !nearby[j]->IsValidForWanderingPopulation() )
				continue;

			if ( nearby[j]->IsBlocked( TF_TEAM_RED ) || nearby[j]->IsBlocked( TF_TEAM_BLUE ) )
				continue;

			validAreas.AddToTail( nearby[j] );
		}
	}

	if ( validAreas.IsEmpty() )
		return;

	int iAttempts = 10;
	while( true )
	{
		CTFNavArea *pArea = validAreas.Random();
		m_vecMobSpawnLocation = pArea->GetCenter() + Vector( 0, 0, StepHeight );
		if ( IsSpaceToSpawnHere( m_vecMobSpawnLocation ) )
			break;

		if ( --iAttempts == 0 )
			return;
	}

	m_nZombiesToSpawn = tf_halloween_zombie_mob_spawn_count.GetInt();
}

bool CTFGameRules::CheckCapsPerRound()
{
	if ( tf_flag_caps_per_round.GetInt() > 0 )
	{
		int iMaxCaps = -1;
		CTFTeam *pMaxTeam = NULL;

		// check to see if any team has won a "round"
		int nTeamCount = TFTeamMgr()->GetTeamCount();
		for ( int iTeam = FIRST_GAME_TEAM; iTeam < nTeamCount; ++iTeam )
		{
			CTFTeam *pTeam = GetGlobalTFTeam( iTeam );
			if ( !pTeam )
				continue;

			// we might have more than one team over the caps limit (if the server op lowered the limit)
			// so loop through to see who has the most among teams over the limit
			if ( pTeam->GetFlagCaptures() >= tf_flag_caps_per_round.GetInt() )
			{
				if ( pTeam->GetFlagCaptures() > iMaxCaps )
				{
					iMaxCaps = pTeam->GetFlagCaptures();
					pMaxTeam = pTeam;
				}
			}
		}

		if ( iMaxCaps != -1 && pMaxTeam != NULL )
		{
			SetWinningTeam( pMaxTeam->GetTeamNumber(), WINREASON_FLAG_CAPTURE_LIMIT );
			return true;
		}
	}

	return false;
}

bool CTFGameRules::CheckWinLimit()
{
	if ( mp_winlimit.GetInt() != 0 )
	{
		bool bWinner = false;

		if ( TFTeamMgr()->GetTeam( TF_TEAM_BLUE )->GetScore() >= mp_winlimit.GetInt() )
		{
			UTIL_LogPrintf( "Team \"BLUE\" triggered \"Intermission_Win_Limit\"\n" );
			bWinner = true;
		}
		else if ( TFTeamMgr()->GetTeam( TF_TEAM_RED )->GetScore() >= mp_winlimit.GetInt() )
		{
			UTIL_LogPrintf( "Team \"RED\" triggered \"Intermission_Win_Limit\"\n" );
			bWinner = true;
		}
		else if (TFTeamMgr()->GetTeam(TF_TEAM_GREEN)->GetScore() >= mp_winlimit.GetInt())
		{
			UTIL_LogPrintf("Team \"GREEN\" triggered \"Intermission_Win_Limit\"\n");
			bWinner = true;
		}
		else if (TFTeamMgr()->GetTeam(TF_TEAM_YELLOW)->GetScore() >= mp_winlimit.GetInt())
		{
			UTIL_LogPrintf("Team \"YELLOW\" triggered \"Intermission_Win_Limit\"\n");
			bWinner = true;
		}

		if ( bWinner )
		{
			IGameEvent *event = gameeventmanager->CreateEvent( "tf_game_over" );
			if ( event )
			{
				event->SetString( "reason", "Reached Win Limit" );
				gameeventmanager->FireEvent( event );
			}

			GoToIntermission();
			return true;
		}
	}

	return false;
}

bool CTFGameRules::CheckFragLimit( void )
{
	if ( fraglimit.GetInt() <= 0 )
		return false;

	for ( int i = 1; i <= CountActivePlayers(); i++ )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( pTFPlayer )
		{
			PlayerStats_t *pStats = CTF_GameStats.FindPlayerStats( pTFPlayer );
			int iScore = CalcPlayerScore( &pStats->statsCurrentRound );

			if ( iScore >= fraglimit.GetInt() )
			{
				GoToIntermission();
				return true;
			}
		}
	}

	return false;
}

bool CTFGameRules::IsInPreMatch() const
{
	// TFTODO    return (cb_prematch_time > gpGlobals->time)
	return false;
}

float CTFGameRules::GetPreMatchEndTime() const
{
	//TFTODO: implement this.
	return gpGlobals->curtime;
}

void CTFGameRules::GoToIntermission( void )
{
	CTF_GameStats.Event_GameEnd();

	BaseClass::GoToIntermission();
}

bool CTFGameRules::FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker, const CTakeDamageInfo &info )
{
	// guard against NULL pointers if players disconnect
	if ( !pPlayer || !pAttacker )
		return false;

	// if pAttacker is an object, we can only do damage if pPlayer is our builder
	if ( pAttacker->IsBaseObject() )
	{
		CBaseObject *pObj = (CBaseObject *)pAttacker;

		if ( pObj->GetBuilder() == pPlayer || pPlayer->GetTeamNumber() != pObj->GetTeamNumber() )
		{
			// Builder and enemies
			return true;
		}
		else
		{
			// Teammates of the builder
			return false;
		}
	}

	return BaseClass::FPlayerCanTakeDamage( pPlayer, pAttacker, info );
}

int CTFGameRules::PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget )
{
	return BaseClass::PlayerRelationship( pPlayer, pTarget );
}

bool CTFGameRules::ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen )
{
	return BaseClass::ClientConnected( pEntity, pszName, pszAddress, reject, maxrejectlen );
}

Vector DropToGround(
	CBaseEntity *pMainEnt,
	const Vector &vPos,
	const Vector &vMins,
	const Vector &vMaxs )
{
	trace_t trace;
	UTIL_TraceHull( vPos, vPos + Vector( 0, 0, -500 ), vMins, vMaxs, MASK_SOLID, pMainEnt, COLLISION_GROUP_NONE, &trace );
	return trace.endpos;
}


void TestSpawnPointType( const char *pEntClassName )
{
	// Find the next spawn spot.
	CBaseEntity *pSpot = gEntList.FindEntityByClassname( NULL, pEntClassName );

	while ( pSpot )
	{
		// trace a box here
		Vector vTestMins = pSpot->GetAbsOrigin() + VEC_HULL_MIN;
		Vector vTestMaxs = pSpot->GetAbsOrigin() + VEC_HULL_MAX;

		if ( UTIL_IsSpaceEmpty( pSpot, vTestMins, vTestMaxs ) )
		{
			// the successful spawn point's location
			NDebugOverlay::Box( pSpot->GetAbsOrigin(), VEC_HULL_MIN, VEC_HULL_MAX, 0, 255, 0, 100, 60 );

			// drop down to ground
			Vector GroundPos = DropToGround( NULL, pSpot->GetAbsOrigin(), VEC_HULL_MIN, VEC_HULL_MAX );

			// the location the player will spawn at
			NDebugOverlay::Box( GroundPos, VEC_HULL_MIN, VEC_HULL_MAX, 0, 0, 255, 100, 60 );

			// draw the spawn angles
			QAngle spotAngles = pSpot->GetLocalAngles();
			Vector vecForward;
			AngleVectors( spotAngles, &vecForward );
			NDebugOverlay::HorzArrow( pSpot->GetAbsOrigin(), pSpot->GetAbsOrigin() + vecForward * 32, 10, 255, 0, 0, 255, true, 60 );
		}
		else
		{
			// failed spawn point location
			NDebugOverlay::Box( pSpot->GetAbsOrigin(), VEC_HULL_MIN, VEC_HULL_MAX, 255, 0, 0, 100, 60 );
		}

		// increment pSpot
		pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );
	}
}

// -------------------------------------------------------------------------------- //

void TestSpawns()
{
	TestSpawnPointType( "info_player_teamspawn" );
}
ConCommand cc_TestSpawns( "map_showspawnpoints", TestSpawns, "Dev - test the spawn points, draws for 60 seconds", FCVAR_CHEAT );

// -------------------------------------------------------------------------------- //

void cc_ShowRespawnTimes()
{
	CTFGameRules *pRules = TFGameRules();
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );

	if ( pRules && pPlayer )
	{
		float flRedMin = ( pRules->m_TeamRespawnWaveTimes[TF_TEAM_RED] >= 0 ? pRules->m_TeamRespawnWaveTimes[TF_TEAM_RED] : mp_respawnwavetime.GetFloat() );
		float flRedScalar = pRules->GetRespawnTimeScalar( TF_TEAM_RED );
		float flNextRedRespawn = pRules->GetNextRespawnWave( TF_TEAM_RED, NULL ) - gpGlobals->curtime;

		float flBlueMin = ( pRules->m_TeamRespawnWaveTimes[TF_TEAM_BLUE] >= 0 ? pRules->m_TeamRespawnWaveTimes[TF_TEAM_BLUE] : mp_respawnwavetime.GetFloat() );
		float flBlueScalar = pRules->GetRespawnTimeScalar( TF_TEAM_BLUE );
		float flNextBlueRespawn = pRules->GetNextRespawnWave( TF_TEAM_BLUE, NULL ) - gpGlobals->curtime;

		char tempRed[128];
		Q_snprintf( tempRed, sizeof( tempRed ), "Red:  Min Spawn %2.2f, Scalar %2.2f, Next Spawn In: %.2f\n", flRedMin, flRedScalar, flNextRedRespawn );

		char tempBlue[128];
		Q_snprintf( tempBlue, sizeof( tempBlue ), "Blue: Min Spawn %2.2f, Scalar %2.2f, Next Spawn In: %.2f\n", flBlueMin, flBlueScalar, flNextBlueRespawn );

		ClientPrint( pPlayer, HUD_PRINTTALK, tempRed );
		ClientPrint( pPlayer, HUD_PRINTTALK, tempBlue );
		
		if ( TFGameRules()->IsFourTeamGame() )
		{
			float flGreenMin = (pRules->m_TeamRespawnWaveTimes[TF_TEAM_GREEN] >= 0 ? pRules->m_TeamRespawnWaveTimes[TF_TEAM_GREEN] : mp_respawnwavetime.GetFloat());
			float flGreenScalar = pRules->GetRespawnTimeScalar(TF_TEAM_GREEN);
			float flNextGreenRespawn = pRules->GetNextRespawnWave(TF_TEAM_GREEN, NULL) - gpGlobals->curtime;

			float flYellowMin = (pRules->m_TeamRespawnWaveTimes[TF_TEAM_YELLOW] >= 0 ? pRules->m_TeamRespawnWaveTimes[TF_TEAM_YELLOW] : mp_respawnwavetime.GetFloat());
			float flYellowScalar = pRules->GetRespawnTimeScalar(TF_TEAM_YELLOW);
			float flNextYellowRespawn = pRules->GetNextRespawnWave(TF_TEAM_YELLOW, NULL) - gpGlobals->curtime;

			char tempGreen[128];
			Q_snprintf(tempBlue, sizeof(tempBlue), "Green: Min Spawn %2.2f, Scalar %2.2f, Next Spawn In: %.2f\n", flGreenMin, flGreenScalar, flNextGreenRespawn);

			char tempYellow[128];
			Q_snprintf(tempBlue, sizeof(tempBlue), "Yellow: Min Spawn %2.2f, Scalar %2.2f, Next Spawn In: %.2f\n", flYellowMin, flYellowScalar, flNextYellowRespawn);

			ClientPrint(pPlayer, HUD_PRINTTALK, tempGreen);
			ClientPrint(pPlayer, HUD_PRINTTALK, tempYellow);
		}
	}
}

ConCommand mp_showrespawntimes( "mp_showrespawntimes", cc_ShowRespawnTimes, "Show the min respawn times for the teams" );

// -------------------------------------------------------------------------------- //

CBaseEntity *CTFGameRules::GetPlayerSpawnSpot( CBasePlayer *pPlayer )
{
	// get valid spawn point
	CBaseEntity *pSpawnSpot = pPlayer->EntSelectSpawnPoint();

	// drop down to ground
	Vector GroundPos = DropToGround( pPlayer, pSpawnSpot->GetAbsOrigin(), VEC_HULL_MIN, VEC_HULL_MAX );

	// Move the player to the place it said.
	pPlayer->SetLocalOrigin( GroundPos + Vector( 0, 0, 1 ) );
	pPlayer->SetAbsVelocity( vec3_origin );
	pPlayer->SetLocalAngles( pSpawnSpot->GetLocalAngles() );
	pPlayer->m_Local.m_vecPunchAngle = vec3_angle;
	pPlayer->m_Local.m_vecPunchAngleVel = vec3_angle;
	pPlayer->SnapEyeAngles( pSpawnSpot->GetLocalAngles() );

	return pSpawnSpot;
}

//-----------------------------------------------------------------------------
// Purpose: Checks to see if the player is on the correct team and whether or
//          not the spawn point is available.
//-----------------------------------------------------------------------------
bool CTFGameRules::IsSpawnPointValid( CBaseEntity *pSpot, CBasePlayer *pPlayer, bool bIgnorePlayers )
{
	// Check the team.
	if ( pSpot->GetTeamNumber() != pPlayer->GetTeamNumber() )
		return false;

	if ( !pSpot->IsTriggered( pPlayer ) )
		return false;

	CTFTeamSpawn *pCTFSpawn = dynamic_cast<CTFTeamSpawn *>( pSpot );
	if ( pCTFSpawn )
	{
		if ( pCTFSpawn->IsDisabled() )
			return false;
	}

	Vector mins = VEC_HULL_MIN;
	Vector maxs = VEC_HULL_MAX;

	if ( !bIgnorePlayers )
	{
		Vector vTestMins = pSpot->GetAbsOrigin() + mins;
		Vector vTestMaxs = pSpot->GetAbsOrigin() + maxs;
		return UTIL_IsSpaceEmpty( pPlayer, vTestMins, vTestMaxs );
	}

	trace_t trace;
	UTIL_TraceHull( pSpot->GetAbsOrigin(), pSpot->GetAbsOrigin(), mins, maxs, MASK_PLAYERSOLID, pPlayer, COLLISION_GROUP_PLAYER_MOVEMENT, &trace );
	return !trace.DidHit();
}

Vector CTFGameRules::VecItemRespawnSpot( CItem *pItem )
{
	return pItem->GetOriginalSpawnOrigin();
}

QAngle CTFGameRules::VecItemRespawnAngles( CItem *pItem )
{
	return pItem->GetOriginalSpawnAngles();
}

float CTFGameRules::FlItemRespawnTime( CItem *pItem )
{
	return ITEM_RESPAWN_TIME;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFGameRules::GetChatFormat( bool bTeamOnly, CBasePlayer *pPlayer )
{
	if ( !pPlayer )  // dedicated server output
	{
		return NULL;
	}

	const char *pszFormat = NULL;

	// team only
	if ( bTeamOnly == true )
	{
		if ( pPlayer->GetTeamNumber() == TEAM_SPECTATOR )
		{
			pszFormat = "TF_Chat_Spec";
		}
		else
		{
			if ( pPlayer->IsAlive() == false && State_Get() != GR_STATE_TEAM_WIN )
			{
				pszFormat = "TF_Chat_Team_Dead";
			}
			else
			{
				const char *chatLocation = GetChatLocation( bTeamOnly, pPlayer );
				if ( chatLocation && *chatLocation )
				{
					pszFormat = "TF_Chat_Team_Loc";
				}
				else
				{
					pszFormat = "TF_Chat_Team";
				}
			}
		}
	}
	// everyone
	else
	{	
		if ( pPlayer->GetTeamNumber() == TEAM_SPECTATOR )
		{
			pszFormat = "TF_Chat_AllSpec";	
		}
		else
		{
			if ( pPlayer->IsAlive() == false && State_Get() != GR_STATE_TEAM_WIN )
			{
				pszFormat = "TF_Chat_AllDead";
			}
			else
			{
				pszFormat = "TF_Chat_All";	
			}
		}
	}

	return pszFormat;
}

VoiceCommandMenuItem_t *CTFGameRules::VoiceCommand( CBaseMultiplayerPlayer *pPlayer, int iMenu, int iItem )
{
	VoiceCommandMenuItem_t *pItem = BaseClass::VoiceCommand( pPlayer, iMenu, iItem );
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );

	if ( pItem )
	{
		int iActivity = ActivityList_IndexForName( pItem->m_szGestureActivity );

		if ( iActivity != ACT_INVALID && pTFPlayer )
		{
			pTFPlayer->DoAnimationEvent( PLAYERANIMEVENT_VOICE_COMMAND_GESTURE, iActivity );

			if ( iMenu == 0 && iItem == 0 )
				pTFPlayer->m_lastCalledMedic.Start();
		}
	}

	return pItem;
}

//-----------------------------------------------------------------------------
// Purpose: Actually change a player's name.  
//-----------------------------------------------------------------------------
void CTFGameRules::ChangePlayerName( CTFPlayer *pPlayer, const char *pszNewName )
{
	const char *pszOldName = pPlayer->GetPlayerName();

	CReliableBroadcastRecipientFilter filter;
	UTIL_SayText2Filter( filter, pPlayer, false, "#TF_Name_Change", pszOldName, pszNewName );

	IGameEvent *event = gameeventmanager->CreateEvent( "player_changename" );
	if ( event )
	{
		event->SetInt( "userid", pPlayer->GetUserID() );
		event->SetString( "oldname", pszOldName );
		event->SetString( "newname", pszNewName );
		gameeventmanager->FireEvent( event );
	}

	pPlayer->SetPlayerName( pszNewName );

	pPlayer->m_flNextNameChangeTime = gpGlobals->curtime + 10.0f;
}

ConVar	tf2v_restrict_fov_max( "tf2v_restrict_fov_max", "140", FCVAR_REPLICATED, "Maximum FOV allowed on the server.", true, MAX_FOV, true, MAX_FOV_UNLOCKED );
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::ClientSettingsChanged( CBasePlayer *pPlayer )
{
	const char *pszName = engine->GetClientConVarValue( pPlayer->entindex(), "name" );

	const char *pszOldName = pPlayer->GetPlayerName();

	CTFPlayer *pTFPlayer = (CTFPlayer *)pPlayer;

	// msg everyone if someone changes their name,  and it isn't the first time (changing no name to current name)
	// Note, not using FStrEq so that this is case sensitive
	if ( pszOldName[0] != 0 && Q_strncmp( pszOldName, pszName, MAX_PLAYER_NAME_LENGTH-1 ) )
	{
		if ( pTFPlayer->m_flNextNameChangeTime < gpGlobals->curtime )
		{
			ChangePlayerName( pTFPlayer, pszName );
		}
		else
		{
			// no change allowed, force engine to use old name again
			engine->ClientCommand( pPlayer->edict(), "name \"%s\"", pszOldName );

			// tell client that he hit the name change time limit
			ClientPrint( pTFPlayer, HUD_PRINTTALK, "#Name_change_limit_exceeded" );
		}
	}

	// keep track of their hud_classautokill value
	int nClassAutoKill = Q_atoi( engine->GetClientConVarValue( pPlayer->entindex(), "hud_classautokill" ) );
	pTFPlayer->SetHudClassAutoKill( nClassAutoKill > 0 ? true : false );

	// keep track of their tf_medigun_autoheal value
	pTFPlayer->SetMedigunAutoHeal( Q_atoi( engine->GetClientConVarValue( pPlayer->entindex(), "tf_medigun_autoheal" ) ) > 0 );

	// keep track of their cl_autorezoom value
	pTFPlayer->SetAutoRezoom( Q_atoi( engine->GetClientConVarValue( pPlayer->entindex(), "cl_autorezoom" ) ) > 0 );

	// keep track of their cl_autoreload value
	pTFPlayer->SetAutoReload( Q_atoi( engine->GetClientConVarValue( pPlayer->entindex(), "cl_autoreload" ) ) > 0 );

	pTFPlayer->SetFlipViewModel( Q_atoi( engine->GetClientConVarValue( pPlayer->entindex(), "cl_flipviewmodels" ) ) > 0 );

	const char *pszFov = engine->GetClientConVarValue( pPlayer->entindex(), "fov_desired" );
	int iFov = atoi(pszFov);
	
	// Check the server's FOV restriction and apply it.
	int iAllowedFov = tf2v_restrict_fov_max.GetInt();
	if ( iAllowedFov < MAX_FOV )
		iAllowedFov = MAX_FOV;
	else if ( iAllowedFov > MAX_FOV_UNLOCKED )
		iAllowedFov = MAX_FOV_UNLOCKED;

	iFov = clamp( iFov, 75, iAllowedFov );
	pTFPlayer->SetDefaultFOV( iFov );
	
		
	// Check if the player is someone of note.
	pTFPlayer->m_iPlayerVIPRanking = pTFPlayer->GetPlayerVIPRanking();
	if ( pTFPlayer->m_iPlayerVIPRanking != 0 )
	{
		pTFPlayer->m_bIsPlayerAVIP = true;
		if ( pTFPlayer->m_iPlayerVIPRanking == 1 )	// Rank 1 members are developers.
			pTFPlayer->m_bIsPlayerADev = true;
	}
	
}

static const char *g_aTaggedConVars[] =
{
	"tf_birthday",
	"birthday",

	"tf_halloween",
	"halloween",

	"tf_christmas",
	"christmas",

	"mp_fadetoblack",
	"fadetoblack",

	"mp_friendlyfire",
	"friendlyfire",

	"tf_weapon_criticals",
	"criticals",

	"tf_damage_disablespread",
	"dmgspread",

	"tf_use_fixed_weaponspreads",
	"nospread",

	"tf2v_force_stock_weapons",
	"stockweapons",

	"tf2v_allow_thirdperson",
	"thirdperson",

	"tf2v_randomizer",
	"randomizer",
	
	"tf2v_random_classes",
	"randomclasses",
	
	"tf2v_random_weapons",
	"randomweapons",

	"tf2v_autojump",
	"autojump",

	"tf2v_duckjump",
	"duckjump",

	"tf2v_player_misses",
	"misses",

	"tf2v_airblast",
	"airblast",

	"tf2v_building_hauling",
	"hauling",

	"tf2v_building_upgrades",
	"buildingupgrades",

	"mp_highlander",
	"highlander",

	"mp_disable_respawn_times",
	"norespawntime",

	"mp_respawnwavetime",
	"respawntimes",

	"mp_stalemate_enable",
	"suddendeath",

	"tf_gamemode_arena",
	"arena",

	"tf_gamemode_cp",
	"cp",

	"tf_gamemode_ctf",
	"ctf",

	"tf_gamemode_sd",
	"sd",

	"tf_gamemode_rd",
	"rd",
	
	"tf_gamemode_pd",
	"pd",

	"tf_gamemode_payload",
	"pl",
	
	"tf_gamemode_plr",
	"plr",

	"tf_gamemode_mvm",
	"mvm",

	"tf_gamemode_passtime",
	"pass",
	
	"tf_gamemode_medieval",
	"medieval",
	
	"tf_gamemode_koth",
	"koth",
	
	"tf_gamemode_vsh",
	"vsh",
	
	"tf_gamemode_dr",
	"dr",
	
	"tf2v_allow_objective_glow_ctf",
	"glow_flag",
	
	"tf2v_allow_objective_glow_pl",
	"glow_cart",
	
	"tf2v_allcrit",
	"allcrit",
		
	"tf2v_homing_rockets",
	"homingrockets",
	
	"tf2v_homing_deflected_rockets",
	"homingdeflections",
	
	"tf2v_allow_cosmetics",
	"cosmetics",
	
	"tf2v_allow_reskins",
	"reskins",

	"tf2v_random_classes",
	"randomclasses",
	
	"tf2v_random_weapons",
	"randomweapons",
	
	"tf2v_allow_mod_weapons",
	"customweapons",
	
	"tf2v_enforce_whitelist",
	"whitelist",
	
	"tf_arena_first_blood",
	"firstblood",
	
	"tf2v_ctf_capcrits",
	"capcrits",
	
	"tf2v_use_new_weapon_swap_speed",
	"modernswapspeed",
	
	"tf_enable_grenades",
	"grenades",
	
	
};

//-----------------------------------------------------------------------------
// Purpose: Tags
//-----------------------------------------------------------------------------
void CTFGameRules::GetTaggedConVarList( KeyValues *pCvarTagList )
{
	COMPILE_TIME_ASSERT( ARRAYSIZE( g_aTaggedConVars ) % 2 == 0 );

	BaseClass::GetTaggedConVarList( pCvarTagList );

	for ( int i = 0; i < ARRAYSIZE( g_aTaggedConVars ); i += 2 )
	{
		KeyValues *pKeyValue = new KeyValues( g_aTaggedConVars[i] );
		pKeyValue->SetString( "convar", g_aTaggedConVars[i] );
		pKeyValue->SetString( "tag", g_aTaggedConVars[i+1] );

		pCvarTagList->AddSubKey( pKeyValue );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Return true if the specified player can carry any more of the ammo type
//-----------------------------------------------------------------------------
bool CTFGameRules::CanHaveAmmo( CBaseCombatCharacter *pPlayer, int iAmmoIndex )
{
	if ( iAmmoIndex > -1 )
	{
		CTFPlayer *pTFPlayer = (CTFPlayer *)pPlayer;

		if ( pTFPlayer )
		{
			// Get the max carrying capacity for this ammo
			int iMaxCarry = pTFPlayer->GetMaxAmmo( iAmmoIndex );

			// Does the player have room for more of this type of ammo?
			if ( pTFPlayer->GetAmmoCount( iAmmoIndex ) < iMaxCarry )
			{
				return true;
			}
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::PlayerKilled( CBasePlayer *pVictim, const CTakeDamageInfo &info )
{
	// Find the killer & the scorer
	CBaseEntity *pInflictor = info.GetInflictor();
	CBaseEntity *pKiller = info.GetAttacker();
	CBaseMultiplayerPlayer *pScorer = ToBaseMultiplayerPlayer( GetDeathScorer( pKiller, pInflictor, pVictim ) );
	CTFPlayer *pAssister = NULL;
	CBaseObject *pObject = NULL;

	// if inflictor or killer is a base object, tell them that they got a kill
	// ( depends if a sentry rocket got the kill, sentry may be inflictor or killer )
	if ( pInflictor )
	{
		if ( pInflictor->IsBaseObject() )
		{
			pObject = dynamic_cast<CBaseObject *>( pInflictor );
		}
		else
		{
			CBaseEntity *pInflictorOwner = pInflictor->GetOwnerEntity();
			if ( pInflictorOwner && pInflictorOwner->IsBaseObject() )
			{
				pObject = dynamic_cast<CBaseObject *>( pInflictorOwner );
			}
		}

	}
	else if ( pKiller && pKiller->IsBaseObject() )
	{
		pObject = dynamic_cast<CBaseObject *>( pKiller );
	}

	if ( pObject )
	{
		pObject->IncrementKills();
		pInflictor = pObject;

		if ( pObject->ObjectType() == OBJ_SENTRYGUN )
		{
			CTFPlayer *pOwner = pObject->GetOwner();
			if ( pOwner )
			{
				int iKills = pObject->GetKills();

				// keep track of max kills per a single sentry gun in the player object
				if ( pOwner->GetMaxSentryKills() < iKills )
				{
					pOwner->SetMaxSentryKills( iKills );
					CTF_GameStats.Event_MaxSentryKills( pOwner, iKills );
				}

				// if we just got 10 kills with one sentry, tell the owner's client, which will award achievement if it doesn't have it already
				if ( iKills == 10 )
				{
					pOwner->AwardAchievement( ACHIEVEMENT_TF_GET_TURRETKILLS );
				}
			}
		}
	}

	// if not killed by suicide or killed by world, see if the scorer had an assister, and if so give the assister credit
	if ( ( pVictim != pScorer ) && pKiller )
	{
		pAssister = ToTFPlayer( GetAssister( pVictim, pScorer, pInflictor ) );
		
		if( pAssister && pAssister->IsAlive() )
		{
			// Award them with Focus.
			pAssister->m_Shared.AddFocusLevel(false);
		}
	}

	//find the area the player is in and see if his death causes a block
	CTriggerAreaCapture *pArea = dynamic_cast<CTriggerAreaCapture *>( gEntList.FindEntityByClassname( NULL, "trigger_capture_area" ) );
	while ( pArea )
	{
		if ( pArea->CheckIfDeathCausesBlock( ToBaseMultiplayerPlayer( pVictim ), pScorer ) )
			break;

		pArea = dynamic_cast<CTriggerAreaCapture *>( gEntList.FindEntityByClassname( pArea, "trigger_capture_area" ) );
	}

	// determine if this kill affected a nemesis relationship
	int iDeathFlags = 0;
	CTFPlayer *pTFPlayerVictim = ToTFPlayer( pVictim );
	CTFPlayer *pTFPlayerScorer = ToTFPlayer( pScorer );
	if ( pScorer )
	{
		CalcDominationAndRevenge( pTFPlayerScorer, pTFPlayerVictim, false, &iDeathFlags );
		if ( pAssister )
		{
			CalcDominationAndRevenge( pAssister, pTFPlayerVictim, true, &iDeathFlags );
		}
	}
	pTFPlayerVictim->SetDeathFlags( iDeathFlags );

	if ( pAssister )
	{
		CTF_GameStats.Event_AssistKill( ToTFPlayer( pAssister ), pVictim );
		if ( pObject )
			pObject->IncrementAssists();
	}

	BaseClass::PlayerKilled( pVictim, info );
}

//-----------------------------------------------------------------------------
// Purpose: Determines if attacker and victim have gotten domination or revenge
//-----------------------------------------------------------------------------
void CTFGameRules::CalcDominationAndRevenge( CTFPlayer *pAttacker, CTFPlayer *pVictim, bool bIsAssist, int *piDeathFlags )
{
	PlayerStats_t *pStatsVictim = CTF_GameStats.FindPlayerStats( pVictim );

	// calculate # of unanswered kills between killer & victim - add 1 to include current kill
	int iKillsUnanswered = pStatsVictim->statsKills.iNumKilledByUnanswered[pAttacker->entindex()] + 1;
	if ( TF_KILLS_DOMINATION == iKillsUnanswered )
	{
		// this is the Nth unanswered kill between killer and victim, killer is now dominating victim
		*piDeathFlags |= ( bIsAssist ? TF_DEATH_ASSISTER_DOMINATION : TF_DEATH_DOMINATION );
		// set victim to be dominated by killer
		pAttacker->m_Shared.SetPlayerDominated( pVictim, true );
		// record stats
		CTF_GameStats.Event_PlayerDominatedOther( pAttacker );
	}
	else if ( pVictim->m_Shared.IsPlayerDominated( pAttacker->entindex() ) )
	{
		// the killer killed someone who was dominating him, gains revenge
		*piDeathFlags |= ( bIsAssist ? TF_DEATH_ASSISTER_REVENGE : TF_DEATH_REVENGE );
		// set victim to no longer be dominating the killer
		pVictim->m_Shared.SetPlayerDominated( pAttacker, false );
		// record stats
		CTF_GameStats.Event_PlayerRevenge( pAttacker );
	}

}

//-----------------------------------------------------------------------------
// Purpose: create some proxy entities that we use for transmitting data */
//-----------------------------------------------------------------------------
void CTFGameRules::CreateStandardEntities()
{
	// Create the player resource
	g_pPlayerResource = (CTFPlayerResource *)CBaseEntity::Create( "tf_player_manager", vec3_origin, vec3_angle );

	// Create the objective resource
	g_pObjectiveResource = (CTFObjectiveResource *)CBaseEntity::Create( "tf_objective_resource", vec3_origin, vec3_angle );

	Assert( g_pObjectiveResource );

	CBaseEntity::Create( "monster_resource", vec3_origin, vec3_angle );

	// Create the entity that will send our data to the client.
	CBaseEntity *pEnt = CBaseEntity::Create( "tf_gamerules", vec3_origin, vec3_angle );
	Assert( pEnt );
	pEnt->SetName( AllocPooledString( "tf_gamerules" ) );

	CBaseEntity::Create( "vote_controller", vec3_origin, vec3_angle );

	CKickIssue *pIssue = new CKickIssue( "Kick" );
	pIssue->Init();
}

//-----------------------------------------------------------------------------
// Purpose: create a memorial to our fallen comrade
//-----------------------------------------------------------------------------
void CTFGameRules::CreateSoldierStatue( void )
{
	if ( m_hSoldierStatue )
		return;

	if ( !IsHolidayActive( kHoliday_SoldierMemorial ) )
		return;

	char const *pszMapName = NULL;
	if ( STRING( gpGlobals->mapname ) )
		pszMapName = STRING( gpGlobals->mapname );

	for ( int i=0; i < ARRAYSIZE( s_StatueMaps ); ++i )
	{
		if ( V_stricmp( pszMapName, s_StatueMaps[i].pszMapName ) == 0 )
		{
			m_hSoldierStatue = CreateEntityByName( "entity_soldier_statue" );
			if ( m_hSoldierStatue )
			{
				m_hSoldierStatue->SetAbsOrigin( s_StatueMaps[i].vecOrigin );
				m_hSoldierStatue->SetAbsAngles( s_StatueMaps[i].vecAngles );

				DispatchSpawn( m_hSoldierStatue );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: determine the class name of the weapon that got a kill
//-----------------------------------------------------------------------------
const char *CTFGameRules::GetKillingWeaponName( const CTakeDamageInfo &info, CTFPlayer *pVictim, int &iOutputID )
{
	CBaseEntity *pInflictor = info.GetInflictor();
	CBaseEntity *pKiller = info.GetAttacker();
	CTFPlayer *pScorer = ToTFPlayer( TFGameRules()->GetDeathScorer( pKiller, pInflictor, pVictim ) );
	CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase *>( info.GetWeapon() );
	int iWeaponID = TF_WEAPON_NONE;

	const char *killer_weapon_name = "world";

	// Handle special kill types first.
	const char *pszCustomKill = NULL;

	switch ( info.GetDamageCustom() )
	{
		case TF_DMG_CUSTOM_SUICIDE:
			pszCustomKill = "world";
			break;
		case TF_DMG_CUSTOM_TAUNTATK_GRAND_SLAM:
			pszCustomKill = "taunt_scout";
			break;
		case TF_DMG_CUSTOM_TAUNTATK_HADOUKEN:
			pszCustomKill = "taunt_pyro";
			break;
		case TF_DMG_CUSTOM_TAUNTATK_HIGH_NOON:
			pszCustomKill = "taunt_heavy";
			break;
		case TF_DMG_CUSTOM_TAUNTATK_FENCING:
			pszCustomKill = "taunt_spy";
			break;
		case TF_DMG_CUSTOM_TAUNTATK_ARROW_STAB:
			pszCustomKill = "taunt_sniper";
			break;
		case TF_DMG_CUSTOM_TAUNTATK_UBERSLICE:
			pszCustomKill = "taunt_medic";
			break;
		case TF_DMG_CUSTOM_TAUNTATK_BARBARIAN_SWING:
			pszCustomKill = "taunt_demoman";
			break;
		case TF_DMG_CUSTOM_TAUNTATK_ENGINEER_GUITAR_SMASH:
			pszCustomKill = "taunt_guitar_kill";
			break;
		case TF_DMG_CUSTOM_TAUNTATK_ENGINEER_ARM_KILL:
			pszCustomKill = "robot_arm_blender_kill";
			break;
		case TF_DMG_CUSTOM_STICKBOMB_EXPLOSION:
			pszCustomKill = "ullapool_caber_explosion";
			break;
		case TF_DMG_CUSTOM_BURNING_ARROW:
			pszCustomKill = "huntsman_flyingburn";
			break;
		case TF_DMG_CUSTOM_BASEBALL:
			pszCustomKill = "ball";
			break;
		case TF_DMG_CUSTOM_TELEFRAG:
			pszCustomKill = "telefrag";
			break;
		case TF_DMG_CUSTOM_CARRIED_BUILDING:
			pszCustomKill = "building_carried_destroyed";
			break;
		case TF_DMG_CUSTOM_COMBO_PUNCH:
			pszCustomKill = "robot_arm_combo_kill";
			break;
		case TF_DMG_CUSTOM_PUMPKIN_BOMB:
			pszCustomKill = "pumpkindeath";
			break;
		case TF_DMG_CUSTOM_DECAPITATION_BOSS:
			pszCustomKill = "headtaker";
			break;
		case TF_DMG_CUSTOM_BLEEDING:
			pszCustomKill = "tf_weapon_bleed_kill";
			break;
		case TF_DMG_CUSTOM_MERASMUS_PLAYER_BOMB:
			pszCustomKill = "merasmus_player_bomb";
			break;
		case TF_DMG_CUSTOM_MERASMUS_GRENADE:
			pszCustomKill = "merasmus_grenade";
			break;
		case TF_DMG_CUSTOM_MERASMUS_ZAP:
			pszCustomKill = "merasmus_zap";
			break;
		case TF_DMG_CUSTOM_MERASMUS_DECAPITATION:
			pszCustomKill = "merasmus_decap";
			break;
		case TF_DMG_CUSTOM_CANNONBALL_PUSH:
			pszCustomKill = "loose_cannon_impact";
			break;
	}

	if ( pszCustomKill != NULL )
		return pszCustomKill;

	if ( info.GetDamageCustom() == TF_DMG_CUSTOM_BURNING )
	{
		// Player stores last weapon that burned him so if he burns to death we know what killed him.
		if ( pWeapon )
		{
			killer_weapon_name = pWeapon->GetClassname();
			iWeaponID = pWeapon->GetWeaponID();

			if ( pInflictor && pInflictor != pScorer )
			{
				CTFBaseRocket *pRocket = dynamic_cast<CTFBaseRocket *>( pInflictor );

				if ( pRocket && pRocket->m_iDeflected )
				{
					// Fire weapon deflects go here.
					switch ( pRocket->GetWeaponID() )
					{
						case TF_WEAPON_FLAREGUN:
							killer_weapon_name = "deflect_flare";
							break;
					}
				}
			}
		}
		else
		{
			// Default to flamethrower if no burn weapon is specified.
			killer_weapon_name = "tf_weapon_flamethrower";
			iWeaponID = TF_WEAPON_FLAMETHROWER;
		}
	}
	else if ( pScorer && pInflictor && ( pInflictor == pScorer ) )
	{
		// If the inflictor is the killer, then it must be their current weapon doing the damage
		CTFWeaponBase *pActiveWpn = pScorer->GetActiveTFWeapon();
		if ( pActiveWpn )
		{
			killer_weapon_name = pActiveWpn->GetClassname();
			iWeaponID = pActiveWpn->GetWeaponID();
		}
	}
	else if ( pInflictor )
	{
		killer_weapon_name = pInflictor->GetClassname();

		if ( CTFWeaponBase *pTFWeapon = dynamic_cast<CTFWeaponBase *>( pInflictor ) )
		{
			iWeaponID = pTFWeapon->GetWeaponID();
		}
		// See if this was a deflect kill.
		else if ( CTFBaseRocket *pRocket = dynamic_cast<CTFBaseRocket *>( pInflictor ) )
		{
			iWeaponID = pRocket->GetWeaponID();

			if ( pRocket->m_iDeflected )
			{
				switch ( pRocket->GetWeaponID() )
				{
					case TF_WEAPON_ROCKETLAUNCHER:
					case TF_WEAPON_ROCKETLAUNCHER_AIRSTRIKE:
						killer_weapon_name = "deflect_rocket";
						break;
					case TF_WEAPON_COMPOUND_BOW: //does this go here?
					case TF_WEAPON_CROSSBOW:
						if ( info.GetDamageType() & DMG_IGNITE )
							killer_weapon_name = "deflect_huntsman_flyingburn";
						else
							killer_weapon_name = "deflect_arrow";
						break;
				}
			}
			else
			{
				killer_weapon_name = pRocket->GetClassname();
				CTFWeaponBase *pLauncher = dynamic_cast<CTFWeaponBase *>( pRocket->m_hLauncher.Get() );
				if ( pLauncher )
				{
					iWeaponID = pLauncher->GetWeaponID();
				}
			}
		}
		else if ( CTFWeaponBaseGrenadeProj *pGrenade = dynamic_cast<CTFWeaponBaseGrenadeProj *>( pInflictor ) )
		{
			iWeaponID = pGrenade->GetWeaponID();

			// Most grenades have their own kill icons except for pipes and stickies, those use weapon icons.
			if ( iWeaponID == TF_WEAPON_GRENADE_DEMOMAN || iWeaponID == TF_WEAPON_GRENADE_PIPEBOMB || iWeaponID == TF_WEAPON_GRENADE_PIPEBOMB_BETA )
			{
				CTFWeaponBase *pLauncher = dynamic_cast<CTFWeaponBase *>( pGrenade->m_hLauncher.Get() );
				if ( pLauncher )
				{
					iWeaponID = pLauncher->GetWeaponID();
				}
			}

			if ( pGrenade->m_iDeflected )
			{
				switch ( pGrenade->GetWeaponID() )
				{
					case TF_WEAPON_GRENADE_PIPEBOMB:
						killer_weapon_name = "deflect_sticky";
						break;
					case TF_WEAPON_GRENADE_DEMOMAN:
						killer_weapon_name = "deflect_promode";
						break;
				}
			}
		}
	}

	// strip certain prefixes from inflictor's classname
	const char *prefix[] ={"tf_weapon_grenade_", "tf_weapon_", "NPC_", "func_"};
	for ( int i = 0; i< ARRAYSIZE( prefix ); i++ )
	{
		// if prefix matches, advance the string pointer past the prefix
		int len = V_strlen( prefix[i] );
		if ( V_strncmp( killer_weapon_name, prefix[i], len ) == 0 )
		{
			killer_weapon_name += len;
			break;
		}
	}

	// In case of a sentry kill change the icon according to sentry level.
	if ( !V_strcmp( killer_weapon_name, "obj_sentrygun" ) )
	{
		CObjectSentrygun *pObject = assert_cast<CObjectSentrygun *>( pInflictor );

		if ( pObject )
		{
			if ( pObject->GetState() == SENTRY_STATE_WRANGLED || pObject->GetState() == SENTRY_STATE_WRANGLED_RECOVERY )
			{
				killer_weapon_name = "wrangler_kill";
			}
			else
			{
				switch ( pObject->GetUpgradeLevel() )
				{
					case 2:
						killer_weapon_name = "obj_sentrygun2";
						break;
					case 3:
						killer_weapon_name = "obj_sentrygun3";
						break;
				}

				if ( pObject->IsMiniBuilding() )
				{
					killer_weapon_name = "obj_minisentry";
				}
			}
		}
	}
	else if ( !V_strcmp( killer_weapon_name, "tf_projectile_sentryrocket" ) )
	{
		// look out for sentry rocket as weapon and map it to sentry gun, so we get the L3 sentry death icon
		killer_weapon_name = "obj_sentrygun3";
	}

	else if ( iWeaponID )
	{
		iOutputID = iWeaponID;
	}

	return killer_weapon_name;
}

//-----------------------------------------------------------------------------
// Purpose: returns the player who assisted in the kill, or NULL if no assister
//-----------------------------------------------------------------------------
CBasePlayer *CTFGameRules::GetAssister( CBasePlayer *pVictim, CBasePlayer *pScorer, CBaseEntity *pInflictor )
{
	CTFPlayer *pTFScorer = ToTFPlayer( pScorer );
	CTFPlayer *pTFVictim = ToTFPlayer( pVictim );
	if ( pTFScorer && pTFVictim )
	{
		// if victim killed himself, don't award an assist to anyone else, even if there was a recent damager
		if ( pTFScorer == pTFVictim )
			return NULL;

		// If a player is healing the scorer, give that player credit for the assist
		CTFPlayer *pHealer = ToTFPlayer( static_cast<CBaseEntity *>( pTFScorer->m_Shared.GetFirstHealer() ) );
		// Must be a medic to receive a healing assist, otherwise engineers get credit for assists from dispensers doing healing.
		// Also don't give an assist for healing if the inflictor was a sentry gun, otherwise medics healing engineers get assists for the engineer's sentry kills.
		if ( pHealer && ( TF_CLASS_MEDIC == pHealer->GetPlayerClass()->GetClassIndex() ) && ( NULL == dynamic_cast<CObjectSentrygun *>( pInflictor ) ) )
		{
			return pHealer;
		}
		// Players who apply jarate should get next priority
		CTFPlayer *pThrower = ToTFPlayer( static_cast<CBaseEntity *>( pTFVictim->m_Shared.m_hUrineAttacker.Get() ) );
		if ( pThrower )
		{
			if ( pThrower != pTFScorer )
				return pThrower;
		}

		// Mad Milk after jarate
		CTFPlayer *pThrowerMilk = ToTFPlayer( static_cast<CBaseEntity *>( pTFVictim->m_Shared.m_hMilkAttacker.Get() ) );
		if ( pThrowerMilk )
		{
			if ( pThrowerMilk != pTFScorer )
				return pThrowerMilk;
		}
		
		// Gas after Mad Milk
		CTFPlayer *pThrowerGas = ToTFPlayer( static_cast<CBaseEntity *>( pTFVictim->m_Shared.m_hGasAttacker.Get() ) );
		if ( pThrowerGas )
		{
			if ( pThrowerGas != pTFScorer )
				return pThrowerGas;
		}

		// See who has damaged the victim 2nd most recently (most recent is the killer), and if that is within a certain time window.
		// If so, give that player an assist.  (Only 1 assist granted, to single other most recent damager.)
		CTFPlayer *pRecentDamager = GetRecentDamager( pTFVictim, 1, TF_TIME_ASSIST_KILL );
		if ( pRecentDamager && ( pRecentDamager != pScorer ) )
			return pRecentDamager;
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Returns specifed recent damager, if there is one who has done damage
//			within the specified time period.  iDamager=0 returns the most recent
//			damager, iDamager=1 returns the next most recent damager.
//-----------------------------------------------------------------------------
CTFPlayer *CTFGameRules::GetRecentDamager( CTFPlayer *pVictim, int iDamager, float flMaxElapsed )
{
	Assert( iDamager < MAX_DAMAGER_HISTORY );

	DamagerHistory_t &damagerHistory = pVictim->GetDamagerHistory( iDamager );
	if ( ( NULL != damagerHistory.hDamager ) && ( gpGlobals->curtime - damagerHistory.flTimeDamage <= flMaxElapsed ) )
	{
		CTFPlayer *pRecentDamager = ToTFPlayer( damagerHistory.hDamager );
		if ( pRecentDamager )
			return pRecentDamager;
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Returns who should be awarded the kill
//-----------------------------------------------------------------------------
CBasePlayer *CTFGameRules::GetDeathScorer( CBaseEntity *pKiller, CBaseEntity *pInflictor, CBaseEntity *pVictim )
{
	if ( ( pKiller == pVictim ) && ( pKiller == pInflictor ) )
	{
		// If this was an explicit suicide, see if there was a damager within a certain time window.  If so, award this as a kill to the damager.
		CTFPlayer *pTFVictim = ToTFPlayer( pVictim );
		if ( pTFVictim )
		{
			CTFPlayer *pRecentDamager = GetRecentDamager( pTFVictim, 0, TF_TIME_SUICIDE_KILL_CREDIT );
			if ( pRecentDamager )
				return pRecentDamager;
		}
	}

	return BaseClass::GetDeathScorer( pKiller, pInflictor, pVictim );
}

//-----------------------------------------------------------------------------
// Purpose: Clone
//-----------------------------------------------------------------------------
void CTFGameRules::DeathNotice( CBasePlayer *pVictim, const CTakeDamageInfo &info )
{
	DeathNotice( pVictim, info, "player_death" );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pVictim - 
//			*pKiller - 
//			*pInflictor - 
//-----------------------------------------------------------------------------
void CTFGameRules::DeathNotice( CBasePlayer *pVictim, const CTakeDamageInfo &info, const char *szName )
{
	int killer_ID = 0;

	// Find the killer & the scorer
	CTFPlayer *pTFPlayerVictim = ToTFPlayer( pVictim );
	CBaseEntity *pInflictor = info.GetInflictor();
	CBaseEntity *pKiller = info.GetAttacker();
	CTFPlayer *pScorer = ToTFPlayer( GetDeathScorer( pKiller, pInflictor, pVictim ) );
	CTFPlayer *pAssister = ToTFPlayer( GetAssister( pVictim, pScorer, pInflictor ) );
	int iWeaponID = TF_WEAPON_NONE;

	if ( pScorer )	// Is the killer a client?
	{
		killer_ID = pScorer->GetUserID();
	}

	int iDeathFlags = pTFPlayerVictim->GetDeathFlags();

	if ( ( IsInArenaMode() && ( !IsInVSHMode() || !IsInDRMode() ) ) && tf_arena_first_blood.GetBool() && !m_bFirstBlood && pScorer && pScorer != pTFPlayerVictim )
	{
		m_bFirstBlood = true;
		float flElapsedTime = gpGlobals->curtime - m_flStalemateStartTime;

		if ( flElapsedTime <= 20.0 )
		{
			for ( int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
			{
				BroadcastSound( i, "Announcer.AM_FirstBloodFast" );
			}
		}
		else if ( flElapsedTime < 50.0 )
		{
			for ( int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
			{
				BroadcastSound( i, "Announcer.AM_FirstBloodRandom" );
			}
		}
		else
		{
			for ( int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
			{
				BroadcastSound( i, "Announcer.AM_FirstBloodFinally" );
			}
		}
		
		float flFirstBloodDuration = tf_arena_first_blood_length.GetFloat();

		iDeathFlags |= TF_DEATH_FIRST_BLOOD;
		if ( flFirstBloodDuration > 0.0f )
		pScorer->m_Shared.AddCond( TF_COND_CRITBOOSTED_FIRST_BLOOD, flFirstBloodDuration );
	}

	if ( pTFPlayerVictim->m_Shared.IsFeigningDeath() )
	{
		iDeathFlags |= TF_DEATH_FEIGN_DEATH;

		CTFPlayer *pDisguiseVictim = ToTFPlayer( pTFPlayerVictim->m_Shared.GetDisguiseTarget() );
		if ( pDisguiseVictim && pDisguiseVictim->GetTeamNumber() == pVictim->GetTeamNumber() ) // Make them think they killed our teammate
		{
			pVictim = pDisguiseVictim;
			pTFPlayerVictim = pDisguiseVictim;
		}
	}

	if ( info.GetWeapon() )
	{
		int nTurnToAustralium = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( info.GetWeapon(), nTurnToAustralium, is_australium_item );
		if ( nTurnToAustralium )
			iDeathFlags |= TF_DEATH_AUSTRALIUM;
	}

	pTFPlayerVictim->SetDeathFlags( iDeathFlags );

	// Work out what killed the player, and send a message to all clients about it
	const char *killer_weapon_name = GetKillingWeaponName( info, pTFPlayerVictim, iWeaponID );
	const char *killer_weapon_log_name = NULL;


	if ( iWeaponID && pScorer )
	{
		CTFWeaponBase *pWeapon = pScorer->Weapon_OwnsThisID( iWeaponID );
		if ( pWeapon )
		{
			CEconItemDefinition *pItemDef = pWeapon->GetItem()->GetStaticData();
			if ( pItemDef )
			{
				if ( pItemDef->GetItemIcon() )
					killer_weapon_name = pItemDef->GetItemIcon();

				if ( pItemDef->GetLogName() )
					killer_weapon_log_name = pItemDef->GetLogName();
			}
		}
	}
	
	if ( info.GetDamageCustom() == TF_DMG_CUSTOM_BOOTS_STOMP )
	{
		killer_weapon_name = "mantreads";
		killer_weapon_log_name = "mantreads";
	}
	
	IGameEvent *event = gameeventmanager->CreateEvent( szName );
	if ( event )
	{
		event->SetInt( "userid", pVictim->GetUserID() );
		event->SetInt( "attacker", killer_ID );
		event->SetInt( "assister", pAssister ? pAssister->GetUserID() : -1 );
		event->SetString( "weapon", killer_weapon_name );
		event->SetInt( "weaponid", iWeaponID );
		event->SetString( "weapon_logclassname", killer_weapon_log_name );
		event->SetInt( "playerpenetratecount", info.GetPlayerPenetrationCount() );
		event->SetInt( "damagebits", info.GetDamageType() );
		event->SetInt( "customkill", info.GetDamageCustom() );
		event->SetInt( "priority", 7 );	// HLTV event priority, not transmitted
		event->SetInt( "death_flags", pTFPlayerVictim->GetDeathFlags() );
	#if 0
		if ( pTFPlayerVictim->GetDeathFlags() & TF_DEATH_DOMINATION )
		{
			event->SetInt( "dominated", 1 );
		}
		if ( pTFPlayerVictim->GetDeathFlags() & TF_DEATH_ASSISTER_DOMINATION )
		{
			event->SetInt( "assister_dominated", 1 );
		}
		if ( pTFPlayerVictim->GetDeathFlags() & TF_DEATH_REVENGE )
		{
			event->SetInt( "revenge", 1 );
		}
		if ( pTFPlayerVictim->GetDeathFlags() & TF_DEATH_ASSISTER_REVENGE )
		{
			event->SetInt( "assister_revenge", 1 );
		}
	#endif
		CTFPlayer *pTFAttacker = ToTFPlayer( info.GetAttacker() );
		if ( pTFAttacker && pTFAttacker->GetActiveTFWeapon() )
		{
			CTFWeaponBase *pActive = pTFAttacker->GetActiveTFWeapon();
			if ( pActive )
			{
				bool bIsSilent = pActive->IsSilentKiller();
				if ( pActive->IsWeapon( TF_WEAPON_KNIFE ) )	// Knife only: Check for backstab.
					event->SetBool( "silent_kill", ( info.GetDamageCustom() == TF_DMG_CUSTOM_BACKSTAB && bIsSilent ) );
					
			}
		}

		gameeventmanager->FireEvent( event );
	}
}

void CTFGameRules::ClientDisconnected( edict_t *pClient )
{
	// clean up anything they left behind
	CTFPlayer *pPlayer = ToTFPlayer( GetContainingEntity( pClient ) );
	const char *pszPlayerName;

	if ( pPlayer )
	{
		pPlayer->TeamFortress_ClientDisconnected();
		pszPlayerName = pPlayer->GetPlayerName();

		// If they're queued up take them out
		RemovePlayerFromQueue( pPlayer );
	}

	// are any of the spies disguising as this player?
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTFPlayer *pTemp = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( pTemp && pTemp != pPlayer )
		{
			if ( pTemp->m_Shared.GetDisguiseTarget() == pPlayer )
			{
				// choose someone else...
				pTemp->m_Shared.FindDisguiseTarget();
			}
		}
	}

	BaseClass::ClientDisconnected( pClient );

	// Do this last so that the player isn't registered on the team anymore
	Arena_ClientDisconnect( pPlayer->GetPlayerName() );
}


// Falling damage stuff.
#define TF_PLAYER_MAX_SAFE_FALL_SPEED	650		

float CTFGameRules::FlPlayerFallDamage( CBasePlayer *pPlayer )
{
	if ( pPlayer->m_Local.m_flFallVelocity > TF_PLAYER_MAX_SAFE_FALL_SPEED )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer(pPlayer);
		if ( pTFPlayer )
		{
			// Check if we have an attribute to cancel the fall damage first.
			int nCancelFallDamage = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER( pTFPlayer, nCancelFallDamage, cancel_falling_damage );
			if ( nCancelFallDamage != 0 )
				return 0;	// No fall damage if we have an item to cancel it.
		}
		
		// Old TFC damage formula
		float flFallDamage = 5 * ( pPlayer->m_Local.m_flFallVelocity / 300 );

		// Fall damage needs to scale according to the player's max health, or
		// it's always going to be much more dangerous to weaker classes than larger.
		float flRatio = (float)pPlayer->GetMaxHealth() / 100.0;
		flFallDamage *= flRatio;
		
		if ( tf2v_falldamage_disablespread.GetBool() == false )
		{
			flFallDamage *= random->RandomFloat( 0.8, 1.2 );
		}
		
		// If we're a boss, no fall damage.
		if ( IsBossClass(pPlayer) )
		{
			return 0;
		}

		return flFallDamage;
	}

	// Fall caused no damage
	return 0;
}

void CTFGameRules::SendWinPanelInfo( void )
{
	IGameEvent *winEvent = gameeventmanager->CreateEvent( "teamplay_win_panel" );

	if ( winEvent )
	{
		int iBlueScore = GetGlobalTeam( TF_TEAM_BLUE )->GetScore();
		int iRedScore = GetGlobalTeam( TF_TEAM_RED )->GetScore();
		int iBlueScorePrev = iBlueScore;
		int iRedScorePrev = iRedScore;
		
		int iGreenScore = GetGlobalTeam(TF_TEAM_GREEN)->GetScore();
		int iYellowScore = GetGlobalTeam(TF_TEAM_YELLOW)->GetScore();
		int iGreenScorePrev = iGreenScore;
		int iYellowScorePrev = iYellowScore;

		bool bRoundComplete = m_bForceMapReset || ( IsGameUnderTimeLimit() && ( GetTimeLeft() <= 0 ) );

		CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
		bool bScoringPerCapture = ( pMaster ) ? ( pMaster->ShouldScorePerCapture() ) : false;

		if ( bRoundComplete && !bScoringPerCapture )
		{
			// if this is a complete round, calc team scores prior to this win
			switch ( m_iWinningTeam )
			{
				case TF_TEAM_BLUE:
					iBlueScorePrev = ( iBlueScore - TEAMPLAY_ROUND_WIN_SCORE >= 0 ) ? ( iBlueScore - TEAMPLAY_ROUND_WIN_SCORE ) : 0;
					break;
				case TF_TEAM_RED:
					iRedScorePrev = ( iRedScore - TEAMPLAY_ROUND_WIN_SCORE >= 0 ) ? ( iRedScore - TEAMPLAY_ROUND_WIN_SCORE ) : 0;
					break;
				case TF_TEAM_GREEN:
					if ( !IsFourTeamGame() )
						break;
					iGreenScorePrev = ( iGreenScore - TEAMPLAY_ROUND_WIN_SCORE >= 0 ) ? (iGreenScore - TEAMPLAY_ROUND_WIN_SCORE ) : 0;
					break;
				case TF_TEAM_YELLOW:
					if ( !IsFourTeamGame() )
						break;
					iYellowScorePrev = ( iYellowScore - TEAMPLAY_ROUND_WIN_SCORE >= 0 ) ? (iYellowScore - TEAMPLAY_ROUND_WIN_SCORE ) : 0;
					break;
				
				case TEAM_UNASSIGNED:
					break;	// stalemate; nothing to do
			}
		}

		winEvent->SetInt( "panel_style", WINPANEL_BASIC );
		winEvent->SetInt( "winning_team", m_iWinningTeam );
		winEvent->SetInt( "winreason", m_iWinReason );
		winEvent->SetString( "cappers", ( m_iWinReason == WINREASON_ALL_POINTS_CAPTURED || m_iWinReason == WINREASON_FLAG_CAPTURE_LIMIT ) ? m_szMostRecentCappers : "" );
		winEvent->SetInt( "flagcaplimit", tf_flag_caps_per_round.GetInt() );
		winEvent->SetInt( "blue_score", iBlueScore );
		winEvent->SetInt( "red_score", iRedScore );
		winEvent->SetInt( "blue_score_prev", iBlueScorePrev );
		winEvent->SetInt( "red_score_prev", iRedScorePrev );
		winEvent->SetInt( "round_complete", bRoundComplete );
		
		if ( IsFourTeamGame() )
		{
			winEvent->SetInt( "green_score", iGreenScore );
			winEvent->SetInt( "yellow_score", iYellowScore );
			winEvent->SetInt( "green_score_prev", iGreenScorePrev );
			winEvent->SetInt( "yellow_score_prev", iYellowScorePrev );	
		}

		CTFPlayerResource *pResource = dynamic_cast<CTFPlayerResource *>( g_pPlayerResource );
		if ( !pResource )
			return;

		// determine the 3 players on winning team who scored the most points this round

		// build a vector of players & round scores
		CUtlVector<PlayerRoundScore_t> vecPlayerScore;
		int iPlayerIndex;
		for ( iPlayerIndex = 1; iPlayerIndex <= MAX_PLAYERS; iPlayerIndex++ )
		{
			CTFPlayer *pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( iPlayerIndex ) );
			if ( !pTFPlayer || !pTFPlayer->IsConnected() )
				continue;
			// filter out spectators and, if not stalemate, all players not on winning team
			int iPlayerTeam = pTFPlayer->GetTeamNumber();
			if ( ( iPlayerTeam < FIRST_GAME_TEAM ) || ( m_iWinningTeam != TEAM_UNASSIGNED && ( m_iWinningTeam != iPlayerTeam ) ) )
				continue;

			int iRoundScore = 0, iTotalScore = 0;
			int iKills = 0, iDeaths = 0;
			PlayerStats_t *pStats = CTF_GameStats.FindPlayerStats( pTFPlayer );
			if ( pStats )
			{
				iRoundScore = CalcPlayerScore( &pStats->statsCurrentRound );
				iTotalScore = CalcPlayerScore( &pStats->statsAccumulated );
				iKills = pStats->statsCurrentRound.m_iStat[TFSTAT_KILLS];
				iDeaths = pStats->statsCurrentRound.m_iStat[TFSTAT_DEATHS];
			}
			PlayerRoundScore_t &playerRoundScore = vecPlayerScore[vecPlayerScore.AddToTail()];
			playerRoundScore.iPlayerIndex = iPlayerIndex;
			playerRoundScore.iRoundScore = iRoundScore;
			playerRoundScore.iTotalScore = iTotalScore;
			playerRoundScore.iKills = iKills;
			playerRoundScore.iDeaths = iDeaths;
		}
		// sort the players by round score
		vecPlayerScore.Sort( PlayerRoundScoreSortFunc );

		// set the top (up to) 3 players by round score in the event data
		int numPlayers = Min( 3, vecPlayerScore.Count() );
		for ( int i = 0; i < numPlayers; i++ )
		{
			// only include players who have non-zero points this round; if we get to a player with 0 round points, stop
			if ( 0 == vecPlayerScore[i].iRoundScore )
				break;

			// set the player index and their round score in the event
			char szPlayerIndexVal[64] = "", szPlayerScoreVal[64] = "";
			char szPlayerKillsVal[64] = "", szPlayerDeathsVal[64] = "";
			Q_snprintf( szPlayerIndexVal, ARRAYSIZE( szPlayerIndexVal ), "player_%d", i + 1 );
			Q_snprintf( szPlayerScoreVal, ARRAYSIZE( szPlayerScoreVal ), "player_%d_points", i + 1 );
			Q_snprintf( szPlayerKillsVal, ARRAYSIZE( szPlayerKillsVal ), "player_%d_kills", i + 1 );
			Q_snprintf( szPlayerDeathsVal, ARRAYSIZE( szPlayerDeathsVal ), "player_%d_deaths", i + 1 );
			winEvent->SetInt( szPlayerIndexVal, vecPlayerScore[i].iPlayerIndex );
			winEvent->SetInt( szPlayerScoreVal, vecPlayerScore[i].iRoundScore );
			winEvent->SetInt( szPlayerKillsVal, vecPlayerScore[i].iKills );
			winEvent->SetInt( szPlayerDeathsVal, vecPlayerScore[i].iDeaths );
		}

		if ( !bRoundComplete && ( TEAM_UNASSIGNED != m_iWinningTeam ) )
		{
			// if this was not a full round ending, include how many mini-rounds remain for winning team to win
			if ( g_hControlPointMasters.Count() && g_hControlPointMasters[0] )
			{
				winEvent->SetInt( "rounds_remaining", g_hControlPointMasters[0]->CalcNumRoundsRemaining( m_iWinningTeam ) );
			}
		}

		// Send the event
		gameeventmanager->FireEvent( winEvent );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sorts players by round score
//-----------------------------------------------------------------------------
int CTFGameRules::PlayerRoundScoreSortFunc( const PlayerRoundScore_t *pRoundScore1, const PlayerRoundScore_t *pRoundScore2 )
{
	// sort first by round score	
	if ( pRoundScore1->iRoundScore != pRoundScore2->iRoundScore )
		return pRoundScore2->iRoundScore - pRoundScore1->iRoundScore;

	// if round scores are the same, sort next by total score
	if ( pRoundScore1->iTotalScore != pRoundScore2->iTotalScore )
		return pRoundScore2->iTotalScore - pRoundScore1->iTotalScore;

	// if scores are the same, sort next by player index so we get deterministic sorting
	return ( pRoundScore2->iPlayerIndex - pRoundScore1->iPlayerIndex );
}

//-----------------------------------------------------------------------------
// Purpose: Called when the teamplay_round_win event is about to be sent, gives
//			this method a chance to add more data to it
//-----------------------------------------------------------------------------
void CTFGameRules::FillOutTeamplayRoundWinEvent( IGameEvent *event )
{
	// determine the losing team
	int iLosingTeam;

	switch ( event->GetInt( "team" ) )
	{
		case TF_TEAM_RED:
			iLosingTeam = TF_TEAM_BLUE;
			break;
		case TF_TEAM_BLUE:
			iLosingTeam = TF_TEAM_RED;
			break;
		default:
			iLosingTeam = TEAM_UNASSIGNED;
			break;
	}

	// set the number of caps that team got any time during the round
	event->SetInt( "losing_team_num_caps", m_iNumCaps[iLosingTeam] );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::StartCompetitiveMatch( void )
{
	SetInWaitingForPlayers( false );
	CleanUpMap();
	State_Transition( GR_STATE_RESTART );
	ResetPlayerAndTeamReadyState();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::EndCompetitiveMatch( void )
{
	State_Transition( GR_STATE_RESTART );
	SetInWaitingForPlayers( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::StopCompetitiveMatch( int eMatchResult )
{
	IGameEvent *event = gameeventmanager->CreateEvent( "competitive_victory" );
	if ( event )
	{
		gameeventmanager->FireEvent( event );
	}

	if ( eMatchResult == 3 )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "player_abandoned_match" );
		if ( event )
		{
			event->SetBool( "game_over", State_Get() == GR_STATE_BETWEEN_RNDS );
			gameeventmanager->FireEvent( event );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::UsePlayerReadyStatusMode( void )
{
	if ( IsMannVsMachineMode() )
		return true;

	if ( IsCompetitiveMode() )
		return true;

	if ( mp_tournament.GetBool() )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::SetupSpawnPointsForRound( void )
{
	if ( !g_hControlPointMasters.Count() || !g_hControlPointMasters[0] || !g_hControlPointMasters[0]->PlayingMiniRounds() )
		return;

	CTeamControlPointRound *pCurrentRound = g_hControlPointMasters[0]->GetCurrentRound();
	if ( !pCurrentRound )
	{
		return;
	}

	// loop through the spawn points in the map and find which ones are associated with this round or the control points in this round
	CBaseEntity *pSpot = gEntList.FindEntityByClassname( NULL, "info_player_teamspawn" );
	while ( pSpot )
	{
		CTFTeamSpawn *pTFSpawn = assert_cast<CTFTeamSpawn *>( pSpot );

		if ( pTFSpawn )
		{
			CHandle<CTeamControlPoint> hControlPoint = pTFSpawn->GetControlPoint();
			CHandle<CTeamControlPointRound> hRoundBlue = pTFSpawn->GetRoundBlueSpawn();
			CHandle<CTeamControlPointRound> hRoundRed = pTFSpawn->GetRoundRedSpawn();
			CHandle<CTeamControlPointRound> hRoundGreen = pTFSpawn->GetRoundGreenSpawn();
			CHandle<CTeamControlPointRound> hRoundYellow = pTFSpawn->GetRoundYellowSpawn();

			if ( hControlPoint && pCurrentRound->IsControlPointInRound( hControlPoint ) )
			{
				// this spawn is associated with one of our control points
				pTFSpawn->SetDisabled( false );
				pTFSpawn->ChangeTeam( hControlPoint->GetOwner() );
			}
			else if ( hRoundBlue && ( hRoundBlue == pCurrentRound ) )
			{
				pTFSpawn->SetDisabled( false );
				pTFSpawn->ChangeTeam( TF_TEAM_BLUE );
			}
			else if ( hRoundRed && ( hRoundRed == pCurrentRound ) )
			{
				pTFSpawn->SetDisabled( false );
				pTFSpawn->ChangeTeam( TF_TEAM_RED );
			}
			else if ( hRoundGreen && ( hRoundGreen == pCurrentRound ) )
			{
				pTFSpawn->SetDisabled(false);
				pTFSpawn->ChangeTeam( TF_TEAM_GREEN );
			}
			else if ( hRoundYellow && (hRoundYellow == pCurrentRound))
			{
				pTFSpawn->SetDisabled(false);
				pTFSpawn->ChangeTeam( TF_TEAM_YELLOW );
			}
			else
			{
				// this spawn isn't associated with this round or the control points in this round
				pTFSpawn->SetDisabled( true );
			}
		}

		pSpot = gEntList.FindEntityByClassname( pSpot, "info_player_teamspawn" );
	}
}


int CTFGameRules::SetCurrentRoundStateBitString( void )
{
	m_iPrevRoundState = m_iCurrentRoundState;

	CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;

	if ( !pMaster )
	{
		return 0;
	}

	int iState = 0;

	for ( int i=0; i<pMaster->GetNumPoints(); i++ )
	{
		CTeamControlPoint *pPoint = pMaster->GetControlPoint( i );

		if ( pPoint->GetOwner() == TF_TEAM_BLUE )
		{
			// Set index to 1 for the point being owned by blue
			iState |= ( 1<<i );
		}
	}

	m_iCurrentRoundState = iState;

	return iState;
}


void CTFGameRules::SetMiniRoundBitMask( int iMask )
{
	m_iCurrentMiniRoundMask = iMask;
}

//-----------------------------------------------------------------------------
// Purpose: NULL pPlayer means show the panel to everyone
//-----------------------------------------------------------------------------
void CTFGameRules::ShowRoundInfoPanel( CTFPlayer *pPlayer /* = NULL */ )
{
	KeyValues *data = new KeyValues( "data" );

	if ( m_iCurrentRoundState < 0 )
	{
		// Haven't set up the round state yet
		return;
	}

	// if prev and cur are equal, we are starting from a fresh round
	if ( m_iPrevRoundState >= 0 && pPlayer == NULL )	// we have data about a previous state
	{
		data->SetInt( "prev", m_iPrevRoundState );
	}
	else
	{
		// don't send a delta if this is just to one player, they are joining mid-round
		data->SetInt( "prev", m_iCurrentRoundState );
	}

	data->SetInt( "cur", m_iCurrentRoundState );

	// get bitmask representing the current miniround
	data->SetInt( "round", m_iCurrentMiniRoundMask );

	if ( pPlayer )
	{
		pPlayer->ShowViewPortPanel( PANEL_ROUNDINFO, true, data );
	}
	else
	{
		for ( int i = 1; i <= MAX_PLAYERS; i++ )
		{
			CTFPlayer *pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

			if ( pTFPlayer && pTFPlayer->IsReadyToPlay() )
			{
				pTFPlayer->ShowViewPortPanel( PANEL_ROUNDINFO, true, data );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::TimerMayExpire( void )
{
	// Prevent timers expiring while control points are contested
	int iNumControlPoints = ObjectiveResource()->GetNumControlPoints();
	for ( int iPoint = 0; iPoint < iNumControlPoints; iPoint++ )
	{
		if ( ObjectiveResource()->GetCappingTeam( iPoint ) )
		{
			// HACK: Fix for some maps adding time to the clock 0.05s after CP is capped.
			// This also fixes payload overtime ending prematurely under certain circumstances
			m_flTimerMayExpireAt = gpGlobals->curtime + 0.1f;
			return false;
		}
	}

	if ( m_flTimerMayExpireAt >= gpGlobals->curtime )
		return false;

	return BaseClass::TimerMayExpire();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::HandleCTFCaptureBonus( int iTeam )
{
	float flBoostTime = tf_ctf_bonus_time.GetFloat();

	if ( m_flCTFBonusTime == 0 )
		return; //Bail on calculating the rest of the function if capture crits are disabled.
	
	if ( m_flCTFBonusTime > -1 )
		flBoostTime = m_flCTFBonusTime;

	if ( ( flBoostTime > 0.0 ) && ( tf2v_ctf_capcrits.GetBool() ) )
	{
		for ( int i = 1; i < gpGlobals->maxClients; i++ )
		{
			CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

			if ( !pPlayer )
				continue;

			if ( pPlayer->GetTeamNumber() == iTeam && pPlayer->IsAlive() )
			{
				pPlayer->m_Shared.AddCond( TF_COND_CRITBOOSTED_CTF_CAPTURE, flBoostTime );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::Arena_CleanupPlayerQueue( void )
{
	for ( int i = 0; i < MAX_PLAYERS; i++ )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( pTFPlayer && pTFPlayer->IsReadyToPlay() && !pTFPlayer->IsHLTV() && !pTFPlayer->IsReplay() && pTFPlayer->GetTeamNumber() != TEAM_SPECTATOR )
		{
			RemovePlayerFromQueue( pTFPlayer );
			pTFPlayer->m_bInArenaQueue = false;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::Arena_ClientDisconnect( const char *pszPlayerName )
{
	int iLightTeam = TEAM_UNASSIGNED, iHeavyTeam = TEAM_UNASSIGNED, iPlayers;
	CTeam *pLightTeam = NULL, *pHeavyTeam = NULL;
	CTFPlayer *pTFPlayer = NULL;

	if ( !TFGameRules()->IsInArenaMode() )
		return;

	if ( IsInWaitingForPlayers() || State_Get() != GR_STATE_PREROUND )
		return;

	if ( IsInTournamentMode() ||  !AreTeamsUnbalanced( iHeavyTeam, iLightTeam ) )
		return;

	pHeavyTeam = GetGlobalTeam( iHeavyTeam );
	pLightTeam = GetGlobalTeam( iLightTeam );
	if ( pHeavyTeam && pLightTeam )
	{
		iPlayers = pHeavyTeam->GetNumPlayers() - pLightTeam->GetNumPlayers();

		// Are there players waiting to play?
		if ( m_hArenaQueue.Count() > 0 && iPlayers > 0 )
		{
			for ( int i = 0; i < iPlayers; i++ )
			{
				pTFPlayer = m_hArenaQueue.Head();
				if ( pTFPlayer && !pTFPlayer->IsHLTV() && !pTFPlayer->IsReplay() )
				{
					pTFPlayer->ForceChangeTeam( TF_TEAM_AUTOASSIGN );

					const char *pszTeamName;

					// Figure out what team the player got balanced to
					if ( pTFPlayer->GetTeam() == pLightTeam )
						pszTeamName = pLightTeam->GetName();
					else
						pszTeamName = pHeavyTeam->GetName();

					UTIL_ClientPrintAll( HUD_PRINTTALK, "#TF_Arena_ClientDisconnect", pTFPlayer->GetPlayerName(), pszTeamName, pszPlayerName );

					if ( !pTFPlayer->m_bInArenaQueue )
						pTFPlayer->m_flArenaQueueTime = gpGlobals->curtime;

					RemovePlayerFromQueue( pTFPlayer );

					pTFPlayer->m_bInArenaQueue = false;
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::Arena_ResetLosersScore( bool bStreakReached )
{
	if ( bStreakReached )
	{
		for ( int i = 0; i < GetNumberOfTeams(); i++ )
		{
			if ( i != GetWinningTeam() )
				GetWinningTeam();

			CTeam *pTeam = GetGlobalTeam( i );

			if ( pTeam )
				pTeam->ResetScores();
		}
	}
	else
	{
		for ( int i = 0; i < GetNumberOfTeams(); i++ )
		{
			if ( i != GetWinningTeam() && GetWinningTeam() > TEAM_SPECTATOR )
			{
				CTeam *pTeam = GetGlobalTeam( i );

				if ( pTeam )
					pTeam->ResetScores();
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::Arena_NotifyTeamSizeChange( void )
{
	CTeam *pTeam = GetGlobalTFTeam( TF_TEAM_BLUE );
	int iTeamCount = pTeam->GetNumPlayers();
	if ( iTeamCount != m_iArenaTeamCount )
	{
		if ( m_iArenaTeamCount )
		{
			if ( iTeamCount >= m_iArenaTeamCount )
			{
				UTIL_ClientPrintAll( HUD_PRINTTALK, "#TF_Arena_TeamSizeIncreased", UTIL_VarArgs( "%d", iTeamCount ) );
			}
			else
			{
				UTIL_ClientPrintAll( HUD_PRINTTALK, "#TF_Arena_TeamSizeDecreased", UTIL_VarArgs( "%d", iTeamCount ) );
			}
		}
		m_iArenaTeamCount = iTeamCount;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGameRules::Arena_PlayersNeededForMatch( void )
{
	int i = 0, j = 0, iReadyToPlay = 0, iNumPlayers = 0, iPlayersNeeded = 0, iMaxTeamSize = 0, iDesiredTeamSize = 0;
	CTFPlayer *pTFPlayer = NULL, *pTFPlayerToBalance = NULL;

	// 1/3 of the players in a full server spectate
	iMaxTeamSize = gpGlobals->maxClients;
	if ( HLTVDirector()->IsActive() )
		iMaxTeamSize--;
	iMaxTeamSize = ( iMaxTeamSize / 3.0 ) + 0.5;

	for ( i = 1; i <= MAX_PLAYERS; i++ )
	{
		pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( pTFPlayer && pTFPlayer->IsReadyToPlay() )
		{
			iReadyToPlay++;
		}
	}

	// **NOTE: live TF2 sets iDesiredTeamSize 
	// to ( iPlayersNeeded + iReadyToPlay ) / 2
	// but this isn't working due to overlap with 
	// the arena queue and IsReadyToPlay()

	iPlayersNeeded = m_hArenaQueue.Count();
	iDesiredTeamSize = ( iReadyToPlay ) / 2;

	// keep the teams even 
	if ( iReadyToPlay % 2 != 0 )
		iPlayersNeeded--;

	if ( iDesiredTeamSize > iMaxTeamSize )
	{
		iPlayersNeeded += ( 2 * iMaxTeamSize ) - iReadyToPlay;

		if ( iPlayersNeeded < 0 )
			iPlayersNeeded = 0;
	}

	if ( GetWinningTeam() > TEAM_SPECTATOR )
	{
		iNumPlayers = GetGlobalTFTeam( GetWinningTeam() )->GetNumPlayers();
		if ( iNumPlayers <= iMaxTeamSize )
		{
			// Is the winning team larger than the desired team size?
			if ( iNumPlayers <= iDesiredTeamSize )
				return iPlayersNeeded;
			iMaxTeamSize = iDesiredTeamSize;
		}
	}
	else
	{
		return iPlayersNeeded;
	}

	// Keep going until teams are balanced
	while ( iNumPlayers >= iMaxTeamSize )
	{
		// Shortest queue time starts very large
		float flShortestQueueTime = 9999.9f, flTimeQueued = 0.0f;

		for ( j = 1; i < MAX_PLAYERS; i++ )
		{
			pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
			if ( pTFPlayer && pTFPlayer->GetTeamNumber() == GetWinningTeam() && !pTFPlayer->IsHLTV() && !pTFPlayer->IsReplay() )
			{
				flTimeQueued = gpGlobals->curtime - pTFPlayer->m_flArenaQueueTime;
				if ( flShortestQueueTime > flTimeQueued )
				{
					// Save the player who has been on the team for the shortest time
					flShortestQueueTime = flTimeQueued;
					pTFPlayerToBalance = pTFPlayer;
				}
			}
		}

		if ( pTFPlayerToBalance )
		{
			// Move the player into the front of the queue
			pTFPlayerToBalance->ForceChangeTeam( TEAM_SPECTATOR );

			pTFPlayerToBalance->m_flArenaQueueTime = gpGlobals->curtime;
			if ( iPlayersNeeded < iMaxTeamSize )
				++iPlayersNeeded;

			// Balance the player
			IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_teambalanced_player" );
			if ( event )
			{
				event->SetInt( "team", GetWinningTeam() );
				event->SetInt( "player", pTFPlayerToBalance->GetUserID() );
				gameeventmanager->FireEvent( event );
			}
			UTIL_ClientPrintAll( HUD_PRINTTALK, "#game_player_was_team_balanced", UTIL_VarArgs( "%s", pTFPlayerToBalance->GetPlayerName() ) );

			pTFPlayerToBalance = NULL;
		}
		iNumPlayers = GetGlobalTFTeam( GetWinningTeam() )->GetNumPlayers();
	}

	return iPlayersNeeded;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::Arena_PrepareNewPlayerQueue( bool bScramble )
{
	int i = 0;
	CTFPlayer *pTFPlayer = NULL;

	for ( i = 0; i < MAX_PLAYERS; i++ )
	{
		pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

		// I really don't like how this is formatted
		if ( pTFPlayer && !pTFPlayer->IsHLTV() && !pTFPlayer->IsReplay() )
		{
			if ( GetWinningTeam() > TEAM_SPECTATOR && pTFPlayer->IsReadyToPlay() && ( pTFPlayer->GetTeamNumber() != GetWinningTeam() || bScramble ) )
			{
				AddPlayerToQueue( pTFPlayer );
			}
		}
	}

	if ( bScramble )
		m_hArenaQueue.Sort( ScramblePlayersSort );
	else
		m_hArenaQueue.Sort( SortPlayerSpectatorQueue );

	for ( i = 0; i < m_hArenaQueue.Count(); ++i )
	{
		pTFPlayer = m_hArenaQueue.Element( i );
		if ( pTFPlayer  && pTFPlayer->IsReadyToPlay() )
		{
			pTFPlayer->ChangeTeam( TEAM_SPECTATOR );
			pTFPlayer->m_bInArenaQueue = true;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::Arena_RunTeamLogic( void )
{
	if ( !TFGameRules()->IsInArenaMode() )
		return;

	bool bStreakReached = false;
	int i = 0, iHeavyCount = 0, iLightCount = 0, iPlayersNeeded = 0, iTeam = TEAM_UNASSIGNED, iActivePlayers = CountActivePlayers();
	CTFTeam *pTeam = GetGlobalTFTeam( GetWinningTeam() );

	if ( !pTeam )
		return;

	if ( tf_arena_use_queue.GetBool() )
	{
		if ( tf_arena_max_streak.GetInt() > 0 && pTeam->GetScore() >= tf_arena_max_streak.GetInt() )
		{
			bStreakReached = true;
			IGameEvent *event = gameeventmanager->CreateEvent( "arena_match_maxstreak" );
			if ( event )
			{
				event->SetInt( "team", iTeam );
				event->SetInt( "streak", tf_arena_max_streak.GetInt() );
				gameeventmanager->FireEvent( event );
			}
		}

		if ( bStreakReached && ( !IsInVSHMode() || !IsInDRMode() ) )
		{
			for ( i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
			{
				BroadcastSound( i, "Announcer.AM_TeamScrambleRandom" );
			}
		}

		if ( !IsInTournamentMode() && ( !IsInVSHMode() || !IsInDRMode() ) )
		{
			if ( iActivePlayers > 0 )
				Arena_ResetLosersScore( bStreakReached );
			else
				Arena_ResetLosersScore( true );
		}
		
		if ( IsInVSHMode() || IsInDRMode() )
			bStreakReached = true;

		if ( iActivePlayers <= 0 )
		{
			Arena_PrepareNewPlayerQueue( true );
			State_Transition( GR_STATE_PREGAME );
		}
		else
		{
			Arena_PrepareNewPlayerQueue( bStreakReached );

			iPlayersNeeded = Arena_PlayersNeededForMatch();

			if ( AreTeamsUnbalanced( iHeavyCount, iLightCount ) && iPlayersNeeded > 0 )
			{
				if ( iPlayersNeeded > m_hArenaQueue.Count() )
					iPlayersNeeded = m_hArenaQueue.Count();

				if ( pTeam )
				{
					// Everyone not on the winning team should be in spectate already
					int iUnknown = pTeam->GetNumPlayers() + iPlayersNeeded;

					iUnknown = ( iUnknown * 0.5 ) - pTeam->GetNumPlayers();
					iTeam = pTeam->GetTeamNumber();

					if ( iTeam == TEAM_UNASSIGNED )
						iTeam = TF_TEAM_AUTOASSIGN;

					for ( i = 0; i < iPlayersNeeded; ++i )
					{
						CTFPlayer *pTFPlayer = m_hArenaQueue.Element( i );
						if ( i >= iUnknown )
							iTeam = TF_TEAM_AUTOASSIGN;

						if ( pTFPlayer )
						{
							pTFPlayer->ForceChangeTeam( iTeam );
							if ( !pTFPlayer->m_bInArenaQueue )
								pTFPlayer->m_flArenaQueueTime = gpGlobals->curtime;
						}
					}
				}
			}
			m_flArenaNotificationSend = gpGlobals->curtime + 1.0f;
			Arena_CleanupPlayerQueue();
			Arena_NotifyTeamSizeChange();
		}
	}
	else if ( iActivePlayers > 0
		|| ( GetGlobalTFTeam( TF_TEAM_RED ) && GetGlobalTFTeam( TF_TEAM_RED )->GetNumPlayers() < 1 )
		|| ( GetGlobalTFTeam( TF_TEAM_BLUE ) && GetGlobalTFTeam( TF_TEAM_BLUE )->GetNumPlayers() < 1 ) )
	{
		State_Transition( GR_STATE_PREGAME );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::Arena_SendPlayerNotifications( void )
{
	CUtlVector< CTFTeam * > pTeam;
	pTeam.AddToTail( GetGlobalTFTeam( TF_TEAM_RED ) );
	pTeam.AddToTail( GetGlobalTFTeam( TF_TEAM_BLUE ) );

	if ( !pTeam[0] || !pTeam[1] )
		return;

	int iPlaying = pTeam[0]->GetNumPlayers() + pTeam[1]->GetNumPlayers(), iReady = 0, iQueued;
	CTFPlayer *pTFPlayer = NULL;

	m_flArenaNotificationSend = 0.0f;

	for ( int i = 0; i < MAX_PLAYERS; i++ )
	{
		pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

		if ( pTFPlayer && pTFPlayer->IsReadyToPlay() && !pTFPlayer->IsHLTV() && !pTFPlayer->IsReplay() )
		{
			if ( pTFPlayer->GetTeamNumber() == TEAM_SPECTATOR )
			{
				// Player is in spectator. Send a notification to the hud
				CRecipientFilter filter;
				filter.AddRecipient( pTFPlayer );
				UserMessageBegin( filter, "HudArenaNotify" );
				WRITE_BYTE( pTFPlayer->entindex() );
				WRITE_BYTE( 1 );
				MessageEnd();
			}

			iReady++;
		}
	}

	if ( iPlaying != iReady )
	{
		iQueued = iReady - iPlaying;
		for ( int i = 0; i < pTeam.Count(); i++ )
		{
			if ( pTeam.Element( i ) )
			{
				for ( int j = 0; j < pTeam[i]->GetNumPlayers(); j++ )
				{
					CBasePlayer *pPlayer = pTeam[i]->GetPlayer( j );
					pTFPlayer = ToTFPlayer( pPlayer );
					if ( pTFPlayer && pTFPlayer->IsReadyToPlay() && !pTFPlayer->IsHLTV() && !pTFPlayer->IsReplay() )
					{
						AddPlayerToQueue( pTFPlayer );
					}
				}
			}

			m_hArenaQueue.Sort( SortPlayerSpectatorQueue );

			//TODO: This is not alerting the correct players 
			// who might sit out the next match
			for ( int j = 0; j <= m_hArenaQueue.Count(); j++ )
			{
				if ( j < iQueued )
				{
					pTFPlayer = m_hArenaQueue.Element( j );

					if ( !pTFPlayer )
						continue;

					//Msg("Player Might Have to Sit Out: %s\n", pTFPlayer->GetPlayerName() );
					// This player might sit out the next game if they lose
					CRecipientFilter filter;
					filter.AddRecipient( pTFPlayer );
					UserMessageBegin( filter, "HudArenaNotify" );
					WRITE_BYTE( pTFPlayer->entindex() );
					WRITE_BYTE( 0 );
					MessageEnd();
				}
			}
		}
	}

	// Clean up the player queue
	Arena_CleanupPlayerQueue();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::AddPlayerToQueue( CTFPlayer *pPlayer )
{
	if ( !pPlayer )
		return;

	if ( !pPlayer->IsArenaSpectator() )
	{
		// Don't know if this is alright
		RemovePlayerFromQueue( pPlayer );
		m_hArenaQueue.AddToTail( pPlayer );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::AddPlayerToQueueHead( CTFPlayer *pPlayer )
{
	if ( !pPlayer )
		return;

	if ( !m_hArenaQueue.HasElement( pPlayer ) && !pPlayer->IsArenaSpectator() )
	{
		m_hArenaQueue.AddToHead( pPlayer );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::RemovePlayerFromQueue( CTFPlayer *pPlayer )
{
	if ( !pPlayer )
		return;

	m_hArenaQueue.FindAndRemove( pPlayer );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::RoundRespawn( void )
{
	// remove any buildings, grenades, rockets, etc. the player put into the world
	for ( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

		if ( pPlayer )
		{
			pPlayer->RemoveAllOwnedEntitiesFromWorld();

			pPlayer->m_Shared.SetKillstreak( 0, 0 );
			pPlayer->m_Shared.SetKillstreak( 1, 0 );
			pPlayer->m_Shared.SetKillstreak( 2, 0 );
		}
	}

	if ( !IsInTournamentMode() )
		Arena_RunTeamLogic();

	// reset the flag captures
	int nTeamCount = TFTeamMgr()->GetTeamCount();
	for ( int iTeam = FIRST_GAME_TEAM; iTeam < nTeamCount; ++iTeam )
	{
		CTFTeam *pTeam = GetGlobalTFTeam( iTeam );
		if ( !pTeam )
			continue;

		pTeam->SetFlagCaptures( 0 );
	}
	CTF_GameStats.ResetRoundStats();

	BaseClass::RoundRespawn();

	// ** AFTER WE'VE BEEN THROUGH THE ROUND RESPAWN, SHOW THE ROUNDINFO PANEL
	if ( !IsInWaitingForPlayers() )
	{
		ShowRoundInfoPanel();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::InternalHandleTeamWin( int iWinningTeam )
{
	// remove any spies' disguises and make them visible (for the losing team only)
	// and set the speed for both teams (winners get a boost and losers have reduced speed)
	for ( int i = 1; i < MAX_PLAYERS; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

		if ( pPlayer )
		{
			if ( pPlayer->GetTeamNumber() > LAST_SHARED_TEAM )
			{
				if ( pPlayer->GetTeamNumber() != iWinningTeam )
				{
					pPlayer->RemoveInvisibility();
					if ( tf2v_remove_loser_disguise.GetBool() )
						pPlayer->RemoveDisguise();

					if ( pPlayer->HasTheFlag() )
					{
						pPlayer->DropFlag();
					}

					// Hide their weapon.
					CTFWeaponBase *pWeapon = pPlayer->GetActiveTFWeapon();
					if ( pWeapon )
					{
						pWeapon->SetWeaponVisible( false );
					}
				}
				else if ( pPlayer->IsAlive() )
				{
					pPlayer->m_Shared.AddCond( TF_COND_CRITBOOSTED_BONUS_TIME );
				}

				pPlayer->TeamFortress_SetSpeed();
			}
		}
	}

	// disable any sentry guns the losing team has built
	CObjectSentrygun *pSentry = gEntList.NextEntByClass( (CObjectSentrygun *)NULL );
	while ( pSentry )
	{
		if ( pSentry->GetTeamNumber() != iWinningTeam )
		{
			pSentry->SetDisabled( true );
		}

		pSentry = gEntList.NextEntByClass( pSentry );
	}

	if ( m_bForceMapReset )
	{
		m_iPrevRoundState = -1;
		m_iCurrentRoundState = -1;
		m_iCurrentMiniRoundMask = 0;
		m_bFirstBlood = false;
	}
}

// sort function for the list of players that we're going to use to scramble the teams
int ScramblePlayersSort( CTFPlayer *const *p1, CTFPlayer *const *p2 )
{
	CTFPlayerResource *pResource = dynamic_cast<CTFPlayerResource *>( g_pPlayerResource );
	if ( pResource )
	{
		// check the priority
		if ( pResource->GetTotalScore( ( *p2 )->entindex() ) > pResource->GetTotalScore( ( *p1 )->entindex() ) )
		{
			return 1;
		}
	}

	return -1;
}

// sort function for the player spectator queue
int SortPlayerSpectatorQueue( CTFPlayer *const *p1, CTFPlayer *const *p2 )
{
	// get the queue times of both players
	float flQueueTime1 = 0.0f, flQueueTime2 = 0.0f;
	flQueueTime1 = ( *p1 )->m_flArenaQueueTime;
	flQueueTime2 = ( *p2 )->m_flArenaQueueTime;

	if ( flQueueTime1 == flQueueTime2 )
	{
		// if both players queued at the same time see who's been in the server longer
		flQueueTime1 = ( *p1 )->GetConnectionTime();
		flQueueTime2 = ( *p2 )->GetConnectionTime();
		if ( flQueueTime1 < flQueueTime2 )
			return -1;
	}
	else if ( flQueueTime1 > flQueueTime2 )
	{
		// the player with the higher queue time queued more recently (gpGlobals->curtime)
		return -1;
	}

	return flQueueTime1 != flQueueTime2;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::HandleScrambleTeams( void )
{
	int i = 0;
	CTFPlayer *pTFPlayer = NULL;
	CUtlVector<CTFPlayer *> pListPlayers;

	// add all the players (that are on blue or red) to our temp list
	for ( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( pTFPlayer && ( pTFPlayer->GetTeamNumber() >= TF_TEAM_RED ) )
		{
			pListPlayers.AddToHead( pTFPlayer );
		}
	}

	// sort the list
	pListPlayers.Sort( ScramblePlayersSort );

	// loop through and put everyone on Spectator to clear the teams (or the autoteam step won't work correctly)
	for ( i = 0; i < pListPlayers.Count(); i++ )
	{
		pTFPlayer = pListPlayers[i];

		if ( pTFPlayer )
		{
			pTFPlayer->ForceChangeTeam( TEAM_SPECTATOR );
		}
	}

	if ( IsInVSHMode() || IsInDRMode() )
	{
		// We random pick a player to be the boss character.
		int iBossPlayer = RandomInt(0, pListPlayers.Count());
		for ( i = 0; i < pListPlayers.Count(); i++ )
		{
			pTFPlayer = pListPlayers[i];

			if ( pTFPlayer && ( i == iBossPlayer ) )
				pTFPlayer->ForceChangeTeam( TF_TEAM_PLAYER_BOSS );
			else if ( pTFPlayer && ( i != iBossPlayer ) )
				pTFPlayer->ForceChangeTeam( TF_TEAM_PLAYER_HORDE );
		}
	}
	else
	{
		// loop through and auto team everyone
		for ( i = 0; i < pListPlayers.Count(); i++ )
		{
			pTFPlayer = pListPlayers[i];

			if ( pTFPlayer )
			{
				pTFPlayer->ForceChangeTeam( TF_TEAM_AUTOASSIGN );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::HandleSwitchTeams( void )
{
	// respawn the players
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( pPlayer )
		{
			pPlayer->RemoveAllOwnedEntitiesFromWorld();

			// Ignore players who aren't on an active team
			if ( pPlayer->GetTeamNumber() != TF_TEAM_RED && pPlayer->GetTeamNumber() != TF_TEAM_BLUE )
			{
				continue;
			}

			if ( pPlayer->GetTeamNumber() == TF_TEAM_RED )
			{
				pPlayer->ForceChangeTeam( TF_TEAM_BLUE );
			}
			else if ( pPlayer->GetTeamNumber() == TF_TEAM_BLUE )
			{
				pPlayer->ForceChangeTeam( TF_TEAM_RED );
			}
		}
	}

	// switch the team scores
	CTFTeam *pRedTeam = GetGlobalTFTeam( TF_TEAM_RED );
	CTFTeam *pBlueTeam = GetGlobalTFTeam( TF_TEAM_BLUE );
	if ( pRedTeam && pBlueTeam )
	{
		int nRed = pRedTeam->GetScore();
		int nBlue = pBlueTeam->GetScore();

		pRedTeam->SetScore( nBlue );
		pBlueTeam->SetScore( nRed );
	}
}

bool CTFGameRules::CanChangeClassInStalemate( void )
{
	return ( gpGlobals->curtime < ( m_flStalemateStartTime + tf_stalematechangeclasstime.GetFloat() ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::SetRoundOverlayDetails( void )
{
	CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;

	if ( pMaster && pMaster->PlayingMiniRounds() )
	{
		CTeamControlPointRound *pRound = pMaster->GetCurrentRound();

		if ( pRound )
		{
			CHandle<CTeamControlPoint> pRedPoint = pRound->GetPointOwnedBy( TF_TEAM_RED );
			CHandle<CTeamControlPoint> pBluePoint = pRound->GetPointOwnedBy( TF_TEAM_BLUE );

			// do we have opposing points in this round?
			if ( pRedPoint && pBluePoint )
			{
				int iMiniRoundMask = ( 1<<pBluePoint->GetPointIndex() ) | ( 1<<pRedPoint->GetPointIndex() );
				SetMiniRoundBitMask( iMiniRoundMask );
			}
			else
			{
				SetMiniRoundBitMask( 0 );
			}

			SetCurrentRoundStateBitString();
		}
	}

	BaseClass::SetRoundOverlayDetails();
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether a team should score for each captured point
//-----------------------------------------------------------------------------
bool CTFGameRules::ShouldScorePerRound( void )
{
	bool bRetVal = true;

	CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
	if ( pMaster && pMaster->ShouldScorePerCapture() )
	{
		bRetVal = false;
	}

	return bRetVal;
}

#endif  // GAME_DLL

void CTFGameRules::LevelShutdownPostEntity( void )
{
#if defined( CLIENT_DLL )
	RevertSavedConvars();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGameRules::GetFarthestOwnedControlPoint( int iTeam, bool bWithSpawnpoints )
{
	int iOwnedEnd = ObjectiveResource()->GetBaseControlPointForTeam( iTeam );
	if ( iOwnedEnd == -1 )
		return -1;

	int iNumControlPoints = ObjectiveResource()->GetNumControlPoints();
	int iWalk = 1;
	int iEnemyEnd = iNumControlPoints-1;
	if ( iOwnedEnd != 0 )
	{
		iWalk = -1;
		iEnemyEnd = 0;
	}

	// Walk towards the other side, and find the farthest owned point that has spawn points
	int iFarthestPoint = iOwnedEnd;
	for ( int iPoint = iOwnedEnd; iPoint != iEnemyEnd; iPoint += iWalk )
	{
		// If we've hit a point we don't own, we're done
		if ( ObjectiveResource()->GetOwningTeam( iPoint ) != iTeam )
			break;

		if ( bWithSpawnpoints && !m_bControlSpawnsPerTeam[iTeam][iPoint] )
			continue;

		iFarthestPoint = iPoint;
	}

	return iFarthestPoint;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::TeamMayCapturePoint( int iTeam, int iPointIndex )
{
	// Is point capturing allowed at all?
	if ( IsInKothMode() && IsInWaitingForPlayers() )
		return false;

	// If the point is explicitly locked it can't be capped.
	if ( ObjectiveResource()->GetCPLocked( iPointIndex ) )
		return false;

	if ( !tf_caplinear.GetBool() )
		return true;

	// Any previous points necessary?
	int iPointNeeded = ObjectiveResource()->GetPreviousPointForPoint( iPointIndex, iTeam, 0 );

	// Points set to require themselves are always cappable 
	if ( iPointNeeded == iPointIndex )
		return true;

	// No required points specified? Require all previous points.
	if ( iPointNeeded == -1 )
	{
		if ( !ObjectiveResource()->PlayingMiniRounds() )
		{
			// No custom previous point, team must own all previous points
			int iFarthestPoint = GetFarthestOwnedControlPoint( iTeam, false );
			return ( abs( iFarthestPoint - iPointIndex ) <= 1 );
		}
		else
		{
			if ( IsInArenaMode() )
				return State_Get() == GR_STATE_STALEMATE;

			return true;
		}
	}

	// Loop through each previous point and see if the team owns it
	for ( int iPrevPoint = 0; iPrevPoint < MAX_PREVIOUS_POINTS; iPrevPoint++ )
	{
		int iPointNeeded = ObjectiveResource()->GetPreviousPointForPoint( iPointIndex, iTeam, iPrevPoint );
		if ( iPointNeeded != -1 )
		{
			if ( ObjectiveResource()->GetOwningTeam( iPointNeeded ) != iTeam )
				return false;
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::PlayerMayCapturePoint( CBasePlayer *pPlayer, int iPointIndex, char *pszReason /* = NULL */, int iMaxReasonLength /* = 0 */ )
{
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );

	if ( !pTFPlayer )
	{
		return false;
	}

	// Disguised and invisible spies cannot capture points
	if ( pTFPlayer->m_Shared.InCond( TF_COND_STEALTHED ) )
	{
		if ( pszReason )
		{
			Q_snprintf( pszReason, iMaxReasonLength, "#Cant_cap_stealthed" );
		}
		return false;
	}

	if ( pTFPlayer->m_Shared.InCond( TF_COND_INVULNERABLE ) || pTFPlayer->m_Shared.InCond( TF_COND_PHASE ) || pTFPlayer->m_Shared.InCond( TF_COND_MEGAHEAL ) )
	{
		if ( pszReason )
		{
			Q_snprintf( pszReason, iMaxReasonLength, "#Cant_cap_invuln" );
		}
		return false;
	}

	if ( pTFPlayer->m_Shared.InCond( TF_COND_DISGUISED ) && pTFPlayer->m_Shared.GetDisguiseTeam() != pTFPlayer->GetTeamNumber() )
	{
		if ( pszReason )
		{
			Q_snprintf( pszReason, iMaxReasonLength, "#Cant_cap_disguised" );
		}
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::PlayerMayBlockPoint( CBasePlayer *pPlayer, int iPointIndex, char *pszReason, int iMaxReasonLength )
{
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
	if ( !pTFPlayer )
		return false;

	// Invuln players can block points
	if ( pTFPlayer->m_Shared.InCond( TF_COND_INVULNERABLE ) || pTFPlayer->m_Shared.InCond( TF_COND_MEGAHEAL ) )
	{
		if ( pszReason )
		{
			Q_snprintf( pszReason, iMaxReasonLength, "#Cant_cap_invuln" );
		}
		return true;
	}

	return false;
}

#if defined( GAME_DLL )
//-----------------------------------------------------------------------------
// Purpose: Fills a vector with valid points that the player can capture right now
// Input:	pPlayer - The player that wants to capture
//			controlPointVector - A vector to fill with results
//-----------------------------------------------------------------------------
void CTFGameRules::CollectCapturePoints( CBasePlayer *pPlayer, CUtlVector<CTeamControlPoint *> *controlPointVector )
{
	Assert( ObjectiveResource() );
	if ( !controlPointVector || !pPlayer )
		return;

	controlPointVector->RemoveAll();

	if ( g_hControlPointMasters.IsEmpty() )
		return;

	CTeamControlPointMaster *pMaster = g_hControlPointMasters[0];
	if ( !pMaster || !pMaster->IsActive() )
		return;

	if ( IsInKothMode() && pMaster->GetNumPoints() == 1 )
	{
		CTeamControlPoint *pPoint = pMaster->GetControlPoint( 0 );
		if ( pPoint && pPoint->GetPointIndex() == 0 )
			controlPointVector->AddToTail( pPoint );

		return;
	}

	for ( int i = 0; i < pMaster->GetNumPoints(); ++i )
	{
		CTeamControlPoint *pPoint = pMaster->GetControlPoint( i );
		if ( !pMaster->IsInRound( pPoint ) )
			continue;

		if ( ObjectiveResource()->GetOwningTeam( pPoint->GetPointIndex() ) == pPlayer->GetTeamNumber() )
			continue;

		if ( !ObjectiveResource()->TeamCanCapPoint( pPoint->GetPointIndex(), pPlayer->GetTeamNumber() ) )
			continue;

		if ( !TeamMayCapturePoint( pPlayer->GetTeamNumber(), pPoint->GetPointIndex() ) )
			continue;
		
		controlPointVector->AddToTail( pPoint );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Fills a vector with valid points that the player needs to defend from capture
// Input:	pPlayer - The player that wants to defend
//			controlPointVector - A vector to fill with results
//-----------------------------------------------------------------------------
void CTFGameRules::CollectDefendPoints( CBasePlayer *pPlayer, CUtlVector<CTeamControlPoint *> *controlPointVector )
{
	Assert( ObjectiveResource() );
	if ( !controlPointVector || !pPlayer )
		return;

	controlPointVector->RemoveAll();

	if ( g_hControlPointMasters.IsEmpty() )
		return;

	CTeamControlPointMaster *pMaster = g_hControlPointMasters[0];
	if ( !pMaster || !pMaster->IsActive() )
		return;

	const int iEnemyTeam = GetEnemyTeam( pPlayer );
	for ( int i = 0; i < pMaster->GetNumPoints(); ++i )
	{
		CTeamControlPoint *pPoint = pMaster->GetControlPoint( i );
		if ( !pMaster->IsInRound( pPoint ) )
			continue;

		if ( ObjectiveResource()->GetOwningTeam( pPoint->GetPointIndex() ) != pPlayer->GetTeamNumber() )
			continue;

		if ( !ObjectiveResource()->TeamCanCapPoint( pPoint->GetPointIndex(), iEnemyTeam ) )
			continue;

		if ( !TeamMayCapturePoint( iEnemyTeam, pPoint->GetPointIndex() ) )
			continue;
		
		controlPointVector->AddToTail( pPoint );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTeamTrainWatcher *CTFGameRules::GetPayloadToPush( int iTeam )
{
	if ( GetGameType() != TF_GAMETYPE_ESCORT )
		return nullptr;

	if ( iTeam == TF_TEAM_BLUE )
	{
		if ( m_hBlueAttackTrain )
			return m_hBlueAttackTrain;

		CTeamTrainWatcher *pWatcher = dynamic_cast<CTeamTrainWatcher *>( gEntList.FindEntityByClassname( NULL, "team_train_watcher" ) );
		while ( pWatcher )
		{
			if ( !pWatcher->IsDisabled() && pWatcher->GetTeamNumber() == iTeam )
			{
				m_hBlueAttackTrain = pWatcher;
				return m_hBlueAttackTrain;
			}

			pWatcher = dynamic_cast<CTeamTrainWatcher *>( gEntList.FindEntityByClassname( pWatcher, "team_train_watcher" ) );
		}
	}

	if ( iTeam == TF_TEAM_RED )
	{
		if ( m_hRedAttackTrain )
			return m_hRedAttackTrain;

		CTeamTrainWatcher *pWatcher = dynamic_cast<CTeamTrainWatcher *>( gEntList.FindEntityByClassname( NULL, "team_train_watcher" ) );
		while ( pWatcher )
		{
			if ( !pWatcher->IsDisabled() && pWatcher->GetTeamNumber() == iTeam )
			{
				m_hRedAttackTrain = pWatcher;
				return m_hRedAttackTrain;
			}

			pWatcher = dynamic_cast<CTeamTrainWatcher *>( gEntList.FindEntityByClassname( pWatcher, "team_train_watcher" ) );
		}
	}

	return nullptr;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTeamTrainWatcher *CTFGameRules::GetPayloadToBlock( int iTeam )
{
	if ( GetGameType() != TF_GAMETYPE_ESCORT )
		return nullptr;

	if ( iTeam == TF_TEAM_BLUE )
	{
		if ( m_hBlueDefendTrain )
			return m_hBlueDefendTrain;

		CTeamTrainWatcher *pWatcher = dynamic_cast<CTeamTrainWatcher *>( gEntList.FindEntityByClassname( NULL, "team_train_watcher" ) );
		while ( pWatcher )
		{
			if ( !pWatcher->IsDisabled() && pWatcher->GetTeamNumber() != iTeam )
			{
				m_hBlueDefendTrain = pWatcher;
				return m_hBlueDefendTrain;
			}

			pWatcher = dynamic_cast<CTeamTrainWatcher *>( gEntList.FindEntityByClassname( pWatcher, "team_train_watcher" ) );
		}
	}

	if ( iTeam == TF_TEAM_RED )
	{
		if ( m_hRedDefendTrain )
			return m_hRedDefendTrain;

		CTeamTrainWatcher *pWatcher = dynamic_cast<CTeamTrainWatcher *>( gEntList.FindEntityByClassname( NULL, "team_train_watcher" ) );
		while ( pWatcher )
		{
			if ( !pWatcher->IsDisabled() && pWatcher->GetTeamNumber() != iTeam )
			{
				m_hRedDefendTrain = pWatcher;
				return m_hRedDefendTrain;
			}

			pWatcher = dynamic_cast<CTeamTrainWatcher *>( gEntList.FindEntityByClassname( pWatcher, "team_train_watcher" ) );
		}
	}

	return nullptr;
}

#endif

//-----------------------------------------------------------------------------
// Purpose: Calculates score for player
//-----------------------------------------------------------------------------
int CTFGameRules::CalcPlayerScore( RoundStats_t *pRoundStats )
{
	int iScore = ( pRoundStats->m_iStat[TFSTAT_KILLS] * TF_SCORE_KILL ) +
		( pRoundStats->m_iStat[TFSTAT_CAPTURES] * TF_SCORE_CAPTURE ) +
		( pRoundStats->m_iStat[TFSTAT_DEFENSES] * TF_SCORE_DEFEND ) +
		( pRoundStats->m_iStat[TFSTAT_BUILDINGSDESTROYED] * TF_SCORE_DESTROY_BUILDING ) +
		( pRoundStats->m_iStat[TFSTAT_HEADSHOTS] / TF_SCORE_HEADSHOT_PER_POINT ) +
		( pRoundStats->m_iStat[TFSTAT_BACKSTABS] * TF_SCORE_BACKSTAB ) +
		( pRoundStats->m_iStat[TFSTAT_HEALING] / TF_SCORE_HEAL_HEALTHUNITS_PER_POINT ) +
		( pRoundStats->m_iStat[TFSTAT_KILLASSISTS] / TF_SCORE_KILL_ASSISTS_PER_POINT ) +
		( pRoundStats->m_iStat[TFSTAT_TELEPORTS] / TF_SCORE_TELEPORTS_PER_POINT ) +
		( pRoundStats->m_iStat[TFSTAT_INVULNS] / TF_SCORE_INVULN ) +
		( pRoundStats->m_iStat[TFSTAT_REVENGE] / TF_SCORE_REVENGE ) +
		( pRoundStats->m_iStat[TFSTAT_DAMAGE] / TF_SCORE_DAMAGE_PER_POINT ) +
		( pRoundStats->m_iStat[TFSTAT_BONUS_POINTS] / TF_SCORE_BONUS_PER_POINT );
	return Max( iScore, 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::IsBirthday( void ) const
{
	if ( IsX360() )
		return false;

	return tf_birthday.GetBool() || IsHolidayActive( kHoliday_TF2Birthday );
}

bool CTFGameRules::IsBirthdayOrPyroVision( void ) const
{
	if ( IsBirthday() )
		return true;

#ifdef CLIENT_DLL
	// Use birthday fun if the local player has an item that allows them to see it (Pyro Goggles)
	if ( IsLocalPlayerUsingVisionFilterFlags( TF_VISION_FILTER_PYRO ) )
	{
		return true;
	}
#endif

	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFGameRules::IsHolidayActive( int eHoliday ) const
{
	return TF_IsHolidayActive( eHoliday );
}

// We can use these to check between normal and boss behavior without writing out individual massive if statements each time.
// For regular classes.
bool CTFGameRules::IsNormalClass( CBaseEntity *pPlayer )
{
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
	if( pTFPlayer == nullptr )
		return false;
	
	if ( pTFPlayer->GetPlayerClass()->GetClassIndex() >= TF_FIRST_NORMAL_CLASS && pTFPlayer->GetPlayerClass()->GetClassIndex() <= TF_LAST_NORMAL_CLASS )
		return true;
	
	return false;
}
// For boss characters.
bool CTFGameRules::IsBossClass( CBaseEntity *pPlayer )
{
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
	if( pTFPlayer == nullptr )
		return false;
	
	if ( pTFPlayer->GetPlayerClass()->GetClassIndex() >= TF_FIRST_BOSS_CLASS && pTFPlayer->GetPlayerClass()->GetClassIndex() <= TF_LAST_BOSS_CLASS )
		return true;
	
	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGameRules::SetIT( CBaseEntity *pEnt )
{
	CBasePlayer *pPlayer = ToBasePlayer( pEnt );
	if ( pPlayer )
	{
		if ( !m_itHandle.Get() || pPlayer != m_itHandle.Get() )
		{
			ClientPrint( pPlayer, HUD_PRINTTALK, "#TF_HALLOWEEN_BOSS_WARN_VICTIM", pPlayer->GetPlayerName() );
			ClientPrint( pPlayer, HUD_PRINTCENTER, "#TF_HALLOWEEN_BOSS_WARN_VICTIM", pPlayer->GetPlayerName() );

			CSingleUserRecipientFilter filter( pPlayer );
			CBaseEntity::EmitSound( filter, pEnt->entindex(), "Player.YouAreIT" );
			pEnt->EmitSound( "Halloween.PlayerScream" );
		}
	}

	CBasePlayer *pIT = ToBasePlayer( m_itHandle.Get() );
	if ( pIT && pEnt != pIT )
	{
		if ( pIT->IsAlive() )
		{
			CSingleUserRecipientFilter filter( pIT );
			CBaseEntity::EmitSound( filter, pIT->entindex(), "Player.TaggedOtherIT" );
			ClientPrint( pIT, HUD_PRINTTALK, "#TF_HALLOWEEN_BOSS_LOST_AGGRO" );
			ClientPrint( pIT, HUD_PRINTCENTER, "#TF_HALLOWEEN_BOSS_LOST_AGGRO" );
		}
	}

	m_itHandle = pEnt;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::AllowThirdPersonCamera( void )
{
#ifdef CLIENT_DLL
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pPlayer )
	{
		if ( pPlayer->IsObserver() )
			return false;

		if ( pPlayer->m_Shared.InCond( TF_COND_ZOOMED ) )
			return false;
	}
#endif

	return tf2v_allow_thirdperson.GetBool();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::AllowGlowOutlinesFlags( void )
{
	return tf2v_allow_objective_glow_ctf.GetBool();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::AllowGlowOutlinesCarts( void )
{
	return tf2v_allow_objective_glow_pl.GetBool();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::ShouldCollide( int collisionGroup0, int collisionGroup1 )
{
	if ( collisionGroup0 > collisionGroup1 )
	{
		// swap so that lowest is always first
		V_swap( collisionGroup0, collisionGroup1 );
	}

	//Don't stand on COLLISION_GROUP_WEAPONs
	if ( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT &&
		collisionGroup1 == COLLISION_GROUP_WEAPON )
	{
		return false;
	}

	// Don't stand on projectiles
	if ( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT &&
		collisionGroup1 == COLLISION_GROUP_PROJECTILE )
	{
		return false;
	}

	// Rockets need to collide with players when they hit, but
	// be ignored by player movement checks
	if ( ( collisionGroup0 == COLLISION_GROUP_PLAYER ) &&
		( collisionGroup1 == TFCOLLISION_GROUP_ROCKETS ) )
		return true;

	if ( ( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT ) &&
		( collisionGroup1 == TFCOLLISION_GROUP_ROCKETS ) )
		return false;

	if ( ( collisionGroup0 == COLLISION_GROUP_WEAPON ) &&
		( collisionGroup1 == TFCOLLISION_GROUP_ROCKETS ) )
		return false;

	if ( ( collisionGroup0 == TFCOLLISION_GROUP_GRENADES ) &&
		( collisionGroup1 == TFCOLLISION_GROUP_ROCKETS ) )
		return false;

	// Grenades don't collide with players. They handle collision while flying around manually.
	if ( ( collisionGroup0 == COLLISION_GROUP_PLAYER ) &&
		( collisionGroup1 == TFCOLLISION_GROUP_GRENADES ) )
		return false;

	if ( ( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT ) &&
		( collisionGroup1 == TFCOLLISION_GROUP_GRENADES ) )
		return false;

	// Respawn rooms only collide with players
	if ( collisionGroup1 == TFCOLLISION_GROUP_RESPAWNROOMS )
		return ( collisionGroup0 == COLLISION_GROUP_PLAYER ) || ( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT );

	// Arrows only collide with players
	if ( collisionGroup1 == TFCOLLISION_GROUP_ARROWS )
		return ( collisionGroup0 == COLLISION_GROUP_PLAYER ) || ( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT );

	// Collide with nothing
	if ( collisionGroup1 == TFCOLLISION_GROUP_NONE )
		return false;

/*	if ( collisionGroup0 == COLLISION_GROUP_PLAYER )
	{
		// Players don't collide with objects or other players
		if ( collisionGroup1 == COLLISION_GROUP_PLAYER  )
			 return false;
	}

	if ( collisionGroup1 == COLLISION_GROUP_PLAYER_MOVEMENT )
	{
		// This is only for probing, so it better not be on both sides!!!
		Assert( collisionGroup0 != COLLISION_GROUP_PLAYER_MOVEMENT );

		// No collide with players any more
		// Nor with objects or grenades
		switch ( collisionGroup0 )
		{
		default:
			break;
		case COLLISION_GROUP_PLAYER:
			return false;
		}
	}
*/
	// don't want caltrops and other grenades colliding with each other
	// caltops getting stuck on other caltrops, etc.)
	if ( ( collisionGroup0 == TFCOLLISION_GROUP_GRENADES ) &&
		( collisionGroup1 == TFCOLLISION_GROUP_GRENADES ) )
	{
		return false;
	}


	if ( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT &&
		collisionGroup1 == TFCOLLISION_GROUP_COMBATOBJECT )
	{
		return false;
	}

	if ( collisionGroup0 == COLLISION_GROUP_PLAYER &&
		collisionGroup1 == TFCOLLISION_GROUP_COMBATOBJECT )
	{
		return false;
	}

	// tf_pumpkin_bomb

	if ( ( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT ) &&
		( collisionGroup1 == TFCOLLISION_GROUP_PUMPKIN_BOMB ) )
		return false;

	if ( ( collisionGroup0 == COLLISION_GROUP_PLAYER ) &&
		( collisionGroup1 == TFCOLLISION_GROUP_PUMPKIN_BOMB ) )
		return true;

	if ( ( collisionGroup0 == TFCOLLISION_GROUP_GRENADES ) &&
		( collisionGroup1 == TFCOLLISION_GROUP_PUMPKIN_BOMB ) )
		return false;

	if ( ( collisionGroup1 == TFCOLLISION_GROUP_PUMPKIN_BOMB ) &&
		( ( collisionGroup0 == TFCOLLISION_GROUP_PUMPKIN_BOMB ) || ( collisionGroup0 == TFCOLLISION_GROUP_ROCKETS ) ) )
		return false;

	if ( ( collisionGroup1 == TFCOLLISION_GROUP_PUMPKIN_BOMB ) &&
		( ( collisionGroup0 == COLLISION_GROUP_WEAPON ) || ( collisionGroup0 == COLLISION_GROUP_PROJECTILE ) ) )
		return false;

	return BaseClass::ShouldCollide( collisionGroup0, collisionGroup1 );
}

//-----------------------------------------------------------------------------
// Purpose: Return the value of this player towards capturing a point
//-----------------------------------------------------------------------------
int	CTFGameRules::GetCaptureValueForPlayer( CBasePlayer *pPlayer )
{
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
	if ( !pTFPlayer )
		return BaseClass::GetCaptureValueForPlayer( pPlayer );

	int iCapRate = 1;
	if ( pTFPlayer->IsPlayerClass( TF_CLASS_SCOUT ) )
	{
		if ( mp_capstyle.GetInt() == 1 )
		{
			// Scouts count for 2 people in timebased capping.
			iCapRate = 2;
		}
		else
		{
			// Scouts can cap all points on their own.
			iCapRate = 10;
		}
	}

	CALL_ATTRIB_HOOK_INT_ON_OTHER( pTFPlayer, iCapRate, add_player_capturevalue );

	return iCapRate;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGameRules::GetTimeLeft( void )
{
	float flTimeLimit = mp_timelimit.GetInt() * 60;

	Assert( flTimeLimit > 0 && "Should not call this function when !IsGameUnderTimeLimit" );

	float flMapChangeTime = m_flMapResetTime + flTimeLimit;

	return ( (int)( flMapChangeTime - gpGlobals->curtime ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::FireGameEvent( IGameEvent *event )
{
	const char *eventName = event->GetName();

#ifdef GAME_DLL
	if ( !Q_strcmp( eventName, "teamplay_point_captured" ) )
	{
		RecalculateControlPointState();

		// keep track of how many times each team caps
		int iTeam = event->GetInt( "team" );
		Assert( iTeam >= FIRST_GAME_TEAM && iTeam < TF_TEAM_COUNT );
		m_iNumCaps[iTeam]++;

		// award a capture to all capping players
		const char *cappers = event->GetString( "cappers" );

		Q_strncpy( m_szMostRecentCappers, cappers, ARRAYSIZE( m_szMostRecentCappers ) );
		for ( int i =0; i < Q_strlen( cappers ); i++ )
		{
			int iPlayerIndex = (int)cappers[i];
			CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( iPlayerIndex ) );
			if ( pPlayer )
			{
				CTF_GameStats.Event_PlayerCapturedPoint( pPlayer );
			}
		}

		if ( !IsHalloweenScenario( HALLOWEEN_SCENARIO_LAKESIDE ) )
			BeginHaunting( 5, 25.0, 35.0 );
	}
	else if ( !Q_strcmp( eventName, "teamplay_capture_blocked" ) )
	{
		int iPlayerIndex = event->GetInt( "blocker" );
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( iPlayerIndex ) );
		CTF_GameStats.Event_PlayerDefendedPoint( pPlayer );
	}
	else if ( !Q_strcmp( eventName, "teamplay_round_win" ) )
	{
		int iWinningTeam = event->GetInt( "team" );
		bool bFullRound = event->GetBool( "full_round" );
		float flRoundTime = event->GetFloat( "round_time" );
		bool bWasSuddenDeath = event->GetBool( "was_sudden_death" );
		CTF_GameStats.Event_RoundEnd( iWinningTeam, bFullRound, flRoundTime, bWasSuddenDeath );
	}
	else if ( !Q_strcmp( eventName, "teamplay_flag_event" ) )
	{
		// if this is a capture event, remember the player who made the capture		
		int iEventType = event->GetInt( "eventtype" );
		if ( TF_FLAGEVENT_CAPTURE == iEventType )
		{
			int iPlayerIndex = event->GetInt( "player" );
			m_szMostRecentCappers[0] = iPlayerIndex;
			m_szMostRecentCappers[1] = 0;
		}
	}
	else if ( !Q_strcmp( eventName, "teamplay_point_unlocked" ) )
	{
		// if this is an unlock event and we're in arena, fire OnCapEnabled
		if ( m_hArenaLogic.IsValid() )
		{
			m_hArenaLogic->OnCapEnabled();
		}
	}
#else
	if ( !Q_strcmp( eventName, "recalculate_holidays" ) )
	{
		UTIL_CalculateHolidays();
	}
#endif

	BaseClass::FireGameEvent( event );
}

//-----------------------------------------------------------------------------
// Purpose: Init ammo definitions
//-----------------------------------------------------------------------------

// shared ammo definition
// JAY: Trying to make a more physical bullet response
#define BULLET_MASS_GRAINS_TO_LB(grains)	(0.002285*(grains)/16.0f)
#define BULLET_MASS_GRAINS_TO_KG(grains)	lbs2kg(BULLET_MASS_GRAINS_TO_LB(grains))

// exaggerate all of the forces, but use real numbers to keep them consistent
#define BULLET_IMPULSE_EXAGGERATION			1	

// convert a velocity in ft/sec and a mass in grains to an impulse in kg in/s
#define BULLET_IMPULSE(grains, ftpersec)	((ftpersec)*12*BULLET_MASS_GRAINS_TO_KG(grains)*BULLET_IMPULSE_EXAGGERATION)


CAmmoDef* GetAmmoDef()
{
	static CAmmoDef def;
	static bool bInitted = false;

	if ( !bInitted )
	{
		bInitted = true;

		// Start at 1 here and skip the dummy ammo type to make CAmmoDef use the same indices
		// as our #defines.
		for ( int i=1; i < TF_AMMO_COUNT; i++ )
		{
			def.AddAmmoType( g_aAmmoNames[i], DMG_BULLET, TRACER_LINE, 0, 0, "ammo_max", 2400, 10, 14 );
			Assert( def.Index( g_aAmmoNames[i] ) == i );
		}
	}

	return &def;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFGameRules::GetTeamGoalString( int iTeam )
{
	if ( iTeam == TF_TEAM_RED )
		return m_pszTeamGoalStringRed.Get();
	if ( iTeam == TF_TEAM_BLUE )
		return m_pszTeamGoalStringBlue.Get();
	if (iTeam == TF_TEAM_GREEN)
		return m_pszTeamGoalStringGreen.Get();
	if (iTeam == TF_TEAM_YELLOW)
		return m_pszTeamGoalStringYellow.Get();

	return NULL;
}

int CTFGameRules::GetAssignedHumanTeam( void ) const
{
#ifdef GAME_DLL
	if ( FStrEq( mp_humans_must_join_team.GetString(), "blue" ) )
		return TF_TEAM_BLUE;
	else if ( FStrEq( mp_humans_must_join_team.GetString(), "red" ) )
		return TF_TEAM_RED;
	else if ( FStrEq( mp_humans_must_join_team.GetString(), "any" ) )
		return TEAM_ANY;
#endif
	return TEAM_ANY;
}

#ifdef GAME_DLL

Vector MaybeDropToGround(
	CBaseEntity *pMainEnt,
	bool bDropToGround,
	const Vector &vPos,
	const Vector &vMins,
	const Vector &vMaxs )
{
	if ( bDropToGround )
	{
		trace_t trace;
		UTIL_TraceHull( vPos, vPos + Vector( 0, 0, -500 ), vMins, vMaxs, MASK_SOLID, pMainEnt, COLLISION_GROUP_NONE, &trace );
		return trace.endpos;
	}
	else
	{
		return vPos;
	}
}

//-----------------------------------------------------------------------------
// Purpose: This function can be used to find a valid placement location for an entity.
//			Given an origin to start looking from and a minimum radius to place the entity at,
//			it will sweep out a circle around vOrigin and try to find a valid spot (on the ground)
//			where mins and maxs will fit.
// Input  : *pMainEnt - Entity to place
//			&vOrigin - Point to search around
//			fRadius - Radius to search within
//			nTries - Number of tries to attempt
//			&mins - mins of the Entity
//			&maxs - maxs of the Entity
//			&outPos - Return point
// Output : Returns true and fills in outPos if it found a spot.
//-----------------------------------------------------------------------------
bool EntityPlacementTest( CBaseEntity *pMainEnt, const Vector &vOrigin, Vector &outPos, bool bDropToGround )
{
	// This function moves the box out in each dimension in each step trying to find empty space like this:
	//
	//											  X  
	//							   X			  X  
	// Step 1:   X     Step 2:    XXX   Step 3: XXXXX
	//							   X 			  X  
	//											  X  
	//

	Vector mins, maxs;
	pMainEnt->CollisionProp()->WorldSpaceAABB( &mins, &maxs );
	mins -= pMainEnt->GetAbsOrigin();
	maxs -= pMainEnt->GetAbsOrigin();

	// Put some padding on their bbox.
	float flPadSize = 5;
	Vector vTestMins = mins - Vector( flPadSize, flPadSize, flPadSize );
	Vector vTestMaxs = maxs + Vector( flPadSize, flPadSize, flPadSize );

	// First test the starting origin.
	if ( UTIL_IsSpaceEmpty( pMainEnt, vOrigin + vTestMins, vOrigin + vTestMaxs ) )
	{
		outPos = MaybeDropToGround( pMainEnt, bDropToGround, vOrigin, vTestMins, vTestMaxs );
		return true;
	}

	Vector vDims = vTestMaxs - vTestMins;


	// Keep branching out until we get too far.
	int iCurIteration = 0;
	int nMaxIterations = 15;

	int offset = 0;
	do
	{
		for ( int iDim=0; iDim < 3; iDim++ )
		{
			float flCurOffset = offset * vDims[iDim];

			for ( int iSign=0; iSign < 2; iSign++ )
			{
				Vector vBase = vOrigin;
				vBase[iDim] += ( iSign*2-1 ) * flCurOffset;

				if ( UTIL_IsSpaceEmpty( pMainEnt, vBase + vTestMins, vBase + vTestMaxs ) )
				{
					// Ensure that there is a clear line of sight from the spawnpoint entity to the actual spawn point.
					// (Useful for keeping things from spawning behind walls near a spawn point)
					trace_t tr;
					UTIL_TraceLine( vOrigin, vBase, MASK_SOLID, pMainEnt, COLLISION_GROUP_NONE, &tr );

					if ( tr.fraction != 1.0 )
					{
						continue;
					}

					outPos = MaybeDropToGround( pMainEnt, bDropToGround, vBase, vTestMins, vTestMaxs );
					return true;
				}
			}
		}

		++offset;
	} while ( iCurIteration++ < nMaxIterations );

	//	Warning( "EntityPlacementTest for ent %d:%s failed!\n", pMainEnt->entindex(), pMainEnt->GetClassname() );
	return false;
}

#else // GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );
}

void CTFGameRules::HandleOvertimeBegin()
{
	C_TFPlayer *pTFPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pTFPlayer )
	{
		pTFPlayer->EmitSound( "Game.Overtime" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::ShouldShowTeamGoal( void )
{
	if ( State_Get() == GR_STATE_PREROUND || State_Get() == GR_STATE_RND_RUNNING || InSetup() )
		return true;

	return false;
}

void CTFGameRules::GetTeamGlowColor( int nTeam, float &r, float &g, float &b )
{
	switch ( nTeam )
	{
		case TF_TEAM_BLUE:
			r = 0.49f; g = 0.66f; b = 0.7699971f;
			break;

		case TF_TEAM_RED:
			r = 0.74f; g = 0.23f; b = 0.23f;
			break;

		case TF_TEAM_GREEN:
			r = 0.03f; g = 0.68f; b = 0;
			break;

		case TF_TEAM_YELLOW:
			r = 1.0f; g = 0.62f; b = 0;
			break;
			
		default:
			r = 0.76f; g = 0.76f; b = 0.76f;
			break;
	}
}


#endif

#ifdef GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::ShutdownCustomResponseRulesDicts()
{
	DestroyCustomResponseSystems();

	if ( m_ResponseRules.Count() != 0 )
	{
		int nRuleCount = m_ResponseRules.Count();
		for ( int iRule = 0; iRule < nRuleCount; ++iRule )
		{
			m_ResponseRules[iRule].m_ResponseSystems.Purge();
		}
		m_ResponseRules.Purge();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::InitCustomResponseRulesDicts()
{
	MEM_ALLOC_CREDIT();

	// Clear if necessary.
	ShutdownCustomResponseRulesDicts();

	// Initialize the response rules for TF.
	m_ResponseRules.AddMultipleToTail( TF_CLASS_COUNT_ALL );

	char szName[512];
	for ( int iClass = TF_FIRST_NORMAL_CLASS; iClass < TF_CLASS_COUNT_ALL; ++iClass )
	{
		m_ResponseRules[iClass].m_ResponseSystems.AddMultipleToTail( MP_TF_CONCEPT_COUNT );

		for ( int iConcept = 0; iConcept < MP_TF_CONCEPT_COUNT; ++iConcept )
		{
			AI_CriteriaSet criteriaSet;
			criteriaSet.AppendCriteria( "playerclass", g_aPlayerClassNames_NonLocalized[iClass] );
			criteriaSet.AppendCriteria( "Concept", g_pszMPConcepts[iConcept] );

			// 1 point for player class and 1 point for concept.
			float flCriteriaScore = 2.0f;

			// Name.
			V_snprintf( szName, sizeof( szName ), "%s_%s\n", g_aPlayerClassNames_NonLocalized[iClass], g_pszMPConcepts[iConcept] );
			m_ResponseRules[iClass].m_ResponseSystems[iConcept] = BuildCustomResponseSystemGivenCriteria( "scripts/talker/response_rules.txt", szName, criteriaSet, flCriteriaScore );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::SendHudNotification( IRecipientFilter &filter, HudNotification_t iType )
{
	UserMessageBegin( filter, "HudNotify" );
	WRITE_BYTE( iType );
	MessageEnd();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::SendHudNotification( IRecipientFilter &filter, const char *pszText, const char *pszIcon, int iTeam /*= TEAM_UNASSIGNED*/ )
{
	UserMessageBegin( filter, "HudNotifyCustom" );
	WRITE_STRING( pszText );
	WRITE_STRING( pszIcon );
	WRITE_BYTE( iTeam );
	MessageEnd();
}

//-----------------------------------------------------------------------------
// Purpose: Is the player past the required delays for spawning
//-----------------------------------------------------------------------------
bool CTFGameRules::HasPassedMinRespawnTime( CBasePlayer *pPlayer )
{
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );

	if ( pTFPlayer && pTFPlayer->GetPlayerClass()->GetClassIndex() == TF_CLASS_UNDEFINED )
		return true;

	float flMinSpawnTime = GetMinTimeWhenPlayerMaySpawn( pPlayer );

	return ( gpGlobals->curtime > flMinSpawnTime );
}

//-----------------------------------------------------------------------------
// Purpose: Sets the game description in the server browser
//-----------------------------------------------------------------------------
const char *CTFGameRules::GetGameDescription( void )
{
	switch ( m_nGameType )
	{
		case TF_GAMETYPE_CTF:
			return "TF2V (CTF)";
			break;
		case TF_GAMETYPE_CP:
			if ( IsInKothMode() )
				return "TF2V (KotH)";

			return "TF2V (CP)";
			break;
		case TF_GAMETYPE_ESCORT:
			return "TF2V (Payload)";
			break;
		case TF_GAMETYPE_ARENA:
			if ( IsInVSHMode() )
				return "TF2V (VSH)";
			else if ( IsInDRMode() )
				return "TF2V (DR)";
			
			return "TF2V (Arena)";
			break;
		case TF_GAMETYPE_MVM:
			return "TF2V (MvM)";
			break;
		case TF_GAMETYPE_RD:
			return "TF2V (RD)";
			break;
		case TF_GAMETYPE_PASSTIME:
			return "TF2V (PASS)";
			break;
		case TF_GAMETYPE_PD:
			return "TF2V (PD)";
			break;	
		case TF_GAMETYPE_MEDIEVAL:
			return "TF2V (Medieval)";
			break;
		default:
			return "TF2V";
			break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::PlayerSpawn( CBasePlayer *pPlayer )
{
	BaseClass::PlayerSpawn( pPlayer );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::PushAllPlayersAway( Vector const &vecPos, float flRange, float flForce, int iTeamNum, CUtlVector<CTFPlayer *> *outVector )
{
	CUtlVector<CTFPlayer *> players;
	CollectPlayers( &players, iTeamNum, true );

	FOR_EACH_VEC( players, i )
	{
		Vector vecTo = players[i]->EyePosition() - vecPos;
		if ( vecTo.LengthSqr() > Square( flRange ) )
			continue;

		vecTo.NormalizeInPlace();

		players[i]->ApplyAbsVelocityImpulse( vecTo * flForce );

		if ( outVector )
			outVector->AddToTail( players[i] );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGameRules::CountActivePlayers( void )
{
	int iActivePlayers = 0;

	if ( IsInArenaMode() )
	{
		// Keep adding ready players as long as they're not HLTV or a replay
		for ( int i = 1; i < MAX_PLAYERS; i++ )
		{
			CTFPlayer *pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
			if ( pTFPlayer && pTFPlayer->IsReadyToPlay() && !pTFPlayer->IsHLTV() && !pTFPlayer->IsReplay() )
			{
				iActivePlayers++;
			}
		}


		if ( m_hArenaQueue.Count() > 1 )
			return iActivePlayers;

		if ( iActivePlayers <= 1 || ( GetGlobalTFTeam( TF_TEAM_BLUE ) && GetGlobalTFTeam( TF_TEAM_BLUE )->GetNumPlayers() <= 0 ) || ( !GetGlobalTFTeam( TF_TEAM_RED ) && GetGlobalTFTeam( TF_TEAM_RED )->GetNumPlayers() <= 0 ) )
			return 0;
	}

	return BaseClass::CountActivePlayers();
}
#endif

float CTFGameRules::GetRespawnWaveMaxLength( int iTeam, bool bScaleWithNumPlayers /* = true */ )
{
	return BaseClass::GetRespawnWaveMaxLength( iTeam, bScaleWithNumPlayers );
}

bool CTFGameRules::ShouldBalanceTeams( void )
{
	return BaseClass::ShouldBalanceTeams();
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::AllowMapParticleEffect( const char *pszParticleEffect )
{
	static const char *s_WeatherEffects[] =
	{
		"tf_gamerules",
		"env_rain_001",
		"env_rain_002_256",
		"env_rain_ripples",
		"env_snow_light_001",
		"env_rain_gutterdrip",
		"env_rain_guttersplash",
		"", // END Marker
	};

	if ( !AllowWeatherParticles() )
	{
		if ( FindInList( s_WeatherEffects, pszParticleEffect ) )
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::AllowWeatherParticles()
{
	return tf_particles_disable_weather.GetBool() ? false : true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::AllowMapVisionFilterShaders( void )
{
	if( m_pVisionFilterWhitelist )
	{
		const char *szMapName = engine->GetLevelName();

		while ( szMapName )
		{
			char c = *szMapName++;
			if ( c == '/' || c == '\\' )
				return m_pVisionFilterWhitelist->GetInt( szMapName ) != 0;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
char const *CTFGameRules::TranslateEffectForVisionFilter( char const *pchEffectType, char const *pchEffectName )
{
	if ( pchEffectType == NULL || pchEffectName == NULL )
		return pchEffectName;

	// Handle duplicate entries by just using the last one encountered
	CUtlVector<char const *> strings;
	strings.AddToTail( pchEffectName );

	int iFlags = GetLocalPlayerVisionFilterFlags( FStrEq( pchEffectType, "weapons" ) );

	KeyValues *pKV = m_pVisionFilterTranslations->FindKey( pchEffectType );
	if ( pKV )
	{
		for ( KeyValues *pSubkey = pKV->GetFirstTrueSubKey(); pSubkey; pSubkey = pSubkey->GetNextTrueSubKey() )
		{
			int iFilterType = atoi( pSubkey->GetName() );
			if( ( iFilterType & iFlags ) != 0 || iFilterType == 0 )
			{
				const char *pTranslation = pSubkey->GetString( pchEffectName );
				if ( pTranslation[ 0 ] != '\0' )
					strings.AddToHead( pTranslation );
			}
		}
	}

	return strings[0];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::SetUpVisionFilterKeyValues( void )
{
	m_pVisionFilterWhitelist = new KeyValues( "VisionFilterShadersMapWhitelist" );
	m_pVisionFilterWhitelist->LoadFromFile( g_pFullFileSystem, "cfg/mtp.cfg", "MOD" );

	m_pVisionFilterTranslations = new KeyValues( "VisionFilterTranslations" );

	KeyValues *pParticles = new KeyValues( "particles" );
	m_pVisionFilterTranslations->AddSubKey( pParticles );

	{
		KeyValues *pNoFilterTrans = new KeyValues( "0" );
		pNoFilterTrans->SetString( "flamethrower_rainbow", "flamethrower" );
		pNoFilterTrans->SetString( "flamethrower_rainbow_FP", "flamethrower" );
		pNoFilterTrans->SetString( "flamethrower_rainbow_new_flame", "new_flame" );
		pParticles->AddSubKey( pNoFilterTrans );

		KeyValues *pFilteredTrans = new KeyValues( "1" );
		pFilteredTrans->SetString("flamethrower", "flamethrower_rainbow");
		pFilteredTrans->SetString("new_flame", "flamethrower_rainbow_new_flame");
		pFilteredTrans->SetString("projectile_fireball", "projectile_fireball_pyrovision");
		pFilteredTrans->SetString("taunt_pyro_gasblast_fireblast", "taunt_pyro_gasblast_rainbow");
		pFilteredTrans->SetString("flamethrower_rainbow_new_flame", "flamethrower_rainbow_new_flame");
		pFilteredTrans->SetString("flamethrower_rainbow", "flamethrower_rainbow");
		pFilteredTrans->SetString("flamethrower_rainbow_FP", "flamethrower_rainbow_FP");
		pFilteredTrans->SetString("burningplayer_blue", "burningplayer_rainbow_blue");
		pFilteredTrans->SetString("burningplayer_red", "burningplayer_rainbow_red");
		pFilteredTrans->SetString("burningplayer_corpse", "burningplayer_corpse_rainbow");
		pFilteredTrans->SetString("water_blood_impact_red_01", "pyrovision_blood");
		pFilteredTrans->SetString("blood_impact_red_01", "pyrovision_blood");
		pFilteredTrans->SetString("blood_spray_red_01", "pyrovision_blood");
		pFilteredTrans->SetString("blood_spray_red_01_far", "pyrovision_blood");
		pFilteredTrans->SetString("ExplosionCore_wall", "pyrovision_explosion");
		pFilteredTrans->SetString("ExplosionCore_buildings", "pyrovision_explosion");
		pFilteredTrans->SetString("ExplosionCore_MidAir", "pyrovision_explosion");
		pFilteredTrans->SetString("rockettrail", "pyrovision_rockettrail");
		pFilteredTrans->SetString("rockettrail_!", "pyrovision_rockettrail");
		pFilteredTrans->SetString("sentry_rocket", "pyrovision_rockettrail");
		pFilteredTrans->SetString("flaregun_trail_blue", "pyrovision_flaregun_trail_blue");
		pFilteredTrans->SetString("flaregun_trail_red", "pyrovision_flaregun_trail_red");
		pFilteredTrans->SetString("flaregun_trail_crit_blue", "pyrovision_flaregun_trail_crit_blue");
		pFilteredTrans->SetString("flaregun_trail_crit_red", "pyrovision_flaregun_trail_crit_red");
		pFilteredTrans->SetString("flaregun_destroyed", "pyrovision_flaregun_destroyed");
		pFilteredTrans->SetString("scorchshot_trail_blue", "pyrovision_scorchshot_trail_blue");
		pFilteredTrans->SetString("scorchshot_trail_red", "pyrovision_scorchshot_trail_red");
		pFilteredTrans->SetString("scorchshot_trail_crit_blue", "pyrovision_scorchshot_trail_crit_blue");
		pFilteredTrans->SetString("scorchshot_trail_crit_red", "pyrovision_scorchshot_trail_crit_red");
		pFilteredTrans->SetString("ExplosionCore_MidAir_Flare", "pyrovision_flaregun_destroyed");
		pFilteredTrans->SetString("pyrotaunt_rainbow_norainbow", "pyrotaunt_rainbow");
		pFilteredTrans->SetString("pyrotaunt_rainbow_bubbles_flame", "pyrotaunt_rainbow_bubbles");
		pFilteredTrans->SetString("v_flaming_arrow", "pyrovision_v_flaming_arrow");
		pFilteredTrans->SetString("flaming_arrow", "pyrovision_flaming_arrow");
		pFilteredTrans->SetString("flying_flaming_arrow", "pyrovision_flying_flaming_arrow");
		pFilteredTrans->SetString("taunt_pyro_balloon", "taunt_pyro_balloon_vision");
		pFilteredTrans->SetString("taunt_pyro_balloon_explosion", "taunt_pyro_balloon_explosion_vision");
		pParticles->AddSubKey( pFilteredTrans );
	}

	KeyValues *pSounds = new KeyValues( "sounds" );
	m_pVisionFilterTranslations->AddSubKey( pSounds );

	{
		KeyValues *pNoFilterTrans = new KeyValues( "0" );
		pNoFilterTrans->SetString("weapons/rainblower/rainblower_start.wav", "weapons/flame_thrower_start.wav");
		pNoFilterTrans->SetString("weapons/rainblower/rainblower_loop.wav", "weapons/flame_thrower_loop.wav");
		pNoFilterTrans->SetString("weapons/rainblower/rainblower_end.wav", "weapons/flame_thrower_end.wav");
		pNoFilterTrans->SetString("weapons/rainblower/rainblower_hit.wav", "weapons/flame_thrower_fire_hit.wav");
		pNoFilterTrans->SetString("weapons/rainblower/rainblower_pilot.wav", "weapons/flame_thrower_pilot.wav");
		pNoFilterTrans->SetString(")weapons/rainblower/rainblower_start.wav", ")weapons/flame_thrower_start.wav");
		pNoFilterTrans->SetString(")weapons/rainblower/rainblower_loop.wav", ")weapons/flame_thrower_loop.wav");
		pNoFilterTrans->SetString(")weapons/rainblower/rainblower_end.wav", ")weapons/flame_thrower_end.wav");
		pNoFilterTrans->SetString(")weapons/rainblower/rainblower_hit.wav", ")weapons/flame_thrower_fire_hit.wav");
		pNoFilterTrans->SetString(")weapons/rainblower/rainblower_pilot.wav", "weapons/flame_thrower_pilot.wav");
		pNoFilterTrans->SetString("Weapon_Rainblower.Fire", "Weapon_FlameThrower.Fire");
		pNoFilterTrans->SetString("Weapon_Rainblower.FireLoop", "Weapon_FlameThrower.FireLoop");
		pNoFilterTrans->SetString("Weapon_Rainblower.WindDown", "Weapon_FlameThrower.WindDown");
		pNoFilterTrans->SetString("Weapon_Rainblower.FireHit", "Weapon_FlameThrower.FireHit");
		pNoFilterTrans->SetString("Weapon_Rainblower.PilotLoop", "Weapon_FlameThrower.PilotLoop");
		pNoFilterTrans->SetString("Taunt.PyroBalloonicorn", "Taunt.PyroHellicorn");
		pNoFilterTrans->SetString(")items/pyro_music_tube.wav", "common/null.wav");
		pSounds->AddSubKey( pNoFilterTrans );

		KeyValues *pFilteredTrans = new KeyValues( "1" );
		pFilteredTrans->SetString("weapons/rainblower/rainblower_start.wav", "weapons/rainblower/rainblower_start.wav");
		pFilteredTrans->SetString("weapons/rainblower/rainblower_loop.wav", "weapons/rainblower/rainblower_loop.wav");
		pFilteredTrans->SetString("weapons/rainblower/rainblower_end.wav", "weapons/rainblower/rainblower_end.wav");
		pFilteredTrans->SetString("weapons/rainblower/rainblower_hit.wav", "weapons/rainblower/rainblower_hit.wav");
		pFilteredTrans->SetString("weapons/rainblower/rainblower_pilot.wav", "weapons/rainblower/rainblower_pilot.wav");
		pFilteredTrans->SetString(")weapons/rainblower/rainblower_start.wav", ")weapons/rainblower/rainblower_start.wav");
		pFilteredTrans->SetString(")weapons/rainblower/rainblower_loop.wav", ")weapons/rainblower/rainblower_loop.wav");
		pFilteredTrans->SetString(")weapons/rainblower/rainblower_end.wav", ")weapons/rainblower/rainblower_end.wav");
		pFilteredTrans->SetString(")weapons/rainblower/rainblower_hit.wav", ")weapons/rainblower/rainblower_hit.wav");
		pFilteredTrans->SetString(")weapons/rainblower/rainblower_pilot.wav", ")weapons/rainblower/rainblower_pilot.wav");
		pFilteredTrans->SetString("Weapon_Rainblower.Fire", "Weapon_Rainblower.Fire");
		pFilteredTrans->SetString("Weapon_Rainblower.FireLoop", "Weapon_Rainblower.FireLoop");
		pFilteredTrans->SetString("Weapon_Rainblower.WindDown", "Weapon_Rainblower.WindDown");
		pFilteredTrans->SetString("Weapon_Rainblower.FireHit", "Weapon_Rainblower.FireHit");
		pFilteredTrans->SetString("Weapon_Rainblower.PilotLoop", "Weapon_Rainblower.PilotLoop");
		pFilteredTrans->SetString("Taunt.PyroBalloonicorn", "Taunt.PyroBalloonicorn");
		pFilteredTrans->SetString(")items/pyro_music_tube.wav", ")items/pyro_music_tube.wav");
		pFilteredTrans->SetString("Taunt.Party_Trick", "Taunt.Party_Trick_Pyro_Vision");
		pFilteredTrans->SetString("Taunt.GasBlast", "Taunt.GasBlastPyrovision");
		pFilteredTrans->SetString("vo/demoman_PainCrticialDeath01.mp3", "vo/demoman_LaughLong01.mp3");
		pFilteredTrans->SetString("vo/demoman_PainCrticialDeath02.mp3", "vo/demoman_LaughLong02.mp3");
		pFilteredTrans->SetString("vo/demoman_PainCrticialDeath03.mp3", "vo/demoman_LaughLong01.mp3");
		pFilteredTrans->SetString("vo/demoman_PainCrticialDeath04.mp3", "vo/demoman_LaughLong02.mp3");
		pFilteredTrans->SetString("vo/demoman_PainCrticialDeath05.mp3", "vo/demoman_LaughLong01.mp3");
		pFilteredTrans->SetString("vo/demoman_PainSevere01.mp3", "vo/demoman_LaughHappy01.mp3");
		pFilteredTrans->SetString("vo/demoman_PainSevere02.mp3", "vo/demoman_LaughHappy02.mp3");
		pFilteredTrans->SetString("vo/demoman_PainSevere03.mp3", "vo/demoman_LaughHappy01.mp3");
		pFilteredTrans->SetString("vo/demoman_PainSevere04.mp3", "vo/demoman_LaughHappy02.mp3");
		pFilteredTrans->SetString("vo/demoman_PainSharp01.mp3", "vo/demoman_LaughShort01.mp3");
		pFilteredTrans->SetString("vo/demoman_PainSharp02.mp3", "vo/demoman_LaughShort02.mp3");
		pFilteredTrans->SetString("vo/demoman_PainSharp03.mp3", "vo/demoman_LaughShort03.mp3");
		pFilteredTrans->SetString("vo/demoman_PainSharp04.mp3", "vo/demoman_LaughShort04.mp3");
		pFilteredTrans->SetString("vo/demoman_PainSharp05.mp3", "vo/demoman_LaughShort05.mp3");
		pFilteredTrans->SetString("vo/demoman_PainSharp06.mp3", "vo/demoman_LaughShort06.mp3");
		pFilteredTrans->SetString("vo/demoman_PainSharp07.mp3", "vo/demoman_LaughShort01.mp3");
		pFilteredTrans->SetString("vo/demoman_AutoOnFire01.mp3", "vo/demoman_PositiveVocalization02.mp3");
		pFilteredTrans->SetString("vo/demoman_AutoOnFire02.mp3", "vo/demoman_PositiveVocalization03.mp3");
		pFilteredTrans->SetString("vo/demoman_AutoOnFire03.mp3", "vo/demoman_PositiveVocalization04.mp3");
		pFilteredTrans->SetString("vo/engineer_PainCrticialDeath01.mp3", "vo/engineer_LaughLong01.mp3");
		pFilteredTrans->SetString("vo/engineer_PainCrticialDeath02.mp3", "vo/engineer_LaughLong02.mp3");
		pFilteredTrans->SetString("vo/engineer_PainCrticialDeath03.mp3", "vo/engineer_LaughLong01.mp3");
		pFilteredTrans->SetString("vo/engineer_PainCrticialDeath04.mp3", "vo/engineer_LaughLong02.mp3");
		pFilteredTrans->SetString("vo/engineer_PainCrticialDeath05.mp3", "vo/engineer_LaughLong01.mp3");
		pFilteredTrans->SetString("vo/engineer_PainCrticialDeath06.mp3", "vo/engineer_LaughLong02.mp3");
		pFilteredTrans->SetString("vo/engineer_PainSevere01.mp3", "vo/engineer_LaughHappy01.mp3");
		pFilteredTrans->SetString("vo/engineer_PainSevere02.mp3", "vo/engineer_LaughHappy02.mp3");
		pFilteredTrans->SetString("vo/engineer_PainSevere03.mp3", "vo/engineer_LaughHappy03.mp3");
		pFilteredTrans->SetString("vo/engineer_PainSevere04.mp3", "vo/engineer_LaughHappy01.mp3");
		pFilteredTrans->SetString("vo/engineer_PainSevere05.mp3", "vo/engineer_LaughHappy02.mp3");
		pFilteredTrans->SetString("vo/engineer_PainSevere06.mp3", "vo/engineer_LaughHappy03.mp3");
		pFilteredTrans->SetString("vo/engineer_PainSevere07.mp3", "vo/engineer_LaughHappy01.mp3");
		pFilteredTrans->SetString("vo/engineer_PainSharp01.mp3", "vo/engineer_LaughShort01.mp3");
		pFilteredTrans->SetString("vo/engineer_PainSharp02.mp3", "vo/engineer_LaughShort02.mp3");
		pFilteredTrans->SetString("vo/engineer_PainSharp03.mp3", "vo/engineer_LaughShort03.mp3");
		pFilteredTrans->SetString("vo/engineer_PainSharp04.mp3", "vo/engineer_LaughShort04.mp3");
		pFilteredTrans->SetString("vo/engineer_PainSharp05.mp3", "vo/engineer_LaughShort01.mp3");
		pFilteredTrans->SetString("vo/engineer_PainSharp06.mp3", "vo/engineer_LaughShort02.mp3");
		pFilteredTrans->SetString("vo/engineer_PainSharp07.mp3", "vo/engineer_LaughShort03.mp3");
		pFilteredTrans->SetString("vo/engineer_PainSharp08.mp3", "vo/engineer_LaughShort04.mp3");
		pFilteredTrans->SetString("vo/engineer_AutoOnFire01.mp3", "vo/engineer_PositiveVocalization01.mp3");
		pFilteredTrans->SetString("vo/engineer_AutoOnFire02.mp3", "vo/engineer_Cheers01.mp3");
		pFilteredTrans->SetString("vo/engineer_AutoOnFire03.mp3", "vo/engineer_Cheers02.mp3");
		pFilteredTrans->SetString("vo/heavy_PainCrticialDeath01.mp3", "vo/heavy_LaughLong01.mp3");
		pFilteredTrans->SetString("vo/heavy_PainCrticialDeath02.mp3", "vo/heavy_LaughLong02.mp3");
		pFilteredTrans->SetString("vo/heavy_PainCrticialDeath03.mp3", "vo/heavy_LaughLong02.mp3");
		pFilteredTrans->SetString("vo/heavy_PainSevere01.mp3", "vo/heavy_LaughHappy01.mp3");
		pFilteredTrans->SetString("vo/heavy_PainSevere02.mp3", "vo/heavy_LaughHappy02.mp3");
		pFilteredTrans->SetString("vo/heavy_PainSevere03.mp3", "vo/heavy_LaughHappy03.mp3");
		pFilteredTrans->SetString("vo/heavy_PainSharp01.mp3", "vo/heavy_LaughShort01.mp3");
		pFilteredTrans->SetString("vo/heavy_PainSharp02.mp3", "vo/heavy_LaughShort02.mp3");
		pFilteredTrans->SetString("vo/heavy_PainSharp03.mp3", "vo/heavy_LaughShort03.mp3");
		pFilteredTrans->SetString("vo/heavy_PainSharp04.mp3", "vo/heavy_LaughShort01.mp3");
		pFilteredTrans->SetString("vo/heavy_PainSharp05.mp3", "vo/heavy_LaughShort02.mp3");
		pFilteredTrans->SetString("vo/heavy_AutoOnFire01.mp3", "vo/heavy_PositiveVocalization01.mp3");
		pFilteredTrans->SetString("vo/heavy_AutoOnFire02.mp3", "vo/heavy_PositiveVocalization02.mp3");
		pFilteredTrans->SetString("vo/heavy_AutoOnFire03.mp3", "vo/heavy_PositiveVocalization03.mp3");
		pFilteredTrans->SetString("vo/heavy_AutoOnFire04.mp3", "vo/heavy_PositiveVocalization04.mp3");
		pFilteredTrans->SetString("vo/heavy_AutoOnFire05.mp3", "vo/heavy_PositiveVocalization05.mp3");
		pFilteredTrans->SetString("vo/medic_PainCrticialDeath01.mp3", "vo/medic_LaughLong01.mp3");
		pFilteredTrans->SetString("vo/medic_PainCrticialDeath02.mp3", "vo/medic_LaughLong02.mp3");
		pFilteredTrans->SetString("vo/medic_PainCrticialDeath03.mp3", "vo/medic_LaughLong01.mp3");
		pFilteredTrans->SetString("vo/medic_PainCrticialDeath04.mp3", "vo/medic_LaughLong02.mp3");
		pFilteredTrans->SetString("vo/medic_PainSevere01.mp3", "vo/medic_LaughHappy01.mp3");
		pFilteredTrans->SetString("vo/medic_PainSevere02.mp3", "vo/medic_LaughHappy02.mp3");
		pFilteredTrans->SetString("vo/medic_PainSevere03.mp3", "vo/medic_LaughHappy03.mp3");
		pFilteredTrans->SetString("vo/medic_PainSevere04.mp3", "vo/medic_LaughHappy01.mp3");
		pFilteredTrans->SetString("vo/medic_PainSharp01.mp3", "vo/medic_LaughShort01.mp3");
		pFilteredTrans->SetString("vo/medic_PainSharp02.mp3", "vo/medic_LaughShort02.mp3");
		pFilteredTrans->SetString("vo/medic_PainSharp03.mp3", "vo/medic_LaughShort03.mp3");
		pFilteredTrans->SetString("vo/medic_PainSharp04.mp3", "vo/medic_LaughShort01.mp3");
		pFilteredTrans->SetString("vo/medic_PainSharp05.mp3", "vo/medic_LaughShort02.mp3");
		pFilteredTrans->SetString("vo/medic_PainSharp06.mp3", "vo/medic_LaughShort03.mp3");
		pFilteredTrans->SetString("vo/medic_PainSharp07.mp3", "vo/medic_LaughShort01.mp3");
		pFilteredTrans->SetString("vo/medic_PainSharp08.mp3", "vo/medic_LaughShort02.mp3");
		pFilteredTrans->SetString("vo/medic_AutoOnFire01.mp3", "vo/medic_PositiveVocalization01.mp3");
		pFilteredTrans->SetString("vo/medic_AutoOnFire02.mp3", "vo/medic_PositiveVocalization02.mp3");
		pFilteredTrans->SetString("vo/medic_AutoOnFire03.mp3", "vo/medic_PositiveVocalization03.mp3");
		pFilteredTrans->SetString("vo/medic_AutoOnFire04.mp3", "vo/medic_PositiveVocalization04.mp3");
		pFilteredTrans->SetString("vo/medic_AutoOnFire05.mp3", "vo/medic_PositiveVocalization05.mp3");
		pFilteredTrans->SetString("vo/pyro_PainCrticialDeath01.mp3", "vo/pyro_LaughLong01.mp3");
		pFilteredTrans->SetString("vo/pyro_PainCrticialDeath02.mp3", "vo/pyro_LaughLong01.mp3");
		pFilteredTrans->SetString("vo/pyro_PainCrticialDeath03.mp3", "vo/pyro_LaughLong01.mp3");
		pFilteredTrans->SetString("vo/pyro_PainSevere01.mp3", "vo/pyro_LaughHappy01.mp3");
		pFilteredTrans->SetString("vo/pyro_PainSevere02.mp3", "vo/pyro_LaughHappy01.mp3");
		pFilteredTrans->SetString("vo/pyro_PainSevere03.mp3", "vo/pyro_LaughHappy01.mp3");
		pFilteredTrans->SetString("vo/pyro_PainSevere04.mp3", "vo/pyro_LaughHappy01.mp3");
		pFilteredTrans->SetString("vo/pyro_PainSevere05.mp3", "vo/pyro_LaughHappy01.mp3");
		pFilteredTrans->SetString("vo/pyro_PainSevere06.mp3", "vo/pyro_LaughHappy01.mp3");
		pFilteredTrans->SetString("vo/pyro_AutoOnFire01.mp3", "vo/pyro_LaughHappy01.mp3");
		pFilteredTrans->SetString("vo/pyro_AutoOnFire02.mp3", "vo/pyro_LaughHappy01.mp3");
		pFilteredTrans->SetString("vo/scout_PainCrticialDeath01.mp3", "vo/scout_LaughLong01.mp3");
		pFilteredTrans->SetString("vo/scout_PainCrticialDeath02.mp3", "vo/scout_LaughLong02.mp3");
		pFilteredTrans->SetString("vo/scout_PainCrticialDeath03.mp3", "vo/scout_LaughLong01.mp3");
		pFilteredTrans->SetString("vo/scout_PainSevere01.mp3", "vo/scout_LaughHappy01.mp3");
		pFilteredTrans->SetString("vo/scout_PainSevere02.mp3", "vo/scout_LaughHappy02.mp3");
		pFilteredTrans->SetString("vo/scout_PainSevere03.mp3", "vo/scout_LaughHappy03.mp3");
		pFilteredTrans->SetString("vo/scout_PainSevere04.mp3", "vo/scout_LaughHappy04.mp3");
		pFilteredTrans->SetString("vo/scout_PainSevere05.mp3", "vo/scout_LaughHappy01.mp3");
		pFilteredTrans->SetString("vo/scout_PainSevere06.mp3", "vo/scout_LaughHappy02.mp3");
		pFilteredTrans->SetString("vo/scout_PainSharp01.mp3", "vo/scout_LaughShort01.mp3");
		pFilteredTrans->SetString("vo/scout_PainSharp02.mp3", "vo/scout_LaughShort02.mp3");
		pFilteredTrans->SetString("vo/scout_PainSharp03.mp3", "vo/scout_LaughShort03.mp3");
		pFilteredTrans->SetString("vo/scout_PainSharp04.mp3", "vo/scout_LaughShort04.mp3");
		pFilteredTrans->SetString("vo/scout_PainSharp05.mp3", "vo/scout_LaughShort05.mp3");
		pFilteredTrans->SetString("vo/scout_PainSharp06.mp3", "vo/scout_LaughShort01.mp3");
		pFilteredTrans->SetString("vo/scout_PainSharp07.mp3", "vo/scout_LaughShort02.mp3");
		pFilteredTrans->SetString("vo/scout_PainSharp08.mp3", "vo/scout_LaughShort03.mp3");
		pFilteredTrans->SetString("vo/scout_AutoOnFire01.mp3", "vo/scout_PositiveVocalization02.mp3");
		pFilteredTrans->SetString("vo/scout_AutoOnFire02.mp3", "vo/scout_PositiveVocalization03.mp3");
		pFilteredTrans->SetString("vo/sniper_PainCrticialDeath01.mp3", "vo/sniper_LaughLong01.mp3");
		pFilteredTrans->SetString("vo/sniper_PainCrticialDeath02.mp3", "vo/sniper_LaughLong02.mp3");
		pFilteredTrans->SetString("vo/sniper_PainCrticialDeath03.mp3", "vo/sniper_LaughLong01.mp3");
		pFilteredTrans->SetString("vo/sniper_PainCrticialDeath04.mp3", "vo/sniper_LaughLong02.mp3");
		pFilteredTrans->SetString("vo/sniper_PainSevere01.mp3", "vo/sniper_LaughHappy01.mp3");
		pFilteredTrans->SetString("vo/sniper_PainSevere02.mp3", "vo/sniper_LaughHappy02.mp3");
		pFilteredTrans->SetString("vo/sniper_PainSevere03.mp3", "vo/sniper_LaughHappy01.mp3");
		pFilteredTrans->SetString("vo/sniper_PainSevere04.mp3", "vo/sniper_LaughHappy02.mp3");
		pFilteredTrans->SetString("vo/sniper_PainSharp01.mp3", "vo/sniper_LaughShort01.mp3");
		pFilteredTrans->SetString("vo/sniper_PainSharp02.mp3", "vo/sniper_LaughShort02.mp3");
		pFilteredTrans->SetString("vo/sniper_PainSharp03.mp3", "vo/sniper_LaughShort03.mp3");
		pFilteredTrans->SetString("vo/sniper_PainSharp04.mp3", "vo/sniper_LaughShort04.mp3");
		pFilteredTrans->SetString("vo/sniper_AutoOnFire01.mp3", "vo/sniper_PositiveVocalization02.mp3");
		pFilteredTrans->SetString("vo/sniper_AutoOnFire02.mp3", "vo/sniper_PositiveVocalization03.mp3");
		pFilteredTrans->SetString("vo/sniper_AutoOnFire03.mp3", "vo/sniper_PositiveVocalization05.mp3");
		pFilteredTrans->SetString("vo/soldier_PainCrticialDeath01.mp3", "vo/soldier_LaughLong01.mp3");
		pFilteredTrans->SetString("vo/soldier_PainCrticialDeath02.mp3", "vo/soldier_LaughLong02.mp3");
		pFilteredTrans->SetString("vo/soldier_PainCrticialDeath03.mp3", "vo/soldier_LaughLong03.mp3");
		pFilteredTrans->SetString("vo/soldier_PainCrticialDeath04.mp3", "vo/soldier_LaughLong01.mp3");
		pFilteredTrans->SetString("vo/soldier_PainSevere01.mp3", "vo/soldier_LaughHappy01.mp3");
		pFilteredTrans->SetString("vo/soldier_PainSevere02.mp3", "vo/soldier_LaughHappy02.mp3");
		pFilteredTrans->SetString("vo/soldier_PainSevere03.mp3", "vo/soldier_LaughHappy03.mp3");
		pFilteredTrans->SetString("vo/soldier_PainSevere04.mp3", "vo/soldier_LaughHappy01.mp3");
		pFilteredTrans->SetString("vo/soldier_PainSevere05.mp3", "vo/soldier_LaughHappy02.mp3");
		pFilteredTrans->SetString("vo/soldier_PainSevere06.mp3", "vo/soldier_LaughHappy03.mp3");
		pFilteredTrans->SetString("vo/soldier_PainSharp01.mp3", "vo/soldier_LaughShort01.mp3");
		pFilteredTrans->SetString("vo/soldier_PainSharp02.mp3", "vo/soldier_LaughShort02.mp3");
		pFilteredTrans->SetString("vo/soldier_PainSharp03.mp3", "vo/soldier_LaughShort03.mp3");
		pFilteredTrans->SetString("vo/soldier_PainSharp04.mp3", "vo/soldier_LaughShort04.mp3");
		pFilteredTrans->SetString("vo/soldier_PainSharp05.mp3", "vo/soldier_LaughShort01.mp3");
		pFilteredTrans->SetString("vo/soldier_PainSharp06.mp3", "vo/soldier_LaughShort02.mp3");
		pFilteredTrans->SetString("vo/soldier_PainSharp07.mp3", "vo/soldier_LaughShort03.mp3");
		pFilteredTrans->SetString("vo/soldier_PainSharp08.mp3", "vo/soldier_LaughShort04.mp3");
		pFilteredTrans->SetString("vo/soldier_AutoOnFire01.mp3", "vo/soldier_PositiveVocalization01.mp3");
		pFilteredTrans->SetString("vo/soldier_AutoOnFire02.mp3", "vo/soldier_PositiveVocalization02.mp3");
		pFilteredTrans->SetString("vo/soldier_AutoOnFire03.mp3", "vo/soldier_PositiveVocalization03.mp3");
		pFilteredTrans->SetString("vo/spy_PainCrticialDeath01.mp3", "vo/spy_LaughLong01.mp3");
		pFilteredTrans->SetString("vo/spy_PainCrticialDeath02.mp3", "vo/spy_LaughLong01.mp3");
		pFilteredTrans->SetString("vo/spy_PainCrticialDeath03.mp3", "vo/spy_LaughLong01.mp3");
		pFilteredTrans->SetString("vo/spy_PainSevere01.mp3", "vo/spy_LaughHappy01.mp3");
		pFilteredTrans->SetString("vo/spy_PainSevere02.mp3", "vo/spy_LaughHappy02.mp3");
		pFilteredTrans->SetString("vo/spy_PainSevere03.mp3", "vo/spy_LaughHappy03.mp3");
		pFilteredTrans->SetString("vo/spy_PainSevere04.mp3", "vo/spy_LaughHappy01.mp3");
		pFilteredTrans->SetString("vo/spy_PainSevere05.mp3", "vo/spy_LaughHappy02.mp3");
		pFilteredTrans->SetString("vo/spy_PainSharp01.mp3", "vo/spy_LaughShort01.mp3");
		pFilteredTrans->SetString("vo/spy_PainSharp02.mp3", "vo/spy_LaughShort02.mp3");
		pFilteredTrans->SetString("vo/spy_PainSharp03.mp3", "vo/spy_LaughShort03.mp3");
		pFilteredTrans->SetString("vo/spy_PainSharp04.mp3", "vo/spy_LaughShort04.mp3");
		pFilteredTrans->SetString("vo/spy_AutoOnFire01.mp3", "vo/spy_PositiveVocalization02.mp3");
		pFilteredTrans->SetString("vo/spy_AutoOnFire02.mp3", "vo/spy_PositiveVocalization04.mp3");
		pFilteredTrans->SetString("vo/spy_AutoOnFire03.mp3", "vo/spy_PositiveVocalization05.mp3");
		pSounds->AddSubKey( pFilteredTrans );
	}

	KeyValues *pWeapons = new KeyValues( "weapons" );
	m_pVisionFilterTranslations->AddSubKey( pWeapons );

	{
		KeyValues *pNoFilterTrans = new KeyValues( "0" );
		pNoFilterTrans->SetString(
			"models/weapons/c_models/c_rainblower/c_rainblower.mdl",
			"models/weapons/c_models/c_flamethrower/c_flamethrower.mdl");
		pNoFilterTrans->SetString(
			"models/weapons/c_models/c_lollichop/c_lollichop.mdl",
			"models/weapons/w_models/w_fireaxe.mdl");
		pWeapons->AddSubKey( pNoFilterTrans );

		KeyValues *pFilteredTrans = new KeyValues( "1" );
		pFilteredTrans->SetString(
			"models/weapons/c_models/c_rainblower/c_rainblower.mdl",
			"models/weapons/c_models/c_rainblower/c_rainblower.mdl");
		pFilteredTrans->SetString(
			"models/weapons/c_models/c_lollichop/c_lollichop.mdl",
			"models/weapons/c_models/c_lollichop/c_lollichop.mdl");
		pFilteredTrans->SetString(
			"models/player/items/demo/demo_bombs.mdl",
			"models/player/items/all_class/mtp_bottle_demo.mdl");
		pFilteredTrans->SetString(
			"models/player/items/pyro/xms_pyro_bells.mdl",
			"models/player/items/all_class/mtp_bottle_pyro.mdl");
		pFilteredTrans->SetString(
			"models/player/items/soldier/ds_can_grenades.mdl",
			"models/player/items/all_class/mtp_bottle_soldier.mdl");
		pWeapons->AddSubKey( pFilteredTrans );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFGameRules::GetVideoFileForMap( bool bWithExtension /*= true*/ )
{
	char mapname[MAX_MAP_NAME];

	Q_FileBase( engine->GetLevelName(), mapname, sizeof( mapname ) );
	Q_strlower( mapname );

#ifdef _X360
	// need to remove the .360 extension on the end of the map name
	char *pExt = Q_stristr( mapname, ".360" );
	if ( pExt )
	{
		*pExt = '\0';
	}
#endif

	static char strFullpath[MAX_PATH];
	Q_strncpy( strFullpath, "media/", MAX_PATH );	// Assume we must play out of the media directory
	Q_strncat( strFullpath, mapname, MAX_PATH );

	if ( bWithExtension )
	{
		Q_strncat( strFullpath, ".bik", MAX_PATH );		// Assume we're a .bik extension type
	}

	return strFullpath;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::ModifySentChat( char *pBuf, int iBufSize )
{
	// Medieval mode only
	if ( IsInMedievalMode() && tf_medieval_autorp.GetBool() )
	{
		if ( !AutoRP() )
		{
			Warning( "AutoRP initialization failed!" );
			return;
		}

		AutoRP()->ApplyRPTo( pBuf, iBufSize );
	}

	int i = 0;
	while ( pBuf[i] )
	{
		if ( pBuf[i] == '"' )
		{
			pBuf[i] = '\'';
		}
		i++;
	}

}

void AddSubKeyNamed( KeyValues *pKeys, const char *pszName )
{
	KeyValues *pKeyvalToAdd = new KeyValues( pszName );

	if ( pKeyvalToAdd )
		pKeys->AddSubKey( pKeyvalToAdd );
}
#endif
