//========= Copyright Valve Corporation, All rights reserved. ============//
//
// TF Nail Projectile
//
//=============================================================================
#ifndef TF_PROJECTILE_NAIL_H
#define TF_PROJECTILE_NAIL_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "tf_projectile_base.h"
#include "tf_weaponbase_gun.h"

//-----------------------------------------------------------------------------
// Purpose: The base Nail projectile
//-----------------------------------------------------------------------------
class CTFProjectile_Nail : public CTFBaseProjectile
{
	DECLARE_CLASS(CTFProjectile_Nail, CTFBaseProjectile);

public:

	CTFProjectile_Nail();
	~CTFProjectile_Nail();

	// Creation.
	static CTFProjectile_Nail *Create(const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner = NULL, CBaseEntity *pScorer = NULL, bool bCritical = false, CBaseEntity *pWeapon = NULL);

	virtual const char *GetProjectileModelName(void);
	virtual float GetGravity(void);

	static float	GetInitialVelocity(void) { return 1000.0; }
};

//-----------------------------------------------------------------------------
// Purpose: Identical to a nail except for model used
//-----------------------------------------------------------------------------
class CTFProjectile_Syringe : public CTFBaseProjectile
{
	DECLARE_CLASS( CTFProjectile_Syringe, CTFBaseProjectile );

public:
	// Creation.
	static CTFBaseProjectile *Create( const Vector &vecOrigin, const QAngle &vecAngles, CTFWeaponBaseGun *pLauncher = NULL, CBaseEntity *pOwner = NULL, CBaseEntity *pScorer = NULL, bool bCritical = false );	

	virtual unsigned int PhysicsSolidMaskForEntity( void ) const;
	virtual const char *GetProjectileModelName( void )	{ return "models/weapons/w_models/w_syringe_proj.mdl"; }
	virtual float GetGravity( void );
};

//-----------------------------------------------------------------------------
// Purpose: Modified syringe for the TF_WEAPON_TRANQ weapon
//-----------------------------------------------------------------------------
class CTFProjectile_Dart : public CTFBaseProjectile
{
	DECLARE_CLASS(CTFProjectile_Dart, CTFBaseProjectile);

public:

	CTFProjectile_Dart();
	~CTFProjectile_Dart();

	// Creation.
	static CTFProjectile_Dart *Create(const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner = NULL, CBaseEntity *pScorer = NULL, bool bCritical = false, CBaseEntity *pWeapon = NULL);

	virtual const char *GetProjectileModelName(void);
	virtual float GetGravity(void);

	static float	GetInitialVelocity(void) { return 2000.0; }
};

#endif	//TF_PROJECTILE_NAIL_H
