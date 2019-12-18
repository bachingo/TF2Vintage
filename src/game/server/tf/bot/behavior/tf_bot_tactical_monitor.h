#ifndef TF_BOT_TACTICAL_MONITOR_H
#define TF_BOT_TACTICAL_MONITOR_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotBehavior.h"

class CObjectTeleporter;

class CTFBotTacticalMonitor : public Action<CTFBot>
{
	DECLARE_CLASS( CTFBotTacticalMonitor, Action<CTFBot> );
public:
	virtual ~CTFBotTacticalMonitor() {}

	virtual const char *GetName( void ) const override;

	virtual ActionResult<CTFBot> OnStart( CTFBot *me, Action<CTFBot> *priorAction ) override;
	virtual ActionResult<CTFBot> Update( CTFBot *me, float dt ) override;

	virtual Action<CTFBot> *InitialContainedAction( CTFBot *actor ) override;

	virtual EventDesiredResult<CTFBot> OnOtherKilled( CTFBot *me, CBaseCombatCharacter *who, const CTakeDamageInfo& info ) override;
	virtual EventDesiredResult<CTFBot> OnNavAreaChanged( CTFBot *me, CNavArea *area1, CNavArea *area2 ) override;
	virtual EventDesiredResult<CTFBot> OnCommandString( CTFBot *me, const char *cmd ) override;

private:
	void AvoidBumpingEnemies( CTFBot *actor );
	CObjectTeleporter *FindNearbyTeleporter( CTFBot *actor );
	void MonitorArmedStickybombs( CTFBot *actor );
	bool ShouldOpportunisticallyTeleport( CTFBot *actor ) const;

	CountdownTimer m_checkUseTeleportTimer;
	// 40 CountdownTimer (related to taunting at humans)
	// 4c CountdownTimer (related to taunting at humans)
	// 58 CountdownTimer (related to taunting at humans)
	CountdownTimer m_stickyMonitorDelay;
	CountdownTimer m_takeTeleporterTimer;
};

#endif