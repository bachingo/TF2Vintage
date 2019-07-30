#include "cbase.h"
#include "NavMeshEntities/func_nav_prerequisite.h"
#include "../../tf_bot.h"
#include "tf_bot_nav_ent_destroy_entity.h"
#include "tf_weapon_pipebomblauncher.h"


CTFBotNavEntDestroyEntity::CTFBotNavEntDestroyEntity( const CFuncNavPrerequisite *prereq )
{
	m_hPrereq = prereq;
}

CTFBotNavEntDestroyEntity::~CTFBotNavEntDestroyEntity()
{
}


const char *CTFBotNavEntDestroyEntity::GetName() const
{
	return "NavEntDestroyEntity";
}


ActionResult<CTFBot> CTFBotNavEntDestroyEntity::OnStart( CTFBot *me, Action<CTFBot> *priorAction )
{
	// TODO
	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotNavEntDestroyEntity::Update( CTFBot *me, float dt )
{
	// TODO
	return Action<CTFBot>::Continue();
}

void CTFBotNavEntDestroyEntity::OnEnd( CTFBot *me, Action<CTFBot> *newAction )
{
	// TODO
}


void CTFBotNavEntDestroyEntity::DetonateStickiesWhenSet( CTFBot *actor, CTFPipebombLauncher *launcher ) const
{
	// TODO
}
