//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#ifndef TF_BOT_DELIVER_FLAG__H
#define TF_BOT_DELIVER_FLAG__H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotBehavior.h"

class CTFBotDeliverFlag : public Action<CTFBot>
{
	DECLARE_CLASS( CTFBotDeliverFlag, Action<CTFBot> );
public:
	virtual ~CTFBotDeliverFlag() {}

	virtual const char *GetName() const override;

	virtual ActionResult<CTFBot> OnStart( CTFBot *me, Action<CTFBot> *priorAction ) override;
	virtual ActionResult<CTFBot> Update( CTFBot *me, float dt ) override;
	virtual void OnEnd( CTFBot *me, Action<CTFBot> *newAction ) override;

	virtual QueryResultType ShouldHurry( const INextBot *me ) const override;
	virtual QueryResultType ShouldRetreat( const INextBot *me ) const override;

private:
	PathFollower m_PathFollower;
	CountdownTimer m_recomputePathTimer;
	// 4814 float
	// 4818 CountdownTimer
	// 4824 int
	// 4828 CountdownTimer
};


class CTFBotPushToCapturePoint : public Action<CTFBot>
{
	DECLARE_CLASS( CTFBotPushToCapturePoint, Action<CTFBot> );
public:
	CTFBotPushToCapturePoint( Action<CTFBot> *doneAction )
		: m_pDoneAction( doneAction ) {}
	virtual ~CTFBotPushToCapturePoint() {}

	virtual const char *GetName() const override;

	virtual ActionResult<CTFBot> Update( CTFBot *me, float dt ) override;

	virtual EventDesiredResult<CTFBot> OnNavAreaChanged( CTFBot *me, CNavArea *area1, CNavArea *area2 ) override;

private:
	PathFollower m_PathFollower;
	CountdownTimer m_recomputePathTimer;
	Action<CTFBot> *m_pDoneAction;
};

#endif
