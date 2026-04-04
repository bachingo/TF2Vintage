//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: TF2VR Melee Base Implementation
//
//=============================================================================

#include "cbase.h"
#include "tfvr_weapon_melee.h"

#if defined( CLIENT_DLL )
	#include "c_tf_player.h"
	#include "tfvr/c_tfvr_hand.h"
#else
	#include "tf_player.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFVRWeaponMelee::CTFVRWeaponMelee()
{
	m_vecLastPosition = vec3_origin;
	m_flLastUpdateTime = 0.0f;
	m_flSwingVelocity = 0.0f;
	m_flLastSwingTime = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFVRWeaponMelee::~CTFVRWeaponMelee()
{
}

//-----------------------------------------------------------------------------
// Purpose: Post-frame update - detect swings
//-----------------------------------------------------------------------------
void CTFVRWeaponMelee::ItemPostFrame()
{
	BaseClass::ItemPostFrame();
	
	// Detect swings based on velocity
	if ( DetectSwing() )
	{
		// Swing detected, perform attack
		CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
		if ( pPlayer )
		{
			Swing( pPlayer );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Primary attack (can also be triggered by button)
//-----------------------------------------------------------------------------
void CTFVRWeaponMelee::PrimaryAttack()
{
	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
	if ( !pPlayer )
		return;
	
	// Perform swing
	Swing( pPlayer );
}

//-----------------------------------------------------------------------------
// Purpose: Detect if player is swinging the weapon
//-----------------------------------------------------------------------------
bool CTFVRWeaponMelee::DetectSwing()
{
	// Check cooldown
	if ( gpGlobals->curtime - m_flLastSwingTime < SWING_COOLDOWN )
		return false;
	
	// Calculate current swing velocity
	float velocity = CalculateSwingVelocity();
	m_flSwingVelocity = velocity;
	
	// Check if velocity exceeds threshold
	return ( velocity >= MIN_SWING_VELOCITY );
}

//-----------------------------------------------------------------------------
// Purpose: Calculate current swing velocity
//-----------------------------------------------------------------------------
float CTFVRWeaponMelee::CalculateSwingVelocity()
{
	Vector currentPos = GetAbsOrigin();
	float currentTime = gpGlobals->curtime;
	
	// Calculate velocity
	float deltaTime = currentTime - m_flLastUpdateTime;
	if ( deltaTime <= 0.0f )
		return 0.0f;
	
	Vector deltaPos = currentPos - m_vecLastPosition;
	float velocity = deltaPos.Length() / deltaTime;
	
	// Update tracking
	m_vecLastPosition = currentPos;
	m_flLastUpdateTime = currentTime;
	
	return velocity;
}

//-----------------------------------------------------------------------------
// Purpose: Calculate damage multiplier based on swing velocity
//-----------------------------------------------------------------------------
float CTFVRWeaponMelee::CalculateSwingDamageMultiplier()
{
	// Base multiplier is 1.0
	// Faster swings do more damage (up to 2x)
	float velocityRatio = m_flSwingVelocity / (MIN_SWING_VELOCITY * 3.0f);
	velocityRatio = clamp( velocityRatio, 0.5f, 2.0f );
	
	return velocityRatio;
}

//-----------------------------------------------------------------------------
// Purpose: Do swing trace from weapon model
//-----------------------------------------------------------------------------
bool CTFVRWeaponMelee::DoSwingTrace( trace_t &trace )
{
	// Get weapon tip position (use muzzle as tip for now)
	Vector tipPos = GetMuzzleAbsOrigin();
	
	// Trace from last position to current position
	Vector startPos = m_vecLastPosition;
	if ( startPos == vec3_origin )
		startPos = tipPos;
	
	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
	if ( !pPlayer )
		return false;
	
	// Perform trace
	CTraceFilterSimple filter( pPlayer, COLLISION_GROUP_NONE );
	UTIL_TraceLine( startPos, tipPos, MASK_SOLID, &filter, &trace );
	
	return ( trace.fraction < 1.0f );
}

//-----------------------------------------------------------------------------
// Purpose: Perform a swing
//-----------------------------------------------------------------------------
void CTFVRWeaponMelee::Swing( CTFPlayer *pPlayer )
{
	if ( !pPlayer )
		return;
	
	// Update last swing time
	m_flLastSwingTime = gpGlobals->curtime;
	
	// Do trace
	trace_t trace;
	if ( DoSwingTrace( trace ) )
	{
		// Hit something!
		// TODO: Apply damage with velocity multiplier
		// This will be implemented in specific weapon classes
	}
	
	// Play swing sound/animation
	// TODO: Implement when we have specific weapons
}

//-----------------------------------------------------------------------------
// Networking
//-----------------------------------------------------------------------------
IMPLEMENT_NETWORKCLASS_ALIASED( TFVRWeaponMelee, DT_TFVRWeaponMelee )

BEGIN_NETWORK_TABLE( CTFVRWeaponMelee, DT_TFVRWeaponMelee )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFVRWeaponMelee )
END_PREDICTION_DATA()

#if !defined( CLIENT_DLL )
BEGIN_DATADESC( CTFVRWeaponMelee )
END_DATADESC()
#endif
