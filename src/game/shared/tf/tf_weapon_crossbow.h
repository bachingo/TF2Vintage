//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_CROSSBOW_H
#define TF_WEAPON_CROSSBOW_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weapon_syringegun.h"

#if defined CLIENT_DLL
#define CTFCrossbow C_TFCrossbow
#endif

class CTFCrossbow : public CTFSyringeGun
{
public:

	DECLARE_CLASS( CTFCrossbow, CTFSyringeGun )
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	virtual bool		Holster( CBaseCombatWeapon *pSwitchingTo );

	virtual float		GetProjectileSpeed( void );
	virtual float		GetProjectileGravity( void );

	virtual int			GetWeaponID( void ) const { return TF_WEAPON_CROSSBOW; }

private:
	CNetworkVar( float, m_flRegenerateDuration );
};

#endif
