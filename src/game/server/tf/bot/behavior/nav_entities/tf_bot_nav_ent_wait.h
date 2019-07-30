#ifndef TF_BOT_NAV_ENT_WAIT_H
#define TF_BOT_NAV_ENT_WAIT_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotBehavior.h"

class CFuncNavPrerequisite;

class CTFBotNavEntWait : public Action<CTFBot>
{
public:
	CTFBotNavEntWait( const CFuncNavPrerequisite *prereq );
	virtual ~CTFBotNavEntWait();

	virtual const char *GetName( void ) const override;

	virtual ActionResult<CTFBot> OnStart( CTFBot *me, Action<CTFBot> *priorAction ) override;
	virtual ActionResult<CTFBot> Update( CTFBot *me, float dt ) override;

private:
	CHandle<CFuncNavPrerequisite> m_hPrereq;
	CountdownTimer m_waitDuration;
};

#endif