//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// TF Rocket Projectile
//
//=============================================================================
#ifndef TF_PROJECTILE_ENERGYBALL_H
#define TF_PROJECTILE_ENERGYBALL_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_rocket.h"

#ifdef GAME_DLL
#include "iscorer.h"
#endif

#if defined CLIENT_DLL
#define CTFEnergyBall C_TFEnergyBall
#endif


//=============================================================================
//
// Generic rocket.
//
#ifdef GAME_DLL
class CTFEnergyBall : public CTFBaseRocket,  public IScorer
#else
class CTFEnergyBall : public CTFBaseRocket
#endif
{
public:

	DECLARE_CLASS( CTFEnergyBall, CTFBaseRocket );
	DECLARE_NETWORKCLASS();

#ifdef GAME_DLL
	// Creation.
	static CTFEnergyBall *Create( CBaseEntity *pWeapon, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner = NULL, CBaseEntity *pScorer = NULL );
	virtual void Spawn();
	virtual void Precache();

	// IScorer interface
	virtual CBasePlayer			*GetScorer(void);
	virtual CBasePlayer			*GetAssistant(void) { return NULL; }
	void	SetScorer(CBaseEntity *pScorer);

	float 			GetRadius(void);
	virtual int		GetDamageType();
#endif

	void	SetIsCharged(bool bCharged)			{ m_bChargedBeam = bCharged; }
	bool	ShotIsCharged(void)					{ return m_bChargedBeam; }

	void	SetCritical( bool bCritical ) { m_bCritical = bCritical; }

	void 			Explode( trace_t *pTrace, CBaseEntity *pOther );
	
protected:
#ifdef GAME_DLL
	EHANDLE m_Scorer;
#endif
	
private:
	CNetworkVar(bool, m_bCritical);
	CNetworkVar(bool, m_bChargedBeam);
	
public:

#ifdef CLIENT_DLL
	void 		CreateRocketTrails( void );
#endif
};

#endif	//TF_PROJECTILE_ENERGYBALL_H