//========= Copyright � Valve LLC, All rights reserved. =======================
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

#include "merasmus_attack.h"
#include "merasmus_stunned.h"
#include "merasmus_zap.h"
#include "merasmus_throwinggrenade.h"
#include "merasmus_staffattack.h"


ConVar tf_merasmus_chase_duration( "tf_merasmus_chase_duration", "7", FCVAR_CHEAT );
ConVar tf_merasmus_chase_range( "tf_merasmus_chase_range", "2000", FCVAR_CHEAT );
extern ConVar tf_merasmus_bomb_head_duration;
extern ConVar tf_merasmus_attack_range;


char const *CMerasmusAttack::GetName( void ) const
{
	return "Attack";
}


ActionResult<CMerasmus> CMerasmusAttack::OnStart( CMerasmus *me, Action<CMerasmus> *priorAction )
{
	m_PathFollower.SetMinLookAheadDistance( 100.0f );
	m_staffAttackTimer.Invalidate();
	m_hTarget = NULL;

	me->GetBodyInterface()->StartActivity( ACT_MP_RUN_ITEM1 );

	m_chooseVictimTimer.Start( 0.0f );
	m_grenadeTimer.Start( RandomFloat( 2.0, 3.0 ) );
	m_zapTimer.Start( RandomFloat( 3.0, 4.0 ) );
	m_recomputeHomeTimer.Start( 3.0f );

	m_vecHome = me->GetAbsOrigin();

	m_bombHeadTimer.Start( 10.0f );

	return Continue();
}

ActionResult<CMerasmus> CMerasmusAttack::Update( CMerasmus *me, float dt )
{
	if ( !me->IsAlive() )
		return Done();

	if ( me->IsStunned() )
		return SuspendFor( new CMerasmusStunned, "Stunned!" );

	SelectVictim( me );
	RecomputeHomePosition();

	if ( m_hTarget && m_hTarget->IsAlive() )
	{
		if ( me->IsRangeGreaterThan( m_hTarget, 100.0f ) || !me->IsLineOfSightClear( m_hTarget ) )
		{
			if ( m_PathFollower.GetAge() > 1.0 )
			{
				CMerasmusPathCost func( me );
				m_PathFollower.Compute( me, m_hTarget, func );
			}

			m_PathFollower.Update( me );
		}
	}
	else
	{
		if ( me->IsRangeGreaterThan( m_vecHome, 100.0f ) )
		{
			if ( m_PathFollower.GetAge() > 3.0 )
			{
				CMerasmusPathCost func( me );
				m_PathFollower.Compute( me, m_vecHome, func );
			}

			m_PathFollower.Update( me );
		}
	}

	if ( m_bombHeadTimer.IsElapsed() )
	{
		BombHeadForTeam( TF_TEAM_RED, tf_merasmus_bomb_head_duration.GetFloat() );
		BombHeadForTeam( TF_TEAM_BLUE, tf_merasmus_bomb_head_duration.GetFloat() );

		m_bombHeadTimer.Start( 16.0f );
	}

	if ( m_hTarget && m_hTarget->IsAlive() )
	{
		if ( m_zapTimer.IsElapsed() )
		{
			m_zapTimer.Start( RandomFloat( 3.0, 4.0 ) );

			return SuspendFor( new CMerasmusZap(), "Zap!" );
		}

		if ( m_grenadeTimer.IsElapsed() )
		{
			m_grenadeTimer.Start( RandomFloat( 2.0, 3.0 ) );

			return SuspendFor( new CMerasmusThrowingGrenade( m_hTarget ), "Fire in the hole!" );
		}

		if( me->IsRangeLessThan( m_hTarget, tf_merasmus_attack_range.GetFloat()) )
		{
			if ( m_staffAttackTimer.IsElapsed() )
			{
				m_staffAttackTimer.Start( 1.0f );

				return SuspendFor( new CMerasmusStaffAttack( m_hTarget ), "Whack!" );
			}

			me->GetLocomotionInterface()->FaceTowards( m_hTarget->WorldSpaceCenter() );
		}
	}

	if ( !me->GetBodyInterface()->IsActivity( ACT_MP_RUN_MELEE ) )
		me->GetBodyInterface()->StartActivity( ACT_MP_RUN_MELEE );

	return Continue();
}


