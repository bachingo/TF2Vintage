#ifndef GHOST_BEHAVIOR_H
#define GHOST_BEHAVIOR_H

#ifdef _WIN32
#pragma once
#endif

#include "NextBotBehavior.h"
#include "ghost.h"

class CGhostBehavior : public Action<CGhost>
{
	DECLARE_CLASS( CGhostBehavior, Action<CGhost> );
public:

	virtual char const *GetName( void ) const OVERRIDE;

	virtual ActionResult<CGhost> OnStart( CGhost *me, Action< CGhost > *priorAction ) OVERRIDE;
	virtual ActionResult<CGhost> Update( CGhost *me, float interval ) OVERRIDE;

private:
	void DriftAroundAndAvoidObstacles( CGhost *me );

	CountdownTimer m_lifeTimer;
	CountdownTimer m_moanTimer;
	CountdownTimer m_scareTimer;
	Vector m_vecLastLocation;
	CountdownTimer m_movementTimer;
};

#endif