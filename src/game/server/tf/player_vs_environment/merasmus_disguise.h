//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#ifndef MERASMUS_DISGUISE_H
#define MERASMUS_DISGUISE_H
#ifdef _WIN32
#pragma once
#endif

#include "NextBotBehavior.h"
#include "merasmus.h"

class CMerasmusDisguise : public Action<CMerasmus>
{
	DECLARE_CLASS( CMerasmusDisguise, Action<CMerasmus> )
public:

	virtual ~CMerasmusDisguise() {}

	virtual char const *GetName( void ) const;

	virtual ActionResult<CMerasmus> OnStart( CMerasmus *me, Action<CMerasmus> *priorAction ) override;
	virtual ActionResult<CMerasmus> Update( CMerasmus *me, float dt ) override;
	virtual void OnEnd( CMerasmus *me, Action<CMerasmus> *newAction ) override;

private:
	void TryToDisguiseSpawn( CMerasmus *me );

	CountdownTimer m_giveUpDuration;
	CountdownTimer m_retryTimer;
	bool m_bDidSpawnProps;
	CountdownTimer m_tauntTimer;
	float m_flDisguiseStartTime;
	int m_nDisguiseStartHealth;
};

#endif