//=============================================================================
//
// Purpose: Payload HUD
//
//=============================================================================
#include "cbase.h"
#include "tf_hud_escort.h"
#include "tf_hud_freezepanel.h"
#include "c_team_objectiveresource.h"
#include "tf_gamerules.h"
#include "iclientmode.h"
#include "vgui_controls/AnimationController.h"

using namespace vgui;

extern ConVar cl_hud_minmode;
extern ConVar mp_blockstyle;
extern ConVar mp_capstyle;

//=============================================================================
// CTFHudEscort
//=============================================================================
CTFHudEscort::CTFHudEscort( Panel *pParent, const char *pszName, int iTeam, bool bMultipleTrains ) : EditablePanel( pParent, pszName )
{
	m_iTeamNum = iTeam;

	m_pLevelBar = new ImagePanel( this, "LevelBar" );

	m_pEscortItemPanel = new EditablePanel( this, "EscortItemPanel" );
	m_pEscortItemImage = new ImagePanel( m_pEscortItemPanel, "EscortItemImage" );
	m_pEscortItemImageBottom = new ImagePanel( m_pEscortItemPanel, "EscortItemImageBottom" );
	m_pEscortItemImageAlert = new ImagePanel( m_pEscortItemPanel, "EscortItemImageAlert" );
	m_pCapNumPlayers = new CExLabel( m_pEscortItemPanel, "CapNumPlayers", "x0" );
	m_pRecedeTime = new CExLabel( m_pEscortItemPanel, "RecedeTime", "0" );
	m_pCapPlayerImage = new ImagePanel( m_pEscortItemPanel, "CapPlayerImage" );
	m_pBackwardsImage = new ImagePanel( m_pEscortItemPanel, "Speed_Backwards" );
	m_pBlockedImage = new ImagePanel( m_pEscortItemPanel, "Blocked" );
	m_pTearDrop = new CEscortStatusTeardrop( m_pEscortItemPanel, "EscortTeardrop" );

	m_pCapHighlightImage = new CControlPointIconSwoop( m_pEscortItemPanel, "CapHighlightImage" );
	m_pCapHighlightImage->SetZPos( 10 );
	m_pCapHighlightImage->SetShouldScaleImage( true );

	for ( int i = 0; i < MAX_CONTROL_POINTS; i++ )
	{
		m_pCPImages[i] = new ImagePanel( this, VarArgs( "cp_%d", i ) );
	}

	m_pCPImageTemplate = new ImagePanel( this, "SimpleControlPointTemplate" );

	for ( int i = 0; i < TEAM_TRAIN_MAX_HILLS; i++ )
	{
		m_pHillPanels[i] = new CEscortHillPanel( this, VarArgs( "hill_%d", i ) );
	}

	m_pProgressBar = new CTFHudEscortProgressBar( this, "ProgressBar", m_iTeamNum );

	m_flProgress = -1.0f;
	m_iSpeedLevel = 0;
	m_flRecedeTime = 0.0f;
	m_iCurrentCP = -1;

	m_bMultipleTrains = bMultipleTrains;
	m_bOnTop = true;
	m_bAlarm = false;
	m_iNumHills = 0;

	ivgui()->AddTickSignal( GetVPanel() );

	ListenForGameEvent( "escort_progress" );
	ListenForGameEvent( "escort_speed" );
	ListenForGameEvent( "escort_recede" );
	ListenForGameEvent( "controlpoint_initialized" );
	ListenForGameEvent( "controlpoint_updateimages" );
	ListenForGameEvent( "controlpoint_updatecapping" );
	ListenForGameEvent( "controlpoint_starttouch" );
	ListenForGameEvent( "controlpoint_endtouch" );
	ListenForGameEvent( "localplayer_respawn" );
	ListenForGameEvent( "localplayer_changeteam" );
	ListenForGameEvent( "teamplay_round_start" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudEscort::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// Get hill data.
	if ( ObjectiveResource() )
	{
		m_iNumHills = ObjectiveResource()->GetNumNodeHillData( m_iTeamNum );
	}

	// Setup conditions.
	KeyValues *pConditions = NULL;
	if ( m_iTeamNum >= FIRST_GAME_TEAM )
	{
		pConditions = new KeyValues( "conditions" );

		AddSubKeyNamed( pConditions, m_iTeamNum == TF_TEAM_RED ? "if_team_red" : "if_team_blue" );

		if ( m_bMultipleTrains )
		{
			AddSubKeyNamed( pConditions, "if_multiple_trains" );
			AddSubKeyNamed( pConditions, m_iTeamNum == TF_TEAM_RED ? "if_multiple_trains_red" : "if_multiple_trains_blue" );
			AddSubKeyNamed( pConditions, m_bOnTop ? "if_multiple_trains_top" : "if_multiple_trains_bottom" );
		}
		else if ( m_iNumHills > 0 )
		{
			AddSubKeyNamed( pConditions, "if_single_with_hills" );
			AddSubKeyNamed( pConditions, m_iTeamNum == TF_TEAM_RED ? "if_single_with_hills_red" : "if_single_with_hills_blue" );
		}
	}

	LoadControlSettings( "resource/ui/ObjectiveStatusEscort.res", NULL, NULL, pConditions );

	UpdateCPImages( true, -1 );

	if ( pConditions )
		pConditions->deleteThis();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudEscort::OnChildSettingsApplied( KeyValues *pInResourceData, Panel *pChild )
{
	/*// Apply settings from template to all CP icons.
	if ( pChild == m_pCPImageTemplate )
	{
		for ( int i = 0; i < MAX_CONTROL_POINTS; i++ )
		{
			if ( m_pCPImages[i] )
			{
				m_pCPImages[i]->ApplySettings( pInResourceData );
			}
		}
	}*/

	BaseClass::OnChildSettingsApplied( pInResourceData, pChild );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudEscort::Reset( void )
{
	m_iCurrentCP = -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudEscort::PerformLayout( void )
{
	// If the tracker's at the bottom show the correct cart image.
	if ( m_pEscortItemImage )
		m_pEscortItemImage->SetVisible( m_bOnTop );

	if ( m_pEscortItemImageBottom )
		m_pEscortItemImageBottom->SetVisible( !m_bOnTop );

	// Place the swooping thingamajig on top of the cart icon.
	if ( m_pCapHighlightImage && m_pEscortItemImage )
	{
		int x, y, wide, tall;
		m_pEscortItemImage->GetBounds( x, y, wide, tall );

		float flHeightMult = ( m_bMultipleTrains || cl_hud_minmode.GetBool() ) ? 0.15f : 0.20f;
		int iSwoopHeight = ScreenHeight() * flHeightMult;

		m_pCapHighlightImage->SetBounds( x + CAP_BOX_INDENT_X, y - iSwoopHeight + CAP_BOX_INDENT_Y, wide - ( CAP_BOX_INDENT_X * 2 ), iSwoopHeight );
	}

	// Update hill panels.
	for ( int i = 0; i < TEAM_TRAIN_MAX_HILLS; i++ )
	{
		CEscortHillPanel *pPanel = m_pHillPanels[i];
		if ( !pPanel )
			continue;

		if ( !ObjectiveResource() || i >= ObjectiveResource()->GetNumNodeHillData( m_iTeamNum ) || !m_pLevelBar )
		{
			if ( pPanel->IsVisible() )
				pPanel->SetVisible( false );

			continue;
		}

		pPanel->SetTeam( m_iTeamNum );
		pPanel->SetHillIndex( i );
		pPanel->Load();

		if ( !pPanel->IsVisible() )
			pPanel->SetVisible( true );

		// Set the panel's bounds according to starting and ending points of the hill.
		int x, y, wide, tall;
		m_pLevelBar->GetBounds( x, y, wide, tall );

		float flStart = 0, flEnd = 0;
		ObjectiveResource()->GetHillData( m_iTeamNum, i, flStart, flEnd );

		int iStartPos = flStart * wide;
		int iEndPos = flEnd * wide;

		pPanel->SetBounds( x + iStartPos, y, iEndPos - iStartPos, tall );

		// Show it on top of the track bar.
		pPanel->SetZPos( m_pLevelBar->GetZPos() + 1 );
	}

	UpdateCPImages( true, -1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFHudEscort::IsVisible( void )
{
	if ( IsInFreezeCam() )
		return false;

	if ( !TFGameRules() || !TFGameRules()->IsInEscortMode() )
	{
		return false;
	}

	if ( TeamplayRoundBasedRules()->State_Get() != GR_STATE_RND_RUNNING && TeamplayRoundBasedRules()->State_Get() != GR_STATE_PREROUND )
	{
		return false;
	}

	return BaseClass::IsVisible();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudEscort::FireGameEvent( IGameEvent *event )
{
	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();

	if ( V_strcmp( event->GetName(), "localplayer_respawn" ) == 0 || V_strcmp( event->GetName(), "localplayer_changeteam" ) == 0 )
	{
		InvalidateLayout( true, true );
		UpdateAlarmAnimations();
		return;
	}
	
	if ( V_strcmp( event->GetName(), "teamplay_round_start" ) == 0 )
	{
		m_pTearDrop->InvalidateLayout( false, false );
		return;
	}

	if ( V_strcmp( event->GetName(), "controlpoint_initialized" ) == 0 )
	{
		UpdateCPImages( true, -1 );
		return;
	}

	if ( V_strcmp( event->GetName(), "controlpoint_updateimages" ) == 0 )
	{
		int iIndex = event->GetInt( "index" );
		UpdateCPImages( false, iIndex );
		UpdateStatusTeardropFor( iIndex );
		return;
	}

	if ( V_strcmp( event->GetName(), "controlpoint_updatecapping" ) == 0 )
	{
		UpdateStatusTeardropFor( event->GetInt( "index" ) );
		return;
	}

	if ( V_strcmp( event->GetName(), "controlpoint_starttouch" ) == 0 )
	{
		int iPlayer = event->GetInt( "player" );
		if ( pLocalPlayer && iPlayer == pLocalPlayer->entindex() )
		{
			m_iCurrentCP = event->GetInt( "area" );
			UpdateStatusTeardropFor( m_iCurrentCP );
		}
		return;
	}

	if ( V_strcmp( event->GetName(), "controlpoint_endtouch" ) == 0 )
	{
		int iPlayer = event->GetInt( "player" );
		if ( pLocalPlayer && iPlayer == pLocalPlayer->entindex() )
		{
			m_iCurrentCP = -1;
			UpdateStatusTeardropFor( m_iCurrentCP );
		}
		return;
	}

	// Ignore events not related to the watched team.
	if ( event->GetInt( "team" ) != m_iTeamNum )
		return;

	if ( V_strcmp( event->GetName(), "escort_progress" ) == 0 )
	{
		if ( event->GetBool( "reset" ) )
		{
			m_flProgress = 0.0f;
		}
		else
		{
			m_flProgress = event->GetFloat( "progress" );
		}
	}
	else if ( V_strcmp( event->GetName(), "escort_speed" ) == 0 )
	{
		// Get the number of cappers.
		int iNumCappers = event->GetInt( "players" );
		int iSpeedLevel = event->GetInt( "speed" );

		if ( m_pEscortItemPanel )
		{
			m_pEscortItemPanel->SetDialogVariable( "numcappers", iNumCappers );
		}

		// Show the number and icon if there any cappers present.
		bool bShowCappers = ( iNumCappers > 0 );
		if ( m_pCapNumPlayers )
		{
			m_pCapNumPlayers->SetVisible( bShowCappers );
		}
		if ( m_pCapPlayerImage )
		{
			m_pCapPlayerImage->SetVisible( bShowCappers );
		}

		// -1 cappers means the cart is blocked.
		if ( m_pBlockedImage )
		{
			m_pBlockedImage->SetVisible( iNumCappers == -1 );
		}

		// -1 speed level means the cart is receding.
		if ( m_pBackwardsImage )
		{
			// NO! CART MOVES WRONG VAY!
			m_pBackwardsImage->SetVisible( iSpeedLevel == -1 );
		}

		if ( m_iSpeedLevel <= 0 && iSpeedLevel > 0 )
		{
			// Do the swooping animation when the cart starts moving but only for the top icon.
			if ( m_pCapHighlightImage && m_bOnTop )
			{		
				m_pCapHighlightImage->SetVisible( true );
				m_pCapHighlightImage->StartSwoop();
			}	
		}

		m_iSpeedLevel = iSpeedLevel;
	}
	else if ( V_strcmp( event->GetName(), "escort_recede" ) == 0 )
	{
		// Get the current recede time of the cart.
		m_flRecedeTime = event->GetFloat( "recedetime" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudEscort::OnTick( void )
{
	if ( !IsVisible() )
		return;

	if ( cl_hud_minmode.GetBool() && !m_bMinHud )
	{
		InvalidateLayout( true, true );
		UpdateAlarmAnimations();
		m_bMinHud = true;
		//return; 
	}
	else if ( !cl_hud_minmode.GetBool() && m_bMinHud )
	{
		InvalidateLayout( true, true );
		UpdateAlarmAnimations();
		m_bMinHud = false;
		//return;
	}

	// Are the control points positioned correctly?
	if ( m_bShouldReload )
	{
		InvalidateLayout( true, false );
	}

	if ( m_pEscortItemPanel && m_pLevelBar )
	{
		// Position the cart icon so the arrow points at its position on the track.
		int x, y, wide, tall, pos;
		m_pLevelBar->GetBounds( x, y, wide, tall );

		pos = (int)( wide * m_flProgress ) - m_pEscortItemPanel->GetWide() / 2;

		m_pEscortItemPanel->SetPos( x + pos, m_pEscortItemPanel->GetYPos() );
	}

	// Update the progress bar.
	if ( m_pProgressBar )
	{
		// Only show progress bar in Payload Race.
		if ( m_bMultipleTrains )
		{
			if ( !m_pProgressBar->IsVisible() )
			{
				m_pProgressBar->SetVisible( true );
			}

			m_pProgressBar->SetProgress( m_flProgress );
		}
		else if ( m_pProgressBar->IsVisible() )
		{
			m_pProgressBar->SetVisible( false );
		}
	}

	// Calculate time left until receding.
	float flRecedeTimeLeft = ( m_flRecedeTime != 0.0f ) ? m_flRecedeTime - gpGlobals->curtime : 0.0f;

	if ( m_pEscortItemPanel )
	{
		m_pEscortItemPanel->SetDialogVariable( "recede", (int)ceil( flRecedeTimeLeft ) );
	}

	if ( m_pRecedeTime )
	{
		// Show the timer if the cart is close to starting to recede.
		bool bShow = flRecedeTimeLeft > 0 && flRecedeTimeLeft < 20.0f;
		if ( m_pRecedeTime->IsVisible() != bShow )
		{
			m_pRecedeTime->SetVisible( bShow );
		}
	}

	// Check for alarm animation.
	bool bInAlarm = false;
	if ( ObjectiveResource() )
	{
		bInAlarm = ObjectiveResource()->GetTrackAlarm( m_iTeamNum );
	}

	if ( bInAlarm != m_bAlarm )
	{
		m_bAlarm = bInAlarm;
		UpdateAlarmAnimations();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudEscort::UpdateCPImages( bool bUpdatePositions, int iIndex )
{
	if ( !ObjectiveResource() )
		return;

	if ( bUpdatePositions )
		m_bShouldReload = true;

	KeyValues *m_pKV = new KeyValues( "SimpleControlPointTemplate" );
	FindChildByName( "SimpleControlPointTemplate" )->GetSettings( m_pKV );

	for ( int i = 0; i < MAX_CONTROL_POINTS; i++ )
	{
		// If an index is specified only update the specified point.
		if ( iIndex != -1 && i != iIndex )
			continue;

		ImagePanel *pImage = m_pCPImages[i];
		if ( !pImage )
			continue;

		if ( bUpdatePositions )
		{
			// Set bounds
			pImage->SetShouldScaleImage( true );
			pImage->SetPos( scheme()->GetProportionalScaledValue( m_pKV->GetInt( "xpos"  ) ), scheme()->GetProportionalScaledValue( m_pKV->GetInt( "ypos" ) ) );
			pImage->SetSize( scheme()->GetProportionalScaledValue( m_pKV->GetInt( "wide" ) ), scheme()->GetProportionalScaledValue( m_pKV->GetInt( "tall" ) ) );
			pImage->SetImage( m_pKV->GetString( "image" ) );
			pImage->SetZPos( m_pKV->GetInt( "zpos" ) );

			// Check if this point exists and should be shown.
			if ( i >= ObjectiveResource()->GetNumControlPoints() ||
				ObjectiveResource()->IsInMiniRound( i ) == false ||
				ObjectiveResource()->IsCPVisible( i ) == false )
			{
				if ( pImage->IsVisible() )
					pImage->SetVisible( false );

				continue;
			}

			if ( !pImage->IsVisible() )
				pImage->SetVisible( true );

			// Get the control point position.
			float flDist = ObjectiveResource()->GetPathDistance( i );

			// Check that the control points are loaded correctly 
			if ( flDist > 0.0f )
				m_bShouldReload = false;

			if ( m_pLevelBar )
			{
				int x, y, wide, tall, pos;
				m_pLevelBar->GetBounds( x, y, wide, tall );

				pos = (int)( wide * flDist ) - pImage->GetWide() / 2;

				pImage->SetPos( x + pos, pImage->GetYPos() );
			}
		}

		// Set the icon according to team.
		const char *pszImage = NULL;
		bool bOpaque = m_bMultipleTrains || m_iNumHills > 0;
		switch ( ObjectiveResource()->GetOwningTeam( i ) )
		{
		case TF_TEAM_RED:
			pszImage = bOpaque ? "../hud/cart_point_red_opaque" : "../hud/cart_point_red";
			break;
		case TF_TEAM_BLUE:
			pszImage = bOpaque ? "../hud/cart_point_blue_opaque" : "../hud/cart_point_blue";
			break;
		default:
			pszImage = bOpaque ? "../hud/cart_point_neutral_opaque" : "../hud/cart_point_neutral";
			break;
		}

		pImage->SetImage( pszImage );
	}

	if ( m_pKV )
		m_pKV->deleteThis();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudEscort::UpdateStatusTeardropFor( int iIndex )
{
	// Don't show teardrop for enemy carts in Payload Race.
	if ( m_bMultipleTrains && m_iCurrentCP != -1 )
	{
		if ( GetLocalPlayerTeam() != m_iTeamNum || !ObjectiveResource()->TeamCanCapPoint( m_iCurrentCP, m_iTeamNum ) )
			return;
	}

	// If they tell us to update all points, update only the one we're standing on.
	if ( iIndex == -1 )
	{
		iIndex = m_iCurrentCP;
	}

	// Ignore requests to display teardrop for points we're not standing on.
	if ( ( m_iCurrentCP != iIndex ) )
		return;

	if ( iIndex >= ObjectiveResource()->GetNumControlPoints() )
		iIndex = -1;

	m_pTearDrop->SetupForPoint( iIndex );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudEscort::UpdateAlarmAnimations( void )
{
	// Only do alert animations in Payload Race.
	if ( !m_pEscortItemImageAlert || !m_bMultipleTrains )
		return;

	if ( m_bAlarm )
	{
		if ( !m_pEscortItemImageAlert->IsVisible() )
			m_pEscortItemImageAlert->SetVisible( true );

		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( m_pEscortItemPanel, "HudCartAlarmPulse" );
	}
	else
	{
		if ( m_pEscortItemImageAlert->IsVisible() )
			m_pEscortItemImageAlert->SetVisible( false );

		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( m_pEscortItemPanel, "HudCartAlarmPulseStop" );
	}
}


//=============================================================================
// CTFHudMultipleEscort
//=============================================================================
CTFHudMultipleEscort::CTFHudMultipleEscort( Panel *pParent, const char *pszName ) : EditablePanel( pParent, pszName )
{
	m_pRedEscort = new CTFHudEscort( this, "RedEscortPanel", TF_TEAM_RED, true );

	m_pBlueEscort = new CTFHudEscort( this, "BlueEscortPanel", TF_TEAM_BLUE, true );
	m_pRedEscort->SetOnTop( false );

	ListenForGameEvent( "localplayer_changeteam" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudMultipleEscort::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// Setup conditions.
	KeyValues *pConditions = new KeyValues( "conditions" );

	int iTeam = GetLocalPlayerTeam();
	AddSubKeyNamed( pConditions, iTeam == TF_TEAM_BLUE ? "if_blue_is_top" : "if_red_is_top" );

	LoadControlSettings( "resource/ui/ObjectiveStatusMultipleEscort.res", NULL, NULL, pConditions );

	pConditions->deleteThis();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudMultipleEscort::FireGameEvent( IGameEvent *event )
{
	if ( V_strcmp( event->GetName(), "localplayer_changeteam" ) == 0 )
	{
		// Show the cart of the local player's team on top.
		int iTeam = GetLocalPlayerTeam();
		if ( m_pRedEscort )
		{
			m_pRedEscort->SetOnTop( iTeam != TF_TEAM_BLUE );
		}
		if ( m_pBlueEscort )
		{
			m_pBlueEscort->SetOnTop( iTeam == TF_TEAM_BLUE );
		}

		// Re-arrange panels when player changes teams.
		InvalidateLayout( false, true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudMultipleEscort::SetVisible( bool bVisible )
{
	// Hide sub-panels as well.
	if ( m_pRedEscort )
	{
		m_pRedEscort->SetVisible( bVisible );
	}
	if ( m_pBlueEscort )
	{
		m_pBlueEscort->SetVisible( bVisible );
	}

	BaseClass::SetVisible( bVisible );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFHudMultipleEscort::IsVisible( void )
{
	if ( IsInFreezeCam() )
		return false;

	return BaseClass::IsVisible();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudMultipleEscort::Reset( void )
{
	if ( m_pRedEscort )
		m_pRedEscort->Reset();

	if ( m_pBlueEscort )
		m_pBlueEscort->Reset();
}


//=============================================================================
// CEscortStatusTeardrop
//=============================================================================
CEscortStatusTeardrop::CEscortStatusTeardrop( Panel *pParent, const char *pszName ) : EditablePanel( pParent, pszName )
{
	m_pProgressText = new Label( this, "ProgressText", "" );
	m_pTearDrop = new CIconPanel( this, "Teardrop" );
	m_pBlocked = new CIconPanel( this, "Blocked" );
	m_pCapping = new ImagePanel( this, "Capping" );

	m_iOrgHeight = 0;
	m_iMidGroupIndex = -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEscortStatusTeardrop::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	m_iOrgHeight = GetTall();
	m_iMidGroupIndex = gHUD.LookupRenderGroupIndexByName( "mid" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CEscortStatusTeardrop::IsVisible( void )
{
	if ( IsInFreezeCam() )
		return false;

	if ( m_iMidGroupIndex != -1 && gHUD.IsRenderGroupLockedFor( NULL, m_iMidGroupIndex ) )
		return false;

	return BaseClass::IsVisible();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEscortStatusTeardrop::SetupForPoint( int iCP )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	bool bInWinState = TeamplayRoundBasedRules() ? TeamplayRoundBasedRules()->RoundHasBeenWon() : false;

	if ( iCP != -1 && !bInWinState )
	{
		SetVisible( true );

		int iCappingTeam = ObjectiveResource()->GetCappingTeam( iCP );
		int iOwnerTeam = ObjectiveResource()->GetOwningTeam( iCP );
		int iPlayerTeam = pPlayer->GetTeamNumber();
		bool bCapBlocked = ObjectiveResource()->CapIsBlocked( iCP );

		if ( !bCapBlocked && iCappingTeam != TEAM_UNASSIGNED && iCappingTeam != iOwnerTeam && iCappingTeam == iPlayerTeam )
		{
			m_pBlocked->SetVisible( false );
			m_pProgressText->SetVisible( false );
			m_pCapping->SetVisible( true );
		}
		else
		{
			m_pBlocked->SetVisible( true );
			m_pCapping->SetVisible( false );

			UpdateBarText( iCP );
		}
	}
	else
	{
		SetVisible( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEscortStatusTeardrop::UpdateBarText( int iCP )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer || !m_pProgressText || iCP == -1 )
		return;

	m_pProgressText->SetVisible( true );

	int iCappingTeam = ObjectiveResource()->GetCappingTeam( iCP );
	int iPlayerTeam = pPlayer->GetTeamNumber();
	int iOwnerTeam = ObjectiveResource()->GetOwningTeam( iCP );

	if ( !TeamplayGameRules()->PointsMayBeCaptured() )
	{
		m_pProgressText->SetText( "#Team_Capture_NotNow" );
		return;
	}

	if ( ObjectiveResource()->GetCPLocked( iCP ) )
	{
		m_pProgressText->SetText( "#Team_Capture_NotNow" );
		return;
	}

	if ( mp_blockstyle.GetInt() == 1 && iCappingTeam != TEAM_UNASSIGNED && iCappingTeam != iPlayerTeam )
	{
		if ( ObjectiveResource()->IsCPBlocked( iCP ) )
		{
			m_pProgressText->SetText( "#Team_Blocking_Capture" );
			return;
		}
		else if ( iOwnerTeam == TEAM_UNASSIGNED )
		{
			m_pProgressText->SetText( "#Team_Reverting_Capture" );
			return;
		}
	}

	if ( ObjectiveResource()->GetOwningTeam( iCP ) == iPlayerTeam )
	{
		// If the opponents can never recapture this point back, we use a different string
		if ( iPlayerTeam != TEAM_UNASSIGNED )
		{
			int iEnemyTeam = ( iPlayerTeam == TF_TEAM_RED ) ? TF_TEAM_BLUE : TF_TEAM_RED;
			if ( !ObjectiveResource()->TeamCanCapPoint( iCP, iEnemyTeam ) )
			{
				m_pProgressText->SetText( "#Team_Capture_Owned" );
				return;
			}
		}

		m_pProgressText->SetText( "#Team_Capture_OwnPoint" );
		return;
	}

	if ( !TeamplayGameRules()->TeamMayCapturePoint( iPlayerTeam, iCP ) )
	{
		if ( TeamplayRoundBasedRules() && TeamplayRoundBasedRules()->IsInArenaMode() == true )
		{
			m_pProgressText->SetText( "#Team_Capture_NotNow" );
		}
		else
		{
			m_pProgressText->SetText( "#Team_Capture_Linear" );
		}

		return;
	}

	char szReason[256];
	if ( !TeamplayGameRules()->PlayerMayCapturePoint( pPlayer, iCP, szReason, sizeof( szReason ) ) )
	{
		m_pProgressText->SetText( szReason );
		return;
	}

	bool bHaveRequiredPlayers = true;

	// In Capstyle 1, more players simply cap faster, no required amounts.
	if ( mp_capstyle.GetInt() != 1 )
	{
		int nNumTeammates = ObjectiveResource()->GetNumPlayersInArea( iCP, iPlayerTeam );
		int nRequiredTeammates = ObjectiveResource()->GetRequiredCappers( iCP, iPlayerTeam );
		bHaveRequiredPlayers = ( nNumTeammates >= nRequiredTeammates );
	}

	if ( iCappingTeam == iPlayerTeam && bHaveRequiredPlayers )
	{
		m_pProgressText->SetText( "#Team_Capture_Blocked" );
		return;
	}

	if ( !ObjectiveResource()->TeamCanCapPoint( iCP, iPlayerTeam ) )
	{
		m_pProgressText->SetText( "#Team_Cannot_Capture" );
		return;
	}

	m_pProgressText->SetText( "#Team_Waiting_for_teammate" );
}

//=============================================================================
// CEscortHillPanel
//=============================================================================
CEscortHillPanel::CEscortHillPanel( Panel *pParent, const char *pszName ) : Panel( pParent, pszName )
{
	// Load the texture.
	/*m_iTextureId = surface()->DrawGetTextureId( "hud/cart_track_arrow" );
	if ( m_iTextureId == -1 )
	{
		m_iTextureId = surface()->CreateNewTextureID( false );
		surface()->DrawSetTextureFile( m_iTextureId, "hud/cart_track_arrow", true, false );
	}*/

	m_bActive = false;
	m_bLowerAlpha = true;
	m_iWidth = 0;
	m_iHeight = 0;
	m_flScrollPerc = 0.0f;
	m_flTextureScale = 0.0f;

	m_iTeamNum = TEAM_UNASSIGNED;
	m_iHillIndex = 0;

	ivgui()->AddTickSignal( GetVPanel(), 750 );

	ListenForGameEvent( "teamplay_round_start" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEscortHillPanel::Load( void )
{
	if ( ObjectiveResource()->IsHillDownhill( m_iTeamNum, m_iHillIndex ) )
	{
		// Load the texture.
		m_iTextureId = surface()->DrawGetTextureId( "hud/cart_track_arrow_flipped" );
		if ( m_iTextureId == -1 )
		{
			m_iTextureId = surface()->CreateNewTextureID( false );
			surface()->DrawSetTextureFile( m_iTextureId, "hud/cart_track_arrow_flipped", true, false );
			m_bIsDownHill = true;
		}
	}
	else
	{
		// Load the texture.
		m_iTextureId = surface()->DrawGetTextureId( "hud/cart_track_arrow" );
		if ( m_iTextureId == -1 )
		{
			m_iTextureId = surface()->CreateNewTextureID( false );
			surface()->DrawSetTextureFile( m_iTextureId, "hud/cart_track_arrow", true, false );
			m_bIsDownHill = false;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEscortHillPanel::Paint( void )
{
	if ( ObjectiveResource() )
	{
		m_bActive = ObjectiveResource()->IsTrainOnHill( m_iTeamNum, m_iHillIndex );
	}
	else
	{
		m_bActive = false;
	}

	if ( m_bActive )
	{
		if ( m_bIsDownHill )
		{
			// Scroll the texture when the cart is on this hill.
			m_flScrollPerc -= 0.02f;
			if ( m_flScrollPerc < -1.0f )
				m_flScrollPerc += 1.0f;
		}
		else
		{
			// Scroll the texture when the cart is on this hill.
			m_flScrollPerc += 0.02f;
			if ( m_flScrollPerc > 1.0f )
				m_flScrollPerc -= 1.0f;
		}
	}

	surface()->DrawSetTexture( m_iTextureId );

	float flMod = m_flTextureScale + m_flScrollPerc;

	Vertex_t vert[4];

	vert[0].Init( Vector2D( 0.0f, 0.0f ), Vector2D( m_flScrollPerc, 0.0f ) );
	vert[1].Init( Vector2D( m_iWidth, 0.0f ), Vector2D( flMod, 0.0f ) );
	vert[2].Init( Vector2D( m_iWidth, m_iHeight ), Vector2D( flMod, 1.0f ) );
	vert[3].Init( Vector2D( 0.0f, m_iHeight ), Vector2D( m_flScrollPerc, 1.0f ) );

	surface()->DrawSetColor( COLOR_WHITE );
	surface()->DrawTexturedPolygon( 4, vert );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEscortHillPanel::PerformLayout( void )
{
	int x, y, textureWide, textureTall;
	GetBounds( x, y, m_iWidth, m_iHeight );
	surface()->DrawGetTextureSize( m_iTextureId, textureWide, textureTall );

	m_flTextureScale = (float)m_iWidth / ( (float)textureWide * ( (float)m_iHeight / (float)textureTall ) );

	SetAlpha( 64 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEscortHillPanel::OnTick( void )
{
	if ( !IsVisible() )
		return;

	if ( m_bActive )
	{
		if ( m_bLowerAlpha )
		{
			// Lower alpha.
			GetAnimationController()->RunAnimationCommand( this, "alpha", 32.0f, 0.0f, 0.75f, AnimationController::INTERPOLATOR_LINEAR );
			m_bLowerAlpha = false;
		}
		else
		{
			// Rise alpha.
			GetAnimationController()->RunAnimationCommand( this, "alpha", 96.0f, 0.0f, 0.75f, AnimationController::INTERPOLATOR_LINEAR );
			m_bLowerAlpha = true;
		}
	}
	else
	{
		// Stop flashing.
		SetAlpha( 64 );
		m_bLowerAlpha = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEscortHillPanel::FireGameEvent( IGameEvent *event )
{
	if ( V_strcmp( event->GetName(), "teamplay_round_start" ) == 0 )
	{
		// Reset scrolling.
		m_flScrollPerc = 0.0f;
	}
}


//=============================================================================
// CTFHudEscortProgressBar
//=============================================================================
CTFHudEscortProgressBar::CTFHudEscortProgressBar( Panel *pParent, const char *pszName, int iTeam ) : ImagePanel( pParent, pszName )
{
	m_iTeamNum = iTeam;

	const char *pszTextureName = m_iTeamNum == TF_TEAM_RED ? "hud/cart_track_red_opaque" : "hud/cart_track_blue_opaque";

	m_iTextureId = surface()->DrawGetTextureId( pszTextureName );
	if ( m_iTextureId == -1 )
	{
		m_iTextureId = surface()->CreateNewTextureID( false );
		surface()->DrawSetTextureFile( m_iTextureId, pszTextureName, true, false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudEscortProgressBar::Paint( void )
{
	if ( m_flProgress == 0.0f )
		return;

	surface()->DrawSetTexture( m_iTextureId );

	int x, y, wide, tall;
	GetBounds( x, y, wide, tall );
	wide *= m_flProgress;

	// Draw the bar.
	Vertex_t vert[4];

	vert[0].Init( Vector2D( 0.0f, 0.0f ), Vector2D( 0.0f, 0.0f ) );
	vert[1].Init( Vector2D( wide, 0.0f ), Vector2D( 1.0f, 0.0f ) );
	vert[2].Init( Vector2D( wide, tall ), Vector2D( 1.0f, 1.0f ) );
	vert[3].Init( Vector2D( 0.0f, tall ), Vector2D( 0.0f, 1.0f ) );

	Color colBar( 255, 255, 255, 210 );
	surface()->DrawSetColor( colBar );
	surface()->DrawTexturedPolygon( 4, vert );

	// Draw a line at the end.
	Color colLine( 245, 229, 196, 210 );
	surface()->DrawSetColor( colLine );
	surface()->DrawLine( wide - 1, 0, wide - 1, tall );
}
