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

	virtual const char *GetName() const override;

	virtual ActionResult<CTFBot> OnStart( CTFBot *me, Action<CTFBot> *priorAaction ) override;
	virtual ActionResult<CTFBot> Update( CTFBot *me, float dt ) override;
	virtual ActionResult<CTFBot> OnResume( CTFBot *me, Action<CTFBot> *priorAaction ) override;

	virtual EventDesiredResult<CTFBot> OnMoveToSuccess( CTFBot *me, const Path *path ) override;
	virtual EventDesiredResult<CTFBot> OnMoveToFailure( CTFBot *me, const Path *path, MoveToFailureType fail ) override;
	virtual EventDesiredResult<CTFBot> OnStuck( CTFBot *me ) override;
	virtual EventDesiredResult<CTFBot> OnTerritoryContested( CTFBot *me, int territoryID ) override;
	virtual EventDesiredResult<CTFBot> OnTerritoryCaptured( CTFBot *me, int territoryID ) override;
	virtual EventDesiredResult<CTFBot> OnTerritoryLost( CTFBot *me, int territoryID ) override;

	virtual QueryResultType ShouldHurry( const INextBot *me ) const override;
	virtual QueryResultType ShouldRetreat( const INextBot *me ) const override;

private:
	bool IsPointSafe( CTFBot *actor );

	PathFollower m_PathFollower;
	CountdownTimer m_recomputePathTimer;
	CTeamControlPoint *m_pPoint;
	CTFNavArea *m_pCPArea;
};

#endif
