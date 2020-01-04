//====== Copyright © 1996-2006, Valve Corporation, All rights reserved. =======//
//
// Purpose: 
//
//=============================================================================//

#ifndef TF_GENERIC_BOMB_H
#define TF_GENERIC_BOMB_H
#ifdef _WIN32
#pragma once
#endif

#include "props.h"

//-----------------------------------------------------------------------------
// Purpose: This class is to get around the fact that DEFINE_FUNCTION doesn't like multiple inheritance
//-----------------------------------------------------------------------------
class CTFGenericBombShim : public CBaseAnimating
{
	virtual void BombTouch( CBaseEntity *pOther ) = 0;

public:
	void Touch( CBaseEntity *pOther ) { return BombTouch( pOther ); }
};


//=============================================================================
//
// TF Generic Bomb Class
//
DECLARE_AUTO_LIST( ITFGenericBomb );

class CTFGenericBomb : public CTFGenericBombShim, public ITFGenericBomb
{
public:
	DECLARE_CLASS( CTFGenericBomb, CBaseAnimating );
	DECLARE_DATADESC();

	CTFGenericBomb();
	virtual void		Precache( void );
	virtual void		Spawn( void );
	virtual void		Event_Killed( const CTakeDamageInfo &info );

	virtual void		InputDetonate( inputdata_t &inputdata );
	virtual void		BombTouch( CBaseEntity *pOther );

protected:
	float				m_flDamage;
	float				m_flRadius;
	string_t			m_iszParticleName;
	string_t			m_iszExplodeSound;
	bool				m_bFriendlyFire;

	COutputEvent		m_OnDetonate;
};



//-----------------------------------------------------------------------------
// Purpose: This class is to get around the fact that DEFINE_FUNCTION doesn't like multiple inheritance
//-----------------------------------------------------------------------------
class CTFPumpkinBombShim : public CBaseAnimating
{
	virtual void PumpkinTouch( CBaseEntity *pOther ) = 0;

public:
	void Touch( CBaseEntity *pOther ) { return PumpkinTouch( pOther ); }
};


//=============================================================================
//
// TF Generic Bomb Class
//
DECLARE_AUTO_LIST( ITFPumpkinBomb );

class CTFPumpkinBomb : public CTFPumpkinBombShim, public ITFPumpkinBomb
{
public:
	DECLARE_CLASS( CTFPumpkinBomb, CBaseAnimating );

	CTFPumpkinBomb();

	virtual void	Precache( void );
	virtual void	Spawn( void );
	virtual int		OnTakeDamage( CTakeDamageInfo const &info );
	virtual void	Event_Killed( CTakeDamageInfo const &info );

	virtual void	PumpkinTouch( CBaseEntity *pOther );

	void			RemovePumpkin( void );
	void			Break();

	bool	m_bSpell;
	
private:
	bool	m_bKilled;
	bool	m_bPrecached;
	int		m_iTeam;
	float	m_flDamage;
	float	m_flScale;
	float	m_flRadius;
	float	m_flLifeTime;
};


#endif // TF_GENERIC_BOMB_H