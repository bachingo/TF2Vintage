//====== Copyright © 1996-2013, Valve Corporation, All rights reserved. ========//
//
// Purpose: Flare used by the flaregun.
//
//=============================================================================//
#include "cbase.h"
#include "tf_projectile_flare.h"
#include "tf_gamerules.h"
#include "tf_weapon_flaregun.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "particles_new.h"
#else
#include "tf_player.h"
#include "tf_fx.h"
#endif

#define TF_WEAPON_FLARE_MODEL		"models/weapons/w_models/w_flaregun_shell.mdl"
#define TF_FLARE_GRAVITY			0.3f

BEGIN_DATADESC( CTFProjectile_Flare )
END_DATADESC()

LINK_ENTITY_TO_CLASS( tf_projectile_flare, CTFProjectile_Flare );
PRECACHE_REGISTER( tf_projectile_flare );

IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_Flare, DT_TFProjectile_Flare )
BEGIN_NETWORK_TABLE( CTFProjectile_Flare, DT_TFProjectile_Flare )
#ifdef GAME_DLL
	SendPropBool( SENDINFO( m_bCritical ) ),
#else
	RecvPropBool( RECVINFO( m_bCritical ) ),
#endif
END_NETWORK_TABLE()

extern ConVar tf2v_minicrits_on_deflect;

