//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// TF Rocket Launcher
//
//=============================================================================
#ifndef TF_WEAPON_PARTICLECANNON_H
#define TF_WEAPON_PARTICLECANNON_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weapon_rocketlauncher.h"

// Particle Cannon.

#if defined CLIENT_DLL
#define CTFParticleCannon C_TFParticleCannon
#endif

class CTFParticleCannon : public CTFRocketLauncher
{
public:

	DECLARE_CLASS( CTFParticleCannon, CTFRocketLauncher )
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	
	// Server specific.
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	CTFParticleCannon();
	virtual ~CTFParticleCannon() {}

	virtual void Precache();

	const char*	GetEffectLabelText( void ) { return "#TF_MANGLER"; }
	virtual int GetWeaponID( void ) const { return TF_WEAPON_PARTICLE_CANNON; }
	virtual float GetProjectileSpeed( void ) { return 1100.0f; }
	virtual float GetProjectileGravity( void ) { return 0.0f; }
	virtual void PlayWeaponShootSound( void );
	const char *GetShootSound( int iIndex ) const OVERRIDE;
	const char *GetMuzzleFlashParticleEffect( void ) OVERRIDE;

	virtual bool CanHolster( void ) const;
	virtual bool Holster( CBaseCombatWeapon *pSwitchingTo );
	virtual bool Deploy( void );
	virtual void ItemPostFrame( void );
	virtual void PrimaryAttack( void );
	virtual void SecondaryAttack( void );
	virtual void WeaponReset( void );

	bool		IsViewModelFlipped( void ) OVERRIDE { return !BaseClass::IsViewModelFlipped(); }

	bool		CanChargeShot(void);
	void		FireChargeShot( void );

#ifdef GAME_DLL
	bool		OwnerCanTaunt( void ) const OVERRIDE;
#endif

	virtual float Energy_GetShotCost( void ) const { return 5.0f; }
	virtual float Energy_GetRechargeCost( void ) const { return 5.0f; }
	
#ifdef CLIENT_DLL
	virtual void OnDataChanged( DataUpdateType_t updateType );
	virtual void ClientEffectsThink( void );
	void CreateChargeEffect( void );

	virtual bool ShouldPlayClientReloadSound() { return true; }

	virtual void CreateMuzzleFlashEffects( C_BaseEntity *pAttachEnt, int nIndex ) OVERRIDE;
	virtual void DispatchMuzzleFlash( char const *effectName, C_BaseEntity *pAttachEnt ) OVERRIDE;
#endif
	
private:
	CNetworkVar( float, m_flChargeBeginTime );
	CNetworkVar( int, m_nChargeEffectParity );

#ifdef CLIENT_DLL
	int m_nOldChargeEffectParity;
#endif

	CTFParticleCannon( CTFParticleCannon const & );
};

#endif // TF_WEAPON_PARTICLECANNON_H