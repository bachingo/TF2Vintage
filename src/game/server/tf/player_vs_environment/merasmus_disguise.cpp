//========= Copyright � Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "nav_mesh/tf_nav_area.h"
#include "nav_pathfind.h"
#include "entity_bossresource.h"
#include "entity_merasmus_trickortreatprop.h"
#include "merasmus_disguise.h"


ConVar tf_merasmus_health_regen_rate( "tf_merasmus_health_regen_rate", "0.001", FCVAR_CHEAT );
ConVar tf_merasmus_disguise_debug( "tf_merasmus_disguise_debug", "0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );

static void GetRandomPropAngles( QAngle *angle, CTFNavArea *pArea )
{
	Vector vecNorm;
	pArea->ComputeNormal( &vecNorm );

	Vector vecOrigin = pArea->GetRandomPoint();
	Vector vecToCenter = vecOrigin - pArea->GetCenter();

	VectorAngles( vecToCenter, vecNorm, *angle );
}


char const *CMerasmusDisguise::GetName( void ) const
{
	return "Disguise";
}


ActionResult<CMerasmus> CMerasmusDisguise::OnStart( CMerasmus *me, Action<CMerasmus> *priorAction )
{
	m_bDidSpawnProps = false;
	m_flDisguiseStartTime = gpGlobals->curtime;
	m_nDisguiseStartHealth = me->GetHealth();

	me->PlayHighPrioritySound( "Halloween.MerasmusInitiateHiding" );
	m_tauntTimer.Start( RandomFloat( 10.0, 25.0 ) );

	if( g_pMonsterResource )
		g_pMonsterResource->SetBossState( 1 );

	m_giveUpDuration.Start( 3.0f );
	TryToDisguiseSpawn( me );

	return Continue();
}

ActionResult<CMerasmus> CMerasmusDisguise::Update( CMerasmus *me, float dt )
{
	if ( me->ShouldLeave() )
		return Done();

	if ( m_bDidSpawnProps )
	{
		if ( m_tauntTimer.IsElapsed() )
		{
			if ( RandomInt( 0, 10 ) == 0 )
				me->PlayHighPrioritySound( "Halloween.MerasmusHiddenRare" );
			else
				me->PlayHighPrioritySound( "Halloween.MerasmusHidden" );

			m_tauntTimer.Start( RandomFloat( 10.0, 25.0 ) );
		}

		if ( me->GetHealth() < me->GetMaxHealth() )
		{
			const float flHealthRegen = me->GetMaxHealth() * tf_merasmus_health_regen_rate.GetFloat() * ( me->GetLevel() - 1 );
			const int nHealth = min( ( gpGlobals->curtime - m_flDisguiseStartTime ) * flHealthRegen + m_nDisguiseStartHealth, me->GetMaxHealth() );

			me->SetHealth( nHealth );

			if ( g_pMonsterResource )
			{
				const float flFraction = me->GetHealth() / me->GetMaxHealth();
				g_pMonsterResource->SetBossHealthPercentage( flFraction );
			}
		}

		if ( me->ShouldReveal() )
			return Done( "Revealed!" );

		return Continue();
	}

	if ( m_giveUpDuration.HasStarted() && m_giveUpDuration.IsElapsed() )
	{
		return Done( "Giving up on disguising." );
	}
	else
	{
		if ( m_retryTimer.IsElapsed() )
			TryToDisguiseSpawn( me );
	}

	return Continue();
}

void CMerasmusDisguise::OnEnd( CMerasmus *me, Action<CMerasmus> *newAction )
{
	if ( me->ShouldLeave() )
		me->OnLeaveWhileInDisguise();

	if( g_pMonsterResource )
		g_pMonsterResource->SetBossState( 0 );

	me->OnRevealed( true );
}


void CMerasmusDisguise::TryToDisguiseSpawn( CMerasmus *me )
{
	m_retryTimer.Start( 1.0f );

	CUtlVector<CTFNavArea *> areas;
	// Pick a set of appropriately sized, playerless areas
	FOR_EACH_VEC( TheNavAreas, i )
	{
		CNavArea *pArea = TheNavAreas[i];
		if ( !pArea->HasFuncNavPrefer() )
			continue;

		if ( pArea->GetSizeX() < 150.0 || pArea->GetSizeY() < 150.0 )
			continue;

		if ( pArea->GetPlayerCount( TF_TEAM_RED ) || pArea->GetPlayerCount( TF_TEAM_BLUE ) )
			continue;

		areas.AddToTail( (CTFNavArea *)pArea );
	}

	if ( areas.IsEmpty() )
		return;

	CUtlVector<CTFNavArea *> shuffledSet;
	// Populate a fixed size list of areas with a minimum distance apart
	SelectSeparatedShuffleSet( 10, 500.0, areas, &shuffledSet );

	if ( shuffledSet.IsEmpty() )
		return;

	if ( tf_merasmus_disguise_debug.GetBool() )
	{
		me->OnDisguise();
		m_bDidSpawnProps = true;

		FOR_EACH_VEC( shuffledSet, i )
			shuffledSet[i]->DrawFilled( 0, 255, 0, 0 );

		return;
	}

	FOR_EACH_VEC( shuffledSet, i )
	{
		int nRandom = RandomInt( 0, shuffledSet.Count() - 1 );
		CTFNavArea *pArea = shuffledSet[ nRandom ];

		QAngle angRandom;
		GetRandomPropAngles( &angRandom, pArea );

		me->AddFakeProp( CTFMerasmusTrickOrTreatProp::Create( pArea->GetCenter(), angRandom ) );

		shuffledSet.FastRemove( nRandom );
	}

	me->OnDisguise();
	m_bDidSpawnProps = true;
}
