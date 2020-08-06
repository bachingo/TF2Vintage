#include "cbase.h"
#include "../../tf_bot.h"
#include "tf_gamerules.h"
#include "nav_mesh/tf_nav_mesh.h"
#include "tf_obj.h"
#include "team_control_point.h"
#include "func_capture_zone.h"
#include "team_train_watcher.h"
#include "map_entities/tf_hint_sentrygun.h"
#include "tf_bot_engineer_move_to_build.h"
#include "../tf_bot_retreat_to_cover.h"
#include "tf_bot_engineer_building.h"
#include "tf_bot_engineer_build_teleport_exit.h"


ConVar tf_bot_debug_sentry_placement( "tf_bot_debug_sentry_placement", "0", FCVAR_CHEAT );
ConVar tf_bot_max_teleport_exit_travel_to_point( "tf_bot_max_teleport_exit_travel_to_point", "2500", FCVAR_CHEAT, "In an offensive engineer bot's tele exit is farther from the point than this, destroy it" );
ConVar tf_bot_min_teleport_travel( "tf_bot_min_teleport_travel", "3000", FCVAR_CHEAT, "Minimum travel distance between teleporter entrance and exit before engineer bot will build one" );


static Vector s_pointCentroid = vec3_origin;

int CompareRangeToPoint( CTFNavArea *const *area1, CTFNavArea *const *area2 )
{
	float dist1 = ( *area1 )->GetCenter().DistToSqr( s_pointCentroid );
	float dist2 = ( *area2 )->GetCenter().DistToSqr( s_pointCentroid );

	if ( dist1 > dist2 )
		return -1;
	if ( dist1 < dist2 )
		return 1;

	return 0;
}


CTFBotEngineerMoveToBuild::CTFBotEngineerMoveToBuild()
{
}

CTFBotEngineerMoveToBuild::~CTFBotEngineerMoveToBuild()
{
}


const char *CTFBotEngineerMoveToBuild::GetName() const
{
	return "EngineerMoveToBuild";
}


