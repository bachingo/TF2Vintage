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

	virtual const char *GetName() const override;

	virtual ActionResult<CTFBot> OnStart( CTFBot *me, Action<CTFBot> *priorAction ) override;
	virtual ActionResult<CTFBot> Update( CTFBot *me, float dt ) override;
	virtual ActionResult<CTFBot> OnResume( CTFBot *me, Action<CTFBot> *priorAction ) override;

	virtual Action<CTFBot> *InitialContainedAction( CTFBot *me ) override;

	virtual EventDesiredResult<CTFBot> OnTerritoryLost( CTFBot *me, int territoryID ) override;

	virtual QueryResultType ShouldHurry( const INextBot *me ) const override;
	virtual QueryResultType ShouldAttack( const INextBot *me, const CKnownEntity *threat ) const override;
};

#endif
