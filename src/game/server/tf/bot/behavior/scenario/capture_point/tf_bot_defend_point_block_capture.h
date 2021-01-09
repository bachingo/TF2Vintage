#ifndef TF_BOT_DEFEND_POINT_BLOCK_CAPTURE_H
#define TF_BOT_DEFEND_POINT_BLOCK_CAPTURE_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotBehavior.h"

class CTFBotDefendPointBlockCapture : public Action<CTFBot>
{
public:
	CTFBotDefendPointBlockCapture();
	virtual ~CTFBotDefendPointBlockCapture();

	virtual const char *GetName() const OVERRIDE;

	virtual ActionResult<CTFBot> OnStart( CTFBot *me, Action<CTFBot> *priorAaction ) OVERRIDE;
	virtual ActionResult<CTFBot> Update( CTFBot *me, float dt ) OVERRIDE;
	virtual ActionResult<CTFBot> OnResume( CTFBot *me, Action<CTFBot> *priorAaction ) OVERRIDE;

	virtual EventDesiredResult<CTFBot> OnMoveToSuccess( CTFBot *me, const Path *path ) OVERRIDE;
	virtual EventDesiredResult<CTFBot> OnMoveToFailure( CTFBot *me, const Path *path, MoveToFailureType fail ) OVERRIDE;
	virtual EventDesiredResult<CTFBot> OnStuck( CTFBot *me ) OVERRIDE;
	virtual EventDesiredResult<CTFBot> OnTerritoryContested( CTFBot *me, int territoryID ) OVERRIDE;
	virtual EventDesiredResult<CTFBot> OnTerritoryCaptured( CTFBot *me, int territoryID ) OVERRIDE;
	virtual EventDesiredResult<CTFBot> OnTerritoryLost( CTFBot *me, int territoryID ) OVERRIDE;

	virtual QueryResultType ShouldHurry( const INextBot *me ) const OVERRIDE;
	virtual QueryResultType ShouldRetreat( const INextBot *me ) const OVERRIDE;

private:
	bool IsPointSafe( CTFBot *actor );

	PathFollower m_PathFollower;
	CountdownTimer m_recomputePathTimer;
	CTeamControlPoint *m_pPoint;
	CTFNavArea *m_pCPArea;
};

#endif
