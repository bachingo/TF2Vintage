#ifndef TF_BOT_SPY_INFILTRATE_H
#define TF_BOT_SPY_INFILTRATE_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotBehavior.h"

class CTFBotSpyInfiltrate : public Action<CTFBot>
{
public:
	CTFBotSpyInfiltrate();
	virtual ~CTFBotSpyInfiltrate();

	virtual const char *GetName( void ) const override;

	virtual ActionResult<CTFBot> OnStart( CTFBot *me, Action<CTFBot> *priorAction ) override;
	virtual ActionResult<CTFBot> Update( CTFBot *me, float dt ) override;
	virtual ActionResult<CTFBot> OnSuspend( CTFBot *me, Action<CTFBot> *newAction ) override;
	virtual ActionResult<CTFBot> OnResume( CTFBot *me, Action<CTFBot> *priorAction ) override;

	virtual EventDesiredResult<CTFBot> OnStuck( CTFBot *me ) override;
	virtual EventDesiredResult<CTFBot> OnTerritoryCaptured( CTFBot *me, int territoryID ) override;
	virtual EventDesiredResult<CTFBot> OnTerritoryLost( CTFBot *me, int territoryID ) override;

	virtual QueryResultType ShouldAttack( const INextBot *me, const CKnownEntity *threat ) const override;

private:
	bool FindHidingSpot( CTFBot *actor );

	CountdownTimer m_recomputePath;
	PathFollower m_PathFollower;
	CTFNavArea *m_HidingArea;
	CountdownTimer m_findHidingAreaDelay;
	CountdownTimer m_waitDuration;
	bool m_bCloaked;
};

#endif