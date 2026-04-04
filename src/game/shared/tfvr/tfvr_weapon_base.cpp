//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: TF2VR Base VR Weapon Implementation
//
//=============================================================================

#include "cbase.h"
#include "tfvr_weapon_base.h"

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
CTFVRWeaponBase::CTFVRWeaponBase()
{
	m_gripLocalOffset = vec3_origin;
	m_gripLocalAngles = vec3_angle;
	m_muzzleLocalOffset = vec3_origin;
	m_muzzleLocalAngles = vec3_angle;
	m_foregripLocalOffset = vec3_origin;
	m_foregripLocalAngles = vec3_angle;
	m_bTwoHands = false;
	m_hOwnerHand = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFVRWeaponBase::~CTFVRWeaponBase()
{
}

//-----------------------------------------------------------------------------
// Purpose: Spawn
//-----------------------------------------------------------------------------
void CTFVRWeaponBase::Spawn()
{
	BaseClass::Spawn();
	
	// Update local transforms from weapon data
	UpdateLocalTransforms();
}

//-----------------------------------------------------------------------------
// Purpose: Precache
//-----------------------------------------------------------------------------
void CTFVRWeaponBase::Precache()
{
	BaseClass::Precache();
	
	// Precache VR-specific model if it exists
	const char *pszVRModel = GetVRWorldModel();
	if ( pszVRModel && pszVRModel[0] )
	{
		PrecacheModel( pszVRModel );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Deploy weapon
//-----------------------------------------------------------------------------
bool CTFVRWeaponBase::Deploy()
{
	if ( !BaseClass::Deploy() )
		return false;
	
	OnEquippedByHand();
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Holster weapon
//-----------------------------------------------------------------------------
bool CTFVRWeaponBase::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	OnDroppedFromHand();
	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: Drop weapon
//-----------------------------------------------------------------------------
void CTFVRWeaponBase::Drop( const Vector &vecVelocity )
{
	OnDroppedFromHand();
	BaseClass::Drop( vecVelocity );
}

//-----------------------------------------------------------------------------
// Purpose: Equip to owner
//-----------------------------------------------------------------------------
void CTFVRWeaponBase::Equip( CBaseCombatCharacter *pOwner )
{
	BaseClass::Equip( pOwner );
}

//-----------------------------------------------------------------------------
// Purpose: Get grip transform in local space
//-----------------------------------------------------------------------------
void CTFVRWeaponBase::GetGripTransformLocal( matrix3x4_t& transform )
{
	AngleMatrix( m_gripLocalAngles, m_gripLocalOffset, transform );
}

//-----------------------------------------------------------------------------
// Purpose: Get muzzle position in world space
//-----------------------------------------------------------------------------
Vector CTFVRWeaponBase::GetMuzzleAbsOrigin() const
{
	matrix3x4_t muzzleMatrix;
	AngleMatrix( m_muzzleLocalAngles, m_muzzleLocalOffset, muzzleMatrix );
	
	matrix3x4_t worldMatrix;
	const matrix3x4_t &weaponMatrix = EntityToWorldTransform();
	ConcatTransforms( weaponMatrix, muzzleMatrix, worldMatrix );
	
	Vector muzzlePos;
	MatrixGetColumn( worldMatrix, 3, muzzlePos );
	return muzzlePos;
}

//-----------------------------------------------------------------------------
// Purpose: Get muzzle angles in world space
//-----------------------------------------------------------------------------
QAngle CTFVRWeaponBase::GetMuzzleAbsAngles() const
{
	matrix3x4_t muzzleMatrix;
	AngleMatrix( m_muzzleLocalAngles, m_muzzleLocalOffset, muzzleMatrix );
	
	matrix3x4_t worldMatrix;
	const matrix3x4_t &weaponMatrix = EntityToWorldTransform();
	ConcatTransforms( weaponMatrix, muzzleMatrix, worldMatrix );
	
	QAngle muzzleAngles;
	MatrixAngles( worldMatrix, muzzleAngles );
	return muzzleAngles;
}

//-----------------------------------------------------------------------------
// Purpose: Get foregrip transform in local space
//-----------------------------------------------------------------------------
void CTFVRWeaponBase::GetForegripTransformLocal( VMatrix& transform )
{
	matrix3x4_t temp;
	AngleMatrix( m_foregripLocalAngles, m_foregripLocalOffset, temp );
	transform.CopyFrom3x4( temp );
}

//-----------------------------------------------------------------------------
// Purpose: Toggle two-handed mode
//-----------------------------------------------------------------------------
bool CTFVRWeaponBase::ToggleTwoHanded( bool bEnabled )
{
	m_bTwoHands = bEnabled;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Set the hand that owns this weapon
//-----------------------------------------------------------------------------
void CTFVRWeaponBase::SetOwnerHand( CTFVRHand* pHand )
{
	m_hOwnerHand = pHand;
}

//-----------------------------------------------------------------------------
// Purpose: Get the hand that owns this weapon
//-----------------------------------------------------------------------------
CTFVRHand* CTFVRWeaponBase::GetOwnerHand() const
{
	return m_hOwnerHand.Get();
}

//-----------------------------------------------------------------------------
// Purpose: Should this weapon draw in VR?
//-----------------------------------------------------------------------------
bool CTFVRWeaponBase::ShouldDrawInVR()
{
	// Draw if we have an owner hand
	return ( m_hOwnerHand.Get() != NULL );
}

//-----------------------------------------------------------------------------
// Purpose: Get VR-specific world model (optional)
//-----------------------------------------------------------------------------
const char* CTFVRWeaponBase::GetVRWorldModel()
{
	// TODO: Read from weapon data when we implement CTFVRWeaponInfo
	// For now, use the regular world model
	return GetWorldModel();
}

//-----------------------------------------------------------------------------
// Purpose: Set model based on which hand is holding it
//-----------------------------------------------------------------------------
void CTFVRWeaponBase::SetModelForHand( bool bRightHand )
{
	// TODO: Load hand-specific models when available
	// For now, use the same model for both hands
}

//-----------------------------------------------------------------------------
// Purpose: Called when equipped by a hand
//-----------------------------------------------------------------------------
void CTFVRWeaponBase::OnEquippedByHand()
{
	// Override in derived classes for weapon-specific behavior
}

//-----------------------------------------------------------------------------
// Purpose: Called when dropped from a hand
//-----------------------------------------------------------------------------
void CTFVRWeaponBase::OnDroppedFromHand()
{
	m_hOwnerHand = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Update local transforms from weapon data
//-----------------------------------------------------------------------------
void CTFVRWeaponBase::UpdateLocalTransforms()
{
	// TODO: Read from weapon data when we implement CTFVRWeaponInfo
	// For now, use default values
	
	// Default grip at weapon origin
	m_gripLocalOffset = vec3_origin;
	m_gripLocalAngles = vec3_angle;
	
	// Default muzzle slightly forward
	m_muzzleLocalOffset = Vector( 24, 0, 0 );
	m_muzzleLocalAngles = vec3_angle;
	
	// Default foregrip
	m_foregripLocalOffset = Vector( 12, 0, -2 );
	m_foregripLocalAngles = vec3_angle;
}

//-----------------------------------------------------------------------------
// Networking
//-----------------------------------------------------------------------------
IMPLEMENT_NETWORKCLASS_ALIASED( TFVRWeaponBase, DT_TFVRWeaponBase )

BEGIN_NETWORK_TABLE( CTFVRWeaponBase, DT_TFVRWeaponBase )
#if defined( CLIENT_DLL )
	RecvPropVector( RECVINFO( m_gripLocalOffset ) ),
	RecvPropQAngles( RECVINFO( m_gripLocalAngles ) ),
	RecvPropVector( RECVINFO( m_muzzleLocalOffset ) ),
	RecvPropQAngles( RECVINFO( m_muzzleLocalAngles ) ),
	RecvPropVector( RECVINFO( m_foregripLocalOffset ) ),
	RecvPropQAngles( RECVINFO( m_foregripLocalAngles ) ),
	RecvPropBool( RECVINFO( m_bTwoHands ) ),
#else
	SendPropVector( SENDINFO( m_gripLocalOffset ) ),
	SendPropQAngles( SENDINFO( m_gripLocalAngles ) ),
	SendPropVector( SENDINFO( m_muzzleLocalOffset ) ),
	SendPropQAngles( SENDINFO( m_muzzleLocalAngles ) ),
	SendPropVector( SENDINFO( m_foregripLocalOffset ) ),
	SendPropQAngles( SENDINFO( m_foregripLocalAngles ) ),
	SendPropBool( SENDINFO( m_bTwoHands ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFVRWeaponBase )
END_PREDICTION_DATA()

#if !defined( CLIENT_DLL )
BEGIN_DATADESC( CTFVRWeaponBase )
END_DATADESC()
#endif
