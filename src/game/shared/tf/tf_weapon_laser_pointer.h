//=============================================================================
//
// Purpose: A remake of the Wrangler from live TF2
//
//=============================================================================
//=============================================================================
//
// Purpose: A remake of the Wrangler from live TF2
//
//=============================================================================
#ifndef TF_WEAPON_LASER_POINTER_H
#define TF_WEAPON_LASER_POINTER_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"

#ifdef GAME_DLL
#include "tf_obj_sentrygun.h"
#else
#include "c_obj_sentrygun.h"
#endif

// Client specific.
#ifdef CLIENT_DLL
#define CTFLaser_Pointer C_TFLaser_Pointer
#endif

//=============================================================================
//
// TF Weapon Laser_Pointer.
//
class CTFLaser_Pointer : public CTFWeaponBaseGun
{
public:

	DECLARE_CLASS( CTFLaser_Pointer, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

// Server specific.
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	CTFLaser_Pointer();

	virtual void	ItemPostFrame( void );
	virtual bool	Deploy( void );
	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo );
	virtual void	WeaponReset( void );

	virtual void	PrimaryAttack( void );
	virtual void	SecondaryAttack( void );

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_LASER_POINTER; }

	void			RemoveGun( void )					{ pGun = NULL; }

private:
#ifdef GAME_DLL
	CObjectSentrygun *pGun;


	CNetworkVar( Vector , m_vecEnd );
#else
	C_ObjectSentrygun *pGun;

	CNewParticleEffect *pLaser;
	Vector m_vecEnd;
#endif
};

#endif // TF_WEAPON_LASER_POINTER_H