//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#ifndef MERASMUS_STAFFATTACK_H
#define MERASMUS_STAFFATTACK_H
#ifdef _WIN32
#pragma once
#endif

#include "NextBotBehavior.h"
#include "merasmus.h"

class CMerasmusStaffAttack : public Action<CMerasmus>
{
	DECLARE_CLASS( CMerasmusStaffAttack, Action<CMerasmus> )
public:

	CMerasmusStaffAttack( CTFPlayer *pTarget );
	virtual ~CMerasmusStaffAttack() {}

	virtual char const *GetName( void ) const;

	virtual ActionResult<CMerasmus> OnStart( CMerasmus *me, Action<CMerasmus> *priorAction ) override;
	virtual ActionResult<CMerasmus> Update( CMerasmus *me, float dt ) override;

private:
	CountdownTimer m_sequenceDuration;
	CountdownTimer m_attackDelay;
	CHandle<CTFPlayer> m_hVictim;
	PathFollower m_PathFollower;
};

#endif