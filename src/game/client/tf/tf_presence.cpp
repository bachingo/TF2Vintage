//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Rich Presence support.
// HACK: This file has also become the client wing of matchmaking. Matchmaking should
// be re-factored to make a more complete client/server/engine interface
//
//=====================================================================================//

#include "cbase.h"
#include "tf_presence.h"
#include "c_team_objectiveresource.h"
#include "tf_gamerules.h"
#include "c_tf_team.h"
#include "c_tf_playerresource.h"
#include "engine/imatchmaking.h"
#include "ixboxsystem.h"
#include "fmtstr.h"
#include "steam/steamclientpublic.h"
#include "steam/isteammatchmaking.h"
#include "steam/isteamgameserver.h"
#include "steam/isteamfriends.h"
#include "steam/steam_api.h"
#include "tier0/icommandline.h"
#include "discord_rpc.h"
#include "discord_register.h"
#include <time.h>
#include <functional>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Global singleton
static CTFPresence s_presence;
static CTFDiscordPresence s_drp;
CTFDiscordPresence *rpc = &s_drp;

struct s_MapName
{
	const char	*pDiskName;
	const char	*pDisplayName;
};

// This array must match the define order in hl2orange.spa.h
static s_MapName s_Scenarios[] = {
								"ctf_2fort",	"2Fort",
								"cp_dustbowl",	"Dustbowl",
								"cp_granary",	"Granary",
								"cp_well",		"Well",
								"cp_gravelpit", "Gravel Pit",
								"tc_hydro",		"Hydro",
								"cloak",		"Cloak (CTF)",
								"cp_cloak",		"Cloak (CP)",
};

struct s_PresenceTranslation
{
	uint		id;
	const char	*pString;
};

// Only presence IDs can be searched by id number, because they're guaranteed to be unique
static s_PresenceTranslation s_PresenceIds[] = {
			CONTEXT_SCENARIO,				 			"CONTEXT_SCENARIO",							
			PROPERTY_CAPS_OWNED,			 			"PROPERTY_CAPS_OWNED",						
			PROPERTY_CAPS_TOTAL,			 			"PROPERTY_CAPS_TOTAL",						
			PROPERTY_FLAG_CAPTURE_LIMIT,	 			"PROPERTY_FLAG_CAPTURE_LIMIT",				
			PROPERTY_NUMBER_OF_ROUNDS,		 			"PROPERTY_NUMBER_OF_ROUNDS",				
			PROPERTY_WIN_LIMIT,							"PROPERTY_WIN_LIMIT",				
			PROPERTY_GAME_SIZE,				 			"PROPERTY_GAME_SIZE",						
			PROPERTY_AUTOBALANCE,			 			"PROPERTY_AUTOBALANCE",						
			PROPERTY_PRIVATE_SLOTS,			 			"PROPERTY_PRIVATE_SLOTS",					
			PROPERTY_MAX_GAME_TIME,			 			"PROPERTY_MAX_GAME_TIME",
			PROPERTY_NUMBER_OF_TEAMS,					"PROPERTY_NUMBER_OF_TEAMS",
			PROPERTY_TEAM,								"PROPERTY_TEAM",
#if defined( _X360 )
			X_CONTEXT_GAME_MODE,					  	"CONTEXT_GAME_MODE",						
			X_CONTEXT_GAME_TYPE,					  	"CONTEXT_GAME_TYPE",						
#endif
};

