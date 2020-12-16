//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_WEAPON_BUILDER_H
#define TF_WEAPON_BUILDER_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase.h"

class CBaseObject;

//=========================================================
// Builder Weapon
//=========================================================
class CTFWeaponBuilder : public CTFWeaponBase
{
	DECLARE_CLASS( CTFWeaponBuilder, CTFWeaponBase );
public:
	CTFWeaponBuilder();
	~CTFWeaponBuilder();

	DECLARE_SERVERCLASS();

	virtual void	SetSubType( int iSubType );
	virtual void	SetObjectMode( int iObjectMode );
	virtual void	Precache( void );
	virtual bool	CanDeploy( void );
	virtual bool	CanHolster( void ) const;
	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo = NULL );
	virtual void	ItemPostFrame( void );
	virtual void	PrimaryAttack( void );
	virtual void	SecondaryAttack( void );
	virtual void	WeaponIdle( void );
	virtual bool	Deploy( void );	
	virtual Activity GetDrawActivity( void );
	virtual const char *GetViewModel( int iViewModel ) const;
	virtual const char *GetWorldModel( void ) const;

	virtual bool	AllowsAutoSwitchTo( void ) const;

	virtual int		GetType( void ) { return m_iObjectType; }

	void	SetCurrentState( int iState );
	void	SwitchOwnersWeaponToLast();

	// Placement
	void	StartPlacement( void );
	void	StopPlacement( void );
	void	UpdatePlacementState( void );		// do a check for valid placement
	bool	IsValidPlacement( void );			// is this a valid placement pos?


	// Building
	void	StartBuilding( void );

	// Selection
	bool	HasAmmo( void );
	int		GetSlot( void ) const;
	int		GetPosition( void ) const;
	const char *GetPrintName( void ) const;

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_BUILDER; }

	virtual void	WeaponReset( void );

	void	SetRoboSapper( bool bSet ) { m_bRoboSapper.Set( bSet ); }
	bool	IsRoboSapper( void ) const { return m_bRoboSapper; }

public:
	CNetworkVar( int, m_iBuildState );
	CNetworkVar( unsigned int, m_iObjectType );
	CNetworkVar( unsigned int, m_iObjectMode );

	CNetworkHandle( CBaseObject, m_hObjectBeingBuilt );

	CNetworkVar( bool, m_bRoboSapper );

	int m_iValidBuildPoseParam;

	float m_flNextDenySound;

protected:
	EHANDLE  m_hLastSappedBuilding;
	Vector   m_vLastSapPos;
};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTFWeaponSapper : public CTFWeaponBuilder
{
public:
	DECLARE_CLASS( CTFWeaponSapper, CTFWeaponBuilder );
	DECLARE_SERVERCLASS();

	CTFWeaponSapper();

	virtual void		WeaponReset( void );
	virtual void		WeaponIdle( void );
	virtual bool		Holster( CBaseCombatWeapon *pSwitchingTo );
	virtual const char *GetViewModel( int iViewModel ) const;
	virtual const char *GetWorldModel( void ) const;

	// All the special Wheatley functionality for the Ap-Sap
	void	WheatleySapperIdle( CTFPlayer *pOwner );
	bool	IsWheatleySapper( void );
	void	WheatleyReset( bool bResetIntro = false );
	void	SetWheatleyState( int iNewState ) { m_iWheatleyState = iNewState; }
	float	WheatleyEmitSound( const char *pSound , bool bEmitToAll = false, bool bNoRepeats = false );
	bool	IsWheatleyTalking( void ) { return gpGlobals->curtime <= m_flWheatleyTalkingUntil; }
	void	WheatleyDamage( void );
	int		GetWheatleyIdleWait() { return RandomInt( 10.0, 20.0 ); }

private:
	float		m_flWheatleyIdleTime;
	CNetworkVar( float, m_flWheatleyTalkingUntil );
	int			m_iWheatleyState;
	float		m_flWheatleyLastDamage;
	float		m_flWheatleyLastDeploy;
	float		m_flWheatleyLastHolster;
	int			m_iNextWheatleyVoiceLine;
	bool		m_bWheatleyIntroPlayed;
};

enum
{
	P2RECSTATE_IDLE,
	P2RECSTATE_HACKING,
	P2RECSTATE_HACKINGPW,
	P2RECSTATE_HACKED,
	P2RECSTATE_FOLLOWUP,
	P2RECSTATE_IDLE_KNIFE,
	P2RECSTATE_IDLE_HARMLESS,
	P2RECSTATE_IDLE_HACK,
	P2RECSTATE_INTRO
};

#endif // TF_WEAPON_BUILDER_H
