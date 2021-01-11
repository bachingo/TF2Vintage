//========= Copyright � Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "tf_bot.h"
#include "tf_gamerules.h"
#include "tf_obj_sentrygun.h"
#include "tf_bot_destroy_enemy_sentry.h"
#include "nav_mesh/tf_nav_mesh.h"
#include "tf_bot_retreat_to_cover.h"
#include "demoman/tf_bot_stickybomb_sentrygun.h"

ConVar tf_bot_debug_destroy_enemy_sentry( "tf_bot_debug_destroy_enemy_sentry", "0", FCVAR_CHEAT );
ConVar tf_bot_max_grenade_launch_at_sentry_range( "tf_bot_max_grenade_launch_at_sentry_range", "1500", FCVAR_CHEAT );
ConVar tf_bot_max_sticky_launch_at_sentry_range( "tf_bot_max_sticky_launch_at_sentry_range", "1500", FCVAR_CHEAT );


class FindSafeSentryApproachAreaScan : public ISearchSurroundingAreasFunctor
{
public:
	virtual ~FindSafeSentryApproachAreaScan() {};

	virtual bool operator()( CNavArea *area, CNavArea *priorArea, float travelDistanceSoFar ) OVERRIDE;
	virtual bool ShouldSearch( CNavArea *adjArea, CNavArea *currentArea, float travelDistanceSoFar ) OVERRIDE;
	virtual void PostSearch() OVERRIDE;

	CTFBot *m_pActor;
	CUtlVector<CTFNavArea *> m_Areas;
	bool m_bInDangerousArea; // TODO: More representitive name
};
bool FindSafeSentryApproachAreaScan::operator()( CNavArea *area, CNavArea *priorArea, float travelDistanceSoFar )
{
	CTFNavArea *pTFArea = (CTFNavArea *)area;
	bool bContinue;

	if ( !m_bInDangerousArea )
	{
		if ( !pTFArea->IsTFMarked() || !priorArea )
			return true;

		m_Areas.AddToTail( (CTFNavArea *)priorArea );
		bContinue = true;
	}
	else
	{
		if ( pTFArea->IsTFMarked() )
			return true;

		m_Areas.AddToTail( pTFArea );
		bContinue = false;
	}

	return bContinue;
}

bool FindSafeSentryApproachAreaScan::ShouldSearch( CNavArea *adjArea, CNavArea *currentArea, float travelDistanceSoFar )
{
	if ( !m_bInDangerousArea && ( (CTFNavArea *)currentArea )->IsTFMarked() )
		return false; 

	return m_pActor->GetLocomotionInterface()->IsAreaTraversable( adjArea );
}

void FindSafeSentryApproachAreaScan::PostSearch()
{
	if ( tf_bot_debug_destroy_enemy_sentry.GetBool() )
	{
		FOR_EACH_VEC( m_Areas, i ) {
			m_Areas[ i ]->DrawFilled( 0x00, 0xff, 0x00, 0xff, 60.0f, true, 5.0f );
		}
	}
}


const char *CTFBotDestroyEnemySentry::GetName() const
{
	return "DestroyEnemySentry";
}


ActionResult<CTFBot> CTFBotDestroyEnemySentry::OnStart( CTFBot *me, Action<CTFBot> *priorAction )
{
	m_PathFollower.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );
	m_PathFollower.Invalidate();

	m_bAtSafeSpot = false;
	m_bUbered = false;

	m_recomputePathTimer.Invalidate();

	if ( me->IsPlayerClass( TF_CLASS_DEMOMAN ) )
		ComputeCornerAttackSpot( me );
	else
		ComputeSafeAttackSpot( me );

	m_hSentry = me->GetTargetSentry();

	return BaseClass::Continue();
}

