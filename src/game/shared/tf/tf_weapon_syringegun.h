//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_SYRINGEGUN_H
#define TF_WEAPON_SYRINGEGUN_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTFSyringeGun C_TFSyringeGun
#endif

//=============================================================================
//
// TF Weapon Syringe gun.
//
class CTFSyringeGun : public CTFWeaponBaseGun
{
public:

	DECLARE_CLASS( CTFSyringeGun, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

// Server specific.
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	CTFSyringeGun() {}
	~CTFSyringeGun() {}

	virtual void	Precache();
	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_SYRINGEGUN_MEDIC; }
	virtual float		GetSpeedMod( void ) const;

private:

	CTFSyringeGun( CTFSyringeGun const& );
};

// Temporary Crossbow, for now.

#if defined CLIENT_DLL
#define CTFCrossbow C_TFCrossbow
#endif

class CTFCrossbow : public CTFSyringeGun
{
public:

	DECLARE_CLASS( CTFCrossbow, CTFSyringeGun )
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	virtual int GetWeaponID( void ) const { return TF_WEAPON_CROSSBOW; }
};

#endif // TF_WEAPON_SYRINGEGUN_H
