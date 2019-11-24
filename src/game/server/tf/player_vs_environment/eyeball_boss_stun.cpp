//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "eyeball_boss_stun.h"

extern ConVar tf_eyeball_boss_hover_height;


CEyeBallBossStunned::CEyeBallBossStunned()
{
}


const char *CEyeBallBossStunned::GetName( void ) const
{
	return "Stunned";
}


ActionResult<CEyeBallBoss> CEyeBallBossStunned::OnStart( CEyeBallBoss *me, Action<CEyeBallBoss> *priorAction )
{
	m_stunDuration.Start( 5.0f );

	int iSequence = me->LookupSequence( "stunned" );
	if ( iSequence )
	{
		me->SetSequence( iSequence );
		me->SetPlaybackRate( 1.0f );
		me->SetCycle( 0.0f );
		me->ResetSequenceInfo();
	}

	me->EmitSound( "Halloween.EyeballBossStunned" );

	me->GetLocomotionInterface()->SetDesiredAltitude( 0 );

	int iMaxHealth = me->GetMaxHealth();
	me->m_iOldHealth = ( ( 1431655766LL * iMaxHealth ) >> 32 ) - ( iMaxHealth >> 31 );

	return Action<CEyeBallBoss>::Continue();
}

ActionResult<CEyeBallBoss> CEyeBallBossStunned::Update( CEyeBallBoss *me, float dt )
{
	if ( m_stunDuration.IsElapsed() )
	{
		me->BecomeEnraged( 20.0f );
		return Action<CEyeBallBoss>::Done();
	}

	return Action<CEyeBallBoss>::Continue();
}

void CEyeBallBossStunned::OnEnd( CEyeBallBoss *me, Action<CEyeBallBoss> *newAction )
{
	me->m_hTarget = nullptr;

	CEyeBallBossLocomotion *pLoco = (CEyeBallBossLocomotion *)me->GetLocomotionInterface();
	pLoco->SetDesiredAltitude( tf_eyeball_boss_hover_height.GetFloat() );
}


EventDesiredResult<CEyeBallBoss> CEyeBallBossStunned::OnInjured( CEyeBallBoss *me, const CTakeDamageInfo& info )
{
	return Action<CEyeBallBoss>::TryToSustain( RESULT_CRITICAL );
}
