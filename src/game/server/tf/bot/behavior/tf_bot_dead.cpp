#include "cbase.h"
#include "../tf_bot.h"
#include "tf_bot_dead.h"
#include "tf_bot_behavior.h"

CTFBotDead::CTFBotDead()
{
	m_flDeathTimestamp = 0.0f;
}

CTFBotDead::~CTFBotDead()
{
}

const char *CTFBotDead::GetName() const
{
	return "Dead";
}

ActionResult<CTFBot> CTFBotDead::OnStart( CTFBot *me, Action<CTFBot> *priorAction )
{
	m_flDeathTimestamp = gpGlobals->curtime;
	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotDead::Update( CTFBot *me, float interval )
{
	if ( ( m_flDeathTimestamp + 3.0f ) > gpGlobals->curtime )
	{
		// I need some time to adjust after what just happened
		return Action<CTFBot>::Continue();
	}

	if ( me->IsAlive() )
	{
		return Action<CTFBot>::ChangeTo( new CTFBotMainAction, "This should not happen!" );
	}

	if ( ( me->m_nBotAttrs & CTFBot::AttributeType::REMOVEONDEATH ) == CTFBot::AttributeType::REMOVEONDEATH )
	{
		engine->ServerCommand( UTIL_VarArgs( "kickid %d\n", me->GetUserID() ) );
		return Action<CTFBot>::Continue();
	}

	if ( ( me->m_nBotAttrs & CTFBot::AttributeType::BECOMESPECTATORONDEATH ) == CTFBot::AttributeType::BECOMESPECTATORONDEATH )
	{
		me->ChangeTeam( TEAM_SPECTATOR, false, true );
		return Action<CTFBot>::Done();
	}

	return Action<CTFBot>::Continue();
}