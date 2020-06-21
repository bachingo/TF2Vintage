//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
//
//=============================================================================
#ifndef TF_WEAPON_GRENADELAUNCHER_H
#define TF_WEAPON_GRENADELAUNCHER_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"
#include "tf_weaponbase_grenadeproj.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTFGrenadeLauncher C_TFGrenadeLauncher
#endif

#define TF_GRENADE_LAUNCHER_XBOX_CLIP 6

//=============================================================================
//
// TF Weapon Grenade Launcher.
//
class CTFGrenadeLauncher : public CTFWeaponBaseGun, public ITFChargeUpWeapon
{
public:

	DECLARE_CLASS( CTFGrenadeLauncher, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	// Server specific.
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	CTFGrenadeLauncher();
	~CTFGrenadeLauncher();

	virtual void	Spawn( void );
	virtual void	Precache();
	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_GRENADELAUNCHER; }
	virtual void	SecondaryAttack();

	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo );
	virtual bool	Deploy( void );
	virtual void	WeaponReset(void);
	virtual void	PrimaryAttack( void );
	virtual void	WeaponIdle( void );
	virtual float	GetProjectileSpeed( void );

	virtual bool	Reload( void );

	virtual int GetMaxClip1( void ) const;
	virtual int GetDefaultClip1( void ) const;

	virtual void SwitchBodyGroups( void );

	int GetDetonateMode( void ) const;

	// Mortar.
	bool IsMortar(void) const;
	int  MortarTime(void);
	
	// ITFChargeUpWeapon
	// These are inverted compared to the regular to compensate for HUD.
	virtual float	GetChargeBeginTime(void) { return m_flChargeBeginTime + MortarTime(); }
	virtual float	GetChargeMaxTime( void ) { return m_flChargeBeginTime; }
	
	float	m_flChargeBeginTime;

#ifdef CLIENT_DLL
	void				ToggleCannonFuse();
	CNewParticleEffect	*m_pCannonFuse;
#endif

	// Donk table.
	struct Donk_t
	{
		CHandle <CBaseEntity> m_hDonk;
		float m_flDonkTime;
	};

public:

	CBaseEntity *FireProjectileInternal( CTFPlayer *pPlayer );
	void LaunchGrenade( void );

private:

	CTFGrenadeLauncher( const CTFGrenadeLauncher & ) {}
};

// Cannon.

#if defined CLIENT_DLL
#define CTFGrenadeLauncher_Cannon C_TFGrenadeLauncher_Cannon
#endif

class CTFGrenadeLauncher_Cannon : public CTFGrenadeLauncher
{
public:

	DECLARE_CLASS( CTFGrenadeLauncher_Cannon, CTFGrenadeLauncher )
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	virtual int GetWeaponID( void ) const { return TF_WEAPON_CANNON; }
};

// Old School Grenade Launcher.

#if defined CLIENT_DLL
#define CTFGrenadeLauncher_Legacy C_TFGrenadeLauncher_Legacy
#endif

class CTFGrenadeLauncher_Legacy : public CTFGrenadeLauncher
{
public:

	DECLARE_CLASS( CTFGrenadeLauncher_Legacy, CTFGrenadeLauncher )
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	virtual int GetWeaponID( void ) const { return TF_WEAPON_GRENADELAUNCHER_LEGACY; }
};

#endif // TF_WEAPON_GRENADELAUNCHER_H