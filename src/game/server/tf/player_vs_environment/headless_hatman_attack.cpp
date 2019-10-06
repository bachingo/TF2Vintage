//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "tf_player.h"
#include "tf_gamerules.h"
#include "team_control_point_master.h"
#include "nav_mesh/tf_nav_area.h"
#include "headless_hatman_attack.h"
#include "headless_hatman_terrify.h"

extern ConVar tf_halloween_bot_attack_range;
extern ConVar tf_halloween_bot_speed_recovery_rate;
extern ConVar tf_halloween_bot_chase_duration;
extern ConVar tf_halloween_bot_chase_range;
extern ConVar tf_halloween_bot_quit_range;


CHeadlessHatmanAttack::CHeadlessHatmanAttack()
{
}


const char *CHeadlessHatmanAttack::GetName( void ) const
{
	return "Attack";
}


ActionResult<CHeadlessHatman> CHeadlessHatmanAttack::OnStart( CHeadlessHatman *me, Action<CHeadlessHatman> *priorAction )
{
	m_PathFollower.SetMinLookAheadDistance( 100.0f );
	me->GetBodyInterface()->StartActivity( ACT_MP_RUN_ITEM1 );

	// investigate for inline reset function
	m_attackTimer.Invalidate();
	m_evilCackleTimer.Invalidate();
	m_chaseDuration.Invalidate();
	m_forcedTargetDuration.Invalidate();
	m_hAimTarget = nullptr;
	m_hOldTarget = nullptr;
	m_hTarget = nullptr;

	m_terrifyTimer.Start( 20.0f );
	m_vecHome = me->GetAbsOrigin();
	m_recomputeHomeTimer.Start( 3.0f );

	return Action<CHeadlessHatman>::Continue();
}