ActionResult<CTFBot> CTFBotDestroyEnemySentry::Update( CTFBot *me, float dt )
{
	if ( !me->GetTargetSentry() )
		return BaseClass::Done( "Enemy sentry is destroyed." );

	if ( me->GetTargetSentry() != m_hSentry )
		return BaseClass::ChangeTo( new CTFBotDestroyEnemySentry, "Changed sentry target." );

	if ( me->m_Shared.IsInvulnerable() )
	{
		if ( !m_bUbered )
		{
			CTFBotPathCost func( me, FASTEST_ROUTE );
			CNavArea *pSentryArea = m_hSentry->GetLastKnownArea();
			CNavArea *pMyArea = me->GetLastKnownArea();

			m_bUbered = true;

			if ( NavAreaTravelDistance( pMyArea, pSentryArea, func, 500.0f ) > 0 )
				return BaseClass::SuspendFor( new CTFBotUberAttackEnemySentry( m_hSentry ), "Go get it!" );
		}
	}
	else
	{
		m_bUbered = false;
	}

	if ( !( me->m_nBotAttrs & CTFBot::AttributeType::IGNOREENEMIES ) )
	{
		const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
		if ( threat && threat->IsVisibleInFOVNow() )
		{
			float flDistToThreat = me->GetRangeSquaredTo( threat->GetEntity() );
			float flDistToTarget = me->GetRangeSquaredTo( m_hSentry );

			// Isn't this backwards?
			if ( ( flDistToTarget / 2 ) > flDistToThreat )
				return BaseClass::Done( "Enemy nearby." );
		}
	}

	bool bOppertunistic = false;
	if ( m_hSentry->GetTimeSinceLastFired() < 1.0f )
	{
		Vector vecFwd;
		AngleVectors( m_hSentry->GetTurretAngles(), &vecFwd );

		Vector vecToActor = me->GetAbsOrigin() - m_hSentry->GetAbsOrigin();
		vecToActor.NormalizeInPlace();

		if ( vecToActor.Dot( vecFwd ) > 0.8f )
			bOppertunistic = true;
	}

	if ( me->IsPlayerClass( TF_CLASS_DEMOMAN ) )
	{
		Vector vecSpot = m_vecAttackSpot;
		if ( !m_bFoundAttackSpot )
			vecSpot = m_hSentry->GetAbsOrigin();

		if ( !m_PathFollower.IsValid() )
		{
			CTFBotPathCost func( me, SAFEST_ROUTE );
			m_PathFollower.Compute( me, vecSpot, func );
			m_recomputePathTimer.Start( 1.0f );

			return BaseClass::Continue();
		}

		if ( m_recomputePathTimer.IsElapsed() )
		{
			CTFBotPathCost func( me, SAFEST_ROUTE );
			m_PathFollower.Compute( me, vecSpot, func );

			m_recomputePathTimer.Reset();

			return BaseClass::Continue();
		}

		if ( bOppertunistic )
		{
			me->EquipLongRangeWeapon();
			me->PressFireButton();
		}
		else
		{
			float flPitch, flYaw, flCharge;
			if ( FindStickybombAim( me, m_hSentry, &flYaw, &flPitch, &flCharge ) )
				return BaseClass::ChangeTo( new CTFBotStickybombSentrygun( m_hSentry, flPitch, flYaw, flCharge ), "Destroying sentry with opportunistic sticky shot." );
		}

		if ( m_bWalkToSpot )
			m_PathFollower.Update( me );

		float flDistToSpot = ( me->GetAbsOrigin() - m_vecAttackSpot ).LengthSqr();
		if ( me->IsLineOfFireClear( m_vecAttackSpot ) && ( flDistToSpot < Square( 25.0f ) || me->IsLineOfFireClear( m_hSentry ) ) )
		{
			if ( me->IsRangeLessThan( m_hSentry, 1000.0f ) )
				return BaseClass::ChangeTo( new CTFBotStickybombSentrygun( m_hSentry ), "Destroying sentry with stickies." );
		}

		if ( !me->IsRangeLessThan( vecSpot, 200.0f ) )
			m_bWalkToSpot = true;

		return BaseClass::Continue();
	}

	if ( !m_bFoundAttackSpot || !me->IsRangeLessThan( m_vecAttackSpot, 20.0f ) )
	{
		if ( !me->IsLineOfFireClear( m_hSentry ) )
		{
			if ( !m_PathFollower.IsValid() || m_recomputePathTimer.IsElapsed() )
			{
				CTFBotPathCost func( me, SAFEST_ROUTE );
				Vector vecSpot = m_bFoundAttackSpot ? m_vecAttackSpot : m_hSentry->GetAbsOrigin();
				if ( !m_PathFollower.Compute( me, vecSpot, func ) )
					return BaseClass::Done( "No path!" );

				m_recomputePathTimer.Start( 1.0f );
			}

			m_PathFollower.Update( me );

			return BaseClass::Continue();
		}
	}

	me->GetBodyInterface()->AimHeadTowards( me->GetTargetSentry() );

	Vector vecToSentry = me->EyePosition() - me->GetTargetSentry()->WorldSpaceCenter();
	vecToSentry.NormalizeInPlace();

	Vector vecFwd;
	me->EyeVectors( &vecFwd );

	if ( vecFwd.Dot( vecToSentry ) > 0.95f )
	{
		if ( !me->EquipLongRangeWeapon() )
			return BaseClass::SuspendFor( new CTFBotRetreatToCover( 0.1f ), "No suitable range weapon available right now" );

		me->PressFireButton();
		m_bAtSafeSpot = true;
	}
	else
	{
		m_bAtSafeSpot = false;
	}

	if( me->IsRangeGreaterThan( me->GetTargetSentry(), SENTRYGUN_BASE_RANGE * 1.1f ) )
		return BaseClass::Continue();

	if ( m_hSentry->GetTimeSinceLastFired() < 1.0f )
	{
		Vector vecFwd;
		AngleVectors( m_hSentry->GetTurretAngles(), &vecFwd );

		Vector vecToActor = me->GetAbsOrigin() - m_hSentry->GetAbsOrigin();
		vecToActor.NormalizeInPlace();

		if ( vecToActor.Dot( vecFwd ) > 0.8f )
			return BaseClass::SuspendFor( new CTFBotRetreatToCover( 0.1f ), "Taking cover from sentry fire" );
	}

	if ( m_bFoundAttackSpot && me->IsRangeLessThan( m_vecAttackSpot, 20.0f ) )
		return BaseClass::Continue();

	if ( !m_PathFollower.IsValid() || m_recomputePathTimer.IsElapsed() )
	{
		CTFBotPathCost func( me, SAFEST_ROUTE );
		Vector vecSpot = m_bFoundAttackSpot ? m_vecAttackSpot : m_hSentry->GetAbsOrigin();
		if ( !m_PathFollower.Compute( me, vecSpot, func ) )
			return BaseClass::Done( "No path!" );

		m_recomputePathTimer.Start( 1.0f );
	}

	m_PathFollower.Update( me );

	return BaseClass::Continue();
}

