//====== Copyright © 1996-2008, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef STEAM_GAMESERVER_H
#define STEAM_GAMESERVER_H
#ifdef _WIN32
#pragma once
#endif

#include "steam_api.h"
#include "isteamgameserver.h"
#include "isteamgameserverstats.h"

enum EServerMode
{
	eServerModeInvalid = 0, // DO NOT USE		
	eServerModeNoAuthentication = 1, // Don't authenticate user logins and don't list on the server list
	eServerModeAuthentication = 2, // Authenticate users, list on the server list, don't run VAC on clients that connect
	eServerModeAuthenticationAndSecure = 3, // Authenticate users, list on the server list and VAC protect clients
};													

// Initialize SteamGameServer client and interface objects, and set server properties which may not be changed.
//
// After calling this function, you should set any additional server parameters, and then
// call ISteamGameServer::LogOnAnonymous() or ISteamGameServer::LogOn()
//
// - usSteamPort is the local port used to communicate with the steam servers.
//   NOTE: unless you are using ver old Steam client binaries, this parameter is ignored, and
//         you should pass 0.  Gameservers now always use WebSockets to talk to Steam.
//         This protocol is TCP-based and thus always uses an ephemeral local port.
//         Older steam client binaries used UDP to talk to Steam, and this argument was useful.
//         A future version of the SDK will remove this argument.
// - usGamePort is the port that clients will connect to for gameplay.
// - usQueryPort is the port that will manage server browser related duties and info
//		pings from clients.  If you pass MASTERSERVERUPDATERPORT_USEGAMESOCKETSHARE for usQueryPort, then it
//		will use "GameSocketShare" mode, which means that the game is responsible for sending and receiving
//		UDP packets for the master  server updater. See references to GameSocketShare in isteamgameserver.h.
// - The version string is usually in the form x.x.x.x, and is used by the master server to detect when the
//		server is out of date.  (Only servers with the latest version will be listed.)
#ifndef _PS3

#ifdef VERSION_SAFE_STEAM_API_INTERFACES
S_API bool SteamGameServer_InitSafe( uint32 unIP, uint16 usSteamPort, uint16 usGamePort, uint16 usQueryPort, EServerMode eServerMode, const char *pchVersionString );
#else
S_API bool SteamGameServer_Init( uint32 unIP, uint16 usSteamPort, uint16 usGamePort, uint16 usQueryPort, EServerMode eServerMode, const char *pchVersionString );
#endif

#else

#ifdef VERSION_SAFE_STEAM_API_INTERFACES
S_API bool SteamGameServer_InitSafe( const SteamPS3Params_t *ps3Params, uint32 unIP, uint16 usSteamPort, uint16 usGamePort, uint16 usQueryPort, EServerMode eServerMode, const char *pchVersionString );
#else
S_API bool SteamGameServer_Init( const SteamPS3Params_t *ps3Params, uint32 unIP, uint16 usSteamPort, uint16 usGamePort, uint16 usQueryPort, EServerMode eServerMode, const char *pchVersionString );
#endif

#endif

// Shutdown SteamGameSeverXxx interfaces, log out, and free resources.
S_API void SteamGameServer_Shutdown();

S_API bool SteamGameServer_BSecure();
S_API uint64 SteamGameServer_GetSteamID();


#ifdef VERSION_SAFE_STEAM_API_INTERFACES
//----------------------------------------------------------------------------------------------------------------------------------------------------------//
// VERSION_SAFE_STEAM_API_INTERFACES uses CSteamAPIContext to provide interfaces to each module in a way that 
// lets them each specify the interface versions they are compiled with.
//
// It's important that these stay inlined in the header so the calling module specifies the interface versions
// for whatever Steam API version it has.
//----------------------------------------------------------------------------------------------------------------------------------------------------------//


// This function must be declared inline in the header so the module using steam_api.dll gets the version names they want.
inline bool CSteamGameServerAPIContext::Init()
{
	if ( !SteamGameServerClient() )
		return false;

	HSteamUser hSteamUser = SteamGameServer_GetHSteamUser();
	HSteamPipe hSteamPipe = SteamGameServer_GetHSteamPipe();

	m_pSteamGameServer = SteamGameServerClient()->GetISteamGameServer( hSteamUser, hSteamPipe, STEAMGAMESERVER_INTERFACE_VERSION );
	if ( !m_pSteamGameServer )
		return false;

	m_pSteamGameServerUtils = SteamGameServerClient()->GetISteamUtils( hSteamPipe, STEAMUTILS_INTERFACE_VERSION );
	if ( !m_pSteamGameServerUtils )
		return false;

	m_pSteamGameServerNetworking = SteamGameServerClient()->GetISteamNetworking( hSteamUser, hSteamPipe, STEAMNETWORKING_INTERFACE_VERSION );
	if ( !m_pSteamGameServerNetworking )
		return false;

	m_pSteamGameServerStats = SteamGameServerClient()->GetISteamGameServerStats( hSteamUser, hSteamPipe, STEAMGAMESERVERSTATS_INTERFACE_VERSION );
	if ( !m_pSteamGameServerStats )
		return false;

	m_pSteamHTTP = SteamGameServerClient()->GetISteamHTTP( hSteamUser, hSteamPipe, STEAMHTTP_INTERFACE_VERSION );
	if ( !m_pSteamHTTP )
		return false;

	m_pSteamUGC = SteamGameServerClient()->GetISteamUGC( hSteamUser, hSteamPipe, STEAMUGC_INTERFACE_VERSION );
	if ( !m_pSteamUGC )
		return false;

	m_pSteamApps = SteamGameServerClient()->GetISteamApps( hSteamUser, hSteamPipe, STEAMAPPS_INTERFACE_VERSION );
	if ( !m_pSteamApps )
		return false;

	return true;
}

#endif // VERSION_SAFE_STEAM_API_INTERFACES


#endif // STEAM_GAMESERVER_H
