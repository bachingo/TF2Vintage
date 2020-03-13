#include "cbase.h"
#include "../../tf_bot.h"
#include "tf_bot_spy_attack.h"
#include "tf_bot_spy_sap.h"
#include "../tf_bot_retreat_to_cover.h"
#include "tf_gamerules.h"
#include "tf_obj.h"


ConVar tf_bot_spy_knife_range( "tf_bot_spy_knife_range", "300", FCVAR_CHEAT, "If threat is closer than this, prefer our knife" );
ConVar tf_bot_spy_change_target_range_threshold( "tf_bot_spy_change_target_range_threshold", "300", FCVAR_CHEAT );


CTFBotSpyAttack::CTFBotSpyAttack( CTFPlayer *victim )
{
	m_hVictim = victim;
}

CTFBotSpyAttack::~CTFBotSpyAttack()
{
}


const char *CTFBotSpyAttack::GetName() const
{
	return "SpyAttack";
}


ActionResult<CTFBot> CTFBotSpyAttack::OnStart( CTFBot *me, Action<CTFBot> *action )
{
	m_ChasePath.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	m_bInDanger = false;

	if ( m_hVictim != nullptr )
		me->GetVisionInterface()->AddKnownEntity( m_hVictim );

	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotSpyAttack::Update( CTFBot *me, float dt )
{
	const CKnownEntity *victim = me->GetVisionInterface()->GetKnown( m_hVictim );
	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();

	if ( victim )
	{
		if ( threat && threat->GetEntity() && victim != threat )
		{
			float victimDist = me->GetRangeTo( victim->GetLastKnownPosition() );
			float threatDist = me->GetRangeTo( threat->GetLastKnownPosition() );
			if ( ( victimDist - threatDist ) > tf_bot_spy_change_target_range_threshold.GetFloat() )
			{
				if ( threat->GetEntity()->IsPlayer() )
				{
					m_hVictim = ToTFPlayer( threat->GetEntity() );
					m_bInDanger = false;
					victim = threat;
				}
			}
		}
	}
	else
	{
		m_bInDanger = false;
		if ( !threat )
			return Action<CTFBot>::Done( "No threat" );

		m_hVictim = ToTFPlayer( threat->GetEntity() );
		victim = threat;
	}

	if ( victim->IsObsolete() )
		return Action<CTFBot>::Done( "No threat" );

	CBaseObject *obj = me->GetNearestKnownSappableTarget();
	if ( obj != nullptr && me->IsEntityBetweenTargetAndSelf( obj, victim->GetEntity() ) )
		return Action<CTFBot>::ChangeTo( new CTFBotSpySap( obj ), "Opportunistically sapping an enemy object between my victim and I" );

	if ( me->IsAnyEnemySentryAbleToAttackMe() )
	{
		m_bInDanger = true;

		me->Weapon_Switch( me->Weapon_GetWeaponByType( TF_WPN_TYPE_PRIMARY ) );

		return Action<CTFBot>::ChangeTo( new CTFBotRetreatToCover, "Escaping sentry fire!" );
	}

	CTFPlayer *victimPlayer = ToTFPlayer( victim->GetEntity() );
	if ( victimPlayer == nullptr )
		return Action<CTFBot>::Done( "Current 'threat' is not a player or a building?" );

	if ( me->m_Shared.IsStealthed() )
	{
		if ( m_stealthTimer.IsElapsed() )
		{
			me->PressAltFireButton();
			m_stealthTimer.Start( 1.0f );
		}
	}

	float flDesiredDot;
	switch ( me->m_iSkill )
	{
		case CTFBot::EASY:
			flDesiredDot = VIEW_FIELD_ULTRA_NARROW;
			break;
		case CTFBot::NORMAL:
			flDesiredDot = DOT_45DEGREE;
			break;
		case CTFBot::HARD:
			flDesiredDot = 0.2;
			break;

		default:
			flDesiredDot = 0.0f;
			break;
	}

	bool bPullKnife = false;
	if ( me->m_Shared.InCond( TF_COND_DISGUISED ) || me->m_Shared.InCond( TF_COND_DISGUISING ) || me->m_Shared.IsStealthed() )
		bPullKnife = true;

	Vector vecToActor, vecFwd;
	victimPlayer->EyeVectors( &vecFwd );

	vecToActor = victimPlayer->GetAbsOrigin() - me->GetAbsOrigin();
	float flLength = vecToActor.NormalizeInPlace();

	if ( flLength >= tf_bot_spy_knife_range.GetFloat() )
	{
		if ( victim->IsVisibleInFOVNow() )
			bPullKnife = ( me->m_iSkill == CTFBot::EASY || vecToActor.Dot( vecFwd ) > flDesiredDot );
	}
	else
	{
		bPullKnife = true;
	}

	float flTimeSinceHurt = me->GetTimeSinceLastInjury( GetEnemyTeam( me ) );
	if ( flTimeSinceHurt < 1.0f || me->IsThreatAimingTowardsMe( victimPlayer, 0.99f ) )
		m_bInDanger = victimPlayer->GetTimeSinceWeaponFired() < 0.25f;

	int iDesiredSlot = TF_WPN_TYPE_MELEE;
	if ( m_bInDanger ||
		 me->m_Shared.InCond( TF_COND_BURNING ) ||
		 me->m_Shared.InCond( TF_COND_BLEEDING ) ||
		 me->m_Shared.InCond( TF_COND_URINE ) ||
		 me->m_Shared.InCond( TF_COND_STEALTHED_BLINK ) ||
		 !bPullKnife )
	{
		iDesiredSlot = TF_WPN_TYPE_PRIMARY;
	}

	me->Weapon_Switch( me->Weapon_GetWeaponByType( iDesiredSlot ) );

	if ( me->GetActiveTFWeapon() && me->GetActiveTFWeapon()->IsMeleeWeapon() )
	{
		if ( victim->IsVisibleInFOVNow() && flLength < 100.0f )
		{
			me->GetBodyInterface()->AimHeadTowards( victimPlayer, IBody::MANDATORY, 0.1f, nullptr, "Aiming my stab" );

			bool bWaitingForStab = false;
			if ( me->m_iSkill == CTFBot::EASY || vecToActor.Dot( vecFwd ) > flDesiredDot )
			{
				bWaitingForStab = true;
			}
			else
			{
				Vector vecMyFwd;
				me->EyeVectors( &vecMyFwd );

				if ( vecMyFwd.AsVector2D().DistToSqr( vecFwd.AsVector2D() ) < 0.0f )
					me->PressRightButton();
				else
					me->PressLeftButton();

				bWaitingForStab = true;
			}

			if ( me->GetDesiredAttackRange() > flLength )
			{
				if ( !me->m_Shared.InCond( TF_COND_DISGUISED ) || ( me->m_iSkill == CTFBot::EASY || vecToActor.Dot( vecFwd ) > flDesiredDot ) || m_bInDanger )
					me->PressFireButton();
			}

			if ( !bWaitingForStab )
				return Action<CTFBot>::Continue();
		}
	}

	me->GetBodyInterface()->AimHeadTowards( victimPlayer, IBody::MANDATORY, 0.1f, nullptr, "Aiming my pistol" );

	if ( !threat->IsVisibleRecently() && me->IsRangeLessThan( threat->GetLastKnownPosition(), 200.0f ) )
	{
		me->GetVisionInterface()->ForgetEntity( threat->GetEntity() );
		return Action<CTFBot>::Done( "I lost my target!" );
	}

	if ( me->IsRangeGreaterThan( victimPlayer, me->GetDesiredAttackRange() ) || !me->IsLineOfFireClear( victimPlayer->EyePosition() ) )
	{
		if ( !threat->IsVisibleRecently() && me->IsRangeLessThan( threat->GetLastKnownPosition(), 200.0f ) )
		{
			me->GetVisionInterface()->ForgetEntity( threat->GetEntity() );
			return Action<CTFBot>::Done( "I lost my target!" );
		}

		CTFBotPathCost cost( me, FASTEST_ROUTE );
		m_ChasePath.Update( me, victimPlayer, cost );
	}

	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotSpyAttack::OnResume( CTFBot *me, Action<CTFBot> *action )
{
	m_ChasePath.Invalidate();

	m_hVictim = nullptr;

	m_bInDanger = false;

	return Action<CTFBot>::Continue();
}


EventDesiredResult<CTFBot> CTFBotSpyAttack::OnContact( CTFBot *me, CBaseEntity *ent, CGameTrace *trace )
{
	if ( me->IsEnemy( ent ) && ToBaseCombatCharacter( ent ) != nullptr )
	{
		CBaseCombatCharacter *enemy = ToBaseCombatCharacter( ent );

		if ( enemy->IsLookingTowards( me ) )
		{
			m_bInDanger = true;
		}
	}

	return Action<CTFBot>::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotSpyAttack::OnStuck( CTFBot *me )
{
	return Action<CTFBot>::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotSpyAttack::OnInjured( CTFBot *me, const CTakeDamageInfo& info )
{
	if ( me->IsEnemy( info.GetAttacker() ) && !me->m_Shared.InCond( TF_COND_DISGUISED ) )
	{
		me->Weapon_Switch( me->Weapon_GetWeaponByType( TF_WPN_TYPE_PRIMARY ) );

		return Action<CTFBot>::TryChangeTo( new CTFBotRetreatToCover, RESULT_IMPORTANT, "Time to get out of here!" );
	}

	return Action<CTFBot>::TryContinue();
}


QueryResultType CTFBotSpyAttack::ShouldHurry( const INextBot *me ) const
{
	return ANSWER_YES;
}

QueryResultType CTFBotSpyAttack::ShouldRetreat( const INextBot *me ) const
{
	return ANSWER_UNDEFINED;
}

QueryResultType CTFBotSpyAttack::ShouldAttack( const INextBot *me, const CKnownEntity *threat ) const
{
	CTFBot *actor = ToTFBot( me->GetEntity() );

	if ( !m_bInDanger &&
		 !actor->m_Shared.InCond( TF_COND_BURNING ) &&
		 !actor->m_Shared.InCond( TF_COND_URINE ) &&
		 !actor->m_Shared.InCond( TF_COND_STEALTHED_BLINK ) &&
		 !actor->m_Shared.InCond( TF_COND_BLEEDING ) )
	{
		return ANSWER_NO;
	}

	return ANSWER_YES;
}

QueryResultType CTFBotSpyAttack::IsHindrance( const INextBot *me, CBaseEntity *blocker ) const
{
	if ( blocker == IS_ANY_HINDRANCE_POSSIBLE )
		return ANSWER_UNDEFINED;

	if ( blocker != nullptr && m_hVictim != nullptr )
	{
		if ( ENTINDEX( blocker ) == ENTINDEX( m_hVictim ) )
			return ANSWER_NO;
	}

	return ANSWER_UNDEFINED;
}

const CKnownEntity *CTFBotSpyAttack::SelectMoreDangerousThreat( const INextBot *nextbot, const CBaseCombatCharacter *them, const CKnownEntity *threat1, const CKnownEntity *threat2 ) const
{
	CTFBot *actor = ToTFBot( nextbot->GetEntity() );

	if ( !actor->IsSelf( them ) )
	{
		return nullptr;
	}

	CTFWeaponBase *weapon = actor->GetActiveTFWeapon();
	if ( weapon == nullptr || !weapon->IsMeleeWeapon() )
	{
		return nullptr;
	}

	float dist1 = actor->GetRangeSquaredTo( threat1->GetEntity() );
	float dist2 = actor->GetRangeSquaredTo( threat2->GetEntity() );

	if ( dist1 < dist2 )
	{
		return threat1;
	}
	else
	{
		return threat2;
	}
}
