#include "cbase.h"
#include "../../tf_bot.h"
#include "tf_obj_sentrygun.h"
#include "tf_weapon_pipebomblauncher.h"
#include "nav_mesh/tf_nav_mesh.h"
#include "tf_bot_stickybomb_sentrygun.h"


ConVar tf_bot_sticky_base_range( "tf_bot_sticky_base_range", "800", FCVAR_CHEAT );
ConVar tf_bot_sticky_charge_rate( "tf_bot_sticky_charge_rate", "0.01", FCVAR_CHEAT, "Seconds of charge per unit range beyond base" );


CTFBotStickybombSentrygun::CTFBotStickybombSentrygun( CObjectSentrygun *sentry )
{
	m_hSentry = sentry;
	m_bOpportunistic = false;
}

CTFBotStickybombSentrygun::CTFBotStickybombSentrygun( CObjectSentrygun *sentry, const Vector& vec )
{
	m_hSentry = sentry;
	m_bOpportunistic = true;

	// 0034 = (x, y, z)
}

CTFBotStickybombSentrygun::~CTFBotStickybombSentrygun()
{
}


const char *CTFBotStickybombSentrygun::GetName() const
{
	return "StickybombSentrygun";
}


ActionResult<CTFBot> CTFBotStickybombSentrygun::OnStart( CTFBot *me, Action<CTFBot> *priorAction )
{
	me->PressAltFireButton();

	me->m_bLookingAroundForEnemies = false;

	m_bReload = true;

	me->SetAbsVelocity( vec3_origin );

	m_aimDuration.Start( 3.0f );

	if ( m_bOpportunistic )
	{
// 0058 = true

// 0068 = actor->GetAbsOrigin();

// 0048 = true

// 0074 = 0034 .z

// TODO: AngleVectors, 1500, EyePosition

// 005c = ...
	}

	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotStickybombSentrygun::Update( CTFBot *me, float dt )
{
	CTFWeaponBase *active = me->m_Shared.GetActiveTFWeapon();
	auto launcher = dynamic_cast<CTFPipebombLauncher *>( me->Weapon_GetSlot( 1 ) );
	if ( launcher == nullptr || active == nullptr )
		return Action<CTFBot>::Done( "Missing weapon" );

	if ( active->GetWeaponID() != TF_WEAPON_PIPEBOMBLAUNCHER )
		me->Weapon_Switch( launcher );

	if ( m_hSentry == nullptr || !m_hSentry->IsAlive() )
		return Action<CTFBot>::Done( "Sentry destroyed" );

	if ( !false/* 0058 */ && m_aimDuration.IsElapsed() )
		return Action<CTFBot>::Done( "Can't find aim" );

	if ( m_bReload )
	{
		int clip_size;
		if ( me->GetAmmoCount( TF_AMMO_SECONDARY ) < launcher->GetMaxClip1() )
		{
			clip_size = me->GetAmmoCount( TF_AMMO_SECONDARY );
		}
		else
		{
			clip_size = launcher->GetMaxClip1();
		}

		if ( launcher->Clip1() >= clip_size )
		{
			m_bReload = false;
		}

		me->PressReloadButton();

		return Action<CTFBot>::Continue();
	}

	if ( launcher->GetPipeBombCount() >= 3 || me->GetAmmoCount( TF_AMMO_SECONDARY ) <= 0 )
	{
// TODO
// stuff related to launcher->m_Pipebombs
// pipebomb+0x4f9: m_bTouched

		me->PressAltFireButton();

		if ( me->GetAmmoCount( TF_AMMO_SECONDARY ) <= 0 )
			return Action<CTFBot>::Done( "Out of ammo" );

		return Action<CTFBot>::Continue();
	}

	if ( false/* 0048 */ )
	{
		const float charge_time = 4.4f * m_flChargeLevel;

		me->GetBodyInterface()->AimHeadTowards( m_vecAimTarget, IBody::CRITICAL, 0.3f, nullptr, "Aiming a sticky bomb at a sentrygun" );

		if ( gpGlobals->curtime - launcher->GetChargeBeginTime() < charge_time )
		{
			me->PressFireButton();
		}
		else
		{
			me->ReleaseFireButton();
			// 0048 = false
		}

		return Action<CTFBot>::Continue();
	}

	if ( gpGlobals->curtime <= launcher->m_flNextPrimaryAttack )
		return Action<CTFBot>::Continue();

	if ( false/* 0058 */ )
	{
		if ( me->IsRangeGreaterThan( vec3_origin/* 0068 */, 1.0f ) )
		{
			// 0058 = false
			m_aimDuration.Reset();
		}

		if ( false/* 0058 */ )
		{
			// TODO: goto LABEL_56
		}
	}

	// TODO: EyePosition and onward
	// ...
	// TODO

	if ( false/* 0058 */ )
	{
		// TODO
	}

	return Action<CTFBot>::Continue();
}

void CTFBotStickybombSentrygun::OnEnd( CTFBot *me, Action<CTFBot> *newAction )
{
	me->PressAltFireButton();

	me->m_bLookingAroundForEnemies = true;
}

ActionResult<CTFBot> CTFBotStickybombSentrygun::OnSuspend( CTFBot *me, Action<CTFBot> *newAction )
{
	me->PressAltFireButton();

	return Action<CTFBot>::Done();
}


EventDesiredResult<CTFBot> CTFBotStickybombSentrygun::OnInjured( CTFBot *me, const CTakeDamageInfo& info )
{
	return Action<CTFBot>::TryDone( RESULT_IMPORTANT, "Ouch!" );
}


QueryResultType CTFBotStickybombSentrygun::ShouldHurry( const INextBot *me ) const
{
	return ANSWER_YES;
}

QueryResultType CTFBotStickybombSentrygun::ShouldRetreat( const INextBot *me ) const
{
	return ANSWER_NO;
}

QueryResultType CTFBotStickybombSentrygun::ShouldAttack( const INextBot *me, const CKnownEntity *threat ) const
{
	return ANSWER_NO;
}


bool CTFBotStickybombSentrygun::IsAimOnTarget( CTFBot *actor, float pitch, float yaw, float speed )
{
	Vector est = actor->EstimateStickybombProjectileImpactPosition( pitch, yaw, speed );

	if ( ( this->m_hSentry->WorldSpaceCenter() - est ).LengthSqr() < Square( 75.0f ) )
	{
		trace_t tr;
		UTIL_TraceLine( this->m_hSentry->WorldSpaceCenter(), est,
						MASK_SOLID_BRUSHONLY, nullptr, 0, &tr );

		if ( !tr.DidHit() )
			return true;
	}

	return false;
}
