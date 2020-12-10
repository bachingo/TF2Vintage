//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "../tf_bot.h"
#include "team.h"
#include "tf_gamerules.h"
#include "tf_obj_sentrygun.h"
#include "tf_weapon_minigun.h"
#include "tf_weapon_flamethrower.h"
#include "tf_weapon_compound_bow.h"
#include "nav_mesh/tf_nav_area.h"
#include "tf_bot_behavior.h"
#include "tf_bot_dead.h"
#include "tf_bot_tactical_monitor.h"
#include "tf_bot_taunt.h"
#include "../tf_bot_manager.h"


ConVar tf_bot_sniper_aim_error( "tf_bot_sniper_aim_error", "0.01", FCVAR_CHEAT );
ConVar tf_bot_sniper_aim_steady_rate( "tf_bot_sniper_aim_steady_rate", "10", FCVAR_CHEAT );
ConVar tf_bot_fire_weapon_min_time( "tf_bot_fire_weapon_min_time", "1", FCVAR_CHEAT );
ConVar tf_bot_taunt_victim_chance( "tf_bot_taunt_victim_chance", "20", FCVAR_NONE );
ConVar tf_bot_notice_backstab_chance( "tf_bot_notice_backstab_chance", "25", FCVAR_CHEAT );
ConVar tf_bot_notice_backstab_min_chance( "tf_bot_notice_backstab_min_chance", "100", FCVAR_CHEAT );
ConVar tf_bot_notice_backstab_max_chance( "tf_bot_notice_backstab_max_chance", "750", FCVAR_CHEAT );
ConVar tf_bot_arrow_elevation_rate( "tf_bot_arrow_elevation_rate", "0.0001", FCVAR_CHEAT, "When firing arrows at far away targets, this is the degree/range slope to raise our aim" );
ConVar tf_bot_ballistic_elevation_rate( "tf_bot_ballistic_elevation_rate", "0.01", FCVAR_CHEAT, "When lobbing grenades at far away targets, this is the degree/range slope to raise our aim" );
ConVar tf_bot_hitscan_range_limit( "tf_bot_hitscan_range_limit", "1800", FCVAR_CHEAT );
ConVar tf_bot_always_full_reload( "tf_bot_always_full_reload", "0", FCVAR_CHEAT );
ConVar tf_bot_fire_weapon_allowed( "tf_bot_fire_weapon_allowed", "1", FCVAR_CHEAT, "If zero, TFBots will not pull the trigger of their weapons (but will act like they did)", true, 0.0, true, 1.0 );


const char *CTFBotMainAction::GetName( void ) const
{
	return "MainAction";
}


Action<CTFBot> *CTFBotMainAction::InitialContainedAction( CTFBot *actor )
{
	return new CTFBotTacticalMonitor;
}


ActionResult<CTFBot> CTFBotMainAction::OnStart( CTFBot *me, Action<CTFBot> *priorAction )
{
	m_flSniperAimErrorRadius = 0;
	m_flSniperAimErrorAngle = 0;
	m_flYawDelta = 0;
	m_flPreviousYaw = 0;
	m_iDesiredDisguise = 0;
	m_bReloadingBarrage = false;

	return BaseClass::Continue();
}

ActionResult<CTFBot> CTFBotMainAction::Update( CTFBot *me, float dt )
{
	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	if ( me->GetTeamNumber() != TF_TEAM_RED && me->GetTeamNumber() != TF_TEAM_BLUE )
		return BaseClass::Done( "Not on a playing team" );

	me->GetVisionInterface()->SetFieldOfView( me->GetFOV() );

	if ( TFGameRules()->IsInTraining() && me->GetTeamNumber() == TF_TEAM_BLUE )
		me->GiveAmmo( 1000, TF_AMMO_METAL, true );

	m_flYawDelta = me->EyeAngles()[ YAW ] - ( m_flPreviousYaw * dt + FLT_EPSILON );
	m_flPreviousYaw = me->EyeAngles()[ YAW ];

	if ( tf_bot_sniper_aim_steady_rate.GetFloat() <= m_flYawDelta )
		m_sniperSteadyInterval.Invalidate();
	else if ( !m_sniperSteadyInterval.HasStarted() )
		m_sniperSteadyInterval.Start();

	if ( !me->IsFiringWeapon() && !me->m_Shared.InCond( TF_COND_DISGUISED ) && !me->m_Shared.InCond( TF_COND_DISGUISING ) )
	{
		if ( me->CanDisguise() )
		{
			if ( m_iDesiredDisguise != 0 )
			{
				me->m_Shared.Disguise( GetEnemyTeam( me ), m_iDesiredDisguise );
				m_iDesiredDisguise = 0;
			}
			else
			{
				// If we are skilled enough, we are aware of what classes they don't have
				if ( me->m_iSkill > CTFBot::NORMAL )
					me->DisguiseAsEnemy();
				else
					me->m_Shared.Disguise( GetEnemyTeam( me ), RandomInt( 1, 9 ) );
			}
		}
	}

	me->EquipRequiredWeapon();
	me->UpdateLookingAroundForEnemies();
	FireWeaponAtEnemy( me );
	Dodge( me );

	return BaseClass::Continue();
}