ActionResult<CTFBot> CTFBotDestroyEnemySentry::OnResume( CTFBot *me, Action<CTFBot> *priorAction )
{
	m_PathFollower.Invalidate();
	m_recomputePathTimer.Invalidate();

	if ( me->IsPlayerClass( TF_CLASS_DEMOMAN ) )
		ComputeCornerAttackSpot( me );
	else
		ComputeSafeAttackSpot( me );

	return BaseClass::Continue();
}


QueryResultType CTFBotDestroyEnemySentry::ShouldHurry( const INextBot *me ) const
{
	if ( m_bAtSafeSpot )
	{
		return ANSWER_YES;
	}

	return ANSWER_UNDEFINED;
}

QueryResultType CTFBotDestroyEnemySentry::ShouldRetreat( const INextBot *me ) const
{
	return ANSWER_NO;
}

QueryResultType CTFBotDestroyEnemySentry::ShouldAttack( const INextBot *me, const CKnownEntity *threat ) const
{
	if ( m_bAtSafeSpot )
	{
		return ANSWER_NO;
	}

	sizeof( Path );
	return ANSWER_UNDEFINED;
}


bool CTFBotDestroyEnemySentry::IsPossible( CTFBot *actor )
{
	if ( actor->GetAmmoCount( TF_AMMO_PRIMARY ) <= 0 || actor->GetAmmoCount( TF_AMMO_SECONDARY ) <= 0 )
		return false;

	if ( !actor->Weapon_OwnsThisID( TF_WEAPON_ROCKETLAUNCHER ) || !actor->Weapon_OwnsThisID( TF_WEAPON_GRENADELAUNCHER ) || !actor->Weapon_OwnsThisID( TF_WEAPON_PIPEBOMBLAUNCHER ) )
		return false;

	return true;
}


