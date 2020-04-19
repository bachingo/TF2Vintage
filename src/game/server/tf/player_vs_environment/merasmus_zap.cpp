//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "merasmus_zap.h"
#include "merasmus_stunned.h"


char const *CMerasmusZap::GetName( void ) const
{
	return "Zap";
}


ActionResult<CMerasmus> CMerasmusZap::OnStart( CMerasmus *me, Action<CMerasmus> *priorAction )
{
	m_zapDelay.Start( 1.3 );
	m_nZapType = RandomInt( 0, 1 );

	me->GetBodyInterface()->StartActivity( ACT_RANGE_ATTACK2 );
	PlayCastSound( me );

	return Continue();
}

ActionResult<CMerasmus> CMerasmusZap::Update( CMerasmus *me, float dt )
{
	if ( me->IsStunned() )
		return ChangeTo( new CMerasmusStunned, "Stun interrupt!" );

	if ( me->IsSequenceFinished() )
		return Done( "Zapped!" );

	if ( m_zapDelay.HasStarted() && m_zapDelay.IsElapsed() )
	{
		m_zapDelay.Invalidate();

		const float flRadius = ( me->GetLevel() - 1 ) * 50.0 + 600.0;
		const float flMinDamage = ( 5 * me->GetLevel() - 5 ) + 20.0;
		const float flMaxDamage = ( 5 * me->GetLevel() - 5 ) + 50.0;

		const bool bZapped = CMerasmus::Zap( me, "effect_staff", flRadius, flMinDamage, flMaxDamage, me->GetLevel() + 5, TEAM_ANY );
		if ( bZapped )
			me->EmitSound( "Halloween.Merasmus_Spell" );
	}

	return Continue();
}


void CMerasmusZap::PlayCastSound( CMerasmus *me )
{
	CPVSFilter filter( me->WorldSpaceCenter() );
	if ( m_nZapType == 1 )
		me->PlayLowPrioritySound( filter, "Halloween.MerasmusLaunchSpell" );
	else
		me->PlayLowPrioritySound( filter, "Halloween.MerasmusCastFireSpell" );
}
