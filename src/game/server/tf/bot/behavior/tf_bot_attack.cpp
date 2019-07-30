#include "cbase.h"
#include "../tf_bot.h"
#include "tf_bot_attack.h"
#include "tf_gamerules.h"


CTFBotAttack::CTFBotAttack()
{
}

CTFBotAttack::~CTFBotAttack()
{
}


const char *CTFBotAttack::GetName() const
{
	return "Attack";
}


ActionResult<CTFBot> CTFBotAttack::OnStart( CTFBot *me, Action<CTFBot> *priorAction )
{
	m_PathFollower.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotAttack::Update( CTFBot *me, float dt )
{
	bool bIsMelee = false;

	CTFWeaponBase *weapon = me->GetActiveTFWeapon();
	if (weapon != nullptr)
	{
		if (weapon->IsWeapon( TF_WEAPON_FLAMETHROWER ) || weapon->IsMeleeWeapon())
		{
			bIsMelee = true;
		}
	}

	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	if (threat == nullptr || threat->IsObsolete() || me->GetIntentionInterface()->ShouldAttack( me, threat ) == ANSWER_NO)
	{
		return Action<CTFBot>::Done( "No threat" );
	}

	me->EquipBestWeaponForThreat( threat );

	if (bIsMelee && threat->IsVisibleRecently() && me->IsRangeLessThan( threat->GetLastKnownPosition(), 1.1f * me->GetDesiredAttackRange() ))
	{
		if (me->TransientlyConsistentRandomValue( 3.0f, 0 ) < 0.5f)
		{
			me->PressLeftButton();
		}
		else
		{
			me->PressRightButton();
		}
	}

	if (threat->IsVisibleRecently() &&
		!me->IsRangeGreaterThan( threat->GetEntity()->GetAbsOrigin(), me->GetDesiredAttackRange() ) &&
		me->IsLineOfFireClear( threat->GetEntity()->EyePosition() ))
	{
		return Action<CTFBot>::Continue();
	}

	if (threat->IsVisibleRecently())
	{
		CTFBotPathCost func( me, ( bIsMelee && TFGameRules()->IsMannVsMachineMode() ? SAFEST_ROUTE : DEFAULT_ROUTE ) );
		m_ChasePath.Update( me, threat->GetEntity(), func );

		return Action<CTFBot>::Continue();
	}

	m_ChasePath.Invalidate();

	if (me->IsRangeLessThan( threat->GetLastKnownPosition(), 20.0f ))
	{
		me->GetVisionInterface()->ForgetEntity( threat->GetEntity() );

		return Action<CTFBot>::Done( "I lost my target!" );
	}

	if (me->IsRangeLessThan( threat->GetLastKnownPosition(), me->GetMaxAttackRange() ))
	{
		const Vector eye = threat->GetLastKnownPosition() + Vector( 0.0f, 0.0f, HumanEyeHeight );
		me->GetBodyInterface()->AimHeadTowards( eye, IBody::IMPORTANT, 0.2f, nullptr, "Looking towards where we lost sight of our victim" );
	}

	m_PathFollower.Update( me );

	if (m_recomputeTimer.IsElapsed())
	{
		m_recomputeTimer.Start( RandomFloat( 3.0f, 5.0f ) );

		CTFBotPathCost func( me, ( bIsMelee && TFGameRules()->IsMannVsMachineMode() ? SAFEST_ROUTE : DEFAULT_ROUTE ) );
		m_PathFollower.Compute( me, threat->GetLastKnownPosition(), func );
	}

	return Action<CTFBot>::Continue();
}


EventDesiredResult<CTFBot> CTFBotAttack::OnMoveToSuccess( CTFBot *me, const Path *path )
{
	return Action<CTFBot>::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotAttack::OnMoveToFailure( CTFBot *me, const Path *path, MoveToFailureType reason )
{
	return Action<CTFBot>::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotAttack::OnStuck( CTFBot *me )
{
	return Action<CTFBot>::TryContinue();
}


QueryResultType CTFBotAttack::ShouldHurry( const INextBot *me ) const
{
	return ANSWER_UNDEFINED;
}

QueryResultType CTFBotAttack::ShouldRetreat( const INextBot *me ) const
{
	return ANSWER_UNDEFINED;
}
