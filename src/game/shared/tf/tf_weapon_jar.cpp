//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_jar.h"
#include "in_buttons.h"
#include "engine/IEngineSound.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
// Server specific.
#else
#include "te_effect_dispatch.h"
#include "tf_player.h"
#include "tf_gamestats.h"
#endif

#define TF_JAR_DAMAGE 0.0f
#define TF_JAR_VEL 1000.0f
#define TF_JAR_GRAV 1.0f

IMPLEMENT_NETWORKCLASS_ALIASED( TFJar, DT_WeaponJar )

BEGIN_NETWORK_TABLE( CTFJar, DT_WeaponJar )
#ifdef CLIENT_DLL
#else
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFJar )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_jar, CTFJar );
PRECACHE_WEAPON_REGISTER( tf_weapon_jar );

// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC( CTFJar )
END_DATADESC()
#endif

CTFJar::CTFJar()
{
	m_flEffectBarRegenTime = 0.0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFJar::Precache( void )
{
	BaseClass::Precache();
	PrecacheParticleSystem( "peejar_drips" );
}

// ---------------------------------------------------------------------------- -
// Purpose: 
//-----------------------------------------------------------------------------
void CTFJar::PrimaryAttack( void )
{
	// Get the player owning the weapon.
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	if ( !CanAttack() )
		return;

	if ( !HasAmmo() )
		return;

	CalcIsAttackCritical();

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );

	pPlayer->SetAnimation( PLAYER_ATTACK1 );
	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );

	FireProjectile( pPlayer );

#if !defined( CLIENT_DLL ) 
	pPlayer->SpeakWeaponFire( MP_CONCEPT_JARATE_LAUNCH );
	CTF_GameStats.Event_PlayerFiredWeapon( pPlayer, IsCurrentAttackACrit() );
#endif

	// Set next attack times.
	float flDelay = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeFireDelay;
	CALL_ATTRIB_HOOK_FLOAT( flDelay, mult_postfiredelay );
	m_flNextPrimaryAttack = gpGlobals->curtime + flDelay;

	SetWeaponIdleTime( gpGlobals->curtime + SequenceDuration() );

	m_flEffectBarRegenTime = gpGlobals->curtime + InternalGetEffectBarRechargeTime();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFJar::GetProjectileDamage( void )
{
	return TF_JAR_DAMAGE;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFJar::GetProjectileSpeed( void )
{
	return TF_JAR_VEL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFJar::GetProjectileGravity( void )
{
	return TF_JAR_GRAV;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFJar::CalcIsAttackCriticalHelper( void )
{
	// No random critical hits.
	return false;
}

//=============================================================================
//
// Weapon JarMilk
//

IMPLEMENT_NETWORKCLASS_ALIASED( TFJarMilk, DT_WeaponJarMilk )

BEGIN_NETWORK_TABLE( CTFJar, DT_WeaponJarMilk )
#ifdef CLIENT_DLL
#else
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFJarMilk )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_jar_milk, CTFJarMilk );
PRECACHE_WEAPON_REGISTER( tf_weapon_jar_milk );

// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC( CTFJarMilk )
END_DATADESC()
#endif

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: Kill splash particles 
//-----------------------------------------------------------------------------
bool C_TFJarMilk::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	C_TFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer && pPlayer->IsLocalPlayer() )
	{
		C_BaseViewModel *vm = pPlayer->GetViewModel();
		if ( vm )
		{
			pPlayer->StopViewModelParticles( vm );
		}
	}

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: Override energydrink particles
//-----------------------------------------------------------------------------
const char* C_TFJarMilk::ModifyEventParticles( const char* token )
{
	if ( !V_stricmp( token, "energydrink_splash" ) )
	{
		CEconItemDefinition *pStatic = GetItem()->GetStaticData();
		if ( pStatic )
		{
			PerTeamVisuals_t *pVisuals = pStatic->GetVisuals( GetTeamNumber() );
			if ( pVisuals )
			{
				const char *pszCustomEffectName = pVisuals->custom_particlesystem;
				if ( pszCustomEffectName[0] != '\0' )
				{
					return pszCustomEffectName;
				}
			}
		}
	}

	return token;
}
#endif

//=============================================================================
//
// Weapon Cleaver
//

IMPLEMENT_NETWORKCLASS_ALIASED(TFCleaver, DT_WeaponCleaver)

BEGIN_NETWORK_TABLE( CTFJar, DT_WeaponCleaver )
#ifdef CLIENT_DLL
#else
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFCleaver )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_cleaver, CTFCleaver );
PRECACHE_WEAPON_REGISTER( tf_weapon_cleaver );

// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC( CTFCleaver )
END_DATADESC()
#endif


//=============================================================================
//
// Weapon JarGas
//

IMPLEMENT_NETWORKCLASS_ALIASED( TFJarGas, DT_WeaponJarGas )

BEGIN_NETWORK_TABLE( CTFJar, DT_WeaponJarGas )
#ifdef CLIENT_DLL
#else
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFJarGas )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_jar_gas, CTFJarGas );
PRECACHE_WEAPON_REGISTER( tf_weapon_jar_gas );

// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC( CTFJarGas )
END_DATADESC()
#endif