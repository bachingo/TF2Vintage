//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: TF2VR Base VR Weapon - extends TF2 weapon system with VR functionality
//
//=============================================================================

#ifndef TFVR_WEAPON_BASE_H
#define TFVR_WEAPON_BASE_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase.h"

#if defined( CLIENT_DLL )
	#define CTFVRWeaponBase C_TFVRWeaponBase
	class C_TFVRHand;
	typedef C_TFVRHand CTFVRHand;
#else
	class CTFVRHand;
#endif

//=============================================================================
//
// TF2VR Base Weapon Class - adds VR-specific functionality to TF2 weapons
//
class CTFVRWeaponBase : public CTFWeaponBase
{
	DECLARE_CLASS( CTFVRWeaponBase, CTFWeaponBase );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

#if !defined( CLIENT_DLL )
	DECLARE_DATADESC();
#endif

public:
	CTFVRWeaponBase();
	virtual ~CTFVRWeaponBase();

	// Overrides
	virtual void Spawn() override;
	virtual void Precache() override;
	virtual bool Deploy() override;
	virtual bool Holster( CBaseCombatWeapon *pSwitchingTo ) override;
	virtual void Drop( const Vector &vecVelocity ) override;
	virtual void Equip( CBaseCombatCharacter *pOwner ) override;

	// VR-specific grip positioning - where hand attaches to weapon
	virtual void GetGripTransformLocal( matrix3x4_t& transform );
	Vector GetGripLocalOffset() const { return m_gripLocalOffset; }
	QAngle GetGripLocalAngles() const { return m_gripLocalAngles; }
	
	// Muzzle positioning - where bullets/effects originate
	Vector GetMuzzleAbsOrigin() const;
	QAngle GetMuzzleAbsAngles() const;
	Vector GetMuzzleLocalOffset() const { return m_muzzleLocalOffset; }
	QAngle GetMuzzleLocalAngles() const { return m_muzzleLocalAngles; }
	
	// Two-handed support (for future weapons like Minigun)
	virtual void GetForegripTransformLocal( VMatrix& transform );
	bool IsTwoHanded() const { return m_bTwoHands; }
	virtual bool ToggleTwoHanded( bool bEnabled );
	
	// Hand attachment
	void SetOwnerHand( CTFVRHand* pHand );
	CTFVRHand* GetOwnerHand() const;
	
	// VR-specific rendering
	virtual bool ShouldDrawInVR();
	virtual const char* GetVRWorldModel(); // VR-specific model if different
	virtual void SetModelForHand( bool bRightHand );
	
	// Called when equipped/dropped by hand
	virtual void OnEquippedByHand();
	virtual void OnDroppedFromHand();
	
	// Update weapon transforms
	virtual void UpdateLocalTransforms();

protected:
	// Local transforms (parsed from weapon data or set per-weapon)
	CNetworkVector( m_gripLocalOffset );
	CNetworkQAngle( m_gripLocalAngles );
	CNetworkVector( m_muzzleLocalOffset );
	CNetworkQAngle( m_muzzleLocalAngles );
	CNetworkVector( m_foregripLocalOffset );
	CNetworkQAngle( m_foregripLocalAngles );
	
	CNetworkVar( bool, m_bTwoHands );
	
#if defined( CLIENT_DLL )
	CHandle<C_TFVRHand> m_hOwnerHand;
#else
	CHandle<CTFVRHand> m_hOwnerHand;
#endif
};

#endif // TFVR_WEAPON_BASE_H
