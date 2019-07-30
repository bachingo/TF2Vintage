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
#include "time.h"
#include "viewport_panel_names.h"
#include "tf_halloween_boss.h"
#include "entity_bossresource.h"
#ifdef CLIENT_DLL
	#include <game/client/iviewport.h>
	#include "c_tf_player.h"
	#include "c_tf_objective_resource.h"
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
	#include "bot/tf_bot_manager.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define ITEM_RESPAWN_TIME	10.0f

enum
{
	BIRTHDAY_RECALCULATE,
	BIRTHDAY_OFF,
	BIRTHDAY_ON,
};

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

ConVar tf_caplinear( "tf_caplinear", "1", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY, "If set to 1, teams must capture control points linearly." );
ConVar tf_stalematechangeclasstime( "tf_stalematechangeclasstime", "20", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY, "Amount of time that players are allowed to change class in stalemates." );
ConVar tf_birthday( "tf_birthday", "0", FCVAR_NOTIFY | FCVAR_REPLICATED );
ConVar tf_halloween( "tf_halloween", "0", FCVAR_NOTIFY | FCVAR_REPLICATED );
ConVar tf_christmas( "tf_christmas", "0", FCVAR_NOTIFY | FCVAR_REPLICATED );
//ConVar tf_forced_holiday( "tf_forced_holiday", "0", FCVAR_NOTIFY | FCVAR_REPLICATED ); Live TF2 uses this instead but for now lets just use separate ConVars
ConVar tf_medieval_autorp( "tf_medieval_autorp", "1", FCVAR_NOTIFY | FCVAR_REPLICATED, "Enable Medieval Mode auto-roleplaying." );

