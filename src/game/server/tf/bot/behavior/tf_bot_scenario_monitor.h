#ifndef TF_BOT_SCENARIO_MONITOR_H
#define TF_BOT_SCENARIO_MONITOR_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotBehavior.h"

class CTFBotScenarioMonitor : public Action<CTFBot>
{
	DECLARE_CLASS( CTFBotScenarioMonitor, Action<CTFBot> );
public:
	virtual ~CTFBotScenarioMonitor() {}

	virtual const char *GetName( void ) const OVERRIDE;

	virtual ActionResult<CTFBot> OnStart( CTFBot *me, Action<CTFBot> *priorAction ) OVERRIDE;
	virtual ActionResult<CTFBot> Update( CTFBot *me, float dt ) OVERRIDE;

	virtual Action<CTFBot> *InitialContainedAction( CTFBot *actor ) OVERRIDE;

private:
	virtual Action<CTFBot> *DesiredScenarioAndClassAction( CTFBot *actor );

	CountdownTimer m_fetchFlagDelay;
	CountdownTimer m_fetchFlagDuration;
};

#endif