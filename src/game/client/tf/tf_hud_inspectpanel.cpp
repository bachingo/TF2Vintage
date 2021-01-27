//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "iclientmode.h"
#include "c_playerresource.h"
#include <vgui/IVGui.h>
#include <vgui_controls/EditablePanel.h>
#include "c_tf_player.h"
#include "vgui/controls/tf_advmodelpanel.h"
#include "view.h"
#include "tf_hud_inspectpanel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;
DECLARE_HUDELEMENT( CHudInspectPanel );

static float s_flLastInspectDownTime = 0.f;

void InspectDown()
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
		return;

	s_flLastInspectDownTime = gpGlobals->curtime;

	KeyValues *kv = new KeyValues( "+inspect_server" );
	engine->ServerCmdKeyValues( kv );
	pLocalPlayer->m_flInspectTime = gpGlobals->curtime;
}
static ConCommand inspect_down_cmd( "+inspect", InspectDown, "", FCVAR_SERVER_CAN_EXECUTE );

void InspectUp()
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
		return;

	if ( gpGlobals->curtime - s_flLastInspectDownTime <= 0.2f )
	{
		CHudInspectPanel *pElement = GET_HUDELEMENT( CHudInspectPanel );
		if ( pElement )
		{
			pElement->UserCmd_InspectTarget();
		}
	}

	KeyValues *kv = new KeyValues( "-inspect_server" );
	engine->ServerCmdKeyValues( kv );
	pLocalPlayer->m_flInspectTime = 0;
}
static ConCommand inspect_up_cmd( "-inspect", InspectUp, "", FCVAR_SERVER_CAN_EXECUTE );


CHudInspectPanel::CHudInspectPanel( char const *pszElementName )
	: CHudElement( pszElementName ), BaseClass( NULL, "HudInspectPanel" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	// TODO: CItemModelPanel...
	m_pItemPanel = new CTFAdvModelPanel( this, "itempanel" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudInspectPanel::ApplySchemeSettings( IScheme *pScheme )
{
	// load control settings...
	LoadControlSettings( "resource/UI/HudInspectPanel.res" );

	BaseClass::ApplySchemeSettings( pScheme );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudInspectPanel::ShouldDraw( void )
{
	C_TFPlayer *pLocalTFPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !CHudElement::ShouldDraw() || !pLocalTFPlayer || !pLocalTFPlayer->IsAlive() )
	{
		//m_hTarget = NULL;
		SetPanelVisible( false );
		return false;
	}

	//return ( m_hTarget != NULL );
	return false;
}

int CHudInspectPanel::HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
{

	return 1;	// key not handled
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudInspectPanel::LockInspectRenderGroup( bool bLock )
{
	int iIndex = gHUD.LookupRenderGroupIndexByName( "inspect_panel" );
	if ( bLock )
	{
		gHUD.LockRenderGroup( iIndex );
	}
	else
	{
		gHUD.UnlockRenderGroup( iIndex );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudInspectPanel::SetPanelVisible( bool bVisible )
{
	if ( m_pItemPanel->IsVisible() != bVisible )
	{
		LockInspectRenderGroup( bVisible );
		m_pItemPanel->SetVisible( bVisible );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudInspectPanel::UserCmd_InspectTarget( void )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
		return;

	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TFPlayer *CHudInspectPanel::GetInspectTarget( C_TFPlayer *pPlayer )
{
	if ( !pPlayer )
		return NULL;

	trace_t tr;
	Vector vecStart, vecEnd;
	VectorMA( MainViewOrigin(), 10, MainViewForward(), vecStart );
	VectorMA( MainViewOrigin(), MAX_TRACE_LENGTH, MainViewForward(), vecEnd );

	UTIL_TraceLine( vecStart, vecEnd, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr );

	C_TFPlayer *pTarget = NULL;
	if ( !tr.startsolid && tr.DidHitNonWorldEntity() )
	{
		if ( tr.m_pEnt && tr.m_pEnt->IsPlayer() )
		{
			pTarget = ToTFPlayer( tr.m_pEnt );

			// Inspect the person a spy is disguised as instead of the spy, no cheating!
			if ( pTarget->IsPlayerClass( TF_CLASS_SPY ) )
			{
				if ( pTarget->GetTeamNumber() != pPlayer->GetTeamNumber() && pTarget->m_Shared.GetDisguiseTeam() == pPlayer->GetTeamNumber() )
				{
					C_TFPlayer *pDisguiseTarget = ToTFPlayer( pTarget->m_Shared.GetDisguiseTarget() );
					if ( pDisguiseTarget && pDisguiseTarget->IsPlayerClass( pTarget->m_Shared.GetDisguiseClass() ) )
					{
						pTarget = pDisguiseTarget;
					}
				}
			}
		}
	}

	return pTarget;
}
