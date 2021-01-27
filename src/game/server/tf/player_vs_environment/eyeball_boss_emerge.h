//========= Copyright � Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#ifndef EYEBALL_BOSS_EMERGE_H
#define EYEBALL_BOSS_EMERGE_H
#ifdef _WIN32
#pragma once
#endif

#include "NextBotBehavior.h"
#include "eyeball_boss.h"

class CEyeBallBossEmerge : public Action<CEyeBallBoss>
{
public:
	CEyeBallBossEmerge();
	virtual ~CEyeBallBossEmerge() { }

	virtual const char *GetName( void ) const OVERRIDE;

	virtual ActionResult<CEyeBallBoss> OnStart( CEyeBallBoss *me, Action<CEyeBallBoss> *priorAction ) OVERRIDE;
	virtual ActionResult<CEyeBallBoss> Update( CEyeBallBoss *me, float dt ) OVERRIDE;

private:
	CountdownTimer m_emergeTimer;
	CountdownTimer m_shakeTimer;
	Vector m_vecTarget;
	float m_flDistance;
};

#endif