#include "cbase.h"
#include "../tf_bot.h"
#include "tf_bot_taunt.h"


CTFBotTaunt::CTFBotTaunt()
{
}

CTFBotTaunt::~CTFBotTaunt()
{
}


const char *CTFBotTaunt::GetName( void ) const
{
	return "Taunt";
}


ActionResult<CTFBot> CTFBotTaunt::OnStart( CTFBot *me, Action<CTFBot> *priorAction )
{
	m_waitDuration.Start( RandomFloat( 0.0f, 1.0f ) );

	m_bTaunting = false;

	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotTaunt::Update( CTFBot *me, float dt )
{
	if ( !m_waitDuration.IsElapsed() )
	{
		return Action<CTFBot>::Continue();
	}

	if ( !m_bTaunting )
	{
		//actor->HandleTauntCommand(0);
		me->Taunt();

		m_tauntTimer.Start( RandomFloat( 3.0f, 5.0f ) );

		m_bTaunting = true;

		return Action<CTFBot>::Continue();
	}

	//if (m_tauntTimer.IsElapsed() && actor + 0x2228 == 3) {
	//	actor->EndLongTaunt();
	//}

	if ( !me->m_Shared.InCond( TF_COND_TAUNTING ) )
	{
		return Action<CTFBot>::Done( "Taunt finished" );
	}

	return Action<CTFBot>::Continue();
}
