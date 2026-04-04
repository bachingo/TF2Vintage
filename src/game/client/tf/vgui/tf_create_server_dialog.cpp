//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: TF2VR Create Server Dialog with bot count selection
//
//=============================================================================//

#include "cbase.h"
#include "tf_create_server_dialog.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui_controls/Label.h>
#include "filesystem.h"
#include "tier1/fmtstr.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

// Bot difficulty names - use plain text since TF2 localized strings may not be available
static const char *g_pBotDifficultyNames[] = 
{
	"Easy",
	"Normal",
	"Hard",
	"Expert"
};

// Singleton dialog instance
static CTFCreateServerDialog *g_pCreateServerDialog = NULL;

//-----------------------------------------------------------------------------
// Purpose: Show the create server dialog
//-----------------------------------------------------------------------------
void ShowCreateServerDialog( vgui::Panel *pParent )
{
	if ( !g_pCreateServerDialog )
	{
		g_pCreateServerDialog = new CTFCreateServerDialog( NULL );
	}

	g_pCreateServerDialog->Activate();
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFCreateServerDialog::CTFCreateServerDialog( vgui::Panel *parent ) : BaseClass( parent, "CreateServerDialog" )
{
	SetDeleteSelfOnClose( false );
	SetMoveable( true );
	SetSizeable( false );
	SetProportional( false );  // Use absolute sizing like the original dialog

	m_pMapComboBox = new ComboBox( this, "MapComboBox", 20, false );
	m_pBotCountComboBox = new ComboBox( this, "BotCountComboBox", 33, false );
	m_pBotDifficultyComboBox = new ComboBox( this, "BotDifficultyComboBox", 4, false );
	m_pStartButton = new Button( this, "StartButton", "Start Server", this, "Start" );
	m_pCancelButton = new Button( this, "CancelButton", "Cancel", this, "Cancel" );

	LoadControlSettings( "resource/ui/CreateServerDialog.res" );

	// Populate bot count dropdown (0-32)
	for ( int i = 0; i <= 32; i++ )
	{
		char szBotCount[8];
		Q_snprintf( szBotCount, sizeof( szBotCount ), "%d", i );
		m_pBotCountComboBox->AddItem( szBotCount, NULL );
	}
	m_pBotCountComboBox->ActivateItemByRow( 0 ); // Default to 0 bots

	// Populate bot difficulty dropdown
	for ( int i = 0; i < ARRAYSIZE( g_pBotDifficultyNames ); i++ )
	{
		m_pBotDifficultyComboBox->AddItem( g_pBotDifficultyNames[i], NULL );
	}
	m_pBotDifficultyComboBox->ActivateItemByRow( 1 ); // Default to Normal

	LoadMapList();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFCreateServerDialog::~CTFCreateServerDialog()
{
	if ( g_pCreateServerDialog == this )
	{
		g_pCreateServerDialog = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Load the list of available maps
//-----------------------------------------------------------------------------
void CTFCreateServerDialog::LoadMapList()
{
	m_pMapComboBox->RemoveAll();

	// Common TF2 map prefixes that support bots
	const char *pMapPrefixes[] = { "cp_", "pl_", "koth_", "ctf_", "arena_", "plr_", "tc_", "sd_", "pd_" };

	FileFindHandle_t findHandle;
	const char *pFilename = g_pFullFileSystem->FindFirst( "maps/*.bsp", &findHandle );
	
	CUtlVector<CUtlString> mapList;
	
	while ( pFilename )
	{
		// Check if it's a supported map type
		bool bSupported = false;
		for ( int i = 0; i < ARRAYSIZE( pMapPrefixes ); i++ )
		{
			if ( Q_strnicmp( pFilename, pMapPrefixes[i], Q_strlen( pMapPrefixes[i] ) ) == 0 )
			{
				bSupported = true;
				break;
			}
		}

		if ( bSupported )
		{
			// Strip the .bsp extension
			char szMapName[MAX_PATH];
			Q_StripExtension( pFilename, szMapName, sizeof( szMapName ) );
			mapList.AddToTail( szMapName );
		}

		pFilename = g_pFullFileSystem->FindNext( findHandle );
	}

	g_pFullFileSystem->FindClose( findHandle );

	// Sort alphabetically
	mapList.Sort( []( const CUtlString *a, const CUtlString *b ) -> int {
		return Q_stricmp( a->Get(), b->Get() );
	});

	// Add to combo box
	for ( int i = 0; i < mapList.Count(); i++ )
	{
		m_pMapComboBox->AddItem( mapList[i].Get(), NULL );
	}

	if ( mapList.Count() > 0 )
	{
		m_pMapComboBox->ActivateItemByRow( 0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Activate the dialog
//-----------------------------------------------------------------------------
void CTFCreateServerDialog::Activate()
{
	BaseClass::Activate();
	
	// Refresh map list each time we open
	LoadMapList();
	
	// Center on screen
	MoveToCenterOfScreen();
	
	RequestFocus();
	MoveToFront();
}

//-----------------------------------------------------------------------------
// Purpose: Apply scheme settings
//-----------------------------------------------------------------------------
void CTFCreateServerDialog::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	SetTitle( "Create Server", true );
}

//-----------------------------------------------------------------------------
// Purpose: Handle commands
//-----------------------------------------------------------------------------
void CTFCreateServerDialog::OnCommand( const char *command )
{
	if ( FStrEq( command, "Start" ) )
	{
		StartServer();
	}
	else if ( FStrEq( command, "Cancel" ) )
	{
		Close();
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Start the server with the selected options
//-----------------------------------------------------------------------------
void CTFCreateServerDialog::StartServer()
{
	// Get selected map
	char szMapName[MAX_PATH];
	m_pMapComboBox->GetText( szMapName, sizeof( szMapName ) );

	if ( Q_strlen( szMapName ) == 0 )
	{
		Warning( "No map selected!\n" );
		return;
	}

	// Get bot count
	char szBotCount[8];
	m_pBotCountComboBox->GetText( szBotCount, sizeof( szBotCount ) );
	int nBotCount = Q_atoi( szBotCount );

	// Get bot difficulty
	int nDifficulty = m_pBotDifficultyComboBox->GetActiveItem();

	// Close the dialog
	Close();

	// Reset server enforced cvars
	g_pCVar->RevertFlaggedConVars( FCVAR_REPLICATED );
	g_pCVar->RevertFlaggedConVars( FCVAR_CHEAT );

	// Set bot configuration
	ConVarRef tf_bot_quota( "tf_bot_quota" );
	tf_bot_quota.SetValue( nBotCount );

	ConVarRef tf_bot_quota_mode( "tf_bot_quota_mode" );
	tf_bot_quota_mode.SetValue( "normal" );

	ConVarRef tf_bot_difficulty( "tf_bot_difficulty" );
	tf_bot_difficulty.SetValue( nDifficulty );

	ConVarRef tf_bot_auto_vacate( "tf_bot_auto_vacate" );
	tf_bot_auto_vacate.SetValue( 0 );

	// Calculate maxplayers (bot count + 1 for the player, minimum of 24)
	int nMaxPlayers = MAX( nBotCount + 1, 24 );
	nMaxPlayers = MIN( nMaxPlayers, 32 );

	// Create the command to start the server
	CFmtStr1024 fmtCommand(
		"disconnect\n"
		"wait\n"
		"wait\n"
		"maxplayers %d\n"
		"progress_enable\n"
		"map %s\n",
		nMaxPlayers,
		szMapName
	);

	engine->ClientCmd_Unrestricted( fmtCommand.Access() );
}

//-----------------------------------------------------------------------------
// Purpose: Handle dialog close
//-----------------------------------------------------------------------------
void CTFCreateServerDialog::OnClose()
{
	BaseClass::OnClose();
}
