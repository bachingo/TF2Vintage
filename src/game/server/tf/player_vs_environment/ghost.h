//========= Copyright � Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#ifndef GHOST_H
#define GHOST_H

#ifdef _WIN32
#pragma once
#endif

#include "NextBot.h"
#include "NextBotBehavior.h"
#include "NextBotGroundLocomotion.h"
#include "Path/NextBotPathFollow.h"

class CGhostLocomotion : public NextBotGroundLocomotion
{
public:
	CGhostLocomotion( INextBot *me )
		: NextBotGroundLocomotion( me ) {}

	virtual float GetRunSpeed( void ) const { return 90; }

	virtual float GetMaxAcceleration( void ) const { return 500; }
	virtual float GetMaxDeceleration( void ) const { return 500; }
};

DECLARE_AUTO_LIST( IGhostAutoList );
class CGhost : public NextBotCombatCharacter, IGhostAutoList
{
	DECLARE_CLASS( CGhost, NextBotCombatCharacter );
public:
	
	DECLARE_INTENTION_INTERFACE( CGhost )

	CGhost();
	virtual ~CGhost();

	static CGhost *Create( const Vector &vecOrigin, const QAngle &vecAngles, float lifetime );

	virtual void Precache();
	virtual void Spawn( void );
	virtual Vector EyePosition( void );
	virtual bool ShouldCollide( int collisionGroup, int contentsMask ) const;

	void SetLifetime( float duration )							{ m_flLifetime = duration; }
	float GetLifetime( void ) const								{ return m_flLifetime; }

	virtual ILocomotion *GetLocomotionInterface( void ) const	{ return m_locomotor; }

private:
	void PrecacheGhost();

	CGhostLocomotion *m_locomotor;

	Vector m_vecEyeOffset;
	Vector m_vecHomePos;

	float m_flLifetime;
};


#endif