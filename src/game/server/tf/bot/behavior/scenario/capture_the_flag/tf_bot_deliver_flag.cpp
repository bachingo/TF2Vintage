//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "tf_bot.h"
#include "tf_gamerules.h"
#include "entity_capture_flag.h"
#include "func_capture_zone.h"
#include "NavMeshEntities/func_nav_prerequisite.h"
#include "tf_bot_deliver_flag.h"
#include "behavior/nav_entities/tf_bot_nav_ent_wait.h"
#include "behavior/nav_entities/tf_bot_nav_ent_move_to.h"



const char *CTFBotDeliverFlag::GetName() const
{
	return "DeliverFlag";
}


ActionResult<CTFBot> CTFBotDeliverFlag::OnStart( CTFBot *me, Action<CTFBot> *priorAction )
{
	m_recomputePathTimer.Invalidate();
	m_PathFollower.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	// MvM carrier upgrade logic goes here

	return BaseClass::Continue();
}

ActionResult<CTFBot> CTFBotDeliverFlag::Update( CTFBot *me, float dt )
{
	CCaptureFlag *pFlag = me->GetFlagToFetch();
	if ( pFlag == nullptr )
		return BaseClass::Done( "No flag" );

	CTFPlayer *pCarrier = ToTFPlayer( pFlag->GetOwnerEntity() );
	if ( pCarrier == nullptr || !me->IsSelf( pCarrier ) )
		return BaseClass::Done( "I'm no longer carrying the flag" );

	Action<CTFBot> *pBuffAction = me->OpportunisticallyUseWeaponAbilities();
	if ( pBuffAction )
		return BaseClass::SuspendFor( pBuffAction, "Opportunistically using buff item" );

	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	if ( threat && threat->IsVisibleRecently() )
	{
		me->EquipBestWeaponForThreat( threat );
	}

	// MvM carrier upgrade logic goes here

	if ( m_recomputePathTimer.IsElapsed() )
	{
		CCaptureZone *pZone = me->GetFlagCaptureZone();
		if ( pZone == nullptr )
			return BaseClass::Done( "No flag capture zone exists!" );

		CTFBotPathCost func( me, FASTEST_ROUTE );
		m_PathFollower.Compute( me, pZone->WorldSpaceCenter(), func );

		m_recomputePathTimer.Start( RandomFloat( 1.0, 2.0 ) );
	}

	m_PathFollower.Update( me );

	return BaseClass::Continue();
}

void CTFBotDeliverFlag::OnEnd( CTFBot *me, Action<CTFBot> *newAction )
{
	
}


QueryResultType CTFBotDeliverFlag::ShouldHurry( const INextBot *nextbot ) const
{
	return ANSWER_YES;
}

QueryResultType CTFBotDeliverFlag::ShouldRetreat( const INextBot *nextbot ) const
{
	return ANSWER_NO;
}



const char *CTFBotPushToCapturePoint::GetName() const
{
	return "PushToCapturePoint";
}


ActionResult<CTFBot> CTFBotPushToCapturePoint::Update( CTFBot *me, float dt )
{
	CCaptureZone *pZone = me->GetFlagCaptureZone();
	if ( pZone == nullptr )
	{
		if ( m_pDoneAction )
			return BaseClass::ChangeTo( m_pDoneAction, "No flag capture zone exists!" );

		return BaseClass::Done( "No flag capture zone exists!" );
	}

	if ( ( pZone->WorldSpaceCenter() - me->GetAbsOrigin() ).LengthSqr() < Square( 50.0f ) )
	{
		if ( m_pDoneAction )
			return BaseClass::ChangeTo( m_pDoneAction, "At destination" );

		return BaseClass::Done( "At destination" );
	}

	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	if ( threat && threat->IsVisibleRecently() )
	{
		me->EquipBestWeaponForThreat( threat );
	}

	if ( m_recomputePathTimer.IsElapsed() )
	{
		CTFBotPathCost func( me, FASTEST_ROUTE );
		m_PathFollower.Compute( me, pZone->WorldSpaceCenter(), func );

		m_recomputePathTimer.Start( RandomFloat( 1.0, 2.0 ) );
	}

	m_PathFollower.Update( me );

	return BaseClass::Continue();
}


EventDesiredResult<CTFBot> CTFBotPushToCapturePoint::OnNavAreaChanged( CTFBot *me, CNavArea *area1, CNavArea *area2 )
{
	if ( area1 == nullptr || !area1->HasPrerequisite() )
		return BaseClass::TryContinue();

	/*FOR_EACH_VEC( area1->GetPrerequisiteVector(), i )
	{
		CFuncNavPrerequisite *prereq = area1->GetPrerequisiteVector()[ i ];
		if ( prereq == nullptr || !prereq->IsEnabled() || !prereq->PassesTriggerFilters( me ) )
			continue;

		if ( prereq->IsTask( CFuncNavPrerequisite::TASK_WAIT ) )
			return BaseClass::TrySuspendFor( new CTFBotNavEntMoveTo( prereq ), RESULT_TRY, "Prerequisite commands me to move to an entity" );

		if ( prereq->IsTask( CFuncNavPrerequisite::TASK_MOVE_TO_ENTITY ) )
			return BaseClass::TrySuspendFor( new CTFBotNavEntWait( prereq ), RESULT_TRY, "Prerequisite commands me to wait" );
	}*/

	return BaseClass::TryContinue();
}
