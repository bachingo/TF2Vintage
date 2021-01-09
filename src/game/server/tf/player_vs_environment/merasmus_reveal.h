//========= Copyright � Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#ifndef MERASMUS_REVEAL_H
#define MERASMUS_REVEAL_H
#ifdef _WIN32
#pragma once
#endif

#include "NextBotBehavior.h"
#include "merasmus.h"

class CMerasmusReveal : public Action<CMerasmus>
{
	DECLARE_CLASS( CMerasmusReveal, Action<CMerasmus> )
public:

	virtual ~CMerasmusReveal() {}

	virtual char const *GetName( void ) const OVERRIDE;

	virtual ActionResult<CMerasmus> OnStart( CMerasmus *me, Action<CMerasmus> *priorAction ) OVERRIDE;
	virtual ActionResult<CMerasmus> Update( CMerasmus *me, float dt ) OVERRIDE;
};

#endif