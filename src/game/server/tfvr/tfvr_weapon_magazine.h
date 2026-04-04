//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: TF2VR Physical Magazine Entity - grabbable magazine for reloading
//
//=============================================================================

#ifndef TFVR_WEAPON_MAGAZINE_H
#define TFVR_WEAPON_MAGAZINE_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "props.h"

class CTFVRWeaponGun;
class CTFPlayer;

//=============================================================================
//
// TF2VR Magazine Entity - physical magazine that can be grabbed and inserted
//
class CTFVRWeaponMagazine : public CPhysicsProp
{
	DECLARE_CLASS( CTFVRWeaponMagazine, CPhysicsProp );
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

public:
	CTFVRWeaponMagazine();
	virtual ~CTFVRWeaponMagazine();

	// Spawning
	virtual void Spawn() override;
	virtual void Precache() override;
	
	// Setup
	void SetWeaponType( int weaponID ) { m_iWeaponType = weaponID; }
	void SetAmmoCount( int ammo ) { m_iAmmoCount = ammo; }
	void SetOwnerWeapon( CTFVRWeaponGun *pWeapon );
	
	// Accessors
	int GetWeaponType() const { return m_iWeaponType; }
	int GetAmmoCount() const { return m_iAmmoCount; }
	CTFVRWeaponGun* GetOwnerWeapon() const;
	
	// Interaction
	virtual void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value ) override;
	bool CanBePickedUp( CTFPlayer *pPlayer );
	
	// Physics
	virtual void VPhysicsCollision( int index, gamevcollisionevent_t *pEvent ) override;
	
	// Lifetime management
	void SetLifetime( float lifetime );
	void FadeAndRemove();

private:
	void FadeThink();

private:
	CNetworkVar( int, m_iWeaponType );  // Which weapon this magazine is for
	CNetworkVar( int, m_iAmmoCount );   // How much ammo in this magazine
	CHandle<CTFVRWeaponGun> m_hOwnerWeapon;  // Weapon that ejected this magazine
	
	float m_flRemoveTime;  // Time to remove this magazine
	bool m_bFading;        // Is this magazine fading out?
};

#endif // TFVR_WEAPON_MAGAZINE_H
