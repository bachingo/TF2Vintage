//========= Copyright � Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#ifndef TF_ZOMBIE_SPECIAL_ATTACK_H
#define TF_ZOMBIE_SPECIAL_ATTACK_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotBehavior.h"

class CZombieSpecialAttack : public Action<CZombie>
{
	DECLARE_CLASS( CZombieSpecialAttack, Action<CZombie> )
public:
	virtual const char *GetName( void ) const OVERRIDE;

	virtual ActionResult<CZombie> OnStart( CZombie *me, Action <CZombie> *priorAction ) OVERRIDE;
	virtual ActionResult<CZombie> Update( CZombie *me, float dt ) OVERRIDE;

private:
	void DoSpecialAttack( CZombie *actor );
	CountdownTimer m_timeUntilAttack;
};

#endif
