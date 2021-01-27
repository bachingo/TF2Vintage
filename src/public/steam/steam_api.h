//====== Copyright 1996-2008, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef STEAM_API_H
#define STEAM_API_H
#ifdef _WIN32
#pragma once
#endif

#include "steam_api_common.h"

#include "isteamclient.h"
#include "isteamuser.h"
#include "isteamfriends.h"
#include "isteamutils.h"
#include "isteammatchmaking.h"
#include "isteamuserstats.h"
#include "isteamapps.h"
#include "isteamnetworking.h"
#include "isteamremotestorage.h"
#include "isteamscreenshots.h"
#include "isteammusic.h"
#include "isteammusicremote.h"
#include "isteamhttp.h"
//#include "isteamunifiedmessages.h"
#include "isteamcontroller.h"
#include "isteamugc.h"
#include "isteamapplist.h"
#include "isteamhtmlsurface.h"

#if defined( _PS3 )
#include "steamps3params.h"
#endif

//----------------------------------------------------------------------------------------------------------------------------------------------------------//
//	Steam API setup & shutdown
//
//	These functions manage loading, initializing and shutdown of the steamclient.dll
//
//----------------------------------------------------------------------------------------------------------------------------------------------------------//

// S_API void SteamAPI_Init(); (see below)
S_API void S_CALLTYPE SteamAPI_Shutdown();

// checks if a local Steam client is running 
S_API bool S_CALLTYPE SteamAPI_IsSteamRunning();

// Detects if your executable was launched through the Steam client, and restarts your game through 
// the client if necessary. The Steam client will be started if it is not running.
//
// Returns: true if your executable was NOT launched through the Steam client. This function will
//          then start your application through the client. Your current process should exit.
//
//          false if your executable was started through the Steam client or a steam_appid.txt file
//          is present in your game's directory (for development). Your current process should continue.
//
// NOTE: This function should be used only if you are using CEG or not using Steam's DRM. Once applied
//       to your executable, Steam's DRM will handle restarting through Steam if necessary.
S_API bool S_CALLTYPE SteamAPI_RestartAppIfNecessary( uint32 unOwnAppID );

// crash dump recording functions
S_API void S_CALLTYPE SteamAPI_WriteMiniDump( uint32 uStructuredExceptionCode, void* pvExceptionInfo, uint32 uBuildID );
S_API void S_CALLTYPE SteamAPI_SetMiniDumpComment( const char *pchMsg );


//
// VERSION_SAFE_STEAM_API_INTERFACES is usually not necessary, but it provides safety against releasing
// new steam_api.dll's without recompiling/rereleasing modules that use it.
//
// If you use VERSION_SAFE_STEAM_API_INTERFACES, then you should call SteamAPI_InitSafe(). Also, to get the 
// Steam interfaces, you must create and Init() a CSteamAPIContext (below) and use the interfaces in there.
//
// If you don't use VERSION_SAFE_STEAM_API_INTERFACES, then you can use SteamAPI_Init() and the SteamXXXX() 
// functions below to get at the Steam interfaces.
//
#ifdef VERSION_SAFE_STEAM_API_INTERFACES
S_API bool S_CALLTYPE SteamAPI_InitSafe();
#else
#if defined(_PS3)
S_API bool S_CALLTYPE SteamAPI_Init( SteamPS3Params_t *pParams );
#else
S_API bool S_CALLTYPE SteamAPI_Init();
#endif // _PS3
#endif // VERSION_SAFE_STEAM_API_INTERFACES


//----------------------------------------------------------------------------------------------------------------------------------------------------------//
//	steam callback and call-result helpers
//
//	The following macros and classes are used to register your application for
//	callbacks and call-results, which are delivered in a predictable manner.
//
//	STEAM_CALLBACK macros are meant for use inside of a C++ class definition.
//	They map a Steam notification callback directly to a class member function
//	which is automatically prototyped as "void func( callback_type *pParam )".
//
//	CCallResult is used with specific Steam APIs that return "result handles".
//	The handle can be passed to a CCallResult object's Set function, along with
//	an object pointer and member-function pointer. The member function will
//	be executed once the results of the Steam API call are available.
//
//	CCallback and CCallbackManual classes can be used instead of STEAM_CALLBACK
//	macros if you require finer control over registration and unregistration.
//
//	Callbacks and call-results are queued automatically and are only
//	delivered/executed when your application calls SteamAPI_RunCallbacks().
//----------------------------------------------------------------------------------------------------------------------------------------------------------//


