//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Short Circuit.
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_mechanical_arm.h"
#include "tf_fx_shared.h"
#include "tf_weaponbase_rocket.h"
#include "particle_parse.h"
// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "tf_viewmodel.h"
#include "c_tf_viewmodeladdon.h"
#include "iefx.h"
#include "dlight.h"
// Server specific.
#else
#include "tf_player.h"
#include "soundent.h"
#include "tf_gamerules.h"
#include "tf_gamestats.h"
#include "ilagcompensationmanager.h"
#include "effect_dispatch_data.h"
#include "Sprite.h"
#include "func_nogrenades.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
extern ConVar tf2v_muzzlelight;
#else
extern ConVar tf_debug_damage;
#endif
extern ConVar tf2v_use_new_short_circuit;

#define TF_WEAPON_MECHANICALARM_MODEL	"models/empty.mdl"
#define TF_WEAPON_MECHANICALARM_RADIUS	80.0f

IMPLEMENT_NETWORKCLASS_ALIASED( TFMechanicalArm, DT_TFMechanicalArm )

BEGIN_NETWORK_TABLE( CTFMechanicalArm, DT_TFMechanicalArm )
END_NETWORK_TABLE()

#if defined( CLIENT_DLL )
BEGIN_PREDICTION_DATA( CTFMechanicalArm )
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( tf_weapon_mechanical_arm, CTFMechanicalArm );
PRECACHE_WEAPON_REGISTER( tf_weapon_mechanical_arm );

BEGIN_DATADESC( CTFMechanicalArm )
END_DATADESC()

#define AMMO_PROJECTILE_SHOCK 15
#define AMMO_PROJECTILE_BALL 65
#define AMMO_PROJECTILE_SHOCK_REFUND 5


