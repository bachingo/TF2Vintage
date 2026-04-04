//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: TF2VR Gun Base - VR-specific gun functionality
//
//=============================================================================

#ifndef TFVR_WEAPON_GUN_H
#define TFVR_WEAPON_GUN_H
#ifdef _WIN32
#pragma once
#endif

#include "tfvr_weapon_base.h"
#include "tf_weaponbase_gun.h"

#if defined( CLIENT_DLL )
	#define CTFVRWeaponGun C_TFVRWeaponGun
#endif

//=============================================================================
//
// TF2VR Gun Base Class - adds VR-specific gun functionality
//
class CTFVRWeaponGun : public CTFVRWeaponBase
{
	DECLARE_CLASS( CTFVRWeaponGun, CTFVRWeaponBase );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

#if !defined( CLIENT_DLL )
	DECLARE_DATADESC();
#endif

public:
	CTFVRWeaponGun();
	virtual ~CTFVRWeaponGun();

	// Overrides for VR firing
	virtual void PrimaryAttack() override;
	virtual void ItemPostFrame() override;
	
	// VR-specific recoil (pushes weapon back in hand)
	virtual void ApplyVRRecoil();
	virtual void GetVRRecoilImpulse( Vector &impulse, QAngle &angularImpulse );
	
	// Fire from muzzle position instead of eye position
	virtual void GetProjectileFireSetup( CTFPlayer *pPlayer, Vector vecOffset, Vector *vecSrc, QAngle *angForward, bool bHitTeammates = true, float flEndDist = 2000.f ) override;

protected:
	// VR recoil parameters
	float m_flVRRecoilAmount;
	float m_flLastRecoilTime;
};

#endif // TFVR_WEAPON_GUN_H
