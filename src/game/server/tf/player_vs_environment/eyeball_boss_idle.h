//========= Copyright � Valve LLC, All rights reserved. =======================
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

	virtual const char *GetName( void ) const OVERRIDE;

	virtual ActionResult<CEyeBallBoss> OnStart( CEyeBallBoss *me, Action<CEyeBallBoss> *priorAction ) OVERRIDE;
	virtual ActionResult<CEyeBallBoss> OnResume( CEyeBallBoss *me, Action<CEyeBallBoss> *interruptingAction ) OVERRIDE;
	virtual ActionResult<CEyeBallBoss> Update( CEyeBallBoss *me, float dt ) OVERRIDE;

	virtual EventDesiredResult<CEyeBallBoss> OnInjured( CEyeBallBoss *me, const CTakeDamageInfo &info ) OVERRIDE;
	virtual EventDesiredResult<CEyeBallBoss> OnOtherKilled( CEyeBallBoss *me, CBaseCombatCharacter *pOther, const CTakeDamageInfo &info ) OVERRIDE;

private:
	CountdownTimer m_lookAroundTimer;
};

#endif