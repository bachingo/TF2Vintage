//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: CTF HealthKit.
//
//=============================================================================//
#ifndef ENTITY_HEALTHKIT_H
#define ENTITY_HEALTHKIT_H

#ifdef _WIN32
#pragma once
#endif

#include "tf_powerup.h"

//=============================================================================
//
// CTF HealthKit class.
//

class CHealthKit : public CTFPowerup
{
public:
	DECLARE_CLASS( CHealthKit, CTFPowerup );

	void	Spawn( void );
	void	Precache( void );
	bool	MyTouch( CBasePlayer *pPlayer );

	powerupsize_t	GetPowerupSize( void ) { return POWERUP_FULL; }
	virtual const char *GetDefaultPowerupModel( void );
	virtual const char *GetHealthKitName( void ) { return "medkit_large"; }
};

class CHealthKitSmall : public CHealthKit
{
public:
	DECLARE_CLASS( CHealthKitSmall, CHealthKit );
	powerupsize_t	GetPowerupSize( void ) { return POWERUP_SMALL; }
	virtual const char *GetDefaultPowerupModel( void );
	virtual const char *GetHealthKitName( void ) { return "medkit_small"; }
};

class CHealthKitMedium : public CHealthKit
{
public:
	DECLARE_CLASS( CHealthKitMedium, CHealthKit );
	powerupsize_t	GetPowerupSize( void ) { return POWERUP_MEDIUM; }
	virtual const char *GetDefaultPowerupModel( void );
	virtual const char *GetHealthKitName( void ) { return "medkit_medium"; }
};

#endif // ENTITY_HEALTHKIT_H


