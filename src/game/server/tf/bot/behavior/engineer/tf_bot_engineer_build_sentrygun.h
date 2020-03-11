#ifndef TF_BOT_ENGINEER_BUILD_SENTRYGUN_H
#define TF_BOT_ENGINEER_BUILD_SENTRYGUN_H
#ifdef _WIN32
#pragma once
#endif

#include "NextBotBehavior.h"
#include "map_entities/tf_hint_sentrygun.h"


class CTFBotEngineerBuildSentryGun : public Action<CTFBot>
{
public:
	CTFBotEngineerBuildSentryGun( CTFBotHintSentrygun *hint=nullptr );
	virtual ~CTFBotEngineerBuildSentryGun();

	virtual const char *GetName() const override;

	virtual ActionResult<CTFBot> OnStart( CTFBot *me, Action<CTFBot> *priorAction ) override;
	virtual ActionResult<CTFBot> Update( CTFBot *me, float dt ) override;
	virtual ActionResult<CTFBot> OnResume( CTFBot *me, Action<CTFBot> *priorAction ) override;

private:
	CountdownTimer m_shimmyTimer;
	CountdownTimer m_fetchAmmoTimer;
	// 004c CountdownTimer
	CountdownTimer m_recomputePathTimer; // +0x0058
	// 0064 CountdownTimer
	int m_iTries;
	PathFollower m_PathFollower;      // +0x0074
	CHandle<CTFBotHintSentrygun> m_pHint;     // +0x4848
	Vector m_vecTarget;               // +0x484c
	int m_iShimmyDirection;
	// 485c bool
	// 4860 
	// 4864 
	// 4868 
};

#endif
