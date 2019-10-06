//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#ifndef EYEBALL_BOSS_BEHAVIOR_H
#define EYEBALL_BOSS_BEHAVIOR_H
#ifdef _WIN32
#pragma once
#endif

#include "NextBotBehavior.h"
#include "eyeball_boss.h"

class CEyeBallBossBehavior : public Action<CEyeBallBoss>
{
public:
	CEyeBallBossBehavior();
	virtual ~CEyeBallBossBehavior() { }

	virtual const char *GetName( void ) const override;

	virtual Action<CEyeBallBoss> *InitialContainedAction( CEyeBallBoss *me ) override;

	virtual ActionResult<CEyeBallBoss> OnStart( CEyeBallBoss *me, Action<CEyeBallBoss> *priorAction ) override;
	virtual ActionResult<CEyeBallBoss> Update( CEyeBallBoss *me, float dt ) override;

	virtual EventDesiredResult<CEyeBallBoss> OnInjured( CEyeBallBoss *me, const CTakeDamageInfo &info ) override;
	virtual EventDesiredResult<CEyeBallBoss> OnKilled( CEyeBallBoss *me, const CTakeDamageInfo &info ) override;

private:
	CountdownTimer m_stunDelay;
};

#endif