ActionResult<CTFBot> CTFBotEngineerMoveToBuild::OnStart( CTFBot *me, Action<CTFBot> *priorAction )
{
	m_PathFollower.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	this->SelectBuildLocation( me );

	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotEngineerMoveToBuild::Update( CTFBot *me, float dt )
{
	// TODO
	// me->timer @ 2513 is elapsed && this->m_recomputeBuildLocation is started && elapsed

	if ( me->GetObjectOfType( OBJ_SENTRYGUN, OBJECT_MODE_NONE ) )
		return Action<CTFBot>::ChangeTo( new CTFBotEngineerBuilding, "Going back to my existing sentry nest" );

	if ( TFGameRules()->GetGameType() == TF_GAMETYPE_CP && !TFGameRules()->IsInKothMode() && me->GetTeamNumber() == TF_TEAM_BLUE )
	{
		CBaseObject *pExit = me->GetObjectOfType( OBJ_TELEPORTER, TELEPORTER_TYPE_EXIT );
		if ( pExit )
		{
			CTeamControlPoint *pPoint = me->GetMyControlPoint();
			if ( pPoint )
			{
				CTFNavArea *pCPArea = TFNavMesh()->GetMainControlPointArea( pPoint->GetPointIndex() );
				if ( pCPArea )
				{
					pExit->UpdateLastKnownArea();
					CTFNavArea *pTeleArea = static_cast<CTFNavArea *>( pExit->GetLastKnownArea() );

					if ( pTeleArea && abs( pTeleArea->GetIncursionDistance( me->GetTeamNumber() ) - pCPArea->GetIncursionDistance( me->GetTeamNumber() ) ) > tf_bot_max_teleport_exit_travel_to_point.GetFloat() )
						pExit->DestroyObject();
				}
			}
		}
		else
		{
			CBaseObject *pEntrance = me->GetObjectOfType( OBJ_TELEPORTER, TELEPORTER_TYPE_ENTRANCE );
			CTFNavArea *pArea = me->GetLastKnownArea();
			if ( pEntrance && pArea )
			{
				pEntrance->UpdateLastKnownArea();
				CTFNavArea *pTeleArea = static_cast<CTFNavArea *>( pEntrance->GetLastKnownArea() );

				if ( pTeleArea && abs( pTeleArea->GetIncursionDistance( me->GetTeamNumber() ) - pArea->GetIncursionDistance( me->GetTeamNumber() ) ) >= tf_bot_min_teleport_travel.GetFloat() )
				{
					if ( me->GetVisionInterface()->GetPrimaryKnownThreat( true ) && !me->m_Shared.InCond( TF_COND_INVULNERABLE ) && ShouldRetreat( me ) == ANSWER_YES )
						return Action<CTFBot>::SuspendFor( new CTFBotRetreatToCover( new CTFBotEngineerBuildTeleportExit ), "Retreating to a safe place to build my teleporter exit" );
				}
			}
			else
			{
				if ( me->GetVisionInterface()->GetPrimaryKnownThreat( true ) && !me->m_Shared.InCond( TF_COND_INVULNERABLE ) && ShouldRetreat( me ) == ANSWER_YES )
					return Action<CTFBot>::SuspendFor( new CTFBotRetreatToCover( new CTFBotEngineerBuildTeleportExit ), "Retreating to a safe place to build my teleporter exit" );
			}
		}
	}

	if ( m_recomputePathTimer.IsElapsed() )
	{
		CTFBotPathCost cost( me, SAFEST_ROUTE );
		m_PathFollower.Compute( me, m_vecBuildLocation, cost );

		m_recomputePathTimer.Start( RandomFloat( 1.0f, 2.0f ) );
	}

	if ( !me->GetLocomotionInterface()->IsOnGround() )
		return Action<CTFBot>::Continue();

	Vector vecFwd;
	me->EyeVectors( &vecFwd );

	Vector vecToSpot = ( m_vecBuildLocation - me->GetAbsOrigin() ) - vecFwd * 20.0f;
	if ( vecToSpot.AsVector2D().LengthSqr() >= Square( 25.0f ) )
	{
		m_PathFollower.Update( me );
		return Action<CTFBot>::Continue();
	}

	if ( m_hSentryHint )
		return Action<CTFBot>::ChangeTo( new CTFBotEngineerBuilding( m_hSentryHint ), "Reached my precise build location" );
	else
		return Action<CTFBot>::ChangeTo( new CTFBotEngineerBuilding(), "Reached my build location" );
}


EventDesiredResult<CTFBot> CTFBotEngineerMoveToBuild::OnMoveToSuccess( CTFBot *me, const Path *path )
{
	return Action<CTFBot>::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotEngineerMoveToBuild::OnMoveToFailure( CTFBot *me, const Path *path, MoveToFailureType fail )
{
	this->SelectBuildLocation( me );

	return Action<CTFBot>::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotEngineerMoveToBuild::OnStuck( CTFBot *me )
{
	return Action<CTFBot>::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotEngineerMoveToBuild::OnTerritoryLost( CTFBot *me, int territoryID )
{
	m_recomputePathTimer.Start( 0.2f );

	return Action<CTFBot>::TryContinue();
}


void CTFBotEngineerMoveToBuild::CollectBuildAreas( CTFBot *actor )
{
	// This is so mangled and fudged, but it collects areas near objectives, then runs through each of those areas to build a collection of areas from potentially visible
	// If there are multiple objective points to pick from, the engineer will be biased to setup on a route immediately between them rather than best one

	if ( actor->m_HomeArea )
		return;

	m_buildAreas.RemoveAll();

	CUtlVector<CTFNavArea *> objectiveAreas;
	Vector vecCenter = vec3_origin;

	CBaseEntity *pFlagArea = actor->GetFlagCaptureZone();
	if ( pFlagArea )
	{
		CTFNavArea *pArea = static_cast<CTFNavArea *>( TheNavMesh->GetNearestNavArea( pFlagArea->WorldSpaceCenter(), false, 500.0f, true ) );
		if ( !pArea )
			return;

		objectiveAreas.AddToTail( pArea );
		vecCenter += pArea->GetCenter();
	}
	else if ( TFGameRules()->GetGameType() == TF_GAMETYPE_ESCORT )
	{
		CTeamTrainWatcher *pTrain;
		switch ( actor->GetTeamNumber() )
		{
			case TF_TEAM_BLUE:
				pTrain = TFGameRules()->GetPayloadToPush( actor->GetTeamNumber() );
				break;
			case TF_TEAM_RED:
				pTrain = TFGameRules()->GetPayloadToBlock( actor->GetTeamNumber() );
				break;
		}
		if ( !pTrain )
			return;

		Vector vecNextCheckpoint = pTrain->GetNextCheckpointPosition();
		CTFNavArea *pArea = static_cast<CTFNavArea *>( TheNavMesh->GetNearestNavArea( vecNextCheckpoint, false, 500.0f, true ) );
		if ( !pArea )
			return;

		objectiveAreas.AddToTail( pArea );
		vecCenter += pArea->GetCenter();
	}
	else
	{
		CBaseEntity *pPoint = actor->GetMyControlPoint();
		if ( pPoint )
		{
			CTFNavArea *pArea = static_cast<CTFNavArea *>( TheNavMesh->GetNearestNavArea( pPoint->WorldSpaceCenter(), false, 500.0f, true ) );
			if ( !pArea && objectiveAreas.IsEmpty() )
				return;

			objectiveAreas.AddToTail( pArea );
			vecCenter += pArea->GetCenter();
		}
	}

	if ( objectiveAreas.IsEmpty() )
		return;

	vecCenter *= 1.0f / objectiveAreas.Count();

	CUtlVector<CTFNavArea *> visibleAreas;
	for ( int i=0; i<objectiveAreas.Count(); ++i )
	{
		CTFNavArea *pArea = objectiveAreas[i];

		NavAreaCollector func;
		pArea->ForAllPotentiallyVisibleAreas( func );

		for ( int j=0; j<func.m_area.Count(); ++j )
		{
			CTFNavArea *pOther = static_cast<CTFNavArea *>( func.m_area[j] );
			if ( pOther->GetIncursionDistance( actor->GetTeamNumber() ) < 0.0f || pOther->GetIncursionDistance( GetEnemyTeam( actor ) ) < 0.0f )
				continue;

			if( TFGameRules()->IsInKothMode() && pOther->GetIncursionDistance( actor->GetTeamNumber() ) >= pOther->GetIncursionDistance( GetEnemyTeam( actor ) ) )
				continue;

			if ( TFGameRules()->GetGameType() == TF_GAMETYPE_CP )
			{
				if ( pOther->HasTFAttributes( CONTROL_POINT ) )
					continue;

				if ( ( vecCenter.z - 150.0f ) > pOther->GetCenter().z )
					continue;

				if ( ( vecCenter - pOther->GetCenter() ).LengthSqr() > Square( 1200.0f ) )
					continue;
			}

			if ( actor->IsLineOfFireClear( pOther->GetCenter() + Vector( 0, 0, 70.0f ), vecCenter + Vector( 0, 0, 60.0f ) ) )
				visibleAreas.AddToTail( pOther );
		}
	}

	s_pointCentroid = vecCenter;

	visibleAreas.Sort( CompareRangeToPoint );
	for ( int i=0; i<visibleAreas.Count(); ++i )
	{
		CTFNavArea *pArea = visibleAreas[i];
		m_buildAreas.AddToTail( pArea );
	}

	m_flArea = 0;
	for ( int i=0; i<m_buildAreas.Count(); ++i )
	{
		CNavArea *pArea = m_buildAreas[i];
		m_flArea += pArea->GetSizeX() * pArea->GetSizeY();

		if ( tf_bot_debug_sentry_placement.GetBool() )
			TheNavMesh->AddToSelectedSet( pArea );
	}
}

void CTFBotEngineerMoveToBuild::SelectBuildLocation( CTFBot *actor )
{
	m_PathFollower.Invalidate();
	m_hSentryHint = nullptr;
	m_vecBuildLocation = vec3_origin;

	if ( actor->m_HomeArea )
	{
		m_vecBuildLocation = actor->m_HomeArea->GetCenter();
		return;
	}

	CUtlVector<CTFBotHintSentrygun *> hints;
	CTFBotHintSentrygun *pHint = dynamic_cast<CTFBotHintSentrygun *>( gEntList.FindEntityByClassname( NULL, "bot_hint_sentrygun" ) );
	while ( pHint )
	{
		if ( pHint->m_hOwner == actor )
			pHint->m_hOwner = nullptr;

		if ( pHint->IsAvailableForSelection( actor ) )
			hints.AddToTail( pHint );

		pHint = dynamic_cast<CTFBotHintSentrygun *>( gEntList.FindEntityByClassname( pHint, "bot_hint_sentrygun" ) );
	}

	if ( !hints.IsEmpty() )
	{
		m_hSentryHint = hints.Random();
		m_vecBuildLocation = m_hSentryHint->GetAbsOrigin();

		return;
	}

	this->CollectBuildAreas( actor );
	const float flDesiredArea = RandomFloat( 0, m_flArea - 1.0f );

	float flArea = 0.0f;
	for ( int i=0; i<m_buildAreas.Count(); ++i )
	{
		CTFNavArea *pArea = m_buildAreas[i];
		flArea += pArea->GetSizeX() * pArea->GetSizeY();

		if ( flArea > flDesiredArea )
		{
			m_vecBuildLocation = pArea->GetRandomPoint();
			return;
		}
	}

	m_vecBuildLocation = actor->GetAbsOrigin();
}
