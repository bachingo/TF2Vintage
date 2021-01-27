#include "cbase.h"
#include "tf_mainmenupanel.h"
#include "controls/tf_advbutton.h"
#include "controls/tf_advslider.h"
#include "vgui_controls/SectionedListPanel.h"
#include "vgui_controls/ImagePanel.h"
#include "tf_notificationmanager.h"
#include "c_sdkversionchecker.h"
#include "engine/IEngineSound.h"
#include "vgui_avatarimage.h"
#include "tf_gamerules.h"

using namespace vgui;
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar tf2v_mainmenu_music( "tf2v_mainmenu_music", "1", FCVAR_ARCHIVE, "Toggle music in the main menu" );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFMainMenuPanel::CTFMainMenuPanel( vgui::Panel* parent, const char *panelName ) : CTFMenuPanelBase( parent, panelName )
{
	Init();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFMainMenuPanel::~CTFMainMenuPanel()
{

}

bool CTFMainMenuPanel::Init()
{
	BaseClass::Init();

	m_psMusicStatus = MUSIC_FIND;
	m_pzMusicLink[0] = '\0';
	m_nSongGuid = 0;

	if ( steamapicontext->SteamUser() )
	{
		m_SteamID = steamapicontext->SteamUser()->GetSteamID();
	}

	m_iShowFakeIntro = 4;
	m_pVersionLabel = NULL;
	m_pNotificationButton = NULL;
	m_pProfileAvatar = NULL;
	m_pFakeBGImage = NULL;
	m_pServerlistPanel = new CTFServerlistPanel( this, "ServerlistPanel" );

	bInMenu = true;
	bInGame = false;
	return true;
}


void CTFMainMenuPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/UI/main_menu/MainMenuPanel.res" );
	m_pVersionLabel = dynamic_cast<CExLabel *>( FindChildByName( "VersionLabel" ) );
	m_pNotificationButton = dynamic_cast<CTFAdvButton *>( FindChildByName( "NotificationButton" ) );
	m_pProfileAvatar = dynamic_cast<CAvatarImagePanel *>( FindChildByName( "AvatarImage" ) );
	m_pFakeBGImage = dynamic_cast<vgui::ImagePanel *>( FindChildByName( "FakeBGImage" ) );

	SetVersionLabel();
}

void CTFMainMenuPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	if ( m_pProfileAvatar )
	{
		m_pProfileAvatar->SetPlayer( m_SteamID, k_EAvatarSize64x64 );
		m_pProfileAvatar->SetShouldDrawFriendIcon( false );
	}

	char szNickName[64];
	V_strcpy_safe( szNickName, ( steamapicontext->SteamFriends() ) ? steamapicontext->SteamFriends()->GetPersonaName() : "Unknown" );
	//SetDialogVariable( "nickname", szNickName ); 
	SetDialogVariable( "playername", szNickName ); // easier than changing all the language resource files

	AutoLayout();

	if ( m_iShowFakeIntro > 0 )
	{
		char szBGName[128];
		engine->GetMainMenuBackgroundName( szBGName, sizeof( szBGName ) );
		char szImage[128];
		Q_snprintf( szImage, sizeof( szImage ), "../console/%s", szBGName );
		int width, height;
		surface()->GetScreenSize( width, height );
		float fRatio = (float)width / (float)height;
		bool bWidescreen = ( fRatio < 1.5 ? false : true );
		if ( bWidescreen )
			Q_strcat( szImage, "_widescreen", sizeof( szImage ) );
		m_pFakeBGImage->SetImage( szImage );
		m_pFakeBGImage->SetVisible( true );
		m_pFakeBGImage->SetAlpha( 255 );
	}
};

