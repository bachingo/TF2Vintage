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
	unk6 = false;
	m_iMetalSource = UNKNOWN;

	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotEngineerBuilding::Update( CTFBot *me, float dt )
{
	CBaseObject *sentry    = me->GetObjectOfType( OBJ_SENTRYGUN, OBJECT_MODE_NONE );
	CBaseObject *dispenser = me->GetObjectOfType( OBJ_DISPENSER, OBJECT_MODE_NONE );
	CBaseObject *entrance  = me->GetObjectOfType( OBJ_TELEPORTER, TELEPORTER_TYPE_ENTRANCE );
	CBaseObject *exit      = me->GetObjectOfType( OBJ_TELEPORTER, TELEPORTER_TYPE_EXIT );

	bool sentry_sapped = sentry ? sentry->HasSapper() : false;
	bool dispenser_sapped = dispenser ? dispenser->HasSapper() : false;

	me->m_bLookingAroundForEnemies = true;

	if ( !sentry )
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

	//if ( !unk6 )
	if ( m_outOfPositionTimer.IsElapsed() )
	{
		m_outOfPositionTimer.Start( RandomFloat( 3.0f, 5.0f ) );
		CheckIfSentryIsOutOfPosition( me );
	}

	unk6 = false;

	if ( dispenser && dispenser->GetAbsOrigin().DistToSqr( sentry->GetAbsOrigin() ) > Square( 500.0f ) )
	{
		dispenser->DestroyObject();
		dispenser = nullptr;
	}

	if ( sentry->GetUpgradeLevel() <= 2 )
	{
		if ( m_iMetalSource == UNKNOWN )
			m_iMetalSource = IsMetalSourceNearby( me ) ? AVAILABLE : UNAVAILABLE;

		if ( m_iMetalSource == AVAILABLE )
		{
			UpgradeAndMaintainBuildings( me );
			return Action<CTFBot>::Continue();
		}
	}

	bool b1 = sentry_sapped || sentry->GetTimeSinceLastInjury() < 1.0f || dispenser_sapped;

	if ( !TFGameRules()->IsInTraining() || exit )
	{
		if ( dispenser )
		{
			m_buildDispenserTimer.Reset();
		}
		else if ( m_buildDispenserTimer.IsElapsed() && !b1 )
		{
			m_buildDispenserTimer.Start( 10.0f );
			return Action<CTFBot>::SuspendFor( new CTFBotEngineerBuildDispenser, "Building a Dispenser" );
		}
	}

	float flBuildTeleTime = 30.0f;
	if ( TFGameRules()->IsInTraining() )
		flBuildTeleTime = 5.0f;

	if ( exit )
	{
		m_buildTeleportTimer.Reset();
		
		UpgradeAndMaintainBuildings( me );
		return Action<CTFBot>::Continue();
	}

	UpgradeAndMaintainBuildings( me );

	if ( m_buildTeleportTimer.IsElapsed() && entrance && !b1 )
	{
		m_buildTeleportTimer.Start( flBuildTeleTime );

		if ( !m_hHint )
		{
			if ( me->IsRangeLessThan( sentry, 300.0f ) )
				return Action<CTFBot>::SuspendFor( new CTFBotEngineerBuildTeleportExit(), "Building Teleporter exit" );

			UpgradeAndMaintainBuildings( me );
			return Action<CTFBot>::Continue();
		}
	}

	// Some collection of hints and a SearchSurroundingAreas happens
	// return Action<CTFBot>::SuspendFor( new CTFBotEngineerBuildTeleportExit( hint ), "Building teleporter exit at nearby hint" );
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


EventDesiredResult<CTFBot> CTFBotEngineerBuilding::OnTerritoryCaptured( CTFBot *actor, int territoryID )
{
	return Action<CTFBot>::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotEngineerBuilding::OnTerritoryLost( CTFBot *actor, int territoryID )
{
	return Action<CTFBot>::TryContinue();
}


bool CTFBotEngineerBuilding::CheckIfSentryIsOutOfPosition( CTFBot *actor ) const
{
	CBaseObject *sentry = actor->GetObjectOfType( OBJ_SENTRYGUN, OBJECT_MODE_NONE );
	if ( sentry == nullptr )
		return false;

	if ( TFGameRules()->GetGameType() == TF_GAMETYPE_ESCORT )
	{
		CTeamTrainWatcher *pTrain = TFGameRules()->GetPayloadToPush( actor->GetTeamNumber() );
		if ( pTrain )
		{
			float flDistance;
			pTrain->ProjectPointOntoPath( sentry->GetAbsOrigin(), NULL, &flDistance );
			if ( flDistance + SENTRYGUN_BASE_RANGE < pTrain->GetTrainDistanceAlongTrack() )
				return true;
		}

		pTrain = TFGameRules()->GetPayloadToBlock( actor->GetTeamNumber() );
		if ( pTrain )
		{
			float flDistance;
			pTrain->ProjectPointOntoPath( sentry->GetAbsOrigin(), NULL, &flDistance );
			if ( flDistance + SENTRYGUN_BASE_RANGE < pTrain->GetTrainDistanceAlongTrack() )
				return true;
		}
	}

	sentry->UpdateLastKnownArea();
	CNavArea *pArea = sentry->GetLastKnownArea();
	CTeamControlPoint *pPoint = actor->GetMyControlPoint();
	if ( !pArea || !pPoint )
		return false;

	CNavArea *pCPArea = TFNavMesh()->GetMainControlPointArea( pPoint->GetPointIndex() );
	if ( !pCPArea )
		return false;

	// If a path can be built given the max distance then we aren't too far away yet
	CTFBotPathCost cost( actor, FASTEST_ROUTE );
	if ( NavAreaTravelDistance( pArea, pCPArea, cost, tf_bot_engineer_max_sentry_travel_distance_to_point.GetFloat() ) >= 0.0f ||
		 NavAreaTravelDistance( pCPArea, pArea, cost, tf_bot_engineer_max_sentry_travel_distance_to_point.GetFloat() ) >= 0.0f )
		return false;

	return true;
}

bool CTFBotEngineerBuilding::IsMetalSourceNearby( CTFBot *actor ) const
{
	CUtlVector<CNavArea *> areas;
	CollectSurroundingAreas( &areas, actor->GetLastKnownArea(), 2000.0f, 
							 actor->GetLocomotionInterface()->GetStepHeight(), actor->GetLocomotionInterface()->GetDeathDropHeight() );

	for ( int i=0; i<areas.Count(); ++i )
	{
		CTFNavArea *pArea = static_cast<CTFNavArea *>( areas[i] );
		if ( pArea->HasTFAttributes( AMMO ) )
			return true;

		if ( actor->GetTeamNumber() == TF_TEAM_RED && pArea->HasTFAttributes( RED_SPAWN_ROOM ) )
			return true;

		if ( actor->GetTeamNumber() == TF_TEAM_BLUE && pArea->HasTFAttributes( BLUE_SPAWN_ROOM ) )
			return true;
	}

	return false;
}

void CTFBotEngineerBuilding::UpgradeAndMaintainBuildings( CTFBot *actor )
{
	CBaseObject *sentry    = actor->GetObjectOfType( OBJ_SENTRYGUN, OBJECT_MODE_NONE );
	CBaseObject *dispenser = actor->GetObjectOfType( OBJ_DISPENSER, OBJECT_MODE_NONE );

	CBaseCombatWeapon *melee = actor->Weapon_GetSlot( 2 );
	if ( melee )
		actor->Weapon_Switch( melee );

	if ( dispenser == nullptr )
	{
		float dist = ( actor->GetAbsOrigin() - sentry->GetAbsOrigin() ).LengthSqr();

		if ( dist < Square( 90.0f ) )
			actor->PressCrouchButton( 0.5f );

		if ( dist > Square( 75.0f ) )
		{
			if ( m_recomputePathTimer.IsElapsed() )
			{
				CTFBotPathCost cost( actor, FASTEST_ROUTE );
				m_PathFollower.Compute( actor, sentry->WorldSpaceCenter(), cost );

				m_recomputePathTimer.Start( RandomFloat( 1.0f, 2.0f ) );
			}
			
			m_PathFollower.Update( actor );
		}
		else
		{
			actor->m_bLookingAroundForEnemies = false;

			actor->GetBodyInterface()->AimHeadTowards( sentry->WorldSpaceCenter(), IBody::CRITICAL, 1.0f, nullptr, "Work on my Sentry" );
			actor->PressFireButton();
		}

		return;
	}

	Vector vecBetweenBuildings = ( dispenser->GetAbsOrigin() + sentry->GetAbsOrigin() ) / 2.0f;
	float flSentryDist = ( actor->GetAbsOrigin() - sentry->GetAbsOrigin() ).LengthSqr();
	float flDispenserDist = ( actor->GetAbsOrigin() - dispenser->GetAbsOrigin() ).LengthSqr();

	if ( flSentryDist < Square( 90.0f ) || flDispenserDist < Square( 90.0f ) )
		actor->PressCrouchButton( 0.5f );

	if ( abs( flSentryDist - flDispenserDist ) > Square( 25.0f ) || flSentryDist > Square( 75.0f ) || flDispenserDist > Square( 75.0f ) )
	{
		if ( m_recomputePathTimer.IsElapsed() )
		{
			CTFBotPathCost cost( actor, FASTEST_ROUTE );
			m_PathFollower.Compute( actor, vecBetweenBuildings, cost );

			m_recomputePathTimer.Start( RandomFloat( 1.0f, 2.0f ) );
		}

		m_PathFollower.Update( actor );
	}
	else if ( flSentryDist < Square( 75.0f ) || flDispenserDist < Square( 75.0f ) )
	{
		unk1.Invalidate();

		// This is a horrible mess
		CBaseObject *workingObject = sentry;
		if ( sentry->HasSapper() )
			workingObject = sentry;
		else if ( dispenser->HasSapper() )
			workingObject = dispenser;
		else if ( sentry->GetTimeSinceLastInjury() >= 1.0f && sentry->GetHealth() >= sentry->GetMaxHealth() && !sentry->IsBuilding() )
			workingObject = dispenser;
		else if ( sentry->GetUpgradeLevel() > 2 )
			workingObject = dispenser;
		else if ( dispenser->GetHealth() >= dispenser->GetMaxHealth() && !dispenser->IsBuilding() )
			workingObject = sentry;

		actor->m_bLookingAroundForEnemies = false;

		actor->GetBodyInterface()->AimHeadTowards( workingObject->WorldSpaceCenter(), IBody::CRITICAL, 1.0f, nullptr, "Work on my buildings" );
		actor->PressFireButton();
	}
}
