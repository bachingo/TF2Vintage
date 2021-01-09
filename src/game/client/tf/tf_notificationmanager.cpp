#include "cbase.h"
#include "tf_notificationmanager.h"
#include "tf_mainmenu.h"
#include "filesystem.h"
#include "script_parser.h"
#include "tf_gamerules.h"
#include "tf_hud_notification_panel.h"
//#include "public\steam\matchmakingtypes.h"

static CTFNotificationManager g_TFNotificationManager;
CTFNotificationManager *GetNotificationManager()
{
	return &g_TFNotificationManager;
}

CON_COMMAND_F( tf2v_updateserverlist, "Check for the messages", FCVAR_DEVELOPMENTONLY )
{
	GetNotificationManager()->UpdateServerlistInfo();
}

ConVar tf2v_updatefrequency( "tf2v_updatefrequency", "15", FCVAR_DEVELOPMENTONLY, "Updatelist update frequency (seconds)" );

bool ServerLessFunc( const int &lhs, const int &rhs )
{
	return lhs < rhs;
}

//-----------------------------------------------------------------------------
// Purpose: constructor
//-----------------------------------------------------------------------------
CTFNotificationManager::CTFNotificationManager() : CAutoGameSystemPerFrame( "CTFNotificationManager" )
{
	if ( !filesystem )
		return;

	m_bInited = false;
	Init();
}

CTFNotificationManager::~CTFNotificationManager()
{
}

//-----------------------------------------------------------------------------
// Purpose: Initializer
//-----------------------------------------------------------------------------
bool CTFNotificationManager::Init()
{
	if ( !m_bInited )
	{
		m_SteamHTTP = steamapicontext->SteamHTTP();
		m_mapServers.SetLessFunc( ServerLessFunc );
		fUpdateLastCheck = tf2v_updatefrequency.GetFloat() * -1;
		bCompleted = false;
		m_bInited = true;

		hRequest = 0;
		MatchMakingKeyValuePair_t filter;
		Q_strncpy( filter.m_szKey, "gamedir", sizeof( filter.m_szKey ) );
		Q_strncpy( filter.m_szValue, "tf2vintage", sizeof( filter.m_szKey ) ); // change "tf2vintage" to engine->GetGameDirectory() before the release
		m_vecServerFilters.AddToTail( filter );
	}
	return true;
}

void CTFNotificationManager::Update( float frametime )
{
	if ( !MAINMENU_ROOT->InGame() && gpGlobals->curtime - fUpdateLastCheck > tf2v_updatefrequency.GetFloat() )
	{
		fUpdateLastCheck = gpGlobals->curtime;
		UpdateServerlistInfo();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Event handler
//-----------------------------------------------------------------------------
void CTFNotificationManager::FireGameEvent( IGameEvent *event )
{
}

void CTFNotificationManager::UpdateServerlistInfo()
{
	ISteamMatchmakingServers *pMatchmaking = steamapicontext->SteamMatchmakingServers();

	if ( !pMatchmaking || pMatchmaking->IsRefreshing( hRequest ) )
		return;

	MatchMakingKeyValuePair_t *pFilters;
	int nFilters = GetServerFilters( &pFilters );
	hRequest = pMatchmaking->RequestInternetServerList( engine->GetAppID(), &pFilters, nFilters, this );
}

gameserveritem_t CTFNotificationManager::GetServerInfo( int index )
{
	return m_mapServers[index];
};

void CTFNotificationManager::ServerResponded( HServerListRequest hRequest, int iServer )
{
	gameserveritem_t *pServerItem = steamapicontext->SteamMatchmakingServers()->GetServerDetails( hRequest, iServer );
	int index = m_mapServers.Find( iServer );
	if ( index == m_mapServers.InvalidIndex() )
	{
		m_mapServers.Insert( iServer, *pServerItem );
		//Msg("%i SERVER %s (%s): PING %i, PLAYERS %i/%i, MAP %s\n", iServer, pServerItem->GetName(), pServerItem->m_NetAdr.GetQueryAddressString(),
			//pServerItem->m_nPing, pServerItem->m_nPlayers, pServerItem->m_nMaxPlayers, pServerItem->m_szMap);
	}
	else
	{
		m_mapServers[index] = *pServerItem;
	}
}

void CTFNotificationManager::RefreshComplete( HServerListRequest hRequest, EMatchMakingServerResponse response )
{
	MAINMENU_ROOT->SetServerlistSize( m_mapServers.Count() );
	MAINMENU_ROOT->OnServerInfoUpdate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
uint32 CTFNotificationManager::GetServerFilters( MatchMakingKeyValuePair_t **pFilters )
{
	*pFilters = m_vecServerFilters.Base();
	return m_vecServerFilters.Count();
}

char* CTFNotificationManager::GetVersionString()
{
	char verString[30];
	if ( g_pFullFileSystem->FileExists( "version.txt" ) )
	{
		FileHandle_t fh = filesystem->Open( "version.txt", "r", "MOD" );
		int file_len = filesystem->Size( fh );
		char* GameInfo = new char[file_len + 1];

		filesystem->Read( (void*)GameInfo, file_len, fh );
		GameInfo[file_len] = 0; // null terminator

		filesystem->Close( fh );

		V_strcpy_safe( verString, GameInfo + 8 );

		delete[] GameInfo;
	}

	char *szResult = (char*)malloc( sizeof( verString ) );
	Q_strncpy( szResult, verString, sizeof( verString ) );
	return szResult;
}