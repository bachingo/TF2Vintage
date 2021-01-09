//========= Copyright � Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#ifndef MERASMUS_ZAP_H
#define MERASMUS_ZAP_H
#ifdef _WIN32
#pragma once
#endif

#include "NextBotBehavior.h"
#include "merasmus.h"

class CMerasmusZap : public Action<CMerasmus>
{
	DECLARE_CLASS( CMerasmusZap, Action<CMerasmus> )
public:

	virtual ~CMerasmusZap() {}

	virtual char const *GetName( void ) const OVERRIDE;

	virtual ActionResult<CMerasmus> OnStart( CMerasmus *me, Action<CMerasmus> *priorAction ) OVERRIDE;
	virtual ActionResult<CMerasmus> Update( CMerasmus *me, float dt ) OVERRIDE;

private:
	void PlayCastSound( CMerasmus *me );

	int m_nZapType;
	CountdownTimer m_zapDelay;
};

#endif