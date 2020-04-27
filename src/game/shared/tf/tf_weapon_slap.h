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
	
#ifdef GAME_DLL

	CTFSlap();

	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_SLAP; }
	virtual void		PrimaryAttack();
	virtual void		ItemPostFrame( void );
	virtual void		WeaponIdle( void );
	virtual bool CanHolster( void ) const;
	virtual void		PrimaryAttackFollowup();

private:

	CTFSlap( const CTFSlap & ) {}
	float m_flNextSlapTime;
	bool  m_bWaitforSecondSlap;
#endif

};

#endif // TF_WEAPON_SLAP_H
