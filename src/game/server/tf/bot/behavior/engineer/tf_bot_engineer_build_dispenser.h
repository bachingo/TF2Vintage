#ifndef TF_BOT_ENGINEER_BUILD_DISPENSER_H
#define TF_BOT_ENGINEER_BUILD_DISPENSER_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotBehavior.h"

class CTFBotEngineerBuildDispenser : public Action<CTFBot>
{
public:
	CTFBotEngineerBuildDispenser();
	virtual ~CTFBotEngineerBuildDispenser();

	virtual const char *GetName() const OVERRIDE;

	virtual ActionResult<CTFBot> OnStart( CTFBot *me, Action<CTFBot> *priorAction ) OVERRIDE;
	virtual ActionResult<CTFBot> Update( CTFBot *me, float dt ) OVERRIDE;
	virtual void OnEnd( CTFBot *me, Action<CTFBot> *newAction ) OVERRIDE;
	virtual ActionResult<CTFBot> OnResume( CTFBot *me, Action<CTFBot> *priorAction ) OVERRIDE;

private:
	CountdownTimer m_retryTimer;
	CountdownTimer m_fetchAmmoTimer;
	CountdownTimer m_recomputePathTimer;
	int m_iTries;
	PathFollower m_PathFollower;
};

#endif
