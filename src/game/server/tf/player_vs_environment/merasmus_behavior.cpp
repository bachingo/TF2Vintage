//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "tf_gamerules.h"
#include "merasmus_behavior.h"
#include "merasmus_dying.h"
#include "merasmus_reveal.h"
#include "merasmus_disguise.h"
#include "merasmus_escape.h"


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *CMerasmusBehavior::GetName( void ) const
{
	return "Behavior";
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
Action<CMerasmus> *CMerasmusBehavior::InitialContainedAction( CMerasmus *me )
{
	return new CMerasmusReveal;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
ActionResult<CMerasmus> CMerasmusBehavior::OnStart( CMerasmus *me, Action<CMerasmus> *priorAction )
{
	return Continue();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
ActionResult<CMerasmus> CMerasmusBehavior::Update( CMerasmus *me, float dt )
{
	if ( !me->IsAlive() )
	{
		if ( !me->DidSpawnWithCheats() )
		{
			for ( int i=0; i<me->m_lastAttackers.Count(); ++i )
			{
				if ( !me->m_lastAttackers[i].m_hPlayer || ( gpGlobals->curtime - me->m_lastAttackers[i].m_flTimeDamaged ) > 5.0f )
					continue;

				CReliableBroadcastRecipientFilter filter;
				UTIL_SayText2Filter( filter, me->m_lastAttackers[i].m_hPlayer, false, "#TF_Halloween_Boss_Killers", me->m_lastAttackers[i].m_hPlayer->GetPlayerName() );
			}
		}

		//TODO: Achievement hunters

		TFGameRules()->SetIT( nullptr );

		return ChangeTo( new CMerasmusDying, "I am dead!" );
	}

	const float flLifeTime = me->m_lifeTimeDuration.GetRemainingTime();
	if ( flLifeTime < 10 && me->m_flTimeLeftAlive > 10 )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "merasmus_escape_warning" );
		if ( event )
		{
			event->SetInt( "level", me->GetLevel() );
			event->SetInt( "timeremaining", 10 );

			gameeventmanager->FireEvent( event );
		}
	}
	else if ( flLifeTime < 30 && me->m_flTimeLeftAlive > 30 )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "merasmus_escape_warning" );
		if ( event )
		{
			event->SetInt( "level", me->GetLevel() );
			event->SetInt( "timeremaining", 30 );

			gameeventmanager->FireEvent( event );
		}
	}
	else if ( flLifeTime < 60 && me->m_flTimeLeftAlive > 60 )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "merasmus_escape_warning" );
		if ( event )
		{
			event->SetInt( "level", me->GetLevel() );
			event->SetInt( "timeremaining", 60 );

			gameeventmanager->FireEvent( event );
		}
	}

	me->m_flTimeLeftAlive = me->m_lifeTimeDuration.GetRemainingTime();

	if ( me->ShouldLeave() && !me->m_bStunned )
		return ChangeTo( new CMerasmusEscape, "I leave!" );

	return Continue();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
EventDesiredResult<CMerasmus> CMerasmusBehavior::OnInjured( CMerasmus *me, const CTakeDamageInfo &info )
{
	if ( me->m_bStunned || !me->m_bRevealed )
		return TryContinue();

	if ( me->ShouldDisguise() )
		return TryChangeTo( new CMerasmusDisguise, RESULT_IMPORTANT, "Time to hide and heal." );

	return TryContinue();
}

QueryResultType CMerasmusBehavior::IsPositionAllowed( const INextBot *me, const Vector& pos ) const
{
	return ANSWER_YES;
}