// Presence values cannot be searched by id number, because they are not unique
static s_PresenceTranslation s_PresenceValues[] = {
	SESSION_MATCH_QUERY_PLAYER_MATCH,			"SESSION_MATCH_QUERY_PLAYER_MATCH",			
	CONTEXT_GAME_MODE_MULTIPLAYER,	 			"CONTEXT_GAME_MODE_MULTIPLAYER",			
	CONTEXT_SCENARIO_CTF_2FORT,		 			"CONTEXT_SCENARIO_CTF_2FORT",				
	CONTEXT_SCENARIO_CP_DUSTBOWL,	 			"CONTEXT_SCENARIO_CP_DUSTBOWL",				
	CONTEXT_SCENARIO_CP_GRANARY,	 			"CONTEXT_SCENARIO_CP_GRANARY",				
	CONTEXT_SCENARIO_CP_WELL,		 			"CONTEXT_SCENARIO_CP_WELL",					
	CONTEXT_SCENARIO_CP_GRAVELPIT,	 			"CONTEXT_SCENARIO_CP_GRAVELPIT",			
	CONTEXT_SCENARIO_TC_HYDRO,		 			"CONTEXT_SCENARIO_TC_HYDRO",				
	CONTEXT_SCENARIO_CTF_CLOAK,		 			"CONTEXT_SCENARIO_CTF_CLOAK",				
	CONTEXT_SCENARIO_CP_CLOAK,		 			"CONTEXT_SCENARIO_CP_CLOAK",				
#if defined( _X360 )
	XSESSION_CREATE_LIVE_MULTIPLAYER_STANDARD,	"SESSION_CREATE_LIVE_MULTIPLAYER_STANDARD",	
	XSESSION_CREATE_LIVE_MULTIPLAYER_RANKED,  	"SESSION_CREATE_LIVE_MULTIPLAYER_RANKED",	
	XSESSION_CREATE_SYSTEMLINK,				  	"SESSION_CREATE_SYSTEMLINK",				
	X_CONTEXT_GAME_TYPE_STANDARD,			  	"CONTEXT_GAME_TYPE_STANDARD",					
	X_CONTEXT_GAME_TYPE_RANKED,				  	"CONTEXT_GAME_TYPE_RANKED",						
#endif
};


//-----------------------------------------------------------------------------
// Discord RPC
//-----------------------------------------------------------------------------
extern ConVar cl_discord_appid;
ConVar cl_discord_presence_enabled( "cl_discord_presence_enabled", "1", FCVAR_ARCHIVE | FCVAR_NOT_CONNECTED );

struct DRPClassImages_t
{
	const char *redTeamImage;
	const char *redTeamImageDead;
	const char *bluTeamImage;
	const char *bluTeamImageDead;
};

static const DRPClassImages_t s_pClassImages[TF_CLASS_COUNT_ALL] =
{
	{ "tf2v_drp_logo",	"tf2v_drp_logo",		"tf2v_drp_logo",	"tf2v_drp_logo"			},
	{ "scout_red",		"scout_red_gray",		"scout_blue",		"scout_blue_gray"		},
	{ "sniper_red",		"sniper_red_gray",		"sniper_blue",		"sniper_blue_gray"		},
	{ "soldier_red",	"soldier_red_gray",		"soldier_blue",		"soldier_blue_gray"		},
	{ "demoman_red",	"demoman_red_gray",		"demoman_blue",		"demoman_blue_gray"		},
	{ "medic_red",		"medic_red_gray",		"medic_blue",		"medic_blue_gray"		},
	{ "heavy_red",		"heavy_red_gray",		"heavy_blue",		"heavy_blue_gray"		},
	{ "pyro_red",		"pyro_red_gray",		"pyro_blue",		"pyro_blue_gray"		},
	{ "spy_red",		"spy_red_gray",			"spy_blue",			"spy_blue_gray"			},
	{ "engineer_red",	"engineer_red_gray",	"engineer_blue",	"engineer_blue_gray"	},
	{ "tf2v_drp_logo",	"tf2v_drp_logo",		"tf2v_drp_logo",	"tf2v_drp_logo"			}
};

//-----------------------------------------------------------------------------
// Convert a map name to a defined ID.
//-----------------------------------------------------------------------------
static unsigned int GetMapID( const char *pMapName )
{
	for ( int i = 0; i < ARRAYSIZE( s_Scenarios ); ++i )
	{
		if ( !Q_stricmp( s_Scenarios[i].pDiskName, pMapName ) )
		{
			return i;
		}
	}
	return 0;
}

