#include "cbase.h"
#include "../../tf_bot.h"
#include "tf_bot_spy_sap.h"
#include "tf_bot_spy_attack.h"
#include "tf_obj.h"


CTFBotSpySap::CTFBotSpySap( CBaseObject *target )
{
	m_hTarget = target;
}

CTFBotSpySap::~CTFBotSpySap()
{
}


const char *CTFBotSpySap::GetName( void ) const
{
	return "SpySap";
}


ActionResult<CTFBot> CTFBotSpySap::OnStart( CTFBot *me, Action<CTFBot> *priorAction )
{
	m_PathFollower.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	me->StopLookingForEnemies();

	if ( me->m_Shared.IsStealthed() )
		me->PressAltFireButton();

	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotSpySap::Update( CTFBot *me, float dt )
{
	if ( me->GetNearestKnownSappableTarget() )
		m_hTarget = me->GetNearestKnownSappableTarget();

	if ( m_hTarget == nullptr )
		return Action<CTFBot>::Done( "Sap target gone" );

	CUtlVector<CKnownEntity> knowns;
	me->GetVisionInterface()->CollectKnownEntities( &knowns );

	CTFPlayer *engie = nullptr;

	FOR_EACH_VEC( knowns, i )
	{
		CTFPlayer *player = ToTFPlayer( knowns[i].GetEntity() );
		if ( player == nullptr ) continue;

		/* why does this only consider the first known player? */
		if ( me->IsEnemy( player ) )
		{
			engie = player;

			if ( player->IsPlayerClass( TF_CLASS_ENGINEER ) && m_hTarget->GetOwner() == player &&
				 me->IsRangeLessThan( player, 150.0f ) && me->IsEntityBetweenTargetAndSelf( engie, m_hTarget ) )
			{
				return Action<CTFBot>::SuspendFor( new CTFBotSpyAttack( engie ), "Backstabbing the engineer before I sap his buildings" );
			}

			break;
		}
	}

	if ( me->IsRangeLessThan( m_hTarget, 80.0f ) )
	{
		CBaseCombatWeapon *sapper = me->Weapon_GetWeaponByType( TF_WPN_TYPE_BUILDING );
		if ( sapper == nullptr )
			return Action<CTFBot>::Done( "I have no sapper" );

		me->Weapon_Switch( sapper );

		if ( me->m_Shared.IsStealthed() )
			me->PressAltFireButton();

		me->GetBodyInterface()->AimHeadTowards( m_hTarget, IBody::MANDATORY, 0.1f, nullptr, "Aiming my sapper" );

		me->PressFireButton();
	}

	if ( !me->IsRangeGreaterThan( m_hTarget, 40.0f ) )
	{
		if ( m_hTarget->HasSapper() )
		{
			CBaseObject *new_target = me->GetNearestKnownSappableTarget();
			if ( new_target != nullptr )
			{
				m_hTarget = new_target;
			}
			else
			{
				if ( engie != nullptr && engie->IsPlayerClass( TF_CLASS_ENGINEER ) )
				{
					return Action<CTFBot>::SuspendFor( new CTFBotSpyAttack( engie ), "Attacking an engineer" );
				}
				else
				{
					return Action<CTFBot>::Done( "All targets sapped" );
				}
			}
		}

		return Action<CTFBot>::Continue();
	}

	if ( m_recomputePath.IsElapsed() )
	{
		m_recomputePath.Start( RandomFloat( 1.0f, 2.0f ) );

		CTFBotPathCost func( me, FASTEST_ROUTE );
		m_PathFollower.Compute( me, this->m_hTarget, func );
	}

	m_PathFollower.Update( me );

	return Action<CTFBot>::Continue();
}

void CTFBotSpySap::OnEnd( CTFBot *me, Action<CTFBot> *newAction )
{
	me->WantsToLookForEnemies();
}

ActionResult<CTFBot> CTFBotSpySap::OnSuspend( CTFBot *me, Action<CTFBot> *newAction )
{
	me->WantsToLookForEnemies();

	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotSpySap::OnResume( CTFBot *me, Action<CTFBot> *priorAction )
{
	me->StopLookingForEnemies();

	return Action<CTFBot>::Continue();
}


EventDesiredResult<CTFBot> CTFBotSpySap::OnStuck( CTFBot *me )
{
	return Action<CTFBot>::TryDone( RESULT_CRITICAL, "I'm stuck, probably on a sapped building that hasn't exploded yet" );
}


QueryResultType CTFBotSpySap::ShouldRetreat( const INextBot *me ) const
{
	return ANSWER_NO;
}

QueryResultType CTFBotSpySap::ShouldAttack( const INextBot *me, const CKnownEntity *threat ) const
{
	CTFBot *actor = ToTFBot( me->GetEntity() );

	if ( m_hTarget == nullptr || m_hTarget->HasSapper() )
	{
		if ( actor->m_Shared.InCond( TF_COND_DISGUISED ) || actor->m_Shared.InCond( TF_COND_DISGUISING ) || actor->m_Shared.IsStealthed() )
		{
			return AreAllDangerousSentriesSapped( actor );
		}
		else
		{
			return ANSWER_YES;
		}
	}

	return ANSWER_NO;
}

QueryResultType CTFBotSpySap::IsHindrance( const INextBot *me, CBaseEntity *it ) const
{
	if ( m_hTarget != nullptr && me->IsRangeLessThan( m_hTarget, 300.0f ) )
		return ANSWER_NO;

	return ANSWER_UNDEFINED;
}


QueryResultType CTFBotSpySap::AreAllDangerousSentriesSapped( CTFBot *actor ) const
{
	CUtlVector<CKnownEntity> knowns;
	actor->GetVisionInterface()->CollectKnownEntities( &knowns );

	FOR_EACH_VEC( knowns, i )
	{
		CBaseObject *obj = dynamic_cast<CBaseObject *>( knowns[i].GetEntity() );
		if ( !obj )
			continue;

		if ( obj->ObjectType() == OBJ_SENTRYGUN && !obj->HasSapper() && actor->IsEnemy( obj ) &&
			 actor->IsRangeLessThan( obj, 1100.0f ) && actor->IsLineOfFireClear( obj ) )
		{
			return ANSWER_NO;
		}
	}

	return ANSWER_YES;
}
