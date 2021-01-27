#include "cbase.h"
#include "NextBot/NavMeshEntities/func_nav_prerequisite.h"
#include "tf_bot.h"
#include "tf_weaponbase.h"
#include "tf_bot_tactical_monitor.h"
#include "tf_gamerules.h"
#include "nav_mesh/tf_nav_mesh.h"
#include "tf_obj_teleporter.h"
#include "tf_obj_sentrygun.h"
#include "tf_weapon_pipebomblauncher.h"
#include "tf_bot_scenario_monitor.h"
#include "nav_entities/tf_bot_nav_ent_wait.h"
#include "nav_entities/tf_bot_nav_ent_move_to.h"
#include "tf_bot_taunt.h"
#include "tf_bot_seek_and_destroy.h"
#include "tf_bot_get_ammo.h"
#include "tf_bot_get_health.h"
#include "tf_bot_retreat_to_cover.h"
#include "tf_bot_destroy_enemy_sentry.h"
#include "tf_bot_use_teleporter.h"


ConVar tf_bot_force_jump( "tf_bot_force_jump", "0", FCVAR_CHEAT, "Force bots to continuously jump", true, 0.0f, true, 1.0f );

class DetonatePipebombsReply : public INextBotReply
{
	virtual void OnSuccess( INextBot *bot )
	{
		CTFBot *actor = ToTFBot( bot->GetEntity() );
		if ( actor->GetActiveWeapon() != actor->Weapon_GetSlot( 1 ) )
			actor->Weapon_Switch( actor->Weapon_GetSlot( 1 ) );

		actor->PressAltFireButton();
	}
};
static DetonatePipebombsReply detReply;

const char *CTFBotTacticalMonitor::GetName( void ) const
{
	return "TacticalMonitor";
}


