#ifndef TF_BOT_TAUNT_H
#define TF_BOT_TAUNT_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotBehavior.h"

class CTFBotTaunt : public Action<CTFBot>
{
public:
	CTFBotTaunt();
	virtual ~CTFBotTaunt();

	virtual const char *GetName( void ) const OVERRIDE;

	virtual ActionResult<CTFBot> OnStart( CTFBot *me, Action<CTFBot> *priorAction ) OVERRIDE;
	virtual ActionResult<CTFBot> Update( CTFBot *me, float dt ) OVERRIDE;

private:
	CountdownTimer m_waitDuration;
	CountdownTimer m_tauntTimer;
	bool m_bTaunting;
};

#endif