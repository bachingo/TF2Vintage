#include "cbase.h"
#include "../tf_bot.h"
#include "nav_mesh/tf_nav_area.h"
#include "tf_bot_move_to_vantage_point.h"


CTFBotMoveToVantagePoint::CTFBotMoveToVantagePoint( float max_cost )
{
	this->m_flMaxCost = max_cost;
}

CTFBotMoveToVantagePoint::~CTFBotMoveToVantagePoint()
{
}


const char *CTFBotMoveToVantagePoint::GetName() const
{
	return "MoveToVantagePoint";
}


ActionResult<CTFBot> CTFBotMoveToVantagePoint::OnStart( CTFBot *actor, Action<CTFBot> *action )
{
	m_PathFollower.SetMinLookAheadDistance( actor->GetDesiredPathLookAheadRange() );

	m_VantagePoint = actor->FindVantagePoint( m_flMaxCost );
	if ( m_VantagePoint == nullptr )
		return Action<CTFBot>::Done( "No vantage point found" );

	m_PathFollower.Invalidate();
	m_recomputePathTimer.Invalidate();

	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotMoveToVantagePoint::Update( CTFBot *actor, float dt )
{
	const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat( false );
	if ( threat != nullptr && threat->IsVisibleInFOVNow() )
		return Action<CTFBot>::Done( "Enemy is visible" );

	if ( !m_PathFollower.IsValid() || m_recomputePathTimer.IsElapsed() )
	{
		m_recomputePathTimer.Start( 1.0f );

		CTFBotPathCost cost( actor, FASTEST_ROUTE );
		if ( !m_PathFollower.Compute( actor, m_VantagePoint->GetCenter(), cost ) )
			return Action<CTFBot>::Done( "No path to vantage point exists" );
	}

	m_PathFollower.Update( actor );

	return Action<CTFBot>::Continue();
}


EventDesiredResult<CTFBot> CTFBotMoveToVantagePoint::OnMoveToSuccess( CTFBot *actor, const Path *path )
{
	return Action<CTFBot>::TryDone( RESULT_CRITICAL, "Vantage point reached" );
}

EventDesiredResult<CTFBot> CTFBotMoveToVantagePoint::OnMoveToFailure( CTFBot *actor, const Path *path, MoveToFailureType fail )
{
	m_PathFollower.Invalidate();

	return Action<CTFBot>::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotMoveToVantagePoint::OnStuck( CTFBot *actor )
{
	m_PathFollower.Invalidate();

	return Action<CTFBot>::TryContinue();
}
