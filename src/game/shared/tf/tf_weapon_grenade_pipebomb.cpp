//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: TF Pipebomb Grenade.
//
//=============================================================================//
// Warning to all who enter trying to figure out grenade code:
// This file contains both Sticky Bombs and Grenade Launcher grenades. Valve seemed to not be able to decide what the hell
// they should call them, and so half of the file refers to stickies as pipebombs, and grenades as grenades,
// and the other half refers to stickies as grenades and grenades as pipebombs.
// I've tried to mark which ones are which with comments at the start of functions so that future coders know what's up.
// - Iamgoofball
//=============================================================================//
#include "cbase.h"
#include "tf_weaponbase.h"
#include "tf_gamerules.h"
#include "npcevent.h"
#include "engine/IEngineSound.h"
#include "tf_weapon_grenade_pipebomb.h"
#include "tf_weapon_pipebomblauncher.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "IEffects.h"
#include "c_team.h"
#include "functionproxy.h"
// Server specific.
#else
#include "tf_player.h"
#include "items.h"
#include "tf_weaponbase_grenadeproj.h"
#include "soundent.h"
#include "KeyValues.h"
#include "IEffects.h"
#include "props.h"
#include "func_respawnroom.h"
#endif

#define TF_WEAPON_PIPEBOMB_TIMER		3.0f //Seconds

#define TF_WEAPON_PIPEBOMB_GRAVITY		0.5f
#define TF_WEAPON_PIPEBOMB_FRICTION		0.8f
#define TF_WEAPON_PIPEBOMB_ELASTICITY	0.45f

#define TF_WEAPON_PIPEBOMB_TIMER_DMG_REDUCTION		0.6

extern ConVar tf_grenadelauncher_max_chargetime;
ConVar tf_grenadelauncher_chargescale( "tf_grenadelauncher_chargescale", "1.0", FCVAR_CHEAT | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );
ConVar tf_grenadelauncher_livetime( "tf_grenadelauncher_livetime", "0.8", FCVAR_CHEAT | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );

#ifndef CLIENT_DLL
ConVar tf_grenadelauncher_min_contact_speed( "tf_grenadelauncher_min_contact_speed", "100", FCVAR_DEVELOPMENTONLY );
#endif

IMPLEMENT_NETWORKCLASS_ALIASED( TFGrenadePipebombProjectile, DT_TFProjectile_Pipebomb )

BEGIN_NETWORK_TABLE( CTFGrenadePipebombProjectile, DT_TFProjectile_Pipebomb )
#ifdef CLIENT_DLL
	RecvPropInt( RECVINFO( m_iType ) ),
	RecvPropInt( RECVINFO( m_iVariant ) ),
	RecvPropBool( RECVINFO( m_bDefensiveBomb ) ),
	RecvPropEHandle( RECVINFO( m_hLauncher ) ),
#else
	SendPropInt( SENDINFO( m_iType ), 2 ),
//	SendPropInt( SENDINFO( m_iVariant ), 2 ),
	SendPropBool( SENDINFO( m_bDefensiveBomb ) ),
	SendPropEHandle( SENDINFO( m_hLauncher ) ),
#endif
END_NETWORK_TABLE()

