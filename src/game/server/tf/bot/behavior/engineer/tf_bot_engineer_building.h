#ifndef TF_BOT_ENGINEER_BUILDING_H
#define TF_BOT_ENGINEER_BUILDING_H
#ifdef _WIN32
#pragma once
#endif

#include "NextBotBehavior.h"
#include "map_entities/tf_hint_sentrygun.h"


class CTFBotEngineerBuilding : public Action<CTFBot>
{
public:
	CTFBotEngineerBuilding( CTFBotHintSentrygun *hint=nullptr );
	virtual ~CTFBotEngineerBuilding();

	enum eAvailability
	{
		UNKNOWN,
		UNAVAILABLE,
		AVAILABLE
	};

	virtual const char *GetName() const override;

	virtual ActionResult<CTFBot> OnStart( CTFBot *me, Action<CTFBot> *priorAction ) override;
	virtual ActionResult<CTFBot> Update( CTFBot *me, float dt ) override;
	virtual void OnEnd( CTFBot *me, Action<CTFBot> *newAction ) override;
	virtual ActionResult<CTFBot> OnResume( CTFBot *me, Action<CTFBot> *priorAction ) override;

	virtual EventDesiredResult<CTFBot> OnTerritoryCaptured( CTFBot *me, int territoryID ) override;
	virtual EventDesiredResult<CTFBot> OnTerritoryLost( CTFBot *me, int territoryID ) override;

private:
	bool CheckIfSentryIsOutOfPosition( CTFBot *actor ) const;
	bool PickTeleportLocation( CTFBot *actor, Vector *pLocation, float &pYaw );
	Vector FindHiddenSpot( CTFNavArea *pPointArea, const CUtlVector<CTFNavArea *> &surroundingAreas );
	bool IsMetalSourceNearby( CTFBot *actor ) const;
	void UpgradeAndMaintainBuildings( CTFBot *actor );

	int m_iTries;
	CountdownTimer m_recomputePathTimer;
	CountdownTimer m_buildDispenserTimer;
	CountdownTimer m_buildTeleportTimer;
	PathFollower m_PathFollower;
	CHandle<CTFBotHintSentrygun> m_hHint;
	bool m_bHadASentry;
	int m_iMetalSource;
	CountdownTimer m_outOfPositionTimer;
	bool m_bIsOutOfPosition;
};

#endif
