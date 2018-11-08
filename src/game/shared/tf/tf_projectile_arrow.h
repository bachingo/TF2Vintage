//=============================================================================//
//
// Purpose: Arrow used by Huntsman.
//
//=============================================================================//

#ifndef TF_PROJECTILE_ARROW_H
#define TF_PROJECTILE_ARROW_H

#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_rocket.h"

#ifdef GAME_DLL
#include "iscorer.h"
#include "tf_player.h"
#endif

#ifdef CLIENT_DLL
#define CTFProjectile_Arrow C_TFProjectile_Arrow
#endif

#ifdef GAME_DLL
class CTFProjectile_Arrow : public CTFBaseRocket, public IScorer
#else
class CTFProjectile_Arrow : public C_TFBaseRocket
#endif
{
public:
	DECLARE_CLASS( CTFProjectile_Arrow, CTFBaseRocket );
	DECLARE_NETWORKCLASS();
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	CTFProjectile_Arrow();
	~CTFProjectile_Arrow();

	void				SetType( int iType ) 				{ m_iType = iType; }
	void				SetFlameArrow( bool bFlame ) 		{ m_bFlame = bFlame; }

	virtual int 		GetWeaponID( void ) const 			{ return TF_WEAPON_COMPOUND_BOW; }

#ifdef GAME_DLL
	static CTFProjectile_Arrow *Create( CBaseEntity *pWeapon, const Vector &vecOrigin, const QAngle &vecAngles, float flSpeed, float flGravity, bool bFlame, CBaseEntity *pOwner, CBaseEntity *pScorer, int iType );

	// IScorer interface
	virtual CBasePlayer *GetScorer( void );
	virtual CBasePlayer *GetAssistant( void ) { return NULL; }

	virtual void		Precache( void );
	virtual void		Spawn( void );
	virtual void		RemoveThink( void ) 			{ UTIL_Remove( this ); }

	void				SetScorer( CBaseEntity *pScorer );

	void				SetCritical( bool bCritical ) 	{ m_bCritical = bCritical; }
	virtual int			GetDamageType();

	virtual bool		IsDeflectable() 				{ return true; }
	virtual void		Deflected( CBaseEntity *pDeflectedBy, Vector &vecDir );

	bool				CanHeadshot( void );
	void				ArrowTouch( CBaseEntity *pOther );
	void				FlyThink( void );
	const char			*GetTrailParticleName( void );
	void				CreateTrail( void );
	void				BreakArrow( void );

	virtual void		UpdateOnRemove( void );

	// Arrow attachment functions
	bool				PositionArrowOnBone( mstudiobbox_t *pbox, CBaseAnimating *pAnim );
	void				GetBoneAttachmentInfo( mstudiobbox_t *pbox, CBaseAnimating *pAnim, Vector &vecOrigin, QAngle &vecAngles, int &bone, int &iPhysicsBone );
	void				CheckRagdollPinned( Vector &, Vector &, int, int, CBaseEntity *, int, int );

	void				PlayImpactSound( CTFPlayer *pAttacker, const char *pszImpactSound, bool bIsPlayerImpact = false );

#else
	virtual void		ClientThink( void );
	virtual void		OnDataChanged( DataUpdateType_t updateType );
	virtual void		Light( void );

	virtual void   		NotifyBoneAttached( C_BaseAnimating* attachTarget );

	// Tell the object when to die
	void				SetDieTime( float flDieTime ) 	{ m_flDieTime = flDieTime; }

#endif

private:
#ifdef GAME_DLL
	EHANDLE m_Scorer;
	CNetworkVar( bool, m_bCritical );
	CNetworkVar( int, m_iType );
	CNetworkVar( bool, m_bFlame);

	EHANDLE m_hSpriteTrail;
#else
	bool		bEmitting;
	bool		m_bCritical;
	bool		m_bFlame;
	int			m_iType;
	float		m_flDieTime;
	bool		m_bAttachment;
#endif
};

#endif // TF_PROJECTILE_ARROW_H
