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

	virtual const char *GetName() const override;

	virtual ActionResult<CTFBot> OnStart( CTFBot *me, Action<CTFBot> *priorAction ) override;
	virtual ActionResult<CTFBot> Update( CTFBot *me, float dt ) override;

	virtual QueryResultType ShouldAttack( const INextBot *me, const CKnownEntity *threat ) const override;

private:
	CountdownTimer m_patienceDuration;
};

#endif
