//====== Copyright ? 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_SYRINGE_H
#define TF_WEAPON_SYRINGE_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_melee.h"

#ifdef CLIENT_DLL
#define CTFSyringe C_TFSyringe
#endif

//=============================================================================
//
// Club class.
//
class CTFSyringe : public CTFWeaponBaseMelee
{
public:

	DECLARE_CLASS(CTFSyringe, CTFWeaponBaseMelee);
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CTFSyringe();
	virtual int			GetWeaponID(void) const			{ return TF_WEAPON_SYRINGE; }
	virtual void		Smack( void );

	bool CanBeHealed( CBaseEntity *pTarget ) const;

private:
	bool HitPlayer( CBaseEntity *pTarget );

	CTFSyringe( CTFSyringe const& );
};

#endif // TF_WEAPON_SYRINGE_H