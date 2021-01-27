//========= Copyright � Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#ifndef MERASMUS_STUNNED_H
#define MERASMUS_STUNNED_H
#ifdef _WIN32
#pragma once
#endif

#include "NextBotBehavior.h"
#include "merasmus.h"

class CMerasmusStunned : public Action<CMerasmus>
{
	DECLARE_CLASS( CMerasmusStunned, Action<CMerasmus> )
public:

	virtual ~CMerasmusStunned() {}

	virtual char const *GetName( void ) const OVERRIDE;

	virtual ActionResult<CMerasmus> OnStart( CMerasmus *me, Action<CMerasmus> *priorAction ) OVERRIDE;
	virtual ActionResult<CMerasmus> Update( CMerasmus *me, float dt ) OVERRIDE;
	virtual void OnEnd( CMerasmus *me, Action<CMerasmus> *newAction ) OVERRIDE;

private:
	enum
	{
		STUN_LOOP,
		STUN_END,
		STUN_FINISH
	} m_nStunState;
	CountdownTimer m_sequenceDuration;
	CountdownTimer m_actionDelay;
};

#endif