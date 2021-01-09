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

	virtual const char *GetName( void ) const OVERRIDE;

	virtual ActionResult<CTFBot> OnStart( CTFBot *me, Action<CTFBot> *action ) OVERRIDE;
	virtual ActionResult<CTFBot> Update( CTFBot *me, float dt ) OVERRIDE;
	virtual void OnEnd( CTFBot *me, Action<CTFBot> *action ) OVERRIDE;
	virtual ActionResult<CTFBot> OnSuspend( CTFBot *me, Action<CTFBot> *action ) OVERRIDE;
	virtual ActionResult<CTFBot> OnResume( CTFBot *me, Action<CTFBot> *action ) OVERRIDE;

	virtual EventDesiredResult<CTFBot> OnStuck( CTFBot *me ) OVERRIDE;

	virtual QueryResultType ShouldRetreat( const INextBot *me ) const OVERRIDE;
	virtual QueryResultType ShouldAttack( const INextBot *me, const CKnownEntity *threat ) const OVERRIDE;
	virtual QueryResultType IsHindrance( const INextBot *me, CBaseEntity *it ) const OVERRIDE;

private:
	QueryResultType AreAllDangerousSentriesSapped( CTFBot *actor ) const;

	CHandle<CBaseObject> m_hTarget;
	CountdownTimer m_recomputePath;
	PathFollower m_PathFollower;
};

#endif