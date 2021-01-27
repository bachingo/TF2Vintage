#include "cbase.h"
#include "../tf_bot.h"
#include "tf_gamerules.h"
#include "tf_obj.h"
#include "nav_mesh/tf_nav_area.h"
#include "tf_bot_get_health.h"

ConVar tf_bot_health_search_near_range( "tf_bot_health_search_near_range", "1000", FCVAR_CHEAT );
ConVar tf_bot_health_search_far_range( "tf_bot_health_search_far_range", "2000", FCVAR_CHEAT );
ConVar tf_bot_health_critical_ratio( "tf_bot_health_critical_ratio", "0.3", FCVAR_CHEAT );
ConVar tf_bot_health_ok_ratio( "tf_bot_health_ok_ratio", "0.8", FCVAR_CHEAT );


static CHandle<CBaseEntity> s_possibleHealth;
static CTFBot *s_possibleBot;
static int s_possibleFrame;


CTFBotGetHealth::CTFBotGetHealth()
{
}

CTFBotGetHealth::~CTFBotGetHealth()
{
}


const char *CTFBotGetHealth::GetName() const
{
	return "GetHealth";
}


ActionResult<CTFBot> CTFBotGetHealth::OnStart( CTFBot *me, Action<CTFBot> *priorAction )
{
	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	m_PathFollower.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	if ( ( gpGlobals->framecount != s_possibleFrame || s_possibleBot != me ) && ( !IsPossible( me ) || !s_possibleHealth ) )
		return Action<CTFBot>::Done( "Can't get health" );

	m_hHealth = s_possibleHealth;
	m_bUsingDispenser = m_hHealth->ClassMatches( "obj_dispenser*" );

	CTFBotPathCost cost( me, SAFEST_ROUTE );
	if ( !m_PathFollower.Compute( me, m_hHealth->WorldSpaceCenter(), cost ) )
		return Action<CTFBot>::Done( "No path to health" );

	if ( me->IsPlayerClass( TF_CLASS_SPY ) && me->m_Shared.IsStealthed() )
		me->PressAltFireButton();

	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotGetHealth::Update( CTFBot *me, float dt )
{
	if ( !m_hHealth || ( m_hHealth->GetFlags() & EF_NODRAW && !m_hHealth->ClassMatches( "func_regenerate" ) ) )
		return Action<CTFBot>::Done( "Health kit I was going for has been taken" );

	if ( me->GetMaxHealth() <= me->GetHealth() )
		return Action<CTFBot>::Done( "I've been healed" );

	CClosestTFPlayer functor( m_hHealth->WorldSpaceCenter() );
	ForEachPlayer( functor );
	if ( functor.m_pPlayer && !functor.m_pPlayer->InSameTeam( me ) )
		return Action<CTFBot>::Done( "An enemy is closer to it" );

	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	for ( int i=0; i<me->m_Shared.GetNumHealers(); ++i )
	{
		if ( !me->m_Shared.HealerIsDispenser( i ) )
			return Action<CTFBot>::Done( "A Medic is healing me" );

		if ( threat && threat->IsVisibleInFOVNow() )
			return Action<CTFBot>::Done( "No time to wait for health, I must fight" );
	}

	if ( !m_PathFollower.IsValid() )
		return Action<CTFBot>::Done( "My path became invalid" );

	me->EquipBestWeaponForThreat( threat );

	m_PathFollower.Update( me );

	return Action<CTFBot>::Continue();
}


EventDesiredResult<CTFBot> CTFBotGetHealth::OnMoveToSuccess( CTFBot *me, const Path *path )
{
	return Action<CTFBot>::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotGetHealth::OnMoveToFailure( CTFBot *me, const Path *path, MoveToFailureType reason )
{
	return Action<CTFBot>::TryDone( RESULT_CRITICAL, "Failed to reach health kit" );
}

EventDesiredResult<CTFBot> CTFBotGetHealth::OnStuck( CTFBot *me )
{
	return Action<CTFBot>::TryDone( RESULT_CRITICAL, "Stuck trying to reach health kit" );
}


QueryResultType CTFBotGetHealth::ShouldHurry( const INextBot *me ) const
{
	return ANSWER_YES;
}


bool CTFBotGetHealth::IsPossible( CTFBot *actor )
{
	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	if ( actor->m_Shared.GetNumHealers() > 0 || TFGameRules()->IsMannVsMachineMode() )
		return false;

	float flRatio = Clamp( ( ( (float)actor->GetHealth() / (float)actor->GetMaxHealth() ) - tf_bot_health_critical_ratio.GetFloat() ) /
						 ( tf_bot_health_ok_ratio.GetFloat() - tf_bot_health_critical_ratio.GetFloat() ), 0.0f, 1.0f );

	float flMinDist, flMaxDist;
	if ( actor->m_Shared.InCond( TF_COND_BURNING ) || actor->m_Shared.InCond( TF_COND_BLEEDING ) )
	{
		flMinDist = 0.0f;
		flMaxDist = tf_bot_health_search_far_range.GetFloat();
	}
	else
	{
		flMaxDist = tf_bot_health_search_far_range.GetFloat();
		flMinDist = flRatio * tf_bot_health_search_near_range.GetFloat() - flMaxDist;
	}

	CUtlVector<EHANDLE> healths;
	CHealthFilter filter( actor );
	actor->SelectReachableObjects( TFGameRules()->GetHealthEnts(), &healths, filter, actor->GetLastKnownArea(), flMinDist + flMaxDist );
	
	if ( healths.IsEmpty() )
	{
		if ( actor->IsDebugging( NEXTBOT_BEHAVIOR ) )
			Warning( "%3.2f: No health nearby\n", gpGlobals->curtime );

		return false;
	}

	CBaseEntity *pHealth = nullptr;
	FOR_EACH_VEC( healths, i )
	{
		if ( healths[i]->GetTeamNumber() == GetEnemyTeam( actor ) )
			continue;

		pHealth = healths[i];
	}

	if ( pHealth )
	{
		CTFBotPathCost func( actor, FASTEST_ROUTE );
		if ( !NavAreaBuildPath( actor->GetLastKnownArea(), NULL, &pHealth->WorldSpaceCenter(), func ) )
		{
			if ( actor->IsDebugging( NEXTBOT_BEHAVIOR ) )
				Warning( "%3.2f: No path to health!\n", gpGlobals->curtime );
		}

		s_possibleBot = actor;
		s_possibleHealth = pHealth;
		s_possibleFrame = gpGlobals->framecount;

		return true;
	}

	if ( actor->IsDebugging( NEXTBOT_BEHAVIOR ) )
		Warning( "%3.2f: No health available to my team nearby\n", gpGlobals->curtime );

	return false;
}


CHealthFilter::CHealthFilter( CTFPlayer *actor )
	: m_pActor( actor )
{
}


bool CHealthFilter::IsSelected( const CBaseEntity *ent ) const
{
	CClosestTFPlayer functor( ent->WorldSpaceCenter() );
	ForEachPlayer( functor );

	// Don't run into enemies while trying to scavenge
	if ( functor.m_pPlayer && !functor.m_pPlayer->InSameTeam( m_pActor ) )
		return false;

	CTFNavArea *pArea = static_cast<CTFNavArea *>( TheNavMesh->GetNearestNavArea( ent->WorldSpaceCenter() ) );
	if ( !pArea )
		return false;

	// Can't use enemy teams resupply cabinet
	if ( FClassnameIs( const_cast<CBaseEntity *>( ent ), "func_regenerate" ) )
	{
		if ( pArea->HasTFAttributes( BLUE_SPAWN_ROOM|RED_SPAWN_ROOM ) &&
			( ( m_pActor->GetTeamNumber() == TF_TEAM_RED && pArea->HasTFAttributes( BLUE_SPAWN_ROOM ) ) ||
			  ( m_pActor->GetTeamNumber() == TF_TEAM_BLUE && pArea->HasTFAttributes( RED_SPAWN_ROOM ) ) ) )
		{
			return false;
		}
	}

	if ( FClassnameIs( const_cast<CBaseEntity *>( ent ), "obj_dispenser*" ) )
	{
		CBaseObject *pObject = static_cast<CBaseObject *>( const_cast<CBaseEntity *>( ent ) );

		// Ignore non-functioning buildings
		if ( pObject->IsDisabled() || pObject->IsBuilding() || pObject->IsPlacing() || pObject->HasSapper() )
			return false;
	}

	// Any other entity or packs that have been picked up are a no go
	if ( FClassnameIs( const_cast<CBaseEntity *>( ent ), "item_healthkit*" ) && ent->GetFlags() & EF_NODRAW )
		return false;

	// Find minimum cost area we are currently searching
	if ( !pArea->IsMarked() || m_flMinCost < pArea->GetCostSoFar() )
		return false;

	const_cast<CHealthFilter *>( this )->m_flMinCost = pArea->GetCostSoFar();
	
	return true;
}
