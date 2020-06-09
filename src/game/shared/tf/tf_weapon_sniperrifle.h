//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: TF Sniper Rifle
//
//=============================================================================//
#ifndef TF_WEAPON_SNIPERRIFLE_H
#define TF_WEAPON_SNIPERRIFLE_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"
#include "Sprite.h"


#if defined( CLIENT_DLL )
#define CTFSniperRifle C_TFSniperRifle
#define CSniperDot C_SniperDot
#endif

#ifdef GAME_DLL
#include "GameEventListener.h"
#endif


//=============================================================================
//
// Sniper Rifle Laser Dot class.
//
class CSniperDot : public CBaseEntity
{
public:

	DECLARE_CLASS( CSniperDot, CBaseEntity );
	DECLARE_NETWORKCLASS();
	DECLARE_DATADESC();

	// Creation/Destruction.
	CSniperDot( void );
	~CSniperDot( void );

	static CSniperDot *Create( const Vector &origin, CBaseEntity *pOwner = NULL, bool bVisibleDot = true );
	void		ResetChargeTime( void ) { m_flChargeStartTime = gpGlobals->curtime; }

	// Attributes.
	int			ObjectCaps()							{ return (BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | FCAP_DONT_SAVE; }

	// Targeting.
	void        Update( CBaseEntity *pTarget, const Vector &vecOrigin, const Vector &vecNormal );
	CBaseEntity	*GetTargetEntity( void )				{ return m_hTargetEnt; }

// Client specific.
#ifdef CLIENT_DLL

	// Rendering.
	virtual bool			IsTransparent( void )		{ return true; }
	virtual RenderGroup_t	GetRenderGroup( void )		{ return RENDER_GROUP_TRANSLUCENT_ENTITY; }
	virtual int				DrawModel( int flags );
	virtual bool			ShouldDraw( void );

	//
	virtual void			OnDataChanged( DataUpdateType_t updateType );

	CMaterialReference		m_hSpriteMaterial;

#endif

protected:

	Vector					m_vecSurfaceNormal;
	EHANDLE					m_hTargetEnt;

	CNetworkVar( float, m_flChargeStartTime );
};

#define TF_WEAPON_SNIPERRIFLE_CHARGE_PER_SEC	50.0f

//=============================================================================
//
// Sniper Rifle class.
//
class CTFSniperRifle : public CTFWeaponBaseGun
{
public:

	DECLARE_CLASS( CTFSniperRifle, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFSniperRifle();
	~CTFSniperRifle();

	virtual int	GetWeaponID( void ) const			{ return TF_WEAPON_SNIPERRIFLE; }
	virtual bool	ShouldDrawCrosshair( void )						{ return false; }
	virtual void Spawn();
	virtual void Precache();
	void		 ResetTimers( void );

	virtual bool Reload( void );
	virtual bool CanHolster( void ) const;
	virtual bool Holster( CBaseCombatWeapon *pSwitchingTo );

	void		 HandleZooms( void );
	virtual void DoFireEffects( void );
	virtual void ItemPostFrame( void );
	virtual bool Lower( void );
	virtual float GetProjectileDamage( void );
	virtual int	GetDamageType() const;
	virtual float GetChargingRate( void )		{ return TF_WEAPON_SNIPERRIFLE_CHARGE_PER_SEC; }

	virtual void WeaponReset( void );

	virtual bool CanFireCriticalShot( bool bIsHeadshot = false );

	float		 GetJarateTime( void );

	bool 		IsPenetrating(void);
	
	virtual bool	HasFocus( void );
	virtual bool	HasChargeBar( void );
	virtual const char* GetEffectLabelText( void ) { return "#TF_SniperRage"; }
	virtual void	ActivateFocus( void );
	
	void CreateSniperDot(void);
	void DestroySniperDot(void);
	void UpdateSniperDot(void);

#ifdef GAME_DLL
	CHandle<CSniperDot>		m_hSniperDot;
#endif

#ifdef CLIENT_DLL
	float GetHUDDamagePerc( void );
#endif

	bool IsZoomed( void );
	void DenySniperShot( void );
	
private:
	// Auto-rezooming handling
	void SetRezoom( bool bRezoom, float flDelay );

	void Zoom( void );
	void ZoomOutIn( void );
	void ZoomIn( void );
	void ZoomOut( void );
	void Fire( CTFPlayer *pPlayer );


public:

	CNetworkVar( float,	m_flChargedDamage );

private:

	// Handles rezooming after the post-fire unzoom
	float m_flUnzoomTime;
	float m_flRezoomTime;
	bool m_bRezoomAfterShot;
	
	// Deals with focus.
	float m_flFocusLevel;

	CTFSniperRifle( const CTFSniperRifle & );
};

// A sniper rifle with defined clipsize.

#if defined CLIENT_DLL
#define CTFSniperRifle_Real C_TFSniperRifle_Real
#endif

class CTFSniperRifle_Real : public CTFSniperRifle
{
public:

	DECLARE_CLASS( CTFSniperRifle_Real, CTFSniperRifle )
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	virtual int GetWeaponID( void ) const { return TF_WEAPON_SNIPERRIFLE_REAL; }
};


// Sniper logic used for the Bazaar Bargin.

#if defined CLIENT_DLL
#define CTFSniperRifle_Decap C_TFSniperRifle_Decap
#endif

class CTFSniperRifle_Decap : public CTFSniperRifle
#ifdef GAME_DLL
	, public CGameEventListener
#endif
{
public:

	DECLARE_CLASS( CTFSniperRifle_Decap, CTFSniperRifle )
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	virtual int GetWeaponID( void ) const { return TF_WEAPON_SNIPERRIFLE_DECAP; }
	virtual bool	HasChargeBar( void )			{ return true; }
	virtual const char* GetEffectLabelText( void ) { return "#TF_Berzerk"; }
	virtual bool	Deploy( void );
	virtual bool 	Holster( CBaseCombatWeapon *pSwitchingTo );
#ifdef GAME_DLL

	virtual void	SetupGameEventListeners( void );
	virtual void	FireGameEvent( IGameEvent *event );
#endif
	virtual void	OnHeadshot( void );
	virtual float 	GetChargingRate(void);

};


// Sniper logic used for the Classic.

#if defined CLIENT_DLL
#define CTFSniperRifle_Classic C_TFSniperRifle_Classic
#endif

class CTFSniperRifle_Classic : public CTFSniperRifle
{
public:

	CTFSniperRifle_Classic();
	~CTFSniperRifle_Classic();
	
	DECLARE_CLASS( CTFSniperRifle_Classic, CTFSniperRifle )
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	virtual int GetWeaponID( void ) const { return TF_WEAPON_SNIPERRIFLE_CLASSIC; }
	
	virtual void 	Precache();
	virtual void 	ItemPostFrame( void );
	virtual bool 	Holster( CBaseCombatWeapon *pSwitchingTo );
	bool			CanFire( void );
	
#ifdef CLIENT_DLL
	virtual void 	ToggleLaser( void );
private:
	HPARTICLEFFECT	m_pLaserSight;
#endif
	
private:
	
	void ZoomIn( void );
	void ZoomOut( void );
	
	bool m_bIsChargingAttack;
	void Fire(CTFPlayer *pPlayer);

};
#endif // TF_WEAPON_SNIPERRIFLE_H
