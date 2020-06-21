//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_PIPEBOMBLAUNCHER_H
#define TF_WEAPON_PIPEBOMBLAUNCHER_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"
#include "tf_weapon_grenade_pipebomb.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTFPipebombLauncher C_TFPipebombLauncher
#endif

//=============================================================================
//
// TF Weapon Pipebomb Launcher.
//
#ifdef GAME_DLL
	class CTFPipebombLauncher : public CTFWeaponBaseGun, public ITFChargeUpWeapon, public IEntityListener
#else
	class CTFPipebombLauncher : public CTFWeaponBaseGun, public ITFChargeUpWeapon
#endif
{
public:

	DECLARE_CLASS( CTFPipebombLauncher, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	// Server specific.
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	CTFPipebombLauncher();
	~CTFPipebombLauncher();

	enum {
		TF_PIPEBOMB_CHECK_NONE,
		TF_PIPEBOMB_GLOW_CHECK,
		TF_PIPEBOMB_DETONATE_CHECK,
	};

	virtual void	Spawn( void );
	virtual void	Precache( void );
	virtual int		GetWeaponID( void ) const { return TF_WEAPON_PIPEBOMBLAUNCHER; }
	virtual CBaseEntity *FireProjectile( CTFPlayer *pPlayer );
	virtual void	ItemPostFrame( void );
	virtual void	ItemBusyFrame( void );
	virtual void	SecondaryAttack();

	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo );
	virtual bool	Deploy( void );
	virtual void	PrimaryAttack( void );
	virtual void	WeaponIdle( void );
	virtual float	GetProjectileSpeed( void );
	virtual bool	Reload( void );
	virtual void	WeaponReset( void );

	// ITFChargeUpWeapon
	virtual float	GetChargeBeginTime( void ) { return m_flChargeBeginTime; }
	virtual float	GetChargeMaxTime( void );

	int				GetDetonateMode( void ) const;

#ifdef CLIENT_DLL
	void			BombHighlightThink( void );
#endif

	int				GetPipeBombCount( void ) { return m_iPipebombCount; }
	virtual void	LaunchGrenade( void );
	virtual bool	DetonateRemotePipebombs( bool bFizzle );
	virtual void	AddPipeBomb( CTFGrenadePipebombProjectile *pBomb );
	virtual bool	ModifyPipebombsInView( int iMode );

	void			DeathNotice( CBaseEntity *pVictim );

#ifdef GAME_DLL
	void			UpdateOnRemove( void );
#endif

	CNetworkVar( int, m_iPipebombCount );	


	// List of active pipebombs
	typedef CHandle<CTFGrenadePipebombProjectile>	PipebombHandle;
	CUtlVector<PipebombHandle> m_Pipebombs;

	float	m_flChargeBeginTime;
	float	m_flLastDenySoundTime;

	CTFPipebombLauncher( CTFPipebombLauncher const& );
};

// Old School Pipebomb/Sticky Launcher.

#if defined CLIENT_DLL
#define CTFPipebombLauncher_Legacy C_TFPipebombLauncher_Legacy
#endif

class CTFPipebombLauncher_Legacy : public CTFPipebombLauncher
{
public:

	DECLARE_CLASS( CTFPipebombLauncher_Legacy, CTFPipebombLauncher )
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	virtual int GetWeaponID( void ) const { return TF_WEAPON_PIPEBOMBLAUNCHER_LEGACY; }
};

// TF2 Beta Pipebomb launcher.

#if defined CLIENT_DLL
#define CTFPipebombLauncher_TF2Beta C_TFPipebombLauncher_TF2Beta
#endif

class CTFPipebombLauncher_TF2Beta : public CTFPipebombLauncher
{
public:

	DECLARE_CLASS( CTFPipebombLauncher_TF2Beta, CTFPipebombLauncher )
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	virtual int GetWeaponID( void ) const { return TF_WEAPON_PIPEBOMBLAUNCHER_TF2BETA; }
};

// TFC Esque Pipebomb launcher.

#if defined CLIENT_DLL
#define CTFPipebombLauncher_TFC C_TFPipebombLauncher_TFC
#endif

class CTFPipebombLauncher_TFC : public CTFPipebombLauncher
{
public:

	DECLARE_CLASS( CTFPipebombLauncher_TFC, CTFPipebombLauncher )
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	virtual int GetWeaponID( void ) const { return TF_WEAPON_PIPEBOMBLAUNCHER_TFC; }
};

#endif // TF_WEAPON_PIPEBOMBLAUNCHER_H
