//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Rich Presence support.
// HACK: This file has also become the client wing of matchmaking. Matchmaking should
// be re-factored to make a more complete client/server/engine interface
//
//=====================================================================================//

#include "cbase.h"
#ifndef POSIX
#include "discord.h"
#endif
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
#include "mathlib/IceKey.H"
#include <inetchannelinfo.h>
#include "tf_gc_client.h"
#include "tf_partyclient.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Global singleton
static CTF_Presence s_presence;

struct s_MapName
{
	const char	*pDiskName;
	const char	*pDisplayName;
};

// This array must match the define order in hl2orange.spa.h
static s_MapName s_Scenarios[] = {
								{ "ctf_2fort",	"2Fort" },
								{ "cp_dustbowl",	"Dustbowl" },
								{ "cp_granary",	"Granary" },
								{ "cp_well",		"Well" },
								{ "cp_gravelpit", "Gravel Pit" },
								{ "tc_hydro",		"Hydro" },
								{ "cloak",		"Cloak (CTF)" },
								{ "cp_cloak",		"Cloak (CP)" },
};

struct s_PresenceTranslation
{
	uint		id;
	const char	*pString;
};

// Only presence IDs can be searched by id number, because they're guaranteed to be unique
static s_PresenceTranslation s_PresenceIds[] = {
			{ CONTEXT_SCENARIO,				 			"CONTEXT_SCENARIO" },
			{ PROPERTY_CAPS_OWNED,			 			"PROPERTY_CAPS_OWNED" },
			{ PROPERTY_CAPS_TOTAL,			 			"PROPERTY_CAPS_TOTAL" },
			{ PROPERTY_FLAG_CAPTURE_LIMIT,	 			"PROPERTY_FLAG_CAPTURE_LIMIT" },
			{ PROPERTY_NUMBER_OF_ROUNDS,	 			"PROPERTY_NUMBER_OF_ROUNDS" },
			{ PROPERTY_WIN_LIMIT,						"PROPERTY_WIN_LIMIT" },
			{ PROPERTY_GAME_SIZE,						"PROPERTY_GAME_SIZE" },
			{ PROPERTY_AUTOBALANCE,			 			"PROPERTY_AUTOBALANCE" },
			{ PROPERTY_PRIVATE_SLOTS,		 			"PROPERTY_PRIVATE_SLOTS" },
			{ PROPERTY_MAX_GAME_TIME,		 			"PROPERTY_MAX_GAME_TIME" },
			{ PROPERTY_NUMBER_OF_TEAMS,					"PROPERTY_NUMBER_OF_TEAMS" },
			{ PROPERTY_TEAM,							"PROPERTY_TEAM" },
#if defined( _X360 )
			{ X_CONTEXT_GAME_MODE,					  	"CONTEXT_GAME_MODE" },
			{ X_CONTEXT_GAME_TYPE,					  	"CONTEXT_GAME_TYPE" },
#endif
};

// Presence values cannot be searched by id number, because they are not unique
static s_PresenceTranslation s_PresenceValues[] = {
	{ SESSION_MATCH_QUERY_PLAYER_MATCH,				"SESSION_MATCH_QUERY_PLAYER_MATCH" },
	{ CONTEXT_GAME_MODE_MULTIPLAYER,	 			"CONTEXT_GAME_MODE_MULTIPLAYER" },
	{ CONTEXT_SCENARIO_CTF_2FORT,		 			"CONTEXT_SCENARIO_CTF_2FORT" },
	{ CONTEXT_SCENARIO_CP_DUSTBOWL,	 				"CONTEXT_SCENARIO_CP_DUSTBOWL" },
	{ CONTEXT_SCENARIO_CP_GRANARY,	 				"CONTEXT_SCENARIO_CP_GRANARY" },
	{ CONTEXT_SCENARIO_CP_WELL,		 				"CONTEXT_SCENARIO_CP_WELL" },
	{ CONTEXT_SCENARIO_CP_GRAVELPIT,	 			"CONTEXT_SCENARIO_CP_GRAVELPIT" },
	{ CONTEXT_SCENARIO_TC_HYDRO,		 			"CONTEXT_SCENARIO_TC_HYDRO" },
	{ CONTEXT_SCENARIO_CTF_CLOAK,		 			"CONTEXT_SCENARIO_CTF_CLOAK" },
	{ CONTEXT_SCENARIO_CP_CLOAK,		 			"CONTEXT_SCENARIO_CP_CLOAK" },
#if defined( _X360 )
	{ XSESSION_CREATE_LIVE_MULTIPLAYER_STANDARD,	"SESSION_CREATE_LIVE_MULTIPLAYER_STANDARD" },
	{ XSESSION_CREATE_LIVE_MULTIPLAYER_RANKED,  	"SESSION_CREATE_LIVE_MULTIPLAYER_RANKED" },
	{ XSESSION_CREATE_SYSTEMLINK,				  	"SESSION_CREATE_SYSTEMLINK" },
	{ X_CONTEXT_GAME_TYPE_STANDARD,			  		"CONTEXT_GAME_TYPE_STANDARD" },
	{ X_CONTEXT_GAME_TYPE_RANKED,				  	"CONTEXT_GAME_TYPE_RANKED" },
#endif
};


