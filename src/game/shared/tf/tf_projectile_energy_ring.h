//====== Copyright © 1996-2013, Valve Corporation, All rights reserved. ========//
//
// Purpose: Bison Projectile.
// baseclass is originally CTFBaseProjectile.
//=============================================================================//
#ifndef TF_PROJECTILE_ENERGY_RING_H
#define TF_PROJECTILE_ENERGY_RING_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_rocket.h"
#ifdef GAME_DLL
#include "iscorer.h"
#endif

// Client specific.
#ifdef CLIENT_DLL
#define CTFProjectile_EnergyRing C_TFProjectile_EnergyRing
#endif

class CTFProjectile_EnergyRing : public CTFBaseRocket
{
public:
	DECLARE_CLASS( CTFProjectile_EnergyRing, CTFBaseRocket );
	DECLARE_DATADESC();
	DECLARE_NETWORKCLASS();

	CTFProjectile_EnergyRing();
	~CTFProjectile_EnergyRing();

	virtual bool UsePenetratingBeam();

#ifdef GAME_DLL

	static CTFProjectile_EnergyRing *Create( CBaseEntity *pWeapon, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner = NULL, CBaseEntity *pScorer = NULL );
	virtual void	Spawn();
	virtual void	Precache();

	// IScorer interface
	virtual CBasePlayer *GetScorer( void );
	virtual CBasePlayer *GetAssistant( void ) { return NULL; }

	virtual int		GetWeaponID( void ) const	{ return TF_WEAPON_RAYGUN; }

	void	SetScorer( CBaseEntity *pScorer );

	void	SetCritical( bool bCritical ) { m_bCritical = bCritical; }

	virtual bool	IsDeflectable() { return false; }

	// Overrides.
	virtual void	Explode( trace_t *pTrace, CBaseEntity *pOther );

	virtual void	RocketTouch( CBaseEntity *pOther ); // ProjectileTouch
#else

	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual void	CreateTrails( void );
	virtual void	CreateLightEffects( void );

#endif

/*CTFProjectile_EnergyRing::CanHeadshot()
CTFProjectile_EnergyRing::GetDamage()
CTFProjectile_EnergyRing::GetDamageCustom()
CTFProjectile_EnergyRing::GetGravity()
CTFProjectile_EnergyRing::GetProjectileModelName()
CTFProjectile_EnergyRing::ImpactTeamPlayer(CTFPlayer*)
CTFProjectile_EnergyRing::PlayImpactEffects(Vector const&, bool)
CTFProjectile_EnergyRing::ResolveFlyCollisionCustom(CGameTrace&, Vector&)*/
private:

	CUtlVector<CBaseEntity*> hPenetratedPlayers;
	int m_nPenetratedCount;
	
#ifdef GAME_DLL

	CBaseHandle m_Scorer;
	CNetworkVar( bool,	m_bCritical );

#else
	bool		m_bCritical;

	CNewParticleEffect	*m_pRing;
#endif

};

#endif //TF_PROJECTILE_ENERGY_RING_H