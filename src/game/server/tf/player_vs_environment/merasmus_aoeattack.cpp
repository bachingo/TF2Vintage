//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "nav_mesh/tf_nav_mesh.h"
#include "nav_pathfind.h"
#include "sceneentity.h"
#include "merasmus_aoeattack.h"


char const *CMerasmusAOEAttack::GetName( void ) const
{
	return "AOE Attack";
}


ActionResult<CMerasmus> CMerasmusAOEAttack::OnStart( CMerasmus *me, Action<CMerasmus> *priorAction )
{
	CTFNavArea *pStartArea = TFNavMesh()->GetMainControlPointArea( 0 );
	if ( !pStartArea )
	{
		return Done( "No control point!" );
	}

	CollectSurroundingAreas( &m_attackAreas, pStartArea, 400.0f, StepHeight, StepHeight );
	if ( m_attackAreas.IsEmpty() )
	{
		return Done( "No nav areas near control point!" );
	}

	me->m_bDoingAOEAttack = true;

	m_attackDelay.Start( 4.0f );
	m_nAttackState = AOE_BEGIN;
	m_pTargetArea = NULL;

	me->GetBodyInterface()->StartActivity( ACT_RANGE_ATTACK1 );

	CFmtStr vcdName( "scenes/bot/merasmus/low/bomb_attack_00%d.vcd", RandomInt( 1, 9 ) );
	InstancedScriptedScene( me, vcdName.Get(), NULL, 0.0f, false, NULL, true );

	int nStaff = me->FindBodygroupByName( "staff" );
	me->SetBodygroup( nStaff, 2 );

	return Continue();
}

ActionResult<CMerasmus> CMerasmusAOEAttack::Update( CMerasmus *me, float dt )
{
	if ( me->IsSequenceFinished() )
		return Done();

	switch ( m_nAttackState )
	{
		case AOE_BEGIN:
		{
			if ( m_attackDelay.IsElapsed() )
			{
				m_nAttackState = AOE_FIRING;
				m_attackTimer.Start( 0.5f );
			}
		}
		break;
		case AOE_FIRING:
		{
			if ( m_attackTimer.IsElapsed() )
			{
				if ( RandomInt(1, 100) > 49 )
					QueueBombRingsForLaunch( me );
				else
					QueueBombSpokesForLaunch( me );

				m_attackTimer.Start( 3.0 );
			}

			if ( !m_queuedBombs.IsEmpty() )
			{
				const Vector vecOrigin = me->WorldSpaceCenter() + Vector( 0, 0, 150.0 );
				FOR_EACH_VEC_BACK( m_queuedBombs, i )
				{
					CMerasmus::CreateMerasmusGrenade( vecOrigin, m_queuedBombs[i].velocity, me, 1.0 );

					m_queuedBombs.FastRemove( i );
				}
			}

			if ( m_recomputeWanderTimer.IsElapsed() )
			{
				m_recomputeWanderTimer.Start( RandomFloat( 1.0, 3.0 ) );
				m_pTargetArea = m_attackAreas.Random();
			}

			CMerasmusFlyingLocomotion *pLoco = assert_cast<CMerasmusFlyingLocomotion *>( me->GetLocomotionInterface() );
			pLoco->SetDesiredAltitude( FastCos( gpGlobals->curtime ) * 25.f + 175.f );

			const Vector vecCenter = m_pTargetArea->GetCenter();
			Vector vecTargetLoc( vecCenter.x, vecCenter.y, me->GetAbsOrigin().z );
			pLoco->Approach( vecTargetLoc );
			pLoco->FaceTowards( vecTargetLoc );
		}
		break;
	}

	return Continue();
}

void CMerasmusAOEAttack::OnEnd( CMerasmus *me, Action<CMerasmus> *newAction )
{
	me->m_bDoingAOEAttack = false;

	int nStaff = me->FindBodygroupByName( "staff" );
	me->SetBodygroup( nStaff, 0 );

	me->ResetStunCount();
}


void CMerasmusAOEAttack::QueueBombRingsForLaunch( CMerasmus *me )
{
	m_queuedBombs.RemoveAll();

	for ( int i=0; i < 2; ++i )
	{
		const float flScale = i / 2;
		const float flSpeed = ( 1900.0 * flScale ) + 100.0f;

		const float flResolution = ( 1.0 - flScale ) * 30.f + 20.f;
		QAngle angDirection( 0, 0, 0 );
		for ( float a = 0; a < 360; a += flResolution )
		{
			Vector vecFwd;
			AngleVectors( angDirection, &vecFwd );

			Vector vecVelocity = vecFwd * flSpeed;
			vecVelocity.z = 750.0f;

			m_queuedBombs.AddToTail( {vecVelocity, me} );

			angDirection.y += flResolution;
		}
	}
}

void CMerasmusAOEAttack::QueueBombSpokesForLaunch( CMerasmus *me )
{
	m_queuedBombs.RemoveAll();

	QAngle angDirection = me->EyeAngles();

	for ( int i=8; i; --i )
	{
		Vector vecFwd;
		AngleVectors( angDirection, &vecFwd );

		for ( int j=0; j < 4; ++j )
		{
			const float flSpeed = ( j * 475.0f ) + 100.0f;
			Vector vecVelocity = vecFwd * flSpeed;
			vecVelocity.z = 750.0f;

			m_queuedBombs.AddToTail( {vecVelocity, me} );
		}

		angDirection.y += 45.f;
	}
}
