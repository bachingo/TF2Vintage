#ifndef TF_BOT_NAV_ENT_DESTROY_ENTITY_H
#define TF_BOT_NAV_ENT_DESTROY_ENTITY_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotBehavior.h"

class CTFPipebombLauncher;
class CFuncNavPrerequisite;

class CTFBotNavEntDestroyEntity : public Action<CTFBot>
{
public:
	CTFBotNavEntDestroyEntity( const CFuncNavPrerequisite *prereq );
	virtual ~CTFBotNavEntDestroyEntity();

	virtual const char *GetName( void ) const override;

	virtual ActionResult<CTFBot> OnStart( CTFBot *me, Action<CTFBot> *priorAction ) override;
	virtual ActionResult<CTFBot> Update( CTFBot *me, float dt ) override;
	virtual void OnEnd( CTFBot *me, Action<CTFBot> *newAction ) override;

private:
	void DetonateStickiesWhenSet( CTFBot *actor, CTFPipebombLauncher *launcher ) const;

	CHandle<CFuncNavPrerequisite> m_hPrereq;
	PathFollower m_PathFollower;
	CountdownTimer m_recomputePath;
	// 4818 bool
};

#endif