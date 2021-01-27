#ifndef TF_BOT_ENGINEER_BUILD_H
#define TF_BOT_ENGINEER_BUILD_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotBehavior.h"

class CTFBotEngineerBuild : public Action<CTFBot>
{
public:
	CTFBotEngineerBuild();
	virtual ~CTFBotEngineerBuild();

	virtual const char *GetName() const OVERRIDE;

	virtual ActionResult<CTFBot> OnStart( CTFBot *me, Action<CTFBot> *priorAction ) OVERRIDE;
	virtual ActionResult<CTFBot> Update( CTFBot *me, float dt ) OVERRIDE;
	virtual ActionResult<CTFBot> OnResume( CTFBot *me, Action<CTFBot> *priorAction ) OVERRIDE;

	virtual Action<CTFBot> *InitialContainedAction( CTFBot *me ) OVERRIDE;

	virtual EventDesiredResult<CTFBot> OnTerritoryLost( CTFBot *me, int territoryID ) OVERRIDE;

	virtual QueryResultType ShouldHurry( const INextBot *me ) const OVERRIDE;
	virtual QueryResultType ShouldAttack( const INextBot *me, const CKnownEntity *threat ) const OVERRIDE;
};

#endif