void CTFBotDestroyEnemySentry::ComputeCornerAttackSpot( CTFBot *actor )
{
	m_bFoundAttackSpot = false;
	m_vecAttackSpot = vec3_origin;

	if ( !actor->GetTargetSentry() || actor->GetTargetSentry() != m_hSentry )
		return;

	m_hSentry->UpdateLastKnownArea();
	CNavArea *pSentryArea = m_hSentry->GetLastKnownArea();
	if ( pSentryArea == nullptr )
		return;

	NavAreaCollector func;
	pSentryArea->ForAllPotentiallyVisibleAreas( func );

	CTFNavArea::MakeNewTFMarker();

	FOR_EACH_VEC( func.m_area, i )
	{
		CTFNavArea *pArea = (CTFNavArea *)func.m_area[i];

		Vector vecClosest;
		pArea->GetClosestPointOnArea( m_hSentry->GetAbsOrigin(), &vecClosest );

		if ( ( m_hSentry->GetAbsOrigin() - vecClosest ).LengthSqr() >= Square( SENTRYGUN_BASE_RANGE ) )
			continue;

		pArea->TFMark();

		if ( tf_bot_debug_destroy_enemy_sentry.GetBool() )
			pArea->DrawFilled( 0xFF, 0, 0, 0xFF, 60.0f );
	}

	FindSafeSentryApproachAreaScan scan;
	scan.m_pActor = actor;
	scan.m_bInDangerousArea = false;
	if ( actor->GetLastKnownArea() && actor->GetLastKnownArea()->IsTFMarked() )
		scan.m_bInDangerousArea = true;

	SearchSurroundingAreas( actor->GetLastKnownArea(), scan );

	if ( !scan.m_Areas.IsEmpty() )
	{
		CTFNavArea *pArea = scan.m_Areas.Random();
		for ( int i=0; i < 25; ++i )
		{
			m_vecAttackSpot = pArea->GetRandomPoint();

			if ( ( m_vecAttackSpot - m_hSentry->WorldSpaceCenter() ).LengthSqr() > Square( SENTRYGUN_BASE_RANGE ) )
				break;

			if ( !actor->IsLineOfFireClear( m_hSentry->WorldSpaceCenter(), m_vecAttackSpot ) )
				break;
		}

		m_bFoundAttackSpot = true;

		if ( tf_bot_debug_destroy_enemy_sentry.GetBool() )
			NDebugOverlay::Cross3D( m_vecAttackSpot, 5.0f, 0xFF, 0, 0, true, 60.0f );
	}
}

void CTFBotDestroyEnemySentry::ComputeSafeAttackSpot( CTFBot *actor )
{
	m_bFoundAttackSpot = false;

	if ( !actor->GetTargetSentry() || actor->GetTargetSentry() != m_hSentry )
		return;

	m_hSentry->UpdateLastKnownArea();
	CNavArea *pSentryArea = m_hSentry->GetLastKnownArea();
	if ( pSentryArea == nullptr )
		return;

	NavAreaCollector func;
	pSentryArea->ForAllPotentiallyVisibleAreas( func );

	CUtlVector<CTFNavArea *> safeAreas;
	if ( func.m_area.IsEmpty() )
	{
		m_bFoundAttackSpot = false;
		return;
	}

	FOR_EACH_VEC( func.m_area, i )
	{
		CNavArea *pArea = func.m_area[i];

		Vector vecClosest;
		pArea->GetClosestPointOnArea( ( pArea->GetCenter() + pArea->GetCenter() ) - pSentryArea->GetCenter(), &vecClosest );

		if ( ( vecClosest - m_hSentry->GetAbsOrigin() ).LengthSqr() <= Square( SENTRYGUN_BASE_RANGE ) )
			continue;

		safeAreas.AddToTail( (CTFNavArea *)pArea );

		if ( tf_bot_debug_destroy_enemy_sentry.GetBool() )
			pArea->DrawFilled( 0, 0xFF, 0, 0xFF, 60.0f, true, 1.0f );
	}

	CUtlVector<CTFNavArea *> validAreas;
	FOR_EACH_VEC( safeAreas, i )
	{
		CNavArea *pArea = safeAreas[i];
		
		Vector vecClosest;
		pArea->GetClosestPointOnArea( pSentryArea->GetCenter(), &vecClosest );

		if ( ( vecClosest - m_hSentry->GetAbsOrigin() ).LengthSqr() >= Square( 1650.0f ) )
			continue;

		safeAreas.AddToTail( (CTFNavArea *)pArea );

		if ( tf_bot_debug_destroy_enemy_sentry.GetBool() )
			pArea->DrawFilled( 100, 0xFF, 0, 0xFF, 60.0f, true, 5.0f );
	}

	CTFNavArea *pArea = validAreas.IsEmpty() ? safeAreas.Random() : validAreas.Random();
	m_vecAttackSpot = pArea->GetRandomPoint();
	m_bFoundAttackSpot = true;

	if ( tf_bot_debug_destroy_enemy_sentry.GetBool() )
	{
		pArea->DrawFilled( 0xFF, 0xFF, 0, 0xFF, 60.0f );
		NDebugOverlay::Cross3D( m_vecAttackSpot, 10.0f, 0xFF, 0, 0, true, 60.0f );
	}
}


