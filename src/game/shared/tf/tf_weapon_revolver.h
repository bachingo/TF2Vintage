//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
//
//=============================================================================
#ifndef TF_WEAPON_REVOLVER_H
#define TF_WEAPON_REVOLVER_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTFRevolver C_TFRevolver
#define CTFRevolver_Secondary C_TFRevolver_Secondary
#endif

#ifdef GAME_DLL
#include "GameEventListener.h"
#endif

//=============================================================================
//
// TF Weapon Revolver.
//
class CTFRevolver : public CTFWeaponBaseGun
{
public:

	DECLARE_CLASS( CTFRevolver, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFRevolver() {}
	~CTFRevolver() {}

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_REVOLVER; }

	virtual bool DefaultReload( int iClipSize1, int iClipSize2, int iActivity );

#if defined( CLIENT_DLL )
	virtual void	GetWeaponCrosshairScale( float &flScale ) override;
#endif

private:

	CTFRevolver( const CTFRevolver & ) {}
};

class CTFRevolver_Secondary : public CTFRevolver
{
public:
	DECLARE_CLASS( CTFRevolver_Secondary, CTFRevolver );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
};

#if defined CLIENT_DLL
#define CTFRevolver_Dex C_TFRevolver_Dex
#endif

class CTFRevolver_Dex : public CTFRevolver
{
public:
	DECLARE_CLASS( CTFRevolver_Dex, CTFRevolver );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CTFRevolver_Dex() {}
	~CTFRevolver_Dex() {}

	virtual void	PrimaryAttack( void );
	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_REVOLVER_DEX; }
	
	virtual void	ItemPostFrame( void );
	virtual void	CritThink( void );
	
	virtual bool	Deploy( void );
	virtual bool	Holster( CBaseCombatWeapon *pSwitchTo );

	virtual const char *GetEffectLabelText( void ) { return "#TF_CRITS"; }


private:

	CTFRevolver_Dex( CTFRevolver_Dex const& );
};

#endif // TF_WEAPON_REVOLVER_H