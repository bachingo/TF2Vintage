//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "tf_zombie.h"
#include "zombie_spawn.h"
#include "zombie_attack.h"

char const *CZombieSpawn::GetName( void ) const
{
	return "Spawn";
}

ActionResult<CZombie> CZombieSpawn::OnStart( CZombie *me, Action<CZombie> *priorAction )
{
	me->GetBodyInterface()->StartActivity( ACT_TRANSITION );
	return BaseClass::Continue();
}

ActionResult<CZombie> CZombieSpawn::Update( CZombie *me, float dt )
{
	if ( me->IsSequenceFinished() )
		return BaseClass::ChangeTo( new CZombieAttack, "Start chasing." );

	return BaseClass::Continue();
}
