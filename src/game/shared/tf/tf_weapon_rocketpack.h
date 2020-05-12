//====== Copyright © 1996-2020, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================
#ifndef TF_WEAPON_ROCKETPACK_H
#define TF_WEAPON_ROCKETPACK_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_melee.h"

#ifdef CLIENT_DLL
#define CTFRocketPack C_TFRocketPack
#endif

//=============================================================================
//
// Bottle class.
//
class CTFRocketPack : public CTFWeaponBase
{
public:

	DECLARE_CLASS( CTFRocketPack, CTFWeaponBase );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFRocketPack();
	~CTFRocketPack() {}
	virtual void		Precache();

	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_ROCKETPACK; }
	virtual void		PrimaryAttack();
	virtual void		SecondaryAttack();

	virtual bool		CanDeploy( void );
	virtual bool		Deploy( void );
	virtual bool		CanHolster( void ) const;

	virtual void		ItemPostFrame( void );

	virtual bool		CanFire( void ) const;

	virtual void		WeaponReset( void );

	virtual bool		VisibleInWeaponSelection( void )	{ return true; }

	void				InitiateLaunch( void );
	void				PreLaunch( void );
	void				Launch( void );
	void				PassengerDelayLaunchThink( void );
	void				RocketLaunchPlayer( CTFPlayer *pPlayer, const Vector& vecLaunch );

private:

	CNetworkVar( bool, m_bEnabled );
	CNetworkVar( float, m_flInitLaunchTime );
	CNetworkVar( float, m_flLaunchTime );
	CNetworkVar( float, m_flToggleEndTime );

	CTFRocketPack( const CTFRocketPack & ) {}
};

#endif // TF_WEAPON_ROCKETPACK_H