//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFMechanicalArm::CTFMechanicalArm()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFMechanicalArm::~CTFMechanicalArm()
{
#ifdef CLIENT_DLL
	StopParticleBeam();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMechanicalArm::Precache( void )
{
	BaseClass::Precache();
	PrecacheScriptSound( "TFPlayer.AirBlastImpact" );
	PrecacheParticleSystem( "dxhr_arm_impact" );
	PrecacheParticleSystem( "dxhr_sniper_fizzle" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMechanicalArm::Spawn( void )
{
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMechanicalArm::PrimaryAttack()
{
	m_bReloadedThroughAnimEvent = false;

	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return;

	if ( pOwner->GetWaterLevel() == WL_Eyes )
		return;

	BaseClass::PrimaryAttack();

	Vector vecForward; 
	QAngle angShot = pOwner->EyeAngles();
	AngleVectors( angShot, &vecForward );
	Vector vecSrc = pOwner->Weapon_ShootPosition();
	Vector vecEnd = vecSrc + vecForward * m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flRange;

	trace_t tr;
	UTIL_TraceLine(vecSrc, vecEnd, MASK_SOLID | CONTENTS_HITBOX, this, COLLISION_GROUP_PLAYER, &tr);

#ifdef CLIENT_DLL
	C_BaseEntity *pEffectOwner = GetWeaponForEffect();
	if ( pEffectOwner )
 	{
		m_pZap = pEffectOwner->ParticleProp()->Create( "dxhr_arm_muzzleflash", PATTACH_POINT_FOLLOW, "muzzle" );
		if ( m_pZap )
			m_pZap->SetControlPoint( 1, tr.endpos );

		if ( pEffectOwner )
			pEffectOwner->ParticleProp()->Create( "dxhr_sniper_fizzle", PATTACH_POINT_FOLLOW, "muzzle" );
	}
#endif
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMechanicalArm::StopParticleBeam( void )
{
	ParticleProp()->StopEmission( m_pZap );
	m_pZap = NULL;
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMechanicalArm::SecondaryAttack()
{
	if ( tf2v_use_new_short_circuit.GetBool() )
		LaunchElectricalBall();
	else
		LaunchElectricalShock();
}


//-----------------------------------------------------------------------------
// Purpose: Launches a straight electrical shock, destroying projectiles. Old Secondary.
//-----------------------------------------------------------------------------
void CTFMechanicalArm::LaunchElectricalShock()
{
	m_bReloadedThroughAnimEvent = false;

	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return;

	if ( pOwner->GetWaterLevel() >= WL_Eyes )
	{
		WeaponSound( EMPTY );
		return;
	}

	if ( pOwner->GetAmmoCount( m_iPrimaryAmmoType ) < ( AMMO_PROJECTILE_SHOCK ) )
	{
		WeaponSound( EMPTY );
		return;
	}
	
	SendWeaponAnim( ACT_VM_PRIMARYATTACK );
	WeaponSound( SPECIAL3 );

	// Share our vector data.
	// Client handles the visuals, server handles the damage.

	Vector vecForward; 
	QAngle angShot = pOwner->EyeAngles();
	AngleVectors( angShot, &vecForward );
	Vector vecSrc = pOwner->Weapon_ShootPosition();
	Vector vecEnd = vecSrc + vecForward * m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flRange;

	// Make a trace. This doesn't do anything for deleting projectiles but does damage + gives client a visual ending.
	trace_t tr;
	UTIL_TraceLine(vecSrc, vecEnd, MASK_SOLID, this, COLLISION_GROUP_DEBRIS, &tr);

#ifdef GAME_DLL
	pOwner->NoteWeaponFired();

	pOwner->SpeakWeaponFire();
	CTF_GameStats.Event_PlayerFiredWeapon( pOwner, false );

	// Move other players back to history positions based on local player's lag
	lagcompensation->StartLagCompensation(pOwner, pOwner->GetCurrentCommand());
	
	// Do a check on where we fire to see what we delete.
	int nDeletedProjectiles = 0;
	
	// Set up a box around our shot.
	Vector vecSize = Vector( 128, 128, 64 );
	float flBlastDist = Max(Max(vecSize.x, vecSize.y), vecSize.z);
	Vector vecOrigin = 	pOwner->EyePosition() + vecForward * flBlastDist;
	
	CBaseEntity *pList[64];
	int count = UTIL_EntitiesInBox( pList, 64, vecOrigin - vecSize, vecOrigin + vecSize, FL_CLIENT|FL_GRENADE );
	for ( int i = 0; i < count; i++ )
	{
		CBaseEntity *pEntity = pList[i];
			
		if ( !pEntity )
			continue;

		if ( pEntity == pOwner )
			continue;

		// Don't affect teammate projectiles.
		if ( InSameTeam( pEntity ) )
			continue;
		
		// Base projectile, delete it.
		CBaseProjectile *pProj = dynamic_cast<CBaseProjectile *>( pList[i] );
		if ( pProj )
		{
			pEntity->EmitSound("Weapon_BarretsArm.Fizzle");
			UTIL_Remove(pEntity);
			nDeletedProjectiles++;
			continue;
		}
		
		// Base grenade, delete it.
		CBaseGrenade *pGren = dynamic_cast<CBaseGrenade *>(pList[i]);
		if ( pGren )
		{
			pEntity->EmitSound("Weapon_BarretsArm.Fizzle");
			UTIL_Remove(pEntity);
			nDeletedProjectiles++;
			continue;
		}
	}

	lagcompensation->FinishLagCompensation( pOwner );
	
	int nAmmoCost = ( nDeletedProjectiles * AMMO_PROJECTILE_SHOCK_REFUND) - AMMO_PROJECTILE_SHOCK;
	
	// Refund our ammo if we blew up enough projectiles.
	if (nAmmoCost > 0)
		pOwner->GiveAmmo(nAmmoCost, m_iPrimaryAmmoType);
	else if (nAmmoCost < 0)	// Deduct ammo.
		pOwner->RemoveAmmo( nAmmoCost, m_iPrimaryAmmoType );

#else
	C_BaseEntity *pEffectOwner = GetWeaponForEffect();
	if ( pEffectOwner )
 	{
		m_pZap = pEffectOwner->ParticleProp()->Create( "dxhr_arm_muzzleflash", PATTACH_POINT_FOLLOW, "muzzle" );
		if ( m_pZap )
			m_pZap->SetControlPoint( 1, tr.endpos );

		if ( pEffectOwner )
			pEffectOwner->ParticleProp()->Create( "dxhr_sniper_fizzle", PATTACH_POINT_FOLLOW, "muzzle" );
	}
#endif
	// Fire rate is based on the primary fire delay.
	float flDelay = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeFireDelay;
	CALL_ATTRIB_HOOK_FLOAT( flDelay, mult_postfiredelay );
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->curtime + flDelay;
}

//-----------------------------------------------------------------------------
// Purpose: Launches an electrical ball that destroys projectiles. New Secondary.
//-----------------------------------------------------------------------------
void CTFMechanicalArm::LaunchElectricalBall()
{
	// Get the player owning the weapon.
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return;

	if ( !CanAttack() )
		return;

	if ( pOwner->GetWaterLevel() >= WL_Eyes )
	{
		WeaponSound( EMPTY );
		return;
	}

	if ( pOwner->GetAmmoCount( m_iPrimaryAmmoType ) < AMMO_PROJECTILE_BALL )
	{
		WeaponSound( EMPTY );
		return;
	}

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );
	WeaponSound( SPECIAL3 );

#ifdef CLIENT_DLL
	C_BaseEntity *pEffectOwner = GetWeaponForEffect();
	if ( pEffectOwner )
 	{
		if ( pEffectOwner )
			pEffectOwner->ParticleProp()->Create( "dxhr_sniper_fizzle", PATTACH_POINT_FOLLOW, "muzzle" );
	}
#else
	pOwner->NoteWeaponFired();

	pOwner->SpeakWeaponFire();
	CTF_GameStats.Event_PlayerFiredWeapon( pOwner, false );

	// Move other players back to history positions based on local player's lag
	lagcompensation->StartLagCompensation(pOwner, pOwner->GetCurrentCommand());


	// Launch a ball.
	CTFProjectile_MechanicalArmOrb::Create(this, pOwner->Weapon_ShootPosition(), pOwner->EyeAngles(), pOwner, pOwner);

	CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), SOUNDENT_VOLUME_MACHINEGUN, 1.0, pOwner, SOUNDENT_CHANNEL_WEAPON );

	lagcompensation->FinishLagCompensation( pOwner );
#endif

	pOwner->RemoveAmmo( AMMO_PROJECTILE_BALL, m_iPrimaryAmmoType );

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->curtime + 0.5f;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMechanicalArm::PlayWeaponShootSound( void )
{
	WeaponSound( SINGLE );
}


#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMechanicalArm::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( !m_bInAttack )
		StopParticleBeam();
}
#endif

//=============================================================================
//
// Orb projectile.
//

BEGIN_DATADESC( CTFProjectile_MechanicalArmOrb )
END_DATADESC()

LINK_ENTITY_TO_CLASS( tf_projectile_energyorb, CTFProjectile_MechanicalArmOrb );
PRECACHE_REGISTER( tf_projectile_energyorb );

IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_MechanicalArmOrb, DT_TFProjectile_MechanicalArmOrb )
BEGIN_NETWORK_TABLE( CTFProjectile_MechanicalArmOrb, DT_TFProjectile_MechanicalArmOrb )
END_NETWORK_TABLE()

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFProjectile_MechanicalArmOrb::CTFProjectile_MechanicalArmOrb()
{

}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFProjectile_MechanicalArmOrb::~CTFProjectile_MechanicalArmOrb()
{
#ifdef CLIENT_DLL
	ParticleProp()->StopEmissionAndDestroyImmediately( m_pOrb );
	ParticleProp()->StopEmissionAndDestroyImmediately( m_pOrbLightning );
	m_pOrb = NULL;
	m_pOrbLightning = NULL;
#else
	m_bCollideWithTeammates = false;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Spawn function
//-----------------------------------------------------------------------------
void CTFProjectile_MechanicalArmOrb::Spawn()
{
	SetModel( TF_WEAPON_MECHANICALARM_MODEL );
	BaseClass::Spawn();
	SetMoveType( MOVETYPE_FLY, MOVECOLLIDE_FLY_CUSTOM );
	AddEFlags( EFL_NO_WATER_VELOCITY_CHANGE );
	SetGravity( 0.0f );

	SetSolid( SOLID_NONE );
	SetSolidFlags( FSOLID_NOT_SOLID );
	SetCollisionGroup( COLLISION_GROUP_NONE );

	m_takedamage = DAMAGE_NO;

#ifdef GAME_DLL
	float flRadius = 0.01f;
	UTIL_SetSize( this, -Vector( flRadius, flRadius, flRadius ), Vector( flRadius, flRadius, flRadius ) );
	m_flDetonateTime = gpGlobals->curtime + 2.0f;

	// Setup the think function.
	SetThink( &CTFProjectile_MechanicalArmOrb::OrbThink );
	SetNextThink( gpGlobals->curtime + 0.15f );
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_MechanicalArmOrb::Precache()
{
	PrecacheModel( TF_WEAPON_MECHANICALARM_MODEL );

#ifdef GAME_DLL
	PrecacheModel( NOGRENADE_SPRITE );
#endif

	PrecacheTeamParticles( "dxhr_lightningball_parent_%s" );
	PrecacheTeamParticles( "dxhr_lightningball_hit_%s" );
	PrecacheParticleSystem( "drg_cow_explosioncore_normal" );
	PrecacheParticleSystem( "drg_cow_explosioncore_normal_blue" );

	PrecacheScriptSound( "Halloween.spell_lightning_impact" );
	PrecacheScriptSound( "Weapon_BarretsArm.Fizzle" );

	BaseClass::Precache();
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFProjectile_MechanicalArmOrb *CTFProjectile_MechanicalArmOrb::Create( CBaseEntity *pWeapon, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, CBaseEntity *pScorer )
{
	CTFProjectile_MechanicalArmOrb *pOrb = static_cast<CTFProjectile_MechanicalArmOrb*>(CBaseEntity::CreateNoSpawn("tf_projectile_energyorb", vecOrigin, vecAngles, pOwner));

	if ( pOrb )
	{
		// Set team.
		pOrb->ChangeTeam( pOwner->GetTeamNumber() );

		// Set scorer.
		pOrb->SetScorer( pScorer );

		// Set firing weapon.
		pOrb->SetLauncher( pWeapon );

		// Spawn.
		DispatchSpawn( pOrb );

		// Setup the initial velocity.
		Vector vecForward, vecRight, vecUp;
		AngleVectors( vecAngles, &vecForward, &vecRight, &vecUp );

		float flVelocity = 800;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pWeapon, flVelocity, mult_projectile_speed );

		Vector vecVelocity = vecForward * flVelocity;
		pOrb->SetAbsVelocity( vecVelocity );
		pOrb->SetupInitialTransmittedGrenadeVelocity( vecVelocity );

		// Setup the initial angles.
		QAngle angles;
		VectorAngles( vecVelocity, angles );
		pOrb->SetAbsAngles( angles );
		
		float flGravity = 0.0f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pWeapon, flGravity, mod_rocket_gravity );

		return pOrb;
	}

	return pOrb;
}

//-----------------------------------------------------------------------------
// Purpose: Think method
//-----------------------------------------------------------------------------
void CTFProjectile_MechanicalArmOrb::OrbThink( void )
{
	if ( tf_debug_damage.GetBool() )
		NDebugOverlay::Sphere( GetAbsOrigin(), GetAbsAngles(), TF_WEAPON_MECHANICALARM_RADIUS, 0, 255, 0, 0, false, 0 );

	SetNextThink( gpGlobals->curtime + 0.15f );

	if ( gpGlobals->curtime > m_flDetonateTime )
	{
		ExplodeAndRemove();
		return;
	}

	CheckForPlayers();
	CheckForProjectiles();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_MechanicalArmOrb::CheckForPlayers( void )
{
 	CBaseEntity *pEntity = NULL;
 	for ( CEntitySphereQuery sphere( GetAbsOrigin(), TF_WEAPON_MECHANICALARM_RADIUS ); ( pEntity = sphere.GetCurrentEntity() ) != NULL; sphere.NextEntity() )
	{
		if ( !pEntity )
			continue;

		if ( InSameTeam( pEntity ) )
			continue;

 		Vector vecHitPoint;
		pEntity->CollisionProp()->CalcNearestPoint( GetAbsOrigin(), &vecHitPoint );
		Vector vecDir = vecHitPoint - GetAbsOrigin();
 		if ( vecDir.LengthSqr() < ( TF_WEAPON_MECHANICALARM_RADIUS * TF_WEAPON_MECHANICALARM_RADIUS ) && ( ( pEntity->IsCombatCharacter() || pEntity->ClassMatches( "prop_physics" ) || pEntity->ClassMatches( "item_item_crate" ) ) ) && pEntity->m_takedamage == DAMAGE_YES )
		{
			EHANDLE hOther = pEntity;
			m_hZapTargets.AddToTail( hOther );
			ZapPlayer( pEntity );
		}
		else
		{
			EHANDLE hOther = pEntity;
			m_hZapTargets.FindAndRemove( hOther );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_MechanicalArmOrb::CheckForProjectiles( void )
{
 	CBaseEntity *pEntity = NULL;
 	for ( CEntitySphereQuery sphere( GetAbsOrigin(), TF_WEAPON_MECHANICALARM_RADIUS ); ( pEntity = sphere.GetCurrentEntity() ) != NULL; sphere.NextEntity() )
	{
		if ( !pEntity )
			continue;

		if ( InSameTeam( pEntity ) )
			continue;

 		Vector vecHitPoint;
		pEntity->CollisionProp()->CalcNearestPoint( GetAbsOrigin(), &vecHitPoint );
		Vector vecDir = vecHitPoint - GetAbsOrigin();
 		if ( vecDir.LengthSqr() < ( TF_WEAPON_MECHANICALARM_RADIUS * TF_WEAPON_MECHANICALARM_RADIUS ) )
		{
			// epic collide
			CTFProjectile_MechanicalArmOrb *pOrb = dynamic_cast<CTFProjectile_MechanicalArmOrb *>( pEntity );
			if ( pOrb )
				ExplodeAndRemove();

			// delete
			CBaseProjectile *pProj = dynamic_cast<CBaseProjectile *>( pEntity );
			if (pProj)
			{
				EHANDLE hOther = pProj;
				m_hZapTargets.AddToTail(hOther);
				CSprite *pGlowSprite = CSprite::SpriteCreate(NOGRENADE_SPRITE, pProj->GetAbsOrigin(), false);
				if (pGlowSprite)
				{
					pGlowSprite->SetTransparency(kRenderGlow, 255, 255, 255, 255, kRenderFxFadeFast);
					pGlowSprite->SetThink(&CSprite::SUB_Remove);
					pGlowSprite->SetNextThink(gpGlobals->curtime + 0.25);
				}
				pEntity->EmitSound("Weapon_BarretsArm.Fizzle");
				UTIL_Remove(pEntity);
			}
		}
		else
		{
			EHANDLE hOther = pEntity;
			m_hZapTargets.FindAndRemove( hOther );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_MechanicalArmOrb::RocketTouch( CBaseEntity *pOther )
{
	// Verify a correct "other."
	Assert( pOther );

	if ( pOther->IsSolidFlagSet( FSOLID_TRIGGER | FSOLID_VOLUME_CONTENTS ) )
	{
		if ( !pOther->ClassMatches( "entity_medigun_shield" ) )
			return;
	}

	ExplodeAndRemove();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_MechanicalArmOrb::Deflected( CBaseEntity *pDeflectedBy, Vector &vecDir )
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
// Purpose: Called when we've collided with another entity
//-----------------------------------------------------------------------------
void CTFProjectile_MechanicalArmOrb::ZapPlayer( /*CTakeDamageInfo const&, trace_t *pTrace,*/ CBaseEntity *pOther )
{
	m_hEnemy = pOther;

	// Damage.
	CBaseEntity *pAttacker = GetOwnerEntity();
	IScorer *pScorerInterface = dynamic_cast<IScorer*>( pAttacker );
	if ( pScorerInterface )
		pAttacker = pScorerInterface->GetScorer();

	float flDamage = 15;

	CTakeDamageInfo info( GetOwnerEntity(), pAttacker, m_hLauncher.Get(), flDamage, DMG_DISSOLVE, TF_DMG_CUSTOM_PLASMA );
	pOther->TakeDamage( info );
	info.SetReportedPosition( pAttacker->GetAbsOrigin() );	
	EmitSound( "Halloween.spell_lightning_impact" );

	ApplyMultiDamage();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CBasePlayer *CTFProjectile_MechanicalArmOrb::GetScorer(void)
{
	return dynamic_cast<CBasePlayer *>(m_Scorer.Get());
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_MechanicalArmOrb::SetScorer( CBaseEntity *pScorer )
{
	m_Scorer = pScorer;
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_MechanicalArmOrb::ExplodeAndRemove( void )
{
	// Invisible.
	SetModelName( NULL_STRING );
	AddSolidFlags( FSOLID_NOT_SOLID );
	m_takedamage = DAMAGE_NO;

#ifdef CLIENT_DLL
	if ( GetTeamNumber() == TF_TEAM_RED )
		DispatchParticleEffect( "drg_cow_explosioncore_normal", WorldSpaceCenter(), vec3_angle );
	else
		DispatchParticleEffect( "drg_cow_explosioncore_normal_blue", WorldSpaceCenter(), vec3_angle );
#endif
	EmitSound( "Halloween.spell_lightning_impact" );

#ifdef GAME_DLL
	// Remove.
	UTIL_Remove( this );
	if ( tf_debug_damage.GetBool() )
		NDebugOverlay::Sphere( GetAbsOrigin(), GetAbsAngles(), TF_WEAPON_MECHANICALARM_RADIUS, 0, 100, 0, 0, false, 1 );
#endif
}

#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_MechanicalArmOrb::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		CreateTrails();
		CreateLightEffects();
	}

	/*if ( m_pOrbLightning )
	{
		for ( int i = 0; i < m_hZapTargets.Count(); i++ )
		{
			C_BaseEntity *pTarget = m_hZapTargets[i].Get();
			if ( pTarget )
				m_pOrbLightning->SetControlPoint( 1, pTarget->WorldSpaceCenter() );
			else
				m_pOrbLightning->SetControlPoint( 1, GetAbsOrigin() );
		}
	}*/

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
void CTFProjectile_MechanicalArmOrb::CreateTrails( void )
{
	if ( IsDormant() )
		return;

	m_pOrb = ParticleProp()->Create( GetTrailParticleName(), PATTACH_POINT_FOLLOW, "empty" );
	//m_pOrbLightning = ParticleProp()->Create( ConstructTeamParticle( "dxhr_lightningball_hit_%s", GetTeamNumber() ), PATTACH_POINT_FOLLOW, "empty" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_MechanicalArmOrb::CreateLightEffects( void )
{
	// Handle the dynamic light
	if ( tf2v_muzzlelight.GetBool() )
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
				dl->color.r = 255; dl->color.g = 30; dl->color.b = 10;
				break;

			case TF_TEAM_BLUE:
				dl->color.r = 10; dl->color.g = 30; dl->color.b = 255;
				break;
			}
			dl->radius = 300.0f;
			dl->die = gpGlobals->curtime + 0.1;

			//tempents->RocketFlare( GetAbsOrigin() );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFProjectile_MechanicalArmOrb::GetTrailParticleName( void )
{
	return ConstructTeamParticle( "dxhr_lightningball_parent_%s", GetTeamNumber() );
}
#endif