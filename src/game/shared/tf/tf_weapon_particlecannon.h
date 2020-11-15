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
	
	virtual int GetWeaponID( void ) const { return TF_WEAPON_PARTICLE_CANNON; }
	virtual float GetProjectileSpeed( void ) { return 1100.0f; }
	virtual float GetProjectileGravity( void ) { return 0.0f; }

	bool		CanHolster( void );
	void		ItemPostFrame( void );
	void		PrimaryAttack( void );
	void		SecondaryAttack( void );

	bool		CanChargeShot(void);
	void		FireChargeShot( void );

#ifdef GAME_DLL
	bool		OwnerCanTaunt( void ) const OVERRIDE;
#endif

	virtual float	Energy_GetShotCost( void ) const;
	virtual float	Energy_GetRechargeCost( void ) const { return 5.f; }
	
#ifdef CLIENT_DLL
	void		CreateMuzzleFlashEffects( C_BaseEntity *pAttachEnt, int nIndex ) OVERRIDE;
#endif
	
private:
	CNetworkVar( float, m_flChargeBeginTime );
};

#endif // TF_WEAPON_PARTICLECANNON_H