#ifdef _WIN32
// disable this warning; this pattern need for steam callback registration
#pragma warning( disable: 4355 )	// 'this' : used in base member initializer list
#endif


//----------------------------------------------------------------------------------------------------------------------------------------------------------//
//	steamclient.dll private wrapper functions
//
//	The following functions are part of abstracting API access to the steamclient.dll, but should only be used in very specific cases
//----------------------------------------------------------------------------------------------------------------------------------------------------------//

// pumps out all the steam messages, calling the register callback
S_API void Steam_RunCallbacks( HSteamPipe hSteamPipe, bool bGameServerCallbacks );

// register the callback funcs to use to interact with the steam dll
S_API void Steam_RegisterInterfaceFuncs( void *hModule );

// returns the HSteamUser of the last user to dispatch a callback
S_API HSteamUser Steam_GetHSteamUserCurrent();

// returns the filename path of the current running Steam process, used if you need to load an explicit steam dll by name
S_API const char *SteamAPI_GetSteamInstallPath();

// returns the pipe we are communicating to Steam with
S_API HSteamPipe SteamAPI_GetHSteamPipe();

// sets whether or not Steam_RunCallbacks() should do a try {} catch (...) {} around calls to issuing callbacks
S_API void SteamAPI_SetTryCatchCallbacks( bool bTryCatchCallbacks );

// backwards compat export, passes through to SteamAPI_ variants
S_API HSteamPipe GetHSteamPipe();
S_API HSteamUser GetHSteamUser();

#ifdef VERSION_SAFE_STEAM_API_INTERFACES
//----------------------------------------------------------------------------------------------------------------------------------------------------------//
// VERSION_SAFE_STEAM_API_INTERFACES uses CSteamAPIContext to provide interfaces to each module in a way that 
// lets them each specify the interface versions they are compiled with.
//
// It's important that these stay inlined in the header so the calling module specifies the interface versions
// for whatever Steam API version it has.
//----------------------------------------------------------------------------------------------------------------------------------------------------------//

S_API HSteamUser SteamAPI_GetHSteamUser();

