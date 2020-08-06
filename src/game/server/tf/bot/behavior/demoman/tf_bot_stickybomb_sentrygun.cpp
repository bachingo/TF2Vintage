#include "cbase.h"
#include "../../tf_bot.h"
#include "tf_obj_sentrygun.h"
#include "tf_weapon_pipebomblauncher.h"
#include "nav_mesh/tf_nav_mesh.h"
#include "tf_bot_stickybomb_sentrygun.h"


ConVar tf_bot_sticky_base_range( "tf_bot_sticky_base_range", "800", FCVAR_CHEAT );
ConVar tf_bot_sticky_charge_rate( "tf_bot_sticky_charge_rate", "0.01", FCVAR_CHEAT, "Seconds of charge per unit range beyond base" );

class DetonatePipebombsReply : public INextBotReply
{
	virtual void OnSuccess( INextBot *bot )
	{
		CTFBot *actor = ToTFBot( bot->GetEntity() );
		if ( actor->GetActiveWeapon() != actor->Weapon_GetSlot( 1 ) )
			actor->Weapon_Switch( actor->Weapon_GetSlot( 1 ) );

		actor->PressAltFireButton();
	}
};
static DetonatePipebombsReply detReply;


CTFBotStickybombSentrygun::CTFBotStickybombSentrygun( CObjectSentrygun *pSentry )
{
	m_hSentry = pSentry;
	m_bOpportunistic = false;
}

CTFBotStickybombSentrygun::CTFBotStickybombSentrygun( CObjectSentrygun *pSentry, float flPitch, float flYaw, float flChargePerc )
{
	m_hSentry = pSentry;
	m_bOpportunistic = true;
	m_flDesiredPitch = flPitch;
	m_flDesiredYaw = flYaw;
	m_flDesiredCharge = flChargePerc;
}


const char *CTFBotStickybombSentrygun::GetName() const
{
	return "StickybombSentrygun";
}


ActionResult<CTFBot> CTFBotStickybombSentrygun::OnStart( CTFBot *me, Action<CTFBot> *priorAction )
{
	me->PressAltFireButton();
	me->m_bLookingAroundForEnemies = false;
	me->SetAbsVelocity( vec3_origin );

	m_bReload = true;
	m_bAimOnTarget = false;
	m_bChargeShot = false;

	m_aimDuration.Start( 3.0f );

	if ( m_bOpportunistic )
	{
		m_bAimOnTarget = true;
		m_vecHome = me->GetAbsOrigin();
		m_bChargeShot = true;
		m_flChargeLevel = m_flDesiredCharge;

		const QAngle angDir( m_flDesiredPitch, m_flDesiredYaw, 0.0f );
		Vector vecDir = vec3_invalid;
		AngleVectors( angDir, &vecDir );

		m_vecAimTarget = me->EyePosition() + vecDir * ( TF_PIPEBOMB_MAX_CHARGE_VEL - TF_PIPEBOMB_MIN_CHARGE_VEL );
	}

	return BaseClass::Continue();
}