ConVar tf2v_use_new_flare_radius("tf2v_use_new_flare_radius", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Uses modern flare explosion sizes (110Hu)instead of older ones (92Hu) for exploding flares.");

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFProjectile_Flare::CTFProjectile_Flare()
{
#ifdef GAME_DLL
	m_bTauntShot = false;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFProjectile_Flare::~CTFProjectile_Flare()
{
#ifdef CLIENT_DLL
	ParticleProp()->StopEmission();
#else
	m_bCollideWithTeammates = false;
#endif
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_Flare::Precache()
{
	PrecacheModel( TF_WEAPON_FLARE_MODEL );

	PrecacheTeamParticles( "flaregun_trail_%s", true );
	PrecacheTeamParticles( "flaregun_trail_crit_%s", true );
	
	PrecacheParticleSystem("drg_manmelter_projectile");
	
	PrecacheTeamParticles( "scorchshot_trail__%s", true );
	PrecacheTeamParticles( "scorchshot_trail__crit_%s", true );

	PrecacheScriptSound( "TFPlayer.FlareImpact" );
	BaseClass::Precache();
}



//-----------------------------------------------------------------------------
// Purpose: Spawn function
//-----------------------------------------------------------------------------
void CTFProjectile_Flare::Spawn()
{
	SetModel( TF_WEAPON_FLARE_MODEL );
	BaseClass::Spawn();
	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_CUSTOM );
	
	CTFWeaponBase *pLauncher = dynamic_cast<CTFWeaponBase*>(m_hLauncher.Get());
	if (pLauncher && pLauncher->IsEnergyWeapon()) // Energy beam
		SetGravity( 0.0f );	
	else								 		  // Regular flare
		SetGravity( TF_FLARE_GRAVITY);
		
	// If we made this while taunting, record it as a taunt shot.
	CTFPlayer *pScorer = ToTFPlayer( GetScorer() );
	if ( pScorer && pScorer->IsTaunting() )
	{
		m_bTauntShot = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_Flare::SetScorer( CBaseEntity *pScorer )
{
	m_Scorer = pScorer;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CBasePlayer *CTFProjectile_Flare::GetScorer( void )
{
	return dynamic_cast<CBasePlayer *>( m_Scorer.Get() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFProjectile_Flare::GetDamageType() 
{ 
	int iDmgType = BaseClass::GetDamageType();
	

	// Buff banner mini-crit calculations
	CTFWeaponBase *pWeapon = ( CTFWeaponBase * )m_hLauncher.Get();
	if ( pWeapon )
	{
		pWeapon->CalcIsAttackMiniCritical();
		if ( pWeapon->IsCurrentAttackAMiniCrit() )
		{
			iDmgType |= DMG_MINICRITICAL;
		}
		
	}
	
	if ( m_bCritical )
	{
		iDmgType |= DMG_CRITICAL;
	}
	if ( ( m_iDeflected > 0 ) && ( tf2v_minicrits_on_deflect.GetBool() ) )
	{
		iDmgType |= DMG_MINICRITICAL;
	}

	return iDmgType;
}

bool CTFProjectile_Flare::IsDeflectable(void)
{
	// Don't deflect projectiles with non-deflect attributes.
	if (m_hLauncher.Get())
	{
		// Check to see if this is a non-deflectable projectile, like an energy projectile.
		int nCannotDeflect = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(m_hLauncher.Get(), nCannotDeflect, energy_weapon_no_deflect);
		if (nCannotDeflect != 0)
			return false;
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_Flare::Deflected( CBaseEntity *pDeflectedBy, Vector &vecDir )
{
	// Get rocket's speed.
	float flVel = GetAbsVelocity().Length();

	QAngle angForward;
	VectorAngles( vecDir, angForward );

	// Now change rocket's direction.
	SetAbsAngles( angForward );
	SetAbsVelocity( vecDir * flVel );

	// And change owner.
	IncremenentDeflected();
	SetOwnerEntity( pDeflectedBy );
	ChangeTeam( pDeflectedBy->GetTeamNumber() );
	SetScorer( pDeflectedBy );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_Flare::Explode( trace_t *pTrace, CBaseEntity *pOther )
{

	// Save this entity as enemy, they will take 100% damage.
	m_hEnemy = pOther;

	// Check the flare mode.
	int nFlareMode = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER(m_hLauncher.Get(), nFlareMode, set_weapon_mode);

	// Pull out a bit.
	if ( pTrace->fraction != 1.0 )
	{
		SetAbsOrigin( pTrace->endpos + ( pTrace->plane.normal * 1.0f ) );
	}

	// Damage.
	CBaseEntity *pAttacker = GetOwnerEntity();
	IScorer *pScorerInterface = dynamic_cast<IScorer*>( pAttacker );
	if ( pScorerInterface )
	{
		pAttacker = pScorerInterface->GetScorer();
	}

	// Play explosion sound and effect.
	Vector vecOrigin = GetAbsOrigin();
	CTFPlayer *pPlayer = ToTFPlayer( pOther );

	Vector vectorReported = pAttacker ? pAttacker->GetAbsOrigin() : vec3_origin;
	
	// Invisible.
	if ( nFlareMode != 3 || ( nFlareMode == 3 && !pPlayer ) )
	{
		SetModelName( NULL_STRING );
		AddSolidFlags( FSOLID_NOT_SOLID );
		m_takedamage = DAMAGE_NO;
	}

	if ( pPlayer )
	{
		// Hit player, do impact sound
		CPVSFilter filter( vecOrigin );
		
		if ( nFlareMode != 3 )
			EmitSound( filter, pPlayer->entindex(), "TFPlayer.FlareImpact" );
		else
		{
			// Scorch shot does knockback damage, and loses all of its speed on impact.

			// Make sure only our knockback force blasts them backward.
			int iDamageType = m_bTauntShot ? 400 : GetDamageType();	// Taunt fired hits are immensely more powerful.
			iDamageType |= DMG_PREVENT_PHYSICS_FORCE;
			
			// We deal with direct contact, do the regular logic.		
			CTakeDamageInfo info( this, pAttacker, m_hLauncher.Get(), GetDamage(), iDamageType, TF_DMG_CUSTOM_FLARE_PELLET );
			info.SetReportedPosition( vectorReported);
			pOther->TakeDamage( info );
			
			// Set up knockback.
			// Faster projectile = more knockback.
			Vector vecToVictim = GetAbsVelocity();
			vecToVictim.NormalizeInPlace();
			vecToVictim.z = 1.0;
			
			// Burning players get more knockback.
			int iForce = 100;
			if (pPlayer->m_Shared.InCond(TF_COND_BURNING))
				iForce *= 2;
			
			Vector vecVelocityImpulse = vecToVictim;
			pPlayer->ApplyAirBlastImpulse( vecVelocityImpulse * iForce );
			pPlayer->m_Shared.StunPlayer(0.5f, 1.0f, 1.0f, TF_STUNFLAG_LIMITMOVEMENT | TF_STUNFLAG_NOSOUNDOREFFECT , ToTFPlayer(pAttacker));
			
			// Stop all of our momentum forwards and backwards.
			vecToVictim.x *= 0.05;
			vecToVictim.y *= 0.05;
			vecToVictim.z = 5.00;

			// Point the new direction and randomly flip
			QAngle angForward;
			VectorAngles( vecToVictim, angForward );
			SetAbsAngles( angForward );

			QAngle angVel = RandomAngle( 0, 360 );
			angVel.x *= RandomFloat(-2,2);
			angVel.y *= RandomFloat(-2,2);
			angVel.z *= RandomFloat(-2,2);
			SetLocalAngularVelocity( angVel );


			// Play the impact sound.
			EmitSound( filter, pPlayer->entindex(), "Rubber.BulletImpact" );
			// Return to wait to hit another object.
			return;
		}
	}
	else
	{
		// Hit world, do the explosion effect.
		CPVSFilter filter( vecOrigin );
		// Pick our explosion effect. Manual Detonator and Scorch shot explode.
		TE_TFExplosion( filter, 0.0f, vecOrigin, pTrace->plane.normal, GetWeaponID(), pOther->entindex() );
	}
	
	// Scorch Shot and Detonator explode when touching the world, or if Detonator was triggered.
	if ( ( ( nFlareMode == 1 || nFlareMode == 3 ) && ( !pPlayer ) ) )
	{
		// We explode in a small radius, set us up as an explosion.
		CTakeDamageInfo newInfo( this, pAttacker, m_hLauncher.Get(), vec3_origin, vecOrigin, GetDamage(), GetDamageType(), TF_DMG_CUSTOM_BURNING, &vectorReported );
		CTFRadiusDamageInfo radiusInfo;
		radiusInfo.info = &newInfo;
		radiusInfo.m_vecSrc = vecOrigin;
		
		// Check the radius.
		float flRadius = GetFlareRadius();
		
		if (nFlareMode == 3)
			radiusInfo.m_flRadius = flRadius;	// Scorch Shot has full radius damage.
		else
			radiusInfo.m_flRadius = 0;	// ...Detonator gets no radius for damaging anyone except ourselves.
		
		radiusInfo.m_flSelfDamageRadius = flRadius;

		TFGameRules()->RadiusDamage( radiusInfo );
	}
	else
	{
		// We deal with direct contact, do the regular logic.
		CTakeDamageInfo info( this, pAttacker, m_hLauncher.Get(), GetDamage(), GetDamageType(), TF_DMG_CUSTOM_BURNING );
		info.SetReportedPosition( vectorReported);
		pOther->TakeDamage( info );
	}

	// Remove.
	UTIL_Remove( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_Flare::Airburst(trace_t *pTrace, CBaseEntity *pOther)
{

	// Invisible.
	SetModelName(NULL_STRING);
	AddSolidFlags(FSOLID_NOT_SOLID);
	m_takedamage = DAMAGE_NO;

	// Pull out a bit.
	if (pTrace->fraction != 1.0)
	{
		SetAbsOrigin(pTrace->endpos + (pTrace->plane.normal * 1.0f));
	}

	// Damage.
	CBaseEntity *pAttacker = GetOwnerEntity();
	IScorer *pScorerInterface = dynamic_cast<IScorer*>(pAttacker);
	if (pScorerInterface)
	{
		pAttacker = pScorerInterface->GetScorer();
	}

	// Play explosion sound and effect.
	Vector vecOrigin = GetAbsOrigin();

	// Hit world, do the explosion effect.
	CPVSFilter filter(vecOrigin);
	// Pick our explosion effect.
	TE_TFExplosion(filter, 0.0f, vecOrigin, pTrace->plane.normal, TF_WEAPON_ROCKETLAUNCHER, pOther->entindex());

	Vector vectorReported = pAttacker ? pAttacker->GetAbsOrigin() : vec3_origin;

	// We explode in a small radius, set us up as an explosion.
	CTakeDamageInfo newInfo(this, pAttacker, m_hLauncher.Get(), vec3_origin, vecOrigin, GetDamage(), GetDamageType(), TF_DMG_CUSTOM_BURNING, &vectorReported);
	CTFRadiusDamageInfo radiusInfo;
	radiusInfo.info = &newInfo;
	radiusInfo.m_vecSrc = vecOrigin;

	// Check the radius.
	float flRadius = GetFlareRadius();
	radiusInfo.m_flRadius = flRadius;

	radiusInfo.m_flSelfDamageRadius = flRadius;

	TFGameRules()->RadiusDamage(radiusInfo);

	// Remove.
	UTIL_Remove(this);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFProjectile_Flare::GetFlareRadius( void )
{
	float flRadius = 92.0f;
	if ( tf2v_use_new_flare_radius.GetBool() )
		flRadius = 110.0;
	
	CALL_ATTRIB_HOOK_FLOAT(flRadius, mult_explosion_radius);
	
	return flRadius;

}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFProjectile_Flare *CTFProjectile_Flare::Create( CBaseEntity *pWeapon, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, CBaseEntity *pScorer )
{
	CTFProjectile_Flare *pFlare = static_cast<CTFProjectile_Flare*>( CBaseEntity::CreateNoSpawn( "tf_projectile_flare", vecOrigin, vecAngles, pOwner ) );

	if ( pFlare )
	{
		// Set team.
		pFlare->ChangeTeam( pOwner->GetTeamNumber() );

		// Set scorer.
		pFlare->SetScorer( pScorer );

		// Set firing weapon.
		pFlare->SetLauncher( pWeapon );

		// Spawn.
		DispatchSpawn( pFlare );

		// Setup the initial velocity.
		Vector vecForward, vecRight, vecUp;
		AngleVectors( vecAngles, &vecForward, &vecRight, &vecUp );

		float flVelocity = 2000.0f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pWeapon, flVelocity, mult_projectile_speed );

		Vector vecVelocity = vecForward * flVelocity;
		pFlare->SetAbsVelocity( vecVelocity );
		pFlare->SetupInitialTransmittedGrenadeVelocity( vecVelocity );

		// Setup the initial angles.
		QAngle angles;
		VectorAngles( vecVelocity, angles );
		pFlare->SetAbsAngles( angles );
		return pFlare;
	}

	return pFlare;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_Flare::Detonate( void )
{
	trace_t		tr;
	Vector		vecSpot;// trace starts here!

	SetThink( NULL );

	vecSpot = GetAbsOrigin() + Vector ( 0 , 0 , 8 );
	UTIL_TraceLine ( vecSpot, vecSpot + Vector ( 0, 0, -32 ), MASK_SHOT_HULL, this, COLLISION_GROUP_NONE, & tr);

	Airburst( &tr, NULL );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_Flare::UpdateOnRemove( void )
{
	// Tell our launcher that we were removed
	CTFFlareGun *pFlareGun = dynamic_cast<CTFFlareGun*>( m_hLauncher.Get() );

	if ( pFlareGun )
	{
		pFlareGun->DeathNotice( this );
	}

	BaseClass::UpdateOnRemove();
}


#else

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_Flare::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );
		
	if ( updateType == DATA_UPDATE_CREATED )
	{
		CTFFlareGun *pLauncher = dynamic_cast<CTFFlareGun*>( m_hLauncher.Get() );

		if ( pLauncher )
		{
			pLauncher->AddFlare( this );
		}
	
		CreateTrails();		
	}

	// Watch team changes and change trail accordingly.
	if ( m_iOldTeamNum && m_iOldTeamNum != m_iTeamNum )
	{
		ParticleProp()->StopEmission();
		CreateTrails();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_Flare::CreateTrails( void )
{
	if ( IsDormant() )
		return;

	const char *pszFormat = nullptr;
	const char *pszEffectName = nullptr;
	CTFWeaponBase *pLauncher = dynamic_cast<CTFWeaponBase*>(m_hLauncher.Get());
	
	int nWeaponMode = 0;
	if (pLauncher)
		CALL_ATTRIB_HOOK_INT_ON_OTHER(pLauncher, nWeaponMode, set_weapon_mode);
	
	if (pLauncher && pLauncher->IsEnergyWeapon()) // Energy beam
		pszFormat = "drg_manmelter_projectile";	
	else if (nWeaponMode == 3) // Scorch Shot
	{
		pszFormat = m_bCritical ? "scorchshot_trail_crit_%s" : "scorchshot_trail__%s";
		pszEffectName = ConstructTeamParticle( pszFormat, GetTeamNumber(), false );		
	}
	else // Standard flare
	{		
		pszFormat = m_bCritical ? "flaregun_trail_crit_%s" : "flaregun_trail_%s";
		pszEffectName = ConstructTeamParticle( pszFormat, GetTeamNumber(), false );
	}

	ParticleProp()->Create( pszEffectName, PATTACH_ABSORIGIN_FOLLOW );
}

#endif