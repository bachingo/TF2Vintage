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
#define CTFJarMilk C_TFJarMilk
#define CTFCleaver C_TFCleaver
#define CTFJarGas C_TFJarGas
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

// Mad Milk Weapon.

class CTFJarMilk : public CTFJar
{
public:
	DECLARE_CLASS( CTFJarMilk, CTFJar );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	// Server specific.
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	virtual int			GetWeaponID( void ) const 			{ return TF_WEAPON_JAR_MILK; }

#ifndef GAME_DLL
	virtual bool Holster( CBaseCombatWeapon *pSwitchingTo );
	virtual const char* ModifyEventParticles( const char* token );
#endif
};

// Flying Guillotine Weapon.

class CTFCleaver : public CTFJar
{
public:
	DECLARE_CLASS( CTFCleaver, CTFJar );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	// Server specific.
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif
	
	virtual float		GetProjectileDamage( void )			{ return 50.0; }
	virtual float		GetProjectileSpeed(void)			{ return 3000.0; }
	virtual float		GetProjectileGravity(void)			{ return 0.5f; } // Same as pipebomb

	virtual int			GetWeaponID( void ) const 			{ return TF_WEAPON_GRENADE_CLEAVER; }
	virtual const char* GetEffectLabelText( void )			{ return "#TF_Cleaver"; }
	virtual float		InternalGetEffectBarRechargeTime()	{ return 6.0; }

};

// Gas Passer Weapon.

class CTFJarGas : public CTFJar
{
public:
	DECLARE_CLASS( CTFJarGas, CTFJar );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	// Server specific.
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	virtual int			GetWeaponID( void ) const 			{ return TF_WEAPON_JAR_GAS; }
	virtual const char* GetEffectLabelText( void )			{ return "#TF_Gas"; }
	virtual float		InternalGetEffectBarRechargeTime()	{ return 30.0; }

};

#endif // TF_WEAPON_JAR_H
