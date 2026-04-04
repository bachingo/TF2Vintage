//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: TF2VR Gun Base Implementation
//
//=============================================================================

#include "cbase.h"
#include "tfvr_weapon_gun.h"

#if defined( CLIENT_DLL )
	#include "c_tf_player.h"
#else
	#include "tf_player.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFVRWeaponGun::CTFVRWeaponGun()
{
	m_flVRRecoilAmount = 0.0f;
	m_flLastRecoilTime = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFVRWeaponGun::~CTFVRWeaponGun()
{
}

//-----------------------------------------------------------------------------
// Purpose: Primary attack - fire from VR muzzle position
//-----------------------------------------------------------------------------
void CTFVRWeaponGun::PrimaryAttack()
{
	// Call base class to handle firing
	BaseClass::PrimaryAttack();
	
	// Apply VR-specific recoil
	ApplyVRRecoil();
}

//-----------------------------------------------------------------------------
// Purpose: Post-frame update
//-----------------------------------------------------------------------------
void CTFVRWeaponGun::ItemPostFrame()
{
	BaseClass::ItemPostFrame();
	
	// TODO: Handle VR-specific input (trigger pull, etc.)
}

//-----------------------------------------------------------------------------
// Purpose: Apply VR recoil (pushes weapon back in hand)
//-----------------------------------------------------------------------------
void CTFVRWeaponGun::ApplyVRRecoil()
{
	m_flLastRecoilTime = gpGlobals->curtime;
	
	// TODO: Apply actual recoil impulse to weapon position
	// This will push the weapon back in the player's hand
	// For now, just track that recoil occurred
}

//-----------------------------------------------------------------------------
// Purpose: Get VR recoil impulse
//-----------------------------------------------------------------------------
void CTFVRWeaponGun::GetVRRecoilImpulse( Vector &impulse, QAngle &angularImpulse )
{
	// Default recoil: push back along weapon's forward axis
	Vector forward;
	AngleVectors( GetAbsAngles(), &forward );
	
	impulse = -forward * 5.0f; // Push back 5 units
	angularImpulse = QAngle( -2.0f, 0, 0 ); // Slight upward kick
}

//-----------------------------------------------------------------------------
// Purpose: Fire from muzzle position instead of eye position
//-----------------------------------------------------------------------------
void CTFVRWeaponGun::GetProjectileFireSetup( CTFPlayer *pPlayer, Vector vecOffset, Vector *vecSrc, QAngle *angForward, bool bHitTeammates, float flEndDist )
{
	if ( !pPlayer )
		return;
	
	// Fire from muzzle position in VR
	*vecSrc = GetMuzzleAbsOrigin();
	*angForward = GetMuzzleAbsAngles();
	
	// Apply any offset
	if ( vecOffset != vec3_origin )
	{
		Vector forward, right, up;
		AngleVectors( *angForward, &forward, &right, &up );
		*vecSrc += forward * vecOffset.x + right * vecOffset.y + up * vecOffset.z;
	}
}

//-----------------------------------------------------------------------------
// Networking
//-----------------------------------------------------------------------------
IMPLEMENT_NETWORKCLASS_ALIASED( TFVRWeaponGun, DT_TFVRWeaponGun )

BEGIN_NETWORK_TABLE( CTFVRWeaponGun, DT_TFVRWeaponGun )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFVRWeaponGun )
END_PREDICTION_DATA()

#if !defined( CLIENT_DLL )
BEGIN_DATADESC( CTFVRWeaponGun )
END_DATADESC()
#endif
