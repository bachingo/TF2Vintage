//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "headless_hatman_emerge.h"
#include "headless_hatman_attack.h"


CHeadlessHatmanEmerge::CHeadlessHatmanEmerge()
{
}


const char *CHeadlessHatmanEmerge::GetName( void ) const
{
	return "Emerge";
}


ActionResult<CHeadlessHatman> CHeadlessHatmanEmerge::OnStart( CHeadlessHatman *me, Action<CHeadlessHatman> *priorAction )
{
	me->GetBodyInterface()->StartActivity( ACT_TRANSITION );

	m_emergeTimer.Start( 3.0f );

	extern void DispatchParticleEffect( const char *pszParticleName, Vector vecOrigin, QAngle vecAngles, CBaseEntity *pEntity );
	DispatchParticleEffect( "halloween_boss_summon", me->GetAbsOrigin(), me->GetAbsAngles(), NULL );

	m_vecTarget = me->GetAbsOrigin() + Vector( 0, 0, 10 );
	m_flDistance = 200.0f;

	me->SetAbsOrigin( m_vecTarget - Vector( 0, 0, 200 ) );
	me->EmitSound( "Halloween.HeadlessBossSpawnRumble" );

	// TODO: Rotate towards closest player

	IGameEvent *event = gameeventmanager->CreateEvent( "pumpkin_lord_summoned" );
	if (event)
		gameeventmanager->FireEvent( event );

	return Action<CHeadlessHatman>::Continue();
}

ActionResult<CHeadlessHatman> CHeadlessHatmanEmerge::Update( CHeadlessHatman * me, float dt )
{
	if (!m_emergeTimer.IsElapsed())
	{
		Vector vec = m_vecTarget + Vector( 0, 0, ( m_emergeTimer.GetRemainingTime() * -m_flDistance ) / m_emergeTimer.GetCountdownDuration() );
		me->SetAbsOrigin( vec );

		if (m_shakeTimer.IsElapsed())
		{
			m_shakeTimer.Start( 0.25f );
			UTIL_ScreenShake( me->GetAbsOrigin(), 15.0f, 5.0f, 1.0f, 1000.0f, SHAKE_START );
		}
	}

	if (me->IsSequenceFinished())
		return Action<CHeadlessHatman>::ChangeTo( new CHeadlessHatmanAttack, "Here I come!" );


	// TODO: emerging pushaway forces

	return Action<CHeadlessHatman>::Continue();
}
