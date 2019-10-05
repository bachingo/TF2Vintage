//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: TF Sniper Rifle
//
//=============================================================================//
#ifndef TF_WEAPON_SNIPERRIFLE_CLASSIC_H
#define TF_WEAPON_SNIPERRIFLE_CLASSIC_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"
#include "tf_weapon_sniperrifle.h"

#if defined( CLIENT_DLL )
#define CTFSniperRifle_Classic C_TFSniperRifle_Classic
#endif

//=============================================================================
//
// TFC Sniper Rifle class.
//
class CTFSniperRifle_Classic : public CTFWeaponBaseGun
{
public:

	DECLARE_CLASS( CTFSniperRifle_Classic, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFSniperRifle_Classic();
	~CTFSniperRifle_Classic();

	virtual int	GetWeaponID( void ) const			{ return TF_WEAPON_SNIPERRIFLE_CLASSIC; }

	virtual void Spawn();
	virtual void Precache();
	void		 ResetTimers( void );

	virtual bool Reload( void );
	virtual bool CanHolster( void ) const;
	virtual bool Holster( CBaseCombatWeapon *pSwitchingTo );

	void		 HandleZooms( void );
	virtual void ItemPostFrame( void );
	virtual bool Lower( void );
	virtual float GetProjectileDamage( void );
	virtual int	GetDamageType() const;

	virtual void WeaponReset( void );

	virtual bool CanFireCriticalShot( bool bIsHeadshot = false );

#ifdef CLIENT_DLL
	float GetHUDDamagePerc( void );
#endif

	bool IsZoomed( void );

private:

	void CreateSniperDot( void );
	void DestroySniperDot( void );
	void UpdateSniperDot( void );

private:
	// Auto-rezooming handling
	void SetRezoom( bool bRezoom, float flDelay );

	void Zoom( void );
	void ZoomOutIn( void );
	void ZoomIn( void );
	void ZoomOut( void );
	void Fire( CTFPlayer *pPlayer );

private:

	CNetworkVar( float,	m_flChargedDamage );

#ifdef GAME_DLL
	CHandle<CSniperDot>		m_hSniperDot;
#endif

	// Handles rezooming after the post-fire unzoom
	float m_flUnzoomTime;
	float m_flRezoomTime;
	bool m_bRezoomAfterShot;

	CTFSniperRifle_Classic( CTFSniperRifle_Classic const& );
};

#endif // TF_WEAPON_SNIPERRIFLE_CLASSIC_H
