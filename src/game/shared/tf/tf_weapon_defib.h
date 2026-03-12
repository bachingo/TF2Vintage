//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_DEFIB_H
#define TF_WEAPON_DEFIB_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_melee.h"

#ifdef CLIENT_DLL
#define CTFDefib C_TFDefib
#endif


//=============================================================================
//
// Bat class.
//
class CTFDefib : public CTFWeaponBaseMelee
{
public:

	DECLARE_CLASS( CTFDefib, CTFWeaponBaseMelee );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFDefib();
	
	void 				Smack( void );
	
#ifdef GAME_DLL
	bool		OnRagdollHit( CTFPlayer *pOwner, Vector vecHitPos );
	bool		RevivePlayer( CTFPlayer *pRevival, CTFPlayer *pOwner );
#endif
	
	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_DEFIB; }



private:

	CTFDefib( const CTFDefib & ) {}
};


#endif // TF_WEAPON_DEFIB_H