//========= Copyright � Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#ifndef EYEBALL_BOSS_ESCAPE_H
#define EYEBALL_BOSS_ESCAPE_H
#ifdef _WIN32
#pragma once
#endif

#include "NextBotBehavior.h"
#include "eyeball_boss.h"

class CEyeBallBossEscape : public Action<CEyeBallBoss>
{
public:
	CEyeBallBossEscape();
	virtual ~CEyeBallBossEscape() { }

	virtual const char *GetName( void ) const OVERRIDE;

	virtual ActionResult<CEyeBallBoss> OnStart( CEyeBallBoss *me, Action<CEyeBallBoss> *priorAction ) OVERRIDE;
	virtual ActionResult<CEyeBallBoss> Update( CEyeBallBoss *me, float dt ) OVERRIDE;
};

#endif