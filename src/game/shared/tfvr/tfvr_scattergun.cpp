//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: TF2VR Scout Scattergun Implementation
//
//=============================================================================

#include "cbase.h"
#include "tfvr_scattergun.h"
#include "tf_gamerules.h"

#if defined( CLIENT_DLL )
	#include "c_tf_player.h"
#else
	#include "tf_player.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=============================================================================
//
// TF2VR Scattergun tables
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFVRScattergun, DT_TFVRScattergun )

BEGIN_NETWORK_TABLE( CTFVRScattergun, DT_TFVRScattergun )
#if defined( CLIENT_DLL )
	RecvPropVector( RECVINFO( m_vecLastPumpPosition ) ),
	RecvPropFloat( RECVINFO( m_flLastPumpTime ) ),
	RecvPropBool( RECVINFO( m_bPumpingBack ) ),
	RecvPropBool( RECVINFO( m_bPumpingForward ) ),
#else
	SendPropVector( SENDINFO( m_vecLastPumpPosition ) ),
	SendPropFloat( SENDINFO( m_flLastPumpTime ) ),
	SendPropBool( SENDINFO( m_bPumpingBack ) ),
	SendPropBool( SENDINFO( m_bPumpingForward ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFVRScattergun )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_scattergun_vr, CTFVRScattergun );
PRECACHE_WEAPON_REGISTER( tf_weapon_scattergun_vr );

#if !defined( CLIENT_DLL )
BEGIN_DATADESC( CTFVRScattergun )
END_DATADESC()
#endif

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFVRScattergun::CTFVRScattergun()
{
	m_vecLastPumpPosition = vec3_origin;
	m_flLastPumpTime = 0.0f;
	m_bPumpingBack = false;
	m_bPumpingForward = false;
	m_flPumpDistance = 6.0f;  // Default pump distance
	m_flPumpCooldown = 0.5f;  // Default cooldown (matches vanilla TF2 reload rate)
	m_flPumpStartDistance = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFVRScattergun::~CTFVRScattergun()
{
}

//-----------------------------------------------------------------------------
// Purpose: Precache
//-----------------------------------------------------------------------------
void CTFVRScattergun::Precache()
{
	BaseClass::Precache();
	
	// TODO: Read pump parameters from weapon data when CTFVRWeaponInfo is integrated
}

//-----------------------------------------------------------------------------
// Purpose: Post-frame update - detect pump gestures
//-----------------------------------------------------------------------------
void CTFVRScattergun::ItemPostFrame()
{
	BaseClass::ItemPostFrame();
	
	// Detect pump gesture for reloading
	if ( NeedsReload() && DetectPumpGesture() )
	{
		AddShellFromPump();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Primary attack - semi-auto firing
//-----------------------------------------------------------------------------
void CTFVRScattergun::PrimaryAttack()
{
	// Fire like vanilla TF2 (semi-auto, no pump required between shots)
	BaseClass::PrimaryAttack();
}

//-----------------------------------------------------------------------------
// Purpose: Check if weapon needs reload
//-----------------------------------------------------------------------------
bool CTFVRScattergun::NeedsReload()
{
	// Need reload if clip is not full
	int maxClip = GetMaxClip1();
	int currentClip = Clip1();
	
	return ( currentClip < maxClip );
}

//-----------------------------------------------------------------------------
// Purpose: Get pump/foregrip position in world space
//-----------------------------------------------------------------------------
Vector CTFVRScattergun::GetPumpAbsOrigin()
{
	// Use foregrip position as pump position
	VMatrix foregripTransform;
	GetForegripTransformLocal( foregripTransform );
	
	matrix3x4_t worldMatrix;
	const matrix3x4_t &weaponMatrix = EntityToWorldTransform();
	
	matrix3x4_t pumpWorldMatrix;
	ConcatTransforms( weaponMatrix, foregripTransform.As3x4(), pumpWorldMatrix );
	
	Vector pumpPos;
	MatrixGetColumn( pumpWorldMatrix, 3, pumpPos );
	return pumpPos;
}

//-----------------------------------------------------------------------------
// Purpose: Detect pump gesture
//-----------------------------------------------------------------------------
bool CTFVRScattergun::DetectPumpGesture()
{
	// Check cooldown
	if ( gpGlobals->curtime - m_flLastPumpTime < m_flPumpCooldown )
		return false;
	
	// Get current pump position
	Vector currentPumpPos = GetPumpAbsOrigin();
	
	// Initialize last position if needed
	if ( m_vecLastPumpPosition == vec3_origin )
	{
		m_vecLastPumpPosition = currentPumpPos;
		m_flPumpStartDistance = 0.0f;
		return false;
	}
	
	// Calculate movement along weapon's forward axis
	Vector weaponForward;
	AngleVectors( GetAbsAngles(), &weaponForward );
	
	Vector deltaPos = currentPumpPos - m_vecLastPumpPosition;
	float movementAlongAxis = DotProduct( deltaPos, weaponForward );
	
	// Track pump state
	if ( !m_bPumpingBack && !m_bPumpingForward )
	{
		// Not pumping yet - check if starting to pull back
		if ( movementAlongAxis < -0.5f )  // Moving backward
		{
			m_bPumpingBack = true;
			m_flPumpStartDistance = 0.0f;
		}
	}
	else if ( m_bPumpingBack )
	{
		// Currently pulling back
		m_flPumpStartDistance += -movementAlongAxis;
		
		// Check if pulled back far enough
		if ( m_flPumpStartDistance >= m_flPumpDistance )
		{
			// Pulled back enough, now wait for forward motion
			m_bPumpingBack = false;
			m_bPumpingForward = true;
		}
		else if ( movementAlongAxis > 0.5f )  // Started moving forward too early
		{
			// Reset if they didn't pull back far enough
			m_bPumpingBack = false;
			m_flPumpStartDistance = 0.0f;
		}
	}
	else if ( m_bPumpingForward )
	{
		// Currently pushing forward
		if ( movementAlongAxis > 0.5f )  // Moving forward
		{
			// Pump complete!
			m_bPumpingForward = false;
			m_flPumpStartDistance = 0.0f;
			m_vecLastPumpPosition = currentPumpPos;
			return true;
		}
		else if ( movementAlongAxis < -0.5f )  // Started pulling back again
		{
			// Reset if they changed direction
			m_bPumpingForward = false;
			m_bPumpingBack = true;
			m_flPumpStartDistance = 0.0f;
		}
	}
	
	// Update last position
	m_vecLastPumpPosition = currentPumpPos;
	
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Add one shell from pump gesture
//-----------------------------------------------------------------------------
void CTFVRScattergun::AddShellFromPump()
{
	// Add one shell to clip
	int maxClip = GetMaxClip1();
	int currentClip = Clip1();
	
	if ( currentClip < maxClip )
	{
		m_iClip1 = currentClip + 1;
		
		// Update last pump time
		m_flLastPumpTime = gpGlobals->curtime;
		
		// Play pump sound
		WeaponSound( RELOAD );
		
#if !defined( CLIENT_DLL )
		// Notify player
		CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
		if ( pPlayer )
		{
			// Visual/audio feedback
			// TODO: Add particle effect or animation
		}
#endif
	}
}

//=============================================================================
