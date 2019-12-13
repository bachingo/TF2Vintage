//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#ifndef TF_BOT_ATTACK_FLAG_DEFENDERS_H
#define TF_BOT_ATTACK_FLAG_DEFENDERS_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotBehavior.h"
#include "behavior/tf_bot_attack.h"

class CTFBotAttackFlagDefenders : public CTFBotAttack
{
	DECLARE_CLASS( CTFBotAttackFlagDefenders, CTFBotAttack );
public:
	CTFBotAttackFlagDefenders( float duration = -1.0f );
	virtual ~CTFBotAttackFlagDefenders() {}

	virtual const char *GetName() const override;

	virtual ActionResult<CTFBot> OnStart( CTFBot *me, Action<CTFBot> *priorAction ) override;
	virtual ActionResult<CTFBot> Update( CTFBot *me, float dt ) override;

private:
	CountdownTimer m_actionDuration;
	CountdownTimer m_checkFlagTimer;
	CHandle<CTFPlayer> m_hTarget;
	PathFollower m_PathFollower;
	CountdownTimer m_recomputePathTimer;
};

#endif
