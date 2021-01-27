//====== Copyright © 1996-2013, Valve Corporation, All rights reserved. ========//
//
// Purpose: Flare used by the flaregun.
//
//=============================================================================//
#include "cbase.h"
#include "tf_projectile_dragons_fury.h"
#include "tf_weapon_compound_bow.h"
#include "tf_projectile_arrow.h"
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

#ifdef GAME_DLL
ConVar  tf_fireball_burn_duration("tf_fireball_burn_duration", "2", FCVAR_CHEAT | FCVAR_REPLICATED, "" );
ConVar  tf_fireball_burning_bonus("tf_fireball_burning_bonus", "3", FCVAR_CHEAT | FCVAR_REPLICATED, "" );
ConVar  tf_fireball_damage("tf_fireball_damage", "25.0", FCVAR_CHEAT | FCVAR_REPLICATED, "" );
ConVar  tf_fireball_distance("tf_fireball_distance", "500", FCVAR_CHEAT | FCVAR_REPLICATED, "" );
ConVar  tf_fireball_draw_debug_radius("tf_fireball_draw_debug_radius", "0", FCVAR_CHEAT | FCVAR_REPLICATED, "" );
ConVar  tf_fireball_radius("tf_fireball_radius", "22.5", FCVAR_CHEAT | FCVAR_REPLICATED, "" );
ConVar  tf_fireball_speed("tf_fireball_speed", "3000", FCVAR_CHEAT | FCVAR_REPLICATED, "" );
#endif

#define TF_WEAPON_FIREBALL_MODEL	"models/empty.mdl"

//=============================================================================
//
// Dragon's Fury Projectile
//

BEGIN_DATADESC( CTFProjectile_BallOfFire )
END_DATADESC()

LINK_ENTITY_TO_CLASS( tf_projectile_balloffire, CTFProjectile_BallOfFire );
PRECACHE_REGISTER( tf_projectile_balloffire );

IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_BallOfFire, DT_TFProjectile_BallOfFire )
BEGIN_NETWORK_TABLE( CTFProjectile_BallOfFire, DT_TFProjectile_BallOfFire )
#ifdef GAME_DLL
	SendPropBool( SENDINFO( m_bCritical ) ),
#else
	RecvPropBool( RECVINFO( m_bCritical ) ),
