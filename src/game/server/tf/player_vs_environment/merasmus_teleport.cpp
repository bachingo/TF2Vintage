//========= Copyright � Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "particle_parse.h"
#include "merasmus_teleport.h"
#include "merasmus_aoeattack.h"


CMerasmusTeleport::CMerasmusTeleport( bool bDoAOEAttack, bool bGotoHome )
{
	m_bGoToHome = bGotoHome;
	m_bPerformAOE = bDoAOEAttack;
}


char const *CMerasmusTeleport::GetName( void ) const
{
	return "Teleport";
}


ActionResult<CMerasmus> CMerasmusTeleport::OnStart( CMerasmus *me, Action<CMerasmus> *priorAction )
{
	m_nTeleportState = TELEPORT_OUT;
	me->GetBodyInterface()->StartActivity( ACT_SHIELD_DOWN );

	return Continue();
}

ActionResult<CMerasmus> CMerasmusTeleport::Update( CMerasmus *me, float dt )
{
	if ( me->IsSequenceFinished() )
	{
		switch ( m_nTeleportState )
		{
			case TELEPORT_OUT:
			{
				DispatchParticleEffect( "merasmus_tp", me->GetAbsOrigin(), me->GetAbsAngles() );
				me->GetBodyInterface()->StartActivity( ACT_SHIELD_UP );
				me->AddEffects( EF_NODRAW | EF_NOINTERP );

				me->SetAbsOrigin( GetTeleportPosition( me ) );

				m_nTeleportState = TELEPORT_IN;
			}
			break;
			case TELEPORT_IN:
			{
				DispatchParticleEffect( "merasmus_tp", me->GetAbsOrigin(), me->GetAbsAngles() );
				me->RemoveEffects( EF_NODRAW | EF_NOINTERP );

				m_nTeleportState = TELEPORT_DONE;
			}
			break;
			case TELEPORT_DONE:
			{
				if ( m_bPerformAOE )
				{
					m_bPerformAOE = false;
					return SuspendFor( new CMerasmusAOEAttack, "AOE attack!" );
				}

				return Done();
			}
		}
	}

	return Continue();
}


Vector CMerasmusTeleport::GetTeleportPosition( CMerasmus *actor )
{
	if ( m_bGoToHome )
		return actor->m_vecHome + Vector( 0, 0, 75 );

	CUtlVector<CNavArea *> areas;
	FOR_EACH_VEC( TheNavAreas, i )
	{
		CNavArea *pArea = TheNavAreas[i];
		if ( pArea->GetSizeX() < 100.0f || pArea->GetSizeY() < 100.0f )
			continue;

		if ( pArea->GetPlayerCount( TF_TEAM_BLUE ) != 0 || pArea->GetPlayerCount( TF_TEAM_RED ) != 0 )
			continue;

		if ( pArea->HasFuncNavPrefer() )
			areas.AddToTail( pArea );
	}

	if( areas.IsEmpty() )
		return actor->m_vecHome + Vector( 0, 0, 75 );

	return areas.Random()->GetCenter();
}