// tf2v specific cvars.
ConVar tf2v_falldamage_disablespread( "tf2v_falldamage_disablespread", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "Toggles random 20% fall damage spread." );
ConVar tf2v_allow_thirdperson( "tf2v_allow_thirdperson", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Allow players to switch to third person mode." );

#ifdef GAME_DLL
// TF overrides the default value of this convar
ConVar mp_waitingforplayers_time( "mp_waitingforplayers_time", ( IsX360() ? "15" : "30" ), FCVAR_GAMEDLL | FCVAR_DEVELOPMENTONLY, "WaitingForPlayers time length in seconds" );

ConVar mp_humans_must_join_team( "mp_humans_must_join_team", "any", FCVAR_GAMEDLL | FCVAR_REPLICATED, "Restricts human players to a single team {any, blue, red, spectator}" );

ConVar tf_arena_force_class( "tf_arena_force_class", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Force random classes in arena." );
ConVar tf_arena_first_blood( "tf_arena_first_blood", "1", FCVAR_NOTIFY | FCVAR_REPLICATED, "Toggles first blood criticals" );

ConVar tf_gamemode_arena( "tf_gamemode_arena", "0", FCVAR_NOTIFY | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );
ConVar tf_gamemode_cp( "tf_gamemode_cp", "0", FCVAR_NOTIFY | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );
ConVar tf_gamemode_ctf( "tf_gamemode_ctf", "0", FCVAR_NOTIFY | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );
ConVar tf_gamemode_sd( "tf_gamemode_sd", "0", FCVAR_NOTIFY | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );
ConVar tf_gamemode_rd( "tf_gamemode_rd", "0", FCVAR_NOTIFY | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );
ConVar tf_gamemode_payload( "tf_gamemode_payload", "0", FCVAR_NOTIFY | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );
ConVar tf_gamemode_mvm( "tf_gamemode_mvm", "0", FCVAR_NOTIFY | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );
ConVar tf_gamemode_passtime( "tf_gamemode_passtime", "0", FCVAR_NOTIFY | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );

ConVar tf_teamtalk( "tf_teamtalk", "1", FCVAR_NOTIFY, "Teammates can always chat with each other whether alive or dead." );
ConVar tf_ctf_bonus_time( "tf_ctf_bonus_time", "10", FCVAR_NOTIFY, "Length of team crit time for CTF capture." );

ConVar tf_tournament_classlimit_scout( "tf_tournament_classlimit_scout", "-1", FCVAR_NOTIFY, "Tournament mode per-team class limit for Scouts.\n" );
ConVar tf_tournament_classlimit_sniper( "tf_tournament_classlimit_sniper", "-1", FCVAR_NOTIFY, "Tournament mode per-team class limit for Snipers.\n" );
ConVar tf_tournament_classlimit_soldier( "tf_tournament_classlimit_soldier", "-1", FCVAR_NOTIFY, "Tournament mode per-team class limit for Soldiers.\n" );
ConVar tf_tournament_classlimit_demoman( "tf_tournament_classlimit_demoman", "-1", FCVAR_NOTIFY, "Tournament mode per-team class limit for Demomen.\n" );
ConVar tf_tournament_classlimit_medic( "tf_tournament_classlimit_medic", "-1", FCVAR_NOTIFY, "Tournament mode per-team class limit for Medics.\n" );
ConVar tf_tournament_classlimit_heavy( "tf_tournament_classlimit_heavy", "-1", FCVAR_NOTIFY, "Tournament mode per-team class limit for Heavies.\n" );
ConVar tf_tournament_classlimit_pyro( "tf_tournament_classlimit_pyro", "-1", FCVAR_NOTIFY, "Tournament mode per-team class limit for Pyros.\n" );
ConVar tf_tournament_classlimit_spy( "tf_tournament_classlimit_spy", "-1", FCVAR_NOTIFY, "Tournament mode per-team class limit for Spies.\n" );
ConVar tf_tournament_classlimit_engineer( "tf_tournament_classlimit_engineer", "-1", FCVAR_NOTIFY, "Tournament mode per-team class limit for Engineers.\n" );
ConVar tf_tournament_classchange_allowed( "tf_tournament_classchange_allowed", "1", FCVAR_NOTIFY, "Allow players to change class while the game is active?.\n" );
ConVar tf_tournament_classchange_ready_allowed( "tf_tournament_classchange_ready_allowed", "1", FCVAR_NOTIFY, "Allow players to change class after they are READY?.\n" );
ConVar tf_classlimit( "tf_classlimit", "0", FCVAR_NOTIFY, "Limit on how many players can be any class (i.e. tf_class_limit 2 would limit 2 players per class).\n" );

extern ConVar tf_halloween_bot_min_player_count;

ConVar tf_halloween_boss_spawn_interval( "tf_halloween_boss_spawn_interval", "480", FCVAR_CHEAT, "Average interval between boss spawns, in seconds" );
ConVar tf_halloween_boss_spawn_interval_variation( "tf_halloween_boss_spawn_interval_variation", "60", FCVAR_CHEAT, "Variation of spawn interval +/-" );

ConVar tf_halloween_eyeball_boss_spawn_interval( "tf_halloween_eyeball_boss_spawn_interval", "180", FCVAR_CHEAT, "Average interval between boss spawns, in seconds" );
ConVar tf_halloween_eyeball_boss_spawn_interval_variation( "tf_halloween_eyeball_boss_spawn_interval_variation", "30", FCVAR_CHEAT, "Variation of spawn interval +/-" );

ConVar tf_halloween_zombie_mob_enabled( "tf_halloween_zombie_mob_enabled", "0", FCVAR_CHEAT, "If set to 1, spawn zombie mobs on non-Halloween Valve maps" );
ConVar tf_halloween_zombie_mob_spawn_interval( "tf_halloween_zombie_mob_spawn_interval", "180", FCVAR_CHEAT, "Average interval between zombie mob spawns, in seconds" );

static bool isBossForceSpawning = false;
CON_COMMAND_F( tf_halloween_force_boss_spawn, "For testing.", FCVAR_CHEAT )
{
	isBossForceSpawning = true;
}
#endif

#ifdef GAME_DLL
void ValidateCapturesPerRound( IConVar *pConVar, const char *oldValue, float flOldValue )
{
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
}
#endif	

ConVar tf_flag_caps_per_round( "tf_flag_caps_per_round", "3", FCVAR_REPLICATED, "Number of flag captures per round on CTF maps. Set to 0 to disable.", true, 0, true, 9
#ifdef GAME_DLL
	, ValidateCapturesPerRound
#endif
);


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
	RecvPropTime( RECVINFO( m_flCapturePointEnableTime ) ),
	RecvPropInt( RECVINFO( m_nHudType ) ),
	RecvPropBool( RECVINFO( m_bPlayingKoth ) ),
	RecvPropBool( RECVINFO( m_bPlayingMedieval ) ),
	RecvPropBool( RECVINFO( m_bPlayingHybrid_CTF_CP ) ),
	RecvPropBool( RECVINFO( m_bPlayingSpecialDeliveryMode ) ),
	RecvPropBool( RECVINFO( m_bPlayingRobotDestructionMode ) ),
	RecvPropBool( RECVINFO( m_bPlayingMannVsMachine ) ),
	RecvPropBool( RECVINFO( m_bCompetitiveMode ) ),
	RecvPropBool( RECVINFO( m_bPowerupMode ) ),
	RecvPropEHandle( RECVINFO( m_hRedKothTimer ) ),
	RecvPropEHandle( RECVINFO( m_hBlueKothTimer ) ),
	RecvPropEHandle( RECVINFO( m_itHandle ) ),
	RecvPropInt( RECVINFO( m_halloweenScenario ) ),

#else

	SendPropInt( SENDINFO( m_nGameType ), 4, SPROP_UNSIGNED ),
	SendPropString( SENDINFO( m_pszTeamGoalStringRed ) ),
	SendPropString( SENDINFO( m_pszTeamGoalStringBlue ) ),
	SendPropTime( SENDINFO( m_flCapturePointEnableTime ) ),
	SendPropInt( SENDINFO( m_nHudType ) ),
	SendPropBool( SENDINFO( m_bPlayingKoth ) ),
	SendPropBool( SENDINFO( m_bPlayingMedieval ) ),
	SendPropBool( SENDINFO( m_bPlayingHybrid_CTF_CP ) ),
	SendPropBool( SENDINFO( m_bPlayingSpecialDeliveryMode ) ),
	SendPropBool( SENDINFO( m_bPlayingRobotDestructionMode ) ),
	SendPropBool( SENDINFO( m_bPlayingMannVsMachine ) ),
	SendPropBool( SENDINFO( m_bCompetitiveMode ) ),
	SendPropBool( SENDINFO( m_bPowerupMode ) ),
	SendPropEHandle( SENDINFO( m_hRedKothTimer ) ),
	SendPropEHandle( SENDINFO( m_hBlueKothTimer ) ),
	SendPropEHandle( SENDINFO( m_itHandle ) ),
	SendPropInt( SENDINFO( m_halloweenScenario ) ),

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
	//DEFINE_KEYFIELD( m_bCTF_Overtime, FIELD_BOOLEAN, "ctf_overtime" ),

	// Inputs.
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetRedTeamRespawnWaveTime", InputSetRedTeamRespawnWaveTime ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetBlueTeamRespawnWaveTime", InputSetBlueTeamRespawnWaveTime ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "AddRedTeamRespawnWaveTime", InputAddRedTeamRespawnWaveTime ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "AddBlueTeamRespawnWaveTime", InputAddBlueTeamRespawnWaveTime ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetRedTeamGoalString", InputSetRedTeamGoalString ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetBlueTeamGoalString", InputSetBlueTeamGoalString ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetRedTeamRole", InputSetRedTeamRole ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetBlueTeamRole", InputSetBlueTeamRole ),
	//DEFINE_INPUTFUNC( FIELD_STRING, "SetRequiredObserverTarget", InputSetRequiredObserverTarget ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "AddRedTeamScore", InputAddRedTeamScore ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "AddBlueTeamScore", InputAddBlueTeamScore ),

	DEFINE_INPUTFUNC( FIELD_VOID, "SetRedKothClockActive", InputSetRedKothClockActive ),
	DEFINE_INPUTFUNC( FIELD_VOID, "SetBlueKothClockActive", InputSetBlueKothClockActive ),

	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetCTFCaptureBonusTime", InputSetCTFCaptureBonusTime ),

	DEFINE_INPUTFUNC( FIELD_STRING, "PlayVORed", InputPlayVORed ),
	DEFINE_INPUTFUNC( FIELD_STRING, "PlayVOBlue", InputPlayVOBlue ),
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
void CTFGameRulesProxy::InputSetRedKothClockActive( inputdata_t &inputdata )
{
	if ( TFGameRules() && TFGameRules()->GetRedKothRoundTimer() )
	{
		TFGameRules()->GetRedKothRoundTimer()->InputEnable( inputdata );

		if ( TFGameRules()->GetBlueKothRoundTimer() )
			TFGameRules()->GetBlueKothRoundTimer()->InputDisable( inputdata );
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
void CTFGameRulesProxy::Activate()
{
	TFGameRules()->Activate();

	TFGameRules()->SetHudType( m_iHud_Type );

	BaseClass::Activate();
}

class CArenaLogic : public CPointEntity
{
public:
	DECLARE_CLASS( CArenaLogic, CPointEntity );
	DECLARE_DATADESC();

	CArenaLogic();

	void	Spawn( void );
	void	FireOnCapEnabled( void );
	//void	ArenaLogicThink( void );

	virtual void InputRoundActivate( inputdata_t &inputdata );

	COutputEvent m_OnArenaRoundStart;
	COutputEvent m_OnCapEnabled;

private:
	int		m_iUnlockPoint;
	bool	m_bCapUnlocked;

};

BEGIN_DATADESC( CArenaLogic )
	DEFINE_KEYFIELD( m_iUnlockPoint, FIELD_INTEGER, "unlock_point" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "RoundActivate", InputRoundActivate ),

	DEFINE_OUTPUT( m_OnArenaRoundStart, "OnArenaRoundStart" ),
	DEFINE_OUTPUT( m_OnCapEnabled, "OnCapEnabled" ),

	//DEFINE_THINKFUNC( ArenaLogicThink ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( tf_logic_arena, CArenaLogic );

CArenaLogic::CArenaLogic()
{
	m_iUnlockPoint = 60;
	m_bCapUnlocked = false;
}

void CArenaLogic::Spawn( void )
{
	BaseClass::Spawn();
	//SetThink( &CArenaLogic::ArenaLogicThink );
	//SetNextThink( gpGlobals->curtime );
}

void CArenaLogic::FireOnCapEnabled( void )
{
	if ( m_bCapUnlocked == false )
	{
		m_bCapUnlocked = true;
		m_OnCapEnabled.FireOutput( this, this );
	}
}

/*void CArenaLogic::ArenaLogicThink( void )
{
	// Live TF2 checks m_fCapEnableTime from TFGameRules here.
	SetNextThink( gpGlobals->curtime + 0.1 );

#ifdef GAME_DLL
	if ( TFGameRules()->State_Get() == GR_STATE_STALEMATE )
	{
		m_bCapUnlocked = true;
		m_OnCapEnabled.FireOutput(this, this);
	}
#endif
}*/

void CArenaLogic::InputRoundActivate( inputdata_t &inputdata )
{
	CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
	if ( !pMaster )
		return;

	for ( int i = 0; i < pMaster->GetNumPoints(); i++ )
	{
		CTeamControlPoint *pPoint = pMaster->GetControlPoint( i );

		variant_t sVariant;
		sVariant.SetInt( m_iUnlockPoint );
		pPoint->AcceptInput( "SetLocked", NULL, NULL, sVariant, 0 );
		g_EventQueue.AddEvent( pPoint, "SetUnlockTime", sVariant, 0.1, NULL, NULL );
	}
}

class CKothLogic : public CPointEntity
{
public:
	DECLARE_CLASS( CKothLogic, CPointEntity );
	DECLARE_DATADESC();

	CKothLogic();

	virtual void	InputAddBlueTimer( inputdata_t &inputdata );
	virtual void	InputAddRedTimer( inputdata_t &inputdata );
	virtual void	InputSetBlueTimer( inputdata_t &inputdata );
	virtual void	InputSetRedTimer( inputdata_t &inputdata );
	virtual void	InputRoundSpawn( inputdata_t &inputdata );
	virtual void	InputRoundActivate( inputdata_t &inputdata );

private:
	int m_iTimerLength;
	int m_iUnlockPoint;

};

BEGIN_DATADESC( CKothLogic )
	DEFINE_KEYFIELD( m_iTimerLength, FIELD_INTEGER, "timer_length" ),
	DEFINE_KEYFIELD( m_iUnlockPoint, FIELD_INTEGER, "unlock_point" ),

	// Inputs.
	DEFINE_INPUTFUNC( FIELD_INTEGER, "AddBlueTimer", InputAddBlueTimer ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "AddRedTimer", InputAddRedTimer ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetBlueTimer", InputSetBlueTimer ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetRedTimer", InputSetRedTimer ),
	DEFINE_INPUTFUNC( FIELD_VOID, "RoundSpawn", InputRoundSpawn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "RoundActivate", InputRoundActivate ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( tf_logic_koth, CKothLogic );

CKothLogic::CKothLogic()
{
	m_iTimerLength = 180;
	m_iUnlockPoint = 30;
}

void CKothLogic::InputRoundSpawn( inputdata_t &inputdata )
{
	variant_t sVariant;

	if ( TFGameRules() )
	{
		sVariant.SetInt( m_iTimerLength );

		TFGameRules()->SetBlueKothRoundTimer( (CTeamRoundTimer *)CBaseEntity::Create( "team_round_timer", vec3_origin, vec3_angle ) );

		if ( TFGameRules()->GetBlueKothRoundTimer() )
		{
			TFGameRules()->GetBlueKothRoundTimer()->SetName( MAKE_STRING( "zz_blue_koth_timer" ) );
			TFGameRules()->GetBlueKothRoundTimer()->SetShowInHud( false );
			TFGameRules()->GetBlueKothRoundTimer()->AcceptInput( "SetTime", NULL, NULL, sVariant, 0 );
			TFGameRules()->GetBlueKothRoundTimer()->AcceptInput( "Pause", NULL, NULL, sVariant, 0 );
			TFGameRules()->GetBlueKothRoundTimer()->ChangeTeam( TF_TEAM_BLUE );
		}

		TFGameRules()->SetRedKothRoundTimer( (CTeamRoundTimer *)CBaseEntity::Create( "team_round_timer", vec3_origin, vec3_angle ) );

		if ( TFGameRules()->GetRedKothRoundTimer() )
		{
			TFGameRules()->GetRedKothRoundTimer()->SetName( MAKE_STRING( "zz_red_koth_timer" ) );
			TFGameRules()->GetRedKothRoundTimer()->SetShowInHud( false );
			TFGameRules()->GetRedKothRoundTimer()->AcceptInput( "SetTime", NULL, NULL, sVariant, 0 );
			TFGameRules()->GetRedKothRoundTimer()->AcceptInput( "Pause", NULL, NULL, sVariant, 0 );
			TFGameRules()->GetRedKothRoundTimer()->ChangeTeam( TF_TEAM_RED );
		}
	}
}

void CKothLogic::InputRoundActivate( inputdata_t &inputdata )
{
	CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
	if ( !pMaster )
		return;

	for ( int i = 0; i < pMaster->GetNumPoints(); i++ )
	{
		CTeamControlPoint *pPoint = pMaster->GetControlPoint( i );

		variant_t sVariant;
		sVariant.SetInt( m_iUnlockPoint );
		pPoint->AcceptInput( "SetLocked", NULL, NULL, sVariant, 0 );
		g_EventQueue.AddEvent( pPoint, "SetUnlockTime", sVariant, 0.1, NULL, NULL );
	}
}

void CKothLogic::InputAddBlueTimer( inputdata_t &inputdata )
{
	if ( TFGameRules() && TFGameRules()->GetBlueKothRoundTimer() )
	{
		TFGameRules()->GetBlueKothRoundTimer()->AddTimerSeconds( inputdata.value.Int() );
	}
}

void CKothLogic::InputAddRedTimer( inputdata_t &inputdata )
{
	if ( TFGameRules() && TFGameRules()->GetRedKothRoundTimer() )
	{
		TFGameRules()->GetRedKothRoundTimer()->AddTimerSeconds( inputdata.value.Int() );
	}
}

void CKothLogic::InputSetBlueTimer( inputdata_t &inputdata )
{
	if ( TFGameRules() && TFGameRules()->GetBlueKothRoundTimer() )
	{
		TFGameRules()->GetBlueKothRoundTimer()->SetTimeRemaining( inputdata.value.Int() );
	}
}

void CKothLogic::InputSetRedTimer( inputdata_t &inputdata )
{
	if ( TFGameRules() && TFGameRules()->GetRedKothRoundTimer() )
	{
		TFGameRules()->GetRedKothRoundTimer()->SetTimeRemaining( inputdata.value.Int() );
	}
}

class CHybridMap_CTF_CP : public CPointEntity
{
public:
	DECLARE_CLASS( CHybridMap_CTF_CP, CPointEntity );
	void	Spawn( void );
};

LINK_ENTITY_TO_CLASS( tf_logic_hybrid_ctf_cp, CHybridMap_CTF_CP );

void CHybridMap_CTF_CP::Spawn( void )
{
	BaseClass::Spawn();
}

class CMultipleEscortLogic : public CPointEntity
{
public:
	DECLARE_CLASS( CMultipleEscortLogic, CPointEntity );
	void Spawn( void );
};

void CMultipleEscortLogic::Spawn( void )
{
	BaseClass::Spawn();
}

LINK_ENTITY_TO_CLASS( tf_logic_multiple_escort, CMultipleEscortLogic );

class CMedievalLogic : public CPointEntity
{
public:
	DECLARE_CLASS( CMedievalLogic, CPointEntity );

	void Spawn( void );
};

void CMedievalLogic::Spawn( void )
{
	BaseClass::Spawn();
}

LINK_ENTITY_TO_CLASS( tf_logic_medieval, CMedievalLogic );

class CCPTimerLogic : public CPointEntity
{
public:
	DECLARE_CLASS( CCPTimerLogic, CPointEntity );
	DECLARE_DATADESC();

	CCPTimerLogic();

	void			Spawn( void );
	void			Think( void );
	virtual void	InputRoundSpawn( inputdata_t &inputdata );

	COutputEvent m_onCountdownStart;
	COutputEvent m_onCountdown15SecRemain;
	COutputEvent m_onCountdown10SecRemain;
	COutputEvent m_onCountdown5SecRemain;
	COutputEvent m_onCountdownEnd;

private:

	string_t					m_iszControlPointName;
	int							m_nTimerLength;
	CHandle<CTeamControlPoint>	m_hPoint;

	bool						m_bRestartTimer;
	CountdownTimer				m_TimeRemaining;

	bool						m_bOn15SecRemain;
	bool						m_bOn10SecRemain;
	bool						m_bOn5SecRemain;

};

BEGIN_DATADESC( CCPTimerLogic )
	DEFINE_KEYFIELD( m_iszControlPointName, FIELD_STRING, "controlpoint" ),
	DEFINE_KEYFIELD( m_nTimerLength, FIELD_INTEGER, "timer_length" ),

	// Inputs.
	DEFINE_INPUTFUNC( FIELD_VOID, "RoundSpawn", InputRoundSpawn ),

	// Outputs.
	DEFINE_OUTPUT( m_onCountdownStart, "OnCountdownStart" ),
	DEFINE_OUTPUT( m_onCountdown15SecRemain, "OnCountdown15SecRemain" ),
	DEFINE_OUTPUT( m_onCountdown10SecRemain, "OnCountdown10SecRemain" ),
	DEFINE_OUTPUT( m_onCountdown5SecRemain, "OnCountdown5SecRemain" ),
	DEFINE_OUTPUT( m_onCountdownEnd, "OnCountdownEnd" ),
END_DATADESC()

CCPTimerLogic::CCPTimerLogic()
{
	m_bOn5SecRemain = true;
	m_bOn10SecRemain = true;
	m_bOn15SecRemain = true;
	m_bRestartTimer = true;
	SetContextThink( &CCPTimerLogic::Think, gpGlobals->curtime + 0.15, "CCPTimerLogicThink" );
}

void CCPTimerLogic::Spawn( void )
{
	BaseClass::Spawn();
}

void CCPTimerLogic::Think( void )
{
	if ( !TFGameRules() || !ObjectiveResource() )
		return;

	float flUnknown = 0.15f;
	if ( TFGameRules()->State_Get() == GR_STATE_TEAM_WIN )
	{
		flUnknown = gpGlobals->curtime + 0.15f;
		m_TimeRemaining.Invalidate();
		SetNextThink( flUnknown );
	}

	if ( m_hPoint )
	{
		int index = m_hPoint->GetPointIndex();

		if ( TFGameRules()->TeamMayCapturePoint( TF_TEAM_BLUE, index ) )
		{
			if ( m_TimeRemaining.GetRemainingTime() <= 0.0 && m_bRestartTimer )
			{
				m_TimeRemaining.Start( flUnknown + m_nTimerLength );
				m_onCountdownStart.FireOutput( this, this );
				ObjectiveResource()->SetCPTimerTime( index, gpGlobals->curtime + m_nTimerLength );
				m_bRestartTimer = false;
			}
			else
			{
				if ( flUnknown <= m_TimeRemaining.GetRemainingTime() )
				{
					// I don't think is actually doing anything
					float flTime = m_TimeRemaining.GetRemainingTime() - flUnknown;
					if ( flTime <= 15.0 && m_bOn15SecRemain )
					{
						m_bOn15SecRemain = false;
					}
					else if ( flTime <= 10.0 && m_bOn10SecRemain )
					{
						m_bOn10SecRemain = false;
					}
					else if ( flTime <= 5.0 && m_bOn5SecRemain )
					{
						m_bOn5SecRemain = false;
					}
				}
				else
				{
					if ( ObjectiveResource()->GetNumControlPoints() <= index || ObjectiveResource()->GetCappingTeam( index ) == TEAM_UNASSIGNED )
					{
						m_TimeRemaining.Invalidate();
						m_onCountdownEnd.FireOutput( this, this );
						m_bOn15SecRemain = true;
						m_bOn10SecRemain = true;
						m_bOn5SecRemain = true;
						m_bRestartTimer = true;
						ObjectiveResource()->SetCPTimerTime( index, -1.0f );
						SetNextThink( TICK_NEVER_THINK );
					}
				}
			}
		}
		else
		{
			m_TimeRemaining.Invalidate();
			m_bRestartTimer = true;
		}
	}
	SetNextThink( gpGlobals->curtime + 0.15f );
}

void CCPTimerLogic::InputRoundSpawn( inputdata_t &inputdata )
{
	if ( m_iszControlPointName != NULL_STRING )
	{
		// We need to re-find our control point, because they're recreated over round restarts
		m_hPoint = dynamic_cast<CTeamControlPoint *>( gEntList.FindEntityByName( NULL, m_iszControlPointName ) );
		if ( !m_hPoint )
		{
			Warning( "%s failed to find control point named '%s'\n", GetClassname(), STRING( m_iszControlPointName ) );
		}
	}
}

LINK_ENTITY_TO_CLASS( tf_logic_cp_timer, CCPTimerLogic );

// Barebone for now, just so maps don't complain
class CTFHolidayEntity : public CPointEntity, public CGameEventListener
{
public:
	DECLARE_CLASS( CTFHolidayEntity, CPointEntity );
	DECLARE_DATADESC();

	CTFHolidayEntity();
	virtual ~CTFHolidayEntity() { }

	virtual int UpdateTransmitState( void ) { return SetTransmitState( FL_EDICT_ALWAYS ); }
	virtual void FireGameEvent( IGameEvent *event );

	void InputHalloweenSetUsingSpells( inputdata_t &inputdata );
	void InputHalloweenTeleportToHell( inputdata_t &inputdata );

private:
	int m_nHolidayType;
	int m_nTauntInHell;
	int m_nAllowHaunting;
};

CTFHolidayEntity::CTFHolidayEntity()
{
	ListenForGameEvent( "player_turned_to_ghost" );
	ListenForGameEvent( "player_team" );
	ListenForGameEvent( "player_disconnect" );
}

void CTFHolidayEntity::FireGameEvent( IGameEvent *event )
{
}

void CTFHolidayEntity::InputHalloweenSetUsingSpells( inputdata_t &inputdata )
{
}

void CTFHolidayEntity::InputHalloweenTeleportToHell( inputdata_t &inputdata )
{
}

BEGIN_DATADESC( CTFHolidayEntity )
	DEFINE_KEYFIELD( m_nHolidayType, FIELD_INTEGER, "holiday_type" ),
	DEFINE_KEYFIELD( m_nTauntInHell, FIELD_INTEGER, "tauntInHell" ),
	DEFINE_KEYFIELD( m_nAllowHaunting, FIELD_INTEGER, "allowHaunting" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "HalloweenSetUsingSpells", InputHalloweenSetUsingSpells ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Halloween2013TeleportToHell", InputHalloweenTeleportToHell ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( tf_logic_holiday, CTFHolidayEntity );

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
CTFGameRules::CTFGameRules()
{
#ifdef GAME_DLL
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

	m_iPreviousRoundWinners = TEAM_UNASSIGNED;
	m_iBirthdayMode = BIRTHDAY_RECALCULATE;

	m_pszTeamGoalStringRed.GetForModify()[0] = '\0';
	m_pszTeamGoalStringBlue.GetForModify()[0] = '\0';

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
	m_iBirthdayMode = BIRTHDAY_RECALCULATE;

	m_nGameType.Set( TF_GAMETYPE_UNDEFINED );

	tf_gamemode_arena.SetValue( 0 );
	tf_gamemode_cp.SetValue( 0 );
	tf_gamemode_ctf.SetValue( 0 );
	tf_gamemode_sd.SetValue( 0 );
	tf_gamemode_payload.SetValue( 0 );
	tf_gamemode_mvm.SetValue( 0 );
	tf_gamemode_rd.SetValue( 0 );
	tf_gamemode_passtime.SetValue( 0 );

	TeamplayRoundBasedRules()->SetMultipleTrains( false );

	m_hRedAttackTrain = NULL;
	m_hBlueAttackTrain = NULL;
	m_hRedDefendTrain = NULL;
	m_hBlueDefendTrain = NULL;

	CMedievalLogic *pMedieval = dynamic_cast<CMedievalLogic *>( gEntList.FindEntityByClassname( NULL, "tf_logic_medieval" ) );
	if ( pMedieval )
	{
		m_nGameType.Set( TF_GAMETYPE_MEDIEVAL );
		return;
	}

	CArenaLogic *pArena = dynamic_cast<CArenaLogic *>( gEntList.FindEntityByClassname( NULL, "tf_logic_arena" ) );
	if ( pArena )
	{
		m_nGameType.Set( TF_GAMETYPE_ARENA );
		tf_gamemode_arena.SetValue( 1 );
		Msg( "Executing server arena config file\n", 1 );
		engine->ServerCommand( "exec config_arena.cfg \n" );
		engine->ServerExecute();
		return;
	}

	CKothLogic *pKoth = dynamic_cast<CKothLogic *>( gEntList.FindEntityByClassname( NULL, "tf_logic_koth" ) );
	if ( pKoth )
	{
		m_nGameType.Set( TF_GAMETYPE_CP );
		m_bPlayingKoth = true;
		return;
	}

	CHybridMap_CTF_CP *pHybridEnt = dynamic_cast<CHybridMap_CTF_CP *>( gEntList.FindEntityByClassname( NULL, "tf_logic_hybrid_ctf_cp" ) );
	if ( pHybridEnt )
	{
		m_nGameType.Set( TF_GAMETYPE_CP );
		m_bPlayingHybrid_CTF_CP = true;
		return;
	}

	CCaptureFlag *pFlag = dynamic_cast<CCaptureFlag *>( gEntList.FindEntityByClassname( NULL, "item_teamflag" ) );
	if ( pFlag )
	{
		m_nGameType.Set( TF_GAMETYPE_CTF );
		tf_gamemode_ctf.SetValue( 1 );
		return;
	}

	CMultipleEscortLogic *pMultipleEscort = dynamic_cast<CMultipleEscortLogic *>( gEntList.FindEntityByClassname( NULL, "tf_logic_multiple_escort" ) );
	if ( pMultipleEscort )
	{
		m_nGameType.Set( TF_GAMETYPE_ESCORT );
		tf_gamemode_payload.SetValue( 1 );
		TeamplayRoundBasedRules()->SetMultipleTrains( true );
		return;
	}

	CTeamTrainWatcher *pTrain = dynamic_cast<CTeamTrainWatcher *>( gEntList.FindEntityByClassname( NULL, "team_train_watcher" ) );
	if ( pTrain )
	{
		m_nGameType.Set( TF_GAMETYPE_ESCORT );
		tf_gamemode_payload.SetValue( 1 );
		return;
	}

	if ( g_hControlPointMasters.Count() )
	{
		m_nGameType.Set( TF_GAMETYPE_CP );
		tf_gamemode_cp.SetValue( 1 );
		return;
	}
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

	if ( IsInTournamentMode() /*||  *((_DWORD *)this + 462) == 7 */ )
	{
		if ( iDesiredClassIndex <= TF_CLASS_COUNT )
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
			result = -1;
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
	{
		return true;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::CanBotChooseClass( CBasePlayer *pBot, int iDesiredClassIndex )
{
	// TODO: Implement CTFBotRoster entity
	return CanPlayerChooseClass( pBot, iDesiredClassIndex );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::CanBotChangeClass( CBasePlayer *pBot )
{
	if ( !pBot || !pBot->IsPlayer()/* || !DWORD(a2 + 2309) */ )
		return false;

	// TODO: Implement CTFBotRoster entity
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

	m_bossSpawnTimer.Invalidate();
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
#ifdef GAME_DLL
	m_szMostRecentCappers[0] = 0;
#endif
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
	// Respawn all the players
	RespawnPlayers( true );

	if ( TFGameRules()->IsInArenaMode() )
	{
		CArenaLogic *pArena = dynamic_cast<CArenaLogic *>( gEntList.FindEntityByClassname( NULL, "tf_logic_arena" ) );
		if ( pArena )
		{
			pArena->m_OnArenaRoundStart.FireOutput( pArena, pArena );

			IGameEvent *event = gameeventmanager->CreateEvent( "arena_round_start" );
			if ( event )
			{
				gameeventmanager->FireEvent( event );
			}

			for ( int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
			{
				BroadcastSound( i, "Announcer.AM_RoundStartRandom" );
				BroadcastSound( i, "Ambient.Siren" );
			}

			m_flStalemateStartTime = gpGlobals->curtime;
		}
		return;
	}

	// Remove everyone's objects
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( pPlayer )
		{
			pPlayer->RemoveAllOwnedEntitiesFromWorld();
		}
	}

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

	CTFPlayer *pPlayer;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

		if ( !pPlayer )
			continue;

		pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_SUDDENDEATH_START );
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

CTFRadiusDamageInfo::CTFRadiusDamageInfo()
{
	m_flRadius = 0.0f;
	m_iClassIgnore = CLASS_NONE;
	m_pEntityIgnore = NULL;
	m_flSelfDamageRadius = 0.0f;
}

ConVar tf_fixedup_damage_radius( "tf_fixedup_damage_radius", "1", FCVAR_DEVELOPMENTONLY );

bool CTFRadiusDamageInfo::ApplyToEntity( CBaseEntity *pEntity )
{
	const int MASK_RADIUS_DAMAGE = MASK_SHOT&( ~CONTENTS_HITBOX );
	trace_t		tr;
	float		falloff;
	Vector		vecSpot;

	if ( info.GetDamageType() & DMG_RADIUS_MAX )
		falloff = 0.0;
	else if ( info.GetDamageType() & DMG_HALF_FALLOFF )
		falloff = 0.5;
	else if ( m_flRadius )
		falloff = info.GetDamage() / m_flRadius;
	else
		falloff = 1.0;

	CBaseEntity *pInflictor = info.GetInflictor();

	//	float flHalfRadiusSqr = Square( flRadius / 2.0f );

	// This value is used to scale damage when the explosion is blocked by some other object.
	float flBlockedDamagePercent = 0.0f;

	// Check that the explosion can 'see' this entity, trace through players.
	vecSpot = pEntity->BodyTarget( m_vecSrc, false );
	CTraceFilterHitPlayer filter( info.GetInflictor(), pEntity, COLLISION_GROUP_PROJECTILE );
	UTIL_TraceLine( m_vecSrc, vecSpot, MASK_RADIUS_DAMAGE, &filter, &tr );

	if ( tr.fraction != 1.0 && tr.m_pEnt != pEntity )
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

		flDistanceToEntity = min( flToWorldSpaceCenter, flToOrigin );
	}
	else
	{
		flDistanceToEntity = ( m_vecSrc - tr.endpos ).Length();
	}

	if ( tf_fixedup_damage_radius.GetBool() )
	{
		flAdjustedDamage = RemapValClamped( flDistanceToEntity, 0, m_flRadius, info.GetDamage(), info.GetDamage() * falloff );
	}
	else
	{
		flAdjustedDamage = flDistanceToEntity * falloff;
		flAdjustedDamage = info.GetDamage() - flAdjustedDamage;
	}

	// Take a little less damage from yourself
	if ( tr.m_pEnt == info.GetAttacker() )
	{
		flAdjustedDamage = flAdjustedDamage * 0.75;
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

	CTakeDamageInfo adjustedInfo = info;
	//Msg("%s: Blocked damage: %f percent (in:%f  out:%f)\n", pEntity->GetClassname(), flBlockedDamagePercent * 100, flAdjustedDamage, flAdjustedDamage - (flAdjustedDamage * flBlockedDamagePercent) );
	adjustedInfo.SetDamage( flAdjustedDamage - ( flAdjustedDamage * flBlockedDamagePercent ) );

	// Now make a consideration for skill level!
	if ( info.GetAttacker() && info.GetAttacker()->IsPlayer() && pEntity->IsNPC() )
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
	CTakeDamageInfo &info = radiusInfo.info;
	CBaseEntity *pAttacker = info.GetAttacker();

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
		//if ( pEntity == pAttacker && radiusInfo.m_flSelfDamageRadius != 0.0f )
			//continue;

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
					default:
						break;
				}
			}
			else
			{
				if ( pTFPlayer->m_Shared.InCond( TF_COND_BURNING ) )
				{
					pTFPlayer->m_Shared.RemoveCond( TF_COND_BURNING );
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
	// For attacker, radius and damage need to be consistent so custom weapons don't screw up rocket jumping.
	/*if ( radiusInfo.m_flSelfDamageRadius != 0.0f )
	{
		if ( pAttacker )
		{
			// Get stock damage.
			CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase *>( info.GetWeapon() );
			if ( pWeapon )
			{
				info.SetDamage( (float)pWeapon->GetTFWpnData().GetWeaponData( TF_WEAPON_PRIMARY_MODE ).m_nDamage );
			}

			// Use stock radius.
			radiusInfo.m_flRadius = radiusInfo.m_flSelfDamageRadius;

			Vector vecHitPoint;
			pAttacker->CollisionProp()->CalcNearestPoint( radiusInfo.m_vecSrc, &vecHitPoint );
			Vector vecDir = vecHitPoint - radiusInfo.m_vecSrc;

			if ( vecDir.LengthSqr() <= ( radiusInfo.m_flRadius * radiusInfo.m_flRadius ) )
			{
				radiusInfo.ApplyToEntity( pAttacker );
			}
		}
	}*/
	return bExtinguished;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::RadiusDamage( CTFRadiusDamageInfo &radiusInfo )
{
	CTakeDamageInfo &info = radiusInfo.info;
	CBaseEntity *pAttacker = info.GetAttacker();
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
		if ( pEntity == pAttacker && radiusInfo.m_flSelfDamageRadius != 0.0f )
			continue;

		// Checking distance from source because Valve were apparently too lazy to fix the engine function.
		Vector vecHitPoint;
		pEntity->CollisionProp()->CalcNearestPoint( radiusInfo.m_vecSrc, &vecHitPoint );
		Vector vecDir = vecHitPoint - radiusInfo.m_vecSrc;

		if ( vecDir.LengthSqr() > ( radiusInfo.m_flRadius * radiusInfo.m_flRadius ) )
			continue;

		if ( radiusInfo.ApplyToEntity( pEntity ) )
		{
			if ( pEntity->IsPlayer() && !pEntity->InSameTeam( pAttacker ) )
			{
				iPlayersDamaged++;
			}
		}
	}

	info.SetDamagedOtherPlayers( iPlayersDamaged );

	// For attacker, radius and damage need to be consistent so custom weapons don't screw up rocket jumping.
	if ( radiusInfo.m_flSelfDamageRadius != 0.0f )
	{
		if ( pAttacker )
		{
			// Get stock damage.
			CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase *>( info.GetWeapon() );
			if ( pWeapon )
			{
				info.SetDamage( (float)pWeapon->GetTFWpnData().GetWeaponData( TF_WEAPON_PRIMARY_MODE ).m_nDamage );
			}

			// Use stock radius.
			radiusInfo.m_flRadius = radiusInfo.m_flSelfDamageRadius;

			Vector vecHitPoint;
			pAttacker->CollisionProp()->CalcNearestPoint( radiusInfo.m_vecSrc, &vecHitPoint );
			Vector vecDir = vecHitPoint - radiusInfo.m_vecSrc;

			if ( vecDir.LengthSqr() <= ( radiusInfo.m_flRadius * radiusInfo.m_flRadius ) )
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
	CTFRadiusDamageInfo radiusInfo;
	radiusInfo.info = info;
	radiusInfo.m_vecSrc = vecSrcIn;
	radiusInfo.m_flRadius = flRadius;
	radiusInfo.m_iClassIgnore = iClassIgnore;
	radiusInfo.m_pEntityIgnore = pEntityIgnore;

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
		if ( TFGameRules()->State_Get() != GR_STATE_TEAM_WIN && TFGameRules()->State_Get() != GR_STATE_GAME_OVER )
		{
			if ( pTalker->IsAlive() == false )
			{
				if ( pListener->IsAlive() == false || tf_teamtalk.GetBool() )
					return ( pListener->InSameTeam( pTalker ) );

				return false;
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

inline void PickSpawnSpot( CHalloweenBaseBoss::HalloweenBossType eBossType, Vector *vecSpot )
{
	if ( !g_hControlPointMasters.IsEmpty() && g_hControlPointMasters[0] )
	{
		CTeamControlPointMaster *pMaster = g_hControlPointMasters[0];
		for ( int i=0; i<pMaster->GetNumPoints(); ++i )
		{
			CTeamControlPoint *pPoint = pMaster->GetControlPoint( i );
			if ( pMaster->IsInRound( pPoint ) &&
				ObjectiveResource()->GetOwningTeam( pPoint->GetPointIndex() ) != TF_TEAM_BLUE &&
				TFGameRules()->TeamMayCapturePoint( TF_TEAM_BLUE, pPoint->GetPointIndex() ) )
			{
				CBaseEntity *pSpawnPoint = gEntList.FindEntityByClassname( NULL, "spawn_boss" );
				if ( pSpawnPoint )
				{
					*vecSpot = pSpawnPoint->GetAbsOrigin();
				}
				else
				{
					*vecSpot = pPoint->GetAbsOrigin();
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

				return;
			}
		}
	}
	else
	{
		CBaseEntity *pSpawnPoint = gEntList.FindEntityByClassname( NULL, "spawn_boss" );
		if ( pSpawnPoint )
		{
			*vecSpot = pSpawnPoint->GetAbsOrigin();
			return;
		}

		CUtlVector<CNavArea *> candidates;
		for ( int i=0; i<TheNavAreas.Count(); ++i )
		{
			CNavArea *area = TheNavAreas[i];
			if ( area->GetSizeX() >= 100.0f && area->GetSizeY() >= 100.0f )
				candidates.AddToTail( area );
		}

		if ( candidates.IsEmpty() )
			return;

		CNavArea *area = candidates.Random();
		*vecSpot = area->GetCenter();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::SpawnHalloweenBoss( void )
{
	// ???
	if ( !IsHolidayActive( kHoliday_Halloween ) )
		return;
	if ( !IsHolidayActive( kHoliday_Halloween ) )
		return SpawnZombieMob();

	float fSpawnInterval;
	float fSpawnVariation;
	CHalloweenBaseBoss::HalloweenBossType iBossType;

	if ( IsHalloweenScenario( HALLOWEEN_SCENARIO_MANOR ) )
	{
		iBossType = CHalloweenBaseBoss::HEADLESS_HATMAN;
		fSpawnInterval = tf_halloween_boss_spawn_interval.GetFloat();
		fSpawnVariation = tf_halloween_boss_spawn_interval_variation.GetFloat();
	}
	else if ( IsHalloweenScenario( HALLOWEEN_SCENARIO_VIADUCT ) )
	{
		iBossType = CHalloweenBaseBoss::EYEBALL_BOSS;
		fSpawnInterval = tf_halloween_eyeball_boss_spawn_interval.GetFloat();
		fSpawnVariation = tf_halloween_eyeball_boss_spawn_interval_variation.GetFloat();
	}
	else if ( /*IsHalloweenScenario( HALLOWEEN_SCENARIO_LAKESIDE )*/false )
	{
		/*if (CMerasmus::m_level > 3)
		{
			fSpawnInterval = 60.0f;
			fSpawnVariation = 0.0f;
		}
		else
		{
			fSpawnInterval = tf_merasmus_spawn_interval.GetFloat();
			fSpawnVariation = tf_merasmus_spawn_interval_variation.GetFloat();
		}*/
		iBossType = CHalloweenBaseBoss::MERASMUS;
	}
	else
	{
		SpawnZombieMob();
		return;
	}

	if ( !m_hNPCs.IsEmpty() )
	{
		if ( m_hNPCs[0] )
		{
			isBossForceSpawning = false;
			m_bossSpawnTimer.Start( RandomFloat( fSpawnInterval - fSpawnVariation, fSpawnInterval + fSpawnVariation ) );
			return;
		}
	}

	Vector vecSpawnLoc = vec3_origin;

	if ( !m_bossSpawnTimer.HasStarted() )
	{
		if ( !isBossForceSpawning )
			return;

		if ( IsHalloweenScenario( HALLOWEEN_SCENARIO_LAKESIDE ) )
			m_bossSpawnTimer.Start( RandomFloat( fSpawnInterval - fSpawnVariation, fSpawnInterval + fSpawnVariation ) );
		else
			m_bossSpawnTimer.Start( RandomFloat( 0, fSpawnInterval + fSpawnVariation ) * 0.5 );

		return;
	}

	if ( m_bossSpawnTimer.IsElapsed() )
	{
		if ( !isBossForceSpawning )
		{
			/*if (BYTE( this + 889 ) || BYTE( this + 900 ))
				return;*/

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

		PickSpawnSpot( iBossType, &vecSpawnLoc );
		CHalloweenBaseBoss::SpawnBossAtPos( iBossType, vecSpawnLoc );
		isBossForceSpawning = false;
		m_bossSpawnTimer.Start( RandomFloat( fSpawnInterval - fSpawnVariation, fSpawnInterval + fSpawnVariation ) );
		return;
	}

	if ( isBossForceSpawning )
	{
		PickSpawnSpot( iBossType, &vecSpawnLoc );
		CHalloweenBaseBoss::SpawnBossAtPos( iBossType, vecSpawnLoc );
		isBossForceSpawning = false;
		m_bossSpawnTimer.Start( RandomFloat( fSpawnInterval - fSpawnVariation, fSpawnInterval + fSpawnVariation ) );
		return;
	}

	if ( IsHalloweenScenario( HALLOWEEN_SCENARIO_LAKESIDE ) )
		m_bossSpawnTimer.Start( RandomFloat( fSpawnInterval - fSpawnVariation, fSpawnInterval + fSpawnVariation ) );
	else
		m_bossSpawnTimer.Start( RandomFloat( 0, fSpawnInterval + fSpawnVariation ) * 0.5 );
}

void CTFGameRules::SpawnZombieMob( void )
{
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

	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );

	const char *pszFormat = NULL;

	// team only
	if ( bTeamOnly == true )
	{
		if ( pTFPlayer->GetTeamNumber() == TEAM_SPECTATOR )
		{
			pszFormat = "TF_Chat_Spec";
		}
		else
		{
			if ( pTFPlayer->IsAlive() == false && State_Get() != GR_STATE_TEAM_WIN )
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
	/*else if ( pTFPlayer->m_bIsPlayerADev )
	{
		if ( pTFPlayer->GetTeamNumber() == TEAM_SPECTATOR )
		{
			pszFormat = "TF_Chat_DevSpec";
		}
		else
		{
			if (pTFPlayer->IsAlive() == false && State_Get() != GR_STATE_TEAM_WIN)
			{
				pszFormat = "TF_Chat_DevDead";
			}
			else
			{
				pszFormat = "TF_Chat_Dev";
			}
		}
	}*/
	else
	{
		if ( pTFPlayer->GetTeamNumber() == TEAM_SPECTATOR )
		{
			pszFormat = "TF_Chat_AllSpec";
		}
		else
		{
			if ( pTFPlayer->IsAlive() == false && State_Get() != GR_STATE_TEAM_WIN )
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
	int iFov = atoi( pszFov );
	iFov = clamp( iFov, 75, MAX_FOV );
	pTFPlayer->SetDefaultFOV( iFov );
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
	"nocrits",

	"tf_damage_disablespread",
	"dmgspread",

	"tf_use_fixed_weaponspreads",
	"nospread",

	"tf2v_force_stock_weapons",
	"stockweapons",

	"tf2v_allow_thirdperson",
	"thirdperson",

	"tf2v_random_weapons",
	"randomizer",

	"tf2v_autojump",
	"autojump",

	"tf2v_duckjump",
	"duckjump",

	"tf2v_allow_special_classes",
	"specialclasses",

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

	"tf_gamemode_payload",
	"payload",

	"tf_gamemode_mvm",
	"mvm",

	"tf_gamemode_passtime",
	"passtime",
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
			pszCustomKill = "bleed_kill";
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
						killer_weapon_name = "deflect_rocket";
						break;
					case TF_WEAPON_COMPOUND_BOW: //does this go here?
						if ( info.GetDamageType() & DMG_IGNITE )
							killer_weapon_name = "deflect_huntsman_flyingburn";
						else
							killer_weapon_name = "deflect_arrow";
						break;
				}
			}
		}
		else if ( CTFWeaponBaseGrenadeProj *pGrenade = dynamic_cast<CTFWeaponBaseGrenadeProj *>( pInflictor ) )
		{
			iWeaponID = pGrenade->GetWeaponID();

			// Most grenades have their own kill icons except for pipes and stickies, those use weapon icons.
			if ( iWeaponID == TF_WEAPON_GRENADE_DEMOMAN || iWeaponID == TF_WEAPON_GRENADE_PIPEBOMB )
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

	// make sure arrow kills are mapping to the huntsman
	else if ( !V_strcmp( killer_weapon_name, "tf_projectile_arrow" ) )
	{
		killer_weapon_name = "huntsman";
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

		// Mad milk after jarate
		CTFPlayer *pThrowerMilk = ToTFPlayer( static_cast<CBaseEntity *>( pTFVictim->m_Shared.m_hMilkAttacker.Get() ) );
		if ( pThrowerMilk )
		{
			if ( pThrowerMilk != pTFScorer )
				return pThrowerMilk;
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
// Purpose: 
// Input  : *pVictim - 
//			*pKiller - 
//			*pInflictor - 
//-----------------------------------------------------------------------------
void CTFGameRules::DeathNotice( CBasePlayer *pVictim, const CTakeDamageInfo &info )
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

	if ( IsInArenaMode() && tf_arena_first_blood.GetBool() && !m_bFirstBlood && pScorer && pScorer != pTFPlayerVictim )
	{
		m_bFirstBlood = true;
		float flElapsedTime = gpGlobals->curtime - m_flStalemateStartTime;;

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

		iDeathFlags |= TF_DEATH_FIRST_BLOOD;
		pScorer->m_Shared.AddCond( TF_COND_CRITBOOSTED_FIRST_BLOOD, 5.0f );
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
				if ( pItemDef->item_iconname[0] )
					killer_weapon_name = pItemDef->item_iconname;

				if ( pItemDef->item_logname[0] )
					killer_weapon_log_name = pItemDef->item_logname;
			}
		}
	}

	IGameEvent *event = gameeventmanager->CreateEvent( "player_death" );
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
			event->SetBool( "silent_kill", ( info.GetDamageCustom() == TF_DMG_CUSTOM_BACKSTAB && pActive->IsSilentKiller() ) );
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

	if ( m_flCTFBonusTime > -1 )
		flBoostTime = m_flCTFBonusTime;

	if ( flBoostTime > 0.0 )
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

		if ( bStreakReached )
		{
			for ( i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
			{
				BroadcastSound( i, "Announcer.AM_TeamScrambleRandom" );
			}
		}

		if ( !IsInTournamentMode() )
		{
			if ( iActivePlayers > 0 )
				Arena_ResetLosersScore( bStreakReached );
			else
				Arena_ResetLosersScore( true );
		}

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
						CTFPlayer *pTFPlayer = m_hArenaQueue.Element( i );;
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
					pTFPlayer = ToTFPlayer( pTeam[i]->GetPlayer( i ) );
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
					//pPlayer->RemoveDisguise();

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

	if ( pTFPlayer->m_Shared.InCond( TF_COND_INVULNERABLE ) || pTFPlayer->m_Shared.InCond( TF_COND_PHASE ) )
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
	if ( pTFPlayer->m_Shared.InCond( TF_COND_INVULNERABLE ) )
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
		if ( pMaster->IsInRound( pPoint ) &&
			ObjectiveResource()->GetOwningTeam( pPoint->GetPointIndex() ) != pPlayer->GetTeamNumber() &&
			ObjectiveResource()->TeamCanCapPoint( pPoint->GetPointIndex(), pPlayer->GetTeamNumber() ) &&
			TeamMayCapturePoint( pPlayer->GetTeamNumber(), pPoint->GetPointIndex() ) )
		{
			controlPointVector->AddToTail( pPoint );
		}
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

	for ( int i = 0; i < pMaster->GetNumPoints(); ++i )
	{
		CTeamControlPoint *pPoint = pMaster->GetControlPoint( i );
		if ( pMaster->IsInRound( pPoint ) &&
			ObjectiveResource()->GetOwningTeam( pPoint->GetPointIndex() ) == pPlayer->GetTeamNumber() &&
			ObjectiveResource()->TeamCanCapPoint( pPoint->GetPointIndex(), GetEnemyTeam( pPlayer ) ) &&
			TeamMayCapturePoint( GetEnemyTeam( pPlayer ), pPoint->GetPointIndex() ) )
		{
			controlPointVector->AddToTail( pPoint );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTeamTrainWatcher *CTFGameRules::GetPayloadToPush( int iTeam )
{
	if ( m_nGameType != TF_GAMETYPE_ESCORT )
		return nullptr;

	if ( iTeam == TF_TEAM_RED )
	{
		if ( m_hRedAttackTrain || HasMultipleTrains() )
			return m_hRedAttackTrain;
	}

	if ( iTeam != TF_TEAM_BLUE )
		return nullptr;

	if ( m_hBlueAttackTrain || HasMultipleTrains() )
		return m_hBlueAttackTrain;

	CTeamTrainWatcher *pWatcher = dynamic_cast<CTeamTrainWatcher *>( gEntList.FindEntityByClassname( NULL, "team_train_watcher" ) );
	while ( pWatcher )
	{
		if ( !pWatcher->IsDisabled() )
		{
			m_hBlueAttackTrain = pWatcher;
			return m_hBlueAttackTrain;
		}

		pWatcher = dynamic_cast<CTeamTrainWatcher *>( gEntList.FindEntityByClassname( pWatcher, "team_train_watcher" ) );
	}

	return m_hBlueAttackTrain;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTeamTrainWatcher *CTFGameRules::GetPayloadToBlock( int iTeam )
{
	if ( m_nGameType != TF_GAMETYPE_ESCORT )
		return nullptr;

	if ( iTeam == TF_TEAM_BLUE )
	{
		if ( m_hBlueDefendTrain || HasMultipleTrains() )
			return m_hBlueDefendTrain;
	}

	if ( iTeam != TF_TEAM_RED )
		return nullptr;

	if ( m_hRedDefendTrain || HasMultipleTrains() )
		return m_hRedAttackTrain;

	CTeamTrainWatcher *pWatcher = dynamic_cast<CTeamTrainWatcher *>( gEntList.FindEntityByClassname( NULL, "team_train_watcher" ) );
	while ( pWatcher )
	{
		if ( !pWatcher->IsDisabled() )
		{
			m_hRedDefendTrain = pWatcher;
			return m_hRedDefendTrain;
		}

		pWatcher = dynamic_cast<CTeamTrainWatcher *>( gEntList.FindEntityByClassname( pWatcher, "team_train_watcher" ) );
	}

	return m_hRedDefendTrain;
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
		( pRoundStats->m_iStat[TFSTAT_BONUS] / TF_SCORE_BONUS_PER_POINT );
	return max( iScore, 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::IsBirthday( void )
{
	if ( IsX360() )
		return false;

	if ( m_iBirthdayMode == BIRTHDAY_RECALCULATE )
	{
		m_iBirthdayMode = BIRTHDAY_OFF;
		if ( tf_birthday.GetBool() )
		{
			m_iBirthdayMode = BIRTHDAY_ON;
		}
		else
		{
			time_t ltime = time( 0 );
			const time_t *ptime = &ltime;
			struct tm *today = localtime( ptime );
			if ( today )
			{
				if ( ( today->tm_mon == 7 && today->tm_mday == 4 )  || ( today->tm_mon == 8 && today->tm_mday == 24 ) )
				{
					m_iBirthdayMode = BIRTHDAY_ON;
				}
			}
		}
	}

	return ( m_iBirthdayMode == BIRTHDAY_ON );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFGameRules::IsHolidayActive( int eHoliday )
{
	bool bActive = false;
	switch ( eHoliday )
	{
		case kHoliday_TF2Birthday:
			bActive = IsBirthday();
			break;
		case kHoliday_Halloween:
			bActive = tf_halloween.GetBool();
			break;
		case kHoliday_Christmas:
			bActive = tf_christmas.GetBool();
			break;
		default:
			break;
	}

	return bActive;
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

	if ( ( collisionGroup0 == TF_COLLISIONGROUP_GRENADES ) &&
		( collisionGroup1 == TFCOLLISION_GROUP_ROCKETS ) )
		return false;

	// Grenades don't collide with players. They handle collision while flying around manually.
	if ( ( collisionGroup0 == COLLISION_GROUP_PLAYER ) &&
		( collisionGroup1 == TF_COLLISIONGROUP_GRENADES ) )
		return false;

	if ( ( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT ) &&
		( collisionGroup1 == TF_COLLISIONGROUP_GRENADES ) )
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
	if ( ( collisionGroup0 == TF_COLLISIONGROUP_GRENADES ) &&
		( collisionGroup1 == TF_COLLISIONGROUP_GRENADES ) )
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

	if ( ( collisionGroup0 == TF_COLLISIONGROUP_GRENADES ) &&
		( collisionGroup1 == TFCOLLISION_GROUP_PUMPKIN_BOMB ) )
		return false;

	if ( ( collisionGroup1 == TFCOLLISION_GROUP_PUMPKIN_BOMB ) &&
		( collisionGroup0 == TFCOLLISION_GROUP_PUMPKIN_BOMB ) || ( collisionGroup0 == TFCOLLISION_GROUP_ROCKETS ) )
		return false;

	if ( ( collisionGroup1 == TFCOLLISION_GROUP_PUMPKIN_BOMB ) &&
		( collisionGroup0 == COLLISION_GROUP_WEAPON ) || ( collisionGroup0 == COLLISION_GROUP_PROJECTILE ) )
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
		CArenaLogic *pArena = dynamic_cast<CArenaLogic *>( gEntList.FindEntityByClassname( NULL, "tf_logic_arena" ) );
		if ( pArena )
		{
			pArena->FireOnCapEnabled();
		}
	}
#endif

#ifdef CLIENT_DLL
	if ( !Q_strcmp( eventName, "game_newmap" ) )
	{
		m_iBirthdayMode = BIRTHDAY_RECALCULATE;
	}
#endif
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

	if ( State_Get() == GR_STATE_STARTGAME )
	{
		m_iBirthdayMode = BIRTHDAY_RECALCULATE;
	}
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
				return "TF2V (Koth)";

			return "TF2V (CP)";
			break;
		case TF_GAMETYPE_ESCORT:
			return "TF2V (Payload)";
			break;
		case TF_GAMETYPE_ARENA:
			return "TF2V (Arena)";
			break;
		case TF_GAMETYPE_MVM:
			return "Implying we will ever have this";
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

void CTFGameRules::ModifySentChat( char *pBuf, int iBufSize )
{
	// Medieval mode only
	/*if ( !IsInMedievalMode() || !tf_medieval_autorp.GetBool() )
		return;

	if ( !AutoRP() )
	{
		Warning( "AutoRP initialization failed!" );
		return;
	}

	AutoRP()->ApplyRPTo( pBuf, iBufSize );

	int i = 0;
	while ( pBuf[i] )
	{
		if ( pBuf[i] == '"' )
		{
			pBuf[i] = '\'';
		}
		i++;
	}*/
}

void AddSubKeyNamed( KeyValues *pKeys, const char *pszName )
{
	KeyValues *pKeyvalToAdd = new KeyValues( pszName );

	if ( pKeyvalToAdd )
		pKeys->AddSubKey( pKeyvalToAdd );
}
#endif