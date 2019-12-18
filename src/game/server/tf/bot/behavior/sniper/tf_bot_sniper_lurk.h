#ifndef TF_BOT_SNIPER_LURK_H
#define TF_BOT_SNIPER_LURK_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotBehavior.h"

class CTFBotSniperLurk : public Action<CTFBot>
{
public:
	CTFBotSniperLurk();
	virtual ~CTFBotSniperLurk();

	virtual const char *GetName() const override;

	virtual ActionResult<CTFBot> OnStart( CTFBot *me, Action<CTFBot> *priorAction ) override;
	virtual ActionResult<CTFBot> Update( CTFBot *me, float dt ) override;
	virtual void OnEnd( CTFBot *actor, Action<CTFBot> *newAction ) override;
	virtual ActionResult<CTFBot> OnSuspend( CTFBot *me, Action<CTFBot> *newAction ) override;
	virtual ActionResult<CTFBot> OnResume( CTFBot *me, Action<CTFBot> *priorAction ) override;

	virtual QueryResultType ShouldRetreat( const INextBot *me ) const override;
	virtual QueryResultType ShouldAttack( const INextBot *me, const CKnownEntity *threat ) const override;

private:
	bool FindHint( CTFBot *actor );
	bool FindNewHome( CTFBot *actor );

	CountdownTimer m_patienceDuration;
	CountdownTimer m_recomputePathTimer;
	PathFollower m_PathFollower;
	int unused;
	Vector m_vecHome;
	bool m_bHasHome;
	bool m_bNearHome;
	CountdownTimer m_findHomeTimer;
	bool m_bOpportunistic;
	CUtlVector< CHandle<CTFBotHint> > m_Hints;
	CHandle<CTFBotHint> m_hHint;
};

#endif
