#ifndef TF_BOT_APPROACH_OBJECT_H
#define TF_BOT_APPROACH_OBJECT_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotBehavior.h"

class CTFBotApproachObject : public Action<CTFBot>
{
public:
	CTFBotApproachObject( CBaseEntity *object, float dist );
	virtual ~CTFBotApproachObject();

	virtual const char *GetName() const OVERRIDE;

	virtual ActionResult<CTFBot> OnStart( CTFBot *actor, Action<CTFBot> *priorAction ) OVERRIDE;
	virtual ActionResult<CTFBot> Update( CTFBot *actor, float dt ) OVERRIDE;

private:
	CHandle<CBaseEntity> m_hObject;
	float m_flDist;
	PathFollower m_PathFollower;
	CountdownTimer m_recomputePathTimer;
};

#endif
