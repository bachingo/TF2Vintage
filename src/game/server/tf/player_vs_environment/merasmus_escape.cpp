//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "entity_bossresource.h"
#include "merasmus_escape.h"


char const *CMerasmusEscape::GetName( void ) const
{
	return "Escape";
}


ActionResult<CMerasmus> CMerasmusEscape::OnStart( CMerasmus *me, Action<CMerasmus> *priorAction )
{
	me->GetBodyInterface()->StartActivity( ACT_FLY );

	if ( RandomInt( 0, 10 ) == 0 )
		me->PlayHighPrioritySound( "Halloween.MerasmusDepartRare" );
	else
		me->PlayHighPrioritySound( "Halloween.MerasmusDepart" );

	UTIL_LogPrintf(
		"HALLOWEEN: merasmus_escaped (max_dps %3.2f) (health %d) (level %d)\n",
			me->m_flDPSMax, me->GetHealth(), me->GetLevel() );

	return Continue();
}

ActionResult<CMerasmus> CMerasmusEscape::Update( CMerasmus *me, float dt )
{
	if ( me->IsSequenceFinished() )
	{
		Vector vecOrigin; QAngle vecAngles;
		me->GetAttachment( "effect_robe", vecOrigin, vecAngles );
		
		extern void DispatchParticleEffect( const char *pszParticleName, Vector vecOrigin, QAngle vecAngles, CBaseEntity *pEntity = NULL );
		DispatchParticleEffect( "merasmus_tp", vecOrigin, vecAngles );

		IGameEvent *event = gameeventmanager->CreateEvent( "merasmus_escaped" );
		if ( event )
		{
			event->SetInt( "level", me->GetLevel() );

			gameeventmanager->FireEvent( event );
		}

		if ( g_pMonsterResource )
			g_pMonsterResource->HideBossHealthMeter();

		me->TriggerLogicRelay( "boss_exit_relay", false );

		CMerasmus::m_level = 1;

		me->StartRespawnTimer();
		UTIL_Remove( me );

		return Done();
	}

	return Continue();
}