ActionResult<CTFBot> CTFBotTacticalMonitor::OnStart( CTFBot *me, Action<CTFBot> *priorAction )
{
	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotTacticalMonitor::Update( CTFBot *me, float dt )
{
	if ( TFGameRules()->State_Get() == GR_STATE_TEAM_WIN )
	{
		if ( TFGameRules()->GetWinningTeam() == me->GetTeamNumber() )
		{
			return Action<CTFBot>::SuspendFor( new CTFBotSeekAndDestroy, "Get the losers!" );
		}
		else
		{
			if ( me->GetVisionInterface()->GetPrimaryKnownThreat( true ) )
			{
				return BaseClass::SuspendFor( new CTFBotRetreatToCover, "Run away from threat!" );
			}
			else
			{
				me->PressCrouchButton();
				return BaseClass::Continue();
			}
		}
	}

	if ( tf_bot_force_jump.GetBool() && !me->GetLocomotionInterface()->IsClimbingOrJumping() )
		me->GetLocomotionInterface()->Jump();

	if ( TFGameRules()->State_Get() == GR_STATE_PREROUND )
		me->GetLocomotionInterface()->ClearStuckStatus( "In preround" );

	Action<CTFBot> *action = me->OpportunisticallyUseWeaponAbilities();
	if ( action != nullptr )
		return BaseClass::SuspendFor( action, "Opportunistically using buff item" );

	if ( TFGameRules()->InSetup() )
	{
		// TODO: taunting at humans stuff
	}

	QueryResultType retreat = me->GetIntentionInterface()->ShouldRetreat( me );
	if ( retreat == ANSWER_YES )
		return BaseClass::SuspendFor( new CTFBotRetreatToCover, "Backing off" );

	if ( retreat != ANSWER_NO && !me->m_Shared.InCond( TF_COND_INVULNERABLE ) && me->m_iSkill >= CTFBot::HARD )
	{
		CTFWeaponBase *pWeapon = (CTFWeaponBase *)me->Weapon_GetSlot( 1 );
		if ( pWeapon && me->IsBarrageAndReloadWeapon( pWeapon ) )
		{
			if ( me->GetAmmoCount( TF_AMMO_PRIMARY ) > 0 && pWeapon->Clip1() <= 1 )
				return BaseClass::SuspendFor( new CTFBotRetreatToCover, "Moving to cover to reload" );
		}
	}

	QueryResultType hurry = me->GetIntentionInterface()->ShouldHurry( me );
	if ( hurry == ANSWER_YES || m_checkUseTeleportTimer.IsElapsed() )
	{
		if ( ShouldOpportunisticallyTeleport( me ) )
		{
			CObjectTeleporter *pTeleporter = FindNearbyTeleporter( me );
			if ( pTeleporter )
			{
				CTFNavArea *pTeleArea = (CTFNavArea *)TFNavMesh()->GetNearestNavArea( pTeleporter );
				CTFNavArea *pMyArea = (CTFNavArea *)me->GetLastKnownArea();
				if ( pTeleArea && pMyArea )
				{
					if ( ( pTeleArea->GetIncursionDistance( me->GetTeamNumber() ) + 350.0f ) > pMyArea->GetIncursionDistance( me->GetTeamNumber() ) )
						return BaseClass::SuspendFor( new CTFBotUseTeleporter( pTeleporter ), "Using nearby teleporter" );
				}
			}
		}
	}
	else
	{
		m_checkUseTeleportTimer.Start( RandomFloat( 0.3f, 0.5f ) );

		bool bLowHealth = false;

		if ( ( me->GetTimeSinceWeaponFired() < 2.0f || me->IsPlayerClass( TF_CLASS_SNIPER ) ) &&
			(float)me->GetHealth() / (float)me->GetMaxHealth() < tf_bot_health_critical_ratio.GetFloat() )
		{
			bLowHealth = true;
		}
		else if ( me->m_Shared.InCond( TF_COND_BURNING ) ||
			(float)me->GetHealth() / (float)me->GetMaxHealth() < tf_bot_health_ok_ratio.GetFloat() )
		{
			bLowHealth = true;
		}

		if ( bLowHealth && CTFBotGetHealth::IsPossible( me ) )
			return Action<CTFBot>::SuspendFor( new CTFBotGetHealth, "Grabbing nearby health" );

		if ( me->IsAmmoLow() && CTFBotGetAmmo::IsPossible( me ) )
			return Action<CTFBot>::SuspendFor( new CTFBotGetAmmo, "Grabbing nearby ammo" );

		if ( me->GetTargetSentry() && CTFBotDestroyEnemySentry::IsPossible( me ) )
			return BaseClass::SuspendFor( new CTFBotDestroyEnemySentry, "Going after an enemy sentry to destroy it" );
	}

	MonitorArmedStickybombs( me );

	if ( me->IsPlayerClass( TF_CLASS_SPY ) )
		AvoidBumpingEnemies( me );

	me->UpdateDelayedThreatNotices();

	return Action<CTFBot>::Continue();
}


Action<CTFBot> *CTFBotTacticalMonitor::InitialContainedAction( CTFBot *actor )
{
	return new CTFBotScenarioMonitor;
}


EventDesiredResult<CTFBot> CTFBotTacticalMonitor::OnOtherKilled( CTFBot *me, CBaseCombatCharacter *who, const CTakeDamageInfo& info )
{
	return Action<CTFBot>::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotTacticalMonitor::OnNavAreaChanged( CTFBot *me, CNavArea *area1, CNavArea *area2 )
{
	if ( area1 == nullptr )
	{
		return Action<CTFBot>::TryContinue();
	}

	/*FOR_EACH_VEC( area1->GetPrerequisiteVector(), i )
	{
		CFuncNavPrerequisite *pPrereq = area1->GetPrerequisiteVector()[i];
		if ( pPrereq == nullptr )
			continue;

		if ( pPrereq->IsTask( CFuncNavPrerequisite::TASK_WAIT ) )
			return Action<CTFBot>::TrySuspendFor( new CTFBotNavEntWait( pPrereq ), RESULT_IMPORTANT, "Prerequisite commands me to wait" );

		if ( pPrereq->IsTask( CFuncNavPrerequisite::TASK_MOVE_TO_ENTITY ) )
			return Action<CTFBot>::TrySuspendFor( new CTFBotNavEntMoveTo( pPrereq ), RESULT_IMPORTANT, "Prerequisite commands me to move to an entity" );
	}*/

	return Action<CTFBot>::TryContinue();
}

// This seems almost entirely used for the "tutorial"
EventDesiredResult<CTFBot> CTFBotTacticalMonitor::OnCommandString( CTFBot *me, const char *cmd )
{
	if ( FStrEq( cmd, "goto action point" ) )
	{
		//return Action<CTFBot>::TrySuspendFor( new CTFGotoActionPoint, RESULT_IMPORTANT, "Received command to go to action point" );
	}
	else if ( FStrEq( cmd, "despawn" ) )
	{
		//return Action<CTFBot>::TrySuspendFor( new CTFDespawn, RESULT_CRITICAL, "Received command to go to de-spawn" );
	}
	else if ( FStrEq( cmd, "taunt" ) )
	{
		return Action<CTFBot>::TrySuspendFor( new CTFBotTaunt, RESULT_TRY, "Received command to taunt" );
	}
	else if ( FStrEq( cmd, "cloak" ) )
	{
		if ( me->IsPlayerClass( TF_CLASS_SPY ) && !me->m_Shared.IsStealthed() )
			me->PressAltFireButton();
	}
	else if ( FStrEq( cmd, "uncloak" ) )
	{
		if ( me->IsPlayerClass( TF_CLASS_SPY ) && !me->m_Shared.IsStealthed() )
			me->PressAltFireButton();
	}
	else if ( FStrEq( cmd, "disguise" ) )
	{
		if ( me->IsPlayerClass( TF_CLASS_SPY ) && me->CanDisguise() )
			me->m_Shared.Disguise( GetEnemyTeam( me ), RandomInt( TF_FIRST_NORMAL_CLASS, TF_LAST_NORMAL_CLASS ) );
	}
	else if ( FStrEq( cmd, "build sentry at nearest sentry hint" ) )
	{
// TODO
	}
	else if ( FStrEq( cmd, "attack sentry at next action point" ) )
	{
// TODO
	}

	return Action<CTFBot>::TryContinue();
}


void CTFBotTacticalMonitor::AvoidBumpingEnemies( CTFBot *actor )
{
	if ( actor->m_iSkill > CTFBot::NORMAL )
	{
		CTFPlayer *pClosest = nullptr;
		float flClosest = Square( 200.0f );

		CUtlVector<CTFPlayer *> enemies;
		CollectPlayers( &enemies, GetEnemyTeam( actor ), true );
		for ( int i=0; i<enemies.Count(); ++i )
		{
			CTFPlayer *pPlayer = enemies[i];
			if ( pPlayer->m_Shared.IsStealthed() || pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) )
				continue;

			float flDistance = ( pPlayer->GetAbsOrigin() - actor->GetAbsOrigin() ).LengthSqr();
			if ( flDistance < flClosest )
			{
				flClosest = flDistance;
				pClosest = pPlayer;
			}
		}

		if ( pClosest )
		{
			if ( actor->GetIntentionInterface()->IsHindrance( actor, pClosest ) == ANSWER_UNDEFINED )
			{
				actor->ReleaseForwardButton();
				actor->ReleaseLeftButton();
				actor->ReleaseRightButton();
				actor->ReleaseBackwardButton();

				actor->GetLocomotionInterface()->Approach(
					actor->GetAbsOrigin() + actor->GetLocomotionInterface()->GetFeet() - pClosest->GetAbsOrigin()
				);
			}
		}
	}
}

CObjectTeleporter *CTFBotTacticalMonitor::FindNearbyTeleporter( CTFBot *actor )
{
	if ( !m_takeTeleporterTimer.IsElapsed() )
		return nullptr;

	m_takeTeleporterTimer.Start( RandomFloat( 1.0f, 2.0f ) );

	if ( !actor->GetLastKnownArea() )
		return nullptr;

	CUtlVector<CNavArea *> nearby;
	CollectSurroundingAreas( &nearby, actor->GetLastKnownArea(), 1000.0f );

	CUtlVector<CBaseObject *> objects;
	TFNavMesh()->CollectBuiltObjects( &objects, actor->GetTeamNumber() );

	CUtlVector<CObjectTeleporter *> candidates;
	for ( int i=0; i<objects.Count(); ++i )
	{
		if ( objects[i]->ObjectType() != OBJ_TELEPORTER )
			continue;

		CObjectTeleporter *pTele = static_cast<CObjectTeleporter *>( objects[i] );
		pTele->UpdateLastKnownArea();

		if ( pTele->GetObjectMode() != TELEPORTER_TYPE_ENTRANCE || !pTele->IsReady() || pTele->GetLastKnownArea() == nullptr || nearby.IsEmpty() )
			continue;

		// the first element is the starting area, it will always exist
		if ( nearby[0]->GetID() == pTele->GetLastKnownArea()->GetID() )
		{
			candidates.AddToTail( pTele );
		}
		else
		{
			for ( int j=0; j<nearby.Count(); ++j )
			{
				if ( nearby[j]->GetID() == pTele->GetLastKnownArea()->GetID() )
					candidates.AddToTail( pTele );
			}
		}
	}

	if ( !candidates.IsEmpty() )
		return candidates.Random();

	return nullptr;
}

void CTFBotTacticalMonitor::MonitorArmedStickybombs( CTFBot *actor )
{
	if ( !m_stickyMonitorDelay.IsElapsed() )
		return;

	m_stickyMonitorDelay.Start( RandomFloat( 0.3f, 1.0f ) );

	CTFPipebombLauncher *pLauncher = dynamic_cast<CTFPipebombLauncher *>( actor->Weapon_GetSlot( 1 ) );
	if ( pLauncher == nullptr || pLauncher->m_Pipebombs.IsEmpty() )
		return;

	if ( actor->m_Shared.InCond( TF_COND_HALLOWEEN_THRILLER ) )
		return;
	
	CUtlVector<CKnownEntity> knowns;
	actor->GetVisionInterface()->CollectKnownEntities( &knowns );
	
	FOR_EACH_VEC( pLauncher->m_Pipebombs, i )
	{
		CTFGrenadePipebombProjectile *pGrenade = pLauncher->m_Pipebombs[i];
		FOR_EACH_VEC( knowns, j )
		{
			const CKnownEntity &pKnown = knowns[j];
			if ( pKnown.IsObsolete() || pKnown.GetEntity()->IsBaseObject() )
				continue;

			if ( GetEnemyTeam( pKnown.GetEntity() ) == pGrenade->GetTeamNumber() )
				continue;

			if ( pGrenade->GetAbsOrigin().DistToSqr( pKnown.GetLastKnownPosition() ) >= Square( 150.0 ) )
				continue;

			actor->GetBodyInterface()->AimHeadTowards( pGrenade->WorldSpaceCenter() + RandomVector( -10.0f, 10.0f ), IBody::IMPORTANT, 0.5f, &detReply, "Looking toward stickies to detonate" );
			return;
		}
	}
}

bool CTFBotTacticalMonitor::ShouldOpportunisticallyTeleport( CTFBot *actor ) const
{
	if ( actor->IsPlayerClass( TF_CLASS_ENGINEER ) )
	{
		return ( actor->GetObjectOfType( OBJ_TELEPORTER, TELEPORTER_TYPE_ENTRANCE ) != nullptr );
	}
	else
	{
		return !actor->IsPlayerClass( TF_CLASS_MEDIC );
	}
}
