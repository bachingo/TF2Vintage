#ifndef TF_BOT_ENGINEER_MOVE_TO_BUILD_H
#define TF_BOT_ENGINEER_MOVE_TO_BUILD_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotBehavior.h"
#include "map_entities/tf_hint_sentrygun.h"

class CTFBotEngineerMoveToBuild : public Action<CTFBot>
{
public:
	CTFBotEngineerMoveToBuild();
	virtual ~CTFBotEngineerMoveToBuild();

	virtual const char *GetName() const override;

	virtual ActionResult<CTFBot> OnStart( CTFBot *me, Action<CTFBot> *priorAction ) override;
	virtual ActionResult<CTFBot> Update( CTFBot *me, float dt ) override;

	virtual EventDesiredResult<CTFBot> OnMoveToSuccess( CTFBot *me, const Path *path ) override;
	virtual EventDesiredResult<CTFBot> OnMoveToFailure( CTFBot *me, const Path *path, MoveToFailureType fail ) override;
	virtual EventDesiredResult<CTFBot> OnStuck( CTFBot *me ) override;
	virtual EventDesiredResult<CTFBot> OnTerritoryLost( CTFBot *me, int territoryID ) override;

private:
	void CollectBuildAreas( CTFBot *actor );
	void SelectBuildLocation( CTFBot *actor );

	CHandle<CTFBotHintSentrygun> m_hSentryHint;
	Vector m_vecBuildLocation;
	PathFollower m_PathFollower;
	CountdownTimer m_recomputePathTimer;
	CUtlVector<CTFNavArea *> m_buildAreas;
	float m_flArea;
	CountdownTimer m_resetBuildLocationTimer;
};

#endif
