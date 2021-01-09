#ifndef TF_BOT_CAPTURE_POINT_H
#define TF_BOT_CAPTURE_POINT_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotBehavior.h"

class CTFBotCapturePoint : public Action<CTFBot>
{
public:
	CTFBotCapturePoint();
	virtual ~CTFBotCapturePoint();

	virtual const char *GetName() const OVERRIDE;

	virtual ActionResult<CTFBot> OnStart( CTFBot *me, Action<CTFBot> *priorAction ) OVERRIDE;
	virtual ActionResult<CTFBot> Update( CTFBot *me, float dt ) OVERRIDE;
	virtual ActionResult<CTFBot> OnResume( CTFBot *me, Action<CTFBot> *priorAction ) OVERRIDE;

	virtual EventDesiredResult<CTFBot> OnMoveToSuccess( CTFBot *me, const Path *path ) OVERRIDE;
	virtual EventDesiredResult<CTFBot> OnMoveToFailure( CTFBot *me, const Path *path, MoveToFailureType fail ) OVERRIDE;
	virtual EventDesiredResult<CTFBot> OnStuck( CTFBot *me ) OVERRIDE;
	virtual EventDesiredResult<CTFBot> OnTerritoryContested( CTFBot *me, int territoryID ) OVERRIDE;
	virtual EventDesiredResult<CTFBot> OnTerritoryCaptured( CTFBot *me, int territoryID ) OVERRIDE;
	virtual EventDesiredResult<CTFBot> OnTerritoryLost( CTFBot *me, int territoryID ) OVERRIDE;

	virtual QueryResultType ShouldHurry( const INextBot *me ) const OVERRIDE;
	virtual QueryResultType ShouldRetreat( const INextBot *me ) const OVERRIDE;

private:
	PathFollower m_PathFollower;
	CountdownTimer m_recomputePathTimer;
};

#endif