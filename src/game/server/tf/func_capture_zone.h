//======= Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: CTF Flag Capture Zone.
//
//=============================================================================//
#ifndef FUNC_CAPTURE_ZONE_H
#define FUNC_CAPTURE_ZONE_H
#ifdef _WIN32
#pragma once
#endif

#include "triggers.h"

//-----------------------------------------------------------------------------
// Purpose: This class is to get around the fact that DEFINE_FUNCTION doesn't like multiple inheritance
//-----------------------------------------------------------------------------
class CCaptureZoneShim : public CBaseTrigger
{
	virtual void CaptureTouch( CBaseEntity *pOther ) = 0;

public:
	void Touch( CBaseEntity *pOther ) { return CaptureTouch( pOther ); }
};

//=============================================================================
//
// CTF Flag Capture Zone class.
//
DECLARE_AUTO_LIST( ICaptureZoneAutoList )
class CCaptureZone : public CCaptureZoneShim, public ICaptureZoneAutoList
{
	DECLARE_CLASS( CCaptureZone, CBaseTrigger );

public:
	DECLARE_SERVERCLASS();

	void	Spawn();

	virtual void CaptureTouch( CBaseEntity *pOther );

	bool	IsDisabled( void );
	void	SetDisabled( bool bDisabled );

	// Input handlers
	void	InputEnable( inputdata_t &inputdata );
	void	InputDisable( inputdata_t &inputdata );

	int		UpdateTransmitState( void );

private:

	bool			m_bDisabled;		// Enabled/Disabled?
	
	int				m_nCapturePoint;	// Used in non-CTF maps to identify this capture point

	COutputEvent	m_outputOnCapture;	// Fired a flag is captured on this point.

	DECLARE_DATADESC();

	float			m_flNextTouchingEnemyZoneWarning;	// don't spew warnings to the player who is touching the wrong cap
};

#endif // FUNC_CAPTURE_ZONE_H
