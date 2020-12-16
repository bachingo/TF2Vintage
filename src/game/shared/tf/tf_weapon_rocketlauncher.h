//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// TF Rocket Launcher
//
//=============================================================================
#ifndef TF_WEAPON_ROCKETLAUNCHER_H
#define TF_WEAPON_ROCKETLAUNCHER_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"
#include "tf_weaponbase_rocket.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTFRocketLauncher C_TFRocketLauncher
#endif

#ifdef GAME_DLL
#include "GameEventListener.h"
#endif

//=============================================================================
//
// TF Weapon Rocket Launcher.
//
class CTFRocketLauncher : public CTFWeaponBaseGun
{
public:

	DECLARE_CLASS( CTFRocketLauncher, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	// Server specific.
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	CTFRocketLauncher();
	~CTFRocketLauncher();

#ifndef CLIENT_DLL
	virtual void	Precache();
#endif
	bool			ShouldBlockPrimaryFire() OVERRIDE	{ return !AutoFiresFullClip(); }

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_ROCKETLAUNCHER; }
	virtual void	Misfire( void );
	virtual CBaseEntity *FireProjectile( CTFPlayer *pPlayer );

	virtual void	ItemPostFrame( void );

	virtual bool	DefaultReload( int iClipSize1, int iClipSize2, int iActivity );
	virtual bool	CheckReloadMisfire( void ) OVERRIDE;

	void			ModifyEmitSoundParams( EmitSound_t &params ) OVERRIDE;

#ifdef CLIENT_DLL
	virtual void CreateMuzzleFlashEffects( C_BaseEntity *pAttachEnt, int nIndex );
	//virtual void DrawCrosshair( void );
#endif

private:
	float	m_flShowReloadHintAt;
	int		m_nReloadPitchStep;

#ifdef GAME_DLL
	bool	m_bOverloading;
#endif

	//CNetworkVar( bool, m_bLockedOn );

	CTFRocketLauncher( const CTFRocketLauncher & ) {}
};

// Server specific
#ifdef GAME_DLL

//=============================================================================
//
// Generic rocket.
//
class CTFRocket : public CTFBaseRocket
{
public:

	DECLARE_CLASS( CTFRocket, CTFBaseRocket );

	// Creation.
	static CTFRocket *Create( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner = NULL );	
	virtual void Spawn();
	virtual void Precache();
};

#endif

// Old School Rocket Launcher.

#if defined CLIENT_DLL
#define CTFRocketLauncher_Legacy C_TFRocketLauncher_Legacy
#endif

class CTFRocketLauncher_Legacy : public CTFRocketLauncher
{
public:

	DECLARE_CLASS( CTFRocketLauncher_Legacy, CTFRocketLauncher )
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	virtual int GetWeaponID( void ) const { return TF_WEAPON_ROCKETLAUNCHER_LEGACY; }
};

// Simple addon logic used for the Air Strike.
#if defined CLIENT_DLL
#define CTFRocketLauncher_Airstrike C_TFRocketLauncher_Airstrike
#endif

class CTFRocketLauncher_Airstrike : public CTFRocketLauncher
#ifdef GAME_DLL
	, public CGameEventListener
#endif
{
public:

	DECLARE_CLASS( CTFRocketLauncher_Airstrike, CTFRocketLauncher )
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	virtual int GetWeaponID( void ) const { return TF_WEAPON_ROCKETLAUNCHER_AIRSTRIKE; }
	virtual bool	HasChargeBar( void )			{ return true; }
	virtual const char* GetEffectLabelText( void )			{ return "#TF_KILLS"; }
	virtual bool	Deploy( void );
	virtual bool 	Holster( CBaseCombatWeapon *pSwitchingTo );
#ifdef GAME_DLL
	virtual void	SetupGameEventListeners( void );
	virtual void	FireGameEvent( IGameEvent *event );
#endif
	virtual void	OnKill( void );
};


#endif // TF_WEAPON_ROCKETLAUNCHER_H