//=============================================================================//
//
// Purpose: StunBallate Projectile
//
//=============================================================================//

#ifndef TF_PROJECTILE_STUNBALL_H
#define TF_PROJECTILE_STUNBALL_H

#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_grenadeproj.h"
#ifdef GAME_DLL
#include "iscorer.h"
#endif

#ifdef CLIENT_DLL
#define CTFStunBall C_TFStunBall
#define CTFBauble C_TFBauble
#endif


class CTFStunBall : public CTFWeaponBaseGrenadeProj
#ifdef GAME_DLL
	,public IScorer
#endif
{
public:
	DECLARE_CLASS( CTFStunBall, CTFWeaponBaseGrenadeProj );
	DECLARE_NETWORKCLASS();
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	CTFStunBall();
	~CTFStunBall();

	virtual int			GetWeaponID( void ) const 	{ return TF_WEAPON_BAT_WOOD; }

#ifdef GAME_DLL
	static CTFStunBall 	*Create( CBaseEntity *pWeapon, const Vector &vecOrigin, const QAngle &vecAngles, const Vector &vecVelocity, CBaseCombatCharacter *pOwner, CBaseEntity *pScorer, const AngularImpulse &angVelocity, const CTFWeaponInfo &weaponInfo );

	// IScorer interface
	virtual CBasePlayer *GetScorer( void ) 			{ return NULL; }
	virtual CBasePlayer *GetAssistant( void );

	virtual void	Precache( void );
	virtual void	Spawn( void );

	virtual int		GetDamageType();

	virtual void	Detonate( void );
	virtual void	VPhysicsCollision( int index, gamevcollisionevent_t *pEvent );

	virtual void	StunBallTouch( CBaseEntity *pOther );
	const char		*GetTrailParticleName( void );
	void			CreateTrail( void );

	void			SetScorer( CBaseEntity *pScorer );

	void			SetCritical( bool bCritical )	{ m_bCritical = bCritical; }

	bool			IsDestroyable( void ) OVERRIDE { return false; }

	virtual bool	IsDeflectable();
	virtual void	Deflected( CBaseEntity *pDeflectedBy, Vector &vecDir );

	virtual void	Explode( trace_t *pTrace, int bitsDamageType );

	bool			CanStun( CTFPlayer *pOther );

#else
	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual void	CreateTrails( void );
	virtual int		DrawModel( int flags );
#endif

protected:
#ifdef GAME_DLL
	CNetworkVar( bool, m_bCritical );

	CHandle<CBaseEntity>	m_hEnemy;
	EHANDLE					m_hScorer;
	EHANDLE					m_hSpriteTrail;
#else
	bool					m_bCritical;
#endif

	float					m_flCreationTime;
};

class CTFBauble : public CTFStunBall
{
public:
	DECLARE_CLASS( CTFBauble, CTFStunBall );
	DECLARE_NETWORKCLASS();
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	virtual int			GetWeaponID( void ) const 	{ return TF_WEAPON_BAT_GIFTWRAP; }

#ifdef GAME_DLL
	static CTFBauble 	*Create( CBaseEntity *pWeapon, const Vector &vecOrigin, const QAngle &vecAngles, const Vector &vecVelocity, CBaseCombatCharacter *pOwner, CBaseEntity *pScorer, const AngularImpulse &angVelocity, const CTFWeaponInfo &weaponInfo );

	virtual void	Precache( void );
	virtual void	Spawn( void );
	
	virtual void	Deflected( CBaseEntity *pDeflectedBy, Vector &vecDir );

	virtual void	VPhysicsCollision( int index, gamevcollisionevent_t *pEvent );
	void			VPhysicsCollisionThink( void );
	void			StunBallTouch( CBaseEntity *pOther ) OVERRIDE;

	void			SetCritical( bool bCritical )	{ m_bCritical = bCritical; }

	virtual void	Explode( trace_t *pTrace, int bitsDamageType );
	virtual void	Shatter( trace_t *pTrace, int bitsDamageType );

#endif

private:
#ifdef GAME_DLL
	CNetworkVar( bool, m_bCritical );
#else
	bool				m_bCritical;
#endif
	Vector				m_vecCollisionVelocity;
};

#endif // TF_PROJECTILE_STUNBALL_H