ActionResult<CTFBot> CTFBotStickybombSentrygun::Update( CTFBot *me, float dt )
{
	CTFWeaponBase *pActive = me->m_Shared.GetActiveTFWeapon();
	CTFPipebombLauncher *pLauncher = dynamic_cast<CTFPipebombLauncher *>( me->Weapon_GetSlot( 1 ) );
	if ( pLauncher == nullptr || pActive == nullptr )
		return BaseClass::Done( "Missing weapon" );

	if ( pActive->GetWeaponID() != TF_WEAPON_PIPEBOMBLAUNCHER )
		me->Weapon_Switch( pLauncher );

	if ( m_hSentry == nullptr || !m_hSentry->IsAlive() )
		return BaseClass::Done( "Sentry destroyed" );

	if ( !m_bAimOnTarget && m_aimDuration.IsElapsed() )
		return BaseClass::Done( "Can't find aim" );

	if ( m_bReload )
	{
		const int iClip = Min( me->GetAmmoCount( TF_AMMO_SECONDARY ), pLauncher->GetMaxClip1() );
		if ( pLauncher->Clip1() >= iClip )
		{
			m_bReload = false;
		}

		me->PressReloadButton();

		return BaseClass::Continue();
	}

	if ( pLauncher->GetPipeBombCount() >= 3 || me->GetAmmoCount( TF_AMMO_SECONDARY ) <= 0 )
	{
		FOR_EACH_VEC( pLauncher->m_Pipebombs, i )
		{
			if ( !pLauncher->m_Pipebombs[i]->Touched() )
				return BaseClass::Continue();
		}

		me->GetBodyInterface()->AimHeadTowards( m_hSentry, IBody::IMPORTANT, 0.5f, &detReply, "Looking toward stickies to detonate" );

		if ( me->GetAmmoCount( TF_AMMO_SECONDARY ) <= 0 )
			return BaseClass::Done( "Out of ammo" );

		return BaseClass::Continue();
	}

	if ( m_bChargeShot )
	{
		const float flChargeTime = 4.4f * m_flChargeLevel;

		me->GetBodyInterface()->AimHeadTowards( m_vecAimTarget, IBody::CRITICAL, 0.3f, nullptr, "Aiming a sticky bomb at a sentrygun" );

		if ( gpGlobals->curtime - pLauncher->GetChargeBeginTime() < flChargeTime )
		{
			me->PressFireButton();
		}
		else
		{
			me->ReleaseFireButton();
			m_bChargeShot = false;
		}

		return BaseClass::Continue();
	}

	if ( gpGlobals->curtime <= pLauncher->m_flNextPrimaryAttack )
		return BaseClass::Continue();

	if ( m_bAimOnTarget )
	{
		if ( me->IsRangeGreaterThan( m_vecHome, 1.0f ) )
		{
			m_bAimOnTarget = false;
			m_aimDuration.Reset();
		}

		if ( m_bAimOnTarget )
		{
			m_vecHome = me->GetAbsOrigin();
			me->PressFireButton();

			return BaseClass::Continue();
		}
	}

	QAngle angAim;
	Vector vecToSentry = me->EyePosition() - m_hSentry->WorldSpaceCenter();
	VectorAngles( vecToSentry, angAim );

	float flPitch = 0.0, flYaw = 0.0, flCharge = 1.0;
	for ( int i=100; i; --i )
	{
		float flYawVariance = angAim.y + RandomFloat( -30, 30 );
		float flRandomPitch = RandomFloat( -85, 85 );
		float flDesiredCharge = 0;

		if ( vecToSentry.LengthSqr() > Square( tf_bot_sticky_base_range.GetFloat() ) )
			flDesiredCharge = Square( RandomFloat( 0.1f, 1.0f ) );

		if ( IsAimOnTarget( me, flRandomPitch, flYawVariance, flDesiredCharge ) && flCharge > flDesiredCharge )
		{
			m_flChargeLevel = flDesiredCharge;
			m_bAimOnTarget = true;

			if ( flDesiredCharge < 0.01 )
			{
				flPitch = flRandomPitch;
				flYaw = flYawVariance;
				break;
			}

			flPitch = flRandomPitch;
			flYaw = flYawVariance;
			flCharge = flDesiredCharge;
		}
	}

	Vector vecDir;
	angAim = QAngle( flPitch, flYaw, 0.0f );
	AngleVectors( angAim, &vecDir );

	m_vecAimTarget = me->EyePosition() + vecDir * 500.0f;

	me->GetBodyInterface()->AimHeadTowards( m_vecAimTarget, IBody::CRITICAL, 0.3f, nullptr, "Searching for aim..." );

	if ( m_bAimOnTarget )
	{
		m_vecHome = me->GetAbsOrigin();
		me->PressFireButton();
		m_bChargeShot = true;
	}

	return BaseClass::Continue();
}

void CTFBotStickybombSentrygun::OnEnd( CTFBot *me, Action<CTFBot> *newAction )
{
	me->PressAltFireButton();

	me->m_bLookingAroundForEnemies = true;
}

ActionResult<CTFBot> CTFBotStickybombSentrygun::OnSuspend( CTFBot *me, Action<CTFBot> *newAction )
{
	me->PressAltFireButton();

	return BaseClass::Done();
}


EventDesiredResult<CTFBot> CTFBotStickybombSentrygun::OnInjured( CTFBot *me, const CTakeDamageInfo& info )
{
	return BaseClass::TryDone( RESULT_IMPORTANT, "Ouch!" );
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


bool CTFBotStickybombSentrygun::IsAimOnTarget( CTFBot *actor, float pitch, float yaw, float charge )
{
	Vector est = actor->EstimateStickybombProjectileImpactPosition( pitch, yaw, charge );

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
