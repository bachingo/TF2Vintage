//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "tf_obj_sentrygun.h"
#include "tf_projectile_rocket.h"
#include "eyeball_boss_rockets.h"

extern ConVar tf_eyeball_boss_speed;


CEyeBallBossLaunchRockets::CEyeBallBossLaunchRockets()
{
}


const char *CEyeBallBossLaunchRockets::GetName( void ) const
{
	return "LaunchRockets";
}


ActionResult<CEyeBallBoss> CEyeBallBossLaunchRockets::OnStart( CEyeBallBoss *me, Action<CEyeBallBoss> *priorAction )
{
	CBaseCombatCharacter *pVictim = me->GetVictim();
	if ( !pVictim || !pVictim->IsAlive() )
		return Action<CEyeBallBoss>::Done( "No target" );

	m_vecShootAt = pVictim->GetAbsOrigin();
	if ( pVictim->IsBaseObject() )
	{
		if ( dynamic_cast<CObjectSentrygun *>( pVictim ) )
			m_vecShootAt = pVictim->EyePosition();
	}

	int iSequence = 0;
	if ( me->GetTeamNumber() != TF_TEAM_NPC ||
		 me->GetHealth() < ( me->GetMaxHealth() / 3 ) ||
		 ( me->m_attitudeTimer.HasStarted() && !me->m_attitudeTimer.IsElapsed() ) )
	{
		m_iNumRockets = 3;
		m_rocketLaunchDelay.Start( 0.25f );

		me->EmitSound( "Halloween.EyeballBossRage" );

		iSequence = me->LookupSequence( "firing3" );
	}
	else if ( me->GetTeamNumber() != TF_TEAM_NPC ||
			  me->GetHealth() < ( me->GetMaxHealth() / 3 ) ||
			  ( me->m_attitudeTimer.HasStarted() && !me->m_attitudeTimer.IsElapsed() ) ||
			  me->GetHealth() >= 2 * ( me->GetMaxHealth() / 3 ) )
	{
		m_iNumRockets = 1;
		m_rocketLaunchDelay.Start( 0.5f );

		iSequence = me->LookupSequence( "firing1" );
	}
	else
	{
		m_iNumRockets = 3;
		m_rocketLaunchDelay.Start( 0.25f );

		iSequence = me->LookupSequence( "firing2" );
	}

	if ( iSequence )
	{
		me->SetSequence( iSequence );
		me->SetPlaybackRate( 1.0f );
		me->SetCycle( 0.0f );
		me->ResetSequenceInfo();
	}

	return Action<CEyeBallBoss>::Continue();
}

ActionResult<CEyeBallBoss> CEyeBallBossLaunchRockets::Update( CEyeBallBoss *me, float dt )
{
	CBaseCombatCharacter *pVictim = me->GetVictim();
	if ( pVictim && pVictim->IsAlive() )
	{
		Vector vecFwd, vecRight;
		me->GetVectors( &vecFwd, &vecRight, NULL );

		if ( me->GetTeamNumber() == TF_TEAM_NPC )
		{
			if ( pVictim->GetFlags() & FL_ONGROUND )
			{
				Vector vecApproach = pVictim->WorldSpaceCenter();
				me->GetLocomotionInterface()->Approach( vecApproach + vecRight * 100.0f );
			}
			else
			{
				Vector vecApproach = pVictim->WorldSpaceCenter();
				me->GetLocomotionInterface()->Approach( vecApproach + vecRight * -100.0f );
			}
		}

		me->GetBodyInterface()->AimHeadTowards( pVictim );
	}
	else
	{
		me->GetBodyInterface()->AimHeadTowards( m_vecShootAt );
	}

	if ( !m_rocketLaunchDelay.IsElapsed() || !m_refireDelay.IsElapsed() )
		return Action<CEyeBallBoss>::Continue();

	float flSpeed = 330.0f;
	if ( me->GetTeamNumber() != TF_TEAM_NPC ||
		 me->GetHealth() < ( me->GetMaxHealth() / 3 ) ||
		 ( me->m_attitudeTimer.HasStarted() && !me->m_attitudeTimer.IsElapsed() ) )
	{
		flSpeed = 1100.0f;
	}

	m_iNumRockets--;
	m_refireDelay.Start( 0.3f );

	Vector vecShootAt = m_vecShootAt;

	// If we're angry we suddenly can predict where to fire our rockets
	if ( me->GetTeamNumber() != TF_TEAM_NPC ||
		 me->GetHealth() < ( me->GetMaxHealth() / 3 ) ||
		 ( me->m_attitudeTimer.HasStarted() && !me->m_attitudeTimer.IsElapsed() ) )
	{
		if ( pVictim && me->GetRangeTo( pVictim ) > 150.0f )
		{
			float flComp = me->GetRangeTo( pVictim ) / flSpeed;
			Vector vecAdjustment = pVictim->GetAbsVelocity() * flComp;

			CTraceFilterNoNPCsOrPlayer filter( me, COLLISION_GROUP_NONE );

			trace_t trace;
			UTIL_TraceLine( me->WorldSpaceCenter(), pVictim->GetAbsOrigin() + vecAdjustment, MASK_SOLID_BRUSHONLY, &filter, &trace );

			if ( trace.DidHit() && ( trace.endpos - vecShootAt ).LengthSqr() > Square( 300.0f ) )
				vecAdjustment = vec3_origin;

			vecShootAt = vecAdjustment + pVictim->GetAbsOrigin();
		}
	}

	QAngle vecAng;
	VectorAngles( vecShootAt - me->WorldSpaceCenter(), vecAng );

	CTFProjectile_Rocket *pRocket = CTFProjectile_Rocket::Create( me, me->WorldSpaceCenter(), vecAng, me, me );
	if ( pRocket )
	{
		pRocket->SetModel( "models/props_halloween/eyeball_projectile.mdl" );
		pRocket->EmitSound( "Weapon_RPG.SingleCrit" );

		Vector vecDir;
		AngleVectors( vecAng, &vecDir );
		pRocket->SetAbsVelocity( vecDir * flSpeed );

		pRocket->SetDamage( 50.0f );
		pRocket->SetCritical( me->GetTeamNumber() == TF_TEAM_NPC );

		pRocket->ChangeTeam( me->GetTeamNumber() );
	}

	if ( m_iNumRockets != 0 )
		return Action<CEyeBallBoss>::Continue();
	else
		return Action<CEyeBallBoss>::Done();
}