ActionResult<CHeadlessHatman> CHeadlessHatmanAttack::Update( CHeadlessHatman *me, float dt )
{
	if (!me->IsAlive())
		return Action<CHeadlessHatman>::Done();

	SelectVictim( me );
	RecomputeHomePosition();

	if (m_attackTimer.IsElapsed() && m_evilCackleTimer.IsElapsed())
	{
		m_evilCackleTimer.Start( RandomFloat( 3.0f, 5.0f ) );

		me->EmitSound( "Halloween.HeadlessBossLaugh" );

		int rand = RandomInt( 0, 100 );
		if (rand <= 24)
		{
			me->AddGesture( ACT_MP_GESTURE_VC_FISTPUMP_MELEE );
		}
		else if (rand <= 49)
		{
			me->AddGesture( ACT_MP_GESTURE_VC_FINGERPOINT_MELEE );
		}
	}

	CBaseCombatCharacter *pVictim = ToBaseCombatCharacter( TFGameRules()->GetIT() );
	if ( pVictim )
	{
		if ( m_notifyVictimTimer.IsElapsed() && !TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_DOOMSDAY ) )
		{
			CBasePlayer *pPlayer = ToBasePlayer( pVictim );
			if ( pPlayer )
			{
				m_notifyVictimTimer.Start( 7.0f );
				ClientPrint( pPlayer, HUD_PRINTCENTER, "#TF_HALLOWEEN_BOSS_WARN_VICTIM", pPlayer->GetPlayerName() );
			}
		}

		if ( !me->IsRangeGreaterThan( pVictim, 100.0f ) && me->IsAbleToSee( pVictim, CBaseCombatCharacter::USE_FOV ) )
		{
			if ( me->IsRangeLessThan( pVictim, tf_halloween_bot_attack_range.GetFloat() ) )
			{
				if ( m_terrifyTimer.IsElapsed() && pVictim->IsPlayer() )
				{
					m_terrifyTimer.Reset();
					return Action<CHeadlessHatman>::SuspendFor( new CHeadlessHatmanTerrify, "Boo!" );
				}

				if ( m_attackDuration.IsElapsed() && m_attackTimer.IsElapsed() )
				{
					if ( TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_DOOMSDAY ) )
						me->AddGesture( ACT_MP_ATTACK_STAND_ITEM2 );
					else
						me->AddGesture( ACT_MP_ATTACK_STAND_ITEM1 );

					m_attackTimer.Start( 0.58f );
					me->EmitSound( "Halloween.HeadlessBossAttack" );
					m_attackDuration.Start( 1.0f );
				}

				me->GetLocomotionInterface()->FaceTowards( pVictim->WorldSpaceCenter() );
			}

			UpdateAxeSwing( me );

			return Action<CHeadlessHatman>::Continue();
		}
		else
		{
			if (m_PathFollower.GetAge() > 1.0f)
			{
				CHeadlessHatmanPathCost func( me );
				m_PathFollower.Compute( me, pVictim, func );
			}

			m_PathFollower.Update( me );
		}
	}

	if (me->IsRangeGreaterThan( m_vecHome, 50.0f ))
	{
		m_PathFollower.Update( me );

		if (m_PathFollower.GetAge() > 3.0f)
		{
			CHeadlessHatmanPathCost func( me );
			m_PathFollower.Compute( me, m_vecHome, func );
		}
	}
	
	if (m_hAimTarget)
		me->GetLocomotionInterface()->FaceTowards( m_hAimTarget->WorldSpaceCenter() );

	if (m_hTarget && m_hTarget->IsAlive())
	{
		if (me->IsRangeLessThan( m_hTarget, tf_halloween_bot_attack_range.GetFloat() ))
		{
			if (m_terrifyTimer.IsElapsed() && m_hTarget->IsPlayer())
			{
				m_terrifyTimer.Reset();
				return Action<CHeadlessHatman>::SuspendFor( new CHeadlessHatmanTerrify, "Boo!" );
			}

			if ( m_attackDuration.IsElapsed() && m_attackTimer.IsElapsed() )
			{
				if ( TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_DOOMSDAY ) )
					me->AddGesture( ACT_MP_ATTACK_STAND_ITEM2 );
				else
					me->AddGesture( ACT_MP_ATTACK_STAND_ITEM1 );

				m_attackTimer.Start( 0.58f );
				me->EmitSound( "Halloween.HeadlessBossAttack" );
				m_attackDuration.Start( 1.0f );
			}

			me->GetLocomotionInterface()->FaceTowards( m_hTarget->WorldSpaceCenter() );
		}
	}

	UpdateAxeSwing( me );

	if (me->GetLocomotionInterface()->IsAttemptingToMove())
	{
		// TODO: deterministic animation from damage
		if ( !me->GetBodyInterface()->IsActivity( ACT_MP_RUN_ITEM1 ) )
			me->GetBodyInterface()->StartActivity( ACT_MP_RUN_ITEM1 );

		if ( !TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_DOOMSDAY ) )
		{
			if ( m_rumbleTimer.IsElapsed() )
			{
				m_rumbleTimer.Start( 0.25f );
				UTIL_ScreenShake( me->GetAbsOrigin(), 150.0f, 5.0f, 1.0f, 1000.0f, SHAKE_START );
			}
		}
	}
	else
	{
		if (!me->GetBodyInterface()->IsActivity( ACT_MP_STAND_ITEM1 ))
			me->GetBodyInterface()->StartActivity( ACT_MP_STAND_ITEM1 );
	}

	return Action<CHeadlessHatman>::Continue();
}


EventDesiredResult<CHeadlessHatman> CHeadlessHatmanAttack::OnStuck( CHeadlessHatman *me )
{
	const Path::Segment *goal = m_PathFollower.GetCurrentGoal();
	if (goal)
	{
		Vector vec = goal->pos + Vector( 0, 0, 10 );
		me->SetAbsOrigin( vec );
	}
	
	return Action<CHeadlessHatman>::TryContinue();
}