CTFBotUberAttackEnemySentry::CTFBotUberAttackEnemySentry( CObjectSentrygun *sentry ) :
	m_hSentry( sentry )
{
}

CTFBotUberAttackEnemySentry::~CTFBotUberAttackEnemySentry()
{
}


const char *CTFBotUberAttackEnemySentry::GetName() const
{
	return "UberAttackEnemySentry";
}


ActionResult<CTFBot> CTFBotUberAttackEnemySentry::OnStart( CTFBot *me, Action<CTFBot> *priorAction )
{
	m_bSavedIgnoreEnemies = !!( me->m_nBotAttrs & CTFBot::AttributeType::IGNOREENEMIES );
	me->m_nBotAttrs |= CTFBot::AttributeType::IGNOREENEMIES;

	return BaseClass::Continue();
}

ActionResult<CTFBot> CTFBotUberAttackEnemySentry::Update( CTFBot *me, float dt )
{
	if ( !me->m_Shared.InCond( TF_COND_INVULNERABLE ) )
		return BaseClass::Done( "No longer uber" );

	if ( !m_hSentry )
		return BaseClass::Done( "Target sentry destroyed" );

	if ( me->IsPlayerClass( TF_CLASS_DEMOMAN ) )
	{
		float flPitch, flYaw;
		if ( FindGrenadeAim( me, m_hSentry, &flYaw, &flPitch ) )
		{
			QAngle angAim( flPitch, flYaw, 0.0f );
			Vector vecDir;
			AngleVectors( angAim, &vecDir );

			me->GetBodyInterface()->AimHeadTowards( me->EyePosition() + vecDir * 5000.0f, IBody::BORING, 0.3f, nullptr, "Aiming at opportunistic grenade shot" );

			Vector vecFwd;
			me->EyeVectors( &vecFwd );

			if ( vecFwd.Dot( vecDir ) > 0.9f )
			{
				if ( !me->EquipLongRangeWeapon() )
					return BaseClass::SuspendFor( new CTFBotRetreatToCover( 0.1f ), "No suitable range weapon available right now" );

				me->PressFireButton();
			}

			if ( !m_PathFollower.IsValid() || m_recomputePathTimer.IsElapsed() )
			{
				CTFBotPathCost func( me, FASTEST_ROUTE );
				m_PathFollower.Compute( me, m_hSentry, func );
				m_recomputePathTimer.Start( 1.0f );
			}

			m_PathFollower.Update( me );

			return BaseClass::Continue();
		}
	}

	if ( me->IsLineOfFireClear( m_hSentry ) )
	{
		me->GetBodyInterface()->AimHeadTowards( m_hSentry );

		Vector vecToSentry = me->EyePosition() - me->GetTargetSentry()->WorldSpaceCenter();
		vecToSentry.NormalizeInPlace();

		Vector vecFwd;
		me->EyeVectors( &vecFwd );

		if ( vecFwd.Dot( vecToSentry ) > 0.95f )
		{
			if ( !me->EquipLongRangeWeapon() )
				return BaseClass::SuspendFor( new CTFBotRetreatToCover( 0.1f ), "No suitable range weapon available right now" );

			me->PressFireButton();
		}

		if( me->IsRangeLessThan( me->GetTargetSentry(), 100.0f ) )
			return BaseClass::Continue();
	}

	if ( !m_PathFollower.IsValid() || m_recomputePathTimer.IsElapsed() )
	{
		CTFBotPathCost func( me, FASTEST_ROUTE );
		m_PathFollower.Compute( me, m_hSentry, func );
		m_recomputePathTimer.Start( 1.0f );
	}

	m_PathFollower.Update( me );

	return BaseClass::Continue();
}

