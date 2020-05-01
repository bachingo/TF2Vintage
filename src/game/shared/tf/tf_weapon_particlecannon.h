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

	bool		CanHolster( void );
	void		ItemPostFrame( void );
	void		SecondaryAttack( void );

	bool		HasChargeUp(void);
	void		ChargeAttack( void );
	
#ifdef CLIENT_DLL
	void		CreateMuzzleFlashEffects( C_BaseEntity *pAttachEnt, int nIndex );
#endif
	
private:
	CNetworkVar(float, m_flChargeUpTime);
};

#endif // TF_WEAPON_PARTICLECANNON_H