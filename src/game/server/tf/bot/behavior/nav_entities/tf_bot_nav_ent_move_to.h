#ifndef TF_BOT_NAV_ENT_MOVE_TO_H
#define TF_BOT_NAV_ENT_MOVE_TO_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotBehavior.h"

class CFuncNavPrerequisite;

class CTFBotNavEntMoveTo : public Action<CTFBot>
{
public:
	CTFBotNavEntMoveTo( const CFuncNavPrerequisite *prereq );
	virtual ~CTFBotNavEntMoveTo();

	virtual const char *GetName( void ) const override;

	virtual ActionResult<CTFBot> OnStart( CTFBot *me, Action<CTFBot> *priorAction ) override;
	virtual ActionResult<CTFBot> Update( CTFBot *me, float dt ) override;

private:
	CHandle<CFuncNavPrerequisite> m_hPrereq;
	Vector m_vecGoalPos;
	CNavArea *m_GoalArea;
	CountdownTimer m_waitDuration;
	PathFollower m_PathFollower;
	CountdownTimer m_recomputePath;
};

#endif