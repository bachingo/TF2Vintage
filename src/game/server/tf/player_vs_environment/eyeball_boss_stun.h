//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#ifndef EYEBALL_BOSS_STUN_H
#define EYEBALL_BOSS_STUN_H
#ifdef _WIN32
#pragma once
#endif

#include "NextBotBehavior.h"
#include "eyeball_boss.h"

class CEyeBallBossStunned : public Action<CEyeBallBoss>
{
public:
	CEyeBallBossStunned();
	virtual ~CEyeBallBossStunned() { }

	virtual const char *GetName( void ) const override;

	virtual ActionResult<CEyeBallBoss> OnStart( CEyeBallBoss *me, Action<CEyeBallBoss> *priorAction ) override;
	virtual ActionResult<CEyeBallBoss> Update( CEyeBallBoss *me, float dt ) override;
	virtual void OnEnd( CEyeBallBoss *me, Action<CEyeBallBoss> *newAction ) override;

	virtual EventDesiredResult<CEyeBallBoss> OnInjured( CEyeBallBoss *me, const CTakeDamageInfo& info ) override;

private:
	CountdownTimer m_stunDuration;
};

#endif