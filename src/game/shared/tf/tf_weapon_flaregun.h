//====== Copyright © 1996-2013, Valve Corporation, All rights reserved. =======
//
// Purpose: A remake of Pyro's flaregun from live TF2s
//
//=============================================================================
#ifndef TF_WEAPON_FLAREGUN_H
#define TF_WEAPON_FLAREGUN_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"
#include "tf_weaponbase_grenadeproj.h"
#include "tf_projectile_flare.h"

#ifdef CLIENT_DLL
#define CTFFlareGun C_TFFlareGun
#endif

class CTFFlareGun : public CTFWeaponBaseGun
{
public:
	DECLARE_CLASS(CTFFlareGun, CTFWeaponBaseGun);
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	// Server specific.
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	CTFFlareGun();

	virtual void	Spawn( void );
	virtual void	PrimaryAttack( void );
	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_FLAREGUN; }
	virtual void	SecondaryAttack();
	bool			HasKnockback() const;
	
#ifdef GAME_DLL
	virtual void	ApplyPostOnHitAttributes( CTakeDamageInfo const &info, CTFPlayer *pVictim );
#endif

	// Used for tracking flares for Detonator.
	typedef CHandle<CTFProjectile_Flare>	FlareHandle;
	CUtlVector<FlareHandle> m_Flares;
};

#endif // TF_WEAPON_FLAREGUN_H