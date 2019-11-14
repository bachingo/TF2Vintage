//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_SHOVEL_H
#define TF_WEAPON_SHOVEL_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_melee.h"

#ifdef CLIENT_DLL
#define CTFShovel C_TFShovel
#endif

//=============================================================================
//
// Shovel class.
//
class CTFShovel : public CTFWeaponBaseMelee
{
public:

	DECLARE_CLASS( CTFShovel, CTFWeaponBaseMelee );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFShovel();
	virtual int			GetWeaponID( void ) const { return TF_WEAPON_SHOVEL; }
	virtual int			GetCustomDamageType() const;
	virtual float		GetSpeedMod( void ) const;
	virtual float		GetMeleeDamage( CBaseEntity *pTarget, int &iCustomDamage );

private:

	CTFShovel( const CTFShovel & ) {}
};

// Shovel Fist, for use with SAXTON HALE.

#if defined CLIENT_DLL
#define CTFShovelFist C_TFShovelFist
#endif

class CTFShovelFist : public CTFShovel
{
public:

	DECLARE_CLASS( CTFShovelFist, CTFShovel )
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	virtual int GetWeaponID( void ) const { return TF_WEAPON_SHOVELFIST; }
};


#endif // TF_WEAPON_SHOVEL_H
