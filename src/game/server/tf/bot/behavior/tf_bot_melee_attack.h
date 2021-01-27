#ifndef TF_BOT_MELEE_ATTACK_H
#define TF_BOT_MELEE_ATTACK_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotBehavior.h"
#include "Path/NextBotChasePath.h"

class CTFBotMeleeAttack : public Action<CTFBot>
{
public:
	CTFBotMeleeAttack( float flAbandonRange = -1.0f );
	virtual ~CTFBotMeleeAttack();

	virtual const char *GetName() const OVERRIDE;

	virtual ActionResult<CTFBot> OnStart( CTFBot *actor, Action<CTFBot> *priorAction ) OVERRIDE;
	virtual ActionResult<CTFBot> Update( CTFBot *actor, float dt ) OVERRIDE;

private:
	float m_flAbandonRange;
	ChasePath m_ChasePath;
};

#endif
