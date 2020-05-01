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

LINK_ENTITY_TO_CLASS( tf_projectile_energy_ball, CTFEnergyBall );
PRECACHE_REGISTER( tf_projectile_energy_ball );

IMPLEMENT_NETWORKCLASS_ALIASED( TFEnergyBall, DT_TFEnergyBall )

BEGIN_NETWORK_TABLE( CTFEnergyBall, DT_TFEnergyBall )
#ifdef CLIENT_DLL
	RecvPropBool(RECVINFO(m_iDeflected)),
	RecvPropBool(RECVINFO(m_bChargedBeam)),
#else
	SendPropBool(SENDINFO(m_bCritical)),
	SendPropBool(SENDINFO(m_bChargedBeam)),
#endif
END_NETWORK_TABLE()

extern ConVar tf_rocket_show_radius;

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFEnergyBall::Precache()
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
CTFEnergyBall *CTFEnergyBall::Create( CBaseEntity *pWeapon, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, CBaseEntity *pScorer )
{
	CTFEnergyBall *pEnergyBall = static_cast<CTFEnergyBall*>( CTFBaseRocket::Create( pWeapon, "tf_projectile_energy_ball", vecOrigin, vecAngles, pOwner ) );

	if ( pEnergyBall )
	{
		pEnergyBall->SetScorer( pScorer );
	}

	return pEnergyBall;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CBasePlayer *CTFEnergyBall::GetScorer( void )
{
	if (dynamic_cast<CBasePlayer *>( m_Scorer.Get() ))
		return dynamic_cast<CBasePlayer *>( m_Scorer.Get() );

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFEnergyBall::SetScorer(CBaseEntity *pScorer)
{
	m_Scorer = pScorer;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFEnergyBall::Spawn()
{
	UseClientSideAnimation();
	SetModel( ENERGYBALL_MODEL );
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CTFEnergyBall::GetRadius( void )
{
	float flRadius = BaseClass::GetRadius();
	// Increase radius on charged shots.
	if  ( ShotIsCharged() )
		flRadius *= 1.3;
	return flRadius;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFEnergyBall::GetDamageType() 
{ 
	int iDmgType = BaseClass::GetDamageType();
	
	// Charged shots ignite targets.
	if (ShotIsCharged() )
		iDmgType |= DMG_IGNITE;

	return iDmgType;
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFEnergyBall::Explode( trace_t *pTrace, CBaseEntity *pOther )
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
	
	EmitSound("Weapon_CowMangler.Explode");
	TE_TFExplosion( filter, 0.0f, vecOrigin, pTrace->plane.normal, GetWeaponID(), pOther->entindex(), iItemID );

	// Damage.
	CBaseEntity *pAttacker = GetOwnerEntity();
	IScorer *pScorerInterface = dynamic_cast<IScorer*>( pAttacker );
	if ( pScorerInterface )
	{
		pAttacker = pScorerInterface->GetScorer();
	}

	float flRadius = GetRadius();

	CTakeDamageInfo newInfo( this, pAttacker, m_hLauncher, vec3_origin, vecOrigin, GetDamage(), GetDamageType() );
	CTFRadiusDamageInfo radiusInfo;
	radiusInfo.info = &newInfo;
	radiusInfo.m_vecSrc = vecOrigin;
	radiusInfo.m_flRadius = flRadius;
	radiusInfo.m_flSelfDamageRadius = flRadius * TF_ROCKET_SELF_RADIUS_RATIO; // Original rocket radius?

	TFGameRules()->RadiusDamage( radiusInfo );
	
	// If we directly hit an enemy building, EMP it.
	CBaseObject *pBuilding = dynamic_cast<CBaseObject *>( pOther );
	if (ShotIsCharged() && ( pBuilding && ( pBuilding->GetTeamNumber() != pAttacker->GetTeamNumber() ) ) )
	{
		pBuilding->OnEMP();
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

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFEnergyBall::CreateRocketTrails( void )
{
	if ( IsDormant() )
		return;

	int iAttachment = LookupAttachment( "trail" );
	if( iAttachment > -1 )
	{
		if (m_bCritical || ShotIsCharged())
		{
			if (m_hLauncher.Get() && m_hLauncher.Get()->GetTeamNumber() == TF_TEAM_BLUE)
			{
				ParticleProp()->Create("drg_cow_rockettrail_normal_blue", PATTACH_POINT_FOLLOW, iAttachment);
			}
			else
			{
				ParticleProp()->Create("drg_cow_rockettrail_charged", PATTACH_POINT_FOLLOW, iAttachment);
			}
		}
		else
		{
			if (m_hLauncher.Get() && m_hLauncher.Get()->GetTeamNumber() == TF_TEAM_BLUE)
			{
				ParticleProp()->Create("drg_cow_rockettrail_normal_blue", PATTACH_POINT_FOLLOW, iAttachment);
			}
			else
			{
				ParticleProp()->Create("drg_cow_rockettrail_normal", PATTACH_POINT_FOLLOW, iAttachment);
			}
		}
	}
}
#endif
