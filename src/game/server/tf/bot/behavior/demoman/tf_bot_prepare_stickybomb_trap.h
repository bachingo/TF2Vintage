#ifndef TF_BOT_PREPARE_STICKY_TRAP_H
#define TF_BOT_PREPARE_STICKY_TRAP_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotBehavior.h"

class CTFBotPrepareStickybombTrap : public Action<CTFBot>
{
public:
	struct BombTargetArea
	{
		CTFNavArea *area;
		int stickies;
	};

	CTFBotPrepareStickybombTrap();
	virtual ~CTFBotPrepareStickybombTrap();

	virtual const char *GetName() const OVERRIDE;

	virtual ActionResult<CTFBot> OnStart( CTFBot *me, Action<CTFBot> *priorAction ) OVERRIDE;
	virtual ActionResult<CTFBot> Update( CTFBot *me, float dt ) OVERRIDE;
	virtual void OnEnd( CTFBot *me, Action<CTFBot> *newAction ) OVERRIDE;
	virtual ActionResult<CTFBot> OnSuspend( CTFBot *me, Action<CTFBot> *priorAction ) OVERRIDE;

	virtual EventDesiredResult<CTFBot> OnInjured( CTFBot *me, const CTakeDamageInfo& info ) OVERRIDE;

	virtual QueryResultType ShouldAttack( const INextBot *me, const CKnownEntity *threat ) const OVERRIDE;

	static bool IsPossible( CTFBot *actor );

private:
	void InitBombTargetAreas( CTFBot *actor );

	bool m_bReload;
	CTFNavArea *m_LastKnownArea;
	CUtlVector<BombTargetArea> m_BombTargetAreas;
	CountdownTimer m_aimDuration;
};

#endif
