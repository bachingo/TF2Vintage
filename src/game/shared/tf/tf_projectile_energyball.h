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
class CTFEnergyBall : public CTFBaseRocket, public IScorer
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
	void	Spawn();
	void	Precache();

	// IScorer interface
	CBasePlayer	*GetScorer(void);
	CBasePlayer	*GetAssistant(void) { return NULL; }
	void	SetScorer(CBaseEntity *pScorer);
	void	SetCritical( bool bCritical ) { m_bCritical = bCritical; }

	float	GetDamage( void );
	float 	GetRadius( void );
	int		GetDamageType( void );
	int		GetDamageCustom( void );
	int		GetWeaponID( void ) const OVERRIDE { return TF_WEAPON_PARTICLE_CANNON; }

	bool	IsDeflectable() OVERRIDE { return true; }
	void	Deflected( CBaseEntity *pDeflectedBy, Vector &vecDir ) OVERRIDE;

	void	SetColor( int iColor, Vector vColor ) { if ( iColor==1 ) m_vColor1=vColor; else m_vColor2=vColor; }
#endif

	void	SetIsCharged(bool bCharged)			{ m_bChargedShot = bCharged; }
	bool	IsChargedShot(void)					{ return m_bChargedShot; }
	char const *GetExplosionParticleName( void );

	void 	Explode( trace_t *pTrace, CBaseEntity *pOther );
	
protected:
#ifdef GAME_DLL
	EHANDLE m_Scorer;
#endif
	
private:
	CNetworkVar( bool, m_bCritical );
	CNetworkVar( bool, m_bChargedShot );
	CNetworkVector( m_vColor1 );
	CNetworkVector( m_vColor2 );
	
public:

#ifdef CLIENT_DLL
	void 		CreateRocketTrails( void );
#endif
};

#endif	//TF_PROJECTILE_ENERGYBALL_H