//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "tf_zombie.h"
#include "zombie_behavior.h"
#include "zombie_spawn.h"


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
	if ( !me->IsAlive() || me->ShouldSuicide() )
	{
		UTIL_Remove( me );
		return BaseClass::Done();
	}

	if ( !m_giggleTimer.HasStarted() )
	{
		switch ( me->m_nType )
		{
			case CZombie::MINION:
				m_giggleTimer.Start( RandomFloat( 2.0, 3.0 ) );
				break;
			case CZombie::KING:
				m_giggleTimer.Start( RandomFloat( 6.0, 7.0 ) );
				break;
			case CZombie::NORMAL:
			default:
				m_giggleTimer.Start( RandomFloat( 4.0, 5.0 ) );
				break;
		}
	}
	else if ( m_giggleTimer.IsElapsed() )
	{
		char const *pszSoundName = "Halloween_skeleton_laugh_medium";
		switch ( me->m_nType )
		{
			case CZombie::MINION:
				pszSoundName = "Halloween_skeleton_laugh_small";
				break;
			case CZombie::KING:
				pszSoundName = "Halloween_skeleton_laugh_giant";
				break;
			default:
				break;
		}

		me->EmitSound( pszSoundName );
	}

	return BaseClass::Continue();
}

EventDesiredResult<CZombie> CZombieBehavior::OnKilled( CZombie *me, CTakeDamageInfo const& info )
{
	UTIL_Remove( me );
	return BaseClass::TryDone();
}

QueryResultType CZombieBehavior::IsPositionAllowed( INextBot const *me, Vector const &position ) const
{
	return ANSWER_YES;
}

Action<CZombie> *CZombieBehavior::InitialContainedAction( CZombie *me )
{
	return new CZombieSpawn;
}
