//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "eyeball_boss_emote.h"

CEyeBallBossEmote::CEyeBallBossEmote( int sequence, const char *sound, Action<CEyeBallBoss> *nextAction )
	: m_iSequence( sequence ), m_pszActionSound( sound )
{
	m_nextAction = nextAction;
}


const char *CEyeBallBossEmote::GetName( void ) const
{
	return "Emote";
}


ActionResult<CEyeBallBoss> CEyeBallBossEmote::OnStart( CEyeBallBoss *me, Action<CEyeBallBoss> *priorAction )
{
	if ( m_iSequence )
	{
		me->SetSequence( m_iSequence );
		me->SetPlaybackRate( 1.0f );
		me->SetCycle( 0.0f );
		me->ResetSequenceInfo();
	}

	if ( m_pszActionSound )
		me->EmitSound( m_pszActionSound );

	return Action<CEyeBallBoss>::Continue();
}

ActionResult<CEyeBallBoss> CEyeBallBossEmote::Update( CEyeBallBoss *me, float dt )
{
	if ( me->IsSequenceFinished() )
	{
		if ( m_nextAction )
		{
			return Action<CEyeBallBoss>::ChangeTo( m_nextAction );
		}
		else
		{
			return Action<CEyeBallBoss>::Done();
		}
	}

	return Action<CEyeBallBoss>::Continue();
}
