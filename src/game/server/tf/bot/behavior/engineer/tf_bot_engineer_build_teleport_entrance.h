#ifndef TF_BOT_ENGINEER_BUILD_TELEPORT_ENTRENCE_H
#define TF_BOT_ENGINEER_BUILD_TELEPORT_ENTRENCE_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotBehavior.h"

class CTFBotEngineerBuildTeleportEntrance : public Action<CTFBot>
{
public:
	CTFBotEngineerBuildTeleportEntrance();
	virtual ~CTFBotEngineerBuildTeleportEntrance();

	virtual const char *GetName() const OVERRIDE;

	virtual ActionResult<CTFBot> OnStart( CTFBot *me, Action<CTFBot> *priorAction ) OVERRIDE;
	virtual ActionResult<CTFBot> Update( CTFBot *me, float dt ) OVERRIDE;

	virtual EventDesiredResult<CTFBot> OnStuck( CTFBot *me ) OVERRIDE;

private:
	PathFollower m_PathFollower;
};

#endif
