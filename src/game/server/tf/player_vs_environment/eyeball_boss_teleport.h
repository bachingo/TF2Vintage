//========= Copyright � Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#ifndef EYEBALL_BOSS_TELEPORT_H
#define EYEBALL_BOSS_TELEPORT_H
#ifdef _WIN32
#pragma once
#endif

#include "NextBotBehavior.h"
#include "eyeball_boss.h"

class CEyeBallBossTeleport : public Action<CEyeBallBoss>
{
public:
	CEyeBallBossTeleport();
	virtual ~CEyeBallBossTeleport() { }

	virtual const char *GetName( void ) const OVERRIDE;

	virtual ActionResult<CEyeBallBoss> OnStart( CEyeBallBoss *me, Action<CEyeBallBoss> *priorAction ) OVERRIDE;
	virtual ActionResult<CEyeBallBoss> Update( CEyeBallBoss *me, float dt ) OVERRIDE;

private:
	enum
	{
		TELEPORT_VANISH,
		TELEPORT_APPEAR,
		TELEPORT_FINISH
	};
	int m_iTeleportState;
};

#endif