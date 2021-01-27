//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_RAYGUN_H
#define TF_WEAPON_RAYGUN_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"
#include "tf_weapon_rocketlauncher.h"

#if defined( CLIENT_DLL )
#define CTFRaygun C_TFRaygun
#define CTFDRGPomson C_TFDRGPomson
#endif

//=============================================================================
//
// Raygun Class.
//
class CTFRaygun : public CTFRocketLauncher
{
public:

	DECLARE_CLASS( CTFRaygun, CTFRocketLauncher );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFRaygun();

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_RAYGUN; }
	virtual float	GetProjectileSpeed( void )			{ return 1200.0f; }
	virtual float	GetProjectileGravity( void )		{ return 0.0f; }
	virtual void	PrimaryAttack();
	
	virtual bool		HasChargeBar( void )				{ return true; }
	virtual const char* GetEffectLabelText( void )			{ return "#TF_BISON"; }

	virtual float		Energy_GetShotCost( void ) const;
	virtual float		Energy_GetRechargeCost( void ) const { return 5.f; }

#ifdef CLIENT_DLL
	virtual bool	ShouldPlayClientReloadSound() { return true; }
#endif

private:

	CTFRaygun( const CTFRaygun & ) {}
};

//=============================================================================
//
// Pomson Class.
//
class CTFDRGPomson : public CTFRaygun
{
public:

	DECLARE_CLASS( CTFDRGPomson, CTFRaygun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFDRGPomson();

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_DRG_POMSON; }
	virtual const char* GetEffectLabelText( void )			{ return "#TF_POMSON_HUD"; }

private:

	CTFDRGPomson( const CTFDRGPomson & ) {}
};


#endif // TF_WEAPON_RAYGUN_H
