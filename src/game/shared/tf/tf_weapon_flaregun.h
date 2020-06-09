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
#define CTFFlareGunRevenge C_TFFlareGunRevenge
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
	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_FLAREGUN; }
	virtual void	SecondaryAttack();
	void			AddFlare(CTFProjectile_Flare *pFlare);
	void			DeathNotice( CBaseEntity *pVictim );
	bool			HasKnockback() const;

	// Used for tracking flares for Detonator.
	typedef CHandle<CTFProjectile_Flare>	FlareHandle;
	CUtlVector<FlareHandle> m_Flares;
};

class CTFFlareGunRevenge : public CTFWeaponBaseGun
{
public:
	DECLARE_CLASS(CTFFlareGunRevenge, CTFFlareGun);
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	// Server specific.
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	CTFFlareGunRevenge();
	
	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_FLAREGUN_REVENGE; }
	virtual bool	Deploy( void );
	virtual bool	Holster( CBaseCombatWeapon *pSwitchTo );
	virtual void	ItemPostFrame(void);
	virtual void	PrimaryAttack( void );
	virtual void	SecondaryAttack( void );
	int				GetCount( void );
	
	bool				HasChargeBar( void );
	virtual const char *GetEffectLabelText( void ) { return "#TF_CRITS"; }
	
	bool			bWaitingtoFire;
	bool			CanGetAirblastCrits( void ) const;
	
#ifdef CLIENT_DLL
	CNewParticleEffect *m_pVacuumEffect;
#endif
};

#endif // TF_WEAPON_FLAREGUN_H