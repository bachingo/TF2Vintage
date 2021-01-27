//========= Copyright � Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#ifndef HEADLESS_HATMAN_EMERGE_H
#define HEADLESS_HATMAN_EMERGE_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotBehavior.h"
#include "headless_hatman.h"

class CHeadlessHatmanEmerge : public Action<CHeadlessHatman>
{
public:
	CHeadlessHatmanEmerge();
	virtual ~CHeadlessHatmanEmerge() { };

	virtual const char *GetName( void ) const OVERRIDE;

	virtual ActionResult<CHeadlessHatman> OnStart( CHeadlessHatman *me, Action<CHeadlessHatman> *priorAction ) OVERRIDE;
	virtual ActionResult<CHeadlessHatman> Update( CHeadlessHatman *me, float dt ) OVERRIDE;

private:
	CountdownTimer m_emergeTimer;
	CountdownTimer m_shakeTimer;
	Vector m_vecTarget;
	float m_flDistance;
};

#endif