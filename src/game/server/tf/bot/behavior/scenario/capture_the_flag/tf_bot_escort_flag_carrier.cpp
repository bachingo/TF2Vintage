//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "tf_bot.h"
#include "entity_capture_flag.h"
#include "tf_bot_escort_flag_carrier.h"
#include "tf_bot_attack_flag_defenders.h"


ConVar tf_bot_flag_escort_give_up_range( "tf_bot_flag_escort_give_up_range", "1000", FCVAR_CHEAT );
ConVar tf_bot_flag_escort_max_count( "tf_bot_flag_escort_max_count", "4", FCVAR_CHEAT );
ConVar tf_bot_flag_escort_range( "tf_bot_flag_escort_range", "500", FCVAR_CHEAT );



const char *CTFBotEscortFlagCarrier::GetName() const
{
	return "EscortFlagCarrier";
}


ActionResult<CTFBot> CTFBotEscortFlagCarrier::OnStart( CTFBot *me, Action<CTFBot> *priorAction )
{
	m_PathFollower.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	return BaseClass::Continue();
}

ActionResult<CTFBot> CTFBotEscortFlagCarrier::Update( CTFBot *me, float dt )
{
	CCaptureFlag *pFlag = me->GetFlagToFetch();
	if ( pFlag == nullptr )
		return BaseClass::Done( "No flag" );

	CTFPlayer *pCarrier = ToTFPlayer( pFlag->GetOwnerEntity() );
	if ( pCarrier == nullptr )
		return BaseClass::Done( "Flag was dropped" );

	if ( me->IsSelf( pCarrier ) )
		return BaseClass::Done( "I picked up the flag!" );

	if ( me->IsRangeGreaterThan( pCarrier, tf_bot_flag_escort_give_up_range.GetFloat() ) && me->SelectRandomReachableEnemy() )
		return BaseClass::ChangeTo( new CTFBotAttackFlagDefenders, "Too far from flag carrier - attack defenders!" );

	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	if ( threat && threat->IsVisibleRecently() )
	{
		me->EquipBestWeaponForThreat( threat );
	}

	CTFWeaponBase *pWeapon = me->GetActiveTFWeapon();
	if ( pWeapon && pWeapon->IsMeleeWeapon() &&
		 me->IsRangeLessThan( pCarrier, tf_bot_flag_escort_range.GetFloat() ) &&
		 me->IsLineOfSightClear( pCarrier ) )
	{
		auto result = m_MeleeAttack.Update( me, dt );

		if ( result.m_type == ActionResultType::CONTINUE || !me->IsRangeGreaterThan( pCarrier, 0.5f * tf_bot_flag_escort_range.GetFloat() ) )
			return BaseClass::Continue();
	}

	if ( m_recomputePathTimer.IsElapsed() )
	{
		if ( GetBotEscortCount( me->GetTeamNumber() ) > tf_bot_flag_escort_max_count.GetInt() && me->SelectRandomReachableEnemy() )
			return BaseClass::Done( "Too many flag escorts - giving up" );

		CTFBotPathCost func( me, FASTEST_ROUTE );
		m_PathFollower.Compute( me, pCarrier, func );

		m_recomputePathTimer.Start( RandomFloat( 1.0f, 2.0f ) );
	}

	m_PathFollower.Update( me );

	return BaseClass::Continue();
}


int GetBotEscortCount( int iTeamNum )
{
	CUtlVector<CTFPlayer *> teammates;
	CollectPlayers( &teammates, iTeamNum, true );

	int nCount = 0;
	FOR_EACH_VEC( teammates, i ) {
		CTFBot *pBot = ToTFBot( teammates[ i ] );
		if ( pBot == nullptr )
			continue;

		auto pBehavior = pBot->GetIntentionInterface()->FirstContainedResponder();
		if ( pBehavior == nullptr )
			continue;

		auto pAction = static_cast<Action<CTFBot> *>( pBehavior->FirstContainedResponder() );
		if ( pAction == nullptr )
			continue;

		while ( pAction->FirstContainedResponder() )
			pAction = static_cast<Action<CTFBot> *>( pAction->FirstContainedResponder() );

		if ( pAction->IsNamed( "EscortFlagCarrier" ) )
			++nCount;
	}

	return nCount;
}
