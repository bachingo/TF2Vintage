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
#include "tf_projectile_flare.h"

#ifdef CLIENT_DLL
#define CTFFlareGun C_TFFlareGun
#define CTFFlareGun_Revenge C_TFFlareGun_Revenge
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

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_FLAREGUN; }
	virtual void	PrimaryAttack( void );
	virtual void	SecondaryAttack( void );
	void			AddFlare(CTFProjectile_Flare *pFlare);
	void			DeathNotice( CBaseEntity *pVictim );

	int				GetFlareGunMode( void ) const;

#if defined( CLIENT_DLL )
	virtual bool	ShouldPlayClientReloadSound() { return true; }
#endif

private:
	float m_flLastDenySoundTime;

#if defined( GAME_DLL )
	// Used for tracking flares for Detonator.
	typedef CHandle<CTFProjectile_Flare>	FlareHandle;
	CUtlVector<FlareHandle> m_Flares;
#endif
};

class CTFFlareGun_Revenge : public CTFFlareGun
{
public:
	DECLARE_CLASS( CTFFlareGun_Revenge, CTFFlareGun );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	// Server specific.
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	CTFFlareGun_Revenge();
	~CTFFlareGun_Revenge();
	
	virtual void	Precache( void );
	virtual void	PrimaryAttack( void );
	virtual void	SecondaryAttack( void );
	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_FLAREGUN_REVENGE; }
	virtual int		GetCustomDamageType( void ) const;
	virtual bool	Deploy( void );
	virtual bool	Holster( CBaseCombatWeapon *pSwitchTo );
	virtual void	WeaponReset( void );
	virtual void	ItemPostFrame(void);
	int				GetCount( void );

	virtual const char *GetMuzzleFlashParticleEffect( void ) { return "drg_manmelter_muzzleflash"; }
	
	bool				HasChargeBar( void );
	virtual const char *GetEffectLabelText( void ) { return "#TF_CRITS"; }
	virtual void		StartCharge( void );
	virtual void		StopCharge( void );
	virtual void		ChargePostFrame( void );
	virtual float		GetChargeBeginTime( void ) const { return m_flChargeBeginTime; }

#if defined( CLIENT_DLL )
	virtual void	OnDataChanged( DataUpdateType_t type );
	virtual void	DispatchMuzzleFlash( const char* effectName, C_BaseEntity* pAttachEnt );
	void			ClientEffectsThink( void );
	void			StartChargeEffects();
	void			StopChargeEffects();
#endif
	
	bool			CanGetAirblastCrits( void ) const;

private:
	bool ExtinguishPlayerInternal( CTFPlayer *pTarget, CTFPlayer *pOwner );

	CNetworkVar( float, m_flChargeBeginTime );
	CNetworkVar( float, m_fLastExtinguishTime );
	
#ifdef CLIENT_DLL
	CSoundPatch	*m_pChargeLoop;
	bool m_bReadyToFire;

	int m_nOldRevengeCrits;
#endif
};

#endif // TF_WEAPON_FLAREGUN_H