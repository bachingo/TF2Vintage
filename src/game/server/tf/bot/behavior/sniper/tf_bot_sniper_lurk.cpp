#include "cbase.h"
#include "tf_gamerules.h"
#include "team_control_point.h"
#include "nav_mesh/tf_nav_mesh.h"
#include "../../tf_bot.h"
#include "tf_bot_sniper_lurk.h"
#include "../tf_bot_melee_attack.h"


ConVar tf_bot_sniper_patience_duration( "tf_bot_sniper_patience_duration", "10", FCVAR_CHEAT, "How long a Sniper bot will wait without seeing an enemy before picking a new spot" );
ConVar tf_bot_sniper_target_linger_duration( "tf_bot_sniper_target_linger_duration", "2", FCVAR_CHEAT, "How long a Sniper bot will keep toward at a target it just lost sight of" );
ConVar tf_bot_sniper_allow_opportunistic( "tf_bot_sniper_allow_opportunistic", "1", FCVAR_NONE, "If set, Snipers will stop on their way to their preferred lurking spot to snipe at opportunistic targets" );
extern ConVar tf_bot_sniper_melee_range;


CTFBotSniperLurk::CTFBotSniperLurk()
{
}

CTFBotSniperLurk::~CTFBotSniperLurk()
{
}


const char *CTFBotSniperLurk::GetName() const
{
	return "SniperLurk";
}


