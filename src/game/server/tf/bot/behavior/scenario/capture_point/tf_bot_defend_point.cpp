#include "cbase.h"
#include "../../../tf_bot.h"
#include "team_control_point_master.h"
#include "tf_gamerules.h"
#include "nav_mesh/tf_nav_mesh.h"
#include "tf_bot_defend_point.h"
#include "tf_bot_defend_point_block_capture.h"
#include "tf_bot_capture_point.h"
#include "../../tf_bot_seek_and_destroy.h"
#include "../../demoman/tf_bot_prepare_stickybomb_trap.h"


ConVar tf_bot_defense_must_defend_time( "tf_bot_defense_must_defend_time", "300", FCVAR_CHEAT, "If timer is less than this, bots will stay near point and guard" );
ConVar tf_bot_max_point_defend_range( "tf_bot_max_point_defend_range", "1250", FCVAR_CHEAT, "How far (in travel distance) from the point defending bots will take up positions" );
ConVar tf_bot_defense_debug( "tf_bot_defense_debug", "0", FCVAR_CHEAT );


CTFBotDefendPoint::CTFBotDefendPoint()
{
}

CTFBotDefendPoint::~CTFBotDefendPoint()
{
}


const char *CTFBotDefendPoint::GetName() const
{
	return "DefendPoint";
}


