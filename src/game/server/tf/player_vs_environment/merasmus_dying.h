//========= Copyright � Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#ifndef MERASMUS_DYING_H
#define MERASMUS_DYING_H
#ifdef _WIN32
#pragma once
#endif

#include "NextBotBehavior.h"
#include "merasmus.h"

class CMerasmusDying : public Action<CMerasmus>
{
	DECLARE_CLASS( CMerasmusDying, Action<CMerasmus> )
public:

	virtual ~CMerasmusDying() {}

	virtual char const *GetName( void ) const OVERRIDE;

	virtual ActionResult<CMerasmus> OnStart( CMerasmus *me, Action<CMerasmus> *priorAction ) OVERRIDE;
	virtual ActionResult<CMerasmus> Update( CMerasmus *me, float dt ) OVERRIDE;
};

#endif