void CTFMainMenuPanel::OnCommand( const char* command )
{
	if ( !Q_strcmp( command, "newquit" ) )
	{
		MAINMENU_ROOT->ShowPanel( QUIT_MENU );
	}
	else if ( !Q_strcmp( command, "newoptionsdialog" ) )
	{
		MAINMENU_ROOT->ShowPanel( OPTIONSDIALOG_MENU );
	}
	else if ( !Q_strcmp( command, "newloadout" ) )
	{
		MAINMENU_ROOT->ShowPanel( LOADOUT_MENU );
	}
	else if ( !Q_strcmp( command, "newstats" ) )
	{
		MAINMENU_ROOT->ShowPanel( STATSUMMARY_MENU );
	}
	else if ( !Q_strcmp( command, "randommusic" ) )
	{
		enginesound->StopSoundByGuid( m_nSongGuid );
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}

void CTFMainMenuPanel::OnTick()
{
	BaseClass::OnTick();

	if ( tf2v_mainmenu_music.GetBool() && !bInGameLayout )
	{
		if ( ( m_psMusicStatus == MUSIC_FIND || m_psMusicStatus == MUSIC_STOP_FIND ) && !enginesound->IsSoundStillPlaying( m_nSongGuid ) )
		{
			Q_strncpy( m_pzMusicLink, GetRandomMusic(), sizeof( m_pzMusicLink ) );
			m_psMusicStatus = MUSIC_PLAY;
		}
		else if ( ( m_psMusicStatus == MUSIC_PLAY || m_psMusicStatus == MUSIC_STOP_PLAY )&& m_pzMusicLink[0] != '\0' )
		{
			enginesound->StopSoundByGuid( m_nSongGuid );
			ConVar *snd_musicvolume = cvar->FindVar( "snd_musicvolume" );
			float fVolume = ( snd_musicvolume ? snd_musicvolume->GetFloat() : 1.0f );
			enginesound->EmitAmbientSound( m_pzMusicLink, fVolume, PITCH_NORM, 0 );
			m_nSongGuid = enginesound->GetGuidForLastSoundEmitted();
			m_psMusicStatus = MUSIC_FIND;
		}
	}
	else if ( m_psMusicStatus == MUSIC_FIND )
	{
		enginesound->StopSoundByGuid( m_nSongGuid );
		m_psMusicStatus = ( m_nSongGuid == 0 ? MUSIC_STOP_FIND : MUSIC_STOP_PLAY );
	}
};

void CTFMainMenuPanel::OnThink()
{
	BaseClass::OnThink();

	if ( m_iShowFakeIntro > 0 )
	{
		m_iShowFakeIntro--;
		if ( m_iShowFakeIntro == 0 )
		{
			vgui::GetAnimationController()->RunAnimationCommand( m_pFakeBGImage, "Alpha", 0, 1.0f, 0.5f, vgui::AnimationController::INTERPOLATOR_SIMPLESPLINE );
		}
	}
	if ( m_pFakeBGImage->IsVisible() && m_pFakeBGImage->GetAlpha() == 0 )
	{
		m_pFakeBGImage->SetVisible( false );
	}
};

void CTFMainMenuPanel::Show()
{
	BaseClass::Show();
	vgui::GetAnimationController()->RunAnimationCommand( this, "Alpha", 255, 0.0f, 0.5f, vgui::AnimationController::INTERPOLATOR_SIMPLESPLINE );
};

void CTFMainMenuPanel::Hide()
{
	BaseClass::Hide();
	vgui::GetAnimationController()->RunAnimationCommand( this, "Alpha", 0, 0.0f, 0.1f, vgui::AnimationController::INTERPOLATOR_LINEAR );
};


void CTFMainMenuPanel::DefaultLayout()
{
	BaseClass::DefaultLayout();
};

void CTFMainMenuPanel::GameLayout()
{
	BaseClass::GameLayout();
};

void CTFMainMenuPanel::PlayMusic()
{

}

void CTFMainMenuPanel::SetVersionLabel()  //GetVersionString
{
	if ( m_pVersionLabel )
	{
		char verString[64];
		Q_snprintf( verString, sizeof( verString ), "Version: %s", GetNotificationManager()->GetVersionString() );
		m_pVersionLabel->SetText( verString );
	}
};

char* CTFMainMenuPanel::GetRandomMusic()
{
	char* pszBasePath = "sound/ui/gamestartup";
	
	int iCount = 0;

	for ( int i = 0; i < 40; i++ )
	{
		char szPath[MAX_PATH];
		char szNumber[5];
		Q_snprintf( szNumber, sizeof( szNumber ), "%d", iCount + 1 );
		Q_strncpy( szPath, pszBasePath, sizeof( szPath ) );
		Q_strncat( szPath, szNumber, sizeof( szPath ) );
		Q_strncat( szPath, ".mp3", sizeof( szPath ) );
		if ( !g_pFullFileSystem->FileExists( szPath ) )
		{
			if ( iCount )
				break;
			else
				return "";
		}
		iCount++;
	}

	char* pszSoundPath = "ui/gamestartup";
	
	int iRand = rand() % iCount;
	char szPath[MAX_PATH];
	char szNumber[5];
	Q_snprintf( szNumber, sizeof( szNumber ), "%d", iRand + 1 );
	Q_strncpy( szPath, pszSoundPath, sizeof( szPath ) );
	Q_strncat( szPath, szNumber, sizeof( szPath ) );
	Q_strncat( szPath, ".mp3", sizeof( szPath ) );
	char *szResult = (char*)malloc( sizeof( szPath ) );
	Q_strncpy( szResult, szPath, sizeof( szPath ) );
	return szResult;
}

void CTFMainMenuPanel::SetServerlistSize( int size )
{
	m_pServerlistPanel->SetServerlistSize( size );
}

void CTFMainMenuPanel::UpdateServerInfo()
{
	m_pServerlistPanel->UpdateServerInfo();
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFServerlistPanel::CTFServerlistPanel( vgui::Panel* parent, const char *panelName ) : CTFMenuPanelBase( parent, panelName )
{
	m_iSize = 0;
	m_pServerList = new vgui::SectionedListPanel( this, "ServerList" );
	m_pConnectButton = new CTFAdvButton( this, "ConnectButton", "Connect" );
	m_pListSlider = new CTFAdvSlider( this, "ListSlider", "" );
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFServerlistPanel::~CTFServerlistPanel()
{
}

void CTFServerlistPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/UI/main_menu/ServerlistPanel.res" );

	m_pServerList->RemoveAll();
	m_pServerList->RemoveAllSections();
	m_pServerList->SetSectionFgColor( 0, Color( 255, 255, 255, 255 ) );
	m_pServerList->SetBgColor( Color( 0, 0, 0, 0 ) );
	m_pServerList->SetBorder( NULL );
	m_pServerList->AddSection( 0, "Servers", ServerSortFunc );
	m_pServerList->AddColumnToSection( 0, "Name", "Servers", SectionedListPanel::COLUMN_BRIGHT, m_iServerWidth );
	m_pServerList->AddColumnToSection( 0, "Players", "Players", SectionedListPanel::COLUMN_BRIGHT, m_iPlayersWidth );
	m_pServerList->AddColumnToSection( 0, "Ping", "Ping", SectionedListPanel::COLUMN_BRIGHT, m_iPingWidth );
	m_pServerList->AddColumnToSection( 0, "Map", "Map", SectionedListPanel::COLUMN_BRIGHT, m_iMapWidth );
	m_pServerList->SetSectionAlwaysVisible( 0, true );
	m_pServerList->GetScrollBar()->UseImages( "", "", "", "" ); //hack to hide the scrollbar

	m_pConnectButton->SetVisible( false );
	UpdateServerInfo();
}

void CTFServerlistPanel::PerformLayout()
{
	BaseClass::PerformLayout();
}

void CTFServerlistPanel::OnThink()
{
	m_pServerList->ClearSelection();
	m_pListSlider->SetVisible( false );
	m_pConnectButton->SetVisible( false );

	if ( !IsCursorOver() )
		return;

	m_pListSlider->SetValue( m_pServerList->GetScrollBar()->GetValue() );

	for ( int i = 0; i < m_pServerList->GetItemCount(); i++ )
	{
		int _x, _y;
		m_pServerList->GetPos( _x, _y );
		int x, y, wide, tall;
		m_pServerList->GetItemBounds( i, x, y, wide, tall );
		int cx, cy;
		surface()->SurfaceGetCursorPos( cx, cy );
		m_pServerList->ScreenToLocal( cx, cy );

		if ( cx > x && cx < x + wide && cy > y && cy < y + tall )
		{
			m_pServerList->SetSelectedItem( i );
			int by = y + _y;
			m_pConnectButton->SetPos( m_iServerWidth + m_iPlayersWidth + m_iPingWidth, by );
			m_pConnectButton->SetVisible( true );
			m_pListSlider->SetVisible( true );

			char szCommand[128];
			Q_snprintf( szCommand, sizeof( szCommand ), "connect %s", m_pServerList->GetItemData( i )->GetString( "ServerIP", "" ) );
			m_pConnectButton->SetCommandString( szCommand );
		}
	}
}

void CTFServerlistPanel::OnCommand( const char* command )
{
	if ( !Q_strcmp( command, "scrolled" ) )
	{
		m_pServerList->GetScrollBar()->SetValue( m_pListSlider->GetValue() );
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Used for sorting servers
//-----------------------------------------------------------------------------
bool CTFServerlistPanel::ServerSortFunc( vgui::SectionedListPanel *list, int itemID1, int itemID2 )
{
	KeyValues *it1 = list->GetItemData( itemID1 );
	KeyValues *it2 = list->GetItemData( itemID2 );
	Assert( it1 && it2 );

	int v1 = it1->GetInt( "CurPlayers" );
	int v2 = it2->GetInt( "CurPlayers" );
	if ( v1 > v2 )
		return true;
	else if ( v1 < v2 )
		return false;

	/*
	int iOff1 = it1->GetBool("Official");
	int iOff2 = it2->GetBool("Official");
	if (iOff1 && !iOff2)
		return true;
	else if (!iOff1 && iOff2)
		return false;
	*/

	int iPing1 = it1->GetInt( "Ping" );
	if ( iPing1 == 0 )
		return false;
	int iPing2 = it2->GetInt( "Ping" );
	return ( iPing1 < iPing2 );
}

void CTFServerlistPanel::SetServerlistSize( int size )
{
	m_iSize = size;
};

void CTFServerlistPanel::UpdateServerInfo()
{
	m_pServerList->RemoveAll();
	HFont Font = GETSCHEME()->GetFont( "FontStoreOriginalPrice", true );

	for ( int i = 0; i < m_iSize; i++ )
	{
		gameserveritem_t m_Server = GetNotificationManager()->GetServerInfo( i );
		if ( m_Server.m_steamID.GetAccountID() == 0 )
			continue;

		// Don't show passworded/locked servers.
		if (m_Server.m_bPassword)
			continue;

		// Hide servers with zero human players.
		if ( (m_Server.m_nPlayers - m_Server.m_nBotPlayers) < 1 )
			continue;

		char szServerName[128];
		char szServerIP[32];
		char szServerPlayers[16];
		int szServerCurPlayers;
		int szServerPing;
		char szServerMap[32];

		Q_snprintf( szServerName, sizeof( szServerName ), "%s", m_Server.GetName() );
		Q_snprintf( szServerIP, sizeof( szServerIP ), "%s", m_Server.m_NetAdr.GetQueryAddressString() );
		Q_snprintf( szServerPlayers, sizeof( szServerPlayers ), "%i/%i", m_Server.m_nPlayers, m_Server.m_nMaxPlayers );
		szServerCurPlayers = m_Server.m_nPlayers - m_Server.m_nBotPlayers; // Current HUMAN Players.
		szServerPing = m_Server.m_nPing;
		Q_snprintf( szServerMap, sizeof( szServerMap ), "%s", m_Server.m_szMap );

		KeyValues *curitem = new KeyValues( "data" );

		curitem->SetString( "Name", szServerName );
		curitem->SetString( "ServerIP", szServerIP );
		curitem->SetString( "Players", szServerPlayers );
		curitem->SetInt( "Ping", szServerPing );
		curitem->SetInt( "CurPlayers", szServerCurPlayers );
		curitem->SetString( "Map", szServerMap );

		int itemID = m_pServerList->AddItem( 0, curitem );

		m_pServerList->SetItemFgColor( itemID, GETSCHEME()->GetColor( "AdvTextDefault", Color( 255, 255, 255, 255 ) ) );

		m_pServerList->SetItemFont( itemID, Font );
		curitem->deleteThis();
	}
	
#ifdef _DEBUG
	if ( m_pServerList->GetItemCount() < 1 )
	{
		// If we don't have any servers listed, make a dummy server for the debugger.
		KeyValues *curitemDEBUG = new KeyValues( "data" );

		curitemDEBUG->SetString( "Name", "DEBUG NAME" );
		curitemDEBUG->SetString( "ServerIP", "127.0.0.1:27015" );
		curitemDEBUG->SetString( "Players", "0/0" );
		curitemDEBUG->SetInt( "Ping", 000 );
		curitemDEBUG->SetInt( "CurPlayers", 0 );
		curitemDEBUG->SetString( "Map", "DEBUG MAP" );

		int itemID = m_pServerList->AddItem( 0, curitemDEBUG );

		m_pServerList->SetItemFgColor( itemID, GETSCHEME()->GetColor( "AdvTextDefault", Color( 255, 255, 255, 255 ) ) );

		m_pServerList->SetItemFont( itemID, Font );
		curitemDEBUG->deleteThis();
	}
	SetVisible( true );
#else
	if ( m_pServerList->GetItemCount() > 0 )
	{
		SetVisible( true );
	}
	else
	{
		SetVisible( false );
	}
#endif

	int min, max;
	m_pServerList->InvalidateLayout( 1, 0 );
	m_pServerList->GetScrollBar()->GetRange( min, max );
	m_pListSlider->SetRange( min, max - m_pServerList->GetScrollBar()->GetButton( 0 )->GetTall() * 4 );
}