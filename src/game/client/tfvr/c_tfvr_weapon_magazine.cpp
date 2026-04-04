//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: TF2VR Physical Magazine Entity Implementation - Client
//
//=============================================================================

#include "cbase.h"
#include "c_tfvr_weapon_magazine.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=============================================================================
//
// TF2VR Magazine tables
//
IMPLEMENT_CLIENTCLASS_DT( C_TFVRWeaponMagazine, DT_TFVRWeaponMagazine, CTFVRWeaponMagazine )
	RecvPropInt( RECVINFO( m_iWeaponType ) ),
	RecvPropInt( RECVINFO( m_iAmmoCount ) ),
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
C_TFVRWeaponMagazine::C_TFVRWeaponMagazine()
{
	m_iWeaponType = TF_WEAPON_NONE;
	m_iAmmoCount = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
C_TFVRWeaponMagazine::~C_TFVRWeaponMagazine()
{
}

//=============================================================================
