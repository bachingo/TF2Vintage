#ifndef TF_WEAPON_SCRIPTED_H
#define TF_WEAPON_SCRIPTED_H

#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase.h"
#include "basescriptedweapon.h"

#if defined(CLIENT_DLL)
#define CTFScriptedWeapon C_TFScriptedWeapon
#endif

class CTFScriptedWeapon : public CBaseScriptedWeapon
{
	DECLARE_CLASS( CTFScriptedWeapon, CBaseScriptedWeapon );
public:
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_ENT_SCRIPTDESC();

	CTFScriptedWeapon();
	virtual ~CTFScriptedWeapon();

	virtual bool IsPredicted( void ) const { return true; }

#if defined(GAME_DLL)
	void ApplyOnHitAttributes( CBaseEntity *pVictim, CTFPlayer *pAttacker, const CTakeDamageInfo &info ) OVERRIDE;
	void ApplyPostHitEffects( CTakeDamageInfo const &info, CTFPlayer *pVictim ) OVERRIDE;
#endif
};

#endif