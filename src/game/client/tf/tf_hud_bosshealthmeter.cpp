#include "cbase.h"
#include "c_tf_player.h"
#include "iclientmode.h"
#include "ienginevgui.h"
#include "tf_hud_bosshealthmeter.h"
#include "entity_bossresource.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

ConVar tf_hud_boss_show_stun( "cl_boss_show_stun", "0", FCVAR_DEVELOPMENTONLY );


DECLARE_HUDELEMENT( CHudBossHealthMeter );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudBossHealthMeter::CHudBossHealthMeter( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudBossHealth" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_pHealthBarPanel = new EditablePanel( this, "HealthBarPanel" );

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	ivgui()->AddTickSignal( GetVPanel() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudBossHealthMeter::ApplySchemeSettings( IScheme *scheme )
{
	LoadControlSettings( "resource/UI/HudBossHealth.res" );

	m_pStunMeter = dynamic_cast<ContinuousProgressBar *>( FindChildByName( "StunMeter" ) );
	m_pBorderImage = dynamic_cast<ImagePanel *>( FindChildByName( "BorderImage" ) );
	m_pBarImage = dynamic_cast<ImagePanel *>( m_pHealthBarPanel->FindChildByName( "BarImage" ) );

	BaseClass::ApplySchemeSettings( scheme );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudBossHealthMeter::ShouldDraw( void )
{
	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if(!pLocalPlayer || pLocalPlayer->GetObserverMode() == OBS_MODE_FREEZECAM)
		return false;

	if (!CHudElement::ShouldDraw() || !g_pMonsterResource)
		return false;

	return g_pMonsterResource->GetBossHealthPercentage() > 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudBossHealthMeter::OnTick( void )
{
	int x = 0, y = 0;
	GetPos( x, y );
	y = m_nHealthAlivePosY;

	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if (pLocalPlayer && !pLocalPlayer->IsAlive())
		y = m_nHealthDeadPosY;

	SetPos( x, y );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudBossHealthMeter::Update( void )
{
	if (g_pMonsterResource && m_pHealthBarPanel)
	{
		m_pHealthBarPanel->SetWide( (int)( m_nHealthBarWide * g_pMonsterResource->GetBossHealthPercentage() ) );
		if (m_pStunMeter)
		{
			if (g_pMonsterResource->GetBossStunPercentage() > 0.0f && tf_hud_boss_show_stun.GetBool())
			{
				if (!m_pStunMeter->IsVisible())
					m_pStunMeter->SetVisible( true );

				m_pStunMeter->SetProgress( g_pMonsterResource->GetBossStunPercentage() );
			}
			else if (m_pStunMeter->IsVisible())
				m_pStunMeter->SetVisible( false );
		}
	}
}
