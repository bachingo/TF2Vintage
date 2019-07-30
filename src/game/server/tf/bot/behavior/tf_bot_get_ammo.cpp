#include "cbase.h"
#include "../tf_bot.h"
#include "tf_gamerules.h"
#include "tf_obj.h"
#include "nav_mesh/tf_nav_mesh.h"
#include "tf_bot_get_ammo.h"


ConVar tf_bot_ammo_search_range( "tf_bot_ammo_search_range", "5000", FCVAR_CHEAT, "How far bots will search to find ammo around them" );
ConVar tf_bot_debug_ammo_scavanging( "tf_bot_debug_ammo_scavanging", "0", FCVAR_CHEAT );


static CHandle<CBaseEntity> s_possibleAmmo;
static CTFBot *s_possibleBot;
static int s_possibleFrame;


CTFBotGetAmmo::CTFBotGetAmmo()
{
	m_PathFollower.Invalidate();
	m_hAmmo = nullptr;
	m_bUsingDispenser = false;
}

CTFBotGetAmmo::~CTFBotGetAmmo()
{
}


const char *CTFBotGetAmmo::GetName() const
{
	return "GetAmmo";
}


ActionResult<CTFBot> CTFBotGetAmmo::OnStart( CTFBot *me, Action<CTFBot> *priorAction )
{
	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	m_PathFollower.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	if ( ( gpGlobals->framecount != s_possibleFrame || s_possibleBot != me ) && ( !IsPossible( me ) || !s_possibleAmmo ) )
		return Action<CTFBot>::Done( "Can't get ammo" );

	m_hAmmo = s_possibleAmmo;
	m_bUsingDispenser = s_possibleAmmo->ClassMatches( "obj_dispenser*" );

	CTFBotPathCost cost( me, FASTEST_ROUTE );
	if ( !m_PathFollower.Compute( me, m_hAmmo->WorldSpaceCenter(), cost ) )
		return Action<CTFBot>::Done( "No path to ammo" );

	if ( me->IsPlayerClass( TF_CLASS_SPY ) && me->m_Shared.IsStealthed() )
		me->PressAltFireButton();

	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotGetAmmo::Update( CTFBot *me, float dt )
{
	if ( me->IsAmmoFull() )
		return Action<CTFBot>::Done( "My ammo is full" );

	if ( !m_hAmmo )
		return Action<CTFBot>::Done( "Ammo I was going for has been taken" );

	if ( m_bUsingDispenser && 
		( me->GetAbsOrigin() - m_hAmmo->GetAbsOrigin() ).LengthSqr() < Square( 75.0f ) &&
		me->GetVisionInterface()->IsLineOfSightClearToEntity(m_hAmmo) )
	{
		if ( me->IsAmmoFull() )
			return Action<CTFBot>::Done( "Ammo refilled by the Dispenser" );

		if ( !me->IsAmmoLow() )
		{
			if ( me->GetVisionInterface()->GetPrimaryKnownThreat() )
				return Action<CTFBot>::Done( "No time to wait for more ammo, I must fight" );
		}
	}

	if ( !m_PathFollower.IsValid() )
		return Action<CTFBot>::Done( "My path became invalid" );

	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	me->EquipBestWeaponForThreat( threat );

	m_PathFollower.Update( me );

	return Action<CTFBot>::Continue();
}


EventDesiredResult<CTFBot> CTFBotGetAmmo::OnContact( CTFBot *me, CBaseEntity *other, CGameTrace *result )
{
	return Action<CTFBot>::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotGetAmmo::OnMoveToSuccess( CTFBot *me, const Path *path )
{
	return Action<CTFBot>::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotGetAmmo::OnMoveToFailure( CTFBot *me, const Path *path, MoveToFailureType fail )
{
	return Action<CTFBot>::TryDone( RESULT_CRITICAL, "Failed to reach ammo" );
}

EventDesiredResult<CTFBot> CTFBotGetAmmo::OnStuck( CTFBot *me )
{
	return Action<CTFBot>::TryDone( RESULT_CRITICAL, "Stuck trying to reach ammo" );
}


QueryResultType CTFBotGetAmmo::ShouldHurry( const INextBot *me ) const
{
	return ANSWER_YES;
}


bool CTFBotGetAmmo::IsPossible( CTFBot *actor )
{
	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	CUtlVector<EHANDLE> ammos;
	for ( CBaseEntity *pEntity = gEntList.FirstEnt(); pEntity; pEntity = gEntList.NextEnt( pEntity ) )
	{
		if ( pEntity->ClassMatches( "tf_ammo_pack" ) )
		{
			EHANDLE hndl( pEntity );
			ammos.AddToTail( hndl );
		}
	}

	ammos.AddVectorToTail( TFGameRules()->GetAmmoEnts() );

	CAmmoFilter filter( actor );
	actor->SelectReachableObjects( ammos, &ammos, filter, actor->GetLastKnownArea(), tf_bot_ammo_search_range.GetFloat() );

	if ( ammos.IsEmpty() )
	{
		if ( actor->IsDebugging( NEXTBOT_BEHAVIOR ) )
			Warning( "%3.2f: No ammo nearby.\n", gpGlobals->curtime );

		return false;
	}

	CBaseEntity *pAmmo = nullptr;
	for( int i=0; i<ammos.Count(); ++i )
	{
		if ( ammos[i]->GetTeamNumber() == GetEnemyTeam( actor ) )
			continue;

		pAmmo = ammos[i];

		if ( tf_bot_debug_ammo_scavanging.GetBool() )
			NDebugOverlay::Cross3D( pAmmo->WorldSpaceCenter(), 5.0f, 255, 100, 154, false, 1.0 );
	}

	if ( pAmmo )
	{
		CTFBotPathCost func( actor, FASTEST_ROUTE );
		if ( !NavAreaBuildPath( actor->GetLastKnownArea(), NULL, &pAmmo->WorldSpaceCenter(), func ) )
		{
			if ( actor->IsDebugging( NEXTBOT_BEHAVIOR ) )
				Warning( "%3.2f: No path to health!\n", gpGlobals->curtime );
		}

		s_possibleAmmo = pAmmo;
		s_possibleBot = actor;
		s_possibleFrame = gpGlobals->framecount;

		return true;
	}

	if ( actor->IsDebugging( NEXTBOT_BEHAVIOR ) )
		Warning( " %3.2f: No ammo nearby.\n", gpGlobals->curtime );

	return false;
}


CAmmoFilter::CAmmoFilter( CTFPlayer *actor )
	: m_pActor( actor )
{
	m_flMinCost = FLT_MAX;
}


bool CAmmoFilter::IsSelected( const CBaseEntity *ent ) const
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
		if ( m_pActor->GetTeamNumber() == TF_TEAM_RED && pArea->HasTFAttributes( BLUE_SPAWN_ROOM ) )
			return false;

		if ( m_pActor->GetTeamNumber() == TF_TEAM_BLUE && pArea->HasTFAttributes( RED_SPAWN_ROOM ) )
			return false;
	}

	if ( FClassnameIs( const_cast<CBaseEntity *>( ent ), "obj_dispenser*" ) )
	{
		CBaseObject *pObject = static_cast<CBaseObject *>( const_cast<CBaseEntity *>( ent ) );

		// Don't try to syphon metal from an unupgraded dispenser if we're trying to setup
		if ( m_pActor->IsPlayerClass( TF_CLASS_ENGINEER ) &&
			 pObject->GetUpgradeLevel() <= 2 &&
			 m_pActor->GetObjectOfType( OBJ_SENTRYGUN, OBJECT_MODE_NONE ) != NULL )
		{
			return false;
		}

		// Ignore non-functioning buildings
		if ( pObject->IsDisabled() || pObject->IsPlacing() || pObject->HasSapper() )
			return false;
	}

	// Any other entity or packs that have been picked up are a no go
	if ( ( FClassnameIs( const_cast<CBaseEntity *>( ent ), "tf_ammo_pack" ) || FClassnameIs( const_cast<CBaseEntity *>( ent ), "item_ammopack*" ) ) && ent->GetFlags() & EF_NODRAW )
		return false;

	// Find minimum cost area we are currently searching
	if ( !pArea->IsMarked() || m_flMinCost < pArea->GetCostSoFar() )
		return false;

	const_cast<CAmmoFilter *>( this )->m_flMinCost = pArea->GetCostSoFar();

	return true;
}
