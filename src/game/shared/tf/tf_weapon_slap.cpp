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
// Weapon FireAxe tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFSlap, DT_TFWeaponSlap )

BEGIN_NETWORK_TABLE( CTFSlap, DT_TFWeaponSlap )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFSlap )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_slap, CTFSlap );

PRECACHE_WEAPON_REGISTER( tf_weapon_slap );

//-----------------------------------------------------------------------------
// Purpose: Sets up our attack.
//-----------------------------------------------------------------------------
void CTFSlap::PrimaryAttack()
{
	BaseClass::PrimaryAttack();
	PrimaryAttackFollowup();
}

//-----------------------------------------------------------------------------
// Purpose: Second slap.
//-----------------------------------------------------------------------------
void CTFSlap::PrimaryAttackFollowup()
{
	// Get the current player.
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	if ( !CanAttack() )
		return;
	
	// Set the weapon usage mode - primary, secondary.
	m_iWeaponMode = TF_WEAPON_SECONDARY_MODE;
	m_bConnected = false;

#ifdef GAME_DLL
	pPlayer->EndClassSpecialSkill();
#endif

	// Swing the weapon.
	SwingFollowup( pPlayer );

	m_bCurrentAttackIsMiniCrit = pPlayer->m_Shared.GetNextMeleeCrit() != kCritType_None;

#if !defined( CLIENT_DLL ) 
	pPlayer->SpeakWeaponFire();
	CTF_GameStats.Event_PlayerFiredWeapon( pPlayer, IsCurrentAttackACritical() );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Logic for second slap. We don't play a new animation/sound.
//-----------------------------------------------------------------------------
void CTFSlap::SwingFollowup(CTFPlayer *pPlayer)
{
	CalcIsAttackCritical();
	CalcIsAttackMiniCritical();

	// Set next attack times.
	float flFireDelay = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeFireDelay;
	CALL_ATTRIB_HOOK_FLOAT( flFireDelay, mult_postfiredelay );

	m_flNextPrimaryAttack = gpGlobals->curtime + flFireDelay;

	SetWeaponIdleTime( m_flNextPrimaryAttack + m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeIdleEmpty );

	m_flSmackTime = gpGlobals->curtime + m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flSmackDelay;
}