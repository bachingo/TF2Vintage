//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#ifndef EYEBALL_BOSS_EMOTE_H
#define EYEBALL_BOSS_EMOTE_H
#ifdef _WIN32
#pragma once
#endif

#include "NextBotBehavior.h"
#include "eyeball_boss.h"

class CEyeBallBossEmote : public Action<CEyeBallBoss>
{
public:
	CEyeBallBossEmote();
	CEyeBallBossEmote( int sequence, const char *sound = nullptr, Action<CEyeBallBoss> *nextAction = nullptr );
	virtual ~CEyeBallBossEmote() { }

	virtual const char *GetName( void ) const override;

	virtual ActionResult<CEyeBallBoss> OnStart( CEyeBallBoss *me, Action<CEyeBallBoss> *priorAction ) override;
	virtual ActionResult<CEyeBallBoss> Update( CEyeBallBoss *me, float dt ) override;

private:
	int m_iSequence;
	const char *m_pszActionSound;
	Action<CEyeBallBoss> *m_nextAction;
};

#endif