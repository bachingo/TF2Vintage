//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: TF2VR Scout Scattergun - VR shotgun with pump reload
//
//=============================================================================

#ifndef TFVR_SCATTERGUN_H
#define TFVR_SCATTERGUN_H
#ifdef _WIN32
#pragma once
#endif

#include "tfvr_weapon_gun.h"

#if defined( CLIENT_DLL )
	#define CTFVRScattergun C_TFVRScattergun
#endif

//=============================================================================
//
// TF2VR Scattergun - Scout's primary weapon with pump-to-reload mechanic
//
class CTFVRScattergun : public CTFVRWeaponGun
{
	DECLARE_CLASS( CTFVRScattergun, CTFVRWeaponGun );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

#if !defined( CLIENT_DLL )
	DECLARE_DATADESC();
#endif

public:
	CTFVRScattergun();
	virtual ~CTFVRScattergun();

	// Weapon identification
	virtual int GetWeaponID( void ) const override { return TF_WEAPON_SCATTERGUN; }
	
	// Overrides
	virtual void Precache() override;
	virtual void ItemPostFrame() override;
	virtual void PrimaryAttack() override;
	
	// Pump-to-reload mechanic
	bool DetectPumpGesture();
	void AddShellFromPump();
	bool NeedsReload();
	
	// Get pump/foregrip position
	Vector GetPumpAbsOrigin();

private:
	// Pump gesture tracking
	CNetworkVector( m_vecLastPumpPosition );
	CNetworkVar( float, m_flLastPumpTime );
	CNetworkVar( bool, m_bPumpingBack );
	CNetworkVar( bool, m_bPumpingForward );
	
	// Pump parameters (from weapon data)
	float m_flPumpDistance;  // How far to pull back (default 6 units)
	float m_flPumpCooldown;  // Time between pumps (default 0.5s)
	
	// Pump state tracking
	float m_flPumpStartDistance;  // Distance when pump started
};

#endif // TFVR_SCATTERGUN_H