void CTFBotUberAttackEnemySentry::OnEnd( CTFBot *me, Action<CTFBot> *newAction )
{
	if ( !m_bSavedIgnoreEnemies )
	{
		me->m_nBotAttrs &= ~CTFBot::AttributeType::IGNOREENEMIES;
	}
}


QueryResultType CTFBotUberAttackEnemySentry::ShouldHurry( const INextBot *me ) const
{
	return ANSWER_YES;
}

QueryResultType CTFBotUberAttackEnemySentry::ShouldRetreat( const INextBot *me ) const
{
	return ANSWER_NO;
}

QueryResultType CTFBotUberAttackEnemySentry::ShouldAttack( const INextBot *me, const CKnownEntity *threat ) const
{
	return ANSWER_YES;
}


bool FindGrenadeAim( CTFBot *actor, CBaseEntity *pTarget, float *pYaw, float *pPitch )
{
	bool success = false;

	Vector delta = ( pTarget->WorldSpaceCenter() - actor->EyePosition() );

	if ( delta.LengthSqr() <= Square( tf_bot_max_grenade_launch_at_sentry_range.GetFloat() ) )
	{
		QAngle dir;
		VectorAngles( delta, dir );

		float yaw   = actor->EyeAngles().y;
		float pitch = actor->EyeAngles().x;

		for ( int i = 10; i; --i )
		{
			Vector est = actor->EstimateProjectileImpactPosition( pitch, yaw, 900.0f );

			if ( ( pTarget->WorldSpaceCenter() - est ).LengthSqr() < Square( 75.0f ) )
			{
				NextBotTraceFilterIgnoreActors filter( pTarget, COLLISION_GROUP_NONE );

				trace_t tr;
				UTIL_TraceLine( pTarget->WorldSpaceCenter(), est, MASK_SOLID_BRUSHONLY, &filter, &tr );

				if ( !tr.DidHit() )
				{
					*pYaw   = yaw;
					*pPitch = pitch;

					success = true;
					break;
				}
			}

			yaw   = RandomFloat( -30.0f, 30.0f ) + dir.y;
			pitch = RandomFloat( -85.0f, 85.0f );
		}
	}

	return success;
}

bool FindStickybombAim( CTFBot *actor, CBaseEntity *pTarget, float *pYaw, float *pPitch, float *pCharge )
{
	bool success = false;

	Vector delta = ( pTarget->WorldSpaceCenter() - actor->EyePosition() );

	if ( delta.LengthSqr() <= Square( tf_bot_max_sticky_launch_at_sentry_range.GetFloat() ) )
	{
		QAngle dir;
		VectorAngles( delta, dir );

		float yaw   = actor->EyeAngles().y;
		float pitch = actor->EyeAngles().x;
		*pCharge = 1.0f;

		for ( int i = 100; i; --i )
		{
			Vector est = actor->EstimateStickybombProjectileImpactPosition( pitch, yaw, 0.0f );

			if ( ( pTarget->WorldSpaceCenter() - est ).LengthSqr() < Square( 75.0f ) )
			{
				NextBotTraceFilterIgnoreActors filter( pTarget, COLLISION_GROUP_NONE );

				trace_t tr;
				UTIL_TraceLine( pTarget->WorldSpaceCenter(), est, MASK_SOLID_BRUSHONLY, &filter, &tr );

				if ( !tr.DidHit() && *pCharge > 0.0f )
				{
					success = true;

					*pCharge = 0.0f;
					*pYaw    = yaw;
					*pPitch  = pitch;

					/* huh? */
					if ( *pCharge < 0.01 )
					{
						break;
					}
				}
			}

			yaw   = RandomFloat( -30.0f, 30.0f ) + dir.y;
			pitch = RandomFloat( -85.0f, 85.0f );
		}
	}

	return success;
}
