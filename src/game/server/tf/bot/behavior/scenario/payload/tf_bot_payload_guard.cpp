//========= Copyright � Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "tf_bot.h"
#include "tf_gamerules.h"
#include "team_train_watcher.h"
#include "tf_bot_payload_guard.h"
#include "tf_bot_payload_block.h"
#include "behavior/demoman/tf_bot_prepare_stickybomb_trap.h"

ConVar tf_bot_payload_guard_range( "tf_bot_payload_guard_range", "1000", FCVAR_CHEAT );
ConVar tf_bot_debug_payload_guard_vantage_points( "tf_bot_debug_payload_guard_vantage_points", "0", FCVAR_CHEAT );

class CCollectPayloadGuardVantagePoints : public ISearchSurroundingAreasFunctor
{
public:
	CCollectPayloadGuardVantagePoints( CTFBot *actor, CBaseEntity *target ) :
		m_pActor( actor ), m_hTarget( target ) {}

	virtual bool operator()( CNavArea *area, CNavArea *priorArea, float travelDistanceSoFar ) OVERRIDE;

	Vector const& GetResult( void ) const;

private:
	CTFBot *m_pActor;
	CHandle<CBaseEntity> m_hTarget;
	CUtlVector<Vector> m_VantagePoints;
};
bool CCollectPayloadGuardVantagePoints::operator()( CNavArea *area, CNavArea *priorArea, float travelDistanceSoFar )
{
	NextBotTraceFilterIgnoreActors filter( nullptr, COLLISION_GROUP_NONE );

	for ( int i = 3; i > 0; --i )
	{
		Vector point = area->GetRandomPoint() + Vector( 0.0f, 0.0f, HumanEyeHeight );

		trace_t tr;
		UTIL_TraceLine( point, m_hTarget->WorldSpaceCenter(), MASK_SOLID_BRUSHONLY, &filter, &tr );

		if ( ( tr.fraction >= 1.0f && !tr.allsolid && !tr.startsolid ) || tr.m_pEnt == m_hTarget )
		{
			m_VantagePoints.AddToTail( point );

			if ( tf_bot_debug_payload_guard_vantage_points.GetBool() )
			{
				NDebugOverlay::Cross3D( point, 5.0f, 0xFF, 0x00, 0xFF, true, 120.0f );
			}
		}
	}

	return true;
}
Vector const& CCollectPayloadGuardVantagePoints::GetResult() const
{
	if ( m_VantagePoints.IsEmpty() )
		return m_hTarget->WorldSpaceCenter();

	return m_VantagePoints.Random();
}


const char *CTFBotPayloadGuard::GetName() const
{
	return "PayloadGuard";
}


ActionResult<CTFBot> CTFBotPayloadGuard::OnStart( CTFBot *me, Action<CTFBot> *priorAction )
{
	m_PathFollower.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );
	m_PathFollower.Invalidate();

	m_vecVantagePoint = me->GetAbsOrigin();

	return BaseClass::Continue();
}

