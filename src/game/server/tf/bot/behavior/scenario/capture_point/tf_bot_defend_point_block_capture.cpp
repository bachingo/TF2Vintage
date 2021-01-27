#include "cbase.h"
#include "../../../tf_bot.h"
#include "nav_mesh/tf_nav_mesh.h"
#include "tf_bot_defend_point_block_capture.h"
#include "../../medic/tf_bot_medic_heal.h"
#include "../../demoman/tf_bot_prepare_stickybomb_trap.h"


ConVar tf_bot_defend_owned_point_percent( "tf_bot_defend_owned_point_percent", "0.5", FCVAR_CHEAT, "Stay on the contested point we own until enemy cap percent falls below this" );


CTFBotDefendPointBlockCapture::CTFBotDefendPointBlockCapture()
{
}

CTFBotDefendPointBlockCapture::~CTFBotDefendPointBlockCapture()
{
}


const char *CTFBotDefendPointBlockCapture::GetName() const
{
	return "DefendPointBlockCapture";
}


ActionResult<CTFBot> CTFBotDefendPointBlockCapture::OnStart( CTFBot *me, Action<CTFBot> *priorAction )
{
	m_PathFollower.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	CTeamControlPoint *pPoint = me->GetMyControlPoint();
	if ( pPoint )
	{
		m_pPoint = pPoint;

		CNavArea *area = TheNavMesh->GetNearestNavArea( pPoint->GetAbsOrigin() );
		if ( area )
		{
			m_pCPArea = (CTFNavArea *)area;
			return Action<CTFBot>::Continue();
		}

		return Action<CTFBot>::Done( "Can't find nav area on point" );
	}

	return Action<CTFBot>::Done( "Point is NULL" );
}

ActionResult<CTFBot> CTFBotDefendPointBlockCapture::Update( CTFBot *me, float dt )
{
	if ( IsPointSafe( me ) )
		return Action<CTFBot>::Done( "Point is safe again" );

	if ( me->IsPlayerClass( TF_CLASS_MEDIC ) )
		return Action<CTFBot>::SuspendFor( new CTFBotMedicHeal );

	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	me->EquipBestWeaponForThreat( threat );

	bool bNearPoint = true;
	if ( !m_pPoint->CollisionProp()->IsPointInBounds( me->GetAbsOrigin() ) )
		bNearPoint = false;

	const CUtlVector<CTFNavArea *> &cpAreas = TFNavMesh()->GetControlPointAreas( m_pPoint->GetPointIndex() );
	for ( int i=0; i<cpAreas.Count(); ++i )
		bNearPoint &= me->GetLastKnownArea() != cpAreas[i];

	if ( bNearPoint && CTFBotPrepareStickybombTrap::IsPossible( me ) )
		return Action<CTFBot>::SuspendFor( new CTFBotPrepareStickybombTrap, "Placing stickies for defense" );

	if ( !m_recomputePathTimer.IsElapsed() )
	{
		m_PathFollower.Update( me );
		return Action<CTFBot>::Continue();
	}

	m_recomputePathTimer.Start( RandomFloat( 0.5f, 1.0f ) );

	float flTotalArea = 0.0f;
	for ( int i=0; i<cpAreas.Count(); ++i )
	{
		CNavArea *area = cpAreas[i];
		flTotalArea += area->GetSizeX() * area->GetSizeY();
	}

	float flRand = RandomFloat( 0, flTotalArea - 1.0f );
	for ( int i=0; i<cpAreas.Count(); ++i )
	{
		CNavArea *area = cpAreas[i];
		if ( flRand - ( area->GetSizeX() * area->GetSizeY() ) <= 0.0f )
		{
			CTFBotPathCost cost( me );
			m_PathFollower.Compute( me, area->GetRandomPoint(), cost );

			return Action<CTFBot>::Continue();
		}
	}

	if ( bNearPoint )
		return Action<CTFBot>::Continue();

	if ( m_recomputePathTimer.IsElapsed() )
	{
		CTFBotPathCost cost( me );
		m_PathFollower.Compute( me, m_pPoint->WorldSpaceCenter(), cost );


		m_recomputePathTimer.Start( RandomFloat( 0.5f, 1.0f ) );
	}

	m_PathFollower.Update( me );

	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotDefendPointBlockCapture::OnResume( CTFBot *me, Action<CTFBot> *priorAction )
{
	m_PathFollower.Invalidate();

	return Action<CTFBot>::Continue();
}


EventDesiredResult<CTFBot> CTFBotDefendPointBlockCapture::OnMoveToSuccess( CTFBot *me, const Path *path )
{
	return Action<CTFBot>::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotDefendPointBlockCapture::OnMoveToFailure( CTFBot *me, const Path *path, MoveToFailureType fail )
{
	m_PathFollower.Invalidate();

	return Action<CTFBot>::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotDefendPointBlockCapture::OnStuck( CTFBot *me )
{
	m_PathFollower.Invalidate();

	me->GetLocomotionInterface()->ClearStuckStatus();

	return Action<CTFBot>::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotDefendPointBlockCapture::OnTerritoryContested( CTFBot *me, int territoryID )
{
	return Action<CTFBot>::TryToSustain();
}

EventDesiredResult<CTFBot> CTFBotDefendPointBlockCapture::OnTerritoryCaptured( CTFBot *me, int territoryID )
{
	return Action<CTFBot>::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotDefendPointBlockCapture::OnTerritoryLost( CTFBot *me, int territoryID )
{
	return Action<CTFBot>::TryDone( RESULT_CRITICAL, "Lost the point" );
}


QueryResultType CTFBotDefendPointBlockCapture::ShouldHurry( const INextBot *me ) const
{
	return ANSWER_YES;
}

QueryResultType CTFBotDefendPointBlockCapture::ShouldRetreat( const INextBot *me ) const
{
	return ANSWER_NO;
}


bool CTFBotDefendPointBlockCapture::IsPointSafe( CTFBot *actor )
{
	if ( !actor->HasPointRecentlyChanged() )
	{
		if ( m_pPoint && m_pPoint->GetTeamCapPercentage( actor->GetTeamNumber() ) >= tf_bot_defend_owned_point_percent.GetFloat() &&
			( !m_pPoint->HasBeenContested() || gpGlobals->curtime - m_pPoint->LastContestedAt() >= 5.0f ) )
		{
			const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat();
			if ( threat && ( threat->GetLastKnownPosition() - actor->GetAbsOrigin() ).LengthSqr() >= Square( 500.0f ) )
				return true;
		}
	}

	return false;
}