ActionResult<CTFBot> CTFBotSniperLurk::OnStart( CTFBot *me, Action<CTFBot> *priorAction )
{
	m_patienceDuration.Start( RandomFloat( 0.9f, 1.1f ) * tf_bot_sniper_patience_duration.GetFloat() );

	m_vecHome = me->GetAbsOrigin();
	m_bHasHome = false;
	m_bNearHome = false;
	// TODO: dword @ 0x4820 = 0
	m_bOpportunistic = tf_bot_sniper_allow_opportunistic.GetBool();

	/*CBaseEntity *ent = nullptr;
	while ( ( ent = gEntList.FindEntityByClassname( ent, "func_tfbot_hint" ) ) != nullptr )
	{
		auto hint = static_cast<CTFBotHint *>( ent );

		// TODO: is m_hint an enum or something?
		if ( hint->m_hint == 0 )
		{
			m_Hints.AddToTail( hint );

			if ( actor->IsSelf( hint->GetOwnerEntity() ) )
			{
				hint->SetOwnerEntity( nullptr );
			}
		}
	}

	m_hHint = nullptr;*/

	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotSniperLurk::Update( CTFBot *me, float dt )
{
	me->AccumulateSniperSpots();

	if ( !m_bHasHome )
	{
		FindNewHome( me );
	}

	bool attacking = false;

	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	if ( threat != nullptr && threat->GetEntity()->IsAlive() &&
		 me->GetIntentionInterface()->ShouldAttack( me, threat ) )
	{
		if ( threat->IsVisibleInFOVNow() )
		{
		// dword @ 0x4820 = 0

			float dist_sqr = threat->GetLastKnownPosition().DistToSqr( me->GetAbsOrigin() );
			if ( dist_sqr >= Square( tf_bot_sniper_melee_range.GetFloat() ) )
			{
				return Action<CTFBot>::SuspendFor( new CTFBotMeleeAttack( 1.25f * tf_bot_sniper_melee_range.GetFloat() ),
														 "Melee attacking nearby threat" );
			}
		}

		if ( threat->GetTimeSinceLastSeen() < tf_bot_sniper_target_linger_duration.GetFloat() &&
			 me->IsLineOfFireClear( threat->GetEntity() ) )
		{
			if ( m_bOpportunistic )
			{
				CBaseCombatWeapon *primary = me->Weapon_GetSlot( 0 );
				if ( primary != nullptr )
				{
					me->Weapon_Switch( primary );
				}

				m_patienceDuration.Reset();

				attacking = true;

				if ( !m_bHasHome )
				{
					m_vecHome = me->GetAbsOrigin();

					m_patienceDuration.Start( RandomFloat( 0.9f, 1.1f ) * tf_bot_sniper_patience_duration.GetFloat() );

					attacking = true;
				}
			}
			else
			{
				attacking = false;

				CBaseCombatWeapon *secondary = me->Weapon_GetSlot( 1 );
				if ( secondary != nullptr )
				{
					me->Weapon_Switch( secondary );
				}
			}
		}
	}

	float dsqr_from_home = ( me->GetAbsOrigin().AsVector2D() - m_vecHome.AsVector2D() ).LengthSqr();
	m_bNearHome = ( dsqr_from_home < Square( 25.0f ) );

	if ( dsqr_from_home < Square( 25.0f ) )
	{
		m_bOpportunistic = tf_bot_sniper_allow_opportunistic.GetBool();

		if ( m_patienceDuration.IsElapsed() )
		{
		// increment int @ 0x4820

			if ( FindNewHome( me ) )
			{
				me->SpeakConceptIfAllowed( MP_CONCEPT_PLAYER_NEGATIVE );

				m_patienceDuration.Start( RandomFloat( 0.9f, 1.1f ) * tf_bot_sniper_patience_duration.GetFloat() );
			}
			else
			{
				m_patienceDuration.Start( 1.0f );
			}
		}
	}
	else
	{
		m_patienceDuration.Reset();

		if ( !attacking )
		{
			if ( m_recomputePathTimer.IsElapsed() )
			{
				m_recomputePathTimer.Start( RandomFloat( 1.0f, 2.0f ) );

				CTFBotPathCost cost( me, SAFEST_ROUTE );
				m_PathFollower.Compute( me, m_vecHome, cost );
			}

			m_PathFollower.Update( me );

			if ( me->m_Shared.InCond( TF_COND_ZOOMED ) )
			{
				me->PressAltFireButton();
			}

			return Action<CTFBot>::Continue();
		}
	}

	CBaseCombatWeapon *primary = me->Weapon_GetSlot( 0 );
	if ( primary != nullptr )
	{
		me->Weapon_Switch( primary );

		auto weapon = static_cast<CTFWeaponBase *>( primary );
		if ( !me->m_Shared.InCond( TF_COND_ZOOMED ) && !weapon->IsWeapon( TF_WEAPON_COMPOUND_BOW ) )
		{
			me->PressAltFireButton();
		}
	}

	return Action<CTFBot>::Continue();
}

void CTFBotSniperLurk::OnEnd( CTFBot *me, Action<CTFBot> *newAction )
{
	if ( me->m_Shared.InCond( TF_COND_ZOOMED ) )
	{
		me->PressAltFireButton();
	}

	/*if ( m_hHint )
	{
		m_hHint->SetOwnerEntity( nullptr );

		if ( tf_bot_debug_sniper.GetBool() )
		{
			DevMsg( "%3.2f: %s: Releasing hint.\n", gpGlobals->curtime, me->GetPlayerName() );
		}
	}*/
}

ActionResult<CTFBot> CTFBotSniperLurk::OnSuspend( CTFBot *me, Action<CTFBot> *newAction )
{
	if ( me->m_Shared.InCond( TF_COND_ZOOMED ) )
	{
		me->PressAltFireButton();
	}

	/*if ( m_hHint )
	{
		m_hHint->SetOwnerEntity( nullptr );

		if ( tf_bot_debug_sniper.GetBool() )
		{
			DevMsg( "%3.2f: %s: Releasing hint.\n", gpGlobals->curtime, me->GetPlayerName() );
		}
	}*/

	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotSniperLurk::OnResume( CTFBot *me, Action<CTFBot> *priorAction )
{
	m_recomputePathTimer.Invalidate();

	//m_hHint = nullptr;
	FindNewHome( me );

	return Action<CTFBot>::Continue();
}


QueryResultType CTFBotSniperLurk::ShouldRetreat( const INextBot *me ) const
{
	return ANSWER_UNDEFINED;
}

QueryResultType CTFBotSniperLurk::ShouldAttack( const INextBot *me, const CKnownEntity *threat ) const
{
	return ANSWER_YES;
}


/*CTFBotHint *CTFBotSniperLurk::FindHint( CTFBot *actor )
{
	// TODO
}*/

bool CTFBotSniperLurk::FindNewHome( CTFBot *actor )
{
	if ( !m_findHomeTimer.IsElapsed() )
		return false;

	m_findHomeTimer.Start( RandomFloat( 1.0f, 2.0f ) );

	/*if ( FindHint( actor ) )
		return true;*/

	if ( actor->m_sniperSpots.IsEmpty() )
	{
		m_bHasHome = false;

		CTeamControlPoint *pPoint = actor->GetMyControlPoint();
		if ( pPoint )
		{
			CCopyableUtlVector<CTFNavArea *> areas = *( (CCopyableUtlVector<CTFNavArea *> *)TFNavMesh()->GetControlPointAreas( pPoint->GetPointIndex() ) );
			if ( areas.IsEmpty() )
			{
				TFNavMesh()->CollectSpawnRoomThresholdAreas( &areas, actor->GetTeamNumber() );
				if ( areas.IsEmpty() )
				{
					m_vecHome = actor->GetAbsOrigin();
				}
				else
				{
					CTFNavArea *area = areas.Random();
					m_vecHome = area->GetRandomPoint();
				}
			}
			else
			{
				CTFNavArea *area = areas.Random();
				m_vecHome = area->GetRandomPoint();
			}
		}

		return false;
	}

	CTFBot::SniperSpotInfo const& info = actor->m_sniperSpots.Random();
	m_vecHome = info.m_vecHome;
	m_bHasHome = true;

	return true;
}