//-----------------------------------------------------------------------------
// Convert a session property string to a display string for gameUI.
//-----------------------------------------------------------------------------
void CTFPresence::GetPropertyDisplayString( uint id, uint value, char *pOutput, int nBytes )
{
	const char *pDisplayString = "";

	switch( id )
	{
#if defined( _X360 )
	case X_CONTEXT_GAME_TYPE:
		switch( value )
		{
		case X_CONTEXT_GAME_TYPE_STANDARD:
			pDisplayString = "#TF_Unranked";
			break;

		case X_CONTEXT_GAME_TYPE_RANKED:
			pDisplayString = "#TF_Ranked";
			break;
		}
		break;
#endif
	case CONTEXT_SCENARIO:
		pDisplayString = s_Scenarios[value].pDisplayName;
		break;

	case PROPERTY_FLAG_CAPTURE_LIMIT:
	case PROPERTY_NUMBER_OF_ROUNDS:
	case PROPERTY_WIN_LIMIT:
		Q_snprintf( pOutput, nBytes, "%d", value ); 
		return;

	case PROPERTY_MAX_GAME_TIME:
		if ( value >= NO_TIME_LIMIT )
		{
			Q_strncpy( pOutput, "#TF_MaxTimeNoLimit", nBytes );
		}
		else
		{
			Q_snprintf( pOutput, nBytes, "%d:00", value ); 
		}
		return;

	case PROPERTY_TEAM:
		switch ( value )
		{
		case 0:
			pDisplayString = "blue";
			break;

		case 1:
			pDisplayString = "red";
			break;

		case 2:
			pDisplayString = "spectator";
			break;
		}
		break;

	default:
		pDisplayString = "Unknown";
		break;
	}

	Q_strncpy( pOutput, pDisplayString, nBytes );
}

//-----------------------------------------------------------------------------
// Convert a presence ID to a string.
//-----------------------------------------------------------------------------
const char *CTFPresence::GetPropertyIdString( const uint id )
{
	for ( int i = 0; i < ARRAYSIZE( s_PresenceIds ); ++i )
	{
		if ( s_PresenceIds[i].id == id )
		{
			return s_PresenceIds[i].pString;
		}
	}
	return "Unknown";
}

