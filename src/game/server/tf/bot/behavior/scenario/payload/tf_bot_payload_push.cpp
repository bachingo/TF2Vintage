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
#include "tf_bot_payload_push.h"


ConVar tf_bot_cart_push_radius( "tf_bot_cart_push_radius", "60", FCVAR_CHEAT );


const char *CTFBotPayloadPush::GetName() const
{
	return "PayloadPush";
}


ActionResult<CTFBot> CTFBotPayloadPush::OnStart( CTFBot *me, Action<CTFBot> *priorAction )
{
	m_PathFollower.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );
	m_PathFollower.Invalidate();

	// float @ 0x4814 = 180.0f

	return BaseClass::Continue();
}

ActionResult<CTFBot> CTFBotPayloadPush::Update( CTFBot *me, float dt )
{
	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	if ( threat && threat->IsVisibleRecently() )
	{
		me->EquipBestWeaponForThreat( threat );
	}

	if ( TFGameRules()->InSetup() )
	{
		m_PathFollower.Invalidate();
		m_recomputePathTimer.Start( RandomFloat( 1.0f, 2.0f ) );

		return BaseClass::Continue();
	}

	CTeamTrainWatcher *pWatcher = TFGameRules()->GetPayloadToPush( me->GetTeamNumber() );
	if ( pWatcher == nullptr )
		return BaseClass::Continue();

	CBaseEntity *pTrain = pWatcher->GetTrainEntity();
	if ( pTrain == nullptr )
		return BaseClass::Continue();

	if ( m_recomputePathTimer.IsElapsed() )
	{
		VPROF_BUDGET( "CTFBotPayloadPush::Update( repath )", "NextBot" );

		Vector vecFwd;
		pTrain->GetVectors( &vecFwd, nullptr, nullptr );

		Vector vecGoal = pTrain->WorldSpaceCenter() - vecFwd * tf_bot_cart_push_radius.GetFloat();

		threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
		if ( threat )
		{
			Vector vecDir = pTrain->WorldSpaceCenter() - threat->GetLastKnownPosition();
			vecDir.NormalizeInPlace();

			vecGoal = pTrain->WorldSpaceCenter() - vecDir * tf_bot_cart_push_radius.GetFloat();
		}

		CTFBotPathCost func( me );
		m_PathFollower.Compute( me, vecGoal, func );

		m_recomputePathTimer.Start( RandomFloat( 0.2f, 0.4f ) );
	}

	m_PathFollower.Update( me );

	return BaseClass::Continue();
}

ActionResult<CTFBot> CTFBotPayloadPush::OnResume( CTFBot *me, Action<CTFBot> *priorAction )
{
	VPROF_BUDGET( "CTFBotPayloadPush::OnResume", "NextBot" );

	m_recomputePathTimer.Invalidate();

	return BaseClass::Continue();
}


EventDesiredResult<CTFBot> CTFBotPayloadPush::OnMoveToSuccess( CTFBot *me, const Path *path )
{
	return BaseClass::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotPayloadPush::OnMoveToFailure( CTFBot *me, const Path *path, MoveToFailureType fail )
{
	VPROF_BUDGET( "CTFBotPayloadPush::OnMoveToFailure", "NextBot" );

	m_recomputePathTimer.Invalidate();

	return BaseClass::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotPayloadPush::OnStuck( CTFBot *me )
{
	VPROF_BUDGET( "CTFBotPayloadPush::OnStuck", "NextBot" );

	m_recomputePathTimer.Invalidate();
	me->GetLocomotionInterface()->ClearStuckStatus();

	return BaseClass::TryContinue();
}


QueryResultType CTFBotPayloadPush::ShouldHurry( const INextBot *me ) const
{
	return ANSWER_UNDEFINED;
}

QueryResultType CTFBotPayloadPush::ShouldRetreat( const INextBot *me ) const
{
	return ANSWER_UNDEFINED;
}
