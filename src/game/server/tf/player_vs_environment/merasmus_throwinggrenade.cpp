//========= Copyright � Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "tf_player.h"
#include "particle_parse.h"
#include "merasmus_throwinggrenade.h"
#include "merasmus_stunned.h"


CMerasmusThrowingGrenade::CMerasmusThrowingGrenade( CTFPlayer *pTarget )
{
	m_hVictim = pTarget;
}


char const *CMerasmusThrowingGrenade::GetName( void ) const
{
	return "Throwing Grenade";
}


ActionResult<CMerasmus> CMerasmusThrowingGrenade::OnStart( CMerasmus *me, Action<CMerasmus> *priorAction )
{
	// If we lost sight of our initial target, just throw it at anyone
	if ( !me->IsLineOfSightClear( m_hVictim ) )
	{
		CUtlVector<CTFPlayer *> players;
		CollectPlayers( &players, TF_TEAM_RED, true );
		CollectPlayers( &players, TF_TEAM_BLUE, true, true );

		CUtlVector<CTFPlayer *> validPlayers;
		FOR_EACH_VEC( players, i )
		{
			if ( players[i] == m_hVictim )
				continue;
			if ( !me->IsLineOfSightClear( players[i] ) )
				continue;

			validPlayers.AddToTail( players[i] );
		}

		if ( !validPlayers.IsEmpty() )
			m_hVictim = validPlayers.Random();
		else
			m_hVictim = NULL;
	}

	if ( !m_hVictim )
		return Done( "No target." );

	m_PathFollower.SetMinLookAheadDistance( 100.0f );
	me->GetLocomotionInterface()->FaceTowards( m_hVictim->WorldSpaceCenter() );

	const int nLayer = me->AddGesture( ACT_MP_ATTACK_STAND_ITEM1 );
	const float flDuration = me->GetLayerDuration( nLayer );
	m_sequenceDuration.Start( flDuration );

	m_actionDelay.Start( 0.25 );

	int nStaff = me->FindBodygroupByName( "staff" );
	me->SetBodygroup( nStaff, 2 );

	return Continue();
}

ActionResult<CMerasmus> CMerasmusThrowingGrenade::Update( CMerasmus *me, float dt )
{
	if ( me->IsStunned() )
		return ChangeTo( new CMerasmusStunned, "Stun interrupt!" );

	if ( m_sequenceDuration.IsElapsed() )
		return Done( "Fire in the hole!" );

	if ( m_actionDelay.HasStarted() && m_actionDelay.IsElapsed() )
	{
		m_actionDelay.Invalidate();

		DispatchParticleEffect( "merasmus_shoot", PATTACH_ABSORIGIN_FOLLOW, me, "effect_hand_R" );

		Vector vecOrigin; QAngle vecAngles;
		me->GetAttachment( "effect_hand_R", vecOrigin, vecAngles );

		Vector vecFwd, vecRight, vecUp;
		AngleVectors( me->EyeAngles(), &vecFwd, &vecRight, &vecUp );
		vecRight *= RandomFloat( -10, 10 );
		vecUp *= RandomFloat( -10, 10 );

		const float flProjSpeed = RandomFloat( 1500.0f, 2000.0f );
		const Vector vecOffset = 200 * vecUp;

		Vector vecVelocity = ( vecRight + vecUp ) + vecOffset;
		vecVelocity += vecFwd * flProjSpeed;

		if ( CMerasmus::CreateMerasmusGrenade( vecOrigin, vecVelocity, me, 1.0 ) )
		{
			if ( RandomInt( 0, 6 ) == 0 )
			{
				CPVSFilter filter( me->WorldSpaceCenter() );
				if ( RandomFloat( 1, 10 ) == 1 )
					me->PlayLowPrioritySound( filter, "Halloween.MerasmusGrenadeThrowRare" );
				else
					me->PlayLowPrioritySound( filter, "Halloween.MerasmusGrenadeThrow" );
			}
		}
	}

	if ( m_hVictim.Get() )
	{
		if ( me->IsRangeGreaterThan( m_hVictim, 100.0f ) || !me->IsLineOfSightClear( m_hVictim ) )
		{
			if ( m_PathFollower.GetAge() > 1.0 )
			{
				CMerasmusPathCost func( me );
				m_PathFollower.Compute( me, m_hVictim, func );
			}

			m_PathFollower.Update( me );
		}

		me->GetLocomotionInterface()->FaceTowards( m_hVictim->WorldSpaceCenter() );
	}

	return Continue();
}

void CMerasmusThrowingGrenade::OnEnd( CMerasmus *me, Action<CMerasmus> *newAction )
{
	int nStaff = me->FindBodygroupByName( "staff" );
	me->SetBodygroup( nStaff, 0 );
}
