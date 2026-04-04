//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: TF2VR Scout Pistol Implementation
//
//=============================================================================

#include "cbase.h"
#include "tfvr_pistol.h"

#if defined( CLIENT_DLL )
	#include "c_tf_player.h"
#else
	#include "tf_player.h"
	#include "tfvr/tfvr_weapon_magazine.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=============================================================================
//
// TF2VR Pistol tables
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFVRPistol, DT_TFVRPistol )

BEGIN_NETWORK_TABLE( CTFVRPistol, DT_TFVRPistol )
#if defined( CLIENT_DLL )
	RecvPropBool( RECVINFO( m_bMagazineEjected ) ),
	RecvPropFloat( RECVINFO( m_flMagazineEjectTime ) ),
#else
	SendPropBool( SENDINFO( m_bMagazineEjected ) ),
	SendPropFloat( SENDINFO( m_flMagazineEjectTime ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFVRPistol )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_pistol_vr, CTFVRPistol );
PRECACHE_WEAPON_REGISTER( tf_weapon_pistol_vr );

#if !defined( CLIENT_DLL )
BEGIN_DATADESC( CTFVRPistol )
END_DATADESC()
#endif

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFVRPistol::CTFVRPistol()
{
	m_bMagazineEjected = false;
	m_flMagazineEjectTime = 0.0f;
#if !defined( CLIENT_DLL )
	m_hEjectedMagazine = NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFVRPistol::~CTFVRPistol()
{
}

//-----------------------------------------------------------------------------
// Purpose: Precache
//-----------------------------------------------------------------------------
void CTFVRPistol::Precache()
{
	BaseClass::Precache();
	
#if !defined( CLIENT_DLL )
	// Precache magazine model
	PrecacheModel( "models/weapons/w_models/w_pistol_clip.mdl" );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Post-frame update
//-----------------------------------------------------------------------------
void CTFVRPistol::ItemPostFrame()
{
	BaseClass::ItemPostFrame();
	
#if !defined( CLIENT_DLL )
	// Check if player is trying to insert a magazine
	// TODO: Implement magazine insertion detection
	// This will check if player's hand with magazine is near the magwell
#endif
}

#if !defined( CLIENT_DLL )
//-----------------------------------------------------------------------------
// Purpose: Check if weapon needs reload
//-----------------------------------------------------------------------------
bool CTFVRPistol::NeedsReload()
{
	// Need reload if clip is empty or magazine has been ejected
	return ( Clip1() <= 0 || m_bMagazineEjected );
}

//-----------------------------------------------------------------------------
// Purpose: Eject magazine
//-----------------------------------------------------------------------------
void CTFVRPistol::EjectMagazine()
{
	if ( m_bMagazineEjected )
		return;  // Already ejected
	
	// Mark as ejected
	m_bMagazineEjected = true;
	m_flMagazineEjectTime = gpGlobals->curtime;
	
	// Spawn physical magazine
	CTFVRWeaponMagazine *pMag = (CTFVRWeaponMagazine *)CreateEntityByName( "tfvr_weapon_magazine" );
	if ( pMag )
	{
		// Set magazine properties
		pMag->SetWeaponType( GetWeaponID() );
		pMag->SetAmmoCount( Clip1() );  // Magazine contains current ammo
		pMag->SetOwnerWeapon( this );
		
		// Get eject position (use grip offset + eject offset)
		Vector ejectPos = GetAbsOrigin();
		QAngle ejectAngles = GetAbsAngles();
		
		// TODO: Use proper eject offset from weapon data
		Vector forward, right, down;
		AngleVectors( ejectAngles, &forward, &right, &down );
		ejectPos += right * -2.0f;  // Eject to the side
		
		// Spawn magazine
		pMag->SetAbsOrigin( ejectPos );
		pMag->SetAbsAngles( ejectAngles );
		pMag->Spawn();
		
		// Give it some velocity (eject with force)
		IPhysicsObject *pPhys = pMag->VPhysicsGetObject();
		if ( pPhys )
		{
			Vector ejectVel = right * -100.0f + down * -50.0f;  // Eject down and to the side
			pPhys->SetVelocity( &ejectVel, NULL );
		}
		
		// Store reference
		m_hEjectedMagazine = pMag;
	}
	
	// Empty the clip
	m_iClip1 = 0;
	
	// Play eject sound
	WeaponSound( SPECIAL1 );
}

//-----------------------------------------------------------------------------
// Purpose: Get magwell position (where magazine inserts)
//-----------------------------------------------------------------------------
Vector CTFVRPistol::GetMagwellAbsOrigin()
{
	// Use grip position as magwell position
	matrix3x4_t gripTransform;
	GetGripTransformLocal( gripTransform );
	
	matrix3x4_t worldMatrix;
	const matrix3x4_t &weaponMatrix = EntityToWorldTransform();
	
	matrix3x4_t magwellWorldMatrix;
	ConcatTransforms( weaponMatrix, gripTransform, magwellWorldMatrix );
	
	Vector magwellPos;
	MatrixGetColumn( magwellWorldMatrix, 3, magwellPos );
	
	// Offset slightly down from grip
	Vector down;
	AngleVectors( GetAbsAngles(), NULL, NULL, &down );
	magwellPos += down * 2.0f;
	
	return magwellPos;
}

//-----------------------------------------------------------------------------
// Purpose: Check if magazine is at magwell insertion point
//-----------------------------------------------------------------------------
bool CTFVRPistol::AtMagwellPoint( CTFVRWeaponMagazine *pMag )
{
	if ( !pMag )
		return false;
	
	// Check if magazine is close to magwell
	Vector magwellPos = GetMagwellAbsOrigin();
	Vector magPos = pMag->GetAbsOrigin();
	
	float distance = (magPos - magwellPos).Length();
	
	// Within 4 units = close enough to insert
	return ( distance < 4.0f );
}

//-----------------------------------------------------------------------------
// Purpose: Insert magazine into weapon
//-----------------------------------------------------------------------------
void CTFVRPistol::InsertMagazine( CTFVRWeaponMagazine *pMag )
{
	if ( !pMag )
		return;
	
	if ( !m_bMagazineEjected )
		return;  // Can't insert if we haven't ejected
	
	// Get ammo from magazine
	int ammo = pMag->GetAmmoCount();
	
	// Fill clip (TF2 arcade style - instant reload, no slide rack needed)
	m_iClip1 = GetMaxClip1();  // Full reload
	
	// Mark as loaded
	m_bMagazineEjected = false;
	
	// Remove magazine entity
	UTIL_Remove( pMag );
	m_hEjectedMagazine = NULL;
	
	// Play insert sound
	WeaponSound( RELOAD );
	
	// Weapon is instantly ready to fire (TF2 arcade style)
}
#endif

//=============================================================================
