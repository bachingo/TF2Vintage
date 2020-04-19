//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "merasmus_stunned.h"
#include "merasmus_teleport.h"


char const *CMerasmusStunned::GetName( void ) const
{
	return "Stunned";
}


ActionResult<CMerasmus> CMerasmusStunned::OnStart( CMerasmus *me, Action<CMerasmus> *priorAction )
{
	const int nLayer = me->AddGesture( ACT_MP_STUN_BEGIN );
	const float flDuration = me->GetLayerDuration( nLayer );
	m_sequenceDuration.Start( flDuration );

	me->OnBeginStun();

	m_nStunState = STUN_LOOP;

	return Continue();
}

ActionResult<CMerasmus> CMerasmusStunned::Update( CMerasmus *me, float dt )
{
	if ( m_nStunState == STUN_FINISH && m_actionDelay.IsElapsed() )
	{
		if ( me->ShouldDisguise() )
			return Done();

		if ( me->GetNumberTimesStunned() > 2 )
			return ChangeTo( new CMerasmusTeleport( true, true ), "Teleport for AOE!" );

		return ChangeTo( new CMerasmusTeleport( false, false ), "Teleport to new area!" );
	}

	if ( !m_sequenceDuration.IsElapsed() )
		return Continue();

	if ( !me->IsStunned() )
	{
		switch ( m_nStunState )
		{
			case STUN_END:
			{
				m_nStunState = STUN_FINISH;

				const int nLayer = me->AddGesture( ACT_MP_STUN_END );
				const float flDuration = me->GetLayerDuration( nLayer );
				m_sequenceDuration.Start( flDuration );

				m_actionDelay.Start( flDuration + 0.5 );
			}
			break;
			case STUN_LOOP:
			{
				m_nStunState = STUN_END;

				const int nLayer = me->AddGesture( ACT_MP_STUN_MIDDLE );
				const float flDuration = me->GetLayerDuration( nLayer );
				m_sequenceDuration.Start( flDuration );
			}
			break;
		}

		return Continue();
	}

	const int nLayer = me->AddGesture( ACT_MP_STUN_MIDDLE );
	const float flDuration = me->GetLayerDuration( nLayer );
	m_sequenceDuration.Start( flDuration );

	m_nStunState = STUN_LOOP;

	return Continue();
}

void CMerasmusStunned::OnEnd( CMerasmus *me, Action<CMerasmus> *newAction )
{
	me->OnEndStun();
}
