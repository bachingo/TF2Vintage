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
	SendPropFloat( SENDINFO( m_flChargeBeginTime ), 0, SPROP_NOSCALE ),
#else
	RecvPropFloat( RECVINFO( m_flChargeBeginTime ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFParticleCannon )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_particle_cannon, CTFParticleCannon );
PRECACHE_WEAPON_REGISTER( tf_weapon_particle_cannon );

// Server specific.

#ifdef GAME_DLL
BEGIN_DATADESC(CTFParticleCannon)
END_DATADESC()
#endif


//-----------------------------------------------------------------------------
// Purpose: No holstering while charging a shot.
//-----------------------------------------------------------------------------
bool CTFParticleCannon::CanHolster( void )
{
	if ( m_flChargeBeginTime > 0 )
		return false;
	
	return BaseClass::CanHolster();
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
	
	WeaponSound(SPECIAL1);
}

//-----------------------------------------------------------------------------
// Purpose: Checks if we're allowed to charge up a shot.
//-----------------------------------------------------------------------------
bool CTFParticleCannon::CanChargeShot( void )
{
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer)
		return false;
	
	int nHasEnergyChargeUp = 0;
	CALL_ATTRIB_HOOK_INT(nHasEnergyChargeUp, energy_weapon_charged_shot);
	
	return (nHasEnergyChargeUp != 0);
}

//-----------------------------------------------------------------------------
// Purpose: Launches a powerful energy ball that can stun buildings.
//-----------------------------------------------------------------------------
void CTFParticleCannon::FireChargeShot( void )
{
	// Remove the slowdown.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( pPlayer)
	{
		if (pPlayer->m_Shared.InCond(TF_COND_AIMING))
			pPlayer->m_Shared.RemoveCond(TF_COND_AIMING);
	}
	
	// We use a special Energy Ball call here to do the charged variant.
	BaseClass::FireEnergyBall( pPlayer, true );
	
	// Drain energy, GetShotCost will handle amount
	Energy_DrainEnergy();
	
	m_flChargeBeginTime = 0;
}

float CTFParticleCannon::Energy_GetShotCost( void ) const
{
	// if we were charging an attack, drain our entire battery
	if ( m_flChargeBeginTime > 0 )
		return Energy_GetMaxEnergy();

	return 5.0f;
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
void CTFParticleCannon::CreateMuzzleFlashEffects( C_BaseEntity *pAttachEnt, int nIndex )
{
	// Call the muzzle flash, but don't use the rocket launcher's backblast version.
	CTFWeaponBase::CreateMuzzleFlashEffects( pAttachEnt, nIndex );
}



#endif