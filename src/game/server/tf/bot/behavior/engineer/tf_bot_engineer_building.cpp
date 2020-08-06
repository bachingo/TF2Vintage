#include "cbase.h"
#include "../../tf_bot.h"
#include "tf_gamerules.h"
#include "tf_obj.h"
#include "team_control_point.h"
#include "team_train_watcher.h"
#include "nav_mesh/tf_nav_mesh.h"
#include "tf_bot_engineer_building.h"
#include "tf_bot_engineer_build_sentrygun.h"
#include "tf_bot_engineer_build_dispenser.h"
#include "tf_bot_engineer_build_teleport_exit.h"
#include "tf_bot_engineer_move_to_build.h"


ConVar tf_bot_engineer_retaliate_range( "tf_bot_engineer_retaliate_range", "750", FCVAR_CHEAT, "If attacker who destroyed sentry is closer than this, attack. Otherwise, retreat" );
ConVar tf_bot_engineer_exit_near_sentry_range( "tf_bot_engineer_exit_near_sentry_range", "2500", FCVAR_CHEAT, "Maximum travel distance between a bot's Sentry gun and its Teleporter Exit" );
ConVar tf_bot_engineer_max_sentry_travel_distance_to_point( "tf_bot_engineer_max_sentry_travel_distance_to_point", "2500", FCVAR_CHEAT, "Maximum travel distance between a bot's Sentry gun and the currently contested point" );


CTFBotEngineerBuilding::CTFBotEngineerBuilding( CTFBotHintSentrygun *hint )
{
	m_hHint = hint;
}

CTFBotEngineerBuilding::~CTFBotEngineerBuilding()
{
}


const char *CTFBotEngineerBuilding::GetName() const
{
	return "EngineerBuilding";
}


