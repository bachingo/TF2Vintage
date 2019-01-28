//=============================================================================//
//
// Purpose: Jarate Projectile
//
//=============================================================================//

#ifndef TF_PROJECTILE_JAR_H
#define TF_PROJECTILE_JAR_H

#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_grenadeproj.h"
#ifdef GAME_DLL
#include "iscorer.h"
#endif

#ifdef CLIENT_DLL
#define CTFProjectile_Jar C_TFProjectile_Jar
#endif

#ifdef GAME_DLL
class CTFProjectile_Jar : public CTFWeaponBaseGrenadeProj, public IScorer
#else
class C_TFProjectile_Jar : public C_TFWeaponBaseGrenadeProj
#endif
{
public:
	DECLARE_CLASS( CTFProjectile_Jar, CTFWeaponBaseGrenadeProj );
	DECLARE_NETWORKCLASS();
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	CTFProjectile_Jar();
	~CTFProjectile_Jar();

	virtual int	GetWeaponID( void ) const 			{ return TF_WEAPON_JAR; }

#ifdef GAME_DLL
	static CTFProjectile_Jar	*Create( CBaseEntity *pWeapon, const Vector &vecOrigin, const QAngle &vecAngles, const Vector &vecVelocity, CBaseCombatCharacter *pOwner, CBaseEntity *pScorer, const AngularImpulse &angVelocity, const CTFWeaponInfo &weaponInfo );

	// IScorer interface
	virtual CBasePlayer			*GetScorer( void ) 	{ return NULL; }
	virtual CBasePlayer			*GetAssistant( void );

	virtual void	Precache( void );
	virtual void	Spawn( void );

	virtual void	Detonate( void );
	virtual void	VPhysicsCollision( int index, gamevcollisionevent_t *pEvent );

	void			JarTouch( CBaseEntity *pOther );

	void			SetScorer( CBaseEntity *pScorer );
		
	void			SetCritical( bool bCritical ) 	{ m_bCritical = bCritical; }

	virtual bool	IsDeflectable() 				{ return true; }
	virtual void	Deflected( CBaseEntity *pDeflectedBy, Vector &vecDir );

	virtual void	Explode( trace_t *pTrace, int bitsDamageType );
#else
	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual void	CreateTrails( void );
	virtual int		DrawModel( int flags );
#endif

private:
#ifdef GAME_DLL
	EHANDLE m_Scorer;
	CNetworkVar( bool, m_bCritical );
#else
	bool		m_bCritical;
#endif

	float		m_flCreationTime;
};
#endif // TF_PROJECTILE_JAR_H