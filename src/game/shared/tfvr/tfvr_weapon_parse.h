//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: TF2VR Weapon Data Parsing - extends TF2 weapon data with VR info
//
//=============================================================================

#ifndef TFVR_WEAPON_PARSE_H
#define TFVR_WEAPON_PARSE_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weapon_parse.h"

//=============================================================================
//
// TF2VR Weapon Info - extends CTFWeaponInfo with VR-specific data
//
class CTFVRWeaponInfo : public CTFWeaponInfo
{
public:
	DECLARE_CLASS_GAMEROOT( CTFVRWeaponInfo, CTFWeaponInfo );
	
	CTFVRWeaponInfo();
	virtual ~CTFVRWeaponInfo();
	
	virtual void Parse( KeyValues *pKeyValuesData, const char *szWeaponName ) override;

public:
	// VR-specific model (optional, uses world model if not set)
	char szVRModel[MAX_WEAPON_STRING];
	char szVRModelLeftHand[MAX_WEAPON_STRING];
	
	// Grip transform (where hand attaches)
	Vector m_vecGripOffset;
	QAngle m_angGripAngles;
	
	// Muzzle transform (where bullets come from)
	Vector m_vecMuzzleOffset;
	QAngle m_angMuzzleAngles;
	
	// Foregrip for two-handed weapons
	Vector m_vecForegripOffset;
	QAngle m_angForegripAngles;
	
	// VR recoil (different feel than screen punch)
	float m_flVRRecoilPitch[2];  // min/max
	float m_flVRRecoilYaw[2];
	
	// Magazine/reload info
	char szMagazineModel[MAX_WEAPON_STRING];
	Vector m_vecMagazineEjectOffset;
	Vector m_vecMagazineInsertOffset;
	
	// Pump-action info (for Scattergun, etc.)
	bool m_bPumpAction;
	Vector m_vecPumpOffset;  // Pump/foregrip rest position
	float m_flPumpDistance;  // How far to pull back
};

#endif // TFVR_WEAPON_PARSE_H
