//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_slap.h"


// Server specific.
#if !defined( CLIENT_DLL )
#include "tf_player.h"
#include "tf_gamestats.h"
#include "ilagcompensationmanager.h"
// Client specific.
#else
#include "c_tf_player.h"
#endif

//=============================================================================
//
// Weapon Slapper table tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFSlap, DT_TFWeaponSlap )

BEGIN_NETWORK_TABLE( CTFSlap, DT_TFWeaponSlap )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFSlap )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_slap, CTFSlap );

PRECACHE_WEAPON_REGISTER( tf_weapon_slap );

#define TF_NEXT_SLAP_TIME 0.5f

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: Sets up our attack.
//-----------------------------------------------------------------------------
CTFSlap::CTFSlap()
{
	m_flNextSlapTime = 0;
	m_bWaitforSecondSlap = false;
}

//-----------------------------------------------------------------------------
// Purpose: Sets up our attack.
//-----------------------------------------------------------------------------
void CTFSlap::PrimaryAttack( void )
{
	BaseClass::PrimaryAttack();
	m_flNextSlapTime = gpGlobals->curtime + TF_NEXT_SLAP_TIME;
	m_bWaitforSecondSlap = true;
}

//-----------------------------------------------------------------------------
// Purpose: Used for checking if we should start the second slap attack trace.
//-----------------------------------------------------------------------------
void CTFSlap::ItemPostFrame( void )
{
	if ( ( ( m_flNextSlapTime > 0 ) && ( gpGlobals->curtime >= m_flNextSlapTime ) ) && m_bWaitforSecondSlap ) 
		PrimaryAttackFollowup();
	return BaseClass::ItemPostFrame();
}
void CTFSlap::WeaponIdle( void )
{
	if ( ( ( m_flNextSlapTime > 0 ) && ( gpGlobals->curtime >= m_flNextSlapTime ) ) && m_bWaitforSecondSlap ) 
		PrimaryAttackFollowup();
	return BaseClass::WeaponIdle();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFSlap::CanHolster( void ) const
{
	// Haven't slapped again, wait.
	if (gpGlobals->curtime <= m_flNextSlapTime)
		return false;
	
	return BaseClass::CanHolster();
}

//-----------------------------------------------------------------------------
// Purpose: Second slap trace. Do this ~0.5s after the first slap.
//-----------------------------------------------------------------------------
void CTFSlap::PrimaryAttackFollowup( void )
{
	m_bWaitforSecondSlap = false;
	
	// Get the current player.
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	if ( !CanAttack() )
		return;
	
	// Set the weapon usage mode - primary, secondary.
	m_iWeaponMode = TF_WEAPON_SECONDARY_MODE;
	m_bConnected = false;

	pPlayer->EndClassSpecialSkill();
	
	// Swing the weapon.
	CalcIsAttackCritical();
	CalcIsAttackMiniCritical();

	// Set next attack times.
	float flFireDelay = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeFireDelay;
	CALL_ATTRIB_HOOK_FLOAT( flFireDelay, mult_postfiredelay );

	m_flNextPrimaryAttack = gpGlobals->curtime + flFireDelay;

	SetWeaponIdleTime( m_flNextPrimaryAttack + m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeIdleEmpty );

	m_flSmackTime = gpGlobals->curtime + m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flSmackDelay;

	m_bCurrentAttackIsMiniCrit = pPlayer->m_Shared.GetNextMeleeCrit() != kCritType_None;

	pPlayer->SpeakWeaponFire();
	CTF_GameStats.Event_PlayerFiredWeapon( pPlayer, IsCurrentAttackACritical() );
}

#endif