ActionResult<CTFBot> CTFBotDefendPoint::OnStart( CTFBot *me, Action<CTFBot> *priorAction )
{
	static const float roamChance[] = {
		10.0f, // easy
		50.0f, // normal
		75.0f, // hard
		90.0f, // expert
	};

	m_PathFollower.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );
	m_DefenseArea = nullptr;
	m_bShouldRoam = ( RandomFloat( 0.0f, 100.0f ) < roamChance[Clamp( (int)me->m_iSkill, 0, 3 )] );

	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotDefendPoint::Update( CTFBot *me, float dt )
{
	if ( !g_hControlPointMasters.IsEmpty() )
	{
		CTeamControlPointMaster *master = g_hControlPointMasters[0];
		if ( master != nullptr && master->GetNumPoints() == 1 )
		{
			CTeamControlPoint *point = master->GetControlPoint( 0 );
			if ( point != nullptr && point->GetPointIndex() == 0 &&
				 point->GetOwner() != me->GetTeamNumber() )
			{
				return Action<CTFBot>::ChangeTo( new CTFBotCapturePoint, "We need to capture the point!" );
			}
		}
	}

	CTeamControlPoint *point = me->GetMyControlPoint();
	if ( point == nullptr )
		return Action<CTFBot>::SuspendFor( new CTFBotSeekAndDestroy( 10.0f ), "Seek and destroy until a point becomes available" );

	if ( point->GetTeamNumber() != me->GetTeamNumber() )
		return Action<CTFBot>::ChangeTo( new CTFBotCapturePoint, "We need to capture our point(s)" );

	if ( this->IsPointThreatened( me ) && this->WillBlockCapture( me ) )
		return Action<CTFBot>::SuspendFor( new CTFBotDefendPointBlockCapture, "Moving to block point capture!" );

	if ( me->m_Shared.IsInvulnerable() )
		return Action<CTFBot>::SuspendFor( new CTFBotSeekAndDestroy( 6.0f ), "Attacking because I'm uber'd!" );

	if ( point->IsLocked() )
		return Action<CTFBot>::SuspendFor( new CTFBotSeekAndDestroy(), "Seek and destroy until the point unlocks" );

	if ( m_bShouldRoam && me->GetTimeLeftToCapture() > tf_bot_defense_must_defend_time.GetFloat() )
		return Action<CTFBot>::SuspendFor( new CTFBotSeekAndDestroy( 15.0f ), "Seek and destroy - we have lots of time" );

	if ( TFGameRules()->InSetup() )
	{
		m_reselectDefenseAreaTimer.Reset();
	}

	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	me->EquipBestWeaponForThreat( threat );

	if ( threat && threat->IsVisibleRecently() )
	{
		m_reselectDefenseAreaTimer.Reset();

		if ( me->IsPlayerClass( TF_CLASS_PYRO ) )
			return Action<CTFBot>::SuspendFor( new CTFBotSeekAndDestroy( 15.0f ), "Going after an enemy" );

		CTFWeaponBase *pWeapon = me->GetActiveTFWeapon();
		if ( pWeapon && ( pWeapon->IsMeleeWeapon() || pWeapon->IsWeapon( TF_WEAPON_FLAMETHROWER ) ) )
		{
			RouteType iRouteType = me->IsPlayerClass( TF_CLASS_PYRO ) ? SAFEST_ROUTE : FASTEST_ROUTE;
			CTFBotPathCost cost( me, iRouteType );
			m_ChasePath.Update( me, threat->GetEntity(), cost );

			return Action<CTFBot>::Continue();
		}
	}

	if ( !m_DefenseArea || !m_reselectDefenseAreaTimer.HasStarted() || m_reselectDefenseAreaTimer.IsElapsed() )
		m_DefenseArea = SelectAreaToDefendFrom( me );

	if ( m_DefenseArea )
	{
		if ( me->GetLastKnownArea() == m_DefenseArea )
		{
			if ( CTFBotPrepareStickybombTrap::IsPossible( me ) )
				return Action<CTFBot>::SuspendFor( new CTFBotPrepareStickybombTrap, "Laying sticky bombs!" );
		}
		else
		{
			if ( m_pathRecomputeTimer.IsElapsed() )
			{
				m_pathRecomputeTimer.Start( RandomFloat( 2.0f, 3.0f ) );

				CTFBotPathCost cost( me );
				m_PathFollower.Compute( me, m_DefenseArea->GetCenter(), cost );

			}
			else
			{
				m_PathFollower.Update( me );
				m_reselectDefenseAreaTimer.Reset();
			}
		}
	}
	
	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotDefendPoint::OnResume( CTFBot *me, Action<CTFBot> *priorAction )
{
	me->ClearMyControlPoint();

	m_pathRecomputeTimer.Invalidate();
	m_PathFollower.Invalidate();

	return Action<CTFBot>::Continue();
}


EventDesiredResult<CTFBot> CTFBotDefendPoint::OnContact( CTFBot *me, CBaseEntity *other, CGameTrace *trace )
{
	return Action<CTFBot>::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotDefendPoint::OnMoveToSuccess( CTFBot *me, const Path *path )
{
	return Action<CTFBot>::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotDefendPoint::OnMoveToFailure( CTFBot *me, const Path *path, MoveToFailureType fail )
{
	m_PathFollower.Invalidate();
	m_DefenseArea = SelectAreaToDefendFrom( me );

	return Action<CTFBot>::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotDefendPoint::OnStuck( CTFBot *me )
{
	m_PathFollower.Invalidate();
	m_DefenseArea = SelectAreaToDefendFrom( me );
	me->GetLocomotionInterface()->ClearStuckStatus();

	return Action<CTFBot>::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotDefendPoint::OnTerritoryContested( CTFBot *me, int territoryID )
{
	return Action<CTFBot>::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotDefendPoint::OnTerritoryCaptured( CTFBot *me, int territoryID )
{
	return Action<CTFBot>::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotDefendPoint::OnTerritoryLost( CTFBot *me, int territoryID )
{
	me->ClearMyControlPoint();

	m_DefenseArea = SelectAreaToDefendFrom( me );

	m_pathRecomputeTimer.Invalidate();
	m_PathFollower.Invalidate();

	return Action<CTFBot>::TryContinue();
}


bool CTFBotDefendPoint::IsPointThreatened( CTFBot *actor )
{
	CTeamControlPoint *point = actor->GetMyControlPoint();
	if ( point == nullptr )
		return false;

	if ( !point->HasBeenContested() || gpGlobals->curtime - point->LastContestedAt() >= 5.0f )
	{
		if ( !actor->HasPointRecentlyChanged() )
			return false;
	}

	return true;
}

class SelectDefenseAreaForPoint : public ISearchSurroundingAreasFunctor
{
public:
	SelectDefenseAreaForPoint( CTFNavArea *area, CUtlVector<CTFNavArea *> *areas, int iTeam )
		: m_startArea( area ), m_iTeam( iTeam ), m_areas( areas )
	{
		areas->RemoveAll();
		m_flIncursionDist = area->GetIncursionDistance( iTeam ) <= 0.0f ? 249.0f : area->GetIncursionDistance( iTeam );
	}

	virtual bool operator()( CNavArea *a, CNavArea *priorArea, float travelDistanceSoFar ) OVERRIDE
	{
		CTFNavArea *area = static_cast<CTFNavArea *>( a );
		if ( TFGameRules()->IsInKothMode() || area->GetIncursionDistance( m_iTeam ) <= m_flIncursionDist )
		{
			if ( m_flIncursionDist < -1.0f )
				return true;

			if ( !area->IsPotentiallyVisible( m_startArea ) || ( m_startArea->GetCenter().z - area->GetCenter().z ) >= 220.0f )
				return true;

			m_areas->AddToTail( area  );
		}

		return true;
	}

	virtual bool ShouldSearch( CNavArea *adjArea, CNavArea *currentArea, float travelDistanceSoFar ) OVERRIDE
	{
		int team = ( TFGameRules()->IsInKothMode() ? TEAM_ANY : m_iTeam );

		if ( adjArea->IsBlocked( team ) || travelDistanceSoFar > tf_bot_max_point_defend_range.GetFloat() )
			return false;

		if ( fabs( currentArea->ComputeAdjacentConnectionHeightChange( adjArea ) ) < 65.0f )
			return true;

		return false;
	}

private:
	CTFNavArea *m_startArea;
	CUtlVector<CTFNavArea *> *m_areas;
	float m_flIncursionDist;
	int m_iTeam;
};
CTFNavArea *CTFBotDefendPoint::SelectAreaToDefendFrom( CTFBot *actor )
{
	VPROF_BUDGET( "CTFBotDefendPoint::SelectAreaToDefendFrom", "NextBot" );

	CTeamControlPoint *pPoint = actor->GetMyControlPoint();
	if ( pPoint == nullptr || pPoint->GetPointIndex() > 7 )
		return nullptr;

	CUtlVector<CTFNavArea *> candidates;
	CTFNavArea *pArea = nullptr;

	CTFNavArea *pCPArea = TFNavMesh()->GetMainControlPointArea( pPoint->GetPointIndex() );
	if ( pCPArea )
	{
		SelectDefenseAreaForPoint functor( pCPArea, &candidates, actor->GetTeamNumber() );
		SearchSurroundingAreas( pCPArea, functor );

		if ( !candidates.IsEmpty() )
		{
			m_reselectDefenseAreaTimer.Start( RandomFloat( 10.0f, 20.0f ) );

			if ( tf_bot_defense_debug.GetBool() )
			{
				for ( int i=0; i<candidates.Count(); ++i )
					candidates[i]->DrawFilled( 0, 200, 200, 255 );
			}

			pArea = candidates.Random();
		}
	}
	
	return pArea;
}

bool CTFBotDefendPoint::WillBlockCapture( CTFBot *actor ) const
{
	if ( TFGameRules()->IsInTraining() )
		return false;

	switch ( actor->m_iSkill )
	{
		case CTFBot::DifficultyType::EASY:
			return false;
		case CTFBot::DifficultyType::NORMAL:
			return ( actor->TransientlyConsistentRandomValue( 10.0f, 0 ) > 0.5f );
		default:
			return true;
	}
}
