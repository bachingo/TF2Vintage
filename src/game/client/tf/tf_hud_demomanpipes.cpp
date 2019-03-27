//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "c_tf_player.h"
#include "iclientmode.h"
#include "ienginevgui.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ProgressBar.h>
#include "engine/IEngineSound.h"
#include "tf_controls.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudDemomanPipes : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudDemomanPipes, EditablePanel );

public:
	CHudDemomanPipes( const char *pElementName );

	virtual void	ApplySchemeSettings( IScheme *scheme );
	virtual bool	ShouldDraw( void );
	virtual void	OnTick( void );

private:
	vgui::EditablePanel *m_pPipesPresent;
	vgui::EditablePanel *m_pNoPipesPresent;
	vgui::ContinuousProgressBar *m_pChargeMeter;
	CExLabel *m_pChargeLabel;
	

	float m_flOldCharge;
};

DECLARE_HUDELEMENT( CHudDemomanPipes );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudDemomanPipes::CHudDemomanPipes( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudDemomanPipes" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_pPipesPresent = new EditablePanel( this, "PipesPresentPanel" );
	m_pNoPipesPresent = new EditablePanel( this, "NoPipesPresentPanel" );
	m_pChargeMeter = new ContinuousProgressBar( this, "ChargeMeter" );
	m_pChargeLabel = new CExLabel( this, "ChargeLabel", "#TF_Charge" );
	m_flOldCharge = 1.0f;

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	vgui::ivgui()->AddTickSignal( GetVPanel() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudDemomanPipes::ApplySchemeSettings( IScheme *pScheme )
{
	// load control settings...
	LoadControlSettings( "resource/UI/HudDemomanPipes.res" );

	BaseClass::ApplySchemeSettings( pScheme );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudDemomanPipes::ShouldDraw( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pPlayer )
	{
		return false;
	}

	if ( !pPlayer->Weapon_OwnsThisID( TF_WEAPON_PIPEBOMBLAUNCHER ) && !pPlayer->m_Shared.HasDemoShieldEquipped() )
	{
		return false;
	}

	if ( !pPlayer->IsAlive() )
	{
		return false;
	}

	return CHudElement::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudDemomanPipes::OnTick( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pPlayer )
		return;

	// We're using a shield
	if ( !pPlayer->Weapon_OwnsThisID( TF_WEAPON_PIPEBOMBLAUNCHER ) ) 
	{
		m_pPipesPresent->SetVisible( false );
		m_pNoPipesPresent->SetVisible( false );
		m_pChargeMeter->SetVisible( true );
		m_pChargeLabel->SetVisible( true );

		float flCharge = pPlayer->m_Shared.GetShieldChargeMeter() / 100.0f;
		m_pChargeMeter->SetProgress( flCharge );

		// Play a ding when full charged.
		if ( m_flOldCharge < 1.0f && flCharge >= 1.0f )
		{
			CLocalPlayerFilter filter;
			C_BaseEntity::EmitSound( filter, SOUND_FROM_LOCAL_PLAYER, "TFPlayer.Recharged" );
		}

		m_flOldCharge = flCharge;

		// We're currently in the middle of a shield charge
		if ( pPlayer->m_Shared.InCond( TF_COND_SHIELD_CHARGE) )
		{
			// Set the charge color based on how far into the charge we are
			if ( flCharge > 0.66 )
			{
				// GREEN
				m_pChargeMeter->SetFgColor( Color( 153, 255, 153, 255 ) );
			}
			else if ( flCharge > 0.33 )
			{
				// YELLOW
				m_pChargeMeter->SetFgColor( Color( 255, 178, 0, 255 ) );
			}
			else
			{
				// RED
				m_pChargeMeter->SetFgColor( Color( 255, 0, 0, 255 ) );
			}
		}
		else
		{
			m_pChargeMeter->SetFgColor( COLOR_WHITE );
		}
	}
	else
	{
		int iPipes = pPlayer->GetNumActivePipebombs();

		m_pPipesPresent->SetDialogVariable( "activepipes", iPipes );
		m_pNoPipesPresent->SetDialogVariable( "activepipes", iPipes );

		m_pPipesPresent->SetVisible( iPipes > 0 );
		m_pNoPipesPresent->SetVisible( iPipes <= 0 );
		m_pChargeMeter->SetVisible( false );
		m_pChargeLabel->SetVisible( false );
	}
}