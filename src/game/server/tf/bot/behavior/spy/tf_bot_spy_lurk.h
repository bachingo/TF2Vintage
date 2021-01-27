#ifndef TF_BOT_SPY_LURK_H
#define TF_BOT_SPY_LURK_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotBehavior.h"

class CTFBotSpyLurk : public Action<CTFBot>
{
public:
	CTFBotSpyLurk();
	virtual ~CTFBotSpyLurk();

	virtual const char *GetName() const OVERRIDE;

	virtual ActionResult<CTFBot> OnStart( CTFBot *me, Action<CTFBot> *priorAction ) OVERRIDE;
	virtual ActionResult<CTFBot> Update( CTFBot *me, float dt ) OVERRIDE;

	virtual QueryResultType ShouldAttack( const INextBot *me, const CKnownEntity *threat ) const OVERRIDE;

private:
	CountdownTimer m_patienceDuration;
};

#endif
