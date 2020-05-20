//====== Copyright © 1996-2013, Valve Corporation, All rights reserved. ========//
//
// Purpose: Flare used by the flaregun.
//
//=============================================================================//
#include "cbase.h"
#include "tf_projectile_energy_ring.h"
// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "particles_new.h"
#include "iefx.h"
#include "dlight.h"
#include "tempent.h"
#include "c_te_legacytempents.h"
#else
#include "tf_player.h"
#include "tf_fx.h"
#include "effect_dispatch_data.h"
#include "collisionutils.h"
#include "tf_team.h"
#include "props.h"
#endif

#ifdef CLIENT_DLL
	extern ConVar tf2v_muzzlelight;
#endif

#define TF_WEAPON_ENERGYRING_MODEL	"models/empty.mdl"

ConVar tf2v_use_new_bison_damage( "tf2v_use_new_bison_damage", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Changes Bison's damage mechanics.", true, 0, true, 2 );
ConVar tf2v_use_new_bison_speed( "tf2v_use_new_bison_speed", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Decreases Bison speed by 30%." );

//=============================================================================
//
// Dragon's Fury Projectile
//

BEGIN_DATADESC( CTFProjectile_EnergyRing )
END_DATADESC()

LINK_ENTITY_TO_CLASS( tf_projectile_energy_ring, CTFProjectile_EnergyRing );
PRECACHE_REGISTER( tf_projectile_energy_ring );

IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_EnergyRing, DT_TFProjectile_EnergyRing )
BEGIN_NETWORK_TABLE( CTFProjectile_EnergyRing, DT_TFProjectile_EnergyRing )
#ifdef GAME_DLL
	SendPropBool( SENDINFO( m_bCritical ) ),
#else
	RecvPropBool( RECVINFO( m_bCritical ) ),
#endif
END_NETWORK_TABLE()

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFProjectile_EnergyRing::CTFProjectile_EnergyRing()
{
#ifdef CLIENT_DLL
	m_pRing = NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFProjectile_EnergyRing::~CTFProjectile_EnergyRing()
{
#ifdef CLIENT_DLL
	ParticleProp()->StopEmissionAndDestroyImmediately( m_pRing );
	m_pRing = NULL;
#else
	m_bCollideWithTeammates = false;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Is used for differentiating between Bison (true) and Pomson (false) shots.
//-----------------------------------------------------------------------------
bool CTFProjectile_EnergyRing::UsePenetratingBeam()
{
	CTFWeaponBase *pWeapon = (CTFWeaponBase *)m_hLauncher.Get();
	if (pWeapon)
		return pWeapon->IsPenetrating();

	return true;
}



#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_EnergyRing::Precache()
{
	PrecacheModel( TF_WEAPON_ENERGYRING_MODEL );

	PrecacheParticleSystem( "drg_bison_projectile" );
	PrecacheParticleSystem( "drg_bison_projectile_crit" );
	PrecacheParticleSystem( "drg_bison_impact" );

	PrecacheScriptSound( "Weapon_Bison.ProjectileImpactWorld" );
	PrecacheScriptSound( "Weapon_Bison.ProjectileImpactFlesh" );

	PrecacheParticleSystem("drg_pomson_projectile");
	PrecacheParticleSystem("drg_pomson_projectile_crit");
	PrecacheParticleSystem("drg_pomson_impact");
	PrecacheParticleSystem("drg_pomson_impact_drain");

	PrecacheScriptSound("Weapon_Pomson.DrainedVictim");
	PrecacheScriptSound("Weapon_Pomson.ProjectileImpactWorld");
	PrecacheScriptSound("Weapon_Pomson.ProjectileImpactFlesh");

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Spawn function
//-----------------------------------------------------------------------------
void CTFProjectile_EnergyRing::Spawn()
{
	SetModel( TF_WEAPON_ENERGYRING_MODEL );
	BaseClass::Spawn();


	float flRadius = 0.01f;
	UTIL_SetSize( this, -Vector( flRadius, flRadius, flRadius ), Vector( flRadius, flRadius, flRadius ) );
	m_nPenetratedCount = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_EnergyRing::RocketTouch( CBaseEntity *pOther )
{
	if ( pOther->m_takedamage != DAMAGE_NO )
	{
		
		bool bShouldDamage = true;
		// Bison with mid era selection does not double dip damage.
		if ( UsePenetratingBeam() && tf2v_use_new_bison_damage.GetInt() == 1 )
		{
			// Check the players hit.
			FOR_EACH_VEC( hPenetratedPlayers, i )
			{
				// Is our victim, don't damage them.
				if (hPenetratedPlayers[i] && hPenetratedPlayers[i] == pOther)
				{
					bShouldDamage = false;
					break;
				}
			}
		}
		
		if (bShouldDamage)
		{
			// Damage.
			CBaseEntity *pAttacker = GetOwnerEntity();
			IScorer *pScorerInterface = dynamic_cast<IScorer*>( pAttacker );
			if ( pScorerInterface )
				pAttacker = pScorerInterface->GetScorer();

			float flDamage;
			if ( UsePenetratingBeam() )
			{
				// Damage done with Bison beams depends on era.
				switch (tf2v_use_new_bison_damage.GetInt())
				{
					case 1:
						// This goes down 25% for each thing we hit. (100%, 75%, 50%, 25%, 0%.)
						// On the upside, this does have a base 45 damage.
						flDamage = 45 - (0.25 * m_nPenetratedCount) * 45;
						break;
					
					// These are solid 20 damage, but they can do damage multiple times.
					case 0:
					case 2:
					default:
						flDamage = 20;
						break;
				}
			}
			else // Just call our damage, nothing interesting.
				flDamage = 60;
			
			int iDamageType = GetDamageType();

			CTFWeaponBase *pWeapon = ( CTFWeaponBase * )m_hLauncher.Get();

			CTakeDamageInfo info( GetOwnerEntity(), pAttacker, pWeapon, flDamage, iDamageType | DMG_PREVENT_PHYSICS_FORCE, TF_DMG_CUSTOM_PLASMA );
			pOther->TakeDamage( info );

			info.SetReportedPosition( pAttacker->GetAbsOrigin() );	

			// We collided with pOther, so try to find a place on their surface to show blood
			trace_t pTrace;
			UTIL_TraceLine(WorldSpaceCenter(), pOther->WorldSpaceCenter(), /*MASK_SOLID*/ MASK_SHOT | CONTENTS_HITBOX, this, COLLISION_GROUP_DEBRIS, &pTrace);

			pOther->DispatchTraceAttack( info, GetAbsVelocity(), &pTrace );

			ApplyMultiDamage();
			
			if (UsePenetratingBeam())	
			{
				// Save this entity so we don't double dip damage on it.
				hPenetratedPlayers.AddToTail(pOther);
				m_nPenetratedCount++;
			}
		}

		// Non Penetrating: Delete the beam on the first thing we hit.
		if (!UsePenetratingBeam())
			UTIL_Remove(this);
		
		// Penetrating: Delete the beam after hitting the 4th target, when on mid era.
		if ( m_nPenetratedCount >= 4 && tf2v_use_new_bison_damage.GetInt() == 1 )		
			UTIL_Remove(this);
	}
	else
	{
		UTIL_Remove( this );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_EnergyRing::SetScorer( CBaseEntity *pScorer )
{
	m_Scorer = pScorer;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CBasePlayer *CTFProjectile_EnergyRing::GetScorer( void )
{
	return dynamic_cast<CBasePlayer *>( m_Scorer.Get() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_EnergyRing::Explode( trace_t *pTrace, CBaseEntity *pOther )
{
	// Save this entity as enemy, they will take 100% damage.
	m_hEnemy = pOther;

	// Invisible.
	SetModelName( NULL_STRING );
	AddSolidFlags( FSOLID_NOT_SOLID );
	m_takedamage = DAMAGE_NO;

	// Pull out a bit.
	if ( pTrace->fraction != 1.0 )
		SetAbsOrigin( pTrace->endpos + ( pTrace->plane.normal * 1.0f ) );

	// Remove.
	UTIL_Remove( this );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFProjectile_EnergyRing *CTFProjectile_EnergyRing::Create( CBaseEntity *pWeapon, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, CBaseEntity *pScorer )
{
	CTFProjectile_EnergyRing * pRing = static_cast<CTFProjectile_EnergyRing*>( CBaseEntity::CreateNoSpawn( "tf_projectile_energy_ring", vecOrigin, vecAngles, pOwner ) );
	if ( pRing )
	{
		// Set team.
		pRing->ChangeTeam( pOwner->GetTeamNumber() );

		// Set scorer.
		pRing->SetScorer( pScorer );

		// Set firing weapon.
		pRing->SetLauncher( pWeapon );

		// Spawn.
		DispatchSpawn(pRing);

		// Setup the initial velocity.
		Vector vecForward, vecRight, vecUp;
		AngleVectors( vecAngles, &vecForward, &vecRight, &vecUp );


		float flVelocity = 1200.0f;
		if (pRing->UsePenetratingBeam() && tf2v_use_new_bison_speed.GetBool()) // New Bison speed is much slower.
			flVelocity *= 0.7;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pWeapon, flVelocity, mult_projectile_speed );

		Vector vecVelocity = vecForward * flVelocity;
		pRing->SetAbsVelocity( vecVelocity );
		pRing->SetupInitialTransmittedGrenadeVelocity( vecVelocity );

		// Setup the initial angles.
		QAngle angles;
		VectorAngles( vecVelocity, angles );
		pRing->SetAbsAngles( angles );
		
		float flGravity = 0.0f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pWeapon, flGravity, mod_rocket_gravity );
		if ( flGravity )
		{
			pRing->SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_CUSTOM );
			pRing->SetGravity( flGravity );
		}

		return pRing;
	}

	return pRing;
}
#else

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_EnergyRing::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		CreateTrails();
		CreateLightEffects();
	}

	// Watch team changes and change trail accordingly.
	if ( m_iOldTeamNum && m_iOldTeamNum != m_iTeamNum )
	{
		ParticleProp()->StopEmission();
		CreateTrails();
		CreateLightEffects();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_EnergyRing::CreateTrails( void )
{
	if ( IsDormant() )
		return;

	const char *pszEffectName;
	if (UsePenetratingBeam())
		pszEffectName = m_bCritical ? "drg_bison_projectile_crit" : "drg_bison_projectile";
	else
		pszEffectName = m_bCritical ? "drg_pomson_projectile_crit" : "drg_pomson_projectile";

	m_pRing = ParticleProp()->Create( pszEffectName, PATTACH_ABSORIGIN_FOLLOW );

	Vector vecColor;

	switch ( GetTeamNumber() )
	{
		case TF_TEAM_RED:
			vecColor.Init( 255, -255, -255 );
			break;
		case TF_TEAM_BLUE:
			vecColor.Init( -255, -255, 255 );
			break;
	}

	m_pRing->SetControlPoint( 2, vecColor );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_EnergyRing::CreateLightEffects( void )
{
	// Handle the dynamic light
	if (tf2v_muzzlelight.GetBool())
	{
		AddEffects( EF_DIMLIGHT );

		dlight_t *dl;
		if ( IsEffectActive( EF_DIMLIGHT ) )
		{	
			dl = effects->CL_AllocDlight( LIGHT_INDEX_TE_DYNAMIC + index );
			dl->origin = GetAbsOrigin();
			switch ( GetTeamNumber() )
			{
			case TF_TEAM_RED:
				if ( !m_bCritical )
				{	dl->color.r = 255; dl->color.g = 30; dl->color.b = 10; }
				else
				{	dl->color.r = 255; dl->color.g = 10; dl->color.b = 10; }
				break;

			case TF_TEAM_BLUE:
				if ( !m_bCritical )
				{	dl->color.r = 10; dl->color.g = 30; dl->color.b = 255; }
				else
				{	dl->color.r = 10; dl->color.g = 10; dl->color.b = 255; }
				break;
			}
			dl->radius = 256.0f;
			dl->die = gpGlobals->curtime + 0.1;

			tempents->RocketFlare( GetAbsOrigin() );
		}
	}
}
#endif