//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#ifndef TF_BOT_FETCH_FLAG_H
#define TF_BOT_FETCH_FLAG_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotBehavior.h"

class CTFBotFetchFlag : public Action<CTFBot>
{
	DECLARE_CLASS( CTFBotFetchFlag, Action<CTFBot> );
public:
	CTFBotFetchFlag( bool bGiveUp )
		: m_bGiveUpWhenDone( bGiveUp ) {}
	virtual ~CTFBotFetchFlag() {}
	
	virtual const char *GetName() const override;
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *me, Action<CTFBot> *priorAction) override;
	virtual ActionResult<CTFBot> Update(CTFBot *me, float dt) override;
	
	virtual QueryResultType ShouldHurry(const INextBot *me) const override;
	virtual QueryResultType ShouldRetreat(const INextBot *me) const override;
	
private:
	bool m_bGiveUpWhenDone;           // +0x0032
	PathFollower m_PathFollower;      // +0x0034
	CountdownTimer m_recomputePathTimer; // +0x4808
};

#endif
