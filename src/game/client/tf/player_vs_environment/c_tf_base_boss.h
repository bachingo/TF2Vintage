//========= Copyright � Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#ifndef C_TF_BASE_BOSS_H
#define C_TF_BASE_BOSS_H

#include "NextBot/c_NextBot.h"
#include "c_tf_mvm_boss_progress_user.h"

class C_TFBaseBoss : public C_NextBotCombatCharacter, public C_TFMvMBossProgressUser
{
	DECLARE_CLASS( C_TFBaseBoss, C_NextBotCombatCharacter );
public:
	DECLARE_CLIENTCLASS();

	virtual ~C_TFBaseBoss() {}

	ShadowType_t ShadowCastType( void );

	// ITFMvMBossProgressUser
	virtual float GetBossStatusProgress() const OVERRIDE { return m_lastHealthPercentage; }

private:

	float m_lastHealthPercentage;
};

#endif
