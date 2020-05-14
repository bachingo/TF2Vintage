#include "cbase.h"
#include "c_NextBot.h"
#include "engine/ivdebugoverlay.h"

ConVar NextBotShadowDist( "nb_shadow_dist", "400" );

IMPLEMENT_CLIENTCLASS( C_NextBotCombatCharacter, DT_NextBot, NextBotCombatCharacter )

BEGIN_RECV_TABLE( C_NextBotCombatCharacter, DT_NextBot )
END_RECV_TABLE()

C_NextBotCombatCharacter::C_NextBotCombatCharacter()
{
	m_nShadowType = SHADOWS_SIMPLE;
}

void C_NextBotCombatCharacter::Spawn( void )
{
	BaseClass::Spawn();
}

void C_NextBotCombatCharacter::UpdateClientSideAnimation( void )
{
	if ( !IsDormant() )
		BaseClass::UpdateClientSideAnimation();
}

void C_NextBotCombatCharacter::UpdateShadowLOD( void )
{
	ShadowType_t oldShadowType = m_nShadowType;

	if ( !C_BasePlayer::GetLocalPlayer() )
	{
		m_nShadowType = SHADOWS_SIMPLE;
	}
	else
	{
		Vector delta = GetAbsOrigin() - C_BasePlayer::GetLocalPlayer()->GetAbsOrigin();
		if ( delta.LengthSqr() > Square( NextBotShadowDist.GetFloat() ) )
		{
			m_nShadowType = SHADOWS_SIMPLE;
		}
		else
		{
			m_nShadowType = SHADOWS_RENDER_TO_TEXTURE_DYNAMIC;
		}
	}

	if ( oldShadowType != m_nShadowType )
	{
		DestroyShadow();
	}
}

ShadowType_t C_NextBotCombatCharacter::ShadowCastType( void )
{
	if ( !IsVisible() )
		return SHADOWS_NONE;
	
	if ( m_shadowUpdateTimer.IsElapsed() )
	{
		m_shadowUpdateTimer.Start( 0.15f );
		UpdateShadowLOD();
	}

	return m_nShadowType;
}
