//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "entity_bossresource.h"
#include "eyeball_boss_escape.h"

CEyeBallBossEscape::CEyeBallBossEscape()
{
}

const char *CEyeBallBossEscape::GetName( void ) const
{
	return "Escape";
}

ActionResult<CEyeBallBoss> CEyeBallBossEscape::OnStart( CEyeBallBoss *me, Action<CEyeBallBoss> *priorAction )
{
	int iSequence = me->LookupSequence( "escape" );
	if ( iSequence )
	{
		me->SetSequence( iSequence );
		me->SetPlaybackRate( 1.0f );
		me->SetCycle( 0.0f );
		me->ResetSequenceInfo();
	}

	me->EmitSound( "Halloween.EyeballBossLaugh" );

	return Action<CEyeBallBoss>::Continue();
}

ActionResult<CEyeBallBoss> CEyeBallBossEscape::Update( CEyeBallBoss *me, float dt )
{
	if ( me->IsSequenceFinished() )
	{
		if ( me->GetTeamNumber() != TF_TEAM_NPC )
			me->EmitSound( "Halloween.spell_spawn_boss_disappear" );

		// Commenting this out until missing DispatchParticleEffect method is added
		//DispatchParticleEffect( "eyeboss_tp_escape", me->GetAbsOrigin(), me->GetAbsAngles() );

		if ( g_pMonsterResource )
			g_pMonsterResource->HideBossHealthMeter();

		UTIL_Remove( me );

		me->m_hTarget = nullptr;
		if ( me->GetTeamNumber() == TF_TEAM_NPC )
		{
			IGameEvent *event = gameeventmanager->CreateEvent( "eyeball_boss_escaped" );
			if ( event )
			{
				event->SetInt( "level", me->GetLevel() );

				gameeventmanager->FireEvent( event );
			}
		}

		CEyeBallBoss::m_level = 1;

		return Action<CEyeBallBoss>::Done();
	}

	return Action<CEyeBallBoss>::Continue();
}
