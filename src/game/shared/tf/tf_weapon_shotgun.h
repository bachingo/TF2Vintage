//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_SHOTGUN_H
#define TF_WEAPON_SHOTGUN_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"

#if defined( CLIENT_DLL )
#define CTFShotgun C_TFShotgun
#define CTFShotgun_Soldier C_TFShotgun_Soldier
#define CTFShotgun_HWG C_TFShotgun_HWG
#define CTFShotgun_Pyro C_TFShotgun_Pyro
#define CTFScatterGun C_TFScatterGun
#define CTFSodaPopper C_TFSodaPopper
#define CTFPepBrawlBlaster C_TFPepBrawlBlaster
#define CTFShotgun_Revenge C_TFShotgun_Revenge
#endif

// Reload Modes
enum
{
	TF_WEAPON_SHOTGUN_RELOAD_START = 0,
	TF_WEAPON_SHOTGUN_RELOAD_SHELL,
	TF_WEAPON_SHOTGUN_RELOAD_CONTINUE,
	TF_WEAPON_SHOTGUN_RELOAD_FINISH
};

//=============================================================================
//
// Shotgun class.
//
class CTFShotgun : public CTFWeaponBaseGun
{
public:

	DECLARE_CLASS( CTFShotgun, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFShotgun();

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_SHOTGUN_PRIMARY; }
	virtual void	PrimaryAttack();

protected:

	void		Fire( CTFPlayer *pPlayer );
	void		UpdatePunchAngles( CTFPlayer *pPlayer );

private:

	CTFShotgun( const CTFShotgun & ) {}
};

// Scout version. Different models, possibly different behaviour later on
class CTFScatterGun : public CTFShotgun
{
public:
	DECLARE_CLASS( CTFScatterGun, CTFShotgun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_SCATTERGUN; }
	virtual void	FireBullet( CTFPlayer *pShooter );

#ifdef GAME_DLL
	virtual void	ApplyPostOnHitAttributes( CTakeDamageInfo const &info, CTFPlayer *pVictim );
#endif

	virtual void	Equip( CBaseCombatCharacter *pEquipTo );
	virtual bool	Deploy( void );
	virtual bool	Holster( CBaseCombatWeapon *pSwitchTo );
	virtual void	FinishReload( void );
	virtual bool	SendWeaponAnim( int iActivity );

	bool			HasKnockback( void ) const;

private:
	bool m_bAutoReload;
};

class CTFPepBrawlBlaster : public CTFScatterGun
{
public:
	DECLARE_CLASS( CTFPepBrawlBlaster, CTFScatterGun);
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_PEP_BRAWLER_BLASTER; }
	virtual bool	HasChargeBar( void )			{ return true; }
	virtual const char* GetEffectLabelText( void )			{ return "#TF_Boost"; }
	virtual float	GetEffectBarProgress( void );
	virtual float	GetSpeedMod( void ) const;
};

class CTFSodaPopper : public CTFScatterGun
{
public:
	DECLARE_CLASS( CTFSodaPopper, CTFScatterGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_SODA_POPPER; }
	virtual bool	HasChargeBar( void )			{ return true; }
	virtual const char* GetEffectLabelText( void )			{ return "#TF_Hype"; }
	virtual float	GetEffectBarProgress( void );
};

class CTFShotgun_Soldier : public CTFShotgun
{
public:
	DECLARE_CLASS( CTFShotgun_Soldier, CTFShotgun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_SHOTGUN_SOLDIER; }
};

// Secondary version. Different weapon slot, different ammo
class CTFShotgun_HWG : public CTFShotgun
{
public:
	DECLARE_CLASS( CTFShotgun_HWG, CTFShotgun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_SHOTGUN_HWG; }
};

class CTFShotgun_Pyro : public CTFShotgun
{
public:
	DECLARE_CLASS( CTFShotgun_Pyro, CTFShotgun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_SHOTGUN_PYRO; }
};


class CTFShotgun_Revenge : public CTFShotgun
{
public:
	DECLARE_CLASS( CTFShotgun_Revenge, CTFShotgun );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CTFShotgun_Revenge();

	virtual void	Precache( void );

#ifdef CLIENT_DLL
	virtual int		GetWorldModelIndex( void );
	
	virtual void	SetWeaponVisible( bool visible );
#endif

	virtual void	PrimaryAttack( void );
	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_SENTRY_REVENGE; }
	virtual int		GetCount( void ) const;

	virtual int		GetCustomDamageType( void ) const;
	
	virtual bool	Deploy( void );
	virtual bool	Holster( CBaseCombatWeapon *pSwitchTo );
	virtual void	Detach( void );

	virtual const char *GetEffectLabelText( void ) { return "#TF_REVENGE"; }

#ifdef GAME_DLL
	virtual void	OnSentryKilled( class CObjectSentrygun *pSentry );
#endif

	bool			CanGetRevengeCrits( void ) const;

private:

	CNetworkVar( int, m_iRevengeCrits );

	CTFShotgun_Revenge( CTFShotgun_Revenge const& );
};

#endif // TF_WEAPON_SHOTGUN_H