EventDesiredResult<CHeadlessHatman> CHeadlessHatmanAttack::OnContact( CHeadlessHatman *me, CBaseEntity *other, CGameTrace *result )
{
	if (other)
	{
		CBaseCombatCharacter *pBCC = dynamic_cast<CBaseCombatCharacter *>( other );
		if (pBCC && pBCC->IsAlive())
		{
			m_hTarget = pBCC;
			m_forcedTargetDuration.Start( 3.0f );
		}
	}
	return Action<CHeadlessHatman>::TryContinue();
}


void CHeadlessHatmanAttack::AttackTarget( CHeadlessHatman *actor, CBaseCombatCharacter *victim, float dist )
{
	if (actor->IsRangeLessThan( victim, dist ))
	{
		Vector vecFwd;
		actor->GetVectors( &vecFwd, nullptr, nullptr );

		Vector vecToActor = ( victim->WorldSpaceCenter() - actor->WorldSpaceCenter() );
		vecToActor.NormalizeInPlace();

		float flComp;
		float flDist = actor->GetRangeTo( victim );
		if (flDist >= ( dist * 0.5f ))
			flComp = ( ( flDist - ( dist * 0.5f ) ) / ( dist * 0.5f ) ) * 0.27f;
		else
			flComp = 0.0f;

		if (vecToActor.Dot( vecFwd ) > flComp)
		{
			if (actor->IsAbleToSee( victim, CBaseCombatCharacter::USE_FOV ))
			{	// this seems wrong, but it seems victim can only ever be m_hTarget anyway
				float flDamage = m_hTarget->GetMaxHealth() * 0.8f;
				CTakeDamageInfo info( actor, actor, flDamage, DMG_CLUB, TF_DMG_CUSTOM_DECAPITATION_BOSS );
				CalculateMeleeDamageForce( &info, vecFwd, actor->WorldSpaceCenter(), 5.0f );

				m_hTarget->TakeDamage( info );
				actor->EmitSound( "Halloween.HeadlessBossAxeHitFlesh" );
			}
		}
	}
}

void CHeadlessHatmanAttack::SelectVictim( CHeadlessHatman *actor )
{
	ValidateChaseVictim( actor );

	m_hAimTarget = ToTFPlayer( TFGameRules()->GetIT() );

	if (!TFGameRules()->GetIT())
	{
		CUtlVector<CTFPlayer *> victims;
		CollectPlayers( &victims, TF_TEAM_RED, true );
		CollectPlayers( &victims, TF_TEAM_BLUE, true, true );

		float flDist1 = FLT_MAX;
		float flDist2 = FLT_MAX;
		CTFPlayer *candidate = nullptr;

		for (int i=0; i<victims.Count(); ++i)
		{
			if (IsPotentiallyChaseable( actor, victims[i] ))
			{
				float flDistance = actor->GetRangeSquaredTo( victims[i] );
				if (flDist2 > flDistance)
				{
					if (actor->IsAbleToSee( victims[i], CBaseCombatCharacter::USE_FOV ))
					{
						flDist2 = flDistance;
						m_hAimTarget = victims[i];
					}

					if (( m_vecHome - victims[i]->GetAbsOrigin() ).LengthSqr() <= Square( tf_halloween_bot_chase_range.GetFloat() ))
					{
						if (flDist1 > flDistance)
						{
							flDist1 = flDistance;
							candidate = victims[i];
						}
					}
				}
			}
		}

		if (candidate)
		{
			m_chaseDuration.Start( tf_halloween_bot_chase_duration.GetFloat() );
			TFGameRules()->SetIT( candidate );
		}
	}

	if (!m_hTarget)
	{
		m_hTarget = ToTFPlayer( TFGameRules()->GetIT() );
		return;
	}

	if (m_forcedTargetDuration.IsElapsed())
	{
		m_hTarget = ToTFPlayer( TFGameRules()->GetIT() );
		return;
	}

	if(!m_hTarget->IsAlive())
		m_hTarget = ToTFPlayer( TFGameRules()->GetIT() );
}

