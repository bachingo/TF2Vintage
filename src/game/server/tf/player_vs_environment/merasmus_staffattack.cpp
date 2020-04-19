//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "tf_player.h"
#include "merasmus_staffattack.h"
#include "merasmus_stunned.h"


ConVar tf_merasmus_attack_range( "tf_merasmus_attack_range", "200", FCVAR_CHEAT );


CMerasmusStaffAttack::CMerasmusStaffAttack( CTFPlayer *pTarget )
{
	m_hVictim = pTarget;
}


char const *CMerasmusStaffAttack::GetName( void ) const
{
	return "Staff Attack";
}


ActionResult<CMerasmus> CMerasmusStaffAttack::OnStart( CMerasmus *me, Action<CMerasmus> *priorAction )
{
	m_PathFollower.SetMinLookAheadDistance( 100.0f );

	const int nLayer = me->AddGesture( ACT_MP_ATTACK_STAND_MELEE );
	const float flDuration = me->GetLayerDuration( nLayer );
	m_sequenceDuration.Start( flDuration );

	m_attackDelay.Start( flDuration / 2 );

	if ( RandomInt( 0, 2 ) == 0 )
	{
		CPVSFilter filter( me->GetAbsOrigin() );
		if ( RandomInt( 1, 5 ) == 1 )
			me->PlayLowPrioritySound( filter, "Halloween.MerasmusStaffAttackRare" );
		else
			me->PlayLowPrioritySound( filter, "Halloween.MerasmusStaffAttack" );
	}

	return Continue();
}

ActionResult<CMerasmus> CMerasmusStaffAttack::Update( CMerasmus *me, float dt )
{
	if ( me->IsStunned() )
		return ChangeTo( new CMerasmusStunned, "Stun interrupt!" );

	if ( m_sequenceDuration.IsElapsed() )
		return Done();

	if( m_hVictim.Get() )
	{
		if ( m_attackDelay.HasStarted() && m_attackDelay.IsElapsed() )
		{
			m_attackDelay.Invalidate();

			Vector vecFwd;
			AngleVectors( me->GetAbsAngles(), &vecFwd );

			const float flDistance = me->GetRangeTo( m_hVictim );
			Vector vecToVictim = m_hVictim->WorldSpaceCenter() - me->WorldSpaceCenter();
			vecToVictim.NormalizeInPlace();

			float flDot = 0.0;
			if ( flDistance >= 100.0f )
				flDot = ( flDistance - 100.0f ) / ( tf_merasmus_attack_range.GetFloat() - 100.0f ) * 0.27f;

			if ( vecToVictim.Dot( vecFwd ) > flDot 
				 && me->IsRangeLessThan( m_hVictim, tf_merasmus_attack_range.GetFloat() * 0.9f ) 
				 && me->IsLineOfSightClear( m_hVictim ) )
			{
				CTakeDamageInfo info( me, me, 70.0, DMG_CLUB, TF_DMG_CUSTOM_MERASMUS_DECAPITATION );
				CalculateMeleeDamageForce( &info, vecToVictim, me->WorldSpaceCenter(), 5.0 );

				m_hVictim->TakeDamage( info );
				me->PushPlayer( m_hVictim, 500.0 );

				CPVSFilter filter( me->WorldSpaceCenter() );
				me->PlayLowPrioritySound( filter, "Halloween.HeadlessBossAxeHitFlesh" );
			}
		}

		if ( me->IsRangeGreaterThan( m_hVictim, 100.0f ) || !me->IsLineOfSightClear( m_hVictim ) )
		{
			if ( m_PathFollower.GetAge() > 1.0 )
			{
				CMerasmusPathCost func( me );
				m_PathFollower.Compute( me, m_hVictim, func );
			}

			m_PathFollower.Update( me );
		}

		me->GetLocomotionInterface()->FaceTowards( m_hVictim->WorldSpaceCenter() );
	}

	return Continue();
}
