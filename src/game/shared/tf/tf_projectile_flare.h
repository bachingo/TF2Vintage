//====== Copyright © 1996-2013, Valve Corporation, All rights reserved. ========//
//
// Purpose: Flare used by the flaregun.
//
//=============================================================================//
#ifndef TF_PROJECTILE_FLARE_H
#define TF_PROJECTILE_FLARE_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_rocket.h"
#ifdef GAME_DLL
#include "iscorer.h"
#endif

// Client specific.
#ifdef CLIENT_DLL
#define CTFProjectile_Flare C_TFProjectile_Flare
#endif

class CTFProjectile_Flare : public CTFBaseRocket
#ifdef GAME_DLL
	, public IScorer
#endif
{
public:
	DECLARE_CLASS( CTFProjectile_Flare, CTFBaseRocket );
	DECLARE_NETWORKCLASS();

	CTFProjectile_Flare();
	~CTFProjectile_Flare();

#ifdef GAME_DLL
	DECLARE_DATADESC();

	static CTFProjectile_Flare 	*Create( CBaseEntity *pWeapon, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner = NULL, CBaseEntity *pScorer = NULL );
	virtual void				Spawn();
	virtual void				Precache();

	// IScorer interface
	virtual CBasePlayer 		*GetScorer( void );
	virtual CBasePlayer			*GetAssistant( void ) 	{ return NULL; }

	virtual int					GetWeaponID(void) const	{ return TF_WEAPON_FLAREGUN; }

	void						SetScorer( CBaseEntity *pScorer );

	void						SetCritical( bool bCritical ) { m_bCritical = bCritical; }
	virtual int					GetDamageType();

	virtual bool 				IsDeflectable();
	virtual void 				Deflected( CBaseEntity *pDeflectedBy, Vector &vecDir );

	bool						IsDestroyable( void ) OVERRIDE { return false; }

	// Overrides.
	virtual void				Explode(trace_t *pTrace, CBaseEntity *pOther);
	virtual void				Airburst(trace_t *pTrace, bool bSelf);
	virtual void				Detonate( bool bSelf = false );
	virtual float				GetFlareRadius( void ) const;
	virtual float				GetProjectileSpeed( void ) const;
	
	virtual void				UpdateOnRemove( void );

	void						ImpactThink( void );

	bool						IsFromTaunt( void ) const { return m_bTauntShot; }
	float						GetLifeTime( void ) const { return gpGlobals->curtime - m_flCreateTime; }
#else
	virtual void				OnDataChanged( DataUpdateType_t updateType );
	virtual void				CreateTrails( void );
#endif

private:
#ifdef GAME_DLL
	CBaseHandle m_hScorer;
	CNetworkVar( bool,	m_bCritical );

	bool m_bTauntShot;

	float m_flImpactTime;
	Vector m_vecImpactNormal;
#else
	bool		m_bCritical;

	HPARTICLEFFECT m_hEffect;
#endif

};

#endif //TF_PROJECTILE_FLARE_H