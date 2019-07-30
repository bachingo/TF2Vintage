#ifndef TF_BOT_ROAM_H
#define TF_BOT_ROAM_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotBehavior.h"

class CTFBotRoam : public Action<CTFBot>
{
	DECLARE_CLASS( CTFBotRoam, Action<CTFBot> )
public:
	CTFBotRoam();
	virtual ~CTFBotRoam();

	virtual const char *GetName( void ) const override;

	virtual ActionResult<CTFBot> OnStart( CTFBot *me, Action<CTFBot> *priorAction ) override;
	virtual ActionResult<CTFBot> Update( CTFBot *me, float dt ) override;

	virtual EventDesiredResult<CTFBot> OnStuck( CTFBot *me ) override;

private:
	PathFollower m_PathFollower;
	CountdownTimer m_waitDuration;
};

#endif