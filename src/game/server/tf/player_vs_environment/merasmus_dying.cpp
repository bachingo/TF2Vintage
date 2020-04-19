//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "tf_gamerules.h"
#include "entity_teleportvortex.h"
#include "merasmus_dying.h"


char const *CMerasmusDying::GetName( void ) const
{
	return "Dying";
}


ActionResult<CMerasmus> CMerasmusDying::OnStart( CMerasmus *me, Action<CMerasmus> *priorAction )
{
	me->GetBodyInterface()->StartActivity( ACT_DIESIMPLE );

	me->PlayHighPrioritySound( "Halloween.MerasmusBanish" );
	TFGameRules()->BroadcastSound( 255, "Halloween.Merasmus_Death" );

	return Continue();
}

ActionResult<CMerasmus> CMerasmusDying::Update( CMerasmus *me, float dt )
{
	if ( me->IsSequenceFinished() )
	{
		extern void DispatchParticleEffect( const char *pszParticleName, Vector vecOrigin, QAngle vecAngles, CBaseEntity *pEntity = NULL );
		DispatchParticleEffect( "merasmus_spawn", me->GetAbsOrigin(), me->GetAbsAngles() );

		IGameEvent *event = gameeventmanager->CreateEvent( "merasmus_killed" );
		if ( event )
		{
			event->SetInt( "level", me->GetLevel() );

			gameeventmanager->FireEvent( event );
		}

		me->TriggerLogicRelay( "boss_dead_relay", false );

		CTeleportVortex *pVortex = (CTeleportVortex *)CBaseEntity::Create( "teleport_vortex", me->WorldSpaceCenter(), vec3_angle );
		if ( pVortex )
			pVortex->SetupVortex( true, true );

		++CMerasmus::m_level;

		me->StartRespawnTimer();
		UTIL_Remove( me );

		return Done();
	}

	return Continue();
}
