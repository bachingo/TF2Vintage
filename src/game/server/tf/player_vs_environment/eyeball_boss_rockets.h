//========= Copyright � Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#ifndef EYEBALL_BOSS_ROCKETS_H
#define EYEBALL_BOSS_ROCKETS_H
#ifdef _WIN32
#pragma once
#endif

#include "NextBotBehavior.h"
#include "eyeball_boss.h"

class CEyeBallBossLaunchRockets : public Action<CEyeBallBoss>
{
public:
	CEyeBallBossLaunchRockets();
	virtual ~CEyeBallBossLaunchRockets() { }

	virtual const char *GetName( void ) const OVERRIDE;

	virtual ActionResult<CEyeBallBoss> OnStart( CEyeBallBoss *me, Action<CEyeBallBoss> *priorAction ) OVERRIDE;
	virtual ActionResult<CEyeBallBoss> Update( CEyeBallBoss *me, float dt ) OVERRIDE;

private:
	CountdownTimer m_rocketLaunchDelay;
	CountdownTimer m_refireDelay;
	int m_iNumRockets;
	Vector m_vecShootAt;
};

#endif