// This function must be inlined so the module using steam_api.dll gets the version names they want.
inline bool CSteamAPIContext::Init()
{
	if ( !SteamClient() )
		return false;

	HSteamUser hSteamUser = SteamAPI_GetHSteamUser();
	HSteamPipe hSteamPipe = SteamAPI_GetHSteamPipe();

	m_pSteamUser = SteamClient()->GetISteamUser( hSteamUser, hSteamPipe, STEAMUSER_INTERFACE_VERSION );
	if ( !m_pSteamUser )
		return false;

	m_pSteamFriends = SteamClient()->GetISteamFriends( hSteamUser, hSteamPipe, STEAMFRIENDS_INTERFACE_VERSION );
	if ( !m_pSteamFriends )
		return false;

	m_pSteamUtils = SteamClient()->GetISteamUtils( hSteamPipe, STEAMUTILS_INTERFACE_VERSION );
	if ( !m_pSteamUtils )
		return false;

	m_pSteamMatchmaking = SteamClient()->GetISteamMatchmaking( hSteamUser, hSteamPipe, STEAMMATCHMAKING_INTERFACE_VERSION );
	if ( !m_pSteamMatchmaking )
		return false;

	m_pSteamMatchmakingServers = SteamClient()->GetISteamMatchmakingServers( hSteamUser, hSteamPipe, STEAMMATCHMAKINGSERVERS_INTERFACE_VERSION );
	if ( !m_pSteamMatchmakingServers )
		return false;

	m_pSteamUserStats = SteamClient()->GetISteamUserStats( hSteamUser, hSteamPipe, STEAMUSERSTATS_INTERFACE_VERSION );
	if ( !m_pSteamUserStats )
		return false;
	
	m_pSteamApps = SteamClient()->GetISteamApps( hSteamUser, hSteamPipe, STEAMAPPS_INTERFACE_VERSION );
	if ( !m_pSteamApps )
		return false;

	m_pSteamNetworking = SteamClient()->GetISteamNetworking( hSteamUser, hSteamPipe, STEAMNETWORKING_INTERFACE_VERSION );
	if ( !m_pSteamNetworking )
		return false;

	m_pSteamRemoteStorage = SteamClient()->GetISteamRemoteStorage( hSteamUser, hSteamPipe, STEAMREMOTESTORAGE_INTERFACE_VERSION );
	if ( !m_pSteamRemoteStorage )
		return false;

	m_pSteamScreenshots = SteamClient()->GetISteamScreenshots( hSteamUser, hSteamPipe, STEAMSCREENSHOTS_INTERFACE_VERSION );
	if ( !m_pSteamScreenshots )
		return false;

	m_pSteamHTTP = SteamClient()->GetISteamHTTP( hSteamUser, hSteamPipe, STEAMHTTP_INTERFACE_VERSION );
	if ( !m_pSteamHTTP )
		return false;

	m_pController = SteamClient()->GetISteamController( hSteamUser, hSteamPipe, STEAMCONTROLLER_INTERFACE_VERSION );
	if ( !m_pController )
		return false;

	m_pSteamUGC = SteamClient()->GetISteamUGC( hSteamUser, hSteamPipe, STEAMUGC_INTERFACE_VERSION );
	if ( !m_pSteamUGC )
		return false;

	m_pSteamAppList = SteamClient()->GetISteamAppList( hSteamUser, hSteamPipe, STEAMAPPLIST_INTERFACE_VERSION );
	if ( !m_pSteamAppList )
		return false;

	m_pSteamMusic = SteamClient()->GetISteamMusic( hSteamUser, hSteamPipe, STEAMMUSIC_INTERFACE_VERSION );
	if ( !m_pSteamMusic )
	{
		return false;
	}

	m_pSteamMusicRemote = SteamClient()->GetISteamMusicRemote( hSteamUser, hSteamPipe, STEAMMUSICREMOTE_INTERFACE_VERSION );
	if ( !m_pSteamMusicRemote )
	{
		return false;
	}

	m_pSteamHTMLSurface = SteamClient()->GetISteamHTMLSurface( hSteamUser, hSteamPipe, STEAMHTMLSURFACE_INTERFACE_VERSION );
	if ( !m_pSteamHTMLSurface )
	{
		return false;
	}

#ifdef _PS3
	m_pSteamPS3OverlayRender = SteamClient()->GetISteamPS3OverlayRender();
#endif

	return true;
}

#endif // VERSION_SAFE_STEAM_API_INTERFACES

#if defined(USE_BREAKPAD_HANDLER) || defined(STEAM_API_EXPORTS)
// this should be called before the game initialized the steam APIs
// pchDate should be of the format "Mmm dd yyyy" (such as from the __DATE__ macro )
// pchTime should be of the format "hh:mm:ss" (such as from the __TIME__ macro )
// bFullMemoryDumps (Win32 only) -- writes out a uuid-full.dmp in the client/dumps folder
// pvContext-- can be NULL, will be the void * context passed into m_pfnPreMinidumpCallback
// PFNPreMinidumpCallback m_pfnPreMinidumpCallback   -- optional callback which occurs just before a .dmp file is written during a crash.  Applications can hook this to allow adding additional information into the .dmp comment stream.
S_API void S_CALLTYPE SteamAPI_UseBreakpadCrashHandler( char const *pchVersion, char const *pchDate, char const *pchTime, bool bFullMemoryDumps, void *pvContext, PFNPreMinidumpCallback m_pfnPreMinidumpCallback );
S_API void S_CALLTYPE SteamAPI_SetBreakpadAppID( uint32 unAppID );
#endif

#endif // STEAM_API_H
