//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "tf_bot.h"
#include "entity_capture_flag.h"
#include "tf_bot_fetch_flag.h"
#include "tf_bot_attack_flag_defenders.h"


const char *CTFBotFetchFlag::GetName() const
{
	return "FetchFlag";
}


ActionResult<CTFBot> CTFBotFetchFlag::OnStart( CTFBot *me, Action<CTFBot> *priorAction )
{
	m_PathFollower.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	return BaseClass::Continue();
}

ActionResult<CTFBot> CTFBotFetchFlag::Update( CTFBot *me, float dt )
{
	CCaptureFlag *pFlag = me->GetFlagToFetch();
	if ( pFlag == nullptr )
		return BaseClass::Done( "No flag" );

	if ( me->m_Shared.IsStealthed() )
		me->PressAltFireButton();

	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	if ( threat && threat->IsVisibleRecently() )
	{
		me->EquipBestWeaponForThreat( threat );
	}

	CTFPlayer *pCarrier = ToTFPlayer( pFlag->GetOwnerEntity() );
	if ( pCarrier )
	{
		if ( m_bGiveUpWhenDone )
		{
			return BaseClass::Done( "Someone else picked up the flag" );
		}
		else
		{
			return BaseClass::SuspendFor( new CTFBotAttackFlagDefenders, "Someone has the flag - attacking the enemy defenders" );
		}
	}

	if ( m_recomputePathTimer.IsElapsed() )
	{
		CTFBotPathCost func( me );
		if ( !m_PathFollower.Compute( me, pFlag->WorldSpaceCenter(), func ) && pFlag->IsDropped() )
			return BaseClass::SuspendFor( new CTFBotAttackFlagDefenders( RandomFloat( 5.0f, 10.0f ) ), "Flag unreachable - Attacking" );

		m_recomputePathTimer.Start( RandomFloat( 1.0f, 2.0f ) );
	}

	m_PathFollower.Update( me );

	return BaseClass::Continue();
}


QueryResultType CTFBotFetchFlag::ShouldHurry( const INextBot *nextbot ) const
{
	return ANSWER_YES;
}

QueryResultType CTFBotFetchFlag::ShouldRetreat( const INextBot *nextbot ) const
{
	return ANSWER_NO;
}