#ifdef GAME_DLL
static string_t s_iszTrainName;
static string_t s_iszSawBlade01;
static string_t s_iszSawBlade02;
#endif

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFGrenadePipebombProjectile::CTFGrenadePipebombProjectile()
{
	m_bTouched = false;
	m_flChargeTime = 0.0f;

#ifdef CLIENT_DLL
	m_pGlowObject = NULL;
#endif

#ifdef GAME_DLL
	s_iszTrainName  = AllocPooledString( "models/props_vehicles/train_enginecar.mdl" );
	s_iszSawBlade01 = AllocPooledString( "sawmovelinear01" );
	s_iszSawBlade02 = AllocPooledString( "sawmovelinear02" );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFGrenadePipebombProjectile::~CTFGrenadePipebombProjectile()
{
#ifdef CLIENT_DLL
	ParticleProp()->StopEmission();
	delete m_pGlowObject;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// PIPEBOMB = STICKY
//-----------------------------------------------------------------------------
int	CTFGrenadePipebombProjectile::GetDamageType( void )
{
	int iDmgType = BaseClass::GetDamageType();

	// If we're a pipebomb, we do distance based damage falloff for just the first few seconds of our life
	if ( m_iType == TF_GL_MODE_REMOTE_DETONATE )
	{
		if ( gpGlobals->curtime - m_flCreationTime < 5.0 )
		{
			iDmgType |= DMG_USEDISTANCEMOD;
		}
	}
	else if ( m_iDeflected > 0 )
	{
		// deflected stickies shouldn't get minicrits 
		iDmgType |= DMG_MINICRITICAL;
	}

	return iDmgType;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadePipebombProjectile::UpdateOnRemove( void )
{
	// Tell our launcher that we were removed
	CTFPipebombLauncher *pLauncher = dynamic_cast<CTFPipebombLauncher*>( m_hLauncher.Get() );

	if ( pLauncher )
	{
		pLauncher->DeathNotice( this );
	}

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFGrenadePipebombProjectile::GetLiveTime( void ) const
{
	float flArmTime = tf_grenadelauncher_livetime.GetFloat();
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_hLauncher, flArmTime, sticky_arm_time );
	return flArmTime;
}

#ifdef CLIENT_DLL
//=============================================================================
//
// TF Pipebomb Grenade Projectile functions (Client specific).
//

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
// STICKYBOMB = STICKY
// PIPEBOMB = GRENADE
//-----------------------------------------------------------------------------
const char *CTFGrenadePipebombProjectile::GetTrailParticleName( void )
{
	if ( m_iType == TF_GL_MODE_REMOTE_DETONATE )
	{
		return ConstructTeamParticle( "stickybombtrail_%s", GetTeamNumber(), true );
	}
	else
	{
		return ConstructTeamParticle( "pipebombtrail_%s", GetTeamNumber(), true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
// GRENADE = STICKY
// PIPE = GRENADE
//-----------------------------------------------------------------------------
void CTFGrenadePipebombProjectile::CreateTrails( void )
{
	CNewParticleEffect *pParticle = ParticleProp()->Create( GetTrailParticleName(), PATTACH_ABSORIGIN_FOLLOW );

	if ( m_bCritical )
	{
		const char *pszFormat = ( m_iType == TF_GL_MODE_REMOTE_DETONATE ) ? "critical_grenade_%s" : "critical_pipe_%s";
		const char *pszEffectName = ConstructTeamParticle( pszFormat, GetTeamNumber(), true );

		pParticle = ParticleProp()->Create( pszEffectName, PATTACH_ABSORIGIN_FOLLOW );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
// PIPEBOMB = STICKY
//-----------------------------------------------------------------------------
void CTFGrenadePipebombProjectile::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		m_flCreationTime = gpGlobals->curtime;

		m_bPulsed = false;

		CTFPipebombLauncher *pLauncher = dynamic_cast<CTFPipebombLauncher*>( m_hLauncher.Get() );

		if ( pLauncher )
		{
			pLauncher->AddPipeBomb( this );
		}

		CreateTrails();

		if ( m_bDefensiveBomb )
		{
			if ( C_BasePlayer::GetLocalPlayer() == GetThrower() )
			{
				Vector vecColor;
				if ( GetTeamNumber() == TF_TEAM_RED )
					vecColor.Init( 150.f, 0.0f, 0.0f );
				else if ( GetTeamNumber() == TF_TEAM_BLUE )
					vecColor.Init( 0.0f, 0.0f, 150.0f );

				m_pGlowObject = new CGlowObject( this, vecColor, 1.0f, true );
			}
		}
	}
	else if ( m_bTouched )
	{
		//ParticleProp()->StopEmission();
	}

	if ( m_iOldTeamNum && m_iOldTeamNum != m_iTeamNum )
	{
		ParticleProp()->StopEmission();
		CreateTrails();
	}
}

void CTFGrenadePipebombProjectile::Simulate( void )
{
	BaseClass::Simulate();

	if ( m_iType != TF_GL_MODE_REMOTE_DETONATE )
		return;

	if ( m_bPulsed == false )
	{
		if ( (gpGlobals->curtime - m_flCreationTime) >= GetLiveTime() )
		{
			const char *pszEffectName = ConstructTeamParticle( "stickybomb_pulse_%s", GetTeamNumber() );
			ParticleProp()->Create( pszEffectName, PATTACH_ABSORIGIN );

			m_bPulsed = true;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Don't draw if we haven't yet gone past our original spawn point
// Input  : flags - 
//-----------------------------------------------------------------------------
int CTFGrenadePipebombProjectile::DrawModel( int flags )
{
	if ( gpGlobals->curtime < ( m_flCreationTime + 0.1 ) )
		return 0;

	return BaseClass::DrawModel( flags );
}

#else

//=============================================================================
//
// TF Pipebomb Grenade Projectile functions (Server specific).
//
#define TF_WEAPON_PIPEBOMB_MODEL        	 "models/weapons/w_models/w_grenade_pipebomb.mdl"
#define TF_WEAPON_GRENADE_MODEL        		 "models/weapons/w_models/w_grenade_grenadelauncher.mdl"
#define TF_WEAPON_STICKYBOMB_MODEL           "models/weapons/w_models/w_stickybomb.mdl"
#define TF_WEAPON_STICKYBOMB_DEFENSIVE_MODEL "models/weapons/w_models/w_stickybomb_d.mdl"
#define TF_WEAPON_PIPEBOMB_BOUNCE_SOUND	   	 "Weapon_Grenade_Pipebomb.Bounce"
#define TF_WEAPON_GRENADE_DETONATE_TIME    2.0f
#define TF_WEAPON_GRENADE_XBOX_DAMAGE      112

BEGIN_DATADESC( CTFGrenadePipebombProjectile )
END_DATADESC()

LINK_ENTITY_TO_CLASS( tf_projectile_pipe_remote, CTFGrenadePipebombProjectile );
PRECACHE_WEAPON_REGISTER( tf_projectile_pipe_remote );

LINK_ENTITY_TO_CLASS( tf_projectile_pipe, CTFGrenadePipebombProjectile );
PRECACHE_WEAPON_REGISTER( tf_projectile_pipe );

LINK_ENTITY_TO_CLASS( tf_weapon_grenade_pipebomb_projectile, CTFGrenadePipebombProjectile );
PRECACHE_WEAPON_REGISTER( tf_weapon_grenade_pipebomb_projectile );

//-----------------------------------------------------------------------------
// Purpose:
// PIPEBOMB = STICKY
// GRENADE = GRENADE (for once)
//-----------------------------------------------------------------------------
CTFGrenadePipebombProjectile* CTFGrenadePipebombProjectile::Create( const Vector &position, const QAngle &angles, 
																    const Vector &velocity, const AngularImpulse &angVelocity, 
																    CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo,
																	int iMode, float flDamageMult, CTFWeaponBase *pWeapon, int iVariant )
{
	CTFGrenadePipebombProjectile *pGrenade = static_cast<CTFGrenadePipebombProjectile*>( CBaseEntity::CreateNoSpawn( ( iMode == TF_GL_MODE_REMOTE_DETONATE ) ? ( ( iVariant == TF_GL_IS_STICKY ) ? "tf_projectile_pipe_remote" : "tf_weapon_grenade_pipebomb_projectile" ): "tf_projectile_pipe", position, angles, pOwner ) );
	if ( pGrenade )
	{
		// Set the pipebomb mode before calling spawn, so the model & associated vphysics get setup properly
		pGrenade->SetPipebombMode( iMode );
		pGrenade->SetPipebombBetaVariant( iVariant );
		DispatchSpawn( pGrenade );

		// Set the launcher for model overrides
		pGrenade->SetLauncher( pWeapon );

		pGrenade->InitGrenade( velocity, angVelocity, pOwner, weaponInfo );

#ifdef _X360 
		if ( pGrenade->m_iType != TF_GL_MODE_REMOTE_DETONATE )
		{
			pGrenade->SetDamage( TF_WEAPON_GRENADE_XBOX_DAMAGE );
		}
#endif

		pGrenade->SetDamage( pGrenade->GetDamage() * flDamageMult );
		pGrenade->m_flFullDamage = pGrenade->GetDamage();

		if ( pGrenade->m_iType != TF_GL_MODE_REMOTE_DETONATE )
		{
			// Some hackery here. Reduce the damage by 25%, so that if we explode on timeout,
			// we'll do less damage. If we explode on contact, we'll restore this to full damage.
			pGrenade->SetDamage( pGrenade->GetDamage() * TF_WEAPON_PIPEBOMB_TIMER_DMG_REDUCTION );
		}

		pGrenade->ApplyLocalAngularVelocityImpulse( angVelocity );
	}

	return pGrenade;
}

//-----------------------------------------------------------------------------
// Purpose:
// PIPEBOMB = STICKY
// PIPEGRENADE\GRENADE = GRENADE
//-----------------------------------------------------------------------------
void CTFGrenadePipebombProjectile::Spawn()
{
	if ( m_iType == TF_GL_MODE_REMOTE_DETONATE )
	{
		// Set this to max, so effectively they do not self-implode.
		SetDetonateTimerLength( FLT_MAX );
		if ( m_iBeta == TF_GL_IS_STICKY )
		SetModel( TF_WEAPON_STICKYBOMB_MODEL );
		else
		SetModel( TF_WEAPON_PIPEBOMB_MODEL );	
	}
	else
	{
		SetDetonateTimerLength( TF_WEAPON_GRENADE_DETONATE_TIME );
		SetTouch( &CTFGrenadePipebombProjectile::PipebombTouch );
		SetModel( TF_WEAPON_GRENADE_MODEL );
	}

	BaseClass::Spawn();

	m_bTouched = false;
	m_flCreationTime = gpGlobals->curtime;

	// Pumpkin Bombs
	AddFlag( FL_GRENADE );

	// We want to get touch functions called so we can damage enemy players
	AddSolidFlags( FSOLID_TRIGGER );

	m_flMinSleepTime = 0;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadePipebombProjectile::Precache()
{
	PrecacheTeamParticles("stickybombtrail_%s", true);

	int index = PrecacheModel( TF_WEAPON_STICKYBOMB_MODEL );
	PrecacheGibsForModel( index );
	
	index = PrecacheModel( TF_WEAPON_STICKYBOMB_DEFENSIVE_MODEL );
	PrecacheGibsForModel( index );

	index = PrecacheModel( TF_WEAPON_GRENADE_MODEL );
	PrecacheGibsForModel( index );

	PrecacheScriptSound( TF_WEAPON_PIPEBOMB_BOUNCE_SOUND );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadePipebombProjectile::SetPipebombMode( int iMode )
{
	m_iType.Set( iMode );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadePipebombProjectile::SetPipebombBetaVariant( int iVariant )
{
	m_iBeta.Set( iVariant );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadePipebombProjectile::BounceSound( void )
{
	EmitSound( TF_WEAPON_PIPEBOMB_BOUNCE_SOUND );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadePipebombProjectile::Detonate()
{
	// If we're detonating stickies then we're currently inside prediction
	// so we gotta make sure all effects show up.
	CDisablePredictionFiltering disabler;

	if ( ShouldNotDetonate() )
	{
		RemoveGrenade( true );
		return;
	}

	if ( m_bFizzle )
	{
		g_pEffects->Sparks( GetAbsOrigin() );

		// CreatePipebombGibs
		CPVSFilter filter( GetAbsOrigin() );
		/*UserMessageBegin( filter, "CheapBreakModel" );
			WRITE_SHORT( GetModelIndex() );
			WRITE_VEC3COORD( GetAbsOrigin() );
		MessageEnd();*/

		RemoveGrenade( false );

		return;
	}

	BaseClass::Detonate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadePipebombProjectile::Fizzle( void )
{
	m_bFizzle = true;
}


void CTFGrenadePipebombProjectile::DetonateStickies( void )
{
	if ( !m_hLauncher )
		return;

	CBaseEntity *pList[64];
	CFlaggedEntitiesEnum enumerator( pList, sizeof( pList ), FL_GRENADE );
	int count = UTIL_EntitiesInSphere( GetAbsOrigin(), GetDamageRadius(), &enumerator );

	for ( int i=0; i<count; ++i )
	{
		CTFGrenadePipebombProjectile *pOther = dynamic_cast<CTFGrenadePipebombProjectile *>( pList[i] );
		if ( !pOther || !pOther->m_hLauncher )
			continue;

		if ( pOther->m_hLauncher->GetTeamNumber() == m_hLauncher->GetTeamNumber() )
			continue;

		trace_t tr;
		UTIL_TraceLine( GetAbsOrigin(), pOther->GetAbsOrigin(), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );
		if ( tr.fraction >= 1.0f )
		{
			pOther->Fizzle();
			pOther->Detonate();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadePipebombProjectile::PipebombTouch( CBaseEntity *pOther )
{
	if ( pOther == GetThrower() )
		return;

	// Verify a correct "other."
	if ( !pOther->IsSolid() || pOther->IsSolidFlagSet( FSOLID_VOLUME_CONTENTS ) )
		return;

	// Handle hitting skybox (disappear).
	trace_t pTrace;
	Vector velDir = GetAbsVelocity();
	VectorNormalize( velDir );
	Vector vecSpot = GetAbsOrigin() - velDir * 32;
	UTIL_TraceLine( vecSpot, vecSpot + velDir * 64, MASK_SOLID, this, COLLISION_GROUP_NONE, &pTrace );

	if ( pTrace.fraction < 1.0 && pTrace.surface.flags & SURF_SKY )
	{
		UTIL_Remove( this );
		return;
	}

	//If we already touched a surface then we're not exploding on contact anymore.
	if ( m_bTouched == true )
		return;

	// Blow up if we hit an enemy we can damage
	if ( pOther->IsCombatCharacter() && pOther->GetTeamNumber() != GetTeamNumber() && pOther->m_takedamage != DAMAGE_NO )
	{
		// Check to see if this is a respawn room.
		if ( !pOther->IsPlayer() )
		{
			CFuncRespawnRoom *pRespawnRoom = dynamic_cast<CFuncRespawnRoom*>( pOther );
			if ( pRespawnRoom )
			{
				if ( !pRespawnRoom->PointIsWithin( GetAbsOrigin() ) )
					return;
			}
		}

		// Restore damage. See comment in CTFGrenadePipebombProjectile::Create() above to understand this.
		m_flDamage = m_flFullDamage;
		// Save this entity as enemy, they will take 100% damage.
		m_hEnemy = pOther;
		Explode( &pTrace, GetDamageType() );
	}

	// Train hack!
	if ( pOther->GetModelName() == s_iszTrainName && ( pOther->GetAbsVelocity().LengthSqr() > 1.0f ) )
	{
		Explode( &pTrace, GetDamageType() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadePipebombProjectile::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
	BaseClass::VPhysicsCollision( index, pEvent );

	int otherIndex = !index;
	CBaseEntity *pHitEntity = pEvent->pEntities[otherIndex];

	if ( !pHitEntity )
		return;

	if ( m_iType != TF_GL_MODE_REMOTE_DETONATE )
	{
		// Blow up if we hit an enemy we can damage
		if ( pHitEntity->IsCombatCharacter() && pHitEntity->GetTeamNumber() != GetTeamNumber() && pHitEntity->m_takedamage != DAMAGE_NO )
		{
			// Save this entity as enemy, they will take 100% damage.
			m_hEnemy = pHitEntity;
			SetThink( &CTFGrenadePipebombProjectile::Detonate );
			SetNextThink( gpGlobals->curtime );
		}
		else if ( m_iType == TF_GL_MODE_FIZZLE )
		{
			// Fizzle on contact with a surface if the we're running loch
			Fizzle();
			Detonate();
		}

		m_bTouched = true;
		return;
	}

	// Handle hitting skybox (disappear).
	surfacedata_t *pprops = physprops->GetSurfaceData( pEvent->surfaceProps[otherIndex] );
	if ( pprops->game.material == 'X' )
	{
		// uncomment to destroy grenade upon hitting sky brush
		//SetThink( &CTFGrenadePipebombProjectile::SUB_Remove );
		//SetNextThink( gpGlobals->curtime );
		return;
	}

	bool bIsDynamicProp = ( NULL != dynamic_cast<CDynamicProp *>( pHitEntity ) );

	// HACK: Prevents stickies from sticking to blades in Sawmill. Need to find a way that is not as silly.
	CBaseEntity *pParent = pHitEntity->GetMoveParent();

	if ( pParent )
	{
		if ( pParent->NameMatches( s_iszSawBlade01 ) || pParent->NameMatches( s_iszSawBlade02 ) )
		{
			bIsDynamicProp = false;
		}
	}

	// Pipebombs stick to the world when they touch it
	if ( ( pHitEntity->IsWorld() || bIsDynamicProp ) && gpGlobals->curtime > m_flMinSleepTime )
	{
		m_bTouched = true;
		VPhysicsGetObject()->EnableMotion( false );

		// Save impact data for explosions.
		m_bUseImpactNormal = true;
		pEvent->pInternalData->GetSurfaceNormal( m_vecImpactNormal );
		m_vecImpactNormal.Negate();
	}
}

ConVar tf_grenade_forcefrom_bullet( "tf_grenade_forcefrom_bullet", "0.8", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );
ConVar tf_grenade_forcefrom_buckshot( "tf_grenade_forcefrom_buckshot", "0.5", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );
ConVar tf_grenade_forcefrom_blast( "tf_grenade_forcefrom_blast", "0.08", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );
ConVar tf_grenade_force_sleeptime( "tf_grenade_force_sleeptime", "1.0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );	// How long after being shot will we re-stick to the world.
ConVar tf_pipebomb_force_to_move( "tf_pipebomb_force_to_move", "1500.0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );

//-----------------------------------------------------------------------------
// Purpose: If we are shot after being stuck to the world, move a bit, unless we're a sticky, in which case, fizzle out and die.
// STICKY = STICKY
// PIPEBOMB = GRENADE
//-----------------------------------------------------------------------------
int CTFGrenadePipebombProjectile::OnTakeDamage( const CTakeDamageInfo &info )
{
	if ( !info.GetAttacker() )
	{
		Assert( !info.GetAttacker() );
		return 0;
	}

	bool bSameTeam = ( info.GetAttacker()->GetTeamNumber() == GetTeamNumber() );


	if ( m_bTouched && ( info.GetDamageType() & (DMG_BULLET|DMG_BUCKSHOT|DMG_BLAST|DMG_CLUB|DMG_SLASH) ) && bSameTeam == false )
	{
		Vector vecForce = info.GetDamageForce();
		// Sticky bombs get destroyed by bullets and melee, not pushed
		if ( m_iType == TF_GL_MODE_REMOTE_DETONATE )
		{
			if ( info.GetDamageType() & (DMG_BULLET|DMG_BUCKSHOT|DMG_CLUB|DMG_SLASH) )
			{
				m_bFizzle = true;
				Detonate();
			}
			else if ( info.GetDamageType() & DMG_BLAST )
			{
				vecForce *= tf_grenade_forcefrom_blast.GetFloat();
			}
		}
		else
		{
			if ( info.GetDamageType() & DMG_BULLET )
			{
				vecForce *= tf_grenade_forcefrom_bullet.GetFloat();
			}
			else if ( info.GetDamageType() & DMG_BUCKSHOT )
			{
				vecForce *= tf_grenade_forcefrom_buckshot.GetFloat();
			}
			else if ( info.GetDamageType() & DMG_BLAST )
			{
				vecForce *= tf_grenade_forcefrom_blast.GetFloat();
			}
		}

		// If the force is sufficient, detach & move the pipebomb
		float flForce = tf_pipebomb_force_to_move.GetFloat();
		if ( vecForce.LengthSqr() > (flForce*flForce) )
		{
			if ( VPhysicsGetObject() )
			{
				VPhysicsGetObject()->EnableMotion( true );
			}

			CTakeDamageInfo newInfo = info;
			newInfo.SetDamageForce( vecForce );

			VPhysicsTakeDamage( newInfo );

			// The pipebomb will re-stick to the ground after this time expires
			m_flMinSleepTime = gpGlobals->curtime + tf_grenade_force_sleeptime.GetFloat();
			m_bTouched = false;

			// It has moved the data is no longer valid.
			m_bUseImpactNormal = false;
			m_vecImpactNormal.Init();

			return 1;
		}
	}

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadePipebombProjectile::Deflected( CBaseEntity *pDeflectedBy, Vector &vecDir )
{
	if ( GetType() == TF_GL_MODE_REMOTE_DETONATE )
	{
		// This is kind of lame.
		Vector vecPushSrc = pDeflectedBy->WorldSpaceCenter();
		Vector vecPushDir = GetAbsOrigin() - vecPushSrc;
		VectorNormalize( vecPushDir );

		CTakeDamageInfo info( pDeflectedBy, pDeflectedBy, 100, DMG_BLAST );
		CalculateExplosiveDamageForce( &info, vecPushDir, vecPushSrc );
		TakeDamage( info );
	}
	else
	{
		BaseClass::Deflected( pDeflectedBy, vecDir );
	}
	// TODO: Live TF2 adds white trail to reflected pipes and stickies. We need one as well.
}


#endif

#if defined(CLIENT_DLL)
class CProxyStickyBombGlowColor : public CResultProxy
{
public:
	virtual ~CProxyStickyBombGlowColor() {}
	virtual void OnBind( void *pObject );
};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CProxyStickyBombGlowColor::OnBind( void *pObject )
{
	if ( !pObject )
	{
		m_pResult->SetVecValue( 1.0f, 1.0f, 1.0f );
		return;
	}

	C_BaseEntity *pEntity = BindArgToEntity( pObject );
	if ( !pEntity )
	{
		m_pResult->SetVecValue( 1.0f, 1.0f, 1.0f );
		return;
	}

	CTFGrenadePipebombProjectile *pProjectile = dynamic_cast<CTFGrenadePipebombProjectile *>( pEntity );
	if ( !pProjectile || !pProjectile->m_pGlowObject )
	{
		m_pResult->SetVecValue( 1.0f, 1.0f, 1.0f );
		return;
	}

	Vector vecColor;

	if ( pProjectile->m_bGlowing )
	{
		if ( pProjectile->GetTeamNumber() == TF_TEAM_RED )
			vecColor.Init( 100.0f, 0.0f, 0.0f );
		else if ( pProjectile->GetTeamNumber() == TF_TEAM_BLUE )
			vecColor.Init( 0.0f, 0.0f, 100.0f );

		pProjectile->m_pGlowObject->SetColor( vecColor * 2.5f );
		m_pResult->SetVecValue( vecColor.x, vecColor.y, vecColor.z );

		return;
	}

	if ( pProjectile->GetTeamNumber() == TF_TEAM_RED )
		vecColor.Init( 200.0f, 100.0f, 100.0f );
	else if ( pProjectile->GetTeamNumber() == TF_TEAM_BLUE )
		vecColor.Init( 100.0f, 100.0f, 200.0f );

	pProjectile->m_pGlowObject->SetColor( vecColor );
	m_pResult->SetVecValue( 1.0f, 1.0f, 1.0f );
}

EXPOSE_INTERFACE( CProxyStickyBombGlowColor, IMaterialProxy, "StickybombGlowColor" IMATERIAL_PROXY_INTERFACE_VERSION );

#endif
