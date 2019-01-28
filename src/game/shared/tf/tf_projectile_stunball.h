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
#endif

#ifdef GAME_DLL
class CTFStunBall : public CTFWeaponBaseGrenadeProj, public IScorer
#else
class C_TFStunBall : public C_TFWeaponBaseGrenadeProj
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

	void			StunBallTouch( CBaseEntity *pOther );
	const char		*GetTrailParticleName( void );
	void			CreateTrail( void );

	void			SetScorer( CBaseEntity *pScorer );

	void			SetCritical( bool bCritical )	{ m_bCritical = bCritical; }

	virtual bool	IsDeflectable() 				{ return true; }
	virtual void	Deflected( CBaseEntity *pDeflectedBy, Vector &vecDir );

	virtual void	Explode( trace_t *pTrace, int bitsDamageType );

	bool			CanStun( CTFPlayer *pOther );

#else
	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual void	CreateTrails( void );
	virtual int		DrawModel( int flags );
#endif

private:
#ifdef GAME_DLL
	CNetworkVar( bool, m_bCritical );

	CHandle<CBaseEntity>	m_hEnemy;
	EHANDLE					m_Scorer;
	EHANDLE					m_hSpriteTrail;
#else
	bool					m_bCritical;
#endif

	float					m_flCreationTime;
};

#endif // TF_PROJECTILE_STUNBALL_H