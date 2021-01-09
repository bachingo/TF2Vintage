//========= Copyright � Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#ifndef EYEBALL_BOSS_NOTICE_H
#define EYEBALL_BOSS_NOTICE_H
#ifdef _WIN32
#pragma once
#endif

#include "NextBotBehavior.h"
#include "eyeball_boss.h"

class CEyeBallBossNotice : public Action<CEyeBallBoss>
{
public:
	CEyeBallBossNotice();
	virtual ~CEyeBallBossNotice() { }

	virtual const char *GetName( void ) const OVERRIDE;

	virtual ActionResult<CEyeBallBoss> OnStart( CEyeBallBoss *me, Action<CEyeBallBoss> *priorAction ) OVERRIDE;
	virtual ActionResult<CEyeBallBoss> Update( CEyeBallBoss *me, float dt ) OVERRIDE;

private:
	CountdownTimer m_chaseDelay;
};

#endif