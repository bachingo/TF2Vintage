//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Mechanical Arm. (Short Circuit)
//
//=============================================================================
#ifndef TF_WEAPON_MECHANICAL_ARM_H
#define TF_WEAPON_MECHANICAL_ARM_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"
#include "tf_weaponbase_rocket.h"

#ifdef GAME_DLL
#include "iscorer.h"
#endif

// Client specific.
#ifdef CLIENT_DLL
#define CTFMechanicalArm C_TFMechanicalArm
#define CTFProjectile_MechanicalArmOrb C_TFProjectile_MechanicalArmOrb
#endif

// Short Circuit.

class CTFMechanicalArm : public CTFWeaponBaseGun
{
public:
	DECLARE_CLASS( CTFMechanicalArm, CTFWeaponBaseGun );
	DECLARE_DATADESC();
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFMechanicalArm();
	~CTFMechanicalArm();

	virtual void	Spawn( void );
	virtual void	Precache( void );

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_MECHANICAL_ARM; }

	virtual void	PrimaryAttack();
	virtual void	SecondaryAttack();
	
	virtual void	LaunchElectricalShock();
	virtual void	LaunchElectricalBall();

	virtual void	PlayWeaponShootSound( void );

#ifdef CLIENT_DLL
	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual void	StopParticleBeam( void );
#endif

	//void		UpdateBodygroups( CBaseCombatCharacter *pOwner, int );

	//virtual int GetAmmoPerShot( void ) const;
	virtual int GetCustomDamageType() const { return TF_DMG_CUSTOM_PLASMA; }

private:
#ifdef CLIENT_DLL
	CNewParticleEffect			*m_pZap;
#endif
};

// Energy Ball Projectile.

#ifdef GAME_DLL
class CTFProjectile_MechanicalArmOrb : public CTFBaseRocket, public IScorer
#else
class CTFProjectile_MechanicalArmOrb : public CTFBaseRocket	
#endif
{
public:
	DECLARE_CLASS( CTFProjectile_MechanicalArmOrb, CTFBaseRocket );
	DECLARE_DATADESC();
	DECLARE_NETWORKCLASS();

	CTFProjectile_MechanicalArmOrb();
	~CTFProjectile_MechanicalArmOrb();

	virtual void	Spawn();
	virtual void	Precache();

#ifdef GAME_DLL

	static CTFProjectile_MechanicalArmOrb *Create( CBaseEntity *pWeapon, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner = NULL, CBaseEntity *pScorer = NULL );
	void OrbThink( void );

	virtual void	ZapPlayer( /*CTakeDamageInfo const&, trace_t *pTrace,*/ CBaseEntity *pOther );
	
	// IScorer interface
	virtual CBasePlayer			*GetScorer(void);
	virtual CBasePlayer			*GetAssistant(void) 	{ return NULL; }
	void			SetScorer(CBaseEntity *pScorer);

	void			CheckForPlayers( void );
	void			CheckForProjectiles( void );

	// Overrides.
	virtual void	RocketTouch( CBaseEntity *pOther );

	virtual bool	IsDeflectable() { return true; }
	virtual void	Deflected( CBaseEntity *pDeflectedBy, Vector &vecDir );

#endif

	virtual void	ExplodeAndRemove(void);


#ifdef CLIENT_DLL

	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual void	CreateTrails( void );
	virtual void	CreateLightEffects( void );

	virtual const char *GetTrailParticleName( void );
#endif

private:
#ifdef CLIENT_DLL
	CUtlVector< CHandle<C_BaseEntity> > m_hZapTargets;
	CNewParticleEffect			*m_pOrb;
	CNewParticleEffect			*m_pOrbLightning;
#else
	CBaseHandle m_Scorer;
	CUtlVector< EHANDLE >		m_hZapTargets;
	float						m_flDetonateTime;
#endif
};

#endif // TF_WEAPON_MECHANICAL_ARM_H