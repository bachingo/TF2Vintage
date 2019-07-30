#include "cbase.h"
#include "UtlSortVector.h"
#include "../../tf_bot.h"
#include "tf_bot_spy_hide.h"
#include "tf_bot_spy_attack.h"
#include "tf_bot_spy_lurk.h"
#include "nav_mesh/tf_nav_mesh.h"


CTFBotSpyHide::CTFBotSpyHide( CTFPlayer *victim )
{
	m_hVictim = victim;
}

CTFBotSpyHide::~CTFBotSpyHide()
{
}


const char *CTFBotSpyHide::GetName() const
{
	return "SpyHide";
}


ActionResult<CTFBot> CTFBotSpyHide::OnStart( CTFBot *me, Action<CTFBot> *action )
{
	m_PathFollower.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	m_HidingSpot = nullptr;
	m_findHidingSpotDelay.Invalidate();
	m_bAtHidingSpot = false;

	/* assigns FLT_MAX instead if last known area is nullptr or if function returns negative */
	m_flEnemyIncursionDistance = me->GetLastKnownArea()->GetIncursionDistance( GetEnemyTeam(me) );

	m_teaseTimer.Start( RandomFloat( 5.0f, 10.0f ) );

	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotSpyHide::Update( CTFBot *me, float dt )
{
	if ( m_hVictim != nullptr && !me->GetVisionInterface()->IsIgnored( m_hVictim ) )
		return Action<CTFBot>::SuspendFor( new CTFBotSpyAttack( m_hVictim ), "Going after our initial victim" );

	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat( false );
	if ( threat != nullptr && threat->GetTimeSinceLastKnown() < 3.0f )
	{
		CTFPlayer *enemy = ToTFPlayer( threat->GetEntity() );
		if ( enemy != nullptr && me->IsRangeLessThan( enemy, 750.0f ) &&
			 enemy->IsLookingTowards( me ) )
		{
			return Action<CTFBot>::SuspendFor( new CTFBotSpyAttack( enemy ), "Opportunistic attack or self defense!" );
		}
	}

	if ( m_teaseTimer.IsElapsed() )
	{
		m_teaseTimer.Start( RandomFloat( 5.0f, 10.0f ) );

		me->EmitSound( "Spy.TeaseVictim" );
	}

	if ( m_bAtHidingSpot )
	{
		CTFNavArea *area = me->GetLastKnownArea();
		if ( area != nullptr )
			m_flEnemyIncursionDistance = area->GetIncursionDistance( GetEnemyTeam( me ) );

		return Action<CTFBot>::SuspendFor( new CTFBotSpyLurk, "Reached hiding spot - lurking" );
	}

	if ( m_HidingSpot == nullptr && m_findHidingSpotDelay.IsElapsed() )
		FindHidingSpot( me );

	m_PathFollower.Update( me );

	if ( m_HidingSpot != nullptr && m_recomputePath.IsElapsed() )
	{
		m_recomputePath.Start( RandomFloat( 0.3f, 0.5f ) );

		CTFBotPathCost func( me, SAFEST_ROUTE );
		m_PathFollower.Compute( me, m_HidingSpot->GetPosition(), func );
	}

	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotSpyHide::OnResume( CTFBot *me, Action<CTFBot> *action )
{
	m_HidingSpot = nullptr;
	m_bAtHidingSpot = false;
	m_hVictim = nullptr;

	return Action<CTFBot>::Continue();
}


EventDesiredResult<CTFBot> CTFBotSpyHide::OnMoveToSuccess( CTFBot *me, const Path *path )
{
	m_bAtHidingSpot = true;

	return Action<CTFBot>::TryContinue( RESULT_CRITICAL );
}

EventDesiredResult<CTFBot> CTFBotSpyHide::OnMoveToFailure( CTFBot *me, const Path *path, MoveToFailureType reason )
{
	m_HidingSpot = nullptr;
	m_bAtHidingSpot = false;

	return Action<CTFBot>::TryContinue( RESULT_IMPORTANT );
}


QueryResultType CTFBotSpyHide::ShouldAttack( const INextBot *me, const CKnownEntity *threat ) const
{
	return ANSWER_NO;
}


#pragma warning( disable:4701 ) // it only *may* be uninitialized
bool CTFBotSpyHide::FindHidingSpot( CTFBot *actor )
{
	if ( actor->GetLastKnownArea() == nullptr )
		return false;

	m_HidingSpot = nullptr;

	CUtlVector<CNavArea *> nearby;
	CollectSurroundingAreas( &nearby, actor->GetLastKnownArea(), 3500.0f, 500.0f, 500.0f );

	CUtlSortVector<IncursionEntry_t, SpyHideIncursionDistanceLess> entries;

	/* this is almost certainly some mangled inlining stuff that we've done a
	 * relatively poor job of un-spaghettifying */

	float incursion_max;

	int enemy_team1 = actor->GetTeamNumber();
	int enemy_team2 = enemy_team1;

	if ( enemy_team1 > TF_TEAM_BLUE )
	{
		incursion_max = 999999.0f;
	}
	else
	{
		if ( enemy_team1 == TF_TEAM_RED )
		{
			enemy_team1 = TF_TEAM_BLUE;
			enemy_team2 = TF_TEAM_BLUE;
		}
		else if ( enemy_team1 == TF_TEAM_BLUE )
		{
			enemy_team1 = TF_TEAM_RED;
			enemy_team2 = TF_TEAM_RED;
		}

		if ( actor->GetLastKnownArea()->GetIncursionDistance( enemy_team2 ) >= 0.0f )
			incursion_max = m_flEnemyIncursionDistance + 1000.0f;
	}

	if ( enemy_team1 <= TF_TEAM_BLUE )
	{
		FOR_EACH_VEC( nearby, i )
		{
			auto area = static_cast<CTFNavArea *>( nearby[i] );

			if ( area->GetHidingSpots()->Count() &&
				 area->GetIncursionDistance( enemy_team2 ) >= 0.0f && area->GetIncursionDistance( enemy_team2 ) <= incursion_max )
			{
				IncursionEntry_t entry;

				entry.teamnum = enemy_team2;
				entry.area    = area;

				entries.Insert( entry );
			}
		}
	}

	if ( !entries.IsEmpty() )
	{
		const HidingSpotVector *spots = entries.Random().area->GetHidingSpots();
		m_HidingSpot = spots->Element( RandomInt( 0, spots->Count()-1 ) );

		return true;
	}

	return false;
}
#pragma warning( default:4701 )


bool SpyHideIncursionDistanceLess::Less( const IncursionEntry_t& lhs, const IncursionEntry_t& rhs, void* )
{
	return ( lhs.area->GetIncursionDistance( lhs.teamnum ) < rhs.area->GetIncursionDistance( rhs.teamnum ) );
}
