#include "cbase.h"
#include "tf_bot.h"
#include "tf_obj.h"
#include "tf_weapon_builder.h"
#include "tf_bot_engineer_build_teleport_exit.h"
#include "../tf_bot_get_ammo.h"


CTFBotEngineerBuildTeleportExit::CTFBotEngineerBuildTeleportExit()
{
	m_bSpotPreDetermined = false;
}

CTFBotEngineerBuildTeleportExit::CTFBotEngineerBuildTeleportExit( const Vector& vecSpot, float yaw )
{
	m_bSpotPreDetermined = true;
	m_vecBuildSpot = vecSpot;
	m_flBuildYaw = yaw;
}

CTFBotEngineerBuildTeleportExit::~CTFBotEngineerBuildTeleportExit()
{
}


const char *CTFBotEngineerBuildTeleportExit::GetName() const
{
	return "EngineerBuildTeleportExit";
}


ActionResult<CTFBot> CTFBotEngineerBuildTeleportExit::OnStart( CTFBot *me, Action<CTFBot> *priorAction )
{
	if ( !m_bSpotPreDetermined )
		m_vecBuildSpot = me->GetAbsOrigin();

	m_attemptBuildDuration.Start( 3.1f );
	m_PathFollower.Invalidate();

	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotEngineerBuildTeleportExit::Update( CTFBot *me, float dt )
{
	if ( me->GetObjectOfType( OBJ_TELEPORTER, TELEPORTER_TYPE_EXIT ) )
		return Action<CTFBot>::Done( "Teleport exit built" );

	if ( me->GetTimeSinceLastInjury() < 1.0f )
		return Action<CTFBot>::Done( "Ouch! I'm under attack" );

	if ( me->CanBuild( OBJ_TELEPORTER, TELEPORTER_TYPE_EXIT ) == CB_NEED_RESOURCES )
	{
		if ( m_fetchAmmoTimer.IsElapsed() && CTFBotGetAmmo::IsPossible( me ) )
		{
			m_fetchAmmoTimer.Start( 1.0f );
			return Action<CTFBot>::SuspendFor( new CTFBotGetAmmo, "Need more metal to build my Teleporter Exit" );
		}
	}

	if ( me->IsRangeGreaterThan( m_vecBuildSpot, 50.0f ) )
	{
		if ( m_recomputePathTimer.IsElapsed() )
		{
			CTFBotPathCost cost( me, FASTEST_ROUTE );
			m_PathFollower.Compute( me, m_vecBuildSpot, cost );

			m_recomputePathTimer.Start( RandomFloat( 2.0f, 3.0f ) );
		}

		if ( m_PathFollower.IsValid() )
		{
			m_PathFollower.Update( me );
			m_attemptBuildDuration.Reset();

			return Action<CTFBot>::Continue();
		}
	}

	if ( m_attemptBuildDuration.IsElapsed() )
		return Action<CTFBot>::Done( "Taking too long - giving up" );

	if ( m_bSpotPreDetermined )
	{
		me->GetBodyInterface()->AimHeadTowards( m_vecBuildSpot, IBody::CRITICAL, 1.0f, nullptr, "Looking toward my precise build location" );

		CBaseObject *pTele = (CBaseObject *)CreateEntityByName( "obj_teleporter" );
		if ( pTele )
		{
			pTele->SetAbsOrigin( m_vecBuildSpot );
			pTele->SetAbsAngles( QAngle( 0, m_flBuildYaw, 0 ) );

			pTele->SetObjectMode( TELEPORTER_TYPE_EXIT );
			pTele->Spawn();
			pTele->StartPlacement( me );
			pTele->StartBuilding( me );
			pTele->SetBuilder( me );

			const float flStepHeight = me->GetLocomotionInterface()->GetStepHeight();
			me->SetAbsOrigin( pTele->GetAbsOrigin() + Vector( 0, 0, flStepHeight ) );

			return Action<CTFBot>::Done( "Teleport exit built at precise location" );
		}
	}
	else
	{
		CTFWeaponBuilder *pBuilder = dynamic_cast<CTFWeaponBuilder *>( me->GetActiveTFWeapon() );
		if ( pBuilder && pBuilder->m_hObjectBeingBuilt )
		{
			if ( pBuilder->IsValidPlacement() )
			{
				me->PressFireButton();
			}
			else if ( m_retryTimer.IsElapsed() )
			{
				m_retryTimer.Start( 1.0f );

				float exp = RandomFloat( -M_PI, M_PI );

				float flSin, flCos;
				FastSinCos( exp, &flSin, &flCos );

				Vector vecDir( flCos * 100.0f, flSin * 100.0f, 0.0f );
				me->GetBodyInterface()->AimHeadTowards( me->EyePosition() - vecDir, IBody::CRITICAL, 1.0f, nullptr, "Trying to place my teleport exit" );
			}
		}
		else
		{
			me->StartBuildingObjectOfType( OBJ_TELEPORTER, TELEPORTER_TYPE_EXIT );
		}
	}

	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotEngineerBuildTeleportExit::OnResume( CTFBot *me, Action<CTFBot> *priorAction )
{
	m_attemptBuildDuration.Reset();
	m_PathFollower.Invalidate();

	return Action<CTFBot>::Continue();
}


EventDesiredResult<CTFBot> CTFBotEngineerBuildTeleportExit::OnStuck( CTFBot *me )
{
	m_PathFollower.Invalidate();

	return Action<CTFBot>::TryContinue();
}
