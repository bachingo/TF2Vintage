//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_SLAP_H
#define TF_WEAPON_SLAP_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_melee.h"

#ifdef CLIENT_DLL
#define CTFSlap C_TFSlap
#endif

//=============================================================================
//
// BrandingIron class.
//
class CTFSlap : public CTFWeaponBaseMelee
{
public:

	DECLARE_CLASS( CTFSlap, CTFWeaponBaseMelee );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFSlap() {}
	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_SLAP; }
	virtual void		PrimaryAttack();
	virtual void		PrimaryAttackFollowup();
	virtual void		SwingFollowup( CTFPlayer *pPlayer );

private:

	CTFSlap( const CTFSlap & ) {}
};

#endif // TF_WEAPON_SLAP_H
