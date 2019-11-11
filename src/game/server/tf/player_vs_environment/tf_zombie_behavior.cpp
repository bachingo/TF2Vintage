#include "cbase.h"
#include "tf_zombie.h"
#include "tf_zombie_behavior.h"


char const *CZombieBehavior::GetName( void ) const
{
	return "ZombieBehavior";
}

ActionResult<CZombie> CZombieBehavior::OnStart( CZombie *me, Action<CZombie> *priorAction )
{
	return BaseClass::Continue();
}

ActionResult<CZombie> CZombieBehavior::Update( CZombie *me, float dt )
{
	return BaseClass::Continue();
}

EventDesiredResult<CZombie> CZombieBehavior::OnKilled( CZombie *me, CTakeDamageInfo const& info )
{
	return BaseClass::TryContinue();
}

Action<CZombie> *CZombieBehavior::InitialContainedAction( CZombie *me )
{
	return nullptr;
}
