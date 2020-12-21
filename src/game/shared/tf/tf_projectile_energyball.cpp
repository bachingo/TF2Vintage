//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: Energy Ball (Particle Cannon)
//
//=============================================================================//
#include "cbase.h"
#include "tf_projectile_energyball.h"
#include "tf_gamerules.h"
#include "tf_shareddefs.h"

#ifdef GAME_DLL
#include "soundent.h"
#include "te_effect_dispatch.h"
#include "tf_fx.h"
#include "iscorer.h"
#include "tf_obj.h"
#endif
//=============================================================================
//
// Energy Ball Projectile Table.
//
#define ENERGYBALL_MODEL "models/weapons/w_models/w_drg_ball.mdl"

LINK_ENTITY_TO_CLASS( tf_projectile_energy_ball, CTFProjectile_EnergyBall );
PRECACHE_REGISTER( tf_projectile_energy_ball );

IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_EnergyBall, DT_TFProjectile_EnergyBall )

BEGIN_NETWORK_TABLE( CTFProjectile_EnergyBall, DT_TFProjectile_EnergyBall )
#ifdef CLIENT_DLL
	RecvPropBool( RECVINFO( m_bCritical ) ),
	RecvPropBool( RECVINFO( m_bChargedShot ) ),
	RecvPropVector( RECVINFO( m_vColor1 ) ),
	RecvPropVector( RECVINFO( m_vColor2 ) )
#else
	SendPropBool( SENDINFO( m_bCritical ) ),
	SendPropBool( SENDINFO( m_bChargedShot ) ),
	SendPropVector( SENDINFO( m_vColor1 ), 8, 0, 0, 1 ),
	SendPropVector( SENDINFO( m_vColor2 ), 8, 0, 0, 1 )
#endif
END_NETWORK_TABLE()

