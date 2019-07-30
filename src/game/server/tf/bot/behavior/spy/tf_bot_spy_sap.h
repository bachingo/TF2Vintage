#ifndef TF_BOT_SPY_SAP_H
#define TF_BOT_SPY_SAP_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotBehavior.h"

class CBaseObject;

class CTFBotSpySap : public Action<CTFBot>
{
public:
	CTFBotSpySap( CBaseObject *target );
	virtual ~CTFBotSpySap();

	virtual const char *GetName( void ) const override;

	virtual ActionResult<CTFBot> OnStart( CTFBot *me, Action<CTFBot> *action ) override;
	virtual ActionResult<CTFBot> Update( CTFBot *me, float dt ) override;
	virtual void OnEnd( CTFBot *me, Action<CTFBot> *action ) override;
	virtual ActionResult<CTFBot> OnSuspend( CTFBot *me, Action<CTFBot> *action ) override;
	virtual ActionResult<CTFBot> OnResume( CTFBot *me, Action<CTFBot> *action ) override;

	virtual EventDesiredResult<CTFBot> OnStuck( CTFBot *me ) override;

	virtual QueryResultType ShouldRetreat( const INextBot *me ) const override;
	virtual QueryResultType ShouldAttack( const INextBot *me, const CKnownEntity *threat ) const override;
	virtual QueryResultType IsHindrance( const INextBot *me, CBaseEntity *it ) const override;

private:
	QueryResultType AreAllDangerousSentriesSapped( CTFBot *actor ) const;

	CHandle<CBaseObject> m_hTarget;
	CountdownTimer m_recomputePath;
	PathFollower m_PathFollower;
};

#endif