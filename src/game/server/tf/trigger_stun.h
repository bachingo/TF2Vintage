//=============================================================================//
//
// Purpose: Stun trigger.
//
//=============================================================================//
#ifndef TRIGGER_STUN_H
#define TRIGGER_STUN_H
#ifdef _WIN32
#pragma once
#endif

#include "triggers.h"

class CTriggerStun : public CBaseTrigger
{
	DECLARE_CLASS( CTriggerStun, CBaseTrigger );

public:
	void	Spawn( void );
	void	Touch( CBaseEntity *pOther );
	void	EndTouch( CBaseEntity *pOther );

	bool	StunEntity( CBaseEntity *pOther );
	void	StunThink( void );

private:
	DECLARE_DATADESC();

	COutputEvent	m_outputOnStun;	// Fired a stun

	float			m_flTriggerDelay;
	float			m_flStunDuration;
	float			m_flMoveSpeedReduction;
	int				m_iStunType;
	bool			m_bStunEffects;

	CUtlVector< CBaseEntity *>	m_stunEntities;
	
};

#endif // TRIGGER_STUN_H
