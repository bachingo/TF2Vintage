#ifndef TF_BOT_DEFEND_POINT_H
#define TF_BOT_DEFEND_POINT_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotBehavior.h"
#include "Path/NextBotChasePath.h"

class CTFBotDefendPoint : public Action<CTFBot>
{
public:
	CTFBotDefendPoint();
	virtual ~CTFBotDefendPoint();

	virtual const char *GetName() const OVERRIDE;

	virtual ActionResult<CTFBot> OnStart( CTFBot *me, Action<CTFBot> *priorAction ) OVERRIDE;
	virtual ActionResult<CTFBot> Update( CTFBot *me, float dt ) OVERRIDE;
	virtual ActionResult<CTFBot> OnResume( CTFBot *me, Action<CTFBot> *priorAction ) OVERRIDE;

	virtual EventDesiredResult<CTFBot> OnContact( CTFBot *me, CBaseEntity *other, CGameTrace *trace ) OVERRIDE;
	virtual EventDesiredResult<CTFBot> OnMoveToSuccess( CTFBot *me, const Path *path ) OVERRIDE;
	virtual EventDesiredResult<CTFBot> OnMoveToFailure( CTFBot *me, const Path *path, MoveToFailureType fail ) OVERRIDE;
	virtual EventDesiredResult<CTFBot> OnStuck( CTFBot *me ) OVERRIDE;
	virtual EventDesiredResult<CTFBot> OnTerritoryContested( CTFBot *me, int territoryID ) OVERRIDE;
	virtual EventDesiredResult<CTFBot> OnTerritoryCaptured( CTFBot *me, int territoryID ) OVERRIDE;
	virtual EventDesiredResult<CTFBot> OnTerritoryLost( CTFBot *me, int territoryID ) OVERRIDE;

private:
	bool IsPointThreatened( CTFBot *actor );
	CTFNavArea *SelectAreaToDefendFrom( CTFBot *actor );
	bool WillBlockCapture( CTFBot *actor ) const;

	PathFollower m_PathFollower;
	ChasePath m_ChasePath;
	CountdownTimer m_pathRecomputeTimer;
	CountdownTimer unk2;
	CountdownTimer m_reselectDefenseAreaTimer;
	CTFNavArea *m_DefenseArea;
	bool m_bShouldRoam;
};

#endif