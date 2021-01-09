//========= Copyright � Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#ifndef MERASMUS_ESCAPE_H
#define MERASMUS_ESCAPE_H
#ifdef _WIN32
#pragma once
#endif

#include "NextBotBehavior.h"
#include "merasmus.h"

class CMerasmusEscape : public Action<CMerasmus>
{
	DECLARE_CLASS( CMerasmusEscape, Action<CMerasmus> )
public:

	virtual ~CMerasmusEscape() {}

	virtual char const *GetName( void ) const OVERRIDE;

	virtual ActionResult<CMerasmus> OnStart( CMerasmus *me, Action<CMerasmus> *priorAction ) OVERRIDE;
	virtual ActionResult<CMerasmus> Update( CMerasmus *me, float dt ) OVERRIDE;
};

#endif