//-----------------------------------------------------------------------------
// Convert a session property string to an ID.
//-----------------------------------------------------------------------------
uint CTFPresence::GetPresenceID( const char *pIDName )
{
	for ( int i = 0; i < ARRAYSIZE( s_PresenceIds ); ++i )
	{
		if ( !Q_stricmp( s_PresenceIds[i].pString, pIDName ) )
		{
			return s_PresenceIds[i].id;
		}
	}

	for ( int i = 0; i < ARRAYSIZE( s_PresenceValues ); ++i )
	{
		if ( !Q_stricmp( s_PresenceValues[i].pString, pIDName ) )
		{
			return s_PresenceValues[i].id;
		}
	}

	Warning( "Presence ID not found for %s\n", pIDName );
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Level init
//-----------------------------------------------------------------------------
void CTFPresence::LevelInitPreEntity( void )
{
	m_bIsInCommentary = false;
	const char *pMapName = MapName();
	if ( pMapName )
	{
		UserSetContext( XBX_GetPrimaryUserId(), CONTEXT_SCENARIO, GetMapID( pMapName ), true );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Init
//-----------------------------------------------------------------------------
bool CTFPresence::Init()
{
	presence = &s_presence;

	ListenForGameEvent( "controlpoint_initialized" );
	ListenForGameEvent( "controlpoint_updateowner" );
	ListenForGameEvent( "controlpoint_timer_updated" );
	ListenForGameEvent( "controlpoint_unlock_updated" );
	ListenForGameEvent( "teamplay_round_start" );
	ListenForGameEvent( "ctf_flag_captured" );
	ListenForGameEvent( "playing_commentary" );

	return CBasePresence::Init();
}

//-----------------------------------------------------------------------------
// Get game session properties from matchmaking.
//-----------------------------------------------------------------------------
void CTFPresence::SetupGameProperties( CUtlVector< XUSER_CONTEXT > &contexts, CUtlVector< XUSER_PROPERTY > &properties )
{
	// Session properties have been set for this game.  Use our knowledge of
	// the properties that have been defined for this game to set rules, cvars, etc.
	char buffer[MAX_PATH];

#if 0 // defined( _X360 ) // absolutely nothing happens in this loop, so I disabled it. It was breaking the compiler in LTCG mode. -egr
	int count = contexts.Count();
	for ( int i = 0; i < count; ++i )
	{
		XUSER_CONTEXT &ctx = contexts[i];
		switch( ctx.dwContextId )
		{
		case X_CONTEXT_GAME_TYPE:
			if ( ctx.dwValue == X_CONTEXT_GAME_TYPE_RANKED )
			{
			}
			else if ( ctx.dwValue == X_CONTEXT_GAME_TYPE_STANDARD )
			{
			}
			break;
		}
	}
#endif

	for ( int i = 0; i < properties.Count(); ++i )
	{
		XUSER_PROPERTY &prop = properties[i];
		switch( prop.dwPropertyId )
		{
		case PROPERTY_FLAG_CAPTURE_LIMIT:
			Q_snprintf( buffer, sizeof( buffer ), "tf_flag_caps_per_round %d", prop.value.nData );
			engine->ClientCmd( buffer );
			break;

		case PROPERTY_NUMBER_OF_ROUNDS:
			Q_snprintf( buffer, sizeof( buffer ), "mp_maxrounds %d", prop.value.nData );
			engine->ClientCmd( buffer );
			break;

		case PROPERTY_WIN_LIMIT:
			Q_snprintf( buffer, sizeof( buffer ), "mp_winlimit %d", prop.value.nData );
			engine->ClientCmd( buffer );
			break;

		case PROPERTY_GAME_SIZE:
			Q_snprintf( buffer, sizeof( buffer ), "maxplayers %d", prop.value.nData );
			engine->ClientCmd( buffer );
			break;

		case PROPERTY_AUTOBALANCE:
			Q_snprintf( buffer, sizeof( buffer ), "mp_autoteambalance %d", prop.value.nData );
			engine->ClientCmd( buffer );		
			break;

		case PROPERTY_MAX_GAME_TIME:
			Q_snprintf( buffer, sizeof( buffer ), "mp_timelimit %d", prop.value.nData );
			engine->ClientCmd( buffer );		
			break;

		}
	}
}


//-----------------------------------------------------------------------------
// Respond to TF game events.
//-----------------------------------------------------------------------------
void CTFPresence::FireGameEvent( IGameEvent *event )
{
	const char *eventname = event->GetName();

	if ( !Q_stricmp( "teamplay_round_start", eventname ) )
	{
		// Set presence for this map
		// TODO: Set appropriate presence mode based on game type
#if defined( _X360 )
		if ( TFGameRules() && !m_bIsInCommentary )
		{
			if ( TFGameRules()->GetGameType() == TF_GAMETYPE_CP )
			{
				UserSetContext( XBX_GetPrimaryUserId(), X_CONTEXT_PRESENCE, CONTEXT_PRESENCE_TF_CP, true );
			}
			else if ( TFGameRules()->GetGameType() == TF_GAMETYPE_CTF )
			{
				// ctf games start tied
				int zeroscore = 0;
				UserSetProperty( XBX_GetPrimaryUserId(), PROPERTY_PLAYER_TEAM_SCORE, sizeof(int), &zeroscore, true );
				UserSetProperty( XBX_GetPrimaryUserId(), PROPERTY_OPPONENT_TEAM_SCORE, sizeof(int), &zeroscore, true );
				UserSetContext( XBX_GetPrimaryUserId(), X_CONTEXT_PRESENCE, CONTEXT_PRESENCE_TF_CTF_TIED, true );
			}
		}
#endif
	}
	else if ( !Q_stricmp( "controlpoint_initialized", eventname ) )
	{
		int nPoints = ObjectiveResource()->GetNumControlPoints();
		int nOwned = ObjectiveResource()->GetNumControlPointsOwned();

		UserSetProperty( XBX_GetPrimaryUserId(), PROPERTY_CAPS_TOTAL, sizeof(int), &nPoints, true );
		UserSetProperty( XBX_GetPrimaryUserId(), PROPERTY_CAPS_OWNED, sizeof(int), &nOwned, true );
	}
	else if ( !Q_stricmp( "controlpoint_updateowner", eventname ) )
	{
		int nOwned = ObjectiveResource()->GetNumControlPointsOwned();
		UserSetProperty( XBX_GetPrimaryUserId(), PROPERTY_CAPS_OWNED, sizeof(int), &nOwned, true );
	}
	else if ( !Q_stricmp( "ctf_flag_captured", eventname ) )
	{
		C_TFTeam *pLocalTeam = GetGlobalTFTeam( GetLocalPlayerTeam() );
		
		if ( pLocalTeam )
		{
			int iOtherScore = 0;
			int iTeamScore = 0;
			int iCappingTeam = event->GetInt( "capping_team" );
			int iCappingTeamScore = event->GetInt( "capping_team_score" );

			// If the local player is on the team that just captured
			if ( iCappingTeam == pLocalTeam->GetTeamNumber() )
			{
				// the newly capped score is our current score
				iTeamScore = iCappingTeamScore;
			}
			else	// Other team capped
			{
				// Start other team score at the newly capped score set by the game event.
				// It can be higher than any we have locally recorded because of networking lag.
				iOtherScore = iCappingTeamScore;
				iTeamScore = pLocalTeam->GetFlagCaptures();
			}

			// highest score of any other team is the opposing score
			for ( int i = 0; i < g_Teams.Count(); i++ )
			{
				C_TFTeam* pCurTeam = ( dynamic_cast< C_TFTeam* >( g_Teams[i] ) );
				if ( pCurTeam )
				{
					if ( GetLocalPlayerTeam() == pCurTeam->GetTeamNumber() )
						continue;

					int iCurScore = pCurTeam->GetFlagCaptures();

					if ( iCurScore > iOtherScore )
					{
						iOtherScore = iCurScore;
					}
				}
			}

			UserSetProperty( XBX_GetPrimaryUserId(), PROPERTY_PLAYER_TEAM_SCORE, sizeof(int), &iTeamScore, true );
			UserSetProperty( XBX_GetPrimaryUserId(), PROPERTY_OPPONENT_TEAM_SCORE, sizeof(int), &iOtherScore, true );
#if defined ( _X360 )
			if ( !m_bIsInCommentary )
			{
				if ( iTeamScore > iOtherScore )
				{
					UserSetContext( XBX_GetPrimaryUserId(), X_CONTEXT_PRESENCE, CONTEXT_PRESENCE_TF_CTF_WINNING, true );
				}
				else if ( iOtherScore > iTeamScore )
				{
					UserSetContext( XBX_GetPrimaryUserId(), X_CONTEXT_PRESENCE, CONTEXT_PRESENCE_TF_CTF_LOSING, true );
				}
				else
				{
					UserSetContext( XBX_GetPrimaryUserId(), X_CONTEXT_PRESENCE, CONTEXT_PRESENCE_TF_CTF_TIED, true );
				}
			}
#endif 
		}	
	}
	else if ( !Q_stricmp( "playing_commentary", eventname ) )
	{
		m_bIsInCommentary = true;
#if defined ( _X360 )
		UserSetContext( XBX_GetPrimaryUserId(), X_CONTEXT_PRESENCE, CONTEXT_PRESENCE_COMMENTARY, true );
		UserSetContext( XBX_GetPrimaryUserId(), X_CONTEXT_GAME_MODE, CONTEXT_GAME_MODE_SINGLEPLAYER, true );
#endif 
	}
}


//-----------------------------------------------------------------------------
// Purpose: Upload player stats to Live.
//-----------------------------------------------------------------------------
void CTFPresence::UploadStats()
{
#if defined( _X360 )
	if ( m_bReportingStats )
	{
		m_ViewProperties[0].dwViewId = X_STATS_VIEW_SKILL;
		m_ViewProperties[1].dwViewId = m_bArbitrated ? STATS_VIEW_PLAYER_MAX_RANKED : STATS_VIEW_PLAYER_MAX_UNRANKED;
		m_ViewProperties[2].dwViewId = STATS_VIEW_PLAYER_MAX_UNRANKED;

		CUtlVector< XUSER_PROPERTY > skillStats;

		C_TF_PlayerResource *tf_PR = dynamic_cast<C_TF_PlayerResource *>( g_PR );
		if ( !tf_PR )
			return;

		XUID localId = matchmaking->PlayerIdToXuid( GetLocalPlayerIndex() );

		for ( int i = 1; i <= gpGlobals->maxClients; ++i )
		{
			XUID id = matchmaking->PlayerIdToXuid( i );
			if ( id == 0 )
				continue;

			// For non-ranked sessions, only the local player's stats are written
			if ( !m_bArbitrated && id != localId )
				continue;

			skillStats.RemoveAll();
			int viewCt = 1;

			if ( id != 0 )
			{
				Msg( "XUID: %d\n", id );
				XUSER_PROPERTY prop;

				int nScore = tf_PR->GetTotalScore( i );

				// Write the player's skill stats
				prop.dwPropertyId = X_PROPERTY_RELATIVE_SCORE;
				prop.value.type = XUSER_DATA_TYPE_INT32;
				prop.value.nData = nScore;
				skillStats.AddToTail( prop );

				prop.dwPropertyId = X_PROPERTY_SESSION_TEAM;
				prop.value.type = XUSER_DATA_TYPE_INT32;
				prop.value.nData = i;
				skillStats.AddToTail( prop );

				m_ViewProperties[0].dwNumProperties = skillStats.Count();
				m_ViewProperties[0].pProperties = skillStats.Base();

				Msg( "Skill:\n" );
				Msg( "Relative Score: %d\n" , skillStats[0].value.nData );
				Msg( "Team: %d\n" , skillStats[1].value.nData );

				if ( id != localId )
				{
					// Write the remote player's points scored
					prop.dwPropertyId = PROPERTY_POINTS_SCORED;
					prop.value.type = XUSER_DATA_TYPE_INT64;
					prop.value.nData = nScore;

					m_ViewProperties[1].dwNumProperties = 1;
					m_ViewProperties[1].pProperties = &prop;

					viewCt = 2;

					Msg( "Points Scored: %d\n" , prop.value.nData );
				}
				else
				{
					// Write the local player's points scored
					prop.dwPropertyId = PROPERTY_POINTS_SCORED;
					prop.value.type = XUSER_DATA_TYPE_INT64;
					prop.value.nData = nScore;
					m_ViewProperties[1].dwNumProperties = 1;
					m_ViewProperties[1].pProperties = &prop;

					// Include the local player's array of personal stats
					m_ViewProperties[2].dwNumProperties = m_PlayerStats.Count();
					m_ViewProperties[2].pProperties = m_PlayerStats.Base();

					viewCt = 3;

					Msg( "Points Scored: %d\n" , prop.value.nData );
					Msg( "Unranked stat count: %d\n", m_ViewProperties[2].dwNumProperties );
				}
			}

			DWORD ret = xboxsystem->WriteStats( m_hSession, id , viewCt, m_ViewProperties, false );
			if ( ret != ERROR_SUCCESS )
			{
				Warning( "Write stats failed with error %d\n", ret );
			}
		}

		m_PlayerStats.RemoveAll();
		m_bReportingStats = false;
	}
#endif
}

#define DECL_DISCORD_HANDLER(obj, name, handler)	obj.##name = &rpc->##handler

DiscordRichPresence CTFDiscordPresence::m_sPresence;
RealTimeCountdownTimer CTFDiscordPresence::m_updateThrottle;
int64 CTFDiscordPresence::m_iCreationTimestamp;

CTFDiscordPresence::CTFDiscordPresence()
	: BaseClass( "TFDiscordPresence" )
{
	Q_memset( m_szMapName, 0, sizeof( m_szMapName ) );
	m_iCreationTimestamp = time( NULL );

	ListenForGameEvent( "server_spawn" );
}

//-----------------------------------------------------------------------------
// Purpose: Catch certain events to update the presence
//-----------------------------------------------------------------------------
void CTFDiscordPresence::FireGameEvent( IGameEvent *event )
{
	bool bForceUpdate = false;
	bool bIsDead = false;
	CUtlString name = event->GetName();

	if ( name == "server_spawn" )
	{
		Q_strncpy( m_szHostName, event->GetString( "hostname" ), DISCORD_FIELD_MAXLEN );
		Q_strncpy( m_szServerInfo, event->GetString( "address" ), DISCORD_FIELD_MAXLEN );
	}

	if ( C_BasePlayer::GetLocalPlayer() == nullptr )
		return;
	
	if ( name == "client_fullconnect" )
	{
		CSteamID steamID{};
		if ( C_BasePlayer::GetLocalPlayer()->GetSteamID( &steamID ) )
			Q_snprintf( m_szSteamID, 65, "%d", steamID.ConvertToUint64() );

		m_sPresence.joinSecret = m_szServerInfo;
		m_sPresence.partyId = m_szSteamID;
	}

	if ( !engine->IsConnected() )
		return;

	if ( name == "player_connect_client" || name == "player_disconnect" )
	{
		if ( !TFPlayerResource() )
			return;

		int maxPlayers = gpGlobals->maxClients;
		int curPlayers = 0;

		for ( int i = 0; i < maxPlayers; ++i )
		{
			if ( TFPlayerResource()->IsConnected( i ) )
				curPlayers++;
		}

		m_sPresence.partySize = curPlayers;
		m_sPresence.partyMax = maxPlayers;
	}
	else if ( name == "player_death" )
	{
		int userid = event->GetInt( "userid" );
		if ( UTIL_PlayerByUserId( userid ) != C_BasePlayer::GetLocalPlayer() )
			return;

		if ( event->GetInt( "death_flags" ) & TF_DEATH_FEIGN_DEATH )
			return;

		bIsDead = true;
		bForceUpdate = true;
	}
	else
	{
		int userid = event->GetInt( "userid" );
		if ( UTIL_PlayerByUserId( userid ) != C_BasePlayer::GetLocalPlayer() )
			return;

		bForceUpdate = true;
	}

	UpdatePresence( bForceUpdate, bIsDead );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFDiscordPresence::Init( void )
{
	Q_memset( &m_sPresence, 0, sizeof( m_sPresence ) );

	DiscordEventHandlers handlers{};
	DECL_DISCORD_HANDLER( handlers, ready, OnReady );
	DECL_DISCORD_HANDLER( handlers, disconnected, OnDisconnected );
	DECL_DISCORD_HANDLER( handlers, errored, OnError );
	DECL_DISCORD_HANDLER( handlers, joinGame, OnJoinedGame );
	DECL_DISCORD_HANDLER( handlers, spectateGame, OnSpectateGame );
	DECL_DISCORD_HANDLER( handlers, joinRequest, OnJoinRequested );

	char command[512];
	V_snprintf( command, sizeof( command ), "%s -game \"%s\" -novid -steam\n", CommandLine()->GetParm( 0 ), CommandLine()->ParmValue( "-game" ) );
	Discord_Register( cl_discord_appid.GetString(), command );
	Discord_Initialize( cl_discord_appid.GetString(), &handlers, false, CFmtStr( "%d", engine->GetAppID() ) );

	ListenForGameEvent( "localplayer_changeteam" );
	ListenForGameEvent( "localplayer_changeclass" );
	ListenForGameEvent( "player_death" );
	ListenForGameEvent( "player_spawn" );
	ListenForGameEvent( "player_connect_client" );
	ListenForGameEvent( "player_disconnect" );
	ListenForGameEvent( "client_fullconnect" );
	ListenForGameEvent( "client_disconnect" );

	Reset();

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFDiscordPresence::Shutdown( void )
{
	Assert( rpc == this );
	rpc = NULL;

	Discord_Shutdown();

	if ( steamapicontext->SteamFriends() )
		steamapicontext->SteamFriends()->ClearRichPresence();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFDiscordPresence::Update( float frametime )
{
	if ( !engine->IsConnected() || m_szMapName[0] == '\0' )
		return;

	UpdatePresence();

	if ( gpGlobals->tickcount % 2 )
		Discord_RunCallbacks();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFDiscordPresence::OnReady( const DiscordUser* user )
{
	if (!cl_discord_presence_enabled.GetBool())
	{
		Discord_Shutdown();

		if ( steamapicontext->SteamFriends() )
			steamapicontext->SteamFriends()->ClearRichPresence();

		return;
	}

	ConColorMsg( Color( 114, 137, 218, 255 ), "[DRP] Ready!\n" );
	ConColorMsg( Color( 114, 137, 218, 255 ), "[DRP] User %s#%s - %s\n", user->username, user->discriminator, user->userId );
	
	rpc->Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFDiscordPresence::OnDisconnected( int errorCode, const char *szMessage )
{
	Warning( "[DRP] Rich presence disconnected. code %d - reason: %s\n", errorCode, szMessage );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFDiscordPresence::OnError( int errorCode, const char *szMessage )
{
	Warning( "[DRP] Rich presence failure. code %d - error: %s\n", errorCode, szMessage );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFDiscordPresence::OnJoinedGame( const char *joinSecret )
{
	ConColorMsg( Color( 114, 137, 218, 255 ), "[DRP] Join Game: %s\n", joinSecret );
	char szCommand[128];
	Q_snprintf( szCommand, sizeof( szCommand ), "connect %s\n", joinSecret );
	engine->ExecuteClientCmd( szCommand );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFDiscordPresence::OnSpectateGame( const char *spectateSecret )
{
	ConColorMsg( Color( 114, 137, 218, 255 ), "[DRP] Spectate Game: %s\n", spectateSecret );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFDiscordPresence::OnJoinRequested( const DiscordUser *joinRequest )
{
	// TODO: Popup dialog
	ConColorMsg( Color( 114, 137, 218, 255 ), "[DRP] Join Request: %s#%s\n", joinRequest->username, joinRequest->discriminator );
	ConColorMsg( Color( 114, 137, 218, 255 ), "[Rich Presence] Join Request Accepted\n" );
	Discord_Respond( joinRequest->userId, DISCORD_REPLY_YES );
}

//-----------------------------------------------------------------------------
// Purpose: Map initialization
//-----------------------------------------------------------------------------
void CTFDiscordPresence::LevelInitPostEntity( void )
{
	Q_memset( &m_sPresence, 0, sizeof( m_sPresence ) );

	char buffer[64];
	Q_snprintf( buffer, sizeof( buffer ), "#TF_Map_%s", m_szMapName );
	wchar *mapName = g_pVGuiLocalize->Find( buffer );
	if ( mapName )
	{
		g_pVGuiLocalize->ConvertUnicodeToANSI( mapName, buffer, sizeof( buffer ) );
		Q_snprintf( m_szGameState, sizeof( m_szGameState ), "Map: %s", buffer );
		m_sPresence.largeImageKey = m_szMapName;
	}
	else
	{
		Q_snprintf( m_szGameState, sizeof( m_szGameState ), "Map: %s", m_szMapName );
		m_sPresence.largeImageKey = "default";
	}


	if ( TFGameRules() )
	{
		wchar *gameType = g_pVGuiLocalize->Find( g_aGameTypeNames[ TFGameRules()->GetGameType() ] );
		if ( gameType )
		{
			g_pVGuiLocalize->ConvertUnicodeToANSI( gameType, m_szGameType, DISCORD_FIELD_MAXLEN );
			m_sPresence.largeImageText = m_szGameType;
		}
	}

	m_sPresence.details = m_szHostName;
	m_sPresence.state = m_szGameState;
	m_sPresence.smallImageKey = "tf2v_drp_logo";
	m_sPresence.startTimestamp = m_iCreationTimestamp;

	if ( steamapicontext->SteamFriends() )
	{
		steamapicontext->SteamFriends()->SetRichPresence( "connect", NULL );
		steamapicontext->SteamFriends()->SetRichPresence( "steam_player_group", NULL );
		steamapicontext->SteamFriends()->SetRichPresence( "steam_player_group_size", NULL );
		steamapicontext->SteamFriends()->SetRichPresence( "status", m_szHostName );
		steamapicontext->SteamFriends()->SetRichPresence( "steam_display", m_szMapName );
	}

	Discord_UpdatePresence( &m_sPresence );
}

//-----------------------------------------------------------------------------
// Purpose: Reset map details
//-----------------------------------------------------------------------------
void CTFDiscordPresence::LevelShutdownPreEntity( void )
{
	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: Revert to default state
//-----------------------------------------------------------------------------
void CTFDiscordPresence::Reset( void )
{
	Q_memset( &m_sPresence, 0, sizeof( m_sPresence ) );

	if ( steamapicontext->SteamFriends() )
	{
		steamapicontext->SteamFriends()->SetRichPresence( "status", "Main Menu" );
		steamapicontext->SteamFriends()->SetRichPresence( "connect", NULL );
		steamapicontext->SteamFriends()->SetRichPresence( "steam_display", "Main Menu" );
		steamapicontext->SteamFriends()->SetRichPresence( "steam_player_group", NULL );
		steamapicontext->SteamFriends()->SetRichPresence( "steam_player_group_size", NULL );
	}

	m_sPresence.details = "Main Menu";
	m_sPresence.largeImageKey = "tf2v_drp_logo";
	m_sPresence.startTimestamp = m_iCreationTimestamp;
	Discord_UpdatePresence( &m_sPresence );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFDiscordPresence::UpdatePresence( bool bForce, bool bIsDead )
{
	if ( !m_updateThrottle.IsElapsed() && !bForce )
		return;

	m_updateThrottle.Start( 8.0f );

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
		return;

	Q_strncpy( m_szClassName, pLocalPlayer->GetPlayerClass()->GetName(), DISCORD_FIELD_MAXLEN );
	Q_snprintf( m_szClassName, DISCORD_FIELD_MAXLEN, "%s%s", m_szClassName, bIsDead ? " (Dead)" : "" );

	const int iClassNum = pLocalPlayer->GetPlayerClass()->GetClassIndex();
	switch ( pLocalPlayer->GetTeamNumber() )
	{
		case TF_TEAM_RED:
		{
			m_sPresence.smallImageKey = s_pClassImages[iClassNum].redTeamImage;
			m_sPresence.smallImageText = m_szClassName;
			break;
		}
		case TF_TEAM_BLUE:
		{
			m_sPresence.smallImageKey = s_pClassImages[iClassNum].bluTeamImage;
			m_sPresence.smallImageText = m_szClassName;
			break;
		}
		default:
			break;
	}

	Discord_UpdatePresence( &m_sPresence );
}
