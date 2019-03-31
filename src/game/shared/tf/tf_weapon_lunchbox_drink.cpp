//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_lunchbox_drink.h"

#ifdef GAME_DLL
#include "tf_player.h"
#else
#include "c_tf_player.h"
#endif

CREATE_SIMPLE_WEAPON_TABLE( TFLunchBox_Drink, tf_weapon_lunchbox_drink )

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFLunchBox_Drink::PrimaryAttack( void )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
	{
		return;
	}

#ifdef GAME_DLL
	pOwner->Taunt();
#endif

	m_flNextPrimaryAttack = gpGlobals->curtime + 0.5f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFLunchBox_Drink::DepleteAmmo( void )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
	{
		return;
	}

	// Switch away from it immediately, don't want it to stick around.
	pOwner->RemoveAmmo( 1, m_iPrimaryAmmoType );
	pOwner->SwitchToNextBestWeapon( this );

	StartEffectBarRegen();
}

#ifndef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: Kill splash particles 
//-----------------------------------------------------------------------------
bool C_TFLunchBox_Drink::Holster( CBaseCombatWeapon *pSwitchingTo )
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
#endif
