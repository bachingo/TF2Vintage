#include "cbase.h"
#include "particle_parse.h"
#include "tf_player.h"
#include "ghost_behavior.h"

char const *CGhostBehavior::GetName( void ) const
{
	return "Behavior";
}

ActionResult<CGhost> CGhostBehavior::OnStart( CGhost *me, Action<CGhost> *priorAction )
{
	m_lifeTimer.Start( me->GetLifetime() );

	m_vecLastLocation = me->GetAbsOrigin();
	m_movementTimer.Start( 1.0 );

	DispatchParticleEffect( "ghost_appearation", me->WorldSpaceCenter(), me->GetAbsAngles() );

	return Continue();
}

ActionResult<CGhost> CGhostBehavior::Update( CGhost *me, float interval )
{
	if ( m_lifeTimer.IsElapsed() || m_movementTimer.IsElapsed() )
	{
		DispatchParticleEffect( "ghost_appearation", me->WorldSpaceCenter(), me->GetAbsAngles() );

		me->EmitSound( "Halloween.Haunted" );

		UTIL_Remove( me );
		return Done();
	}

	if ( m_moanTimer.IsElapsed() )
	{
		me->EmitSound( "Halloween.GhostMoan" );
		m_moanTimer.Start( RandomFloat( 5.0f, 7.0f ) );
	}

	DriftAroundAndAvoidObstacles( me );

	if ( m_scareTimer.IsElapsed() )
	{
		m_scareTimer.Start( 1.0f );

		CUtlVector<CTFPlayer *> victims;
		CollectPlayers( &victims, TF_TEAM_RED, true );
		CollectPlayers( &victims, TF_TEAM_BLUE, true, true );

		FOR_EACH_VEC( victims, i )
		{
			CTFPlayer *pVictim = victims[i];
			if ( pVictim->m_purgatoryDuration.HasStarted() && !pVictim->m_purgatoryDuration.IsElapsed() )
				continue;

			if ( !me->IsLineOfSightClear( pVictim ) || !me->IsRangeLessThan( pVictim, 192.0 ) )
				continue;

			pVictim->m_Shared.StunPlayer( 2.0, 0.0, 1.0, TF_STUNFLAGS_GHOSTSCARE, NULL );
		}
	}

	return Continue();
}

void CGhostBehavior::DriftAroundAndAvoidObstacles( CGhost *me )
{
	Vector vecFwd;
	me->GetVectors( &vecFwd, NULL, NULL );

	CTraceFilterNoNPCsOrPlayer filter( me, COLLISION_GROUP_NONE );
	const Vector vecLeft( -vecFwd.y, vecFwd.x, 0 );
	const Vector vecRight( vecFwd.y, -vecFwd.x, 0 );

	trace_t trLeft;
	UTIL_TraceLine( me->WorldSpaceCenter(), me->WorldSpaceCenter() + ( vecFwd + vecLeft ) * 150.0, MASK_PLAYERSOLID, &filter, &trLeft );

	trace_t trRight;
	UTIL_TraceLine( me->WorldSpaceCenter(), me->WorldSpaceCenter() + ( vecFwd + vecRight ) * 150.0, MASK_PLAYERSOLID, &filter, &trRight );

	if ( trLeft.DidHit() )
	{
		if ( !trRight.DidHit() || trRight.fraction > trLeft.fraction )
			vecFwd += vecRight * 0.2;
		else
			vecFwd += vecLeft * 0.2;
	}
	else
	{
		if ( trRight.DidHit() )
			vecFwd += vecLeft * 0.2;
	}

	vecFwd.NormalizeInPlace();

	Vector vecGoal = me->GetAbsOrigin() + vecFwd * 100.0;

	me->GetLocomotionInterface()->Approach( vecGoal );
	me->GetLocomotionInterface()->FaceTowards( vecGoal );
	me->GetLocomotionInterface()->Run();

	if ( me->IsRangeGreaterThan( m_vecLastLocation, 50.0f ) )
	{
		m_vecLastLocation = me->GetAbsOrigin();
		m_movementTimer.Reset();
	}
}
