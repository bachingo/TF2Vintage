//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#ifndef MERASMUS_BEHAVIOR_H
#define MERASMUS_BEHAVIOR_H
#ifdef _WIN32
#pragma once
#endif

#include "NextBotBehavior.h"
#include "merasmus.h"

class CMerasmusBehavior : public Action<CMerasmus>
{
	DECLARE_CLASS( CMerasmusBehavior, Action<CMerasmus> )
public:
	virtual ~CMerasmusBehavior() { };

	virtual const char *GetName( void ) const;

	virtual Action<CMerasmus> *InitialContainedAction( CMerasmus *me ) override;

	virtual ActionResult<CMerasmus> OnStart( CMerasmus *me, Action<CMerasmus> *priorAction ) override;
	virtual ActionResult<CMerasmus> Update( CMerasmus *me, float dt ) override;

	virtual EventDesiredResult<CMerasmus> OnInjured( CMerasmus *me, const CTakeDamageInfo &info ) override;

	virtual QueryResultType IsPositionAllowed( const INextBot *me, const Vector& pos ) const override;
};

#endif