#endif
END_NETWORK_TABLE()

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFProjectile_BallOfFire::CTFProjectile_BallOfFire()
{

}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFProjectile_BallOfFire::~CTFProjectile_BallOfFire()
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
void CTFProjectile_BallOfFire::Precache()
{
	PrecacheModel( TF_WEAPON_FIREBALL_MODEL );

	PrecacheParticleSystem( "projectile_fireball" );
	PrecacheParticleSystem( "projectile_fireball_pyrovision" );
	PrecacheParticleSystem( "rockettrail_waterbubbles" );
	PrecacheTeamParticles( "projectile_fireball_crit_%s" );

	PrecacheScriptSound( "Weapon_DragonsFury.Impact" );
	PrecacheScriptSound( "Weapon_DragonsFury.BonusDamageHit" );
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Spawn function
//-----------------------------------------------------------------------------
void CTFProjectile_BallOfFire::Spawn()
{
	SetModel( TF_WEAPON_FIREBALL_MODEL );
	BaseClass::Spawn();
	SetMoveType( MOVETYPE_FLY, MOVECOLLIDE_FLY_CUSTOM );
	AddEFlags( EFL_NO_WATER_VELOCITY_CHANGE );
	SetGravity( 0.0f );

	SetSolid( SOLID_NONE );
	SetSolidFlags( FSOLID_NOT_SOLID );
	SetCollisionGroup( COLLISION_GROUP_NONE );

	//float flRadius = GetDamageRadius();
	float flRadius = GetFireballScale();
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( GetOwnerEntity(), flRadius, mult_flame_size );
	UTIL_SetSize( this, -Vector( flRadius, flRadius, flRadius ), Vector( flRadius, flRadius, flRadius ) );

	// Setup attributes.
	m_takedamage = DAMAGE_NO;
	m_vecInitialPos = GetOwnerEntity()->GetAbsOrigin();
	m_vecPrevPos = m_vecInitialPos;

	// Setup the think function.
	SetThink( &CTFProjectile_BallOfFire::ExpireDelayThink );
	SetNextThink( gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
// Purpose: Think method
//-----------------------------------------------------------------------------
void CTFProjectile_BallOfFire::ExpireDelayThink( void )
{
	// Render debug visualization if convar on
	if ( tf_fireball_draw_debug_radius.GetBool() )
	{
		NDebugOverlay::Sphere( GetAbsOrigin(), GetAbsAngles(), GetDamageRadius(), 0, 255, 0, 0, false, 0 );
	}

	SetNextThink( gpGlobals->curtime );

	m_vecPrevPos = GetAbsOrigin();

 	CBaseEntity *pEntity = NULL;
	Vector vecOrigin = GetAbsOrigin();
	CBaseEntity *pBaseAttacker = GetOwnerEntity();
	IScorer *pScorerInterface = dynamic_cast<IScorer*>( pBaseAttacker );
	if ( pScorerInterface )
		pBaseAttacker = pScorerInterface->GetScorer();

	CTFPlayer *pAttacker = dynamic_cast<CTFPlayer *>( pBaseAttacker );
	if ( !pAttacker )
		return;

 	for ( CEntitySphereQuery sphere( vecOrigin, GetDamageRadius() ); ( pEntity = sphere.GetCurrentEntity() ) != NULL; sphere.NextEntity() )
	{
		if ( !pEntity )
			continue;

		// if we've already burnt this entity, don't do more damage, so skip even checking for collision with the entity
		int iIndex = m_hEntitiesBurnt.Find( pEntity );
		if ( iIndex != m_hEntitiesBurnt.InvalidIndex() )
			continue;

 		Vector vecHitPoint;
		pEntity->CollisionProp()->CalcNearestPoint( vecOrigin, &vecHitPoint );
		Vector vecDir = vecHitPoint - vecOrigin;
 		if ( vecDir.LengthSqr() < ( GetDamageRadius() * GetDamageRadius() ) )
		{
			if ( pEntity && !pEntity->InSameTeam( pAttacker ) )
			{
				Burn( pEntity );
			}
		}
	}

	// Render debug visualization if convar on
	if ( tf_fireball_draw_debug_radius.GetBool() )
	{
		if ( m_hEntitiesBurnt.Count() > 0 )
		{
			int val = ( (int) ( gpGlobals->curtime * 10 ) ) % 255;
			NDebugOverlay::Sphere( GetAbsOrigin(), GetAbsAngles(), GetDamageRadius(), val, 255, val, 0, false, 0 );
		} 
		else 
		{
			NDebugOverlay::Sphere( GetAbsOrigin(), GetAbsAngles(), GetDamageRadius(), 0, 100, 0, 0, false, 0 );
		}
	}

	// if we've expired, remove ourselves
	float flDistance = GetAbsOrigin().DistTo( m_vecInitialPos );
	if ( flDistance >= tf_fireball_distance.GetFloat() )
	{
		#ifdef GAME_DLL
		CEffectData	data;
		data.m_nHitBox = GetParticleSystemIndex( "projectile_fireball_end" );
		data.m_vOrigin = WorldSpaceCenter();
		data.m_vAngles = vec3_angle;
		data.m_nEntIndex = 0;

		CPVSFilter filter( WorldSpaceCenter() );
		te->DispatchEffect( filter, 0.0, data.m_vOrigin, "ParticleEffect", data );
		#endif
		UTIL_Remove( this );
		if ( tf_fireball_draw_debug_radius.GetBool() )
			NDebugOverlay::Sphere( GetAbsOrigin(), GetAbsAngles(), GetDamageRadius(), 0, 100, 0, 0, false, 1 );

		return;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_BallOfFire::RocketTouch( CBaseEntity *pOther )
{
	BaseClass::RocketTouch( pOther );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_BallOfFire::SetScorer( CBaseEntity *pScorer )
{
	m_Scorer = pScorer;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CBasePlayer *CTFProjectile_BallOfFire::GetScorer( void )
{
	return dynamic_cast<CBasePlayer *>( m_Scorer.Get() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFProjectile_BallOfFire::GetDamageType() 
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
	if ( m_iDeflected > 0 )
	{
		iDmgType |= DMG_MINICRITICAL;
	}

	return iDmgType;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_BallOfFire::Deflected( CBaseEntity *pDeflectedBy, Vector &vecDir )
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
void CTFProjectile_BallOfFire::Explode( trace_t *pTrace, CBaseEntity *pOther )
{
	// Save this entity as enemy, they will take 100% damage.
	m_hEnemy = pOther;

	// Invisible.
	SetModelName( NULL_STRING );
	AddSolidFlags( FSOLID_NOT_SOLID );
	m_takedamage = DAMAGE_NO;

	// Pull out a bit.
	if ( pTrace->fraction != 1.0 )
	{
		SetAbsOrigin( pTrace->endpos + ( pTrace->plane.normal * 1.0f ) );
	}

	// Don't decal players with scorch.
	if ( !pOther->IsPlayer() )
	{
		UTIL_DecalTrace( pTrace, "Scorch" );
	}

	/*#ifdef GAME_DLL
	CEffectData	data;
	data.m_nHitBox = GetParticleSystemIndex( "projectile_fireball_end" );
	data.m_vOrigin = WorldSpaceCenter();
	data.m_vAngles = vec3_angle;
	data.m_nEntIndex = 0;

	CPVSFilter filter( WorldSpaceCenter() );
	te->DispatchEffect( filter, 0.0, data.m_vOrigin, "ParticleEffect", data );
	#endif*/

	// Remove.
	UTIL_Remove( this );
	if ( tf_fireball_draw_debug_radius.GetBool() )
		NDebugOverlay::Sphere( GetAbsOrigin(), GetAbsAngles(), GetDamageRadius(), 0, 100, 0, 0, false, 1 );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFProjectile_BallOfFire *CTFProjectile_BallOfFire::Create( CBaseEntity *pWeapon, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, CBaseEntity *pScorer )
{
	CTFProjectile_BallOfFire *pFireball = static_cast<CTFProjectile_BallOfFire*>( CBaseEntity::CreateNoSpawn( "tf_projectile_balloffire", vecOrigin, vecAngles, pOwner ) );

	if ( pFireball )
	{
		// Set team.
		pFireball->ChangeTeam( pOwner->GetTeamNumber() );

		// Set scorer.
		pFireball->SetScorer( pScorer );

		// Set firing weapon.
		pFireball->SetLauncher( pWeapon );

		// Spawn.
		DispatchSpawn( pFireball );

		// Setup the initial velocity.
		Vector vecForward, vecRight, vecUp;
		AngleVectors( vecAngles, &vecForward, &vecRight, &vecUp );

		float flVelocity = tf_fireball_speed.GetFloat();
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pWeapon, flVelocity, mult_projectile_speed );

		Vector vecVelocity = vecForward * flVelocity;
		pFireball->SetAbsVelocity( vecVelocity );
		pFireball->SetupInitialTransmittedGrenadeVelocity( vecVelocity );

		// Setup the initial angles.
		QAngle angles;
		VectorAngles( vecVelocity, angles );
		pFireball->SetAbsAngles( angles );
		
		float flGravity = 0.0f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pWeapon, flGravity, mod_rocket_gravity );
		if ( flGravity )
		{
			pFireball->SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_CUSTOM );
			pFireball->SetGravity( flGravity );
		}

		return pFireball;
	}

	return pFireball;
}
#else

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFProjectile_BallOfFire::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		CreateTrails();
		CreateLightEffects();
	}

	// Watch team changes and change trail accordingly.
	if ( m_iDeflected != m_iOldDeflected )
	{
		ParticleProp()->StopEmission();
		CreateTrails();
		CreateLightEffects();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFProjectile_BallOfFire::CreateTrails( void )
{
	if ( IsDormant() )
		return;

	if ( enginetrace->GetPointContents( GetAbsOrigin() ) & MASK_WATER )
	{
		ParticleProp()->Create( "rockettrail_waterbubbles", PATTACH_POINT_FOLLOW, "empty" );
	}
	else
	{
		const char *pszFormat = m_bCritical ? "projectile_fireball_crit_%s" : "projectile_fireball";
		const char *pszEffectName = ConstructTeamParticle( pszFormat, GetTeamNumber() );

		ParticleProp()->Create( pszEffectName, PATTACH_POINT_FOLLOW, "empty" );
	}
}

void C_TFProjectile_BallOfFire::CreateLightEffects( void )
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

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: Called when we've collided with another entity
//-----------------------------------------------------------------------------
void CTFProjectile_BallOfFire::Burn( CBaseEntity *pOther )
{
	// remember that we've burnt this one
 	m_hEntitiesBurnt.AddToTail( pOther );

	// Save this entity as enemy, they will take 100% damage.
	m_hEnemy = pOther;

	// Invisible.
	SetModelName( NULL_STRING );
	AddSolidFlags( FSOLID_NOT_SOLID );
	m_takedamage = DAMAGE_NO;

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
	CBreakableProp *pProp = dynamic_cast< CBreakableProp * >( pOther );

	float flDamage = tf_fireball_damage.GetFloat();
	int iDamageCustom = TF_DMG_CUSTOM_DRAGONS_FURY_IGNITE;

	CTFWeaponBase *pWeapon = ( CTFWeaponBase * )m_hLauncher.Get();

	if ( pPlayer )
	{
		// Hit player, do impact sound and more damage
		if ( m_hEntitiesBurnt.Count() > 0 )
		{
			if ( pPlayer->m_Shared.InCond( TF_COND_BURNING ) )
			{
				flDamage = tf_fireball_damage.GetFloat() * tf_fireball_burning_bonus.GetFloat();

				iDamageCustom = TF_DMG_CUSTOM_DRAGONS_FURY_BONUS_BURNIN;
				#ifdef GAME_DLL
				CEffectData	data;
				data.m_nHitBox = GetParticleSystemIndex( "dragons_fury_effect_parent" );
				data.m_vOrigin = pPlayer->GetAbsOrigin();
				data.m_vAngles = vec3_angle;
				data.m_nEntIndex = 0;

				CPVSFilter filter( vecOrigin );
				te->DispatchEffect( filter, 0.0, data.m_vOrigin, "ParticleEffect", data );

				EmitSound_t params;
				params.m_flSoundTime = 0;
				params.m_pSoundName = GetExplodeEffectSound();
				EmitSound( filter, pAttacker->entindex(), params );
				#endif
			}
		}

		if ( pWeapon )
			pWeapon->m_flNextPrimaryAttack = gpGlobals->curtime - 0.4f;
	}
	else if ( pProp )
	{
		// If we won't be able to break it, don't burn
		if ( pProp->m_takedamage == DAMAGE_YES )
		{
			pProp->IgniteLifetime( tf_fireball_burn_duration.GetFloat() );
			ApplyMultiDamage();
		}
	}
	else
	{
		ApplyMultiDamage();
	}

	CTakeDamageInfo info( GetOwnerEntity(), pAttacker, pWeapon, flDamage, GetDamageType(), iDamageCustom );
	pOther->TakeDamage( info );

	info.SetReportedPosition( pAttacker->GetAbsOrigin() );	
		
	// We collided with pOther, so try to find a place on their surface to show blood
	trace_t pTrace;
	UTIL_TraceLine(WorldSpaceCenter(), pOther->WorldSpaceCenter(), /*MASK_SOLID*/ MASK_SHOT | CONTENTS_HITBOX, this, COLLISION_GROUP_DEBRIS, &pTrace);

	pOther->DispatchTraceAttack( info, GetAbsVelocity(), &pTrace );

	ApplyMultiDamage();
}
#endif

#ifdef GAME_DLL
float CTFProjectile_BallOfFire::GetDamageRadius( void ) const
{
	return tf_fireball_radius.GetFloat();
}

// need correct number
float CTFProjectile_BallOfFire::GetFireballScale( void ) const
{
	return 0.01f;
}

const char *CTFProjectile_BallOfFire::GetExplodeEffectSound( void )
{ 
	return "Weapon_DragonsFury.BonusDamageHit";
}
#endif