//====== Copyright © 1996-2013, Valve Corporation, All rights reserved. =======
//
// Purpose: A remake of the Bonk! Atomic Punch from live TF2
//
//=============================================================================
#ifndef TF_WEAPON_LUNCHBOX_DRINK_H
#define TF_WEAPON_LUNCHBOX_DRINK_H

#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase.h"

#ifdef CLIENT_DLL
#define CTFLunchBox_Drink C_TFLunchBox_Drink
#endif

class CTFLunchBox_Drink : public CTFWeaponBase
{
public:
	DECLARE_CLASS( CTFLunchBox_Drink, CTFWeaponBase );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	virtual int 		GetWeaponID() const 						{ return TF_WEAPON_LUNCHBOX_DRINK; }

	virtual bool		ShouldBlockPrimaryFire( void ) 				{ return true; }

	virtual void		PrimaryAttack( void );

	virtual void		DepleteAmmo( void );

	virtual bool		HasChargeBar( void ) 						{ return true; }
	virtual float		InternalGetEffectBarRechargeTime( void ) 	{ return 22.2f; }
	virtual const char*	GetEffectLabelText( void )					{ return "#TF_EnergyDrink"; }

#ifndef GAME_DLL
	virtual bool		Holster( CBaseCombatWeapon *pSwitchingTo );
#endif
};

#endif // TF_WEAPON_LUNCHBOX_DRINK_H
