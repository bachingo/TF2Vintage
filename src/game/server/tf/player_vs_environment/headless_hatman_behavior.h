//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#ifndef HEADLESS_HATMAN_BEHAVIOR_H
#define HEADLESS_HATMAN_BEHAVIOR_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotBehavior.h"
#include "headless_hatman.h"

class CHeadlessHatmanBehavior : public Action<CHeadlessHatman>
{
public:
	CHeadlessHatmanBehavior();
	virtual ~CHeadlessHatmanBehavior() { };

	virtual const char *GetName( void ) const override;

	virtual Action<CHeadlessHatman> *InitialContainedAction( CHeadlessHatman *me ) override;

	virtual ActionResult<CHeadlessHatman> Update( CHeadlessHatman *me, float dt ) override;

	virtual QueryResultType IsPositionAllowed( const INextBot *me, const Vector& pos ) const override;
};

#endif