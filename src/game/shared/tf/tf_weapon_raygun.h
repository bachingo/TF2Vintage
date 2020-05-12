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

#if defined( CLIENT_DLL )
#define CTFRaygun C_TFRaygun
#define CTFPomson C_TFPomson
#endif

//=============================================================================
//
// Raygun Class.
//
class CTFRaygun : public CTFWeaponBaseGun
{
public:

	DECLARE_CLASS( CTFRaygun, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFRaygun();

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_RAYGUN; }
	virtual void	PrimaryAttack();
	
	virtual bool		HasChargeBar( void )				{ return true; }
	virtual const char* GetEffectLabelText( void )			{ return "#TF_BISON"; }

private:

	CTFRaygun( const CTFRaygun & ) {}
};

//=============================================================================
//
// Pomson Class.
//
class CTFPomson : public CTFWeaponBaseGun
{
public:

	DECLARE_CLASS( CTFPomson, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFPomson();

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_DRG_POMSON; }
	virtual const char* GetEffectLabelText( void )			{ return "#TF_POMSON_HUD"; }

private:

	CTFPomson( const CTFPomson & ) {}
};


#endif // TF_WEAPON_RAYGUN_H
