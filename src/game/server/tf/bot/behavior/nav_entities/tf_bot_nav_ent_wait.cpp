#include "cbase.h"
#include "NavMeshEntities/func_nav_prerequisite.h"
#include "../../tf_bot.h"
#include "tf_bot_nav_ent_wait.h"


CTFBotNavEntWait::CTFBotNavEntWait(const CFuncNavPrerequisite *prereq)
{
	if (prereq != nullptr) {
		m_hPrereq = prereq;
	}
}

CTFBotNavEntWait::~CTFBotNavEntWait()
{
}


const char *CTFBotNavEntWait::GetName() const
{
	return "NavEntWait";
}


ActionResult<CTFBot> CTFBotNavEntWait::OnStart(CTFBot *me, Action<CTFBot> *priorAction)
{
	if (m_hPrereq == nullptr) {
		return Action<CTFBot>::Done("Prerequisite has been removed before we started");
	}
	
	// TODO
	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotNavEntWait::Update(CTFBot *me, float dt)
{
	// TODO
	return Action<CTFBot>::Continue();
}
