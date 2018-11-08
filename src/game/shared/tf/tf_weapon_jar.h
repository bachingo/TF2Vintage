//=============================================================================
//
// Purpose: A remake of Huntsman from live TF2.
//
//=============================================================================
#ifndef TF_WEAPON_JAR_H
#define TF_WEAPON_JAR_H

#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"

#ifdef CLIENT_DLL
#define CTFJar C_TFJar
#endif

class CTFJar : public CTFWeaponBaseGun
{
public:
	DECLARE_CLASS( CTFJar, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	// Server specific.
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	CTFJar();

	virtual int			GetWeaponID( void ) const 			{ return TF_WEAPON_JAR; }

	virtual void		Precache( void );

	virtual void		PrimaryAttack( void );

	virtual float		GetProjectileDamage( void );
	virtual float		GetProjectileSpeed( void );
	virtual float		GetProjectileGravity( void );
	virtual bool		CalcIsAttackCriticalHelper( void );

	virtual bool		HasChargeBar( void )				{ return true; }
	virtual const char* GetEffectLabelText( void )			{ return "#TF_Jar"; }
	virtual float		InternalGetEffectBarRechargeTime()	{ return 20.0; }
};

#endif // TF_WEAPON_JAR_H
