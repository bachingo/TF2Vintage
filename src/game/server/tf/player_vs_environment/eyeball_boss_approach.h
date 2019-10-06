//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#ifndef EYEBALL_BOSS_APPROACH_H
#define EYEBALL_BOSS_APPROACH_H
#ifdef _WIN32
#pragma once
#endif

#include "NextBotBehavior.h"
#include "eyeball_boss.h"

class CEyeBallBossApproachTarget : public Action<CEyeBallBoss>
{
public:
	CEyeBallBossApproachTarget();
	virtual ~CEyeBallBossApproachTarget() { }

	virtual const char *GetName( void ) const override;

	virtual ActionResult<CEyeBallBoss> OnStart( CEyeBallBoss *me, Action<CEyeBallBoss> *priorAction ) override;
	virtual ActionResult<CEyeBallBoss> Update( CEyeBallBoss *me, float dt ) override;

private:
	CountdownTimer m_targetVerifyTimer;
	CountdownTimer m_targetDuration;
	CountdownTimer m_keepTargetDuration;
};

#endif