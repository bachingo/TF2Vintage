//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: TF2VR Scout Bat - VR melee weapon implementation
//
// DEPRECATED: Physical VR melee is now handled directly in CTFWeaponBaseMelee.
// All melee weapons automatically get VR physical melee support without
// needing separate VR-specific weapon classes. See tf_weaponbase_melee.h.
//
//=============================================================================

#ifndef TFVR_BAT_H
#define TFVR_BAT_H
#ifdef _WIN32
#pragma once
#endif

#include "tfvr_weapon_melee.h"

#if defined( CLIENT_DLL )
	#define CTFVRBat C_TFVRBat
#endif

//=============================================================================
//
// TF2VR Bat - Scout's melee weapon with velocity-based damage
//
class CTFVRBat : public CTFVRWeaponMelee
{
	DECLARE_CLASS( CTFVRBat, CTFVRWeaponMelee );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

#if !defined( CLIENT_DLL )
	DECLARE_DATADESC();
#endif

public:
	CTFVRBat();
	virtual ~CTFVRBat();

	// Weapon identification
	virtual int GetWeaponID( void ) const override { return TF_WEAPON_BAT; }
	
	// Overrides
	virtual void Precache() override;
	virtual void Swing( CTFPlayer *pPlayer ) override;
	virtual bool DoSwingTrace( trace_t &trace ) override;
	
#if !defined( CLIENT_DLL )
	// Damage calculation with velocity multiplier
	virtual float GetMeleeDamage( CBaseEntity *pTarget, int *piDamageType, int *piCustomDamage );
	virtual void OnEntityHit( CBaseEntity *pEntity, CTakeDamageInfo *info );
#endif

private:
	// Bat-specific constants
	static constexpr float BAT_RANGE = 48.0f;  // Swing range in units
};

#endif // TFVR_BAT_H