extern ConVar tf_rocket_show_radius;

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_EnergyBall::Precache()
{
	PrecacheModel( ENERGYBALL_MODEL );
	PrecacheScriptSound( "Weapon_CowMangler.Explode" );
	
	PrecacheParticleSystem( "drg_cow_rockettrail_charged" );
	PrecacheParticleSystem( "drg_cow_rockettrail_charged_blue" );
	PrecacheParticleSystem( "drg_cow_rockettrail_normal" );
	PrecacheParticleSystem( "drg_cow_rockettrail_normal_blue" );


	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFProjectile_EnergyBall *CTFProjectile_EnergyBall::Create( CBaseEntity *pWeapon, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, CBaseEntity *pScorer )
{
	CTFProjectile_EnergyBall *pEnergyBall = static_cast<CTFProjectile_EnergyBall*>( CTFBaseRocket::Create( pWeapon, "tf_projectile_energy_ball", vecOrigin, vecAngles, pOwner ) );

	if ( pEnergyBall )
	{
		pEnergyBall->SetScorer( pScorer );
	}

	return pEnergyBall;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CBasePlayer *CTFProjectile_EnergyBall::GetScorer( void )
{
	return assert_cast<CBasePlayer *>( m_hScorer.Get() );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_EnergyBall::SetScorer(CBaseEntity *pScorer)
{
	m_hScorer = pScorer;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_EnergyBall::Spawn()
{
	UseClientSideAnimation();
	SetModel( ENERGYBALL_MODEL );
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CTFProjectile_EnergyBall::GetDamage( void )
{
	return m_flDamage;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CTFProjectile_EnergyBall::GetRadius( void )
{
	float flRadius = BaseClass::GetRadius();
	// Increase radius on charged shots.
	if  ( IsChargedShot() )
		flRadius *= 1.3;
	return flRadius;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFProjectile_EnergyBall::GetDamageType() 
{ 
	int iDmgType = BaseClass::GetDamageType();
	
	// Charged shots ignite targets.
	if ( IsChargedShot() )
		iDmgType |= DMG_IGNITE;

	return iDmgType;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFProjectile_EnergyBall::GetDamageCustom()
{
	return m_bChargedShot ? TF_DMG_CUSTOM_PLASMA_CHARGED : TF_DMG_CUSTOM_PLASMA;
}

void CTFProjectile_EnergyBall::Deflected( CBaseEntity *pDeflectedBy, Vector &vecDir )
{
	CTFPlayer *pDeflector = ToTFPlayer( pDeflectedBy );
	if ( !pDeflector )
		return;

	ChangeTeam( pDeflector->GetTeamNumber() );
	SetLauncher( pDeflector->GetActiveWeapon() );

	CTFPlayer* pOldOwner = ToTFPlayer( GetOwnerEntity() );
	SetOwnerEntity( pDeflector );

	if ( pOldOwner )
	{
		pOldOwner->SpeakConceptIfAllowed( MP_CONCEPT_DEFLECTED, "projectile:1,victim:1" );
	}

	if ( pDeflector->m_Shared.IsCritBoosted() )
	{
		SetCritical( true );
	}

	IncremenentDeflected();
	SetScorer( pDeflector );

	// Change particle color data.
	if ( GetTeamNumber() == TF_TEAM_BLUE )
	{
		m_vColor1 = Vector( 0.345, 0.52, 0.635 );
		m_vColor2 = Vector( 0.145, 0.427, 0.55 );
	}
	else
	{
		m_vColor1 = Vector( 0.72, 0.22, 0.23 );
		m_vColor2 = Vector( 0.5, 0.18, 0.125 );
	}

	if ( pDeflector && pOldOwner )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "object_deflected" );
		if ( event )
		{
			event->SetInt( "userid", pDeflector->GetUserID() );
			event->SetInt( "ownerid", pOldOwner->GetUserID() );
			event->SetInt( "weaponid", GetWeaponID() );
			event->SetInt( "object_entindex", entindex() );

			gameeventmanager->FireEvent( event );
		}
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_EnergyBall::Explode( trace_t *pTrace, CBaseEntity *pOther )
{
#ifdef GAME_DLL
	// Save this entity as enemy, they will take 100% damage.
	m_hEnemy = pOther;

	// Invisible.
	SetModelName( NULL_STRING );
	AddSolidFlags( FSOLID_NOT_SOLID );
	m_takedamage = DAMAGE_NO;

	// Figure out Econ ID.
	int iItemID = -1;
	if ( m_hLauncher.Get() )
	{
		CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase *>( m_hLauncher.Get() );
		if ( pWeapon )
		{
			iItemID = pWeapon->GetItemID();
		}
	}

	// Pull out a bit.
	if ( pTrace->fraction != 1.0 )
	{
		SetAbsOrigin( pTrace->endpos + ( pTrace->plane.normal * 1.0f ) );
	}

	// Play explosion sound and effect.
	Vector vecOrigin = GetAbsOrigin();
	CPVSFilter filter( vecOrigin );
	
	QAngle angExplosion( 0.f, 0.f, 0.f );
	VectorAngles( pTrace->plane.normal, angExplosion );
	TE_TFParticleEffect( filter, 0.f, GetExplosionParticleName(), vecOrigin, pTrace->plane.normal, angExplosion, NULL );

	// Screenshake
	if ( m_bChargedShot )
	{
		UTIL_ScreenShake( WorldSpaceCenter(), 25.0f, 150.0f, 1.0f, 750.0f, SHAKE_START );
	}

	EmitSound("Weapon_CowMangler.Explode");
	CSoundEnt::InsertSound( SOUND_COMBAT, vecOrigin, 1024, 3.0 );

	// Damage.
	CBaseEntity *pAttacker = GetOwnerEntity();
	IScorer *pScorerInterface = dynamic_cast<IScorer*>( pAttacker );
	if ( pScorerInterface )
	{
		pAttacker = pScorerInterface->GetScorer();
	}

	const float flRadius = GetRadius();

	if( pAttacker )
	{
		const float flSelfRadius = TF_ROCKET_SELF_DAMAGE_RADIUS * ( (m_bChargedShot) ? 1.33f : 1.0f );
		CTakeDamageInfo newInfo( this, pAttacker, m_hLauncher, vec3_origin, vecOrigin, GetDamage(), GetDamageType(), GetDamageCustom() );
		CTFRadiusDamageInfo radiusInfo( &newInfo, vecOrigin, flRadius, NULL, flSelfRadius );
		TFGameRules()->RadiusDamage( radiusInfo );

		// If we directly hit an enemy building, EMP it.
		CBaseObject *pBuilding = dynamic_cast<CBaseObject *>( pOther );
		if ( IsChargedShot() && pBuilding && ( pBuilding->GetTeamNumber() != pAttacker->GetTeamNumber() ) )
		{
			pBuilding->OnEMP();
		}
	}

	// Debug!
	if ( tf_rocket_show_radius.GetBool() )
	{
		DrawRadius( flRadius );
	}

	// Don't decal players with scorch.
	if ( !pOther->IsPlayer() )
	{
		UTIL_DecalTrace( pTrace, "Scorch" );
	}

	// Remove the rocket.
	UTIL_Remove( this );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFProjectile_EnergyBall::GetExplosionParticleName( void )
{
	if ( m_bChargedShot )
	{
		return ( GetTeamNumber() == TF_TEAM_RED ) ? "drg_cow_explosioncore_charged" : "drg_cow_explosioncore_charged_blue";
	}
	else
	{
		return ( GetTeamNumber() == TF_TEAM_RED ) ? "drg_cow_explosioncore_normal" : "drg_cow_explosioncore_normal_blue";
	}
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *C_TFProjectile_EnergyBall::GetTrailParticleName( void )
{
	if ( m_bChargedShot )
	{
		return ( GetTeamNumber() == TF_TEAM_RED ) ? "drg_cow_rockettrail_charged" : "drg_cow_rockettrail_charged_blue";
	}
	else
	{
		return ( GetTeamNumber() == TF_TEAM_RED ) ? "drg_cow_rockettrail_normal" : "drg_cow_rockettrail_normal_blue";
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFProjectile_EnergyBall::CreateTrails( void )
{
	if ( IsDormant() )
		return;

	if ( m_hEffect )
		ParticleProp()->StopEmission( m_hEffect );

	ParticleProp()->Init( this );
	m_hEffect = ParticleProp()->Create( GetTrailParticleName(), PATTACH_ABSORIGIN_FOLLOW );

	if ( m_hEffect != NULL )
	{
		if ( m_iDeflected != m_iOldDeflected )
		{
			if ( GetTeamNumber() == TF_TEAM_BLUE )
			{
				m_hEffect->SetControlPoint( CUSTOM_COLOR_CP1, Vector( 0.345, 0.52, 0.635 ) );
				m_hEffect->SetControlPoint( CUSTOM_COLOR_CP2, Vector( 0.145, 0.427, 0.55 ) );
			}
			else
			{
				m_hEffect->SetControlPoint( CUSTOM_COLOR_CP1, Vector( 0.72, 0.22, 0.23 ) );
				m_hEffect->SetControlPoint( CUSTOM_COLOR_CP2, Vector( 0.5, 0.18, 0.125 ) );
			}
		}
		else
		{
			m_hEffect->SetControlPoint( CUSTOM_COLOR_CP1, m_vColor1 );
			m_hEffect->SetControlPoint( CUSTOM_COLOR_CP2, m_vColor2 );
		}
	}
}
#endif