ActionResult<CTFBot> CTFBotPayloadGuard::Update( CTFBot *me, float dt )
{
	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	if ( threat && threat->IsVisibleRecently() )
	{
		me->EquipBestWeaponForThreat( threat );
	}

	CTeamTrainWatcher *pWatcher = TFGameRules()->GetPayloadToBlock( me->GetTeamNumber() );
	if ( pWatcher == nullptr )
		return BaseClass::Continue();

	CBaseEntity *pTrain = pWatcher->GetTrainEntity();
	if ( pTrain == nullptr )
		return BaseClass::Continue();

	if ( !pWatcher->IsDisabled() && pWatcher->GetCapturerCount() > 0 )
	{
		if( !m_blockPayloadDelay.HasStarted() )
			m_blockPayloadDelay.Start( RandomFloat( 0.5, 3.0 ) );
	}

	if ( m_blockPayloadDelay.IsElapsed() && pWatcher->GetCapturerCount() > 0 )
	{
		return BaseClass::SuspendFor( new CTFBotPayloadBlock, "Moving to block the cart's forward motion" );
	}
	else if ( pWatcher->GetCapturerCount() <= 0 )
	{
		m_blockPayloadDelay.Invalidate();
	}

	if ( m_vecVantagePoint.DistToSqr( me->GetAbsOrigin() ) > Square( 25.0f ) )
		m_recomputeVantagePointTimer.Start( RandomFloat( 3.0f, 15.0f ) );

	if ( !me->IsLineOfFireClear( pTrain ) )
		m_recomputeVantagePointTimer.Invalidate();

	if ( m_recomputeVantagePointTimer.IsElapsed() )
	{
		m_vecVantagePoint = FindVantagePoint( me, pTrain );
		m_recomputePathTimer.Invalidate();
	}
	
	if ( m_vecVantagePoint.DistToSqr( me->GetAbsOrigin() ) > Square( 25.0f ) )
	{
		if ( m_recomputePathTimer.IsElapsed() )
		{
			CTFBotPathCost func( me );
			m_PathFollower.Compute( me, m_vecVantagePoint, func );

			m_recomputePathTimer.Start( RandomFloat( 0.5f, 1.0f ) );
		}

		m_PathFollower.Update( me );

		return BaseClass::Continue();
	}

	if ( CTFBotPrepareStickybombTrap::IsPossible( me ) )
		return BaseClass::SuspendFor( new CTFBotPrepareStickybombTrap, "Laying sticky bombs!" );

	return BaseClass::Continue();
}

ActionResult<CTFBot> CTFBotPayloadGuard::OnResume( CTFBot *me, Action<CTFBot> *action )
{
	VPROF_BUDGET( "CTFBotPayloadGuard::OnResume", "NextBot" );

	m_blockPayloadDelay.Invalidate();
	m_recomputeVantagePointTimer.Invalidate();

	return BaseClass::Continue();
}


EventDesiredResult<CTFBot> CTFBotPayloadGuard::OnMoveToSuccess( CTFBot *me, const Path *path )
{
	return BaseClass::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotPayloadGuard::OnMoveToFailure( CTFBot *me, const Path *path, MoveToFailureType fail )
{
	VPROF_BUDGET( "CTFBotPayloadGuard::OnMoveToFailure", "NextBot" );

	m_blockPayloadDelay.Invalidate();
	m_recomputeVantagePointTimer.Invalidate();

	return BaseClass::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotPayloadGuard::OnStuck( CTFBot *me )
{
	VPROF_BUDGET( "CTFBotPayloadGuard::OnStuck", "NextBot" );

	m_recomputeVantagePointTimer.Invalidate();
	me->GetLocomotionInterface()->ClearStuckStatus();

	return BaseClass::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotPayloadGuard::OnTerritoryContested( CTFBot *me, int i1 )
{
	return BaseClass::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotPayloadGuard::OnTerritoryCaptured( CTFBot *me, int i1 )
{
	return BaseClass::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotPayloadGuard::OnTerritoryLost( CTFBot *me, int i1 )
{
	return BaseClass::TryContinue();
}


QueryResultType CTFBotPayloadGuard::ShouldHurry( const INextBot *nextbot ) const
{
	return ANSWER_UNDEFINED;
}

QueryResultType CTFBotPayloadGuard::ShouldRetreat( const INextBot *nextbot ) const
{
	CTFBot *actor = ToTFBot( nextbot->GetEntity() );

	CHandle<CTeamTrainWatcher> watcher = TFGameRules()->GetPayloadToBlock( actor->GetTeamNumber() );
	if ( watcher != nullptr && watcher->IsTrainNearCheckpoint() )
	{
		return ANSWER_NO;
	}

	return ANSWER_UNDEFINED;
}


Vector CTFBotPayloadGuard::FindVantagePoint( CTFBot *actor, CBaseEntity *target )
{
	CCollectPayloadGuardVantagePoints functor( actor, target );
	SearchSurroundingAreas( TheNavMesh->GetNearestNavArea( target ), functor, tf_bot_payload_guard_range.GetFloat() );

	return functor.GetResult();
}

