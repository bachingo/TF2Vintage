//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#ifndef EYEBALL_BOSS_DEAD_H
#define EYEBALL_BOSS_DEAD_H
#ifdef _WIN32
#pragma once
#endif

#include "NextBotBehavior.h"
#include "eyeball_boss.h"

class CEyeBallBossDead : public Action<CEyeBallBoss>
{
public:
	CEyeBallBossDead();
	virtual ~CEyeBallBossDead() { }

	virtual const char *GetName( void ) const override;

	virtual ActionResult<CEyeBallBoss> OnStart( CEyeBallBoss *me, Action<CEyeBallBoss> *priorAction ) override;
	virtual ActionResult<CEyeBallBoss> Update( CEyeBallBoss *me, float dt ) override;

private:
	CountdownTimer m_dyingDuration;
};

#endif