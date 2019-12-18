//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "tf_bot.h"
#include "tf_gamerules.h"
#include "team_train_watcher.h"
#include "tf_bot_payload_block.h"



const char *CTFBotPayloadBlock::GetName() const
{
	return "PayloadBlock";
}


ActionResult<CTFBot> CTFBotPayloadBlock::OnStart( CTFBot *me, Action<CTFBot> *priorAction )
{
	m_PathFollower.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );
	m_PathFollower.Invalidate();

	m_blockDuration.Start( RandomFloat( 3.0f, 5.0f ) );

	return BaseClass::Continue();
}

ActionResult<CTFBot> CTFBotPayloadBlock::Update( CTFBot *me, float dt )
{
	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	if ( threat != nullptr && threat->IsVisibleRecently() )
	{
		me->EquipBestWeaponForThreat( threat );
	}

	if ( m_blockDuration.IsElapsed() )
		return BaseClass::Done( "Been blocking long enough" );

	if ( m_recomputePathTimer.IsElapsed() )
	{
		VPROF_BUDGET( "CTFBotPayloadBlock::Update( repath )", "NextBot" );

		CTeamTrainWatcher *pWatcher = TFGameRules()->GetPayloadToBlock( me->GetTeamNumber() );
		if ( pWatcher == nullptr )
			return BaseClass::Done( "Train Watcher is missing" );

		CBaseEntity *pTrain = pWatcher->GetTrainEntity();
		if ( pTrain == nullptr )
			return BaseClass::Done( "Cart is missing" );

		CTFBotPathCost func( me, FASTEST_ROUTE );
		m_PathFollower.Compute( me, pTrain->WorldSpaceCenter(), func, 0.0f, true );

		m_recomputePathTimer.Start( RandomFloat( 0.2f, 0.4f ) );
	}

	m_PathFollower.Update( me );

	return BaseClass::Continue();
}

ActionResult<CTFBot> CTFBotPayloadBlock::OnResume( CTFBot *me, Action<CTFBot> *priorAction )
{
	VPROF_BUDGET( "CTFBotPayloadBlock::OnResume", "NextBot" );

	m_recomputePathTimer.Invalidate();

	return BaseClass::Continue();
}


EventDesiredResult<CTFBot> CTFBotPayloadBlock::OnMoveToSuccess( CTFBot *me, const Path *path )
{
	return BaseClass::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotPayloadBlock::OnMoveToFailure( CTFBot *me, const Path *path, MoveToFailureType fail )
{
	VPROF_BUDGET( "CTFBotPayloadBlock::OnMoveToFailure", "NextBot" );

	m_recomputePathTimer.Invalidate();

	return BaseClass::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotPayloadBlock::OnStuck( CTFBot *me )
{
	VPROF_BUDGET( "CTFBotPayloadBlock::OnStuck", "NextBot" );

	m_recomputePathTimer.Invalidate();
	me->GetLocomotionInterface()->ClearStuckStatus();

	return BaseClass::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotPayloadBlock::OnTerritoryContested( CTFBot *me, int iPointIdx )
{
	return BaseClass::TryToSustain( RESULT_IMPORTANT );
}

EventDesiredResult<CTFBot> CTFBotPayloadBlock::OnTerritoryCaptured( CTFBot *me, int iPointIdx )
{
	return BaseClass::TryToSustain( RESULT_IMPORTANT );
}

EventDesiredResult<CTFBot> CTFBotPayloadBlock::OnTerritoryLost( CTFBot *me, int iPointIdx )
{
	return BaseClass::TryToSustain( RESULT_IMPORTANT );
}


QueryResultType CTFBotPayloadBlock::ShouldHurry( const INextBot *me ) const
{
	return ANSWER_YES;
}

QueryResultType CTFBotPayloadBlock::ShouldRetreat( const INextBot *me ) const
{
	return ANSWER_UNDEFINED;
}
