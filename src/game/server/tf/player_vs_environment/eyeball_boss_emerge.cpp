//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "eyeball_boss_emerge.h"
#include "eyeball_boss_idle.h"

CEyeBallBossEmerge::CEyeBallBossEmerge()
{
}

const char *CEyeBallBossEmerge::GetName( void ) const
{
	return "Emerge";
}

ActionResult<CEyeBallBoss> CEyeBallBossEmerge::OnStart( CEyeBallBoss *me, Action<CEyeBallBoss> *priorAction )
{
	if ( me->GetTeamNumber() != TF_TEAM_NPC )
		return Action<CEyeBallBoss>::Done();

	int iSequence = me->LookupSequence( "arrives" );
	if ( iSequence > 0 )
	{
		me->SetSequence( iSequence );
		me->SetPlaybackRate( 1.0f );
		me->ResetSequenceInfo();
	}

	m_emergeTimer.Start( 3.0f );

	// Commenting this out until missing DispatchParticleEffect method is added
	//DispatchParticleEffect( "halloween_boss_summon", me->GetAbsOrigin(), me->GetAbsAngles() );

	m_vecTarget = me->GetAbsOrigin() + Vector( 0, 0, 100 );
	m_flDistance = 150.0f;

	me->SetAbsOrigin( m_vecTarget - Vector( 0, 0, 150 ) );
	me->EmitSound( "Halloween.HeadlessBossSpawnRumble" );

	IGameEvent *event = gameeventmanager->CreateEvent( "eyeball_boss_summoned" );
	if ( event )
	{
		event->SetInt( "level", me->GetLevel() );
		gameeventmanager->FireEvent( event );
	}

	return Action<CEyeBallBoss>::Continue();
}

ActionResult<CEyeBallBoss> CEyeBallBossEmerge::Update( CEyeBallBoss *me, float dt )
{
	if ( !m_emergeTimer.IsElapsed() )
	{
		Vector vec = m_vecTarget + Vector( 0, 0, ( m_emergeTimer.GetRemainingTime() * -m_flDistance ) / m_emergeTimer.GetCountdownDuration() );
		me->SetAbsOrigin( vec );

		if ( m_shakeTimer.IsElapsed() )
		{
			m_shakeTimer.Start( 0.25f );
			UTIL_ScreenShake( me->GetAbsOrigin(), 15.0f, 5.0f, 1.0f, 1000.0f, SHAKE_START );
		}
	}

	if ( me->IsSequenceFinished() )
	{
		return Action<CEyeBallBoss>::ChangeTo( new CEyeBallBossIdle, "Here I am!" );
	}
	else if ( me->GetTeamNumber() == TF_TEAM_NPC )
	{

		// TODO: Hurt or destroy things in my way when spawning after 2 seconds
	}

	return Action<CEyeBallBoss>::Continue();
}
