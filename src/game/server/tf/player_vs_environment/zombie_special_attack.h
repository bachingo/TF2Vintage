//========= Copyright © Valve LLC, All rights reserved. =======================
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
	virtual const char *GetName( void ) const;

	virtual ActionResult<CZombie> OnStart( CZombie *me, Action <CZombie> *priorAction ) override;
	virtual ActionResult<CZombie> Update( CZombie *me, float dt ) override;

private:
	void DoSpecialAttack( CZombie *actor );
	CountdownTimer m_timeUntilAttack;
};

#endif
