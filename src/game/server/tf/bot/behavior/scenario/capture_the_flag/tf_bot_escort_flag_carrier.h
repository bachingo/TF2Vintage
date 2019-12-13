//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#ifndef TF_BOT_ESCORT_FLAG_CARRIER_H
#define TF_BOT_ESCORT_FLAG_CARRIER_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotBehavior.h"
#include "behavior/tf_bot_melee_attack.h"

class CTFBotEscortFlagCarrier : public Action<CTFBot>
{
	DECLARE_CLASS( CTFBotEscortFlagCarrier, Action<CTFBot> );
public:
	virtual ~CTFBotEscortFlagCarrier() {}

	virtual const char *GetName() const override;

	virtual ActionResult<CTFBot> OnStart( CTFBot *me, Action<CTFBot> *priorAction ) override;
	virtual ActionResult<CTFBot> Update( CTFBot *me, float dt ) override;

private:
	PathFollower m_PathFollower;
	CountdownTimer m_recomputePathTimer;
	CTFBotMeleeAttack m_MeleeAttack;
};


extern int GetBotEscortCount( int teamnum );

#endif