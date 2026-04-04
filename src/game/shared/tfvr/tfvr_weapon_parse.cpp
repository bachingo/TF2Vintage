//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: TF2VR Weapon Data Parsing Implementation
//
//=============================================================================

#include "cbase.h"
#include "tfvr_weapon_parse.h"
#include "KeyValues.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFVRWeaponInfo::CTFVRWeaponInfo()
{
	szVRModel[0] = '\0';
	szVRModelLeftHand[0] = '\0';
	
	m_vecGripOffset = vec3_origin;
	m_angGripAngles = vec3_angle;
	
	m_vecMuzzleOffset = Vector( 24, 0, 0 ); // Default forward
	m_angMuzzleAngles = vec3_angle;
	
	m_vecForegripOffset = Vector( 12, 0, -2 );
	m_angForegripAngles = vec3_angle;
	
	m_flVRRecoilPitch[0] = -2.0f;
	m_flVRRecoilPitch[1] = -5.0f;
	m_flVRRecoilYaw[0] = -1.0f;
	m_flVRRecoilYaw[1] = 1.0f;
	
	szMagazineModel[0] = '\0';
	m_vecMagazineEjectOffset = Vector( 0, -2, 0 );
	m_vecMagazineInsertOffset = Vector( 0, -2, -4 );
	
	m_bPumpAction = false;
	m_vecPumpOffset = Vector( 12, 0, -2 );
	m_flPumpDistance = 6.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFVRWeaponInfo::~CTFVRWeaponInfo()
{
}

//-----------------------------------------------------------------------------
// Purpose: Parse VR weapon data from KeyValues
//-----------------------------------------------------------------------------
void CTFVRWeaponInfo::Parse( KeyValues *pKeyValuesData, const char *szWeaponName )
{
	// Parse base TF2 weapon data first
	BaseClass::Parse( pKeyValuesData, szWeaponName );
	
	// Parse VR-specific data
	KeyValues *pVRData = pKeyValuesData->FindKey( "VRWeaponData" );
	if ( pVRData )
	{
		// VR Models
		const char *pszVRModel = pVRData->GetString( "VRModel", "" );
		if ( pszVRModel && pszVRModel[0] )
		{
			Q_strncpy( szVRModel, pszVRModel, MAX_WEAPON_STRING );
		}
		
		const char *pszVRModelLeft = pVRData->GetString( "VRModelLeftHand", "" );
		if ( pszVRModelLeft && pszVRModelLeft[0] )
		{
			Q_strncpy( szVRModelLeftHand, pszVRModelLeft, MAX_WEAPON_STRING );
		}
		
		// Grip transform
		const char *pszGripOffset = pVRData->GetString( "GripOffset", "0 0 0" );
		UTIL_StringToVector( m_vecGripOffset.Base(), pszGripOffset );
		
		const char *pszGripAngles = pVRData->GetString( "GripAngles", "0 0 0" );
		UTIL_StringToVector( m_angGripAngles.Base(), pszGripAngles );
		
		// Muzzle transform
		const char *pszMuzzleOffset = pVRData->GetString( "MuzzleOffset", "24 0 0" );
		UTIL_StringToVector( m_vecMuzzleOffset.Base(), pszMuzzleOffset );
		
		const char *pszMuzzleAngles = pVRData->GetString( "MuzzleAngles", "0 0 0" );
		UTIL_StringToVector( m_angMuzzleAngles.Base(), pszMuzzleAngles );
		
		// Foregrip transform
		const char *pszForegripOffset = pVRData->GetString( "ForegripOffset", "12 0 -2" );
		UTIL_StringToVector( m_vecForegripOffset.Base(), pszForegripOffset );
		
		const char *pszForegripAngles = pVRData->GetString( "ForegripAngles", "0 0 0" );
		UTIL_StringToVector( m_angForegripAngles.Base(), pszForegripAngles );
		
		// VR Recoil
		m_flVRRecoilPitch[0] = pVRData->GetFloat( "VRRecoilPitchMin", -2.0f );
		m_flVRRecoilPitch[1] = pVRData->GetFloat( "VRRecoilPitchMax", -5.0f );
		m_flVRRecoilYaw[0] = pVRData->GetFloat( "VRRecoilYawMin", -1.0f );
		m_flVRRecoilYaw[1] = pVRData->GetFloat( "VRRecoilYawMax", 1.0f );
		
		// Magazine info
		const char *pszMagazineModel = pVRData->GetString( "MagazineModel", "" );
		if ( pszMagazineModel && pszMagazineModel[0] )
		{
			Q_strncpy( szMagazineModel, pszMagazineModel, MAX_WEAPON_STRING );
		}
		
		const char *pszMagEjectOffset = pVRData->GetString( "MagazineEjectOffset", "0 -2 0" );
		UTIL_StringToVector( m_vecMagazineEjectOffset.Base(), pszMagEjectOffset );
		
		const char *pszMagInsertOffset = pVRData->GetString( "MagazineInsertOffset", "0 -2 -4" );
		UTIL_StringToVector( m_vecMagazineInsertOffset.Base(), pszMagInsertOffset );
		
		// Pump-action info
		m_bPumpAction = pVRData->GetBool( "PumpAction", false );
		
		const char *pszPumpOffset = pVRData->GetString( "PumpOffset", "12 0 -2" );
		UTIL_StringToVector( m_vecPumpOffset.Base(), pszPumpOffset );
		
		m_flPumpDistance = pVRData->GetFloat( "PumpDistance", 6.0f );
	}
}