ActionResult<CTFBot> CTFBotEngineerBuilding::OnStart( CTFBot *me, Action<CTFBot> *priorAction )
{
	m_iTries = 5;
	m_outOfPositionTimer.Invalidate();
	m_bHadASentry = false;
	m_iMetalSource = UNKNOWN;

	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotEngineerBuilding::Update( CTFBot *me, float dt )
{
	CBaseObject *pSentry    = me->GetObjectOfType( OBJ_SENTRYGUN, OBJECT_MODE_NONE );
	CBaseObject *pDispenser = me->GetObjectOfType( OBJ_DISPENSER, OBJECT_MODE_NONE );
	CBaseObject *pEntrance  = me->GetObjectOfType( OBJ_TELEPORTER, TELEPORTER_TYPE_ENTRANCE );
	CBaseObject *pExit      = me->GetObjectOfType( OBJ_TELEPORTER, TELEPORTER_TYPE_EXIT );

	me->m_bLookingAroundForEnemies = true;

	bool bSentrySapped = pSentry ? pSentry->HasSapper() : false;
	bool bDispenserSapped = pDispenser ? pDispenser->HasSapper() : false;

	if ( !pSentry )
	{
		m_iMetalSource = UNKNOWN;

		const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
		if ( threat != nullptr && threat->IsVisibleRecently() )
			me->EquipBestWeaponForThreat( threat );

		if ( m_bHadASentry || m_iTries <= 0 )
			return Action<CTFBot>::ChangeTo( new CTFBotEngineerMoveToBuild, "Couldn't find a place to build" );

		--m_iTries;

		if ( m_hHint )
			return Action<CTFBot>::SuspendFor( new CTFBotEngineerBuildSentryGun( m_hHint ), "Building a Sentry at a hint location" );
		else
			return Action<CTFBot>::SuspendFor( new CTFBotEngineerBuildSentryGun(), "Building a Sentry" );
	}

	m_bHadASentry = true;

	if ( !m_hHint )
	{
		// skipping some logic that seems entirely unused
	}
	else if ( m_hHint->IsDisabled() )
	{
		m_hHint = nullptr;
	}

	if ( m_outOfPositionTimer.IsElapsed() )
	{
		m_outOfPositionTimer.Start( RandomFloat( 3.0f, 5.0f ) );
		CheckIfSentryIsOutOfPosition( me );
	}

	if ( pDispenser && pDispenser->GetAbsOrigin().DistToSqr( pSentry->GetAbsOrigin() ) > Square( 500.0f ) )
	{
		pDispenser->DestroyObject();
		pDispenser = nullptr;
	}

	if ( pSentry->GetUpgradeLevel() <= 2 )
	{
		if ( m_iMetalSource == UNKNOWN )
			m_iMetalSource = IsMetalSourceNearby( me ) ? AVAILABLE : UNAVAILABLE;

		if ( m_iMetalSource == AVAILABLE )
		{
			UpgradeAndMaintainBuildings( me );
			return Action<CTFBot>::Continue();
		}
	}

	bool bPrioritizeRepair = bSentrySapped || me->GetTimeSinceLastInjury() < 1.0f || bDispenserSapped;

	if ( !TFGameRules()->IsInTraining() || pExit )
	{
		if ( pDispenser )
		{
			m_buildDispenserTimer.Reset();
		}
		else if ( m_buildDispenserTimer.IsElapsed() && !bPrioritizeRepair )
		{
			m_buildDispenserTimer.Start( 10.0f );
			return Action<CTFBot>::SuspendFor( new CTFBotEngineerBuildDispenser, "Building a Dispenser" );
		}
	}

	const float flBuildTeleTime = TFGameRules()->IsInTraining() ? 5.0f : 30.0f;
	if ( pExit )
	{
		m_buildTeleportTimer.Start( flBuildTeleTime );
		
		UpgradeAndMaintainBuildings( me );
		return Action<CTFBot>::Continue();
	}
	
	if ( m_buildTeleportTimer.IsElapsed() && pEntrance && !bPrioritizeRepair )
	{
		m_buildTeleportTimer.Start( flBuildTeleTime );

		if ( m_hHint )
		{
			CUtlVector<CBaseEntity *> hints;
			CBaseTFBotHintEntity *pEntity = (CBaseTFBotHintEntity *)gEntList.FirstEnt();
			while ( pEntity )
			{
				if ( !pEntity->IsDisabled() && me->InSameTeam( pEntity ) )
					hints.AddToTail( pEntity );

				pEntity = (CBaseTFBotHintEntity *)gEntList.FindEntityByClassname( pEntity, "bot_hint_teleporter_exit" );
			}

			if( !hints.IsEmpty() )
			{
				pSentry->UpdateLastKnownArea();
				CBaseEntity *pHint = SelectClosestEntityByTravelDistance( me, hints, pSentry->GetLastKnownArea(), tf_bot_engineer_exit_near_sentry_range.GetFloat() );

				Vector vecOrigin = pHint->GetAbsOrigin();
				float flYaw = pHint->GetAbsAngles()[ YAW ];
				return Action<CTFBot>::SuspendFor( new CTFBotEngineerBuildTeleportExit( vecOrigin, flYaw ), "Building teleporter exit at nearby hint" );
			}
		}
		else
		{
			Vector vecPoint; float flYaw;
			if( PickTeleportLocation( me, &vecPoint, flYaw ) )
				return Action<CTFBot>::SuspendFor( new CTFBotEngineerBuildTeleportExit( vecPoint, flYaw ), "Building Teleporter exit" );

			if( me->IsRangeLessThan( pSentry, 300.0 ) )
				return Action<CTFBot>::SuspendFor( new CTFBotEngineerBuildTeleportExit(), "Building Teleporter exit" );
		}
	}
	
	UpgradeAndMaintainBuildings( me );
	return Action<CTFBot>::Continue();
}

void CTFBotEngineerBuilding::OnEnd( CTFBot *me, Action<CTFBot> *newAction )
{
	me->m_bLookingAroundForEnemies = true;
}

ActionResult<CTFBot> CTFBotEngineerBuilding::OnResume( CTFBot *me, Action<CTFBot> *priorAction )
{
	return Action<CTFBot>::Continue();
}


EventDesiredResult<CTFBot> CTFBotEngineerBuilding::OnTerritoryCaptured( CTFBot *me, int territoryID )
{
	return Action<CTFBot>::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotEngineerBuilding::OnTerritoryLost( CTFBot *me, int territoryID )
{
	return Action<CTFBot>::TryContinue();
}


bool CTFBotEngineerBuilding::CheckIfSentryIsOutOfPosition( CTFBot *me ) const
{
	CBaseObject *pSentry = me->GetObjectOfType( OBJ_SENTRYGUN, OBJECT_MODE_NONE );
	if ( pSentry == nullptr )
		return false;

	if ( TFGameRules()->GetGameType() == TF_GAMETYPE_ESCORT )
	{
		CTeamTrainWatcher *pTrain = TFGameRules()->GetPayloadToPush( me->GetTeamNumber() );
		if ( pTrain )
		{
			float flDistance;
			pTrain->ProjectPointOntoPath( pSentry->GetAbsOrigin(), NULL, &flDistance );
			if ( flDistance + SENTRYGUN_BASE_RANGE < pTrain->GetTrainDistanceAlongTrack() )
				return true;
		}

		pTrain = TFGameRules()->GetPayloadToBlock( me->GetTeamNumber() );
		if ( pTrain )
		{
			float flDistance;
			pTrain->ProjectPointOntoPath( pSentry->GetAbsOrigin(), NULL, &flDistance );
			if ( flDistance + SENTRYGUN_BASE_RANGE < pTrain->GetTrainDistanceAlongTrack() )
				return true;
		}
	}

	pSentry->UpdateLastKnownArea();
	CNavArea *pArea = pSentry->GetLastKnownArea();
	if ( pArea == nullptr )
		return false;

	CTeamControlPoint *pPoint = me->GetMyControlPoint();
	if ( pPoint == nullptr )
		return false;

	CNavArea *pCPArea = TFNavMesh()->GetMainControlPointArea( pPoint->GetPointIndex() );
	if ( !pCPArea )
		return false;

	// If a path can be built given the max distance then we aren't too far away yet
	CTFBotPathCost func( me, FASTEST_ROUTE );
	if ( NavAreaTravelDistance( pArea, pCPArea, func, tf_bot_engineer_max_sentry_travel_distance_to_point.GetFloat() ) < 0.0f )
	{
		if( NavAreaTravelDistance( pCPArea, pArea, func, tf_bot_engineer_max_sentry_travel_distance_to_point.GetFloat() ) < 0.0f )
			return true;
	}

	return false;
}

bool CTFBotEngineerBuilding::PickTeleportLocation( CTFBot *actor, Vector *pLocation, float &pYaw )
{
	CUtlVector<CTFNavArea *> surroundingAreas;
	const float searchRange = 1000.0f;
	const float maxHeight = 200.0f;
	Vector vecAreaCenter;
	CTFNavArea *pArea = NULL;

	if ( TFGameRules()->GetGameType() == TF_GAMETYPE_ESCORT )
	{
		CTeamTrainWatcher *pTrain = NULL;
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
			return false;

		Vector vecNextCheckpoint = pTrain->GetNextCheckpointPosition();
		pArea = static_cast<CTFNavArea *>( TheNavMesh->GetNearestNavArea( vecNextCheckpoint, false, 500.0f, true ) );
		if ( !pArea )
			return false;

		vecAreaCenter = pArea->GetCenter();
		CollectSurroundingAreas( &surroundingAreas, pArea, searchRange, maxHeight, maxHeight );
	}
	else if ( TFGameRules()->GetGameType() == TF_GAMETYPE_CP )
	{
		CBaseEntity *pPoint = actor->GetMyControlPoint();
		if ( !pPoint )
			return false;

		pArea = static_cast<CTFNavArea *>( TheNavMesh->GetNearestNavArea( pPoint->WorldSpaceCenter(), false, 500.0f, true ) );
		if ( !pArea )
			return false;

		vecAreaCenter = pArea->GetCenter();
		CollectSurroundingAreas( &surroundingAreas, pArea, searchRange, maxHeight, maxHeight );
	}
	else
	{
		return false;
	}

	Vector vecLocation = FindHiddenSpot( pArea, surroundingAreas );
	if ( vecLocation.IsZero() )
		return false;

	Vector vecDirection = vecLocation - vecAreaCenter;

	// Try to point it away from smacking your face into a wall
	CTraceFilterWorldAndPropsOnly filter;
	Vector vecStart = vecLocation + Vector( 0, 0, StepHeight ); // Check if we are not able to step over something
	Vector vecEnd = vecStart + vecDirection * 50;

	trace_t trace;
	UTIL_TraceLine( vecStart, vecEnd, MASK_PLAYERSOLID, &filter, &trace );
	if ( trace.DidHit() )
	{
		const int tries = 30;
		for ( int i=0; i < tries; ++i )
		{
			const float flYaw = RandomFloat( -180.0, 180.0 );
			vecDirection = UTIL_YawToVector( flYaw );
			vecEnd = vecStart + vecDirection * 50;

			UTIL_TraceLine( vecStart, vecEnd, MASK_PLAYERSOLID, &filter, &trace );
			if ( !trace.DidHit() )
				break;
		}
	}

	*pLocation = vecLocation;
	pYaw = vecDirection[ YAW ];
	return true;
}

Vector CTFBotEngineerBuilding::FindHiddenSpot( CTFNavArea *pPointArea, const CUtlVector<CTFNavArea *> &surroundingAreas )
{
	CTraceFilterWorldAndPropsOnly filter;
	Vector vecLocation = vec3_origin;

	CUtlVector<CTFNavArea *> validAreas;
	for ( CTFNavArea *pArea : surroundingAreas )
	{
		const float minDistance = 750.0f;
		if ( pArea->GetCenter().DistToSqr( pPointArea->GetCenter() ) < Square( minDistance ) )
			continue;

		validAreas.AddToTail( pArea );
	}

	for ( CTFNavArea *pArea : validAreas )
	{
		const int tries = 10;
		for ( int i = 0; i < tries; ++i )
		{
			Vector vecSpot = pArea->GetRandomPoint();

			// See if we can immediately see the capture point from here
			trace_t trace;
			UTIL_TraceLine( vecSpot + Vector( 0, 0, HalfHumanHeight ), pPointArea->GetCenter(), MASK_PLAYERSOLID, &filter, &trace );

			// If not, we can hide it here
			if ( trace.DidHit() )
			{
				vecLocation = vecSpot;
				break;
			}
		}

		if ( !vecLocation.IsZero() )
			break;
	}

	return vecLocation;
}

bool CTFBotEngineerBuilding::IsMetalSourceNearby( CTFBot *me ) const
{
	CUtlVector<CTFNavArea *> areas;
	CollectSurroundingAreas( &areas, me->GetLastKnownArea(), 2000.0f, 
							 me->GetLocomotionInterface()->GetStepHeight(), me->GetLocomotionInterface()->GetDeathDropHeight() );

	for ( CTFNavArea *pArea : areas )
	{
		if ( pArea->HasTFAttributes( AMMO ) )
			return true;

		if ( me->GetTeamNumber() == TF_TEAM_RED && pArea->HasTFAttributes( RED_SPAWN_ROOM ) )
			return true;

		if ( me->GetTeamNumber() == TF_TEAM_BLUE && pArea->HasTFAttributes( BLUE_SPAWN_ROOM ) )
			return true;
	}

	return false;
}

void CTFBotEngineerBuilding::UpgradeAndMaintainBuildings( CTFBot *me )
{
	CBaseObject *pSentry    = me->GetObjectOfType( OBJ_SENTRYGUN, OBJECT_MODE_NONE );
	CBaseObject *pDispenser = me->GetObjectOfType( OBJ_DISPENSER, OBJECT_MODE_NONE );

	CBaseCombatWeapon *pMelee = me->Weapon_GetSlot( TF_LOADOUT_SLOT_MELEE );
	if ( pMelee )
		me->Weapon_Switch( pMelee );

	if ( pDispenser == nullptr )
	{
		float flDist = ( me->GetAbsOrigin() - pSentry->GetAbsOrigin() ).Length();

		if ( flDist < 90.0f )
			me->PressCrouchButton( 0.5f );

		if ( flDist > 75.0f )
		{
			if ( m_recomputePathTimer.IsElapsed() )
			{
				CTFBotPathCost cost( me, FASTEST_ROUTE );
				m_PathFollower.Compute( me, pSentry->WorldSpaceCenter(), cost );

				m_recomputePathTimer.Start( RandomFloat( 1.0f, 2.0f ) );
			}
			
			m_PathFollower.Update( me );
		}
		else
		{
			me->m_bLookingAroundForEnemies = false;

			me->GetBodyInterface()->AimHeadTowards( pSentry->WorldSpaceCenter(), IBody::CRITICAL, 1.0f, nullptr, "Work on my Sentry" );
			me->PressFireButton();
		}

		return;
	}

	Vector vecBetweenBuildings = ( pDispenser->GetAbsOrigin() + pSentry->GetAbsOrigin() ) / 2.0f;
	float flSentryDist = ( me->GetAbsOrigin() - pSentry->GetAbsOrigin() ).Length();
	float flDispenserDist = ( me->GetAbsOrigin() - pDispenser->GetAbsOrigin() ).Length();

	if ( flSentryDist < 90.0f && flDispenserDist < 90.0f )
		me->PressCrouchButton( 0.5f );

	if ( abs( flSentryDist - flDispenserDist ) > 25.0f || flSentryDist > 75.0f || flDispenserDist > 75.0f )
	{
		if ( m_recomputePathTimer.IsElapsed() )
		{
			CTFBotPathCost cost( me, FASTEST_ROUTE );
			m_PathFollower.Compute( me, vecBetweenBuildings, cost );

			m_recomputePathTimer.Start( RandomFloat( 1.0f, 2.0f ) );
		}

		m_PathFollower.Update( me );
	}
	else if ( flSentryDist < 75.0f || flDispenserDist < 75.0f )
	{
		// This is a horrible mess
		CBaseObject *workingObject = pSentry;
		if ( !pSentry->HasSapper() )
		{
			if ( pDispenser->HasSapper() )
			{
				workingObject = pDispenser;
			}
			else if ( pSentry->GetTimeSinceLastInjury() > 1.0 )
			{
				if ( pSentry->GetMaxHealth() <= pSentry->GetHealth() && !pSentry->IsBuilding() )
				{
					if ( pDispenser->IsBuilding() )
					{
						workingObject = pDispenser;
					}
					else if ( pSentry->GetUpgradeLevel() >= pSentry->GetMaxUpgradeLevel() )
					{
						if ( pDispenser->GetMaxHealth() <= pDispenser->GetHealth() )
						{
							if ( pDispenser->GetUpgradeLevel() < pSentry->GetUpgradeLevel() )
								workingObject = pDispenser;
						}
						else
						{
							workingObject = pDispenser;
						}
					}
				}
			}
		}

		me->m_bLookingAroundForEnemies = false;

		me->GetBodyInterface()->AimHeadTowards( workingObject->WorldSpaceCenter(), IBody::CRITICAL, 1.0f, nullptr, "Work on my buildings" );
		me->PressFireButton();
	}
}
