#include "cbase.h"
#include "../tf_bot.h"
#include "tf_bot_seek_and_destroy.h"
#include "tf_bot_attack.h"
#include "nav_mesh/tf_nav_mesh.h"
#include "team_control_point.h"
#include "tf_gamerules.h"


ConVar tf_bot_debug_seek_and_destroy( "tf_bot_debug_seek_and_destroy", "0", FCVAR_CHEAT, "", true, 0.0f, true, 1.0f );


CTFBotSeekAndDestroy::CTFBotSeekAndDestroy( float duration )
{
	if ( duration > 0.0f )
	{
		m_actionDuration.Start( duration );
	}
}

CTFBotSeekAndDestroy::~CTFBotSeekAndDestroy()
{
}


const char *CTFBotSeekAndDestroy::GetName() const
{
	return "SeekAndDestroy";
}


ActionResult<CTFBot> CTFBotSeekAndDestroy::OnStart( CTFBot *me, Action<CTFBot> *priorAction )
{
	m_PathFollower.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	RecomputeSeekPath( me );

	CTeamControlPoint *point = me->GetMyControlPoint();
	if ( point != nullptr )
	{
		m_bPointLocked = point->IsLocked();
	}
	else
	{
		m_bPointLocked = false;
	}

	/* start the countdown timer back to the beginning */
	if ( m_actionDuration.HasStarted() )
		m_actionDuration.Reset();

	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotSeekAndDestroy::Update( CTFBot *me, float dt )
{
	if ( TFGameRules()->IsInTraining() && me->IsAnyPointBeingCaptured() )
		return Action<CTFBot>::Done( "Assist trainee in capturing the point" );

	if ( me->IsCapturingPoint() )
		return Action<CTFBot>::Done( "Keep capturing point I happened to stumble upon" );

	if ( m_bPointLocked )
	{
		CTeamControlPoint *point = me->GetMyControlPoint();
		if ( point != nullptr && !point->IsLocked() )
			return Action<CTFBot>::Done( "The point just unlocked" );
	}

	/*extern ConVar tf_bot_offense_must_push_time;
	if ( TFGameRules()->State_Get() != GR_STATE_TEAM_WIN &&
		 me->GetTimeLeftToCapture() < tf_bot_offense_must_push_time.GetFloat() )
	{
		return Action<CTFBot>::Done( "Time to push for the objective" );
	}*/

	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	if ( threat != nullptr )
	{
		if ( TFGameRules()->State_Get() == GR_STATE_TEAM_WIN )
		{
			return Action<CTFBot>::SuspendFor( new CTFBotAttack(), "Chasing down the losers" );
		}

		if ( me->IsRangeLessThan( threat->GetLastKnownPosition(), 1000.0f ) )
		{
			return Action<CTFBot>::SuspendFor( new CTFBotAttack(), "Going after an enemy" );
		}
	}

	if ( m_actionDuration.HasStarted() && m_actionDuration.IsElapsed() )
		return Action<CTFBot>::Done( "Behavior duration elapsed" );

	m_PathFollower.Update( me );

	if ( !m_PathFollower.IsValid() && m_recomputeTimer.IsElapsed() )
	{
		m_recomputeTimer.Start( 1.0f );
		RecomputeSeekPath( me );
	}

	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotSeekAndDestroy::OnResume( CTFBot *me, Action<CTFBot> *interruptingAction )
{
	RecomputeSeekPath( me );

	return Action<CTFBot>::Continue();
}


EventDesiredResult<CTFBot> CTFBotSeekAndDestroy::OnMoveToSuccess( CTFBot *me, const Path *path )
{
	RecomputeSeekPath( me );

	return Action<CTFBot>::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotSeekAndDestroy::OnMoveToFailure( CTFBot *me, const Path *path, MoveToFailureType reason )
{
	RecomputeSeekPath( me );

	return Action<CTFBot>::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotSeekAndDestroy::OnStuck( CTFBot *me )
{
	RecomputeSeekPath( me );

	return Action<CTFBot>::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotSeekAndDestroy::OnTerritoryContested( CTFBot *me, int pointID )
{
	return Action<CTFBot>::TryDone( RESULT_IMPORTANT, "Defending the point" );
}

EventDesiredResult<CTFBot> CTFBotSeekAndDestroy::OnTerritoryCaptured( CTFBot *me, int pointID )
{
	return Action<CTFBot>::TryDone( RESULT_IMPORTANT, "Giving up due to point capture" );
}

EventDesiredResult<CTFBot> CTFBotSeekAndDestroy::OnTerritoryLost( CTFBot *me, int pointID )
{
	return Action<CTFBot>::TryDone( RESULT_IMPORTANT, "Giving up due to point lost" );
}


QueryResultType CTFBotSeekAndDestroy::ShouldHurry( const INextBot *me ) const
{
	return ANSWER_UNDEFINED;
}

QueryResultType CTFBotSeekAndDestroy::ShouldRetreat( const INextBot *me ) const
{
	if ( ToTFPlayer( me->GetEntity() )->IsPlayerClass( TF_CLASS_PYRO ) )
	{
		return ANSWER_NO;
	}
	else
	{
		return ANSWER_UNDEFINED;
	}
}


CTFNavArea *CTFBotSeekAndDestroy::ChooseGoalArea( CTFBot *actor )
{
	CUtlVector<CTFNavArea *> areas;
	TFNavMesh()->CollectSpawnRoomThresholdAreas( &areas, GetEnemyTeam( actor ) );

	CTeamControlPoint *point = actor->GetMyControlPoint();
	if ( point && !point->IsLocked() )
	{
		int index = point->GetPointIndex();
		if ( index < MAX_CONTROL_POINTS )
		{	// this is somewhat what's happening, no idea what (TheNavMesh + 20 * index + 1536) is, it's between m_sentryAreas & m_CPAreas
			const CUtlVector<CTFNavArea *> &cpAreas = TFNavMesh()->GetControlPointAreas( index );
			if ( cpAreas.Count() > 0 )
			{
				CTFNavArea *area = cpAreas.Random();
				areas.AddToHead( area );
			}
		}
	}

	if ( tf_bot_debug_seek_and_destroy.GetBool() )
	{
		FOR_EACH_VEC( areas, i )
		{
			TheNavMesh->AddToSelectedSet( areas[i] );
		}
	}

	if ( !areas.IsEmpty() )
	{
		return areas.Random();
	}
	else
	{
		return nullptr;
	}
}

void CTFBotSeekAndDestroy::RecomputeSeekPath( CTFBot *actor )
{
	if ( ( m_GoalArea = ChooseGoalArea( actor ) ) == nullptr )
	{
		m_PathFollower.Invalidate();
		return;
	}

	CTFBotPathCost func( actor, SAFEST_ROUTE );
	m_PathFollower.Compute( actor, m_GoalArea->GetCenter(), func );
}