void CHeadlessHatmanAttack::ValidateChaseVictim( CHeadlessHatman *actor )
{
	CTFPlayer *pIT = ToTFPlayer( TFGameRules()->GetIT() );
	if (pIT && (!m_hOldTarget || m_hOldTarget != pIT))
	{
		m_chaseDuration.Start( tf_halloween_bot_chase_duration.GetFloat() );
		m_hOldTarget = pIT;
	}

	if (!IsPotentiallyChaseable( actor, pIT ))
		TFGameRules()->SetIT( nullptr );
}

bool CHeadlessHatmanAttack::IsPotentiallyChaseable( CHeadlessHatman *actor, CTFPlayer *victim )
{
	if (!victim)
		return false;
	if (!victim->IsAlive())
		return false;

	CTFNavArea *area = static_cast<CTFNavArea *>( victim->GetLastKnownArea() );
	if (!area || area->HasTFAttributes( RED_SPAWN_ROOM|BLUE_SPAWN_ROOM ))
		return false;

	if (!actor->GetGroundEntity())
	{
		if (( victim->GetAbsOrigin() - m_vecHome ).LengthSqr() > Square( tf_halloween_bot_quit_range.GetFloat() ))
			return false;
	}
	else
	{
		Vector vecSpot;
		area->GetClosestPointOnArea( victim->GetAbsOrigin(), &vecSpot );
		if (( victim->GetAbsOrigin() - vecSpot ).Length2DSqr() <= Square( 50.0f ))
		{
			if (( victim->GetAbsOrigin() - m_vecHome ).LengthSqr() > Square( tf_halloween_bot_quit_range.GetFloat() ))
				return false;
		}
	}

	if (victim->m_Shared.IsInvulnerable())
		return false;

	return true;
}

bool CHeadlessHatmanAttack::IsSwingingAxe( void )
{
	return !m_attackTimer.IsElapsed();
}

void CHeadlessHatmanAttack::UpdateAxeSwing( CHeadlessHatman *actor )
{
	if (m_attackTimer.HasStarted() && m_attackTimer.IsElapsed())
	{
		m_attackTimer.Invalidate();

		AttackTarget( actor, m_hTarget, tf_halloween_bot_attack_range.GetFloat() );

		actor->EmitSound( "Halloween.HeadlessBossAxeHitWorld" );

		if( !TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_DOOMSDAY ) )
			UTIL_ScreenShake( actor->GetAbsOrigin(), 150.0f, 5.0f, 1.0f, 1000.0f, SHAKE_START );

		if (actor->GetWeapon())
		{
			Vector org; QAngle ang;
			if (actor->GetWeapon()->GetAttachment( "axe_blade", org, ang ))
			{
				extern void DispatchParticleEffect( const char *pszParticleName, Vector vecOrigin, QAngle vecAngles, CBaseEntity *pEntity );
				DispatchParticleEffect( "halloween_boss_axe_hit_world", org, ang, NULL );
			}
		}
	}
}

void CHeadlessHatmanAttack::RecomputeHomePosition( void )
{
	if (m_recomputeHomeTimer.IsElapsed())
	{
		m_recomputeHomeTimer.Reset();

		if (g_hControlPointMasters.IsEmpty())
			return;

		CTeamControlPointMaster *pMaster = g_hControlPointMasters[0];
		for (int i=0; i<pMaster->GetNumPoints(); ++i)
		{
			CTeamControlPoint *pPoint = pMaster->GetControlPoint( i );
			if (pMaster->IsInRound( pPoint ) &&
				 ObjectiveResource()->GetOwningTeam( pPoint->GetPointIndex() ) != TF_TEAM_BLUE &&
				 TFGameRules()->TeamMayCapturePoint( TF_TEAM_BLUE, pPoint->GetPointIndex() ))
			{
				m_vecHome = pPoint->GetAbsOrigin();
				return;
			}
		}
	}
}
