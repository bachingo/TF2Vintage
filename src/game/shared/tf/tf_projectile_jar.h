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
#define CTFProjectile_JarMilk C_TFProjectile_JarMilk
#define CTFProjectile_Cleaver C_TFProjectile_Cleaver
#define CTFProjectile_JarGas C_TFProjectile_JarGas
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

	virtual bool	IsDeflectable();
	virtual void	Deflected( CBaseEntity *pDeflectedBy, Vector &vecDir );

	virtual void	Explode( trace_t *pTrace, int bitsDamageType );
	virtual int		GetEffectCondition( void ) { return TF_COND_URINE; }

	CHandle<CBaseEntity>	m_hEnemy;

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

// Mad Milk Projectile.

#ifdef GAME_DLL
class CTFProjectile_JarMilk : public CTFProjectile_Jar
#else
class C_TFProjectile_JarMilk : public C_TFProjectile_Jar
#endif
{
	public:
	DECLARE_CLASS( CTFProjectile_JarMilk, CTFProjectile_Jar );
	DECLARE_NETWORKCLASS();
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	virtual int	GetWeaponID( void ) const 			{ return TF_WEAPON_JAR_MILK; }

#ifdef GAME_DLL
	static CTFProjectile_JarMilk	*Create( CBaseEntity *pWeapon, const Vector &vecOrigin, const QAngle &vecAngles, const Vector &vecVelocity, CBaseCombatCharacter *pOwner, CBaseEntity *pScorer, const AngularImpulse &angVelocity, const CTFWeaponInfo &weaponInfo );

	virtual void	Precache( void );

	virtual int		GetEffectCondition( void ) { return TF_COND_MAD_MILK; }
#endif
};

// Flying Guillotine Projectile.

#ifdef GAME_DLL
class CTFProjectile_Cleaver : public CTFProjectile_Jar
#else
class C_TFProjectile_Cleaver : public C_TFProjectile_Jar
#endif
{
public:
	DECLARE_CLASS(CTFProjectile_Cleaver, CTFProjectile_Jar);
	DECLARE_NETWORKCLASS();
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	virtual int	GetWeaponID( void ) const 			{ return TF_WEAPON_GRENADE_CLEAVER; }

#ifdef GAME_DLL
	static CTFProjectile_Cleaver	*Create( CBaseEntity *pWeapon, const Vector &vecOrigin, const QAngle &vecAngles, const Vector &vecVelocity, CBaseCombatCharacter *pOwner, CBaseEntity *pScorer, const AngularImpulse &angVelocity, const CTFWeaponInfo &weaponInfo );

	virtual void	Precache( void );

	virtual void	Explode(trace_t *pTrace, int bitsDamageType);
	virtual int		GetEffectCondition( void ) { return TF_COND_BLEEDING; }

	private:
	float		m_flCreationTime;
#endif
};

// Gas Passer Projectile.

#ifdef GAME_DLL
class CTFProjectile_JarGas : public CTFProjectile_Jar
#else
class C_TFProjectile_JarGas : public C_TFProjectile_Jar
#endif
{
	public:
	DECLARE_CLASS( CTFProjectile_JarGas, CTFProjectile_Jar );
	DECLARE_NETWORKCLASS();
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	virtual int	GetWeaponID( void ) const 			{ return TF_WEAPON_JAR_GAS; }

#ifdef GAME_DLL
	static CTFProjectile_JarGas	*Create( CBaseEntity *pWeapon, const Vector &vecOrigin, const QAngle &vecAngles, const Vector &vecVelocity, CBaseCombatCharacter *pOwner, CBaseEntity *pScorer, const AngularImpulse &angVelocity, const CTFWeaponInfo &weaponInfo );

	virtual void	Precache( void );

	virtual int		GetEffectCondition( void ) { return TF_COND_GAS; }
#endif
};

// 

#endif // TF_PROJECTILE_JAR_H