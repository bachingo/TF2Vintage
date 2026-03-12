//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Cow Mangler (Particle Cannon)
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_particlecannon.h"
#include "tf_fx_shared.h"
#include "in_buttons.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
// Server specific.
#else
#include "tf_gamestats.h"
#include "tf_player.h"
#endif

#define TF_CANNON_CHARGEUP_TIME 2.0f

//=============================================================================
//
// Particle Cannon Table.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFParticleCannon, DT_ParticleCannon )

BEGIN_NETWORK_TABLE( CTFParticleCannon, DT_ParticleCannon )
#ifndef CLIENT_DLL
	SendPropTime( SENDINFO( m_flChargeBeginTime ) ),
	SendPropInt( SENDINFO( m_nChargeEffectParity ) ),
#else
	RecvPropTime( RECVINFO( m_flChargeBeginTime ) ),
	RecvPropInt( RECVINFO( m_nChargeEffectParity ) ),
#endif
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tf_weapon_particle_cannon, CTFParticleCannon );
PRECACHE_WEAPON_REGISTER( tf_weapon_particle_cannon );

// Client specific.

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CTFParticleCannon )
	DEFINE_PRED_FIELD( m_flChargeBeginTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()
#endif

// Server specific.

#ifdef GAME_DLL
BEGIN_DATADESC(CTFParticleCannon)
END_DATADESC()
#endif


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFParticleCannon::CTFParticleCannon()
{
#ifdef CLIENT_DLL
	m_nOldChargeEffectParity = 0;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFParticleCannon::Precache()
{
	BaseClass::Precache();

	PrecacheParticleSystem( "drg_cow_explosioncore_charged" );
	PrecacheParticleSystem( "drg_cow_explosioncore_charged_blue" );
	PrecacheParticleSystem( "drg_cow_explosioncore_normal" );
	PrecacheParticleSystem( "drg_cow_explosioncore_normal_blue" );
	PrecacheParticleSystem( "drg_cow_muzzleflash_charged" );
	PrecacheParticleSystem( "drg_cow_muzzleflash_charged_blue" );
	PrecacheParticleSystem( "drg_cow_muzzleflash_normal" );
	PrecacheParticleSystem( "drg_cow_muzzleflash_normal_blue" );
	PrecacheParticleSystem( "drg_cow_idle" );

	PrecacheScriptSound( "Weapon_CowMangler.ReloadFinal" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFParticleCannon::PlayWeaponShootSound( void )
{
	WeaponSound( SINGLE );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
char const *CTFParticleCannon::GetShootSound( int iIndex ) const
{
	if ( iIndex != RELOAD )
		return BaseClass::GetShootSound( iIndex );

	const float flEnergyAfterReload = Energy_GetEnergy() + Energy_GetRechargeCost();
	if ( flEnergyAfterReload == Energy_GetMaxEnergy() )
		return "Weapon_CowMangler.ReloadFinal";

	return BaseClass::GetShootSound( iIndex );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFParticleCannon::GetMuzzleFlashParticleEffect( void )
{
	if ( m_flChargeBeginTime > 0 )
	{
		return ( GetTeamNumber() == TF_TEAM_RED ) ? "drg_cow_muzzleflash_charged" : "drg_cow_muzzleflash_charged_blue";
	}
	else
	{
		return ( GetTeamNumber() == TF_TEAM_RED ) ? "drg_cow_muzzleflash_normal" : "drg_cow_muzzleflash_normal_blue";
	}
}

//-----------------------------------------------------------------------------
// Purpose: No holstering while charging a shot.
//-----------------------------------------------------------------------------
bool CTFParticleCannon::CanHolster( void ) const
{
	if ( m_flChargeBeginTime > 0 )
		return false;
	
	return BaseClass::CanHolster();
}

//-----------------------------------------------------------------------------
// Purpose: Reset the charge when we holster
//-----------------------------------------------------------------------------
bool CTFParticleCannon::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( pPlayer && pPlayer->m_Shared.InCond( TF_COND_AIMING ) )
		return false;

	m_flChargeBeginTime = 0;

#ifdef CLIENT_DLL
	GetViewmodelAddon()->ParticleProp()->StopParticlesNamed( "drg_cowmangler_idle", true );
#endif

	StopSound( "Weapon_CowMangler.Charging" );

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: Reset the charge when we deploy
//-----------------------------------------------------------------------------
bool CTFParticleCannon::Deploy( void )
{
	m_flChargeBeginTime = 0;

#ifdef CLIENT_DLL
	SetContextThink( &CTFParticleCannon::ClientEffectsThink, gpGlobals->curtime + rand() % 5, "PC_EFFECTS_THINK" );
#endif

	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFParticleCannon::ItemPostFrame( void )
{
	BaseClass::ItemPostFrame();

	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner )
		return;

	if ( ( m_flChargeBeginTime > 0 ) && ( m_flChargeBeginTime + TF_CANNON_CHARGEUP_TIME < gpGlobals->curtime ) )
	{
		FireChargeShot();
	}

#ifdef CLIENT_DLL
	if ( WeaponState() == WEAPON_IS_ACTIVE )
	{
		if ( GetIndexForThinkContext( "PC_EFFECTS_THINK" ) == NO_THINK_CONTEXT )
			SetContextThink( &CTFParticleCannon::ClientEffectsThink, gpGlobals->curtime + rand() % 5, "PC_EFFECTS_THINK" );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFParticleCannon::PrimaryAttack( void )
{
	// Have energy to shoot?
	if ( !Energy_HasEnergy() )
		return;

	// Charging special attack?
	if ( m_flChargeBeginTime > 0 )
		return;

	BaseClass::PrimaryAttack();
}
//-----------------------------------------------------------------------------
// Purpose: Starts the charge up on the Cow Mangler.
//-----------------------------------------------------------------------------
void CTFParticleCannon::SecondaryAttack( void )
{
	// Need to have the attribute to use this.
	if ( !CanChargeShot() )
		return;
	
	// Are we capable of firing again?
	if ( m_flNextSecondaryAttack > gpGlobals->curtime )
		return;

	// Already charging?
	if ( m_flChargeBeginTime > 0 )
		return;
	
	// Do we have full energy to launch a charged shot?
	if ( !Energy_FullyCharged() )
	{
		Reload();
		return;
	}

	if ( !CanAttack() )
	{
		m_flChargeBeginTime = 0;
		return;
	}
	
	// Set our charging time, and prevent us attacking until it is done.
	m_flChargeBeginTime = gpGlobals->curtime;
	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;

	SendWeaponAnim( ACT_PRIMARY_VM_PRIMARYATTACK_3 );
	
	// Slow down the player.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( pPlayer)
	{
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY_SUPER );
		pPlayer->m_Shared.AddCond( TF_COND_AIMING );
		pPlayer->TeamFortress_SetSpeed();
	}
	
	WeaponSound( SPECIAL1 );

	m_nChargeEffectParity = m_nChargeEffectParity + 1;
}

void CTFParticleCannon::WeaponReset( void )
{
	BaseClass::WeaponReset();
	m_flChargeBeginTime = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Checks if we're allowed to charge up a shot.
//-----------------------------------------------------------------------------
bool CTFParticleCannon::CanChargeShot( void )
{
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( pPlayer == NULL )
		return false;
	
	int nHasEnergyChargeUp = 0;
	CALL_ATTRIB_HOOK_INT( nHasEnergyChargeUp, energy_weapon_charged_shot );
	
	return ( nHasEnergyChargeUp != 0 );
}

//-----------------------------------------------------------------------------
// Purpose: Launches a powerful energy ball that can stun buildings.
//-----------------------------------------------------------------------------
void CTFParticleCannon::FireChargeShot( void )
{
	// Remove the slowdown.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( pPlayer )
	{
		pPlayer->m_Shared.RemoveCond( TF_COND_AIMING );
		pPlayer->TeamFortress_SetSpeed();
	}
	
#ifdef GAME_DLL
	CTF_GameStats.Event_PlayerFiredWeapon( pPlayer, false );
#endif

	// We use a special Energy Ball call here to do the charged variant.
	FireEnergyBall( pPlayer, true );

	float flFireDelay = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeFireDelay;
	CALL_ATTRIB_HOOK_FLOAT( flFireDelay, mult_postfiredelay );

	m_flNextPrimaryAttack = gpGlobals->curtime + flFireDelay;
	SetWeaponIdleTime( gpGlobals->curtime + SequenceDuration() );
	
	m_iReloadMode = TF_RELOAD_START;
	m_flChargeBeginTime = 0;

	Energy_SetEnergy( 0 );
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFParticleCannon::OwnerCanTaunt( void ) const
{
	if ( m_flChargeBeginTime > 0 )
		return false;

	return true;
}
#else
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFParticleCannon::ClientEffectsThink( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	if ( !pPlayer->IsLocalPlayer() )
		return;

	if ( !pPlayer->GetViewModel() )
		return;

	if ( WeaponState() != WEAPON_IS_ACTIVE )
		return;

	const int nRandomTime = 2 + rand() % 5;
	SetContextThink( &CTFParticleCannon::ClientEffectsThink, gpGlobals->curtime + nRandomTime, "PC_EFFECTS_THINK" );

	if ( pPlayer->m_Shared.InCond( TF_COND_TAUNTING ) )
		return;

	const char *pszAttachPoint;
	switch ( rand() % 4 )
	{
		case 1:
			pszAttachPoint = "crit_frontspark2";
			break;
		case 2:
			pszAttachPoint = "crit_frontspark3";
			break;
		case 3:
			pszAttachPoint = "crit_frontspark4";
			break;
		default:
			pszAttachPoint = "crit_frontspark1";
			break;
	}

	const char *pszIdleParticle = ( GetTeamNumber() == TF_TEAM_RED ) ? "drg_cow_idle" : "drg_cow_idle_blue";
	CNewParticleEffect *pEffect = GetViewmodelAddon()->ParticleProp()->Create( pszIdleParticle, PATTACH_POINT_FOLLOW, pszAttachPoint );
	if ( pEffect )
	{
		pEffect->SetControlPoint( CUSTOM_COLOR_CP1, GetEnergyWeaponColor( false ) );
		pEffect->SetControlPoint( CUSTOM_COLOR_CP2, GetEnergyWeaponColor( true ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFParticleCannon::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( IsCarrierAlive() && ( WeaponState() == WEAPON_IS_ACTIVE ) )
	{
		if ( m_nChargeEffectParity != m_nOldChargeEffectParity )
		{
			CreateChargeEffect();
			m_nOldChargeEffectParity = m_nChargeEffectParity;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFParticleCannon::DispatchMuzzleFlash( const char* effectName, C_BaseEntity* pAttachEnt )
{
	DispatchParticleEffect( effectName, PATTACH_POINT_FOLLOW, pAttachEnt, "muzzle", GetEnergyWeaponColor( false ), GetEnergyWeaponColor( true ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFParticleCannon::CreateMuzzleFlashEffects( C_BaseEntity *pAttachEnt, int nIndex )
{
	// Call the muzzle flash, but don't use the rocket launcher's backblast version.
	CTFWeaponBase::CreateMuzzleFlashEffects( pAttachEnt, nIndex );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFParticleCannon::CreateChargeEffect()
{
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( pPlayer )
	{
		DispatchParticleEffect( "drg_cowmangler_muzzleflash_chargeup", PATTACH_POINT_FOLLOW, GetAppropriateWorldOrViewModel(), "muzzle", GetEnergyWeaponColor( false ), GetEnergyWeaponColor( true ) );
	}
}

#endif