//-----------------------------------------------------------------------------
// Discord RPC
//-----------------------------------------------------------------------------
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
void CTF_Presence::GetPropertyDisplayString( uint id, uint value, char *pOutput, int nBytes )
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
const char *CTF_Presence::GetPropertyIdString( const uint id )
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
uint CTF_Presence::GetPresenceID( const char *pIDName )
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
void CTF_Presence::LevelInitPreEntity( void )
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
bool CTF_Presence::Init()
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
void CTF_Presence::SetupGameProperties( CUtlVector< XUSER_CONTEXT > &contexts, CUtlVector< XUSER_PROPERTY > &properties )
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
void CTF_Presence::FireGameEvent( IGameEvent *event )
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
void CTF_Presence::UploadStats()
{
#if defined( _X360 )
	if ( m_bReportingStats )
	{
		m_ViewProperties[0].dwViewId = X_STATS_VIEW_SKILL;
		m_ViewProperties[1].dwViewId = m_bArbitrated ? STATS_VIEW_PLAYER_MAX_RANKED : STATS_VIEW_PLAYER_MAX_UNRANKED;
		m_ViewProperties[2].dwViewId = STATS_VIEW_PLAYER_MAX_UNRANKED;

		CUtlVector< XUSER_PROPERTY > skillStats;

		if ( !g_TF_PR )
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

				int nScore = g_TF_PR->GetTotalScore( i );

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

#ifndef POSIX
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
static CTFDiscordPresence s_drp;

#define DISCORD_COLOR Color( 114, 137, 218, 255 )


CTFDiscordPresence::CTFDiscordPresence()
	: m_szHostName(""), m_szServerInfo(""), m_szSteamID(""), m_szTeamPref(""), m_nSourceTVPort(27020)
{
	VCRHook_Time( &m_iCreationTimestamp );
	m_flLastPlayerJoinTime = 0;

	rpc = this;
}

//-----------------------------------------------------------------------------
// Purpose: Catch certain events to update the presence
//-----------------------------------------------------------------------------
void CTFDiscordPresence::FireGameEvent( IGameEvent *event )
{
	bool bIsDead = false;
	const char *name = event->GetName();

	if ( g_pDiscord == NULL )
		return;

	if ( FStrEq( name, "server_spawn" ) )
	{
		Q_strncpy( m_szHostName, event->GetString( "hostname" ), DISCORD_FIELD_MAXLEN );
		Q_strncpy( m_szServerInfo, event->GetString( "address" ), DISCORD_FIELD_MAXLEN );

		// Read the SourceTV port from the event (engine provides it as "tvport").
		// Default to 27020 when the field is absent (e.g. older server builds).
		m_nSourceTVPort = event->GetInt( "tvport", 27020 );
		if ( m_nSourceTVPort <= 0 )
			m_nSourceTVPort = 27020;

		m_Activity.GetSecrets().SetJoin( GetJoinSecret() );
		m_Activity.GetSecrets().SetSpectate( GetSpectateSecret() );

		// Also sync the party ID so the Steam "Join Game" overlay works.
		// Use the party ID if the player is in a GC party; fall back to SteamID.
		if ( GTFPartyClient()->BHaveActiveParty() )
		{
			char szPartyID[ 32 ];
			V_snprintf( szPartyID, sizeof( szPartyID ), "party_%llu", GTFPartyClient()->GetActivePartyID() );
			m_Activity.GetParty().SetId( szPartyID );
		}

		g_pDiscord->ActivityManager().UpdateActivity( m_Activity, &OnActivityUpdate );
	}

	if ( !engine->IsConnected() )
		return;

	if ( FStrEq( name, "localplayer_changeteam" ) )
	{
		// Keep our team preference current so the join secret stays accurate.
		m_szTeamPref[0] = '\0';
		C_TFPlayer *pLocal = C_TFPlayer::GetLocalTFPlayer();
		if ( pLocal )
		{
			switch ( pLocal->GetTeamNumber() )
			{
				case TF_TEAM_RED:  V_strncpy( m_szTeamPref, "red",  sizeof( m_szTeamPref ) ); break;
				case TF_TEAM_BLUE: V_strncpy( m_szTeamPref, "blue", sizeof( m_szTeamPref ) ); break;
				default: break;
			}
		}
		// Refresh join secret so next invitee gets the current team hint.
		m_Activity.GetSecrets().SetJoin( GetJoinSecret() );
		g_pDiscord->ActivityManager().UpdateActivity( m_Activity, &OnActivityUpdate );
	}

	if ( FStrEq( name, "player_connect" ) || FStrEq( name, "player_disconnect" ) )
	{
		if ( !g_TF_PR )
			return;

		const int maxPlayers = gpGlobals->maxClients;
		int curPlayers = 0;

		for ( int i = 1; i <= maxPlayers; ++i )
		{
			if ( g_TF_PR->IsConnected( i ) )
				curPlayers++;
		}

		// On map change *all* connected players are reconnected, prevent rate limits here
		if ( ( gpGlobals->curtime - m_flLastPlayerJoinTime ) < 1.0f )
		{
			m_nPlayerCount = curPlayers;
			return;
		}

		m_Activity.GetParty().GetSize().SetCurrentSize( curPlayers );
		m_Activity.GetParty().GetSize().SetMaxSize( maxPlayers );

		m_flLastPlayerJoinTime = gpGlobals->curtime;
	}
	else if ( FStrEq( name, "player_death" ) )
	{
		int userid = event->GetInt( "userid" );
		if ( UTIL_PlayerByUserId( userid ) != C_BasePlayer::GetLocalPlayer() )
			return;

		if ( event->GetInt( "death_flags" ) & TF_DEATH_FEIGN_DEATH )
			return;

		bIsDead = true;
	}

	UpdatePresence( bIsDead );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFDiscordPresence::Init( void )
{
	ListenForGameEvent( "server_spawn" );
	ListenForGameEvent( "localplayer_changeteam" );
	ListenForGameEvent( "localplayer_changeclass" );
	ListenForGameEvent( "localplayer_respawn" );
	ListenForGameEvent( "player_death" );
	ListenForGameEvent( "player_connect" );
	ListenForGameEvent( "player_disconnect" );

	return BaseClass::Init();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFDiscordPresence::InitPresence( void )
{
	if ( g_pDiscord == NULL )
		return true;

	g_pDiscord->SetLogHook(
	#ifdef DEBUG
		discord::LogLevel::Debug,
	#else
		discord::LogLevel::Warn,
	#endif
		&OnLogMessage
	);

	Q_memset( &m_CurrentUser, 0, sizeof( discord::User ) );
	g_pDiscord->UserManager().OnCurrentUserUpdate.Connect( &OnReady );

	char command[512];
	V_snprintf( command, sizeof( command ), "%s -game \"%s\" -novid -steam", CommandLine()->GetParm( 0 ), CommandLine()->ParmValue( "-game" ) );
	g_pDiscord->ActivityManager().RegisterCommand( command );
	g_pDiscord->ActivityManager().RegisterSteam( engine->GetAppID() );

	g_pDiscord->ActivityManager().OnActivityJoin.Connect( &OnJoinedGame );
	g_pDiscord->ActivityManager().OnActivityJoinRequest.Connect( &OnJoinRequested );
	g_pDiscord->ActivityManager().OnActivitySpectate.Connect( &OnSpectateGame );

	CSteamID steamID{};
	if ( steamapicontext && steamapicontext->SteamUser() )
		steamID = steamapicontext->SteamUser()->GetSteamID();
	if ( steamID.BIndividualAccount() )
		V_sprintf_safe( m_szSteamID, "%llu", steamID.ConvertToUint64() );

	m_Activity.GetSecrets().SetMatch( GetMatchSecret() );
	m_Activity.GetParty().SetId( m_szSteamID );
	g_pDiscord->ActivityManager().UpdateActivity( m_Activity, &OnActivityUpdate );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFDiscordPresence::Shutdown( void )
{
	BaseClass::Shutdown();

	Q_memset( &m_Activity, 0, sizeof( discord::Activity ) );
	Q_memset( &m_CurrentUser, 0, sizeof( discord::User ) );

	if ( steamapicontext->SteamFriends() )
		steamapicontext->SteamFriends()->ClearRichPresence();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFDiscordPresence::OnReady()
{
	extern ConVar cl_discord_presence_enabled;
	if ( !cl_discord_presence_enabled.GetBool() )
	{
		if ( g_pDiscord )
		{
			delete g_pDiscord;
			g_pDiscord = NULL;
		}

		if ( steamapicontext->SteamFriends() )
			steamapicontext->SteamFriends()->ClearRichPresence();

		return;
	}

	discord::User user;
	g_pDiscord->UserManager().GetCurrentUser( &user );
	static_cast<CTFDiscordPresence *>( rpc )->SetCurrentUser( user );

	ConDColorMsg( DISCORD_COLOR, "[DRP] Ready!\n" );
	ConDColorMsg( DISCORD_COLOR, "[DRP] User %s#%s - %lld\n", user.GetUsername(), user.GetDiscriminator(), user.GetId() );

	rpc->ResetPresence();
}

//-----------------------------------------------------------------------------
// Purpose: Handles the Discord "join game" button.
// The join secret payload format is "ip:port" or "ip:port/red" / "ip:port/blue".
// We use GTFGCClientSystem()->ConnectToServer() so the GC matchmaking layer
// stays informed (records it as a recent match server, etc.).  If the GC client
// isn't available we fall back to a raw engine connect command.
// When a team hint is present we queue a "jointeam" after connection so the
// joiner ends up on the same team as the inviter.
//-----------------------------------------------------------------------------
void CTFDiscordPresence::OnJoinedGame( const char *joinSecret )
{
	char szDecoded[ DISCORD_FIELD_MAXLEN ];
	V_strncpy( szDecoded, joinSecret, sizeof( szDecoded ) );
	UTIL_DecodeICE( (unsigned char *)szDecoded, sizeof( szDecoded ), rpc->GetEncryptionKey() );
	ConDColorMsg( DISCORD_COLOR, "[DRP] Join Game: %s\n", szDecoded );

	// Split optional team hint: "ip:port/red" → addr="ip:port", team="red"
	char szAddr[ 64 ];
	char szTeam[ 16 ];
	szTeam[0] = '\0';

	char *pSlash = V_strrchr( szDecoded, '/' );
	if ( pSlash )
	{
		const char *pTeamPart = pSlash + 1;
		if ( FStrEq( pTeamPart, "red" ) || FStrEq( pTeamPart, "blue" ) )
		{
			V_strncpy( szTeam, pTeamPart, sizeof( szTeam ) );
			int nAddrLen = (int)( pSlash - szDecoded );
			V_strncpy( szAddr, szDecoded, MIN( nAddrLen + 1, (int)sizeof( szAddr ) ) );
		}
		else
		{
			V_strncpy( szAddr, szDecoded, sizeof( szAddr ) );
		}
	}
	else
	{
		V_strncpy( szAddr, szDecoded, sizeof( szAddr ) );
	}

	// Use the GC system's connect path when available — it records the server in
	// the recent-match-server list and appends the "matchmaking" connect flag so
	// the engine handshake completes correctly on insecure servers.
	CTFGCClientSystem *pGC = GTFGCClientSystem();
	if ( pGC && pGC->BConnectedtoGC() )
	{
		pGC->ConnectToServer( szAddr );
	}
	else
	{
		char szCommand[ 128 ];
		Q_snprintf( szCommand, sizeof( szCommand ), "connect %s\n", szAddr );
		engine->ExecuteClientCmd( szCommand );
	}

	// Queue a team join hint to fire ~3 seconds after connect (~180 ticks at 66Hz).
	// HandleCommand_JoinTeam on the server will honour team balance rules, so this
	// is best-effort rather than a guarantee.
	if ( szTeam[0] != '\0' )
	{
		char szJoinCmd[ 64 ];
		Q_snprintf( szJoinCmd, sizeof( szJoinCmd ), "wait 180; jointeam %s\n", szTeam );
		engine->ExecuteClientCmd( szJoinCmd );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handles the Discord "spectate" button.
// The spectate secret encodes "ip:stvport" so we connect directly to SourceTV
// without guessing the port.
//-----------------------------------------------------------------------------
void CTFDiscordPresence::OnSpectateGame( const char *spectateSecret )
{
	char szDecoded[ DISCORD_FIELD_MAXLEN ];
	V_strncpy( szDecoded, spectateSecret, sizeof( szDecoded ) );
	UTIL_DecodeICE( (unsigned char *)szDecoded, sizeof( szDecoded ), rpc->GetEncryptionKey() );
	ConDColorMsg( DISCORD_COLOR, "[DRP] Spectate Game: %s\n", szDecoded );

	// The secret already encodes "ip:stvport" — connect directly.
	char szCommand[ 128 ];
	Q_snprintf( szCommand, sizeof( szCommand ), "connect %s\n", szDecoded );
	engine->ExecuteClientCmd( szCommand );
}

//-----------------------------------------------------------------------------
// Purpose: Handles an incoming "ask to join" request from another Discord user.
// cl_discord_join_requests controls behaviour:
//   1 (default) — auto-accept (original behaviour, fine for casual public servers)
//   0           — silently decline (server/tournament use)
// A proper VGUI confirmation dialog is a future TODO.
//-----------------------------------------------------------------------------
static ConVar cl_discord_join_requests( "cl_discord_join_requests", "1", FCVAR_ARCHIVE,
	"Controls how Discord join requests are handled. 1=auto-accept, 0=auto-decline." );

void CTFDiscordPresence::OnJoinRequested( discord::User const &joinRequester )
{
	ConDColorMsg( DISCORD_COLOR, "[DRP] Join Request from %s#%s (id %lld)\n",
		joinRequester.GetUsername(), joinRequester.GetDiscriminator(), joinRequester.GetId() );

	if ( cl_discord_join_requests.GetBool() )
	{
		ConDColorMsg( DISCORD_COLOR, "[DRP] Join Request accepted (cl_discord_join_requests=1)\n" );
		g_pDiscord->ActivityManager().SendRequestReply(
			joinRequester.GetId(), discord::ActivityJoinRequestReply::Yes, &OnJoinRequestReply );
	}
	else
	{
		ConDColorMsg( DISCORD_COLOR, "[DRP] Join Request declined (cl_discord_join_requests=0)\n" );
		g_pDiscord->ActivityManager().SendRequestReply(
			joinRequester.GetId(), discord::ActivityJoinRequestReply::No, &OnJoinRequestReply );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFDiscordPresence::OnJoinRequestReply( discord::Result result )
{
	ConDColorMsg( DISCORD_COLOR, "[DRP] Join Request result: %d", result );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFDiscordPresence::OnLogMessage( discord::LogLevel logLevel, char const *pszMessage )
{
	switch ( logLevel )
	{
		case discord::LogLevel::Error:
		case discord::LogLevel::Warn:
			Warning( "[DRP] %s\n", pszMessage );
			break;
		default:
			ConDColorMsg( DISCORD_COLOR, "[DRP] %s\n", pszMessage );
			break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFDiscordPresence::OnActivityUpdate( discord::Result result )
{
	ConDColorMsg( DISCORD_COLOR, "[DRP] Activity update: %s\n", ( ( result == discord::Result::Ok ) ? "Succeeded" : "Failed" ) );
}

//-----------------------------------------------------------------------------
// Purpose: Map initialization
//-----------------------------------------------------------------------------
void CTFDiscordPresence::LevelInitPostEntity( void )
{
	Q_memset( &m_Activity, 0, sizeof( discord::Activity ) );

	// --- Large image + game type tooltip ---
	char buffer[64];
	Q_snprintf( buffer, sizeof( buffer ), "#TF_Map_%s", GetLevelName() );
	wchar *mapName = g_pVGuiLocalize->Find( buffer );
	char szMapDisplay[ DISCORD_FIELD_MAXLEN ];
	if ( mapName )
	{
		g_pVGuiLocalize->ConvertUnicodeToANSI( mapName, szMapDisplay, sizeof( szMapDisplay ) );
		m_Activity.GetAssets().SetLargeImage( GetLevelName() );
	}
	else
	{
		V_strncpy( szMapDisplay, GetLevelName(), sizeof( szMapDisplay ) );
		m_Activity.GetAssets().SetLargeImage( "default" );
	}

	if ( TFGameRules() )
	{
		extern const char *s_aGameTypeNames[];
		wchar *gameType = g_pVGuiLocalize->Find( GameRules()->GetGameTypeName() );
		if ( gameType )
		{
			char szGameType[ DISCORD_FIELD_MAXLEN ];
			g_pVGuiLocalize->ConvertUnicodeToANSI( gameType, szGameType, sizeof( szGameType ) );
			m_Activity.GetAssets().SetLargeText( szGameType );
		}
	}

	// --- Details = server hostname, State = "Map: <name>" ---
	// UpdatePresence(bIsDead) will fold in team/class once the player spawns.
	char szState[ DISCORD_FIELD_MAXLEN ];
	V_snprintf( szState, sizeof( szState ), "Map: %s", szMapDisplay );

	m_Activity.SetDetails( m_szHostName );
	m_Activity.SetState( szState );
	m_Activity.GetAssets().SetSmallImage( "tf2v_drp_logo" );
	m_Activity.GetTimestamps().SetStart( m_iCreationTimestamp );

	// --- Steam Rich Presence fields (Steam overlay "Join Game" button) ---
	if ( steamapicontext && steamapicontext->SteamFriends() )
	{
		steamapicontext->SteamFriends()->SetRichPresence( "connect",
			VarArgs( "steam://connect/%s", m_szServerInfo ) );
		steamapicontext->SteamFriends()->SetRichPresence( "status", m_szHostName );
		steamapicontext->SteamFriends()->SetRichPresence( "steam_display", GetLevelName() );
		steamapicontext->SteamFriends()->SetRichPresence( "currentmap", szMapDisplay );

		// Advertise the party group so friends can see it in the overlay.
		if ( GTFPartyClient()->BHaveActiveParty() )
		{
			steamapicontext->SteamFriends()->SetRichPresence( "steam_player_group",
				CFmtStr( "party_%llu", GTFPartyClient()->GetActivePartyID() ) );
			steamapicontext->SteamFriends()->SetRichPresence( "steam_player_group_size",
				CFmtStr( "%d", GTFPartyClient()->CountNumOnlinePartyMembers() ) );
		}
	}

	// --- Secrets ---
	m_Activity.GetSecrets().SetJoin( GetJoinSecret() );
	m_Activity.GetSecrets().SetSpectate( GetSpectateSecret() );

	// Party ID: use GC party when available so party members share a Discord party.
	if ( GTFPartyClient()->BHaveActiveParty() )
	{
		char szPartyID[ 32 ];
		V_snprintf( szPartyID, sizeof( szPartyID ), "party_%llu", GTFPartyClient()->GetActivePartyID() );
		m_Activity.GetParty().SetId( szPartyID );
	}
	else
	{
		m_Activity.GetParty().SetId( m_szSteamID );
	}

	if ( g_pDiscord )
	{
		g_pDiscord->ActivityManager().UpdateActivity( m_Activity, &OnActivityUpdate );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Reset map details
//-----------------------------------------------------------------------------
void CTFDiscordPresence::LevelShutdownPreEntity( void )
{
	ResetPresence();
}

//-----------------------------------------------------------------------------
// Purpose: Revert to default state
//-----------------------------------------------------------------------------
void CTFDiscordPresence::ResetPresence( void )
{
	Q_memset( &m_Activity, 0, sizeof( discord::Activity ) );
	Q_memset( &m_Activity, 0, sizeof( discord::Lobby ) );

	if ( steamapicontext->SteamFriends() )
	{
		steamapicontext->SteamFriends()->SetRichPresence( "status", "Main Menu" );
		steamapicontext->SteamFriends()->SetRichPresence( "connect", VarArgs( "steam://connect/%s", m_szServerInfo ) );
		steamapicontext->SteamFriends()->SetRichPresence( "steam_display", "Main Menu" );
	}

	m_Activity.SetDetails( "Main Menu" );
	m_Activity.GetAssets().SetLargeImage( "tf2v_drp_logo" );
	m_Activity.GetTimestamps().SetStart( m_iCreationTimestamp );

	if( g_pDiscord )
	{
		g_pDiscord->ActivityManager().UpdateActivity( m_Activity, &OnActivityUpdate );
	}
}

void CTFDiscordPresence::UpdatePresence( void )
{
	if ( m_flLastPlayerJoinTime != -1.0f && ( gpGlobals->curtime - m_flLastPlayerJoinTime ) > 1.0f )
	{
		m_flLastPlayerJoinTime = -1.0f;

		m_Activity.GetParty().GetSize().SetCurrentSize( m_nPlayerCount );
		m_Activity.GetParty().GetSize().SetMaxSize( gpGlobals->maxClients );

		g_pDiscord->ActivityManager().UpdateActivity( m_Activity, &OnActivityUpdate );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Builds the plaintext payload embedded in the join secret.
// When the local player is on a real team the payload carries a team hint so
// OnJoinedGame can queue a jointeam command after connecting.
// Format:  "ip:port"          (spectator / not on a team)
//          "ip:port/red"      (on RED)
//          "ip:port/blue"     (on BLU)
//-----------------------------------------------------------------------------
void CTFDiscordPresence::BuildJoinPayload( char *pBuf, int nBufLen ) const
{
	if ( m_szTeamPref[0] != '\0' )
		V_snprintf( pBuf, nBufLen, "%s/%s", m_szServerInfo, m_szTeamPref );
	else
		V_strncpy( pBuf, m_szServerInfo, nBufLen );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
char const *CTFDiscordPresence::GetMatchSecret( void ) const
{
	IceKey ice(0);
	ice.set( GetEncryptionKey() );
	int nBlockSize = ice.blockSize();

	int nLength = V_strlen( m_szSteamID );
	unsigned char *cypher = (unsigned char *)_alloca( PAD_NUMBER( nLength, nBlockSize ) );
	unsigned char *temp = (unsigned char *)m_szSteamID;

	int nBytesLeft = nLength;
	for( ; nBytesLeft >= nBlockSize; nBytesLeft -= nBlockSize )
	{
		ice.encrypt( temp, cypher );

		cypher += nBlockSize;
		temp += nBlockSize;
	}
	
	Q_memcpy( cypher, temp, nLength - nBytesLeft );
	cypher -= nLength - nBytesLeft;
	return (char *)cypher;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
char const *CTFDiscordPresence::GetJoinSecret( void ) const
{
	IceKey ice( 0 );
	ice.set( GetEncryptionKey() );
	int nBlockSize = ice.blockSize();

	// Build payload: "ip:port" or "ip:port/red" / "ip:port/blue"
	char szPayload[ DISCORD_FIELD_MAXLEN ];
	BuildJoinPayload( szPayload, sizeof( szPayload ) );

	int nLength = V_strlen( szPayload );
	unsigned char *cypher = (unsigned char *)_alloca( PAD_NUMBER( nLength, nBlockSize ) );
	unsigned char *temp = (unsigned char *)szPayload;

	int nBytesLeft = nLength;
	for ( ; nBytesLeft >= nBlockSize; nBytesLeft -= nBlockSize )
	{
		ice.encrypt( temp, cypher );

		cypher += nBlockSize;
		temp += nBlockSize;
	}

	Q_memcpy( cypher, temp, nLength - nBytesLeft );
	cypher -= nLength - nBytesLeft;
	return (char *)cypher;
}

//-----------------------------------------------------------------------------
// Purpose: Spectate secret encodes "ip:stvport" so the receiving client
// connects directly to SourceTV without guessing the port.
//-----------------------------------------------------------------------------
char const *CTFDiscordPresence::GetSpectateSecret( void ) const
{
	IceKey ice( 0 );
	ice.set( GetEncryptionKey() );
	int nBlockSize = ice.blockSize();

	// Build "ip:stvport" — strip any existing port from m_szServerInfo (ip:gameport)
	// then append the SourceTV port.
	char szBase[64];
	V_strncpy( szBase, m_szServerInfo, sizeof( szBase ) );
	char *pColon = V_strrchr( szBase, ':' );
	if ( pColon )
		*pColon = '\0'; // strip game port, leaving bare IP

	char szPayload[ DISCORD_FIELD_MAXLEN ];
	V_snprintf( szPayload, sizeof( szPayload ), "%s:%d", szBase, m_nSourceTVPort );

	int nLength = V_strlen( szPayload );
	unsigned char *cypher = (unsigned char *)_alloca( PAD_NUMBER( nLength, nBlockSize ) );
	unsigned char *temp = (unsigned char *)szPayload;

	int nBytesLeft = nLength;
	for ( ; nBytesLeft >= nBlockSize; nBytesLeft -= nBlockSize )
	{
		ice.encrypt( temp, cypher );

		cypher += nBlockSize;
		temp += nBlockSize;
	}

	Q_memcpy( cypher, temp, nLength - nBytesLeft );
	cypher -= nLength - nBytesLeft;
	return (char *)cypher;
}

//-----------------------------------------------------------------------------
// Purpose: Updates class/team icon and folds team name into the State line.
//-----------------------------------------------------------------------------
void CTFDiscordPresence::UpdatePresence( bool bIsDead )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
		return;

	const int iClassNum = pLocalPlayer->GetPlayerClass()->GetClassIndex();
	const int iTeam    = pLocalPlayer->GetTeamNumber();

	// Keep the team preference member current so join secrets stay accurate.
	m_szTeamPref[0] = '\0';
	const char *pTeamName = "Spectator";
	switch ( iTeam )
	{
		case TF_TEAM_RED:
			pTeamName = "RED";
			V_strncpy( m_szTeamPref, "red", sizeof( m_szTeamPref ) );
			break;
		case TF_TEAM_BLUE:
			pTeamName = "BLU";
			V_strncpy( m_szTeamPref, "blue", sizeof( m_szTeamPref ) );
			break;
		default:
			break;
	}

	// Class label — append "(Dead)" suffix when applicable.
	char szClassName[ DISCORD_FIELD_MAXLEN ];
	V_snprintf( szClassName, sizeof( szClassName ), "%s%s",
		pLocalPlayer->GetPlayerClass()->GetName(),
		bIsDead ? " (Dead)" : "" );

	// State: "Map: 2Fort | RED"
	// Details retains the server hostname set in LevelInitPostEntity/server_spawn.
	char szState[ DISCORD_FIELD_MAXLEN ];
	V_snprintf( szState, sizeof( szState ), "Map: %s | %s", GetLevelName(), pTeamName );
	m_Activity.SetState( szState );

	// Small icon = team-colored class portrait, grayed out when dead.
	switch ( iTeam )
	{
		case TF_TEAM_RED:
			m_Activity.GetAssets().SetSmallImage(
				bIsDead ? s_pClassImages[iClassNum].redTeamImageDead : s_pClassImages[iClassNum].redTeamImage );
			m_Activity.GetAssets().SetSmallText( szClassName );
			break;
		case TF_TEAM_BLUE:
			m_Activity.GetAssets().SetSmallImage(
				bIsDead ? s_pClassImages[iClassNum].bluTeamImageDead : s_pClassImages[iClassNum].bluTeamImage );
			m_Activity.GetAssets().SetSmallText( szClassName );
			break;
		default:
			m_Activity.GetAssets().SetSmallImage( "tf2v_drp_logo" );
			m_Activity.GetAssets().SetSmallText( szClassName );
			break;
	}

	g_pDiscord->ActivityManager().UpdateActivity( m_Activity, &OnActivityUpdate );
}

#endif // !POSIX