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
#include "tf_bot_attack_flag_defenders.h"
#include "tf_bot_escort_flag_carrier.h"


extern ConVar tf_bot_flag_escort_range;
extern ConVar tf_bot_flag_escort_max_count;


CTFBotAttackFlagDefenders::CTFBotAttackFlagDefenders( float duration )
{
	if ( duration > 0.0f )
		m_actionDuration.Start( duration );
}


const char *CTFBotAttackFlagDefenders::GetName() const
{
	return "AttackFlagDefenders";
}


ActionResult<CTFBot> CTFBotAttackFlagDefenders::OnStart( CTFBot *me, Action<CTFBot> *action )
{
	m_PathFollower.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	m_hTarget = nullptr;

	return BaseClass::OnStart( me, action );
}

ActionResult<CTFBot> CTFBotAttackFlagDefenders::Update( CTFBot *me, float dt )
{
	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	if ( threat && threat->IsVisibleRecently() )
	{
		me->EquipBestWeaponForThreat( threat );
	}

	if ( m_checkFlagTimer.IsElapsed() && m_actionDuration.IsElapsed() )
	{
		m_checkFlagTimer.Start( RandomFloat( 1.0f, 3.0f ) );

		CCaptureFlag *pFlag = me->GetFlagToFetch();
		if ( pFlag == nullptr )
			return BaseClass::Done( "No flag" );

		if ( !pFlag->IsHome() )
		{
			CTFPlayer *pCarrier = ToTFPlayer( pFlag->GetOwnerEntity() );
			if ( pCarrier == nullptr )
				return BaseClass::Done( "Flag was dropped" );

			if ( me->IsSelf( pCarrier ) )
				return BaseClass::Done( "I picked up the flag!" );
			
			CTFBot *pCarrierBot = ToTFBot( pCarrier );
			if ( pCarrierBot == nullptr || !pCarrierBot->m_pSquad )
			{
				if ( me->IsRangeLessThan( pCarrier, tf_bot_flag_escort_range.GetFloat() ) &&
					 GetBotEscortCount( me->GetTeamNumber() ) < tf_bot_flag_escort_max_count.GetInt() )
				{
					return BaseClass::ChangeTo( new CTFBotEscortFlagCarrier, "Near flag carrier - escorting" );
				}
			}
		}
	}

	auto result = BaseClass::Update( me, dt );
	if ( result.m_type != ActionResultType::DONE )
		return BaseClass::Continue();

	if ( !m_hTarget || !m_hTarget->IsAlive() )
	{
		CTFPlayer *target = me->SelectRandomReachableEnemy();
		if ( target == nullptr )
			return BaseClass::ChangeTo( new CTFBotEscortFlagCarrier, "No reachable victim - escorting flag" );

		m_hTarget = target;
	}

	me->GetVisionInterface()->AddKnownEntity( m_hTarget );

	if ( m_recomputePathTimer.IsElapsed() )
	{
		m_recomputePathTimer.Start( RandomFloat( 1.0f, 3.0f ) );

		CTFBotPathCost func( me );
		m_PathFollower.Compute( me, m_hTarget, func );
	}

	m_PathFollower.Update( me );

	return BaseClass::Continue();
}