EventDesiredResult<CTFBot> CTFBotMainAction::OnContact( CTFBot *me, CBaseEntity *other, CGameTrace *trace )
{
	if ( !other || other->IsSolidFlagSet( FSOLID_NOT_SOLID ) )
		return BaseClass::TryContinue();

	// some miniboss crush logic for MvM happens
	if ( !other->IsWorld() && !other->IsPlayer() )
	{
		m_hLastTouch = other;
		m_flLastTouchTime = gpGlobals->curtime;
	}
	return BaseClass::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotMainAction::OnStuck( CTFBot *me )
{
	if ( me->m_Shared.IsControlStunned() )
	{
		// bot is stunned, not stuck
		return TryContinue();
	}

	UTIL_LogPrintf( "\"%s<%i><%s><%s>\" stuck (position \"%3.2f %3.2f %3.2f\") (duration \"%3.2f\") ",
					me->GetPlayerName(), me->entindex(), me->GetNetworkIDString(), me->GetTeam()->GetName(),
					me->GetAbsOrigin().x, me->GetAbsOrigin().y, me->GetAbsOrigin().z,
					me->GetLocomotionInterface()->GetStuckDuration() );
	if ( me->GetCurrentPath() && me->GetCurrentPath()->IsValid() )
	{
		UTIL_LogPrintf( "   path_goal ( \"%3.2f %3.2f %3.2f\" )\n",
						VectorExpand( me->GetCurrentPath()->GetEndPosition() ) );
	}
	else
	{
		UTIL_LogPrintf( "   path_goal ( \"NULL\" )\n" );
	}

	me->GetLocomotionInterface()->Jump();

	if ( RandomInt( 0, 100 ) < 50 )
		me->PressLeftButton();
	else
		me->PressRightButton();

	return BaseClass::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotMainAction::OnInjured( CTFBot *me, const CTakeDamageInfo &info )
{
	if ( !dynamic_cast<CBaseObject *>( info.GetInflictor() ) )
	{
		me->GetVisionInterface()->AddKnownEntity( info.GetAttacker() );
	}
	else
	{
		me->GetVisionInterface()->AddKnownEntity( info.GetInflictor() );
	}

	if ( info.GetInflictor() && info.GetInflictor()->GetTeamNumber() != me->GetTeamNumber() )
	{
		if ( dynamic_cast<CObjectSentrygun *>( info.GetInflictor() ) )
		{
			me->NoteTargetSentry( info.GetInflictor() );
		}

		if ( info.GetDamageCustom() == TF_DMG_CUSTOM_BACKSTAB )
		{
			me->DelayedThreatNotice( info.GetInflictor(), 0.5 );

			CUtlVector<CTFPlayer *> teammates;
			CollectPlayers( &teammates, me->GetTeamNumber(), true );

			float flChanceMod = 1.0f / ( tf_bot_notice_backstab_max_chance.GetFloat() - tf_bot_notice_backstab_min_chance.GetFloat() );

			FOR_EACH_VEC( teammates, i )
			{
				CTFBot *bot = ToTFBot( teammates[ i ] );
				if ( bot )
				{
					if ( !me->IsSelf( bot ) && me->IsRangeLessThan( bot, tf_bot_notice_backstab_max_chance.GetFloat() ) )
					{
						int iChance = Float2Int(
							( tf_bot_notice_backstab_chance.GetFloat() * flChanceMod )
							*
							( me->GetRangeTo( bot ) - tf_bot_notice_backstab_min_chance.GetFloat() )
						);

						if ( iChance > RandomInt( 0, 100 ) )
							continue;

						bot->DelayedThreatNotice( info.GetInflictor(), 0.5 );
					}
				}
			}
		}
		else
		{
			// I get the DMG_CRITICAL, but DMG_BURN?
			if ( info.GetDamageType() & ( DMG_CRITICAL | DMG_BURN ) )
			{
				if ( me->IsRangeLessThan( info.GetAttacker(), tf_bot_notice_backstab_max_chance.GetFloat() ) )
					me->DelayedThreatNotice( info.GetAttacker(), 0.5 );
			}
		}
	}

	return BaseClass::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotMainAction::OnKilled( CTFBot *me, const CTakeDamageInfo &info )
{
	return BaseClass::TryChangeTo( new CTFBotDead, RESULT_CRITICAL, "I died!" );
}

EventDesiredResult<CTFBot> CTFBotMainAction::OnOtherKilled( CTFBot *me, CBaseCombatCharacter *who, const CTakeDamageInfo &info )
{
	me->GetVisionInterface()->ForgetEntity( who );

	CTFPlayer *pPlayer = ToTFPlayer( who );
	if ( pPlayer )
	{
		me->ForgetSpy( pPlayer );

		if ( me->IsSelf( info.GetAttacker() ) && me->IsPlayerClass( TF_CLASS_SPY ) )
		{
			// Disguise as who we just killed for maximum espionage
			m_iDesiredDisguise = pPlayer->GetPlayerClass()->GetClassIndex();
		}

		if ( !pPlayer->IsBot() && me->IsEnemy( who ) )
		{
			const float flRand = RandomFloat( 0.0f, 100.0f );
			if ( flRand <= tf_bot_taunt_victim_chance.GetFloat() )
				return Action<CTFBot>::TrySuspendFor( new CTFBotTaunt, RESULT_IMPORTANT, "Taunting our victim" );
		}

		if ( me->IsFriend( who ) && me->IsLineOfSightClear( who->WorldSpaceCenter() ) )
		{
			CBaseEntity *pInflictor = info.GetInflictor();
			if ( pInflictor && dynamic_cast<CObjectSentrygun *>( pInflictor ) && !me->GetTargetSentry() )
			{
				me->NoteTargetSentry( pInflictor );
			}
		}
	}

	return Action<CTFBot>::TryContinue();
}


QueryResultType CTFBotMainAction::ShouldHurry( const INextBot *me ) const
{
	return ANSWER_UNDEFINED;
}

QueryResultType CTFBotMainAction::ShouldRetreat( const INextBot *me ) const
{
	CTFBot *actor = static_cast<CTFBot *>( me->GetEntity() );
	if ( actor->m_Shared.IsInvulnerable() ||
		( actor->IsPlayerClass( TF_CLASS_SPY ) &&
		 ( actor->m_Shared.InCond( TF_COND_DISGUISED ) ||
		   actor->m_Shared.InCond( TF_COND_DISGUISING ) ||
		   actor->m_Shared.IsStealthed() ) ) )
	{
		return ANSWER_NO;
	}

	if ( actor->m_Shared.IsControlStunned() || actor->m_Shared.IsLoser() )
		return ANSWER_YES;

	if ( TFGameRules()->InSetup() )
		return ANSWER_NO;

	if ( TheTFBots().IsMeleeOnly() )
		return ANSWER_YES;

	return ANSWER_UNDEFINED;
}

QueryResultType CTFBotMainAction::ShouldAttack( const INextBot *me, const CKnownEntity *threat ) const
{
	return ANSWER_YES;
}

QueryResultType CTFBotMainAction::IsPositionAllowed( const INextBot *me, const Vector &pos ) const
{
	return ANSWER_YES;
}


Vector CTFBotMainAction::SelectTargetPoint( const INextBot *me, const CBaseCombatCharacter *them ) const
{
	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	if ( them->IsBaseObject() && dynamic_cast<const CObjectSentrygun *>( them ) )
		return them->GetAbsOrigin() + them->GetViewOffset();

	CTFBot *actor = ToTFBot( me->GetEntity() );
	CTFWeaponBaseGun *pWeapon = (CTFWeaponBaseGun *)actor->GetActiveTFWeapon();
	if ( !pWeapon )
		return them->WorldSpaceCenter();

	if ( actor->m_iSkill != CTFBot::EASY )
	{
		// Try to aim for their feet for best damage
		if ( pWeapon->GetWeaponID() == TF_WEAPON_ROCKETLAUNCHER )
		{
			if ( them->GetAbsOrigin().z - 30.0f > actor->GetAbsOrigin().z )
			{
				if ( actor->GetVisionInterface()->IsAbleToSee( them->GetAbsOrigin(), IVision::DISREGARD_FOV ) )
					return them->GetAbsOrigin();

				if ( actor->GetVisionInterface()->IsAbleToSee( them->WorldSpaceCenter(), IVision::DISREGARD_FOV ) )
					return them->WorldSpaceCenter();
			}
			else
			{
				if ( !them->GetGroundEntity() )
				{
					trace_t trace;
					UTIL_TraceLine( them->GetAbsOrigin(), them->GetAbsOrigin() - Vector( 0, 0, 200.0f ), MASK_SOLID, them, COLLISION_GROUP_NONE, &trace );

					if ( trace.DidHit() )
						return trace.endpos;
				}

				float flDistance = actor->GetRangeTo( them->GetAbsOrigin() );
				if ( flDistance > 150.0f )
				{
					float flRangeMod = flDistance * 0.00090909092; // Investigate for constant
					Vector vecVelocity = them->GetAbsVelocity() * flRangeMod;

					if ( actor->GetVisionInterface()->IsAbleToSee( them->GetAbsOrigin() + vecVelocity, IVision::DISREGARD_FOV ) )
						return them->GetAbsOrigin() + vecVelocity;

					return them->EyePosition() + vecVelocity;
				}
			}

			return them->EyePosition();
		}

		if ( pWeapon->GetWeaponID() == TF_WEAPON_COMPOUND_BOW )
		{
			float flDistance = actor->GetRangeTo( them->GetAbsOrigin() );
			Vector vecTarget = vec3_origin;

			if ( flDistance <= 150.0f )
				vecTarget = them->EyePosition();
			else if ( actor->m_iSkill == CTFBot::NORMAL )
				vecTarget = them->WorldSpaceCenter();
			else
				vecTarget = them->EyePosition();

			float t = flDistance / pWeapon->GetProjectileSpeed();
			vecTarget += them->GetAbsVelocity() * t;

			float flElevationRate = Min( flDistance * tf_bot_arrow_elevation_rate.GetFloat(), 45.0f );

			float flSin, flCos;
			FastSinCos( flElevationRate * 0.0174532925199433, &flSin, &flCos );
			if ( flCos > 0.0f )
				return vecTarget + Vector( 0, 0, ( flDistance * flSin ) / flCos );

			return vecTarget;
		}
	}

	if ( pWeapon->IsWeapon( TF_WEAPON_GRENADELAUNCHER ) || pWeapon->IsWeapon( TF_WEAPON_PIPEBOMBLAUNCHER ) )
	{
		Vector vecToActor = them->GetAbsOrigin() - actor->GetAbsOrigin();
		float flLength = vecToActor.NormalizeInPlace();
		float flElevationRate = Min( flLength * tf_bot_ballistic_elevation_rate.GetFloat(), 45.0f );

		float flSin, flCos;
		FastSinCos( flElevationRate * 0.0174532925199433, &flSin, &flCos );
		if ( flCos > 0.0f )
			return them->WorldSpaceCenter() + Vector( 0, 0, ( flLength * flSin ) / flCos );

		return them->WorldSpaceCenter();
	}

	if ( WeaponID_IsSniperRifle( pWeapon->GetWeaponID() ) )
	{
		if ( m_sniperAimErrorTimer.IsElapsed() )
		{
			m_sniperAimErrorTimer.Start( RandomFloat( 0.5f, 1.5f ) );

			m_flSniperAimErrorRadius = RandomFloat( 0.0f, tf_bot_sniper_aim_error.GetFloat() );
			m_flSniperAimErrorAngle = RandomFloat( -M_PI_F, M_PI_F );
		}

		Vector vecToActor = them->GetAbsOrigin() - actor->GetAbsOrigin();


		float flErrorSin, flErrorCos;
		FastSinCos( m_flSniperAimErrorRadius, &flErrorSin, &flErrorCos );

		float flError = vecToActor.NormalizeInPlace() * flErrorSin;

		FastSinCos( m_flSniperAimErrorAngle, &flErrorSin, &flErrorCos );

		Vector vecTarget = vec3_origin;
		switch ( actor->m_iSkill )
		{
			case CTFBot::NORMAL:
			{
				vecTarget = ( them->EyePosition() + them->EyePosition() + them->WorldSpaceCenter() ) / 3;
				break;
			}
			case CTFBot::EASY:
			{
				vecTarget = them->WorldSpaceCenter();
				break;
			}
			case CTFBot::HARD:
			case CTFBot::EXPERT:
			{
				vecTarget = them->EyePosition();
				break;
			}
			default:
				break;
		}

		vecTarget.x += vecToActor.x * ( flErrorSin * flError );
		vecTarget.y += vecToActor.y * ( flErrorSin * flError );
		vecTarget.z += vecToActor.z * ( flError * flErrorCos );

		return vecTarget;
	}

	return them->WorldSpaceCenter();
}


const CKnownEntity *CTFBotMainAction::SelectMoreDangerousThreat( const INextBot *me, const CBaseCombatCharacter *them, const CKnownEntity *threat1, const CKnownEntity *threat2 ) const
{
	CTFBot *actor = static_cast<CTFBot *>( me->GetEntity() );

	const CKnownEntity *result = SelectMoreDangerousThreatInternal( me, them, threat1, threat2 );
	if ( actor->m_iSkill == CTFBot::EASY )
		return result;

	if ( actor->TransientlyConsistentRandomValue( 10.0f, 0 ) >= 0.5f || actor->m_iSkill >= CTFBot::HARD )
		return GetHealerOfThreat( result );

	return result;
}


void CTFBotMainAction::Dodge( CTFBot *actor )
{
	if ( actor->m_iSkill == CTFBot::EASY )
		return;

	if ( ( actor->m_nBotAttrs & CTFBot::AttributeType::DISABLEDODGE ) != 0 )
		return;

	if ( actor->m_Shared.IsInvulnerable() ||
		 actor->m_Shared.InCond( TF_COND_ZOOMED ) ||
		 actor->m_Shared.InCond( TF_COND_TAUNTING ) ||
		 !actor->IsCombatWeapon() )
	{
		return;
	}

	if ( actor->GetIntentionInterface()->ShouldHurry( actor ) == ANSWER_YES ||
		 actor->IsPlayerClass( TF_CLASS_ENGINEER ) ||
		 actor->m_Shared.InCond( TF_COND_DISGUISED ) ||
		 actor->m_Shared.InCond( TF_COND_DISGUISING ) ||
		 actor->m_Shared.IsStealthed() )
	{
		return;
	}

	const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat( false );
	if ( threat == nullptr || !threat->IsVisibleRecently() )
		return;

	CTFWeaponBase *weapon = static_cast<CTFWeaponBase *>( actor->Weapon_GetSlot( 0 ) );
	if ( weapon && weapon->IsWeapon( TF_WEAPON_COMPOUND_BOW ) )
	{
		CTFCompoundBow *huntsman = static_cast<CTFCompoundBow *>( weapon );
		if ( huntsman->GetCurrentCharge() != 0.0f )
		{
			return;
		}
	}
	else
	{
		if ( !actor->IsLineOfFireClear( threat->GetLastKnownPosition() ) )
		{
			return;
		}
	}

	Vector vecFwd;
	actor->EyeVectors( &vecFwd );

	Vector2D vecRight( -vecFwd.y, vecFwd.x );
	vecRight.NormalizeInPlace();

	int random = RandomInt( 0, 100 );
	if ( random < 33 )
	{
		const Vector strafe_left = actor->GetAbsOrigin() + Vector( 25.0f * vecRight.x, 25.0f * vecRight.y, 0.0f );
		if ( !actor->GetLocomotionInterface()->HasPotentialGap( actor->GetAbsOrigin(), strafe_left ) )
		{
			actor->PressLeftButton();
		}
	}
	else if ( random > 66 )
	{
		const Vector strafe_right = actor->GetAbsOrigin() - Vector( 25.0f * vecRight.x, 25.0f * vecRight.y, 0.0f );
		if ( !actor->GetLocomotionInterface()->HasPotentialGap( actor->GetAbsOrigin(), strafe_right ) )
		{
			actor->PressRightButton();
		}
	}
}

void CTFBotMainAction::FireWeaponAtEnemy( CTFBot *actor )
{
	if ( !actor->IsAlive() )
		return;

	if ( ( actor->m_nBotAttrs & (CTFBot::AttributeType::SUPPRESSFIRE|CTFBot::AttributeType::IGNOREENEMIES) ) != 0 )
		return;

	if ( !tf_bot_fire_weapon_allowed.GetBool() )
		return;

	if ( actor->m_Shared.InCond( TF_COND_HALLOWEEN_THRILLER ) )
		return;

	CTFWeaponBase *pWeapon = actor->GetActiveTFWeapon();
	if ( pWeapon == nullptr )
		return;

	if ( actor->IsPlayerClass( TF_CLASS_MEDIC ) && pWeapon->IsWeapon( TF_WEAPON_MEDIGUN ) )
		return;

	if ( actor->IsBarrageAndReloadWeapon() && tf_bot_always_full_reload.GetBool() )
	{
		if ( pWeapon->Clip1() <= 0 )
		{
			m_bReloadingBarrage = true;
		}

		if ( m_bReloadingBarrage )
		{
			if ( pWeapon->Clip1() < pWeapon->GetMaxClip1() )
				return;

			m_bReloadingBarrage = false;
		}
	}

	if ( actor->IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) && !actor->IsAmmoLow() )
	{
		if ( !actor->GetIntentionInterface()->ShouldHurry( actor ) )
		{
			if ( actor->GetVisionInterface()->GetTimeSinceVisible( GetEnemyTeam( actor ) ) < 3.0f )
			{
				actor->PressAltFireButton();
			}
		}
	}

	const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat();
	if ( threat == nullptr || threat->GetEntity() == nullptr || !threat->IsVisibleRecently() )
		return;

	if ( !actor->IsLineOfFireClear( threat->GetEntity()->EyePosition() ) &&
		 !actor->IsLineOfFireClear( threat->GetEntity()->WorldSpaceCenter() ) &&
		 !actor->IsLineOfFireClear( threat->GetEntity()->GetAbsOrigin() ) )
	{
		return;
	}

	if ( !actor->GetIntentionInterface()->ShouldAttack( actor, threat ) || TFGameRules()->InSetup() )
		return;

	if ( !actor->GetBodyInterface()->IsHeadAimingOnTarget() )
		return;

	if ( pWeapon->IsMeleeWeapon() )
	{
		if ( actor->IsRangeLessThan( threat->GetEntity(), 250.0f ) )
			actor->PressFireButton();

		return;
	}

	if ( pWeapon->IsWeapon( TF_WEAPON_FLAMETHROWER ) )
	{
		CTFFlameThrower *flamethrower = static_cast<CTFFlameThrower *>( pWeapon );
		if ( flamethrower->CanAirBlast() && actor->ShouldFireCompressionBlast() )
		{
			actor->PressAltFireButton();
			return;
		}

		if ( threat->GetTimeSinceLastSeen() < 1.0f )
		{
			const Vector vecToActor = ( actor->GetAbsOrigin() - threat->GetEntity()->GetAbsOrigin() );
			if ( vecToActor.IsLengthLessThan( actor->GetMaxAttackRange() ) )
				actor->PressFireButton( tf_bot_fire_weapon_min_time.GetFloat() );
		}

		return;
	}

	if ( pWeapon->IsWeapon( TF_WEAPON_COMPOUND_BOW ) )
	{
		CTFCompoundBow *huntsman = static_cast<CTFCompoundBow *>( pWeapon );
		if ( huntsman->GetCurrentCharge() >= 0.95f && actor->IsLineOfFireClear( threat->GetEntity() ) )
			return;

		actor->PressFireButton();
		return;
	}

	if ( WeaponID_IsSniperRifle( pWeapon->GetWeaponID() ) )
	{
		if ( !actor->m_Shared.InCond( TF_COND_ZOOMED ) )
			return;

		if ( !m_sniperSteadyInterval.HasStarted() || m_sniperSteadyInterval.IsLessThen( 0.1f ) )
			return;

		Vector vecFwd;
		actor->EyeVectors( &vecFwd );

		trace_t trace;
		UTIL_TraceLine( actor->EyePosition(),
						actor->EyePosition() + vecFwd * 9000.0f,
						MASK_SHOT, actor, COLLISION_GROUP_NONE, &trace );

		if ( trace.m_pEnt != threat->GetEntity() )
			return;

		actor->PressFireButton();
		return;
	}

	const Vector vecToThreat = ( threat->GetEntity()->GetAbsOrigin() - actor->GetAbsOrigin() );
	float flDistToThreat = vecToThreat.Length();
	if ( flDistToThreat >= actor->GetMaxAttackRange() )
		return;

	if ( actor->IsContinuousFireWeapon() )
	{
		actor->PressFireButton( tf_bot_fire_weapon_min_time.GetFloat() );
		return;
	}

	if ( actor->IsExplosiveProjectileWeapon() )
	{
		Vector vecFwd;
		actor->EyeVectors( &vecFwd );
		vecFwd.NormalizeInPlace();

		vecFwd *= ( 1.1f * flDistToThreat );

		trace_t trace;
		UTIL_TraceLine( actor->EyePosition(),
						actor->EyePosition() + vecFwd,
						MASK_SHOT,
						actor,
						COLLISION_GROUP_NONE,
						&trace );

		if ( ( trace.fraction * ( 1.1f * flDistToThreat ) ) < 146.0f && ( trace.m_pEnt == nullptr || !trace.m_pEnt->IsCombatCharacter() ) )
			return;
	}

	actor->PressFireButton();
}

const CKnownEntity *CTFBotMainAction::GetHealerOfThreat( const CKnownEntity *threat ) const
{
	if ( threat == nullptr || threat->GetEntity() == nullptr )
		return nullptr;

	CTFPlayer *pPlayer = ToTFPlayer( threat->GetEntity() );
	if ( pPlayer == nullptr )
		return threat;

	const CKnownEntity *knownHealer = threat;

	for ( int i = 0; i < pPlayer->m_Shared.GetNumHealers(); ++i )
	{
		CTFPlayer *pHealer = ToTFPlayer( pPlayer->m_Shared.GetHealerByIndex( i ) );
		if ( pHealer )
		{
			knownHealer = GetActor()->GetVisionInterface()->GetKnown( pHealer );

			if ( knownHealer && knownHealer->IsVisibleInFOVNow() )
				break;
		}
	}

	return knownHealer;
}

bool CTFBotMainAction::IsImmediateThreat( const CBaseCombatCharacter *who, const CKnownEntity *threat ) const
{
	CTFBot *actor = GetActor();
	if ( actor == nullptr )
		return false;

	if ( !actor->IsSelf( who ) )
		return false;

	if ( actor->InSameTeam( threat->GetEntity() ) )
		return false;

	if ( !threat->GetEntity()->IsAlive() )
		return false;

	if ( !threat->IsVisibleRecently() )
		return false;

	if ( !actor->IsLineOfFireClear( threat->GetEntity() ) )
		return false;

	CTFPlayer *pPlayer = ToTFPlayer( threat->GetEntity() );

	Vector vecToActor = actor->GetAbsOrigin() - threat->GetLastKnownPosition();
	float flDistance = vecToActor.Length();

	if ( flDistance < 500.0f )
		return true;

	if ( actor->IsThreatFiringAtMe( threat->GetEntity() ) )
		return true;

	vecToActor.NormalizeInPlace();

	Vector vecThreatFwd;

	if ( pPlayer )
	{
		if ( pPlayer->IsPlayerClass( TF_CLASS_SNIPER ) )
		{
			pPlayer->EyeVectors( &vecThreatFwd );
			return ( vecToActor.Dot( vecThreatFwd ) > 0.0f );
		}

		if ( actor->m_iSkill > CTFBot::NORMAL && pPlayer->IsPlayerClass( TF_CLASS_MEDIC ) )
			return true;

		if ( actor->m_iSkill > CTFBot::NORMAL && pPlayer->IsPlayerClass( TF_CLASS_ENGINEER ) )
			return true;
	}
	else
	{
		CObjectSentrygun *pSentry = dynamic_cast<CObjectSentrygun *>( threat->GetEntity() );
		if ( pSentry && !pSentry->HasSapper() && !pSentry->IsPlacing() && flDistance < SENTRYGUN_BASE_RANGE * 1.5f ) // slightly larger than radius in case of manual aiming
		{
			AngleVectors( pSentry->GetTurretAngles(), &vecThreatFwd );
			return ( vecToActor.Dot( vecThreatFwd ) > 0.8f );
		}
	}

	return false;
}

const CKnownEntity *CTFBotMainAction::SelectCloserThreat( CTFBot *actor, const CKnownEntity *threat1, const CKnownEntity *threat2 ) const
{
	if ( actor->GetRangeSquaredTo( threat1->GetEntity() ) < actor->GetRangeSquaredTo( threat2->GetEntity() ) )
		return threat1;
	else
		return threat2;
}

const CKnownEntity *SelectClosestSpyToMe( CTFBot *actor, const CKnownEntity *known1, const CKnownEntity *known2 )
{
	CTFPlayer *spy1 = ToTFPlayer( known1->GetEntity() );
	CTFPlayer *spy2 = ToTFPlayer( known2->GetEntity() );

	bool bValid1 = ( spy1 && spy1->IsPlayerClass( TF_CLASS_SPY ) );
	bool bValid2 = ( spy2 && spy2->IsPlayerClass( TF_CLASS_SPY ) );

	if ( bValid1 && bValid2 )
	{
		if ( actor->GetRangeSquaredTo( spy1 ) > actor->GetRangeSquaredTo( spy2 ) )
		{
			return known2;
		}
		else
		{
			return known1;
		}
	}

	if ( bValid1 )
	{
		return known1;
	}
	if ( bValid2 )
	{
		return known2;
	}

	return nullptr;
}

const CKnownEntity *CTFBotMainAction::SelectMoreDangerousThreatInternal( const INextBot *me, const CBaseCombatCharacter *them, const CKnownEntity *threat1, const CKnownEntity *threat2 ) const
{
	CTFBot *actor = static_cast<CTFBot *>( me->GetEntity() );

	const CKnownEntity *closer = this->SelectCloserThreat( actor, threat1, threat2 );

	CObjectSentrygun *sentry1 = nullptr;
	if ( threat1->IsVisibleRecently() && !threat1->GetEntity()->IsPlayer() )
	{
		sentry1 = dynamic_cast<CObjectSentrygun *>( threat1->GetEntity() );
	}
	CObjectSentrygun *sentry2 = nullptr;
	if ( threat2->IsVisibleRecently() && !threat2->GetEntity()->IsPlayer() )
	{
		sentry2 = dynamic_cast<CObjectSentrygun *>( threat2->GetEntity() );
	}

	bool sentry1_danger =
		( sentry1 && actor->IsRangeLessThan( sentry1, SENTRYGUN_BASE_RANGE ) &&
		  !sentry1->HasSapper() && !sentry1->IsPlacing() );
	bool sentry2_danger =
		( sentry2 && actor->IsRangeLessThan( sentry2, SENTRYGUN_BASE_RANGE ) &&
		  !sentry2->HasSapper() && !sentry2->IsPlacing() );

	if ( sentry1_danger && sentry2_danger )
	{
		return closer;
	}
	else if ( sentry1_danger )
	{
		return threat1;
	}
	else if ( sentry2_danger )
	{
		return threat2;
	}

	bool imm1 = IsImmediateThreat( them, threat1 );
	bool imm2 = IsImmediateThreat( them, threat2 );

	if ( imm1 && imm2 )
	{
		const CKnownEntity *spy = SelectClosestSpyToMe( actor, threat1, threat2 );
		if ( spy != nullptr )
		{
			return spy;
		}

		bool firing1 = actor->IsThreatFiringAtMe( threat1->GetEntity() );
		bool firing2 = actor->IsThreatFiringAtMe( threat2->GetEntity() );

		if ( firing1 && firing2 )
		{
			return closer;
		}
		else if ( firing1 )
		{
			return threat1;
		}
		else if ( firing2 )
		{
			return threat2;
		}
		else
		{
			return closer;
		}
	}
	else if ( imm1 )
	{
		return threat1;
	}
	else if ( imm2 )
	{
		return threat2;
	}
	else
	{
		return closer;
	}
}
