//====== Copyright © 1996-2006, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_GRAPPLINGHOOK_H
#define TF_WEAPON_GRAPPLINGHOOK_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"

#ifdef CLIENT_DLL
#define CTFGrapplingHook C_TFGrapplingHook
#endif

class CTFGrapplingHook : public CTFWeaponBaseGun
{
public:

	DECLARE_CLASS( CTFGrapplingHook, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CTFGrapplingHook();
	~CTFGrapplingHook();

	virtual void	Precache();
	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_GRAPPLINGHOOK; }

	virtual void	PrimaryAttack( void );
	virtual void	SecondaryAttack( void );

	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo );
	virtual void	ItemPostFrame( void );
	virtual bool	Deploy( void );

	virtual float	GetProjectileGravity( void ) { return 0.0f; }
	virtual float	GetProjectileSpeed( void );

	virtual bool	ShouldRemoveDisguiseOnPrimaryAttack( void ) const { return false; }
	virtual bool	ShouldRemoveInvisibilityOnPrimaryAttack( void ) const { return true; }

	virtual bool	CanAttack( void );

	virtual int		GetWeaponProjectileType( void ) const { return TF_PROJECTILE_GRAPPLINGHOOK; }
	virtual CBaseEntity *FireProjectile( CTFPlayer *pPlayer );
	void			RemoveHookProjectile( bool bSomething );

	virtual void	PlayWeaponShootSound( void );

	virtual acttable_t *ActivityList( int &iActivityCount );
	static acttable_t s_grapplinghook_normal_acttable[];
	//static acttable_t s_grapplinghook_engineer_acttable[];

	/*GetCanAttackFlags() const
	GetPlayerPoseParamList(int&)
	GetProjectileFireSetup(CTFPlayer*, Vector, Vector*, QAngle*, bool, float)
	OnHookReleased(bool)
	SendWeaponAnim(int)*/

	virtual int		TranslateViewmodelHandActivity( int iActivity );
protected:
	CNetworkHandle( CBaseEntity, m_hProjectile );

private:

#ifdef CLIENT_DLL
	CSoundPatch	*m_pPullingLoop;
#endif

	CTFGrapplingHook(const CTFGrapplingHook &) {}
};

#endif // TF_WEAPON_GRAPPLINGHOOK_H
