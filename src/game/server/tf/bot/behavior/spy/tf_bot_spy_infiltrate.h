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

	virtual const char *GetName( void ) const OVERRIDE;

	virtual ActionResult<CTFBot> OnStart( CTFBot *me, Action<CTFBot> *priorAction ) OVERRIDE;
	virtual ActionResult<CTFBot> Update( CTFBot *me, float dt ) OVERRIDE;
	virtual ActionResult<CTFBot> OnSuspend( CTFBot *me, Action<CTFBot> *newAction ) OVERRIDE;
	virtual ActionResult<CTFBot> OnResume( CTFBot *me, Action<CTFBot> *priorAction ) OVERRIDE;

	virtual EventDesiredResult<CTFBot> OnStuck( CTFBot *me ) OVERRIDE;
	virtual EventDesiredResult<CTFBot> OnTerritoryCaptured( CTFBot *me, int territoryID ) OVERRIDE;
	virtual EventDesiredResult<CTFBot> OnTerritoryLost( CTFBot *me, int territoryID ) OVERRIDE;

	virtual QueryResultType ShouldAttack( const INextBot *me, const CKnownEntity *threat ) const OVERRIDE;

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