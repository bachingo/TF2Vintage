#include "cbase.h"
#include "../../tf_bot.h"
#include "tf_bot_spy_infiltrate.h"
#include "tf_bot_spy_sap.h"
#include "tf_bot_spy_attack.h"
#include "../tf_bot_retreat_to_cover.h"
#include "tf_gamerules.h"
#include "tf_obj.h"
#include "nav_mesh/tf_nav_mesh.h"


ConVar tf_bot_debug_spy( "tf_bot_debug_spy", "0", FCVAR_CHEAT );


CTFBotSpyInfiltrate::CTFBotSpyInfiltrate()
{
}

CTFBotSpyInfiltrate::~CTFBotSpyInfiltrate()
{
}


const char *CTFBotSpyInfiltrate::GetName( void ) const
{
	return "SpyInfiltrate";
}


ActionResult<CTFBot> CTFBotSpyInfiltrate::OnStart( CTFBot *me, Action<CTFBot> *action )
{
	m_PathFollower.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	m_HidingArea = nullptr;
	m_bCloaked = false;

	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotSpyInfiltrate::Update( CTFBot *me, float dt )
{
	CBaseCombatWeapon *revolver = me->Weapon_GetWeaponByType( TF_WPN_TYPE_PRIMARY );
	if ( revolver )
		me->Weapon_Switch( revolver );

	CTFNavArea *area = me->GetLastKnownArea();
	if ( area == nullptr )
		return Action<CTFBot>::Continue();

	if ( !me->m_Shared.IsStealthed() && !area->HasTFAttributes( RED_SPAWN_ROOM|BLUE_SPAWN_ROOM|SPAWN_ROOM_EXIT ) && area->IsInCombat() && !m_bCloaked )
	{
		m_bCloaked = true;
		me->PressAltFireButton();
	}

	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	if ( threat && threat->GetEntity() && threat->GetEntity()->IsBaseObject() )
	{
		CBaseObject *obj = static_cast<CBaseObject *>( threat->GetEntity() );
		if ( !obj->HasSapper() && me->IsEnemy( obj ) )
			return Action<CTFBot>::SuspendFor( new CTFBotSpySap( obj ), "Sapping an enemy object" );
	}

	if ( me->GetTargetSentry() && !me->GetTargetSentry()->HasSapper() )
		return Action<CTFBot>::SuspendFor( new CTFBotSpySap( me->GetTargetSentry() ), "Sapping a Sentry" );

	if ( !m_HidingArea && m_findHidingAreaDelay.IsElapsed() )
	{
		FindHidingSpot( me );

		m_findHidingAreaDelay.Start( 3.0f );
	}

	if ( !TFGameRules()->InSetup() && threat && threat->GetTimeSinceLastKnown() < 3.0f )
	{
		CTFPlayer *victim = ToTFPlayer( threat->GetEntity() );
		if ( victim != nullptr )
		{
			CTFNavArea *victim_area = static_cast<CTFNavArea *>( victim->GetLastKnownArea() );
			if ( victim_area && victim_area->GetIncursionDistance( victim->GetTeamNumber() ) > area->GetIncursionDistance( victim->GetTeamNumber() ) )
			{
				if ( me->m_Shared.IsStealthed() )
					return Action<CTFBot>::SuspendFor( new CTFBotRetreatToCover( new CTFBotSpyAttack( victim ) ), "Hiding to decloak before going after a backstab victim" );
				else
					return Action<CTFBot>::SuspendFor( new CTFBotSpyAttack( victim ), "Going after a backstab victim" );
			}
		}
	}

	if ( m_HidingArea == nullptr )
		return Action<CTFBot>::Continue();

	if ( tf_bot_debug_spy.GetBool() )
	{
		m_HidingArea->DrawFilled( 255, 255, 0, 255 );
	}

	if ( m_HidingArea == area )
	{
		if ( TFGameRules()->InSetup() )
		{
			m_waitDuration.Start( RandomFloat( 0.0f, 5.0f ) );
		}
		else
		{
			if ( m_waitDuration.HasStarted() && m_waitDuration.IsElapsed() )
			{
				m_HidingArea = nullptr;
				return Action<CTFBot>::Continue();
			}
			else
			{
				m_waitDuration.Start( RandomFloat( 5.0f, 10.0f ) );
			}
		}
	}
	
	if ( !m_recomputePath.IsElapsed() )
	{
		m_PathFollower.Update( me );

		m_waitDuration.Invalidate();

		return Action<CTFBot>::Continue();
	}

	m_recomputePath.Start( RandomFloat( 1.0f, 2.0f ) );

	CTFBotPathCost func( me, SAFEST_ROUTE );
	m_PathFollower.Compute( me, m_HidingArea->GetCenter(), func );

	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotSpyInfiltrate::OnSuspend( CTFBot *me, Action<CTFBot> *newAction )
{
	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotSpyInfiltrate::OnResume( CTFBot *me, Action<CTFBot> *priorAction )
{
	m_recomputePath.Invalidate();

	return Action<CTFBot>::Continue();
}


EventDesiredResult<CTFBot> CTFBotSpyInfiltrate::OnStuck( CTFBot *me )
{
	m_HidingArea = nullptr;
	m_findHidingAreaDelay.Invalidate();

	return Action<CTFBot>::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotSpyInfiltrate::OnTerritoryCaptured( CTFBot *me, int territoryID )
{
	m_findHidingAreaDelay.Start( 5.0f );

	return Action<CTFBot>::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotSpyInfiltrate::OnTerritoryLost( CTFBot *me, int territoryID )
{
	m_findHidingAreaDelay.Start( 5.0f );

	return Action<CTFBot>::TryContinue();
}


QueryResultType CTFBotSpyInfiltrate::ShouldAttack( const INextBot *me, const CKnownEntity *threat ) const
{
	return ANSWER_NO;
}


bool CTFBotSpyInfiltrate::FindHidingSpot( CTFBot *actor )
{
	m_HidingArea = nullptr;

	if ( actor->GetAliveDuration() < 5.0f && TFGameRules()->InSetup() )
		return false;

	const CUtlVector<CTFNavArea *> &exits = TFNavMesh()->GetSpawnRoomExitsForTeam( GetEnemyTeam( actor ) );
	if ( exits.IsEmpty() )
	{
		if ( tf_bot_debug_spy.GetBool() )
			DevMsg( "%3.2f: No enemy spawn room exit areas found\n", gpGlobals->curtime );

		return false;
	}

	CUtlVector<CNavArea *> surrounding;
	FOR_EACH_VEC( exits, i )
	{
		CUtlVector<CNavArea *> temp;
		CollectSurroundingAreas( &temp, exits[i], 2500.0f );

		surrounding.AddVectorToTail( temp );
	}

	CUtlVector<CNavArea *> areas;
	FOR_EACH_VEC( surrounding, i )
	{
		if ( !actor->GetLocomotionInterface()->IsAreaTraversable( surrounding[i] ) )
			continue;

		bool visible = false;
		FOR_EACH_VEC( exits, j )
		{
			if ( surrounding[i]->IsPotentiallyVisible( exits[j] ) )
			{
				visible = true;
				break;
			}
		}

		if ( !visible )
			areas.AddToTail( surrounding[i] );
	}

	if ( areas.IsEmpty() )
	{
		if ( tf_bot_debug_spy.GetBool() )
		{
			DevMsg( "%3.2f: Can't find any non-visible hiding areas, "
					"trying for anything near the spawn exit...\n", gpGlobals->curtime );
		}

		FOR_EACH_VEC( surrounding, i )
		{
			if ( actor->GetLocomotionInterface()->IsAreaTraversable( surrounding[i] ) )
				areas.AddToTail( surrounding[i] );
		}

		if ( areas.IsEmpty() )
		{
			if ( tf_bot_debug_spy.GetBool() )
			{
				DevMsg( "%3.2f: Can't find any areas near the enemy spawn exit - "
						"just heading to the enemy spawn and hoping...\n", gpGlobals->curtime );
			}

			m_HidingArea = exits.Random();

			return false;
		}
	}

	m_HidingArea = static_cast<CTFNavArea *>( areas.Random() );

	return true;
}
