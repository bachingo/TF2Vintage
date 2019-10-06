//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#ifndef EYEBALL_BOSS_IDLE_H
#define EYEBALL_BOSS_IDLE_H
#ifdef _WIN32
#pragma once
#endif

#include "NextBotBehavior.h"
#include "eyeball_boss.h"

class CEyeBallBossIdle : public Action<CEyeBallBoss>
{
public:
	CEyeBallBossIdle();
	virtual ~CEyeBallBossIdle() { }

	virtual const char *GetName( void ) const override;

	virtual ActionResult<CEyeBallBoss> OnStart( CEyeBallBoss *me, Action<CEyeBallBoss> *priorAction ) override;
	virtual ActionResult<CEyeBallBoss> OnResume( CEyeBallBoss *me, Action<CEyeBallBoss> *interruptingAction ) override;
	virtual ActionResult<CEyeBallBoss> Update( CEyeBallBoss *me, float dt ) override;

	virtual EventDesiredResult<CEyeBallBoss> OnInjured( CEyeBallBoss *me, const CTakeDamageInfo &info ) override;
	virtual EventDesiredResult<CEyeBallBoss> OnOtherKilled( CEyeBallBoss *me, CBaseCombatCharacter *pOther, const CTakeDamageInfo &info ) override;

private:
	CountdownTimer m_lookAroundTimer;
};

#endif