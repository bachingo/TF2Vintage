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


