//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: TF2VR Scout Pistol - VR pistol with magazine reload
//
//=============================================================================

#ifndef TFVR_PISTOL_H
#define TFVR_PISTOL_H
#ifdef _WIN32
#pragma once
#endif

#include "tfvr_weapon_gun.h"

#if defined( CLIENT_DLL )
	#define CTFVRPistol C_TFVRPistol
#else
	class CTFVRWeaponMagazine;
#endif

//=============================================================================
//
// TF2VR Pistol - Scout's secondary weapon with magazine reload
//
class CTFVRPistol : public CTFVRWeaponGun
{
	DECLARE_CLASS( CTFVRPistol, CTFVRWeaponGun );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

#if !defined( CLIENT_DLL )
	DECLARE_DATADESC();
#endif

public:
	CTFVRPistol();
	virtual ~CTFVRPistol();

	// Weapon identification
	virtual int GetWeaponID( void ) const override { return TF_WEAPON_PISTOL; }
	
	// Overrides
	virtual void Precache() override;
	virtual void ItemPostFrame() override;
	
#if !defined( CLIENT_DLL )
	// Magazine reload mechanics
	void EjectMagazine();
	bool AtMagwellPoint( CTFVRWeaponMagazine *pMag );
	void InsertMagazine( CTFVRWeaponMagazine *pMag );
	bool NeedsReload();
	
	// Get magwell position (where magazine inserts)
	Vector GetMagwellAbsOrigin();
#endif

private:
	CNetworkVar( bool, m_bMagazineEjected );  // Has magazine been ejected?
	CNetworkVar( float, m_flMagazineEjectTime );  // When was magazine ejected?
	
#if !defined( CLIENT_DLL )
	CHandle<CTFVRWeaponMagazine> m_hEjectedMagazine;  // Reference to ejected magazine
#endif
};

#endif // TFVR_PISTOL_H
