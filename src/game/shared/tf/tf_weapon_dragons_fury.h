//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// TF Rocket Launcher
//
//=============================================================================
#ifndef TF_WEAPON_DRAGONS_FURY_H
#define TF_WEAPON_DRAGONS_FURY_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"
#include "tf_weaponbase_rocket.h"
#include "tf_weapon_rocketlauncher.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTFWeaponFlameBall C_TFWeaponFlameBall
#endif

//=============================================================================
//
// TF Weapon Flame Ball.
//

class CTFWeaponFlameBall : public CTFRocketLauncher
{
public:
	DECLARE_CLASS( CTFWeaponFlameBall, CTFRocketLauncher );
	DECLARE_DATADESC();
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFWeaponFlameBall();
	~CTFWeaponFlameBall();

	virtual void	Spawn( void );
	virtual void	Precache( void );

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_ROCKETLAUNCHER_FIREBALL; }

	virtual void	PrimaryAttack();
	virtual void	SecondaryAttack();

	virtual CBaseEntity *FireProjectile( CTFPlayer *pPlayer );
	virtual void	ItemPostFrame( void );
	virtual bool	DefaultReload( int iClipSize1, int iClipSize2, int iActivity );

	virtual void	GetProjectileFireSetup( CTFPlayer *pPlayer, Vector vecOffset, Vector *vecSrc, QAngle *angForward, bool bHitTeammates = true, bool bUseHitboxes = false );

	virtual float		GetEffectBarProgress( void );
#ifdef GAME_DLL
	void			StartPressureSound( void );
	virtual void		OnResourceMeterFilled( void );

	virtual void	DeflectEntity(CBaseEntity *pEntity, CTFPlayer *pAttacker, Vector &vecDir);
	virtual void	DeflectPlayer(CTFPlayer *pVictim, CTFPlayer *pAttacker, Vector &vecDir);

	bool			CanAirBlast(void);
	bool			CanAirBlastDeflectProjectile(void);
	bool			CanAirBlastPushPlayers(void);
	bool			CanAirBlastPutOutTeammate(void);
#else
	virtual void		OnDataChanged( DataUpdateType_t updateType );
	virtual bool		Deploy( void );
	void				UpdatePoseParam( void );
#endif

	CNetworkVar( float, m_flRechargeScale );
};

#endif // TF_WEAPON_DRAGONS_FURY_H