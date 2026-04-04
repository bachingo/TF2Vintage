//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: TF2VR Physical Magazine Entity Implementation
//
//=============================================================================

#include "cbase.h"
#include "tfvr_weapon_magazine.h"
#include "tf/tf_player.h"
#include "tfvr/tfvr_weapon_gun.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=============================================================================
//
// TF2VR Magazine tables
//
IMPLEMENT_SERVERCLASS_ST( CTFVRWeaponMagazine, DT_TFVRWeaponMagazine )
	SendPropInt( SENDINFO( m_iWeaponType ) ),
	SendPropInt( SENDINFO( m_iAmmoCount ) ),
END_SEND_TABLE()

BEGIN_DATADESC( CTFVRWeaponMagazine )
	DEFINE_FIELD( m_iWeaponType, FIELD_INTEGER ),
	DEFINE_FIELD( m_iAmmoCount, FIELD_INTEGER ),
	DEFINE_FIELD( m_hOwnerWeapon, FIELD_EHANDLE ),
	DEFINE_FIELD( m_flRemoveTime, FIELD_TIME ),
	DEFINE_FIELD( m_bFading, FIELD_BOOLEAN ),
	DEFINE_THINKFUNC( FadeThink ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( tfvr_weapon_magazine, CTFVRWeaponMagazine );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFVRWeaponMagazine::CTFVRWeaponMagazine()
{
	m_iWeaponType = TF_WEAPON_NONE;
	m_iAmmoCount = 0;
	m_hOwnerWeapon = NULL;
	m_flRemoveTime = 0.0f;
	m_bFading = false;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFVRWeaponMagazine::~CTFVRWeaponMagazine()
{
}

//-----------------------------------------------------------------------------
// Purpose: Spawn
//-----------------------------------------------------------------------------
void CTFVRWeaponMagazine::Spawn()
{
	// Set default model (will be overridden by weapon-specific model)
	SetModel( "models/weapons/w_models/w_pistol_clip.mdl" );
	
	BaseClass::Spawn();
	
	// Enable physics
	SetSolid( SOLID_VPHYSICS );
	SetMoveType( MOVETYPE_VPHYSICS );
	
	// Create physics object
	VPhysicsInitNormal( SOLID_VPHYSICS, 0, false );
	
	// Set collision group
	SetCollisionGroup( COLLISION_GROUP_WEAPON );
	
	// Set lifetime (30 seconds by default)
	SetLifetime( 30.0f );
}

//-----------------------------------------------------------------------------
// Purpose: Precache
//-----------------------------------------------------------------------------
void CTFVRWeaponMagazine::Precache()
{
	PrecacheModel( "models/weapons/w_models/w_pistol_clip.mdl" );
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Set owner weapon
//-----------------------------------------------------------------------------
void CTFVRWeaponMagazine::SetOwnerWeapon( CTFVRWeaponGun *pWeapon )
{
	m_hOwnerWeapon = pWeapon;
}

//-----------------------------------------------------------------------------
// Purpose: Get owner weapon
//-----------------------------------------------------------------------------
CTFVRWeaponGun* CTFVRWeaponMagazine::GetOwnerWeapon() const
{
	return m_hOwnerWeapon.Get();
}

//-----------------------------------------------------------------------------
// Purpose: Use - called when player tries to grab magazine
//-----------------------------------------------------------------------------
void CTFVRWeaponMagazine::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CTFPlayer *pPlayer = ToTFPlayer( pActivator );
	if ( !pPlayer )
		return;
	
	if ( !CanBePickedUp( pPlayer ) )
		return;
	
	// TODO: Implement magazine pickup and attachment to hand
	// This will be handled by the hand system when we integrate it
	
	// For now, just remove the magazine
	UTIL_Remove( this );
}

//-----------------------------------------------------------------------------
// Purpose: Can this magazine be picked up by the player?
//-----------------------------------------------------------------------------
bool CTFVRWeaponMagazine::CanBePickedUp( CTFPlayer *pPlayer )
{
	if ( !pPlayer )
		return false;
	
	// Check if player has a weapon that can use this magazine
	// TODO: Implement when we have weapon checking
	
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Physics collision
//-----------------------------------------------------------------------------
void CTFVRWeaponMagazine::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
	BaseClass::VPhysicsCollision( index, pEvent );
	
	// Play impact sound
	// TODO: Add magazine-specific impact sounds
}

//-----------------------------------------------------------------------------
// Purpose: Set lifetime before removal
//-----------------------------------------------------------------------------
void CTFVRWeaponMagazine::SetLifetime( float lifetime )
{
	m_flRemoveTime = gpGlobals->curtime + lifetime;
	SetThink( &CTFVRWeaponMagazine::FadeThink );
	SetNextThink( m_flRemoveTime - 2.0f );  // Start fading 2 seconds before removal
}

//-----------------------------------------------------------------------------
// Purpose: Fade out and remove
//-----------------------------------------------------------------------------
void CTFVRWeaponMagazine::FadeAndRemove()
{
	// Start fading
	m_bFading = true;
	
	// Fade out over 2 seconds
	SetRenderMode( kRenderTransTexture );
	SetRenderColorA( 255 );
	
	// Remove after fade
	SetThink( &CBaseEntity::SUB_Remove );
	SetNextThink( gpGlobals->curtime + 2.0f );
}

//-----------------------------------------------------------------------------
// Purpose: Think function for fading
//-----------------------------------------------------------------------------
void CTFVRWeaponMagazine::FadeThink()
{
	if ( !m_bFading )
	{
		// Start fading
		FadeAndRemove();
	}
	else
	{
		// Continue fading
		float fadeTime = 2.0f;
		float timeLeft = m_flRemoveTime - gpGlobals->curtime;
		float alpha = (timeLeft / fadeTime) * 255.0f;
		alpha = clamp( alpha, 0.0f, 255.0f );
		
		SetRenderColorA( (byte)alpha );
		
		// Continue thinking
		SetNextThink( gpGlobals->curtime + 0.1f );
	}
}

//=============================================================================