EventDesiredResult<CMerasmus> CMerasmusAttack::OnStuck( CMerasmus *me )
{
	const Path::Segment *goal = m_PathFollower.GetCurrentGoal();
	if (goal)
	{
		Vector vec = goal->pos + Vector( 0, 0, 10 );
		me->SetAbsOrigin( vec );
	}

	return TryContinue();
}

EventDesiredResult<CMerasmus> CMerasmusAttack::OnContact( CMerasmus *me, CBaseEntity *other, CGameTrace *result )
{
	if ( other )
	{
		if ( other->IsPlayer() && other->IsAlive() )
		{
			m_hTarget = ToTFPlayer( other );
			m_chooseVictimTimer.Start( tf_merasmus_chase_duration.GetFloat() );
		}
	}

	return TryContinue();
}


void CMerasmusAttack::RecomputeHomePosition( void )
{
	if ( m_recomputeHomeTimer.IsElapsed() )
	{
		m_recomputeHomeTimer.Reset();

		if ( g_hControlPointMasters.IsEmpty() )
			return;

		CTeamControlPointMaster *pMaster = g_hControlPointMasters[0];
		for (int i=0; i<pMaster->GetNumPoints(); ++i)
		{
			CTeamControlPoint *pPoint = pMaster->GetControlPoint( i );
			if ( !pMaster->IsInRound( pPoint ) )
				continue;

			if ( ObjectiveResource()->GetOwningTeam( pPoint->GetPointIndex() ) == TF_TEAM_BLUE )
				continue;

			if ( !TFGameRules()->TeamMayCapturePoint( TF_TEAM_BLUE, pPoint->GetPointIndex() ) )
				continue;

			m_vecHome = pPoint->GetAbsOrigin();
			return;
		}
	}
}

bool CMerasmusAttack::IsPotentiallyChaseable( CMerasmus *me, CTFPlayer *pVictim )
{
	if ( pVictim == nullptr || !pVictim->IsAlive() )
		return false;

	CTFNavArea *pArea = static_cast<CTFNavArea *>( pVictim->GetLastKnownArea() );
	if ( !pArea || pArea->HasTFAttributes( RED_SPAWN_ROOM|BLUE_SPAWN_ROOM ) )
		return false;

	if ( pVictim->m_Shared.IsInvulnerable() )
		return false;

	if ( pVictim->GetGroundEntity() )
	{
		Vector vecSpot;
		pArea->GetClosestPointOnArea( pVictim->GetAbsOrigin(), &vecSpot );
		if ( ( pVictim->GetAbsOrigin() - vecSpot ).Length2DSqr() > Square( 50.0f ) )
			return false;
	}
			
	if ( ( pVictim->GetAbsOrigin() - m_vecHome ).LengthSqr() > Square( tf_merasmus_chase_range.GetFloat() ) )
		return false;

	return true;
}

void CMerasmusAttack::SelectVictim( CMerasmus *me )
{
	if ( !IsPotentiallyChaseable( me, m_hTarget ) || m_chooseVictimTimer.IsElapsed() )
	{
		CUtlVector<CTFPlayer *> victims;
		CollectPlayers( &victims, TF_TEAM_RED, true );
		CollectPlayers( &victims, TF_TEAM_BLUE, true, true );

		float flMinDist = FLT_MAX;
		CTFPlayer *pClosest = nullptr;

		FOR_EACH_VEC( victims, i )
		{
			if ( !IsPotentiallyChaseable( me, victims[i] ) )
				continue;

			float flDistance = me->GetRangeSquaredTo( victims[i] );
			if ( flDistance < flMinDist )
			{
				flMinDist = flDistance;
				pClosest = victims[i];
			}
		}

		if ( pClosest )
		{
			m_chooseVictimTimer.Start( tf_merasmus_chase_duration.GetFloat() );
			m_hTarget = pClosest;
		}
	}
}
