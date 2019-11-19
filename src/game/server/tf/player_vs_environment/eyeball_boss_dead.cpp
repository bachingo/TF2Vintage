//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "entity_teleportvortex.h"
#include "eyeball_boss_dead.h"


#define JARATE_RANGE 750.0f


CEyeBallBossDead::CEyeBallBossDead()
{
}


const char *CEyeBallBossDead::GetName( void ) const
{
	return "Dead";
}


ActionResult<CEyeBallBoss> CEyeBallBossDead::OnStart( CEyeBallBoss *me, Action<CEyeBallBoss> *priorAction )
{
	int iSequence = me->LookupSequence( "death" );
	if ( iSequence )
	{
		me->SetSequence( iSequence );
		me->SetPlaybackRate( 1.0f );
		me->SetCycle( 0.0f );
		me->ResetSequenceInfo();
	}

	me->EmitSound( "Halloween.EyeballBossStunned" );

	m_dyingDuration.Start( 10.0f );

	return Action<CEyeBallBoss>::Continue();
}

ActionResult<CEyeBallBoss> CEyeBallBossDead::Update( CEyeBallBoss *me, float dt )
{
	Vector vecGround = me->WorldSpaceCenter();
	TheNavMesh->GetSimpleGroundHeight( vecGround, &vecGround.z );

	if ( m_dyingDuration.IsElapsed() || ( me->WorldSpaceCenter().z - vecGround.z ) < 100.0f )
	{
		// Commenting this out until missing DispatchParticleEffect method is added
		//DispatchParticleEffect( "eyeboss_death", me->GetAbsOrigin(), me->GetAbsAngles() );

		me->EmitSound( "Cart.Explode" );
		me->EmitSound( "Halloween.EyeballBossDie" );

		UTIL_ScreenShake( me->GetAbsOrigin(), 25.0f, 5.0f, 5.0f, 1000.0f, SHAKE_START );

		UTIL_Remove( me );

		if ( me->GetTeamNumber() == TF_TEAM_NPC )
		{
			IGameEvent *event = gameeventmanager->CreateEvent( "eyeball_boss_killed" );
			if ( event )
			{
				event->SetInt( "level", me->GetLevel() );

				gameeventmanager->FireEvent( event );
			}

			CEyeBallBoss::m_level++;
		}

		me->JarateNearbyPlayer( JARATE_RANGE );

		CTeleportVortex *pVortex = (CTeleportVortex *)CBaseEntity::Create( "teleport_vortex", me->GetAbsOrigin(), vec3_angle );
		if ( pVortex )
			pVortex->SetupVortex( true, false );
	}

	return Action<CEyeBallBoss>::Continue();
}
