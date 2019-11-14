//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#ifndef TF_ZOMBIE_SPAWN_H
#define TF_ZOMBIE_SPAWN_H
#ifdef _WIN32
#pragma once
#endif

#include "NextBotBehavior.h"

class CZombieSpawn : public Action<CZombie>
{
	DECLARE_CLASS( CZombieSpawn, Action<CZombie> )
public:
	virtual char const *GetName( void ) const;

	virtual ActionResult<CZombie> OnStart( CZombie *me, Action <CZombie> *priorAction ) override;
	virtual ActionResult<CZombie> Update( CZombie *me, float dt ) override;
};

#endif
