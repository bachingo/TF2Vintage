//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "entity_teleportvortex.h"
#include "eyeball_boss_teleport.h"


CEyeBallBossTeleport::CEyeBallBossTeleport()
{
}


const char *CEyeBallBossTeleport::GetName( void ) const
{
	return "Teleport";
}


ActionResult<CEyeBallBoss> CEyeBallBossTeleport::OnStart( CEyeBallBoss *me, Action<CEyeBallBoss> *priorAction )
{
	int iSequence = me->LookupSequence( "teleport_out" );
	if ( iSequence )
	{
		me->SetSequence( iSequence );
		me->SetPlaybackRate( 1.0f );
		me->SetCycle( 0.0f );
		me->ResetSequenceInfo();
	}

	m_iTeleportState = TELEPORT_VANISH;

	return Action<CEyeBallBoss>::Continue();
}

ActionResult<CEyeBallBoss> CEyeBallBossTeleport::Update( CEyeBallBoss *me, float dt )
{
	if ( me->IsSequenceFinished() )
	{
		switch ( m_iTeleportState )
		{
			case TELEPORT_VANISH:
			{
				CTeleportVortex *pVortex = (CTeleportVortex *)CBaseEntity::Create( "teleport_vortex", me->GetAbsOrigin(), vec3_angle );
				if ( pVortex )
					pVortex->SetupVortex( false, false );

				// Commenting this out until missing DispatchParticleEffect method is added
				//DispatchParticleEffect( "eyeboss_tp_normal", me->GetAbsOrigin(), me->GetAbsAngles() );
				me->EmitSound( "Halloween.EyeballBossTeleport" );

				me->AddEffects( EF_NODRAW|EF_NOINTERP );

				Vector vecNewSpot = me->PickNewSpawnSpot();
				vecNewSpot.z += 75.0f;

				me->SetAbsOrigin( vecNewSpot );

				m_iTeleportState = TELEPORT_APPEAR;

				break;
			}
			case TELEPORT_APPEAR:
			{
				// Commenting this out until missing DispatchParticleEffect method is added
				//DispatchParticleEffect( "eyeboss_tp_normal", me->GetAbsOrigin(), me->GetAbsAngles() );

				int iSequence = me->LookupSequence( "teleport_in" );
				if ( iSequence )
				{
					me->SetSequence( iSequence );
					me->SetPlaybackRate( 1.0f );
					me->SetCycle( 0.0f );
					me->ResetSequenceInfo();
				}

				me->RemoveEffects( EF_NODRAW|EF_NOINTERP );

				m_iTeleportState = TELEPORT_FINISH;
				break;
			}
			case TELEPORT_FINISH:
				return Action<CEyeBallBoss>::Done();
		}
	}
	
	return Action<CEyeBallBoss>::Continue();
}
