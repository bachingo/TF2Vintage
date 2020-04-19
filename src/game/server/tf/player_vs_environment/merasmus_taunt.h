//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#ifndef MERASMUS_TAUNT_H
#define MERASMUS_TAUNT_H
#ifdef _WIN32
#pragma once
#endif

#include "NextBotBehavior.h"
#include "merasmus.h"

class CMerasmusTaunt : public Action<CMerasmus>
{
	DECLARE_CLASS( CMerasmusTaunt, Action<CMerasmus> )
public:

	virtual ~CMerasmusTaunt() {}

	virtual char const *GetName( void ) const;

	virtual ActionResult<CMerasmus> OnStart( CMerasmus *me, Action<CMerasmus> *priorAction ) override;
	virtual ActionResult<CMerasmus> Update( CMerasmus *me, float dt ) override;
	virtual void OnEnd( CMerasmus *me, Action<CMerasmus> *newAction ) override;

private:
	CountdownTimer m_tauntDuration;
};

#endif